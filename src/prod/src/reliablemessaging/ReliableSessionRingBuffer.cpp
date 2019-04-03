// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace ReliableMessaging
{
/*** Inbound Ring Buffer Methods ***/
#pragma region InboundRingBuffer
	// always call with bufferLock_ held
	void WindowInboundRingBuffer::UnsafeSendAcknowledgement()
	{
		FABRIC_SEQUENCE_NUMBER acknowledgedSequenceNumber = nextSequenceNumberToAcknowledge_-1;

		Common::ErrorCode ackResult = ownerService_->AcknowledgeMessageNumber(acknowledgedSequenceNumber);

		if (ackResult.IsSuccess())
		{
			lastSequenceNumberAcknowledged_ = acknowledgedSequenceNumber;
			dispatchedButUnAcknowledgedMessageCount_ = 0;
		}
		else
		{
			RMTRACE->MessageAckFailed(ownerService_->TraceId,ownerService_->SessionId,acknowledgedSequenceNumber,ackResult.ErrorCodeValueToString());
		}
	}

	// this is a best effort method used in Abort
	void WindowInboundRingBuffer::ClearBuffer()
	{
		K_LOCK_BLOCK(bufferLock_)
		{
			// This method is called by Abort and there are cases where the buffer may already be closed
			if (UnsafeIsBufferClosed()) return;

			bufferState_ = Closed;
			sessionAborted_ = true;

			for (ULONG i = 0; i < maxWindowSize_; i++)
			{
				QueuedInboundMessageSPtr messageToClear = messageOrderingBuffer_[i];
				messageOrderingBuffer_[i] = nullptr;

				if (messageToClear != nullptr)
				{
					ownerService_->ReturnQueuedInboundMessage(messageToClear);
				}
			}

			bufferedMessageCount_ = 0;

			if (dispatchedButUnAcknowledgedMessageCount_ > 0)
			{
				UnsafeSendAcknowledgement();
			}
		}
	}

	void WindowInboundRingBuffer::DropDuplicateMessage(
		__in QueuedInboundMessageSPtr const &message,
		__in Common::StringLiteral traceMessage)
	{
		droppedMessagesInSessionCount_++;

		RMTRACE->MessageDropped(
			message->SessionHeader.SessionId,
			message->SessionHeader.SequenceNumber,
			traceMessage);

		QueuedInboundMessageSPtr messageToReturn = message;
		ownerService_->ReturnQueuedInboundMessage(messageToReturn);
	}

	// We always return Success -- perhaps this should be a void return method
	Common::ErrorCode WindowInboundRingBuffer::InsertAndFlush(
		__in QueuedInboundMessageSPtr const &message)
	{
		FABRIC_SEQUENCE_NUMBER messageSequenceNumber = message->SessionHeader.SequenceNumber;
		ULONG index =  messageSequenceNumber % maxWindowSize_;

		// TODO: most of this can be done with a Read lock -- Flush requires a Write lock
		K_LOCK_BLOCK(bufferLock_)
		{
			if (UnsafeIsBufferClosed()) 
			{
				RMTRACE->MessageReceiveAfterSessionClose(ownerService_->TraceId,message->SessionHeader.SessionId,message->SessionHeader.SequenceNumber);
				return Common::ErrorCodeValue::InvalidState;
			}

			receivedMessagesInSessionCount_++;

			if (messageSequenceNumber <= lastSequenceNumberAcknowledged_)
			{
				DropDuplicateMessage(message, "duplicate already acknowledged");
				return Common::ErrorCodeValue::Success;
			}
			else if (messageSequenceNumber < nextSequenceNumberToAcknowledge_)
			{
				DropDuplicateMessage(message, "duplicate not yet acknowledged");
				return Common::ErrorCodeValue::Success;
			}
			else if (!(KSHARED_IS_NULL(messageOrderingBuffer_[index])))
			{
				// the same unacknowledged message must have arrived twice -- assert that
				ASSERT_IF(
					messageOrderingBuffer_[index]->SessionHeader.SequenceNumber != message->SessionHeader.SequenceNumber,
					"Attempting to overwrite message SequenceNumber {0} in WindowInboundRingBuffer: SequenceNumber = {1} SessionId = {2}",
					messageOrderingBuffer_[index]->SessionHeader.SequenceNumber,
					message->SessionHeader.SequenceNumber,
					ownerService_->SessionId);

				DropDuplicateMessage(message, "duplicate not yet acknowledged");
				return Common::ErrorCodeValue::Success;
			}


			ASSERT_IF(messageSequenceNumber >= nextSequenceNumberToAcknowledge_+maxWindowSize_, "message with sequence number beyond the window received");


			messageOrderingBuffer_[index] = message;

			bufferedMessageCount_++;

			if (messageSequenceNumber==nextSequenceNumberToAcknowledge_)
			{
				// We are acknowledging only the truly enqued messages 
				Flush(index);

				if (dispatchedButUnAcknowledgedMessageCount_>=messageAckBatchSize_)
				{
					UnsafeSendAcknowledgement();
				}
			}

			// if there are any buffered or dispatched and unacknowledged messages make sure timer is running
			if ((dispatchedButUnAcknowledgedMessageCount_>0 || bufferedMessageCount_ > 0) && !ackTimerRunning_)
			{
				KAsyncContextBase::CompletionCallback retryCallback 
					= KAsyncContextBase::CompletionCallback(this, &WindowInboundRingBuffer::batchedAckTimerCallback);

				batchedAckTimer_->StartTimer(ackIntervalInMilliSeconds,nullptr,retryCallback);
				ackTimerRunning_ = true;
				RMTRACE->StartingAckTimer(ownerService_->SessionId,messageSequenceNumber,"Message arrival");
			}

		}

		return Common::ErrorCodeValue::Success;
	}

	Common::ErrorCode  WindowInboundRingBuffer::Flush(
		__in  ULONG index)
	{
		Common::ErrorCode flushResult = Common::ErrorCodeValue::Success;

		for (ULONG i = index; messageOrderingBuffer_[i] != nullptr; i = (i+1) % maxWindowSize_)
		{
			flushResult = ownerService_->Enqueue(messageOrderingBuffer_[i]);

			if (!flushResult.IsSuccess())
			{
				break;
			}

			messageOrderingBuffer_[i] = nullptr;
			bufferedMessageCount_--;
			nextSequenceNumberToAcknowledge_++;
			dispatchedButUnAcknowledgedMessageCount_++;
		}

		return flushResult;
	}

	void WindowInboundRingBuffer::batchedAckTimerCallback(
		KAsyncContextBase* const,           
		KAsyncContextBase& timer      
		)
	{
		KTimer *completedTimer 
			= static_cast<KTimer*>(&timer);

		ASSERT_IFNOT(completedTimer==batchedAckTimer_.RawPtr(),"batchedAckTimerCallback returned wrong timer");

		K_LOCK_BLOCK(bufferLock_)
		{
			// if there is anything to acknowledge, acknowledge it
			if (dispatchedButUnAcknowledgedMessageCount_>0)
			{
				UnsafeSendAcknowledgement();
			}

			batchedAckTimer_->Reuse();

			// if there are any buffered or dispatched and unacknowledged messages restart timer
			// dispatchedButUnAcknowledgedMessageCount_ > 0 is possible if the Ack failed
			if (dispatchedButUnAcknowledgedMessageCount_ > 0 || bufferedMessageCount_>0) 
			{
				KAsyncContextBase::CompletionCallback retryCallback 
					= KAsyncContextBase::CompletionCallback(this, &WindowInboundRingBuffer::batchedAckTimerCallback);

				batchedAckTimer_->StartTimer(ackIntervalInMilliSeconds,nullptr,retryCallback);
				ackTimerRunning_ = true;
				RMTRACE->StartingAckTimer(ownerService_->SessionId,0,"Retry left some messages unacknowledged");
			}
			else
			{
				ackTimerRunning_ = false;
				RMTRACE->StoppingAckTimer(ownerService_->SessionId,"Retry left no messages unacknowledged");
			}
		}
	}

	void WindowInboundRingBuffer::CloseBuffer()
	{
		K_LOCK_BLOCK(bufferLock_)
		{
			// This method is called during failure scenarios and there are cases where the buffer may already be closed
			if (UnsafeIsBufferClosed()) return;

			bufferState_ = Closed;

			ASSERT_IF(dispatchedButUnAcknowledgedMessageCount_>0, "dispatchedButUnAcknowledgedMessageCount_>0 in WindowInboundRingBuffer::CloseBuffer(), SessionId = {0}", ownerService_->SessionId);
			ASSERT_IF(bufferedMessageCount_>0, "bufferedMessageCount {0} (i.e., >0) in WindowInboundRingBuffer::CloseBuffer(), SessionId = {1}", bufferedMessageCount_, ownerService_->SessionId);
		}
	}  
#pragma endregion
/*** End inbound Ring Buffer Methods ***/
		
/*** Outbound Ring Buffer Methods ***/
#pragma region OutboundRingBuffer
	Common::ErrorCode WindowOutboundRingBuffer::SendOrHold(AsyncSendOperationSPtr const &sendOp)
	{
		/***

		The sequencxe number used for outbound messages is controlled by the outbound buffer to ensure that messages 
		enter the buffer exactly in the sequence number order ande there are never any empty slots in the buffer between 
		the last sequence number acknowledged and the highest sequence number in the buffer

		Reasoning:  The slot we are using here cannot be occupied because

		A. sendOperationQuota_ limits the number of sendOps that can be allocated to this service
		B. every consecutive sequence number is allocated to a sendOp and thus represents a sendOp
		B. the sendOps are only recycled into the quota when their state machine completes
		C. the state machine completes when the ack is processed below which also vacates the slot
		D. acks are processed in order of increasing sequence number, i.e., if a sendOp with sequence
		number X is not processed for an ack then it and all sendOps representing larger allocated
		sequence numbers are still outstanding, and not recycled into the quota

		If a sequence number smaller than the current one modulo sendOperationQuota_ has not been processed for an ack 
		and is still occupying the slot, then the number of sendOps outstanding must exceed the sendOperationQuota_

		***/

		K_LOCK_BLOCK(bufferLock_)
		{
			if (UnsafeIsBufferClosed())
			{
				RMTRACE->MessageSendAfterSessionClose(ownerService_->TraceId,sendOp->MessageId.SessionId,sendOp->MessageId.SequenceNumber);
				return Common::ErrorCodeValue::InvalidState;
			}

			ReliableMessageId messageId(ownerService_->SessionId,highestSequenceNumberInBuffer_+1);

			Common::ErrorCode result = sendOp->AddSessionHeader(messageId);

			if (!result.IsSuccess())
			{
				RMTRACE->MessageSendFailed(ownerService_->TraceId,messageId.SessionId,messageId.SequenceNumber,result.ErrorCodeValueToString());
				return result;
			}

			bufferedSendOpCount_++;
			highestSequenceNumberInBuffer_++;

			ASSERT_IFNOT(KSHARED_IS_NULL(sendOpBuffer_[highestSequenceNumberInBuffer_ % sendOperationQuota_]), "sendOpBuffer slot not empty for new sendOp");
			ASSERT_IFNOT(highestSequenceNumberInBuffer_ - lastSequenceNumberAcknowledged_ == bufferedSendOpCount_,
				"bufferedSendOpCount_ mismatch with high/low cursors implies gaps in the sendOpBuffer_ during ack processing");

			sendOpBuffer_[highestSequenceNumberInBuffer_ % sendOperationQuota_] = sendOp;

			ASSERT_IFNOT(bufferedSendOpCount_>0, "Incorrect buffer send operation count");

			if (!retryTimerRunning_) 
			{
				KAsyncContextBase::CompletionCallback retryCallback 
					= KAsyncContextBase::CompletionCallback(this, &WindowOutboundRingBuffer::retryTimerCallback);

				retryTimer_->StartTimer(defaultContentMessageRetryIntervalInMilliSeconds,nullptr,retryCallback);

				retryTimerRunning_ = true;
			}

			if (highestSequenceNumberInBuffer_ <= lastSequenceNumberAcknowledged_+windowSize_)
			{
				sendOp->TrySendMessage(Common::Stopwatch::Now());
			}
		}

		return Common::ErrorCodeValue::Success;
	}

	// always call under bufferLock_ write hold
	ULONG WindowOutboundRingBuffer::SendWindowSegment(
		__in FABRIC_SEQUENCE_NUMBER start,
		__in FABRIC_SEQUENCE_NUMBER end)
	{
		ASSERT_IF(end < start, "Invalid send window segment used in ring buffer");

		Common::StopwatchTime currentTime = Common::Stopwatch::Now();		
		ULONG messagesToTry = (ULONG) (end-start+1);
		ULONG messagesTried = 0;
		ULONG messagesSent = 0;

		for (ULONG j = start % sendOperationQuota_; messagesTried < messagesToTry; j = ++j % sendOperationQuota_)
		{
			if (sendOpBuffer_[j] != nullptr)
			{
				bool actuallySent = sendOpBuffer_[j]->TrySendMessage(currentTime);

				if (actuallySent)
				{
					messagesSent++;
				}
			}
			messagesTried++;
		}

		return messagesSent;
	}

	// helper to clear sendOps outside bufferLock_ when an ack is received
	void WindowOutboundRingBuffer::AckReceivedMessageRelease(
		__in AsyncSendOperationSPtr *sendOpReleaseBuffer,
		__in ULONG releaseCount)
	{
		for (ULONG i = 0; i < releaseCount; i++)
		{
			sendOpReleaseBuffer[i]->EraseTransportMessage(TransportMessageEraseReason::AckReceived);
			sendOpReleaseBuffer[i] = nullptr;
		}

		delete[] sendOpReleaseBuffer;
	}

	void WindowOutboundRingBuffer::ReleaseSendops(ULONG clearSegmentStart, ULONG clearSegmentCount)
	{
		AsyncSendOperationSPtr *sendOpReleaseBuffer = new (std::nothrow) AsyncSendOperationSPtr[clearSegmentCount];

		ASSERT_IF(
			sendOpReleaseBuffer == nullptr, 
			"WindowOutboundRingBuffer::ProcessMessageAck failed to allocate temporary KArray for sendOp release");

		// We need to complete the send operations and clear the ack waiter; however, send operation completion will go through transport stack 
		// delete callback which will trigger async operation completion and go all the way to the service completing the Begin/End async
		// we don't want to do this while holding bufferLock_: in particular, traveling through service code -- therefore we copy the sendOps to a 
		// temporary array and do the release outside the lock by posting that work item to the threadpool since we are on a transport thread

		ULONG cleared = 0;

		for (ULONG i = clearSegmentStart; cleared < clearSegmentCount; i = ++i % sendOperationQuota_)
		{
			ASSERT_IF(KSHARED_IS_NULL(sendOpBuffer_[i]),"Acked message missing in sendOpBuffer_");

			sendOpReleaseBuffer[cleared++] = sendOpBuffer_[i];
			sendOpBuffer_[i] = nullptr;
			bufferedSendOpCount_--;
		}

		Common::Threadpool::Post(
			[sendOpReleaseBuffer, clearSegmentCount]()
				{
					WindowOutboundRingBuffer::AckReceivedMessageRelease(sendOpReleaseBuffer, clearSegmentCount);
				}
		);
	}


	// This routine only processes positive Acks -- Nacks are intercepted by the session-level caller
	void WindowOutboundRingBuffer::ProcessMessageAck(
		__in FABRIC_SEQUENCE_NUMBER sequenceNumberAcknowledged)
	{
		K_LOCK_BLOCK(ackProcessingStateLock_)
		{
			waitingToProcessAckThreadCount_++;
		}


		bool readyToCompleteClose = false;

		K_LOCK_BLOCK(bufferLock_)
		{
			RMTRACE->StartedMessageAckProcessing(ownerService_->SessionId, sequenceNumberAcknowledged);

			if (sessionAborted_)
			{
				// session has been aborted, ack is not relevant and cannot be processed
				RMTRACE->MessageAckDropped(ownerService_->SessionId,sequenceNumberAcknowledged,"Session Aborted");
				return;
			}

			if (sequenceNumberAcknowledged <= lastSequenceNumberAcknowledged_)
			{
				// trace out of order or repeat ack
				if (sequenceNumberAcknowledged<lastSequenceNumberAcknowledged_)
				{
					RMTRACE->MessageAckDropped(ownerService_->SessionId,sequenceNumberAcknowledged,"OutOfOrder");
				}
				else
				{
					RMTRACE->MessageAckDropped(ownerService_->SessionId,sequenceNumberAcknowledged,"Repeat");
				}
			}
			else
			{
				ASSERT_IF(sequenceNumberAcknowledged > lastSequenceNumberAcknowledged_+windowSize_,
					"Acknowledged session sequence number is outside the window");

				// Reset the retry interval
				numberOfRetriesSinceLastAck_ = 0;
				ULONG clearSegmentStart = (lastSequenceNumberAcknowledged_+1) % sendOperationQuota_;
				ULONG clearSegmentCount = (ULONG)(sequenceNumberAcknowledged - lastSequenceNumberAcknowledged_);

				ReleaseSendops(clearSegmentStart, clearSegmentCount);

				/*	TODO: prior code to be deleted
				for (ULONG i = clearSegmentStart; cleared < clearSegmentCount; i = ++i % sendOperationQuota_)
				{
					ASSERT_IF(KSHARED_IS_NULL(sendOpBuffer_[i]),"Acked message missing in sendOpBuffer_");

					// complete the send operation and clear the ack waiter
					// TODO:  this is going through transport stack delete callback, and all the way to the service completing the Begin/End async
					//        all while holding this lock: in particular, traveling through service code -- need a way to avoid holding the lock throughout
					//        copy the sendOps to a temporary array and do the release outside the lock and/or post that work item to a threadpool since we are on a transport thread

					sendOpBuffer_[i]->EraseTransportMessage(TransportMessageEraseReason::AckReceived);
					sendOpBuffer_[i] = nullptr;
					cleared++;
					bufferedSendOpCount_--;
				}
				*/

				FABRIC_SEQUENCE_NUMBER previousSequenceNumberAcknowledged = lastSequenceNumberAcknowledged_;
				lastSequenceNumberAcknowledged_ = sequenceNumberAcknowledged;

				ASSERT_IFNOT(highestSequenceNumberInBuffer_ - lastSequenceNumberAcknowledged_ == bufferedSendOpCount_,
					"bufferedSendOpCount_ mismatch with high/low cursors implies gaps in the sendOpBuffer_ during ack processing");

				lastAckAt_ = Common::Stopwatch::Now();

				if (retryTimerRunning_)
				{
					// The Cancel process is not synchronous -- callback with STATUS_CANCELLED will be invoked asynchronously
					// This means there is a race with the SendOrHold method -- we do not want SendOrHold to attempt to start 
					// the timer before the cancel process is finished -- hence we do not change the state of retryTimerRunning_
					// until we reach the callback
					retryTimer_->Cancel();
				}

				// move the window if there are any messages in the window that have been held rather than sent
				// these messages are being sent for the first time
				if (bufferedSendOpCount_ > 0)
				{
					ASSERT_IFNOT(highestSequenceNumberInBuffer_>lastSequenceNumberAcknowledged_, "Incorrect buffer send operation count during ack processing");
					FABRIC_SEQUENCE_NUMBER startSegmentAt = previousSequenceNumberAcknowledged+windowSize_+1;
					FABRIC_SEQUENCE_NUMBER endSegmentAt = lastSequenceNumberAcknowledged_+windowSize_;
					ULONG actuallySent = SendWindowSegment(startSegmentAt,endSegmentAt);
					RMTRACE->SendWindowSegment("Moving Window",ownerService_->SessionId,startSegmentAt,endSegmentAt,actuallySent);
				}
				else if (UnsafeIsBufferClosed())
				{
					readyToCompleteClose = true;
				}			
			}
		
			RMTRACE->FinishedMessageAckProcessing(ownerService_->SessionId, sequenceNumberAcknowledged);
		}

		if (readyToCompleteClose)
		{
			CompleteCloseProcess();
		}

		K_LOCK_BLOCK(ackProcessingStateLock_)
		{
			waitingToProcessAckThreadCount_--;
		}
	}



	void WindowOutboundRingBuffer::retryTimerCallback(
		KAsyncContextBase* const,           
		KAsyncContextBase& timer      
		)
	{
		KTimer *completedTimer 
			= static_cast<KTimer*>(&timer);

		ASSERT_IFNOT(completedTimer==retryTimer_.RawPtr(),"retryTimerCallback returned wrong timer");

		bool delayForAckProcessing = false;

		K_LOCK_BLOCK(ackProcessingStateLock_)
		{
			// delay if there is a thread waiting to process an ack
			if (waitingToProcessAckThreadCount_>0) 
			{
				delayForAckProcessing = true;
			}
		}

		if (delayForAckProcessing)
		{
			// sleep for a retry timer cycle
			Sleep(defaultContentMessageRetryIntervalInMilliSeconds);
		}

		K_LOCK_BLOCK(bufferLock_)
		{
			retryTimer_->Reuse();

			Common::StopwatchTime currentTime = Common::Stopwatch::Now();

			int64 timeSinceLastRetry = (currentTime - lastRetryAt_).TotalPositiveMilliseconds();
			int64 timeSinceLastAck = (currentTime - lastAckAt_).TotalPositiveMilliseconds();

			// exponentially increase actual retry interval with numberOfRetriesSinceLastAck_
			if (
				timeSinceLastAck >= defaultContentMessageRetryIntervalInMilliSeconds &&
				bufferedSendOpCount_>0 
				&& timeSinceLastRetry >= defaultContentMessageRetryIntervalInMilliSeconds*(1<<numberOfRetriesSinceLastAck_))
			{
				lastRetryAt_ = currentTime;
				numberOfRetriesSinceLastAck_++;

				// something left to retry
				FABRIC_SEQUENCE_NUMBER startSegmentAt = lastSequenceNumberAcknowledged_+1;
				FABRIC_SEQUENCE_NUMBER endSegmentAt = lastSequenceNumberAcknowledged_+windowSize_;
				ULONG actuallySent = SendWindowSegment(startSegmentAt,endSegmentAt);
				RMTRACE->SendWindowSegment("Retry Sending Whole Window",ownerService_->SessionId,startSegmentAt,endSegmentAt,actuallySent);
			}

			// Keep timer going if there are messages to send
			if (bufferedSendOpCount_>0)
			{
				KAsyncContextBase::CompletionCallback retryCallback 
					= KAsyncContextBase::CompletionCallback(this, &WindowOutboundRingBuffer::retryTimerCallback);

				// timer fires in defaultContentMessageRetryIntervalInMilliSeconds but actual retry may not occur depending on numberOfRetriesSinceLastAck_
				retryTimer_->StartTimer(defaultContentMessageRetryIntervalInMilliSeconds,nullptr,retryCallback);

				retryTimerRunning_ = true;
			}
			else
			{
				retryTimerRunning_ = false;
			}
		}
	}

	void WindowOutboundRingBuffer::CloseBuffer()
	{
		K_LOCK_BLOCK(bufferLock_)
		{
			ASSERT_IF(UnsafeIsBufferClosed(), "Outbound ring buffer closed twice");
			ASSERT_IFNOT(bufferedSendOpCount_ == 0, "Abortive CloseBuffer called on non-empty outbound ring buffer");

			bufferState_ = Closed;
		}
	}

	void WindowOutboundRingBuffer::CloseBuffer(BufferClosureCompleteCallback callback)
	{
		bool readyToCompleteClose = false;

		K_LOCK_BLOCK(bufferLock_)
		{
			ASSERT_IF(UnsafeIsBufferClosed(), "Outbound ring buffer closed twice");

			bufferState_ = Closed;
			closureCompleteCallback_ = callback;

			if (bufferedSendOpCount_ == 0)
			{
				readyToCompleteClose = true;
			}
		}

		if (readyToCompleteClose)
		{
			// do this on a different thread to avoid deadlock since CloseBuffer is called under sessionServiceInstanceLock_
			// to implement atomic state change to Closing, preventing further message Send operations when the Close API is called
			Common::Threadpool::Post([this](){ this->CompleteCloseProcess();});
		}
	}

	void WindowOutboundRingBuffer::CompleteCloseProcess()
	{
		K_LOCK_BLOCK(bufferLock_)
		{
			ASSERT_IFNOT(UnsafeIsBufferClosed(), "Outbound buffer not closed during closure completion process");
			ASSERT_IFNOT(lastSequenceNumberAcknowledged_==highestSequenceNumberInBuffer_, "Outbound buffer not drained during closure completion process");
			ASSERT_IFNOT(bufferedSendOpCount_==0, "Outbound buffer not drained during closure completion process");
			ASSERT_IF(numberOfRetriesSinceLastAck_!=0, "Buffer retry count non-zero during closure completion process");
			bufferClosureProcessCompleted_ = true;
		}

		closureCompleteCallback_();
	}

	// Used to cancel and clear all outstanding sendOps during abort
	// parameter expectClosed is true iff the session state prior to Abort was Closing which means this buffer should already be closed
	void WindowOutboundRingBuffer::ClearBuffer(bool expectClosed)
	{
		K_LOCK_BLOCK(bufferLock_)
		{
			if (!expectClosed)
			{
				ASSERT_IF(UnsafeIsBufferClosed(), "Outbound ring buffer closed twice");
			}
			else
			{
				ASSERT_IFNOT(UnsafeIsBufferClosed(), "Outbound ring buffer not closed when expected");
			}

			bufferState_ = Closed;
			sessionAborted_ = true;

			ULONG clearSegmentStart = (lastSequenceNumberAcknowledged_+1) % sendOperationQuota_;
			ULONG clearSegmentCount = (ULONG)(highestSequenceNumberInBuffer_ - lastSequenceNumberAcknowledged_);

			ASSERT_IFNOT(clearSegmentCount==bufferedSendOpCount_,
					"bufferedSendOpCount_ mismatch with high/low cursors implies gaps in the sendOpBuffer_ during ClearBuffer processing");

			ReleaseSendops(clearSegmentStart, clearSegmentCount);

			/* TODO: prior code to be deleted
			for (ULONG i = clearSegmentStart; cleared < clearSegmentCount; i = ++i % sendOperationQuota_)
			{
				ASSERT_IF(KSHARED_IS_NULL(sendOpBuffer_[i]),"SendOp missing in sendOpBuffer_ during Abort");

				// Cancel the send operation and clear the ack waiter
				// TODO:  this is going through transport stack delete callback, and all the way to the service completing the Begin/End async
				//        all while holding this lock: in particular, traveling through service code -- need a way to avoid holding the lock throughout
				//        copy the sendOps to a temporary array and do the release outside the lock and/or post that work item to a threadpool since we are on a transport thread
				sendOpBuffer_[i]->EraseTransportMessage(TransportMessageEraseReason::SendCanceled);
				sendOpBuffer_[i] = nullptr;
				cleared++;
				bufferedSendOpCount_--;
			}
			*/
		}
	}  
#pragma endregion
/*** End outbound Ring Buffer Methods ***/
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

using namespace Common;

namespace ReliableMessaging
{
	K_FORCE_SHARED_NO_OP_DESTRUCTOR(AsyncProtocolOperation)
	K_FORCE_SHARED_NO_OP_DESTRUCTOR(AsyncSendOperation)
	K_FORCE_SHARED_NO_OP_DESTRUCTOR(AsyncReceiveOperation)

	
	const ULONG AsyncProtocolOperation::LinkOffset = FIELD_OFFSET(AsyncProtocolOperation, qlink_);
	const ULONG AsyncSendOperation::LinkOffset = FIELD_OFFSET(AsyncSendOperation, qlink_);
	const ULONG AsyncReceiveOperation::LinkOffset = FIELD_OFFSET(AsyncReceiveOperation, qlink_);

	//
	// string converter for tracing
	//
	Common::StringLiteral EraseReasonToString(TransportMessageEraseReason eraseReason)
	{
		switch (eraseReason)
		{
			case AckReceived: return "AckReceived";
			case SendCanceled: return "SendCanceled";
			case MaxRetryExceeded: return "MaxRetryExceeded";
			case SendNotStartedSuccessfully: return "SendNotStartedSuccessfully";
			default: Common::Assert::CodingError("EraseReasonToString called with unknown TransportMessageEraseReason {0}", (int)eraseReason);
		}
	}


	//
	//  AsyncSendOperation methods
	//

	// constructor
	AsyncProtocolOperation::AsyncProtocolOperation() :
		lastSendResult_(Common::ErrorCodeValue::NotReady),
		sessionId_(Common::Guid::Empty()),
		retryCount_(0),
		maxRetryCount_(-1),
		protocolCompleted_(false),
		lastSendAttemptAt_(Common::StopwatchTime::Zero),
		protocolAction_(ProtocolAction::Unknown),
		expectedAckAction_(ProtocolAction::Unknown),
		messageTarget_(nullptr),
		myEnvironment_(nullptr)
	{
		NTSTATUS createStatus = KTimer::Create(retryTimer_,Common::GetSFDefaultPagedKAllocator(),RMSG_ALLOCATION_TAG);

		if (!NT_SUCCESS(createStatus))
		{
			SetConstructorStatus(createStatus);
		}
	}

	AsyncProtocolOperationSPtr AsyncProtocolOperation::GetOperation(ReliableMessagingEnvironmentSPtr myEnvironment)
	{
		AsyncProtocolOperationSPtr operationToGet;
		myEnvironment->GetPooledResource(operationToGet);
		return operationToGet;
	}

	void AsyncProtocolOperation::ReturnOperation(AsyncProtocolOperationSPtr& protocolOp, ReliableMessagingEnvironmentSPtr myEnvironment)
	{
		myEnvironment->ReturnPooledResource(protocolOp);
	}

	void AsyncProtocolOperation::ProtocolCompletionCallback(
					KAsyncContextBase* const,           
					KAsyncContextBase& protocolOp)
	{
		AsyncProtocolOperationSPtr completedOp 
			= static_cast<AsyncProtocolOperation*>(&protocolOp);

		ASSERT_IFNOT(completedOp.RawPtr()==this,"ProtocolCompletionCallback returned wrong Protocol Op");

		// we don't actually use a buffers deleted callback from Transport for protocol messages but just a safer way
		Transport::MessageUPtr messageToDelete = std::move(protocolMessage_);

		ReturnOperation(completedOp, myEnvironment_);
	}


	Common::ErrorCode AsyncProtocolOperation::StartProtocolOperation(
			__in ProtocolAction::Code protocolAction, 
			__in ProtocolAction::Code expectedAckAction,
			__in Common::Guid const &sessionId,
			__in Transport::MessageUPtr &&protocolMessage, 
			__in ISendTarget::SPtr const &messageTarget,
			__in ReliableMessagingEnvironmentSPtr const &myEnvironment,
			__in int maxRetryCount)
	{
		// record for retry
		protocolAction_ = protocolAction;
		expectedAckAction_ = expectedAckAction;
		protocolMessage_ = std::move(protocolMessage);
		messageTarget_ = messageTarget;
		myEnvironment_ = myEnvironment;
		sessionId_ = sessionId;
		maxRetryCount_ = maxRetryCount;

		KAsyncContextBase::CompletionCallback completionCallback 
			= KAsyncContextBase::CompletionCallback(this,&AsyncProtocolOperation::ProtocolCompletionCallback);

		ProtocolAckKey ackKey(sessionId_,expectedAckAction_);
		//
		// register to receive Ack

		NTSTATUS insertStatus = STATUS_INSUFFICIENT_RESOURCES;

		AsyncProtocolOperationSPtr operationToInsert = KSharedPtr<AsyncProtocolOperation>(this);
		AsyncProtocolOperationSPtr existingOperation = nullptr;

		K_LOCK_BLOCK(myEnvironment_->sessionServiceCommonLock_)
		{
			insertStatus = myEnvironment_->sessionProtocolAckWaiters_.FindOrInsert(operationToInsert,ackKey,existingOperation);
		}

		if (insertStatus==STATUS_OBJECT_NAME_EXISTS)
		{
			// We do not have more than one outstanding protocol operation for the same session, except for OpenReliableSessionResponse in some edge cases
			// such as session manager closing between the first message and retry of an OpenReliableSessionRequest being received 
			// TODO: if we want to avoid tombstones for sessions we must tolerate this situation as a no-op (fail the Start) for ProtocolResponse messages 
			// this really only makes sense in the "not found" case which we don't know here -- would need an extra parameter
			ASSERT_IF(
				protocolAction != ProtocolAction::Code::OpenReliableSessionResponse, 
				"Unexpected duplicate key SessionId: {0} Code: {1} in sessionProtocolAckWaiters_", sessionId_, (int32)expectedAckAction_);
		}
		else if (insertStatus==STATUS_INSUFFICIENT_RESOURCES)
		{
			RMTRACE->OutOfMemory("AsyncProtocolOperation::OnStart");
			return Common::ErrorCodeValue::OutOfMemory;
		}

		Start(nullptr,completionCallback);

		KAsyncContextBase::CompletionCallback retryCallback 
			= KAsyncContextBase::CompletionCallback(this, &AsyncProtocolOperation::retryTimerCallback);

		// timer fires in defaultProtocolMessageRetryIntervalInMilliSeconds
		// use self as parent to avoid timer-cancel and timer-reuse race at completion of the AsyncProtocolOperation
		retryTimer_->StartTimer(defaultProtocolMessageRetryIntervalInMilliSeconds,this,retryCallback);

		// Do this last to avoid completing before StartProtocolOperation starts the retry timer subOp
		SendProtocolMessage();


		return Common::ErrorCodeValue::Success;
	}

	void AsyncProtocolOperation::OnCancel()
	{
		retryTimer_->Cancel();
		Complete(Common::ErrorCodeValue::OperationCanceled);
	}

	void AsyncProtocolOperation::OnReuse()
	{
		ASSERT_IFNOT(KSHARED_IS_NULL(protocolMessage_),"Outbound Transport Message not NULL in AsyncSendOperation::OnReuse");
		lastSendResult_.Reset(Common::ErrorCodeValue::NotReady);
		retryCount_ = 0;
		maxRetryCount_ = -1;
		retryTimer_->Reuse();
		protocolCompleted_ = false;
		lastSendAttemptAt_ = Common::StopwatchTime::Zero;
		protocolAction_ = ProtocolAction::Unknown;
		expectedAckAction_ = ProtocolAction::Unknown;
		sessionId_ = Common::Guid::Empty();
		messageTarget_ = nullptr;
		myEnvironment_ = nullptr;

		__super::OnReuse();
	}

	void AsyncProtocolOperation::SendProtocolMessage()
	{
			Transport::MessageUPtr protocolMessageClone = protocolMessage_->Clone();

			if (retryCount_==0)
			{
				ASSERT_IFNOT(lastSendResult_.ReadValue()==Common::ErrorCodeValue::NotReady, 
												 "Unexpected lastSendResult_ initialization in first SendProtocolMessage attempt");
			}
			else
			{
				//RMTRACE retry protocol mesage with lastResult as ..
			}

			lastSendResult_ = myEnvironment_->transport_->SendOneWay(messageTarget_, std::move(protocolMessageClone));
				
			// we count both successful and unsuccessful sends for retry--the retry limit is primarily for Abort messages
			// which tend to occur in difficult circumstances where we want to limit the retry cycle
			retryCount_++;
			lastSendAttemptAt_ = Common::Stopwatch::Now();

			if (!lastSendResult_.IsSuccess())
			{
				//RMTRACE
			}
	}


	void AsyncProtocolOperation::retryTimerCallback(
				__in KAsyncContextBase* const,           
				__in KAsyncContextBase& timer      
				)
	{
		KTimer *completedTimer 
			= static_cast<KTimer*>(&timer);

		ASSERT_IFNOT(completedTimer==retryTimer_.RawPtr(),"AsyncProtocolOperation::retryTimerCallback returned wrong timer");
		ASSERT_IF(maxRetryCount_ != -1 && retryCount_ > maxRetryCount_,"retryCount exceeded max for AsyncProtocolOperation");

		if (!protocolCompleted_)
		{
			// check if we have a limit on retry, and if we do, the limit has been reached
			if (maxRetryCount_ != -1 && retryCount_ == maxRetryCount_)
			{
				ProtocolAckKey ackKey(sessionId_,protocolAction_);
				BOOLEAN extractResult=FALSE;
				AsyncProtocolOperationSPtr waitingOperation = nullptr;

				K_LOCK_BLOCK(myEnvironment_->sessionServiceCommonLock_)
				{
					extractResult = myEnvironment_->sessionProtocolAckWaiters_.Extract(ackKey,waitingOperation);
				}

				if (extractResult)
				{
					ASSERT_IFNOT(waitingOperation.RawPtr()==this, "Protocol max retry exceeded path extracted unexpected operation to cancel");
					Complete(Common::ErrorCodeValue::OperationFailed);
				}
				else
				{
					// lost a race with successful completion
					//RMTRACE->MultipleProtocolAcksReceived(sessionId,receivedSessionHeader.ActionCode);
				}
			}
			else
			{
				SendProtocolMessage();
			
				// Keep timer going 
				retryTimer_->Reuse();
				KAsyncContextBase::CompletionCallback retryCallback 
					= KAsyncContextBase::CompletionCallback(this, &AsyncProtocolOperation::retryTimerCallback);

				// use self as parent to avoid timer-cancel and timer-reuse race at completion of the AsyncProtocolOperation
				retryTimer_->StartTimer(defaultProtocolMessageRetryIntervalInMilliSeconds,this,retryCallback);
			}
		}
	}

	BOOLEAN AsyncProtocolOperation::CompleteProtocolOperation(
			__in ProtocolAction::Code ackAction)
	{
		ASSERT_IFNOT(ackAction==expectedAckAction_, "unexpected protocol ack received for session");
		protocolCompleted_ = true;
		retryTimer_->Cancel();
		return Complete(Common::ErrorCodeValue::Success);
	}

	//
	//  AsyncSendOperation methods
	//

	// constructor
	AsyncSendOperation::AsyncSendOperation() : 
		outboundMessage_(nullptr), 
		trySendResult_(Common::ErrorCodeValue::NotReady),
		sendTryCount_(0),
		lastSendAttemptAt_(Common::StopwatchTime::Zero),
		eraseReason_(TransportMessageEraseReason::Unknown)
	{
		originalMessage_.SetNoAddRef(nullptr);
	}



	// private helper for StartSendOperation
	// This may throw an exception in low memory situations -- should be wrappen in a try catch block when called
	Common::ErrorCode AsyncSendOperation::CreateTransportMessage(
				__out Transport::MessageUPtr &transportMessage)
	{
		ASSERT_IF(originalMessage_.GetRawPointer()==nullptr, "AsyncSendOperation::CreateTransportMessage called with null originalMessage_");
		// TODO: check if Transport tolerates empty buffers--do we even want to allow those?

		std::vector<Common::const_buffer> bufferVector;

		ULONG bufferCount;
		const FABRIC_OPERATION_DATA_BUFFER *buffers;

		HRESULT hr = originalMessage_->GetData(&bufferCount, &buffers);
		ASSERT_IF(FAILED(hr),"Unexpected error from IFabricOperationData::GetData");

		try	// implicit memory allocations happening
		{
			for (ULONG i=0; i<bufferCount; i++)
			{
				bufferVector.push_back(
						Common::const_buffer((buffers[i]).Buffer, 
												(buffers[i]).BufferSize));
			}

			// Make the base message with buffers
			transportMessage = Common::make_unique<Transport::Message>(
																	bufferVector,
																	AsyncSendOperation::MessageBuffersFreed, // delete callback
																	this); // Transport will hand this context back in the MessageBuffersFreed
		}
		catch (std::exception const&)
		{
			RMTRACE->OutOfMemory("AsyncSendOperation::CreateTransportMessage");
			return Common::ErrorCodeValue::OutOfMemory;
		}

		return Common::ErrorCodeValue::Success;
	}

	Common::ErrorCode AsyncSendOperation::AddSessionHeader(
		__in ReliableMessageId &messageId)
	{
		try // Needed for Transport functionality usage -- exceptions will occur in low memory conditions
		{
			ReliableSessionHeader reliableSessionHeader(messageId.SessionId, ProtocolAction::SendReliableMessage, messageId.SequenceNumber);

			// Add the headers--this may cause exceptions in low memory situations as well, caught below
			MessageHeaders& headers = outboundMessage_->Headers;
			headers.Add(reliableSessionHeader);
			headers.Add(Transport::ActionHeader(L"ReliableSessionMessage"));

			// the messageId in the headers is serialized and opaque, this keeps it transparent
			messageId_ = messageId;

			return Common::ErrorCodeValue::Success;
		}
		catch (std::exception const &)
		{
			RMTRACE->TransportException("AsyncSendOperation::AddSessionHeader");
			return Common::ErrorCodeValue::OutOfMemory;
		}
	}


	Common::ErrorCode AsyncSendOperation::StartSendOperation(    
			__in IFabricOperationData *originalMessage,
			__in_opt KAsyncContextBase* const ParentAsyncContext,
			__in_opt KAsyncContextBase::CompletionCallback CallbackPtr
			)
	{
		ASSERT_IF(originalMessage==nullptr, "AsyncSendOperation::StartSendOperation called with null originalMessage");

		originalMessage_.SetAndAddRef(originalMessage);

		Transport::MessageUPtr transportMessage;
		Common::ErrorCode result = CreateTransportMessage(outboundMessage_);

		if (!result.IsSuccess()) 
		{
			RMTRACE->MessageSendFailed(sessionContext_->TraceId,messageId_.SessionId,messageId_.SequenceNumber,result.ErrorCodeValueToString());
			return result;
		}


		RMTRACE->AsyncSendStarted(messageId_.SessionId,messageId_.SequenceNumber);

		// Start the KAsyncContextBase state machine--this must be done before InsertIntoSendBuffer because that operation can lead 
		// to quick completion of the sendOp and completion must not occur before start!

		Start(ParentAsyncContext, CallbackPtr);

		// the reason the InsertIntoSendBuffer is done here rather than in OnStart is that the service may do a BeginClose 
		// on the outbound session right after the last BeginSend and that sets up a race between the Onstart signal and
		// the closure process.  The closure process almost always wins and the outbound buffer will then get closed before
		// the final sends actually get into the outbound ring buffer causing InvalidState failures which would be unexpected

		AsyncSendOperationSPtr operation = KSharedPtr<AsyncSendOperation>(this);
		// The buffer will call us back to add a session header--the reason for the convoluted path is to make sure the
		// sequence numbers of messages inserted in the buffer never have gaps even with parallel sending threads
		result = sessionContext_->InsertIntoSendBuffer(operation);
		if (!result.IsSuccess())
		{
			RMTRACE->MessageSendFailed(sessionContext_->TraceId,messageId_.SessionId,messageId_.SequenceNumber,result.ErrorCodeValueToString());
			// this will complete the operation after and as a result of freeing the messaqge buffers
			// this sendOp never made it into the outbound ring buffer so its completion is independent of the normal path
			EraseTransportMessage(SendNotStartedSuccessfully);
		}		


		return Common::ErrorCodeValue::Success;
	}

	void AsyncSendOperation::OnStart()
	{
	}



	// mechanism to actually send a message--used for initial send as well as retry
	bool AsyncSendOperation::TrySendMessage(
			__in Common::StopwatchTime currentTime)
	{
		bool actuallySend = false;

		if (lastSendAttemptAt_ == Common::StopwatchTime::Zero)
		{
			// never been sent
			ASSERT_IFNOT(trySendResult_.ReadValue()==Common::ErrorCodeValue::NotReady, 
				                             "Inconsistent lastSendAttemptAt_ and trySendResult_ in AsyncSendOperation");
			actuallySend = true;
		}
		else if (!trySendResult_.IsSuccess())
		{
			// error in last send attempt
			actuallySend = true;
				
			//RMTRACE
		}
		else
		{
			// no ack for retry interval?
			actuallySend = (currentTime - lastSendAttemptAt_).TotalPositiveMilliseconds() > defaultContentMessageRetryIntervalInMilliSeconds;
		}

		if (actuallySend)
		{
			Transport::MessageUPtr outboundMessageClone = outboundMessage_->Clone();
			trySendResult_ = sessionContext_->Send(std::move(outboundMessageClone));
			lastSendAttemptAt_ = Common::Stopwatch::Now();
			sendTryCount_++;
		}

		// return the fact of whether this actually got out on the wire
		return actuallySend && trySendResult_.IsSuccess();
	}

	// completion callback that is triggered by transport when transport buffers are freed
	void AsyncSendOperation::MessageBuffersFreed(std::vector<Common::const_buffer> const &buffers, void *sendOp)
	{
		// buffers are being freed and that is the semantic of the EndSend which we will now enable
		UNREFERENCED_PARAMETER(buffers);
		AsyncSendOperation *sendOperation = static_cast<AsyncSendOperation*>(sendOp);
		RMTRACE->AsyncSendCompleted(sendOperation->messageId_.SessionId,sendOperation->messageId_.SequenceNumber,EraseReasonToString(sendOperation->eraseReason_));

		ASSERT_IF(sendOperation->eraseReason_ == TransportMessageEraseReason::Unknown, "Unknown erase reason in message buffer delete callback");

		Common::ErrorCode finalResult;

		switch (sendOperation->eraseReason_)
		{
		case AckReceived: 
			finalResult = Common::ErrorCodeValue::Success;
			break;

		case SendCanceled:
			finalResult = Common::ErrorCodeValue::OperationCanceled;
			break;

		case SendNotStartedSuccessfully:
			finalResult = Common::ErrorCodeValue::InvalidState;
			break;

		default:
			Common::Assert::CodingError("Unexpected sendOperation->eraseReason_ in MessageBuffersFreed callback");
		}

		BOOLEAN completeSuccess = sendOperation->Complete(finalResult);
		ASSERT_IF(completeSuccess==FALSE,"Complete called twice on AsyncSendOperation");
	}


	//
	//  AsyncReceiveOperation methods
	//

	// constructor
	AsyncReceiveOperation::AsyncReceiveOperation() : noWaitDequeuedMessage_(nullptr)
	{
		receivedMessage_.SetNoAddRef(nullptr);
	}

	void AsyncReceiveOperation::OnStart()
	{
		if (!waitForMessages_)
		{
			ProcessQueuedInboundMessage(noWaitDequeuedMessage_);
		}
	}


	// This Start method does all the work; we don't need to defer any work to OnStart since this does nothing in this case 
	Common::ErrorCode AsyncReceiveOperation::StartReceiveOperation(
		__in BOOLEAN waitForMessages,
		__in_opt KAsyncContextBase* const ParentAsyncContext,
		__in_opt KAsyncContextBase::CompletionCallback CallbackPtr
		)
	{
		Common::ErrorCode finalResult;
		RMTRACE->AsyncReceiveStarted(sessionContext_->SessionId);

		waitForMessages_ = waitForMessages==TRUE;

		if (waitForMessages==FALSE)
		{
			Common::ErrorCode immediateDequeueResult = sessionContext_->TryDequeue(noWaitDequeuedMessage_);

			if (immediateDequeueResult.IsSuccess())
			{
				// Start the AsyncReceiveOperation state machine; OnStart continuation will finish the work
				Start(ParentAsyncContext, CallbackPtr);
			}

			finalResult = immediateDequeueResult;
		}
		else
		{
			InboundMessageQueue::DequeueOperation::SPtr deqOperation;

			Common::ErrorCode dequeueCreateResult = sessionContext_->CreateDequeueOperation(KSharedPtr<AsyncReceiveOperation>(this),deqOperation);

			if (dequeueCreateResult.IsSuccess())
			{
				// Start the AsyncReceiveOperation state machine
				Start(ParentAsyncContext, CallbackPtr);

				// start the receive waiter
				KAsyncContextBase::CompletionCallback completionCallback 
					= KAsyncContextBase::CompletionCallback(this, &AsyncReceiveOperation::AsyncDequeueCallback);

				// StartDequeue itself does not fail -- may complete the DequeueOperation with failure if the queue is being shut down
				deqOperation->StartDequeue(nullptr,completionCallback);
			}
			else
			{
				RMTRACE->CreateDequeueOperationFailed(sessionContext_->TraceId,sessionContext_->SessionId,dequeueCreateResult.ErrorCodeValueToString());

				// ASSERT_IFNOT(dequeueCreateResult.IsSuccess(), "temporary assert SessionId = {0} Result = {1}", sessionContext_->SessionId,dequeueCreateResult.ErrorCodeValueToString());

				// report failure to start
			}

			finalResult = dequeueCreateResult;
		}

		if (!finalResult.IsSuccess())
		{
			RMTRACE->AsyncReceiveCompleted(sessionContext_->SessionId,finalResult.ErrorCodeValueToString());
		}

		return finalResult;
	}

	void AsyncReceiveOperation::ProcessQueuedInboundMessage(
		__in QueuedInboundMessageSPtr &inboundMessage)
	{
		if (inboundMessage->IsCloseMessage)
		{
			// Done with QueuedInboundMessage return to pool -- return before Complete so sessionContext_ is still alive
			sessionContext_->ReturnQueuedInboundMessage(inboundMessage);			
			
			// the last message in the session was processed, close the session
			sessionContext_->ContinueCloseSessionInbound();
			Complete(FABRIC_E_OBJECT_CLOSED);
		}
		else
		{
			Transport::MessageUPtr realMessage;
			inboundMessage->GetMessageUPtr(realMessage);

			// Done with QueuedInboundMessage return to pool -- return before Complete so sessionContext_ is still alive
			sessionContext_->ReturnQueuedInboundMessage(inboundMessage);

			// This Transport::MessageUPtr was already Cloned by the original message handler so we can hang on to the underlying buffers
			// so long as we hang on to this Transport::MessageUPtr as we do on the receivedMessage_ below

			IFabricOperationData *receivedMessage;
			HRESULT hr = ComFabricReliableSessionMessage::CreateFromTransportMessage(std::move(realMessage),&receivedMessage);

			if (SUCCEEDED(hr))
			{
				receivedMessage_.SetNoAddRef(receivedMessage);
				Complete(S_OK);
			}
			else
			{
				Complete(hr);
			}
		}
	}

	// Callback from KAsyncQueue when the DequeueOperation started by this AsyncReceive completes
	// Normal case: a message is actually received for the session, otherwise when the queue is deactivated completion occurs with K_STATUS_SHUTDOWN_PENDING
	void AsyncReceiveOperation::AsyncDequeueCallback(
			    KAsyncContextBase* const,
				KAsyncContextBase& CompletingReceiveOp      // CompletingReceiveOp
				)
	{
		InboundMessageQueue::DequeueOperation *completedReceiveOp 
			= static_cast<InboundMessageQueue::DequeueOperation*>(&CompletingReceiveOp);

		NTSTATUS completedReceiveOpStatus = completedReceiveOp->Status();

		// TODO: make a simple KQueue of these session-specific DequeueOperations for reuse
		// Among other failures, avoid processing the DequeueOperations being flushed by Deactivate of the AsyncQueue
		if (!NT_SUCCESS(completedReceiveOpStatus)) 
		{
			if (completedReceiveOpStatus != K_STATUS_SHUTDOWN_PENDING)
			{
				RMTRACE->CompletedReceiveOpFailed(sessionContext_->TraceId,sessionContext_->SessionId, completedReceiveOpStatus);
				Complete(Common::ErrorCode::FromNtStatus(completedReceiveOpStatus));
			}
			else
			{
				// the waiting DequeueOperation is being drained at session Close
				Complete(Common::ErrorCodeValue::OperationCanceled);
			}
		}
		else
		{
			QueuedInboundMessage& queuedInboundMessage = completedReceiveOp->GetDequeuedItem();

			QueuedInboundMessageSPtr inboundMessageSptr = nullptr;
			inboundMessageSptr.Attach(&queuedInboundMessage);

			ProcessQueuedInboundMessage(inboundMessageSptr);
		}
	}
}

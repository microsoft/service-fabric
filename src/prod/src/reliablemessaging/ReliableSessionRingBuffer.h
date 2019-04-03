// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace ReliableMessaging
{
	typedef KDelegate<VOID()> BufferClosureCompleteCallback;

	enum BufferState {
		Initialized = 100,
		Opened,						// TODO: this state currently not used -- probably should be
		Closed
	};


	class WindowInboundRingBuffer : public KObject<WindowInboundRingBuffer>
	{
		friend class ReliableSessionService;

	public:

        __declspec (property(get=get_maxWindowSize)) ULONG MaxWindowSize;
        inline ULONG get_maxWindowSize() const { return maxWindowSize_; }

        __declspec (property(get=get_currentWindowSize)) ULONG CurrentWindowSize;
        inline ULONG get_currentWindowSize() const { return currentWindowSize_; }

        __declspec (property(get=get_bufferedMessageCount)) ULONG BufferedMessageCount;
        inline ULONG get_bufferedMessageCount() const { return bufferedMessageCount_; }

		WindowInboundRingBuffer(
				KAllocator &allocator, 
				ULONG maxWindowSize, 
				ULONG ackBatchSize,
				ReliableSessionServiceSPtr const &ownerService) :
			messageOrderingBuffer_(allocator,maxWindowSize),
			bufferState_(Initialized),
			sessionAborted_(false),
			nextSequenceNumberToAcknowledge_(0),
			lastSequenceNumberAcknowledged_(-1),
			dispatchedButUnAcknowledgedMessageCount_(0),
			bufferedMessageCount_(0),
			maxWindowSize_(maxWindowSize),
			currentWindowSize_(std::min(maxWindowSize,ackBatchSize*ackBatchSizeToWindowSizeFactor)),
			ownerService_(ownerService),
			messageAckBatchSize_(ackBatchSize),
			ackTimerRunning_(false),
			droppedMessagesInSessionCount_(0),
			receivedMessagesInSessionCount_(0)
		{
			NTSTATUS bufferStatus = messageOrderingBuffer_.Status();
			if (!NT_SUCCESS(bufferStatus))
			{
				SetConstructorStatus(bufferStatus);
				return;
			}

			NTSTATUS createStatus = KTimer::Create(batchedAckTimer_,allocator,RMSG_ALLOCATION_TAG);

			if (!NT_SUCCESS(createStatus))
			{
				SetConstructorStatus(createStatus);
			}
			
		}

		// TODO: inline?
		bool UnsafeIsBufferClosed()
		{
			return bufferState_ == Closed;
		}

		bool IsBufferClosed()
		{
			bool response = false;

			K_LOCK_BLOCK(bufferLock_)
			{
				response = UnsafeIsBufferClosed();
			}

			return response;
		}

		bool UnsafeIsBufferOpened()
		{
			return bufferState_==Opened;
		}

		Common::ErrorCode InsertAndFlush(
			__in QueuedInboundMessageSPtr const &message);

		// Used to close the inbound session in an orderly way--this can be called at most once
		// It is only valid to use this method in response to a CloseReliableSession message from the sender indicating that 
		// all messages sent so far have been acked and no further messages are coming -- we assert accordingly
		void CloseBuffer();

		// this is a best effort method used in Abort
		void ClearBuffer();

	private:

		// TODO: use the Unsafe naming convention for all methods that assume appropriate locks are held
		void UnsafeSendAcknowledgement();

		void WindowInboundRingBuffer::DropDuplicateMessage(
			__in QueuedInboundMessageSPtr const &message,
			__in Common::StringLiteral traceMessage);

		void batchedAckTimerCallback(
					KAsyncContextBase* const,           
					KAsyncContextBase& timer      
					);

		Common::ErrorCode Flush(
			__in  ULONG index);

		ULONG maxWindowSize_;
		ULONG currentWindowSize_;
		volatile FABRIC_SEQUENCE_NUMBER nextSequenceNumberToAcknowledge_;
		volatile FABRIC_SEQUENCE_NUMBER lastSequenceNumberAcknowledged_;

		volatile BufferState bufferState_;
		volatile bool sessionAborted_;
		volatile int64 droppedMessagesInSessionCount_;
		volatile int64 receivedMessagesInSessionCount_;

		ReliableSessionServiceSPtr ownerService_;

		volatile ULONG dispatchedButUnAcknowledgedMessageCount_;
		volatile ULONG bufferedMessageCount_;
		KArray<QueuedInboundMessageSPtr> messageOrderingBuffer_;
		KSpinLock bufferLock_;
		KTimer::SPtr batchedAckTimer_;
		bool ackTimerRunning_;
		ULONG messageAckBatchSize_;
	};



	class WindowOutboundRingBuffer : public KObject<WindowOutboundRingBuffer>
	{
		friend class ReliableSessionService;

	public:

		WindowOutboundRingBuffer(
				KAllocator &allocator, 
				ULONG sendOperationQuota,
				ReliableSessionServiceSPtr const &ownerService) :
			sendOpBuffer_(allocator,sendOperationQuota),
			windowSize_(0),
			bufferState_(Initialized),
			sessionAborted_(false),
			bufferClosureProcessCompleted_(false),
			closureCompleteCallback_(nullptr),
			sendOperationQuota_(sendOperationQuota),
			bufferedSendOpCount_(0),
			retryTimerRunning_(false),
			lastRetryAt_(Common::StopwatchTime::Zero),
			lastAckAt_(Common::StopwatchTime::Zero),
			numberOfRetriesSinceLastAck_(0),
			retryIntervalInMilliSeconds_(defaultContentMessageRetryIntervalInMilliSeconds),
			waitingToProcessAckThreadCount_(0),
			lastSequenceNumberAcknowledged_(-1),
			highestSequenceNumberInBuffer_(-1),
			ownerService_(ownerService)
		{
			NTSTATUS bufferStatus = sendOpBuffer_.Status();
			if (!NT_SUCCESS(bufferStatus))
			{
				SetConstructorStatus(bufferStatus);
				return;
			}

			NTSTATUS createStatus = KTimer::Create(retryTimer_,allocator,RMSG_ALLOCATION_TAG);

			if (!NT_SUCCESS(createStatus))
			{
				SetConstructorStatus(createStatus);
			}
		}

        __declspec (property(get=get_windowSize)) ULONG WindowSize;
        inline ULONG get_windowSize() const { return windowSize_; }

		bool GetFinalSequenceNumberAtClose(FABRIC_SEQUENCE_NUMBER &finalNumber)
		{
			if (IsBufferClosed())
			{
				finalNumber = highestSequenceNumberInBuffer_;
				return true;
			}
			else
			{
				finalNumber = 0;
				return false;
			}
		}

		Common::ErrorCode SendOrHold(AsyncSendOperationSPtr const &sendOp);

		void SetWindowSize(ULONG size)
		{
			// TODO: consider making this max window size and making window size dynamic
			windowSize_ = size;
		}

		// Used to cancel and clear all outstanding sendOps during abort
		void ClearBuffer(bool expectClosed);

		// Used to close the outbound buffer in cases of failed open protocol
		void CloseBuffer();

		// Used to close the outbound session in an orderly way
		void CloseBuffer(BufferClosureCompleteCallback callback);

		void ProcessMessageAck(
			__in FABRIC_SEQUENCE_NUMBER sequenceNumberAcknowledged);

		
	private:

		// helper to send a message buffer segment on ack or retry timer
		ULONG SendWindowSegment(
			__in FABRIC_SEQUENCE_NUMBER start,
			__in FABRIC_SEQUENCE_NUMBER end);

		void CompleteCloseProcess();

		// helper to clear sendOps outside lock -- the array parameter is temporary and deleted when the release is done
		static void AckReceivedMessageRelease(
			__in AsyncSendOperationSPtr *sendOpReleaseBuffer,
			__in ULONG releaseCount);

		void WindowOutboundRingBuffer::ReleaseSendops(
			__in ULONG clearSegmentStart, 
			__in ULONG clearSegmentCount);

		void retryTimerCallback(
					KAsyncContextBase* const,           
					KAsyncContextBase& timer      
					);

		ULONG sendOperationQuota_;

		bool UnsafeIsBufferClosed()
		{
			return bufferState_ == Closed;
		}

		bool IsBufferClosed()
		{
			bool response = false;

			K_LOCK_BLOCK(bufferLock_)
			{
				response = UnsafeIsBufferClosed();
			}

			return response;
		}

		bool UnsafeIsBufferOpened()
		{
			return bufferState_ == Opened;
		}

		volatile BufferState bufferState_;
		volatile bool sessionAborted_;
		volatile bool bufferClosureProcessCompleted_;
		volatile ULONG windowSize_;
		volatile FABRIC_SEQUENCE_NUMBER lastSequenceNumberAcknowledged_;
		volatile FABRIC_SEQUENCE_NUMBER highestSequenceNumberInBuffer_;

		ReliableSessionServiceSPtr ownerService_;
		volatile ULONG bufferedSendOpCount_;
		KArray<AsyncSendOperationSPtr> sendOpBuffer_;
		KSpinLock bufferLock_;
		KSpinLock ackProcessingStateLock_;
		KTimer::SPtr retryTimer_;
		volatile bool retryTimerRunning_;
		Common::StopwatchTime lastRetryAt_;
		Common::StopwatchTime lastAckAt_;
		volatile ULONG retryIntervalInMilliSeconds_;
		volatile int waitingToProcessAckThreadCount_;
		volatile int numberOfRetriesSinceLastAck_;
		BufferClosureCompleteCallback closureCompleteCallback_;
	};
}

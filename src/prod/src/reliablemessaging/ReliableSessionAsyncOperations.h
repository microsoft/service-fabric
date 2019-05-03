// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace ReliableMessaging
{
	// Needed for unknown reasons to get K_FORCE_SHARED to compile properly below
	using ::_delete;


	// If you update this, also update EraseReasonToString
	enum TransportMessageEraseReason
	{
		Unknown,
		AckReceived,
		SendCanceled,
		MaxRetryExceeded,
		SendNotStartedSuccessfully
	};

	Common::StringLiteral EraseReasonToString(TransportMessageEraseReason eraseReason);

	// This is the abstract base class for the async ops used as context in Begin/End patterns
	class SessionAsyncOperationContext : public Ktl::Com::FabricAsyncContextBase
	{
		friend class ComFabricReliableSession;

	protected:

		SessionAsyncOperationContext() : sessionContext_(nullptr) {}

		void OnReuse() override
		{
			sessionContext_ = nullptr;
			__super::OnReuse();
		}

		void SetSessionContext(ReliableSessionServiceSPtr sessionContext) 
		{
			ASSERT_IFNOT(KSHARED_IS_NULL(sessionContext_),"Setting Session Context When Context Present");
			sessionContext_ = sessionContext;
		}

		ReliableSessionServiceSPtr sessionContext_;
		KListEntry qlink_;
	};


	// this class does not work within a session context since there is not always a working session context 
	// to be found especially when an error response to a protocol request is involved
	class AsyncProtocolOperation : public Ktl::Com::FabricAsyncContextBase
	{
		K_FORCE_SHARED(AsyncProtocolOperation);

	public:

		static const ULONG LinkOffset;

		// Used by the pooling mechanism to create and grow the pool
		static Common::ErrorCode Create(
			__in KAllocator &allocator, 
			__out AsyncProtocolOperationSPtr &result)
		{
			result = _new(RMSG_ALLOCATION_TAG, allocator)
				AsyncProtocolOperation();
						 						     			 
			KTL_CREATE_CHECK_AND_RETURN(result,AsyncOperationCreateFailed,AsyncSendOperation)
		}

		// These two methods use the Environment's AsyncProtocolOperation pool
		static AsyncProtocolOperationSPtr GetOperation(ReliableMessagingEnvironmentSPtr myEnvironment);

		static void ReturnOperation(AsyncProtocolOperationSPtr& protocolOp, ReliableMessagingEnvironmentSPtr myEnvironment);

		Common::ErrorCode StartProtocolOperation(
			__in ProtocolAction::Code protocolAction, 
			__in ProtocolAction::Code expectedAckAction,
			__in Common::Guid const &sessionId,
			__in Transport::MessageUPtr &&protocolMessage, 
			__in ISendTarget::SPtr const &responseTarget,
			__in ReliableMessagingEnvironmentSPtr const &myEnvironment,
			__in int maxRetryCount = -1);


		// Processing Protocol Request Ack
		BOOLEAN CompleteProtocolOperation(
			__in ProtocolAction::Code ackAction);

	private:

		void retryTimerCallback(
					KAsyncContextBase* const,           
					KAsyncContextBase& timer);

		void SendProtocolMessage();

		void ProtocolCompletionCallback(
					KAsyncContextBase* const,           
					KAsyncContextBase& protocolOp);

		KListEntry qlink_;
		
		void OnCancel() override;
		void OnReuse() override;
		void OnStart() override
		{}
		
		volatile int retryCount_;
		int maxRetryCount_;
		Common::StopwatchTime lastSendAttemptAt_;
		Common::ErrorCode lastSendResult_;
		volatile bool protocolCompleted_;
		Common::Guid sessionId_;
		KTimer::SPtr retryTimer_;

		Transport::MessageUPtr protocolMessage_;
		ProtocolAction::Code protocolAction_;
		ProtocolAction::Code expectedAckAction_;

		ISendTarget::SPtr messageTarget_;
		ReliableMessagingEnvironmentSPtr myEnvironment_;
		// TODO: the environment should really be baked into all these pooled resources at the environment level
	};

	class AsyncSendOperation : public SessionAsyncOperationContext
	{
		K_FORCE_SHARED(AsyncSendOperation);
		friend class ReliableSessionService;
		friend class WindowOutboundRingBuffer;

	public:

		static const ULONG LinkOffset;


        __declspec (property(get=get_messageId)) ReliableMessageId const & MessageId;
        inline ReliableMessageId const & get_messageId() const { return messageId_; }

		static Common::ErrorCode Create(
			__in KAllocator &allocator, 
			__out AsyncSendOperationSPtr &result)
		{
			result = _new(RMSG_ALLOCATION_TAG, allocator)
													AsyncSendOperation();
						 						     			 
			KTL_CREATE_CHECK_AND_RETURN(result,AsyncOperationCreateFailed,AsyncSendOperation)
		}

		Common::ErrorCode StartSendOperation(    
			__in IFabricOperationData *originalMessage,
			__in_opt KAsyncContextBase* const ParentAsyncContext,
			__in_opt KAsyncContextBase::CompletionCallback CallbackPtr
			);

		// Method used to trigger completion of the async by causing transport to free the message buffers (by deleting the transport buffer vector holding them)
		// used by outbound ring buffer during ack processing in normal operation and by clear buffer processing during abort

		// This is an unsafe function, must always be called under bufferLock_ of the outboundRingBuffer_ of the relevant outbound session service
		void EraseTransportMessage(TransportMessageEraseReason eraseReason)
		{
			ASSERT_IF(KSHARED_IS_NULL(outboundMessage_),"Outbound Transport Message NULL before Ack or Cancel");
			ASSERT_IFNOT(eraseReason_==TransportMessageEraseReason::Unknown, "Attempt to erase Transport message twice");

			eraseReason_ = eraseReason;

			// Release transport message and thus allow buffers to be released by Transport

			// If a nullptr is assigned to outboundMessage_ there is a race here that causes a call to Return AsyncSendOperation::ReturnOperation
			// and from there to OnReuse before the field is actually made null due to the behavior of std::unique_pointer::reset() which calls delete 
			// before resetting _MyPtr.  That raises the small but real possibility that the sendOp gets reused before it has been reset.

			// This variant does not have that problem.

			Transport::MessageUPtr messageToDelete = std::move(outboundMessage_);
		}

		// completion callback that is triggered by transport when message buffers are actually freed (the transport buffer vector holding them is deleted)
		static void MessageBuffersFreed(std::vector<Common::const_buffer> const &buffers, void * asyncOpContext);

		bool TrySendMessage(__in Common::StopwatchTime currentTime);

	private:

		// private helper for StartSendOperation
		Common::ErrorCode CreateTransportMessage(
			__out Transport::MessageUPtr &transportMessage);
			
		Common::ErrorCode AddSessionHeader(
			__in ReliableMessageId &messageId);

		void OnStart() override;

		void OnReuse() override
		{
			ASSERT_IFNOT(KSHARED_IS_NULL(outboundMessage_),"Outbound Transport Message not NULL in AsyncSendOperation::OnReuse");

			IFabricOperationData *originalMessage = originalMessage_.DetachNoRelease();
			originalMessage->Release();

			memset(&messageId_,0,sizeof(messageId_));
			trySendResult_.Reset(Common::ErrorCodeValue::NotReady);
			lastSendAttemptAt_ = Common::StopwatchTime::Zero;
			sendTryCount_ = 0;
			eraseReason_ = TransportMessageEraseReason::Unknown;

			__super::OnReuse();
		}
			
		int sendTryCount_;
		Common::StopwatchTime lastSendAttemptAt_;

		ComPointer<IFabricOperationData> originalMessage_;
		Transport::MessageUPtr outboundMessage_;
		ReliableMessageId messageId_;
		Common::ErrorCode trySendResult_;
		TransportMessageEraseReason eraseReason_;
	};


	class AsyncReceiveOperation : public SessionAsyncOperationContext
	{
		K_FORCE_SHARED(AsyncReceiveOperation);
		friend class ReliableSessionService;

	public:

		static const ULONG LinkOffset;

		static Common::ErrorCode Create(
			__in KAllocator &allocator, 
			__out AsyncReceiveOperationSPtr &result)
		{
			result = _new(RMSG_ALLOCATION_TAG, allocator)
													AsyncReceiveOperation();
						 						     			 
			KTL_CREATE_CHECK_AND_RETURN(result,AsyncOperationCreateFailed,AsyncReceiveOperation)
		}

		Common::ErrorCode StartReceiveOperation(
			__in BOOLEAN waitForMessages,
			__in_opt KAsyncContextBase* const ParentAsyncContext,
			__in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

		IFabricOperationData* GetReceivedMessage()
		{
			return receivedMessage_.DetachNoRelease();
		}

	private:

		void OnStart() override;

		void OnReuse() override
		{
			IFabricOperationData *receivedMessage = receivedMessage_.DetachNoRelease();
			ASSERT_IFNOT(receivedMessage==nullptr,"ComPointer receivedMessage_ not empty at AsyncReceiveOperation Reuse");
			__super::OnReuse();
		}

		void AsyncReceiveOperation::ProcessQueuedInboundMessage(
			__in QueuedInboundMessageSPtr &inboundMessage);
	
		void AsyncDequeueCallback(
						KAsyncContextBase* const,           
						KAsyncContextBase& CompletingSubOp      
						);

		ComPointer<IFabricOperationData> receivedMessage_;
		bool waitForMessages_;
		QueuedInboundMessageSPtr noWaitDequeuedMessage_;
	};
}



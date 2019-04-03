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

	class ReliableSessionService : public KObject<ReliableSessionService>, public KShared<ReliableSessionService>
		
		// : public FabricAsyncDeferredClosureServiceBase
		
		//
	{
		friend class AsyncReceiveOperation;
		friend class AsyncOutboundOpenOperation;
		friend class AsyncOutboundCloseOperation;

		K_FORCE_SHARED(ReliableSessionService)

		enum SessionState {
			Initialized = 100,
			Opening,					// API or protocol to initiate Open was accepted 
			Opened,						// Open process completed successfully
			Closing,					// API or wire request to initiate Close was accepted -- ring buffer closed
			SendBufferDrained,			// (outbound only) the ring buffer has been drained after all messages were acknowledged: ready to send closure message
			Closed,						// Closing process completed: closure message dequeued or CloseResponse wire message received
			Unknown						// Used when needed to initialize local state variable
		};

	public:

        __declspec (property(get=get_traceId)) std::wstring const &TraceId;
        inline std::wstring const &get_traceId() const { return traceId_; }

		// if you update this update the SignalAsString method below as well
		enum SessionSignal {
			OpenApiCalled = 200,
			OpenApiInternalFailure,
			OpenRequestMessage,
			OpenResponseSuccessMessage,
			OpenResponseFailureMessage,
			SessionOfferAccepted,
			SessionOfferNotAccepted,	// could be offer callback failure or could be actual rejection
			CloseApiCalled,
			AllMessagesAcknowledged,
			CloseApiInternalFailure,
			CloseRequestMessageReceived, 
			CloseRequestMessageDequeued,
			CloseResponseSuccessMessage,
			CloseResponseFailureMessage,
			AbortApiCalled,
			AbortOutboundSessionRequestMessage,
			AbortOutboundSessionOnInvalidStateAck,
			AbortInboundSessionRequestMessage,
			AbortConflictingInboundSession,
			AbortSessionForManagerClose
		};

		Common::StringLiteral SignalAsString(SessionSignal signal)
		{
			switch (signal)
			{
			case OpenApiCalled: return "OpenApiCalled";
			case OpenApiInternalFailure: return "OpenApiInternalFailure";
			case OpenRequestMessage: return "OpenRequestMessage";
			case OpenResponseSuccessMessage: return "OpenResponseSuccessMessage";
			case OpenResponseFailureMessage: return "OpenResponseFailureMessage";
			case SessionOfferAccepted: return "SessionOfferAccepted";
			case SessionOfferNotAccepted: return "SessionOfferNotAccepted";
			case CloseApiCalled: return "CloseApiCalled";
			case AllMessagesAcknowledged: return "AllMessagesAcknowledged";
			case CloseApiInternalFailure: return "CloseApiInternalFailure";
			case CloseRequestMessageReceived: return "CloseRequestMessageReceived"; 
			case CloseRequestMessageDequeued: return "CloseRequestMessageDequeued";
			case CloseResponseSuccessMessage: return "CloseResponseSuccessMessage";
			case CloseResponseFailureMessage: return "CloseResponseFailureMessage";
			case AbortApiCalled: return "AbortApiCalled";
			case AbortOutboundSessionRequestMessage: return "AbortOutboundSessionRequestMessage";
			case AbortOutboundSessionOnInvalidStateAck: return "AbortOutboundSessionOnInvalidStateAck";
			case AbortInboundSessionRequestMessage: return "AbortInboundSessionRequestMessage";
			case AbortConflictingInboundSession: return "AbortConflictingInboundSession";
			case AbortSessionForManagerClose: return "AbortSessionForManagerClose";
			default: Common::Assert::CodingError("ReliableSessionService::SignalAsString called with unknown signal {0}", (int)signal);
			}
		}

	private:

		/***
		This is the state machine manager.  It is invoked at the entry point of every signal to the session.
		The method will atomically

		A. Validate that the signal is acceptable in the current state
		B. Change the current state if the signal implies it

		The method return true if the signal was accepted, otherwise returns false
		It also returns, through the out parameter, the state as it is at the end of the (atomic part of) the method execution

		This is an unsafe method.  It must be called while holding the sessionServiceInstanceLock_.  
		The reason being that the state transitions involved often require other session level state changes to occur atomically.

		TODO: consider separating the state checks from the state transitions and doing the former in a safe method.
		***/
		bool UnsafeProcessSessionSignal(
			__in SessionSignal signal,
			__out SessionState &currentState);

		/*** State check functions ***/

		bool UnsafeIsClosed()
		{
			return (sessionState_ == Closed);
		}

		bool UnsafeWasClosedNormally()
		{
			return normalCloseOccurred_;
		}

		bool UnsafeIsOpenForReceive()
		{
			return (sessionState_ == Opening || sessionState_ == Opened || sessionState_ == Closing);
		}

	public:

		// Used in constructing a NOFAIL KNodeHashTable map of ReliableSessionService objects with session ID keys
		static const ULONG LinkOffset;
		static const ULONG KeyOffset;

		bool IsOpened()
		{
			bool result = false;

			// another opportunity to use a read-write lock: the lock ensures atomic state transitions and reads
			// Need to lock to ensure that the rest of the session state corresponds to the sessionState_ marker
			K_LOCK_BLOCK(sessionServiceInstanceLock_)
			{
				result = (sessionState_ == Opened);
			}

			return result;
		}

		bool IsOpenForReceive()
		{
			bool result = false;

			// another opportunity to use a read-write lock: the lock ensures atomic state transitions and reads
			// Need to lock to ensure that the rest of the session state corresponds to the sessionState_ marker
			K_LOCK_BLOCK(sessionServiceInstanceLock_)
			{
				result = UnsafeIsOpenForReceive();
			}

			return result;
		}

		void GetDataSnapshot(SESSION_DATA_SNAPSHOT &snapshot)
		{
			K_LOCK_BLOCK(sessionServiceInstanceLock_)
			{
				snapshot.InboundMessageQuota = queuedInboundMessageQuota_;
				snapshot.InboundMessageCount = queuedInboundMessageCount_;
				snapshot.SendOperationQuota = asyncSendOperationQuota_;
				snapshot.SendOperationCount = asyncSendOperationCount_;
				snapshot.ReceiveOperationQuota = asyncReceiveOperationQuota_;
				snapshot.ReceiveOperationCount = asyncReceiveOperationCount_;
				snapshot.IsOutbound = sessionKind_ == Outbound ? TRUE : FALSE;
				snapshot.SessionId = sessionId_.AsGUID();
				snapshot.NormalCloseOccurred = normalCloseOccurred_ ? TRUE : FALSE;
				snapshot.IsOpenForSend = sessionState_ == Opened &&  sessionKind_ == Outbound ? TRUE : FALSE;
				snapshot.IsOpenForReceive = UnsafeIsOpenForReceive() && sessionKind_ == Inbound ? TRUE : FALSE;
				snapshot.LastOutboundSequenceNumber = outboundRingBuffer_.highestSequenceNumberInBuffer_;
				snapshot.FinalInboundSequenceNumberInSession = finalSequenceNumberInSession_;
				snapshot.reserved = nullptr;
			}
		}


		// We have both inbound and outbound structures in all session service objects, but only the appropriate ones are initialized with resources
		// We anticipate a future generalization to duplex sessions

		ReliableSessionService(
							ReliableMessagingServicePartitionSPtr const &sourceServicePartition, 
							ReliableMessagingServicePartitionSPtr const &targetServicePartition, 
							SessionManagerServiceSPtr const &ownerSessionManager,
							ISendTarget::SPtr const &partnerSendTarget,
							SessionKind sessionKind,
							Common::Guid sessionId,
							ReliableMessagingEnvironmentSPtr myEnvironment,
							LONG queuedInboundMessageQuota = sessionQueuedInboundMessageQuota,
							LONG asyncSendOperationQuota = sessionAsyncSendOperationQuota,
							LONG asyncReceiveOperationQuota = sessionAsyncReceiveOperationQuota
							);

		__declspec(property(get=get_sourceServicePartition)) ReliableMessagingServicePartitionSPtr const & SourceServicePartition;
        ReliableMessagingServicePartitionSPtr const & get_sourceServicePartition() const { return sourceServicePartition_; }

		__declspec(property(get=get_targetServicePartition)) ReliableMessagingServicePartitionSPtr const & TargetServicePartition;
        ReliableMessagingServicePartitionSPtr const & get_targetServicePartition() const { return targetServicePartition_; }

		__declspec(property(get=get_sessionId)) Common::Guid const & SessionId;
		Common::Guid const & get_sessionId() const { return sessionId_; }

		__declspec(property(get=get_sessionKind)) bool IsOutbound;
		bool get_sessionKind() const { return sessionKind_ == Outbound; }

		// this is not a constant, is subject to races, but once set to true it stays set to true
		__declspec(property(get=get_normalClosureStatus)) bool WasClosedNormally;
		bool get_normalClosureStatus() const { return normalCloseOccurred_ == true; }

		__declspec(property(get=get_Environment)) ReliableMessagingEnvironmentSPtr const & Environment;
		ReliableMessagingEnvironmentSPtr const & get_Environment() const { return myEnvironment_; }



		/*** COM API Entry Points ***/

		Common::ErrorCode ReliableSessionService::StartOpenSessionOutbound(
			__in AsyncOutboundOpenOperationSPtr asyncOpenOp,
			__in_opt KAsyncContextBase* const ParentAsync, 
			__in KAsyncContextBase::CompletionCallback OpenCallbackPtr);

		// Implementation of StartCloseSession for outbound case
		Common::ErrorCode StartCloseSessionOutbound(
			__in AsyncOutboundCloseOperationSPtr asyncOpenOp,
			__in_opt KAsyncContextBase* const ParentAsync, 
			__in KAsyncContextBase::CompletionCallback CloseCallbackPtr);
	

		//--------------------------------------------------------------------------
		// Function: AbortSession
		//
		// Description:
		// Rudely close the session protocol and release all resources
		// This is meaningful for both inbound and outbound sessions
		// Abort and Close are alternatives: Abort is an abrupt version of Close
		// If Close has been started on an outbound session service Abort is disabled
		// Abort is the only version of Close available for inbound services at the API level
		//

		// AbortSession is idempotent

		// Return codes:
		//	Common::ErrorCodeValue::Success
		//	FABRIC_E_INVALID_OPERATION -- if the session is already closed
		//
        Common::ErrorCode AbortSession(SessionSignal abortSignal);

		/*** End COM API Entry Points ***/
		

		/*** Session Manager Entry Points ***/

		Common::ErrorCode StartOpenSessionInbound();


		// starts the closure process including posting a close session message to the inbound queue
		Common::ErrorCode StartCloseSessionInbound(
			__in ReliableSessionHeader const &sessionHeader);

		/*** End Session Manager Entry Points ***/

		/*** ReliableMessagingTransport Entry Points ***/

		// Processing content message Ack
		void ProcessMessageAck(
			__in FABRIC_SEQUENCE_NUMBER sequenceNumberAcknowledged,
			__in Common::ErrorCode result)
		{
			ASSERT_IFNOT(result.IsSuccess() || result.ReadValue()==Common::ErrorCodeValue::InvalidState, 
											 "Unexpected SendReliableMessageAck result");

			// there is a case where the result is InvalidState: exactly if the Insert into the inbound buffer failed on the 
			// receiving side because that session was aborted
			// ReliableSessionNotFound must not occur given the current design: Aborted inbound sessions leave tombstones

			// the cleanest thing to do if the result is InvalidState is to abort the session: many parallel things could go
			// on beyond this point including acks and nacks but they are pointless and hard to process correctly
			// since the normal order of things has been destroyed

			// We treat this as equivalent to an AbortOutboundSessionRequestMessage, which may of course also arrive or has already arrived
			// but we use a different AbortOutboundSessionOnInvalidStateAck signal for tracing clarity
			// Abort often being a distress situation we don't count on it, which is also why Abort is idempotent

			if (result.ReadValue()==Common::ErrorCodeValue::InvalidState)
			{
				AbortSession(AbortOutboundSessionOnInvalidStateAck);
				return;
			}		

			outboundRingBuffer_.ProcessMessageAck(sequenceNumberAcknowledged);
		}

		// Processing OpenSession Response 
		static void CompleteOpenSessionProtocol(
			__in Common::Guid sessionId, 
			__in Common::ErrorCode result, 
			__in ReliableSessionParametersHeader const &sessionParametersHeader,
			__in ReliableMessagingEnvironmentSPtr const &myEnvironment);

		// Processing CloseSession Response
		static void CompleteCloseSessionProtocol(
			__in Common::Guid sessionId, 
			__in Common::ErrorCode result, 
			__in ReliableMessagingEnvironmentSPtr const &myEnvironment);

		/*** End ReliableMessagingTransport Entry Points ***/


		/*** Callback Entry Points ***/

		// From AsyncReceiveOperation processing close session message in the queue signifying there no more messages in the session
		void ContinueCloseSessionInbound();

		// From outbound ring buffer when all outstanding outbound messages have been acked during closure process
		void OnOutboundBufferClosureCompleted();

		/*** End Callback Entry Points ***/

		/*** Helpers for protocol execution used by this class and friend classes ***/

		// helper for AsyncSendOperation: accessing the private outboundRingBuffer
		Common::ErrorCode InsertIntoSendBuffer(AsyncSendOperationSPtr const &sendOp);

		// helper to send message to current session partner endpoint
		Common::ErrorCode Send(Transport::MessageUPtr &&message);

		// helper to post an incoming message from transport to inbound buffer
		Common::ErrorCode InsertInbound(
			__in QueuedInboundMessageSPtr const &sendMsgSPtr);

		// helper to post an incoming message from inbound buffer to delivery queue
		Common::ErrorCode Enqueue(
			__in QueuedInboundMessageSPtr const &sendMsgSPtr);

		// helper to perform no wait dequeue on delivery queue
		Common::ErrorCode TryDequeue(
			__out QueuedInboundMessageSPtr& queuedMessage);

		Common::ErrorCode SendProtocolRequestMessage(
				__in ProtocolAction::Code actionCode, 
				__in ProtocolAction::Direction direction,
				__in int maxRetryCount = -1);

		// helper for inbound ring buffer to send a batched message ack
		Common::ErrorCode AcknowledgeMessageNumber(
			__in FABRIC_SEQUENCE_NUMBER sequenceNumberToAcknowledge);

		/*** End Helpers for protocol execution used by this class and friend classes ***/

		/*** Session context factories ***/

		// Factory for Session level dequeue operations with an outstanding receive as context
		Common::ErrorCode CreateDequeueOperation(
			__in AsyncReceiveOperationSPtr const &receiveOp,	// outstanding receive
			__out InboundMessageQueue::DequeueOperation::SPtr& result);

		// Some protocol response messages like SendReliableMessageAck need only messageId, no service partition headers
		// this is a functional response and will be retried if not acked
		static Common::ErrorCode SendProtocolResponseMessage(
			__in ProtocolAction::Code actionCode, 
			__in ReliableMessageId const &messageId,
			__in Common::ErrorCode response,
			__in ISendTarget::SPtr const &responseTarget,
			__in ReliableMessagingEnvironmentSPtr myEnvironment,
			__in ReliableMessagingServicePartitionSPtr const fromServicePartition = nullptr,
			__in ReliableMessagingServicePartitionSPtr const toServicePartition = nullptr
			);

		// this is a simple Ack/Nack with no retry: also used for batched ordinary message Acks and Nacks for SendReliableMessage
		static Common::ErrorCode SendAck(
			__in ProtocolAction::Code actionCode, 
			__in ReliableMessageId const &messageId,
			__in Common::ErrorCode response,
			__in ISendTarget::SPtr const &responseTarget,
			__in ReliableMessagingEnvironmentSPtr myEnvironment);

		/*** End static session context independent factory and protocol helper that are also useful in error situations where session context does not exist ***/


		/*** Pooled resource allocation methods with optional quota enforcement ***/

		// These two methods use the Environment's QueuedInboundMessage pool subject to the session quota
		QueuedInboundMessageSPtr GetQueuedInboundMessage(bool applyQuota=true);
		void ReturnQueuedInboundMessage(QueuedInboundMessageSPtr& message);

		// These two methods use the Environment's AsyncReceiveOperation pool subject to the session quota
		AsyncReceiveOperationSPtr GetAsyncReceiveOperation(bool applyQuota=true);
		void ReturnAsyncReceiveOperation(AsyncReceiveOperationSPtr& message);

		// These two methods use the Environment's AsyncSendOperation pool subject to the session quota
		AsyncSendOperationSPtr GetAsyncSendOperation();
		void ReturnAsyncSendOperation(AsyncSendOperationSPtr& message);

		// Helper to calculate the right Ack code for protocol messages
		static ProtocolAction::Code AckCodeFor(ProtocolAction::Code actionCode)
		{
			ProtocolAction::Code ackCode = ProtocolAction::Unknown;

			switch (actionCode)
			{
			case ProtocolAction::OpenReliableSessionRequest: 
				{
					ackCode = ProtocolAction::OpenReliableSessionRequestAck;
					break;
				}
			case ProtocolAction::OpenReliableSessionResponse: 
				{
					ackCode = ProtocolAction::OpenReliableSessionResponseAck;
					break;
				}
			case ProtocolAction::CloseReliableSessionRequest: 
				{
					ackCode = ProtocolAction::CloseReliableSessionRequestAck;
					break;
				}
			case ProtocolAction::CloseReliableSessionResponse: 
				{
					ackCode = ProtocolAction::CloseReliableSessionResponseAck;
					break;
				}
			case ProtocolAction::SendReliableMessage:
				{
					ackCode = ProtocolAction::SendReliableMessageAck;
					break;
				}

			case ProtocolAction::AbortOutboundReliableSessionRequest:
				{
					ackCode = ProtocolAction::AbortOutboundReliableSessionRequestAck;
					break;
				}

			case ProtocolAction::AbortInboundReliableSessionRequest:
				{
					ackCode = ProtocolAction::AbortInboundReliableSessionRequestAck;
					break;
				}
			}

			return ackCode;
		}

	private:

		// Factory to create a session protocol transport message (as opposed to message with client supplied data)
		Common::ErrorCode CreateProtocolMessage(
			__in ProtocolAction::Code actionCode, 
			__in ProtocolAction::Direction direction,
			__out Transport::MessageUPtr &message);

		/*** End Session context factories ***/

		/*** static session context independent factory and protocol helper that are also useful in error situations where session context does not exist ***/

		// Factory for constructing protocol messages like open and close and their acks
		// source and partition headers are added only when those parameters are not null
		static Common::ErrorCode CreateProtocolMessage(
			__in ProtocolAction::Code actionCode, 
			__in ReliableMessageId const &messageId,
			__out Transport::MessageUPtr &message,
			__in ReliableMessagingServicePartitionSPtr sourceServicePartition = nullptr,
			__in ReliableMessagingServicePartitionSPtr targetServicePartition = nullptr
			);

		// Instance method for processing OpenSessionAck message in waiting session service
		BOOLEAN CompleteOpenSessionProtocol(
			__in Common::ErrorCode result,
			__in ULONG maxWindowSize = 0);

		// Instance method for processing CloseSessionAck message in waiting session service
		BOOLEAN CompleteCloseSessionProtocol(
			__in Common::ErrorCode result);

		// Instance method for cleaning up in the context of failure or Abort of the session
		bool CloseAndCleanupInboundSession(SessionSignal cleanupSignal);

		// Instance method for cleaning up in the context of failure or Abort of the session
		bool CloseAndCleanupOutboundSession(SessionSignal cleanupSignal);

		// premature termination helper for open session process in case of resource or transport failures
		BOOLEAN FailedOpenSessionOutbound(
			__in Common::ErrorCode result);

		// premature termination helper for close session process in case of resource or transport failures
		BOOLEAN FailedCloseSessionOutbound(
			__in Common::ErrorCode result);

		Common::ErrorCode SendOpenSessionResponseMessage(
			__in Common::ErrorCode response);

		// Internal helper : not safe to call StopQueue more than once
		void StopQueue();

		// thread switching work item wrapper for inbound session requested callback to owner service
		void InboundSessionRequestedCallbackAndContinuation();

		/*** Template methods for pooled resource management ***/

		template <class ItemType>
		void GetItemFromQuota(LONG &itemCount, LONG itemQuota, KSharedPtr<ItemType>& itemToGet)
		{
			bool okToGet = false;
			itemToGet = nullptr;

			K_LOCK_BLOCK(sessionServiceInstanceLock_)
			{
				if (itemCount < itemQuota)
				{
					InterlockedIncrement(&itemCount);
					okToGet = true;
				}
			}

			if (okToGet)
			{
				myEnvironment_->GetPooledResource(itemToGet);

				if (itemToGet==nullptr)
				{
					InterlockedDecrement(&itemCount);
				}
			}
			else
			{
				// DbgBreakPoint();
			}
		}

		template <class ItemType>
		void ReturnItemToQuota(LONG &itemCount, KSharedPtr<ItemType>& itemToReturn)
		{
			InterlockedDecrement(&itemCount);
			myEnvironment_->ReturnPooledResource(itemToReturn);
		}

		/*** End Template methods for pooled resource management ***/

		/*** Async Service event processors ***/

		void OnServiceOpenOutbound();

		/*** End async Service event processors ***/

		/*** data members ***/

		LONG queuedInboundMessageQuota_;
		LONG queuedInboundMessageCount_;

		LONG asyncSendOperationQuota_;
		LONG asyncSendOperationCount_;

		LONG asyncReceiveOperationQuota_;
		LONG asyncReceiveOperationCount_;

		const SessionKind sessionKind_;

		const ReliableMessagingServicePartitionSPtr sourceServicePartition_;
		const ReliableMessagingServicePartitionSPtr targetServicePartition_;
		SessionManagerServiceSPtr ownerSessionManager_;

		// async operations to manage outbound open and close processes
		AsyncOutboundOpenOperationSPtr openOp_;
		AsyncOutboundCloseOperationSPtr closeOp_;

		// is constant but cannot be declared const because it is used as KAvlTree key
		KListEntry qlink_;
		Common::Guid sessionId_;
		volatile FABRIC_SEQUENCE_NUMBER finalSequenceNumberInSession_;

		ISendTarget::SPtr partnerSendTarget_;

		KSpinLock sessionServiceInstanceLock_;

		// queue for holding and dispatching inbound messages demuxed to current session
		InboundMessageQueueSPtr inboundAsyncSessionQueue_;

		WindowInboundRingBuffer inboundRingBuffer_;
		WindowOutboundRingBuffer outboundRingBuffer_;
		ReliableMessagingEnvironmentSPtr myEnvironment_;

		std::wstring traceId_;

		SessionState sessionState_;
		bool normalCloseOccurred_;
		bool asyncQueueDeactivated_;

		/*** end data members ***/

	};

}

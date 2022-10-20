// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
 
#include "stdafx.h"


namespace ReliableMessaging
{
	const ULONG ReliableSessionService::LinkOffset = FIELD_OFFSET(ReliableSessionService, qlink_);
	const ULONG ReliableSessionService::KeyOffset = FIELD_OFFSET(ReliableSessionService, sessionId_);

	K_FORCE_SHARED_NO_OP_DESTRUCTOR(ReliableSessionService)

/*** Methods for acquiring and releasing pooled resources ***/
#pragma region PooledResourceMethods
	QueuedInboundMessageSPtr ReliableSessionService::GetQueuedInboundMessage(bool applyQuota)
	{
		QueuedInboundMessageSPtr message = nullptr;
		if (applyQuota)
		{
			RMTRACE->GetFromQuotaQueuedInboundMessage(sessionId_,queuedInboundMessageCount_,queuedInboundMessageQuota_);
			GetItemFromQuota(queuedInboundMessageCount_,queuedInboundMessageQuota_,message);
		}
		else
		{
			LONG dummyCount = 0;
			GetItemFromQuota(dummyCount,queuedInboundMessageQuota_,message);
		}
		return message;
	}

	void ReliableSessionService::ReturnQueuedInboundMessage(QueuedInboundMessageSPtr& message)
	{
		ReturnItemToQuota(queuedInboundMessageCount_,message);
		RMTRACE->ReturnToQuotaQueuedInboundMessage(sessionId_,queuedInboundMessageCount_,queuedInboundMessageQuota_);
	}

	AsyncSendOperationSPtr ReliableSessionService::GetAsyncSendOperation()
	{
		RMTRACE->GetFromQuotaAsyncSendOperation(sessionId_,asyncSendOperationCount_,asyncSendOperationQuota_);
		AsyncSendOperationSPtr sendOp = nullptr;
		GetItemFromQuota(asyncSendOperationCount_,asyncSendOperationQuota_,sendOp);

		if (sendOp != nullptr)
		{
			sendOp->SetSessionContext(KSharedPtr<ReliableSessionService>(this));
		}

		return sendOp;
	}

	void ReliableSessionService::ReturnAsyncSendOperation(AsyncSendOperationSPtr& sendOp)
	{
		RMTRACE->ReturnToQuotaAsyncSendOperation(sessionId_,asyncSendOperationCount_,asyncSendOperationQuota_);
		ReturnItemToQuota(asyncSendOperationCount_,sendOp);
	}

	AsyncReceiveOperationSPtr ReliableSessionService::GetAsyncReceiveOperation(bool applyQuota)
	{
		AsyncReceiveOperationSPtr receiveOp = nullptr;

		if (applyQuota)
		{
			RMTRACE->GetFromQuotaAsyncReceiveOperation(sessionId_,asyncReceiveOperationCount_,asyncReceiveOperationQuota_);
			GetItemFromQuota(asyncReceiveOperationCount_,asyncReceiveOperationQuota_,receiveOp);
		}
		else
		{
			LONG dummyCount = 0;
			GetItemFromQuota(dummyCount,asyncReceiveOperationQuota_,receiveOp);
		}

		if (receiveOp != nullptr)
		{
			receiveOp->SetSessionContext(KSharedPtr<ReliableSessionService>(this));
		}

		return receiveOp;
	}

	void ReliableSessionService::ReturnAsyncReceiveOperation(AsyncReceiveOperationSPtr& receiveOp)
	{
		RMTRACE->ReturnToQuotaAsyncReceiveOperation(sessionId_,asyncReceiveOperationCount_,asyncReceiveOperationQuota_);
		ReturnItemToQuota(asyncReceiveOperationCount_,receiveOp);
	}  
#pragma endregion
/*** End methods for acquiring and releasing pooled resources ***/

/*** State transition mechanism ***/
	bool ReliableSessionService::UnsafeProcessSessionSignal(
			__in SessionSignal signal,
			__out SessionState &currentState)
	{
		bool signalSuccess = false;

		switch (signal)
		{
		case OpenApiCalled:	// outbound case
			if (sessionState_==Initialized)
			{
				//  inbound state goes directly to Opened ready to be offered in active state to the receive-side service
				sessionState_ = Opening;
				signalSuccess = true;
			}
			break;

		case OpenRequestMessage:	// inbound case
			if (sessionState_==Initialized)
			{
				sessionState_ = Opening;
				signalSuccess = true;
			}
			break;

		case SessionOfferAccepted:	// inbound case
			if (sessionState_==Opening)
			{
				sessionState_ = Opened;
				signalSuccess = true;
			}
			break;

		case SessionOfferNotAccepted: // inbound case: Offer rejected or offer callback failed
			if (sessionState_==Opened)
			{
				sessionState_ = Closed;
				signalSuccess = true;
			}
			break;

		case OpenResponseSuccessMessage:	// outbound case
			if (sessionState_==Opening)
			{
				sessionState_ = Opened;
				signalSuccess = true;
			}
			break;

		case OpenResponseFailureMessage:	// outbound cases
		case OpenApiInternalFailure:
			if (sessionState_==Opening)
			{
				sessionState_ = Closed;
				signalSuccess = true;
			}
			break;

		case CloseApiCalled:	// outbound case
			if (sessionState_==Opened)
			{
				sessionState_ = Closing;
				signalSuccess = true;
			}
			break;

		case AllMessagesAcknowledged:	// outbound case
			if (sessionState_==Closing)
			{
				sessionState_ = SendBufferDrained;
				signalSuccess = true;
			}
			break;

		case CloseRequestMessageReceived:	// inbound case
			if (sessionState_==Opened)
			{
				sessionState_ = Closing;
				signalSuccess = true;
			}
			break;

		case CloseRequestMessageDequeued:	// inbound case
			if (sessionState_==Closing)
			{
				sessionState_ = Closed;
				signalSuccess = true;
			}
			break;

		case CloseResponseSuccessMessage:	// outbound cases
		case CloseResponseFailureMessage:
		case CloseApiInternalFailure:
			if (sessionState_==SendBufferDrained)
			{
				sessionState_ = Closed;
				signalSuccess = true;
			}
			break;

		case AbortApiCalled:	// inbound or outbound case
		case AbortInboundSessionRequestMessage:
		case AbortOutboundSessionRequestMessage:
		case AbortOutboundSessionOnInvalidStateAck:
		case AbortConflictingInboundSession:
		case AbortSessionForManagerClose:
			if (sessionState_!=Closed)
			{
				sessionState_ = Closed;
				signalSuccess = true;
			}
			break;
		}

		currentState = sessionState_;

		return signalSuccess;
	}
/*** End state transition mechanism ***/

/*** Constructor ***/
	ReliableSessionService::ReliableSessionService(
						ReliableMessagingServicePartitionSPtr const &sourceServicePartition, 
						ReliableMessagingServicePartitionSPtr const &targetServicePartition, 
						SessionManagerServiceSPtr const &ownerSessionManager,
						ISendTarget::SPtr const &partnerSendTarget,
						SessionKind sessionKind,
						Common::Guid sessionId,
						ReliableMessagingEnvironmentSPtr myEnvironment,
						LONG queuedInboundMessageQuota,
						LONG asyncSendOperationQuota,
						LONG asyncReceiveOperationQuota
						) 
							:	sourceServicePartition_(sourceServicePartition),
								targetServicePartition_(targetServicePartition),
								ownerSessionManager_(ownerSessionManager),
								partnerSendTarget_(partnerSendTarget),
								finalSequenceNumberInSession_(-1), // watch out for the 0 messages in session before close edge case
								sessionId_(sessionId),
								sessionKind_(sessionKind),
								inboundAsyncSessionQueue_(nullptr),
								myEnvironment_(myEnvironment),
								queuedInboundMessageQuota_(queuedInboundMessageQuota),
								queuedInboundMessageCount_(0),
								asyncSendOperationQuota_(asyncSendOperationQuota),
								asyncSendOperationCount_(0),
								asyncReceiveOperationQuota_(asyncReceiveOperationQuota),
								asyncReceiveOperationCount_(0),
								traceId_(ownerSessionManager->TraceId),
								sessionState_(Initialized),
								normalCloseOccurred_(false),
								asyncQueueDeactivated_(false),
								inboundRingBuffer_(
										GetThisAllocator(), 
										sessionKind==SessionKind::Inbound ? sessionQueuedInboundMessageQuota : 0, 
										maxMessageAckBatchSize,
										KSharedPtr<ReliableSessionService>(this)),									
								outboundRingBuffer_(
										GetThisAllocator(), 
										sessionKind==SessionKind::Outbound ? asyncSendOperationQuota : 0, 
										KSharedPtr<ReliableSessionService>(this))
	{
		if (sessionKind==SessionKind::Inbound)
		{
			Common::ErrorCode result = InboundMessageQueue::Create(inboundAsyncSessionQueue_);

			if (!result.IsSuccess())
			{
				RMTRACE->FailureCompletionFromInternalFunction(traceId_,"ReliableSessionService::Ctor",result.ErrorCodeValueToString());	
				SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
			}
		}
	}
/*** End Constructor ***/

/*** Methods for Starting an inbound session ***/
#pragma region InboundOpenSessionFunctionality
	Common::ErrorCode ReliableSessionService::StartOpenSessionInbound()
	{
		RMTRACE->AsyncOpenInboundSessionStarted(traceId_,SessionId);

		bool signalSuccess = false;

		K_LOCK_BLOCK_EX(sessionServiceInstanceLock_, lock1)
		{
			SessionState nextState;
			signalSuccess = UnsafeProcessSessionSignal(OpenRequestMessage,nextState);

			if (signalSuccess)
			{
				ASSERT_IFNOT(nextState==Opening, "Unexpected next state from successful OpenRequestMessage signal");

				// Ignoring status from Activate since it is always STATUS_PENDING inless there is an assert fired
				inboundAsyncSessionQueue_->Activate(nullptr,nullptr);

				// finish OpenSession process

				bool insertSuccess = false;

				K_LOCK_BLOCK_EX(myEnvironment_->sessionServiceCommonLock_, lock2)
				{
					insertSuccess = myEnvironment_->commonReliableInboundSessionsById_.Insert(this);
				}

				if (insertSuccess)
				{
					// call service to offer the session, but on a different thread, release the service instance lock and the KTL thread
					// open session can still fail if the receiving service rejects the session -- the continuation below will send the appropriate response
					Common::Threadpool::Post([this](){ this->InboundSessionRequestedCallbackAndContinuation();});
				}
				else
				{
					// Insertion in the underlying KNodeHashTable does not require allocation, only a collision causes failure
					Common::Assert::CodingError("Unexpected duplicate for newly accepted inbound session found, SessionId: {0}", this->SessionId.ToStringA());
				}
			}
		}

		if (!signalSuccess)
		{
			Common::ErrorCode errorCode = Common::ErrorCodeValue::InvalidState;
			RMTRACE->OpenSessionInboundCompleting(traceId_,SessionId,errorCode.ErrorCodeValueToString());
			return Common::ErrorCodeValue::InvalidState;
		}
		else
		{
			return Common::ErrorCodeValue::Success;
		}
	}

	// We now finish the job by asking the callee service to accept the session and depending on the response let the caller know the result
	// Note that this work is done on a regular thread pool thread not on a KTL thread since we are calling into service code
	void ReliableSessionService::InboundSessionRequestedCallbackAndContinuation()
	{
		// At this point the service status is Opened, the Queue has been started the session is fully functional
		// There is nothing stopping the callee from successfully parking receives on this session even before it is accepted
		// This is as it should be since there is no further protocol to let the callee know that the session is ready

		// call the service to offer the opened session
		// TODO: what if this callback faults?  catch it
		LONG sessionAccepted;
		Common::ErrorCode callbackResult = ownerSessionManager_->SessionRequestedCallback(SourceServicePartition,KSharedPtr<ReliableSessionService>(this),sessionAccepted);

		if (callbackResult.IsSuccess() && !sessionAccepted)
		{
			callbackResult = Common::ErrorCodeValue::ReliableSessionRejected;
		}

		if (!callbackResult.IsSuccess())
		{
			bool cleanupSuccess = CloseAndCleanupInboundSession(SessionOfferNotAccepted);

			if (cleanupSuccess)
			{
				RMTRACE->OpenSessionInboundCompleting(traceId_,SessionId, callbackResult.ErrorCodeValueToString());
				SendOpenSessionResponseMessage(callbackResult);
				return;
			}
		}
		else
		{
			bool signalSuccess = false;
			SessionState nextState;

			K_LOCK_BLOCK(sessionServiceInstanceLock_)
			{
				// anything could have happened, the service may have thrown an exception, it may even abort the session during the acceptance call
				signalSuccess = UnsafeProcessSessionSignal(SessionOfferAccepted,nextState);

				if (signalSuccess)
				{
					ASSERT_IFNOT(nextState==Opened, "Unexpected next state from successful SessionOfferAccepted signal");
				}
			} // release instance lock

			if (signalSuccess)
			{
				// finally the success case!
				Common::ErrorCode code = Common::ErrorCodeValue::Success;
				RMTRACE->OpenSessionInboundCompleting(traceId_,SessionId, code.ErrorCodeValueToString());
				SendOpenSessionResponseMessage(Common::ErrorCodeValue::Success);
			}
		}
	}  
#pragma endregion
/*** End methods for Starting an inbound session ***/

/*** Methods for Closing an inbound session ***/
#pragma region InboundCloseSessionFunctionality
	// This method runs on a separate thread initiated by the session manager 
	Common::ErrorCode ReliableSessionService::StartCloseSessionInbound(
		__in ReliableSessionHeader const &sessionHeader)
	{
		RMTRACE->AsyncCloseInboundSessionStarted(traceId_,sessionHeader.SessionId);

		ASSERT_IFNOT(SessionId==sessionHeader.SessionId,"StartCloseSessionInbound called for the wrong session ID");

		bool signalSuccess = false;

		// get ready to insert closure message into the queue
		// get a blank message ignoring quota
		QueuedInboundMessageSPtr closureMessage = GetQueuedInboundMessage(false);

		if (closureMessage==nullptr)
		{
			// TODO: we are truly out of resources, notify Fabric -- asserting for now
			Common::Assert::CodingError("Unable to create close message for session");
		}

		closureMessage->InitializeClosureMessage(sessionHeader);


		// ready to try the state transition
		K_LOCK_BLOCK(sessionServiceInstanceLock_)
		{
			SessionState nextState;
			signalSuccess = UnsafeProcessSessionSignal(CloseRequestMessageReceived,nextState);

			if (signalSuccess)
			{
				ASSERT_IFNOT(nextState==Closing, "Unexpected next state from successful  CloseRequestMessageReceived signal");
				// Normal case, no races or retries, do the essential things under lock
				// Close the inbound buffer and create and insert the end-of-session message into the queue
				inboundRingBuffer_.CloseBuffer();

				ASSERT_IF(finalSequenceNumberInSession_ >= 0, 
					"Unexpected finalSequenceNumberInSession_ set previously in first successful pass through StartCloseSessionInbound");
				// Use the sequence number of the CloseSession message to cap this session
				finalSequenceNumberInSession_ = sessionHeader.SequenceNumber;

				// if we lose a race with Close/Abort we will get a status of K_STATUS_SHUTDOWN_PENDING
				NTSTATUS status = inboundAsyncSessionQueue_->Enqueue(*closureMessage);

				if (!NT_SUCCESS(status))
				{
					// TODO: we seem to be completely out of resources -- need to signal fault -- assert for now
					Common::Assert::CodingError("Failure to enqueue closure message in inbound queue");
				}
				else
				{
					// successfully initialized and Enqueued closureMessage -- now keep it alive as we exit
					closureMessage->AddRef();
				}
			}
		}

		if (signalSuccess)
		{
			return Common::ErrorCodeValue::Success;
		} 
		else
		{
			// the closureMessage didn't get used
			ReturnQueuedInboundMessage(closureMessage);

			// if finalSequenceNumberInSession_ >= 0 it is a retry, nothing to do then, but if it is not a retry then Close lost the race with Abort..
			if (finalSequenceNumberInSession_ < 0) // not retry
			{
				Common::ErrorCode result = Common::ErrorCodeValue::InvalidState;
				RMTRACE->CloseSessionInboundCompleting(traceId_,SessionId,result.ErrorCodeValueToString());
				return result;
			}
			else
			{
				return Common::ErrorCodeValue::Success;
			}
		}
	}

	// This method is a callback from the AsyncReceiveOperation that processes the Session Closure message in the inboound queue
	void ReliableSessionService::ContinueCloseSessionInbound()
	{
		bool closeSuccess = CloseAndCleanupInboundSession(CloseRequestMessageDequeued);

		if (closeSuccess)
		{
			RMTRACE->CloseSessionInboundCompleting(traceId_,SessionId, Common::ErrorCode(Common::ErrorCodeValue::Success).ErrorCodeValueToString());

			ReliableMessageId messageId(SessionId,0);
			Common::ErrorCode responseResult = SendProtocolResponseMessage(
				ProtocolAction::CloseReliableSessionResponse,
				messageId,
				Common::ErrorCodeValue::Success,
				partnerSendTarget_,
				myEnvironment_,
				targetServicePartition_,
				sourceServicePartition_);

		}
		else
		{
			// TODO: trace!
			//RMTRACE
			// lost race with Abort
			// Do we send a response message?
		}
	}  
#pragma endregion
/*** End methods for Closing an inbound session ***/

/*** Methods for Starting an outbound session ***/
#pragma region OutboundOpenSessionFunctionality
	Common::ErrorCode ReliableSessionService::StartOpenSessionOutbound(
		__in AsyncOutboundOpenOperationSPtr asyncOpenOp,
		__in_opt KAsyncContextBase* const ParentAsync, 
		__in KAsyncContextBase::CompletionCallback OpenCallbackPtr)
	{
		RMTRACE->AsyncOpenOutboundSessionStarted(traceId_,SessionId);

		bool signalSuccess = false;

		K_LOCK_BLOCK(sessionServiceInstanceLock_)
		{
			SessionState nextState;
			signalSuccess = UnsafeProcessSessionSignal(OpenApiCalled,nextState);

			if (signalSuccess)
			{
				ASSERT_IFNOT(nextState==Opening, "Unexpected next state from successful  OpenApiCalled signal");
				openOp_ = asyncOpenOp;
				asyncOpenOp->Start(ParentAsync,OpenCallbackPtr);
			}
			else
			{
				Common::ErrorCode traceCode = Common::ErrorCodeValue::InvalidState;
				RMTRACE->OpenSessionOutboundCompleting(traceId_,SessionId, traceCode.ErrorCodeValueToString());
			}
		}

		if (!signalSuccess)
		{
			return Common::ErrorCodeValue::InvalidState;
		}
		else
		{
			return Common::ErrorCodeValue::Success;
		}
	}

	// OnStart functionality for the async open process
	// This function operates in Opening state when the session is not functional -- not open for Send
	// It is possible that the session will be aborted in a race while we are initiating the open protocol
	// In that case we will fail to complete the protocol when the response arrives
	void ReliableSessionService::OnServiceOpenOutbound()
	{
		// pessimistic initialization
		NTSTATUS insertStatus=STATUS_INSUFFICIENT_RESOURCES;
		ReliableSessionServiceSPtr sessionToInsert = KSharedPtr<ReliableSessionService>(this);

		K_LOCK_BLOCK(myEnvironment_->sessionServiceCommonLock_)
		{
			insertStatus = myEnvironment_->sessionOpenResponseWaiters_.FindOrInsert(sessionToInsert,sessionId_,sessionToInsert);
		}

		if (insertStatus==STATUS_OBJECT_NAME_EXISTS)
		{
			// Start cannot happen twice (failfast on second call) hence this function can only be invoked once
			Common::Assert::CodingError("Unexpected repeat OnServiceOpen for outbound session");
		}
		else if (insertStatus==STATUS_INSUFFICIENT_RESOURCES)
		{
			RMTRACE->OutOfMemory("ReliableSessionService::OnServiceOpenOutbound");
			FailedOpenSessionOutbound(Common::ErrorCodeValue::OutOfMemory);
		}

		ProtocolAction::Code protocolAction = ProtocolAction::OpenReliableSessionRequest;
		ProtocolAction::Direction direction = ProtocolAction::Direction::ToTargetPartition;

		Common::ErrorCode result = SendProtocolRequestMessage(protocolAction,direction);

		if (!result.IsSuccess())
		{
			RMTRACE->ProtocolFailure("SendProtocolRequestMessage","ReliableSessionService::OnServiceOpenOutbound");
			FailedOpenSessionOutbound(result);
		}
	}

	// Instance method to process failure to initiate wire protocol
	BOOLEAN ReliableSessionService::FailedOpenSessionOutbound(
		__in Common::ErrorCode operationResult)
	{
		RMTRACE->OpenSessionOutboundCompleting(traceId_,SessionId, operationResult.ErrorCodeValueToString());

		ASSERT_IF(operationResult.IsSuccess(), "Premature open session protocol termination invoked with success status");

		ReliableSessionServiceSPtr waitingService = nullptr;
		BOOLEAN extractResult=FALSE;

		K_LOCK_BLOCK(myEnvironment_->sessionServiceCommonLock_)
		{
			extractResult = myEnvironment_->sessionOpenResponseWaiters_.Extract(sessionId_,waitingService);
		}

		if (!extractResult)
		{
			// Among termination cases the only case for not finding sessionOpenResponseWaiters_ entry here is that the insert into sessionOpenResponseWaiters_ itself failed
			ASSERT_IFNOT(operationResult.ReadValue()==Common::ErrorCodeValue::OutOfMemory,
										"Unexpected missing session open protocol waiter in open failure case");
		}
		else
		{
			ASSERT_IFNOT(waitingService.RawPtr()==this, "Open session protocol termination extracted the wrong waiting service from Open Waiters map");
		}

		CloseAndCleanupOutboundSession(OpenApiInternalFailure);

		BOOLEAN completeSuccess = openOp_->Complete(operationResult);
		openOp_ = nullptr;
		return completeSuccess;
	}
	
	// Static method called by ReliableMessagingTransport::AsyncDequeueCallback
	void ReliableSessionService::CompleteOpenSessionProtocol(
		__in Common::Guid sessionId, 
		__in Common::ErrorCode operationResult, 
		__in ReliableSessionParametersHeader const &sessionParametersHeader,
		__in ReliableMessagingEnvironmentSPtr const &myEnvironment)
	{
		ReliableSessionServiceSPtr waitingService = nullptr;
		BOOLEAN extractResult=FALSE;

		K_LOCK_BLOCK(myEnvironment->sessionServiceCommonLock_)
		{
			extractResult = myEnvironment->sessionOpenResponseWaiters_.Extract(sessionId,waitingService);
		}

		if (extractResult)
		{
			ASSERT_IF(KSHARED_IS_NULL(waitingService), "Null waitingService returned by sessionOpenResponseWaiters_ successful Extract");

			BOOLEAN completeSuccessful = waitingService->CompleteOpenSessionProtocol(operationResult,sessionParametersHeader.WindowSize);

			ASSERT_IF(completeSuccessful==FALSE, "Complete called more than once on open session protocol");
		}
		else
		{
			RMTRACE->MultipleOpenSessionAcksReceived(sessionId,operationResult.ErrorCodeValueToString());
		}
	}

	// Instance method to process Ack/Nack from target partition
	BOOLEAN ReliableSessionService::CompleteOpenSessionProtocol(
		__in Common::ErrorCode operationResult, 
		__in ULONG maxWindowSize)
	{
		RMTRACE->OpenSessionOutboundCompleting(traceId_,SessionId, operationResult.ErrorCodeValueToString());

		bool signalSuccess = false;

		if (operationResult.IsSuccess())
		{
			// successfully completed open session protocol, make the transition to Opened state

			K_LOCK_BLOCK_EX(sessionServiceInstanceLock_, lock1)
			{		
				SessionState nextState;
				signalSuccess = UnsafeProcessSessionSignal(OpenResponseSuccessMessage,nextState);

				if (signalSuccess)
				{
					ASSERT_IFNOT(nextState==Opened, "Unexpected next state from successful OpenResponseSuccessMessage signal");
					outboundRingBuffer_.SetWindowSize(maxWindowSize);

					K_LOCK_BLOCK_EX(myEnvironment_->sessionServiceCommonLock_, lock2)
					{
						bool insertSuccess = myEnvironment_->commonReliableOutboundSessionsById_.Insert(this);

						// We do not process more than one Ack for the same session open request
						ASSERT_IF(!insertSuccess,"Unexpected duplicate for newly acked outbound session found");
					}
				}
			}
		}
		else
		{
			signalSuccess = CloseAndCleanupOutboundSession(OpenResponseFailureMessage);
		}

		// TODO: review lifecycle management of openOp_
		BOOLEAN completeSuccess = FALSE;

		if (signalSuccess)
		{
			completeSuccess = openOp_->Complete(operationResult);
		}
		else
		{
			completeSuccess = openOp_->Complete(Common::ErrorCodeValue::InvalidState);
		}

		openOp_ = nullptr;

		return completeSuccess;
	}  
#pragma endregion
/*** End methods for Starting an outbound session ***/

/*** Methods for Closing an outbound session ***/
#pragma region OutboundCloseSessionFunctionality
	Common::ErrorCode ReliableSessionService::StartCloseSessionOutbound(
		__in AsyncOutboundCloseOperationSPtr asyncCloseOp,
		__in_opt KAsyncContextBase* const ParentAsync, 
		__in KAsyncContextBase::CompletionCallback CloseCallbackPtr)
	{		
		bool signalSuccess = false;
		SessionState nextState = Unknown;

		K_LOCK_BLOCK(sessionServiceInstanceLock_)
		{
			signalSuccess = UnsafeProcessSessionSignal(CloseApiCalled,nextState);

			if (signalSuccess)
			{
				ASSERT_IFNOT(nextState==Closing, "Unexpected next state from successful  CloseApiCalled signal");

				RMTRACE->AsyncCloseOutboundSessionStarted(traceId_,SessionId);

				closeOp_ = asyncCloseOp;
				asyncCloseOp->Start(ParentAsync,CloseCallbackPtr);

				BufferClosureCompleteCallback closureCallback(this,&ReliableSessionService::OnOutboundBufferClosureCompleted);

				// atomically disable send capability as part of state change to Closing
				outboundRingBuffer_.CloseBuffer(closureCallback);

				// now wait for closureCallback notification that the outbound messages already in the buffer have been sent and acknowledged
				// the callback may happen synchronously--in that case the callback is posted to a different thread to avoid deadlock over sessionServiceInstanceLock_		
			}
		}

		if(signalSuccess)
		{
			return Common::ErrorCodeValue::Success;
		}
		else
		{
			return Common::ErrorCodeValue::InvalidState;
		}
	}


	// Instance method to process failure to initiate wire protocol
	BOOLEAN ReliableSessionService::FailedCloseSessionOutbound(
		__in Common::ErrorCode operationResult)
	{
		RMTRACE->CloseSessionOutboundCompleting(traceId_,SessionId, operationResult.ErrorCodeValueToString());

		ASSERT_IF(operationResult.IsSuccess(), "Premature close session protocol termination invoked with success status");

		ReliableSessionServiceSPtr waitingService = nullptr;
		BOOLEAN extractResult=FALSE;

		K_LOCK_BLOCK(myEnvironment_->sessionServiceCommonLock_)
		{
			extractResult = myEnvironment_->sessionCloseResponseWaiters_.Extract(sessionId_,waitingService);
		}

		if (!extractResult)
		{
			// Among termination cases the only case for not finding sessionCloseResponseWaiters_ entry here is that the insert into sessionCloseResponseWaiters_ itself failed
			ASSERT_IFNOT(operationResult.ReadValue()==Common::ErrorCodeValue::OutOfMemory,
										"Unexpected missing session close protocol waiter in close failure case");
		}
		else
		{
			ASSERT_IFNOT(waitingService.RawPtr()==this, "Close session protocol termination extracted the wrong waiting service from Close Waiters map");
		}

		// ignore the result from this call--if we lost a race with Abort that's OK
		CloseAndCleanupOutboundSession(CloseApiInternalFailure);

		BOOLEAN completeSuccess = closeOp_->Complete(operationResult);
		closeOp_ = nullptr;
		return completeSuccess;
	}



	// this method is a callback from the closed outbound ring buffer signifying that all outbound messages have been acknowledged
	void ReliableSessionService::OnOutboundBufferClosureCompleted()
	{
		// pessimistic initialization
		bool signalSuccess = false;
		NTSTATUS insertStatus=STATUS_INSUFFICIENT_RESOURCES;

		K_LOCK_BLOCK_EX(sessionServiceInstanceLock_, lock1)
		{
			SessionState nextState;
			signalSuccess = UnsafeProcessSessionSignal(AllMessagesAcknowledged,nextState);

			if (signalSuccess)
			{
				ASSERT_IFNOT(nextState==SendBufferDrained, "Unexpected next state from successful  CloseRequestMessageReceived signal");

				// outbound messages already in the buffer have been sent and acknowledged
				// now initiate closure protocol with receiver service partition

				ReliableSessionServiceSPtr sessionToInsert = KSharedPtr<ReliableSessionService>(this);

				K_LOCK_BLOCK_EX(myEnvironment_->sessionServiceCommonLock_, lock2)
				{
					insertStatus = myEnvironment_->sessionCloseResponseWaiters_.FindOrInsert(sessionToInsert,sessionId_,sessionToInsert);
				}
			}
		} // release the instance lock

		// if we failed the state transition we lost the race with an Abort and there is nothing to do wince the session is already closed and cleaned up
		// if we succeeded we proceed to initiate the close session protocol with the receiving service partition
		if (signalSuccess)
		{
			if (insertStatus==STATUS_OBJECT_NAME_EXISTS)
			{
				// We do not process more than one Ack for the same session open request
				Common::Assert::CodingError("Unexpected repeat OnServiceClose for outbound session");
			}
			else if (insertStatus==STATUS_INSUFFICIENT_RESOURCES)
			{
				// TODO: we should probably report fault here
				RMTRACE->OutOfMemory("ReliableSessionService::OnOutboundBufferClosureCompleted");
				FailedCloseSessionOutbound(Common::ErrorCodeValue::OutOfMemory);
				return;
			}

			ProtocolAction::Code protocolAction = ProtocolAction::CloseReliableSessionRequest;
			ProtocolAction::Direction direction = ProtocolAction::Direction::ToTargetPartition;

			Common::ErrorCode result = SendProtocolRequestMessage(protocolAction,direction);

			// the only error possible is OutOfMemory since SendProtocolRequestMessage has built in retry for all other possibilities
			if (!result.IsSuccess())
			{
				RMTRACE->ProtocolFailure("SendProtocolRequestMessage","ReliableSessionService::OnOutboundBufferClosureCompleted");
				FailedCloseSessionOutbound(result);
			}
		}
	}

	// Static method called by ReliableMessagingTransport::AsyncDequeueCallback
	void ReliableSessionService::CompleteCloseSessionProtocol(
		__in Common::Guid sessionId, 
		__in Common::ErrorCode operationResult, 
		__in ReliableMessagingEnvironmentSPtr const &myEnvironment)
	{
		ReliableSessionServiceSPtr waitingService = nullptr;
		BOOLEAN extractResult=FALSE;

		K_LOCK_BLOCK(myEnvironment->sessionServiceCommonLock_)
		{
			extractResult = myEnvironment->sessionCloseResponseWaiters_.Extract(sessionId,waitingService);
		}

		if (extractResult)
		{
			ASSERT_IF(KSHARED_IS_NULL(waitingService), "Null waitingService returned by sessionCloseResponseWaiters_ successful Extract");

			BOOLEAN completeSuccessful = waitingService->CompleteCloseSessionProtocol(operationResult);

			ASSERT_IFNOT(completeSuccessful==TRUE, "Complete called more than once on close session protocol");
		}
		else
		{
			// TODO: MultipleCloseSessionAcksReceived needs to be changed to CloseSessionAckDropped
			RMTRACE->MultipleCloseSessionAcksReceived(sessionId,operationResult.ErrorCodeValueToString());
		}
	}

	// Instance method to process Ack/Nack from target partition
	BOOLEAN ReliableSessionService::CompleteCloseSessionProtocol(
		__in Common::ErrorCode operationResult)
	{
		RMTRACE->CloseSessionOutboundCompleting(traceId_,SessionId, operationResult.ErrorCodeValueToString());

		// If the outbound session got cleaned up under us before the protocol completed, that comes through here as a virtual CloseResponseFailureMessage 
		// but in that case the state transition to Close has already occurred so the CloseAndCleanupOutboundSession is a no-op
		// We go through the motions anyway in case a future case occurs where an OperationCanceled error can occur in other ways 
		// CloseAndCleanupOutboundSession is meant to be resilient to multiple calls with different causes
		SessionSignal signal = operationResult.IsSuccess() ? CloseResponseSuccessMessage : CloseResponseFailureMessage;
		bool signalSuccess = CloseAndCleanupOutboundSession(signal);

		BOOLEAN completeSuccess = FALSE;
		
		// Take care of the case where an AbortOutboundSessionRequest is causing completion with a result of OperationCanceled
		if (signalSuccess || operationResult.ReadValue() == Common::ErrorCodeValue::OperationCanceled)
		{
			completeSuccess = closeOp_->Complete(operationResult);
		}
		else
		{
			completeSuccess = closeOp_->Complete(Common::ErrorCodeValue::InvalidState);
		}

		closeOp_ = nullptr;

		ASSERT_IFNOT(completeSuccess==TRUE, "Complete called more than once in close session protocol");

		return completeSuccess;
	}  
#pragma endregion
/*** End methods for Closing an outbound session ***/

/*** Methods to manage session Abort and cleanup ***/
#pragma region SessionCleanupFunctionality

	// This routine is used for cleanup in all cases for inbound sessions: normal closure as well as session offer failure and abort
	bool ReliableSessionService::CloseAndCleanupInboundSession(SessionSignal cleanupSignal)
	{
		RMTRACE->CleanupInboundSessionStarted(traceId_,sessionId_,SignalAsString(cleanupSignal));

		switch (cleanupSignal)
		{
		case AbortApiCalled:
		case AbortInboundSessionRequestMessage:
		case AbortConflictingInboundSession:
		case AbortSessionForManagerClose:
		case CloseRequestMessageDequeued:
		case SessionOfferNotAccepted:
			break;

		default:
			Common::Assert::CodingError("Unexpected CloseAndCleanup signal for inbound session");	
		}

		bool signalSuccess = false;

		K_LOCK_BLOCK(sessionServiceInstanceLock_)
		{
			SessionState nextState;
			signalSuccess = UnsafeProcessSessionSignal(cleanupSignal,nextState);

			if (signalSuccess)
			{
				ASSERT_IFNOT(nextState==Closed, "Unexpected next state from successful cleanup signal");	

				switch (cleanupSignal)
				{
				case AbortApiCalled:
				case AbortSessionForManagerClose:
					{
						inboundRingBuffer_.ClearBuffer();

						ProtocolAction::Code protocolAction = ProtocolAction::AbortOutboundReliableSessionRequest;
						ProtocolAction::Direction direction = ProtocolAction::Direction::ToSourcePartition;

						// The result of this operation is ignored, this is a best effort signal
						// OK to send under sessionServiceInstanceLock_ since there is no quota for these operations
						SendProtocolRequestMessage(protocolAction,direction,maxRetryCountForAbortMessage);
						break;
					}

				case AbortInboundSessionRequestMessage:
				case AbortConflictingInboundSession:
					{
						ASSERT_IFNOT(sessionKind_==Inbound, "Unexpected AbortInboundSessionRequestMessage signal for outbound session");	
						inboundRingBuffer_.ClearBuffer();

						// notify service of impending involuntary abort on a separate thread.  This has no semantic significance for abort
						Common::Threadpool::Post(
							[this]()
								{
									this->ownerSessionManager_->SessionAbortedCallback(
																	SessionKind::Inbound,
																	this->SourceServicePartition,
																	KSharedPtr<ReliableSessionService>(this));
								}
						);

						break;
					}

				case SessionOfferNotAccepted:
					{
						inboundRingBuffer_.CloseBuffer();
						break;
					}

				case CloseRequestMessageDequeued:
					{
						// buffer was already closed when the closure message was Enqueued
						normalCloseOccurred_ = true;
						break;
					}
				}

				// inbound ring buffer should now be closed in all cases
				ASSERT_IFNOT(inboundRingBuffer_.IsBufferClosed(), "CloseAndCleanupInboundSession called with inboundRingBuffer_ not closed");

				// now stop the queue to finish shutdown
				StopQueue();

				// mutually release session manager relationship
				ownerSessionManager_->ReleaseSessionInbound(KSharedPtr<ReliableSessionService>(this));

				// TODO: look closely at lifecycle management os sessions and session managers
				//ownerSessionManager_ = nullptr;
				
				// we still need the partnerSendTarget_ for the CloseResponse protocol and possibly for other responses since we are leaving a tombstone
				// TODO: revisit inbound session cleanup, some tricky race conditions including multiple colliding protocol responses when session not found during CloseRequest retry
				// partnerSendTarget_ = nullptr;
			}
		}

		RMTRACE->CleanupInboundSessionCompleted(traceId_,sessionId_,signalSuccess);


		return signalSuccess;
	}

	bool ReliableSessionService::CloseAndCleanupOutboundSession(SessionSignal cleanupSignal)
	{
		RMTRACE->CleanupOutboundSessionStarted(traceId_,sessionId_,SignalAsString(cleanupSignal));

		switch (cleanupSignal)
		{
		case AbortApiCalled:
		case AbortOutboundSessionRequestMessage:
		case AbortOutboundSessionOnInvalidStateAck:
		case AbortSessionForManagerClose:
		case OpenApiInternalFailure:
		case OpenResponseFailureMessage:
		case CloseResponseSuccessMessage:
		case CloseResponseFailureMessage:
		case CloseApiInternalFailure:
			break;

		default:
			Common::Assert::CodingError("Unexpected CloseAndCleanup signal for outbound session");	
		}

		bool signalSuccess = false;

		// Variables to set inside the lock and use outside the lock if it turns out that we were waiting 
		// for normal CloseSession protocol to complete and an Abort occurred
		BOOLEAN extractResult = FALSE;
		ReliableSessionServiceSPtr waitingService = nullptr;

		K_LOCK_BLOCK_EX(sessionServiceInstanceLock_, lock1)
		{
			SessionState previousState = sessionState_;
			SessionState nextState = Unknown;
			signalSuccess = UnsafeProcessSessionSignal(cleanupSignal,nextState);

			if (signalSuccess)
			{
				ASSERT_IFNOT(nextState==Closed, "Unexpected next state from successful cleanup signal");	

				switch (cleanupSignal)
				{
				case AbortSessionForManagerClose:
				case AbortApiCalled:
					{
						bool expectClosed = (previousState==Closing || previousState == SendBufferDrained);
						outboundRingBuffer_.ClearBuffer(expectClosed);

						ProtocolAction::Code protocolAction = ProtocolAction::AbortInboundReliableSessionRequest;
						ProtocolAction::Direction direction = ProtocolAction::Direction::ToTargetPartition;

						// The result of this operation is ignored, this is a best effort signal
						SendProtocolRequestMessage(protocolAction,direction,maxRetryCountForAbortMessage);
						break;
					}

				case AbortOutboundSessionRequestMessage:
				case AbortOutboundSessionOnInvalidStateAck:
					{
						ASSERT_IFNOT(sessionKind_==Outbound, "Unexpected AbortOutboundSession signal for inbound session");	
						bool expectClosed = (previousState==Closing || previousState == SendBufferDrained);
						outboundRingBuffer_.ClearBuffer(expectClosed);

						// notify service of impending involuntary abort on a separate thread.  This has no semantic significance for abort.
						Common::Threadpool::Post(
							[this]()
								{
									this->ownerSessionManager_->SessionAbortedCallback(
																	SessionKind::Outbound,
																	this->TargetServicePartition,
																	KSharedPtr<ReliableSessionService>(this));
								}
						);

						break;
					}

				case OpenApiInternalFailure:
				case OpenResponseFailureMessage:
					{
						outboundRingBuffer_.CloseBuffer();
						break;
					}
				}

				// outbound ring buffer should now be closed in all cases
				ASSERT_IFNOT(outboundRingBuffer_.IsBufferClosed(), "CloseAndCleanupOutboundSession called with outboundRingBuffer_ not closed");

				// mutually release session manager relationship
				ownerSessionManager_->ReleaseSessionOutbound(KSharedPtr<ReliableSessionService>(this));

				// TODO: look closely at lifecycle management os sessions and session managers
				//ownerSessionManager_ = nullptr;

				bool removeSuccess = false;

				K_LOCK_BLOCK_EX(myEnvironment_->sessionServiceCommonLock_, lock2)
				{
					removeSuccess = myEnvironment_->commonReliableOutboundSessionsById_.Remove(this);

					// also stop waiting for a response to a normal Close protocol message: it may never arrive
					extractResult = myEnvironment_->sessionCloseResponseWaiters_.Extract(sessionId_,waitingService);
				}

				if (cleanupSignal==CloseResponseSuccessMessage || cleanupSignal==CloseResponseFailureMessage || cleanupSignal == CloseApiInternalFailure)
				{
					// We only do this during a transition to Closed state which can only happen at most once and only during close outbound session process
					ASSERT_IFNOT(removeSuccess,"Service not found in SessionId Map whicle completing close outbound session protocol");
				}

				if (cleanupSignal==CloseResponseSuccessMessage)
				{
					normalCloseOccurred_ = true;
				}

				// no further communication with partner service partition is possible so release the connection and TCP buffers
				partnerSendTarget_ = nullptr;
			}
		}

		if (extractResult)
		{
			//RMTRACE if extractResult is TRUE
			ASSERT_IFNOT(waitingService.RawPtr()==this, "Unexpected waiting service extracted from sessionCloseResponseWaiters_ during outbound Abort");
			// Complete the process appropriately
			CompleteCloseSessionProtocol(Common::ErrorCodeValue::OperationCanceled);
		}

		RMTRACE->CleanupOutboundSessionCompleted(traceId_,sessionId_,signalSuccess);
		return signalSuccess;
	}


	Common::ErrorCode ReliableSessionService::AbortSession(SessionSignal abortSignal)
	{
		bool signalSuccess = false;

		if (sessionKind_ == SessionKind::Inbound)
		{
			RMTRACE->AbortInboundSessionStarted(traceId_,SessionId,SignalAsString(abortSignal));
			signalSuccess = CloseAndCleanupInboundSession(abortSignal);
		}
		else
		{
			RMTRACE->AbortOutboundSessionStarted(traceId_,SessionId,SignalAsString(abortSignal));
			signalSuccess = CloseAndCleanupOutboundSession(abortSignal);
		}

		Common::ErrorCode result;
		
		if (!signalSuccess)
		{
			result = Common::ErrorCodeValue::InvalidState;
		}
		else
		{
			result = Common::ErrorCodeValue::Success;
		}

		// we compute the result just for tracing
		if (sessionKind_ == SessionKind::Inbound)
		{
			RMTRACE->AbortInboundSessionCompleting(traceId_,SessionId,result.ErrorCodeValueToString());
		}
		else
		{
			RMTRACE->AbortOutboundSessionCompleting(traceId_,SessionId,result.ErrorCodeValueToString());
		}

		// At the API level, Abort never fails
		return Common::ErrorCodeValue::Success;
	}  
#pragma endregion
/*** End methods to manage session Abort and cleanup ***/

/*** Helper functions to drive session wire protocol ***/
#pragma region WireProtocolHelpers
	Common::ErrorCode ReliableSessionService::CreateProtocolMessage(
		__in ProtocolAction::Code actionCode, 
		__in ProtocolAction::Direction direction,
		__out Transport::MessageUPtr &message)
	{
		ReliableMessagingServicePartitionSPtr source;
		ReliableMessagingServicePartitionSPtr target;

		switch (direction)
		{
		case ProtocolAction::Direction::ToTargetPartition:
			{
				source = sourceServicePartition_;
				target = targetServicePartition_;
				break;
			}

		case ProtocolAction::Direction::ToSourcePartition:
			{
				source = targetServicePartition_;
				target = sourceServicePartition_;
				break;
			}
		default: Common::Assert::CodingError("Unexpected ProtocolAction::Direction in CreateProtocolMessage"); 
		}

		FABRIC_SEQUENCE_NUMBER protocolSequenceNumber;

		if (actionCode==ProtocolAction::CloseReliableSessionRequest)
		{
			FABRIC_SEQUENCE_NUMBER finalSequenceNumberAtClose;

			bool success = outboundRingBuffer_.GetFinalSequenceNumberAtClose(finalSequenceNumberAtClose);
			ASSERT_IFNOT(success, "Found outbound buffer not closed while sending CloseReliableSessionRequest");

			// this message is going to get into the receive side queue as a closure token
			protocolSequenceNumber = finalSequenceNumberAtClose+1;
		}
		else
		{
			protocolSequenceNumber = 0;
		}

		ReliableMessageId messageId(sessionId_,protocolSequenceNumber);

		return CreateProtocolMessage(actionCode, messageId, message, source, target);
	}


	Common::ErrorCode ReliableSessionService::CreateProtocolMessage(
		__in ProtocolAction::Code actionCode, 
		__in ReliableMessageId const &messageId,
		__out Transport::MessageUPtr &message,
		__in ReliableMessagingServicePartitionSPtr sourceServicePartition,
		__in ReliableMessagingServicePartitionSPtr targetServicePartition)
	{
		try // Needed for Transport functionality usage -- exceptions will occur in low memory conditions
		{
			ReliableSessionHeader reliableSessionHeader(messageId.SessionId, actionCode, messageId.SequenceNumber);

			// Make the empty base message
			Transport::MessageUPtr reliableSessionProtocolMessage 
				= Common::make_unique<Transport::Message>();

			// Add the headers
			MessageHeaders& headers = reliableSessionProtocolMessage->Headers;
			headers.Add(reliableSessionHeader);

			if (sourceServicePartition != nullptr)
			{
				ReliableMessagingSourceHeaderSPtr reliableSessionSourceHeader = sourceServicePartition->SourceHeader;
				headers.Add(*reliableSessionSourceHeader);
			}

			if (targetServicePartition != nullptr)
			{
				ReliableMessagingTargetHeaderSPtr reliableSessionTargetHeader = targetServicePartition->TargetHeader;
				headers.Add(*reliableSessionTargetHeader);
			}

			headers.Add(Transport::ActionHeader(L"ReliableSessionMessage"));

			message = std::move(reliableSessionProtocolMessage);	
			return Common::ErrorCodeValue::Success;
		}
		catch (std::exception const&)
		{
			// TODO: do we need to look more closely into these exceptions?
			message = nullptr;
			RMTRACE->TransportException("ReliableSessionService::CreateProtocolMessage");
			return Common::ErrorCodeValue::OutOfMemory;
		}
	}

	Common::ErrorCode ReliableSessionService::SendProtocolRequestMessage(
		__in ProtocolAction::Code actionCode, 
		__in ProtocolAction::Direction direction,
		__in int maxRetryCount)
	{
		Transport::MessageUPtr protocolRequestMessage;

		Common::ErrorCode result = CreateProtocolMessage(actionCode,direction,protocolRequestMessage);

		if (result.IsSuccess())
		{
			ProtocolAction::Code ackCode = AckCodeFor(actionCode);

			ASSERT_IF(ackCode==ProtocolAction::Unknown, "Unknown ackCode in ReliableSessionService::SendProtocolRequestMessage");

			AsyncProtocolOperationSPtr protocolOp = AsyncProtocolOperation::GetOperation(myEnvironment_);

			if (protocolOp == nullptr) 
			{
				return Common::ErrorCodeValue::OutOfMemory;
			}

			result = protocolOp->StartProtocolOperation(
												actionCode,
												ackCode,
												SessionId,
												std::move(protocolRequestMessage),
												partnerSendTarget_,
												myEnvironment_,
												maxRetryCount);
		}

		return result;
	}

	Common::ErrorCode ReliableSessionService::SendProtocolResponseMessage(
		__in ProtocolAction::Code actionCode, 
		__in ReliableMessageId const &messageId,
		__in Common::ErrorCode response,
		__in ISendTarget::SPtr const &responseTarget,
		__in ReliableMessagingEnvironmentSPtr myEnvironment,
		__in ReliableMessagingServicePartitionSPtr const fromServicePartition,
		__in ReliableMessagingServicePartitionSPtr const toServicePartition
		)
	{
		Transport::MessageUPtr responseMessage;

		Common::ErrorCode result = ReliableSessionService::CreateProtocolMessage(
			actionCode,
			messageId,
			responseMessage,
			fromServicePartition,
			toServicePartition
			);

		if (result.IsSuccess())
		{
			try // Transport functions may throw exceptions in low memory situations
			{
				ReliableMessagingProtocolResponseHeader responseHeader(response);
				MessageHeaders& headers = responseMessage->Headers;
				headers.Add(responseHeader);
			}
			catch (std::exception const &)
			{
				RMTRACE->TransportException("ReliableSessionService::SendProtocolResponseMessage");
				return Common::ErrorCodeValue::OutOfMemory;
			}

			ProtocolAction::Code ackCode = AckCodeFor(actionCode);

			ASSERT_IF(ackCode==ProtocolAction::Unknown, "Unknown ackCode in ReliableSessionService::SendProtocolRequestMessage");

			AsyncProtocolOperationSPtr protocolOp = AsyncProtocolOperation::GetOperation(myEnvironment);

			if (protocolOp == nullptr) 
			{
				return Common::ErrorCodeValue::OutOfMemory;
			}

			// We do not send in session context here since the session may have been rejected or closed or not found etc
			result = protocolOp->StartProtocolOperation(actionCode,ackCode,messageId.SessionId,std::move(responseMessage),responseTarget,myEnvironment);
		}

		return result;
	}

	Common::ErrorCode ReliableSessionService::SendAck(
		__in ProtocolAction::Code actionCode, 
		__in ReliableMessageId const &messageId,
		__in Common::ErrorCode response,
		__in ISendTarget::SPtr const &responseTarget,
		__in ReliableMessagingEnvironmentSPtr myEnvironment)
	{
		Transport::MessageUPtr ackMessage;
		Common::ErrorCode result = ReliableSessionService::CreateProtocolMessage(
			actionCode,
			messageId,
			ackMessage);

		if (result.IsSuccess())
		{
			try // Transport functions may throw exceptions in low memory situations
			{
				ReliableMessagingProtocolResponseHeader responseHeader(response);
				MessageHeaders& headers = ackMessage->Headers;
				headers.Add(responseHeader);
			}
			catch (std::exception const &)
			{
				RMTRACE->TransportException("ReliableSessionService::SendProtocolResponseMessage");
				// TODO: no other recourse here?  Report fault?
				return Common::ErrorCodeValue::OutOfMemory;
			}

			result = myEnvironment->transport_->SendOneWay(responseTarget, std::move(ackMessage));	
		}
		else
		{
			// TODO: no other recourse here?  Report fault?
		}

		return result;
	}

	Common::ErrorCode ReliableSessionService::SendOpenSessionResponseMessage(
		__in Common::ErrorCode response)
	{
		Transport::MessageUPtr responseMessage;

		Common::ErrorCode result = ReliableSessionService::CreateProtocolMessage(
			ProtocolAction::OpenReliableSessionResponse,
			ReliableMessageId(sessionId_,0),
			responseMessage,
			targetServicePartition_,
			sourceServicePartition_
			);

		if (result.IsSuccess())
		{
			try // Transport functions may throw exceptions in low memory situations
			{
				ReliableMessagingProtocolResponseHeader responseHeader(response);
				ReliableSessionParametersHeader sessionParametersHeader(inboundRingBuffer_.CurrentWindowSize);
				MessageHeaders& headers = responseMessage->Headers;
				headers.Add(responseHeader);
				headers.Add(sessionParametersHeader);
			}
			catch (std::exception const &)
			{
				RMTRACE->TransportException("ReliableSessionService::SendProtocolResponseMessage");
				return Common::ErrorCodeValue::OutOfMemory;
			}

			AsyncProtocolOperationSPtr protocolOp = AsyncProtocolOperation::GetOperation(myEnvironment_);

			if (protocolOp == nullptr) 
			{
				return Common::ErrorCodeValue::OutOfMemory;
			}

			// We do not send in session context here since the session may have been rejected or closed or not found etc
			result = protocolOp->StartProtocolOperation(
				ProtocolAction::OpenReliableSessionResponse,
				ProtocolAction::OpenReliableSessionResponseAck,
				sessionId_,
				std::move(responseMessage),
				partnerSendTarget_,
				myEnvironment_);
		}

		return result;
	}


	// standard helper to send message to current target endpoint
	Common::ErrorCode ReliableSessionService::Send(Transport::MessageUPtr &&message)
	{
		return 	myEnvironment_->transport_->SendOneWay(partnerSendTarget_, std::move(message));
	}

#pragma endregion
/*** End helper functions to drive session wire protocol ***/

/*** Helper functions to manage outbound and inbound ring buffers ***/
#pragma region RingBufferHelpers
	Common::ErrorCode ReliableSessionService::InsertIntoSendBuffer(AsyncSendOperationSPtr const &sendOp)
	{
		if (!IsOpened())
		{
			// This check is here to prevent a lot of sends from being initiated when the session is aborting
			return Common::ErrorCodeValue::InvalidState;
		}
		else
		{
			return outboundRingBuffer_.SendOrHold(sendOp);
		}
	}

	Common::ErrorCode ReliableSessionService::InsertInbound(
		__in QueuedInboundMessageSPtr const &sendMsgSPtr)
	{
		Common::ErrorCode result = inboundRingBuffer_.InsertAndFlush(sendMsgSPtr);

		return result;
	}

	// for use by timer-based ack
	Common::ErrorCode ReliableSessionService::AcknowledgeMessageNumber(
		__in FABRIC_SEQUENCE_NUMBER sequenceNumberToAcknowledge)
	{
		ReliableMessageId messageId(sessionId_,sequenceNumberToAcknowledge);
		Common::ErrorCode ackResult = SendAck(
			ProtocolAction::SendReliableMessageAck,
			messageId,
			Common::ErrorCodeValue::Success,
			partnerSendTarget_,
			myEnvironment_);

		if (ackResult.IsSuccess())
		{
			RMTRACE->SendingMessageAck(SessionId,sequenceNumberToAcknowledge,inboundRingBuffer_.BufferedMessageCount);
		}

		return ackResult;
	}  
#pragma endregion
/*** End helper functions to manage outbound and inbound ring buffers ***/

/*** Helper functions to manage inbound async queue ***/
#pragma region AsyncQueueHelpers
	Common::ErrorCode ReliableSessionService::Enqueue(
		__in QueuedInboundMessage::SPtr const &sendMsgSPtr)
	{
		// Checking IsOpened here cause deadlock with Abort, since that needs the sessionServiceInstanceLock
		// OK to try enqueue in any case, the queue gets flushed in case of Abort
		// if we lose a race with Close/Abort we will get a status of K_STATUS_SHUTDOWN_PENDING
		NTSTATUS status = inboundAsyncSessionQueue_->Enqueue(*sendMsgSPtr);

		if (status==K_STATUS_SHUTDOWN_PENDING)
		{
			// the queue is in the process of being deactivated, as a part of session close
			return Common::ErrorCodeValue::InvalidState;
		}
		else
		{
			return Common::ErrorCode::FromNtStatus(status);
		}
	}

	Common::ErrorCode ReliableSessionService::TryDequeue(
		__out QueuedInboundMessageSPtr& queuedMessage)
	{
		Common::ErrorCode result = Common::ErrorCodeValue::NotReady;

		K_LOCK_BLOCK(sessionServiceInstanceLock_)
		{
			if (UnsafeWasClosedNormally())
			{
				return Common::ErrorCodeValue::ObjectClosed;
			}

			if (!UnsafeIsOpenForReceive())
			{
				return Common::ErrorCodeValue::InvalidState;
			}

			queuedMessage = inboundAsyncSessionQueue_->TryDequeue();

			if (queuedMessage==nullptr)
			{
				return Common::ErrorCodeValue::ReliableSessionQueueEmpty;
			}
		}

		return Common::ErrorCodeValue::Success;
	}

	// for Session level dequeue operations with an outstanding receive as context
	Common::ErrorCode ReliableSessionService::CreateDequeueOperation(
		__in AsyncReceiveOperationSPtr const &receiveOp,		// outstanding receive
		__out InboundMessageQueue::DequeueOperation::SPtr& result)
	{
		Common::ErrorCode createResult = Common::ErrorCodeValue::NotReady;

		K_LOCK_BLOCK(sessionServiceInstanceLock_)
		{
			if (UnsafeWasClosedNormally())
			{
				return Common::ErrorCodeValue::ObjectClosed;
			}

			if (!UnsafeIsOpenForReceive())
			{
				return Common::ErrorCodeValue::InvalidState;
			}

			createResult = inboundAsyncSessionQueue_->CreateDequeueOperation(receiveOp,result);
		}

		return createResult;
	}



	// not safe to call StopQueue more than once: Deactivate will failfast
	// must always call under sessionServiceInstanceLock_
	void ReliableSessionService::StopQueue()
	{
		if (inboundAsyncSessionQueue_ != nullptr)
		{
			// TODO: add dropped item callback to handle dropped enqueued messages?  To send back Nacks perhaps?  Not essential but courteous ..
			// Must not set inboundAsyncSessionQueue_ to null because it may still be accessed after deactivate due to numerous race conditions
			inboundAsyncSessionQueue_->Deactivate(nullptr);
			asyncQueueDeactivated_ = true;
		} 
	}  
#pragma endregion
/*** End helper functions to manage inbound async queue ***/

}


// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace ReliableMessaging
{
	K_FORCE_SHARED_NO_OP_DESTRUCTOR(ReliableMessagingTransport)

	Common::ErrorCode ReliableMessagingTransport::GetSingletonListenAddress(std::wstring & address)
	{ 
		ReliableMessagingEnvironmentSPtr singletonEnvironment = ReliableMessagingEnvironment::TryGetsingletonEnvironment();

		if (singletonEnvironment == nullptr) 
		{
			return Common::ErrorCodeValue::OutOfMemory;
		}
		
		address = singletonEnvironment->transport_->ListenAddress;
				
		return Common::ErrorCodeValue::Success;
	}

	Common::ErrorCode ReliableMessagingTransport::ResolveTarget(
		__in std::wstring const & address,
		__out ISendTarget::SPtr &target)
	{
		try // Transport functions may throw exceptions in low memory situations
		{
			target = myEnvironment_->transport_->transport_->ResolveTarget(address);
			return Common::ErrorCodeValue::Success;
		}
		catch (std::exception const &)
		{
			target = nullptr;
			RMTRACE->TransportException("ReliableMessagingTransport::ResolveTarget");
			return Common::ErrorCodeValue::OutOfMemory;
		}			
	}

    Common::ErrorCode ReliableMessagingTransport::SendOneWay(ISendTarget::SPtr const & target, MessageUPtr && message)
	{
		try	// Transport functions throw exceptions in low memory situations
		{
			Common::ErrorCode result = myEnvironment_->transport_->transport_->SendOneWay(target, std::move(message));
					
			if (!result.IsSuccess())
			{
				RMTRACE->CommunicationFailure("TcpDatagramTransport::SendOneWay","ReliableMessagingTransport::SendOneWay", result.ErrorCodeValueToString());
			}

			return result;
		}
		catch (std::exception const &)
		{
			RMTRACE->TransportException("ReliableMessagingTransport::SendOneWay");
			return Common::ErrorCodeValue::OutOfMemory;
		}
	}




	Common::ErrorCode ReliableMessagingTransport::Start()
	{
		// transport is guaranteed to be in started state if any messaging resources are active
		if (state_==NotInitialized)
		{
			Common::ErrorCode status = InboundMessageQueue::Create(inboundAsyncTransportQueue_);
			if (!status.IsSuccess())
			{
				return status;
			}

			inboundAsyncTransportQueue_->Activate(nullptr,nullptr);

			try // calling a number of transport functions which may throw exceptions in low memory conditions
			{
				// Get local host name
				Common::ErrorCode startResult = TcpTransportUtility::GetLocalFqdn(hostname_);

				if (!startResult.IsSuccess()) 
				{
					RMTRACE->TransportStartupFailure(startResult.ErrorCodeValueToString());
					return Common::ErrorCodeValue::ReliableSessionTransportStartupFailure;
				}

				// Use port 0 to acquire dynamic port
				transport_ = DatagramTransportFactory::CreateTcp(Common::wformatString("{0}:0",hostname_), L"", L"ReliableMessaging");
        
				// The message handler uses inboundAsyncTransportQueue_ to post inbound messages for async processing
				transport_->SetMessageHandler(std::bind(&ReliableMessagingTransport::CommonMessageHandler, 
															 this,
															 std::placeholders::_1, 
															 std::placeholders::_2));

				startResult = transport_->Start();

				if (!startResult.IsSuccess()) 
				{
					RMTRACE->TransportStartupFailure(startResult.ErrorCodeValueToString());
					return Common::ErrorCodeValue::ReliableSessionTransportStartupFailure;
				}

				listeningAt_ = transport_->ListenAddress();
			}
			catch (std::exception const &)
			{
				Stop();
				RMTRACE->TransportException("ReliableMessagingTransport::Start");
				return Common::ErrorCodeValue::OutOfMemory;
			}
														 
			state_ = Started;

			// Start the configured number of dequeue waiters to allow inbound message handling to start
			for (int i=0; i<transportMessageDequeueWaitersInitialCount; i++)
			{
				InboundMessageQueue::DequeueOperation::SPtr deqMessage;
				status = inboundAsyncTransportQueue_->CreateDequeueOperation(deqMessage);

				if (status.IsSuccess())
				{
					deqMessage->StartDequeue(nullptr,inboundDequeuecallback_);
				}
				else 
				{
					// We seem to be out of resources already, shutdown and exit with failure
					Stop();
					return status;
				}
			}

			return Common::ErrorCodeValue::Success;
		}
		else if (state_==Started) 
		{
			// Already initiatized
			return Common::ErrorCodeValue::Success;
		}
		else 
        {
            return Common::ErrorCodeValue::OperationFailed;
        }
	}

	void ReliableMessagingTransport::Stop()
	{
		// transport is guaranteed to be in started state if any messaging resources are active
		if (state_==Started)
		{
			transport_->Stop();
			inboundAsyncTransportQueue_->Deactivate();

			hostname_ = L"";
			listeningAt_ = L"";
		}

		// This operation always succeeds in stopping the transport function regardless of initial state
		// It is the caller's responsibility to ensure that no transport function dependent resource is still active
		// Stop is always followed by delete -- a new ReliableMessagingTransport is created if messaging activity restarts
		state_ = Stopped;
	}


	void ReliableMessagingTransport::CommonMessageHandler(MessageUPtr &message, ISendTarget::SPtr const & responseTarget)
	{
		// TODO: this code needs to be factored and structured to amke it easy to bypass Transport for intra-process communication
		RMTRACE->MessageReceived(responseTarget->Address());
		InterlockedIncrement(&myEnvironment_->numberOfMessagesReceived_);
		
		MessageUPtr localMessage = message->Clone();

		ReliableSessionHeader sessionHeader;
		bool headerFound = localMessage->Headers.TryReadFirst(sessionHeader);

		if (!headerFound)
		{
			// TODO: what do we do with invalid messages?  For now, assert!
			RMTRACE->InvalidMessage(responseTarget->Address(),"Session Message Lacking Session Header");
			Common::Assert::CodingError("Reliable session listen port received message without session heeader");
			return;
		}

		RMTRACE->TransportReceivedMessage(ProtocolAction::CodeAsString(sessionHeader.ActionCode),sessionHeader.SessionId,sessionHeader.SequenceNumber);

		switch(sessionHeader.ActionCode)
		{

		case ProtocolAction::OpenReliableSessionRequestAck:
		case ProtocolAction::OpenReliableSessionResponseAck:
		case ProtocolAction::CloseReliableSessionRequestAck:
		case ProtocolAction::CloseReliableSessionResponseAck:
		case ProtocolAction::AbortInboundReliableSessionRequestAck:
		case ProtocolAction::AbortOutboundReliableSessionRequestAck:
			{
				ProtocolAckKey ackKey(sessionHeader.SessionId,sessionHeader.ActionCode);
				BOOLEAN extractResult=FALSE;
				AsyncProtocolOperationSPtr waitingOperation = nullptr;

				K_LOCK_BLOCK(myEnvironment_->sessionServiceCommonLock_)
				{
					extractResult = myEnvironment_->sessionProtocolAckWaiters_.Extract(ackKey,waitingOperation);
				}

				if (!extractResult)
				{
					//RMTRACE->MultipleProtocolAcksReceived(sessionId,receivedSessionHeader.ActionCode);
				}

				BOOLEAN completeSuccessful = TRUE;

				if (waitingOperation != nullptr)
				{
					completeSuccessful = waitingOperation->CompleteProtocolOperation(sessionHeader.ActionCode);
				}

				ASSERT_IFNOT(completeSuccessful==TRUE, "Complete called more than once for Protocol Ack");

				return;
			}

		case ProtocolAction::SendReliableMessage:
			{
				// Enueue to the right session queue via helper
				ReliableSessionServiceSPtr inboundSession = nullptr;
				bool sessionFound = false;

				QueuedInboundMessageSPtr contentMessageSPtr = nullptr;
				Common::ErrorCode enqResult;


				K_LOCK_BLOCK(myEnvironment_->sessionServiceCommonLock_)
				{
					// TODO: make sure hash table structure has a const Find key parameter (unlike KAvlTree) so we can drop this extra assignment
					Common::Guid sessionId = sessionHeader.SessionId;
					ReliableSessionService *inboundSessionRawPtr = nullptr;
					sessionFound = myEnvironment_->commonReliableInboundSessionsById_.Find(sessionId,inboundSessionRawPtr);
					inboundSession = inboundSessionRawPtr;
				}

				if (sessionFound)
				{
					// get message from session pool
					contentMessageSPtr = inboundSession->GetQueuedInboundMessage();

					if (contentMessageSPtr==nullptr)
					{
						// TODO: think through this situation -- just dropping the message right now -- consider sending a Nack
						// unsigned __int64 environmentNumberOfMessagesDropped = InterlockedIncrement(&myEnvironment_->numberOfMessagesDropped_);
						// float environmentDroppedMessagePercentage = ((float)environmentNumberOfMessagesDropped)/((float)myEnvironment_->numberOfMessagesReceived_)*100;
						RMTRACE->MessageDropped(sessionHeader.SessionId,sessionHeader.SequenceNumber,"Session Message Pool Exceeded");
						return;
					}

					RMTRACE->InboundMessageBuffered(sessionHeader.SessionId, sessionHeader.SequenceNumber);

					// We need to keep it alive with AddRef as we exit scope
					contentMessageSPtr->AddRef();

					contentMessageSPtr->Initialize(std::move(localMessage), responseTarget,sessionHeader);
					enqResult = inboundSession->InsertInbound(contentMessageSPtr);

					if (!enqResult.IsSuccess())
					{
						// return to pool
						inboundSession->ReturnQueuedInboundMessage(contentMessageSPtr);

						// unsigned __int64 environmentNumberOfMessagesDropped = InterlockedIncrement(&myEnvironment_->numberOfMessagesDropped_);
						// float environmentDroppedMessagePercentage = ((float)environmentNumberOfMessagesDropped)/((float)myEnvironment_->numberOfMessagesReceived_)*100;
						RMTRACE->MessageDropped(sessionHeader.SessionId,sessionHeader.SequenceNumber,"Session Enqueue Failure");

						// Send a message Nack for sessionId and sequenceNumber

						ReliableMessageId messageId(sessionHeader.SessionId,sessionHeader.SequenceNumber);
						Common::ErrorCode responseResult = ReliableSessionService::SendAck(
																						ProtocolAction::SendReliableMessageAck,
																						messageId,
																						enqResult,
																						responseTarget,
																						myEnvironment_);
					}
				}
				else
				{
						ReliableMessageId messageId(sessionHeader.SessionId,sessionHeader.SequenceNumber);
						Common::ErrorCode responseResult = ReliableSessionService::SendAck(
																						ProtocolAction::SendReliableMessageAck,
																						messageId,
																						Common::ErrorCodeValue::ReliableSessionNotFound,
																						responseTarget,
																						myEnvironment_);
				}

				KFinally([&]()
				{
					if (contentMessageSPtr != nullptr)
					{
						if (!enqResult.IsSuccess())
						{
							// Release ref. as insert message Ops failed.
							contentMessageSPtr->Release();
						}
					}
				});

				return;
			}

		case ProtocolAction::SendReliableMessageAck:
			{
				// Enueue to the right session queue via helper
				ReliableSessionServiceSPtr outboundSession = nullptr;
				bool sessionFound = false;

				K_LOCK_BLOCK(myEnvironment_->sessionServiceCommonLock_)
				{
					// TODO: make sure hash table structure has a const Find key parameter (unlike KAvlTree) so we can drop this extra assignment
					Common::Guid sessionId = sessionHeader.SessionId;
					ReliableSessionService *outboundSessionRawPtr = nullptr;
					sessionFound = myEnvironment_->commonReliableOutboundSessionsById_.Find(sessionId,outboundSessionRawPtr);
					outboundSession = outboundSessionRawPtr;
				}

				if (sessionFound)
				{
					// we don't know if the response is positive or nagative: let the session handle it
					ReliableMessagingProtocolResponseHeader protocolResponseHeader;
					headerFound = localMessage->Headers.TryReadFirst(protocolResponseHeader);
					ASSERT_IF(!headerFound, "No Protocol Response Header in SendReliableMessageAck Message");

					outboundSession->ProcessMessageAck(sessionHeader.SequenceNumber, protocolResponseHeader.ProtocolResponse);
				}
				else
				{
					RMTRACE->MessageAckDropped(sessionHeader.SessionId,sessionHeader.SequenceNumber,"Session Not Found");
				}

				return;
			}

		default: // session protocol functional messages may require service callbacks and trigger complex behavior -- defer through an async queue
			{
				QueuedInboundMessageSPtr protocolMessageSPtr = myEnvironment_->GetQueuedInboundMessage();
				NTSTATUS status;

				if (protocolMessageSPtr==nullptr)
				{
					// TODO: think through this situation -- just dropping the message right now -- need throttling protocol
					// unsigned __int64 environmentNumberOfMessagesDropped = InterlockedIncrement(&myEnvironment_->numberOfMessagesDropped_);
					// float environmentDroppedMessagePercentage = ((float)environmentNumberOfMessagesDropped)/((float)myEnvironment_->numberOfMessagesReceived_)*100;
					RMTRACE->MessageDropped(sessionHeader.SessionId,sessionHeader.SequenceNumber,"Transport Message Pool Exceeded");
					return;
				}

				protocolMessageSPtr->Initialize(std::move(localMessage), responseTarget,sessionHeader);

				// We need to keep it alive with AddRef as we exit scope
				protocolMessageSPtr->AddRef();
				status = inboundAsyncTransportQueue_->Enqueue(*protocolMessageSPtr);

				if (!NT_SUCCESS(status))
				{
					// unsigned __int64 environmentNumberOfMessagesDropped = InterlockedIncrement(&myEnvironment_->numberOfMessagesDropped_);
					// float environmentDroppedMessagePercentage = ((float)environmentNumberOfMessagesDropped)/((float)myEnvironment_->numberOfMessagesReceived_)*100;
					RMTRACE->MessageDropped(sessionHeader.SessionId,sessionHeader.SequenceNumber,"Transport Enqueue Failure");
					myEnvironment_->ReturnQueuedInboundMessage(protocolMessageSPtr);

				}

				KFinally([&]()
				{
					if (protocolMessageSPtr != nullptr)
					{
						if (!NT_SUCCESS(status))
						{
							// Release the Ref as Enqueue() ops failed.
							protocolMessageSPtr->Release();
						}
					}
				});

				return;
			}
		}
	}


	void ReliableMessagingTransport::AsyncDequeueCallback(
			        KAsyncContextBase* const,           
					KAsyncContextBase& CompletingSubOp      // CompletingSubOp
					)
	{
		InboundMessageQueue::DequeueOperation *completedDequeueOp 
			= static_cast<InboundMessageQueue::DequeueOperation*>(&CompletingSubOp);

		// Avoid processing the DequeueOperations being flushed by Deactivate of the AsyncQueue
		if (!NT_SUCCESS(completedDequeueOp->Status())) return;

		QueuedInboundMessage& queuedInboundMessage = completedDequeueOp->GetDequeuedItem();

		// Reset the completedDequeueOp for reuse
		completedDequeueOp->Reuse();

		// Reinsert as a waiter for inbound transport messages
		completedDequeueOp->StartDequeue(nullptr,inboundDequeuecallback_);

		/* look at the index in the message header and check against appropriate message in the array */
		Transport::MessageUPtr realMessage;
		queuedInboundMessage.GetMessageUPtr(realMessage);

		ISendTarget::SPtr responseTarget = queuedInboundMessage.ResponseTarget;
		ReliableSessionHeader receivedSessionHeader = queuedInboundMessage.SessionHeader;

		QueuedInboundMessageSPtr messageToReturnToPool = nullptr;
		messageToReturnToPool.Attach(&queuedInboundMessage);
		myEnvironment_->ReturnQueuedInboundMessage(messageToReturnToPool);

		Transport::ActionHeader receivedActionHeader;
		ReliableMessagingSourceHeader receivedSourceHeader;
		ReliableMessagingTargetHeader receivedTargetHeader;
		ReliableMessagingServicePartitionSPtr  sourceServicePartition = nullptr;
		ReliableMessagingServicePartitionSPtr  targetServicePartition = nullptr;

		RMTRACE->TransportReceivedMessage(ProtocolAction::CodeAsString(receivedSessionHeader.ActionCode),receivedSessionHeader.SessionId,receivedSessionHeader.SequenceNumber);

		ProtocolAction::Code messageActionCode = receivedSessionHeader.ActionCode;

		bool headerFound = false;

		ASSERT_IF(
			messageActionCode == ProtocolAction::SendReliableMessage || messageActionCode == ProtocolAction::SendReliableMessageAck, 
			"SendReliableMessage/Ack ActionCode encountered in Transport Async Queue");

		// SendReliableMessage and SendReliableMessageAck don't have and don't need source and target partition headers
		headerFound = realMessage->Headers.TryReadFirst(receivedSourceHeader);
		ASSERT_IF(!headerFound, "No SessionSourceHeader Header in Reliable Session Message");
					
		headerFound = realMessage->Headers.TryReadFirst(receivedTargetHeader);
		ASSERT_IF(!headerFound, "No SessionTargetHeader Header in Reliable Session  Message");

		Common::ErrorCode result = ReliableMessagingServicePartition::Create(receivedSourceHeader.ServiceInstanceName,
																			receivedSourceHeader.ServicePartitionKey,
																			sourceServicePartition);

		// TODO: this is actually not a coding error -- not clear what the recourse is if we run out of resources here: a Nack?  Not sure that would work
		// Is this another pooling opportunity?
		ASSERT_IF(!result.IsSuccess(), "Failed to initialize Source Service Partition in Reliable Session  Message");

		result = ReliableMessagingServicePartition::Create(receivedTargetHeader.ServiceInstanceName,
																			receivedTargetHeader.ServicePartitionKey,
																			targetServicePartition);

		// TODO: this is actually not a coding error -- not clear what the recourse is if we run out of resources here: a Nack?  Not sure that would work
		// Is this another pooling opportunity?
		ASSERT_IF(!result.IsSuccess(), "No SessionTargetHeader Header in Reliable Session  Message");
		
				
		Common::Guid sessionId = receivedSessionHeader.SessionId;

		// ALways ack protocol messages to stop retry
		ReliableMessageId messageId(sessionId,0);

		ProtocolAction::Code ackCode = ReliableSessionService::AckCodeFor(receivedSessionHeader.ActionCode);

		// The result of this action is ignored: if the Ack is not successful retry will occur from the sender
		ReliableSessionService::SendAck(
									ackCode,
									messageId,
									Common::ErrorCodeValue::Success,
									responseTarget,
									myEnvironment_);

		switch (receivedSessionHeader.ActionCode)
		{
		case ProtocolAction::OpenReliableSessionRequest:
			{
				Common::ErrorCode operationResult = SessionManagerService::ReliableInboundSessionRequested(
																							targetServicePartition,
																							sourceServicePartition,
																							responseTarget,
																							sessionId,
																							myEnvironment_);

				// Successful Ack will come out of the open completion callback
				if (!operationResult.IsSuccess())
				{
					Common::ErrorCode responseResult = ReliableSessionService::SendProtocolResponseMessage(
																						ProtocolAction::OpenReliableSessionResponse,
																						messageId,
																						operationResult,
																						responseTarget,
																						myEnvironment_,
																						targetServicePartition,
																						sourceServicePartition);
				}

				break;
			}

		case ProtocolAction::OpenReliableSessionResponse:
			{
				ReliableMessagingProtocolResponseHeader protocolResponseHeader;
				headerFound = realMessage->Headers.TryReadFirst(protocolResponseHeader);
				ASSERT_IF(!headerFound, "No Protocol Response Header in Open Reliable Session Ack Message");

				Common::ErrorCode protocolResponse = protocolResponseHeader.ProtocolResponse;

				ReliableSessionParametersHeader sessionParametersHeader;
				if (protocolResponse.IsSuccess())
				{
					headerFound = realMessage->Headers.TryReadFirst(sessionParametersHeader);
					ASSERT_IF(!headerFound, "No Session Parameters Header in Open Reliable Session Success Ack Message");
				}

				// TODO: do we need these static methods?  Should we just do the search here instead?
				ReliableSessionService::CompleteOpenSessionProtocol(sessionId,protocolResponse,sessionParametersHeader,myEnvironment_);

				break;
			}

		case ProtocolAction::CloseReliableSessionRequest:
			{
				ReliableSessionServiceSPtr sessionToClose = nullptr;
				bool sessionFound = false;

				K_LOCK_BLOCK(myEnvironment_->sessionServiceCommonLock_)
				{
					ReliableSessionService *inboundSessionRawPtr = nullptr;
					sessionFound = myEnvironment_->commonReliableInboundSessionsById_.Find(sessionId,inboundSessionRawPtr);
					sessionToClose = inboundSessionRawPtr;
				}

				// presume success, if session is not found this will result in dropping the message which is the right thing to do
				Common::ErrorCode closeStartResult = Common::ErrorCodeValue::Success;

				if (sessionFound)
				{
					closeStartResult = sessionToClose->StartCloseSessionInbound(receivedSessionHeader);
				}


				// Successful Ack will come out of the close completion callback
				if (!closeStartResult.IsSuccess())
				{
					Common::ErrorCode responseResult = ReliableSessionService::SendProtocolResponseMessage(
																						ProtocolAction::CloseReliableSessionResponse,
																						messageId,
																						closeStartResult,
																						responseTarget,
																						myEnvironment_,
																						targetServicePartition,
																						sourceServicePartition);
				}

				// TODO: what do we do if responseResult is failure?

				break;
			}

		case ProtocolAction::CloseReliableSessionResponse:
			{
				ReliableMessagingProtocolResponseHeader protocolResponseHeader;
				headerFound = realMessage->Headers.TryReadFirst(protocolResponseHeader);
				ASSERT_IF(!headerFound, "No Protocol Response Header in Close Reliable Session Ack Message");

				Common::ErrorCode protocolResponse = protocolResponseHeader.ProtocolResponse;

				// TODO: do we need these static methods?
				ReliableSessionService::CompleteCloseSessionProtocol(sessionId,protocolResponse,myEnvironment_);

				break;
			}

		case ProtocolAction::AbortOutboundReliableSessionRequest:
			{
				ReliableSessionServiceSPtr sessionToAbort = nullptr;
				bool sessionFound = false;

				K_LOCK_BLOCK(myEnvironment_->sessionServiceCommonLock_)
				{
					ReliableSessionService *outboundSessionRawPtr = nullptr;
					sessionFound = myEnvironment_->commonReliableOutboundSessionsById_.Find(sessionId,outboundSessionRawPtr);
					sessionToAbort = outboundSessionRawPtr;
				}

				if (sessionFound)
				{
					// Nothing to do if AbortSession fails: usually because the session is already closed
					sessionToAbort->AbortSession(ReliableSessionService::AbortOutboundSessionRequestMessage);
				}
			}

		case ProtocolAction::AbortInboundReliableSessionRequest:
			{
				ReliableSessionServiceSPtr sessionToAbort = nullptr;
				bool sessionFound = false;

				K_LOCK_BLOCK(myEnvironment_->sessionServiceCommonLock_)
				{
					ReliableSessionService *inboundSessionRawPtr = nullptr;
					sessionFound = myEnvironment_->commonReliableInboundSessionsById_.Find(sessionId,inboundSessionRawPtr);
					sessionToAbort = inboundSessionRawPtr;
				}

				if (sessionFound)
				{
					// Nothing to do if AbortSession fails: usually because the session is already closed
					sessionToAbort->AbortSession(ReliableSessionService::AbortInboundSessionRequestMessage);
				}
			}
		}
	}
}


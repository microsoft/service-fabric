// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

using namespace Common;

namespace ReliableMessaging
{
	// these three declarations are used in ShutDown below
	typedef KNodeWrapper<ReliableSessionServiceSPtr> KNodeReliableSessionService;
	typedef KSharedPtr<KNodeReliableSessionService> KNodeReliableSessionServiceSPtr;

	const ULONG KNodeReliableSessionService::LinkOffset = FIELD_OFFSET(KNodeReliableSessionService, qlink_);

	K_FORCE_SHARED_NO_OP_DESTRUCTOR(KNodeReliableSessionService)
	K_FORCE_SHARED_NO_OP_DESTRUCTOR(SessionManagerService)

	SessionManagerService::SessionManagerService(
			Common::Guid ownerPartitionId,
			FABRIC_REPLICA_ID ownerReplicaId,
			ReliableMessagingServicePartitionSPtr const &ownerServicePartition, 
			ReliableMessagingEnvironmentSPtr myEnvironment) : 
			ownerPartitionId_(ownerPartitionId),
			ownerReplicaId_(ownerReplicaId),
			inboundSessionCallback_(nullptr), 
			ownerServicePartition_(ownerServicePartition), 
			myEnvironment_(myEnvironment),
			outboundSessions_(&(ReliableMessagingServicePartition::compare),Common::GetSFDefaultPagedKAllocator())
		{
			traceId_.reserve(60);
			StringWriter(traceId_).Write("{0}::{1}", ownerPartitionId_, ownerReplicaId_);
		}

	// There is a singleton instance of this class for each calling service--created by this static method
	// the existing session manager will be returned on repeat calls, unless it has been released in the mean time
	Common::ErrorCode SessionManagerService::Create( 
						__in Common::Guid ownerPartitionId,
						__in FABRIC_REPLICA_ID ownerReplicaId,
						__in ReliableMessagingServicePartitionSPtr &ownerServicePartition,
						__out SessionManagerServiceSPtr &sessionManager,
						__in_opt ReliableMessagingEnvironmentSPtr environment)
	{
		// Synchronous call on client API thread
		{
			//#pragma warning( disable : 4100 )				
			
			sessionManager = nullptr;
			
			ReliableMessagingEnvironmentSPtr myEnvironment = environment;

			if (myEnvironment==nullptr)
			{
				myEnvironment = ReliableMessagingEnvironment::TryGetsingletonEnvironment();

				if (myEnvironment==nullptr)
				{
					return Common::ErrorCodeValue::OutOfMemory;
				}
			}

			K_LOCK_BLOCK(myEnvironment->sessionManagerCommonLock_)
			{
				Common::ErrorCode result;

				BOOLEAN managerFound = myEnvironment->sessionManagers_.Find(ownerServicePartition,sessionManager);

				if (managerFound)
				{
					return Common::ErrorCodeValue::ReliableSessionManagerExists;
				}
				else
				{
					SessionManagerServiceSPtr newMgr 
						= _new (RMSG_ALLOCATION_TAG, Common::GetSFDefaultPagedKAllocator()) SessionManagerService(ownerPartitionId, ownerReplicaId, ownerServicePartition,myEnvironment);

					if (newMgr == nullptr || !NT_SUCCESS(newMgr->Status())) 
					{
						// TODO: do we need to handle the case that we just created the singletonEnvironment_ and should shut it down?
						// Revisit after we make the default KtlSystem the singleton hook
						sessionManager = nullptr;
						return Common::ErrorCodeValue::OutOfMemory;
					}

					NTSTATUS insertStatus =
						myEnvironment->sessionManagers_.Insert(newMgr,ownerServicePartition);

					if (NT_SUCCESS(insertStatus) && ownerServicePartition->PartitionId.KeyType == FABRIC_PARTITION_KEY_TYPE_INT64)
					{
						// special secondary index to find requested Int64 partition number in the owner's range
						insertStatus = myEnvironment->sessionManagerInboundSessionRequestFinderIndex_.Insert(newMgr,ownerServicePartition);
					}

					if (!NT_SUCCESS(insertStatus))
					{
						sessionManager = nullptr;
						return Common::ErrorCodeValue::OutOfMemory;
					}

					sessionManager = newMgr;
				}
			}
				
			return Common::ErrorCodeValue::Success;
		}
	}

	Common::ErrorCode SessionManagerService::StartOpenSessionManager(
			__in IFabricReliableSessionAbortedCallback *sessionCallback,
			__in_opt KAsyncContextBase* const ParentAsync, 
            __in OpenCompletionCallback OpenCallbackPtr,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride)
	{
		ReliableSessionAbortedCallback *newCallbackWrapper = _new (RMSG_ALLOCATION_TAG,Common::GetSFDefaultPagedKAllocator()) ReliableSessionAbortedCallback(sessionCallback);

		if (newCallbackWrapper == nullptr || !NT_SUCCESS(newCallbackWrapper->Status()))
		{
			RMTRACE->OutOfMemory("SessionManagerService::StartOpenSessionManager");
			return Common::ErrorCodeValue::OutOfMemory;
		}
		else
		{
			sessionAbortedCallback_ = KSharedPtr<ReliableSessionAbortedCallback>(newCallbackWrapper);
		}

		NTSTATUS status = StartOpen(ParentAsync,OpenCallbackPtr,GlobalContextOverride);
		return Common::ErrorCode::FromNtStatus(status);
	}

	void SessionManagerService::OnServiceOpen()
	{
		BOOLEAN completeResult = CompleteOpen(Common::ErrorCodeValue::Success);
		ASSERT_IFNOT(completeResult==TRUE, "Unexpected CompleteOpen failure in SessionManagerService::OnServiceOpen");
	}

	Common::ErrorCode SessionManagerService::StartCloseSessionManager(
        __in_opt KAsyncContextBase* const ParentAsync, 
        __in CloseCompletionCallback CloseCallbackPtr)
	{
		// StartClose synchronously changes service state to disallow any further activities being acquired via TryArcquireServiceActivity
		// Thus no new inbound or outbound sessions can be created via this session manager
		NTSTATUS status = StartClose(ParentAsync,CloseCallbackPtr);

		if (status==STATUS_PENDING)
		{
			ShutDown();
			return Common::ErrorCodeValue::Success;
		}
		else
		{
			return Common::ErrorCode::FromNtStatus(status);
		}
	}

	// This method is called after all service activities have completed, i.e., all sessions have been released
	void SessionManagerService::OnServiceClose()
	{
		BOOLEAN completeResult = CompleteClose(Common::ErrorCodeValue::Success);
		ASSERT_IFNOT(completeResult==TRUE, "Unexpected CompleteClose failure in SessionManagerService::OnServiceClose");
	}

	// TODO: do we need a DeleteOutboundSession API?  Will that close automatically?  Perhaps at the managed level ..

	// creates a session object locally but must be "opened" to activate wire protocol
	// the outbound session in the scope of a single session manager for a given target service
	// instance is a singleton: the existing session will be returned on repeat calls but with an error HRESULT
    Common::ErrorCode SessionManagerService::CreateOutboundSession( 
						__in ReliableMessagingServicePartitionSPtr &targetServicePartition,
						__in std::wstring const &targetEndpoint,
						__out ReliableSessionServiceSPtr &session
						)
	{
		RMTRACE->BeginCreateOutboundSessionStart(traceId_, Common::Guid::Empty());

		K_LOCK_BLOCK(sessionManagerInstanceLock_)
		{
			// Synchronous call on client API thread

			// acquire an activity to represent the session -- do this inside the lock so that the ShutDown process will not race with this
			ACQUIRE_ACTIVITY_VERIFY(SessionManagerService::CreateOutboundSession)

			// The existing session will not get back to the caller in the managed API because of the way Task completion works
			BOOLEAN sessionFound = outboundSessions_.Find(targetServicePartition,session);

			if (sessionFound)
			{
				// the session that was found is assigned to the out parameter even though we are returning an error
				return Common::ErrorCodeValue::ReliableSessionAlreadyExists;
			}
			else
			{
				ISendTarget::SPtr partnerSendTarget = nullptr;
				Common::ErrorCode resolveResult = myEnvironment_->transport_->ResolveTarget(targetEndpoint,partnerSendTarget);

				if (!resolveResult.IsSuccess()) 
				{
					RMTRACE->CommunicationFailure("ReliableSessionCannotConnect","SessionManagerService::CreateOutboundSession", resolveResult.ErrorCodeValueToString());
					ERROR_RETURN_ACTIVITY_RELEASE(this,SessionManagerService::CreateOutboundSession,resolveResult.ReadValue())
				}

				if (partnerSendTarget == nullptr) 
				{
					ERROR_RETURN_ACTIVITY_RELEASE(this,SessionManagerService::CreateOutboundSession,Common::ErrorCodeValue::ReliableSessionCannotConnect)
				}

				ReliableSessionServiceSPtr newSession = _new (RMSG_ALLOCATION_TAG, GetThisAllocator()) 
																	ReliableSessionService(
																				ownerServicePartition_,
																				targetServicePartition,
																				KSharedPtr<SessionManagerService>(this),
																				partnerSendTarget,
																				SessionKind::Outbound,
																				Common::Guid::NewGuid(),
																				myEnvironment_);

				if (newSession == nullptr || !NT_SUCCESS(newSession->Status()))
				{
					ERROR_RETURN_ACTIVITY_RELEASE(this,SessionManagerService::CreateOutboundSession,Common::ErrorCodeValue::OutOfMemory)
				}

				NTSTATUS status =
					outboundSessions_.Insert(newSession,targetServicePartition);

				if (!NT_SUCCESS(status))
				{
					ERROR_RETURN_ACTIVITY_RELEASE(this,SessionManagerService::CreateOutboundSession,Common::ErrorCodeValue::OutOfMemory)
				}

				session = newSession;
			}
		}

		RMTRACE->BeginCreateOutboundSessionFinish(traceId_, session->SessionId);
		return Common::ErrorCodeValue::Success;
	}
       
	Common::ErrorCode SessionManagerService::SessionRequestedCallback(
		__in ReliableMessagingServicePartitionSPtr const &source,	// source partition
		__in ReliableSessionServiceSPtr const &newSession,			// session offered
		__out LONG &response
		)
	{
		Common::ErrorCode result;

		K_LOCK_BLOCK(sessionManagerInstanceLock_)
		{
			// don't want the callback unregistered while we are calling back
		
			if (inboundSessionCallback_ == nullptr)
			{
				result = Common::ErrorCodeValue::ReliableSessionManagerNotListening;
			}
			else
			{
				RMTRACE->SessionRequestedCallbackStarting(traceId_,newSession->SessionId);
				result = inboundSessionCallback_->SessionRequested(source,newSession,response);
				RMTRACE->SessionRequestedCallbackReturned(traceId_,newSession->SessionId,response);
			}
		}

		return result;
	}

	void SessionManagerService::SessionAbortedCallback(
		__in SessionKind kind,
		__in ReliableMessagingServicePartitionSPtr const &source,	// source partition
		__in ReliableSessionServiceSPtr const &closedSession		// session closed
		)
	{
		ASSERT_IF(KSHARED_IS_NULL(sessionAbortedCallback_), "Null session closed callback in open session manager");
		//RMTRACE->SessionAbortedCallbackStarting(closedSession->SessionId);
		sessionAbortedCallback_->SessionAbortedByPartner(kind,source,closedSession);
		//RMTRACE->SessionAbortedCallbackReturned(closedSession->SessionId);
	}


	// starts listening for inbound sessions and registers a callback interface
	// can only do this once per service: ReliableSessionManagerAlreadyListening returned on repeat
    Common::ErrorCode SessionManagerService::RegisterInboundSessionCallback( 
						__in IFabricReliableInboundSessionCallback *listenerCallback,
						__out std::wstring &sessionListenerEndpoint)
	{
		// Synchronous call on client API thread
		K_LOCK_BLOCK(sessionManagerInstanceLock_)
		{
			if (inboundSessionCallback_ != nullptr) return Common::ErrorCodeValue::ReliableSessionManagerAlreadyListening;

			InboundSessionRequestedCallback *newCallbackWrapper = _new (RMSG_ALLOCATION_TAG,Common::GetSFDefaultPagedKAllocator()) InboundSessionRequestedCallback(listenerCallback);

			if (newCallbackWrapper == nullptr || !NT_SUCCESS(newCallbackWrapper->Status()))
			{
				RMTRACE->OutOfMemory("SessionManagerService::RegisterInboundSessionCallback");
				return Common::ErrorCodeValue::OutOfMemory;
			}
			else
			{
				inboundSessionCallback_ = KSharedPtr<InboundSessionRequestedCallback>(newCallbackWrapper);
				sessionListenerEndpoint = myEnvironment_->transport_->ListenAddress;
			}
		}

		return Common::ErrorCodeValue::Success;
	} // release sessionManagerInstanceLock_ writeHold
        


	// Unregisters the registered callback interface and stops listening for inbound sessions for this service
	// Now open for RegisterReliableInboundSessionCallback again
    Common::ErrorCode SessionManagerService::UnregisterReliableInboundSessionCallback()	
	{
		K_LOCK_BLOCK(sessionManagerInstanceLock_)
		{
			// Synchronous call on client API thread
			inboundSessionCallback_ = nullptr;
		}
		return Common::ErrorCodeValue::Success;
	}
        
	// Release and unregister the session manager for the owner service instance
	// This is not an idempotent operation -- it will assert if called more than once

	// we assume that all session resources have been released before shutdown so this is fast
	Common::ErrorCode SessionManagerService::ShutDown()
	{
		// Abort all registered sessions
		// 	typedef KSharedPtr<ReliableMessagingServicePartition> ReliableMessagingServicePartitionSPtr;

		PARTITION_TRACE(SessionManagerShutdownStarted,ownerServicePartition_)

		KNodeListShared<KNodeReliableSessionService> inboundSessionsToRelease(KNodeReliableSessionService::LinkOffset);
		KNodeListShared<KNodeReliableSessionService> outboundSessionsToRelease(KNodeReliableSessionService::LinkOffset);

		K_LOCK_BLOCK(sessionManagerInstanceLock_)
		{
			KAvlTree<SecondaryIndex::SPtr,ReliableMessagingServicePartitionSPtr>::Iterator inboundIterator = inboundSessionsSourceAndTargetIndex_.GetIterator();
			ReliableSessionServiceOwnerMap::Iterator outboundIterator = outboundSessions_.GetIterator();
			
			// TODO: currently we just abort all inbound sessions, need a more nuanced shutdown semantic if sessions are used standalone, i.e., not via streams
			for (inboundIterator.First(); inboundIterator.IsValid(); inboundIterator.Next())
			{
				SecondaryIndex::SPtr index = inboundIterator.Get();
				ReliableSessionServiceOwnerMap::Iterator inboundIndexIterator = index->GetIterator();

				for (inboundIndexIterator.First(); inboundIndexIterator.IsValid(); inboundIndexIterator.Next())
				{
					ReliableSessionServiceSPtr inboundSession = inboundIndexIterator.Get();

					// we need to remember this and then call abort later to avoid deadlock on sessionManagerInstanceLock_
					// We need to wrap in a new KNodeReliableSessionService because the KListEntry of the session service
					// is already in use in a hash table
					KNodeReliableSessionServiceSPtr newNode = _new (RMSG_ALLOCATION_TAG, GetThisAllocator()) 
																	KNodeReliableSessionService(inboundSession);

					if (KSHARED_IS_NULL(newNode))
					{
						RMTRACE->KtlObjectCreationFailed("KNodeReliableSessionService");
						return Common::ErrorCodeValue::OutOfMemory;
					}
					else if (!NT_SUCCESS(newNode->Status()))
					{
						RMTRACE->KtlObjectCreationFailed("KNodeReliableSessionService");
						return Common::ErrorCode::FromNtStatus(newNode->Status());
					}

					inboundSessionsToRelease.AppendTail(newNode);
				}
			}

				// TODO: currently we just abort all outbound sessions, need a more nuanced shutdown semantic if sessions are used standalone, i.e., not via streams
			for (outboundIterator.First(); outboundIterator.IsValid(); outboundIterator.Next())
			{
				ReliableSessionServiceSPtr outboundSession = outboundIterator.Get();
				
				// we need to remember this and then call abort later to avoid deadlock on sessionManagerInstanceLock_
				// We need to wrap in a new KNodeReliableSessionService because the KListEntry of the session service 
				// is already in use in a hash table
				KNodeReliableSessionServiceSPtr newNode = _new (RMSG_ALLOCATION_TAG, GetThisAllocator()) 
																KNodeReliableSessionService(outboundSession);

				if (KSHARED_IS_NULL(newNode))
				{
					RMTRACE->KtlObjectCreationFailed("KNodeReliableSessionService");
					return Common::ErrorCodeValue::OutOfMemory;
				}
				else if (!NT_SUCCESS(newNode->Status()))
				{
					RMTRACE->KtlObjectCreationFailed("KNodeReliableSessionService");
					return Common::ErrorCode::FromNtStatus(newNode->Status());
				}

				outboundSessionsToRelease.AppendTail(newNode);
			}
		}

		for (KNodeReliableSessionServiceSPtr inboundSessionWrapper = inboundSessionsToRelease.RemoveHead(); 
			 !KSHARED_IS_NULL(inboundSessionWrapper); 
		     inboundSessionWrapper = inboundSessionsToRelease.RemoveHead())
		{

			inboundSessionWrapper->GetContent()->AbortSession(ReliableSessionService::AbortSessionForManagerClose);
		}

		for (KNodeReliableSessionServiceSPtr outboundSessionWrapper = outboundSessionsToRelease.RemoveHead(); 
			 !KSHARED_IS_NULL(outboundSessionWrapper); 
		     outboundSessionWrapper = outboundSessionsToRelease.RemoveHead())
		{

			outboundSessionWrapper->GetContent()->AbortSession(ReliableSessionService::AbortSessionForManagerClose);
		}

		// Unregister self from the session managers map

		K_LOCK_BLOCK(myEnvironment_->sessionManagerCommonLock_)
		{
			SessionManagerServiceSPtr manager;
			BOOLEAN managerFound 
				= myEnvironment_->sessionManagers_.Find(ownerServicePartition_,manager);

			ASSERT_IF((!managerFound || manager.RawPtr() != this), 
											"Active Reliable Session Manager Not Found in Session Manager Map");

			// remove from map
			BOOLEAN removeSuccess = myEnvironment_->sessionManagers_.Remove(ownerServicePartition_);

			ASSERT_IF(removeSuccess==FALSE,"Reliable Session Manager found but could not be removed from Session Manager Map");

			if (ownerServicePartition_->PartitionId.KeyType == FABRIC_PARTITION_KEY_TYPE_INT64)
			{
				// special secondary index to find requested Int64 partition number in the owner's range
				removeSuccess = myEnvironment_->sessionManagerInboundSessionRequestFinderIndex_.Remove(ownerServicePartition_);
			}

			ASSERT_IF(removeSuccess==FALSE,"Int64 Range Reliable Session Manager found but could not be removed from secondary search index");

			if (myEnvironment_->sessionManagers_.IsEmpty()) 
			{
				// TODO: No messaging resources active, should we shut down and delete the transport?  For now, no, it doesn't save much
				// TODO:  do we erase the singleton environment now if myEnvironment_ is the singleton?
			}
		}

		PARTITION_TRACE(SessionManagerShutdownCompleted,ownerServicePartition_)

		return Common::ErrorCodeValue::Success;
	}

	/*
	BOOLEAN SessionManagerService::EraseSessionFromPartitionMap(ReliableSessionServiceSPtr sessionToRemove, SessionKind sessionKind)
	{
		BOOLEAN removeSuccess = FALSE;

		ReliableSessionServiceOwnerMap& sessionMap = (sessionKind == SessionKind::Inbound) ? inboundSessionsSourceAndTargetIndex_ : outboundSessions_;

		K_LOCK_BLOCK(sessionManagerInstanceLock_)
		{
			ReliableSessionServiceSPtr mappedSession = nullptr;

			ReliableMessagingServicePartitionSPtr key = (sessionKind == SessionKind::Inbound) ? sessionToRemove->SourceServicePartition : sessionToRemove->TargetServicePartition;

			BOOLEAN sessionFound = sessionMap.Find(key,mappedSession);

			if (sessionFound && mappedSession->SessionId == sessionToRemove->SessionId)
			{
				// we have the right session, erase the session object from the session map
				removeSuccess = sessionMap.Remove(key);
			}
		}

		return removeSuccess;
	}
	*/

	// Session Manager instance method
	// This will either return error or start the open process on a new or existing session and return success
	// In case it returns success the start process is responsible for sending an ack otherwise the caller is
	Common::ErrorCode SessionManagerService::ReliableInboundSessionRequested( 
				__in ReliableMessagingServicePartitionSPtr &targetServicePartition,
				__in ReliableMessagingServicePartitionSPtr &sourceServicePartition,
				__in ISendTarget::SPtr const &responseTarget,
				__in Common::Guid const &sessionId)
	{

		if (inboundSessionCallback_ == nullptr) 
		{
			ERROR_RETURN_ACTIVITY_RELEASE(this,SessionManagerService::ReliableInboundSessionRequested,Common::ErrorCodeValue::ReliableSessionManagerNotListening)
		}

		ReliableSessionServiceSPtr session = nullptr;
		bool conflictingSessionFound = false;

		K_LOCK_BLOCK(sessionManagerInstanceLock_)
		{
			// acquire an activity to represent the session -- do this inside the lock so that the ShutDown process will not race with this
			ACQUIRE_ACTIVITY_VERIFY(SessionManagerService::ReliableInboundSessionRequested)

			BOOLEAN sessionFound = inboundSessionsSourceAndTargetIndex_.Find(sourceServicePartition,targetServicePartition,session);

			if (sessionFound)
			{
				if (session->SessionId == sessionId)
				{
					// This must be a retry, returning success is equivalent to dropping the message, since transport layer only 
					// sends error responses immediately; the first open message we processed will generate the real response
					return Common::ErrorCodeValue::Success;
				}
				else
				{
					// The case where this is expected to occur is if a send-side service crashes without sending an Abort signal and tries to 
					// re-establish a session upon recovery.  Need to really look at security issues ..
					// This has to be done via an error return continuation to avoid locking conflicts
					conflictingSessionFound = true;
				}
			}
			else
			{
				// normal case, new session, no retry or conflict
				session = _new (RMSG_ALLOCATION_TAG, GetThisAllocator()) ReliableSessionService(
																							sourceServicePartition,
																							targetServicePartition,
																							KSharedPtr<SessionManagerService>(this),
																							responseTarget,
																							SessionKind::Inbound,
																							sessionId,
																							myEnvironment_);

				if (session==nullptr || !NT_SUCCESS(session->Status()))
				{
					ERROR_RETURN_ACTIVITY_RELEASE(this,SessionManagerService::ReliableInboundSessionRequested,Common::ErrorCodeValue::OutOfMemory)
				}

				Common::ErrorCode insertStatus =
					inboundSessionsSourceAndTargetIndex_.Insert(sourceServicePartition,targetServicePartition,session);

				if (!insertStatus.IsSuccess())
				{
					ERROR_RETURN_ACTIVITY_RELEASE(this,SessionManagerService::ReliableInboundSessionRequested,Common::ErrorCodeValue::OutOfMemory)
				}
			}
		} // release sessionManagerInstanceLock_ writeHold

		if (conflictingSessionFound)
		{
			RMTRACE->ConflictingInboundSessionAborted(traceId_,sessionId, session->SessionId);
			session->AbortSession(ReliableSessionService::AbortConflictingInboundSession);
			ERROR_RETURN_ACTIVITY_RELEASE(this,SessionManagerService::ReliableInboundSessionRequested,Common::ErrorCodeValue::ReliableSessionConflictingSessionAborted)
		}
		else
		{
			// continue normal case, new session, no retry or conflict
			// The ack will be sent from the continuation of this StartOpen unless StartOpen returns an error
			Common::ErrorCode startResult = session->StartOpenSessionInbound();

			if (startResult.IsSuccess())
			{
				return Common::ErrorCodeValue::Success;
			}
			else
			{
				ReleaseSessionInbound(session);
				return startResult;
			}
		}
	}

	// static function to find the right session manager to request
	// This will either return error or start the open process on a new or existing session and return success
	// In case it returns success the start open process is responsible for sending an ack 
    Common::ErrorCode SessionManagerService::ReliableInboundSessionRequested( 
						__in ReliableMessagingServicePartitionSPtr &targetServicePartition,
						__in ReliableMessagingServicePartitionSPtr &sourceServicePartition,
						__in ISendTarget::SPtr const &responseTarget,
						__in Common::Guid const &sessionId,
						__in ReliableMessagingEnvironmentSPtr myEnvironment)
	{
		if (!targetServicePartition->IsValidTargetKey())
		{
			return Common::ErrorCodeValue::ReliableSessionInvalidTargetPartition;
		}

		SessionManagerServiceSPtr targetManager = nullptr;

		K_LOCK_BLOCK(myEnvironment->sessionManagerCommonLock_)
		{
			BOOLEAN managerFound = FALSE;

			if (targetServicePartition->PartitionId.KeyType == FABRIC_PARTITION_KEY_TYPE_INT64)
			{
				// special secondary index to find requested Int64 partition number in the owner's range
				managerFound = myEnvironment->sessionManagerInboundSessionRequestFinderIndex_.Find(targetServicePartition, targetManager);
			}
			else
			{
				managerFound = myEnvironment->sessionManagers_.Find(targetServicePartition, targetManager);
			}

			if (!managerFound)
			{
				return Common::ErrorCodeValue::ReliableSessionManagerNotFound; 
			}
		} // release sessionManagerStaticLock_ readHold

		// target session manager exists--invoke the instance method to complete
		Common::ErrorCode result = targetManager->ReliableInboundSessionRequested(targetServicePartition,sourceServicePartition,responseTarget,sessionId);

		if (result.ReadValue()==Common::ErrorCodeValue::ReliableSessionConflictingSessionAborted)
		{
			// try again, we cleaned the slate
			result = targetManager->ReliableInboundSessionRequested(targetServicePartition,sourceServicePartition,responseTarget,sessionId);
		}
			
		return result;
	}
}

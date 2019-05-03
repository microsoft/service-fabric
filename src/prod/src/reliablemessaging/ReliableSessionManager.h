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

	class SessionManagerService : public FabricAsyncDeferredClosureServiceBase 
	{
		K_FORCE_SHARED(SessionManagerService);

	public:

        __declspec (property(get=get_ownerServicePartition)) ReliableMessagingServicePartitionSPtr const &OwnerServicePartition;
        inline ReliableMessagingServicePartitionSPtr const &get_ownerServicePartition() const { return ownerServicePartition_; }

        __declspec (property(get=get_traceId)) std::wstring const &TraceId;
        inline std::wstring const &get_traceId() const { return traceId_; }

		/*** Methods that implement the COM API ***/

		// This is the entry point for all the rest of the functionality
		// There is a singleton insance of this class for each calling service--created by this static method
		static Common::ErrorCode Create( 
						__in Common::Guid ownerPartitionId,
						__in FABRIC_REPLICA_ID ownerReplicaId,
						__in ReliableMessagingServicePartitionSPtr &ownerServicePartition,
						__out SessionManagerServiceSPtr &sessionManager,
						__in_opt ReliableMessagingEnvironmentSPtr environment = nullptr);

		// the following three methods are called via the async wrappers used by thye COM API
		Common::ErrorCode CreateOutboundSession( 
						__in ReliableMessagingServicePartitionSPtr &targetServicePartition,
						__in std::wstring const &targetEndpoint,
						__out ReliableSessionServiceSPtr &session
						); // session name is an optional demux convenience for session owner

		// this method registers a callback also signifying the intention of the owner service to listen for inbound sessions
		Common::ErrorCode RegisterInboundSessionCallback( 
						__in IFabricReliableInboundSessionCallback *listenerCallback,
						__out std::wstring &sessionListenerEndpoint);

		// this method unregisters the callback also signifying the intention of the owner service to stop listening for inbound sessions
		Common::ErrorCode UnregisterReliableInboundSessionCallback();

		/*** End methods called by COM API ***/


		// Helper to perform the callback
		Common::ErrorCode SessionRequestedCallback(
			__in ReliableMessagingServicePartitionSPtr const &source,	// source partition
			__in ReliableSessionServiceSPtr const &newSession,			// session offered
			__out LONG &response
			);

		// Helper to perform the callback
		void SessionAbortedCallback(
			__in SessionKind kind,										// SessionKind from this manager's viewpoint
			__in ReliableMessagingServicePartitionSPtr const &source,	// source partition
			__in ReliableSessionServiceSPtr const &closedSession		// session closed
			);

		// static method called from transport
		static Common::ErrorCode ReliableInboundSessionRequested( 
						__in ReliableMessagingServicePartitionSPtr &targetServicePartition,
						__in ReliableMessagingServicePartitionSPtr  &sourceServicePartition,
						__in ISendTarget::SPtr const &responseTarget,
						__in Common::Guid const &sessionId,
						__in ReliableMessagingEnvironmentSPtr myEnvironment);

		// instance method called by static method after target manager is resolved
		Common::ErrorCode ReliableInboundSessionRequested( 
						__in ReliableMessagingServicePartitionSPtr &targetServicePartition,
						__in ReliableMessagingServicePartitionSPtr &sourceServicePartition,
						__in ISendTarget::SPtr const &responseTarget,
						__in Common::Guid const &sessionId);

		Common::ErrorCode StartOpenSessionManager(
			__in IFabricReliableSessionAbortedCallback *sessionCallback,
            __in_opt KAsyncContextBase* const ParentAsync, 
            __in OpenCompletionCallback OpenCallbackPtr,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr);

		Common::ErrorCode StartCloseSessionManager(
            __in_opt KAsyncContextBase* const ParentAsync, 
            __in CloseCompletionCallback CloseCallbackPtr);


		static Common::ErrorCode CreateAndStartSingletonEnvironment(__out ReliableMessagingEnvironmentSPtr&);

		Common::ErrorCode ShutDown();

		// called by a closing or aborting inbound session to release its registration and activity
		// This is the only place where the session should be erased from the inboundSessionsSourceAndTargetIndex_ map
		void ReleaseSessionInbound(ReliableSessionServiceSPtr closingSession)
		{
			// erase the session from the inbound sessions map

			BOOLEAN eraseSuccess = FALSE;

			K_LOCK_BLOCK(sessionManagerInstanceLock_)
			{
				ReliableSessionServiceSPtr mappedSession = nullptr;

				ReliableMessagingServicePartitionSPtr sourceKey = closingSession->SourceServicePartition;
				ReliableMessagingServicePartitionSPtr targetKey = closingSession->TargetServicePartition;

				BOOLEAN sessionFound = inboundSessionsSourceAndTargetIndex_.Find(sourceKey,targetKey,mappedSession);

				if (sessionFound && mappedSession->SessionId == closingSession->SessionId)
				{
					// we have the right session, erase the session object from the session map
					eraseSuccess = inboundSessionsSourceAndTargetIndex_.Remove(sourceKey,targetKey);
				}
			}

			if (!eraseSuccess)
			{
				// TODO: reconsider this assert: this can happen in a race between session manager abort and an incoming session open request
				// ASSERT_IFNOT(eraseSuccess==TRUE, "Service not found in PartitionKey Map whicle completing close inbound session protocol");
				RMTRACE->SessionEraseFailure(traceId_,"ReliableSessionService::OnServiceCloseInbound Partition Map");
			}				
			// release the long-running activity representing the session
			NORMAL_ACTIVITY_RELEASE(this,SessionManagerService::ReleaseSessionInbound);
		}

		// called by a closing or aborting outbound session to release its registration and activity
		// This is the only place where the session should be erased from the outboundSessions_ map
		void ReleaseSessionOutbound(ReliableSessionServiceSPtr closingSession)
		{
			BOOLEAN eraseSuccess = FALSE;

			K_LOCK_BLOCK(sessionManagerInstanceLock_)
			{
				ReliableSessionServiceSPtr mappedSession = nullptr;

				ReliableMessagingServicePartitionSPtr key = closingSession->TargetServicePartition;

				BOOLEAN sessionFound = outboundSessions_.Find(key,mappedSession);

				if (sessionFound && mappedSession->SessionId == closingSession->SessionId)
				{
					// we have the right session, erase the session object from the session map
					eraseSuccess = outboundSessions_.Remove(key);
				}
			}

			if (!eraseSuccess)
			{
				// TODO: reconsider this assert: this can happen in a race between session manager abort and a regular session close process
				// ASSERT_IFNOT(eraseSuccess==TRUE, "Service not found in PartitionKey Map whicle completing close outbound session protocol");
				RMTRACE->SessionEraseFailure(traceId_,"ReliableSessionService::OnServiceCloseInbound Partition Map");
			}	

			// release the long-running activity representing the session
			NORMAL_ACTIVITY_RELEASE(this,SessionManagerService::ReleaseSessionOutbound);
		}

		// TODO:  should we keep track of source service partition Address() from ISendTarget provided by transport
		// on inbound messages and react to changes?  These addresses will be IP not FQDN so may change due to DHCP
		// In Azure will they by VIPs or DIps


		SessionManagerService(
			Common::Guid ownerPartitionId,
			FABRIC_REPLICA_ID ownerReplicaId,
			ReliableMessagingServicePartitionSPtr const &ownerServicePartition, 
			ReliableMessagingEnvironmentSPtr myEnvironment);

	private:

		// BOOLEAN EraseSessionFromPartitionMap(ReliableSessionServiceSPtr service, SessionKind sessionKind);

		void OnServiceOpen() override;
		void OnServiceClose() override;

		// We need two tier a source target index for inbound sessions because the same source can legitimately
		// start multiple inbound sessions targeting Int64 range partitions: the taget from the source's point of view
		// is a single Int64 partition number but multiple of those happen to fall into the same target range
		TwoTierKAvlTree inboundSessionsSourceAndTargetIndex_;

		ReliableSessionServiceOwnerMap outboundSessions_;
		InboundSessionRequestedCallbackSPtr inboundSessionCallback_;
		ReliableSessionAbortedCallbackSPtr sessionAbortedCallback_;
		KSpinLock sessionManagerInstanceLock_;
		ReliableMessagingServicePartitionSPtr ownerServicePartition_;
		Common::Guid ownerPartitionId_;
		FABRIC_REPLICA_ID ownerReplicaId_;
		std::wstring traceId_;

		ReliableMessagingEnvironmentSPtr myEnvironment_;
	};
}

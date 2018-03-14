// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class PrimaryReplicator : public Common::TextTraceComponent<Common::TraceTaskCodes::Replication>
        {
            DENY_COPY(PrimaryReplicator);

        public:
            static PrimaryReplicatorSPtr CreatePrimary(
                REInternalSettingsSPtr const & config,
                REPerformanceCountersSPtr const & perfCounters,
                FABRIC_REPLICA_ID replicaId,
                bool hasPersistedState,
                ReplicationEndpointId const & endpointUniqueId,
                FABRIC_EPOCH const & epoch,
                ComProxyStateProvider const & stateProvider,
                ComProxyStatefulServicePartition const & partition,
                Replicator::ReplicatorInternalVersion version,
                ReplicationTransportSPtr const & transport,
                FABRIC_SEQUENCE_NUMBER initialProgress,
                Common::Guid const & partitionId,
                IReplicatorHealthClientSPtr const & healthClient,
                ApiMonitoringWrapperSPtr const & apiMonitor);

            static PrimaryReplicatorSPtr CreatePrimary(
                REInternalSettingsSPtr const & config,
                REPerformanceCountersSPtr const & perfCounters,
                FABRIC_REPLICA_ID replicaId,
                bool hasPersistedState,
                ReplicationEndpointId const & endpointUniqueId,
                FABRIC_EPOCH const & epoch,
                ComProxyStateProvider const & stateProvider,
                ComProxyStatefulServicePartition const & partition,
                Replicator::ReplicatorInternalVersion version,
                ReplicationTransportSPtr const & transport,
                Common::Guid const & partitionId,
                IReplicatorHealthClientSPtr const & healthClient,
                ApiMonitoringWrapperSPtr const & apiMonitor,
                OperationQueue && secondaryQueue);
                            
            ~PrimaryReplicator();

            void CreateSecondary(
                FABRIC_EPOCH const & epoch,
                REPerformanceCountersSPtr const & perfCounters,
                IReplicatorHealthClientSPtr const & healthClient,                
                ApiMonitoringWrapperSPtr const & apiMonitor,
                __out SecondaryReplicatorSPtr & secondary);

            Common::ErrorCode Open();

            // Called when the role changes to secondary
            // or the replicator is closed
            Common::AsyncOperationSPtr BeginClose(
                bool createSecondary,
                Common::TimeSpan const & waitForQuorumTimeout,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);
            Common::ErrorCode EndClose(Common::AsyncOperationSPtr const &);

            void Abort();

            Common::ErrorCode GetCurrentProgress(
                __out FABRIC_SEQUENCE_NUMBER & lastSequenceNumber) const;
            Common::ErrorCode GetCatchUpCapability(
                __out FABRIC_SEQUENCE_NUMBER & firstSequenceNumber) const;

            Common::ErrorCode UpdateCatchUpReplicaSetConfiguration(
                ReplicaInformationVector const & previousActiveSecondaryReplicas, 
                ULONG previousWriteQuorum,
                ReplicaInformationVector const & currentActiveSecondaryReplicas, 
                ULONG currentWriteQuorum);

            Common::AsyncOperationSPtr BeginWaitForCatchUpQuorum(
                FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);
            Common::ErrorCode EndWaitForCatchUpQuorum(Common::AsyncOperationSPtr const &);

            Common::ErrorCode UpdateCurrentReplicaSetConfiguration(
                ReplicaInformationVector const & currentActiveSecondaryReplicas, 
                ULONG currentWriteQuorum);

            Common::AsyncOperationSPtr BeginBuildReplica(
                ReplicaInformation const & toReplica,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state = Common::AsyncOperationSPtr());
            Common::ErrorCode EndBuildReplica(Common::AsyncOperationSPtr const &);

            Common::AsyncOperationSPtr BeginUpdateEpoch(
                __in FABRIC_EPOCH epoch,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);
            Common::ErrorCode EndUpdateEpoch(
                Common::AsyncOperationSPtr const &);

            Common::ErrorCode RemoveReplica(FABRIC_REPLICA_ID replicaId);

            Common::AsyncOperationSPtr BeginReplicate(
                Common::ComPointer<IFabricOperationData> && comOperationPointer,
                __out FABRIC_SEQUENCE_NUMBER & sequenceNumber,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);
            Common::ErrorCode EndReplicate(
                Common::AsyncOperationSPtr const &);

            Common::ErrorCode GetLastSequenceNumber(
                FABRIC_EPOCH const & epoch,
                FABRIC_SEQUENCE_NUMBER & lastSequenceNumber);

            // Method called after data loss, when the state provider
            // changed state
            Common::ErrorCode ResetReplicationQueue(FABRIC_SEQUENCE_NUMBER newProgress);
            
            virtual void ReplicationAckMessageHandler(
                __in Transport::Message & message, 
                ReplicationFromHeader const & fromHeader,
                PrimaryReplicatorWPtr primaryReplicatorWPtr);

            virtual void CopyContextMessageHandler(
                __in Transport::Message & message, 
                ReplicationFromHeader const & fromHeader);
            
            Common::ErrorCode PrimaryReplicator::GetReplicationQueueCounters(
                __out FABRIC_INTERNAL_REPLICATION_QUEUE_COUNTERS & counters);

            Common::ErrorCode PrimaryReplicator::GetReplicatorStatus(
                __out ServiceModel::ReplicatorStatusQueryResultSPtr & result);

            bool CheckReportedFault(bool callReportFault = false) const;
            
            void SetReportedFault(
                Common::ErrorCode const & error,
                std::wstring const & faultDescription);

            static ServiceModel::ReplicatorStatusQueryResultSPtr GetReplicatorStatusQueryResult(
                std::vector<ReplicationSessionSPtr> const & idleReplicas,
                std::vector<ReplicationSessionSPtr> const & readyReplicas,
                OperationQueue const & queue);
        protected:
            PrimaryReplicator(
                REInternalSettingsSPtr const & config,
                REPerformanceCountersSPtr const & perfCounters,
                FABRIC_REPLICA_ID replicaId,
                bool hasPersistedState,
                ReplicationEndpointId const & endpointUniqueId,
                FABRIC_EPOCH const & epoch,
                ComProxyStateProvider const & stateProvider,
                ComProxyStatefulServicePartition const & partition,
                Replicator::ReplicatorInternalVersion version,
                ReplicationTransportSPtr const & transport,
                FABRIC_SEQUENCE_NUMBER initialProgress,
                Common::Guid const & partitionId,
                IReplicatorHealthClientSPtr const & healthClient,
                ApiMonitoringWrapperSPtr const & apiMonitor);

            PrimaryReplicator(
                REInternalSettingsSPtr const & config,
                REPerformanceCountersSPtr const & perfCounters,
                FABRIC_REPLICA_ID replicaId,
                bool hasPersistedState,
                ReplicationEndpointId const & endpointUniqueId,
                FABRIC_EPOCH const & epoch,
                ComProxyStateProvider const & stateProvider,
                ComProxyStatefulServicePartition const & partition,
                Replicator::ReplicatorInternalVersion version,
                ReplicationTransportSPtr const & transport,
                Common::Guid const & partitionId,
                IReplicatorHealthClientSPtr const & healthClient,
                ApiMonitoringWrapperSPtr const & apiMonitor,
                OperationQueue && secondaryQueue);
            
        private:
            class BuildIdleAsyncOperation;
            class CatchupAsyncOperation;
            class CloseAsyncOperation;
            class ReplicateAsyncOperation;
            class ReplicateBatchAsyncOperation;
            class UpdateEpochAsyncOperation;

            static void UpdateProgressAndCatchupOperation(PrimaryReplicatorWPtr primaryWPtr);
            void UpdateCatchupOperation();
            
            REInternalSettingsSPtr const config_;
            REPerformanceCountersSPtr perfCounters_;
            FABRIC_REPLICA_ID const replicaId_;
            bool const hasPersistedState_;
            ReplicationEndpointId const endpointUniqueId_;
            ComProxyStateProvider stateProvider_;
            ComProxyStatefulServicePartition partition_;
            ReplicationTransportSPtr transport_;
            Common::Guid const partitionId_; 
            Common::ErrorCode faultErrorCode_;
            std::wstring faultDescription_;
            MUTABLE_RWLOCK(REPrimaryReplicatorFaultError, faultErrorLock_);

            // Replica manager keeps track of the active and idle replicas
            // and the replication queue
            std::shared_ptr<ReplicaManager> replicaManagerSPtr_;

            // Catchup async operation; at most one can exist at one time
            Common::AsyncOperationSPtr catchupAsyncOp_;
            MUTABLE_RWLOCK(REPrimaryReplicatorCatchup, catchupLock_);
            
            Replicator::ReplicatorInternalVersion const version_;

            IReplicatorHealthClientSPtr healthClient_;
            ApiMonitoringWrapperSPtr apiMonitor_;
        }; // end class PrimaryReplicator
        
    } // end namespace ReplicationComponent
} // end namespace Reliability

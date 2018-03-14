// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // Manages active and idle replicas 
        // in the current configuration. 
        // Also keeps track of the replication queue, 
        // updating it based on the numbers in the sessions.
        // Used by the Primary Replicator.
        class ReplicaManager
            : public Common::ComponentRoot
        {
            DENY_COPY(ReplicaManager);

        public:
            ReplicaManager(
                REInternalSettingsSPtr const & config,
                bool hasPersistedState,
                REPerformanceCountersSPtr const & perfCounters,
                FABRIC_REPLICA_ID replicaId,
                bool requireServiceAck,
                ReplicationEndpointId const & endpointUniqueId,
                ReplicationTransportSPtr const & transport, 
                FABRIC_SEQUENCE_NUMBER initialProgress,
                FABRIC_EPOCH const & epoch,
                ComProxyStateProvider const & stateProvider,
                ComProxyStatefulServicePartition const & partition,
                IReplicatorHealthClientSPtr const & healthClient,
                ApiMonitoringWrapperSPtr const & apiMonitor,
                Common::Guid const & partitionId);

            ReplicaManager(
                REInternalSettingsSPtr const & config,
                bool hasPersistedState,
                REPerformanceCountersSPtr const & perfCounters,
                FABRIC_REPLICA_ID replicaId,
                ReplicationEndpointId const & endpointUniqueId,
                ReplicationTransportSPtr const & transport, 
                FABRIC_EPOCH const & epoch,
                ComProxyStateProvider const & stateProvider,
                ComProxyStatefulServicePartition const & partition,
                Common::Guid const & partitionId,
                IReplicatorHealthClientSPtr const & healthClient,
                ApiMonitoringWrapperSPtr const & apiMonitor,
                OperationQueue && queue);

            ~ReplicaManager();

            __declspec(property(get=get_Queue)) OperationQueue && Queue;
            OperationQueue && get_Queue();

            __declspec(property(get=get_EndpointUniqueId)) ReplicationEndpointId EndpointUniqueId;
            ReplicationEndpointId get_EndpointUniqueId();

            Common::ErrorCode UpdateEpochAndGetLastSequenceNumber(
                FABRIC_EPOCH const & epoch,
                FABRIC_SEQUENCE_NUMBER & lastSequenceNumber);

            // *****************************
            // Replication queue related methods
            // *****************************
            Common::ErrorCode ResetReplicationQueue(FABRIC_SEQUENCE_NUMBER newProgress);

            Common::ErrorCode GetCurrentProgress(
                __out FABRIC_SEQUENCE_NUMBER & lastSequenceNumber) const;

            Common::ErrorCode GetCatchUpCapability(
                __out FABRIC_SEQUENCE_NUMBER & firstSequenceNumber) const;

            Common::ErrorCode GetLastSequenceNumber(
                __out FABRIC_SEQUENCE_NUMBER & sequenceNumber) const;

            bool GetQueueProgressForCatchup(
                __out bool & hasEnoughReplicasForQuorum,
                __out FABRIC_SEQUENCE_NUMBER & quorumAckLSN,
                __out FABRIC_SEQUENCE_NUMBER & allAckLSN,
                __out FABRIC_SEQUENCE_NUMBER & latestLSN,
                __out FABRIC_SEQUENCE_NUMBER & previousConfigCatchupLsn,
                __out FABRIC_SEQUENCE_NUMBER & lowestLSNAmongstMustCatchupReplicas) const;

            Common::ErrorCode GetReplicationQueueCounters(
                __out FABRIC_INTERNAL_REPLICATION_QUEUE_COUNTERS & counters);

            Common::ErrorCode GetReplicatorStatus(
                __out ServiceModel::ReplicatorStatusQueryResultSPtr & result);

            // ******************************************
            // Method that is used to wait for the 
            // replication operations to complete after close
            // ********************************************
            Common::AsyncOperationSPtr BeginWaitForReplicateOperationsCallbacksToComplete(
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & state);
            Common::ErrorCode EndWaitForReplicateOperationsCallbacksToComplete(
                Common::AsyncOperationSPtr const & asyncOperation);

            // *******************************************************
            // Method that is used to wait for the 
            // pending replication operations to complete during close
            // within the timeout specified
            // *******************************************************
            Common::AsyncOperationSPtr BeginWaitForReplicateOperationsToComplete(
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & state);
            Common::ErrorCode EndWaitForReplicateOperationsToComplete(
                Common::AsyncOperationSPtr const & asyncOperation);

            // *****************************
            // Methods that need both 
            // replication queue and sessions
            // *****************************
            bool UpdateProgress();
                        
            // *****************************
            // Replication sessions related methods
            // *****************************
            // Add an idle replica
            Common::ErrorCode TryAddIdleReplica(
                ReplicaInformation const & replica,
                __out ReplicationSessionSPtr & session,
                __out FABRIC_SEQUENCE_NUMBER & replicationStartSeq);
                        
            // Removes idle replica.
            // Returns true if the replica was found and removed.
            bool RemoveReplica(FABRIC_REPLICA_ID replicaId);
            
            // Get the replica with specified address,
            // returning its role if found.
            // Returns null if replica not found or 
            // if the replica manager is closing/closed
            ReplicationSessionSPtr GetReplica(
                std::wstring const & address, 
                ReplicationEndpointId const & endpointUniqueId,
                __out FABRIC_REPLICA_ROLE & role);

            // Update the replicas with the new configuration.
            // When reconfiguration is completed,
            // quorum should not take into account the previous active replicas;
            // in that case, the user passes previous quorum count 0 
            // and the previous configuration is disabled.
            Common::ErrorCode UpdateConfiguration(
                ReplicaInformationVector const & previousReplicas, 
                ULONG previousWriteQuorum,
                ReplicaInformationVector const & currentReplicas, 
                ULONG currentWriteQuorum,
                bool updatingCatchUpReplicaSetConfiguration,
                bool testOnly);

            // Enqueue the replicate operation and
            // give it to all active and idle replicas
            Common::ErrorCode AddReplicateOperation(
                Common::AsyncOperationSPtr const & thisSPtr,
                Common::ComPointer<IFabricOperationData> && comOperationPointer,
                __out bool & operationCommitted,
                __out ComOperationCPtr & opComPtr);
        
            std::wstring ToString() const;

            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
            void FillEventData(Common::TraceEventContext & context) const;
            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

            // Used by CITs
            bool Test_TryGetProgress(
                __out std::wstring & currentActiveReplicas,
                __out std::wstring & previousActiveReplicas,
                __out std::wstring & idleReplicas,
                __out FABRIC_SEQUENCE_NUMBER & committedLSN,
                __out FABRIC_SEQUENCE_NUMBER & completedLSN,
                __out bool & replicationAckProcessingInProgress);
            
            Common::ErrorCode Open();
            Common::ErrorCode Close(bool creatingSecondary);
        private:
            class CompleteReplicateJobQueue;
            class BeginReplicateJobQueue;
            class Helpers;

            typedef std::unique_ptr<CompleteReplicateJobQueue> CompleteReplicateJobQueueUPtr;
            typedef std::vector<ReplicationSessionSPtr> ReplicationSessionVector;
            typedef std::unique_ptr<BeginReplicateJobQueue> BeginReplicateJobQueueUPtr;
            typedef std::unordered_map<ReplicationSessionSPtr, ComOperationRawPtrVector> ReplicationSessionPendingOperationsMap;
            typedef std::pair<ReplicationSessionSPtr, ComOperationRawPtrVector> ReplicationSessionPendingOperationsEntry;

            // Validates the UpdateConfig call against the current replica set
            bool ValidateReplicas(ReplicaInformationVector const & currentReplicas);

            Common::ErrorCode AddReplicateOperationPrivate(
                Common::AsyncOperationSPtr const & thisSPtr,
                Common::ComPointer<IFabricOperationData> && comOperationPointer,
                __out uint & replicationQueueUsagePercent,
                __out bool & operationCommitted,
                __out ComOperationCPtr & opComPtr);

            // Update active and idle replicas.
            // Returns true if the configuration is different than the current one
            // and the changes have been applied,
            // false otherwise.
            // When previous config is specified, there are 2 cases:
            // - a secondary was promoted to primary and has no previous config; 
            //   in this case, create all previous config based on the passed vector
            // - a current active replicas configuration exists, in which case 
            //   the previous replicas passed must be a subset of the current active ones.
            // If a previous config is still set (a previous reconfig didn't finish),
            // the new previous config must be a subset of the old one.
            // The out parameters:
            // secondariesWithPendingOps - list of added replicas that need to be caught up with pending operations 
            bool TryUpdateReplicasCallerHoldsLock(
                ULONG previousWriteQuorum,
                ReplicaInformationVector const & previousReplicas, 
                ReplicaInformationVector const & currentReplicas, 
                __out ReplicationSessionPendingOperationsMap & secondariesWithPendingOps);
           
            bool ProcessActiveReplicaCallerHoldsLock(
                ReplicaInformationVector const & previousReplicas, 
                ReplicaInformationVector const & currentReplicas, 
                ReplicationSessionSPtr const & session,
                bool previousReplicasDone,
                __inout std::vector<bool> & processedReplicas,
                __inout std::vector<bool> & processedPreviousReplicas,
                __inout ReplicationSessionVector & replicasToClose);

            bool ProcessIdleReplicaCallerHoldsLock(
                ReplicaInformationVector const & currentReplicas,
                ReplicaInformationVector const & previousReplicas,
                ReplicationSessionSPtr const & session,
                __inout std::vector<bool> & processedReplicas,
                __inout std::vector<bool> & processedPreviousReplicas);

            // If there are enough replicas for quorum,
            // gets the replicas progress,
            // updates the replication queue
            // and returns true; otherwise, returns false
            bool TryGetAndUpdateProgressCallerHoldsLock(
                __out std::vector<Common::AsyncOperationSPtr> & operationsToComplete);

            bool HasEnoughReplicas(
                ReplicationSessionVector const & replicas, 
                ULONG writeQuorum);

            // Get the committed and completed LSNs based
            // on all replicas last Acked LSN
            void GetProgressCallerHoldsLock(
                __out FABRIC_SEQUENCE_NUMBER & committedLSN,
                __out FABRIC_SEQUENCE_NUMBER & completedLSN);

            // and update the replication queue
            void UpdateProgressInQueueCallerHoldsLock(
                FABRIC_SEQUENCE_NUMBER committedLSN,
                FABRIC_SEQUENCE_NUMBER completedLSN,
                __out FABRIC_SEQUENCE_NUMBER & oldCommittedLSN,
                __out FABRIC_SEQUENCE_NUMBER & newCommittedLSN,
                __out bool & removedItemsFromQueue);

            ReplicationSessionSPtr CreateReplica(ReplicaInformation const & replica);
            bool GetPendingOperationsCallerHoldsLock(
                ReplicationSessionSPtr const & session,
                __out ReplicationSessionPendingOperationsMap & secondariesWithPendingOps);
            bool FindReplicaCallerHoldsLock(
                ReplicaInformation const & replica,
                ReplicationSessionSPtr & session);
            void GetReplicasImpactingQueueUsageCallerHoldsLock(
                __out ReplicationSessionVector & session) const;

            void CompleteReplicateOperationsCallerHoldsLock(
                std::vector<Common::AsyncOperationSPtr> & operationsToComplete);

            void CloseInnerObjects(bool isCreatingSecondary);
            bool UpdateProgressPrivateCallerHoldsLock(bool forceUpdate);

            void FaultSlowIdleReplicasIfPossible(uint replicationQueueusagepercent);
            void FaultSlowActiveSecondaryReplicasIfPossible(uint replicationQueueusagepercent);
            
            bool isActive_;
            REInternalSettingsSPtr const & config_;
            REPerformanceCountersSPtr const & perfCounters_;
            FABRIC_REPLICA_ID const replicaId_;
            ReplicationEndpointId const endpointUniqueId_;
            ReplicationTransportSPtr const & transport_;
            FABRIC_EPOCH epoch_;

            // ReplicaManager's lifetime is the same as the Primary's.
            // Since primary keeps the stateProvider alive, it's ok
            // to keep it by reference here.
            ComProxyStateProvider const & stateProvider_;
            ComProxyStatefulServicePartition const & partition_;
            Common::Guid const & partitionId_;

            // The desired number or replicas that form the quorum.
            // Includes the primary.
            // If not enough replicas are present, catch up and replicate 
            // operations are stuck.
            ULONG writeQuorum_;
            bool hasEnoughReplicasForQuorum_;

            ULONG previousWriteQuorum_;
            bool hasEnoughReplicasForQuorumInPreviousConfig_;
            
            // List of replicas that are part of quorum
            // Active replicas in current configuration
            ReplicationSessionVector activeReplicas_;
            // Active replicas in previous configuration;
            // they are only needed while a reconfiguration is in process.
            // While a reconfiguration is in progress, 
            // catch up and replicate operations
            // must take into account both the current and the previous configurations.
            bool usePreviousActiveReplicas_;
            ReplicationSessionVector previousActiveReplicas_;

            bool updatingCatchUpReplicaSetConfiguration_;

            // List of idle replicas, that receive the replication operations
            // but are not part of quorum
            ReplicationSessionVector idleReplicas_;
            
            // Apply ACK LSN's of the replicas which is used to determine if catchup should complete
            FABRIC_SEQUENCE_NUMBER majorityReplicasApplyAckedLSNinCC_;
            FABRIC_SEQUENCE_NUMBER allReplicasApplyAckedLSNinCC_;

            // Replication queue manager encapsulates the replication queue
            ReplicationQueueManager replicationQueueManager_;

            // List with pending replicate operations
            std::unordered_map<FABRIC_SEQUENCE_NUMBER, Common::AsyncOperationSPtr> replicateOperations_;

            CompleteReplicateJobQueueUPtr completeReplicateJobQueueUPtr_;
            BeginReplicateJobQueueUPtr beginReplicateJobQueueUPtr_;

            std::shared_ptr<Common::AsyncManualResetEvent> waitForPendingOperationsToCompleteEventSPtr_;
            
            MUTABLE_RWLOCK(REReplicaManager, lock_);
            
            Common::TimerSPtr periodicTimer_;
            Common::Stopwatch lastTraceWatch_;
            Common::Stopwatch lastQueueFullTraceWatch_;
            Common::Stopwatch idleReplicaBuildCompleteLsnUpdatedWatch_;

            BatchedHealthReporterSPtr healthReporter_;
            ApiMonitoringWrapperSPtr apiMonitor_;

          bool const hasPersistedState_;
        }; // end class ReplicaManager

    } // end namespace ReplicationComponent
} // end namespace Reliability

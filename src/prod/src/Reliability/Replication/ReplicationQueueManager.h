// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // Manages the Replication queue on the Primary
        class ReplicationQueueManager
        {
            DENY_COPY(ReplicationQueueManager);

        public:
            ReplicationQueueManager(
                REInternalSettingsSPtr const & config,
                REPerformanceCountersSPtr const & perfCounters,
                bool hasPersistedState,
                ReplicationEndpointId const & endpointUniqueId,
                FABRIC_SEQUENCE_NUMBER initialProgress,
                Common::Guid const & partitionId);

            ReplicationQueueManager(
                REInternalSettingsSPtr const & config,
                REPerformanceCountersSPtr const & perfCounters,
                ReplicationEndpointId const & endpointUniqueId,
                Common::Guid const & partitionId,
                OperationQueue && queue);

            ~ReplicationQueueManager();

            __declspec(property(get=get_Queue)) OperationQueue && Queue;
            OperationQueue && get_Queue();

            __declspec (property(get=get_Mask)) ULONGLONG const & Mask;
            ULONGLONG const & get_Mask() const { return replicationQueue_.Mask; }
            
            __declspec (property(get=get_ReplicationOperationCount)) ULONGLONG OperationCount;
            ULONGLONG get_ReplicationOperationCount() const { return replicationQueue_.OperationCount; }

            __declspec (property(get=get_FirstLSNInReplicationQueue)) FABRIC_SEQUENCE_NUMBER FirstLSNInReplicationQueue;
            FABRIC_SEQUENCE_NUMBER get_FirstLSNInReplicationQueue() const { return replicationQueue_.NextToBeCompletedSequenceNumber; }

            __declspec (property(get=get_FirstOperationInReplicationQueueEnqueuedSince)) Common::TimeSpan FirstOperationInReplicationQueueEnqueuedSince;
            Common::TimeSpan get_FirstOperationInReplicationQueueEnqueuedSince() const { return replicationQueue_.FirstOperationInReplicationQueueEnqueuedSince; }

            void ResetQueue(FABRIC_SEQUENCE_NUMBER newInitialProgress);

            Common::ErrorCode Enqueue(
                Common::ComPointer<IFabricOperationData> && comOperationPointer,
                FABRIC_EPOCH const & epoch,
                __out ComOperationCPtr & operationRawPtr);
                
            FABRIC_SEQUENCE_NUMBER GetCurrentProgress() const;
            FABRIC_SEQUENCE_NUMBER GetCatchUpCapability() const;
            FABRIC_SEQUENCE_NUMBER GetLastSequenceNumber() const;
            
            Common::ErrorCode GetReplicationQueueCounters(
                __out FABRIC_INTERNAL_REPLICATION_QUEUE_COUNTERS & counters);

            Common::ErrorCode GetReplicatorStatus(
                __in std::vector<ReplicationSessionSPtr> inBuildReplicas,
                __in std::vector<ReplicationSessionSPtr> readyReplicas,
                __out ServiceModel::ReplicatorStatusQueryResultSPtr & result);

            void UpdateQueue(
                FABRIC_SEQUENCE_NUMBER committedLSN,
                FABRIC_SEQUENCE_NUMBER completedLSN,
                __out FABRIC_SEQUENCE_NUMBER & oldCommittedLSN,
                __out FABRIC_SEQUENCE_NUMBER & newCommittedLSN,
                __out bool & removedItemsFromQueue);

            void UpdateCatchupCompletionLSN();
                
            void CheckWithCatchupLsn(
                __inout FABRIC_SEQUENCE_NUMBER & sequenceNumber);

            void GetQueueProgressForCatchup(
                FABRIC_SEQUENCE_NUMBER replicasCommitted,
                FABRIC_SEQUENCE_NUMBER replicasCompleted,
                __out FABRIC_SEQUENCE_NUMBER & committed,
                __out FABRIC_SEQUENCE_NUMBER & completed,
                __out FABRIC_SEQUENCE_NUMBER & last,
                __out FABRIC_SEQUENCE_NUMBER & previousLast) const;

            bool GetOperations(
                FABRIC_SEQUENCE_NUMBER start,
                __out ComOperationRawPtrVector & pendingOperations) const; 

            bool GetPendingOperationsToSendDuringBuild(
                __in bool stateproviderSupportsBuildUntilLatestLsn,
                __out FABRIC_SEQUENCE_NUMBER & fromSequenceNumber,
                __out ComOperationRawPtrVector & pendingOperations) const; 
            
        private:

            static std::wstring GetQueueDescription(ReplicationEndpointId const & id);

            inline void UpdatePerfCounters();
                        
            REInternalSettingsSPtr const & config_;
            REPerformanceCountersSPtr const & perfCounters_;
            // PartitionId and description are passed from ReplicaManager;
            // since they are kept alive by ReplicaManager
            // and the queue manager has the same lifetime as the replica manager,
            // it's ok to keep them by ref.
            Common::Guid const & partitionId_;
            ReplicationEndpointId const & endpointUniqueId_;
            
            OperationQueue replicationQueue_;
            // The sequence number that will be issued
            // to the next store operation for this partition
            FABRIC_SEQUENCE_NUMBER nextSequenceNumber_;
           
            // sequence number in previous configuration up to which the catchup must happen
            // Catchup happens until a quorum of replicas in CC get ALL operations that were generated in PC
            // This is not a requirement (we could just wait until quorum of replicas in CC get the quorum ack'd LSN that was present in PC).
            // However, this approach is followed for guarantee's during swap primary to ensure no replica is progressed further than the primary
            FABRIC_SEQUENCE_NUMBER previousConfigCatchupLsn_;

            // This LSN is the quorum LSN of the PC. We do not really need this as the catchup uses the above LSN.
            // The only place this is used is to determine the number of operations to send during build to the idle as the above catchuplsn_ cannot be used
            // That is because the state provider may not have the items that are part of "previousConfigCatchupLsn_" (as they may not be quorum ack'd in CC as yet), but the 
            // state provider is guaranteed to have everything that is part of "previousConfigQuorumLsn_"
            FABRIC_SEQUENCE_NUMBER previousConfigQuorumLsn_;

        }; // end class ReplicationQueueManager

    } // end namespace ReplicationComponent
} // end namespace Reliability

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // Class that deals with replication operations received by secondary.
        // It dispatches the replication operations in order so they can be taken by the service
        // and keeps track of the progress so ACKs can be send to the primary.
        // The class is not thread safe, and must be protected by the secondary lock.
        class SecondaryReplicationReceiver
        {
            DENY_COPY(SecondaryReplicationReceiver);

        public:
            SecondaryReplicationReceiver(
                REInternalSettingsSPtr const & config,
                REPerformanceCountersSPtr const & perfCounters,
                bool requireServiceAck,
                ReplicationEndpointId const & endpointUniqueId,
                Common::Guid const & partitionId,
                IReplicatorHealthClientSPtr const & healthClient,
                bool enableEOSOpAndStreamFaults);

            SecondaryReplicationReceiver::SecondaryReplicationReceiver(
                REInternalSettingsSPtr const & config,
                REPerformanceCountersSPtr const & perfCounters,
                bool requireServiceAck,
                ReplicationEndpointId const & endpointUniqueId,
                FABRIC_EPOCH const & epoch,
                Common::Guid const & partitionId,
                bool enableEOSOpAndStreamFaults,
                IReplicatorHealthClientSPtr const & healthClient,
                OperationQueue && primaryQueue);

            ~SecondaryReplicationReceiver();

            __declspec(property(get=get_Queue)) OperationQueue && Queue;
            OperationQueue && get_Queue();

            __declspec (property(get=get_DispatchQueue)) DispatchQueueSPtr const & DispatchQueue;
            DispatchQueueSPtr const & get_DispatchQueue() const { return dispatchQueue_; }

            __declspec (property(get=get_AllOperationsAcked)) bool AllOperationsAcked;
            bool get_AllOperationsAcked() 
            { 
                if (enqueuedEndOfStreamOperation_)
                {
                    // If end of stream operation was enqueued, we must check that it has been ACK'd
                    return ackedEndOfStreamOperation_ && !HasCommittedOperations();
                }
                else
                {
                    return !HasCommittedOperations();
                }
            }
            
            static std::wstring GetHealthReportWarningDescription(
                __in OperationQueueUPtr const & queue,
                __in uint const & queueUsage,
                __in uint const & queueUsageThreshold);

            void DiscardOutOfOrderOperations();

            bool IsReplicationQueueCreated() const { return replicationQueue_ != nullptr; }

            bool IsStreamInitialized()
            {
                auto stream = stream_.lock();
                return (stream != nullptr);
            }
                        
            OperationStreamSPtr GetStream(
                SecondaryReplicatorSPtr const & secondary);

            FABRIC_SEQUENCE_NUMBER GetCurrentProgress() const;

            FABRIC_SEQUENCE_NUMBER GetCatchUpCapability() const;

            FABRIC_SEQUENCE_NUMBER GetNextToBeCompleted() const;

            ServiceModel::ReplicatorQueueStatusSPtr GetQueueStatusForQuery() const;
                        
            void CleanQueueOnClose(
                Common::ComponentRoot const & root,
                bool clearPendingQueueItems,
                bool ignoreEOSOperationAck);

            void CleanQueueOnUpdateEpochFailure(Common::ComponentRoot const & root);

            void ProcessStartCopyOperation(
                FABRIC_SEQUENCE_NUMBER replicationStartSequenceNumber,
                FABRIC_EPOCH const & startCopyEpoch,
                __out bool & progressDone);

            bool ProcessReplicationOperation(
                std::vector<ComOperationCPtr> && batchOperation, 
                FABRIC_SEQUENCE_NUMBER completedSequenceNumber,
                __out bool & checkForceSendAck);

            bool OnAckReplicationOperation(
                ComOperation const & operation,
                __out ProgressVectorEntryUPtr & pendingUpdateEpoch);

            void OnAckEndOfStreamOperation(ComOperation const & operation);

            void UpdateStateProviderEpochDone(bool closeInProgress);

            void GetAck(
                __out FABRIC_SEQUENCE_NUMBER & replicationCommittedLSN, 
                __out FABRIC_SEQUENCE_NUMBER & replicationCompletedLSN);

            FABRIC_SEQUENCE_NUMBER GetAckProgress();
        
            void EnqueueEndOfStreamOperationAndKeepRootAlive(OperationAckCallback const & callback, Common::ComponentRoot const & root);

            void MarkEOSOperationACK(bool ackIgnored = false);

            void OnStreamReportFault(Common::ComponentRoot const & root);
        private:
            void CreateReplicationQueue(
                FABRIC_SEQUENCE_NUMBER replicationStartSeq);
            
            void UpdateReplicationQueue();

            bool HasCommittedOperations() const { return replicationQueue_ && replicationQueue_->HasCommittedOperations(); }

            void DispatchReplicationOperation(ComOperationCPtr const & operation);
            void SetReplicationCallback(bool shouldEnable);
                        
            void UpdateEpochIfNeededCallerHoldsLock(
                ComOperationCPtr const & operation,
                __out bool & needFullBarrier);
            bool TryDispatchReplication(
                __out bool & checkForceSendAck);

            void CleanQueue(
                Common::ComponentRoot const & root,
                bool clearPendingQueueItems,
                bool ignoreEOSOperationAck);

            inline void UpdatePerfCountersAndReportHealthIfNeeded();

            REInternalSettingsSPtr const config_;
            REPerformanceCountersSPtr const perfCounters_;
            bool const requireServiceAck_;
            ReplicationEndpointId const endpointUniqueId_;
            Common::Guid const partitionId_;
            // Indicates if end of stream operation and faulted stream configs are enabled (both are enabled/disabled together)
            bool const enableEOSOpAndStreamFaults_;  

            // Replication queue that orders incoming operations,
            // and keeps track of the in-order and out-of-order operations.
            OperationQueueUPtr replicationQueue_;

            // Dispatch queue where replication operations are placed to be 
            // taken by the service.
            DispatchQueueSPtr dispatchQueue_;

            // The stream given to the state provider to pump
            // replication operations.
            OperationStreamWPtr stream_;
            Common::atomic_bool streamCreated_;
            
            // Epoch in which copy was done
            FABRIC_EPOCH startCopyEpoch_;

            // The latest epoch updated on the state provider.
            FABRIC_EPOCH stateProviderUpdatedEpoch_;

            // Keeps track of the progress vector entry that 
            // must be updated on the state provider.
            ProgressVectorEntryUPtr progressVector_;
            
            // Remembers the last replication completed ACK sent to the primary.
            // Used to determine how many pending operations 
            // are not yet ACKed.
            FABRIC_SEQUENCE_NUMBER lastReplicationCompletedAck_;
            
            // Indicates if the end of stream operation has been enqueued
            bool enqueuedEndOfStreamOperation_;

            // If last end of stream operation has been ack'd by service or not
            bool ackedEndOfStreamOperation_;

            bool replicationOrCopyStreamFaulted_;

            BatchedHealthReporterSPtr healthReporter_;
        }; // end class SecondaryReplicationReceiver

    } // end namespace ReplicationComponent
} // end namespace Reliability

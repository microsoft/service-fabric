// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class SecondaryReplicator : public Common::ComponentRoot
        {
            DENY_COPY(SecondaryReplicator);

        public:

            // Create the initial Idle replica
            static SecondaryReplicatorSPtr CreateSecondary(
                REInternalSettingsSPtr const & config,
                REPerformanceCountersSPtr const & perfCounters,
                FABRIC_REPLICA_ID replicaId,
                bool hasPersistedState,
                ReplicationEndpointId const & endpointUniqueId,
                ComProxyStateProvider const & stateProvider,
                ComProxyStatefulServicePartition const & partition,
                Replicator::ReplicatorInternalVersion version,
                ReplicationTransportSPtr const & transport,
                Common::Guid const & partitionId,
                IReplicatorHealthClientSPtr const & healthClient,
                ApiMonitoringWrapperSPtr const & apiMonitor);

            // Create a secondary using an old primary's replication queue
            static SecondaryReplicatorSPtr CreateSecondary(
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
                OperationQueue && primaryQueue);
                            
            __declspec(property(get=get_ApiMonitor)) ApiMonitoringWrapperSPtr ApiMonitor;
            ApiMonitoringWrapperSPtr get_ApiMonitor()
            {
                Common::AcquireReadLock lock(queuesLock_);
                return apiMonitor_;
            }

            ~SecondaryReplicator();

            void CreatePrimary(
                FABRIC_EPOCH const & epoch,
                REPerformanceCountersSPtr const & perfCounters,
                IReplicatorHealthClientSPtr const & healthClient,
                ApiMonitoringWrapperSPtr const & apiMonitor,
                __out PrimaryReplicatorSPtr & primary);

            // Promote Idle to active secondary
            void PromoteToActiveSecondary(
                Common::AsyncOperationSPtr const & promoteIdleAsyncOperation) ;
            
            void Open();

            // Called when the replicator is closed or
            // the secondary is promoted to primary
            Common::AsyncOperationSPtr BeginClose(
                bool isPromoting,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);
            Common::ErrorCode EndClose(Common::AsyncOperationSPtr const &);

            void Abort();

            Common::ErrorCode GetCurrentProgress(
                __out FABRIC_SEQUENCE_NUMBER & lastSequenceNumber);
            Common::ErrorCode GetCatchUpCapability(
                __out FABRIC_SEQUENCE_NUMBER & firstSequenceNumber);
            Common::ErrorCode GetReplicatorStatus(
                __out ServiceModel::ReplicatorStatusQueryResultSPtr & result);

            Common::AsyncOperationSPtr BeginUpdateEpoch(
                FABRIC_EPOCH const & epoch,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & state);
            Common::ErrorCode EndUpdateEpoch(
                Common::AsyncOperationSPtr const & asyncOperation);

            OperationStreamSPtr GetCopyStream(SecondaryReplicatorSPtr const & thisSPtr);

            OperationStreamSPtr GetReplicationStream(SecondaryReplicatorSPtr const & thisSPtr);
            
            bool ProcessCopyOperation(
                ComOperationCPtr && operation, 
                FABRIC_REPLICA_ID replicaId, 
                FABRIC_EPOCH const & epoch,
                bool isLast,
                __out bool & progressDone);
            
            bool ProcessReplicationOperation(
                ReplicationFromHeader const & fromHeader,
                std::vector<ComOperationCPtr> && batchOperation, 
                FABRIC_EPOCH const & epoch, 
                FABRIC_SEQUENCE_NUMBER completedSequenceNumber);

            bool ProcessStartCopy(
                ReplicationFromHeader const & fromHeader,
                FABRIC_EPOCH const & epoch,
                FABRIC_REPLICA_ID replicaId, 
                FABRIC_SEQUENCE_NUMBER replicationStartSequenceNumber,
                __out bool & shouldDispatch,
                __out bool & startCopyContext);

            void ProcessInduceFault(std::wstring const & reason);
                              
            // Message handlers
            void ReplicationOperationMessageHandler(
                __in Transport::Message & message, 
                ReplicationFromHeader const & fromHeader);
            void CopyOperationMessageHandler(
                __in Transport::Message & message, 
                ReplicationFromHeader const & fromHeader);
            void StartCopyMessageHandler(
                __in Transport::Message & message, 
                ReplicationFromHeader const & fromHeader);
            void CopyContextAckMessageHandler(
                __in Transport::Message & message, 
                ReplicationFromHeader const & fromHeader);
            void RequestAckMessageHandler(
                __in Transport::Message & message, 
                ReplicationFromHeader const & fromHeader);
            void InduceFaultMessageHandler(
                __in Transport::Message & message, 
                ReplicationFromHeader const & fromHeader);

            void SetReportedFault(
                Common::ErrorCode const & error,
                std::wstring const & faultDescription,
                FABRIC_FAULT_TYPE faultType = FABRIC_FAULT_TYPE_TRANSIENT);

            bool CheckReportedFault(bool callReportFault = false) const;
            
            void EnqueueEndOfStreamOperations();

            // Method used by operation stream to indicate a fault on the strema
            void OnStreamFault(FABRIC_FAULT_TYPE faultType, std::wstring const & errorMessage);

            Common::AsyncOperationSPtr BeginUpdateStateProviderEpochFromOperationStream(
                FABRIC_EPOCH const & epoch,
                FABRIC_SEQUENCE_NUMBER lsn,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & state);

            Common::ErrorCode EndUpdateStateProviderEpochFromOperationStream(
                Common::AsyncOperationSPtr const & asyncOperation);

        protected:
            // Create the initial Idle replica
            SecondaryReplicator(
                REInternalSettingsSPtr const & config,
                REPerformanceCountersSPtr const & perfCounters,
                FABRIC_REPLICA_ID replicaId,
                bool hasPersistedState,
                ReplicationEndpointId const & endpointUniqueId,
                ComProxyStateProvider const & stateProvider,
                ComProxyStatefulServicePartition const & partition,
                Replicator::ReplicatorInternalVersion version,
                ReplicationTransportSPtr const & transport,
                Common::Guid const & partitionId,
                IReplicatorHealthClientSPtr const & healthClient,
                ApiMonitoringWrapperSPtr const & apiMonitor);

            // Create a secondary using an old primary's replication queue
            SecondaryReplicator(
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
                OperationQueue && primaryQueue); 

            __declspec (property(get=get_CopyAckCallback)) OperationAckCallback const & CopyAckCallback;
            OperationAckCallback const & get_CopyAckCallback() const { return copyAckCallback_; }

            __declspec (property(get=get_ReplicationAckCallback)) OperationAckCallback const & ReplicationAckCallback;
            OperationAckCallback const & get_ReplicationAckCallback() const { return replicationAckCallback_; }

            void Test_ForceDispatch();

            // Method used internally and by the CITs
            void GetAck(
                __out FABRIC_SEQUENCE_NUMBER & replicationCommittedLSN, 
                __out FABRIC_SEQUENCE_NUMBER & replicationCompletedLSN, 
                __out FABRIC_SEQUENCE_NUMBER & copyCommittedLSN,
                __out FABRIC_SEQUENCE_NUMBER & copyCompletedLSN,
                __out std::wstring & primaryTarget,
                __out KBuffer::SPtr & headersStream,
                __out Transport::ISendTarget::SPtr & target,
                __out int & errorCodeValue,
                __out std::vector<SecondaryReplicatorTraceInfo> & pendingReplOpMessages,
                __out std::vector<SecondaryReplicatorTraceInfo> & pendingCopyOpMessages);

        private:
            class DrainQueueAsyncOperation;
            class UpdateEpochAsyncOperation;
            class CloseAsyncOperation;
            class CloseAsyncOperation2;
            class GetOperationAsyncOperation;
            class OperationDispatchJobQueue;
            class StateProviderUpdateEpochAsyncOperation;
            typedef std::unique_ptr<OperationDispatchJobQueue> OperationDispatchJobQueueUPtr;

            __declspec (property(get=get_IsStartCopyEpochEstablishedCallerHoldsLock)) bool IsStartCopyEpochEstablishedCallerHoldsLock;
            bool get_IsStartCopyEpochEstablishedCallerHoldsLock() const { return startCopyEpoch_ != Constants::InvalidEpoch; }
            
            void OnAckReplicationOperation(ComOperation & operation);

            void OnAckCopyOperation(ComOperation & operation);

            void OnAckEndOfStreamCopyOperation(ComOperation & operation);

            void OnAckEndOfStreamReplicationOperation(ComOperation & operation);
            
            void ScheduleDispatchReplication();

            void ScheduleDispatchCopy();
             
            Common::AsyncOperationSPtr BeginUpdateStateProviderEpochPrivate(
                bool updateQueueStates,
                FABRIC_EPOCH const & epoch,
                FABRIC_SEQUENCE_NUMBER lsn,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & state);

            void ScheduleUpdateStateProviderEpochOnReplicationAck(std::shared_ptr<ProgressVectorEntry> const & pendingUpdateEpoch);
            void FinishUpdateStateProviderEpochOnReplicationAck(Common::AsyncOperationSPtr const & asyncOperation, bool completedSync);

            // Finish processing start copy message, which may
            // call user code or execute lengthy operations.
            // Should not be executed on the dispatch thread.
            void FinishProcessStartCopy(
                bool shouldDispatch,
                bool startCopyContext);

            void BeginCopyContext(CopySenderSPtr copySender);

            bool HasPrimaryInfoChangesCallerHoldsLock(
                ReplicationEndpointId const & primaryDemuxer,
                std::wstring const & primaryAddress);
            void SetPrimaryInfoCallerHoldsLock(
                ReplicationEndpointId const & primaryDemuxer,
                std::wstring const & priamryAddress);

            bool ShouldForceSendAck();

            bool SendAck(bool shouldTrace);

            bool SendCopyContextOperation(
                ComOperationCPtr const & operationPtr,
                bool isLast);

            void CopyContextCallback(Common::AsyncOperationSPtr const & asyncOperation);

            void ProcessCopyContextFailure(Common::ErrorCode const & error);
            
            void FinishWaitForCopyQueueToDrain(
                Common::AsyncOperationSPtr const & asyncOperation,
                bool completedSynchronously);

            bool SendTransportMessage(
                KBuffer::SPtr && sharedHeaders,
                Transport::ISendTarget::SPtr const & receiverTarget, 
                __in Transport::MessageUPtr && message, 
                bool setExpiration);

            void CloseDispatchQueues(bool abortCopy, bool abortRepl);

            REInternalSettingsSPtr const config_;

            // This needs to be explicitly reset in close/abort.
            // Since a secondary replicator object can live for arbitrarily long duration after close,
            // failing to release this explicitly could lead to a new object trying to create a perf counter 
            // instance with the same partition and replica id. Attempting to do so would ASSERT in the perf infrastructure 
            REPerformanceCountersSPtr perfCounters_;

            FABRIC_REPLICA_ID const replicaId_;
            bool const hasPersistedState_;
            bool const requireServiceAck_;
            ReplicationEndpointId const endpointUniqueId_;
            
            // Indicates if end of stream operation and faulted stream configs are enabled (both are enabled/disabled together)
            bool const enableEOSOpAndStreamFaults_;  

            // Both these epochs are initialized to a base value when we receive the first start copy operation.
            // The difference between the two is that minAllowedEpoch_ is modified when we receive REPL operations from a new primary, while the start copy epoch never changes after initialization
            // Without these 2 states, we cannot detect the following scenario:

            //  1. P1 (e1) builds I1
            //  2. I1 establishes min epoch = e1 when P1's start copy reaches I1.
            //  3. P1 goes down and P2 is new primary (e2)
            //  4. Before P2's start copy reaches I1, P2's REPL message reaches I1
            //  5. I1 updates min epoch = e2
            //  6. P2's start copy reaches I1, but I1 ignores the message since it assumes it is a stale one since min epoch = e2.
            // This will cause build to get stuck on P2
            FABRIC_EPOCH minAllowedEpoch_;
            FABRIC_EPOCH startCopyEpoch_;

            ComProxyStateProvider stateProvider_;
            ComProxyStatefulServicePartition partition_;
            ReplicationTransportSPtr transport_;
            Common::Guid const partitionId_;
            
            // Replication receiver that processes replication operations received from
            // primary and dispatches them to the service
            SecondaryReplicationReceiver replicationReceiver_;

            // Copy receiver that processes copy operations received from
            // primary and dispatches them to the service
            SecondaryCopyReceiver copyReceiver_;

            // Copy sender used to take copy context operations from state provider
            // and send them to the primary.
            // Protected by the queuesLock.
            CopySenderSPtr copySender_;
            int copyErrorCodeValue_;

            Common::ErrorCode faultErrorCode_;
            std::wstring faultDescription_;
            FABRIC_FAULT_TYPE faultType_;
            MUTABLE_RWLOCK(RESecondaryReplicatorFaultError, faultErrorLock_);
            
            OperationAckCallback copyAckCallback_;
            OperationAckCallback replicationAckCallback_;

            OperationAckCallback endOfStreamCopyAckCallback_;
            OperationAckCallback endOfStreamReplicationAckCallback_;

            Common::AsyncOperationSPtr closeAsyncOperation_;
            Common::AsyncOperationSPtr drainCopyOperationAsyncOperation_;
            Common::AsyncOperationSPtr drainReplOperationAsyncOperation_;
            
            // Cache the primary address and target; whenever the address changes, 
            // the target must be re-resolved
            std::wstring primaryAddress_;
            ReplicationEndpointId primaryDemuxerActor_;
            Transport::ISendTarget::SPtr primaryTarget_;
            
            AckSender ackSender_;
            
            // Keeps track of whether the secondary is still active,
            // preventing processing messages after close has been called.
            bool isActive_;
           
            // Lock that protects the copy and replication queues,
            // as well as the state (isActive_ flag).
            MUTABLE_RWLOCK(RESecondaryReplicatorQueuesLock, queuesLock_);

            Replicator::ReplicatorInternalVersion const version_;

            // These 2 Events are used by close and abort method to wait on a pending state provider update epoch or 
            // getcopycontext call
            std::shared_ptr<Common::AsyncManualResetEvent> finishedStateProviderUpdateEpoch_;
            bool doNotInvokeUEOnStateProvider_;

            std::shared_ptr<Common::AsyncManualResetEvent> finishedStateProviderGetCopyContext_;

            Common::DateTime lastReplicationOperationReceivedAt_;
            Common::DateTime lastCopyOperationReceivedAt_;

            Common::atomic_bool isPromotedToActiveSecondary_;

            IReplicatorHealthClientSPtr healthClient_;
            ApiMonitoringWrapperSPtr apiMonitor_;
            OperationDispatchJobQueueUPtr dispatchJobQueue_;
            
            // Message Sending Headers that are serialized every time the primary address changes 
            // These cannot be declared const because the primary address can change in the lifetime of a secondary replica

            // Also these are unique_ptrs because it was non trivial to re use the old ByteBique as they are ref counted.
            KBuffer::SPtr replicationAckHeadersSPtr_;
            KBuffer::SPtr copyContextHeadersSPtr_;

            // Class used to batch traces for receiving and processing acks
            SecondaryReplicatorTraceHandler replicationTraceHandler_;
            SecondaryReplicatorTraceHandler copyTraceHandler_;

        }; // end class SecondaryReplicator

    } // end namespace ReplicationComponent
} // end namespace Reliability

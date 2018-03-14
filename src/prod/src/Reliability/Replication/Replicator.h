// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class Replicator : public Common::ComponentRoot, public Common::TextTraceComponent<Common::TraceTaskCodes::Replication>
        {
            DENY_COPY(Replicator);

        public:
            enum ReplicatorInternalVersion
            {
                V1 = 1,
                V1Plus = 2,
            };

            Replicator(
                REInternalSettingsSPtr && config,
                FABRIC_REPLICA_ID replicaId,
                Common::Guid const & partitionId,
                bool hasPersistedState,
                ComProxyStatefulServicePartition && partition,
                ComProxyStateProvider && stateProvider,
                ReplicatorInternalVersion version,
                ReplicationTransportSPtr && transport,
                IReplicatorHealthClientSPtr && healthClient);
                            
            ~Replicator();

            // Get the replication endpoint used by the P2P transport
            // to exchange messages between replication components.
            __declspec(property(get=get_ReplicationEndpoint)) std::wstring const & ReplicationEndpoint;
            std::wstring const & get_ReplicationEndpoint() const { return endpoint_; }

            __declspec(property(get=get_ReplicaId)) FABRIC_REPLICA_ID ReplicaId;
            FABRIC_REPLICA_ID get_ReplicaId() const { return replicaId_; }

            __declspec(property(get=get_IncarnationId)) Common::Guid const & IncarnationId;
            Common::Guid const & get_IncarnationId() const { return endpointUniqueId_.IncarnationId; }

            __declspec(property(get=get_PartitionId)) Common::Guid const & PartitionId;
            Common::Guid const & get_PartitionId() const { return partitionId_; }

            ReplicationEndpointId const & get_ReplicationEndpointId() const { return this->endpointUniqueId_; }
            
            __declspec(property(get=get_Config)) REInternalSettingsSPtr const & Config;
            REInternalSettingsSPtr const & get_Config() const { return config_; }

            __declspec(property(get = get_Test_HealthClient)) IReplicatorHealthClientSPtr const & Test_HealthClient;
            IReplicatorHealthClientSPtr const & get_Test_HealthClient() const { return healthClient_; }

            __declspec(property(get=get_ReplicatorTransport)) ReplicationTransportSPtr const & ReplicatorTransport;
            ReplicationTransportSPtr const & get_ReplicatorTransport() const { return transport_; }

            __declspec(property(get=get_Version)) ReplicatorInternalVersion const Version;
            ReplicatorInternalVersion const get_Version() const { return version_; }
            
            // *****************************
            // IFabricReplicator methods
            // *****************************
            // Opens the replication engine; the inner objects 
            // (primary and secondary) will not be created at this time.
            Common::AsyncOperationSPtr BeginOpen(
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);
            Common::ErrorCode EndOpen(Common::AsyncOperationSPtr const &, __out std::wstring & endpoint);

            // Closes the replication engine, which closes the inner
            // objects (primary or secondary).
            // The replication engine will not be able to be used after this.
            Common::AsyncOperationSPtr BeginClose(
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);
            Common::ErrorCode EndClose(Common::AsyncOperationSPtr const &);

            void Abort();

            // Changes the role of the current node.
            // Possible changes: 
            // Initial transitions to Idle and Primary;
            // Role changes from Secondary <-> Primary, Idle -> Primary.
            // When a secondary is promoted to primary,
            // UpdateCatchUpConfiguration must be called to update the active and idle replicas.
            Common::AsyncOperationSPtr BeginChangeRole(
                FABRIC_EPOCH const & epoch,
                FABRIC_REPLICA_ROLE newRole,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);
            Common::ErrorCode EndChangeRole(Common::AsyncOperationSPtr const &);
            
            // Update the configuration version to the specified value
            // Once the new configuration version is set, 
            // the replication layer will not accept any replication operation initiated by a 
            // lower configuration version. 
            // This protects agains stale operations.
            Common::AsyncOperationSPtr BeginUpdateEpoch(
                FABRIC_EPOCH const & epoch,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & state);
            Common::ErrorCode EndUpdateEpoch(
                Common::AsyncOperationSPtr const & asyncOperation);

            // Gets the sequence numbers available for catchup and 
            // the most advanced one. 
            Common::ErrorCode GetCurrentProgress(
                __out FABRIC_SEQUENCE_NUMBER & lastSequenceNumber);
            Common::ErrorCode GetCatchUpCapability(
                __out FABRIC_SEQUENCE_NUMBER & firstSequenceNumber);

            // Update replicator settings
            Common::ErrorCode UpdateSecuritySettings(
                __in Transport::SecuritySettings const & securitySettings);

            // *****************************
            // IFabricPrimaryReplicator methods
            // *****************************
            // Applies the new configuration that includes all
            // replicas except the current one.
            Common::ErrorCode UpdateCatchUpReplicaSetConfiguration(
                ReplicaInformationVector const & previousActiveSecondaryReplicas, 
                ULONG previousWriteQuorum,
                ReplicaInformationVector const & currentActiveSecondaryReplicas, 
                ULONG currentWriteQuorum);

            // Wait for catchup to be done - either full or quorum.
            // Quorum: After a reconfiguration, 
            // the quorum acked number may have changed;
            // this function waits until the previous quorum number is reached.
            // Full: Waits until all replicas are caught up.
            // Called on swap primary.
            Common::AsyncOperationSPtr BeginWaitForCatchUpQuorum(
                FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);
            Common::ErrorCode EndWaitForCatchUpQuorum(Common::AsyncOperationSPtr const &);

            Common::ErrorCode UpdateCurrentReplicaSetConfiguration(
                ReplicaInformationVector const & currentActiveSecondaryReplicas, 
                ULONG currentWriteQuorum);

            // Build an idle replica.
            // The replica is a new one, that doesn't exist in a 
            // previous configuration.
            Common::AsyncOperationSPtr BeginBuildReplica(
                ReplicaInformation const & toReplica,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state = Common::AsyncOperationSPtr());
            Common::ErrorCode EndBuildReplica(Common::AsyncOperationSPtr const &);

            // Removes an idle replica. 
            // If copy operation is in progress, it will be cancelled.
            Common::ErrorCode RemoveReplica(FABRIC_REPLICA_ID replicaId);

            Common::AsyncOperationSPtr BeginOnDataLoss(
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);
            Common::ErrorCode EndOnDataLoss(
                Common::AsyncOperationSPtr const &, 
                __out BOOLEAN & isStateChanged);

            // *****************************
            // IFabricStateReplicator methods
            // *****************************
            // Start a replication operation.
            // Should be called by the store only on the primary.
            // The operation returns success if the replication operation was ACKed by a quorum.
            // Otherwise, an error code describes why the operation failed:
            // the node is not the primary, no write quorum is defined, the replication queue is full etc.
            // Returns the sequence number replication associated with the operation.
            Common::AsyncOperationSPtr BeginReplicate(
                Common::ComPointer<IFabricOperationData> && comOperationPointer,
                __out FABRIC_SEQUENCE_NUMBER & sequenceNumber,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);
            Common::ErrorCode EndReplicate(
                Common::AsyncOperationSPtr const &);
            
            // Get the replication stream. 
            // The stream contains the replication operations to be applied on the store asynchronously.
            // Should be called on non-primary partition.
            // It's the store responsibility to apply the operations retrieved in the correct order.
            Common::ErrorCode GetReplicationStream(
                __out OperationStreamSPtr & stream);
            
            // Get the copy stream. 
            // The stream contains the copy operations to be applied on the store asynchronously.
            // Should be called on non-primary partition.
            // It's the store responsibility to apply the operations retrieved in the correct order.
            // If the store doesn't support parallel application 
            // of copy and replication operations,
            // it should take all copy operations before getting any of the replication operations.
            Common::ErrorCode GetCopyStream(
                __out OperationStreamSPtr & stream);

            // IFabricInternalStateReplicator Methods
            Common::ErrorCode ReserveSequenceNumber(
                __in bool alwaysReserveWhenPrimary,
                __out FABRIC_SEQUENCE_NUMBER * sequenceNumber);

            Common::ErrorCode GetReplicationQueueCounters(
                __out FABRIC_INTERNAL_REPLICATION_QUEUE_COUNTERS & counters);

            // IFabricInternalReplicator Methods
            Common::ErrorCode GetReplicatorStatus(
                __out ServiceModel::ReplicatorStatusQueryResultSPtr & result);

            // *****************************
            // MessageProcessor methods
            // *****************************
            void ProcessMessage(__in Transport::Message & request, __out Transport::ReceiverContextUPtr & from);

            // General methods
            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
            void FillEventData(Common::TraceEventContext & context) const;
            std::wstring ToString() const;
            void WriteTo(__in Common::TextWriter &, Common::FormatOptions const &) const;

            static void ReportFault(
                ComProxyStatefulServicePartition const & partition,
                Common::Guid const & partitionId,
                ReplicationEndpointId const & description,
                std::wstring const & failureDesc,
                Common::ErrorCode error, 
                FABRIC_FAULT_TYPE faultType = FABRIC_FAULT_TYPE_TRANSIENT);

            static ServiceModel::ReplicatorQueueStatus GetReplicatorQueueStatusForQuery(
                OperationQueue const & queue);

            static uint Replicator::GetQueueFullPercentage(
                OperationQueue const & queue);

        private:
            class OpenAsyncOperation;
            class CloseAsyncOperation;
            class CatchupReplicaSetAsyncOperation;
            class BuildIdleAsyncOperation;
            class ReplicateAsyncOperation;
            class ChangeRoleAsyncOperation;
            class OnDataLossAsyncOperation;
            class UpdateEpochAsyncOperation;
            class MessageJobItem
            {
                DENY_COPY(MessageJobItem);
            public:
                MessageJobItem();
                MessageJobItem(Transport::MessageUPtr && message);
                MessageJobItem(MessageJobItem &&);
                MessageJobItem& operator=(MessageJobItem&& other);
                bool ProcessJob(Replicator & siteNode);

            private:
                Transport::MessageUPtr message_;
            };

            void CreateMessageProcessorCallerHoldsLock();
            bool VerifyIsPrimaryCallerHoldsLock(
                __in std::wstring const & checkReason,
                __out Common::ErrorCode & error);
            bool VerifyIsSecondaryCallerHoldsLock(
                __in std::wstring const & checkReason,
                __out Common::ErrorCode & error);
            bool VerifyAccessGranted(__out Common::ErrorCode & error);
            bool VerifyIsPrimaryAndAccessGrantedCallerHoldsLock(
                __in std::wstring const & checkReason, 
                __in bool alwaysGrantAccessWhenPrimary, 
                __out Common::ErrorCode & error);
            void ProcessMessageInternal(__in Transport::MessageUPtr && request);
            
            REInternalSettingsSPtr const config_;
            REPerformanceCountersSPtr perfCounters_;
            bool const hasPersistedState_;
            ComProxyStatefulServicePartition partition_;
            ComProxyStateProvider stateProvider_;
            ReplicationTransportSPtr transport_;
            
            Common::Guid const partitionId_;
            // Endpoint used by the replication engine to interact 
            // with remote replication engines.
            std::wstring endpoint_;

            ReplicationEndpointId const endpointUniqueId_;

            FABRIC_REPLICA_ID const replicaId_;
                    
            ReplicatorState state_;
            PrimaryReplicatorSPtr primary_;
            SecondaryReplicatorSPtr secondary_;
            FABRIC_SEQUENCE_NUMBER stateProviderInitialProgress_;
            MUTABLE_RWLOCK(REReplicator, lock_);
            
            ReplicatorInternalVersion const version_;

            IReplicatorHealthClientSPtr healthClient_;
            ApiMonitoringWrapperSPtr apiMonitor_;


            std::unique_ptr<Common::JobQueue<MessageJobItem, Replicator>> messageProcessorJobQueueUPtr_;
        }; // end class Replicator

    } // end namespace ReplicationComponent
} // end namespace Reliability

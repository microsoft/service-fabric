// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ReplicationUnitTest
{
    using namespace Reliability::ReplicationComponent;

    class ComProxyTestReplicator;
    class ComTestStatefulServicePartition;
    typedef std::unique_ptr<ComProxyTestReplicator> ComProxyTestReplicatorUPtr;
    typedef std::shared_ptr<ComProxyTestReplicator> ComProxyTestReplicatorSPtr;

    class ComProxyTestReplicator
    {
        DENY_COPY(ComProxyTestReplicator);

    public:

        ComProxyTestReplicator(
            Common::ComPointer<::IFabricReplicator> const & replicator,
            bool supportsParallelStreams,
            bool batchEnabled);

        static void CreateComProxyTestReplicator(
            Common::Guid const & partitionId,
            __in ComReplicatorFactory & factory,
            ::FABRIC_REPLICA_ID replicaId,
            Common::ComPointer<::IFabricStateProvider> const & provider,
            bool hasPersistedState,
            bool supportsParallelStreams,
            Replicator::ReplicatorInternalVersion version,
            bool batchEnabled,
            __out ComProxyTestReplicatorSPtr & replicatorProxy,
            __out ComTestStatefulServicePartition ** partition);

        // Will only be available after open
        __declspec(property(get=get_ReplicationEndpoint)) std::wstring const & ReplicationEndpoint;
        std::wstring const & get_ReplicationEndpoint() const { return endpoint_; }

        __declspec(property(get = get_ComReplicator)) Common::ComPointer<::IFabricReplicator> const & ComReplicator;
        Common::ComPointer<::IFabricReplicator> const & get_ComReplicator() const { return replicator_; }

        // *********************************
        // IFabricReplicator methods
        // *********************************
        void Open();

        void Close();

        void ChangeRole(
            ::FABRIC_EPOCH const & epoch,
            ::FABRIC_REPLICA_ROLE newRole);

        void CheckCurrentProgress(FABRIC_SEQUENCE_NUMBER expectedProgress);

        Common::AsyncOperationSPtr BeginUpdateEpoch(
            ::FABRIC_EPOCH const * epoch,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndUpdateEpoch(Common::AsyncOperationSPtr const & asyncOperation);

        // *********************************
        // IFabricStateReplicator related methods
        // *********************************
        Common::AsyncOperationSPtr BeginReplicate(
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);
        Common::AsyncOperationSPtr BeginReplicate(
            Common::ComPointer<IFabricOperationData> && op,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndReplicate(
            FABRIC_SEQUENCE_NUMBER expectedSequenceNumber,
            Common::AsyncOperationSPtr const & asyncOperation);
        Common::AsyncOperationSPtr BeginReplicateBatch(
            Common::ComPointer<IFabricBatchOperationData> && batchOp,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndReplicateBatch(
            Common::AsyncOperationSPtr const & asyncOperation);
        Common::ErrorCode ReserveSequenceNumber(BOOLEAN alwaysReserveWhenPrimary, FABRIC_SEQUENCE_NUMBER * sequenceNumber);

        void Replicate(
            FABRIC_SEQUENCE_NUMBER expectedSequenceNumber,
            bool expectSuccess);

        void StartCopyOperationPump(ComProxyTestReplicatorSPtr const & root);
        void StartReplicationOperationPump(ComProxyTestReplicatorSPtr const & root);
        void StartBatchReplicationOperationPump(ComProxyTestReplicatorSPtr const & root);
        typedef std::function<void(Common::ComPointer<IFabricOperation> const &)> ReplicationOperationProcessor;
        void SetReplicationOperationProcessor(ReplicationOperationProcessor const & replicationOperationProcessor)
        {
            replicationOperationProcessor_ = replicationOperationProcessor;
        }
        typedef std::function<void(Common::ComPointer<IFabricBatchOperation> const &)> ReplicationBatchOperationProcessor;
        void SetReplicationBatchOperationProcessor(ReplicationBatchOperationProcessor const & replicationBatchOperationProcessor)
        {
            replicationBatchOperationProcessor_ = replicationBatchOperationProcessor;
        }
        
        // *********************************
        // IFabricPrimaryReplicator methods
        // *********************************
        void UpdateCatchUpReplicaSetConfiguration(
            Reliability::ReplicationComponent::ReplicaInformationVector const & currentReplicaDescriptions);

        void UpdateCatchUpReplicaSetConfiguration(
            Reliability::ReplicationComponent::ReplicaInformationVector const & currentReplicaDescriptions,
            ULONG currentQuorum);

        void UpdateCatchUpReplicaSetConfiguration(
            Reliability::ReplicationComponent::ReplicaInformationVector const & previousReplicaDescriptions,
            ULONG previousQuorum,
            Reliability::ReplicationComponent::ReplicaInformationVector const & currentReplicaDescriptions,
            ULONG currentQuorum);

        void UpdateCurrentReplicaSetConfiguration(
            Reliability::ReplicationComponent::ReplicaInformationVector const & currentReplicaDescriptions);

        Common::AsyncOperationSPtr BeginWaitForCatchUpQuorum(
            ::FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndWaitForCatchUpQuorum(
            Common::AsyncOperationSPtr const & asyncOperation);

        Common::AsyncOperationSPtr BeginBuildReplica( 
            Reliability::ReplicationComponent::ReplicaInformation const & idleReplicaDescription,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndBuildReplica(Common::AsyncOperationSPtr const & asyncOperation);       
        
        void BuildIdle(
            Reliability::ReplicationComponent::ReplicaInformation const & replica,
            bool cancel,
            bool successExpected,
            Common::TimeSpan const & waitTimeBeforeCancel);
        void BuildIdles(
            std::vector<Reliability::ReplicationComponent::ReplicaInformation> const & idleReplicasDescription);
        void OnDataLoss(bool expectChangeState);

        Common::ErrorCode RemoveReplica(FABRIC_REPLICA_ID replicaId);

        static Common::StringLiteral const ComProxyTestReplicatorSource;

    private:
        class OpenAsyncOperation;
        class ChangeRoleAsyncOperation;
        class BuildIdleAsyncOperation;
        class CatchUpAsyncOperation;
        class CloseAsyncOperation;
        class GetOperationAsyncOperation;
        class GetBatchOperationAsyncOperation;
        class ReplicateAsyncOperation;
        class ReplicateBatchAsyncOperation;
        class OnDataLossAsyncOperation;
        class UpdateEpochAsyncOperation;
                
        void StartOperationPump(Common::ComPointer<::IFabricOperationStream> const & stream, std::wstring const & streamPurpose, ComProxyTestReplicatorSPtr const & root);
        void StartBatchOperationPump(Common::ComPointer<::IFabricBatchOperationStream> const & batchStream, std::wstring const & streamPurpose, ComProxyTestReplicatorSPtr const & root);
        bool FinishGetOperation(Common::AsyncOperationSPtr const & asyncOperation, ComProxyTestReplicatorSPtr const & root);
        bool FinishGetBatchOperation(Common::AsyncOperationSPtr const & asyncOperation);

        Common::ComPointer<::IFabricReplicator> replicator_;
        Common::ComPointer<::IFabricStateReplicator> stateReplicator_;
        Common::ComPointer<::IFabricStateReplicator2> stateReplicator2_;
        Common::ComPointer<::IFabricInternalStateReplicator> internalStateReplicator_;
        Common::ComPointer<::IFabricPrimaryReplicator> primary_;
        bool supportsParallelStreams_;
        bool batchEnabled_;
        std::wstring endpoint_;
        ReplicationOperationProcessor replicationOperationProcessor_;
        ReplicationBatchOperationProcessor replicationBatchOperationProcessor_;
    };
}

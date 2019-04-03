// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace V1ReplPerf
{
    class Service : public Common::ComponentRoot
    {
        DENY_COPY(Service);

    public:
        Service(
            FABRIC_PARTITION_ID partitionId,
            FABRIC_REPLICA_ID replicaId,
            Common::ComponentRootSPtr const & root);

        virtual ~Service();

        void OnOpen(
            IFabricStatefulServicePartition *statefulServicePartition,
            Common::ComPointer<IFabricStateReplicator> const& replicator);

        std::wstring OnChangeRole(FABRIC_REPLICA_ROLE newRole);

        void OnClose();

        void OnAbort();

        HRESULT STDMETHODCALLTYPE UpdateEpoch(
            FABRIC_EPOCH const * epoch,
            FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber);

        HRESULT STDMETHODCALLTYPE GetCurrentProgress(
            FABRIC_SEQUENCE_NUMBER *sequenceNumber);

        HRESULT STDMETHODCALLTYPE BeginOnDataLoss(
            IFabricAsyncOperationCallback *callback,
            IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndOnDataLoss(
            IFabricAsyncOperationContext *context,
            BOOLEAN * isStateChanged);

        HRESULT STDMETHODCALLTYPE GetCopyContext(IFabricOperationDataStream ** copyContextEnumerator);

        HRESULT STDMETHODCALLTYPE GetCopyState(
            FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
            IFabricOperationDataStream * copyContextEnumerator,
            IFabricOperationDataStream ** copyStateEnumerator);

        virtual std::wstring const & GetPartitionId() const { return partitionId_; }

    private:
        class ComAsyncEnumOperationData;
        class ComGetNextOperation;

        enum PumpState { PumpNotStarted, PumpCopy, PumpReplication };
        
        std::wstring const partitionId_;
        FABRIC_REPLICA_ID const replicaId_;
        std::wstring const httpListenAddress_;
        std::wstring const changeRoleEndpoint_;
        LONGLONG const instanceId_;
        Common::ComponentRootSPtr const & root_;
        KtlSystem * ktlSystem_;

        bool isOpen_;
        FABRIC_REPLICA_ROLE replicaRole_;
        Common::ComPointer<IFabricStatefulServicePartition3> partition_;
        Common::ComPointer<IFabricStateReplicator> replicator_;
        Common::RwLock stateLock_;
        Common::ComPointer<IFabricStateProvider> stateProviderCPtr_;

        // The service doesn't support parallel pumps, 
        // so keep track of the pump we are at
        PumpState pumpState_;

        Common::ManualResetEvent pumpingCompleteEvent_;

        HttpServer::IHttpServerSPtr httpServerSPtr_;

        bool testInProgress_;
        NightWatchTXRService::PerfResult::SPtr testResultSptr_;

        void StartReplicationOperationPump(Common::ComPointer<IFabricStateReplicator> const & replicator);
        void StartCopyOperationPump(Common::ComPointer<IFabricStateReplicator> const & replicator);

        void BeginGetOperations(Common::ComPointer<IFabricStateReplicator> const & replicator, Common::ComPointer<IFabricOperationStream> const & stream);
        bool FinishProcessingOperation(IFabricAsyncOperationContext * context, Common::ComPointer<IFabricStateReplicator> const & replicator, Common::ComPointer<IFabricOperationStream> const & stream);
        void Cleanup();

        void StartTest(__in NightWatchTXRService::NightWatchTestParameters const & testParameters);

        Common::ErrorCode OnHttpPostRequest(Common::ByteBufferUPtr && body, Common::ByteBufferUPtr & responseBody);
    };

    typedef std::shared_ptr<Service> ServiceSPtr;
};

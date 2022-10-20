//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ComDummyReplicator : 
        public IFabricPrimaryReplicator,
        protected Common::ComUnknownBase,
        Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ComDummyReplicator)

        COM_INTERFACE_LIST2(
            ComDummyReplicator,
            IID_IFabricReplicator,
            IFabricReplicator,
            IID_IFabricPrimaryReplicator,
            IFabricPrimaryReplicator)

    public:
        ComDummyReplicator() = default;

        ComDummyReplicator(
            std::wstring const & serviceName,
            Common::Guid const & partitionId,
            FABRIC_REPLICA_ID replicaId);

        virtual ~ComDummyReplicator();

        FABRIC_EPOCH GetCurrentEpoch();

        HRESULT STDMETHODCALLTYPE BeginOpen(
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) override;

        HRESULT STDMETHODCALLTYPE EndOpen(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **replicationAddress) override;

        HRESULT STDMETHODCALLTYPE BeginChangeRole(
            /* [in] */ const FABRIC_EPOCH *epoch,
            /* [in] */ FABRIC_REPLICA_ROLE role,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) override;

        HRESULT STDMETHODCALLTYPE EndChangeRole(
            /* [in] */ IFabricAsyncOperationContext *context) override;

        HRESULT STDMETHODCALLTYPE BeginUpdateEpoch(
            /* [in] */ const FABRIC_EPOCH *epoch,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) override;

        HRESULT STDMETHODCALLTYPE EndUpdateEpoch(
            /* [in] */ IFabricAsyncOperationContext *context) override;

        HRESULT STDMETHODCALLTYPE BeginClose(
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) override;

        HRESULT STDMETHODCALLTYPE EndClose(
            /* [in] */ IFabricAsyncOperationContext *context) override;

        void STDMETHODCALLTYPE Abort(void) override;

        HRESULT STDMETHODCALLTYPE GetCurrentProgress(
            /* [out] */ FABRIC_SEQUENCE_NUMBER *lastSequenceNumber) override;

        HRESULT STDMETHODCALLTYPE GetCatchUpCapability(
            /* [out] */ FABRIC_SEQUENCE_NUMBER *fromSequenceNumber) override;
        
        HRESULT STDMETHODCALLTYPE BeginOnDataLoss(
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) override;

        HRESULT STDMETHODCALLTYPE EndOnDataLoss(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ BOOLEAN *isStateChanged) override;

        HRESULT STDMETHODCALLTYPE UpdateCatchUpReplicaSetConfiguration(
            /* [in] */ const FABRIC_REPLICA_SET_CONFIGURATION *currentConfiguration,
            /* [in] */ const FABRIC_REPLICA_SET_CONFIGURATION *previousConfiguration) override;

        HRESULT STDMETHODCALLTYPE BeginWaitForCatchUpQuorum(
            /* [in] */ FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) override;

        HRESULT STDMETHODCALLTYPE EndWaitForCatchUpQuorum(
            /* [in] */ IFabricAsyncOperationContext *context) override;

        HRESULT STDMETHODCALLTYPE UpdateCurrentReplicaSetConfiguration(
            /* [in] */ const FABRIC_REPLICA_SET_CONFIGURATION *currentConfiguration) override;

        HRESULT STDMETHODCALLTYPE BeginBuildReplica(
            /* [in] */ const FABRIC_REPLICA_INFORMATION *replica,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) override;

        HRESULT STDMETHODCALLTYPE EndBuildReplica(
            /* [in] */ IFabricAsyncOperationContext *context) override;

        HRESULT STDMETHODCALLTYPE RemoveReplica(
            /* [in] */ FABRIC_REPLICA_ID replicaId) override;

     private:
        std::wstring const traceId_;

        // NOTE: This value is written only in the replicator changerole/updateepoch and is read only in the replica changerole.
        // So there is no need for explicit synchronization between the read and write as of now. Please add synchronization if
        // this is accessed outside of changerole.
        FABRIC_EPOCH currentEpoch_;
    };
}

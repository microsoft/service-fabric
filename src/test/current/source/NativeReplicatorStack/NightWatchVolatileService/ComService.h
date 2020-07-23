// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace V1ReplPerf
{
    class ComService : public IFabricStatefulServiceReplica, public IFabricStateProvider, public Common::ComUnknownBase
    {
        DENY_COPY(ComService);

        BEGIN_COM_INTERFACE_LIST(ComService)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatefulServiceReplica)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServiceReplica, IFabricStatefulServiceReplica)
            COM_INTERFACE_ITEM(IID_IFabricStateProvider, IFabricStateProvider)
        END_COM_INTERFACE_LIST()

    public:
        ComService(Service & service);
        virtual ~ComService() {}

        HRESULT STDMETHODCALLTYPE BeginOpen( 
            /* [in] */ FABRIC_REPLICA_OPEN_MODE openMode,
            /* [in] */ IFabricStatefulServicePartition *partition,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndOpen( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricReplicator **replicationEngine);

        HRESULT STDMETHODCALLTYPE BeginChangeRole( 
            /* [in] */ FABRIC_REPLICA_ROLE newRole,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndChangeRole( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **serviceEndpoint);

        HRESULT STDMETHODCALLTYPE BeginClose( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndClose( 
            /* [in] */ IFabricAsyncOperationContext *context);

        void STDMETHODCALLTYPE Abort();

        HRESULT STDMETHODCALLTYPE BeginUpdateEpoch( 
            /* [in] */ ::FABRIC_EPOCH const * epoch,
            /* [in] */ ::FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        HRESULT STDMETHODCALLTYPE EndUpdateEpoch( 
            /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE GetLastCommittedSequenceNumber( 
            /* [retval][out] */ FABRIC_SEQUENCE_NUMBER *sequenceNumber);

        HRESULT STDMETHODCALLTYPE BeginOnDataLoss( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndOnDataLoss( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][string][out] */ BOOLEAN * isStateChanged);
        
        HRESULT STDMETHODCALLTYPE GetCopyContext(/*[out, retval]*/ IFabricOperationDataStream ** copyContextEnumerator);

        HRESULT STDMETHODCALLTYPE GetCopyState(
            /*[in]*/ FABRIC_SEQUENCE_NUMBER uptoSequenceNumber, 
            /*[in]*/ IFabricOperationDataStream * copyContextEnumerator,
            /*[out, retval]*/ IFabricOperationDataStream ** copyStateEnumerator);

    private:
        Common::ComponentRootSPtr root_;
        Service & service_;
        Common::ComPointer<IFabricReplicator> replicationEngine_;
        Common::ComPointer<IFabricStateReplicator> stateReplicator_;
        std::wstring serviceEndpoint_;
    };
};

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class ComTXRService
        : public IFabricStatefulServiceReplica
        , public Common::ComUnknownBase
    {
        DENY_COPY(ComTXRService);

        BEGIN_COM_INTERFACE_LIST(ComTXRService)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatefulServiceReplica)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServiceReplica, IFabricStatefulServiceReplica)
        END_COM_INTERFACE_LIST()

    public:
        ComTXRService(TXRService & innerService);
        virtual ~ComTXRService() {}

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

    private:
        virtual ComPointer<IFabricStateProvider2Factory> CreateStateProvider2Factory(
            __in std::wstring const & nodeId,
            __in std::wstring const & serviceName,
            __in KAllocator & allocator);

        virtual ComPointer<IFabricDataLossHandler> CreateDataLossHandler(
            __in KAllocator & allocator,
            __out TxnReplicator::TestCommon::TestDataLossHandler::SPtr & dataLossHandler);

        void CheckForReportFaultsAndDelays(
            Common::ComPointer<IFabricStatefulServicePartition> partition, 
            ApiFaultHelper::ComponentName compName, 
            std::wstring operationName);

        Common::ComponentRootSPtr root_;
        TXRService & innerService_;
        Common::ComPointer<IFabricPrimaryReplicator> primaryReplicator_;
        bool isClosedCalled_;
        bool isDecrementSequenceNumberNeeded_;
    };
}

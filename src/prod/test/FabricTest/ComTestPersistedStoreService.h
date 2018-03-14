// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class ComTestPersistedStoreService :
        public IFabricStatefulServiceReplica,
        public IFabricStateProvider,
        public Common::ComUnknownBase
    {
        DENY_COPY(ComTestPersistedStoreService)

        BEGIN_COM_INTERFACE_LIST(ComTestPersistedStoreService)
        COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatefulServiceReplica)
        COM_INTERFACE_ITEM(IID_IFabricStatefulServiceReplica, IFabricStatefulServiceReplica)
        COM_INTERFACE_ITEM(IID_IFabricStateProvider, IFabricStateProvider)
        END_COM_INTERFACE_LIST()

    public:
        ComTestPersistedStoreService(__in TestPersistedStoreService &);
        virtual ~ComTestPersistedStoreService();

        // ***********************
        // IFabricStatefulServiceReplica
        // ***********************

        virtual HRESULT STDMETHODCALLTYPE BeginOpen(
            __in ::FABRIC_REPLICA_OPEN_MODE openMode,
            __in IFabricStatefulServicePartition * partition, 
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context);
        virtual HRESULT STDMETHODCALLTYPE EndOpen(
            __in IFabricAsyncOperationContext * context,
            __out IFabricReplicator ** replication);

        virtual HRESULT STDMETHODCALLTYPE BeginChangeRole(
            __in ::FABRIC_REPLICA_ROLE newRole,
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context);
        virtual HRESULT STDMETHODCALLTYPE EndChangeRole(
            __in IFabricAsyncOperationContext * context,
            __out IFabricStringResult ** serviceLocation);

        virtual HRESULT STDMETHODCALLTYPE BeginClose(
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context);
        virtual HRESULT STDMETHODCALLTYPE EndClose(
            __in IFabricAsyncOperationContext * context);

        virtual void STDMETHODCALLTYPE Abort();
        // **************
        // IFabricStateProvider
        // **************
        
        virtual HRESULT STDMETHODCALLTYPE BeginUpdateEpoch( 
            __in ::FABRIC_EPOCH const * epoch,
            __in ::FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context);
        virtual HRESULT STDMETHODCALLTYPE EndUpdateEpoch( 
            __in IFabricAsyncOperationContext *context);

        virtual HRESULT STDMETHODCALLTYPE GetLastCommittedSequenceNumber(
            __out ::FABRIC_SEQUENCE_NUMBER * sequenceNumber);

        virtual HRESULT STDMETHODCALLTYPE BeginOnDataLoss(
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context);

        virtual HRESULT STDMETHODCALLTYPE EndOnDataLoss(
            __in IFabricAsyncOperationContext * context,
            __out BOOLEAN * isStateChangedResult);

        virtual HRESULT STDMETHODCALLTYPE GetCopyContext(
            __out IFabricOperationDataStream ** context);
    
        virtual HRESULT STDMETHODCALLTYPE GetCopyState(
            __in ::FABRIC_SEQUENCE_NUMBER uptoOperationLSN,
            __in IFabricOperationDataStream * context, 
            __out IFabricOperationDataStream ** enumerator);

    private:
        class ComOpenOperationContext;
        class ComChangeRoleOperationContext;
        class ComCloseOperationContext;
        class ComOnDataLossOperationContext;

        HRESULT InitializeAndStart(
            __in Common::ComAsyncOperationContextCPtr & contextCPtr,
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context);

        void CheckForReportFaultsAndDelays(Common::ComPointer<IFabricStatefulServicePartition> partition, ApiFaultHelper::ComponentName compName, std::wstring operationName);

        Common::ComponentRootSPtr root_;
        TestPersistedStoreService & service_;
        bool isClosedCalled_;
        bool isDecrementSequenceNumberNeeded_;

       ::FABRIC_REPLICA_ROLE role_;
       ReadWriteStatusValidatorUPtr readWriteStatusValidator_;
    };
};

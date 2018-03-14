// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class SGComStatefulService : 
        public IFabricStatefulServiceReplica, 
        public IFabricStateProvider, 
        public IFabricAtomicGroupStateProvider,
        public Common::ComUnknownBase
    {
        DENY_COPY(SGComStatefulService);

        BEGIN_COM_INTERFACE_LIST(SGComStatefulService)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatefulServiceReplica)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServiceReplica, IFabricStatefulServiceReplica)
            COM_INTERFACE_ITEM(IID_IFabricStateProvider, IFabricStateProvider)
            if (riid == (IID_IFabricAtomicGroupStateProvider) && ImplementAtomicGroupStateProvider()) 
            { *result = static_cast<IFabricAtomicGroupStateProvider*>(this); } 
            else
        END_COM_INTERFACE_LIST()
    public:
        //
        // Constructor/Destructor.
        //
        SGComStatefulService(SGStatefulService & service);
        virtual ~SGComStatefulService();

        //
        // IFabricStatefulServiceReplica methods. 
        //
        virtual HRESULT STDMETHODCALLTYPE BeginOpen(
            __in FABRIC_REPLICA_OPEN_MODE openMode,
            __in IFabricStatefulServicePartition* partition,
            __in IFabricAsyncOperationCallback* callback,
            __out IFabricAsyncOperationContext** context);
        virtual HRESULT STDMETHODCALLTYPE EndOpen(
            __in IFabricAsyncOperationContext* context,
            __out IFabricReplicator** replicator);

        virtual HRESULT STDMETHODCALLTYPE BeginChangeRole(
            __in FABRIC_REPLICA_ROLE newRole,
            __in IFabricAsyncOperationCallback* callback,
            __out IFabricAsyncOperationContext** context);
        virtual HRESULT STDMETHODCALLTYPE EndChangeRole(
            __in IFabricAsyncOperationContext* context,
            __out IFabricStringResult** serviceEndpoint);

        virtual HRESULT STDMETHODCALLTYPE BeginClose(
            __in IFabricAsyncOperationCallback* callback,
            __out IFabricAsyncOperationContext** context);
        virtual HRESULT STDMETHODCALLTYPE EndClose(
            __in IFabricAsyncOperationContext * context);

        virtual void STDMETHODCALLTYPE Abort();

        //
        // IFabricStateProvider methods.
        //
        virtual HRESULT STDMETHODCALLTYPE BeginUpdateEpoch( 
            __in ::FABRIC_EPOCH const * epoch,
            __in ::FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context);
        virtual HRESULT STDMETHODCALLTYPE EndUpdateEpoch( 
            __in IFabricAsyncOperationContext *context);

        virtual HRESULT STDMETHODCALLTYPE GetLastCommittedSequenceNumber(
            __out FABRIC_SEQUENCE_NUMBER* sequenceNumber);

        virtual HRESULT STDMETHODCALLTYPE BeginOnDataLoss(
            __in IFabricAsyncOperationCallback* callback,
            __out IFabricAsyncOperationContext** context);
        virtual HRESULT STDMETHODCALLTYPE EndOnDataLoss(
            __in IFabricAsyncOperationContext* context,
            __out BOOLEAN* isStateChanged);

        virtual HRESULT STDMETHODCALLTYPE GetCopyContext(
            __out IFabricOperationDataStream** copyContextEnumerator);

        virtual HRESULT STDMETHODCALLTYPE GetCopyState(
            __in FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
            __in IFabricOperationDataStream* copyContextEnumerator,
            __out IFabricOperationDataStream** copyStateEnumerator);

        //
        // IFabricAtomicGroupStateProvider methods.
        //
        virtual HRESULT STDMETHODCALLTYPE BeginAtomicGroupCommit( 
            __in FABRIC_ATOMIC_GROUP_ID atomicGroupId,
            __in FABRIC_SEQUENCE_NUMBER commitSequenceNumber,
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context) ;
        
        virtual HRESULT STDMETHODCALLTYPE EndAtomicGroupCommit( 
            __in IFabricAsyncOperationContext *context) ;
        
        virtual HRESULT STDMETHODCALLTYPE BeginAtomicGroupRollback( 
            __in FABRIC_ATOMIC_GROUP_ID atomicGroupId,
            __in FABRIC_SEQUENCE_NUMBER rollbackequenceNumber,
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context) ;
        
        virtual HRESULT STDMETHODCALLTYPE EndAtomicGroupRollback( 
            __in IFabricAsyncOperationContext *context) ;
        
        virtual HRESULT STDMETHODCALLTYPE BeginUndoProgress( 
            __in FABRIC_SEQUENCE_NUMBER fromCommitSequenceNumber,
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context) ;
        
        virtual HRESULT STDMETHODCALLTYPE EndUndoProgress( 
            __in IFabricAsyncOperationContext *context) ;

    private:

        bool ImplementAtomicGroupStateProvider()
        {
            return service_.Settings.GetBool(L"atomicgroupstateprovider");
        }

        SGStatefulService & service_;

        Common::ComponentRootSPtr root_;

        Common::ComPointer<IFabricReplicator> replicator_;

        static Common::StringLiteral const TraceSource;
    };
};

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {99EBA99E-7686-42B8-909E-58216EC41160}
    static const GUID CLSID_ComStatefulServiceReplica = 
    { 0x99eba99e, 0x7686, 0x42b8, { 0x90, 0x9e, 0x58, 0x21, 0x6e, 0xc4, 0x11, 0x60 } };

    class ComStatefulServiceReplica :
        public IFabricStatefulServiceReplica,
        public IFabricInternalStatefulServiceReplica2,
        protected Common::ComUnknownBase
    {
        DENY_COPY(ComStatefulServiceReplica)

        BEGIN_COM_INTERFACE_LIST(ComStatefulServiceReplica)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatefulServiceReplica)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServiceReplica, IFabricStatefulServiceReplica)
            COM_INTERFACE_ITEM(IID_IFabricInternalStatefulServiceReplica, IFabricInternalStatefulServiceReplica)
            COM_INTERFACE_ITEM(IID_IFabricInternalStatefulServiceReplica2, IFabricInternalStatefulServiceReplica2)
            COM_INTERFACE_ITEM(CLSID_ComStatefulServiceReplica, ComStatefulServiceReplica)
        END_COM_INTERFACE_LIST()

    public:
        ComStatefulServiceReplica(IStatefulServiceReplicaPtr const & impl);
        virtual ~ComStatefulServiceReplica();

        inline IStatefulServiceReplicaPtr const & get_Impl() const { return impl_; }

        // 
        // IFabricStatefulServiceReplica methods
        // 
        HRESULT STDMETHODCALLTYPE BeginOpen( 
            /* [in] */ FABRIC_REPLICA_OPEN_MODE openMode,
            /* [in] */ IFabricStatefulServicePartition *partition,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT STDMETHODCALLTYPE EndOpen( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricReplicator **replicator);
        
        HRESULT STDMETHODCALLTYPE BeginChangeRole( 
            /* [in] */ FABRIC_REPLICA_ROLE newRole,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT STDMETHODCALLTYPE EndChangeRole( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **serviceAddress);
        
        HRESULT STDMETHODCALLTYPE BeginClose( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT STDMETHODCALLTYPE EndClose( 
            /* [in] */ IFabricAsyncOperationContext *context);
        
        void STDMETHODCALLTYPE Abort(void);

        //
        // IFabricInternalStatefulServiceReplica
        //

        HRESULT STDMETHODCALLTYPE GetStatus(
            /* [out, retval] */ IFabricStatefulServiceReplicaStatusResult ** result);

        //
        // IFabricInternalStatefulServiceReplica2
        //

        HRESULT UpdateInitializationData(
            /* [in] */ ULONG bufferSize,
            /* [in] */ const BYTE * buffer);

    private:
        class OpenOperationContext;
        class ChangeRoleOperationContext;
        class CloseOperationContext;

    private:
        IStatefulServiceReplicaPtr impl_;
    };
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTypeHost
{
    class ComStatelessServiceInstance : 
        public IFabricStatelessServiceInstance,
        protected Common::ComUnknownBase,
        Common::TextTraceComponent<Common::TraceTaskCodes::FabricTypeHost>
    {
        DENY_COPY(ComStatelessServiceInstance)

        COM_INTERFACE_LIST1(
            ComStatelessServiceInstance,
            IID_IFabricStatelessServiceInstance,
            IFabricStatelessServiceInstance)

    public:
        ComStatelessServiceInstance(
            std::wstring const & serviceName,
            std::wstring const & serviceTypeName,
            Common::Guid const & partitionId,
            FABRIC_INSTANCE_ID instanceId);
        virtual ~ComStatelessServiceInstance();

        HRESULT STDMETHODCALLTYPE BeginOpen( 
            /* [in] */ IFabricStatelessServicePartition *partition,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT STDMETHODCALLTYPE EndOpen( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **serviceAddress);
        
        HRESULT STDMETHODCALLTYPE BeginClose( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT STDMETHODCALLTYPE EndClose( 
            /* [in] */ IFabricAsyncOperationContext *context);
        
        void STDMETHODCALLTYPE Abort(void);

    private:
        HRESULT SetupEndpointAddresses();

    private:
        std::wstring const serviceName_;
        std::wstring const serviceTypeName_;
        std::wstring const id_;
        std::wstring endpoints_;
        Common::Guid const partitionId_;
        ::FABRIC_INSTANCE_ID const instanceId_;
    };
}

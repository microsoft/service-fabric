// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {47FF07EB-BC7B-4A86-88FC-97443376E4DE}
    static const GUID CLSID_ComStatelessServiceInstance = 
    { 0x47ff07eb, 0xbc7b, 0x4a86, { 0x88, 0xfc, 0x97, 0x44, 0x33, 0x76, 0xe4, 0xde } };

    class ComStatelessServiceInstance :
        public IFabricStatelessServiceInstance,
        protected Common::ComUnknownBase
    {
        DENY_COPY(ComStatelessServiceInstance)

        BEGIN_COM_INTERFACE_LIST(ComStatelessServiceInstance)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatelessServiceInstance)
            COM_INTERFACE_ITEM(IID_IFabricStatelessServiceInstance, IFabricStatelessServiceInstance)
            COM_INTERFACE_ITEM(CLSID_ComStatelessServiceInstance, ComStatelessServiceInstance)
        END_COM_INTERFACE_LIST()

    public:
        ComStatelessServiceInstance(IStatelessServiceInstancePtr const & impl);
        virtual ~ComStatelessServiceInstance();

        // 
        // IFabricStatelessServiceInstance methods
        // 
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
        
        void STDMETHODCALLTYPE Abort( void);

    private:
        class OpenOperationContext;
        class CloseOperationContext;

    private:
        IStatelessServiceInstancePtr impl_;
    };
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // {a069561b-7885-44b1-904a-3f0a3e1b2cd9}
    static const GUID CLSID_ComFabricRuntime = 
        { 0xa069561b, 0x7885, 0x44b1, { 0x90, 0x4a, 0x3f, 0x0a, 0x3e, 0x1b, 0x2c, 0xd9 } };

    class ComFabricRuntime :
        public IFabricRuntime,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComFabricRuntime);

        BEGIN_COM_INTERFACE_LIST(ComFabricRuntime)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricRuntime)
            COM_INTERFACE_ITEM(IID_IFabricRuntime, IFabricRuntime)
            COM_INTERFACE_ITEM(CLSID_ComFabricRuntime, ComFabricRuntime)
        END_COM_INTERFACE_LIST()

    public:
        ComFabricRuntime(FabricRuntimeImplSPtr const & fabricRuntime);
        virtual ~ComFabricRuntime();

        HRESULT STDMETHODCALLTYPE BeginRegisterStatelessServiceFactory( 
            /* [in] */ LPCWSTR serviceType,
            /* [in] */ IFabricStatelessServiceFactory *factory,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT STDMETHODCALLTYPE EndRegisterStatelessServiceFactory( 
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT STDMETHODCALLTYPE RegisterStatelessServiceFactory( 
            /* [in] */ LPCWSTR serviceType,
            /* [in] */ IFabricStatelessServiceFactory *factory);
        
        HRESULT STDMETHODCALLTYPE BeginRegisterStatefulServiceFactory( 
            /* [in] */ LPCWSTR serviceType,
            /* [in] */ IFabricStatefulServiceFactory *factory,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT STDMETHODCALLTYPE EndRegisterStatefulServiceFactory( 
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT STDMETHODCALLTYPE RegisterStatefulServiceFactory( 
            /* [in] */ LPCWSTR serviceType,
            /* [in] */ IFabricStatefulServiceFactory *factory);
         
        HRESULT STDMETHODCALLTYPE CreateServiceGroupFactoryBuilder( 
            /* [retval][out] */ IFabricServiceGroupFactoryBuilder **builder);
        
        HRESULT STDMETHODCALLTYPE BeginRegisterServiceGroupFactory( 
            /* [in] */ LPCWSTR groupServiceType,
            /* [in] */ IFabricServiceGroupFactory *factory,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT STDMETHODCALLTYPE EndRegisterServiceGroupFactory( 
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT STDMETHODCALLTYPE RegisterServiceGroupFactory( 
            /* [in] */ LPCWSTR groupServiceType,
            /* [in] */ IFabricServiceGroupFactory *factory);

        __declspec(property(get=get_Runtime)) FabricRuntimeImplSPtr const & Runtime;
        inline FabricRuntimeImplSPtr const & get_Runtime() const { return fabricRuntime_; }

    private:
        class RegisterStatelessServiceFactoryComAsyncOperationContext;
        class RegisterStatefulServiceFactoryComAsyncOperationContext;
        class RegisterServiceGroupFactoryComAsyncOperationContext;

    private: 
        FabricRuntimeImplSPtr const fabricRuntime_;
    };
}

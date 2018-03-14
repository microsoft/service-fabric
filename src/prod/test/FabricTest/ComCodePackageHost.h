// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class ComCodePackageHost : public IFabricCodePackageHost, public Common::ComUnknownBase
    {
        DENY_COPY(ComCodePackageHost);

        COM_INTERFACE_LIST1(
            ComCodePackageHost,
            IID_IFabricCodePackageHost,
            IFabricCodePackageHost)

    public:
        explicit ComCodePackageHost(std::wstring const& hostId) : hostId_(hostId){}
        virtual ~ComCodePackageHost(){ }

        HRESULT STDMETHODCALLTYPE BeginActivate( 
            LPCWSTR codePackageId,
            IFabricCodePackageActivationContext * activationContext,
            IFabricRuntime * fabricRuntime,
            IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context);
        HRESULT STDMETHODCALLTYPE EndActivate( 
            IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE BeginDeactivate( 
            LPCWSTR codePackageId,
            IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context);
        HRESULT STDMETHODCALLTYPE EndDeactivate( 
            IFabricAsyncOperationContext *context);

    private:
        std::wstring hostId_;
    };
};

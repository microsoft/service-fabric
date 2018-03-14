// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    static const GUID CLSID_ComCompletedAsyncOperationContext = 
        {0x3abbe2ae,0x8470,0x47d2,{0xbd,0x1b,0x0b,0xa7,0x49,0x66,0x5e,0x23}};

    class ComCompletedAsyncOperationContext : public ComAsyncOperationContext
    {
        DENY_COPY(ComCompletedAsyncOperationContext)

        COM_INTERFACE_AND_DELEGATE_LIST(
            ComCompletedAsyncOperationContext,
            CLSID_ComCompletedAsyncOperationContext,
            ComCompletedAsyncOperationContext,
            ComAsyncOperationContext);

    public:
        ComCompletedAsyncOperationContext() {}
        virtual ~ComCompletedAsyncOperationContext() {}
        static HRESULT End(__in IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE Initialize(
            __in ComponentRootSPtr const & rootSPtr,
            __in_opt IFabricAsyncOperationCallback * callback);

        HRESULT STDMETHODCALLTYPE Initialize(
            __in HRESULT completionResult,
            __in ComponentRootSPtr const & rootSPtr,
            __in_opt IFabricAsyncOperationCallback * callback);

        HRESULT STDMETHODCALLTYPE Initialize(
            __in HRESULT completionResult,
            __in TimeSpan delayTime,
            __in ComponentRootSPtr const & rootSPtr,
            __in_opt IFabricAsyncOperationCallback * callback);

    protected:
        virtual void OnStart(
            __in Common::AsyncOperationSPtr const & proxySPtr);

    private:
        HRESULT completionResult_;
        Common::TimeSpan delayTime_;
    };
}


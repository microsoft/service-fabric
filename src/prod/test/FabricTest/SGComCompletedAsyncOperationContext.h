// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    // {3BF8EA3A-23EF-4AC8-8469-B0D615ECB6C6}
    static const GUID CLSID_SGComCompletedAsyncOperationContext = 
    { 0x3bf8ea3a, 0x23ef, 0x4ac8, { 0x84, 0x69, 0xb0, 0xd6, 0x15, 0xec, 0xb6, 0xc6 } };


    class SGComCompletedAsyncOperationContext : public ComAsyncOperationContext
    {
        DENY_COPY(SGComCompletedAsyncOperationContext)

        COM_INTERFACE_AND_DELEGATE_LIST(
            SGComCompletedAsyncOperationContext,
            CLSID_SGComCompletedAsyncOperationContext,
            SGComCompletedAsyncOperationContext,
            ComAsyncOperationContext);

    public:
        SGComCompletedAsyncOperationContext() : random_() {}
        virtual ~SGComCompletedAsyncOperationContext() {}
        static HRESULT End(__in IFabricAsyncOperationContext * context);

        HRESULT STDMETHODCALLTYPE Initialize(
            __in ComponentRootSPtr const & rootSPtr,
            __in_opt IFabricAsyncOperationCallback * callback);

        HRESULT STDMETHODCALLTYPE Initialize(
            __in HRESULT completionResult,
            __in ComponentRootSPtr const & rootSPtr,
            __in_opt IFabricAsyncOperationCallback * callback);

    protected:
        virtual void OnStart(
            __in Common::AsyncOperationSPtr const & proxySPtr);
        virtual bool OnCancel () { return true; }

    private:
        HRESULT completionResult_;
        Common::Random random_;
    };
} // end namespace FabricTest

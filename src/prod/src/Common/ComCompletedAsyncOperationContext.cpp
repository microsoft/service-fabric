// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Common
{
    HRESULT ComCompletedAsyncOperationContext::End(__in IFabricAsyncOperationContext * context)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<ComCompletedAsyncOperationContext> thisOperation(context, CLSID_ComCompletedAsyncOperationContext);
        HRESULT hr = thisOperation->ComAsyncOperationContext::End();;
        if (FAILED(hr)) { return hr; }

        return thisOperation->Result;
    }

    HRESULT STDMETHODCALLTYPE ComCompletedAsyncOperationContext::Initialize(
        __in ComponentRootSPtr const & rootSPtr,
        __in_opt IFabricAsyncOperationCallback * callback)
    {
        return Initialize(S_OK, rootSPtr, callback);
    }

    HRESULT STDMETHODCALLTYPE ComCompletedAsyncOperationContext::Initialize(
        __in HRESULT completionResult,
        __in ComponentRootSPtr const & rootSPtr,
        __in_opt IFabricAsyncOperationCallback * callback)
    {
        return Initialize(completionResult, TimeSpan::Zero, rootSPtr, callback);
    }

    
    HRESULT STDMETHODCALLTYPE ComCompletedAsyncOperationContext::Initialize(
        __in HRESULT completionResult,
        __in TimeSpan delayTime,
        __in ComponentRootSPtr const & rootSPtr,
        __in_opt IFabricAsyncOperationCallback * callback)
    {
        completionResult_ = completionResult;
        delayTime_ = delayTime;
        HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
        if (FAILED(hr)) { return hr; }
        return S_OK;
    }

    void ComCompletedAsyncOperationContext::OnStart(
        __in Common::AsyncOperationSPtr const & proxySPtr)
    {
        if (delayTime_ == TimeSpan::Zero)
        {
            this->TryComplete(proxySPtr, completionResult_);
        }
        else
        {
            Threadpool::Post([this, proxySPtr]
            {
                this->TryComplete(proxySPtr, completionResult_);
            },
            delayTime_,
            false);
        }
    }
}

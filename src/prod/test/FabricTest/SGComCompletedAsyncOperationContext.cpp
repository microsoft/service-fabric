// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace FabricTest;
using namespace Common;

HRESULT SGComCompletedAsyncOperationContext::End(__in IFabricAsyncOperationContext * context)
{
    if (context == NULL) { return E_POINTER; }

    ComPointer<SGComCompletedAsyncOperationContext> thisOperation(context, CLSID_SGComCompletedAsyncOperationContext);
    HRESULT hr = thisOperation->ComAsyncOperationContext::End();;
    if (FAILED(hr)) { return hr; }

    return thisOperation->Result;
}

HRESULT STDMETHODCALLTYPE SGComCompletedAsyncOperationContext::Initialize(
    __in ComponentRootSPtr const & rootSPtr,
    __in_opt IFabricAsyncOperationCallback * callback)
{
    return Initialize(S_OK, rootSPtr, callback);
}

HRESULT STDMETHODCALLTYPE SGComCompletedAsyncOperationContext::Initialize(
    __in HRESULT completionResult,
    __in ComponentRootSPtr const & rootSPtr,
    __in_opt IFabricAsyncOperationCallback * callback)
{
    completionResult_ = completionResult;
    HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
    if (FAILED(hr)) { return hr; }
    return S_OK;
}

void SGComCompletedAsyncOperationContext::OnStart(
    __in Common::AsyncOperationSPtr const & proxySPtr)
{
    if (0 == (random_.Next() & 1))
    {
        this->TryComplete(proxySPtr, completionResult_);
    }
    else
    {
        Threadpool::Post([this, proxySPtr]()
        {
            this->TryComplete(proxySPtr, this->completionResult_);
        });
    }
}

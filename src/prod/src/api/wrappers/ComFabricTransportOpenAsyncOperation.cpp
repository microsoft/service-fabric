// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Transport;

HRESULT ComFabricTransportOpenAsyncOperation::Initialize(
    __in DWORD timeoutInMilliSeconds,
    __in IFabricAsyncOperationCallback * callback,
    Common::ComponentRootSPtr const & rootSPtr)
{
    HRESULT hr = this->ComAsyncOperationContext::Initialize(rootSPtr, callback);
    if (FAILED(hr)) { return hr; }

    if (timeoutInMilliSeconds == INFINITE)
    {
        timeout_ = TimeSpan::MaxValue;
    }
    else
    {
        timeout_ = TimeSpan::FromMilliseconds(timeoutInMilliSeconds);
    }

    return hr;
}

HRESULT ComFabricTransportOpenAsyncOperation::End(__in IFabricAsyncOperationContext * context)
{
    if (context == NULL) { return E_POINTER; }

    ComPointer<ComFabricTransportOpenAsyncOperation> thisOperation(context, CLSID_ComFabricTransportOpenAsyncOperation);


    HRESULT hr = thisOperation->ComAsyncOperationContextEnd();
  
    return hr;
}


void ComFabricTransportOpenAsyncOperation::OnStart(AsyncOperationSPtr const & proxySPtr)
{
    auto operation = impl_->BeginOpen(
        timeout_,
        [this](AsyncOperationSPtr const & operation)
    {
        this->FinishRequest(operation, false);
    },
        proxySPtr);

    this->FinishRequest(operation, true);
}

void ComFabricTransportOpenAsyncOperation::FinishRequest(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = impl_->EndOpen(operation);

    TryComplete(operation->Parent, error.ToHResult());
}

HRESULT ComFabricTransportOpenAsyncOperation::ComAsyncOperationContextEnd()
{
    return ComAsyncOperationContext::End();
}

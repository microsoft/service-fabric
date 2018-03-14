// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Transport;

HRESULT ComFabricTransportSendRequestAsyncOperation::Initialize(
    __in IFabricTransportMessage *request,
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

    if (request == NULL) { return E_POINTER; }
    hr = ComFabricTransportMessage::ToNativeTransportMessage(request ,deleteCallBack_, request_);

    return hr;
}

HRESULT ComFabricTransportSendRequestAsyncOperation::End(__in IFabricAsyncOperationContext * context, __out IFabricTransportMessage **reply)
{
    if (context == NULL) { return E_POINTER; }

    ComPointer<ComFabricTransportSendRequestAsyncOperation> thisOperation(context, CLSID_ComFabricTransportSendRequestAsyncOperation);


    HRESULT hr = thisOperation->ComAsyncOperationContextEnd();
    if (SUCCEEDED(hr))
    {
        auto replyCptr = make_com<ComFabricTransportMessage, IFabricTransportMessage>(move(thisOperation->reply_));
        *reply = replyCptr.DetachNoRelease();

    }

    return hr;
}


void ComFabricTransportSendRequestAsyncOperation::OnStart(AsyncOperationSPtr const & proxySPtr)
{
    auto operation = impl_->BeginRequest(
        move(request_),
        timeout_,
        [this](AsyncOperationSPtr const & operation)
    {
        this->FinishRequest(operation, false);
    },
        proxySPtr);

    this->FinishRequest(operation, true);
}

void ComFabricTransportSendRequestAsyncOperation::FinishRequest(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = impl_->EndRequest(operation, reply_);

    TryComplete(operation->Parent, error.ToHResult());
}

HRESULT ComFabricTransportSendRequestAsyncOperation::ComAsyncOperationContextEnd()
{
    return ComAsyncOperationContext::End();
}

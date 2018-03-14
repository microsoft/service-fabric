// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Transport;



HRESULT ComFabricSendRequestAsyncOperation::Initialize(
    __in IFabricServiceCommunicationMessage *request,
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
    hr = ComFabricServiceCommunicationMessage::ToNativeTransportMessage(request, request_);

    return hr;
}

HRESULT ComFabricSendRequestAsyncOperation::End(__in IFabricAsyncOperationContext * context, __out IFabricServiceCommunicationMessage **reply)
{
    if (context == NULL) { return E_POINTER; }

    ComPointer<ComFabricSendRequestAsyncOperation> thisOperation(context, CLSID_SendRequestAsyncOperation);


    HRESULT hr = thisOperation->ComAsyncOperationContextEnd();
    if (SUCCEEDED(hr))
    {
        auto replyCptr = make_com<ComFabricServiceCommunicationMessage, IFabricServiceCommunicationMessage>(thisOperation->reply_);
        *reply = replyCptr.DetachNoRelease();

    }

    return hr;
}


void ComFabricSendRequestAsyncOperation::OnStart(AsyncOperationSPtr const & proxySPtr)
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

void ComFabricSendRequestAsyncOperation::FinishRequest(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = impl_->EndRequest(operation, reply_);

    TryComplete(operation->Parent, error.ToHResult());
}

HRESULT ComFabricSendRequestAsyncOperation::ComAsyncOperationContextEnd()
{
    return ComAsyncOperationContext::End();
}

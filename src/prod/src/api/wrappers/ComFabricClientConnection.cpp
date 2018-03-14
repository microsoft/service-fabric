// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Transport;

ComFabricClientConnection::ComFabricClientConnection(IClientConnectionPtr const & impl)
: IFabricClientConnection(),
ComUnknownBase(),
impl_(impl)
{
}

HRESULT  ComFabricClientConnection::BeginRequest(
    /* [in] */ IFabricServiceCommunicationMessage *message,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{

    ComPointer<IUnknown> rootCPtr;
    HRESULT hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr)) { return hr; }

    auto rootSPtr = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    auto senderImpl = RootedObjectPointer<ICommunicationMessageSender>(impl_.get(), impl_.get_Root());
    ComPointer<ComFabricSendRequestAsyncOperation> operation = make_com<ComFabricSendRequestAsyncOperation>(senderImpl);

    hr = operation->Initialize(
        message,
        timeoutMilliseconds,
        callback,
        rootSPtr);

    if (FAILED(hr)) 
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT  ComFabricClientConnection::EndRequest(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricServiceCommunicationMessage **reply)
{
    return ComUtility::OnPublicApiReturn(ComFabricSendRequestAsyncOperation::End(context, reply));
}

HRESULT  ComFabricClientConnection::SendMessage(
    /* [in] */ IFabricServiceCommunicationMessage *message)
{
    MessageUPtr msg;
    auto    hr = ComFabricServiceCommunicationMessage::ToNativeTransportMessage(message, msg);
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }
    auto error = impl_->SendOneWay(move(msg));
    return ComUtility::OnPublicApiReturn(error.ToHResult());
}

COMMUNICATION_CLIENT_ID  ComFabricClientConnection::get_ClientId()
{
    return impl_->Get_ClientId().c_str();
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Transport;

ComFabricServiceCommunicationClient::ComFabricServiceCommunicationClient(
    IServiceCommunicationClientPtr const & impl)
    :
    ComUnknownBase(),
    impl_(impl)
{
}

ComFabricServiceCommunicationClient::~ComFabricServiceCommunicationClient()
{
    //We need to call Close on Client Object,so that demuxer doesn't holds anymore the ServiceCommnicationClient root. 
    //Demuxer increments  the Client refCount in Open Api and decrements in closeApi.
    impl_->CloseClient();
}

HRESULT ComFabricServiceCommunicationClient::BeginRequest(
    /* [in] */IFabricServiceCommunicationMessage *message,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
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


HRESULT ComFabricServiceCommunicationClient::EndRequest(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ IFabricServiceCommunicationMessage **message)
{
    return ComUtility::OnPublicApiReturn(ComFabricSendRequestAsyncOperation::End(context, message));
}

HRESULT ComFabricServiceCommunicationClient::SendMessage(
    IFabricServiceCommunicationMessage *message)
{
    MessageUPtr msg;
    HRESULT hr;



    hr = ComFabricServiceCommunicationMessage::ToNativeTransportMessage(message, msg);

    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    auto error = impl_->SendOneWay(move(msg));

    return ComUtility::OnPublicApiReturn(error.ToHResult());
}


void ComFabricServiceCommunicationClient::Abort(void)
{
    impl_->AbortClient();
}

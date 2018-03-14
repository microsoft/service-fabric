// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Transport;
using namespace Communication::TcpServiceCommunication;



ComFabricTransportClient::ComFabricTransportClient(
    IServiceCommunicationClientPtr const & impl,
    DeleteCallback const & deleteCallback)
    :ComUnknownBase(),
    impl_(impl),
    deleteCallBack_(deleteCallback)
{    
   
}

ComFabricTransportClient::~ComFabricTransportClient()
{
    //We need to call Close on Client Object,so that demuxer doesn't holds anymore the ServiceCommnicationClient root. 
    //Demuxer increments  the Client refCount in Open Api and decrements in closeApi.
    impl_->CloseClient();
}

HRESULT ComFabricTransportClient::BeginRequest(
    /* [in] */IFabricTransportMessage *message,
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    ComPointer<IUnknown> rootCPtr;
    HRESULT hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr)) { return hr; }

    auto rootSPtr = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    auto senderImpl = RootedObjectPointer<ICommunicationMessageSender>(impl_.get(), impl_.get_Root());

    ComPointer<ComFabricTransportSendRequestAsyncOperation> operation = make_com<ComFabricTransportSendRequestAsyncOperation>(senderImpl, deleteCallBack_);

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


HRESULT ComFabricTransportClient::EndRequest(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [out, retval] */ IFabricTransportMessage **message)
{
    return ComUtility::OnPublicApiReturn(ComFabricTransportSendRequestAsyncOperation::End(context, message));
}


HRESULT ComFabricTransportClient::Send(
    IFabricTransportMessage *message)
{
    MessageUPtr msg;
    HRESULT hr = ComFabricTransportMessage::ToNativeTransportMessage(message, deleteCallBack_, msg);

    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    auto error = impl_->SendOneWay(move(msg));

    return ComUtility::OnPublicApiReturn(error.ToHResult());
}

void ComFabricTransportClient::Abort(void)
{
    impl_->AbortClient();
}

HRESULT ComFabricTransportClient::BeginOpen(
    /* [in] */ DWORD timeoutMilliseconds,
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [out, retval] */ IFabricAsyncOperationContext ** context)
{
    ComPointer<IUnknown> rootCPtr;
    HRESULT hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr)) { return hr; }

    auto rootSPtr = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    auto senderImpl = RootedObjectPointer<IServiceCommunicationClient>(impl_.get(), impl_.get_Root());

    ComPointer<ComFabricTransportOpenAsyncOperation> operation = make_com<ComFabricTransportOpenAsyncOperation>(senderImpl);

    hr = operation->Initialize(
        timeoutMilliseconds,
        callback,
        rootSPtr);

    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricTransportClient::EndOpen(
    /* [in] */   IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(ComFabricTransportOpenAsyncOperation::End(context));
}

HRESULT ComFabricTransportClient::BeginClose(
    /* [in] */  DWORD timeoutMilliseconds,
    /* [in] */  IFabricAsyncOperationCallback * callback,
    /* [out, retval] */  IFabricAsyncOperationContext ** context)
{
    ComPointer<IUnknown> rootCPtr;
    HRESULT hr = this->QueryInterface(IID_IUnknown, (void**)rootCPtr.InitializationAddress());
    if (FAILED(hr)) { return hr; }

    auto rootSPtr = make_shared<ComComponentRoot<IUnknown>>(move(rootCPtr));

    auto senderImpl = RootedObjectPointer<IServiceCommunicationClient>(impl_.get(), impl_.get_Root());

    ComPointer<ComFabricTransportCloseAsyncOperation> operation = make_com<ComFabricTransportCloseAsyncOperation>(senderImpl);

    hr = operation->Initialize(
        timeoutMilliseconds,
        callback,
        rootSPtr);

    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    return ComUtility::OnPublicApiReturn(ComAsyncOperationContext::StartAndDetach(move(operation), context));
}

HRESULT ComFabricTransportClient::EndClose(
    /* [in] */  IFabricAsyncOperationContext * context)
{
    return ComUtility::OnPublicApiReturn(ComFabricTransportCloseAsyncOperation::End(context));

}

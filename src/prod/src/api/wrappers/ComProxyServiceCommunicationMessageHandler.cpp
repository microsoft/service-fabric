// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Transport;

class ComProxyServiceCommunicationMessageHandler::ProcessRequestAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(ProcessRequestAsyncOperation)

public:
    ProcessRequestAsyncOperation(
        __in IFabricCommunicationMessageHandler & comImpl,
        wstring const & clientId,
        MessageUPtr && request,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , clientId_(clientId)
        , request_(move(request))
        , timeout_(timeout)
    {
    }

    virtual ~ProcessRequestAsyncOperation() {}

    static ErrorCode End(AsyncOperationSPtr const & operation, MessageUPtr & reply)
    {
        auto thisPtr = AsyncOperation::End<ProcessRequestAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            reply = move(thisPtr->reply_);
        }
        return thisPtr->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        ComPointer<IFabricServiceCommunicationMessage> msgCPointer = make_com<ComFabricServiceCommunicationMessage,
            IFabricServiceCommunicationMessage>(request_);

        return comImpl_.BeginProcessRequest(
            clientId_.c_str(),
            msgCPointer.GetRawPointer(),
            GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        ComPointer<IFabricServiceCommunicationMessage> replyPointerCptr;
        auto hr = comImpl_.EndProcessRequest(context, replyPointerCptr.InitializationAddress());
        if (SUCCEEDED(hr))
        {
            hr = ComFabricServiceCommunicationMessage::ToNativeTransportMessage(
                replyPointerCptr.GetRawPointer(),
                reply_);
        }
        return hr;
    }


    IFabricCommunicationMessageHandler & comImpl_;
    MessageUPtr request_;
    MessageUPtr reply_;
    wstring clientId_;
    TimeSpan const timeout_;
};

// ********************************************************************************************************************
// ComProxyServiceCommunicationMessageHandler Implementation
//
ComProxyServiceCommunicationMessageHandler::ComProxyServiceCommunicationMessageHandler(
    ComPointer<IFabricCommunicationMessageHandler> const & comImpl)
    : IServiceCommunicationMessageHandler()
    , comImpl_(comImpl)
{
}


ComProxyServiceCommunicationMessageHandler::~ComProxyServiceCommunicationMessageHandler()
{
}

AsyncOperationSPtr ComProxyServiceCommunicationMessageHandler::BeginProcessRequest(
    wstring const & clientId,
    MessageUPtr && message,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<ProcessRequestAsyncOperation>(
        *comImpl_.GetRawPointer(),
        clientId,
        move(message),
        timeout,
        callback,
        parent);
    return operation;

}

ErrorCode ComProxyServiceCommunicationMessageHandler::EndProcessRequest(
    AsyncOperationSPtr const &  operation,
    MessageUPtr & reply)
{
    return ProcessRequestAsyncOperation::End(operation, reply);
}

ErrorCode ComProxyServiceCommunicationMessageHandler::HandleOneWay(
    wstring const & clientId,
    MessageUPtr && message)
{
    ComPointer<IFabricServiceCommunicationMessage> msgCPointer = make_com<ComFabricServiceCommunicationMessage,
        IFabricServiceCommunicationMessage>(message);

    auto hr = comImpl_->HandleOneWay(
        clientId.c_str(),
        msgCPointer.GetRawPointer());

    return ErrorCode::FromHResult(hr);
}

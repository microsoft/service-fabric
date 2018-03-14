// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Transport;

class ComProxyFabricTransportMessageHandler::ProcessRequestAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(ProcessRequestAsyncOperation)

public:
    ProcessRequestAsyncOperation(
        __in IFabricTransportMessageHandler & comImpl,
        wstring const & clientId,
        MessageUPtr && request,
        TimeSpan const & timeout,
        DeleteCallback const & deleteCallback,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , clientId_(clientId)
        , request_(move(request))
        , timeout_(timeout)
        ,deleteCallback_(deleteCallback)
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
        ComPointer<IFabricTransportMessage> msgCPointer = make_com<ComFabricTransportMessage,
            IFabricTransportMessage>(move(request_));

        return comImpl_.BeginProcessRequest(
            clientId_.c_str(),
            msgCPointer.GetRawPointer(),
            GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        ComPointer<IFabricTransportMessage> replyPointerCptr;
        auto hr = comImpl_.EndProcessRequest(context, replyPointerCptr.InitializationAddress());
        if (SUCCEEDED(hr))
        {
            hr = ComFabricTransportMessage::ToNativeTransportMessage(
                replyPointerCptr.GetRawPointer(),
                deleteCallback_,
                reply_
                );              
            
        }
        return hr;
    }


    IFabricTransportMessageHandler & comImpl_;
    MessageUPtr request_;
    MessageUPtr reply_;
    wstring clientId_;
    TimeSpan const timeout_;
    DeleteCallback deleteCallback_;
};
// ********************************************************************************************************************
// ComProxyFabricTransportMessageHandler Implementation
//
ComProxyFabricTransportMessageHandler::ComProxyFabricTransportMessageHandler(
    ComPointer<IFabricTransportMessageHandler> const & comImpl,
    ComPointer<IFabricTransportMessageDisposer> const & comdisposerImpl)
    : IServiceCommunicationMessageHandler()
    , comImpl_(comImpl)
    ,comDisposerImpl_(comdisposerImpl)
{   
}

void ComProxyFabricTransportMessageHandler::Initialize() {

    this->disposeQueue_ = make_unique<BatchJobQueue<ComFabricTransportMessageCPtr,
        ComponentRoot>>(
        L"DisposeQueue",
        [this](vector<ComFabricTransportMessageCPtr> & items , ComponentRoot & root) {
        ComProxyFabricTransportDisposer disposer(this->comDisposerImpl_);
        disposer.ProcessJob(items, root);
        },
        *this,
        false,
        2);

}

ComProxyFabricTransportMessageHandler::~ComProxyFabricTransportMessageHandler()
{
}

AsyncOperationSPtr ComProxyFabricTransportMessageHandler::BeginProcessRequest(
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
        [this](ComPointer<IFabricTransportMessage> message) 
        {
            if (!this->disposeQueue_->Enqueue(move(message))) 
            {//if Enqueue Fails(e.g Queue Full), then Dispose will be called on the same threads
                message->Dispose();
            }
         },
        callback,
        parent);
    return operation;
}

ErrorCode ComProxyFabricTransportMessageHandler::EndProcessRequest(
    AsyncOperationSPtr const &  operation,
    MessageUPtr & reply)
{
    return ProcessRequestAsyncOperation::End(operation, reply);

}

ErrorCode ComProxyFabricTransportMessageHandler::HandleOneWay(
    wstring const & clientId,
    MessageUPtr && message)
{
    ComPointer<IFabricTransportMessage> msgCPointer = make_com<ComFabricTransportMessage,
        IFabricTransportMessage>(move(message));
        
    auto hr = comImpl_->HandleOneWay(
        clientId.c_str(),
        msgCPointer.GetRawPointer());

    return ErrorCode::FromHResult(hr);

}

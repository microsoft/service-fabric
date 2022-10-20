// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace HttpServer;
using namespace HttpClient;
using namespace HttpCommon;
using namespace HttpApplicationGateway;

StringLiteral const TraceType("WebSocketIoForwarder");

class WebSocketIoForwarder::ForwardIoAsyncOperation : public AsyncOperation
{
public:
    ForwardIoAsyncOperation(
        WebSocketIoForwarder &owner,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , statusCode_(KWebSocket::CloseStatusCode::GoingAway)
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const &operation, 
        __out KWebSocket::CloseStatusCode &statusCode,
        __out KSharedBufferStringA::SPtr &statusReason)
    {
        auto thisPtr = AsyncOperation::End<ForwardIoAsyncOperation>(operation);
        statusCode = thisPtr->statusCode_;
        statusReason = thisPtr->statusReason_;
        return thisPtr->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const &thisSPtr)
    {
        StartReceive(thisSPtr, owner_.fragmentBuffer_);
    }

private:
    void StartReceive(AsyncOperationSPtr const &thisSPtr, KBuffer::SPtr &buffer);
    void OnReceiveComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);
    void StartSend(
        AsyncOperationSPtr const &thisSPtr, 
        KBuffer::SPtr &buffer,
        ULONG bytesReceived,
        bool isLastFragment,
        KWebSocket::MessageContentType contentType);
    void OnSendComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);

    KWebSocket::CloseStatusCode statusCode_;
    KSharedBufferStringA::SPtr statusReason_;
    WebSocketIoForwarder &owner_;
};

void WebSocketIoForwarder::ForwardIoAsyncOperation::StartReceive(
    AsyncOperationSPtr const &thisSPtr, 
    KBuffer::SPtr &buffer)
{
    if (thisSPtr->IsCancelRequested)
    {
        TryComplete(thisSPtr, ErrorCodeValue::OperationCanceled);
        return;
    }

    auto operation = owner_.readChannelSPtr_->BeginReceiveFragment(
        buffer,
        [this](AsyncOperationSPtr const &operation)
    {
        OnReceiveComplete(operation, false);
    },
        thisSPtr);

    OnReceiveComplete(operation, true);
}

void WebSocketIoForwarder::ForwardIoAsyncOperation::OnReceiveComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    KBuffer::SPtr buffer;
    ULONG bytesReceived;
    bool isLastFragment;
    KWebSocket::MessageContentType contentType;
    auto error = owner_.readChannelSPtr_->EndReceiveFragment(
        operation,
        buffer,
        &bytesReceived,
        isLastFragment,
        contentType);
    
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType,
            "{0} Receivefragment operation failed with {1}, ConnectionStatus {2}",
            owner_.traceId_,
            error,
            (SHORT)owner_.readChannelSPtr_->GetConnectionStatus());

        owner_.readChannelSPtr_->GetRemoteCloseStatus(statusCode_, statusReason_);
        TryComplete(operation->Parent, error);
        return;
    }

    StartSend(operation->Parent, buffer, bytesReceived, isLastFragment, contentType);
}

void WebSocketIoForwarder::ForwardIoAsyncOperation::StartSend(
    AsyncOperationSPtr const &thisSPtr,
    KBuffer::SPtr &buffer,
    ULONG bytesToSend,
    bool isLastFragment,
    KWebSocket::MessageContentType contentType)
{
    if (thisSPtr->IsCancelRequested)
    {
        TryComplete(thisSPtr, ErrorCodeValue::OperationCanceled);
        return;
    }

    auto operation = owner_.writeChannelSPtr_->BeginSendFragment(
        buffer,
        bytesToSend,
        isLastFragment,
        contentType,
        [this](AsyncOperationSPtr const &operation)
        {
            OnSendComplete(operation, false);
        },
        thisSPtr);

    OnSendComplete(operation, true);
}

void WebSocketIoForwarder::ForwardIoAsyncOperation::OnSendComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = owner_.writeChannelSPtr_->EndSendFragment(operation);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType,
            "{0} SendFragment operation failed with {1}, ConnectionStatus {2}",
            owner_.traceId_,
            error,
            (SHORT)owner_.writeChannelSPtr_->GetConnectionStatus());

        owner_.writeChannelSPtr_->GetRemoteCloseStatus(statusCode_, statusReason_);

        TryComplete(operation->Parent, error);
        return;
    }

    StartReceive(operation->Parent, owner_.fragmentBuffer_);
}

void WebSocketIoForwarder::InitializeBuffers()
{
    auto status = KBuffer::Create(
        fragmentBufferSize_, 
        fragmentBuffer_, 
        GetSFDefaultPagedKAllocator());

    ASSERT_IF(!NT_SUCCESS(status), "fragment buffer allocation failure");
}

AsyncOperationSPtr WebSocketIoForwarder::BeginForwardingIo(
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<ForwardIoAsyncOperation>(*this, callback, parent);
}

ErrorCode WebSocketIoForwarder::EndForwardingIo(
    AsyncOperationSPtr const &operation,
    __out KWebSocket::CloseStatusCode &statusCode,
    __out KSharedBufferStringA::SPtr &statusReason)
{
    return ForwardIoAsyncOperation::End(operation, statusCode, statusReason);
}

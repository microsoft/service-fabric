// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#if NTDDI_VERSION >= NTDDI_WIN8 

using namespace std;
using namespace Common;
using namespace HttpCommon;

class WebSocketHandler::ReceiveFragmentAsyncOperation : public AsyncOperation
{
public:
    ReceiveFragmentAsyncOperation(
        WebSocketHandler &owner,
        KBuffer::SPtr &allocatedBuffer,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , allocatedBuffer_(allocatedBuffer)
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const& operation,
        __out KBuffer::SPtr &buffer,
        __out ULONG *bytesReceived,
        __out bool &isFinalFragment,
        __out KWebSocket::MessageContentType &contentType)
    {
        auto thisPtr = AsyncOperation::End<ReceiveFragmentAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            buffer = thisPtr->allocatedBuffer_;
            *bytesReceived = thisPtr->bytesReceived_;
            isFinalFragment = thisPtr->isFinalFragment_;
            contentType = thisPtr->contentType_;
        }

        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const &thisSPtr);

private:
    VOID OnReceiveComplete(
        KAsyncContextBase* const,
        KAsyncContextBase &subOp)
    {
        auto status = subOp.Status();
        if (NT_SUCCESS(status))
        {
            isFinalFragment_ = owner_.receiveFragmentKtlOperation_->IsFinalFragment() ? true : false;
            contentType_ = owner_.receiveFragmentKtlOperation_->GetContentType();
            bytesReceived_ = owner_.receiveFragmentKtlOperation_->GetBytesReceived();
        }
        else
        {
            WriteWarning(
                owner_.traceType_,
                "Receive fragment operation for request url {0} failed with status {1}",
                owner_.requestUri_,
                status);
        }

        TryComplete(thisSPtr_, ErrorCode::FromNtStatus(status));
        thisSPtr_.reset();
    }

    WebSocketHandler &owner_;
    AsyncOperationSPtr thisSPtr_;
    bool isFinalFragment_;
    ULONG bytesReceived_;
    KWebSocket::MessageContentType contentType_;
    KBuffer::SPtr allocatedBuffer_;
};

void WebSocketHandler::ReceiveFragmentAsyncOperation::OnStart(AsyncOperationSPtr const &thisSPtr)
{
    ASSERT_IF(owner_.receiveFragmentKtlOperation_ == nullptr, "Receive called before Open");

    owner_.receiveFragmentKtlOperation_->Reuse();
    thisSPtr_ = thisSPtr;

    owner_.receiveFragmentKtlOperation_->StartReceiveFragment(
        *allocatedBuffer_,
        nullptr,
        KAsyncServiceBase::CompletionCallback(this, &WebSocketHandler::ReceiveFragmentAsyncOperation::OnReceiveComplete),
        0, // offset
        allocatedBuffer_->QuerySize()); // the entire buffer can be used
}

class WebSocketHandler::SendFragmentAsyncOperation : public AsyncOperation
{
public:
    SendFragmentAsyncOperation(
        WebSocketHandler &owner,
        KBuffer::SPtr &buffer,
        ULONG bytesToSend,
        bool isFinalFragment,
        KWebSocket::MessageContentType &contentType,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , buffer_(buffer)
        , bytesToSend_(bytesToSend)
        , isFinalFragment_(isFinalFragment)
        , contentType_(contentType)
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const& operation)
    {
        auto thisPtr = AsyncOperation::End<SendFragmentAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const &thisSPtr);

private:
    VOID OnSendComplete(
        KAsyncContextBase* const,
        KAsyncContextBase &subOp)
    {
        auto status = subOp.Status();
        if (!NT_SUCCESS(status))
        {
            WriteWarning(
                owner_.traceType_,
                "Send fragment operation for request url {0} failed with status {1}",
                owner_.requestUri_,
                status);
        }

        TryComplete(thisSPtr_, ErrorCode::FromNtStatus(status));
        thisSPtr_.reset();
    }

    WebSocketHandler &owner_;
    AsyncOperationSPtr thisSPtr_;
    ULONG bytesToSend_;
    bool isFinalFragment_;
    KWebSocket::MessageContentType contentType_;
    KBuffer::SPtr buffer_;
};

void WebSocketHandler::SendFragmentAsyncOperation::OnStart(AsyncOperationSPtr const &thisSPtr)
{
    ASSERT_IF(owner_.sendFragmentKtlOperation_ == nullptr, "Send called before Open");

    owner_.sendFragmentKtlOperation_->Reuse();
    thisSPtr_ = thisSPtr;

    owner_.sendFragmentKtlOperation_->StartSendFragment(
        *buffer_,
        isFinalFragment_,
        contentType_,
        nullptr,
        KAsyncServiceBase::CompletionCallback(this, &WebSocketHandler::SendFragmentAsyncOperation::OnSendComplete),
        0,
        bytesToSend_);
}

AsyncOperationSPtr WebSocketHandler::BeginReceiveFragment(
    __in KBuffer::SPtr &allocatedBuffer,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<ReceiveFragmentAsyncOperation>(*this, allocatedBuffer, callback, parent);
}

ErrorCode WebSocketHandler::EndReceiveFragment(
    AsyncOperationSPtr const &operation,
    __out KBuffer::SPtr &buffer,
    __out ULONG *bytesReceived,
    __out bool &isFinalFragment,
    __out KWebSocket::MessageContentType &contentType)
{
    return ReceiveFragmentAsyncOperation::End(
        operation, 
        buffer, 
        bytesReceived, 
        isFinalFragment, 
        contentType);
}

AsyncOperationSPtr WebSocketHandler::BeginSendFragment(
    __in KBuffer::SPtr &buffer,
    __in ULONG bytesToSend,
    __in bool isFinalFragment,
    __in KWebSocket::MessageContentType &contentType,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<SendFragmentAsyncOperation>(
        *this,
        buffer,
        bytesToSend,
        isFinalFragment,
        contentType,
        callback,
        parent);
}

ErrorCode WebSocketHandler::EndSendFragment(
    AsyncOperationSPtr const &operation)
{
    return SendFragmentAsyncOperation::End(operation);
}

VOID WebSocketHandler::CloseReceived(__in_opt KWebSocket&)
{
    closeReceivedCallback_();
}

#endif

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#if NTDDI_VERSION >= NTDDI_WIN8 

using namespace std;
using namespace Common;
using namespace HttpCommon;
using namespace HttpServer;

StringLiteral const TraceType("HttpServerWebSocket");

HttpServerWebSocket::HttpServerWebSocket(
    __in IRequestMessageContext &clientRequest,
    __in KStringViewA const &supportedSubProtocols,
    __in_opt ULONG receiveBufferSize,
    __in_opt ULONG sendBufferSize,
    __in_opt ULONGLONG openHandshakeTimeoutMilliSeconds,
    __in_opt ULONGLONG gracefulCloseTimeoutMilliSeconds,
    __in_opt ULONGLONG pongKeepAlivePeriodMilliSeconds,
    __in_opt ULONGLONG pingIntervalMilliSeconds,
    __in_opt ULONGLONG pongTimeoutMilliSeconds)
    : WebSocketHandler(TraceType, clientRequest.GetUrl())
    , clientRequest_(clientRequest)
    , supportedSubProtocols_(supportedSubProtocols)
    , receiveBufferSize_(receiveBufferSize)
    , sendBufferSize_(sendBufferSize)
    , openHandshakeTimeoutMilliSeconds_(openHandshakeTimeoutMilliSeconds)
    , gracefulCloseTimeoutMilliSeconds_(gracefulCloseTimeoutMilliSeconds)
    , pongKeepAlivePeriodMilliSeconds_(pongKeepAlivePeriodMilliSeconds)
    , pingIntervalMilliSeconds_(pingIntervalMilliSeconds)
    , pongTimeoutMilliSeconds_(pongTimeoutMilliSeconds)
{}

class HttpServerWebSocket::OpenAsyncOperation : public AsyncOperation
{
public:
    OpenAsyncOperation(
        HttpServerWebSocket &owner,
        WebSocketCloseReceivedCallback const &closeReceivedCallback,
        Common::AsyncCallback const &callback,
        Common::AsyncOperationSPtr const &parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
    {
        owner.closeReceivedCallback_ = closeReceivedCallback;
    }

    static ErrorCode End(__in AsyncOperationSPtr const& operation)
    {
        auto thisPtr = AsyncOperation::End<OpenAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const &thisSPtr);

private:
    void OnOpenComplete(
        KAsyncContextBase* const ,
        KAsyncServiceBase& ,
        NTSTATUS openStatus)
    {
        if (!NT_SUCCESS(openStatus))
        {
            WriteWarning(
                TraceType,
                "WebSocket open failed with NTSTATUS - {0}, for request url {1}",
                owner_.clientRequest_.GetUrl(),
                openStatus);
        }

        TryComplete(thisSPtr_, ErrorCode::FromNtStatus(openStatus));
        thisSPtr_.reset();
    }

    HttpServerWebSocket &owner_;
    AsyncOperationSPtr thisSPtr_;
};

void HttpServerWebSocket::OpenAsyncOperation::OnStart(AsyncOperationSPtr const &thisSPtr)
{
    auto status = KHttpServerWebSocket::Create(owner_.kWebSocket_, owner_.clientRequest_.GetThisAllocator());
    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceType,
            "KHttpServerWebSocket create failed with NTSTATUS - {0}, for request url {1}",
            owner_.clientRequest_.GetUrl(),
            status);

        TryComplete(thisSPtr, ErrorCode::FromNtStatus(status));
        return;
    }

    status = owner_.kWebSocket_->CreateReceiveFragmentOperation(owner_.receiveFragmentKtlOperation_);
    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceType,
            "create receive fragment operation failed with NTSTATUS - {0}, for request url {1}",
            owner_.clientRequest_.GetUrl(),
            status);

        TryComplete(thisSPtr, ErrorCode::FromNtStatus(status));
        return;
    }

    status = owner_.kWebSocket_->CreateSendFragmentOperation(owner_.sendFragmentKtlOperation_);
    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceType,
            "Create send fragment operation failed with NTSTATUS - {0}, for request url {1}",
            owner_.clientRequest_.GetUrl(),
            status);

        TryComplete(thisSPtr, ErrorCode::FromNtStatus(status));
        return;
    }

    auto error = owner_.SetTimingConstants();
    if (!error.IsSuccess())
    {
        TryComplete(thisSPtr, error);
        return;
    }

    KStringA::SPtr ktlSubProtocolAnsiString;
    status = KStringA::Create(ktlSubProtocolAnsiString, HttpUtil::GetKtlAllocator(), owner_.supportedSubProtocols_, TRUE);
    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceType,
            "Getting subprotocol string failed with NTSTATUS - {0}, for request url {1}",
            owner_.clientRequest_.GetUrl(),
            status);

        TryComplete(thisSPtr, ErrorCode::FromNtStatus(status));
        return;
    }

    thisSPtr_ = shared_from_this();

    status = owner_.kWebSocket_->StartOpenWebSocket(
        *(static_cast<RequestMessageContext&>(owner_.clientRequest_)).GetInnerRequest(),
        KWebSocket::WebSocketCloseReceivedCallback(&owner_, &WebSocketHandler::CloseReceived),
        nullptr,
        KAsyncServiceBase::OpenCompletionCallback(this, &HttpServerWebSocket::OpenAsyncOperation::OnOpenComplete),
        owner_.receiveBufferSize_,
        owner_.sendBufferSize_,
        ktlSubProtocolAnsiString);

    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceType,
            "Starting websocket open failed with NTSTATUS - {0}, for request url {1}",
            owner_.clientRequest_.GetUrl(),
            status);

        thisSPtr_.reset();
        TryComplete(thisSPtr, ErrorCode::FromNtStatus(status));
        return;
    }
}

class HttpServerWebSocket::CloseAsyncOperation : public AsyncOperation
{
public:
    CloseAsyncOperation(
        HttpServerWebSocket &owner,
        __in KWebSocket::CloseStatusCode &statusCode,
        __in KSharedBufferStringA::SPtr &statusReason,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , statusCode_(statusCode)
        , statusReason_(statusReason)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const& operation)
    {
        auto thisPtr = AsyncOperation::End<CloseAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const &thisSPtr);

private:
    VOID
    OnCloseComplete(
        KAsyncContextBase* const,
        KAsyncServiceBase&,
        NTSTATUS closeStatus)
    {
        if (!NT_SUCCESS(closeStatus))
        {
            WriteWarning(
                TraceType,
                "Close complete for request url {0} with status {1}",
                owner_.clientRequest_.GetUrl(),
                closeStatus);
        }

        TryComplete(thisSPtr_, ErrorCode::FromNtStatus(closeStatus));
        thisSPtr_.reset();
    }

    HttpServerWebSocket &owner_;
    AsyncOperationSPtr thisSPtr_;
    KWebSocket::CloseStatusCode statusCode_;
    KSharedBufferStringA::SPtr statusReason_;
};

void HttpServerWebSocket::CloseAsyncOperation::OnStart(AsyncOperationSPtr const &thisSPtr)
{
    thisSPtr_ = thisSPtr;

    if (owner_.kWebSocket_->GetConnectionStatus() == KWebSocket::ConnectionStatus::CloseInitiated ||
        owner_.kWebSocket_->GetConnectionStatus() == KWebSocket::ConnectionStatus::CloseCompleting ||
        owner_.kWebSocket_->GetConnectionStatus() == KWebSocket::ConnectionStatus::Closed)
    {
        TryComplete(thisSPtr, ErrorCodeValue::ObjectClosed);
        return;
    }

    owner_.kWebSocket_->StartCloseWebSocket(
        nullptr,
        KAsyncServiceBase::CloseCompletionCallback(this, &HttpServerWebSocket::CloseAsyncOperation::OnCloseComplete),
        statusCode_,
        statusReason_);

}

AsyncOperationSPtr HttpServerWebSocket::BeginOpen(
    WebSocketCloseReceivedCallback const &closeReceivedCallback,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(
        *this,
        closeReceivedCallback,
        callback,
        parent);
}

ErrorCode HttpServerWebSocket::EndOpen(
    __in AsyncOperationSPtr const &operation)
{
    return OpenAsyncOperation::End(operation);
}

AsyncOperationSPtr HttpServerWebSocket::BeginClose(
    __in KWebSocket::CloseStatusCode &statusCode,
    __in KSharedBufferStringA::SPtr &statusReason,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(
        *this,
        statusCode,
        statusReason,
        callback,
        parent);
}

ErrorCode HttpServerWebSocket::EndClose(
    AsyncOperationSPtr const &operation)
{
    return CloseAsyncOperation::End(operation);
}

ErrorCode HttpServerWebSocket::GetRemoteCloseStatus(
    __out KWebSocket::CloseStatusCode &statusCode,
    __out KSharedBufferStringA::SPtr &statusReason)
{
    if (kWebSocket_->GetConnectionStatus() == KWebSocket::ConnectionStatus::CloseReceived ||
        kWebSocket_->GetConnectionStatus() == KWebSocket::ConnectionStatus::CloseCompleting ||
        kWebSocket_->GetConnectionStatus() == KWebSocket::ConnectionStatus::Closed)
    {
        statusCode = kWebSocket_->GetRemoteWebSocketCloseStatusCode();
        statusReason = kWebSocket_->GetRemoteCloseReason();

        return ErrorCodeValue::Success;
    }

    return ErrorCodeValue::NotReady;
}

ErrorCode HttpServerWebSocket::SetTimingConstants()
{
    auto status = kWebSocket_->SetTimingConstant(
        KWebSocket::TimingConstant::OpenTimeoutMs,
        static_cast<KWebSocket::TimingConstantValue>(openHandshakeTimeoutMilliSeconds_));

    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceType,
            "Set OpenTimeout failed with NTSTATUS - {0}, for request url {1}",
            status,
            requestUri_);

        return ErrorCode::FromNtStatus(status);
    }

    status = kWebSocket_->SetTimingConstant(
        KWebSocket::TimingConstant::CloseTimeoutMs,
        static_cast<KWebSocket::TimingConstantValue>(gracefulCloseTimeoutMilliSeconds_));

    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceType,
            "Set CloseTimeout failed with NTSTATUS - {0}, for request url {1}",
            status,
            requestUri_);

        return ErrorCode::FromNtStatus(status);
    }

    status = kWebSocket_->SetTimingConstant(
        KWebSocket::TimingConstant::PingQuietChannelPeriodMs,
        static_cast<KWebSocket::TimingConstantValue>(pingIntervalMilliSeconds_));

    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceType,
            "Set PingQuietChannelPeriodMs failed with NTSTATUS - {0}, for request url {1}",
            status,
            requestUri_);

        return ErrorCode::FromNtStatus(status);
    }

    status = kWebSocket_->SetTimingConstant(
        KWebSocket::TimingConstant::PongTimeoutMs,
        static_cast<KWebSocket::TimingConstantValue>(pongTimeoutMilliSeconds_));

    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceType,
            "Set PongTimeoutMs failed with NTSTATUS - {0}, for request url {1}",
            status,
            requestUri_);

        return ErrorCode::FromNtStatus(status);
    }

    status = kWebSocket_->SetTimingConstant(
        KWebSocket::TimingConstant::PongKeepalivePeriodMs,
        static_cast<KWebSocket::TimingConstantValue>(pongKeepAlivePeriodMilliSeconds_));

    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceType,
            "Set PongKeepalivePeriodMs failed with NTSTATUS - {0}, for request url {1}",
            status,
            requestUri_);

        return ErrorCode::FromNtStatus(status);
    }

    return ErrorCodeValue::Success;
}

KWebSocket::ConnectionStatus HttpServerWebSocket::GetConnectionStatus()
{
    return kWebSocket_->GetConnectionStatus();
}

#endif

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#if NTDDI_VERSION >= NTDDI_WIN8 

using namespace std;
using namespace Common;
using namespace HttpCommon;
using namespace HttpClient;

StringLiteral const TraceType("HttpClientWebSocket");

HttpClientWebSocket::HttpClientWebSocket(
    __in std::wstring const &requestUri,
    __in std::wstring const &supportedSubProtocols,
    __in KAllocator &ktlAllocator,
    __in_opt ULONG receiveBufferSize,
    __in_opt ULONG sendBufferSize,
    __in_opt ULONGLONG openHandshakeTimeoutMilliSeconds,
    __in_opt ULONGLONG gracefulCloseTimeoutMilliSeconds,
    __in_opt KString::SPtr headers,
    __in_opt PCCERT_CONTEXT pCertContext,
    __in_opt KHttpClientWebSocket::ServerCertValidationHandler handler,
    __in_opt bool allowRedirects,
    __in_opt bool enableCookies)
    : WebSocketHandler(TraceType, requestUri)
    , ktlAllocator_(ktlAllocator)
    , supportedSubProtocols_(supportedSubProtocols)
    , receiveBufferSize_(receiveBufferSize)
    , sendBufferSize_(sendBufferSize)
    , openHandshakeTimeoutMilliSeconds_(openHandshakeTimeoutMilliSeconds)
    , gracefulCloseTimeoutMilliSeconds_(gracefulCloseTimeoutMilliSeconds)
    , headers_(headers)
    , pCertContext_(pCertContext)
    , serverCertValidationHandler_(handler)
    , allowRedirects_(allowRedirects)
    , enableCookies_(enableCookies)
{
}

class HttpClientWebSocket::OpenAsyncOperation : public AsyncOperation
{
public:
    OpenAsyncOperation(
        HttpClientWebSocket &owner,
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
        KAsyncContextBase* const,
        KAsyncServiceBase&,
        NTSTATUS openStatus)
    {
        if (!NT_SUCCESS(openStatus))
        {
            WriteWarning(
                TraceType,
                "WebSocket open failed for request url - {0}, NTSTATUS - {1}, winHttpError {2}",
                owner_.requestUri_,
                openStatus,
                owner_.kWebSocket_->GetWinhttpError());
        }

        TryComplete(thisSPtr_, ErrorCode::FromNtStatus(openStatus));
        thisSPtr_.reset();
    }

    HttpClientWebSocket &owner_;
    AsyncOperationSPtr thisSPtr_;
};

void HttpClientWebSocket::OpenAsyncOperation::OnStart(AsyncOperationSPtr const &thisSPtr)
{
    auto status = KHttpClientWebSocket::Create(owner_.kWebSocket_, owner_.ktlAllocator_);
    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceType,
            "KHttpClientWebSocket create failed with NTSTATUS - {0}, for request url {1}",
            status,
            owner_.requestUri_);

        TryComplete(thisSPtr, ErrorCode::FromNtStatus(status));
        return;
    }

    status = owner_.kWebSocket_->CreateReceiveFragmentOperation(owner_.receiveFragmentKtlOperation_);
    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceType,
            "create receive fragment operation failed with NTSTATUS - {0}, for request url {1}",
            owner_.requestUri_,
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
            owner_.requestUri_,
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

    string subProtocolsAnsi;
    StringUtility::UnicodeToAnsi(owner_.supportedSubProtocols_, subProtocolsAnsi);
    KStringA::SPtr ktlSubProtocolAnsiString;
    status = KStringA::Create(ktlSubProtocolAnsiString, HttpUtil::GetKtlAllocator(), subProtocolsAnsi.c_str(), TRUE);
    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceType,
            "Getting subprotocol string failed with NTSTATUS - {0}, for request url {1}",
            owner_.requestUri_,
            status);

        TryComplete(thisSPtr, ErrorCode::FromNtStatus(status));
        return;
    }

    KUri::SPtr uri;
    KStringView uriStr(owner_.requestUri_.c_str());
    status = KUri::Create(uriStr, HttpUtil::GetKtlAllocator(), uri);
    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceType,
            "Parsing uri {0} failed with {1}",
            owner_.requestUri_,
            status);

        TryComplete(thisSPtr, ErrorCode::FromNtStatus(status));
        return;
    }

    thisSPtr_ = shared_from_this();

    status = owner_.kWebSocket_->StartOpenWebSocket(
        *uri,
        KWebSocket::WebSocketCloseReceivedCallback(&owner_, &WebSocketHandler::CloseReceived),
        nullptr,
        KAsyncServiceBase::OpenCompletionCallback(this, &HttpClientWebSocket::OpenAsyncOperation::OnOpenComplete),
        owner_.receiveBufferSize_,
        ktlSubProtocolAnsiString,
        nullptr, 
        nullptr, // no additional headers
        owner_.headers_, // header blob
        owner_.pCertContext_,
        owner_.serverCertValidationHandler_,
        owner_.allowRedirects_ ? TRUE : FALSE,
        owner_.enableCookies_ ? TRUE : FALSE);

    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceType,
            "Starting websocket open failed with NTSTATUS - {0}, for request url {1}",
            owner_.requestUri_,
            status);

        thisSPtr_.reset();
        TryComplete(thisSPtr, ErrorCode::FromNtStatus(status));
        return;
    }
}

class HttpClientWebSocket::CloseAsyncOperation : public AsyncOperation
{
public:
    CloseAsyncOperation(
        HttpClientWebSocket &owner,
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
                "Close complete for request url {0} with status {1}, winHttpError {2}",
                owner_.requestUri_,
                closeStatus,
                owner_.kWebSocket_->GetWinhttpError());
        }

        TryComplete(thisSPtr_, ErrorCode::FromNtStatus(closeStatus));
        thisSPtr_.reset();
    }

    HttpClientWebSocket &owner_;
    AsyncOperationSPtr thisSPtr_;
    KWebSocket::CloseStatusCode statusCode_;
    KSharedBufferStringA::SPtr statusReason_;
};

void HttpClientWebSocket::CloseAsyncOperation::OnStart(AsyncOperationSPtr const &thisSPtr)
{
    thisSPtr_ = thisSPtr;

    auto status = owner_.kWebSocket_->StartCloseWebSocket(
        nullptr,
        KAsyncServiceBase::CloseCompletionCallback(this, &HttpClientWebSocket::CloseAsyncOperation::OnCloseComplete),
        statusCode_,
        statusReason_);

    if (!NT_SUCCESS(status))
    {
        WriteWarning(
            TraceType,
            "Closing websocket failed with NTSTATUS - {0}, for request url {1}",
            status,
            owner_.requestUri_);

        thisSPtr_.reset();

        TryComplete(thisSPtr, ErrorCode::FromNtStatus(status));
        return;
    }
}

AsyncOperationSPtr HttpClientWebSocket::BeginOpen(
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

ErrorCode HttpClientWebSocket::EndOpen(
    __in AsyncOperationSPtr const &operation)
{
    return OpenAsyncOperation::End(operation);
}

AsyncOperationSPtr HttpClientWebSocket::BeginClose(
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

ErrorCode HttpClientWebSocket::EndClose(
    AsyncOperationSPtr const &operation)
{
    return CloseAsyncOperation::End(operation);
}

ErrorCode HttpClientWebSocket::GetActiveSubprotocol(__out KStringViewA &activeSubprotocol)
{
    if (kWebSocket_->GetConnectionStatus() != KWebSocket::ConnectionStatus::Open)
    {
        return ErrorCodeValue::NotReady;
    }

    activeSubprotocol = kWebSocket_->GetActiveSubProtocol();
    return ErrorCodeValue::Success;
}

ErrorCode HttpClientWebSocket::GetRemoteCloseStatus(
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

ErrorCode HttpClientWebSocket::SetTimingConstants()
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

    return ErrorCodeValue::Success;
}

KWebSocket::ConnectionStatus HttpClientWebSocket::GetConnectionStatus()
{
    return kWebSocket_->GetConnectionStatus();
}

ULONG HttpClientWebSocket::GetWinHttpError()
{
    return kWebSocket_->GetWinhttpError();
}

ULONG HttpClientWebSocket::GetHttpResponseCode()
{
    return kWebSocket_->GetHttpResponseCode();
}

ErrorCode HttpClientWebSocket::GetResponseHeader(
    __in std::wstring const& headerName,
    __out std::wstring &headerValue)
{
    KString::SPtr value;

    auto status = kWebSocket_->GetHttpResponseHeader(headerName.c_str(), value);
    if (!NT_SUCCESS(status))
    {
        return ErrorCode::FromNtStatus(status);
    }

    headerValue = *value;
    return ErrorCodeValue::Success;
}

#endif

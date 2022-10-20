// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace HttpCommon;
using namespace HttpClient;

StringLiteral const TraceType("HttpClientRequest");

class ClientRequest::SendRequestHeaderAsyncOperation : public AllocableAsyncOperation
{
public:
    SendRequestHeaderAsyncOperation(
        ULONG totalRequestLength,
        ClientRequestSPtr &clientRequest,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
        : AllocableAsyncOperation(callback, parent)
        , clientRequest_(clientRequest)
        , totalRequestLength_(totalRequestLength)
    {
    }

    static Common::ErrorCode End(AsyncOperationSPtr const &asyncOperation, ULONG *winHttpError)
    {
        auto thisPtr = AsyncOperation::End<SendRequestHeaderAsyncOperation>(asyncOperation);

        if (!thisPtr->Error.IsSuccess())
        {
            *winHttpError = thisPtr->clientRequest_->winHttpError_;
        }

        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const &thisSPtr);

private:

    void OnRequestComplete(
        KAsyncContextBase* const,
        KAsyncContextBase &operation)
    {
        auto status = operation.Status();
        if (!NT_SUCCESS(status))
        {
            clientRequest_->winHttpError_ = clientRequest_->kHttpRequest_->GetWinHttpError();
        }

        TryComplete(thisSPtr_, ErrorCode::FromNtStatus(status));
        thisSPtr_.reset();
    }

    ClientRequestSPtr clientRequest_;
    Common::AsyncOperationSPtr thisSPtr_;
    ULONG totalRequestLength_;
};

void ClientRequest::SendRequestHeaderAsyncOperation::OnStart(AsyncOperationSPtr const &)
{
    //
    // KHttpClientRequest interface hides the actual async. The callback we register is not the
    // actual async completion callback, but we are safe to release our self reference in the callback.
    //
    thisSPtr_ = shared_from_this();

    auto requestValue = KStringView(clientRequest_->requestVerb_.c_str());
    clientRequest_->kHttpRequest_->SetTarget(clientRequest_->kEndpoint_, requestValue);

    clientRequest_->asyncRequestDataContext_->Reuse();
    clientRequest_->asyncRequestDataContext_->StartSendRequestHeader(
        *clientRequest_->kHttpRequest_,
        totalRequestLength_, // content length if known
        clientRequest_->kHttpResponse_,
        nullptr, // parent
        NULL, // requestcallback
        nullptr,
        KAsyncContextBase::CompletionCallback(this, &ClientRequest::SendRequestHeaderAsyncOperation::OnRequestComplete));
}

class ClientRequest::SendRequestChunkAsyncOperation : public AllocableAsyncOperation
{
public:
    SendRequestChunkAsyncOperation(
        KMemRef &memRef,
        bool isLastSegment,
        bool disconnect,
        ClientRequestSPtr &clientRequest,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
        : AllocableAsyncOperation(callback, parent)
        , clientRequest_(clientRequest)
        , memRef_(memRef)
        , isLastSegment_(isLastSegment)
        , disconnect_(disconnect)
    {
    }

    static Common::ErrorCode End(AsyncOperationSPtr const &asyncOperation, ULONG *winHttpError)
    {
        auto thisPtr = AsyncOperation::End<SendRequestChunkAsyncOperation>(asyncOperation);

        if (!thisPtr->Error.IsSuccess())
        {
            *winHttpError = thisPtr->clientRequest_->winHttpError_;
        }

        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const &thisSPtr) override
    {
        if (!memRef_._Address && !isLastSegment_ && !disconnect_)
        {
            WriteWarning(
                TraceType,
                "SendRequestChunk called without body or EOF during - {0} Uri processing",
                clientRequest_->requestUri_);

            TryComplete(thisSPtr, ErrorCodeValue::InvalidArgument);
            return;
        }

        //
        // KHttpClientRequest interface hides the actual async. The callback we register is not the
        // actual async completion callback, but we are safe to release our self reference in the callback.
        //
        thisSPtr_ = shared_from_this();

        if (disconnect_)
        {
            Disconnect(thisSPtr);
        }
        else if (memRef_._Address)
        {
            SendRequestChunk(thisSPtr);
        }
        else
        {
            FinishSendRequest(thisSPtr);
        }
    }

private:
    //
    // These functions assume that this async operation's refcount is already been tracked by thisSPtr_.
    // TODO: need to come up with a better implementation(base class) to do this transparently
    //
    void SendRequestChunk(AsyncOperationSPtr const &thisSPtr);
    void FinishSendRequest(AsyncOperationSPtr const &thisSPtr);
    void Disconnect(AsyncOperationSPtr const &thisSPtr);

    void OnSendRequestChunkComplete(
        KAsyncContextBase* const,
        KAsyncContextBase &operation)
    {
        auto status = operation.Status();
        if (!NT_SUCCESS(status))
        {
            clientRequest_->winHttpError_ = clientRequest_->kHttpRequest_->GetWinHttpError();
            TryComplete(thisSPtr_, ErrorCode::FromNtStatus(status));
            thisSPtr_.reset();
            return;
        }

        if (isLastSegment_)
        {
            //
            // Complete the send
            //
            FinishSendRequest(thisSPtr_);
        }
        else
        {
            //
            // There is still more data to send.
            //
            TryComplete(thisSPtr_, ErrorCode::FromNtStatus(status));
            thisSPtr_.reset();
        }

    }

    void OnRequestComplete(
        KAsyncContextBase* const,
        KAsyncContextBase &operation)
    {
        auto status = operation.Status();
        if (!NT_SUCCESS(status))
        {
            clientRequest_->winHttpError_ = clientRequest_->kHttpRequest_->GetWinHttpError();
        }

        TryComplete(thisSPtr_, ErrorCode::FromNtStatus(status));
        thisSPtr_.reset();
    }

    ClientRequestSPtr clientRequest_;
    KMemRef memRef_;
    bool isLastSegment_;
    bool disconnect_;
    AsyncOperationSPtr thisSPtr_;
};

void ClientRequest::SendRequestChunkAsyncOperation::SendRequestChunk(AsyncOperationSPtr const &)
{
    clientRequest_->asyncRequestDataContext_->Reuse();
    clientRequest_->asyncRequestDataContext_->StartSendRequestDataContent(
        *clientRequest_->kHttpRequest_,
        memRef_,
        nullptr,
        KAsyncContextBase::CompletionCallback(this, &ClientRequest::SendRequestChunkAsyncOperation::OnSendRequestChunkComplete));
}

void ClientRequest::SendRequestChunkAsyncOperation::FinishSendRequest(AsyncOperationSPtr const &)
{
    clientRequest_->asyncRequestDataContext_->Reuse();
    clientRequest_->asyncRequestDataContext_->StartEndRequest(
        *clientRequest_->kHttpRequest_,
        nullptr,
        KAsyncContextBase::CompletionCallback(this, &ClientRequest::SendRequestChunkAsyncOperation::OnRequestComplete));
}

void ClientRequest::SendRequestChunkAsyncOperation::Disconnect(AsyncOperationSPtr const &)
{
    clientRequest_->asyncRequestDataContext_->Reuse();
    clientRequest_->asyncRequestDataContext_->StartDisconnect(
        *clientRequest_->kHttpRequest_,
        STATUS_SUCCESS,
        nullptr,
        KAsyncContextBase::CompletionCallback(this, &ClientRequest::SendRequestChunkAsyncOperation::OnRequestComplete));
}

class ClientRequest::GetResponseBodyChunkAsyncOperation : public AllocableAsyncOperation
{
public:
    GetResponseBodyChunkAsyncOperation(
        KMemRef &memRef,
        ClientRequestSPtr &clientRequest,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
        : AllocableAsyncOperation(callback, parent)
        , clientRequest_(clientRequest)
        , memRef_(memRef)
    {
    }

    static Common::ErrorCode End(AsyncOperationSPtr const &asyncOperation, KMemRef &memRef, ULONG *winHttpError)
    {
        auto thisPtr = AsyncOperation::End<GetResponseBodyChunkAsyncOperation>(asyncOperation);

        if (!thisPtr->Error.IsSuccess())
        {
            *winHttpError = thisPtr->clientRequest_->winHttpError_;
        }
        else
        {
            memRef = move(thisPtr->memRef_);
        }

        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const &thisSPtr) override;

private:

    void OnRequestComplete(
        KAsyncContextBase* const,
        KAsyncContextBase &operation)
    {
        auto status = operation.Status();
        if (!NT_SUCCESS(status))
        {
            clientRequest_->winHttpError_ = clientRequest_->kHttpRequest_->GetWinHttpError();
        }

        TryComplete(thisSPtr_, ErrorCode::FromNtStatus(status));
        thisSPtr_.reset();
    }

    ClientRequestSPtr clientRequest_;
    AsyncOperationSPtr thisSPtr_;
    KMemRef memRef_;
};

void ClientRequest::GetResponseBodyChunkAsyncOperation::OnStart(AsyncOperationSPtr const &)
{
    //
    // KHttpClientRequest interface hides the actual async. The callback we register is not the
    // actual async completion callback, but we are safe to release our self reference in the callback.
    //
    thisSPtr_ = shared_from_this();

    clientRequest_->asyncRequestDataContext_->Reuse();
    clientRequest_->asyncRequestDataContext_->StartReceiveResponseData(
        *clientRequest_->kHttpRequest_,
        &memRef_,
        nullptr,
        KAsyncContextBase::CompletionCallback(this, &ClientRequest::GetResponseBodyChunkAsyncOperation::OnRequestComplete));
}

ErrorCode ClientRequest::CreateClientRequest(
    KHttpClient::SPtr &client,
    wstring const &requestUri,
    KAllocator &ktlAllocator,
    TimeSpan const &connectTimeout,
    TimeSpan const &sendTimeout,
    TimeSpan const &receiveTimeout,
    bool allowRedirects,
    bool enableCookies,
    bool enableWinauthAutoLogon,
    __out IHttpClientRequestSPtr &clientRequestSPtr)
{
    shared_ptr<ClientRequest> temp = shared_ptr<ClientRequest>(new ClientRequest(requestUri));
    auto error = temp->Initialize(
        client,
        ktlAllocator,
        connectTimeout,
        sendTimeout,
        receiveTimeout,
        allowRedirects,
        enableCookies,
        enableWinauthAutoLogon);

    if (!error.IsSuccess())
    {
        return error;
    }

    clientRequestSPtr = move(temp);
    return error;
}

ErrorCode ClientRequest::Initialize(
    KHttpClient::SPtr &client,
    KAllocator &ktlAllocator,
    TimeSpan const &connectTimeout,
    TimeSpan const &sendTimeout,
    TimeSpan const &receiveTimeout,
    bool allowRedirects,
    bool enableCookies,
    bool enableWinauthAutoLogon)
{
    auto result = client->CreateMultiPartRequest(
        kHttpRequest_,
        ktlAllocator,
        0,  // INFINITE time for name resolution - default for WinHttp
        static_cast<int>(connectTimeout.TotalMilliseconds()),
        static_cast<int>(sendTimeout.TotalMilliseconds()),
        static_cast<int>(receiveTimeout.TotalMilliseconds()));

    if (!NT_SUCCESS(result))
    {
        auto error = ErrorCode::FromNtStatus(result);
        WriteWarning(
            TraceType,
            "Creation of client request for uri {0} failed with {1}",
            requestUri_,
            error);

        return error;
    }

    result = KHttpCliRequest::AsyncRequestDataContext::Create(
        asyncRequestDataContext_,
        GetThisAllocator(),
        HttpConstants::AllocationTag);

    if (!NT_SUCCESS(result))
    {
        auto error = ErrorCode::FromNtStatus(result);
        WriteWarning(
            TraceType,
            "Creation of AsyncRequestDataContext for uri {0} failed with {1}",
            requestUri_,
            error);

        return error;
    }

    KUriView uriView(requestUri_.c_str());
    result = KHttpNetworkEndpoint::Create(
        uriView,
        GetThisAllocator(),
        kEndpoint_);

    if (!NT_SUCCESS(result))
    {
        auto error = ErrorCode::FromNtStatus(result);
        WriteWarning(
            TraceType,
            "Creation of kNetworkEndpoint for uri {0} failed with {1}",
            requestUri_,
            error);

        return error;
    }

    result = kHttpRequest_->SetAllowRedirects(allowRedirects ? TRUE : FALSE);
    if (!NT_SUCCESS(result))
    {
        auto error = ErrorCode::FromNtStatus(result);
        WriteWarning(
            TraceType,
            "SetAllowRedirects for uri {0} failed with {1}",
            requestUri_,
            error);

        return error;
    }

    result = kHttpRequest_->SetEnableCookies(enableCookies ? TRUE : FALSE);
    if (!NT_SUCCESS(result))
    {
        auto error = ErrorCode::FromNtStatus(result);
        WriteWarning(
            TraceType,
            "SetEnableCookies for uri {0} failed with {1}",
            requestUri_,
            error);

        return error;
    }

    result = kHttpRequest_->SetAutoLogonForAllRequests(enableWinauthAutoLogon ? TRUE : FALSE);
    if (!NT_SUCCESS(result))
    {
        auto error = ErrorCode::FromNtStatus(result);
        WriteWarning(
            TraceType,
            "SetAutoLogonForAllRequests for uri {0} failed with {1}",
            requestUri_,
            error);

        return error;
    }

    return ErrorCodeValue::Success;
}

KAllocator& ClientRequest::GetThisAllocator() const
{
    return kHttpRequest_->GetThisAllocator();
}

wstring ClientRequest::GetRequestUrl() const
{
    return requestUri_;
}

BOOLEAN ClientRequest::IsSecure() const
{
    return HttpUtil::IsHttpsUri(requestUri_);
}

void ClientRequest::SetVerb(wstring const &verb)
{
    requestVerb_ = verb;
}

void ClientRequest::SetVerb(KHttpUtils::OperationType verb)
{
    KStringView verbStr;
    KHttpUtils::OpToString(verb, verbStr);
    requestVerb_ = verbStr;
}

ErrorCode ClientRequest::SetRequestHeader(
    wstring const& headerName,
    wstring const& headerValue)
{
    KStringView headerNameStr(headerName.c_str());
    KStringView headerValueStr(headerValue.c_str());
    return SetRequestHeader(headerNameStr, headerValueStr);
}

ErrorCode ClientRequest::SetRequestHeader(
    __in KStringView & headerName,
    __in KStringView & headerValue)
{
    auto status = kHttpRequest_->AddHeader(headerName, headerValue);

    if (NT_SUCCESS(status))
    { 
        return ErrorCodeValue::Success; 
    }

    return ErrorCode::FromNtStatus(status);
}

ErrorCode ClientRequest::SetRequestHeaders(wstring const &headers)
{
    // Headers other than the last header should be terminated by CR/LF
    KString::SPtr headersSPtr;
    auto status = KString::Create(
        headersSPtr,
        GetThisAllocator(),
        headers.c_str(),
        TRUE);

    if (!NT_SUCCESS(status))
    {
        return ErrorCode::FromNtStatus(status);
    }

    return SetRequestHeaders(headersSPtr);
}

ErrorCode ClientRequest::SetRequestHeaders(__in KString::SPtr const &headerBlock)
{
    auto status = kHttpRequest_->SetHeaders(headerBlock);

    if (!NT_SUCCESS(status))
    {
        return ErrorCode::FromNtStatus(status);
    }

    return ErrorCodeValue::Success;
}

ErrorCode ClientRequest::SetClientCertificate(PCCERT_CONTEXT pCertContext)
{
    auto status = kHttpRequest_->SetClientCertificate(pCertContext);

    if (!NT_SUCCESS(status))
    {
        return ErrorCode::FromNtStatus(status);
    }

    return ErrorCodeValue::Success;
}

ErrorCode ClientRequest::SetServerCertValidationHandler(KHttpCliRequest::ServerCertValidationHandler handler)
{
    auto status = kHttpRequest_->SetServerCertValidationHandler(handler);

    if (!NT_SUCCESS(status))
    {
        return ErrorCode::FromNtStatus(status);
    }

    return ErrorCodeValue::Success;
}

AsyncOperationSPtr ClientRequest::BeginSendRequest(
    ByteBufferUPtr body,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
{
    auto requestVerbValue = KStringView(requestVerb_.c_str());
    kHttpRequest_->SetTarget(kEndpoint_, requestVerbValue);

    auto clientRequestSPtr = this->shared_from_this();

    return AllocableAsyncOperation::CreateAndStart<SendRequestAsyncOperation>(
        GetThisAllocator(),
        move(body),
        clientRequestSPtr,
        callback,
        parent);
}

AsyncOperationSPtr ClientRequest::BeginSendRequest(
    KBuffer::SPtr &&body,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
{
    // TODO: This logic should be changed to use asyncoperation multiroot
    ClientRequestSPtr thisSPtr = this->shared_from_this();

    return AllocableAsyncOperation::CreateAndStart<SendRequestAsyncOperation>(
        GetThisAllocator(),
        move(body),
        thisSPtr,
        callback,
        parent);
}

ErrorCode ClientRequest::EndSendRequest(AsyncOperationSPtr const& operation, ULONG *winHttpError)
{
    return SendRequestAsyncOperation::End(operation, winHttpError);
}

AsyncOperationSPtr ClientRequest::BeginSendRequestHeaders(
    ULONG totalRequestLength,
    Common::AsyncCallback const& callback,
    Common::AsyncOperationSPtr const& parent)
{
    // TODO: This logic should be changed to use asyncoperation multiroot
    ClientRequestSPtr thisSPtr = this->shared_from_this();

    return AllocableAsyncOperation::CreateAndStart<SendRequestHeaderAsyncOperation>(
        GetThisAllocator(),
        totalRequestLength,
        thisSPtr,
        callback,
        parent);
}

Common::ErrorCode ClientRequest::EndSendRequestHeaders(
    Common::AsyncOperationSPtr const& operation,
    __out ULONG *winHttpError)
{
    return SendRequestHeaderAsyncOperation::End(operation, winHttpError);
}

AsyncOperationSPtr ClientRequest::BeginSendRequestChunk(
    KMemRef &memRef,
    bool isLastSegment,
    bool disconnect,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
{
    // TODO: This logic should be changed to use asyncoperation multiroot
    ClientRequestSPtr thisSPtr = this->shared_from_this();

    return AllocableAsyncOperation::CreateAndStart<SendRequestChunkAsyncOperation>(
        GetThisAllocator(),
        memRef,
        isLastSegment,
        disconnect,
        thisSPtr,
        callback,
        parent);
}

ErrorCode ClientRequest::EndSendRequestChunk(
    AsyncOperationSPtr const& operation,
    __out ULONG *winHttpError)
{
    return SendRequestChunkAsyncOperation::End(operation, winHttpError);
}

ErrorCode ClientRequest::GetResponseStatusCode(
    __out USHORT *statusCode,
    __out wstring &description) const
{
    if (!kHttpResponse_)
    {
        return ErrorCodeValue::InvalidState;
    }

    *statusCode = (USHORT)kHttpResponse_->GetHttpResponseCode();

    KString::SPtr value;

    auto error = kHttpResponse_->GetHttpResponseStatusText(value);
    if (NT_SUCCESS(error))
    {
        description = *value;
    }
    else
    {
        // If fetching the status text fails, return empty description.
        description = L"";
    }

    return ErrorCodeValue::Success;
}

ErrorCode ClientRequest::GetResponseHeader(
    __in std::wstring const& headerName,
    __out std::wstring &headerValue) const
{
    KString::SPtr value;
    
    auto error = GetResponseHeader(headerName, value);
    if (!error.IsSuccess())
    {
        return error;
    }

    headerValue = *value;
    return error;
}

ErrorCode ClientRequest::GetResponseHeader(
    __in wstring const& headerName,
    __out KString::SPtr &headerValue) const
{
    if (!kHttpResponse_)
    {
        return ErrorCodeValue::InvalidState;
    }

    auto status = kHttpResponse_->GetHeader(headerName.c_str(), headerValue);
    if (!NT_SUCCESS(status))
    {
        return ErrorCode::FromNtStatus(status);
    }

    return ErrorCodeValue::Success;
}

ErrorCode ClientRequest::GetAllResponseHeaders(__out std::wstring &headers)
{
    KString::SPtr kHeaders;
    auto error = GetAllResponseHeaders(kHeaders);
    if (!error.IsSuccess())
    {
        return error;
    }

    headers = *kHeaders;
    return error;
}

ErrorCode ClientRequest::GetAllResponseHeaders(__out KString::SPtr &headerValue)
{
    if (!kHttpResponse_)
    {
        return ErrorCodeValue::InvalidState;
    }

    auto status = kHttpResponse_->GetAllHeaders(headerValue);
    if (!NT_SUCCESS(status))
    {
        return ErrorCode::FromNtStatus(status);
    }

    return ErrorCodeValue::Success;
}

AsyncOperationSPtr ClientRequest::BeginGetResponseBody(
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
{
    // TODO: This logic should be changed to use asyncoperation multiroot
    ClientRequestSPtr thisSPtr = this->shared_from_this();

    return AllocableAsyncOperation::CreateAndStart<GetResponseBodyAsyncOperation>(
        GetThisAllocator(),
        thisSPtr,
        callback,
        parent);
}

ErrorCode ClientRequest::EndGetResponseBody(
    AsyncOperationSPtr const& operation,
    __out ByteBufferUPtr &body)
{
    return GetResponseBodyAsyncOperation::End(operation, body);
}

ErrorCode ClientRequest::EndGetResponseBody(
    AsyncOperationSPtr const& operation,
    __out KBuffer::SPtr &body)
{
    return GetResponseBodyAsyncOperation::End(operation, body);
}

AsyncOperationSPtr ClientRequest::BeginGetResponseChunk(
    KMemRef &memRef,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
{
    // TODO: This logic should be changed to use asyncoperation multiroot
    ClientRequestSPtr thisSPtr = this->shared_from_this();

    return AllocableAsyncOperation::CreateAndStart<GetResponseBodyChunkAsyncOperation>(
        GetThisAllocator(),
        memRef,
        thisSPtr,
        callback,
        parent);
}

ErrorCode ClientRequest::EndGetResponseChunk(
    AsyncOperationSPtr const& operation,
    __out KMemRef &memRef,
    __out ULONG *winHttpError)
{
    return GetResponseBodyChunkAsyncOperation::End(operation, memRef, winHttpError);
}

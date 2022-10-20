// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace HttpServer;
using namespace HttpCommon;

class RequestMessageContext::ReadBodyChunkAsyncOperation 
    : public KtlProxyAsyncOperation
{
public:
    ReadBodyChunkAsyncOperation(
        RequestMessageContext const & owner,
        KMemRef &memRef,
        AsyncCallback const& callback,
        AsyncOperationSPtr const& parent)
        : KtlProxyAsyncOperation(owner.ktlAsyncReqDataContext_.RawPtr(), nullptr, callback, parent)
        , owner_(owner)
        , memRef_(memRef)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const& operation, KMemRef &memRef)
    {
        auto thisSPtr = AsyncOperation::End<ReadBodyChunkAsyncOperation>(operation);

        if (thisSPtr->Error.IsSuccess())
        {
            memRef = move(thisSPtr->memRef_);
        }

        return thisSPtr->Error;
    }

    NTSTATUS StartKtlAsyncOperation(
        __in KAsyncContextBase * const parentAsyncContext,
        __in KAsyncContextBase::CompletionCallback callback)
    {
        owner_.ktlAsyncReqDataContext_->Reuse();
        owner_.ktlAsyncReqDataContext_->StartReceiveRequestData(
            *owner_.kHttpRequest_,
            &memRef_,
            parentAsyncContext,
            callback);

        return STATUS_SUCCESS;
    }

private:
    RequestMessageContext const &owner_;
    KMemRef memRef_;
    ULONG size_;
};

class RequestMessageContext::SendResponseHeadersAsyncOperation 
    : public KtlProxyAsyncOperation
{
public:
    SendResponseHeadersAsyncOperation(
        RequestMessageContext const & owner,
        __in USHORT statusCode,
        __in std::wstring &description,
        bool bodyExists,
        AsyncCallback const& callback,
        AsyncOperationSPtr const& parent)
        : KtlProxyAsyncOperation(owner.ktlAsyncReqDataContext_.RawPtr(), nullptr, callback, parent)
        , owner_(owner)
        , statusCode_(statusCode)
        , description_(description)
        , bodyExists_(bodyExists)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const& operation)
    {
        auto thisSPtr = AsyncOperation::End<SendResponseHeadersAsyncOperation>(operation);

        return thisSPtr->Error;
    }

    NTSTATUS StartKtlAsyncOperation(
        __in KAsyncContextBase * const parentAsyncContext,
        __in KAsyncContextBase::CompletionCallback callback)
    {
        owner_.ktlAsyncReqDataContext_->Reuse();
        auto httpStatus = (KHttpUtils::HttpResponseCode) statusCode_;
        StringUtility::UnicodeToAnsi(description_, httpStatusLine_);

        ULONG flags = HTTP_SEND_RESPONSE_FLAG_BUFFER_DATA;
        if (bodyExists_)
        {
            flags |= HTTP_SEND_RESPONSE_FLAG_MORE_DATA;
        }

        owner_.ktlAsyncReqDataContext_->StartSendResponseHeader(
            *owner_.kHttpRequest_,
            httpStatus,
            const_cast<char*>(httpStatusLine_.c_str()),
            flags,
            parentAsyncContext,
            callback);

        return STATUS_SUCCESS;
    }

private:
    RequestMessageContext const &owner_;
    USHORT statusCode_;
    wstring description_;
    string httpStatusLine_;
    bool bodyExists_;
};

class RequestMessageContext::SendResponseBodyChunkAsyncOperation
    : public KtlProxyAsyncOperation
{
public:
    SendResponseBodyChunkAsyncOperation(
        RequestMessageContext const & owner,
        __in KMemRef &memRef,
        bool noMoreData,
        bool disconnect,
        AsyncCallback const& callback,
        AsyncOperationSPtr const& parent)
        : KtlProxyAsyncOperation(owner.ktlAsyncReqDataContext_.RawPtr(), nullptr, callback, parent)
        , owner_(owner)
        , memRef_(memRef)
        , isLastSegment_(noMoreData)
        , disconnect_(disconnect)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const& operation)
    {
        auto thisSPtr = AsyncOperation::End<SendResponseBodyChunkAsyncOperation>(operation);

        return thisSPtr->Error;
    }

    NTSTATUS StartKtlAsyncOperation(
        __in KAsyncContextBase * const parentAsyncContext,
        __in KAsyncContextBase::CompletionCallback callback)
    {
        owner_.ktlAsyncReqDataContext_->Reuse();

        ULONG flags = HTTP_SEND_RESPONSE_FLAG_BUFFER_DATA;
        if (!isLastSegment_)
        {
            // TODO: If disconnect_ is specified in this case, return error.
            flags |= HTTP_SEND_RESPONSE_FLAG_MORE_DATA;
        }
        else if (disconnect_)
        {
            flags |= HTTP_SEND_RESPONSE_FLAG_DISCONNECT;
        }

        // TODO: KTL doesnt support sending a null buffer with just the flag. Check and fix it.
        owner_.ktlAsyncReqDataContext_->StartSendResponseDataContent(
            *owner_.kHttpRequest_,
            memRef_,
            flags,
            parentAsyncContext,
            callback);

        return STATUS_SUCCESS;
    }

private:
    RequestMessageContext const &owner_;
    KMemRef memRef_;
    bool isLastSegment_;
    bool disconnect_;
};

class RequestMessageContext::ReadCertificateAsyncOperation :
public KtlProxyAsyncOperation
{
public:

    ReadCertificateAsyncOperation(
        KHttpServerRequest::AsyncResponseDataContext::SPtr &asyncContext,
        KHttpServerRequest::SPtr const &request,
        TimeSpan const& timeout,
        AsyncCallback const& callback,
        AsyncOperationSPtr const& parent)
        : KtlProxyAsyncOperation(asyncContext.RawPtr(), nullptr, callback, parent)
        , asyncContext_(asyncContext)
        , request_(request)
        , timeoutHelper_(timeout)
        , timer_()
        , timerLock_()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const& operation, KBuffer::SPtr &certBuffer)
    {
        auto thisSPtr = AsyncOperation::End<ReadCertificateAsyncOperation>(operation);

        {
            AcquireWriteLock lock(thisSPtr->timerLock_);
            if (thisSPtr->timer_)
            {
                thisSPtr->timer_->Cancel();
                thisSPtr->timer_.reset();
            }
        }

        if (thisSPtr->Error.IsSuccess())
        {
            certBuffer = move(thisSPtr->buffer_);
        }

        return thisSPtr->Error;
    }

    NTSTATUS StartKtlAsyncOperation(
        __in KAsyncContextBase * const parentAsyncContext,
        __in KAsyncContextBase::CompletionCallback callback)
    {
        asyncContext_->StartReceiveRequestCertificate(
            *request_,
            buffer_,
            parentAsyncContext,
            callback);

        // Start the certificate read timer.
        {
            AcquireExclusiveLock lock(timerLock_);
            timer_ = Timer::Create(
                "CertificateReadTimer",
                [this](TimerSPtr const &timer)
            {
                timer->Cancel();
                this->OnTimerCallback();
            });
            timer_->Change(timeoutHelper_.GetRemainingTime());
        }

        return STATUS_SUCCESS;
    }
	
    void OnTimerCallback()
    {
        if (asyncContext_)
        {
            // Initiate a cancel and let it complete.
            asyncContext_->Cancel();
        }
    }

private:

    KHttpServerRequest::AsyncResponseDataContext::SPtr &asyncContext_;
    KBuffer::SPtr buffer_;
    KHttpServerRequest::SPtr const &request_;
    Common::TimeoutHelper timeoutHelper_;
    Common::TimerSPtr timer_;
    Common::ExclusiveLock timerLock_;
};

RequestMessageContext::RequestMessageContext(KHttpServerRequest::SPtr &request)
: kHttpRequest_(request)
{
}

RequestMessageContext::~RequestMessageContext()
{
}

ErrorCode RequestMessageContext::TryParseRequest()
{
    KString::SPtr opStr;
    
    if (!kHttpRequest_->GetOpString(opStr))
    {
        return ErrorCodeValue::InvalidOperation;
    }

    verb_ = LPCWSTR(*opStr);

    KUri::SPtr kUriSPtr;
    NTSTATUS status = kHttpRequest_->GetUrl(kUriSPtr);
    if (!NT_SUCCESS(status)) { return ErrorCode::FromNtStatus(status); }

    url_ = LPCWSTR(*kUriSPtr);

    kUriSPtr = kHttpRequest_->GetRelativeUrl();
    suffix_ = LPCWSTR(*kUriSPtr);

    //If client specifies a RequestId in header "X-ServiceFabricRequestId", use it for correlation
    auto error = GetRequestHeader(HttpConstants::ServiceFabricHttpClientRequestIdHeader, clientRequestId_);
    if (!error.IsSuccess())
    {
        clientRequestId_ = Guid::NewGuid().ToString();
    }

    status = KHttpServerRequest::AsyncResponseDataContext::Create(
        ktlAsyncReqDataContext_,
        GetThisAllocator(),
        HttpConstants::AllocationTag);

    if (!NT_SUCCESS(status)) { return ErrorCode::FromNtStatus(status); }

    return ErrorCode::Success();
}

KAllocator& RequestMessageContext::GetThisAllocator() const
{
    return kHttpRequest_->GetThisAllocator();
}

wstring const& RequestMessageContext::GetVerb() const
{
    return verb_;
}

wstring RequestMessageContext::GetUrl() const
{
    return url_;
}

wstring RequestMessageContext::GetClientRequestId() const
{
    return clientRequestId_;
}

wstring RequestMessageContext::GetSuffix() const
{
    return suffix_;
}

ErrorCode RequestMessageContext::GetRequestHeader(__in wstring const &headerName, __out wstring &headerValue) const
{
    if (!headersMap_)
    {
        auto error = GetHeadersInternal();
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    auto headersMap = *headersMap_;
    if (headersMap.count(headerName) > 0)
    {
        headerValue = headersMap[headerName];
        return ErrorCodeValue::Success;
    }

    return ErrorCodeValue::NotFound;
}

ErrorCode RequestMessageContext::GetAllRequestHeaders(__out KString::SPtr &headers) const
{
    if (*headers_)
    {
        headers = headers_;
        return ErrorCodeValue::Success;
    }

    auto error = GetHeadersInternal();
    if (error.IsSuccess())
    {
        headers = headers_;
    }

    return error;
}

ErrorCode RequestMessageContext::GetAllRequestHeaders(__out KHashTable<KWString, KString::SPtr> &headers) const
{
    auto status = kHttpRequest_->GetAllHeaders(headers);
    return ErrorCode::FromNtStatus(status);
}

KHttpUtils::HttpAuthType RequestMessageContext::GetRequestAuthType() const
{
    return kHttpRequest_->GetRequestAuthType();
}

ErrorCode RequestMessageContext::GetClientToken(__out HANDLE &hToken) const
{
    HTTP_AUTH_STATUS authStatus = kHttpRequest_->GetRequestToken(hToken);

    if (authStatus != HttpAuthStatusSuccess)
    {
        return ErrorCodeValue::InvalidCredentials;
    }
    else
    {
        return ErrorCode::Success();
    }
}

AsyncOperationSPtr RequestMessageContext::BeginGetClientCertificate(
    __in Common::TimeSpan const& timeout,
    __in Common::AsyncCallback const& callback,
    __in Common::AsyncOperationSPtr const& parent) const
{
    ktlAsyncReqDataContext_->Reuse();
    return AllocableAsyncOperation::CreateAndStart<ReadCertificateAsyncOperation>(
        GetThisAllocator(),
        ktlAsyncReqDataContext_,
        kHttpRequest_,
        timeout,
        callback,
        parent);
}

ErrorCode RequestMessageContext::EndGetClientCertificate(
    __in Common::AsyncOperationSPtr const& operation,
    __out Common::CertContextUPtr &certContext) const
{
    KBuffer::SPtr certificateBuffer;
    auto error = ReadCertificateAsyncOperation::End(operation, certificateBuffer);
    if (!error.IsSuccess())
    {
        return error;
    }

    PCCERT_CONTEXT pCertContext = CertCreateCertificateContext(
        X509_ASN_ENCODING,
        ((PHTTP_SSL_CLIENT_CERT_INFO)certificateBuffer->GetBuffer())->pCertEncoded,
        ((PHTTP_SSL_CLIENT_CERT_INFO)certificateBuffer->GetBuffer())->CertEncodedSize);

    certContext = CertContextUPtr(pCertContext);

    return error;
}

ErrorCode RequestMessageContext::GetRemoteAddress(__out KString::SPtr &remoteAddress) const
{
    NTSTATUS status = kHttpRequest_->GetRemoteAddress(remoteAddress);
    if (!NT_SUCCESS(status)) { return ErrorCode::FromNtStatus(status); }

    return ErrorCode::Success();
}

ErrorCode RequestMessageContext::GetRemoteAddress(__out wstring &remoteAddress) const
{
    KString::SPtr remoteAddressKString;
    auto error = GetRemoteAddress(remoteAddressKString);
    if (error.IsSuccess()) { remoteAddress = *remoteAddressKString; }

    return error;
}

ErrorCode RequestMessageContext::SetResponseHeader(__in wstring const &headerName, __in wstring const &headerValue)
{
    KStringView value(headerValue.c_str());
    KHttpUtils::HttpHeaderType headerCode;
    NTSTATUS status;

    bool ret = HttpUtil::HeaderStringToKnownHeaderCode(headerName, headerCode);

    if (ret)
    {
        status = kHttpRequest_->SetResponseHeaderByCode(headerCode, value);
    }
    else
    {
        KStringView name(headerName.c_str());
        status = kHttpRequest_->SetResponseHeader(name, value);
    }

    return ErrorCode::FromNtStatus(status);
}

ErrorCode RequestMessageContext::SetAllResponseHeaders(KString::SPtr const&headers)
{
    // TODO: This functionality should be added to the KTL layer
    wstring headerString(*headers);
    StringCollection headerCollection;
    StringUtility::Split<wstring>(headerString, headerCollection, L"\r\n", true);

    ErrorCode error;
    for (wstring const &headerItem : headerCollection)
    {
        error = SetHeaderInternal(headerItem);
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    return ErrorCodeValue::NotImplemented;
}

AsyncOperationSPtr RequestMessageContext::BeginSendResponse(
    __in ErrorCode operationStatus,
    __in ByteBufferUPtr bodyUPtr,
    __in Common::AsyncCallback const& callback,
    __in Common::AsyncOperationSPtr const& parent)
{
    return AsyncOperation::CreateAndStart<SendResponseAsyncOperation>(operationStatus, move(bodyUPtr), *this, callback, parent);
}

AsyncOperationSPtr RequestMessageContext::BeginSendResponse(
    __in USHORT statusCode,
    __in std::wstring description,
    __in Common::ByteBufferUPtr bodyUPtr,
    __in Common::AsyncCallback const& callback,
    __in Common::AsyncOperationSPtr const& parent)
{
    return AsyncOperation::CreateAndStart<SendResponseAsyncOperation>(statusCode, description, move(bodyUPtr), *this, callback, parent);
}

ErrorCode RequestMessageContext::EndSendResponse(
    __in AsyncOperationSPtr const& operation)
{
    return SendResponseAsyncOperation::End(operation);
}

AsyncOperationSPtr RequestMessageContext::BeginGetMessageBody(
    __in Common::AsyncCallback const& callback,
    __in Common::AsyncOperationSPtr const& parent) const
{
    return AsyncOperation::CreateAndStart<ReadBodyAsyncOperation>(*this, callback, parent);
}

ErrorCode RequestMessageContext::EndGetMessageBody(
    __in AsyncOperationSPtr const& operation,
    __out ByteBufferUPtr &body) const
{
    return ReadBodyAsyncOperation::End(operation, body);
}

AsyncOperationSPtr RequestMessageContext::BeginGetMessageBodyChunk(
    __in KMemRef &memRef,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent) const
{
    return AllocableAsyncOperation::CreateAndStart<ReadBodyChunkAsyncOperation>(
        GetThisAllocator(),
        *this,
        memRef,
        callback,
        parent);
}

ErrorCode RequestMessageContext::EndGetMessageBodyChunk(
    __in AsyncOperationSPtr const& operation,
    __out KMemRef &memRef) const
{
    return ReadBodyChunkAsyncOperation::End(operation, memRef);
}

AsyncOperationSPtr RequestMessageContext::BeginSendResponseHeaders(
    __in USHORT statusCode,
    __in std::wstring description,
    bool bodyExists,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
{
    return AllocableAsyncOperation::CreateAndStart<SendResponseHeadersAsyncOperation>(
        GetThisAllocator(),
        *this,
        statusCode,
        description,
        bodyExists,
        callback,
        parent);
}

ErrorCode RequestMessageContext::EndSendResponseHeaders(
    AsyncOperationSPtr const& operation)
{
    return SendResponseHeadersAsyncOperation::End(operation);
}

AsyncOperationSPtr RequestMessageContext::BeginSendResponseChunk(
    KMemRef &mem,
    bool isLastSegment,
    bool disconnect,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
{
    return AllocableAsyncOperation::CreateAndStart<SendResponseBodyChunkAsyncOperation>(
        GetThisAllocator(),
        *this,
        mem,
        isLastSegment,
        disconnect,
        callback,
        parent);
}

ErrorCode RequestMessageContext::EndSendResponseChunk(
    AsyncOperationSPtr const& operation)
{
    return SendResponseBodyChunkAsyncOperation::End(operation);
}

ErrorCode RequestMessageContext::GetHeadersInternal() const
{
    auto error = ErrorCode::FromNtStatus(kHttpRequest_->GetAllHeaders(headers_));
    if (!error.IsSuccess())
    {
        return error;
    }

    headersMap_ = make_shared<map<wstring, wstring>>();
    wstring headerString(*headers_);
    StringCollection headerCollection;
    StringUtility::Split<wstring>(headerString, headerCollection, L"\r\n", true);

    for (wstring const &headerItem : headerCollection)
    {
        wstring headerName;
		wstring headerValue;
        GetHeaderNameValue(headerItem, headerName, headerValue);
		(*headersMap_)[headerName] = headerValue;
    }

    return ErrorCodeValue::Success;
}

ErrorCode RequestMessageContext::SetHeaderInternal(wstring const& headerString)
{
    wstring headerName;
    wstring headerValue;
    GetHeaderNameValue(headerString, headerName, headerValue);

    SetResponseHeader(headerName, headerValue);

    return ErrorCodeValue::Success;
}

void RequestMessageContext::GetHeaderNameValue(
    wstring const &headerString,
    __out wstring &headerName,
    __out wstring &headerValue) const
{
    StringUtility::SplitOnce<wstring, wchar_t>(headerString, headerName, headerValue, L':');
    StringUtility::TrimSpaces(headerName);
    StringUtility::TrimSpaces(headerValue);
}

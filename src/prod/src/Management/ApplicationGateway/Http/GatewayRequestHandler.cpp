// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Client;
using namespace HttpCommon;
using namespace HttpServer;
using namespace HttpClient;
using namespace Naming;
using namespace HttpApplicationGateway;
using namespace Transport;

// Callback that is called to verify the server's certificate.
// Context : pointer to string representing the  trace ID to be printed by this static method.
// The traceId can be used to track traces related to a particular request.
BOOLEAN GatewayRequestHandler::ValidateServerCertificate(PVOID Context, PCCERT_CONTEXT pCertContext)
{
    CertValidationContext* validationContext = (CertValidationContext *)Context;

    if (!validationContext)
    {
        return FALSE;
    }
    
    ThumbprintSet thumbprintsToMatch;
    SecurityConfig::X509NameMap remoteNamesToMatch;

    auto configPolicyVal = validationContext->Config().GetCertValidationPolicy();

    if (configPolicyVal == CertValidationPolicy::None)
    {
        return TRUE;
    }

    if (configPolicyVal != CertValidationPolicy::ServiceCertificateThumbprints
        && configPolicyVal != CertValidationPolicy::ServiceCommonNameAndIssuer)
    {
        HttpApplicationGatewayEventSource::Trace->ServiceCertValidationPolicyUnrecognizedError(
            validationContext->TraceId(),
            configPolicyVal);

        return FALSE;
    }

    if (configPolicyVal == CertValidationPolicy::ServiceCertificateThumbprints)
    {
        wstring const & certThumbprints = validationContext->Config().ServiceCertificateThumbprints;
        auto error = thumbprintsToMatch.Set(certThumbprints);
        if (!error.IsSuccess())
        {
            HttpApplicationGatewayEventSource::Trace->ServiceCertValidationThumbprintError(
                validationContext->TraceId(),
                error);
            return FALSE;
        }
    }
    else
    {
        remoteNamesToMatch = validationContext->Config().ServiceCommonNameAndIssuer;
    }

    NTSTATUS status = SecurityUtil::VerifyCertificate(
        pCertContext,
        thumbprintsToMatch,
        remoteNamesToMatch,
        true,
        validationContext->Config().CrlCheckingFlag,
        validationContext->Config().IgnoreCrlOfflineError);

    if (status == SEC_E_OK)
    {
        return TRUE;
    }

    HttpApplicationGatewayEventSource::Trace->ServiceCertValidationError(
        validationContext->TraceId(),
        status);

    return FALSE;
}

class GatewayRequestHandler::HandlerAsyncOperation
    : public Common::AllocableAsyncOperation
    , public HttpApplicationGateway::IHttpApplicationGatewayHandlerOperation
{
    DENY_COPY(HandlerAsyncOperation)
public:
    HandlerAsyncOperation(
        GatewayRequestHandler & owner,
        HttpServer::IRequestMessageContextUPtr messageContext,
        Common::AsyncCallback const &callback,
        Common::AsyncOperationSPtr const &parent)
        : Common::AllocableAsyncOperation(callback, parent)
        , messageContext_(std::move(messageContext))
        , owner_(owner)
        , timeout_(Common::TimeSpan::MaxValue)
        , traceId_(Common::Guid::NewGuid().ToString())
    {
    }
    static Common::ErrorCode End(__in Common::AsyncOperationSPtr const & asyncOperation);

    HttpServer::IRequestMessageContextUPtr & MessageContext()
    {
        return messageContext_;
    }

protected:
    void OnStart(AsyncOperationSPtr const &thisSPtr);

private:
    void OnCertificateReadComplete(Common::AsyncOperationSPtr const &operation);
    void InvokeHandler(Common::AsyncOperationSPtr const &operation);
    void OnBeginProcessReverseProxyRequestComplete(Common::AsyncOperationSPtr const &operation, bool);
    void SetAdditionalHeadersToSend(std::unordered_map<std::wstring, std::wstring>&&);

    // IHttpApplicationGatewayHandlerOperation interface methods
    void GetServiceName(std::wstring &)
    {
        return;
    }
    void GetAdditionalHeadersToSend(std::unordered_map<std::wstring, std::wstring>&);

    HttpServer::IRequestMessageContextUPtr messageContext_;
    Common::TimeSpan timeout_;
    GatewayRequestHandler & owner_;
    std::unordered_map<std::wstring, std::wstring> additionalHeaders_;
    std::wstring traceId_;
    // certContext_ is the certificate presented by the client. 
    // Store the certContext data here, in case of retriable errors if reverse proxy re resolves and retries, 
    // we will need to resend this.
    Common::CertContextUPtr certContext_;
};

void GatewayRequestHandler::HandlerAsyncOperation::GetAdditionalHeadersToSend(
    std::unordered_map<std::wstring, std::wstring>& headers)
{
    headers = additionalHeaders_;
}

void GatewayRequestHandler::HandlerAsyncOperation::SetAdditionalHeadersToSend(
    std::unordered_map<std::wstring, std::wstring>&& headers)
{
    additionalHeaders_ = move(headers);
}

void GatewayRequestHandler::HandlerAsyncOperation::OnStart(AsyncOperationSPtr const &thisSPtr)
{
    // For secure reverse proxy, if ForwardClientCertificate is set, begin reading the certificate data
    if (owner_.gateway_.GetProtocolType() == ServiceModel::ProtocolType::Enum::Https
        && owner_.gateway_.GetGatewayConfig().ForwardClientCertificate)
    {
        // TODO: Use a more appropriate timeout 
        auto operation = messageContext_->BeginGetClientCertificate(
            owner_.gateway_.GetGatewayConfig().DefaultHttpRequestTimeout,
            [this](AsyncOperationSPtr const& operation)
        {
            this->OnCertificateReadComplete(operation);
        },
            thisSPtr);
    }
    else
    {
        InvokeHandler(thisSPtr);
    }
}

void GatewayRequestHandler::HandlerAsyncOperation::OnCertificateReadComplete(AsyncOperationSPtr const& thisSPtr)
{
    auto error = messageContext_->EndGetClientCertificate(thisSPtr, certContext_);

    // Continue with the request even if reading client certificate fails or if client fails to send a certificate. 
    // Let the service decide how to handle this.
    if (!error.IsSuccess())
    {
        HttpApplicationGatewayEventSource::Trace->ReadClientCertError(
            traceId_,
            error,
            error.Message);
    }

    if (owner_.gateway_.GetGatewayConfig().ForwardClientCertificate)
    {
        // Add Client-Certificate header. If Client certificate was not presented, add empty header
        wstring certStr = L"";
        if (certContext_)
        {
            certStr = CryptoUtility::BytesToBase64String(
                certContext_->pbCertEncoded,
                certContext_->cbCertEncoded);
        }
        unordered_map<wstring, wstring> headersToAdd;
        pair<wstring, wstring> clientCertHeader(Constants::HttpClientCertHeaderName, move(certStr));
        headersToAdd.insert(move(clientCertHeader));
        this->SetAdditionalHeadersToSend(move(headersToAdd));
    }
    InvokeHandler(thisSPtr->Parent);
}

void GatewayRequestHandler::HandlerAsyncOperation::InvokeHandler(AsyncOperationSPtr const& thisSPtr)
{
    ByteBufferUPtr nullBody = nullptr;
    AsyncOperationSPtr operation = owner_.BeginProcessReverseProxyRequest(
        traceId_,
        this->MessageContext(),
        move(nullBody), // Body has not been read yet
        wstring(),
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnBeginProcessReverseProxyRequestComplete(operation, false);
    },
        thisSPtr);

    OnBeginProcessReverseProxyRequestComplete(operation, true);
}

void GatewayRequestHandler::HandlerAsyncOperation::OnBeginProcessReverseProxyRequestComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode error = ErrorCodeValue::Success;
    error = owner_.EndProcessReverseProxyRequest(operation);
    if (!error.IsSuccess())
    {
        HttpApplicationGatewayEventSource::Trace->EndProcessReverseProxyRequestFailed(
            traceId_,
            error);
    }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    handlerOperation->TryComplete(operation->Parent, error);
}

ErrorCode GatewayRequestHandler::HandlerAsyncOperation::End(__in AsyncOperationSPtr const& operation)
{
    auto thisPtr = AsyncOperation::End<HandlerAsyncOperation>(operation);
    return thisPtr->Error;
}

AsyncOperationSPtr GatewayRequestHandler::BeginProcessRequest(
    IRequestMessageContextUPtr request,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
{
    return AllocableAsyncOperation::CreateAndStart<HandlerAsyncOperation>(
        request->GetThisAllocator(),
        *this,
        move(request),
        callback,
        parent);
}

ErrorCode GatewayRequestHandler::EndProcessRequest(AsyncOperationSPtr const& operation)
{
    return HandlerAsyncOperation::End(operation);
}

AsyncOperationSPtr GatewayRequestHandler::BeginProcessReverseProxyRequest(
    wstring const &traceId,
    IRequestMessageContextUPtr &request,
    Common::ByteBufferUPtr &&body,
    wstring const & forwardingUri,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
{
    return AllocableAsyncOperation::CreateAndStart<ProcessRequestAsyncOperation>(
        request->GetThisAllocator(),
        *this,
        request,
        move(body),
        forwardingUri,
        traceId,
        callback,
        parent);
}

ErrorCode GatewayRequestHandler::EndProcessReverseProxyRequest(AsyncOperationSPtr const& operation)
{
    return ProcessRequestAsyncOperation::End(operation);
}

AsyncOperationSPtr GatewayRequestHandler::BeginSendRequestToService(
    wstring const &traceId,
    IRequestMessageContextUPtr &requestFromClient,
    IHttpClientRequestSPtr &requestToService,
    KHashTable<KWString, KString::SPtr> &requestHeaders,
    Common::ByteBufferUPtr && body,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
{
    return AllocableAsyncOperation::CreateAndStart<SendRequestToServiceAsyncOperation>(
        requestFromClient->GetThisAllocator(),
        traceId,
        requestFromClient,
        requestToService,
        requestHeaders,
        move(body),
        *this,
        callback,
        parent);
}

ErrorCode GatewayRequestHandler::EndSendRequestToService(
    AsyncOperationSPtr const& operation,
    ULONG *winHttpError,
    bool *safeToRetry,
    wstring & failurePhase)
{
    return SendRequestToServiceAsyncOperation::End(operation, winHttpError, safeToRetry, failurePhase);
}

AsyncOperationSPtr GatewayRequestHandler::BeginSendResponseToClient(
    wstring const &traceId,
    __in IRequestMessageContextUPtr &requestFromClient,
    __in IHttpClientRequestSPtr &requestToService,
    __in DateTime const& startTime,
    AsyncCallback const& callback,
    AsyncOperationSPtr const& parent)
{
    return AllocableAsyncOperation::CreateAndStart<SendResponseToClientAsyncOperation>(
        requestFromClient->GetThisAllocator(),
        traceId,
        requestFromClient,
        requestToService,
        startTime,
        gateway_,
        callback,
        parent);
}

ErrorCode GatewayRequestHandler::EndSendResponseToClient(
    AsyncOperationSPtr const& operation,
    ULONG *winHttpError,
    wstring & failurePhase)
{
    return SendResponseToClientAsyncOperation::End(operation, winHttpError, failurePhase);
}

//
// Allow retry for idempotent safe methods (with no body semantics)
//
bool GatewayRequestHandler::IsRetriableVerb(std::wstring const& verb)
{
    if (verb == HttpConstants::HttpGetVerb 
        || verb == HttpConstants::HttpHeadVerb
        || verb == HttpConstants::HttpOptionsVerb)
    {
        return true;
    }

    return false;
}

bool GatewayRequestHandler::IsRetriableError(ErrorCode &error)
{
    switch (error.ReadValue())
    {
    case ErrorCodeValue::GatewayUnreachable:
    case ErrorCodeValue::ServiceTooBusy:
    case ErrorCodeValue::ServiceOffline:
    case ErrorCodeValue::NotFound:
        return true;
    default:
        return false;
    }
}

bool GatewayRequestHandler::IsRetriableError(ULONG winHttpError)
{
    if (winHttpError == ERROR_WINHTTP_TIMEOUT ||
        IsConnectionError(winHttpError))
    {
        return true;
    }

    return false;
}

bool GatewayRequestHandler::IsConnectionError(ULONG winHttpError)
{
    if (winHttpError == ERROR_WINHTTP_CANNOT_CONNECT ||
        winHttpError == ERROR_WINHTTP_CONNECTION_ERROR)
    {
        return true;
    }

    return false;
}

void GatewayRequestHandler::ErrorCodeToHttpStatus(
    __in ULONG winhttpError,
    __out KHttpUtils::HttpResponseCode &httpStatus,
    __out std::wstring &httpStatusLine) const
{
    switch (winhttpError)
    {
    case ERROR_WINHTTP_TIMEOUT:
        httpStatus = KHttpUtils::HttpResponseCode::GatewayTimeout;
        httpStatusLine = L"Reverse Proxy Timeout";
        break;

    case ERROR_WINHTTP_SECURE_INVALID_CERT:
        httpStatus = KHttpUtils::HttpResponseCode::BadGateway;
        httpStatusLine = L"Invalid SSL Certificate";
        break;

    case ERROR_WINHTTP_SECURE_FAILURE:
        httpStatus = KHttpUtils::HttpResponseCode::BadGateway;
        httpStatusLine = L"SSL Handshake Failed";
        break;

    default:
        httpStatus = KHttpUtils::HttpResponseCode::InternalServerError;
        httpStatusLine = L"Internal Server Error";
        break;
    }
}

void GatewayRequestHandler::ProcessHeaders(
    __in KHashTable<KWString, KString::SPtr> & requestHeaders,
    __in KAllocator &allocator,
    __in KString::SPtr clientAddress,
    __in bool isWebSocketRequest,
    __in std::wstring & traceId,
    __out KString::SPtr &headers)
{
    //
    // http://tools.ietf.org/html/draft-ietf-httpbis-p1-messaging-14#section-7.1.3.
    // 1. Set all the End-to-end headers, X-Fwded Headers.
    // 2. Hop-by-Hop headers must be removed - Connection, Keep-Alive, Proxy-Authenticate, Transfer-Encoding, Upgrade 
    // 3. TODO: need to handle Connection header, TE: with trailers.
    // 4. TODO: Have a configuration to preserve the host header. If that is not set, then dont
    // transfer the original host header from the client.
    // 5. If Windows Auth is being used at the gateway, do not forward the Authorization header since it is related to the 
    // Client -> Gateway connection. Reason being this might break scenarios where service is also implementing Windows Auth and it tries to 
    // read this Authorization header that gets forwarded by the gateway. For instance, services based on HTTP.SYS might return 400 BadRequest for 
    // the request with the forwarded Authorization header. Remove the header here so that gateway's http client can add its own during the 
    // negotiation.
    // Also note: The current reverse proxy (FabricApplicationGateway) does not support Windows Auth, so when Windows Auth is not configured, 
    // Authorization headers must be forwarded to support token based authentication that the service may implement.

    // iterate through the map adding the key and value sizes
    ULONG headersLength = 0;
    KWString headerName(allocator);
    KString::SPtr headerValue;
    NTSTATUS status;

    // starting ktl map iterator - set the iterator position to begining
    requestHeaders.Reset();
    while (1)
    {
        status = requestHeaders.Next(headerName, headerValue);
        if (status == STATUS_NO_MORE_ENTRIES)
        {
            break;
        }

        if (headerName == *Constants::HttpHostHeaderName ||
            headerName == *Constants::HttpConnectionHeaderName ||
            headerName == *Constants::HttpKeepAliveHeaderName ||
            headerName == *Constants::HttpProxyAuthenticateHeaderName ||
            headerName == *Constants::HttpXForwardedForHeaderName ||
            headerName == *Constants::HttpXForwardedHostHeaderName ||
            headerName == *Constants::HttpXForwardedProtoHeaderName ||
            (Transport::SecurityProvider::IsWindowsProvider(gateway_.GetSecurityProvider()) && headerName == *Constants::HttpAuthorizationHeaderName) ||
            (isWebSocketRequest && headerName == *Constants::HttpWebSocketSubprotocolHeaderName))
        {
            continue;
        }

        headersLength += headerName.Length() + headerValue->Length();
        headersLength += 3; // for ":" + "\r\n"
    }

    //
    // Determine the X-Fwded headers.
    // Error return status => X-Forwarded-* headers are not present
    // This can be safely ignored.
    //
    KString::SPtr existingFwdedForHeaderValue;
    status = requestHeaders.Get(
        *Constants::HttpXForwardedForHeaderName,
        existingFwdedForHeaderValue);

    KString::SPtr existingFwdedHostHeaderValue;
    status = requestHeaders.Get(
        *Constants::HttpXForwardedHostHeaderName,
        existingFwdedHostHeaderValue);

    KString::SPtr hostHeaderValue;
    status = requestHeaders.Get(
        *Constants::HttpHostHeaderName,
        hostHeaderValue);
    if (!NT_SUCCESS(status))
    {
        HttpApplicationGatewayEventSource::Trace->MissingHostHeader(
            traceId);
    }

    KString::SPtr newFwdedForHeaderValue;
    Utility::CombineHeaderValues(allocator, existingFwdedForHeaderValue, clientAddress, newFwdedForHeaderValue);

    KString::SPtr newFwdedHostHeaderValue;
    Utility::CombineHeaderValues(allocator, existingFwdedHostHeaderValue, hostHeaderValue, newFwdedHostHeaderValue);

    headersLength += Constants::HttpXForwardedForHeaderName->Length() + newFwdedForHeaderValue->Length() + 3;
    headersLength += Constants::HttpXForwardedHostHeaderName->Length() + newFwdedHostHeaderValue->Length() + 3;
    headersLength += Constants::HttpXForwardedProtoHeaderName->Length() + static_cast<ULONG>(gateway_.GetProtocol().length()) + 3;

    KString::Create(headers, allocator, headersLength + 1);
    requestHeaders.Reset();
    while (1)
    {
        status = requestHeaders.Next(headerName, headerValue);
        if (status == STATUS_NO_MORE_ENTRIES)
        {
            break;
        }

        if (headerName == *Constants::HttpHostHeaderName ||
            headerName == *Constants::HttpConnectionHeaderName ||
            headerName == *Constants::HttpKeepAliveHeaderName ||
            headerName == *Constants::HttpProxyAuthenticateHeaderName ||
            headerName == *Constants::HttpXForwardedForHeaderName ||
            headerName == *Constants::HttpXForwardedHostHeaderName ||
            headerName == *Constants::HttpXForwardedProtoHeaderName ||
            Transport::SecurityProvider::IsWindowsProvider(gateway_.GetSecurityProvider()) && headerName == *Constants::HttpAuthorizationHeaderName ||
            isWebSocketRequest && headerName == *Constants::HttpWebSocketSubprotocolHeaderName)
        {
            continue;
        }

        AppendHeader(headers, headerName, headerValue);
    }

    AppendHeader(headers, *Constants::HttpXForwardedForHeaderName, newFwdedForHeaderValue);
    AppendHeader(headers, *Constants::HttpXForwardedHostHeaderName, newFwdedHostHeaderValue);
    AppendHeader(headers, *Constants::HttpXForwardedProtoHeaderName, gateway_.GetProtocol().c_str(), true); // last header

    headers->SetNullTerminator();
}

void GatewayRequestHandler::AppendHeader(
    KString::SPtr &headers,
    KWString &headerName,
    LPCWSTR headerValue,
    bool isLastHeader)
{
    if (headerName.Length() == 0)
    {
        return;
    }

    headers->Concat(KStringView((WCHAR*)headerName));
    headers->Concat(L":");
    headers->Concat(headerValue);
    if (!isLastHeader)
    {
        headers->Concat(L"\r\n");
    }
}

//
// Requests with empty header values shouldnt be rejected or stripped out.
//
void GatewayRequestHandler::AppendHeader(
    KString::SPtr &headers,
    KWString &headerName,
    KString::SPtr &headerValue,
    bool isLastHeader)
{
    if (headerName.Length() == 0)
    {
        return;
    }

    headers->Concat(KStringView((WCHAR*)headerName));
    headers->Concat(L":");

    if (headerValue->Length() != 0)
        headers->Concat(*headerValue);

    if (!isLastHeader)
    {
        headers->Concat(L"\r\n");
    }
}

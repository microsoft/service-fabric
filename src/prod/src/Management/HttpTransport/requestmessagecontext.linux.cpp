// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "cpprest/json.h"
#include "cpprest/http_listener.h"
#include "cpprest/uri.h"
#include "cpprest/asyncrt_utils.h"
#include "requestmessagecontext.linux.h"
#include "Common/CryptoUtility.Linux.h"

using namespace Common;
using namespace Transport;
using namespace std;
using namespace web; 
using namespace utility;
using namespace http;
using namespace web::http::experimental::listener;
using namespace HttpServer; 

static const StringLiteral TraceType("RequestMessageContext");

RequestMessageContext::RequestMessageContext(): responseUPtr_(new web::http::http_response()) {}

RequestMessageContext::RequestMessageContext(http_request& message): requestUPtr_(new http_request(message)),
    responseUPtr_(new web::http::http_response())
{
}

RequestMessageContext::~RequestMessageContext()
{
}

ErrorCode RequestMessageContext::TryParseRequest()
{
    //LINUXTODO: get full uri
    string requesturi = "http://localhost" + requestUPtr_->absolute_uri().to_string();

    string verb = requestUPtr_->method();
    string relativeuri = requestUPtr_->relative_uri().to_string();

    StringUtility::Utf8ToUtf16(requesturi, url_);
    StringUtility::Utf8ToUtf16(verb, verb_);	
    StringUtility::Utf8ToUtf16(relativeuri, suffix_);

    //If client specifies a RequestId in header "X-ServiceFabricRequestId", use it for correlation
    auto error = GetRequestHeader(HttpCommon::HttpConstants::ServiceFabricHttpClientRequestIdHeader, clientRequestId_);
    if (!error.IsSuccess())
    {
        clientRequestId_ = Guid::NewGuid().ToString();
    }

    return ErrorCode::Success();
}

std::wstring const& RequestMessageContext::GetVerb() const
{
    return verb_;
}

std::wstring RequestMessageContext::GetUrl() const
{
    return url_;
}

std::wstring RequestMessageContext::GetSuffix() const
{
    return suffix_;
}

wstring RequestMessageContext::GetClientRequestId() const
{
    return clientRequestId_;
}

ErrorCode RequestMessageContext::GetRequestHeader(__in wstring const &headerName, __out wstring &headerValue) const
{
    string value;
    string name;
    StringUtility::Utf16ToUtf8(headerName, name); 
	  
    if (requestUPtr_->headers().match(name, value))
    {
	    StringUtility::Utf8ToUtf16(value, headerValue);
        return ErrorCode::Success();
    }

    return ErrorCode::FromNtStatus(STATUS_NOT_FOUND);
}

ErrorCode RequestMessageContext::GetClientToken(__out HANDLE &hToken) const
{
    hToken = INVALID_HANDLE_VALUE;
    return ErrorCode::Success();   
}

AsyncOperationSPtr RequestMessageContext::BeginGetClientCertificate(
    __in Common::TimeSpan const& timeout,
    __in Common::AsyncCallback const& callback,
    __in Common::AsyncOperationSPtr const& parent) const
{
    UNREFERENCED_PARAMETER(timeout);
    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(callback, parent);
}

/// This passes raw pointer, life cycle of this ssl* is managed by underlying casablanca m_ssl_stream in connection object.
ErrorCode RequestMessageContext::EndGetClientCertificate(
    __in Common::AsyncOperationSPtr const& operation,
    __out SSL**  sslContext) const
{
  *sslContext = const_cast<SSL*>(requestUPtr_->client_ssl());
  if(*sslContext)
  {
    X509* cert = SSL_get_peer_certificate(*sslContext);
    if(cert)
    {
	string commonname; 
	LinuxCryptUtil().GetCommonName(cert, commonname);
	Trace.WriteInfo(TraceType, "Certificate object found with CN={0}", commonname);
    }
    else
    {
	Trace.WriteError(TraceType, "Certificate object not found");
	return ErrorCodeValue::InvalidCredentials;
    }
  }
  return ErrorCode::Success();
}

ErrorCode RequestMessageContext::GetRemoteAddress(__out wstring &remoteAddress) const
{
    // get ip address of host machine
    remoteAddress = L"";
    return ErrorCode::Success();
}

ErrorCode RequestMessageContext::SetResponseHeader(__in wstring const &headerName, __in wstring const &headerValue)
{
    string name;
    string value;
    StringUtility::Utf16ToUtf8(headerName, name);
    StringUtility::Utf16ToUtf8(headerValue, value);
    http_headers& responseHeader = responseUPtr_->headers();
    responseHeader.add(name, value);
    return ErrorCode::Success();
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
    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(callback, parent);
}

ErrorCode RequestMessageContext::EndGetMessageBody(
    __in AsyncOperationSPtr const& operation,
    __out ByteBufferUPtr &body) const
{
    try
    {
        // move this to separate asycn method. 
        // extract vector of char body from request.
        // TODO: Change this to a continuation
        body = make_unique<ByteBuffer>(move(requestUPtr_->extract_vector().get()));
    }
    catch (...)
    {
        auto eptr = std::current_exception();
        RequestMessageContext::HandleException(eptr, GetClientRequestId());
    }

    return CompletedAsyncOperation::End(operation); 
}

AsyncOperationSPtr RequestMessageContext::BeginGetFileFromUpload(
    __in ULONG fileSize,
    __in ULONG maxEntityBodyForUploadChunkSize,
    __in ULONG defaultEntityBodyForUploadChunkSize,
    __in Common::AsyncCallback const& callback,
    __in Common::AsyncOperationSPtr const& parent) const
{
    return AsyncOperation::CreateAndStart<GetFileFromUploadAsyncOperation>(*this, fileSize, maxEntityBodyForUploadChunkSize, defaultEntityBodyForUploadChunkSize, callback, parent);
}

ErrorCode RequestMessageContext::EndGetFileFromUpload(
    __in AsyncOperationSPtr const& operation,
    __out wstring & uniqueFileName) const
{
    return GetFileFromUploadAsyncOperation::End(operation, uniqueFileName);
}

void RequestMessageContext::HandleException(std::exception_ptr eptr, std::wstring const& clientRequestId)
{
    try
    {
        if (eptr)
        {
            std::rethrow_exception(eptr);
        }
    }
    catch (const std::exception& e) 
    {
        Trace.WriteInfo(TraceType, "Exception while replying to http_request. Client Request Id : {0}, Exception details : {1}", clientRequestId, e.what());
    }
    catch (...)
    {
        Trace.WriteError(TraceType, "Unknown Exception while replying to http_request. Client Request Id : {0}", clientRequestId);
    }
}

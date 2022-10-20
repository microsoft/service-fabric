// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace HttpGateway;
using namespace Transport;
using namespace std;

StringLiteral const TraceType("HttpGatewayRequestHandler");

void RequestHandlerBase::HandlerAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
{
    wstring urlSuffix = MessageContext.GetSuffix();
    wstring rawUrl = MessageContext.GetUrl();
    wstring const& verb = MessageContext.GetVerb();
    ErrorCode error = ErrorCodeValue::Success;

    auto hstsHeaderValue = HttpGatewayConfig::GetConfig().HttpStrictTransportSecurityHeader;
    if (!hstsHeaderValue.empty())
    {
        error = MessageContext.SetResponseHeader(Constants::HSTSHeader, hstsHeaderValue);
        if (!error.IsSuccess())
        {
            WriteInfo(TraceType, "SetHSTSResponseHeader failed with error- {0}", error);

            OnError(thisSPtr, HttpCommon::HttpStatusCode::InternalServerError, L"Set Strict-Transport-Security header failed");
            return;
        }
    }

    if (!GatewayUri::TryParse(owner_.validHandlerUris, verb, rawUrl, urlSuffix, uri_, owner_.allowHierarchicalEntityKeys))
    {
        WriteInfo(TraceType, "Invalid URL - {0}", rawUrl);

        OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    //If client specifies a ClusterId in header and the cluster contains a clusterId setting
    //we need to ensure that those two match. This is to ensure that a message from 
    //admin client is not forwarded by a rogue cluster to target cluster that it is not
    //intented for
    wstring clusterId;
    error = MessageContext.GetRequestHeader(Constants::ClusterIdHeader, clusterId);
    if (error.IsSuccess())
    {
        auto expectedId = PaasConfig::GetConfig().ClusterId;
        if (!expectedId.empty())
        {
            if (!StringUtility::AreEqualCaseInsensitive(clusterId, expectedId))
            {
                WriteInfo(TraceType, "Request clusterId {0} doesnt match expected {1}", clusterId, expectedId);
                OnError(thisSPtr, ErrorCodeValue::AccessDenied);
                return;
            }
        }
    }
    if (!Uri.AllowAnonymous)
    {
        auto inner = owner_.server_.BeginCheckAccess(
            MessageContext,
            TimeSpan::FromMinutes(Constants::DefaultAccessCheckTimeoutMin),
            [this](AsyncOperationSPtr const operation)
        {
            this->OnAccessCheckComplete(operation, false);
        },
            thisSPtr);

        OnAccessCheckComplete(inner, true);
    }
    else
    {
        InvokeHandler(thisSPtr, RoleMask::None);
    }
}

void RequestHandlerBase::HandlerAsyncOperation::OnAccessCheckComplete(__in AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    USHORT statusCode;
    wstring authHeaderName;
    wstring authHeaderValue;
    Transport::RoleMask::Enum role;
    auto error = owner_.server_.EndCheckAccess(operation, statusCode, authHeaderName, authHeaderValue, role);
    if (!error.IsSuccess())
    {
        wstring remoteAddress;
        MessageContext.GetRemoteAddress(remoteAddress);

        WriteInfo(
            TraceType,
            "CheckAccess failed for URL: {0}, from {1}, operation: {2}, responding with ErrorCode: {3} authheader: {4}:{5}, ClientRequestId: {6}",
            MessageContext.GetUrl(),
            remoteAddress,
            Uri.Verb,
            statusCode,
            authHeaderName,
            authHeaderValue,
            MessageContext.GetClientRequestId());

        if (error.IsError(ErrorCodeValue::InvalidCredentials))
        {
            if (!authHeaderName.empty())
            {
                MessageContext.SetResponseHeader(authHeaderName, authHeaderValue);
            }
            OnError(operation->Parent, statusCode, Constants::StatusDescriptionUnauthorized);
        }
        else
        {
            OnError(operation->Parent, error);
        }
        return;
    }

    InvokeHandler(operation->Parent, role);
}

void RequestHandlerBase::HandlerAsyncOperation::InvokeHandler(
    __in AsyncOperationSPtr const& thisSPtr, 
    __in Transport::RoleMask::Enum requestRole)
{
    auto &server = static_cast<HttpGatewayImpl&>(owner_.server_);

    if (requestRole == RoleMask::Admin || requestRole == RoleMask::None)
    {
        SetFabricClient(server.AdminClient);
    }
    else if (requestRole == RoleMask::User)
    {
        SetFabricClient(server.UserClient);
    }
    else
    {
        Assert::CodingError("Unknown client role {0}", requestRole);
    }

    UpdateRequestTimeout();

    if (Uri.Verb == Constants::HttpPostVerb || Uri.Verb == Constants::HttpPutVerb || Uri.Verb == Constants::HttpDeleteVerb)
    {
        this->GetRequestBody(thisSPtr);
    }
    else
    {
        LogRequest();
        Uri.Handler(thisSPtr);
    }
}

void RequestHandlerBase::HandlerAsyncOperation::GetRequestBody(Common::AsyncOperationSPtr const& thisSPtr)
{
    auto inner = MessageContext.BeginGetMessageBody(
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetBodyComplete(operation, false);
    },
        thisSPtr);

    OnGetBodyComplete(inner, true);
}

void RequestHandlerBase::HandlerAsyncOperation::OnGetBodyComplete(__in AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = MessageContext.EndGetMessageBody(operation, body_);
    if (!error.IsSuccess())
    {
        OnError(operation->Parent, error);
        return;
    }

    LogRequest();
    Uri.Handler(operation->Parent);
}

void RequestHandlerBase::HandlerAsyncOperation::OnError(
    AsyncOperationSPtr const& thisSPtr,
    ErrorCode const & error)
{
    ByteBufferUPtr responseBody;

    ErrorBody errorBody = ErrorBody::CreateErrorBodyFromErrorCode(error);
    ErrorCode innerError = JsonSerialize(errorBody, responseBody);
    if (!innerError.IsSuccess())
    {
         Common::Assert::CodingError("OnError cannot serialize json message {0}", error.ToHResult());
    }

    innerError = SetContentTypeResponseHeaders(Constants::JsonContentType);
    if (!innerError.IsSuccess())
    {
        TryComplete(thisSPtr, innerError);
        return;
    }

    WriteInfo(
        TraceType,
        "Responding with error {0} {1} for ClientRequestId {2}.",
        errorBody.GetErrorCode(),
        errorBody.GetErrorMessage(),
        MessageContext.GetClientRequestId());

    AsyncOperationSPtr operation = messageContext_->BeginSendResponse(
        error,
        move(responseBody),
        [this, error](AsyncOperationSPtr const& operation)
    {
        this->messageContext_->EndSendResponse(operation);
        this->TryComplete(operation->Parent, error);
    },
        thisSPtr);
}

void RequestHandlerBase::HandlerAsyncOperation::OnError(Common::AsyncOperationSPtr const& thisSPtr, __in USHORT statusCode, __in wstring const& statusDescription)
{
    ByteBufferUPtr emptyBody;

    wstring statusDesc;
    if (statusCode == Constants::StatusAuthenticate)
    {
        statusDesc = *Constants::StatusDescriptionUnauthorized;
    }
    else if (statusCode == Constants::StatusUnauthorized)
    {
        statusDesc = *Constants::StatusDescriptionClientCertificateRequired;
    }
    else
    {
        statusDesc = statusDescription;
    }

    WriteInfo(
        TraceType,
        "Responding with header: {0}, description: {1} for ClientRequestId {2}.",
        statusCode,
        statusDesc,
        MessageContext.GetClientRequestId());

    AsyncOperationSPtr operation = messageContext_->BeginSendResponse(
        statusCode,
        statusDesc,
        move(emptyBody),
        [this](AsyncOperationSPtr const& operation)
    {
        auto error = this->messageContext_->EndSendResponse(operation);
        this->TryComplete(operation->Parent, error);
    },
        thisSPtr);
}

void RequestHandlerBase::HandlerAsyncOperation::OnSuccess(AsyncOperationSPtr const& thisSPtr, __in ByteBufferUPtr bufferUPtr)
{
    OnSuccess(thisSPtr, move(bufferUPtr), *HttpGateway::Constants::JsonContentType);
}

void RequestHandlerBase::HandlerAsyncOperation::OnSuccess(AsyncOperationSPtr const& thisSPtr, __in ByteBufferUPtr bufferUPtr, __in wstring const& contentType)
{
    auto error = SetContentTypeResponseHeaders(contentType);

    if (!error.IsSuccess())
    {
        TryComplete(thisSPtr, error);
        return;
    }

    AsyncOperationSPtr operation = messageContext_->BeginSendResponse(
        ErrorCode::Success(),
        move(bufferUPtr),
        [this](AsyncOperationSPtr const& operation)
    {
        auto error = this->messageContext_->EndSendResponse(operation);
        this->TryComplete(operation->Parent, error);
    },
        thisSPtr);
}

void RequestHandlerBase::HandlerAsyncOperation::OnSuccess(AsyncOperationSPtr const& thisSPtr, __in ByteBufferUPtr bufferUPtr, __in USHORT statusCode, __in wstring const &description)
{
    auto error = SetContentTypeResponseHeaders(Constants::JsonContentType);

    if (!error.IsSuccess())
    {
        TryComplete(thisSPtr, error);
        return;
    }

    AsyncOperationSPtr operation = messageContext_->BeginSendResponse(
        statusCode,
        description,
        move(bufferUPtr),
        [this](AsyncOperationSPtr const& operation)
    {
        auto error = this->messageContext_->EndSendResponse(operation);
        this->TryComplete(operation->Parent, error);
    },
        thisSPtr);
}

ErrorCode RequestHandlerBase::HandlerAsyncOperation::End(__in AsyncOperationSPtr const& operation)
{
    auto thisPtr = AsyncOperation::End<HandlerAsyncOperation>(operation);

    ASSERT_IF(thisPtr->messageContext_ == nullptr, "Message context not valid");

    return thisPtr->Error;
}

void RequestHandlerBase::HandlerAsyncOperation::UpdateRequestTimeout()
{
    wstring timeoutString;
    if (uri_.GetItem(Constants::TimeoutString, timeoutString))
    {
        uint timeout;
        if (StringUtility::TryFromWString<uint>(timeoutString, timeout, 10))
        {
            timeout_ = TimeSpan::FromSeconds(timeout);
        }
        else
        {
            WriteWarning(
                TraceType,
                "Invalid timeout - {0} was specified in uri for ClientRequestId {1}, using default timeout.",
                timeoutString,
                MessageContext.GetClientRequestId());
        }
    }
}

void RequestHandlerBase::HandlerAsyncOperation::LogRequest()
{
    wstring remoteAddress;

    MessageContext.GetRemoteAddress(remoteAddress);
    WriteInfo(
        TraceType,
        "Dispatching URL {0}, from {1}, operation: {2}, Anonymous: {3}, Role: {4}, Body: {5}, ClientRequestId: {6}",
        MessageContext.GetUrl(),
        remoteAddress,
        Uri.Verb,
        Uri.AllowAnonymous,
        client_->ClientRole,
        GetMessageBodyForTrace(),
        MessageContext.GetClientRequestId());
}

string RequestHandlerBase::HandlerAsyncOperation::GetMessageBodyForTrace()
{
    if (Uri.Verb == Constants::HttpPostVerb)
    {
        return string((char*)(Body->data()), Body->size());
    }
    else
    {
        return string();
    }
}

void RequestHandlerBase::HandlerAsyncOperation::SetFabricClient(__in FabricClientWrapperSPtr const& client)
{
    client_ = client;
}

ErrorCode RequestHandlerBase::HandlerAsyncOperation::SetContentTypeResponseHeaders(__in wstring const& contentType)
{
    if (!contentType.empty())
    {
        auto error = MessageContext.SetResponseHeader(Constants::ContentTypeHeader, contentType);
        if (!error.IsSuccess())
        {
            return error;
        }

        // Set x-content-type-options: nosniff header in the response.
        error = MessageContext.SetResponseHeader(Constants::ContentTypeOptionsHeader, Constants::ContentTypeNoSniffValue);
        return error;
    }

    return ErrorCode::Success();
}

#if !defined(PLATFORM_UNIX)
void RequestHandlerBase::HandlerAsyncOperation::GetAdditionalHeadersToSend(
    std::unordered_map<std::wstring, std::wstring>& headers)
{
    headers = additionalHeaders_;
}

void RequestHandlerBase::HandlerAsyncOperation::SetAdditionalHeadersToSend(
    std::unordered_map<std::wstring, std::wstring>&& headers)
{
    additionalHeaders_ = move(headers);
}

void RequestHandlerBase::HandlerAsyncOperation::SetServiceName(std::wstring serviceName)
{
    serviceName_ = serviceName;
}

void RequestHandlerBase::HandlerAsyncOperation::GetServiceName(
    std::wstring& serviceName)
{
    serviceName = serviceName_;
}
#endif

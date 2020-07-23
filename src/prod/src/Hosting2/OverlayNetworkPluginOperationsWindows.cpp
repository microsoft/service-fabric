// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;
using namespace HttpClient;
using namespace HttpCommon;

StringLiteral const TraceType_OverlayNetworkPluginNetworkOperations("OverlayNetworkPluginNetworkOperations");

Common::WStringLiteral const QueryResponse::ReturnCodeParameter(L"ReturnCode");
Common::WStringLiteral const QueryResponse::MessageParameter(L"Message");

OverlayNetworkPluginOperations::OverlayNetworkPluginOperations(
    ComponentRootSPtr const & root)
    : RootedObject(*root)
{
}

OverlayNetworkPluginOperations::~OverlayNetworkPluginOperations()
{
}

Common::AsyncOperationSPtr OverlayNetworkPluginOperations::BeginUpdateRoutes(
    OverlayNetworkRoutingInformationSPtr const & routingInfo,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<UpdateRoutesAsyncOperation>(
        *this,
        routingInfo,
        timeout,
        callback,
        parent);
}

Common::ErrorCode OverlayNetworkPluginOperations::EndUpdateRoutes(
    Common::AsyncOperationSPtr const & operation)
{
    return UpdateRoutesAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr OverlayNetworkPluginOperations::BeginAttachContainer(
    OverlayNetworkContainerParametersSPtr const & params,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<AttachContainerAsyncOperation>(
        *this,
        params,
        timeout,
        callback,
        parent);
}

Common::ErrorCode OverlayNetworkPluginOperations::EndAttachContainer(
    Common::AsyncOperationSPtr const & operation)
{
    return AttachContainerAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr OverlayNetworkPluginOperations::BeginDetachContainer(
    std::wstring containerId,
    std::wstring networkName,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DetachContainerAsyncOperation>(
        *this,
        containerId,
        networkName,
        timeout,
        callback,
        parent);
}

Common::ErrorCode OverlayNetworkPluginOperations::EndDetachContainer(
    Common::AsyncOperationSPtr const & operation)
{
    return DetachContainerAsyncOperation::End(operation);
}

OverlayNetworkPluginOperations::UpdateRoutesAsyncOperation::UpdateRoutesAsyncOperation(
    OverlayNetworkPluginOperations & owner,
    OverlayNetworkRoutingInformationSPtr const & routingInfo,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent),
    owner_(owner),
    routingInfo_(routingInfo),
    timeoutHelper_(timeout)
{
}

OverlayNetworkPluginOperations::UpdateRoutesAsyncOperation::~UpdateRoutesAsyncOperation()
{
}

ErrorCode OverlayNetworkPluginOperations::UpdateRoutesAsyncOperation::UpdateRoutesAsyncOperation::End(
    AsyncOperationSPtr const & operation)
{
    auto thisPtr = AsyncOperation::End<UpdateRoutesAsyncOperation>(operation);
    return thisPtr->Error;
}

void OverlayNetworkPluginOperations::UpdateRoutesAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    client_ = nullptr;
    clientRequest_ = nullptr;

    HttpClientImpl::CreateHttpClient(
        L"OverlayNetworkPluginClient",
        owner_.Root,
        client_);

    wstring uri = Constants::NetworkSetup::OverlayNetworkUpdateRoutes;
    TimeSpan timeout = timeoutHelper_.GetRemainingTime();
    auto error = client_->CreateHttpRequest(
        uri,
        timeout,
        timeout,
        timeout,
        clientRequest_);

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType_OverlayNetworkPluginNetworkOperations,
            owner_.Root.TraceId,
            "Failed to create update routes request {0}.",
            error);

        TryComplete(thisSPtr, error);
        return;
    }

    clientRequest_->SetVerb(*HttpConstants::HttpPostVerb);

    WriteInfo(
        TraceType_OverlayNetworkPluginNetworkOperations,
        owner_.Root.TraceId,
        "Overlay network routing info {0}.",
        this->routingInfo_);

    wstring routingInfoString;
    error = JsonHelper::Serialize(*this->routingInfo_.get(), routingInfoString);
    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, error);
        return;
    }

    WriteInfo(
        TraceType_OverlayNetworkPluginNetworkOperations,
        owner_.Root.TraceId,
        "Routing info {0}.",
        routingInfoString);

    ByteBufferUPtr requestBody;
    error = JsonHelper::Serialize(*this->routingInfo_.get(), requestBody);
    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, error);
        return;
    }

    clientRequest_->SetRequestHeader(*HttpConstants::ContentTypeHeader, *Constants::HttpContentTypeJson);

    auto operation = clientRequest_->BeginSendRequest(
        move(requestBody),
        [this](AsyncOperationSPtr const &operation)
    {
        this->OnHttpRequestCompleted(operation, false);
    },
        thisSPtr);

    this->OnHttpRequestCompleted(operation, true);
}

void OverlayNetworkPluginOperations::UpdateRoutesAsyncOperation::OnHttpRequestCompleted(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    ULONG winHttpError;
    auto error = clientRequest_->EndSendRequest(operation, &winHttpError);
    if (!error.IsSuccess())
    {
        TryComplete(operation->Parent, error);
    }

    USHORT statusCode;
    wstring description;
    auto err = clientRequest_->GetResponseStatusCode(&statusCode, description);

    WriteInfo(
        TraceType_OverlayNetworkPluginNetworkOperations,
        owner_.Root.TraceId,
        "Publish network mapping returned error {0} and statuscode {1} description {2}.",
        err,
        statusCode,
        description);

    owner_.GetHttpResponse(operation->Parent, clientRequest_);
}

OverlayNetworkPluginOperations::AttachContainerAsyncOperation::AttachContainerAsyncOperation(
    OverlayNetworkPluginOperations & owner,
    OverlayNetworkContainerParametersSPtr const & params,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent),
    owner_(owner),
    params_(params),
    timeoutHelper_(timeout)
{
}

OverlayNetworkPluginOperations::AttachContainerAsyncOperation::~AttachContainerAsyncOperation()
{
}

ErrorCode OverlayNetworkPluginOperations::AttachContainerAsyncOperation::AttachContainerAsyncOperation::End(
    AsyncOperationSPtr const & operation)
{
    auto thisPtr = AsyncOperation::End<AttachContainerAsyncOperation>(operation);
    return thisPtr->Error;
}

void OverlayNetworkPluginOperations::AttachContainerAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    client_ = nullptr;
    clientRequest_ = nullptr;

    HttpClientImpl::CreateHttpClient(
        L"OverlayNetworkPluginClient",
        owner_.Root,
        client_);

    wstring uri = Constants::NetworkSetup::OverlayNetworkAttachContainer;
    TimeSpan timeout = timeoutHelper_.GetRemainingTime();
    auto error = client_->CreateHttpRequest(
        uri,
        timeout,
        timeout,
        timeout,
        clientRequest_);

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType_OverlayNetworkPluginNetworkOperations,
            owner_.Root.TraceId,
            "Failed to attach container request {0}.",
            error);

        TryComplete(thisSPtr, error);
        return;
    }

    clientRequest_->SetVerb(*HttpConstants::HttpPostVerb);

    WriteInfo(
        TraceType_OverlayNetworkPluginNetworkOperations,
        owner_.Root.TraceId,
        "Attach container to overlay network params {0}.",
        this->params_);

    wstring paramsString;
    error = JsonHelper::Serialize(*this->params_.get(), paramsString);
    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, error);
        return;
    }

    WriteInfo(
        TraceType_OverlayNetworkPluginNetworkOperations,
        owner_.Root.TraceId,
        "Attach container parameters {0}.",
        paramsString);

    ByteBufferUPtr requestBody;
    error = JsonHelper::Serialize(*this->params_.get(), requestBody);
    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, error);
        return;
    }

    clientRequest_->SetRequestHeader(*HttpConstants::ContentTypeHeader, *Constants::HttpContentTypeJson);

    auto operation = clientRequest_->BeginSendRequest(
        move(requestBody),
        [this](AsyncOperationSPtr const &operation)
    {
        this->OnHttpRequestCompleted(operation, false);
    },
        thisSPtr);

    this->OnHttpRequestCompleted(operation, true);
}

void OverlayNetworkPluginOperations::AttachContainerAsyncOperation::OnHttpRequestCompleted(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    ULONG winHttpError;
    auto error = clientRequest_->EndSendRequest(operation, &winHttpError);
    if (!error.IsSuccess())
    {
        TryComplete(operation->Parent, error);
    }

    USHORT statusCode;
    wstring description;
    auto err = clientRequest_->GetResponseStatusCode(&statusCode, description);

    WriteInfo(
        TraceType_OverlayNetworkPluginNetworkOperations,
        owner_.Root.TraceId,
        "Attach container returned error {0} and statuscode {1} description {2}.",
        err,
        statusCode,
        description);

    owner_.GetHttpResponse(operation->Parent, clientRequest_);
}

OverlayNetworkPluginOperations::DetachContainerAsyncOperation::DetachContainerAsyncOperation(
    OverlayNetworkPluginOperations & owner,
    std::wstring containerId,
    std::wstring networkName,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent),
    owner_(owner),
    containerId_(containerId),
    networkName_(networkName),
    timeoutHelper_(timeout)
{
}

OverlayNetworkPluginOperations::DetachContainerAsyncOperation::~DetachContainerAsyncOperation()
{
}

ErrorCode OverlayNetworkPluginOperations::DetachContainerAsyncOperation::DetachContainerAsyncOperation::End(
    AsyncOperationSPtr const & operation)
{
    auto thisPtr = AsyncOperation::End<DetachContainerAsyncOperation>(operation);
    return thisPtr->Error;
}

void OverlayNetworkPluginOperations::DetachContainerAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    client_ = nullptr;
    clientRequest_ = nullptr;

    HttpClientImpl::CreateHttpClient(
        L"OverlayNetworkPluginClient",
        owner_.Root,
        client_);

    wstring uri = Constants::NetworkSetup::OverlayNetworkDetachContainer;
    TimeSpan timeout = timeoutHelper_.GetRemainingTime();
    auto error = client_->CreateHttpRequest(
        uri,
        timeout,
        timeout,
        timeout,
        clientRequest_);

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType_OverlayNetworkPluginNetworkOperations,
            owner_.Root.TraceId,
            "Failed to detach container request {0}.",
            error);

        TryComplete(thisSPtr, error);
        return;
    }

    clientRequest_->SetVerb(*HttpConstants::HttpPostVerb);

    auto params = make_shared<OverlayNetworkContainerParameters>(
        containerId_, 
        networkName_, 
        false); //outboundNAT

    WriteInfo(
        TraceType_OverlayNetworkPluginNetworkOperations,
        owner_.Root.TraceId,
        "Detach container from overlay network params {0}.",
        params);

    wstring paramsString;
    error = JsonHelper::Serialize(*params.get(), paramsString);
    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, error);
        return;
    }

    WriteInfo(
        TraceType_OverlayNetworkPluginNetworkOperations,
        owner_.Root.TraceId,
        "Detach container parameters {0}.",
        paramsString);

    ByteBufferUPtr requestBody;
    error = JsonHelper::Serialize(*params.get(), requestBody);
    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, error);
        return;
    }

    clientRequest_->SetRequestHeader(*HttpConstants::ContentTypeHeader, *Constants::HttpContentTypeJson);

    auto operation = clientRequest_->BeginSendRequest(
        move(requestBody),
        [this](AsyncOperationSPtr const &operation)
    {
        this->OnHttpRequestCompleted(operation, false);
    },
        thisSPtr);

    this->OnHttpRequestCompleted(operation, true);
}

void OverlayNetworkPluginOperations::DetachContainerAsyncOperation::OnHttpRequestCompleted(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    ULONG winHttpError;
    auto error = clientRequest_->EndSendRequest(operation, &winHttpError);
    if (!error.IsSuccess())
    {
        TryComplete(operation->Parent, error);
    }

    USHORT statusCode;
    wstring description;
    auto err = clientRequest_->GetResponseStatusCode(&statusCode, description);

    WriteInfo(
        TraceType_OverlayNetworkPluginNetworkOperations,
        owner_.Root.TraceId,
        "Detach container returned error {0} and statuscode {1} description {2}.",
        err,
        statusCode,
        description);

    owner_.GetHttpResponse(operation->Parent, clientRequest_);
}

void OverlayNetworkPluginOperations::GetHttpResponse(
    AsyncOperationSPtr const & thisSPtr, 
    HttpClient::IHttpClientRequestSPtr clientRequest)
{
    auto operation = clientRequest->BeginGetResponseBody(
        [this, clientRequest](AsyncOperationSPtr const &operation)
    {
        this->OnGetHttpResponseCompleted(operation, clientRequest, false);
    },
        thisSPtr);
    this->OnGetHttpResponseCompleted(operation, clientRequest, true);
}

void OverlayNetworkPluginOperations::OnGetHttpResponseCompleted(
    AsyncOperationSPtr const & operation,
    HttpClient::IHttpClientRequestSPtr clientRequest,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    ByteBufferUPtr body;
    auto error = clientRequest->EndGetResponseBody(operation, body);

    USHORT statusCode;
    wstring description;
    clientRequest->GetResponseStatusCode(&statusCode, description);

    if (!error.IsSuccess())
    {
        operation->Parent->TryComplete(operation->Parent, error);
        return;
    }

    if (statusCode != 200)
    {
        operation->Parent->TryComplete(operation->Parent, ErrorCodeValue::InvalidState);
        return;
    }

    QueryResponse queryResponse;
    error = JsonHelper::Deserialize(queryResponse, body);

    WriteInfo(
        TraceType_OverlayNetworkPluginNetworkOperations,
        "Network plugin query call returned statuscode {0} message {1}.",
        queryResponse.ReturnCode,
        queryResponse.Message);

    if (!error.IsSuccess())
    {
        operation->Parent->TryComplete(operation->Parent, error);
        return;
    }
    
    auto finalErrorCode = (queryResponse.ReturnCode == 0) ? (ErrorCode(ErrorCodeValue::Success)) : (ErrorCode(ErrorCodeValue::OperationFailed));
    operation->Parent->TryComplete(operation->Parent, finalErrorCode);
}

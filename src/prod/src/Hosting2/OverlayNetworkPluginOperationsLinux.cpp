//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------
#include "stdafx.h"
#include "cpprest/http_client.h"
#include "cpprest/json.h"
#include "cpprest/uri.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;
using namespace utility;
using namespace web;
using namespace web::http;
using namespace web::http::client;

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

void OverlayNetworkPluginOperations::ExecutePostRequest(std::unique_ptr<web::http::client::http_client> httpClient, string uri, wstring jsonString, AsyncOperationSPtr const & thisSPtr)
{
    uri_builder builder(U(uri));

    string utf8JsonString;
    StringUtility::Utf16ToUtf8(jsonString, utf8JsonString);

    try
    {
        auto httpCreateResponse = httpClient->request(methods::POST, builder.to_string(), (utf8string)utf8JsonString, "application/json").then([this, uri, thisSPtr](pplx::task<http_response> requestTask) -> pplx::task<json::value>
        {
            try
            {
                auto response = requestTask.get();
                auto statuscode = response.status_code();
                if (statuscode == web::http::status_codes::OK)
                {
                    response.headers().set_content_type("application/json");
                    return response.extract_json();
                }
                else
                {
                    WriteWarning(
                        TraceType_OverlayNetworkPluginNetworkOperations,
                        "Network plugin call for uri {0} returned statuscode {1}.",
                        uri,
                        statuscode);
                    thisSPtr->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
                }
            }
            catch (exception const & e)
            {
                WriteError(
                    TraceType_OverlayNetworkPluginNetworkOperations,
                    "Network plugin call for uri {0} http_response error {1}.",
                    uri,
                    e.what());
            }
            return pplx::task_from_result(json::value());
        }).then([this, uri, thisSPtr](pplx::task<json::value> responseTask)
        {
            try
            {
                auto jsonVal = responseTask.get();
                if (jsonVal.is_null())
                {
                    WriteWarning(
                        TraceType_OverlayNetworkPluginNetworkOperations,
                        "Network plugin call for uri {0} failed as json is null.",
                        uri);
                    thisSPtr->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
                    return;
                }
                else
                {
                    auto jsonString = wformatString("{0}", jsonVal.serialize());
                    WriteNoise(
                        TraceType_OverlayNetworkPluginNetworkOperations,
                        "Network plugin call for uri {0} returned json string {1}.",
                        uri,
                        jsonString);

                    QueryResponse queryResponse;
                    auto error = JsonHelper::Deserialize(queryResponse, jsonString);
                    if (!error.IsSuccess())
                    {
                        WriteWarning(
                            TraceType_OverlayNetworkPluginNetworkOperations,
                            "Network plugin call for uri {0} failed as json cannot be deserialized, error {1}.",
                            uri,
                            error);
                        thisSPtr->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
                        return;
                    }
                    else
                    {
                        WriteInfo(
                            TraceType_OverlayNetworkPluginNetworkOperations,
                            "Network plugin call for uri {0} returned statuscode {1} message {2}.",
                            uri,
                            queryResponse.ReturnCode,
                            queryResponse.Message);

                        auto finalErrorCode = (queryResponse.ReturnCode == 0) ? (ErrorCode(ErrorCodeValue::Success)) : (ErrorCode(ErrorCodeValue::OperationFailed));
                        thisSPtr->TryComplete(thisSPtr, finalErrorCode);
                        return;
                    }
                }
            }
            catch (exception const & ex)
            {
                WriteError(
                    TraceType_OverlayNetworkPluginNetworkOperations,
                    "Network plugin call for uri {0} failed with error {1}.",
                    uri,
                    ex.what());
                thisSPtr->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
                return;
            }
        });
    }
    catch (std::exception const & e)
    {
        WriteWarning(
            TraceType_OverlayNetworkPluginNetworkOperations,
            "Failed to execute network plugin call for uri {0} error {1}.",
            uri,
            e.what());
        thisSPtr->TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
    }
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
    WriteInfo(
        TraceType_OverlayNetworkPluginNetworkOperations,
        owner_.Root.TraceId,
        "Overlay network routing info {0}.",
        this->routingInfo_);

    wstring routingInfoString;
    auto error = JsonHelper::Serialize(*this->routingInfo_.get(), routingInfoString);
    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, error);
        return;
    }

    WriteInfo(
        TraceType_OverlayNetworkPluginNetworkOperations,
        owner_.Root.TraceId,
        "Json string used to update routes: {0}.",
        routingInfoString);

    utility::seconds timeout(timeoutHelper_.GetRemainingTime().TotalSeconds());
    http_client_config config;
    config.set_timeout(timeout);
    httpClient_ = make_unique<http_client>(string_t(Constants::NetworkSetup::OverlayNetworkBaseUrl), config);

    owner_.ExecutePostRequest(move(httpClient_), Constants::NetworkSetup::OverlayNetworkUpdateRoutes, routingInfoString, thisSPtr);
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
    WriteInfo(
        TraceType_OverlayNetworkPluginNetworkOperations,
        owner_.Root.TraceId,
        "Attach container to overlay network params {0}.",
        this->params_);

    wstring paramsString;
    auto error = JsonHelper::Serialize(*this->params_.get(), paramsString);
    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, error);
        return;
    }

    WriteInfo(
        TraceType_OverlayNetworkPluginNetworkOperations,
        owner_.Root.TraceId,
        "Json string used to attach container: {0}.",
        paramsString);

    utility::seconds timeout(timeoutHelper_.GetRemainingTime().TotalSeconds());
    http_client_config config;
    config.set_timeout(timeout);
    httpClient_ = make_unique<http_client>(string_t(Constants::NetworkSetup::OverlayNetworkBaseUrl), config);

    owner_.ExecutePostRequest(move(httpClient_), Constants::NetworkSetup::OverlayNetworkAttachContainer, paramsString, thisSPtr);
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
    auto error = JsonHelper::Serialize(*params.get(), paramsString);
    if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, error);
        return;
    }

    WriteInfo(
        TraceType_OverlayNetworkPluginNetworkOperations,
        owner_.Root.TraceId,
        "Json string used to detach container: {0}.",
        paramsString);

    utility::seconds timeout(timeoutHelper_.GetRemainingTime().TotalSeconds());
    http_client_config config;
    config.set_timeout(timeout);
    httpClient_ = make_unique<http_client>(string_t(Constants::NetworkSetup::OverlayNetworkBaseUrl), config);

    owner_.ExecutePostRequest(move(httpClient_), Constants::NetworkSetup::OverlayNetworkDetachContainer, paramsString, thisSPtr);
}
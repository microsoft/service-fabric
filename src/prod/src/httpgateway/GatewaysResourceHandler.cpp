//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Api;
using namespace Query;
using namespace ServiceModel;
using namespace Naming;
using namespace Management::ClusterManager;
using namespace HttpGateway;
using namespace ServiceModel::ModelV2;

StringLiteral const TraceType("GatewaysResourceHandler");

GatewaysResourceHandler::GatewaysResourceHandler(HttpGatewayImpl & server)
    : RequestHandlerBase(server)
{
}

GatewaysResourceHandler::~GatewaysResourceHandler()
{
}

ErrorCode GatewaysResourceHandler::Initialize()
{
    //
    // Resource names are non hierarchical.
    //
    this->allowHierarchicalEntityKeys = false;

    vector<HandlerUriTemplate> uris;

    // /Resources/Gateways/<name>
    uris.push_back(HandlerUriTemplate(
        Constants::GatewaysResourceEntityKeyPath,
        Constants::HttpPutVerb,
        MAKE_HANDLER_CALLBACK(CreateOrUpdateGateway)));

    // /Resources/Gateways/<name>
    uris.push_back(HandlerUriTemplate(
        Constants::GatewaysResourceEntityKeyPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetGatewayByName)));

    // /Resources/Gateways
    uris.push_back(HandlerUriTemplate(
        Constants::GatewaysResourceEntitySetPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetAllGateways)));

    // /Resources/Gateways/<name>
    uris.push_back(HandlerUriTemplate(
        Constants::GatewaysResourceEntityKeyPath,
        Constants::HttpDeleteVerb,
        MAKE_HANDLER_CALLBACK(DeleteGateway)));

    validHandlerUris.insert(validHandlerUris.begin(), uris.begin(), uris.end());

    return server_.InnerServer->RegisterHandler(Constants::GatewaysHandlerPath, shared_from_this());
}

void GatewaysResourceHandler::CreateOrUpdateGateway(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    ModelV2::GatewayResourceDescription description;
    auto error = handlerOperation->Deserialize(description, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    error = argumentParser.TryGetGatewayName(description.Name);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto inner = client.GatewayResourceManagerClient->BeginCreateOrUpdateGatewayResource(
        move(description.ToString()),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnCreateOrUpdateGatewayComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    this->OnCreateOrUpdateGatewayComplete(inner, true);
}

void GatewaysResourceHandler::OnCreateOrUpdateGatewayComplete(AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    wstring descriptionStr;
    auto error = client.GatewayResourceManagerClient->EndCreateOrUpdateGatewayResource(operation, descriptionStr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    ByteBufferUPtr bufferUPtr = make_unique<ByteBuffer>();
    error = Utility::StringToBytesUtf8(descriptionStr, *bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }
    handlerOperation->OnSuccess(thisSPtr, move(bufferUPtr), Constants::StatusAccepted, *Constants::StatusDescriptionAccepted);
}

void GatewaysResourceHandler::DeleteGateway(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    wstring name;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    auto error = argumentParser.TryGetGatewayName(name);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto operation = client.GatewayResourceManagerClient->BeginDeleteGatewayResource(
        name,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operation)
        {
            this->OnDeleteGatewayComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    this->OnDeleteGatewayComplete(operation, true);
}

void GatewaysResourceHandler::OnDeleteGatewayComplete(AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    auto error = client.GatewayResourceManagerClient->EndDeleteGatewayResource(operation);
    if (!error.IsSuccess())
    {
        if (error.IsError(ErrorCodeValue::NotFound))
        {
            ByteBufferUPtr emptyBody;
            handlerOperation->OnSuccess(thisSPtr, move(emptyBody), Constants::StatusNoContent, *Constants::StatusDescriptionNoContent);
            return;
        }
        else
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(thisSPtr, move(emptyBody), Constants::StatusAccepted, *Constants::StatusDescriptionAccepted);
}

void GatewaysResourceHandler::GetAllGateways(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    UriArgumentParser argumentParser(handlerOperation->Uri);

    GatewayResourceQueryDescription queryDescription;
    QueryPagingDescription pagingDescription;
    auto error = argumentParser.TryGetPagingDescription(pagingDescription);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }
    queryDescription.QueryPagingDescriptionUPtr = make_unique<QueryPagingDescription>(move(pagingDescription));

    AsyncOperationSPtr operation = client.GatewayResourceManagerClient->BeginGetGatewayResourceList(
        move(queryDescription),
        handlerOperation->Timeout,
        [this] (AsyncOperationSPtr const& operation)
        {
            this->OnGetAllGatewaysComplete(operation, false);
        },
        thisSPtr);

    OnGetAllGatewaysComplete(operation, true);
}

void GatewaysResourceHandler::OnGetAllGatewaysComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<wstring> gatewaysResult;
    PagingStatusUPtr pagingStatus;
    auto error = client.GatewayResourceManagerClient->EndGetGatewayResourceList(operation, gatewaysResult, pagingStatus);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    GatewayResourceDescriptionList list;
    if (pagingStatus)
    {
        list.ContinuationToken = pagingStatus->TakeContinuationToken();
    }

    vector<GatewayResourceDescription> gatewayDescriptionList;
    for (auto& gatewayStr : gatewaysResult)
    {
        GatewayResourceDescription description;
        error = GatewayResourceDescription::FromString(gatewayStr, description);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(operation->Parent, error);
            return;
        }
        list.Items.push_back(move(description));
    }
 
    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(list, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void GatewaysResourceHandler::GetGatewayByName(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    GatewayResourceQueryDescription queryDescription;
    wstring gatewayName;
    auto error = argumentParser.TryGetGatewayName(gatewayName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }
    queryDescription.GatewayNameFilter = move(gatewayName);

    AsyncOperationSPtr operation = client.GatewayResourceManagerClient->BeginGetGatewayResourceList(
        move(queryDescription),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetGatewayByNameComplete(operation, false);
    },
        thisSPtr);

    this->OnGetGatewayByNameComplete(operation, true);
}

void GatewaysResourceHandler::OnGetGatewayByNameComplete(AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    vector<wstring> gatewaysResult;
    PagingStatusUPtr pagingStatus;
    auto error = client.GatewayResourceManagerClient->EndGetGatewayResourceList(operation, gatewaysResult, pagingStatus);

    // We shouldn't receive paging status for just one gateway
    TESTASSERT_IF(pagingStatus, "OnGetGatewayByNameComplete: paging status shouldn't be set");

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    if (gatewaysResult.size() == 0)
    {
        handlerOperation->OnSuccess(
            thisSPtr,
            move(bufferUPtr),
            Constants::StatusNoContent,
            Constants::StatusDescriptionNoContent);
        return;
    }

    if (gatewaysResult.size() != 1)
    {
        UriArgumentParser argumentParser(handlerOperation->Uri);
        wstring gatewayName;
        argumentParser.TryGetGatewayName(gatewayName);

        handlerOperation->OnError(
            thisSPtr,
            ErrorCode(ErrorCodeValue::OperationFailed, wformatString(GET_HTTP_GATEWAY_RC(Multiple_Entries), gatewayName)));
        return;
    }

    bufferUPtr = make_unique<ByteBuffer>();
    error = Utility::StringToBytesUtf8(gatewaysResult[0], *bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    handlerOperation->OnSuccess(thisSPtr, move(bufferUPtr));
}

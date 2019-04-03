// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Api;
using namespace HttpGateway;
using namespace ServiceModel;
using namespace Query;

StringLiteral const TraceType("NetworksHandler");

NetworksHandler::NetworksHandler(HttpGatewayImpl & server)
    : RequestHandlerBase(server)
{
}

NetworksHandler::~NetworksHandler()
{
}

ErrorCode NetworksHandler::Initialize()
{
    //
    // Resource names are non hierarchical.
    //
    this->allowHierarchicalEntityKeys = false;

    // GET /Resources/Networks/
    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::NetworksEntitySetPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetAllNetworks),
        Constants::V64ApiVersion));

    // GET /Resources/Networks/{NetworkName}/
    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::NetworksEntityKeyPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetNetworkByName),
        Constants::V64ApiVersion));

    // GET /Resources/Networks/{NetworkName}/ApplicationRefs/
    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::ApplicationsEntitySetPathViaNetwork,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetAllNetworkApplications),
        Constants::V64ApiVersion));

    // GET /Resources/Networks/{NetworkName}/ApplicationRefs/{ApplicationId}/
    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::ApplicationsEntityKeyPathViaNetwork,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetNetworkApplicationByName),
        Constants::V64ApiVersion));

    // GET /Resources/Networks/{NetworkName}/DeployedNodes/
    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::NodesEntitySetPathViaNetwork,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetAllNetworkNodes),
        Constants::V64ApiVersion));

    // GET /Resources/Networks/{NetworkName}/DeployedNodes/{NodeId}/
    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::NodesEntityKeyPathViaNetwork,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetNetworkNodeByName),
        Constants::V64ApiVersion));

    // PUT /Resources/Networks/{NetworkName}/
    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::NetworksEntityKeyPath,
        Constants::HttpPutVerb,
        MAKE_HANDLER_CALLBACK(CreateNetwork),
        Constants::V64ApiVersion));

    // DELETE /Resources/Networks/{NetworkName}/
    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::NetworksEntityKeyPath,
        Constants::HttpDeleteVerb,
        MAKE_HANDLER_CALLBACK(DeleteNetwork),
        Constants::V64ApiVersion));

    return server_.InnerServer->RegisterHandler(Constants::NetworksHandlerPath, shared_from_this());
}

void NetworksHandler::GetAllNetworks(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    NetworkQueryDescription networkQueryDescription;
    bool success = GetNetworkQueryDescription(thisSPtr, false, networkQueryDescription);
    if (!success)
    {
        return;
    }

    AsyncOperationSPtr operation = client.NetworkMgmtClient->BeginGetNetworkList(
        move(networkQueryDescription),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnGetAllNetworksComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnGetAllNetworksComplete(operation, true);
}

void NetworksHandler::OnGetAllNetworksComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<ModelV2::NetworkResourceDescriptionQueryResult> networksResult;
    PagingStatusUPtr pagingStatus;
    auto error = client.NetworkMgmtClient->EndGetNetworkList(operation, networksResult, pagingStatus);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ModelV2::NetworkResourceDescriptionQueryResultList networkList;
    if (pagingStatus)
    {
        networkList.ContinuationToken = pagingStatus->TakeContinuationToken();
    }

    networkList.Items = move(networksResult);

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(networkList, bufferUPtr);

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void NetworksHandler::GetNetworkByName(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    NetworkQueryDescription networkQueryDescription;
    bool success = GetNetworkQueryDescription(thisSPtr, true, networkQueryDescription);
    if (!success)
    {
        return;
    }

    AsyncOperationSPtr operation = client.NetworkMgmtClient->BeginGetNetworkList(
        move(networkQueryDescription),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetNetworkByNameComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnGetNetworkByNameComplete(operation, true);
}

void NetworksHandler::OnGetNetworkByNameComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<ModelV2::NetworkResourceDescriptionQueryResult> networksResult;
    PagingStatusUPtr pagingStatus;
    auto error = client.NetworkMgmtClient->EndGetNetworkList(operation, networksResult, pagingStatus);

    TESTASSERT_IF(pagingStatus, "OnGetNetworkByNameComplete: paging status shouldn't be set");

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    if (networksResult.size() == 0)
    {
        handlerOperation->OnSuccess(
            operation->Parent,
            move(bufferUPtr),
            Constants::StatusNoContent,
            Constants::StatusDescriptionNoContent);

        return;
    }

    if (networksResult.size() != 1)
    {
        handlerOperation->OnError(operation->Parent, ErrorCodeValue::OperationFailed);
        return;
    }

    error = handlerOperation->Serialize(networksResult[0], bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

bool NetworksHandler::GetNetworkQueryDescription(
    Common::AsyncOperationSPtr const& thisSPtr,
    bool includeNetworkName,
    __out NetworkQueryDescription & queryDescription)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    UriArgumentParser argumentParser(handlerOperation->Uri);
    
    ErrorCode error;    
    if (includeNetworkName) // GetNetworkInfo
    {
        wstring networkName;
        error = argumentParser.TryGetNetworkName(networkName);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return false;
        }

        queryDescription.NetworkNameFilter = move(networkName);
    }
    else // GetNetworkInfoList
    {        
        QueryPagingDescription pagingDescription;
        error = argumentParser.TryGetPagingDescription(pagingDescription);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return false;
        }

        queryDescription.QueryPagingDescriptionObject = make_unique<QueryPagingDescription>(move(pagingDescription));

        DWORD networkStatusFilter = FABRIC_NETWORK_STATUS_FILTER_DEFAULT;
        error = argumentParser.TryGetNetworkStatusFilter(networkStatusFilter);
        if (error.IsSuccess())
        {
            queryDescription.NetworkStatusFilter = networkStatusFilter;
        }
        else if (error.IsError(ErrorCodeValue::NameNotFound))
        {
            error = ErrorCode::Success();
        }

        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return false;
        }
    }

    return true;
}

void NetworksHandler::GetAllNetworkApplications(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    NetworkApplicationQueryDescription networkApplicationQueryDescription;
    bool success = GetNetworkApplicationQueryDescription(thisSPtr, false, networkApplicationQueryDescription);
    if (!success)
    {
        return;
    }

    AsyncOperationSPtr operation = client.NetworkMgmtClient->BeginGetNetworkApplicationList(
        move(networkApplicationQueryDescription),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetAllNetworkApplicationsComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnGetAllNetworkApplicationsComplete(operation, true);
}

void NetworksHandler::OnGetAllNetworkApplicationsComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<NetworkApplicationQueryResult> networkApplications;
    PagingStatusUPtr pagingStatus;
    auto error = client.NetworkMgmtClient->EndGetNetworkApplicationList(operation, networkApplications, pagingStatus);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    NetworkApplicationList networkApplicationList;
    if (pagingStatus)
    {
        networkApplicationList.ContinuationToken = pagingStatus->TakeContinuationToken();
    }

    networkApplicationList.Items = move(networkApplications);

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(networkApplicationList, bufferUPtr);

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void NetworksHandler::GetNetworkApplicationByName(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    NetworkApplicationQueryDescription networkApplicationQueryDescription;
    bool success = GetNetworkApplicationQueryDescription(thisSPtr, true, networkApplicationQueryDescription);
    if (!success)
    {
        return;
    }

    AsyncOperationSPtr operation = client.NetworkMgmtClient->BeginGetNetworkApplicationList(
        move(networkApplicationQueryDescription),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetNetworkApplicationByNameComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnGetNetworkApplicationByNameComplete(operation, true);
}

void NetworksHandler::OnGetNetworkApplicationByNameComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<NetworkApplicationQueryResult> networkApplications;
    PagingStatusUPtr pagingStatus;
    auto error = client.NetworkMgmtClient->EndGetNetworkApplicationList(operation, networkApplications, pagingStatus);

    TESTASSERT_IF(pagingStatus, "OnGetNetworkApplicationByNameComplete: paging status shouldn't be set");

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    if (networkApplications.size() == 0)
    {
        handlerOperation->OnSuccess(
            operation->Parent,
            move(bufferUPtr),
            Constants::StatusNoContent,
            Constants::StatusDescriptionNoContent);

        return;
    }

    if (networkApplications.size() != 1)
    {
        handlerOperation->OnError(operation->Parent, ErrorCodeValue::OperationFailed);
        return;
    }

    error = handlerOperation->Serialize(networkApplications[0], bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

bool NetworksHandler::GetNetworkApplicationQueryDescription(
    Common::AsyncOperationSPtr const& thisSPtr,
    bool includeApplicationName,
    __out NetworkApplicationQueryDescription & queryDescription)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    UriArgumentParser argumentParser(handlerOperation->Uri);

    ErrorCode error;
    wstring networkName;
    error = argumentParser.TryGetNetworkName(networkName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return false;
    }

    queryDescription.NetworkName = move(networkName);

    if (includeApplicationName) // GetNetworkApplicationInfo
    {
        NamingUri applicationName;
        error = argumentParser.TryGetApplicationName(applicationName);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return false;
        }

        queryDescription.ApplicationNameFilter = move(applicationName);
    }
    else // GetNetworkApplicationList
    {
        QueryPagingDescription pagingDescription;
        error = argumentParser.TryGetPagingDescription(pagingDescription);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return false;
        }

        queryDescription.QueryPagingDescriptionObject = make_unique<QueryPagingDescription>(move(pagingDescription));
    }

    return true;
}

void NetworksHandler::GetAllNetworkNodes(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    NetworkNodeQueryDescription networkNodeQueryDescription;
    bool success = GetNetworkNodeQueryDescription(thisSPtr, false, networkNodeQueryDescription);
    if (!success)
    {
        return;
    }

    AsyncOperationSPtr operation = client.NetworkMgmtClient->BeginGetNetworkNodeList(
        move(networkNodeQueryDescription),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetAllNetworkNodesComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnGetAllNetworkNodesComplete(operation, true);
}

void NetworksHandler::OnGetAllNetworkNodesComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<NetworkNodeQueryResult> networkNodes;
    PagingStatusUPtr pagingStatus;
    auto error = client.NetworkMgmtClient->EndGetNetworkNodeList(operation, networkNodes, pagingStatus);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    NetworkNodeList networkNodeList;
    if (pagingStatus)
    {
        networkNodeList.ContinuationToken = pagingStatus->TakeContinuationToken();
    }

    networkNodeList.Items = move(networkNodes);

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(networkNodeList, bufferUPtr);

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void NetworksHandler::GetNetworkNodeByName(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    NetworkNodeQueryDescription networkNodeQueryDescription;
    bool success = GetNetworkNodeQueryDescription(thisSPtr, true, networkNodeQueryDescription);
    if (!success)
    {
        return;
    }

    AsyncOperationSPtr operation = client.NetworkMgmtClient->BeginGetNetworkNodeList(
        move(networkNodeQueryDescription),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetNetworkNodeByNameComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnGetNetworkNodeByNameComplete(operation, true);
}

void NetworksHandler::OnGetNetworkNodeByNameComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<NetworkNodeQueryResult> networkNodes;
    PagingStatusUPtr pagingStatus;
    auto error = client.NetworkMgmtClient->EndGetNetworkNodeList(operation, networkNodes, pagingStatus);

    TESTASSERT_IF(pagingStatus, "OnGetNetworkNodeByNameComplete: paging status shouldn't be set");

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    if (networkNodes.size() == 0)
    {
        handlerOperation->OnSuccess(
            operation->Parent,
            move(bufferUPtr),
            Constants::StatusNoContent,
            Constants::StatusDescriptionNoContent);

        return;
    }

    if (networkNodes.size() != 1)
    {
        handlerOperation->OnError(operation->Parent, ErrorCodeValue::OperationFailed);
        return;
    }

    error = handlerOperation->Serialize(networkNodes[0], bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

bool NetworksHandler::GetNetworkNodeQueryDescription(
    Common::AsyncOperationSPtr const& thisSPtr,
    bool includeNodeName,
    __out NetworkNodeQueryDescription & queryDescription)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    UriArgumentParser argumentParser(handlerOperation->Uri);

    ErrorCode error;
    wstring networkName;
    error = argumentParser.TryGetNetworkName(networkName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return false;
    }

    queryDescription.NetworkName = move(networkName);

    if (includeNodeName) // GetNetworkNodeInfo
    {
        wstring nodeName;
        error = argumentParser.TryGetNodeName(nodeName);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return false;
        }

        queryDescription.NodeNameFilter = move(nodeName);
    }
    else // GetNetworkNodeList
    {
        QueryPagingDescription pagingDescription;
        error = argumentParser.TryGetPagingDescription(pagingDescription);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return false;
        }

        queryDescription.QueryPagingDescriptionObject = make_unique<QueryPagingDescription>(move(pagingDescription));
    }

    return true;
}

void NetworksHandler::CreateNetwork(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    ModelV2::NetworkResourceDescription networkResourceDescription;
    auto error = handlerOperation->Deserialize(networkResourceDescription, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    // The network name specified in request uri will replace the name in network description in request body.
    wstring networkName;
    error = argumentParser.TryGetNetworkName(networkResourceDescription.Name);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    error = networkResourceDescription.TryValidate(L"");
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto inner = client.NetworkMgmtClient->BeginCreateNetwork(
        networkResourceDescription,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnCreateNetworkComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnCreateNetworkComplete(inner, true);
}

void NetworksHandler::OnCreateNetworkComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.NetworkMgmtClient->EndCreateNetwork(operation);

    // This is to allow multiple calls for create for the dev scenario, this is a workaround for now.
    // TODO: Remove when update network is implemented
    if (error.IsError(ErrorCodeValue::NameAlreadyExists))
    {
        ByteBufferUPtr emptyBody;
        handlerOperation->OnSuccess(thisSPtr, move(emptyBody), Constants::StatusAccepted, *Constants::StatusDescriptionAccepted);
        return;
    }

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody), Constants::StatusCreated, *Constants::StatusDescriptionCreated);
}

void NetworksHandler::DeleteNetwork(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring networkName;
    auto error = argumentParser.TryGetNetworkName(networkName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto inner = client.NetworkMgmtClient->BeginDeleteNetwork(
        DeleteNetworkDescription(move(networkName)),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnDeleteNetworkComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnDeleteNetworkComplete(inner, true);
}

void NetworksHandler::OnDeleteNetworkComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.NetworkMgmtClient->EndDeleteNetwork(operation);

    if (!error.IsSuccess())
    {
        if (error.IsError(ErrorCodeValue::NetworkNotFound))
        {
            ByteBufferUPtr emptyBody;
            handlerOperation->OnSuccess(thisSPtr, move(emptyBody), Constants::StatusNoContent, *Constants::StatusDescriptionNoContent);
            return;
        }
        else
        {
            handlerOperation->OnError(operation->Parent, error);
            return;
        }
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}
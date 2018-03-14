// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Api;
using namespace HttpGateway;
using namespace Management::FaultAnalysisService;
using namespace ServiceModel;
using namespace Query;

StringLiteral const TraceType("NodesHandler");

NodesHandler::NodesHandler(HttpGatewayImpl & server)
    : RequestHandlerBase(server)
{
}

NodesHandler::~NodesHandler()
{
}

ErrorCode NodesHandler::Initialize()
{
    // V2apiversion onwards we support continuation token
    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::NodesEntitySetPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetAllNodes)));

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::NodesEntityKeyPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetNodeByName)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::NodesEntityKeyPath, Constants::GetLoadInformation),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetNodeLoadInformation)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::NodesEntityKeyPath, Constants::Activate),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(ActivateNode)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::NodesEntityKeyPath, Constants::Deactivate),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(DeactivateNode)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::NodesEntityKeyPath, Constants::RemoveNodeState),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(NodeStateRemoved)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::NodesEntityKeyPath, Constants::GetHealth),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(EvaluateNodeHealth)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::NodesEntityKeyPath, Constants::GetHealth),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(EvaluateNodeHealth)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::NodesEntityKeyPath, Constants::ReportHealth),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(ReportNodeHealth)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::NodesEntityKeyPath, Constants::Restart),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(RestartNode)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::NodesEntityKeyPath, Constants::Stop),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(StopNode)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::NodesEntityKeyPath, Constants::Start),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(StartNode)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::CodePackagesOnNodeEntitySetPath, Constants::Restart),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(RestartDeployedCodePackage)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::CodePackagesOnNodeEntitySetPath, Constants::ContainerLogs),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetContainerLogs)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::CodePackagesOnNodeEntitySetPath, Constants::ContainerApi),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(CallContainerApi)));

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::ApplicationsOnNodeEntitySetPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(ApplicationsOnNode)));

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::ApplicationsOnNodeEntityKeyPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(ApplicationsOnNodeByName)));

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::ServicePackagesOnNodeEntitySetPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(ServicePackagesOnNode)));

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::ServicePackagesOnNodeEntityKeyPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(ServicePackagesOnNodeByName)));

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::ServiceTypesOnNodeEntitySetPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(ServiceTypesOnNode)));

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::ServiceTypesOnNodeEntityKeyPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(ServiceTypesOnNodeByType)));

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::CodePackagesOnNodeEntitySetPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(CodePackagesOnNode)));

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::ServiceReplicasOnNodeEntitySetPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(ServiceReplicasOnNode)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ApplicationsOnNodeEntityKeyPath, Constants::GetHealth),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(EvaluateApplicationOnNodeHealth)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ApplicationsOnNodeEntityKeyPath, Constants::GetHealth),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(EvaluateApplicationOnNodeHealth)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ApplicationsOnNodeEntityKeyPath, Constants::ReportHealth),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(ReportApplicationOnNodeHealth)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePackagesOnNodeEntityKeyPath, Constants::GetHealth),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(EvaluateServicePackagesOnNodeHealth)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePackagesOnNodeEntityKeyPath, Constants::GetHealth),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(EvaluateServicePackagesOnNodeHealth)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePackagesOnNodeEntityKeyPath, Constants::ReportHealth),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(ReportServicePackagesOnNodeHealth)));

    // Delete Replica is actually Remove Replica. Delete replica has been effectively deprecated by not documenting in swagger.
    // Even though deprecated we need to keep the implementation for backward compat
    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServiceReplicaEntityKeyPathViaPartitionViaNode, Constants::Delete),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(RemoveReplica)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServiceReplicaEntityKeyPathViaPartitionViaNode, Constants::Remove),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(RemoveReplica)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServiceReplicaEntityKeyPathViaPartitionViaNode, Constants::Restart),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(RestartReplica)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServicePartitionEntityKeyPathViaNode, Constants::GetReplicas),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(ServiceReplicasOnHost)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ServiceReplicaEntityKeyPathViaPartitionViaNode, Constants::GetDeployedReplicaDetail),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(ServiceReplicasOnHost)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::NodesEntityKeyPath, Constants::DeployServicePackageToNode),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(DeployServicePackageToNode)));

    return server_.InnerServer->RegisterHandler(Constants::NodesHandlerPath, shared_from_this());
}

void NodesHandler::GetAllNodes(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    UriArgumentParser argumentParser(handlerOperation->Uri);

    // Starting with Constants::V2ApiVersion, input can specify continuation token
    wstring continuationToken;
    if (handlerOperation->Uri.ApiVersion != Constants::V1ApiVersion)
    {
        auto error = argumentParser.TryGetContinuationToken(continuationToken);
        if (error.IsError(ErrorCodeValue::NameNotFound))
        {
            error = ErrorCode::Success();
        }

        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }
    }

    DWORD nodeStatus = FABRIC_QUERY_NODE_STATUS_FILTER_DEFAULT;
    auto error = argumentParser.TryGetNodeStatusFilter(nodeStatus);
    if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::NameNotFound))
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    AsyncOperationSPtr operation = client.QueryClient->BeginGetNodeList(
        EMPTY_STRING_QUERY_FILTER,
        nodeStatus,
        false,
        continuationToken,
        handlerOperation->Timeout,
        [this] (AsyncOperationSPtr const& operation)
        {
            this->OnGetAllNodesComplete(operation, false);
        },
        thisSPtr);

    OnGetAllNodesComplete(operation, true);
}

void NodesHandler::OnGetAllNodesComplete(
    __in AsyncOperationSPtr const &operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<NodeQueryResult> nodesResult;
    PagingStatusUPtr pagingStatus;
    auto error = client.QueryClient->EndGetNodeList(operation, nodesResult, pagingStatus);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr = make_unique<ByteBuffer>();
    if (handlerOperation->Uri.ApiVersion == Constants::V1ApiVersion)
    {
        error = handlerOperation->Serialize(nodesResult, bufferUPtr);
    }
    else
    {
        NodeList list;
        if (pagingStatus)
        {
            list.ContinuationToken = pagingStatus->TakeContinuationToken();
        }

        list.Items = move(nodesResult);

        error = handlerOperation->Serialize(list, bufferUPtr);
    }

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void NodesHandler::GetNodeByName(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring nodeName;
    auto error = argumentParser.TryGetNodeName(nodeName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    AsyncOperationSPtr operation = client.QueryClient->BeginGetNodeList(
        nodeName,
        EMPTY_STRING_QUERY_FILTER, //continuation token
        handlerOperation->Timeout,
        [this] (AsyncOperationSPtr const& operation)
        {
            this->OnGetNodeByNameComplete(operation, false);
        },
        thisSPtr);

    OnGetNodeByNameComplete(operation, true);
}

void NodesHandler::OnGetNodeByNameComplete(
    __in AsyncOperationSPtr const &operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<NodeQueryResult> nodesResult;
    PagingStatusUPtr pagingStatus;
    auto error = client.QueryClient->EndGetNodeList(operation, nodesResult, pagingStatus);

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    // We shouldn't receive paging status for just one node
    TESTASSERT_IF(pagingStatus, "OnGetNodeByNameComplete: paging status shouldn't be set");

    ByteBufferUPtr bufferUPtr;
    if (nodesResult.size() == 0)
    {
        handlerOperation->OnSuccess(
            operation->Parent,
            move(bufferUPtr),
            Constants::StatusNoContent,
            Constants::StatusDescriptionNoContent);

        return;
    }

    error = handlerOperation->Serialize(nodesResult[0], bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void NodesHandler::StopNode(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring nodeName;
    auto error = argumentParser.TryGetNodeName(nodeName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    NodeControlData data;
    error = handlerOperation->Deserialize(data, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    AsyncOperationSPtr inner = client.ClusterMgmtClient->BeginStopNode(
        nodeName,
        data.NodeInstanceId,
        false/*Don't Restart*/,
        false/*Don't Create Fabric Dumps*/,
        handlerOperation->Timeout,
        [this] (AsyncOperationSPtr const& operation)
        {
            this->OnStopNodeComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnStopNodeComplete(inner, true);
}

void NodesHandler::OnStopNodeComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.ClusterMgmtClient->EndStopNode(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void NodesHandler::RestartDeployedCodePackage(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring nodeNameString;
    auto error = argumentParser.TryGetNodeName(nodeNameString);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    NamingUri appNameUri;
    error = argumentParser.TryGetApplicationName(appNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    CodePackageControlData data;
    error = handlerOperation->Deserialize(data, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    FABRIC_INSTANCE_ID codePackageInstanceId;
    if (!StringUtility::TryFromWString(data.CodePackageInstanceId, codePackageInstanceId))
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    RestartDeployedCodePackageDescriptionUsingNodeName restartData(
        nodeNameString,
        appNameUri,
        data.ServiceManifestName,
        data.ServicePackageActivationId,
        data.CodePackageName,
        codePackageInstanceId);

    AsyncOperationSPtr inner = client.FaultMgmtClient->BeginRestartDeployedCodePackage(
        restartData,
        handlerOperation->Timeout,
        [this, restartData] (AsyncOperationSPtr const& operation)
        {
            this->OnRestartDeployedCodePackageComplete(operation, false, restartData);
        },
        handlerOperation->shared_from_this());

    OnRestartDeployedCodePackageComplete(inner, true, restartData);
}

void NodesHandler::OnRestartDeployedCodePackageComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously,
    __in RestartDeployedCodePackageDescriptionUsingNodeName const & description)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    IRestartDeployedCodePackageResultPtr result;
    auto error = client.FaultMgmtClient->EndRestartDeployedCodePackage(operation, description, result);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void NodesHandler::CallContainerApi(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring nodeNameString;
    auto error = argumentParser.TryGetNodeName(nodeNameString);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    NamingUri appNameUri;
    error = argumentParser.TryGetApplicationName(appNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    wstring serviceManifestNameFilter, codePackageNameFilter;
    handlerOperation->Uri.GetItem(Constants::ServiceManifestNameString, serviceManifestNameFilter);
    handlerOperation->Uri.GetItem(Constants::CodePackageNameIdString, codePackageNameFilter);

    unique_ptr<ContainerApiCall> containerApiCall;
    if (handlerOperation->Uri.Verb == Constants::HttpPostVerb && !handlerOperation->Body->empty())
    {
        containerApiCall = make_unique<ContainerApiCall>();

        error = handlerOperation->Deserialize(*containerApiCall, handlerOperation->Body);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(
                thisSPtr,
                ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Deserialization_Error), L"CallContainerApi")));
            return;
        }
    }

    ContainerInfoArgMap containerInfoArgMap;
    containerInfoArgMap.Insert(*StringConstants::HttpVerb, containerApiCall->HttpVerb());
    containerInfoArgMap.Insert(*StringConstants::UriPath, containerApiCall->UriPath());
    containerInfoArgMap.Insert(*StringConstants::HttpContentType, containerApiCall->ContentType());
    containerInfoArgMap.Insert(*StringConstants::HttpRequestBody, containerApiCall->RequestBody());

    AsyncOperationSPtr inner = client.AppMgmtClient->BeginInvokeContainerApi(
        nodeNameString,
        appNameUri,
        serviceManifestNameFilter,
        codePackageNameFilter,
        containerInfoArgMap,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation) { OnContainerApiComplete(operation, false); },
        handlerOperation->shared_from_this());

    OnContainerApiComplete(inner, true);
}

void NodesHandler::OnContainerApiComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    wstring callResult;
    auto error = client.AppMgmtClient->EndInvokeContainerApi(operation, callResult);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    string requestUtf8;
    StringUtility::Utf16ToUtf8(callResult, requestUtf8);

    auto responseBody = make_unique<ByteBuffer>(requestUtf8.data(), requestUtf8.data() + requestUtf8.size());
    handlerOperation->OnSuccess(operation->Parent, move(responseBody));
}

void NodesHandler::GetContainerLogs(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring nodeNameString;
    auto error = argumentParser.TryGetNodeName(nodeNameString);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    NamingUri appNameUri;
    error = argumentParser.TryGetApplicationName(appNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    wstring serviceManifestNameFilter, codePackageNameFilter, tailArgument;
    handlerOperation->Uri.GetItem(Constants::ServiceManifestNameString, serviceManifestNameFilter);
    handlerOperation->Uri.GetItem(Constants::CodePackageNameIdString, codePackageNameFilter);
    handlerOperation->Uri.GetItem(ContainerInfoArgs::Tail, tailArgument);
    
    ContainerInfoArgMap containerInfoArgMap;
    containerInfoArgMap.Insert(ContainerInfoArgs::Tail, tailArgument);

    AsyncOperationSPtr inner = client.AppMgmtClient->BeginGetContainerInfo(
        nodeNameString,
        appNameUri,
        serviceManifestNameFilter,
        codePackageNameFilter,
        StringUtility::ToWString(ContainerInfoType::Enum::Logs),
        containerInfoArgMap,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetContainerLogsComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnGetContainerLogsComplete(inner, true);
}

void NodesHandler::OnGetContainerLogsComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    wstring containerInfo;
    auto error = client.AppMgmtClient->EndGetContainerInfo(operation, containerInfo);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ContainerInfoData result(move(containerInfo));
    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(result, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void NodesHandler::RestartNode(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring nodeName;
    auto error = argumentParser.TryGetNodeName(nodeName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    RestartNodeControlData data;
    error = handlerOperation->Deserialize(data, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    bool isCreateFabricDump = data.CreateFabricDump == L"True" ? true : false;
    AsyncOperationSPtr inner = client.ClusterMgmtClient->BeginStopNode(
        nodeName,
        data.NodeInstanceId,
        true/*Restart*/,
        isCreateFabricDump,
        handlerOperation->Timeout,
        [this] (AsyncOperationSPtr const& operation)
    {
        this->OnRestartNodeComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnRestartNodeComplete(inner, true);
}

void NodesHandler::OnRestartNodeComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.ClusterMgmtClient->EndStopNode(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void NodesHandler::StartNode(__in AsyncOperationSPtr const & thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring nodeName;
    auto error = argumentParser.TryGetNodeName(nodeName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    StartNodeData data;
    error = handlerOperation->Deserialize(data, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    if (!data.Validate())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    AsyncOperationSPtr inner = client.ClusterMgmtClient->BeginStartNode(
        nodeName,
        data.NodeInstanceId,
        data.IpAddressOrFQDN,
        data.ClusterConnectionPort,
        handlerOperation->Timeout,
        [this] (AsyncOperationSPtr const & operation)
        {
            this->OnStartNodeComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnStartNodeComplete(inner, true);
}

void NodesHandler::OnStartNodeComplete(
    __in AsyncOperationSPtr const & operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.ClusterMgmtClient->EndStartNode(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void NodesHandler::ActivateNode(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring nodeName;
    auto error = argumentParser.TryGetNodeName(nodeName);
    if (!error.IsSuccess())
    {
        return handlerOperation->OnError(thisSPtr, error);
    }

    AsyncOperationSPtr inner = client.ClusterMgmtClient->BeginActivateNode(
        nodeName,
        handlerOperation->Timeout,
        [this] (AsyncOperationSPtr const& operation)
        {
            this->OnActivateNodeComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnActivateNodeComplete(inner, true);
}

void NodesHandler::OnActivateNodeComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.ClusterMgmtClient->EndActivateNode(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void NodesHandler::DeactivateNode(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring nodeName;
    auto error = argumentParser.TryGetNodeName(nodeName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    DeactivateNodeData data;
    error = handlerOperation->Deserialize(data, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    auto inner = client.ClusterMgmtClient->BeginDeactivateNode(
        nodeName,
        data.DeactivationIntent,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnDeactivateNodeComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnDeactivateNodeComplete(inner, true);
}

void NodesHandler::OnDeactivateNodeComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    auto error = client.ClusterMgmtClient->EndDeactivateNode(operation);

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void NodesHandler::NodeStateRemoved(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring nodeName;
    auto error = argumentParser.TryGetNodeName(nodeName);
    if (!error.IsSuccess())
    {
        return handlerOperation->OnError(thisSPtr, error);
    }

    AsyncOperationSPtr inner = client.ClusterMgmtClient->BeginNodeStateRemoved(
        nodeName,
        handlerOperation->Timeout,
        [this] (AsyncOperationSPtr const& operation)
        {
            this->OnNodeStateRemovedComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnNodeStateRemovedComplete(inner, true);
}

void NodesHandler::OnNodeStateRemovedComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    QueryResult result;
    auto error = client.ClusterMgmtClient->EndNodeStateRemoved(operation);

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void NodesHandler::ApplicationsOnNode(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    auto apiVersion = handlerOperation->Uri.ApiVersion;
    if (apiVersion > Constants::V6ApiVersion)
    {
        GetDeployedApplicationPagedList(thisSPtr, handlerOperation, client, false);
    }
    else
    {
        // Unpaged version

        // The paged version takes care of TryGetNodeName, so we only need to place it here.
        UriArgumentParser argumentParser(handlerOperation->Uri);
        wstring nodeName;
        auto error = argumentParser.TryGetNodeName(nodeName);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        AsyncOperationSPtr inner = client.QueryClient->BeginGetDeployedApplicationList(
            nodeName,
            EMPTY_URI_QUERY_FILTER,
            handlerOperation->Timeout,
            [this](AsyncOperationSPtr const& operation)
        {
            this->OnApplicationsOnNodeComplete(operation, false);
        },
            handlerOperation->shared_from_this());

        OnApplicationsOnNodeComplete(inner, true);
    }
}

void NodesHandler::OnApplicationsOnNodeComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<DeployedApplicationQueryResult> applicationsResult;
    auto error = client.QueryClient->EndGetDeployedApplicationList(operation, applicationsResult);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    vector<DeployedApplicationQueryResultWrapper> applicationResultWrapper;
    for (size_t i = 0; i < applicationsResult.size(); i++)
    {
        DeployedApplicationQueryResultWrapper wrapper(applicationsResult[i]);
        applicationResultWrapper.push_back(move(wrapper));
    }

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(applicationResultWrapper, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

// This does the following
//      1. Read incoming information
//      2. Calls method to begin query
//      3. Calls OnComplete method
void NodesHandler::GetDeployedApplicationPagedList(
    Common::AsyncOperationSPtr const & thisSPtr,
    HandlerAsyncOperation * const handlerOperation,
    FabricClientWrapper const & client,
    bool expectApplicationName)
{
    UriArgumentParser argumentParser(handlerOperation->Uri);
    ServiceModel::DeployedApplicationQueryDescription queryDescription;

    wstring nodeName;
    auto error = argumentParser.TryGetNodeName(nodeName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    queryDescription.NodeName = move(nodeName);

    if (expectApplicationName)
    {
        NamingUri appNameUri;
        error = argumentParser.TryGetApplicationName(appNameUri);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        queryDescription.ApplicationName = move(appNameUri);
    }

    bool includeHealthState;
    error = argumentParser.GetIncludeHealthState(includeHealthState);
    if (error.IsSuccess())
    {
        queryDescription.IncludeHealthState = includeHealthState;
    }
    else if (!error.IsError(ErrorCodeValue::NotFound))
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    if (expectApplicationName)
    {
        AsyncOperationSPtr inner = client.QueryClient->BeginGetDeployedApplicationPagedList(
            queryDescription,
            handlerOperation->Timeout,
            [this](AsyncOperationSPtr const& operation)
        {
            this->OnApplicationsOnNodePagedComplete(operation, false, true);
        },
            handlerOperation->shared_from_this());

        OnApplicationsOnNodePagedComplete(inner, true, true);
    }
    else
    {
        ServiceModel::QueryPagingDescription pagingDescription;
        error = argumentParser.TryGetPagingDescription(pagingDescription);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }
        queryDescription.QueryPagingDescriptionObject = make_unique<QueryPagingDescription>(move(pagingDescription));

        AsyncOperationSPtr inner = client.QueryClient->BeginGetDeployedApplicationPagedList(
            queryDescription,
            handlerOperation->Timeout,
            [this](AsyncOperationSPtr const& operation)
        {
            this->OnApplicationsOnNodePagedComplete(operation, false, false);
        },
            handlerOperation->shared_from_this());

        OnApplicationsOnNodePagedComplete(inner, true, false);
    }
}

void NodesHandler::OnApplicationsOnNodePagedComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously,
    bool expectApplicationName)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<DeployedApplicationQueryResult> deployedApplicationsResult;
    PagingStatusUPtr pagingStatus;

    // Get the results
    auto error = client.QueryClient->EndGetDeployedApplicationPagedList(operation, deployedApplicationsResult, pagingStatus);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    vector<DeployedApplicationQueryResultWrapper> applicationResultWrapper;
    for (size_t i = 0; i < deployedApplicationsResult.size(); i++)
    {
        DeployedApplicationQueryResultWrapper wrapper(deployedApplicationsResult[i], true);
        applicationResultWrapper.push_back(move(wrapper));
    }

    // Create paging status and ApplicationTypeList as one object to serialize
    DeployedApplicationPagedListWrapper pagedList;
    if (pagingStatus)
    {
        pagedList.ContinuationToken = pagingStatus->TakeContinuationToken();
    }

    pagedList.Items = move(applicationResultWrapper);

    ByteBufferUPtr bufferUPtr;
    if (expectApplicationName)
    {
        if (pagedList.Items.size() == 0)
        {
            handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr), Constants::StatusNoContent, Constants::StatusDescriptionNoContent);
            return;
        }

        error = handlerOperation->Serialize(pagedList.Items[0], bufferUPtr);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(operation->Parent, error);
            return;
        }
    }
    else
    {
        error = handlerOperation->Serialize(pagedList, bufferUPtr);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(operation->Parent, error);
            return;
        }
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void NodesHandler::ApplicationsOnNodeByName(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    auto apiVersion = handlerOperation->Uri.ApiVersion;
    if (apiVersion > Constants::V6ApiVersion)
    {
        GetDeployedApplicationPagedList(thisSPtr, handlerOperation, client, true);
    }
    else
    {
        UriArgumentParser argumentParser(handlerOperation->Uri);

        wstring nodeName;
        auto error = argumentParser.TryGetNodeName(nodeName);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        NamingUri appNameUri;
        error = argumentParser.TryGetApplicationName(appNameUri);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        AsyncOperationSPtr inner = client.QueryClient->BeginGetDeployedApplicationList(
            nodeName,
            appNameUri,
            handlerOperation->Timeout,
            [this](AsyncOperationSPtr const& operation)
        {
            this->OnApplicationsOnNodeByNameComplete(operation, false);
        },
            handlerOperation->shared_from_this());

        OnApplicationsOnNodeByNameComplete(inner, true);
    }
}

void NodesHandler::OnApplicationsOnNodeByNameComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<DeployedApplicationQueryResult> applicationsResult;
    auto error = client.QueryClient->EndGetDeployedApplicationList(operation, applicationsResult);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    if (applicationsResult.size() == 0)
    {
        handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr), Constants::StatusNoContent, Constants::StatusDescriptionNoContent);
        return;
    }

    DeployedApplicationQueryResultWrapper applicationResultWrapper(applicationsResult[0]);
    error = handlerOperation->Serialize(applicationResultWrapper, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void NodesHandler::ServicePackagesOnNode(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring nodeName;
    auto error = argumentParser.TryGetNodeName(nodeName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    NamingUri appNameUri;
    error = argumentParser.TryGetApplicationName(appNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    AsyncOperationSPtr inner = client.QueryClient->BeginGetDeployedServicePackageList(
        nodeName,
        appNameUri,
        EMPTY_STRING_QUERY_FILTER,
        handlerOperation->Timeout,
        [this] (AsyncOperationSPtr const& operation)
        {
            this->OnServicePackagesOnNodeComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnServicePackagesOnNodeComplete(inner, true);
}

void NodesHandler::OnServicePackagesOnNodeComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<DeployedServiceManifestQueryResult> serviceManifestResult;
    auto error = client.QueryClient->EndGetDeployedServicePackageList(operation, serviceManifestResult);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(serviceManifestResult, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void NodesHandler::ServicePackagesOnNodeByName(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring nodeName;
    auto error = argumentParser.TryGetNodeName(nodeName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    NamingUri appNameUri;
    error = argumentParser.TryGetApplicationName(appNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    wstring serviceManifestName;
    handlerOperation->Uri.GetItem(Constants::ServiceManifestNameString, serviceManifestName);

    AsyncOperationSPtr inner = client.QueryClient->BeginGetDeployedServicePackageList(
        nodeName,
        appNameUri,
        serviceManifestName,
        handlerOperation->Timeout,
        [this] (AsyncOperationSPtr const& operation)
        {
            this->OnServicePackagesOnNodeByNameComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnServicePackagesOnNodeByNameComplete(inner, true);
}

void NodesHandler::OnServicePackagesOnNodeByNameComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<DeployedServiceManifestQueryResult> serviceManifestResult;
    auto error = client.QueryClient->EndGetDeployedServicePackageList(operation, serviceManifestResult);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    if (serviceManifestResult.size() == 0)
    {
        handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr), Constants::StatusNoContent, Constants::StatusDescriptionNoContent);
        return;
    }

    error = handlerOperation->Serialize(serviceManifestResult, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void NodesHandler::ServiceTypesOnNode(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring nodeName;
    auto error = argumentParser.TryGetNodeName(nodeName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    NamingUri appNameUri;
    error = argumentParser.TryGetApplicationName(appNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    //
    // If a manifest name filter is specified, retrieve it.
    //
    wstring serviceManifestNameFilter;
    handlerOperation->Uri.GetItem(Constants::ServiceManifestNameString, serviceManifestNameFilter);

    wstring noServiceTypeFilter;
    AsyncOperationSPtr inner = client.QueryClient->BeginGetDeployedServiceTypeList(
        nodeName,
        appNameUri,
        serviceManifestNameFilter,
        noServiceTypeFilter,
        handlerOperation->Timeout,
        [this] (AsyncOperationSPtr const& operation)
        {
            this->OnServiceTypesOnNodeComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnServiceTypesOnNodeComplete(inner, true);
}

void NodesHandler::OnServiceTypesOnNodeComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<DeployedServiceTypeQueryResult> serviceTypeResult;
    auto error = client.QueryClient->EndGetDeployedServiceTypeList(operation, serviceTypeResult);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(serviceTypeResult, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void NodesHandler::ServiceTypesOnNodeByType(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring nodeName;
    auto error = argumentParser.TryGetNodeName(nodeName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    NamingUri appNameUri;
    error = argumentParser.TryGetApplicationName(appNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    wstring serviceManifestNameFilter;
    handlerOperation->Uri.GetItem(Constants::ServiceManifestNameString, serviceManifestNameFilter);

    wstring serviceTypeNameFilter;
    handlerOperation->Uri.GetItem(Constants::ServiceTypeNameIdString, serviceTypeNameFilter);

    AsyncOperationSPtr inner = client.QueryClient->BeginGetDeployedServiceTypeList(
        nodeName,
        appNameUri,
        serviceManifestNameFilter,
        serviceTypeNameFilter,
        handlerOperation->Timeout,
        [this] (AsyncOperationSPtr const& operation)
        {
            this->OnServiceTypesOnNodeByTypeComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnServiceTypesOnNodeByTypeComplete(inner, true);
}

void NodesHandler::OnServiceTypesOnNodeByTypeComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<DeployedServiceTypeQueryResult> serviceTypeResult;
    auto error = client.QueryClient->EndGetDeployedServiceTypeList(operation, serviceTypeResult);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    if (serviceTypeResult.size() == 0)
    {
        handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr), Constants::StatusNoContent, Constants::StatusDescriptionNoContent);
        return;
    }

    if (handlerOperation->Uri.ApiVersion <= Constants::V3ApiVersion)
    {
        error = handlerOperation->Serialize(serviceTypeResult[0], bufferUPtr);
    }
    else
    {
        error = handlerOperation->Serialize(serviceTypeResult, bufferUPtr);
    }

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void NodesHandler::CodePackagesOnNode(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring nodeName;
    auto error = argumentParser.TryGetNodeName(nodeName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    NamingUri appNameUri;
    error = argumentParser.TryGetApplicationName(appNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    wstring serviceManifestNameFilter, codePackageNameFilter;
    handlerOperation->Uri.GetItem(Constants::ServiceManifestNameString, serviceManifestNameFilter);
    handlerOperation->Uri.GetItem(Constants::CodePackageNameIdString, codePackageNameFilter);

    AsyncOperationSPtr inner = client.QueryClient->BeginGetDeployedCodePackageList(
        nodeName,
        appNameUri,
        serviceManifestNameFilter,
        codePackageNameFilter,
        handlerOperation->Timeout,
        [this] (AsyncOperationSPtr const& operation)
        {
            this->OnCodePackagesOnNodeComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnCodePackagesOnNodeComplete(inner, true);
}

void NodesHandler::OnCodePackagesOnNodeComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<DeployedCodePackageQueryResult> codePackageResult;
    auto error = client.QueryClient->EndGetDeployedCodePackageList(operation, codePackageResult);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(codePackageResult, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void NodesHandler::ServiceReplicasOnNode(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring nodeNameString;
    auto error = argumentParser.TryGetNodeName(nodeNameString);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    NamingUri appNameUri;
    error = argumentParser.TryGetApplicationName(appNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    //
    // If servicemanifest name filter or partition id filter is specified, retrieve it.
    //
    wstring serviceManifestNameFilter;
    handlerOperation->Uri.GetItem(Constants::ServiceManifestNameString, serviceManifestNameFilter);

    Guid partitionId;
    error = argumentParser.TryGetOptionalPartitionId(partitionId);
    if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::NameNotFound))
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    AsyncOperationSPtr inner = client.QueryClient->BeginGetDeployedReplicaList(
        nodeNameString,
        appNameUri,
        serviceManifestNameFilter,
        partitionId,
        handlerOperation->Timeout,
        [this] (AsyncOperationSPtr const& operation)
        {
            this->OnServiceReplicasOnNodeComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnServiceReplicasOnNodeComplete(inner, true);
}

void NodesHandler::OnServiceReplicasOnNodeComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<DeployedServiceReplicaQueryResult> serviceReplicaResult;
    auto error = client.QueryClient->EndGetDeployedReplicaList(operation, serviceReplicaResult);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    if (serviceReplicaResult.size() == 0)
    {
        handlerOperation->OnSuccess(
            operation->Parent,
            move(bufferUPtr),
            Constants::StatusNoContent,
            Constants::StatusDescriptionNoContent);

        return;
    }

    error = handlerOperation->Serialize(serviceReplicaResult, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void NodesHandler::ServiceReplicasOnHost(__in Common::AsyncOperationSPtr const & thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring nodeName;
    auto error = argumentParser.TryGetNodeName(nodeName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    Guid partitionId;
    error = argumentParser.TryGetRequiredPartitionId(partitionId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    FABRIC_REPLICA_ID replicaId;
    error = argumentParser.TryGetReplicaId(replicaId);
    if (error.IsError(ErrorCodeValue::NameNotFound))
    {
        replicaId = -1;
    }
    else if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    AsyncOperationSPtr inner = client.QueryClient->BeginGetDeployedServiceReplicaDetail(
        nodeName,
        partitionId,
        replicaId,
        handlerOperation->Timeout,
        [this] (AsyncOperationSPtr const& operation)
        {
            this->OnServiceReplicasOnHostComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnServiceReplicasOnHostComplete(inner, true);
}

void NodesHandler::OnServiceReplicasOnHostComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    DeployedServiceReplicaDetailQueryResult result;
    auto error = client.QueryClient->EndGetDeployedServiceReplicaDetail(operation, result);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(result, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void NodesHandler::EvaluateNodeHealth(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring nodeNameString;
    auto error = argumentParser.TryGetNodeName(nodeNameString);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    unique_ptr<ClusterHealthPolicy> healthPolicy;
    if (handlerOperation->Uri.Verb == Constants::HttpPostVerb && !handlerOperation->Body->empty())
    {
        healthPolicy = make_unique<ClusterHealthPolicy>();

        error = handlerOperation->Deserialize(*healthPolicy, handlerOperation->Body);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(
                thisSPtr,
                ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Deserialization_Error), L"ClusterHealthPolicy")));
            return;
        }
    }

    NodeHealthQueryDescription queryDescription(move(nodeNameString), move(healthPolicy));

    wstring eventsFilterString;
    if (handlerOperation->Uri.GetItem(Constants::EventsHealthStateFilterString, eventsFilterString))
    {
        DWORD filter;
        error = Utility::TryParseQueryFilter(eventsFilterString, FABRIC_HEALTH_STATE_FILTER_DEFAULT, filter);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        queryDescription.SetEventsFilter(make_unique<HealthEventsFilter>(filter));
    }

    AsyncOperationSPtr operation = client.HealthClient->BeginGetNodeHealth(
        queryDescription,
        handlerOperation->Timeout,
        [this] (AsyncOperationSPtr const& operation)
        {
            this->EvaluateNodeHealthComplete(operation, false);
        },
        thisSPtr);

    EvaluateNodeHealthComplete(operation, true);
}

void NodesHandler::EvaluateNodeHealthComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    NodeHealth health;
    auto error = client.HealthClient->EndGetNodeHealth(operation, health);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(health, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void NodesHandler::EvaluateApplicationOnNodeHealth(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring nodeNameString;
    auto error = argumentParser.TryGetNodeName(nodeNameString);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    NamingUri appNameUri;
    error = argumentParser.TryGetApplicationName(appNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    unique_ptr<ApplicationHealthPolicy> healthPolicy;
    if (handlerOperation->Uri.Verb == Constants::HttpPostVerb && !handlerOperation->Body->empty())
    {
        healthPolicy = make_unique<ApplicationHealthPolicy>();

        error = handlerOperation->Deserialize(*healthPolicy, handlerOperation->Body);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(
                thisSPtr,
                ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Deserialization_Error), L"ApplicationHealthPolicy")));
            return;
        }
    }

    DeployedApplicationHealthQueryDescription queryDescription(move(appNameUri), move(nodeNameString), move(healthPolicy));

    DeployedApplicationHealthStatisticsFilterUPtr statsFilter;
    bool excludeStatsFilter = false;
    error = argumentParser.TryGetExcludeStatisticsFilter(excludeStatsFilter);
    if (error.IsSuccess())
    {
        statsFilter = make_unique<DeployedApplicationHealthStatisticsFilter>(excludeStatsFilter);
        queryDescription.SetStatisticsFilter(move(statsFilter));
    }
    else if (!error.IsError(ErrorCodeValue::NotFound))
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    wstring eventsFilterString;
    if (handlerOperation->Uri.GetItem(Constants::EventsHealthStateFilterString, eventsFilterString))
    {
        DWORD filter;
        error = Utility::TryParseQueryFilter(eventsFilterString, FABRIC_HEALTH_STATE_FILTER_DEFAULT, filter);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        queryDescription.SetEventsFilter(make_unique<HealthEventsFilter>(filter));
    }

    wstring deployedServicePackagesFilterString;
    if (handlerOperation->Uri.GetItem(Constants::DeployedServicePackagesHealthStateFilterString, deployedServicePackagesFilterString))
    {
        DWORD filter;
        error = Utility::TryParseQueryFilter(deployedServicePackagesFilterString, FABRIC_HEALTH_STATE_FILTER_DEFAULT, filter);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        queryDescription.SetDeployedServicePackagesFilter(make_unique<DeployedServicePackageHealthStatesFilter>(filter));
    }

    AsyncOperationSPtr operation = client.HealthClient->BeginGetDeployedApplicationHealth(
        queryDescription,
        handlerOperation->Timeout,
        [this] (AsyncOperationSPtr const& operation)
        {
            this->EvaluateApplicationOnNodeHealthComplete(operation, false);
        },
        thisSPtr);

    EvaluateApplicationOnNodeHealthComplete(operation, true);
}

void NodesHandler::EvaluateApplicationOnNodeHealthComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    DeployedApplicationHealth health;
    auto error = client.HealthClient->EndGetDeployedApplicationHealth(operation, health);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(health, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void NodesHandler::EvaluateServicePackagesOnNodeHealth(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    NamingUri appNameUri;
    auto error = argumentParser.TryGetApplicationName(appNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    wstring nodeNameString;
    error = argumentParser.TryGetNodeName(nodeNameString);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    wstring servicePackageActivationId;
    error = argumentParser.TryGetServicePackageActivationId(servicePackageActivationId);
    if (error.IsError(ErrorCodeValue::NameNotFound))
    {
        WriteNoise(TraceType, "ServicePackageActivationId is not specified for GetDeployedServicePackageHealth query. Using default.");
    }
    else if(!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    wstring serviceManifestName;
    handlerOperation->Uri.GetItem(Constants::ServiceManifestNameIdString, serviceManifestName);

    unique_ptr<ApplicationHealthPolicy> healthPolicy;
    if (handlerOperation->Uri.Verb == Constants::HttpPostVerb && !handlerOperation->Body->empty())
    {
        healthPolicy = make_unique<ApplicationHealthPolicy>();

        error = handlerOperation->Deserialize(*healthPolicy, handlerOperation->Body);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(
                thisSPtr,
                ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Deserialization_Error), L"ApplicationHealthPolicy")));
            return;
        }
    }

    DeployedServicePackageHealthQueryDescription queryDescription(
        move(appNameUri),
        move(serviceManifestName),
        move(servicePackageActivationId),
        move(nodeNameString),
        move(healthPolicy));

    wstring eventsFilterString;
    if (handlerOperation->Uri.GetItem(Constants::EventsHealthStateFilterString, eventsFilterString))
    {
        DWORD filter;
        error = Utility::TryParseQueryFilter(eventsFilterString, FABRIC_HEALTH_STATE_FILTER_DEFAULT, filter);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        queryDescription.SetEventsFilter(make_unique<HealthEventsFilter>(filter));
    }

    AsyncOperationSPtr operation = client.HealthClient->BeginGetDeployedServicePackageHealth(
        queryDescription,
        handlerOperation->Timeout,
        [this] (AsyncOperationSPtr const& operation)
        {
            this->EvaluateServicePackagesOnNodeHealthComplete(operation, false);
        },
        thisSPtr);

    EvaluateServicePackagesOnNodeHealthComplete(operation, true);
}

void NodesHandler::EvaluateServicePackagesOnNodeHealthComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    DeployedServicePackageHealth health;
    auto error = client.HealthClient->EndGetDeployedServicePackageHealth(operation, health);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(health, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void NodesHandler::ReportNodeHealth(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring nodeName;
    auto error = argumentParser.TryGetNodeName(nodeName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    HealthInformation healthInfo;
    error = handlerOperation->Deserialize(healthInfo, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(
            thisSPtr,
            ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Deserialization_Error), L"HealthInformation")));
        return;
    }

    Federation::NodeId nodeId;
    error = Federation::NodeIdGenerator::GenerateFromString(nodeName, nodeId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    AttributeList attributeList;
    if (!nodeName.empty())
    {
        attributeList.AddAttribute(*HealthAttributeNames::NodeName, nodeName);
    }

    HealthReport healthReport(
        EntityHealthInformation::CreateNodeEntityHealthInformation(nodeId.IdValue, nodeName, FABRIC_INVALID_NODE_INSTANCE_ID),
        healthInfo.SourceId,
        healthInfo.Property,
        healthInfo.TimeToLive,
        healthInfo.State,
        healthInfo.Description,
        healthInfo.SequenceNumber,
        healthInfo.RemoveWhenExpired,
        move(attributeList));

    error = healthReport.TryAccept(true /*disallowSystemReport*/);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    bool immediate;
    error = argumentParser.TryGetImmediateOption(immediate);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    HealthReportSendOptionsUPtr sendOptions;
    if (immediate)
    {
        sendOptions = make_unique<HealthReportSendOptions>(immediate);
    }

    ByteBufferUPtr bufferUPtr;
    error = client.HealthClient->ReportHealth(move(healthReport), move(sendOptions));
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    handlerOperation->OnSuccess(thisSPtr, move(bufferUPtr));
}

void NodesHandler::ReportApplicationOnNodeHealth(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring nodeName;
    auto error = argumentParser.TryGetNodeName(nodeName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    NamingUri appNameUri;
    error = argumentParser.TryGetApplicationName(appNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    HealthInformation healthInfo;
    error = handlerOperation->Deserialize(healthInfo, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(
            thisSPtr,
            ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Deserialization_Error), L"HealthInformation")));
        return;
    }

    Federation::NodeId nodeId;
    error = Federation::NodeIdGenerator::GenerateFromString(nodeName, nodeId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    AttributeList attributeList;
    if (!nodeName.empty())
    {
        attributeList.AddAttribute(*HealthAttributeNames::NodeName, nodeName);
    }

    HealthReport healthReport(
        EntityHealthInformation::CreateDeployedApplicationEntityHealthInformation(appNameUri.ToString(), nodeId.IdValue, nodeName, FABRIC_INVALID_INSTANCE_ID),
        healthInfo.SourceId,
        healthInfo.Property,
        healthInfo.TimeToLive,
        healthInfo.State,
        healthInfo.Description,
        healthInfo.SequenceNumber,
        healthInfo.RemoveWhenExpired,
        move(attributeList));

    error = healthReport.TryAccept(true /*disallowSystemReport*/);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    bool immediate;
    error = argumentParser.TryGetImmediateOption(immediate);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    HealthReportSendOptionsUPtr sendOptions;
    if (immediate)
    {
        sendOptions = make_unique<HealthReportSendOptions>(immediate);
    }

    ByteBufferUPtr bufferUPtr;
    error = client.HealthClient->ReportHealth(move(healthReport), move(sendOptions));
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    handlerOperation->OnSuccess(thisSPtr, move(bufferUPtr));
}

void NodesHandler::ReportServicePackagesOnNodeHealth(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring nodeName;
    auto error = argumentParser.TryGetNodeName(nodeName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    NamingUri appNameUri;
    error = argumentParser.TryGetApplicationName(appNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    wstring serviceManifestName;
    handlerOperation->Uri.GetItem(Constants::ServiceManifestNameIdString, serviceManifestName);

    wstring servicePackageActivationId;
    error = argumentParser.TryGetServicePackageActivationId(servicePackageActivationId);
    if (error.IsError(ErrorCodeValue::NameNotFound))
    {
        WriteNoise(TraceType, "ServicePackageActivationId is not specified for ReportServicePackagesOnNodeHealth query. Using default.");
    }
    else if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    HealthInformation healthInfo;
    error = handlerOperation->Deserialize(healthInfo, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(
            thisSPtr,
            ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Deserialization_Error), L"HealthInformation")));
        return;
    }

    Federation::NodeId nodeId;
    error = Federation::NodeIdGenerator::GenerateFromString(nodeName, nodeId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    AttributeList attributeList;
    if (!nodeName.empty())
    {
        attributeList.AddAttribute(*HealthAttributeNames::NodeName, nodeName);
    }

    HealthReport healthReport(
        EntityHealthInformation::CreateDeployedServicePackageEntityHealthInformation(appNameUri.ToString(), serviceManifestName, servicePackageActivationId, nodeId.IdValue, nodeName, FABRIC_INVALID_INSTANCE_ID),
        healthInfo.SourceId,
        healthInfo.Property,
        healthInfo.TimeToLive,
        healthInfo.State,
        healthInfo.Description,
        healthInfo.SequenceNumber,
        healthInfo.RemoveWhenExpired,
        move(attributeList));

    error = healthReport.TryAccept(true /*disallowSystemReport*/);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    bool immediate;
    error = argumentParser.TryGetImmediateOption(immediate);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    HealthReportSendOptionsUPtr sendOptions;
    if (immediate)
    {
        sendOptions = make_unique<HealthReportSendOptions>(immediate);
    }

    ByteBufferUPtr bufferUPtr;
    error = client.HealthClient->ReportHealth(move(healthReport), move(sendOptions));
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    handlerOperation->OnSuccess(thisSPtr, move(bufferUPtr));
}

void NodesHandler::RemoveReplica(__in Common::AsyncOperationSPtr const& thisSPtr)
{
    ReportFault(thisSPtr, Reliability::FaultType::Permanent);
}

void NodesHandler::RestartReplica(__in Common::AsyncOperationSPtr const& thisSPtr)
{
    ReportFault(thisSPtr, Reliability::FaultType::Transient);
}

void NodesHandler::ReportFault(__in Common::AsyncOperationSPtr const &thisSPtr, Reliability::FaultType::Enum faultType)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring nodeName;
    auto error = argumentParser.TryGetNodeName(nodeName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    Common::Guid partitionId;
    error = argumentParser.TryGetRequiredPartitionId(partitionId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    FABRIC_REPLICA_ID replicaId = 0;
    error = argumentParser.TryGetRequiredReplicaId(replicaId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    bool isForceRemove = false;
    error = argumentParser.TryGetForceRemove(isForceRemove);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto op = client.ServiceMgmtClient->BeginReportFault(
        nodeName,
        partitionId,
        replicaId,
        faultType,
        isForceRemove,
        handlerOperation->Timeout,
        [this] (AsyncOperationSPtr const & inner)
        {
            this->ReportFaultComplete(inner, false);
        },
        thisSPtr);

    ReportFaultComplete(op, true);
}

void NodesHandler::ReportFaultComplete(__in Common::AsyncOperationSPtr const & operation, __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.ServiceMgmtClient->EndReportFault(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void NodesHandler::GetNodeLoadInformation(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring nodeName;
    auto error = argumentParser.TryGetNodeName(nodeName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto inner = client.QueryClient->BeginGetNodeLoadInformation(
        nodeName,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnGetNodeLoadInformationComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnGetNodeLoadInformationComplete(inner, true);
}

void NodesHandler::OnGetNodeLoadInformationComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    NodeLoadInformationQueryResult queryResult;

    auto error = client.QueryClient->EndGetNodeLoadInformation(operation, queryResult);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;

    error = handlerOperation->Serialize(queryResult, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void NodesHandler::DeployServicePackageToNode(__in AsyncOperationSPtr const& thisSPtr)
{
    DeployServicePackageToNodeMessage servicePackageMessage;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    wstring nodeName;
    handlerOperation->Uri.GetItem(Constants::NodeIdString, nodeName);

    auto error = Utility::ValidateString(nodeName);
    if (!error.IsSuccess())
    {
        return handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
    }


    error = handlerOperation->Deserialize(servicePackageMessage, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    wstring sharingPoliciesStr;
    error = servicePackageMessage.GetSharingPoliciesFromRest(sharingPoliciesStr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto inner = client.AppMgmtClient->BeginDeployServicePackageToNode(
        servicePackageMessage.ApplicationTypeName,
        servicePackageMessage.ApplicationTypeVersion,
        servicePackageMessage.ServiceManifestName,
        sharingPoliciesStr,
        nodeName,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnDeployServicePackageToNodeComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnDeployServicePackageToNodeComplete(inner, true);
}

void NodesHandler::OnDeployServicePackageToNodeComplete(
    __in AsyncOperationSPtr const &operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.AppMgmtClient->EndDeployServicePackageToNode(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }
    ByteBufferUPtr bufferUPtr;
    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

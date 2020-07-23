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

StringLiteral const TraceType("ApplicationsResourceHandler");

ApplicationsResourceHandler::ApplicationsResourceHandler(HttpGatewayImpl & server)
    : RequestHandlerBase(server)
{
}

ApplicationsResourceHandler::~ApplicationsResourceHandler()
{
}

ErrorCode ApplicationsResourceHandler::Initialize()
{
    //
    // Resource names are non hierarchical.
    //
    this->allowHierarchicalEntityKeys = false;

    vector<HandlerUriTemplate> uris;

    // /Resources/Applications/<name>
    uris.push_back(HandlerUriTemplate(
        Constants::ApplicationsResourceEntityKeyPath,
        Constants::HttpPutVerb,
        MAKE_HANDLER_CALLBACK(CreateOrUpdateApplication)));

    // /Resources/Applications/<name>
    uris.push_back(HandlerUriTemplate(
        Constants::ApplicationsResourceEntityKeyPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetApplicationByName)));

    // /Resources/Applications
    uris.push_back(HandlerUriTemplate(
        Constants::ApplicationsResourceEntitySetPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetAllApplications)));

    // /Resources/Applications/<name>
    uris.push_back(HandlerUriTemplate(
        Constants::ApplicationsResourceEntityKeyPath,
        Constants::HttpDeleteVerb,
        MAKE_HANDLER_CALLBACK(DeleteApplication)));

    // /Resources/Applications/<name>/Services/
    uris.push_back(HandlerUriTemplate(
        Constants::ServicesResourceEntitySetPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetAllServices)));

    // /Resources/Applications/<name>/Services/<name>
    uris.push_back(HandlerUriTemplate(
        Constants::ServicesResourceEntityKeyPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetServiceByName)));

    // /Resources/Applications/<name>/Services/<name>/Replicas
    uris.push_back(HandlerUriTemplate(
        Constants::ReplicasResourceEntitySetPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetAllReplicas)));

    // /Resources/Applications/<name>/Services/<name>/Replicas/<name>
    uris.push_back(HandlerUriTemplate(
        Constants::ReplicasResourceEntityKeyPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetReplicaByName)));

    // /Resources/Applications/<name>/Services/<name>/Replicas/<name>/CodePackages/{codePackageName}/Logs?&Tail={Tail}
    uris.push_back(HandlerUriTemplate(
        Constants::ContainerCodePackageLogsKeyPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetContainerCodePackageLogs)));

    validHandlerUris.insert(validHandlerUris.begin(), uris.begin(), uris.end());

    return server_.InnerServer->RegisterHandler(Constants::ApplicationsResourceHandlerPath, shared_from_this());
}

void ApplicationsResourceHandler::CreateOrUpdateApplication(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    ModelV2::ApplicationDescription description;
    auto error = handlerOperation->Deserialize(description, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    wstring applicationNameInUri;
    if (!handlerOperation->Uri.GetItem(Constants::ApplicationIdString, applicationNameInUri))
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::NameNotFound);
        return;
    }

    if (!description.Name.empty() && description.Name != applicationNameInUri)
    {
        handlerOperation->OnError(thisSPtr, ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_COMMON_RC(Resource_Name_Mismatch), "Application", applicationNameInUri, description.Name)));
        return;
    }
    description.Name = move(applicationNameInUri);

    error = argumentParser.TryGetApplicationName(description.ApplicationUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto inner = client.ResourceMgmtClient->BeginCreateOrUpdateApplicationResource(
        move(description),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnCreateOrUpdateApplicationComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    this->OnCreateOrUpdateApplicationComplete(inner, true);
}

void ApplicationsResourceHandler::OnCreateOrUpdateApplicationComplete(AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    // TODO - return the description with status.
    ModelV2::ApplicationDescription description;
    auto error = client.ResourceMgmtClient->EndCreateOrUpdateApplicationResource(operation, description);
    if (error.IsError(ErrorCodeValue::SingleInstanceApplicationAlreadyExists))
    {
        ByteBufferUPtr emptyBody;
        handlerOperation->OnSuccess(thisSPtr, move(emptyBody), Constants::StatusCreated, *Constants::StatusDescriptionCreated);
        return;
    }
    else if (error.IsError(ErrorCodeValue::CMRequestAlreadyProcessing))
    {
        ByteBufferUPtr emptyBody;
        handlerOperation->OnSuccess(thisSPtr, move(emptyBody), Constants::StatusServiceUnavailable, *Constants::StatusDescriptionServiceUnavailable);
        return;
    }
    else if (error.IsError(ErrorCodeValue::ApplicationDeploymentInProgress))
    {
        error = ErrorCode::Success();
    }

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody), Constants::StatusAccepted, *Constants::StatusDescriptionAccepted);
}

void ApplicationsResourceHandler::DeleteApplication(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    std::wstring name;
    if (!handlerOperation->Uri.GetItem(Constants::ApplicationIdString, name))
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::NameNotFound);
        return;
    }

    auto operation = client.ResourceMgmtClient->BeginDeleteSingleInstanceDeployment(
        DeleteSingleInstanceDeploymentDescription(name),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operation)
        {
            this->OnDeleteApplicationComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    this->OnDeleteApplicationComplete(operation, true);

}

void ApplicationsResourceHandler::OnDeleteApplicationComplete(AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    auto error = client.ResourceMgmtClient->EndDeleteSingleInstanceDeployment(operation);
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

void ApplicationsResourceHandler::GetAllApplications(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    UriArgumentParser argumentParser(handlerOperation->Uri);

    ApplicationQueryDescription queryDescription;
    auto error = ApplicationsHandler::GetApplicationQueryDescription(thisSPtr, false, queryDescription);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    AsyncOperationSPtr operation = client.ResourceMgmtClient->BeginGetApplicationResourceList(
        move(queryDescription),
        handlerOperation->Timeout,
        [this] (AsyncOperationSPtr const& operation)
        {
            this->OnGetAllApplicationsComplete(operation, false);
        },
        thisSPtr);

    OnGetAllApplicationsComplete(operation, true);
}

void ApplicationsResourceHandler::OnGetAllApplicationsComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<ModelV2::ApplicationDescriptionQueryResult> applicationsResult;
    PagingStatusUPtr pagingStatus;
    auto error = client.ResourceMgmtClient->EndGetApplicationResourceList(operation, applicationsResult, pagingStatus);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ApplicationDescriptionQueryResultList list;
    if (pagingStatus)
    {
        list.ContinuationToken = pagingStatus->TakeContinuationToken();
    }

    list.Items = move(applicationsResult);
    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(list, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ApplicationsResourceHandler::GetApplicationByName(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    ApplicationQueryDescription queryDescription;
    auto error = ApplicationsHandler::GetApplicationQueryDescription(thisSPtr, true, queryDescription);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto operation = client.ResourceMgmtClient->BeginGetApplicationResourceList(
        move(queryDescription),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operation)
        {
            this->OnGetApplicationByNameComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    this->OnGetApplicationByNameComplete(operation, true);

}

void ApplicationsResourceHandler::OnGetApplicationByNameComplete(AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    vector<ModelV2::ApplicationDescriptionQueryResult> applicationsResult;
    PagingStatusUPtr pagingStatus;
    auto error = client.ResourceMgmtClient->EndGetApplicationResourceList(operation, applicationsResult, pagingStatus);

    // We shouldn't receive paging status for just one application
    TESTASSERT_IF(pagingStatus, "OnGetApplicationByNameComplete: paging status shouldn't be set");

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    if (applicationsResult.size() == 0)
    {
        handlerOperation->OnSuccess(
            operation->Parent,
            move(bufferUPtr),
            Constants::StatusNoContent,
            Constants::StatusDescriptionNoContent);

        return;
    }

    if (applicationsResult.size() != 1)
    {
        handlerOperation->OnError(operation->Parent, ErrorCodeValue::OperationFailed);
        return;
    }

    error = handlerOperation->Serialize(applicationsResult[0], bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ApplicationsResourceHandler::GetAllServices(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    NamingUri appNameUri;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    auto error = argumentParser.TryGetApplicationName(appNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    wstring continuationToken;
    error = argumentParser.TryGetContinuationToken(continuationToken);
    if (error.IsError(ErrorCodeValue::NameNotFound))
    {
        error = ErrorCode::Success();
    }

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    QueryPagingDescription queryDescription;
    queryDescription.ContinuationToken = move(continuationToken);

    ServiceQueryDescription description(
        move(appNameUri),
        NamingUri(*EMPTY_URI_QUERY_FILTER),
        L"",
        move(queryDescription));

    AsyncOperationSPtr operation = client.ResourceMgmtClient->BeginGetServiceResourceList(
        description,
        handlerOperation->Timeout,
        [this] (AsyncOperationSPtr const& operation)
        {
            this->OnGetAllServicesComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnGetAllServicesComplete(operation, true);
}

void ApplicationsResourceHandler::OnGetAllServicesComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<ModelV2::ContainerServiceQueryResult> serviceResult;
    PagingStatusUPtr pagingStatus;
    auto error = client.ResourceMgmtClient->EndGetServiceResourceList(operation, serviceResult, pagingStatus);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ContainerServiceQueryResultList list;
    if (pagingStatus)
    {
        list.ContinuationToken = pagingStatus->TakeContinuationToken();
    }

    list.Items = move(serviceResult);

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(list, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ApplicationsResourceHandler::GetServiceByName(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    NamingUri appNameUri;
    NamingUri serviceNameUri;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    auto error = argumentParser.TryGetApplicationName(appNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    error = argumentParser.TryGetAbsoluteServiceName(serviceNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    ServiceQueryDescription description(move(appNameUri), move(serviceNameUri), L"", QueryPagingDescription());
    AsyncOperationSPtr operation = client.ResourceMgmtClient->BeginGetServiceResourceList(
        description,
        handlerOperation->Timeout,
        [this] (AsyncOperationSPtr const& operation)
        {
            this->OnGetServiceByNameComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnGetServiceByNameComplete(operation, true);
}

void ApplicationsResourceHandler::OnGetServiceByNameComplete(
    AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<ModelV2::ContainerServiceQueryResult> serviceResult;
    PagingStatusUPtr pagingStatus;
    auto error = client.ResourceMgmtClient->EndGetServiceResourceList(operation, serviceResult, pagingStatus);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    // We shouldn't receive paging status for just one service
    TESTASSERT_IF(pagingStatus, "OnGetServiceByIdComplete: paging status shouldn't be set");

    ByteBufferUPtr bufferUPtr;
    if (serviceResult.size() == 0)
    {
        handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr), Constants::StatusNoContent, Constants::StatusDescriptionNoContent);
        return;
    }

    if (serviceResult.size() != 1)
    {
        handlerOperation->OnError(operation->Parent, ErrorCode(ErrorCodeValue::OperationFailed));
        return;
    }

    error = handlerOperation->Serialize(serviceResult[0], bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, ErrorCode(ErrorCodeValue::SerializationError));
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ApplicationsResourceHandler::GetAllReplicas(AsyncOperationSPtr const & thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto & client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    NamingUri appNameUri;
    auto error = argumentParser.TryGetApplicationName(appNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    NamingUri serviceNameUri;
    error = argumentParser.TryGetAbsoluteServiceName(serviceNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto operation = client.ResourceMgmtClient->BeginGetReplicaResourceList(
        ReplicasResourceQueryDescription(
            move(appNameUri),
            move(serviceNameUri)),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operation)
        {
            this->OnGetAllReplicasComplete(operation, false);
        },
        handlerOperation->shared_from_this());
    this->OnGetAllReplicasComplete(operation, true);
}

void ApplicationsResourceHandler::OnGetAllReplicasComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const thisSPtr = operation->Parent;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto & client = handlerOperation->FabricClient;

    vector<ReplicaResourceQueryResult> result;
    PagingStatusUPtr pagingStatus;
    ErrorCode error = client.ResourceMgmtClient->EndGetReplicaResourceList(operation, result, pagingStatus);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    ReplicaResourceList list;
    if (pagingStatus)
    {
        list.ContinuationToken = pagingStatus->TakeContinuationToken();
    }
    list.Items = move(result);

    ByteBufferUPtr bufferUPtr = make_unique<ByteBuffer>();
    error = handlerOperation->Serialize(list, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }
    handlerOperation->OnSuccess(thisSPtr, move(bufferUPtr));
}

void ApplicationsResourceHandler::GetReplicaByName(AsyncOperationSPtr const & thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    handlerOperation;
    auto & client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    NamingUri appNameUri;
    auto error = argumentParser.TryGetApplicationName(appNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    NamingUri serviceNameUri;
    error = argumentParser.TryGetAbsoluteServiceName(serviceNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    int64 replicaId;
    error = argumentParser.TryGetReplicaId(replicaId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto operation = client.ResourceMgmtClient->BeginGetReplicaResourceList(
        ReplicasResourceQueryDescription(
            move(appNameUri),
            move(serviceNameUri),
            replicaId),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operation)
        {
            this->OnGetReplicaByNameComplete(operation, false);
        },
        handlerOperation->shared_from_this());
    this->OnGetReplicaByNameComplete(operation, true);
}

void ApplicationsResourceHandler::OnGetReplicaByNameComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const thisSPtr = operation->Parent;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto & client = handlerOperation->FabricClient;

    vector<ReplicaResourceQueryResult> result;
    PagingStatusUPtr pagingStatus;
    ErrorCode error = client.ResourceMgmtClient->EndGetReplicaResourceList(operation, result, pagingStatus);

    TESTASSERT_IF(pagingStatus, "OnGetReplicaByNameComplete: paging status shouldn't be set");

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    if (result.size() == 0)
    {
        handlerOperation->OnSuccess(thisSPtr, move(bufferUPtr), Constants::StatusNoContent, Constants::StatusDescriptionNoContent);
        return;
    }
    else if (result.size() != 1)
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::OperationFailed);
        return;
    }

    error = handlerOperation->Serialize(result[0], bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    handlerOperation->OnSuccess(thisSPtr, move(bufferUPtr));
}

void ApplicationsResourceHandler::GetContainerCodePackageLogs(AsyncOperationSPtr const & thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    handlerOperation;
    auto & client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    NamingUri appNameUri;
    auto error = argumentParser.TryGetApplicationName(appNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    NamingUri serviceNameUri;
    error = argumentParser.TryGetAbsoluteServiceName(serviceNameUri);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    int64 replicaId;
    error = argumentParser.TryGetReplicaId(replicaId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    wstring codePackageName;
    if (!handlerOperation->Uri.GetItem(Constants::CodePackageNameIdString, codePackageName))
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::NameNotFound);
        return;
    }

    wstring tailArgument;
    handlerOperation->Uri.GetItem(ContainerInfoArgs::Tail, tailArgument);

    ContainerInfoArgMap containerInfoArgMap;
    containerInfoArgMap.Insert(ContainerInfoArgs::Tail, tailArgument);

    AsyncOperationSPtr operation = client.ResourceMgmtClient->BeginGetContainerCodePackageLogs(
        appNameUri,
        serviceNameUri,
        StringUtility::ToWString(replicaId),
        codePackageName,
        containerInfoArgMap,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operation)
    {
        this->OnGetContainerCodePackageLogsComplete(operation, false);
    },
        thisSPtr);

    this->OnGetContainerCodePackageLogsComplete(operation, true);
}

void ApplicationsResourceHandler::OnGetContainerCodePackageLogsComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const thisSPtr = operation->Parent;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto & client = handlerOperation->FabricClient;

    wstring containerInfo;
    auto error = client.ResourceMgmtClient->EndGetContainerCodePackageLogs(operation, containerInfo);
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

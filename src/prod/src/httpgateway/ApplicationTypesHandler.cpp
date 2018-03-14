// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Api;
using namespace Query;
using namespace ServiceModel;
using namespace Naming;
using namespace HttpGateway;
using namespace Management::ClusterManager;

StringLiteral const TraceType("ApplicationTypesHandler");

ApplicationTypesHandler::ApplicationTypesHandler(HttpGatewayImpl & server)
    : RequestHandlerBase(server)
{
}

ApplicationTypesHandler::~ApplicationTypesHandler()
{
}

ErrorCode ApplicationTypesHandler::Initialize()
{
    // This corresponds to GetApplicationType WITHOUT the application type name filter
    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::ApplicationTypesEntitySetPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetAllApplicationsTypes)));

    // This corresponds to GetApplicationType WITH the application type name filter
    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::ApplicationTypesEntityKeyPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetApplicationTypesByType)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ApplicationTypesEntitySetPath, Constants::Provision),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(ProvisionApplicationType)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ApplicationTypesEntityKeyPath, Constants::UnProvision),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(UnprovisionApplicationType)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ApplicationTypesEntityKeyPath, Constants::GetApplicationManifest),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetApplicationManifest)));

    // TODO - check if this URI is correct
    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ApplicationTypesEntityKeyPath, Constants::GetServiceManifest),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetServiceManifest)));

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::ServiceTypesEntitySetPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetServiceTypes)));

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::ServiceTypesEntityKeyPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetServiceTypesByType)));

    return server_.InnerServer->RegisterHandler(Constants::ApplicationTypesHandlerPath, shared_from_this());
}

// This is a helper function for GetAllApplicationsTypes and GetApplicationTypesByType
//
// This does the following
//      1. Read incoming information
//      2. Calls method to begin query
void ApplicationTypesHandler::GetApplicationTypesPagedHelper(__in AsyncOperationSPtr const& thisSPtr, HandlerAsyncOperation * const& handlerOperation, FabricClientWrapper const& client)
{
    ApplicationTypeQueryDescription queryDescription;

    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring appTypeName;
    // GetItem returns true if anything is found. Doesn't touch appTypeName if nothing is found
    if (handlerOperation->Uri.GetItem(Constants::ApplicationTypeNameString, appTypeName))
    {
        queryDescription.ApplicationTypeNameFilter = move(appTypeName);
    }

    wstring appTypeVersion;
    // GetItem returns true if anything is found. Doesn't touch appTypeVersion if nothing is found
    if (handlerOperation->Uri.GetItem(Constants::ApplicationTypeVersionString, appTypeVersion))
    {
        queryDescription.ApplicationTypeVersionFilter = move(appTypeVersion);
    }

    DWORD applicationTypeDefinitionKindFilter = FABRIC_APPLICATION_TYPE_DEFINITION_KIND_FILTER_DEFAULT;
    ErrorCode error = argumentParser.TryGetApplicationTypeDefinitionKindFilter(applicationTypeDefinitionKindFilter);
    if (error.IsError(ErrorCodeValue::NameNotFound))
    {
        error = ErrorCode::Success();
    }

    if (error.IsSuccess())
    {
        queryDescription.ApplicationTypeDefinitionKindFilter = applicationTypeDefinitionKindFilter;
    }
    else
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    int64 maxResults;
    auto errorMaxResults = argumentParser.TryGetMaxResults(maxResults);
    if (errorMaxResults.IsSuccess())
    {
        queryDescription.MaxResults = maxResults;
    }
    else if (!errorMaxResults.IsSuccess() && !errorMaxResults.IsError(ErrorCodeValue::NameNotFound))
    {
        handlerOperation->OnError(thisSPtr, errorMaxResults);
        return;
    }

    bool excludeApplicationParameters;
    // This always returns a boolean value. False is no value is found.
    auto errorExcludeApplicationParameters = argumentParser.TryGetExcludeApplicationParameters(excludeApplicationParameters);
    if (!errorExcludeApplicationParameters.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, errorExcludeApplicationParameters);
        return;
    }
    queryDescription.ExcludeApplicationParameters = excludeApplicationParameters;

    wstring continuationTokenString;
    auto errorContinuationToken = argumentParser.TryGetContinuationToken(continuationTokenString);
    if (errorContinuationToken.IsSuccess()) {
        queryDescription.ContinuationToken = move(continuationTokenString);
    }
    else if (!errorContinuationToken.IsSuccess() && !errorContinuationToken.IsError(ErrorCodeValue::NameNotFound))
    {
        // If an error occurred and a continuation token has been provided
        handlerOperation->OnError(thisSPtr, errorContinuationToken);
        return;
    }

    AsyncOperationSPtr operation = client.QueryClient->BeginGetApplicationTypePagedList(
        move(queryDescription),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetApplicationTypesPagedComplete(operation, false);
    },
        thisSPtr);

    OnGetApplicationTypesPagedComplete(operation, true);
}

void ApplicationTypesHandler::GetAllApplicationsTypes(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    // If the API version is less than 4.0 then we use the non paged version of the API
    if (handlerOperation->Uri.ApiVersion <= Constants::V3ApiVersion)
    {
        AsyncOperationSPtr operation = client.QueryClient->BeginGetApplicationTypeList(
            wstring(),
            handlerOperation->Timeout,
            [this](AsyncOperationSPtr const& operation)
            {
                this->OnGetAllApplicationTypesComplete(operation, false);
            },
            thisSPtr);

        OnGetAllApplicationTypesComplete(operation, true);
    }
    else
    {
        GetApplicationTypesPagedHelper(thisSPtr, handlerOperation, client);
    }
}

void ApplicationTypesHandler::OnGetAllApplicationTypesComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<ApplicationTypeQueryResult> applicationTypesResult;
    auto error = client.QueryClient->EndGetApplicationTypeList(operation, applicationTypesResult);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr = make_unique<ByteBuffer>();
    error = handlerOperation->Serialize(applicationTypesResult, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ApplicationTypesHandler::OnGetApplicationTypesPagedComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    // Get the actual results
    vector<ApplicationTypeQueryResult> applicationTypesResult;
    PagingStatusUPtr pagingStatus;

    auto error = client.QueryClient->EndGetApplicationTypePagedList(operation, applicationTypesResult, pagingStatus);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;

    // This different return behaviour is only for version 4. Other versions will not have it
    if (handlerOperation->Uri.ApiVersion == Constants::V4ApiVersion) {
        if (applicationTypesResult.size() == 0)
        {
            handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr), Constants::StatusNoContent, Constants::StatusDescriptionNoContent);
            return;
        }
    }

    // Create paging status and ApplicationTypeList as one object to serialize
    ApplicationTypePagedList pagedList;
    if (pagingStatus)
    {
        pagedList.ContinuationToken = pagingStatus->TakeContinuationToken();
    }

    pagedList.Items = move(applicationTypesResult);

    error = handlerOperation->Serialize(pagedList, bufferUPtr);

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ApplicationTypesHandler::GetApplicationTypesByType(__in AsyncOperationSPtr const& thisSPtr)
{
    // For this and GetAllApplicationsTypes above
    // The continuationToken comparison is done such that lower case c comes after upper case W (ordinal)
    wstring appType;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    handlerOperation->Uri.GetItem(Constants::ApplicationTypeNameString, appType);

    // If the API version is 1, then we use the non paged version of the API
    if (handlerOperation->Uri.ApiVersion <= Constants::V3ApiVersion)
    {
        AsyncOperationSPtr operation = client.QueryClient->BeginGetApplicationTypeList(
            appType,
            handlerOperation->Timeout,
            [this](AsyncOperationSPtr const& operation)
            {
                this->OnGetApplicationTypesByTypeComplete(operation, false);
            },
            handlerOperation->shared_from_this());

        OnGetApplicationTypesByTypeComplete(operation, true);
    }
    else
    {
        GetApplicationTypesPagedHelper(thisSPtr, handlerOperation, client);
    }
}

void ApplicationTypesHandler::OnGetApplicationTypesByTypeComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<ApplicationTypeQueryResult> applicationTypesResult;
    auto error = client.QueryClient->EndGetApplicationTypeList(operation, applicationTypesResult);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;

    // This different return behaviour is only for version 4. Other versions will not have it
    if (handlerOperation->Uri.ApiVersion == Constants::V4ApiVersion) {
        if (applicationTypesResult.size() == 0)
        {
            handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr), Constants::StatusNoContent, Constants::StatusDescriptionNoContent);
            return;
        }
    }

    error = handlerOperation->Serialize(applicationTypesResult, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ApplicationTypesHandler::ProvisionApplicationType(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    ProvisionApplicationTypeDescription description;
    ErrorCode error(ErrorCodeValue::Success);

    if (handlerOperation->Uri.ApiVersion <= Constants::V6ApiVersion)
    {
        error = handlerOperation->Deserialize(description, handlerOperation->Body);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(
                thisSPtr,
                ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Deserialization_Error), L"ProvisionApplicationTypeDescription")));
            return;
        }
    }
    else
    {
        // The body has either image store based provisioning or external store provisioning
        auto descriptionBase = make_shared<ProvisionApplicationTypeDescriptionBase>();
        error = handlerOperation->Deserialize(descriptionBase, handlerOperation->Body);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(
                thisSPtr,
                ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Deserialization_Error), L"ProvisionApplicationTypeDescriptionBase")));
            return;
        }

        error = description.Update(move(descriptionBase));
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }
    }

    bool isAsync = description.IsAsync;

    //
    // Make the actual FabricClient operation.
    //
    auto inner = client.AppMgmtClient->BeginProvisionApplicationType(
        description,
        handlerOperation->Timeout,
        [this, isAsync](AsyncOperationSPtr const& operation)
        {
            this->OnProvisionComplete(isAsync, operation, false);
        },
        handlerOperation->shared_from_this());

    OnProvisionComplete(isAsync, inner, true);
}

void ApplicationTypesHandler::OnProvisionComplete(
    bool isAsync,
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.AppMgmtClient->EndProvisionApplicationType(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    if (isAsync)
    {
        handlerOperation->OnSuccess(operation->Parent, move(emptyBody), Constants::StatusAccepted, *Constants::StatusDescriptionAccepted);
    }
    else
    {
        handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
    }
}

void ApplicationTypesHandler::UnprovisionApplicationType(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    wstring appTypeId;
    handlerOperation->Uri.GetItem(Constants::ApplicationTypeNameString, appTypeId);

    ApplicationTypeVersionData unprovisionData;
    auto error = handlerOperation->Deserialize(unprovisionData, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    // Can't replace ApplicationTypeVersionData with UnprovisionApplicationTypeDescription
    // since the former doesn't have the application type name as a field
    //
    Management::ClusterManager::UnprovisionApplicationTypeDescription description(
        appTypeId,
        unprovisionData.ApplicationTypeVersion,
        unprovisionData.IsAsync);

    bool isAsync = description.IsAsync;

    //
    // Make the actual FabricClient operation.
    //
    auto inner = client.AppMgmtClient->BeginUnprovisionApplicationType(
        description,
        handlerOperation->Timeout,
        [this, isAsync](AsyncOperationSPtr const& operation)
        {
            this->OnUnprovisionComplete(isAsync, operation, false);
        },
        handlerOperation->shared_from_this());

    OnUnprovisionComplete(isAsync, inner, true);
}

void ApplicationTypesHandler::OnUnprovisionComplete(
    bool isAsync,
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.AppMgmtClient->EndUnprovisionApplicationType(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    if (isAsync)
    {
        handlerOperation->OnSuccess(operation->Parent, move(emptyBody), Constants::StatusAccepted, *Constants::StatusDescriptionAccepted);
    }
    else
    {
        handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
    }
}

void ApplicationTypesHandler::GetApplicationManifest(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    wstring appTypeId, appTypeVersion;
    handlerOperation->Uri.GetItem(Constants::ApplicationTypeNameString, appTypeId);

    UriArgumentParser argumentParser(handlerOperation->Uri);
    auto error = argumentParser.TryGetApplicationTypeVersion(appTypeVersion);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto inner = client.AppMgmtClient->BeginGetApplicationManifest(
        appTypeId,
        appTypeVersion,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnGetApplicationManifestComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnGetApplicationManifestComplete(inner, true);
}

void ApplicationTypesHandler::OnGetApplicationManifestComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    wstring appManifest;

    auto error = client.AppMgmtClient->EndGetApplicationManifest(operation, appManifest);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    Manifest result(move(appManifest));
    ByteBufferUPtr bufferUPtr;

    error = handlerOperation->Serialize(result, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ApplicationTypesHandler::GetServiceManifest(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    wstring appTypeId, serviceManifestName, appTypeVersion;
    UriArgumentParser argumentParser(handlerOperation->Uri);
    auto error = argumentParser.TryGetApplicationTypeVersion(appTypeVersion);

    if (!error.IsSuccess() ||
        !handlerOperation->Uri.GetItem(Constants::ApplicationTypeNameString, appTypeId) ||
        !handlerOperation->Uri.GetItem(Constants::ServiceManifestNameString, serviceManifestName))
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    auto inner = client.ServiceMgmtClient->BeginGetServiceManifest(
        appTypeId,
        appTypeVersion,
        serviceManifestName,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnGetServiceManifestComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnGetServiceManifestComplete(inner, true);
}

void ApplicationTypesHandler::OnGetServiceManifestComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    wstring serviceManifest;

    auto error = client.ServiceMgmtClient->EndGetServiceManifest(operation, serviceManifest);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    Manifest result(move(serviceManifest));
    ByteBufferUPtr bufferUPtr;

    error = handlerOperation->Serialize(result, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ApplicationTypesHandler::GetServiceTypes(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    wstring appType, appTypeVersion;
    handlerOperation->Uri.GetItem(Constants::ApplicationTypeNameString, appType);

    UriArgumentParser argumentParser(handlerOperation->Uri);
    auto error = argumentParser.TryGetApplicationTypeVersion(appTypeVersion);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    AsyncOperationSPtr operation = client.QueryClient->BeginGetServiceTypeList(
        appType,
        appTypeVersion,
        EMPTY_STRING_QUERY_FILTER,
        handlerOperation->Timeout,
        [this] (AsyncOperationSPtr const& operation)
        {
            this->OnGetServiceTypesComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnGetServiceTypesComplete(operation, true);
}

void ApplicationTypesHandler::OnGetServiceTypesComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<ServiceTypeQueryResult> serviceTypesResult;
    auto error = client.QueryClient->EndGetServiceTypeList(operation, serviceTypesResult);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(serviceTypesResult, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ApplicationTypesHandler::GetServiceTypesByType(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    wstring appType, appTypeVersion, serviceTypeName;
    handlerOperation->Uri.GetItem(Constants::ApplicationTypeNameString, appType);
    handlerOperation->Uri.GetItem(Constants::ServiceTypeNameIdString, serviceTypeName);

    UriArgumentParser argumentParser(handlerOperation->Uri);
    auto error = argumentParser.TryGetApplicationTypeVersion(appTypeVersion);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    AsyncOperationSPtr operation = client.QueryClient->BeginGetServiceTypeList(
        appType,
        appTypeVersion,
        serviceTypeName,
        handlerOperation->Timeout,
        [this] (AsyncOperationSPtr const& operation)
        {
            this->OnGetServiceTypesByTypeComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnGetServiceTypesByTypeComplete(operation, true);
}

void ApplicationTypesHandler::OnGetServiceTypesByTypeComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<ServiceTypeQueryResult> serviceTypesResult;
    auto error = client.QueryClient->EndGetServiceTypeList(operation, serviceTypesResult);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    if (serviceTypesResult.size() == 0)
    {
        handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr), Constants::StatusNoContent, Constants::StatusDescriptionNoContent);
        return;
    }

    if (serviceTypesResult.size() != 1)
    {
        handlerOperation->OnError(operation->Parent, ErrorCodeValue::OperationFailed);
        return;
    }

    error = handlerOperation->Serialize(serviceTypesResult[0], bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

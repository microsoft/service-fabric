// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace Api;
using namespace Query;
using namespace HttpGateway;

StringLiteral const TraceType("ComposeDeploymentsHandler");

ComposeDeploymentsHandler::ComposeDeploymentsHandler(HttpGatewayImpl & server)
    : RequestHandlerBase(server)
{
}

ErrorCode ComposeDeploymentsHandler::Initialize()
{
    vector<HandlerUriTemplate> handlerUris;

    GetComposeDeploymentApiHandlers(handlerUris);
    validHandlerUris.insert(validHandlerUris.begin(), handlerUris.begin(), handlerUris.end());

    return server_.InnerServer->RegisterHandler(Constants::ComposeDeploymentsHandlerPath, shared_from_this());
}

void ComposeDeploymentsHandler::GetComposeDeploymentApiHandlers(vector<HandlerUriTemplate> & handlerUris)
{
    vector<HandlerUriTemplate> uris;

    uris.push_back(HandlerUriTemplate(
        Constants::ComposeDeploymentsEntitySetPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetAllComposeDeployments)));

    uris.push_back(HandlerUriTemplate(
        Constants::ComposeDeploymentsEntityKeyPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetComposeDeploymentById)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ComposeDeploymentsEntityKeyPath, Constants::Delete),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(DeleteComposeDeployment)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ComposeDeploymentsEntityKeyPath, Constants::Upgrade),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(UpgradeComposeDeployment)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ComposeDeploymentsEntitySetPath, Constants::Create),
        Constants::HttpPutVerb,
        MAKE_HANDLER_CALLBACK(CreateComposeDeployment)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ComposeDeploymentsEntityKeyPath, Constants::GetUpgradeProgress),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetUpgradeProgress)));

    uris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::ComposeDeploymentsEntityKeyPath, Constants::RollbackUpgrade),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(RollbackComposeDeployment)));

    handlerUris = move(uris);
}

void ComposeDeploymentsHandler::GetAllComposeDeployments(AsyncOperationSPtr const & thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring continuationToken;

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

    int64 maxResults = 0;
    error = argumentParser.TryGetMaxResults(maxResults);
    if (error.IsError(ErrorCodeValue::NameNotFound))
    {
        error = ErrorCodeValue::Success;
    }

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    AsyncOperationSPtr operation = client.ComposeAppMgmtClient->BeginGetComposeDeploymentStatusList(
        ComposeDeploymentStatusQueryDescription(move(continuationToken), maxResults),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operation)
        {
            this->OnGetAllComposeDeploymentsComplete(operation, false);
        },
        thisSPtr);
    this->OnGetAllComposeDeploymentsComplete(operation, true);
}

void ComposeDeploymentsHandler::OnGetAllComposeDeploymentsComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<ComposeDeploymentStatusQueryResult> result;
    PagingStatusUPtr pagingStatus;
    auto error = client.ComposeAppMgmtClient->EndGetComposeDeploymentStatusList(operation, result, pagingStatus);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr = make_unique<ByteBuffer>();

    ComposeDeploymentList list;
    if (pagingStatus)
    {
        list.ContinuationToken = pagingStatus->TakeContinuationToken();
    }

    list.Items = move(result);

    error = handlerOperation->Serialize(list, bufferUPtr);

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ComposeDeploymentsHandler::GetComposeDeploymentById(AsyncOperationSPtr const & thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto & client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring deploymentName;
    auto error = argumentParser.TryGetDeploymentName(deploymentName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    AsyncOperationSPtr operation = client.ComposeAppMgmtClient->BeginGetComposeDeploymentStatusList(
        ComposeDeploymentStatusQueryDescription(move(deploymentName), wstring()),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operation)
        {
            this->OnGetComposeDeploymentByIdComplete(operation, false);
        },
        thisSPtr);
    this->OnGetComposeDeploymentByIdComplete(operation, true);
}

void ComposeDeploymentsHandler::OnGetComposeDeploymentByIdComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    vector<ComposeDeploymentStatusQueryResult> result;
    PagingStatusUPtr pagingStatus;
    auto error = client.ComposeAppMgmtClient->EndGetComposeDeploymentStatusList(operation, result, pagingStatus);

    // No paging status for one compose application
    TESTASSERT_IF(pagingStatus, "OnGetComposeByIdComplete: paging status shouldn't be set");

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

void ComposeDeploymentsHandler::DeleteComposeDeployment(AsyncOperationSPtr const & thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto & client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring deploymentName;
    auto error = argumentParser.TryGetDeploymentName(deploymentName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto operation = client.ComposeAppMgmtClient->BeginDeleteComposeDeployment(
        deploymentName,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operation)
        {
            this->OnDeleteComposeDeploymentComplete(operation, false);
        },
        handlerOperation->shared_from_this());
    this->OnDeleteComposeDeploymentComplete(operation, true);
}

void ComposeDeploymentsHandler::OnDeleteComposeDeploymentComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    auto error = client.ComposeAppMgmtClient->EndDeleteComposeDeployment(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(thisSPtr, move(emptyBody), Constants::StatusAccepted, *Constants::StatusDescriptionAccepted);
}

void ComposeDeploymentsHandler::CreateComposeDeployment(AsyncOperationSPtr const & thisSPtr)
{
    //TODO: validate content type: text/yaml or text/yml
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);

    auto & client = handlerOperation->FabricClient;
    CreateComposeDeploymentData data;
    auto error = handlerOperation->Deserialize(data, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    wstring deploymentName = data.DeploymentName;

    string utf8Str;
    StringUtility::Utf16ToUtf8(data.ComposeFileContent, utf8Str);

    ByteBuffer composeFileContents(utf8Str.size() * sizeof(char));
    ByteBuffer overridesFileContents;
    KMemCpySafe(composeFileContents.data(), composeFileContents.size(), utf8Str.data(), utf8Str.size() * sizeof(char));

    auto operation = client.ComposeAppMgmtClient->BeginCreateComposeDeployment(
        deploymentName,
        move(composeFileContents),
        move(overridesFileContents),
        data.RegistryCredentialObj.RegistryUserName,
        data.RegistryCredentialObj.RegistryPassword,
        data.RegistryCredentialObj.PasswordEncrypted,
        handlerOperation->Timeout,
        [this] (AsyncOperationSPtr const & operation) { this->OnCreateComposeDeploymentComplete(operation, false); },
        handlerOperation->shared_from_this());
    this->OnCreateComposeDeploymentComplete(operation, true);
}

void ComposeDeploymentsHandler::OnCreateComposeDeploymentComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto & client = handlerOperation->FabricClient;

    auto error = client.ComposeAppMgmtClient->EndCreateComposeDeployment(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(thisSPtr, move(emptyBody), Constants::StatusAccepted, *Constants::StatusDescriptionAccepted);
}

void ComposeDeploymentsHandler::UpgradeComposeDeployment(AsyncOperationSPtr const & thisSPtr)
{
    //TODO: validate content type: text/yaml or text/yml
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto & client = handlerOperation->FabricClient;

    UpgradeComposeDeploymentData data;
    auto error = handlerOperation->Deserialize(data, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    ByteBuffer composeFileContents;
    ByteBuffer overridesFileContents;
    error = Utility::StringToBytesUtf8(data.ComposeFileContent, composeFileContents);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    vector<ByteBuffer> composeFiles;
    composeFiles.push_back(move(composeFileContents));
    vector<ByteBuffer> overridesFiles;

    ComposeDeploymentUpgradeDescription upgradeDescription(
        data.TakeDeploymentName(),
        L"",
        move(composeFiles),
        move(overridesFiles),
        data.RegistryCredentialObj.TakeUserName(),
        data.RegistryCredentialObj.TakePassword(),
        data.RegistryCredentialObj.PasswordEncrypted,
        data.UpgradeKind,
        data.UpgradeMode,
        data.ForceRestart,
        data.TakeMonitoringPolicy(),
        data.HealthPolicy != nullptr,
        data.HealthPolicy == nullptr ? ApplicationHealthPolicy() : move(*data.HealthPolicy),
        TimeSpan::FromSeconds(data.ReplicaSetTimeoutInSec));

    error = upgradeDescription.Validate();
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto operation = client.ComposeAppMgmtClient->BeginUpgradeComposeDeployment(
        upgradeDescription,
        handlerOperation->Timeout,
        [this] (AsyncOperationSPtr const & operation) { this->OnUpgradeComposeDeploymentComplete(operation, false); },
        handlerOperation->shared_from_this());
    this->OnUpgradeComposeDeploymentComplete(operation, true);
}

void ComposeDeploymentsHandler::OnUpgradeComposeDeploymentComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto & client = handlerOperation->FabricClient;

    auto error = client.ComposeAppMgmtClient->EndUpgradeComposeDeployment(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(thisSPtr, move(emptyBody), Constants::StatusAccepted, *Constants::StatusDescriptionAccepted);
}

void ComposeDeploymentsHandler::GetUpgradeProgress(AsyncOperationSPtr const & thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring deploymentName;
    auto error = argumentParser.TryGetDeploymentName(deploymentName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto operation = client.ComposeAppMgmtClient->BeginGetComposeDeploymentUpgradeProgress(
        deploymentName,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operation)
        {
            this->OnGetUpgradeProgressComplete(operation, false);
        },
        handlerOperation->shared_from_this());
    this->OnGetUpgradeProgressComplete(operation, true);
}

void ComposeDeploymentsHandler::OnGetUpgradeProgressComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }
    AsyncOperationSPtr thisSPtr = operation->Parent;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    ComposeDeploymentUpgradeProgress upgradeProgress;

    auto error = client.ComposeAppMgmtClient->EndGetComposeDeploymentUpgradeProgress(operation, upgradeProgress);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(upgradeProgress, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }
    handlerOperation->OnSuccess(thisSPtr, move(bufferUPtr));
}

void ComposeDeploymentsHandler::RollbackComposeDeployment(AsyncOperationSPtr const & thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto & client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring deploymentName;
    auto error = argumentParser.TryGetDeploymentName(deploymentName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto operation = client.ComposeAppMgmtClient->BeginRollbackComposeDeployment(
        deploymentName,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operation)
        {
            this->OnRollbackComposeDeploymentComplete(operation, false);
        },
        handlerOperation->shared_from_this());
    this->OnRollbackComposeDeploymentComplete(operation, true);
}

void ComposeDeploymentsHandler::OnRollbackComposeDeploymentComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    auto error = client.ComposeAppMgmtClient->EndRollbackComposeDeployment(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

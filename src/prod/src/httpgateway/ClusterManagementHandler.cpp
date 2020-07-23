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
using namespace Management::RepairManager;
using namespace Management::UpgradeOrchestrationService;

StringLiteral const TraceType("ClusterManagementHandler");

ClusterManagementHandler::ClusterManagementHandler(HttpGatewayImpl & server)
    : RequestHandlerBase(server)
{
    InitializeContentTypes();
}

ClusterManagementHandler::~ClusterManagementHandler()
{
}

ErrorCode ClusterManagementHandler::Initialize()
{
    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::Upgrade),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(UpgradeFabric)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::UpdateUpgrade),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(UpdateFabricUpgrade)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::RollbackUpgrade),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(RollbackFabricUpgrade)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::ToggleServicePlacementHealthReportingVerbosity),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(ToggleVerboseServicePlacementHealthReporting)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::GetUpgradeProgress),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetUpgradeProgress)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::MoveToNextUpgradeDomain),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(MoveNextUpgradeDomain)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::GetClusterManifest),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetClusterManifest)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::GetClusterVersion),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetClusterVersion)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::GetClusterHealth),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(EvaluateClusterHealth)));

    // support application health policy starting v2
    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::GetClusterHealth),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(EvaluateClusterHealth)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::Provision),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(ProvisionFabric)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::UnProvision),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(UnprovisionFabric)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::GetProvisionedFabricCodeVersions),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetProvisionedFabricCodeVersions)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath,Constants::GetProvisionedFabricConfigVersions),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetProvisionedFabricConfigVersions)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::RecoverAllPartitions),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(RecoverPartitions)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::RecoverSystemPartitions),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(RecoverSystemServicePartitions)));

    // Allow accessing metadata without authorization
    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::GetdSTSMetadata),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetTokenValidationServiceMetadata),
        true, //requireApiVersion
        true)); // allowAnonymous

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::GetAadMetadata),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetAzureActiveDirectoryMetadata),
        true, //requireApiVersion
        true)); // allowAnonymous

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::GetLoadInformation),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetClusterLoadInformation)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::ReportClusterHealth),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(ReportClusterHealth)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::InvokeInfrastructureCommand),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(InvokeInfrastructureCommand)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::InvokeInfrastructureQuery),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(InvokeInfrastructureQuery)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::CreateRepairTask),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(CreateRepairTask)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::CancelRepairTask),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(CancelRepairTask)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::ForceApproveRepairTask),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(ForceApproveRepairTask)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::DeleteRepairTask),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(DeleteRepairTask)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::UpdateRepairExecutionState),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(UpdateRepairExecutionState)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::GetRepairTaskList),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetRepairTaskList)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::GetClusterHealthChunk),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(EvaluateClusterHealthChunk)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::GetClusterHealthChunk),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(EvaluateClusterHealthChunk)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::StartClusterConfigurationUpgrade),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(StartClusterConfigurationUpgrade)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::GetClusterConfigurationUpgradeStatus),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetClusterConfigurationUpgradeStatus)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::GetClusterConfiguration),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetClusterConfiguration)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::GetUpgradeOrchestrationServiceState),
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetUpgradeOrchestrationServiceState)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::SetUpgradeOrchestrationServiceState),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(SetUpgradeOrchestrationServiceState)));

    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::UpdateRepairTaskHealthPolicy),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(UpdateRepairTaskHealthPolicy)));

    //Commented - Need Design and it is not intended for user at the moment.

    //handlerUri.SuffixPath = MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::GetUpgradesPendingApproval);
    //handlerUri.Verb = Constants::HttpGetVerb;
    //handlerUri.HandlerCallback = MAKE_HANDLER_CALLBACK(GetUpgradesPendingApproval);
    //validHandlerUris.push_back(handlerUri);

    //handlerUri.SuffixPath = MAKE_SUFFIX_PATH(Constants::SystemEntitySetPath, Constants::StartApprovedUpgrades);
    //handlerUri.Verb = Constants::HttpPostVerb;
    //handlerUri.HandlerCallback = MAKE_HANDLER_CALLBACK(StartApprovedUpgrades);
    //validHandlerUris.push_back(handlerUri);

    //
    // !!!! These registrations should always be at the end of the list. New path registrations should happen !!!!
    // !!!! above this line. !!!!
    //

    bool isAadEnabled = AzureActiveDirectory::ServerWrapper::IsAadEnabled();

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::ExplorerPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetGenericStaticFile),
        false, // requireApiVersion
        isAadEnabled));  // allowAnonymous access

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::ExplorerRedirectPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(RedirectToExplorerIndexPage),
        false, // requireApiVersion
        isAadEnabled));  // allowAnonymous access

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::ExplorerRedirectPath2,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(RedirectToExplorerIndexPage),
        false, // requireApiVersion
        isAadEnabled));  // allowAnonymous access

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::HtmlRootPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetRootPage),
        false, // requireApiVersion
        isAadEnabled));  // allowAnonymous access

    //
    // !!!! Do not add new registrations here !!!!
    //

    return server_.InnerServer->RegisterHandler(Constants::ClusterManagementHandlerPath, shared_from_this());
}

void ClusterManagementHandler::GetClusterManifest(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    wstring targetVersionString;
    ClusterManifestQueryDescription queryDescription;

    UriArgumentParser argumentParser(handlerOperation->Uri);

    auto error = argumentParser.TryGetClusterManifestVersion(targetVersionString);
    if (error.IsSuccess())
    {
        FabricConfigVersion targetVersion;
        error = targetVersion.FromString(targetVersionString);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        queryDescription = ClusterManifestQueryDescription(move(targetVersion));
    }
    else if (!error.IsError(ErrorCodeValue::InvalidArgument))
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    WriteInfo(TraceType, "GetClusterManifest: desc={0} parsed={1}", queryDescription.ConfigVersion, targetVersionString);

    auto inner = client.ClusterMgmtClient->BeginGetClusterManifest(
        queryDescription,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnGetClusterManifestComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnGetClusterManifestComplete(inner, true);
}

void ClusterManagementHandler::OnGetClusterManifestComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    wstring clusterManifest;

    auto error = client.ClusterMgmtClient->EndGetClusterManifest(operation, clusterManifest);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    Manifest result(move(clusterManifest));
    ByteBufferUPtr bufferUPtr;

    error = handlerOperation->Serialize(result, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ClusterManagementHandler::GetClusterVersion(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    auto inner = client.ClusterMgmtClient->BeginGetClusterVersion(
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetClusterVersionComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnGetClusterVersionComplete(inner, true);
}

void ClusterManagementHandler::OnGetClusterVersionComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    wstring queryResult;

    auto error = client.ClusterMgmtClient->EndGetClusterVersion(operation, queryResult);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ClusterVersion result(move(queryResult));
    ByteBufferUPtr bufferUPtr;

    error = handlerOperation->Serialize(result, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ClusterManagementHandler::StartClusterConfigurationUpgrade(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    StartUpgradeDescription description;
    auto error = JsonDeserialize(description, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    auto inner = client.ClusterMgmtClient->BeginUpgradeConfiguration(
        description,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnStartClusterConfigurationUpgradeComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnStartClusterConfigurationUpgradeComplete(inner, true);
}

void ClusterManagementHandler::OnStartClusterConfigurationUpgradeComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    auto error = client.ClusterMgmtClient->EndUpgradeConfiguration(operation);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType, "StartClusterConfigurationUpgrade failed. {0}", error);
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody), Constants::StatusAccepted, *Constants::StatusDescriptionAccepted);
}

void ClusterManagementHandler::GetClusterConfigurationUpgradeStatus(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    auto inner = client.ClusterMgmtClient->BeginGetClusterConfigurationUpgradeStatus(
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetClusterConfigurationUpgradeStatusComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnGetClusterConfigurationUpgradeStatusComplete(inner, true);
}

void ClusterManagementHandler::OnGetClusterConfigurationUpgradeStatusComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    IFabricOrchestrationUpgradeStatusResultPtr result;

    auto error = client.ClusterMgmtClient->EndGetClusterConfigurationUpgradeStatus(operation, result);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    FabricOrchestrationUpgradeProgress upgradeProgress;
    error = upgradeProgress.FromInternalInterface(result);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, ErrorCodeValue::OperationFailed);
        return;
    }

    ByteBufferUPtr bufferUPtr = make_unique<ByteBuffer>();
    error = JsonSerialize(upgradeProgress, bufferUPtr);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType, "GetClusterConfigurationUpgradeStatus failed. {0}", error);
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ClusterManagementHandler::GetClusterConfiguration(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);

    auto &client = handlerOperation->FabricClient;
    wstring apiVersion;

    if (handlerOperation->Uri.ApiVersion >= Constants::V6ApiVersion)
    {
        UriArgumentParser argumentParser(handlerOperation->Uri);
        auto error = argumentParser.TryGetClusterConfigurationApiVersion(apiVersion);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }
    }

    WriteInfo(TraceType, "GetClusterConfiguration: ConfigurationApiVersion={0}", apiVersion);

    auto inner = client.ClusterMgmtClient->BeginGetClusterConfiguration(
        apiVersion,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetClusterConfigurationComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnGetClusterConfigurationComplete(inner, true);
}

void ClusterManagementHandler::OnGetClusterConfigurationComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    wstring result;

    auto error = client.ClusterMgmtClient->EndGetClusterConfiguration(operation, result);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ClusterConfiguration clusterConfiguration(move(result));
    ByteBufferUPtr bufferUPtr = make_unique<ByteBuffer>();
    error = JsonSerialize(clusterConfiguration, bufferUPtr);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType, "GetClusterConfiguration failed. {0}", error);
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ClusterManagementHandler::GetUpgradeOrchestrationServiceState(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    auto inner = client.ClusterMgmtClient->BeginGetUpgradeOrchestrationServiceState(
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnGetUpgradeOrchestrationServiceStateComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnGetUpgradeOrchestrationServiceStateComplete(inner, true);
}

void ClusterManagementHandler::OnGetUpgradeOrchestrationServiceStateComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    wstring result;

    auto error = client.ClusterMgmtClient->EndGetUpgradeOrchestrationServiceState(operation, result);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    UpgradeOrchestrationServiceStateString serviceState(move(result));
    ByteBufferUPtr bufferUPtr = make_unique<ByteBuffer>();
    error = JsonSerialize(serviceState, bufferUPtr);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType, "GetUpgradeOrchestrationServiceState failed. {0}", error);
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ClusterManagementHandler::SetUpgradeOrchestrationServiceState(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    UpgradeOrchestrationServiceStateString data;
    auto error = JsonDeserialize(data, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    auto inner = client.ClusterMgmtClient->BeginSetUpgradeOrchestrationServiceState(
        data.ServiceState,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnSetUpgradeOrchestrationServiceStateComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnSetUpgradeOrchestrationServiceStateComplete(inner, true);
}

void ClusterManagementHandler::OnSetUpgradeOrchestrationServiceStateComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    IFabricUpgradeOrchestrationServiceStateResultPtr result;

    auto error = client.ClusterMgmtClient->EndSetUpgradeOrchestrationServiceState(operation, result);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    std::shared_ptr<UpgradeOrchestrationServiceState> serviceStateSPtr = result->GetState();
    ByteBufferUPtr bufferUPtr = make_unique<ByteBuffer>();
    error = JsonSerialize(*(serviceStateSPtr.get()), bufferUPtr);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType, "SetUpgradeOrchestrationServiceState failed. {0}", error);
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ClusterManagementHandler::GetUpgradesPendingApproval(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    auto inner = client.ClusterMgmtClient->BeginGetUpgradesPendingApproval(
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetUpgradesPendingApprovalComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnGetUpgradesPendingApprovalComplete(inner, true);
}

void ClusterManagementHandler::OnGetUpgradesPendingApprovalComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    auto error = client.ClusterMgmtClient->EndGetUpgradesPendingApproval(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody), Constants::StatusAccepted, *Constants::StatusDescriptionAccepted);
}

void ClusterManagementHandler::StartApprovedUpgrades(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    auto inner = client.ClusterMgmtClient->BeginStartApprovedUpgrades(
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnStartApprovedUpgradesComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnStartApprovedUpgradesComplete(inner, true);
}

void ClusterManagementHandler::OnStartApprovedUpgradesComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    auto error = client.ClusterMgmtClient->EndStartApprovedUpgrades(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody), Constants::StatusAccepted, *Constants::StatusDescriptionAccepted);
}

void ClusterManagementHandler::EvaluateClusterHealth(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    UriArgumentParser argumentParser(handlerOperation->Uri);

    ClusterHealthQueryDescription queryDescription;

    if (handlerOperation->Uri.Verb == Constants::HttpPostVerb && !handlerOperation->Body->empty())
    {
        if (handlerOperation->Uri.ApiVersion == Constants::V1ApiVersion)
        {
            auto healthPolicy = make_unique<ClusterHealthPolicy>();

            auto error = handlerOperation->Deserialize(*healthPolicy, handlerOperation->Body);
            if (!error.IsSuccess())
            {
                handlerOperation->OnError(
                    thisSPtr,
                    ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Deserialization_Error), L"ClusterHealthPolicy")));
                return;
            }

            queryDescription.SetClusterHealthPolicy(move(healthPolicy));
        }
        else //if(handlerOperation->Uri.ApiVersion == Constants::V2ApiVersion)
        {
            ClusterHealthPolicies policies;
            auto error = handlerOperation->Deserialize(policies, handlerOperation->Body);
            if (!error.IsSuccess())
            {
                handlerOperation->OnError(
                    thisSPtr,
                    ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Deserialization_Error), L"ClusterHealthPolicies")));
                return;
            }

            queryDescription.SetClusterHealthPolicy(make_unique<ClusterHealthPolicy>(move(policies.ClusterHealthPolicyEntry)));
            queryDescription.SetApplicationHealthPolicies(move(policies.ApplicationHealthPolicies));
        }
    }

    wstring eventsFilterString;
    if (handlerOperation->Uri.GetItem(Constants::EventsHealthStateFilterString, eventsFilterString))
    {
        DWORD filter;
        auto error = Utility::TryParseQueryFilter(eventsFilterString, FABRIC_HEALTH_STATE_FILTER_DEFAULT, filter);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        queryDescription.SetEventsFilter(make_unique<HealthEventsFilter>(filter));
    }

    wstring nodesFilterString;
    if (handlerOperation->Uri.GetItem(Constants::NodesHealthStateFilterString, nodesFilterString))
    {
        DWORD filter;
        auto error = Utility::TryParseQueryFilter(nodesFilterString, FABRIC_HEALTH_STATE_FILTER_DEFAULT, filter);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        queryDescription.SetNodesFilter(make_unique<NodeHealthStatesFilter>(filter));
    }

    wstring applicationsFilterString;
    if (handlerOperation->Uri.GetItem(Constants::ApplicationsHealthStateFilterString, applicationsFilterString))
    {
        DWORD filter;
        auto error = Utility::TryParseQueryFilter(applicationsFilterString, FABRIC_HEALTH_STATE_FILTER_DEFAULT, filter);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        queryDescription.SetApplicationsFilter(make_unique<ApplicationHealthStatesFilter>(filter));
    }

    ClusterHealthStatisticsFilterUPtr statsFilter;
    bool excludeStatsFilter = false;
    auto error = argumentParser.TryGetExcludeStatisticsFilter(excludeStatsFilter);
    if (error.IsSuccess())
    {
        statsFilter = make_unique<ClusterHealthStatisticsFilter>(excludeStatsFilter);
    }
    else if (!error.IsError(ErrorCodeValue::NotFound))
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    bool includeSystemApps;
    error = argumentParser.TryGetIncludeSystemApplicationHealthStatisticsFilter(includeSystemApps);
    if (error.IsSuccess())
    {
        if (!statsFilter)
        {
            statsFilter = make_unique<ClusterHealthStatisticsFilter>(false, includeSystemApps);
        }
        else
        {
            if (excludeStatsFilter && includeSystemApps)
            {
                handlerOperation->OnError(thisSPtr, ErrorCode(ErrorCodeValue::InvalidArgument, GET_HM_RC(IncludeSystemApplication_With_ExcludeStats)));
            }

            statsFilter->IncludeSystemApplicationHealthStatistics = includeSystemApps;
        }
    }
    else if (!error.IsError(ErrorCodeValue::NotFound))
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    if (statsFilter)
    {
        queryDescription.SetStatisticsFilter(move(statsFilter));
    }

    AsyncOperationSPtr operation = client.HealthClient->BeginGetClusterHealth(
        queryDescription,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnEvaluateClusterHealthComplete(operation, false);
    },
        thisSPtr);

    OnEvaluateClusterHealthComplete(operation, true);
}

void ClusterManagementHandler::OnEvaluateClusterHealthComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    ClusterHealth health;
    auto error = client.HealthClient->EndGetClusterHealth(operation, health);
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

void ClusterManagementHandler::EvaluateClusterHealthChunk(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    ClusterHealthChunkQueryDescription queryDescription;

    if (handlerOperation->Uri.Verb == Constants::HttpPostVerb && !handlerOperation->Body->empty())
    {
        auto error = handlerOperation->Deserialize(queryDescription, handlerOperation->Body);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(
                thisSPtr,
                ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Deserialization_Error), L"ClusterHealthChunkQueryDescription")));
            return;
        }

        error = queryDescription.ValidateFilters();
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }
    }

    AsyncOperationSPtr operation = client.HealthClient->BeginGetClusterHealthChunk(
        move(queryDescription),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnEvaluateClusterHealthChunkComplete(operation, false);
    },
        thisSPtr);

    OnEvaluateClusterHealthChunkComplete(operation, true);
}

void ClusterManagementHandler::OnEvaluateClusterHealthChunkComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    ClusterHealthChunk health;
    auto error = client.HealthClient->EndGetClusterHealthChunk(operation, health);
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

void ClusterManagementHandler::UpgradeFabric(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    FabricUpgradeDescriptionWrapper wrapper;

    auto error = handlerOperation->Deserialize(wrapper, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    //
    // Make the actual fabricclient operation.
    //
    auto inner = client.ClusterMgmtClient->BeginUpgradeFabric(
        wrapper,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnUpgradeFabricComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnUpgradeFabricComplete(inner, true);
}

void ClusterManagementHandler::OnUpgradeFabricComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.ClusterMgmtClient->EndUpgradeFabric(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody), Constants::StatusAccepted, *Constants::StatusDescriptionAccepted);
}

void ClusterManagementHandler::UpdateFabricUpgrade(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    FabricUpgradeUpdateDescription updateDescription;
    auto error = handlerOperation->Deserialize(updateDescription, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    //
    // Make the actual fabricclient operation.
    //
    auto inner = client.ClusterMgmtClient->BeginUpdateFabricUpgrade(
        updateDescription,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnUpdateFabricUpgradeComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnUpdateFabricUpgradeComplete(inner, true);
}

void ClusterManagementHandler::OnUpdateFabricUpgradeComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.ClusterMgmtClient->EndUpdateFabricUpgrade(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void ClusterManagementHandler::RollbackFabricUpgrade(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    //
    // Make the actual fabricclient operation.
    //
    auto inner = client.ClusterMgmtClient->BeginRollbackFabricUpgrade(
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnRollbackFabricUpgradeComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnRollbackFabricUpgradeComplete(inner, true);
}

void ClusterManagementHandler::OnRollbackFabricUpgradeComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.ClusterMgmtClient->EndRollbackFabricUpgrade(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody), Constants::StatusAccepted, *Constants::StatusDescriptionAccepted);
}

void ClusterManagementHandler::ToggleVerboseServicePlacementHealthReporting(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    bool enabled;
    handlerOperation->Uri.GetBool(L"Enabled", enabled);

    AsyncOperationSPtr operation = client.ClusterMgmtClient->BeginToggleVerboseServicePlacementHealthReporting(
        enabled,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnToggleVerboseServicePlacementHealthReportingComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnToggleVerboseServicePlacementHealthReportingComplete(operation, true);
}

void ClusterManagementHandler::OnToggleVerboseServicePlacementHealthReportingComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    auto error = client.ClusterMgmtClient->EndToggleVerboseServicePlacementHealthReporting(operation);

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void ClusterManagementHandler::GetUpgradeProgress(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    auto inner = client.ClusterMgmtClient->BeginGetFabricUpgradeProgress(
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetUpgradeProgressComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnGetUpgradeProgressComplete(inner, true);
}

void ClusterManagementHandler::OnGetUpgradeProgressComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    IUpgradeProgressResultPtr resultPtr;

    auto error = client.ClusterMgmtClient->EndGetFabricUpgradeProgress(operation, resultPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    FabricUpgradeProgress upgradeProgress;
    upgradeProgress.FromInternalInterface(resultPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, ErrorCodeValue::OperationFailed);
        return;
    }

    ByteBufferUPtr bufferUPtr = make_unique<ByteBuffer>();
    error = handlerOperation->Serialize(upgradeProgress, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ClusterManagementHandler::MoveNextUpgradeDomain(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    MoveNextUpgradeDomainData data;

    auto error = handlerOperation->Deserialize(data, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    //
    // Make the actual fabricclient operation.
    //
    auto inner = client.ClusterMgmtClient->BeginMoveNextFabricUpgradeDomain2(
        data.NextUpgradeDomain,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnMoveNextUpgradeDomainComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnMoveNextUpgradeDomainComplete(inner, true);
}

void ClusterManagementHandler::OnMoveNextUpgradeDomainComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.ClusterMgmtClient->EndMoveNextFabricUpgradeDomain2(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void ClusterManagementHandler::ProvisionFabric(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    ProvisionFabricData provisionData;

    auto error = handlerOperation->Deserialize(provisionData, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    if (provisionData.ClusterManifestFilePath.empty() && provisionData.CodeFilePath.empty())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    if (provisionData.CodeFilePath.length() > ParameterValidator::MaxFilePathSize ||
        provisionData.ClusterManifestFilePath.length() > ParameterValidator::MaxFilePathSize)
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    //
    // Make the actual fabricclient operation.
    //
    auto inner = client.ClusterMgmtClient->BeginProvisionFabric(
        provisionData.CodeFilePath,
        provisionData.ClusterManifestFilePath,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnProvisionComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnProvisionComplete(inner, true);
}

void ClusterManagementHandler::OnProvisionComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.ClusterMgmtClient->EndProvisionFabric(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void ClusterManagementHandler::UnprovisionFabric(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UnprovisionFabricData unprovisionData;

    auto error = handlerOperation->Deserialize(unprovisionData, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    FabricCodeVersion codeVersion = FabricCodeVersion::Invalid;
    FabricConfigVersion configVersion = FabricConfigVersion::Invalid;

    if (!unprovisionData.CodeVersion.empty())
    {
        if (!FabricCodeVersion::TryParse(unprovisionData.CodeVersion, codeVersion))
        {
            handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
            return;
        }
    }

    if (!unprovisionData.ConfigVersion.empty())
    {
        if (!FabricConfigVersion::TryParse(unprovisionData.ConfigVersion, configVersion))
        {
            handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
            return;
        }
    }

    //
    // Make the actual fabricclient operation.
    //
    auto inner = client.ClusterMgmtClient->BeginUnprovisionFabric(
        codeVersion,
        configVersion,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnUnprovisionComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnUnprovisionComplete(inner, true);
}

void ClusterManagementHandler::OnUnprovisionComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.ClusterMgmtClient->EndUnprovisionFabric(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void ClusterManagementHandler::GetProvisionedFabricCodeVersions(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    wstring codeVersionFilter;
    handlerOperation->Uri.GetItem(QueryResourceProperties::Cluster::CodeVersionFilter, codeVersionFilter);

    //
    // Make the actual fabricclient operation.
    //
    auto inner = client.QueryClient->BeginGetProvisionedFabricCodeVersionList(
        codeVersionFilter,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetProvisionedFabricCodeVersionsComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnGetProvisionedFabricCodeVersionsComplete(inner, true);
}

void ClusterManagementHandler::OnGetProvisionedFabricCodeVersionsComplete(AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<ProvisionedFabricCodeVersionQueryResultItem> result;
    auto error = client.QueryClient->EndGetProvisionedFabricCodeVersionList(operation, result);
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

void ClusterManagementHandler::GetProvisionedFabricConfigVersions(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    wstring configVersionFilter;
    handlerOperation->Uri.GetItem(QueryResourceProperties::Cluster::ConfigVersionFilter, configVersionFilter);

    //
    // Make the actual fabricclient operation.
    //
    auto inner = client.QueryClient->BeginGetProvisionedFabricConfigVersionList(
        configVersionFilter,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetProvisionedFabricConfigVersionsComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnGetProvisionedFabricConfigVersionsComplete(inner, true);
}

void ClusterManagementHandler::OnGetProvisionedFabricConfigVersionsComplete(AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<ProvisionedFabricConfigVersionQueryResultItem> result;
    auto error = client.QueryClient->EndGetProvisionedFabricConfigVersionList(operation, result);
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

void ClusterManagementHandler::GetTokenValidationServiceMetadata(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    auto inner = client.TvsClient->BeginGetMetadata(
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetMetadataComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnGetMetadataComplete(inner, true);
}

void ClusterManagementHandler::OnGetMetadataComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    TokenValidationServiceMetadata metadata;

    auto error = client.TvsClient->EndGetMetadata(operation, metadata);
    if (!error.IsSuccess())
    {
        //
        // Metadata is static information given by the TVS. If there is an error retrieving it,
        // send a re-triable error so that the client can retry.
        //
        handlerOperation->OnError(operation->Parent, ErrorCodeValue::NotReady);
        return;
    }

    ByteBufferUPtr bufferUPtr = make_unique<ByteBuffer>();
    error = handlerOperation->Serialize(metadata, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ClusterManagementHandler::GetAzureActiveDirectoryMetadata(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto metadata = AzureActiveDirectory::ServerWrapper::IsAadEnabled()
        ? Transport::ClaimsRetrievalMetadata::CreateForAAD()
        : Transport::ClaimsRetrievalMetadata();

    auto bufferUPtr = make_unique<ByteBuffer>();
    auto error = handlerOperation->Serialize(metadata, bufferUPtr);

    if (error.IsSuccess())
    {
        handlerOperation->OnSuccess(thisSPtr, move(bufferUPtr));
    }
    else
    {
        handlerOperation->OnError(thisSPtr, error);
    }
}

void ClusterManagementHandler::RecoverPartitions(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    auto inner = client.ClusterMgmtClient->BeginRecoverPartitions(
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnRecoverPartitionsComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnRecoverPartitionsComplete(inner, true);
}

void ClusterManagementHandler::OnRecoverPartitionsComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.ClusterMgmtClient->EndRecoverPartitions(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void ClusterManagementHandler::RecoverSystemServicePartitions(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    auto inner = client.ClusterMgmtClient->BeginRecoverSystemPartitions(
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnRecoverSystemServicePartitionsComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnRecoverSystemServicePartitionsComplete(inner, true);
}

void ClusterManagementHandler::OnRecoverSystemServicePartitionsComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.ClusterMgmtClient->EndRecoverSystemPartitions(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void ClusterManagementHandler::GetClusterLoadInformation(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    auto inner = client.QueryClient->BeginGetClusterLoadInformation(
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetClusterLoadInformationComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnGetClusterLoadInformationComplete(inner, true);
}

void ClusterManagementHandler::OnGetClusterLoadInformationComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    ClusterLoadInformationQueryResult queryResult;

    auto error = client.QueryClient->EndGetClusterLoadInformation(operation, queryResult);
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

void ClusterManagementHandler::ReportClusterHealth(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    HealthInformation healthInfo;
    auto error = handlerOperation->Deserialize(healthInfo, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(
            thisSPtr,
            ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_HTTP_GATEWAY_RC(Deserialization_Error), L"HealthInformation")));
        return;
    }

    AttributeList attributeList;
    HealthReport healthReport(
        EntityHealthInformation::CreateClusterEntityHealthInformation(),
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

void ClusterManagementHandler::InvokeInfrastructureCommand(AsyncOperationSPtr const& thisSPtr)
{
    InnerInvokeInfrastructureService(true, thisSPtr);
}

void ClusterManagementHandler::InvokeInfrastructureQuery(AsyncOperationSPtr const& thisSPtr)
{
    InnerInvokeInfrastructureService(false, thisSPtr);
}

void ClusterManagementHandler::InnerInvokeInfrastructureService(bool isAdminCommand, AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    NamingUri serviceName;
    auto error = argumentParser.TryGetServiceName(serviceName);
    if (!error.IsSuccess())
    {
        if (error.IsError(ErrorCodeValue::NameNotFound))
        {
            NamingUri::TryParse(*SystemServiceApplicationNameHelper::PublicInfrastructureServiceName, serviceName);
        }
        else
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }
    }

    wstring unescapedCommand;
    wstring command;
    if (handlerOperation->Uri.GetItem(Constants::CommandString, command))
    {
        error = NamingUri::UnescapeString(command, unescapedCommand);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }
    }

    auto inner = client.InfrastructureClient->BeginInvokeInfrastructureCommand(
        isAdminCommand,
        serviceName,
        unescapedCommand,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnInvokeInfrastructureServiceComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnInvokeInfrastructureServiceComplete(inner, true);
}

void ClusterManagementHandler::OnInvokeInfrastructureServiceComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    wstring commandResponse;
    auto error = client.InfrastructureClient->EndInvokeInfrastructureCommand(operation, commandResponse);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;

    if (!commandResponse.empty())
    {
        bufferUPtr = make_unique<ByteBuffer>();

        error = Utility::StringToBytesUtf8(commandResponse, *bufferUPtr);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(operation->Parent, error);
            return;
        }
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ClusterManagementHandler::CreateRepairTask(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    RepairTask repairTask;

    auto error = handlerOperation->Deserialize(repairTask, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    // Overwrite any user-provided scope, since the scope is determined by the URI prefix
    repairTask.ReplaceScope(make_shared<ClusterRepairScopeIdentifier>());

    auto inner = client.RepairMgmtClient->BeginCreateRepairTask(
        repairTask,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnCreateRepairTaskComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnCreateRepairTaskComplete(inner, true);
}

void ClusterManagementHandler::OnCreateRepairTaskComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    int64 commitVersion = 0;

    auto error = client.RepairMgmtClient->EndCreateRepairTask(operation, commitVersion);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    CommitRepairTaskResponse responseObject(commitVersion);
    ByteBufferUPtr bufferUPtr;

    error = handlerOperation->Serialize(responseObject, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ClusterManagementHandler::CancelRepairTask(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    CancelRepairTaskMessageBody requestBody;

    auto error = handlerOperation->Deserialize(requestBody, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    // Overwrite any user-provided scope, since the scope is determined by the URI prefix
    requestBody.ReplaceScope(make_shared<ClusterRepairScopeIdentifier>());

    auto inner = client.RepairMgmtClient->BeginCancelRepairTask(
        requestBody,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnCancelRepairTaskComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnCancelRepairTaskComplete(inner, true);
}

void ClusterManagementHandler::OnCancelRepairTaskComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    int64 commitVersion = 0;

    auto error = client.RepairMgmtClient->EndCancelRepairTask(operation, commitVersion);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    CommitRepairTaskResponse responseObject(commitVersion);
    ByteBufferUPtr bufferUPtr;

    error = handlerOperation->Serialize(responseObject, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ClusterManagementHandler::ForceApproveRepairTask(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    ApproveRepairTaskMessageBody requestBody;

    auto error = handlerOperation->Deserialize(requestBody, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    // Overwrite any user-provided scope, since the scope is determined by the URI prefix
    requestBody.ReplaceScope(make_shared<ClusterRepairScopeIdentifier>());

    auto inner = client.RepairMgmtClient->BeginForceApproveRepairTask(
        requestBody,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnForceApproveRepairTaskComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnForceApproveRepairTaskComplete(inner, true);
}

void ClusterManagementHandler::OnForceApproveRepairTaskComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    int64 commitVersion = 0;

    auto error = client.RepairMgmtClient->EndForceApproveRepairTask(operation, commitVersion);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    CommitRepairTaskResponse responseObject(commitVersion);
    ByteBufferUPtr bufferUPtr;

    error = handlerOperation->Serialize(responseObject, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ClusterManagementHandler::DeleteRepairTask(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    DeleteRepairTaskMessageBody requestBody;

    auto error = handlerOperation->Deserialize(requestBody, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    // Overwrite any user-provided scope, since the scope is determined by the URI prefix
    requestBody.ReplaceScope(make_shared<ClusterRepairScopeIdentifier>());

    auto inner = client.RepairMgmtClient->BeginDeleteRepairTask(
        requestBody,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnDeleteRepairTaskComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnDeleteRepairTaskComplete(inner, true);
}

void ClusterManagementHandler::OnDeleteRepairTaskComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.RepairMgmtClient->EndDeleteRepairTask(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ClusterManagementHandler::UpdateRepairExecutionState(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    RepairTask repairTask;

    auto error = handlerOperation->Deserialize(repairTask, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    // Overwrite any user-provided scope, since the scope is determined by the URI prefix
    repairTask.ReplaceScope(make_shared<ClusterRepairScopeIdentifier>());

    auto inner = client.RepairMgmtClient->BeginUpdateRepairExecutionState(
        repairTask,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnUpdateRepairExecutionStateComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    OnUpdateRepairExecutionStateComplete(inner, true);
}

void ClusterManagementHandler::OnUpdateRepairExecutionStateComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    int64 commitVersion = 0;

    auto error = client.RepairMgmtClient->EndUpdateRepairExecutionState(operation, commitVersion);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    CommitRepairTaskResponse responseObject(commitVersion);
    ByteBufferUPtr bufferUPtr;

    error = handlerOperation->Serialize(responseObject, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ClusterManagementHandler::GetRepairTaskList(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    wstring taskIdFilterString;
    if (handlerOperation->Uri.GetItem(Constants::TaskIdFilter, taskIdFilterString))
    {
        wstring unescaped;
        auto error = NamingUri::UnescapeString(taskIdFilterString, unescaped);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        taskIdFilterString = move(unescaped);
    }

    RepairTaskState::Enum stateFilter = RepairTaskState::Invalid;
    wstring stateFilterString;
    if (handlerOperation->Uri.GetItem(Constants::StateFilter, stateFilterString))
    {
        DWORD filter;
        auto error = Utility::TryParseQueryFilter(stateFilterString, FABRIC_REPAIR_TASK_STATE_FILTER_DEFAULT, filter);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        if (filter != (filter & FABRIC_REPAIR_TASK_STATE_FILTER_ALL))
        {
            handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
            return;
        }

        stateFilter = static_cast<RepairTaskState::Enum>(filter);
    }

    wstring executorFilterString;
    if (handlerOperation->Uri.GetItem(Constants::ExecutorFilter, executorFilterString))
    {
        wstring unescaped;
        auto error = NamingUri::UnescapeString(executorFilterString, unescaped);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        executorFilterString = move(unescaped);
    }

    ClusterRepairScopeIdentifier scope;

    AsyncOperationSPtr operation = client.RepairMgmtClient->BeginGetRepairTaskList(
        scope,
        taskIdFilterString,
        stateFilter,
        executorFilterString,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
    {
        this->OnGetRepairTaskListComplete(operation, false);
    },
        thisSPtr);

    OnGetRepairTaskListComplete(operation, true);
}

void ClusterManagementHandler::OnGetRepairTaskListComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<RepairTask> repairTasksResult;
    auto error = client.RepairMgmtClient->EndGetRepairTaskList(operation, repairTasksResult);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    error = handlerOperation->Serialize(repairTasksResult, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ClusterManagementHandler::UpdateRepairTaskHealthPolicy(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    UpdateRepairTaskHealthPolicyMessageBody requestBody;

    auto error = JsonDeserialize(requestBody, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    // Overwrite any user-provided scope, since the scope is determined by the URI prefix
    requestBody.ReplaceScope(make_shared<ClusterRepairScopeIdentifier>());

    auto inner = client.RepairMgmtClient->BeginUpdateRepairTaskHealthPolicy(
        requestBody,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnUpdateRepairTaskHealthPolicyComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnUpdateRepairTaskHealthPolicyComplete(inner, true);
}

void ClusterManagementHandler::OnUpdateRepairTaskHealthPolicyComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    int64 commitVersion = 0;

    auto error = client.RepairMgmtClient->EndUpdateRepairTaskHealthPolicy(operation, commitVersion);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    CommitRepairTaskResponse responseObject(commitVersion);
    ByteBufferUPtr bufferUPtr;

    error = JsonSerialize(responseObject, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void ClusterManagementHandler::InitializeContentTypes()
{
    contentTypes[*Constants::HtmlExtension] = Constants::HtmlContentType;
    contentTypes[*Constants::JavaScriptExtension] = Constants::JavaScriptContentType;
    contentTypes[*Constants::CssExtension] = Constants::CssContentType;
    contentTypes[*Constants::PngExtension] = Constants::PngContentType;
    contentTypes[*Constants::IcoExtension] = Constants::IcoContentType;
    contentTypes[*Constants::EotExtension] = Constants::EotContentType;
    contentTypes[*Constants::SvgExtension] = Constants::SvgContentType;
    contentTypes[*Constants::TtfExtension] = Constants::TtfContentType;
    contentTypes[*Constants::WoffExtension] = Constants::WoffContentType;
    contentTypes[*Constants::Woff2Extension] = Constants::Woff2ContentType;
    contentTypes[*Constants::MapExtension] = Constants::MapContentType;
}

void ClusterManagementHandler::GetGenericStaticFile(AsyncOperationSPtr const& operation)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation);
    GatewayUri uri(handlerOperation->Uri);

    wstring path;
    bool success = uri.GetItem(Constants::StaticContentPathArg, path);
    if (!success)
    {
        handlerOperation->OnError(operation, ErrorCodeValue::NameNotFound);
        return;
    }

    path = Path::Combine(*Constants::ExplorerFilesRootPath, path);
    RespondWithContentFromFile(path, operation);
}

void ClusterManagementHandler::RedirectToExplorerIndexPage(AsyncOperationSPtr const& operation)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation);
    ByteBufferUPtr bufferUPtr;

    auto error = handlerOperation->MessageContext.SetResponseHeader(Constants::LocationHeader, Constants::ExplorerRootPath);
    if (!error.IsSuccess())
    {
        handlerOperation->TryComplete(operation, error);
        return;
    }

    auto redirectOperation = handlerOperation->MessageContext.BeginSendResponse(
        Constants::StatusMovedPermanently,
        *Constants::ServerRedirectResponse,
        move(bufferUPtr),
        [](AsyncOperationSPtr const& redirectOperation)
    {
        auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(redirectOperation->Parent);
        auto error = handlerOperation->MessageContext.EndSendResponse(redirectOperation);
        handlerOperation->TryComplete(redirectOperation->Parent, error);
    },
        operation);
}

void ClusterManagementHandler::GetRootPage(AsyncOperationSPtr const& operation)
{
    RespondWithContentFromFile(Constants::RootPageName, operation);
}

void ClusterManagementHandler::RespondWithContentFromFile(wstring const& inFileName, AsyncOperationSPtr const& operation)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation);

    wstring fileName = inFileName;
    StringUtility::Replace<std::wstring>(fileName, *Constants::SegmentDelimiter, *Constants::DirectorySeparator); // Switch to Windows path delimiters
    wstring fileExtension = Path::GetExtension(fileName);

    // Check for valid file name
    ErrorCode error = this->ValidateFileName(fileName, fileExtension);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation, error);
        return;
    }

    wstring fullPath = Path::Combine(*Constants::StaticFilesRootPath, fileName);

    WriteNoise(TraceType, "Responding with the content from {0}", fullPath);

    wstring contentType = contentTypes[fileExtension];

    //We will revert back to the async file read once we root cause bug no : 5917518
#ifdef PLATFORM_UNIX
    ByteBufferUPtr bufferUPtr;
    error = ReadFileToBufferSyncForLinux(fullPath, bufferUPtr);
    if (error.IsSuccess())
    {
        handlerOperation->OnSuccess(operation, move(bufferUPtr), contentType);
    }
    else
    {
        handlerOperation->OnError(operation, error);
        return;
    }
#else

    auto readFileOperation = Common::AsyncFile::BeginReadFile(
        fullPath,
        handlerOperation->Timeout,
        [this, contentType](AsyncOperationSPtr const& readFileOperation)
    {
        this->OnRespondWithContentFromFileComplete(readFileOperation, contentType);
    },
        operation);

#endif
}

#ifdef PLATFORM_UNIX
ErrorCode ClusterManagementHandler::ReadFileToBufferSyncForLinux(std::wstring const& path, __out Common::ByteBufferUPtr& bufferUPtr)
{
    WriteNoise(TraceType, "Responding with the content from {0}", path);

    File file;
    auto error = file.TryOpen(path, FileMode::Open, FileAccess::Read, FileShare::Read, FileAttributes::Normal);
    if (!error.IsSuccess())
    {
        WriteInfo(TraceType, "Could not open file '{0}' for read: error = {1}.", path, error);
        return error;
    }

    int fileSize = static_cast<int>(file.size());
    bufferUPtr = make_unique<ByteBuffer>();
    bufferUPtr->resize(fileSize);

    DWORD bytesRead = 0;
    error = file.TryRead2(reinterpret_cast<void*>(bufferUPtr->data()), fileSize, bytesRead);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType, "Failed to read file '{0}': error {1}, expected {2} bytes, read {3} bytes.", path, error, fileSize, bytesRead);
        return error;
    }

    if (bytesRead != fileSize)
    {
        WriteWarning(TraceType, "Failed to read all bytes from file '{0}': expected {1} bytes, read {2} bytes.", path, fileSize, bytesRead);
        return ErrorCodeValue::OperationFailed;
    }

    return ErrorCode::Success();
}

#else

void ClusterManagementHandler::OnRespondWithContentFromFileComplete(Common::AsyncOperationSPtr const& operation, std::wstring const &contentTypeValue)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);

    ByteBuffer buffer;
    ErrorCode result = Common::AsyncFile::EndReadFile(operation, buffer);
    ByteBufferUPtr bufferUPtr = make_unique<ByteBuffer>(buffer);
    if (result.IsSuccess())
    {
        handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr), contentTypeValue);
    }
    else
    {
        handlerOperation->OnError(operation->Parent, result);
    }
}
#endif

ErrorCode ClusterManagementHandler::ValidateFileName(wstring const& path, std::wstring const& fileExtension)
{
    // Checking to make sure that no path traversal is going on.
    if (path.find(*Constants::PathTraversalDisallowedString) != wstring::npos)
    {
        WriteInfo(TraceType, "Rejecting URL {0} because it may contain a path traversal attack.", path);
        return ErrorCodeValue::InvalidArgument;
    }

    // Check for valid extension
    if (fileExtension.empty() || contentTypes.count(fileExtension) == 0)
    {
        WriteInfo(TraceType, "Rejecting URL {0} because extension {1} was not recognized.", path, fileExtension);
        return ErrorCodeValue::NotFound;
    }

    return ErrorCodeValue::Success;
}

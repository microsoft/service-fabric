// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Management/healthmanager/HealthManagerPointers.Internal.h"
#include "Management/healthmanager/HealthManagerReplica.h"
#include "Management/DnsService/config/DnsValidateName.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Naming;
using namespace Reliability;
using namespace ServiceModel;
using namespace ServiceModel::ModelV2;
using namespace Transport;
using namespace Query;
using namespace Store;
using namespace SystemServices;
using namespace Management;
using namespace Management::ClusterManager;
using namespace Management::HealthManager;
using namespace Api;
using namespace ClientServerTransport;
using namespace Query;

StringLiteral const TraceComponent("Replica");
StringLiteral const FailedTraceComponent("ReplicaFailed");
StringLiteral const TraceComponentOpen("Open");
StringLiteral const TraceComponentChangeRole("ChangeRole");
StringLiteral const TraceComponentClose("Close");
StringLiteral const CleanupAppTypeTimerTag("AppTypeCleanupTimer");

// **************
// Helper classes
// **************

// Completes accepting a client operation by committing the transaction and enqueueing
// the context for internal processing.
//
// If this operation is still pending (was already accepted previously), then the caller
// should have already updated the existing context on disk. These updates will be picked
// up by the RolloutManager upon its next refresh (if any). Currently, the relevant updates include:
//
// - Changing the internal processing operation timeout from a new client request
// - Setting the cancel flag on application upgrades
// - Reporting healthy upgrade domains to the upgrade process
//
// If there were no changes, then we optimize by not committing the transaction. However,
// we *must* still attempt to enqueue the context since this could be a retry due to
// the previous commit timing out. When a commit times out, the underlying replication
// can still complete and commit afterwards. The RolloutManager is responsible for
// blocking duplicate contexts from processing simultaneously.
//
class ClusterManagerReplica::FinishAcceptRequestAsyncOperation : public AsyncOperation
{
public:
    FinishAcceptRequestAsyncOperation(
        __in ClusterManagerReplica &,
        ErrorCode &&,
        AsyncCallback const &,
        AsyncOperationSPtr const &);

    FinishAcceptRequestAsyncOperation(
        __in ClusterManagerReplica &,
        StoreTransaction &&,
        shared_ptr<RolloutContext> &&,
        ErrorCode &&,
        bool shouldCommit,
        bool shouldCompleteClient,
        TimeSpan const,
        AsyncCallback const &,
        AsyncOperationSPtr const &);

    static ErrorCode End(AsyncOperationSPtr const &);

    void OnStart(AsyncOperationSPtr const &);

private:
    void OnCommitComplete(AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
    void FinishAcceptRequest(AsyncOperationSPtr const &);

    bool ShouldCommit();
    bool ShouldEnqueue();

    ClusterManagerReplica & owner_;
    StoreTransaction storeTx_;
    shared_ptr<RolloutContext> context_;
    ErrorCode error_;

    // Don't commit - just enqueue
    bool shouldCommit_;

    // Complete the client request once commit succeeds
    bool shouldCompleteClient_;

    TimeoutHelper timeoutHelper_;
    AsyncOperationSPtr innerAsyncOperationSPtr_;
};

// *********************
// ClusterManagerReplica
// *********************

ClusterManagerReplica::ClusterManagerReplica(
    Guid const & partitionId,
    FABRIC_REPLICA_ID replicaId,
    __in FederationWrapper & federation,
    __in ServiceResolver & resolver,
    __in IImageBuilder & imageBuilder,
    Api::IClientFactoryPtr const & clientFactory,
    wstring const & workingDir,
    std::wstring const & nodeName,
    ComponentRoot const & fabricRoot)
    : KeyValueStoreReplica(
        partitionId,
        replicaId)
    , RootedObject(fabricRoot)
    , workingDir_(workingDir)
    , OnChangeRoleCallback(nullptr)
    , OnCloseReplicaCallback(nullptr)
    , federation_(federation)
    , resolver_(resolver)
    , imageBuilder_(imageBuilder)
    , clientProxy_(clientFactory, Store::PartitionedReplicaId(Guid(partitionId), replicaId))
    , namingJobQueue_()
    , rolloutManagerUPtr_()
    , nodeName_(nodeName)
    , stringRequestTrackerUPtr_()
    , nameRequestTrackerUPtr_()
    , applicationTypeTrackerUPtr_()
    , isGettingApplicationTypeInfo_(false)
    , isFabricImageBuilderOperationActive_(false)
    , fabricRoot_(fabricRoot.CreateComponentRoot())
    , serviceLocation_()
    , messageFilterSPtr_()
    , contextProcessingDelay_(TimeSpan::Zero)
    , queryMessageHandler_(nullptr)
    , healthManagerReplica_()
    , currentClusterManifest_(nullptr)
    , cleanupAppTypejobQueue_()
    , cleanupApplicationTypeTimer_()
    , cleanupApplicationTypeTimerLock_()
    , callbackLock_()
{
    WriteInfo(
        TraceComponent,
        "{0} ctor: node = {1} this = {2}",
        this->TraceId,
        federation_.Instance,
        static_cast<void*>(this));
    volumeManagerUPtr_ = make_unique<VolumeManager>(*this);
    this->Initialize();
}

ClusterManagerReplica::~ClusterManagerReplica()
{
    WriteInfo(
        TraceComponent,
        "{0} ~dtor: node = {1} this = {2}",
        this->TraceId,
        federation_.Instance,
        static_cast<void*>(this));
}

// Private method to expose the health manager replica
HealthManagerReplicaSPtr ClusterManagerReplica::GetHealthManagerReplica() const
{
    AcquireReadLock lock(lock_);
    return healthManagerReplica_;
}

// class HealthManagerReplica is forward declared in headers
Common::ErrorCode ClusterManagerReplica::GetHealthAggregator(__inout HealthManager::IHealthAggregatorSPtr & healthAggregator) const
{
    HealthManagerReplicaSPtr hm = GetHealthManagerReplica();
    if (!hm)
    {
        return ErrorCodeValue::NotPrimary;
    }

    healthAggregator = static_pointer_cast<IHealthAggregator>(hm);
    return ErrorCode::Success();
}

// *********************
// Management operations
// *********************

bool ClusterManagerReplica::TryAcceptRequestByString(
    std::wstring const & key,
    __in ClientRequestSPtr & clientRequest)
{
    if (clientRequest->IsLocalRetry()) { return true; }

    MessageUPtr failureReply;

    bool accepted = stringRequestTrackerUPtr_->TryAcceptRequest(
        clientRequest->ActivityId,
        key,
        clientRequest->RequestInstance,
        failureReply);

    if (!accepted)
    {
        clientRequest->SetReply(move(failureReply));

        clientRequest->CompleteRequest(ErrorCodeValue::Success);
    }

    return accepted;
}

bool ClusterManagerReplica::TryAcceptRequestByName(
    Common::NamingUri const & key,
    __in ClientRequestSPtr & clientRequest)
{
    if (clientRequest->IsLocalRetry()) { return true; }

    MessageUPtr failureReply;

    bool accepted = nameRequestTrackerUPtr_->TryAcceptRequest(
        clientRequest->ActivityId,
        key,
        clientRequest->RequestInstance,
        failureReply);

    if (!accepted)
    {
        clientRequest->SetReply(move(failureReply));

        clientRequest->CompleteRequest(ErrorCodeValue::Success);
    }

    return accepted;
}

bool ClusterManagerReplica::TryAcceptClusterOperationRequest(
    __in ClientRequestSPtr & clientRequest)
{
    return this->TryAcceptRequestByName(NamingUri::RootNamingUri, clientRequest);
}

Common::ErrorCode ClusterManagerReplica::CheckApplicationTypeNameAndVersion(
    ServiceModelTypeName const & typeName,
    ServiceModelVersion const & typeVersion) const
{
    if (typeName.size() > static_cast<size_t>(ManagementConfig::GetConfig().MaxApplicationTypeNameLength))
    {
        return this->TraceAndGetErrorDetails(ErrorCodeValue::EntryTooLarge, wformatString("{0} ({1}, {2})", GET_RC(AppTypeName_Too_Long),
            typeName,
            ManagementConfig::GetConfig().MaxApplicationTypeNameLength));
    }
    else if (typeVersion.size() > static_cast<size_t>(ManagementConfig::GetConfig().MaxApplicationTypeVersionLength))
    {
        return this->TraceAndGetErrorDetails(ErrorCodeValue::EntryTooLarge, wformatString("{0} ({1}, {2})", GET_RC(AppTypeVersion_Too_Long),
            typeVersion,
            ManagementConfig::GetConfig().MaxApplicationTypeVersionLength));
    }

    return ErrorCode::Success();
}


Common::ErrorCode Management::ClusterManager::ClusterManagerReplica::CloseAutomaticCleanupApplicationType()
{
    Common::TimerSPtr timer;
    {
        AcquireWriteLock lock(cleanupApplicationTypeTimerLock_);
        if (cleanupApplicationTypeTimer_)
        {
            timer = cleanupApplicationTypeTimer_;
        }
    }

    if (timer)
    {
        timer->Cancel();
    }

    if (cleanupAppTypejobQueue_)
    {
        cleanupAppTypejobQueue_->Close();
    }

    return ErrorCodeValue::Success;
}

bool Management::ClusterManager::ClusterManagerReplica::QueueAutomaticCleanupApplicationType(std::wstring const & appTypeName, Common::ActivityId const & activityId) const
{
    if (!ManagementConfig::GetConfig().CleanupUnusedApplicationTypes)
    {
        return true;
    }

    auto jobItem = DefaultJobItem<ClusterManagerReplica>(
        [this, appTypeName, activityId](ClusterManagerReplica &) mutable
    {
        auto replica = const_cast<ClusterManagerReplica *>(this);
        auto error = replica->CheckAndDeleteUnusedApplicationTypes(appTypeName, activityId);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                TraceId,
                "Failed to cleanup apptype for {0} resulted in error:{1}.",
                appTypeName,
                error);
        }
    });

    bool ret = cleanupAppTypejobQueue_->Enqueue(move(jobItem));
    if (!ret)
    {
        WriteWarning(
            TraceComponent,
            TraceId,
            "{0}: Failed to enqueue the operation to cleanupAppTypejobQueue for activityId:{1}.",
            appTypeName,
            activityId);
    }

    return ret;
}

void ClusterManagerReplica::MarkUsedAppTypeVersions(
    vector<ApplicationTypeContext> const & tempAppTypeContexts,
    std::wstring const &targetVersion, 
    __out vector<bool> &usedApplicationVersions)
{
    auto it = std::find_if(tempAppTypeContexts.begin(), tempAppTypeContexts.end(),
        [&targetVersion](const ApplicationTypeContext & v) { return v.TypeVersion.Value == targetVersion; });

    if (it != tempAppTypeContexts.end())
    {
        auto idx = std::distance(tempAppTypeContexts.begin(), it);
        usedApplicationVersions[idx] = true;
    }
}

Common::ErrorCode ClusterManagerReplica::FindUsedAppTypeVersionVersion(
    vector<ApplicationTypeContext> const & tempAppTypeContexts,
    vector<ApplicationContext> const & appContexts,
    wstring const &appTypeName,
    __out vector<bool> &usedApplicationVersions)
{
    ErrorCode error(ErrorCodeValue::Success);
    for (auto const & appContext : appContexts)
    {
        // filter out application contexts that do not match the type being unprovisioned
        if (appContext.TypeName.Value != appTypeName) { continue; }

        // check for active application upgrades against this target version
        if (appContext.IsUpgrading)
        {
            auto upgradeContext = make_unique<ApplicationUpgradeContext>(appContext.ApplicationName);
            error = ReadExact<ApplicationUpgradeContext>(*upgradeContext);
            if (error.IsSuccess() && !upgradeContext->IsComplete)
            {
                MarkUsedAppTypeVersions(tempAppTypeContexts, upgradeContext->UpgradeDescription.TargetApplicationTypeVersion, usedApplicationVersions);
            }
        }

        // Current target version
        MarkUsedAppTypeVersions(tempAppTypeContexts, appContext.TypeVersion.Value, usedApplicationVersions);
    }

    return error;
}

Common::ErrorCode Management::ClusterManager::ClusterManagerReplica::CheckAndDeleteUnusedApplicationTypes()
{
    ActivityId activityId;

    WriteInfo(
        TraceComponent,
        "{0} Periodic cleanup for all app types triggered with activityId:{1}",
        this->TraceId,
        activityId);

    vector<ApplicationTypeContext> appTypeContexts;
    auto error = ReadPrefix<ApplicationTypeContext>(Constants::StoreType_ApplicationTypeContext, appTypeContexts);
    if (!error.IsSuccess())
    {
        return error;
    }

    set<wstring> uniqueAppTypeNames;
    for (auto const &appTypeContext : appTypeContexts)
    {
        uniqueAppTypeNames.insert(appTypeContext.TypeName.Value);
    }

    bool ret = true;
    // Trigger automatic cleanup for each app type
    for (auto const &appTypeName : uniqueAppTypeNames)
    {
        if (!QueueAutomaticCleanupApplicationType(appTypeName, activityId))
        {
            ret = false;
            break;
        }

        activityId.IncrementIndex();
    }

    if (!ret)
    {
        WriteInfo(
            TraceComponent,
            "{0} Triggering periodic cleanup again since previous enqueue unsuccessful for activityId:{1}",
            this->TraceId,
            activityId);

        AcquireWriteLock lock(this->cleanupApplicationTypeTimerLock_);
        this->cleanupApplicationTypeTimer_->Change(ManagementConfig::GetConfig().InitialPeriodicAppTypeCleanupInterval, ManagementConfig::GetConfig().PeriodicAppTypeCleanupInterval);
    }

    return error;
}

Common::ErrorCode Management::ClusterManager::ClusterManagerReplica::CheckAndDeleteUnusedApplicationTypes(wstring const & appTypeName, Common::ActivityId const & activityId)
{
    // Ignore compose deployment
    if (StringUtility::StartsWith(appTypeName, *Constants::ComposeDeploymentTypePrefix))
    {
        return ErrorCodeValue::Success;
    }

    vector<ApplicationTypeContext> appTypeContexts;
    auto error = ReadPrefix<ApplicationTypeContext>(Constants::StoreType_ApplicationTypeContext, appTypeName, appTypeContexts);
    if (!error.IsSuccess())
    {
        return error;
    }

    int numberOfVersionsToSkip = ManagementConfig::GetConfig().MaxUnusedAppTypeVersionsToKeep;
    if (appTypeContexts.size() <= numberOfVersionsToSkip)
    {
        return ErrorCodeValue::Success;
    }

    vector<ApplicationContext> appContexts;
    error = ReadPrefix<ApplicationContext>(Constants::StoreType_ApplicationContext, appContexts);
    if (!error.IsSuccess())
    {
        return error;
    }

    WriteInfo(
        TraceComponent,
        "{0} Current number of applicationTypes/versions {1} appTypeName:{2} applicationContexts:{3} activityId:{4}",
        this->TraceId,
        appTypeContexts.size(),
        appTypeName,
        appContexts.size(),
        activityId);

    // Consider applicationtype that are in Completed state only
    vector<ApplicationTypeContext> tempAppTypeContexts;
    std::copy_if(appTypeContexts.begin(), appTypeContexts.end(), std::back_inserter(tempAppTypeContexts),
        [&appTypeName](const ApplicationTypeContext& item)
    {
        return item.IsComplete;
    });

    // To sort by latest version
    sort(tempAppTypeContexts.begin(), tempAppTypeContexts.end(), [](const ApplicationTypeContext& lhs, const ApplicationTypeContext& rhs)
    {
        return lhs.SequenceNumber > rhs.SequenceNumber;
    });

    vector<bool> usedApplicationVersion(tempAppTypeContexts.size(), false);
    error = FindUsedAppTypeVersionVersion(tempAppTypeContexts, appContexts, appTypeName, usedApplicationVersion);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0} Unable to get app type in use for appTypeName:{1} activityId:{2}",
            this->TraceId,
            appTypeName,
            activityId);

        return error;
    }

    vector<ApplicationTypeContext> versionsToRemove;
    int numVersionsSkipped = 0;
    for (int i = 0; i < usedApplicationVersion.size(); ++i)
    {
        if (!usedApplicationVersion[i])
        {
            if (numVersionsSkipped >= numberOfVersionsToSkip)
            {
                versionsToRemove.emplace_back(tempAppTypeContexts[i]);
            }
            else
            {
                ++numVersionsSkipped;
            }
        }
    }

    for (auto &v : versionsToRemove)
    {
        auto descriptionSPtr = make_shared<UnprovisionApplicationTypeDescription>(
            v.TypeName.Value,
            v.TypeVersion.Value,
            true);

        WriteNoise(
            TraceComponent,
            "{0} Trying to Unprovision application type for typename:{1} typeversion:{2} during automatic application type cleanup. ActivityId:{3}",
            this->TraceId,
            descriptionSPtr->ApplicationTypeName,
            descriptionSPtr->ApplicationTypeVersion,
            activityId);

        auto unprovOperation = Client.BeginUnprovisionApplicationType(
            *descriptionSPtr,
            ManagementConfig::GetConfig().AutomaticUnprovisionInterval,
            [this, descriptionSPtr, activityId](AsyncOperationSPtr const& operation)
        {
            this->OnUnprovisionAcceptComplete(descriptionSPtr, activityId, operation, false);
        },
            this->CreateAsyncOperationRoot());

        OnUnprovisionAcceptComplete(descriptionSPtr, activityId, unprovOperation, true);
    }

    return error;
}

void ClusterManagerReplica::OnUnprovisionAcceptComplete(
    std::shared_ptr<UnprovisionApplicationTypeDescription> const & descriptionSPtr,
    ActivityId const & activityId,
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = Client.EndUnprovisionApplicationType(operation);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0} Unprovision application type failed to be accepted during automatic cleanup with error {1} for typename:{2} typeversion:{3} activityId:{4}",
            this->TraceId,
            error,
            descriptionSPtr->ApplicationTypeName,
            descriptionSPtr->ApplicationTypeVersion,
            activityId);
    }
    else
    {
        WriteInfo(
            TraceComponent,
            "{0} Unprovision application type accepted during automatic cleanup for typename:{1} typeversion:{2} activityId:{3}",
            this->TraceId,
            descriptionSPtr->ApplicationTypeName,
            descriptionSPtr->ApplicationTypeVersion,
            activityId);
    }
}

AsyncOperationSPtr ClusterManagerReplica::BeginAcceptProvisionApplicationType(
    ProvisionApplicationTypeDescription const & body,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    auto error = body.CheckIsValid();
    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    wstring trimmedBuildPath;

    ServiceModelTypeName typeName;
    ServiceModelVersion typeVersion;
    if (body.NeedsDownload())
    {
        // The application type info must be specified when downloading from external store
        typeName = ServiceModelTypeName(body.ApplicationTypeName);
        typeVersion = ServiceModelVersion(body.ApplicationTypeVersion);
    }
    else
    {
        trimmedBuildPath = body.ApplicationTypeBuildPath;
        StringUtility::TrimTrailing<wstring>(trimmedBuildPath, L"\\");

        // Get the application type info
        if (isGettingApplicationTypeInfo_.exchange(true))
        {
            return RejectRequest(move(clientRequest), ErrorCodeValue::CMRequestAlreadyProcessing, callback, root);
        }

        error = imageBuilder_.GetApplicationTypeInfo(
            trimmedBuildPath,
            clientRequest->Timeout,
            typeName,
            typeVersion);

        isGettingApplicationTypeInfo_.store(false);

        if (!error.IsSuccess())
        {
            return RejectRequest(move(clientRequest), move(error), callback, root);
        }
    }

    error = CheckApplicationTypeNameAndVersion(typeName, typeVersion);
    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    auto context = make_shared<ApplicationTypeContext>(
        *this,
        clientRequest,
        trimmedBuildPath,
        typeName,
        typeVersion,
        body.IsAsync,
        body.ApplicationPackageDownloadUri,
        body.ApplicationPackageCleanupPolicy);

    if (!this->TryAcceptRequestByString(context->Key, clientRequest))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::Success, callback, root);
    }

    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, context->ActivityId);

    auto existingContext = make_unique<ApplicationTypeContext>(typeName, typeVersion);

    // Case-insensitive checks are performed inside ProcessApplicationTypeContextAsyncOperation
    //
    error = this->GetApplicationTypeContext(storeTx, *existingContext);

    bool shouldCommit(true);
    if (error.IsError(ErrorCodeValue::ApplicationTypeNotFound))
    {
        context->QueryStatusDetails = GET_RC(ImageBuilder_Running);

        error = storeTx.Insert(*context);
    }
    else if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }
    else if (existingContext->IsFailed)
    {
        // Resume previously failed async context
        context->SetSequenceNumber(existingContext->SequenceNumber);
        context->QueryStatusDetails = GET_RC(ImageBuilder_Running);

        error = storeTx.Update(*context);
    }
    else if (existingContext->IsPending)
    {
        error = this->UpdateOperationTimeout(storeTx, *existingContext, clientRequest->Timeout, shouldCommit);
    }
    else if (!existingContext->IsComplete)
    {
        // Could have read a previous deleting context
        return RejectRequest(move(clientRequest), ErrorCodeValue::CMRequestAlreadyProcessing, callback, root);
    }
    else
    {
        WriteInfo(
            TraceComponent,
            "{0} application type ({1}, {2}) already exists",
            this->TraceId,
            typeName,
            typeVersion);

        return RejectRequest(move(clientRequest), ErrorCodeValue::ApplicationTypeAlreadyExists, callback, root);
    }

    return FinishAcceptRequest(
        move(storeTx),
        move(context),
        move(error),
        shouldCommit,
        timeout,
        callback,
        root);
}

AsyncOperationSPtr ClusterManagerReplica::BeginAcceptCreateComposeDeployment(
    wstring const & deploymentNameArg,
    wstring const & appNameString,
    vector<ByteBuffer> && composeFiles,
    vector<ByteBuffer> && sfSettingsFiles,
    wstring const & registryUserName,
    wstring const & registryPassword,
    bool isPasswordEncrypted,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    unique_ptr<NamingUri> applicationNamePtr;
    ErrorCode error = ErrorCode::Success();

    wstring deploymentName = deploymentNameArg;
    if (deploymentName.empty())
    {
        NamingUri applicationName;
        error = ValidateAndGetComposeDeploymentName(appNameString, applicationName);
        applicationNamePtr = make_unique<NamingUri>(applicationName);
        if (!error.IsSuccess())
        {
            return RejectRequest(move(clientRequest), move(error), callback, root);
        }
        deploymentName = ClusterManagerReplica::GetDeploymentNameFromAppName(appNameString);
    }
    else
    {
        applicationNamePtr = make_unique<NamingUri>(deploymentName);
    }

    if (!this->TryAcceptRequestByString(deploymentName, clientRequest))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::Success, callback, root);
    }

    // Generate type name and version. These will be needed for ImageBuilder
    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, clientRequest->ActivityId);
    ServiceModelTypeName applicationTypeName;
    ServiceModelVersion applicationTypeVersion;
    error = this->dockerComposeAppTypeVersionGenerator_->GetTypeNameAndVersion(
        storeTx,
        *applicationNamePtr,
        applicationTypeName,
        applicationTypeVersion);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0} {1} ({2})",
            this->TraceId,
            error.Message,
            error);

        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    WriteInfo(
        TraceComponent,
        "{0}:{1} BeginAcceptCreateComposeDeployment {2}. Generated type name {3} type version {4} Repo UserName {5}",
        this->TraceId,
        clientRequest->ActivityId,
        deploymentName,
        applicationTypeName,
        applicationTypeVersion,
        registryUserName);

    if (isGettingApplicationTypeInfo_.exchange(true))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::CMRequestAlreadyProcessing, callback, root);
    }

    // ImageBuilder.ValidateYml
    error = imageBuilder_.ValidateComposeFile(
        composeFiles[0],
        *applicationNamePtr,
        applicationTypeName,
        applicationTypeVersion,
        timeout);

    isGettingApplicationTypeInfo_.store(false);

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    // Check ApplicationContext exists, if yes return error

    ServiceModelApplicationId applicationId;
    StoreDataGlobalInstanceCounter instanceCounter;
    error = instanceCounter.GetNextApplicationId(storeTx, applicationTypeName, applicationTypeVersion, applicationId);
    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    // Check if application already exists
    auto applicationContext = make_shared<ApplicationContext>(*applicationNamePtr);

    error = this->GetApplicationContext(storeTx, *applicationContext);
    if (error.IsError(ErrorCodeValue::ApplicationNotFound))
    {
        error = ErrorCodeValue::Success;
    }
    // Application already exists
    else if (error.IsSuccess())
    {
        if (applicationContext->ApplicationDefinitionKind == ApplicationDefinitionKind::Enum::Compose)
        {
            error = ErrorCodeValue::ComposeDeploymentAlreadyExists;
        }
        else
        {
            error = ErrorCodeValue::ApplicationAlreadyExists;
        }
    }

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    // Check and persist ComposeDeploymentContext
    auto composeDeploymentContext = make_unique<ComposeDeploymentContext>(
        *this,
        clientRequest,
        deploymentName,
        *applicationNamePtr,
        applicationId,
        instanceCounter.Value,
        applicationTypeName,
        applicationTypeVersion);

    bool readExisting = false;
    error = storeTx.TryReadOrInsertIfNotFound(*composeDeploymentContext, readExisting);

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    //
    // TODO:
    // If the context has been marked as failed, this path allow restarting the context. It would mean that when a context is marked failed,
    // we should rollback the changes made and leave the context in failed state, so that we can update it here and restart. For now,
    // we expect the user to delete and recreate.
    //
    if (readExisting)
    {
        if (composeDeploymentContext->IsComplete)
        {
            WriteInfo(
                TraceComponent,
                "{0} compose deployment {1} already exists",
                this->TraceId,
                deploymentName);
            return RejectRequest(move(clientRequest), ErrorCodeValue::ComposeDeploymentAlreadyExists, callback, root);
        }
        else if (composeDeploymentContext->IsFailed)
        {
            WriteInfo(
                TraceComponent,
                "{0} recreate failed compose deployment {1}",
                this->TraceId,
                deploymentName);
            composeDeploymentContext = make_unique<ComposeDeploymentContext>(
                *this,
                clientRequest,
                deploymentName,
                *applicationNamePtr,
                applicationId,
                instanceCounter.Value,
                applicationTypeName,
                applicationTypeVersion);
            error = storeTx.Update(*composeDeploymentContext);
            if (!error.IsSuccess())
            {
                return RejectRequest(move(clientRequest), move(error), callback, root);
            }
        }
        else
        {
            WriteNoise(
                TraceComponent,
                "{0} compose deployment {1} still being processed ({2})",
                this->TraceId,
                deploymentName,
                composeDeploymentContext->Status);
            return RejectRequest(move(clientRequest), ErrorCodeValue::CMRequestAlreadyProcessing, callback, root);
        }
    }

    //
    // Update the status in the compose application context and the rollout context.
    //
    error = composeDeploymentContext->StartCreating(storeTx);
    if (error.IsSuccess())
    {
        error = instanceCounter.UpdateNextApplicationId(storeTx);
    }

    auto storeDataComposeDeploymentInstance = make_unique<StoreDataComposeDeploymentInstance>(
        deploymentName,
        applicationTypeVersion,
        move(composeFiles),
        move(sfSettingsFiles),
        registryUserName,
        registryPassword,
        isPasswordEncrypted);

    if (error.IsSuccess())
    {
        error = storeTx.UpdateOrInsertIfNotFound(*storeDataComposeDeploymentInstance);
    }

    return FinishAcceptRequest(
        move(storeTx),
        move(composeDeploymentContext),
        move(error),
        true, //shouldCommit
        true, //shouldCompleteClient
        timeout,
        callback,
        root);
}

AsyncOperationSPtr ClusterManagerReplica::BeginAcceptCreateSingleInstanceApplication(
    ModelV2::ApplicationDescription && description,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    wstring deploymentName = description.Name;

    if (!this->TryAcceptRequestByString(deploymentName, clientRequest))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::Success, callback, root);
    }

    // Generate application name, type name ,and version.
    NamingUri const & applicationName = description.ApplicationUri;
    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, clientRequest->ActivityId);
    ServiceModelTypeName applicationTypeName;
    ServiceModelVersion applicationVersion;
    ErrorCode error = this->containerGroupAppTypeVersionGenerator_->GetTypeNameAndVersion(
        storeTx,
        NamingUri(), //deprecated
        applicationTypeName,
        applicationVersion);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0} ({1} {2})",
            this->TraceId,
            error,
            error.Message);
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    ServiceModelApplicationId applicationId;
    StoreDataGlobalInstanceCounter instanceCounter;
    error = instanceCounter.GetNextApplicationId(storeTx, applicationTypeName, applicationVersion, applicationId);
    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    // TODO: Consider remove validation for speed. Commiting all the contexts in one transaction would fail in the end anyway.
    {
        auto composeDeploymentContext = make_shared<ComposeDeploymentContext>(deploymentName);
        error = this->GetComposeDeploymentContext(storeTx, *composeDeploymentContext);

        if (error.IsError(ErrorCodeValue::ComposeDeploymentNotFound))
        {
            error = ErrorCodeValue::Success;
        }
        else if (error.IsSuccess())
        {
            error = ErrorCodeValue::ComposeDeploymentAlreadyExists;
        }

        if (!error.IsSuccess())
        {
            return RejectRequest(move(clientRequest), move(error), callback, root);
        }
    }

    // Insert context
    auto context = make_shared<SingleInstanceDeploymentContext>(
        *this,
        clientRequest,
        deploymentName,
        DeploymentType::Enum::V2Application,
        applicationName,
        SingleInstanceDeploymentStatus::Enum::Creating,
        applicationId,
        instanceCounter.Value,
        applicationTypeName,
        applicationVersion,
        L"");

    bool readExisting = false;
    error = storeTx.TryReadOrInsertIfNotFound(*context, readExisting);

    if (error.IsSuccess())
    {
        error = instanceCounter.UpdateNextApplicationId(storeTx);
    }

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

     // Look up volume refs and return early if not found.
    for (auto & service : description.Services)
    {
        for (auto & codePackage : service.Properties.CodePackages)
        {
            for (auto & containerVolumeRef : codePackage.VolumeRefs)
            {
                wstring const & volumeName = containerVolumeRef.Name;
                StoreDataVolume storeDataVolume(volumeName);
                error = storeTx.ReadExact(storeDataVolume);

                if (error.IsError(ErrorCodeValue::NotFound))
                {
                    WriteInfo(
                        TraceComponent,
                        "{0} Volume {1} does not exist",
                        this->TraceId,
                        volumeName);

                    return this->RejectRequest(move(clientRequest), ErrorCode(ErrorCodeValue::VolumeNotFound, wformatString(GET_RC(Volume_Not_Found), volumeName)), callback, root);
                }
            }
        }
    }

    if (readExisting)
    {
         WriteInfo(
            TraceComponent,
            "{0} single instance deployment {1} already exists",
            this->TraceId,
            deploymentName);

        auto storeDataSingleInstanceApplicationInstance = make_shared<StoreDataSingleInstanceApplicationInstance>(
            deploymentName,
            context->TypeVersion);

        bool storeDataExists = storeTx.ReadExact(*storeDataSingleInstanceApplicationInstance).IsSuccess();
        bool dupRequest = storeDataExists
            && storeDataSingleInstanceApplicationInstance->ApplicationDescription == description;

        if (context->IsComplete || context->IsFailed)
        {
            if (dupRequest)
            {
                return RejectRequest(move(clientRequest), ErrorCodeValue::SingleInstanceApplicationAlreadyExists, callback, root);
            }
            else if (context->IsComplete && storeDataExists &&
                     storeDataSingleInstanceApplicationInstance->ApplicationDescription.CanUpgrade(description))
            {
                SingleInstanceApplicationUpgradeDescription upgradeDescription(
                    move(deploymentName),
                    applicationName.ToString(),
                    UpgradeType::Enum::Rolling,
                    RollingUpgradeMode::Enum::Monitored,
                    false,
                    RollingUpgradeMonitoringPolicy(
                        MonitoredUpgradeFailureAction::Enum::Rollback,
                        TimeSpan::Zero,
                        ManagementConfig::GetConfig().SBZ_HealthCheckStableDuration,
                        ManagementConfig::GetConfig().SBZ_HealthCheckRetryDuration,
                        ManagementConfig::GetConfig().SBZ_UpgradeTimeout,
                        ManagementConfig::GetConfig().SBZ_UpgradeDomainTimeout),
                    true, //isHealthPolicyValid
                    ApplicationHealthPolicy(),
                    ManagementConfig::GetConfig().SBZ_ReplicaSetCheckTimeout,
                    description);

                 return this->BeginProcessUpgradeSingleInstanceApplication(
                     move(context),
                     upgradeDescription,
                     move(storeTx),
                     move(clientRequest),
                     timeout,
                     callback,
                     root);
            }
            else
            {
                return this->BeginReplaceSingleInstanceApplication(
                    move(context),
                    description,
                    move(storeTx),
                    move(clientRequest),
                    timeout,
                    callback,
                    root);
            }
        }
        else
        {
            // Httpgateway return same as new PUT
            if (dupRequest)
            {
                return RejectRequest(move(clientRequest), ErrorCodeValue::ApplicationDeploymentInProgress, callback, root);
            }
            else
            {
                return RejectRequest(move(clientRequest), ErrorCodeValue::CMRequestAlreadyProcessing, callback, root);
            }
        }
    }

    WriteInfo(
        TraceComponent,
        "{0} BeginAcceptCreateSingleInstanceApplication {1}. Generated type name {2} type version {3}",
        this->TraceId,
        deploymentName,
        applicationTypeName,
        applicationVersion);

    // ApplicationContext and ApplicationTypeContext are created before hand to avoid future conflicts with SFApplicationPackage deployment.
    auto applicationContext = make_shared<ApplicationContext>(
        context->ReplicaActivityId,
        context->ApplicationName,
        context->ApplicationId,
        context->GlobalInstanceCount,
        context->TypeName,
        context->TypeVersion,
        L"",
        map<wstring, wstring>(),
        ApplicationDefinitionKind::Enum::MeshApplicationDescription);

    if (error.IsSuccess())
    {
        error = storeTx.TryReadOrInsertIfNotFound(*applicationContext, readExisting);
    }

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    if (readExisting)
    {
        WriteInfo(
            TraceComponent,
            "{0} application context {1} already exists",
            this->TraceId,
            context->ApplicationName);
        return RejectRequest(move(clientRequest), ErrorCodeValue::ApplicationAlreadyExists, callback, root);
    }

    auto applicationTypeContext = make_shared<ApplicationTypeContext>(
        context->ReplicaActivityId,
        context->TypeName,
        context->TypeVersion,
        false,
        ApplicationTypeDefinitionKind::Enum::MeshApplicationDescription,
        L"");

    error = storeTx.TryReadOrInsertIfNotFound(*applicationTypeContext, readExisting);
    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    if (readExisting)
    {
        WriteInfo(
            TraceComponent,
            "{0} application type context {1} {2} already exists",
            this->TraceId,
            context->TypeName,
            context->TypeVersion);
        return RejectRequest(move(clientRequest), ErrorCodeValue::ApplicationTypeAlreadyExists, callback, root);
    }

    auto storeDataSingleInstanceApplicationInstance = make_unique<StoreDataSingleInstanceApplicationInstance>(
        deploymentName,
        context->TypeVersion,
        description);
    error = storeTx.Insert(*storeDataSingleInstanceApplicationInstance);

    // Call IB only once in the context process for sake of performance.

    return FinishAcceptRequest(
        move(storeTx),
        move(context),
        move(error),
        true, //shouldCommit
        true, //shouldCompleteClient
        timeout,
        callback,
        root);
}

AsyncOperationSPtr ClusterManagerReplica::BeginReplaceSingleInstanceApplication(
    shared_ptr<SingleInstanceDeploymentContext> && context,
    ModelV2::ApplicationDescription const & description,
    StoreTransaction && storeTx,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    ServiceModelVersion newVersion;
    ErrorCode error = this->containerGroupAppTypeVersionGenerator_->GetNextVersion(
        storeTx,
        context->ApplicationName.ToString(),
        context->TypeVersion,
        newVersion);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0} Replace single instance application: Failed to generate type version ({1} {2})",
            this->TraceId,
            error,
            error.Message);
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    WriteInfo(
        TraceComponent,
        "{0} BeginReplaceSingleInstanceApplication {1}. type name {2}, current version {3}, target version {4}",
        this->TraceId,
        context->DeploymentName,
        context->TypeName,
        context->TypeVersion.Value,
        newVersion.Value);

    context->ReInitializeContext(*this, clientRequest);

    context->NewTypeVersion = newVersion;
    error = context->StartReplacing(storeTx);
    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    auto applicationTypeContext = make_shared<ApplicationTypeContext>(
        context->ReplicaActivityId,
        context->TypeName,
        context->NewTypeVersion,
        false,
        ApplicationTypeDefinitionKind::Enum::MeshApplicationDescription,
        L"");

    bool readExisting = false;
    error = storeTx.TryReadOrInsertIfNotFound(*applicationTypeContext, readExisting);
    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    if (readExisting)
    {
        WriteInfo(
            TraceComponent,
            "{0} application type context {1} {2} already exists",
            this->TraceId,
            context->TypeName,
            context->NewTypeVersion);
        return RejectRequest(move(clientRequest), ErrorCodeValue::ApplicationTypeAlreadyExists, callback, root);
    }

    auto storeDataSingleInstanceApplicationInstance = make_unique<StoreDataSingleInstanceApplicationInstance>(
        context->DeploymentName,
        newVersion,
        description);
    error = storeTx.Insert(*storeDataSingleInstanceApplicationInstance);

    return FinishAcceptRequest(
        move(storeTx),
        move(context),
        move(error),
        true,
        true,
        timeout,
        callback,
        root);
}

// Assuming context is completed.
AsyncOperationSPtr ClusterManagerReplica::BeginProcessUpgradeSingleInstanceApplication(
    shared_ptr<SingleInstanceDeploymentContext> && context,
    SingleInstanceApplicationUpgradeDescription const & upgradeDescription,
    StoreTransaction && storeTx,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    if (upgradeDescription.UpgradeMode == RollingUpgradeMode::Invalid)
    {
        auto error = this->TraceAndGetErrorDetails(ErrorCodeValue::InvalidArgument, wformatString("{0} {1}",
            GET_RC(Invalid_Upgrade_Mode2),
            upgradeDescription.UpgradeMode));

        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    // 1. Generate new version
    ServiceModelVersion newVersion;
    ErrorCode error = this->containerGroupAppTypeVersionGenerator_->GetNextVersion(
        storeTx,
        context->ApplicationName.ToString(),
        context->TypeVersion,
        newVersion);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0} Upgrade single instance application: Failed to generate type version ({1} {2})",
            this->TraceId,
            error,
            error.Message);

        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    WriteInfo(
        TraceComponent,
        "{0} BeginProcessUpgradeSingleInstanceApplication {1}, TypeName {2}, CurrentVersion {3}, TargetVersion {4}",
        this->TraceId,
        context->DeploymentName,
        context->TypeName,
        context->TypeVersion.Value,
        newVersion.Value);

    // 2. Insert target application type
    auto applicationTypeContext = make_shared<ApplicationTypeContext>(
        context->ReplicaActivityId,
        context->TypeName,
        newVersion,
        false,
        ApplicationTypeDefinitionKind::Enum::MeshApplicationDescription,
        L"");

    bool readExistingTargetApplicationType = false;
    error = storeTx.TryReadOrInsertIfNotFound(*applicationTypeContext, readExistingTargetApplicationType);
    if (!error.IsSuccess())
    {
        return this->RejectRequest(move(clientRequest), move(error), callback, root);
    }
    else if (readExistingTargetApplicationType)
    {
        WriteWarning(
            TraceComponent,
            "{0} application type context {1} {2} already exists",
            this->TraceId,
            applicationTypeContext->TypeName,
            applicationTypeContext->TypeVersion);
        return RejectRequest(move(clientRequest), ErrorCodeValue::ApplicationTypeAlreadyExists, callback, root);
    }

    // 3. Insert single instance deployment upgrade context
    bool readExistingUpgradeContext = false;
    map<wstring, wstring> applicationParameters;
    auto singleInstanceDeploymentUpgradeContext = make_unique<SingleInstanceDeploymentUpgradeContext>(
        *this,
        clientRequest,
        context->DeploymentName,
        context->DeploymentType,
        context->ApplicationName,
        make_shared<ApplicationUpgradeDescription>(
            context->ApplicationName,
            newVersion.Value,
            applicationParameters,
            upgradeDescription.UpgradeType,
            upgradeDescription.UpgradeMode,
            upgradeDescription.ReplicaSetCheckTimeout,
            upgradeDescription.MonitoringPolicy,
            upgradeDescription.HealthPolicy,
            upgradeDescription.HealthPolicyValid),
        context->TypeName,
        context->TypeVersion,
        newVersion);

    error = storeTx.TryReadOrInsertIfNotFound(*singleInstanceDeploymentUpgradeContext, readExistingUpgradeContext);
    if (!error.IsSuccess())
    {
        return this->RejectRequest(move(clientRequest), move(error), callback, root);
    }

    if (readExistingUpgradeContext)
    {
        // We don't support interruption with another upgrade now.
        switch (singleInstanceDeploymentUpgradeContext->CurrentStatus)
        {
            case SingleInstanceDeploymentUpgradeState::Enum::CompletedRollforward:
            case SingleInstanceDeploymentUpgradeState::Enum::CompletedRollback:
            case SingleInstanceDeploymentUpgradeState::Enum::Failed:
			case SingleInstanceDeploymentUpgradeState::Enum::Invalid:
				break;

            default:
                return this->RejectRequest(move(clientRequest), ErrorCodeValue::CMRequestAlreadyProcessing, callback, root);
        }

        auto seqNumber = singleInstanceDeploymentUpgradeContext->SequenceNumber;

        singleInstanceDeploymentUpgradeContext = make_unique<SingleInstanceDeploymentUpgradeContext>(
            *this,
            clientRequest,
            context->DeploymentName,
            context->DeploymentType,
            context->ApplicationName,
            make_shared<ApplicationUpgradeDescription>(
                context->ApplicationName,
                newVersion.Value,
                applicationParameters,
                upgradeDescription.UpgradeType,
                upgradeDescription.UpgradeMode,
                upgradeDescription.ReplicaSetCheckTimeout,
                upgradeDescription.MonitoringPolicy,
                upgradeDescription.HealthPolicy,
                upgradeDescription.HealthPolicyValid),
            context->TypeName,
            context->TypeVersion,
            newVersion);

        singleInstanceDeploymentUpgradeContext->SetSequenceNumber(seqNumber);
        error = storeTx.Update(*singleInstanceDeploymentUpgradeContext);
        if (!error.IsSuccess())
        {
            return RejectRequest(move(clientRequest), move(error), callback, root);
        }
    }

    // 4. Insert application upgrade context
    auto applicationContext = make_unique<ApplicationContext>(context->ApplicationName);
    error = this->GetCompletedOrUpgradingApplicationContext(storeTx, *applicationContext);

    if (!error.IsSuccess())
    {
        return this->RejectRequest(move(clientRequest), move(error), callback, root);
    }

    if (!applicationContext->IsUpgrading)
    {
        error = applicationContext->SetUpgradePending(storeTx);
        if (!error.IsSuccess())
        {
            return this->RejectRequest(move(clientRequest), move(error), callback, root);
        }
    }

    uint64 upgradeInstance = ApplicationUpgradeContext::GetNextUpgradeInstance(applicationContext->PackageInstance);

    bool readExistingApplicationUpgradeContext(false);
    auto applicationUpgradeContext = make_unique<ApplicationUpgradeContext>(
        singleInstanceDeploymentUpgradeContext->ReplicaActivityId,
        singleInstanceDeploymentUpgradeContext->UpgradeDescription,
        singleInstanceDeploymentUpgradeContext->CurrentTypeVersion,
        L"", //targetApplicationManifestId
        upgradeInstance,
        ApplicationDefinitionKind::Enum::MeshApplicationDescription);

    error = storeTx.TryReadOrInsertIfNotFound(*applicationUpgradeContext, readExistingApplicationUpgradeContext);
    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    if (readExistingApplicationUpgradeContext)
    {
        if (!applicationUpgradeContext->IsComplete && !applicationUpgradeContext->IsFailed)
        {
            Assert::TestAssert(
                TraceComponent,
                "{0} application upgrade context is not finished during single instance application upgrade. Status: {1}",
                this->TraceId,
                applicationUpgradeContext->Status);
        }

        auto seqNumber = applicationUpgradeContext->SequenceNumber;
        applicationUpgradeContext = make_unique<ApplicationUpgradeContext>(
            singleInstanceDeploymentUpgradeContext->ReplicaActivityId,
            singleInstanceDeploymentUpgradeContext->UpgradeDescription,
            singleInstanceDeploymentUpgradeContext->CurrentTypeVersion,
            L"", //targetApplicationManifestId
            upgradeInstance,
            ApplicationDefinitionKind::Enum::MeshApplicationDescription);
        applicationUpgradeContext->SetSequenceNumber(seqNumber);

        error = storeTx.Update(*applicationUpgradeContext);
        if (!error.IsSuccess())
        {
            return RejectRequest(move(clientRequest), move(error), callback, root);
        }
    }

    // 5. Insert store data
    auto storeDataSingleInstanceApplicationInstance = make_unique<StoreDataSingleInstanceApplicationInstance>(
        context->DeploymentName,
        newVersion,
        upgradeDescription.TargetApplicationDescription);
    error = storeTx.Insert(*storeDataSingleInstanceApplicationInstance);

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    error = context->StartUpgrading(storeTx);

    return FinishAcceptRequest(
        move(storeTx),
        move(singleInstanceDeploymentUpgradeContext),
        move(error),
        true, //shouldCommit
        true, //shouldCompleteClient
        timeout,
        callback,
        root);
}

AsyncOperationSPtr ClusterManagerReplica::BeginAcceptCreateApplication(
    NamingUri const & appName,
    ServiceModelTypeName const & typeName,
    ServiceModelVersion const & typeVersion,
    std::map<wstring, wstring> const & parameters,
    Reliability::ApplicationCapacityDescription const & applicationCapacity,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    if (!this->TryAcceptRequestByName(appName, clientRequest))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::Success, callback, root);
    }

    // Validate the name before processing the request.
    auto error = NamingUri::ValidateName(appName, appName.ToString(), true /*allowFragment*/);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0} can't create application: {1} {2}",
            this->TraceId,
            error,
            error.Message);
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, clientRequest->ActivityId);

    auto appTypeContext = make_unique<ApplicationTypeContext>(typeName, typeVersion);
    error = this->GetCompletedApplicationTypeContext(storeTx, *appTypeContext);

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    if (appTypeContext->ApplicationTypeDefinitionKind != ApplicationTypeDefinitionKind::Enum::ServiceFabricApplicationPackage)
    {
        return RejectRequest(move(clientRequest), ErrorCode(ErrorCodeValue::ApplicationTypeInUse, GET_RC( Not_SF_Application_Type)), callback, root);
    }

    // Get application ID
    //
    ServiceModelApplicationId appId;
    StoreDataGlobalInstanceCounter instanceCounter;
    error = instanceCounter.GetNextApplicationId(storeTx, appTypeContext->TypeName, appTypeContext->TypeVersion, appId);

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    {
        auto composeDeploymentContext = make_unique<ComposeDeploymentContext>(appName);
        error = this->GetComposeDeploymentContext(storeTx, *composeDeploymentContext);

        if (error.IsError(ErrorCodeValue::ComposeDeploymentNotFound))
        {
            error = ErrorCodeValue::Success;
        }
        else if (error.IsSuccess())
        {
            error = ErrorCodeValue::ComposeDeploymentAlreadyExists;
        }

        if (!error.IsSuccess())
        {
            return RejectRequest(move(clientRequest), move(error), callback, root);
        }
    }

    // Check for pre-existing application instance
    //
    auto appContext = make_shared<ApplicationContext>(
        *this,
        clientRequest,
        appName,
        appId,
        instanceCounter.Value,
        appTypeContext->TypeName,
        appTypeContext->TypeVersion,
        appTypeContext->ManifestId,
        parameters);
    error = appContext->SetApplicationCapacity(applicationCapacity);

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    bool readExisting;
    error = storeTx.TryReadOrInsertIfNotFound(*appContext, readExisting);

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    bool shouldCommit(true);
    if (readExisting)
    {
        if (appContext->IsComplete)
        {
            WriteInfo(
                TraceComponent,
                "{0} application {1} already exists",
                this->TraceId,
                appName);

            error = ErrorCodeValue::ApplicationAlreadyExists;
        }
        else
        {
            WriteNoise(
                TraceComponent,
                "{0} application {1} still being processed ({2})",
                this->TraceId,
                appName,
                appContext->Status);

            if (appContext->IsPending)
            {
                error = this->UpdateOperationTimeout(storeTx, *appContext, clientRequest->Timeout, shouldCommit);
            }
            else if (appContext->IsFailed)
            {
                appContext->ReInitializeContext(*this, clientRequest);
                error = appContext->UpdateStatus(storeTx, RolloutStatus::Pending);
            }
            else
            {
                return RejectRequest(move(clientRequest), ErrorCodeValue::CMRequestAlreadyProcessing, callback, root);
            }
        }
    }
    else
    {
        if (error.IsSuccess())
        {
            error = instanceCounter.UpdateNextApplicationId(storeTx);
        }

        // Persist an entry to enable looking up application contexts by application Id
        if (error.IsSuccess())
        {
            StoreDataApplicationIdMap appIdMap(appId, appName);
            error = storeTx.Insert(appIdMap);
        }
    }

    return FinishAcceptRequest(
        move(storeTx),
        move(appContext),
        move(error),
        shouldCommit,
        timeout,
        callback,
        root);
}

AsyncOperationSPtr ClusterManagerReplica::BeginAcceptUpdateApplication(
    ServiceModel::ApplicationUpdateDescriptionWrapper const & updateDescription,
    ClientRequestSPtr && clientRequest,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & root)
{
    Common::NamingUri appName;
    if (!NamingUri::TryParse(updateDescription.ApplicationName, appName))
    {
        WriteWarning(
            TraceComponent,
            "{0} could not parse '{1}' as an application name ",
            this->TraceId,
            updateDescription.ApplicationName);
        return RejectRequest(move(clientRequest), ErrorCodeValue::InvalidNameUri, callback, root);
    }

    if (!this->TryAcceptRequestByName(appName, clientRequest))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::Success, callback, root);
    }

    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, clientRequest->ActivityId);

    ErrorCode error;

    wstring errorDetails;
    if (!updateDescription.TryValidate(errorDetails))
    {
        error = this->TraceAndGetErrorDetails(ErrorCodeValue::InvalidArgument, move(errorDetails));

        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    // Try to get context and fail if it does not exist
    auto appContext = make_shared<ApplicationContext>(appName);

    error = this->GetCompletedOrUpgradingApplicationContext(storeTx, *appContext);

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }
    else if (appContext->ApplicationDefinitionKind != ApplicationDefinitionKind::Enum::ServiceFabricApplicationDescription)
    {
        return RejectRequest(move(clientRequest),
            ErrorCode(
                ErrorCodeValue::OperationFailed,
                wformatString(GET_RC(Invalid_Application_Definition_Kind_Operation), appContext->ApplicationDefinitionKind)),
            callback, root);
    }

    Reliability::ApplicationCapacityDescription newCapacityDescription;
    // Set the new values for application context
    if (updateDescription.RemoveApplicationCapacity)
    {
        newCapacityDescription = Reliability::ApplicationCapacityDescription();
    }
    else
    {
        newCapacityDescription = appContext->ApplicationCapacity;
        if (updateDescription.UpdateMinimumNodes)
        {
            newCapacityDescription.MinimumNodes = updateDescription.MinimumNodes;
        }
        if (updateDescription.UpdateMaximumNodes)
        {
            newCapacityDescription.MaximumNodes = updateDescription.MaximumNodes;
        }
        if (updateDescription.UpdateMetrics)
        {
            newCapacityDescription.Metrics = updateDescription.Metrics;
        }

        if (!newCapacityDescription.TryValidate(errorDetails))
        {
            error = this->TraceAndGetErrorDetails(ErrorCodeValue::InvalidArgument, move(errorDetails));

            return RejectRequest(move(clientRequest), move(error), callback, root);
        }
    }

    auto applicationUpdateContext = make_shared<ApplicationUpdateContext>(
        *this,
        clientRequest,
        appName,
        appContext->ApplicationId,
        appContext->ApplicationCapacity,
        newCapacityDescription);

    bool readExisting;
    error = storeTx.TryReadOrInsertIfNotFound(*applicationUpdateContext, readExisting);

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    if (readExisting)
    {
        // There is another update in progress. Reject this one
        return RejectRequest(move(clientRequest), ErrorCodeValue::ApplicationUpdateInProgress, callback, root);
    }

    return FinishAcceptRequest(
        move(storeTx),
        move(applicationUpdateContext),
        move(error),
        true,
        timeout,
        callback,
        root);
}

AsyncOperationSPtr ClusterManagerReplica::BeginAcceptCreateService(
    __in PartitionedServiceDescriptor & psd,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    // Verify application name format
    //
    NamingUri applicationName;
    if (!NamingUri::TryParse(psd.Service.ApplicationName, applicationName))
    {
        WriteWarning(
            TraceComponent,
            "{0} could not parse '{1}' as an application name ",
            this->TraceId,
            psd.Service.ApplicationName);
        return RejectRequest(move(clientRequest), ErrorCodeValue::InvalidNameUri, callback, root);
    }

    //TODO: ban creating under compose application
    //Lifecycle management of compose application should happen only via the Compose* api's, so creating a regular service under a compose application should not be allowed.

    NamingUri absoluteServiceName;
    if (!NamingUri::TryParse(psd.Service.Name, absoluteServiceName))
    {
        WriteWarning(
            TraceComponent,
            "{0} could not parse '{1}' as a service name",
            this->TraceId,
            psd.Service.Name);
        return RejectRequest(move(clientRequest), ErrorCodeValue::InvalidNameUri, callback, root);
    }

    if (!this->TryAcceptRequestByName(absoluteServiceName, clientRequest))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::Success, callback, root);
    }

    if (!applicationName.IsPrefixOf(absoluteServiceName))
    {
        auto error = this->TraceAndGetErrorDetails( ErrorCodeValue::InvalidNameUri, wformatString("{0} ({1}, {2})", GET_RC( Invalid_Service_Prefix ),
                applicationName,
                absoluteServiceName));

        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    if (absoluteServiceName == applicationName)
    {
        auto error = this->TraceAndGetErrorDetails( ErrorCodeValue::InvalidNameUri, wformatString("{0} {1}", GET_RC( Invalid_Service_Name ),
                absoluteServiceName));

        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    // Get application ID
    //
    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, clientRequest->ActivityId);

    auto appContext = make_unique<ApplicationContext>(applicationName);
    ErrorCode error = this->GetCompletedOrUpgradingApplicationContext(storeTx, *appContext);

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }
    else if (appContext->ApplicationDefinitionKind != ApplicationDefinitionKind::Enum::ServiceFabricApplicationDescription)
    {
        return RejectRequest(move(clientRequest), ErrorCode(ErrorCodeValue::OperationFailed, GET_RC( INVALID_COMPOSE_DEPLOYMENT_OPERATION)), callback, root);
    }

    auto serviceTypeName = ServiceModelTypeName(psd.Service.Type.ServiceTypeName);

    // Get package name
    //
    auto servicePackage = make_unique<StoreDataServicePackage>(appContext->ApplicationId, serviceTypeName);

    error = storeTx.ReadExact(*servicePackage);

    if (!error.IsSuccess())
    {
        if (error.IsError(ErrorCodeValue::NotFound))
        {
            WriteWarning(
                TraceComponent,
                "{0} Service type '{1}' not supported in application {2}. Cannot create service {3}.",
                this->TraceId,
                psd.Service.Type,
                applicationName,
                absoluteServiceName);

            error = ErrorCodeValue::ServiceTypeNotFound;
        }

        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    // Validate against additional restrictions imposed by a pending upgrade
    //
    error = ValidateServiceTypeDuringUpgrade(storeTx, *appContext, serviceTypeName);

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    // Validate client request service type-level parameters against those defined in the manifest
    //
    if (psd.Service.IsStateful != servicePackage->TypeDescription.IsStateful)
    {
        WriteWarning(
            TraceComponent,
            "{0} Service type '{1}' IsStateful mismatch: requested = {2} manifest = {3}",
            this->TraceId,
            psd.Service.Type,
            psd.Service.IsStateful,
            servicePackage->TypeDescription.IsStateful);

        return RejectRequest(move(clientRequest), ErrorCodeValue::ServiceTypeMismatch, callback, root);
    }

    if (psd.Service.HasPersistedState != servicePackage->TypeDescription.HasPersistedState)
    {
        WriteWarning(
            TraceComponent,
            "{0} Service type '{1}' HasPersistedState mismatch: requested = {2} manifest = {3}",
            this->TraceId,
            psd.Service.Type,
            psd.Service.HasPersistedState,
            servicePackage->TypeDescription.HasPersistedState);

        return RejectRequest(move(clientRequest), ErrorCodeValue::ServiceTypeMismatch, callback, root);
    }

    if (!psd.Service.ServiceDnsName.empty())
    {
        if (!IsDnsServiceEnabled())
        {
            WriteWarning(
                TraceComponent,
                "{0} DNS name requested '{1}' but DNS feature is disabled",
                this->TraceId,
                psd.Service.ServiceDnsName);

            return RejectRequest(move(clientRequest), ErrorCodeValue::DnsServiceNotFound, callback, root);
        }

        error = ValidateServiceDnsName(psd.Service.ServiceDnsName);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0} Service DNS name '{1}' is not valid, error {2}",
                this->TraceId,
                psd.Service.ServiceDnsName,
                error);

            return RejectRequest(move(clientRequest), ErrorCodeValue::InvalidDnsName, callback, root);
        }
    }

    if (!psd.IsPlacementConstraintsValid)
    {
        WriteNoise(
            TraceComponent,
            "{0} Service type '{1}' PlacementConstraints undefined (overriding): manifest = {2}",
            this->TraceId,
            psd.Service.Type,
            servicePackage->TypeDescription.PlacementConstraints);

        psd.SetPlacementConstraints(move(servicePackage->MutableTypeDescription.PlacementConstraints));
    }

    if (!psd.IsPlacementPolicyValid)
    {
        WriteNoise(
            TraceComponent,
            "{0} Service type '{1}' ServicePlacementPolicies undefined (overriding): manifest = {2}",
            this->TraceId,
            psd.Service.Type,
            servicePackage->TypeDescription.ServicePlacementPolicies);

        psd.SetPlacementPolicies(move(servicePackage->MutableTypeDescription.ServicePlacementPolicies));
    }

    if (psd.Service.Metrics.empty())
    {
        std::vector<Reliability::ServiceLoadMetricDescription> newLoadMetrics;
        for(auto i=servicePackage->TypeDescription.LoadMetrics.begin();i!=servicePackage->TypeDescription.LoadMetrics.end();++i)
        {
            //The type of ServiceModel::ServiceLoadMetricDescription does not match with Reliability::ServiceLoadMetricDescription, thus manually convert it
            newLoadMetrics.push_back(Reliability::ServiceLoadMetricDescription(i->Name,WeightType::ToPublicApi(i->Weight),i->PrimaryDefaultLoad,i->SecondaryDefaultLoad));
        }

        WriteNoise(
            TraceComponent,
            "{0} Service type '{1}' LoadMetrics undefined (overriding): manifest = {2}",
            this->TraceId,
            psd.Service.Type,
            servicePackage->TypeDescription.LoadMetrics);

        psd.SetLoadMetrics(move(newLoadMetrics));
    }

    auto targetPackageName = servicePackage->PackageName;
    auto targetPackageVersion = servicePackage->PackageVersion;
    auto targetPackageInstance = appContext->PackageInstance;

    if (appContext->IsUpgrading)
    {
        auto upgradeContext = make_unique<ApplicationUpgradeContext>(appContext->ApplicationName);
        error = storeTx.ReadExact(*upgradeContext);

        if (!error.IsSuccess())
        {
            return RejectRequest(move(clientRequest), move(error), callback, root);
        }

        switch (upgradeContext->UpgradeState)
        {
            case ApplicationUpgradeState::RollingForward:
            {
                ServiceModelPackageName packageName;
                ServiceModelVersion packageVersion;

                if (upgradeContext->TryGetTargetServicePackage(serviceTypeName, packageName, packageVersion))
                {
                    targetPackageName = move(packageName);
                    targetPackageVersion = move(packageVersion);
                }
                else
                {
                    // no-op: service type is not affected by upgrade
                }

                targetPackageInstance = upgradeContext->RollforwardInstance;
                break;
            }

            case ApplicationUpgradeState::RollingBack:
            case ApplicationUpgradeState::Interrupted:
                targetPackageInstance = upgradeContext->RollbackInstance;
                break;

            default:
                // no-op: keep instance on application context (last completed upgrade instance)
                break;
        }
    }

    auto serviceContext = make_shared<ServiceContext>(
        *this,
        clientRequest,
        appContext->ApplicationId,
        targetPackageName,
        targetPackageVersion,
        targetPackageInstance,
        psd);

    // Check for pre-existing service
    //
    bool readExisting;
    error = storeTx.TryReadOrInsertIfNotFound(*serviceContext, readExisting);

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    bool shouldCommit(true);
    if (readExisting)
    {
        if (serviceContext->IsComplete)
        {
            WriteInfo(
                TraceComponent,
                "{0} service ({1}) {2} already exists",
                this->TraceId,
                applicationName,
                absoluteServiceName);

            error = ErrorCodeValue::UserServiceAlreadyExists;
        }
        else
        {
            WriteNoise(
                TraceComponent,
                "{0} service ({1}) {2} still being processed ({3})",
                this->TraceId,
                applicationName,
                absoluteServiceName,
                serviceContext->Status);

            if (serviceContext->IsPending)
            {
                error = this->UpdateOperationTimeout(storeTx, *serviceContext, clientRequest->Timeout, shouldCommit);
            }
            else if (serviceContext->IsFailed)
            {
                serviceContext->ReInitializeContext(*this, clientRequest);
                error = serviceContext->UpdateStatus(storeTx, RolloutStatus::Pending);
            }
            else
            {
                return RejectRequest(move(clientRequest), ErrorCodeValue::CMRequestAlreadyProcessing, callback, root);
            }
        }
    }

    return FinishAcceptRequest(
        move(storeTx),
        move(serviceContext),
        move(error),
        shouldCommit,
        timeout,
        callback,
        root);
}

AsyncOperationSPtr ClusterManagerReplica::BeginAcceptCreateServiceFromTemplate(
    NamingUri const & applicationName,
    NamingUri const & absoluteServiceName,
    wstring const & serviceDnsName,
    ServiceModelTypeName const & typeName,
    ServiceModel::ServicePackageActivationMode::Enum servicePackageActivationMode,
    std::vector<byte> && initializationData,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, clientRequest->ActivityId);

    auto appContext = make_unique<ApplicationContext>(applicationName);
    ErrorCode error = this->GetCompletedOrUpgradingApplicationContext(storeTx, *appContext);

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    auto serviceTemplate = make_unique<StoreDataServiceTemplate>(appContext->ApplicationId, typeName);
    error = storeTx.ReadExact(*serviceTemplate);

    if (error.IsError(ErrorCodeValue::NotFound))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::ServiceTypeTemplateNotFound, callback, root);
    }
    else if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    storeTx.CommitReadOnly();

    // ApplicationName, ApplicationId, and TypeName should have already been set by IImageBuilder
    serviceTemplate->MutablePartitionedServiceDescriptor.MutableService.Name = absoluteServiceName.ToString();
    serviceTemplate->MutablePartitionedServiceDescriptor.MutableService.PackageActivationMode = servicePackageActivationMode;
    serviceTemplate->MutablePartitionedServiceDescriptor.MutableService.ServiceDnsName = serviceDnsName;

    // If IsServiceGroup, fill data into each member and some housekeeping
    if (serviceTemplate->MutablePartitionedServiceDescriptor.IsServiceGroup)
    {
        error = ProcessServiceGroupFromTemplate(*serviceTemplate, absoluteServiceName, initializationData);
        if (!error.IsSuccess())
        {
            return RejectRequest(move(clientRequest), move(error), callback, root);
        }
    }
    else
    {
        serviceTemplate->MutablePartitionedServiceDescriptor.MutableService.InitializationData = move(initializationData);
    }

    // Generate new CUID's for each create service call.
    serviceTemplate->MutablePartitionedServiceDescriptor.GenerateRandomCuids();

    return this->BeginAcceptCreateService(
        serviceTemplate->MutablePartitionedServiceDescriptor,
        move(clientRequest),
        timeout,
        callback,
        root);
}

AsyncOperationSPtr ClusterManagerReplica::BeginAcceptUnprovisionApplicationType(
    UnprovisionApplicationTypeDescription const & description,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    if (StringUtility::StartsWith(description.ApplicationTypeName, *Constants::ComposeDeploymentTypePrefix))
    {
        return RejectRequest(move(clientRequest), ErrorCode(ErrorCodeValue::ApplicationTypeInUse, GET_RC( Not_SF_Application_Type)) , callback, root);
    }

    ServiceModelTypeName appTypeName(description.ApplicationTypeName);
    ServiceModelVersion appTypeVersion(description.ApplicationTypeVersion);

    auto appTypeContext = make_unique<ApplicationTypeContext>(
        appTypeName,
        appTypeVersion);

    if (!this->TryAcceptRequestByString(appTypeContext->Key, clientRequest))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::Success, callback, root);
    }

    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, appTypeContext->ActivityId);

    auto error = this->GetApplicationTypeContext(storeTx, *appTypeContext);

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    vector<ApplicationContext> appContexts;
    error = storeTx.ReadPrefix<ApplicationContext>(Constants::StoreType_ApplicationContext, L"", appContexts);

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    for (auto const & appContext : appContexts)
    {
        // filter out application contexts that do not match the type being unprovisioned
        if (appContext.TypeName != appTypeName) { continue; }

        // check for active application upgrades against this target version
        if (appContext.IsUpgrading)
        {
            auto upgradeContext = make_unique<ApplicationUpgradeContext>(appContext.ApplicationName);
            error = storeTx.ReadExact(*upgradeContext);

            if (error.IsSuccess() && !upgradeContext->IsComplete)
            {
                if (ServiceModelVersion(upgradeContext->UpgradeDescription.TargetApplicationTypeVersion) == appTypeVersion)
                {
                    WriteWarning(
                        TraceComponent,
                        "{0} application type:version '{1}:{2}' still in use by upgrade: [{3}] [{4}]",
                        this->TraceId,
                        appTypeName,
                        appTypeVersion,
                        appContext,
                        *upgradeContext);
                    return RejectRequest(move(clientRequest), ErrorCodeValue::ApplicationTypeInUse, callback, root);
                }
            }
            else if (!error.IsError(ErrorCodeValue::NotFound))
            {
                return RejectRequest(move(clientRequest), move(error), callback, root);
            }
        }

        // check for application contexts still using this target version
        if (appContext.TypeVersion == appTypeVersion)
        {
            WriteWarning(
                TraceComponent,
                "{0} application type:version '{1}:{2}' still in use by: [{3}]",
                this->TraceId,
                appTypeName,
                appTypeVersion,
                appContext);
            return RejectRequest(move(clientRequest), ErrorCodeValue::ApplicationTypeInUse, callback, root);
        }
    }

    appTypeContext->UpdateIsAsync(description.IsAsync);

    return FinishAcceptDeleteContext(
        move(storeTx),
        move(clientRequest),
        move(appTypeContext),
        timeout,
        callback,
        root);
}

AsyncOperationSPtr ClusterManagerReplica::BeginAcceptStartInfrastructureTask(
    InfrastructureTaskDescription const & taskDescription,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    auto context = make_unique<InfrastructureTaskContext>(*this, clientRequest, taskDescription);

    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, clientRequest->ActivityId);

    bool readExisting(false);
    ErrorCode error = storeTx.TryReadOrInsertIfNotFound(*context, readExisting);

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    WriteInfo(
        TraceComponent,
        "{0} start {1}",
        this->TraceId,
        taskDescription);

    bool shouldCommit(true); // out
    if (readExisting)
    {
        bool taskComplete(false); // out
        error = context->TryAcceptStartTask(taskDescription, taskComplete, shouldCommit);

        if (error.IsSuccess())
        {
            context->ReInitializeContext(*this, clientRequest);

            clientRequest->SetReply(ClusterManagerMessage::GetClientOperationSuccess(InfrastructureTaskReplyBody(taskComplete)));

            if (shouldCommit)
            {
                error = context->UpdateStatus(storeTx, RolloutStatus::Pending);
            }

            if (error.IsSuccess())
            {
                error = ErrorCodeValue::InfrastructureTaskInProgress;
            }
        }
    }
    else
    {
        clientRequest->SetReply(ClusterManagerMessage::GetClientOperationSuccess(InfrastructureTaskReplyBody(false)));

        error = ErrorCodeValue::InfrastructureTaskInProgress;
    }

    return FinishAcceptRequest(
        move(storeTx),
        move(context),
        move(error),
        shouldCommit,
        timeout,
        callback,
        root);
}

AsyncOperationSPtr ClusterManagerReplica::BeginAcceptFinishInfrastructureTask(
    TaskInstance const & taskInstance,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    auto context = make_unique<InfrastructureTaskContext>(taskInstance);

    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, clientRequest->ActivityId);

    ErrorCode error = storeTx.ReadExact(*context);

    if (error.IsError(ErrorCodeValue::NotFound))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::InfrastructureTaskNotFound, callback, root);
    }
    else if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    WriteInfo(
        TraceComponent,
        "{0} finish {1}",
        this->TraceId,
        taskInstance);

    bool taskComplete(false); // out
    bool shouldCommit(false); // out
    error = context->TryAcceptFinishTask(taskInstance, taskComplete, shouldCommit);

    if (error.IsSuccess())
    {
        context->ReInitializeContext(*this, clientRequest);

        clientRequest->SetReply(ClusterManagerMessage::GetClientOperationSuccess(InfrastructureTaskReplyBody(taskComplete)));

        if (shouldCommit)
        {
            error = context->UpdateStatus(storeTx, RolloutStatus::Pending);
        }

        if (error.IsSuccess())
        {
            error = ErrorCodeValue::InfrastructureTaskInProgress;
        }
    }

    return FinishAcceptRequest(
        move(storeTx),
        move(context),
        move(error),
        shouldCommit,
        timeout,
        callback,
        root);
}

AsyncOperationSPtr ClusterManagerReplica::BeginAcceptDeleteComposeDeployment(
    wstring const &deploymentNameArg,
    NamingUri const & appName,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    wstring deploymentName = deploymentNameArg;
    if (deploymentName.empty())
    {
        deploymentName = ClusterManagerReplica::GetDeploymentNameFromAppName(appName.ToString());
    }

    if (!this->TryAcceptRequestByString(deploymentName, clientRequest))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::Success, callback, root);
    }

    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, clientRequest->ActivityId);

    auto composeDeploymentContext = make_unique<ComposeDeploymentContext>(deploymentName);
    auto error = this->GetDeletableComposeDeploymentContext(storeTx, *composeDeploymentContext);

    if (error.IsError(ErrorCodeValue::SingleInstanceApplicationUpgradeInProgress))
    {
        ComposeDeploymentUpgradeContext upgradeContext(deploymentName);
        error = storeTx.ReadExact(upgradeContext);

        if (error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0} externally failing compose upgrade to allow delete {1}",
                this->TraceId,
                deploymentName);
            rolloutManagerUPtr_->ExternallyFailUpgrade(upgradeContext);

            return RejectRequest(move(clientRequest), ErrorCodeValue::CMRequestAlreadyProcessing, callback, root);
        }
        else if (error.IsError(ErrorCodeValue::NotFound))
        {
            WriteWarning(
                TraceComponent,
                "{0} upgrade context not found for upgrading compose {1}",
                this->TraceId,
                deploymentName);

            Assert::TestAssert();
            error = composeDeploymentContext->Complete(storeTx);

            ApplicationContext appContext(composeDeploymentContext->ApplicationName);
            error = storeTx.ReadExact(appContext);
            if (error.IsSuccess())
            {
                appContext.SetUpgradeComplete(storeTx, appContext.PackageInstance);
            }
        }
        storeTx.CommitReadOnly();
    }
    else if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    //
    // Update the ComposeDeployment state and the rollout context state to deleting.
    //
    error = composeDeploymentContext->StartDeleting(storeTx);
    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    composeDeploymentContext->ReInitializeContext(*this, clientRequest);

    return FinishAcceptRequest(
        move(storeTx),
        move(composeDeploymentContext),
        move(error),
        true, // commit
        true, // Delete is async, so reply to client
        timeout,
        callback,
        root);
}

AsyncOperationSPtr ClusterManagerReplica::BeginAcceptDeleteSingleInstanceDeployment(
    DeleteSingleInstanceDeploymentDescription const & description,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    wstring const & deploymentName = description.DeploymentName;
    if (!this->TryAcceptRequestByString(deploymentName, clientRequest))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::Success, callback, root);
    }

    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, clientRequest->ActivityId);

    auto context = make_unique<SingleInstanceDeploymentContext>(deploymentName);
    ErrorCode error = this->GetDeletableSingleInstanceDeploymentContext(storeTx, *context);
    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    if (context->IsDeleting)
    {
        return RejectRequest(move(clientRequest), move(ErrorCodeValue::CMRequestAlreadyProcessing), callback, root);
    }

    error = context->StartDeleting(storeTx);
    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }
    context->ReInitializeContext(*this, clientRequest);

    return FinishAcceptRequest(
        move(storeTx),
        move(context),
        move(error),
        true,
        true,
        timeout,
        callback,
        root);
}

AsyncOperationSPtr ClusterManagerReplica::BeginAcceptDeleteApplication(
    DeleteApplicationMessageBody const & body,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    NamingUri const & appName = body.Description.ApplicationName;
    if (!this->TryAcceptRequestByName(appName, clientRequest))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::Success, callback, root);
    }
    bool const isForce = body.Description.IsForce;

    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, clientRequest->ActivityId);

    auto appContext = make_shared<ApplicationContext>(appName, isForce);
    ErrorCode error = this->GetDeletableApplicationContext(storeTx, *appContext);
    if (appContext->IsDeleting)
    {
        appContext->IsConvertToForceDelete = !appContext->IsForceDelete && isForce;
    }
    // Override ForceDelete flag
    appContext->IsForceDelete = isForce;

    if (appContext->ApplicationDefinitionKind != ApplicationDefinitionKind::Enum::ServiceFabricApplicationDescription)
    {
        return RejectRequest(move(clientRequest),
            ErrorCode(
                ErrorCodeValue::OperationFailed,
                wformatString(GET_RC(Invalid_Application_Definition_Kind_Operation), appContext->ApplicationDefinitionKind)),
            callback, root);
    }
    else if (error.IsError(ErrorCodeValue::ApplicationUpgradeInProgress))
    {
        auto upgradeContext = make_unique<ApplicationUpgradeContext>(appName);
        error = storeTx.ReadExact(*upgradeContext);

        if (error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0} forcefully failing upgrade to allow delete {1}",
                this->TraceId,
                appName);

            rolloutManagerUPtr_->ExternallyFailUpgrade(*upgradeContext);

            // Retry until failed upgrade shows up in persisted state,
            // at which point, the delete will be allowed to proceed.
            //
            return RejectRequest(move(clientRequest), ErrorCodeValue::CMRequestAlreadyProcessing, callback, root);
        }
        else if (error.IsError(ErrorCodeValue::NotFound))
        {
            WriteWarning(
                TraceComponent,
                "{0} upgrade context not found for upgrading application {1}",
                this->TraceId,
                appName);

            Assert::TestAssert();

            error = appContext->SetUpgradeComplete(storeTx, appContext->PackageInstance);
        }
    }
    else if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    return FinishAcceptDeleteContext(
        move(storeTx),
        move(clientRequest),
        move(appContext),
        timeout,
        callback,
        root);
}

AsyncOperationSPtr ClusterManagerReplica::BeginAcceptDeleteService(
    NamingUri const & appName,
    NamingUri const & absoluteServiceName,
    bool const isForce,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    if (!this->TryAcceptRequestByName(absoluteServiceName, clientRequest))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::Success, callback, root);
    }

    // For compatibility reasons, we must be able to delete a service created before path prefix check was introduced.
    // Only check string prefix and not Uri prefix so we can delete services created
    if (!StringUtility::StartsWith(absoluteServiceName.ToString(), appName.ToString()))
    {
        auto error = this->TraceAndGetErrorDetails(ErrorCodeValue::InvalidNameUri, wformatString("{0} ({1}, {2})", GET_RC(Invalid_Service_Prefix),
            appName,
            absoluteServiceName));
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    // Get application ID
    //
    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, clientRequest->ActivityId);

    auto appContext = make_unique<ApplicationContext>(appName);
    ErrorCode error = this->GetCompletedOrUpgradingApplicationContext(storeTx, *appContext);

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }
    else if (appContext->ApplicationDefinitionKind != ApplicationDefinitionKind::Enum::ServiceFabricApplicationDescription)
    {
        return RejectRequest(move(clientRequest), ErrorCode(ErrorCodeValue::OperationFailed, GET_RC( INVALID_COMPOSE_DEPLOYMENT_OPERATION)), callback, root);
    }

    auto serviceContext = make_shared<ServiceContext>(
        appContext->ApplicationId,
        appName,
        absoluteServiceName,
        isForce);

    error = storeTx.ReadExact(*serviceContext);

    if (error.IsError(ErrorCodeValue::NotFound))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::UserServiceNotFound, callback, root);
    }
    else if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }
    else if (!serviceContext->IsDeleting && !serviceContext->IsComplete)
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::CMRequestAlreadyProcessing, callback, root);
    }

    if (serviceContext->IsDeleting)
    {
        serviceContext->IsConvertToForceDelete = !serviceContext->IsForceDelete && isForce;
    }

    // Override ForceRemove flag
    serviceContext->IsForceDelete = isForce;

    return FinishAcceptDeleteContext(
        move(storeTx),
        move(clientRequest),
        move(serviceContext),
        timeout,
        callback,
        root);
}

AsyncOperationSPtr ClusterManagerReplica::BeginAcceptUpgradeComposeDeployment(
    ComposeDeploymentUpgradeDescription const & composeUpgradeDescription,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    if (!this->TryAcceptRequestByString(composeUpgradeDescription.DeploymentName, clientRequest))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::Success, callback, parent);
    }

    if (composeUpgradeDescription.UpgradeMode == RollingUpgradeMode::Invalid)
    {
        auto error = this->TraceAndGetErrorDetails( ErrorCodeValue::InvalidArgument, wformatString("{0} {1}",
            GET_RC( Invalid_Upgrade_Mode2 ),
            composeUpgradeDescription.UpgradeMode));

        return RejectRequest(move(clientRequest), move(error), callback, parent);
    }

    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, clientRequest->ActivityId);

    //
    // Check the compose deployment db to confirm that this deployment is compose and is in ready state. We dont need to
    // check the application context here since compose deployment being ready is sufficient.
    //
    auto composeDeploymentContext = make_unique<ComposeDeploymentContext>(composeUpgradeDescription.DeploymentName);
    auto error = this->GetCompletedOrUpgradingComposeDeploymentContext(storeTx, *composeDeploymentContext);
    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, parent);
    }

    if (composeDeploymentContext->IsUpgrading)
    {
        //
        // Reject the upgrade since we are already upgrading. Upgrades can be canceled or updated only by using
        // explicit api calls.
        //
        error = this->TraceAndGetErrorDetails( ErrorCodeValue::CMRequestAlreadyProcessing,
            wformatString(GET_RC( Compose_Deployment_Upgrade_In_Progress ), composeUpgradeDescription.DeploymentName));

        return RejectRequest(move(clientRequest), move(error), callback, parent);
    }

    //
    // Generate the new type version. These will be needed for ImageBuilder
    //
    ServiceModelVersion applicationTypeVersion;
    error = this->dockerComposeAppTypeVersionGenerator_->GetNextVersion(
        storeTx,
        composeDeploymentContext->DeploymentName,
        composeDeploymentContext->TypeVersion,
        applicationTypeVersion);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0} Compose deployment upgrade : Failed to generate type version {1} ({2})",
            this->TraceId,
            error,
            error.Message);

        return RejectRequest(move(clientRequest), move(error), callback, parent);
    }

    WriteInfo(
        TraceComponent,
        "{0} BeginAcceptUpgradeComposeDeployment {1}. type name {2}, current version {3}, target version {4}",
        this->TraceId,
        composeUpgradeDescription.DeploymentName,
        composeDeploymentContext->TypeName,
        composeDeploymentContext->TypeVersion.Value,
        applicationTypeVersion.Value);

    if (isGettingApplicationTypeInfo_.exchange(true))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::CMRequestAlreadyProcessing, callback, parent);
    }

    // ImageBuilder.ValidateYml
    error = imageBuilder_.ValidateComposeFile(
        composeUpgradeDescription.ComposeFiles[0],
        composeDeploymentContext->ApplicationName,
        composeDeploymentContext->TypeName,
        applicationTypeVersion,
        timeout);

    isGettingApplicationTypeInfo_.store(false);

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, parent);
    }

    bool readExistingUpgradeContext = false;
    map<wstring, wstring> applicationParameters;
    shared_ptr<ComposeDeploymentUpgradeContext> composeUpgradeContext = make_shared<ComposeDeploymentUpgradeContext>(
        *this,
        clientRequest,
        composeUpgradeDescription.DeploymentName,
        composeDeploymentContext->ApplicationName,
        make_shared<ApplicationUpgradeDescription>(
            composeDeploymentContext->ApplicationName,
            applicationTypeVersion.Value,
            applicationParameters,
            composeUpgradeDescription.UpgradeType,
            composeUpgradeDescription.UpgradeMode,
            composeUpgradeDescription.ReplicaSetCheckTimeout,
            composeUpgradeDescription.MonitoringPolicy,
            composeUpgradeDescription.HealthPolicy,
            composeUpgradeDescription.HealthPolicyValid),
        composeDeploymentContext->TypeName,
        composeDeploymentContext->TypeVersion,
        applicationTypeVersion);

    error = storeTx.TryReadOrInsertIfNotFound(*composeUpgradeContext, readExistingUpgradeContext);

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, parent);
    }

    auto storeDataComposeDeploymentInstance = make_unique<StoreDataComposeDeploymentInstance>(
        composeUpgradeDescription.DeploymentName,
        applicationTypeVersion,
        composeUpgradeDescription.TakeComposeFiles(),
        composeUpgradeDescription.TakeSettingsFiles(),
        composeUpgradeDescription.RegistryUserName,
        composeUpgradeDescription.RegistryPassword,
        composeUpgradeDescription.PasswordEncrypted);

    if (error.IsSuccess())
    {
        error = storeTx.Insert(*storeDataComposeDeploymentInstance);
    }

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, parent);
    }

    if (readExistingUpgradeContext)
    {
        auto seqNumber = composeUpgradeContext->SequenceNumber;
        composeUpgradeContext = make_shared<ComposeDeploymentUpgradeContext>(
            *this,
            clientRequest,
            composeUpgradeDescription.DeploymentName,
            composeDeploymentContext->ApplicationName,
            make_shared<ApplicationUpgradeDescription>(
                composeDeploymentContext->ApplicationName,
                applicationTypeVersion.Value,
                applicationParameters,
                composeUpgradeDescription.UpgradeType,
                composeUpgradeDescription.UpgradeMode,
                composeUpgradeDescription.ReplicaSetCheckTimeout,
                composeUpgradeDescription.MonitoringPolicy,
                composeUpgradeDescription.HealthPolicy,
                composeUpgradeDescription.HealthPolicyValid),
            composeDeploymentContext->TypeName,
            composeDeploymentContext->TypeVersion,
            applicationTypeVersion);

        composeUpgradeContext->SetSequenceNumber(seqNumber);
        error = storeTx.Update(*composeUpgradeContext);
        if (!error.IsSuccess())
        {
            return RejectRequest(move(clientRequest), move(error), callback, parent);
        }
    }

    error = composeDeploymentContext->StartUpgrading(storeTx);

    return FinishAcceptRequest(
        move(storeTx),
        move(composeUpgradeContext),
        move(error),
        true, //shouldCommit
        true, //shouldCompleteClient
        timeout,
        callback,
        parent);
}

AsyncOperationSPtr ClusterManagerReplica::BeginAcceptRollbackComposeDeployment(
    wstring const &deploymentName,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    if (!this->TryAcceptRequestByString(deploymentName, clientRequest))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::Success, callback, root);
    }

    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, clientRequest->ActivityId);

    auto composeDeploymentContext = make_unique<ComposeDeploymentContext>(deploymentName);
    auto error = this->GetComposeDeploymentContext(storeTx, *composeDeploymentContext);
    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    auto composeUpgradeContext = make_unique<ComposeDeploymentUpgradeContext>(deploymentName);
    error = storeTx.ReadExact(*composeUpgradeContext);

    bool shouldCommit = false;
    if (error.IsSuccess())
    {
        if (!composeUpgradeContext->IsPending)
        {
            WriteInfo(
                TraceComponent,
                "{0} compose deployment {1} not upgrading",
                this->TraceId,
                deploymentName);

            return RejectRequest(move(clientRequest), ErrorCodeValue::ComposeDeploymentNotUpgrading, callback, root);
        } 

        error = composeUpgradeContext->TryInterrupt(storeTx);
        if (error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0} interrupted compose deployment upgrade {1} for rollback: {2}",
                this->TraceId,
                deploymentName,
                *composeUpgradeContext);
            shouldCommit = true;
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0} failed to interrupt compose deployment upgrade {1} for rollback. Error: {2}",
                this->TraceId,
                deploymentName,
                error);
        }
    }
    else if (error.IsError(ErrorCodeValue::NotFound))
    {
        WriteInfo(
            TraceComponent,
            "{0} upgrade compose deployment context not found {1}",
            this->TraceId,
            deploymentName);
        error = ErrorCodeValue::ComposeDeploymentNotUpgrading;
    }

    return FinishAcceptRequest(
        move(storeTx),
        move(composeUpgradeContext),
        move(error),
        shouldCommit,
        true,
        timeout,
        callback,
        root);
}

AsyncOperationSPtr ClusterManagerReplica::BeginAcceptUpgradeApplication(
    ApplicationUpgradeDescription const & upgradeDescription,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, clientRequest->ActivityId);

    auto appName = upgradeDescription.ApplicationName;

    if (!this->TryAcceptRequestByName(appName, clientRequest))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::Success, callback, root);
    }

    if (upgradeDescription.RollingUpgradeMode == RollingUpgradeMode::Invalid)
    {
        auto error = this->TraceAndGetErrorDetails( ErrorCodeValue::InvalidArgument, wformatString("{0} {1}",
            GET_RC( Invalid_Upgrade_Mode2 ),
            upgradeDescription.RollingUpgradeMode));

        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    auto appContext = make_unique<ApplicationContext>(appName);
    ErrorCode error = this->GetCompletedOrUpgradingApplicationContext(storeTx, *appContext);

    shared_ptr<ApplicationUpgradeContext> upgradeContext;
    bool readExistingUpgradeContext(false);

    uint64 upgradeInstance = 0;

    if (error.IsSuccess())
    {
        if (appContext->ApplicationDefinitionKind != ApplicationDefinitionKind::Enum::ServiceFabricApplicationDescription)
        {
            return RejectRequest(move(clientRequest),
                ErrorCode(
                    ErrorCodeValue::OperationFailed,
                    wformatString(GET_RC(Invalid_Application_Definition_Kind_Operation), appContext->ApplicationDefinitionKind)),
                    callback, root);
        }

        upgradeInstance = ApplicationUpgradeContext::GetNextUpgradeInstance(appContext->PackageInstance);

        upgradeContext = make_shared<ApplicationUpgradeContext>(
            *this,
            clientRequest,
            upgradeDescription,
            appContext->TypeVersion,
            appContext->ManifestId, // target manifestid
            upgradeInstance);

        error = storeTx.TryReadOrInsertIfNotFound(*upgradeContext, readExistingUpgradeContext);
    }

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    // Check availability of target type version
    //
    auto appTypeContext = make_unique<ApplicationTypeContext>(appContext->TypeName, ServiceModelVersion(upgradeDescription.TargetApplicationTypeVersion));
    error = this->GetCompletedApplicationTypeContext(storeTx, *appTypeContext);
    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    // Prevent duplicate upgrade request calls only if the previous upgrade completed
    // or if no upgrade has ever occurred
    //
    bool checkTypeVersion = true;
    if (readExistingUpgradeContext && !upgradeContext->IsFailed)
    {
        switch (upgradeContext->UpgradeState)
        {
            case ApplicationUpgradeState::CompletedRollforward:
            case ApplicationUpgradeState::CompletedRollback:
            case ApplicationUpgradeState::Interrupted:
                checkTypeVersion = true;
                break;

            case ApplicationUpgradeState::RollingBack:
                //
                // The rollout context is still rolling back, but allow setting the
                // goal state to be consumed automatically once the rollback completes
                // (Intentional fall-through).
                //

            case ApplicationUpgradeState::RollingForward:
                //
                // The rollout context is still running, but allow interruption of previous upgrade.
                //
                checkTypeVersion = false;
                break;

            default:
                this->TestAssertAndTransientFault(wformatString("unrecognized application upgrade state '{0}' ({1})", appName, upgradeContext->UpgradeState));
                return RejectRequest(move(clientRequest), ErrorCodeValue::OperationFailed, callback, root);
        }
    }
    else
    {
        // updating the manifestId based on ApplicationTypeContext for first upgrade
        upgradeContext->TargetApplicationManifestId = appTypeContext->ManifestId;
        storeTx.Update(*upgradeContext);
    }

    if (checkTypeVersion && !upgradeContext->IsFailed)
    {
        StoreDataApplicationManifest currentManifest(appContext->TypeName, appContext->TypeVersion);
        error = storeTx.ReadExact(currentManifest);

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: Application Manifest {1}:{2} not found",
                this->TraceId,
                appContext->TypeName,
                appContext->TypeVersion);
            return RejectRequest(move(clientRequest), ErrorCodeValue::ApplicationTypeNotFound, callback, root);
        }

        if (!appContext->CanAcceptUpgrade(upgradeDescription, currentManifest))
        {
            if (readExistingUpgradeContext && upgradeContext->UpgradeState == ApplicationUpgradeState::Interrupted)
            {
                // When interrupting a V1->V2 upgrade with V1 as the target, flip the state to CompletedRollforward
                // and reply with success instead of failing with AlreadyInTargetVersion and leaving the upgrade
                // state in CompletedRollback.
                //
                auto rollbackDescription = upgradeContext->TakeUpgradeDescription();
                rollbackDescription.SetTargetVersion(appContext->TypeVersion.Value);
                rollbackDescription.SetUpgradeMode(RollingUpgradeMode::UnmonitoredAuto);
                rollbackDescription.SetApplicationParameters(appContext->Parameters);

                upgradeContext = make_shared<ApplicationUpgradeContext>(
                    *this,
                    clientRequest,
                    rollbackDescription,
                    appContext->TypeVersion,
                    appTypeContext->ManifestId,
                    upgradeInstance);

                upgradeContext->CompleteRollforward(storeTx);

                return FinishAcceptRequest(
                    move(storeTx),
                    move(upgradeContext),
                    ErrorCodeValue::Success,
                    true, // commit
                    true, // complete client request
                    timeout,
                    callback,
                    root);
            }
            else
            {
                WriteWarning(
                    TraceComponent,
                    "{0} application {1} is already version '{2}' or application parameters have not changed",
                    this->TraceId,
                    appContext->ApplicationName,
                    upgradeDescription.TargetApplicationTypeVersion);

                return RejectRequest(move(clientRequest), ErrorCodeValue::ApplicationAlreadyInTargetVersion, callback, root);
            }
        }
    }

    wstring errorDetails;
    if (upgradeDescription.RollingUpgradeMode == RollingUpgradeMode::Monitored && !upgradeDescription.TryValidate(errorDetails))
    {
        error = this->TraceAndGetErrorDetails( ErrorCodeValue::ApplicationUpgradeValidationError, move(errorDetails));

        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    // If the previous upgrade context has completed, then replace it with
    // this new pending upgrade context since we leave completed
    // upgrade contexts around until a new upgrade comes in. Upgrade contexts
    // are only deleted when their corresponding application contexts are deleted
    // or if the upgrades fail in a way that cannot be rolled back.
    //
    // If the previous upgrade context is still pending, then cancel it first.
    //
    bool shouldCommit(true);
    bool shouldCompleteClient(false);
    if (readExistingUpgradeContext)
    {
        if (upgradeContext->IsComplete || upgradeContext->IsFailed)
        {
            auto sequenceNumber = upgradeContext->SequenceNumber;

            upgradeContext = make_shared<ApplicationUpgradeContext>(
                *this,
                clientRequest,
                upgradeDescription,
                appContext->TypeVersion,
                appTypeContext->ManifestId,
                upgradeInstance);
            upgradeContext->SetSequenceNumber(sequenceNumber);

            error = storeTx.Update(*upgradeContext);

            // Ensure verified domains are cleared since we may be recovering from a failed
            // upgrade that never completed or rolled back.
            //
            if (error.IsSuccess())
            {
                auto verifiedDomainsStoreData = make_unique<StoreDataVerifiedUpgradeDomains>(appContext->ApplicationId);

                error = this->ClearVerifiedUpgradeDomains(storeTx, *verifiedDomainsStoreData);
            }

            if (error.IsSuccess())
            {
                auto goalStateContext = make_unique<GoalStateApplicationUpgradeContext>(upgradeContext->UpgradeDescription.ApplicationName);

                error = storeTx.DeleteIfFound(*goalStateContext);
            }
        }
        else if (upgradeContext->IsPending)
        {
            // Only interrupt the pending upgrade if some characteristic of
            // the upgrade request has actually changed. This prevents
            // restarting a pending upgrade if the client retries the same
            // upgrade.
            //
            if (upgradeContext->UpgradeDescription == upgradeDescription)
            {
                WriteInfo(
                    TraceComponent,
                    "{0} application upgrade already in progress: [{1}]",
                    this->TraceId,
                    upgradeContext->UpgradeDescription);

                error = this->UpdateOperationTimeout(storeTx, *upgradeContext, clientRequest->Timeout, shouldCommit);

                if (error.IsError(ErrorCodeValue::CMRequestAlreadyProcessing))
                {
                    // Commit new timeout, but return error to client instead of internally retrying
                    // since upgrade could take a long time
                    //
                    error = ErrorCodeValue::ApplicationUpgradeInProgress;
                }
            }
            //
            // Updating health policies via upgrade (TryUpdateHealthPolicies) is not supported for applications
            // due to conflicting interactions with support for goal state upgrades. Clients should use
            // the offical update API instead to modify upgrade health polices.
            //
            else
            {
                bool interrupted = upgradeContext->TryInterrupt();

                if (interrupted)
                {
                    WriteInfo(
                        TraceComponent,
                        "{0} interrupted application {1}: pending=[{2}] existing=[{3}]",
                        this->TraceId,
                        appContext->ApplicationName,
                        upgradeDescription,
                        *upgradeContext);
                }
                else
                {
                    WriteInfo(
                        TraceComponent,
                        "{0} updating goal state upgrade on application {1}: pending=[{2}] existing=[{3}]",
                        this->TraceId,
                        appContext->ApplicationName,
                        upgradeDescription,
                        *upgradeContext);
                }

                auto goalStateContext = make_unique<GoalStateApplicationUpgradeContext>(
                    *this,
                    clientRequest,
                    upgradeDescription,
                    appContext->TypeVersion,
                    appTypeContext->ManifestId,
                    upgradeInstance);

                error = storeTx.UpdateOrInsertIfNotFound(*goalStateContext);

                if (error.IsSuccess())
                {
                    error = storeTx.Update(*upgradeContext);
                }

                if (error.IsSuccess())
                {
                    shouldCompleteClient = true;
                    upgradeContext->ClientRequest = clientRequest;
                }
            }
        }
        else
        {
            WriteNoise(
                TraceComponent,
                "{0} application {1} already has pending upgrade ({2})",
                this->TraceId,
                appContext->ApplicationName,
                *upgradeContext);

            error = this->UpdateOperationTimeout(storeTx, *upgradeContext, clientRequest->Timeout, shouldCommit);
        }
    } // if readExistingUpgradeContext

    if (error.IsSuccess())
    {
        if (!appContext->IsUpgrading)
        {
            error = appContext->SetUpgradePending(storeTx);
        }
    }

    return FinishAcceptRequest(
        move(storeTx),
        move(upgradeContext),
        move(error),
        shouldCommit,
        shouldCompleteClient,
        timeout,
        callback,
        root);
}

AsyncOperationSPtr ClusterManagerReplica::BeginAcceptRollbackApplicationUpgrade(
    NamingUri const & appName,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, clientRequest->ActivityId);

    if (!this->TryAcceptRequestByName(appName, clientRequest))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::Success, callback, root);
    }

    auto appContext = make_unique<ApplicationContext>(appName);
    ErrorCode error = this->GetApplicationContext(storeTx, *appContext);

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }
    else if (appContext->ApplicationDefinitionKind != ApplicationDefinitionKind::Enum::ServiceFabricApplicationDescription)
    {
        return RejectRequest(move(clientRequest),
            ErrorCode(
                ErrorCodeValue::OperationFailed,
                wformatString(GET_RC(Invalid_Application_Definition_Kind_Operation), appContext->ApplicationDefinitionKind)),
            callback, root);
    }

    auto upgradeContext = make_unique<ApplicationUpgradeContext>(appContext->ApplicationName);
    error = storeTx.ReadExact(*upgradeContext);

    bool shouldCommit = false;
    if (error.IsSuccess())
    {
        if (!upgradeContext->IsPending)
        {
            WriteInfo(
                TraceComponent,
                "{0} application not upgrading {1}",
                this->TraceId,
                appName);

            return RejectRequest(move(clientRequest), ErrorCodeValue::ApplicationNotUpgrading, callback, root);
        }

        bool interrupted = upgradeContext->TryInterrupt();

        if (interrupted)
        {
            error = storeTx.Update(*upgradeContext);

            if (error.IsSuccess() && !appContext->IsUpgrading)
            {
                error = appContext->SetUpgradePending(storeTx);
            }

            if (error.IsSuccess())
            {
                WriteInfo(
                    TraceComponent,
                    "{0} interrupted application {1} for rollback: {2}",
                    this->TraceId,
                    appContext->ApplicationName,
                    *upgradeContext);

                shouldCommit = true;
            }
            else
            {
                WriteInfo(
                    TraceComponent,
                    "{0} failed to interrupt application {1} for rollback: {2}",
                    this->TraceId,
                    appContext->ApplicationName,
                    error);
            }
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0} application {1} already interrupted: {2}",
                this->TraceId,
                appContext->ApplicationName,
                *upgradeContext);
        }

        if (error.IsSuccess())
        {
            auto goalStateContext = make_unique<GoalStateApplicationUpgradeContext>(upgradeContext->UpgradeDescription.ApplicationName);

            error = storeTx.DeleteIfFound(*goalStateContext);

            if (error.IsSuccess())
            {
                WriteInfo(
                    TraceComponent,
                    "{0} cleared goal state upgrade on {1}",
                    this->TraceId,
                    appContext->ApplicationName);
            }
        }

        if (error.IsSuccess())
        {
            error = ErrorCodeValue::ApplicationUpgradeInProgress;
        }
    }
    else if (error.IsError(ErrorCodeValue::NotFound))
    {
        WriteInfo(
            TraceComponent,
            "{0} upgrade context not found {1}",
            this->TraceId,
            appName);

        error = ErrorCodeValue::ApplicationNotUpgrading;
    }

    return FinishAcceptRequest(
        move(storeTx),
        move(upgradeContext),
        move(error),
        shouldCommit,
        timeout,
        callback,
        root);
}

AsyncOperationSPtr ClusterManagerReplica::BeginAcceptUpdateApplicationUpgrade(
    ApplicationUpgradeUpdateDescription const & updateDescription,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, clientRequest->ActivityId);

    auto const & appName = updateDescription.ApplicationName;

    if (!this->TryAcceptRequestByName(appName, clientRequest))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::Success, callback, root);
    }

    auto appContext = make_unique<ApplicationContext>(appName);
    auto error = this->GetCompletedOrUpgradingApplicationContext(storeTx, *appContext);

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }
    else if (appContext->ApplicationDefinitionKind != ApplicationDefinitionKind::Enum::ServiceFabricApplicationDescription)
    {
        return RejectRequest(move(clientRequest),
            ErrorCode(
                ErrorCodeValue::OperationFailed,
                wformatString(GET_RC(Invalid_Application_Definition_Kind_Operation), appContext->ApplicationDefinitionKind)),
            callback, root);
    }

    auto upgradeContext = make_unique<ApplicationUpgradeContext>(appName);
    error = storeTx.ReadExact(*upgradeContext);

    if (error.IsError(ErrorCodeValue::NotFound))
    {
        WriteInfo(
            TraceComponent,
            "{0} upgrade context not found {1}",
            this->TraceId,
            appName);

        return RejectRequest(move(clientRequest), ErrorCodeValue::ApplicationNotUpgrading, callback, root);
    }
    else if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    WriteInfo(
        TraceComponent,
        "{0} modifying application upgrade: {1}",
        this->TraceId,
        updateDescription);

    wstring errorDetails;
    if (!upgradeContext->TryModifyUpgrade(updateDescription, errorDetails))
    {
        error = this->TraceAndGetErrorDetails( ErrorCodeValue::ApplicationUpgradeValidationError, move(errorDetails));

        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    error = storeTx.Update(*upgradeContext);

    if (error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} modified upgrade description: {1}",
            this->TraceId,
            upgradeContext->UpgradeDescription);

        // We expect the upgrade to be pending and do not want
        // to get a CMRequestAlreadyProcessing error, which would be retried
        // by the client async operation wrapper.
        //
        error = ErrorCodeValue::ApplicationUpgradeInProgress;
    }

    return FinishAcceptRequest(
        move(storeTx),
        move(upgradeContext),
        move(error),
        true,   // shouldCommit
        timeout,
        callback,
        root);
}

ErrorCode ClusterManagerReplica::GetApplicationUpgradeStatus(
    Common::NamingUri const & applicationName,
    ClientRequestSPtr const & clientRequest,
    __out ApplicationUpgradeStatusDescription & statusResult)
{
    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, clientRequest->ActivityId);

    auto appContext = make_unique<ApplicationContext>(applicationName);
    ErrorCode error = this->GetApplicationContext(storeTx, *appContext);

    if (!error.IsSuccess())
    {
        return error;
    }

    auto upgradeContext = make_unique<ApplicationUpgradeContext>(appContext->ApplicationName);
    error = storeTx.ReadExact(*upgradeContext);

    if (error.IsSuccess())
    {
        auto upgradeDescription = make_shared<ApplicationUpgradeDescription>(upgradeContext->TakeUpgradeDescription());
        auto targetVersion = upgradeDescription->TargetApplicationTypeVersion;
        auto upgradeMode = upgradeDescription->RollingUpgradeMode;
        auto upgradeState = upgradeContext->UpgradeState;

        if (upgradeContext->IsFailed)
        {
            WriteInfo(
                TraceComponent,
                "{0} overriding {1} upgrade state {2} with Failed",
                this->TraceId,
                applicationName,
                upgradeState);

            upgradeState = ApplicationUpgradeState::Failed;
        }

        switch (upgradeState)
        {
            case ApplicationUpgradeState::RollingBack:
            case ApplicationUpgradeState::CompletedRollback:
            case ApplicationUpgradeState::Interrupted:
                targetVersion = upgradeContext->RollbackApplicationTypeVersion.Value;
                upgradeDescription->SetTargetVersion(targetVersion);
                upgradeDescription->SetUpgradeMode(upgradeMode);
                upgradeDescription->SetApplicationParameters(appContext->Parameters);
                upgradeDescription->SetReplicaSetCheckTimeout(this->GetRollbackReplicaSetCheckTimeout(
                    upgradeDescription->ReplicaSetCheckTimeout));
                break;

            case ApplicationUpgradeState::RollingForward:
            case ApplicationUpgradeState::CompletedRollforward:
            case ApplicationUpgradeState::Failed:
                // no-op: keep values from upgrade description
                break;

            default:
                WriteWarning(
                    TraceComponent,
                    "{0} invalid upgrade state {1}",
                    this->TraceId,
                    upgradeState);

                Assert::TestAssert();
                return ErrorCodeValue::OperationFailed;
        }

        // this in-memory copy of the upgrade context is only used to build the result
        statusResult = ApplicationUpgradeStatusDescription(
            applicationName,
            appContext->TypeName.Value,
            targetVersion,
            upgradeState,
            move(const_cast<wstring&>(upgradeContext->InProgressUpgradeDomain)),
            move(const_cast<vector<wstring>&>(upgradeContext->CompletedUpgradeDomains)),
            move(const_cast<vector<wstring>&>(upgradeContext->PendingUpgradeDomains)),
            upgradeContext->RollforwardInstance,
            upgradeMode,
            move(upgradeDescription),
            upgradeContext->OverallUpgradeElapsedTime,
            upgradeContext->UpgradeDomainElapsedTime,
            upgradeContext->TakeUnhealthyEvaluations(),
            upgradeContext->TakeCurrentUpgradeDomainProgress(),
            upgradeContext->TakeCommonUpgradeContextData());
    }
    else if (error.IsError(ErrorCodeValue::NotFound))
    {
        if (appContext->IsComplete)
        {
            statusResult = ApplicationUpgradeStatusDescription(
                applicationName,
                appContext->TypeName.Value,
                appContext->TypeVersion.Value,
                ApplicationUpgradeState::CompletedRollforward,
                wstring(),
                vector<wstring>(),
                vector<wstring>(),
                0,
                RollingUpgradeMode::Invalid,
                shared_ptr<ApplicationUpgradeDescription>(),
                TimeSpan::Zero,
                TimeSpan::Zero,
                vector<HealthEvaluation>(),
                Reliability::UpgradeDomainProgress(),
                CommonUpgradeContextData());

            error = ErrorCodeValue::Success;
        }
        else
        {
            error = ErrorCodeValue::CMRequestAlreadyProcessing;
        }
    }

    if (error.IsSuccess())
    {
        storeTx.CommitReadOnly();
    }

    return error;
}

AsyncOperationSPtr ClusterManagerReplica::BeginAcceptReportUpgradeHealth(
    NamingUri const & appName,
    vector<wstring> && verifiedDomains,
    uint64 upgradeInstance,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    if (!this->TryAcceptRequestByName(appName, clientRequest))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::Success, callback, root);
    }

    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, clientRequest->ActivityId);

    auto appContext = make_unique<ApplicationContext>(appName);
    ErrorCode error = this->GetCompletedOrUpgradingApplicationContext(storeTx, *appContext);

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }
    else if (!appContext->IsUpgrading)
    {
        WriteInfo(
            TraceComponent,
            "{0} application not upgrading ({1}): {2}",
            this->TraceId,
            appContext->ApplicationId,
            verifiedDomains);

        return RejectRequest(move(clientRequest), ErrorCodeValue::ApplicationNotUpgrading, callback, root);
    }

    auto upgradeContext = make_unique<ApplicationUpgradeContext>(appName);
    error = storeTx.ReadExact(*upgradeContext);

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }
    else if (!upgradeContext->UpgradeDescription.IsUnmonitoredManual)
    {
        error = this->TraceAndGetErrorDetails( ErrorCodeValue::InvalidArgument, wformatString("{0} ({1}, {2})", GET_RC( Invalid_Upgrade_Mode ),
                appContext->ApplicationId,
                upgradeContext->UpgradeDescription.RollingUpgradeMode));

        return RejectRequest(move(clientRequest), move(error), callback, root);
    }
    // Allow 0 instances for old clients
    else if (upgradeInstance > 0 && upgradeInstance != upgradeContext->RollforwardInstance)
    {
        error = this->TraceAndGetErrorDetails( ErrorCodeValue::InvalidMessage, wformatString("{0} ({1}, {2}, {3})", GET_RC( Invalid_Upgrade_Instance ),
                appContext->ApplicationId,
                upgradeInstance,
                upgradeContext->RollforwardInstance));

        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    vector<wstring> const & completedDomains = upgradeContext->CompletedUpgradeDomains;

    // Filter out upgrade domain (UD) names that haven't actually completed yet.
    // This prevents the FM from progressing through UDs too soon
    // since the FM assumes an ordering on UD names and only does a simple
    // inequality comparison between the current pending UD name and the last
    // verified UD name being reported.
    //
    verifiedDomains.erase(
            remove_if(verifiedDomains.begin(), verifiedDomains.end(), [&completedDomains](wstring const & item)
                {
                    return (find(completedDomains.begin(), completedDomains.end(), item) == completedDomains.end());
                }),
            verifiedDomains.end());

    WriteInfo(
        TraceComponent,
        "{0} reporting verified upgrade domains ({1}): {2}",
        this->TraceId,
        appContext->ApplicationId,
        verifiedDomains);

    auto verifiedDomainsStoreData = make_unique<StoreDataVerifiedUpgradeDomains>(appContext->ApplicationId, move(verifiedDomains));

    error = storeTx.UpdateOrInsertIfNotFound(*verifiedDomainsStoreData);

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    return FinishAcceptRequest(
        move(storeTx),
        move(upgradeContext),
        move(error),
        true,   // shouldCommit
        timeout,
        callback,
        root);
}

AsyncOperationSPtr ClusterManagerReplica::BeginAcceptMoveNextUpgradeDomain(
    NamingUri const & appName,
    wstring const & nextUpgradeDomain,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    ApplicationUpgradeStatusDescription upgradeStatus;

    ErrorCode error = this->GetApplicationUpgradeStatus(appName, clientRequest, upgradeStatus);
    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    auto const & completedDomains = upgradeStatus.CompletedUpgradeDomains;

    // We expect the first pending upgrade domain to be processed next by the FM.
    // When this contract changes, the logic here needs to be updated and the protocol
    // between the CM and FM should be updated to make the next domain explicit.
    //
    if (upgradeStatus.NextUpgradeDomain.empty())
    {
        WriteInfo(
            TraceComponent,
            "{0} no pending UD for move next: state = {1}",
            this->TraceId,
            upgradeStatus.UpgradeState);

        return RejectRequest(move(clientRequest), ErrorCodeValue::ApplicationNotUpgrading, callback, root);
    }
    else if (!completedDomains.empty() && find(completedDomains.begin(), completedDomains.end(), nextUpgradeDomain) != completedDomains.end())
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::UpgradeDomainAlreadyCompleted, callback, root);
    }
    else if (!upgradeStatus.InProgressUpgradeDomain.empty() || nextUpgradeDomain != upgradeStatus.NextUpgradeDomain)
    {
        error = this->TraceAndGetErrorDetails( ErrorCodeValue::InvalidArgument, wformatString(GET_RC( Invalid_Upgrade_Domain ),
                nextUpgradeDomain,
                upgradeStatus.NextUpgradeDomain,
                upgradeStatus.InProgressUpgradeDomain));

        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    return this->BeginAcceptReportUpgradeHealth(
        appName,
        move(const_cast<vector<wstring>&>(upgradeStatus.CompletedUpgradeDomains)),
        0,
        move(clientRequest),
        timeout,
        callback,
        root);
}

// *****************
// Fabric Management
// *****************

AsyncOperationSPtr ClusterManagerReplica::BeginAcceptProvisionFabric(
    wstring const & codeFilepath,
    wstring const & clusterManifestFilepath,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    if (!this->TryAcceptClusterOperationRequest(clientRequest))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::Success, callback, root);
    }

    FabricVersion provisionVersion;
    auto error = this->GetFabricVersionInfo(
        codeFilepath,
        clusterManifestFilepath,
        clientRequest->Timeout,
        provisionVersion);

    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} failed to get Fabric version from Image Builder: {1}",
            this->TraceId,
            error);

        return RejectRequest(move(clientRequest), move(error), callback, root);
    }
    else if (!provisionVersion.CodeVersion.IsValid && !provisionVersion.ConfigVersion.IsValid)
    {
        error = this->TraceAndGetErrorDetails( ErrorCodeValue::InvalidArgument, wformatString("{0} {1}", GET_RC( Invalid_Fabric_Version ),
                provisionVersion));

        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, clientRequest->ActivityId);

    auto context = make_unique<FabricProvisionContext>(
        *this,
        clientRequest,
        codeFilepath,
        clusterManifestFilepath,
        provisionVersion);

    bool containsVersion(false);
    error = this->GetProvisionedFabricContext(storeTx, provisionVersion, *context, containsVersion);

    bool shouldCommit(true);
    if (error.IsError(ErrorCodeValue::FabricVersionNotFound))
    {
        error = storeTx.Insert(*context);
    }
    else if (error.IsError(ErrorCodeValue::CMRequestAlreadyProcessing))
    {
        error = this->UpdateOperationTimeout(storeTx, *context, clientRequest->Timeout, shouldCommit);
    }
    else if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }
    else
    {
        if (containsVersion)
        {
            WriteInfo(
                TraceComponent,
                "{0} Fabric version {1} already exists: code = {2} config = {3}",
                this->TraceId,
                provisionVersion,
                context->FormatProvisionedCodeVersions(),
                context->FormatProvisionedConfigVersions());

            return RejectRequest(move(clientRequest), ErrorCodeValue::FabricVersionAlreadyExists, callback, root);
        }
        else
        {
            error = context->StartProvisioning(
                storeTx,
                *this,
                clientRequest,
                codeFilepath,
                clusterManifestFilepath,
                provisionVersion);
        }
    }

    return FinishAcceptRequest(
        move(storeTx),
        move(context),
        move(error),
        shouldCommit,
        timeout,
        callback,
        root);
}

AsyncOperationSPtr ClusterManagerReplica::BeginAcceptUpgradeFabric(
    FabricUpgradeDescription const & upgradeDescription,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    if (!this->TryAcceptClusterOperationRequest(clientRequest))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::Success, callback, root);
    }

    if (upgradeDescription.RollingUpgradeMode == RollingUpgradeMode::Invalid)
    {
        auto error = this->TraceAndGetErrorDetails( ErrorCodeValue::InvalidArgument, wformatString("{0} {1}",
            GET_RC( Invalid_Upgrade_Mode2 ),
            upgradeDescription.RollingUpgradeMode));

        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, clientRequest->ActivityId);

    // Check that target versions have been provisioned
    //
    auto provisionContext = make_unique<FabricProvisionContext>();
    bool containsVersion(false);
    ErrorCode error = this->GetProvisionedFabricContext(storeTx, upgradeDescription.Version, *provisionContext, containsVersion);
    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }
    else if (!containsVersion)
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::FabricVersionNotFound, callback, root);
    }

    // Current version is invalid for the first Fabric upgrade
    //
    auto upgradeContext = make_shared<FabricUpgradeContext>(
        *this,
        clientRequest,
        upgradeDescription,
        FabricVersion::Invalid,
        DateTime::Now().Ticks);

    bool readExistingUpgradeContext(false);
    error = storeTx.TryReadOrInsertIfNotFound(*upgradeContext, readExistingUpgradeContext);

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    // The first Fabric upgrade MUST establish both the initial code and config versions.
    // That is, the first Fabric upgrade cannnot be a code or config only upgrade.
    //
    if (!upgradeContext->CurrentVersion.IsValid)
    {
        if (!upgradeDescription.Version.IsValid)
        {
            error = this->TraceAndGetErrorDetails( ErrorCodeValue::FabricUpgradeValidationError, wformatString("{0} {1}", GET_RC( Invalid_First_Upgrade ),
                    upgradeDescription.Version));

            return RejectRequest(move(clientRequest), move(error), callback, root);
        }
    }
    else
    {
        // For any Fabric upgrade after the first, either the code or config version be valid.
        // This check is performed on the client, but also check on the server.
        //
        if (!upgradeDescription.Version.CodeVersion.IsValid && !upgradeDescription.Version.ConfigVersion.IsValid)
        {
            error = this->TraceAndGetErrorDetails( ErrorCodeValue::InvalidArgument, wformatString("{0} {1}", GET_RC( Invalid_Fabric_Upgrade ),
                    upgradeDescription.Version));

            return RejectRequest(move(clientRequest), move(error), callback, root);
        }

        if (upgradeDescription.Version.CodeVersion.IsValid)
        {
            // Unsupported preview feature(s) are not expected to work across cluster upgrades for various reasons (e.g. state management, functional changes).
            // Thus, prevent any code upgrades from taking place if preview feature support is enabled. Config upgrades should be okay to upgrade since
            // they cannot introduce a change that breaks existing code.
            if (CommonConfig::GetConfig().EnableUnsupportedPreviewFeatures)
            {
                auto errorToReturn = this->TraceAndGetErrorDetails(
                    ErrorCodeValue::FabricUpgradeValidationError,
                    wformatString(GET_RC(UnsupportedPreviewFeature_Upgrade_Failed), upgradeDescription.Version));

                return RejectRequest(move(clientRequest), move(errorToReturn), callback, root);
            }
        }
    }

    // If NULL was specified for one of the upgrade version parameters, then replace the NULL version
    // with the latest known current version to indicate that upgrade is not changing for that component.
    //
    // The state of the context doesn't matter since the CurrentVersion property
    // contains the last successfully upgraded version.
    //
    auto targetCodeVersion = upgradeDescription.Version.CodeVersion;
    if (!targetCodeVersion.IsValid)
    {
        targetCodeVersion = upgradeContext->CurrentVersion.CodeVersion;
    }

    auto targetConfigVersion = upgradeDescription.Version.ConfigVersion;
    if (!targetConfigVersion.IsValid)
    {
        targetConfigVersion = upgradeContext->CurrentVersion.ConfigVersion;
    }

    FabricVersion targetVersion(targetCodeVersion, targetConfigVersion);
    if (targetVersion != upgradeDescription.Version)
    {
        WriteInfo(
            TraceComponent,
            "{0} replacing target version {1} with {2} for Fabric upgrade",
            this->TraceId,
            upgradeDescription.Version,
            targetVersion);
    }

    // Replace with the digested target FabricVersion
    //
    FabricUpgradeDescription targetDescription(
        targetVersion,
        upgradeDescription.UpgradeType,
        upgradeDescription.RollingUpgradeMode,
        upgradeDescription.ReplicaSetCheckTimeout,
        upgradeDescription.MonitoringPolicy,
        upgradeDescription.HealthPolicy,
        upgradeDescription.IsHealthPolicyValid,
        upgradeDescription.EnableDeltaHealthEvaluation,
        upgradeDescription.UpgradeHealthPolicy,
        upgradeDescription.IsUpgradeHealthPolicyValid,
        upgradeDescription.ApplicationHealthPolicies);

    // Basic version checks to validate the requested upgrade
    //
    if (readExistingUpgradeContext && !upgradeContext->IsFailed)
    {
        switch (upgradeContext->UpgradeState)
        {
            case FabricUpgradeState::CompletedRollforward:
            case FabricUpgradeState::CompletedRollback:
            case FabricUpgradeState::Interrupted:
                // Unlike application upgrades, we only check against the existing version if a prior Fabric upgrade
                // has occurred. Until a Fabric upgrade has completed, we cannot be certain of the cluster version
                // since it was managed out of band.
                //
                if (upgradeContext->CurrentVersion == targetVersion)
                {
                    if (upgradeContext->UpgradeState == FabricUpgradeState::Interrupted)
                    {
                        // When interrupting a V1->V2 upgrade with V1 as the target, flip the state to CompletedRollforward
                        // and reply with success instead of failing with AlreadyInTargetVersion and leaving the upgrade
                        // state in CompletedRollback.
                        //
                        upgradeContext->MutableUpgradeDescription.SetTargetVersion(targetVersion);
                        upgradeContext->MutableUpgradeDescription.SetUpgradeMode(RollingUpgradeMode::UnmonitoredAuto);

                        upgradeContext->CompleteRollforward(storeTx);

                        return FinishAcceptRequest(
                            move(storeTx),
                            move(upgradeContext),
                            ErrorCodeValue::Success,
                            true, // commit
                            true, // complete client request
                            timeout,
                            callback,
                            root);
                    }
                    else
                    {
                        WriteWarning(
                            TraceComponent,
                            "{0} Fabric is already version '{1}'",
                            this->TraceId,
                            targetVersion);

                        return RejectRequest(move(clientRequest), ErrorCodeValue::FabricAlreadyInTargetVersion, callback, root);
                    }
                }
                break;

            case FabricUpgradeState::RollingBack:
                // The rollout context is still rolling back. New upgrades shouldn't be
                // started until it has entered the CompletedRollback state.
                //
                WriteWarning(
                    TraceComponent,
                    "{0} Fabric is already rolling back {1} -> {2}",
                    this->TraceId,
                    upgradeContext->UpgradeDescription.Version,
                    upgradeContext->CurrentVersion);

                return RejectRequest(move(clientRequest), ErrorCodeValue::CMRequestAlreadyProcessing, callback, root);

            case FabricUpgradeState::RollingForward:
                // No validation needed
                break;

            default:
                this->TestAssertAndTransientFault(wformatString("unrecognized Fabric upgrade state ({0})", upgradeContext->UpgradeState));
                return RejectRequest(move(clientRequest), ErrorCodeValue::OperationFailed, callback, root);
        }
    }

    wstring errorDetails;
    if (upgradeDescription.RollingUpgradeMode == RollingUpgradeMode::Monitored && !upgradeDescription.TryValidate(errorDetails))
    {
        error = this->TraceAndGetErrorDetails( ErrorCodeValue::FabricUpgradeValidationError, move(errorDetails));

        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    // Either start a new upgrade or interrupt the existing upgrade if it
    // passed the version checks above
    //
    bool shouldCommit(true);
    bool shouldCompleteClient(false);
    if (!readExistingUpgradeContext)
    {
        // Update target description, which may contain replaced Invalid versions
        //
        error = upgradeContext->StartUpgrading(
            storeTx,
            *this,
            clientRequest,
            targetDescription);
    }
    else
    {
        // Allow restarting of a failed Fabric upgrade context
        //
        if (upgradeContext->IsComplete || upgradeContext->IsFailed)
        {
            error = upgradeContext->StartUpgrading(
                storeTx,
                *this,
                clientRequest,
                targetDescription);

            // Ensure verified domains are cleared since we may be recovering from a failed
            // upgrade that never completed or rolled back.
            //
            if (error.IsSuccess())
            {
                auto verifiedDomainsStoreData = make_unique<StoreDataVerifiedFabricUpgradeDomains>();

                error = this->ClearVerifiedUpgradeDomains(storeTx, *verifiedDomainsStoreData);
            }
        }
        else if (upgradeContext->IsPending)
        {
            // Only interrupt the pending upgrade if some characteristic of
            // the upgrade request has actually changed. This prevents
            // restarting a pending upgrade if the client retries the same
            // upgrade.
            //
            if (upgradeContext->UpgradeDescription == targetDescription)
            {
                WriteInfo(
                    TraceComponent,
                    "{0} Fabric upgrade already in progress: [{1}]",
                    this->TraceId,
                    upgradeContext->UpgradeDescription);

                error = this->UpdateOperationTimeout(storeTx, *upgradeContext, clientRequest->Timeout, shouldCommit);

                if (error.IsError(ErrorCodeValue::CMRequestAlreadyProcessing))
                {
                    // Commit new timeout, but return error to client instead of internally retrying
                    // since upgrade could take a long time
                    //
                    error = ErrorCodeValue::FabricUpgradeInProgress;
                }
            }
            else if (upgradeContext->TryUpdateHealthPolicies(targetDescription))
            {
                error = storeTx.Update(*upgradeContext);

                if (error.IsSuccess())
                {
                    shouldCompleteClient = true;

                    WriteInfo(
                        TraceComponent,
                        "{0} updated Fabric health policies. Upgrade description: {1}",
                        this->TraceId,
                        upgradeContext->UpgradeDescription);
                }
                else
                {
                    WriteInfo(
                        TraceComponent,
                        "{0} failed to update Fabric health policies. Upgrade description = {1}, error = {2}",
                        this->TraceId,
                        upgradeContext->UpgradeDescription,
                        error);
                }
            }
            else
            {
                bool interrupted = upgradeContext->TryInterrupt();

                if (interrupted)
                {
                    WriteInfo(
                        TraceComponent,
                        "{0} interrupted Fabric upgrade: pending = [{1}] existing = [{2}]",
                        this->TraceId,
                        targetDescription,
                        *upgradeContext);
                }
                else
                {
                    WriteInfo(
                        TraceComponent,
                        "{0} Fabric upgrade ({1}) was already interrupted",
                        this->TraceId,
                        *upgradeContext);
                }

                shouldCommit = (interrupted || upgradeContext->TrySetOperationTimeout(clientRequest->Timeout));

                if (shouldCommit)
                {
                    error = storeTx.Update(*upgradeContext);
                }

                if (error.IsSuccess())
                {
                    error = ErrorCodeValue::CMRequestAlreadyProcessing;
                }
            }
        }
        else
        {
            WriteNoise(
                TraceComponent,
                "{0} Fabric already has pending upgrade ({1})",
                this->TraceId,
                *upgradeContext);

            error = this->UpdateOperationTimeout(storeTx, *upgradeContext, clientRequest->Timeout, shouldCommit);
        }
    } // if readExistingUpgradeContext

    return FinishAcceptRequest(
        move(storeTx),
        move(upgradeContext),
        move(error),
        shouldCommit,
        shouldCompleteClient,
        timeout,
        callback,
        root);
}

AsyncOperationSPtr ClusterManagerReplica::BeginAcceptRollbackFabricUpgrade(
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    if (!this->TryAcceptClusterOperationRequest(clientRequest))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::Success, callback, root);
    }

    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, clientRequest->ActivityId);

    auto upgradeContext = make_unique<FabricUpgradeContext>();
    ErrorCode error = storeTx.ReadExact(*upgradeContext);

    bool shouldCommit = false;

    if (error.IsSuccess())
    {
        if (!upgradeContext->IsPending)
        {
            WriteInfo(
                TraceComponent,
                "{0} Fabric not upgrading - cannot rollback",
                this->TraceId);

            return RejectRequest(move(clientRequest), ErrorCodeValue::FabricNotUpgrading, callback, root);
        }

        bool interrupted = upgradeContext->TryInterrupt();

        if (interrupted)
        {
            error = storeTx.Update(*upgradeContext);

            if (error.IsSuccess())
            {
                WriteInfo(
                    TraceComponent,
                    "{0} interrupted Fabric upgrade for rollback: {1}",
                    this->TraceId,
                    *upgradeContext);

                shouldCommit = true;
            }
            else
            {
                WriteInfo(
                    TraceComponent,
                    "{0} failed to interrupt Fabric upgrade for rollback: {1}",
                    this->TraceId,
                    error);
            }
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0} Fabric upgrade already interrupted: {1}",
                this->TraceId,
                *upgradeContext);
        }

        if (error.IsSuccess())
        {
            error = ErrorCodeValue::FabricUpgradeInProgress;
        }
    }
    else if (error.IsError(ErrorCodeValue::NotFound))
    {
        WriteInfo(
            TraceComponent,
            "{0} Fabric upgrade context not found",
            this->TraceId);

        error = ErrorCodeValue::FabricNotUpgrading;
    }

    return FinishAcceptRequest(
        move(storeTx),
        move(upgradeContext),
        move(error),
        shouldCommit,
        timeout,
        callback,
        root);
}

void ClusterManagerReplica::CreateApplicationHealthReport(
    NamingUri const & applicationName,
    Common::SystemHealthReportCode::Enum reportCode,
    std::wstring && applicationPolicyString,
    std::wstring const & applicationTypeName,
    ApplicationDefinitionKind::Enum const applicationDefinitionKind,
    uint64 instanceId,
    uint64 sequenceNumber,
    __inout vector<HealthReport> & reports) const
{
    AttributeList attributes;

    if (Constants::ReportHealthEntityApplicationType)
    {
        TESTASSERT_IF(applicationTypeName.empty(), "{0}: Application type name should not be empty", applicationName);
        attributes.AddAttribute(HealthAttributeNames::ApplicationTypeName, applicationTypeName);
    }

    if (!applicationPolicyString.empty())
    {
        attributes.AddAttribute(HealthAttributeNames::ApplicationHealthPolicy, move(applicationPolicyString));
    }

    attributes.AddAttribute(HealthAttributeNames::ApplicationDefinitionKind, wformatString((int)applicationDefinitionKind));

    auto appInfo = EntityHealthInformation::CreateApplicationEntityHealthInformation(applicationName.ToString(), instanceId);
    auto healthReport = HealthReport::CreateSystemHealthReport(
        reportCode,
        move(appInfo),
        L"" /*extraDescription*/,
        sequenceNumber,
        move(attributes));

    reports.push_back(move(healthReport));
}

void ClusterManagerReplica::CreateApplicationHealthReport(
    ApplicationContext const & appContext,
    Common::SystemHealthReportCode::Enum reportCode,
    std::wstring && applicationPolicyString,
    __inout vector<HealthReport> & reports) const
{
    this->CreateApplicationHealthReport(
        appContext.ApplicationName,
        reportCode,
        move(applicationPolicyString),
        appContext.TypeName.Value,
        appContext.ApplicationDefinitionKind,
        appContext.GlobalInstanceCount,
        appContext.SequenceNumber,
        reports);
}

void ClusterManagerReplica::CreateDeleteApplicationHealthReport(
    std::wstring const & applicationName,
    uint64 instanceId,
    uint64 sequenceNumber,
    __inout vector<HealthReport> & reports) const
{
    auto appInfo = EntityHealthInformation::CreateApplicationEntityHealthInformation(applicationName, instanceId);
    auto healthReport = HealthReport::CreateSystemDeleteEntityHealthReport(
        SystemHealthReportCode::CM_DeleteApplication,
        move(appInfo),
        sequenceNumber);
    reports.push_back(move(healthReport));
}

ErrorCode ClusterManagerReplica::ReportApplicationsType(
    StringLiteral const & traceComponent,
    Common::ActivityId const & activityId,
    AsyncOperationSPtr const & thisSPtr,
    vector<wstring> const & appNames,
    TimeSpan const timeout) const
{
    if (!Constants::ReportHealthEntityApplicationType)
    {
        WriteInfo(TraceComponent, "{0}: {1}: Skip reporting application types, ReportHealthEntityApplicationType=false", this->TraceId, activityId);
        return ErrorCode::Success();
    }

    WriteInfo(TraceComponent, "{0}: {1}: Report application type for {2} apps", this->TraceId, activityId, appNames.size());

    vector<ApplicationContext> contexts;
    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, activityId);
    ErrorCode error(ErrorCodeValue::Success);
    for (auto it = appNames.begin(); it != appNames.end(); ++it)
    {
        NamingUri appName;
        if (!NamingUri::TryParse(*it, appName))
        {
            WriteInfo(traceComponent, "{0} {1}: {2}: not a valid NamingUri", this->TraceId, activityId, *it);
            continue;
        }

        ApplicationContext context(appName);
        error = this->GetCompletedOrUpgradingApplicationContext(storeTx, context);

        if (error.IsSuccess())
        {
            contexts.push_back(move(context));
        }
        else if (error.IsError(ErrorCodeValue::NotFound))
        {
            WriteInfo(
                traceComponent,
                "{0} {1} app context {2}:{3}:{4} not found, skip",
                this->TraceId,
                activityId,
                context.ApplicationName,
                context.TypeName,
                context.TypeVersion);
            error = ErrorCode::Success();
        }
        else
        {
            break;
        }
    }

    if (error.IsSuccess())
    {
        storeTx.CommitReadOnly();
    }

    if (!contexts.empty())
    {
        IHealthAggregatorSPtr healthAggregator;
        error = this->GetHealthAggregator(healthAggregator);
        if (!error.IsSuccess())
        {
            return error;
        }

        vector<ServiceModel::HealthReport> reports;
        for (auto it = contexts.begin(); it != contexts.end(); ++it)
        {
            this->CreateApplicationHealthReport(
                *it,
                Common::SystemHealthReportCode::CM_ApplicationUpdated,
                L"", /*ignore health policy, keep existing values*/
                reports);
        }

        auto operation = healthAggregator->BeginUpdateApplicationsHealth(
            activityId,
            move(reports),
            timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnReportApplicationsTypeComplete(operation, false); },
            thisSPtr);

        this->OnReportApplicationsTypeComplete(operation, true);
    }

    return ErrorCode::Success();
}

void ClusterManagerReplica::OnReportApplicationsTypeComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously) const
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    IHealthAggregatorSPtr healthAggregator;
    auto error = this->GetHealthAggregator(healthAggregator);
    if (error.IsSuccess())
    {
        error = healthAggregator->EndUpdateApplicationsHealth(operation);
    }
}

AsyncOperationSPtr ClusterManagerReplica::BeginAcceptUpdateFabricUpgrade(
    FabricUpgradeUpdateDescription const & updateDescription,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    if (!this->TryAcceptClusterOperationRequest(clientRequest))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::Success, callback, root);
    }

    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, clientRequest->ActivityId);

    auto upgradeContext = make_unique<FabricUpgradeContext>();
    auto error = storeTx.ReadExact(*upgradeContext);

    if (error.IsError(ErrorCodeValue::NotFound))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::FabricNotUpgrading, callback, root);
    }
    else if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    if (upgradeContext->IsFailed)
    {
        WriteWarning(
            TraceComponent,
            "{0}: {1}: Fabric upgrade suspended due to a failed initial upgrade",
            this->TraceId,
            clientRequest->ActivityId);
        return RejectRequest(move(clientRequest), ErrorCodeValue::FabricNotUpgrading, callback, root);
    }

    WriteInfo(
        TraceComponent,
        "{0}: {1}: modifying fabric upgrade: {2}",
        this->TraceId,
        clientRequest->ActivityId,
        updateDescription);

    if (updateDescription.UpdateDescription &&
        updateDescription.UpdateDescription->RollingUpgradeMode &&
        *(updateDescription.UpdateDescription->RollingUpgradeMode) == RollingUpgradeMode::Monitored &&
        upgradeContext->UpgradeDescription.RollingUpgradeMode != RollingUpgradeMode::Monitored)
    {
        ClusterUpgradeStateSnapshot snapshot;
        IHealthAggregatorSPtr healthAggregator;
        error = this->GetHealthAggregator(healthAggregator);
        if (error.IsSuccess())
        {
            error = healthAggregator->GetClusterUpgradeSnapshot(
                clientRequest->ActivityId,
                snapshot);
        }

        if (error.IsSuccess())
        {
            StoreDataClusterUpgradeStateSnapshot snapshotData(move(snapshot));
            error = storeTx.UpdateOrInsertIfNotFound(snapshotData);

            if (error.IsSuccess())
            {
                WriteInfo(
                    TraceComponent,
                    "{0}: {1}: upgrade mode changed from {2} to {3}: resetting cluster state snapshot: {4}",
                    this->TraceId,
                    clientRequest->ActivityId,
                    upgradeContext->UpgradeDescription.RollingUpgradeMode,
                    updateDescription.UpdateDescription->RollingUpgradeMode,
                    snapshotData.StateSnapshot);
            }
        }

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0}: failed to reset cluster state snapshot: error={1}",
                clientRequest->ActivityId,
                error);
            return RejectRequest(move(clientRequest), move(error), callback, root);
        }
    }

    wstring errorDetails;
    if (!upgradeContext->TryModifyUpgrade(updateDescription, errorDetails))
    {
        error = this->TraceAndGetErrorDetails( ErrorCodeValue::FabricUpgradeValidationError, move(errorDetails));

        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    error = storeTx.Update(*upgradeContext);

    if (error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0}: {1}: modified upgrade description: {2}",
            this->TraceId,
            clientRequest->ActivityId,
            upgradeContext->UpgradeDescription);

        // We expect the upgrade to be pending and do not want
        // to get a CMRequestAlreadyProcessing error, which would be retried
        // by the client async operation wrapper.
        //
        error = ErrorCodeValue::FabricUpgradeInProgress;
    }

    return FinishAcceptRequest(
        move(storeTx),
        move(upgradeContext),
        move(error),
        true,   // shouldCommit
        timeout,
        callback,
        root);
}

ErrorCode ClusterManagerReplica::GetFabricUpgradeStatus(
    ClientRequestSPtr const & clientRequest,
    __out FabricUpgradeStatusDescription & statusResult)
{
    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, clientRequest->ActivityId);

    auto upgradeContext = make_unique<FabricUpgradeContext>();
    ErrorCode error = storeTx.ReadExact(*upgradeContext);

    if (error.IsSuccess())
    {
        auto upgradeState = upgradeContext->UpgradeState;

        if (upgradeContext->IsFailed)
        {
            WriteInfo(
                TraceComponent,
                "{0} overriding Fabric upgrade state {1} with Failed",
                this->TraceId,
                upgradeState);

            upgradeState = FabricUpgradeState::Failed;
        }

        auto upgradeDescription = make_shared<FabricUpgradeDescription>(upgradeContext->TakeUpgradeDescription());
        auto targetVersion = upgradeDescription->Version;
        auto upgradeMode = upgradeDescription->RollingUpgradeMode;

        switch (upgradeState)
        {
            case FabricUpgradeState::RollingBack:
            case FabricUpgradeState::CompletedRollback:
            case FabricUpgradeState::Interrupted:
                targetVersion = upgradeContext->CurrentVersion;
                upgradeDescription->SetTargetVersion(targetVersion);
                upgradeDescription->SetUpgradeMode(upgradeMode);
                upgradeDescription->SetReplicaSetCheckTimeout(this->GetRollbackReplicaSetCheckTimeout(
                    upgradeDescription->ReplicaSetCheckTimeout));
                break;

            case FabricUpgradeState::RollingForward:
            case FabricUpgradeState::CompletedRollforward:
            case FabricUpgradeState::Failed:
                // no-op: Keep values on upgrade description
                break;

            default:
                WriteInfo(
                    TraceComponent,
                    "{0} invalid upgrade state {1}",
                    this->TraceId,
                    upgradeState);

                Assert::TestAssert();
                return ErrorCodeValue::OperationFailed;
        }

        // this in-memory copy of the upgrade context is only used to build the result
        statusResult = FabricUpgradeStatusDescription(
            targetVersion,
            upgradeState,
            move(const_cast<wstring&>(upgradeContext->InProgressUpgradeDomain)),
            move(const_cast<vector<wstring>&>(upgradeContext->CompletedUpgradeDomains)),
            move(const_cast<vector<wstring>&>(upgradeContext->PendingUpgradeDomains)),
            upgradeContext->RollforwardInstance,
            upgradeMode,
            move(upgradeDescription),
            upgradeContext->OverallUpgradeElapsedTime,
            upgradeContext->UpgradeDomainElapsedTime,
            upgradeContext->TakeUnhealthyEvaluations(),
            upgradeContext->TakeCurrentUpgradeDomainProgress(),
            upgradeContext->TakeCommonUpgradeContextData());
    }
    else if (error.IsError(ErrorCodeValue::NotFound))
    {
        statusResult = FabricUpgradeStatusDescription(
            FabricVersion::Invalid,
            FabricUpgradeState::CompletedRollforward,
            wstring(),
            vector<wstring>(),
            vector<wstring>(),
            0,
            RollingUpgradeMode::Invalid,
            shared_ptr<FabricUpgradeDescription>(),
            TimeSpan::Zero,
            TimeSpan::Zero,
            vector<HealthEvaluation>(),
            Reliability::UpgradeDomainProgress(),
            CommonUpgradeContextData());

        error = ErrorCodeValue::Success;
    }

    if (error.IsSuccess())
    {
        storeTx.CommitReadOnly();
    }

    return error;
}

AsyncOperationSPtr ClusterManagerReplica::BeginAcceptReportFabricUpgradeHealth(
    vector<wstring> && verifiedDomains,
    uint64 upgradeInstance,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    if (!this->TryAcceptClusterOperationRequest(clientRequest))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::Success, callback, root);
    }

    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, clientRequest->ActivityId);

    auto upgradeContext = make_unique<FabricUpgradeContext>();
    ErrorCode error = storeTx.ReadExact(*upgradeContext);

    if (error.IsError(ErrorCodeValue::NotFound) || (error.IsSuccess() && upgradeContext->IsComplete))
    {
        WriteInfo(
            TraceComponent,
            "{0} Fabric not upgrading: {1}",
            this->TraceId,
            verifiedDomains);

        return RejectRequest(move(clientRequest), ErrorCodeValue::FabricNotUpgrading, callback, root);
    }
    else if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }
    else if (!upgradeContext->UpgradeDescription.IsUnmonitoredManual)
    {
        error = this->TraceAndGetErrorDetails( ErrorCodeValue::InvalidArgument, wformatString("{0} {1}", GET_RC( Invalid_Upgrade_Mode ),
                upgradeContext->UpgradeDescription.RollingUpgradeMode));

        return RejectRequest(move(clientRequest), move(error), callback, root);
    }
    // Allow 0 instances for internal MoveNext
    else if (upgradeInstance > 0 && upgradeInstance != upgradeContext->RollforwardInstance)
    {
        error = this->TraceAndGetErrorDetails( ErrorCodeValue::InvalidMessage, wformatString("{0} ({1}, {2})", GET_RC( Invalid_Upgrade_Instance ),
                upgradeInstance,
                upgradeContext->RollforwardInstance));

        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    vector<wstring> const & completedDomains = upgradeContext->CompletedUpgradeDomains;

    // Filter out upgrade domain (UD) names that haven't actually completed yet.
    // This prevents the FM from progressing through UDs too soon
    // since the FM assumes an ordering on UD names and only does a simple
    // inequality comparison between the current pending UD name and the last
    // verified UD name being reported.
    //
    verifiedDomains.erase(
            remove_if(verifiedDomains.begin(), verifiedDomains.end(), [&completedDomains](wstring const & item)
                {
                    return (find(completedDomains.begin(), completedDomains.end(), item) == completedDomains.end());
                }),
            verifiedDomains.end());

    WriteInfo(
        TraceComponent,
        "{0} reporting verified Fabric upgrade domains: {1}",
        this->TraceId,
        verifiedDomains);

    auto verifiedDomainsStoreData = make_unique<StoreDataVerifiedFabricUpgradeDomains>(move(verifiedDomains));

    error = storeTx.UpdateOrInsertIfNotFound(*verifiedDomainsStoreData);

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    return FinishAcceptRequest(
        move(storeTx),
        move(upgradeContext),
        move(error),
        true,   // shouldCommit
        timeout,
        callback,
        root);
}

AsyncOperationSPtr ClusterManagerReplica::BeginAcceptMoveNextFabricUpgradeDomain(
    wstring const & nextUpgradeDomain,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    FabricUpgradeStatusDescription upgradeStatus;

    ErrorCode error = this->GetFabricUpgradeStatus(clientRequest, upgradeStatus);
    if (!error.IsSuccess()) {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    auto const & completedDomains = upgradeStatus.CompletedUpgradeDomains;

    // We expect the first pending upgrade domain to be processed next by the FM.
    // When this contract changes, the logic here needs to be updated and the protocol
    // between the CM and FM should be updated to make the next domain explicit.
    //
    if (upgradeStatus.NextUpgradeDomain.empty())
    {
        WriteInfo(
            TraceComponent,
            "{0} no pending UD for move next: state = {1}",
            this->TraceId,
            upgradeStatus.UpgradeState);

        return RejectRequest(move(clientRequest), ErrorCodeValue::FabricNotUpgrading, callback, root);
    }
    else if (!completedDomains.empty() && find(completedDomains.begin(), completedDomains.end(), nextUpgradeDomain) != completedDomains.end())
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::UpgradeDomainAlreadyCompleted, callback, root);
    }
    else if (!upgradeStatus.InProgressUpgradeDomain.empty() || nextUpgradeDomain != upgradeStatus.NextUpgradeDomain)
    {
        error = this->TraceAndGetErrorDetails( ErrorCodeValue::InvalidArgument, wformatString(GET_RC( Invalid_Upgrade_Domain ),
                nextUpgradeDomain,
                upgradeStatus.NextUpgradeDomain,
                upgradeStatus.InProgressUpgradeDomain));

        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    return this->BeginAcceptReportFabricUpgradeHealth(
        move(const_cast<vector<wstring>&>(upgradeStatus.CompletedUpgradeDomains)),
        0,
        move(clientRequest),
        timeout,
        callback,
        root);
}

AsyncOperationSPtr ClusterManagerReplica::BeginAcceptUnprovisionFabric(
    FabricVersion const & unprovisionVersion,
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    if (!this->TryAcceptClusterOperationRequest(clientRequest))
    {
        return RejectRequest(move(clientRequest), ErrorCodeValue::Success, callback, root);
    }

    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, clientRequest->ActivityId);

    auto upgradeContext = make_unique<FabricUpgradeContext>();
    ErrorCode error = storeTx.ReadExact(*upgradeContext);

    if (error.IsError(ErrorCodeValue::NotFound))
    {
        error = ErrorCodeValue::Success;
    }
    else if (error.IsSuccess())
    {
        FabricCodeVersion const & codeVersion = unprovisionVersion.CodeVersion;
        if (codeVersion.IsValid)
        {
            if (codeVersion == upgradeContext->CurrentVersion.CodeVersion ||
                (!upgradeContext->IsComplete && codeVersion == upgradeContext->UpgradeDescription.Version.CodeVersion))
            {
                WriteWarning(
                    TraceComponent,
                    "{0} cannot unprovision Fabric code version {1}: current = {2} target = {3} upgrade = {4}",
                    this->TraceId,
                    codeVersion,
                    upgradeContext->CurrentVersion,
                    upgradeContext->UpgradeDescription.Version,
                    upgradeContext->UpgradeState);

                error = ErrorCodeValue::FabricVersionInUse;
            }
        }

        auto configVersion = unprovisionVersion.ConfigVersion;
        if (configVersion.IsValid)
        {
            if (configVersion == upgradeContext->CurrentVersion.ConfigVersion ||
                (!upgradeContext->IsComplete && configVersion == upgradeContext->UpgradeDescription.Version.ConfigVersion))
            {
                WriteWarning(
                    TraceComponent,
                    "{0} cannot unprovision Fabric config version {1}: current = {2} target = {3} upgrade = {4}",
                    this->TraceId,
                    configVersion,
                    upgradeContext->CurrentVersion,
                    upgradeContext->UpgradeDescription.Version,
                    upgradeContext->UpgradeState);

                error.Overwrite(ErrorCodeValue::FabricVersionInUse);
            }
        }
    }

    if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }

    auto context = make_unique<FabricProvisionContext>();
    bool containsVersion(false);
    error = this->GetProvisionedFabricContext(storeTx, unprovisionVersion, *context, containsVersion);

    bool shouldCommit(true);
    if (error.IsError(ErrorCodeValue::CMRequestAlreadyProcessing))
    {
        error = this->UpdateOperationTimeout(storeTx, *context, clientRequest->Timeout, shouldCommit);
    }
    else if (!error.IsSuccess())
    {
        return RejectRequest(move(clientRequest), move(error), callback, root);
    }
    else
    {
        if (containsVersion)
        {
            error = context->StartUnprovisioning(
                storeTx,
                *this,
                clientRequest,
                unprovisionVersion);
        }
        else
        {
            return RejectRequest(move(clientRequest), ErrorCodeValue::FabricVersionNotFound, callback, root);
        }
    }

    return FinishAcceptRequest(
        move(storeTx),
        move(context),
        move(error),
        shouldCommit,
        timeout,
        callback,
        root);
}

ErrorCode ClusterManagerReplica::EnqueueRolloutContext(shared_ptr<RolloutContext> && context)
{
    return rolloutManagerUPtr_->Enqueue(std::move(context));
}

ErrorCode ClusterManagerReplica::ProcessServiceGroupFromTemplate(
    StoreDataServiceTemplate & storeDataServiceTemplate, NamingUri const & absoluteServiceName, vector<byte> const & initializationData)
{
    PartitionedServiceDescriptor & partitionedServiceDescriptor = storeDataServiceTemplate.MutablePartitionedServiceDescriptor;

    // Deserialize into original ServiceGroup object
    vector<byte> & serializedInitializationData = partitionedServiceDescriptor.get_MutableService().get_MutableInitializationData();
    CServiceGroupDescription cServiceGroupDescription;
    ErrorCode errorCode = Common::FabricSerializer::Deserialize(cServiceGroupDescription, serializedInitializationData);
    if (!errorCode.IsSuccess())
    {
        WriteError(
            TraceComponent,
            "Error while deserializing initialization data into service group object");

        return errorCode;
    }

    cServiceGroupDescription.ServiceGroupInitializationData.insert(
        cServiceGroupDescription.ServiceGroupInitializationData.begin(),
        initializationData.begin(),
        initializationData.begin() + initializationData.size());

    for (int i = 0; i < cServiceGroupDescription.ServiceGroupMemberData.size(); i++)
    {
        CServiceGroupMemberDescription & cServiceGroupMemberDescription = cServiceGroupDescription.ServiceGroupMemberData[i];

        NamingUri fullServiceName;
        absoluteServiceName.TryCombine(cServiceGroupMemberDescription.ServiceName, fullServiceName);
        cServiceGroupMemberDescription.ServiceName = fullServiceName.ToString();
    }

    // Serialize into bytes
    serializedInitializationData.clear();
    errorCode = Common::FabricSerializer::Serialize(&cServiceGroupDescription, serializedInitializationData);
    if (!errorCode.IsSuccess())
    {
        WriteError(
            TraceComponent,
            "Error while serializing service group object into initialization data");

        return errorCode;
    }

    return errorCode;
}

//
// KeyValueStoreReplica
//

ErrorCode ClusterManagerReplica::OnOpen(ComPointer<IFabricStatefulServicePartition> const & servicePartition)
{
    UNREFERENCED_PARAMETER(servicePartition);

    auto error = imageBuilder_.VerifyImageBuilderSetup();

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0} OnOpen(): node instance = {1} trace ID = {2}: failed verification of ImageBuilder setup: error = {3}",
            static_cast<void*>(this),
            this->NodeInstance,
            this->TraceId,
            error);

        return error;
    }

    stringRequestTrackerUPtr_ = make_unique<StringRequestInstanceTracker>(this->Root, L"String", this->PartitionedReplicaId);
    nameRequestTrackerUPtr_ = make_unique<NameRequestInstanceTracker>(this->Root, L"Name", this->PartitionedReplicaId);

    // Cache service location since it will not change after open
    //
    serviceLocation_ = SystemServiceLocation(
        this->NodeInstance,
        this->PartitionId,
        this->ReplicaId,
        DateTime::Now().Ticks);

    messageFilterSPtr_ = make_shared<SystemServiceMessageFilter>(serviceLocation_);

    CMEvents::Trace->ReplicaOpen(
        this->PartitionedReplicaId,
        this->NodeInstance,
        serviceLocation_);

    namingJobQueue_ = make_unique<AsyncOperationWorkJobQueue>(
        L"CM.Naming.",
        this->TraceId,
        this->CreateComponentRoot(),
        make_unique<NamingJobQueueConfigSettingsHolder>());

    error = clientProxy_.Initialize();

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0} failed to initialize client proxy: {1}",
            this->TraceId,
            error);

        return error;
    }

    InitializeMessageHandler();

    OpenReplicaCallback callback = OnOpenReplicaCallback;
    if (callback)
    {
        auto root = shared_from_this();
        auto cmServiceSPtr = dynamic_pointer_cast<ClusterManagerReplica>(root);
        ASSERT_IF(!cmServiceSPtr, "{0} failed to cast CM replica", this->TraceId);
        callback(cmServiceSPtr);
    }

    this->InitializeCachedFilePtrs();
    return error;
}

ErrorCode ClusterManagerReplica::OpenHealthManagerReplica()
{
    auto healthManagerReplica = make_shared<HealthManagerReplica>(
        *this,
        this->PartitionId,
        this->ReplicaId,
        federation_.Federation,
        *this);

    { // lock
        AcquireWriteLock lock(lock_);
        ASSERT_IF(healthManagerReplica_, "{0}: OpenHealthManagerReplica: Health manager shouldn't exist", this->TraceId);
        healthManagerReplica_ = healthManagerReplica;
    } // endlock

    auto error = healthManagerReplica->OnOpen(
        this->ReplicatedStore,
        messageFilterSPtr_);

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0}: node instance = {1}: failed to open HM, error = {2}",
            this->TraceId,
            this->NodeInstance,
            error);

        return error;
    }

    return ErrorCode::Success();
}

ErrorCode ClusterManagerReplica::CloseHealthManagerReplica()
{
    HealthManagerReplicaSPtr healthManagerReplica;
    { // lock
        AcquireWriteLock lock(lock_);
        healthManagerReplica = move(healthManagerReplica_);
    } // endlock

    if (!healthManagerReplica)
    {
        // Never created and opened, nothing to do
        return ErrorCode::Success();
    }

    auto error = healthManagerReplica->OnClose();
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0}: node instance = {1}: failed to close HM, error = {2}",
            this->TraceId,
            this->NodeInstance,
            error);

        healthManagerReplica->OnAbort();
    }

    this->TryStartLeakDetectionTimer();

    return ErrorCode::Success();
}

ErrorCode ClusterManagerReplica::OnChangeRole(::FABRIC_REPLICA_ROLE newRole, __out wstring & serviceLocation)
{
    CMEvents::Trace->ReplicaChangeRole(
        this->PartitionedReplicaId,
        ToWString(newRole));

    ErrorCode error(ErrorCodeValue::Success);
    if (newRole == ::FABRIC_REPLICA_ROLE_PRIMARY)
    {
        error = OpenHealthManagerReplica();
        if (!error.IsSuccess())
        {
            return error;
        }

        imageBuilder_.Enable();

        if (!applicationTypeTrackerUPtr_)
        {
            applicationTypeTrackerUPtr_ = make_unique<ApplicationTypeRequestTracker>(this->PartitionedReplicaId);
        }

        // Initialize RolloutManager only if we know Open() succeeded since
        // it keeps ClusterManagerReplica alive and expects a Close() call to
        // break this loop
        //
        if (!rolloutManagerUPtr_)
        {
            rolloutManagerUPtr_ = make_unique<RolloutManager>(*this);
        }

        error = rolloutManagerUPtr_->Start();
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0} OnChangeRole(): node instance = {1} trace ID = {2}: start rollout manager failed, error = {3}",
                static_cast<void*>(this),
                this->NodeInstance,
                this->TraceId,
                error);

            return error;
        }

        if (cleanupAppTypejobQueue_)
        {
            auto cleanupAppTypejobQueue = move(cleanupAppTypejobQueue_);
            cleanupAppTypejobQueue->Close();
        }

        cleanupAppTypejobQueue_ = make_unique<AppTypeCleanupJobQueue<ClusterManagerReplica>>(
            L"CM.AppTypeCleanupQueue",
            *this,
            false /* forceEnqueue*/,
            ManagementConfig::GetConfig().NamingJobQueueThreadCount,
            ManagementConfig::GetConfig().NamingJobQueueSize);

        // Automatic unprovision check
        {
            AcquireWriteLock lock(cleanupApplicationTypeTimerLock_);
            cleanupApplicationTypeTimer_ = Timer::Create(
                CleanupAppTypeTimerTag,
                [this](TimerSPtr const &)
            {
                if (ManagementConfig::GetConfig().PeriodicCleanupUnusedApplicationTypes)
                {
                    AcquireWriteLock callbackLock(callbackLock_);
                    CheckAndDeleteUnusedApplicationTypes();
                }
            },
                false);

            this->cleanupApplicationTypeTimer_->Change(ManagementConfig::GetConfig().InitialPeriodicAppTypeCleanupInterval, ManagementConfig::GetConfig().PeriodicAppTypeCleanupInterval);
        }
    }
    else
    {
        if (rolloutManagerUPtr_)
        {
            rolloutManagerUPtr_->Stop();
        }

        if (applicationTypeTrackerUPtr_)
        {
            applicationTypeTrackerUPtr_->Clear();
        }

        CloseAutomaticCleanupApplicationType();

        imageBuilder_.Disable();

        error = CloseHealthManagerReplica();
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    serviceLocation = serviceLocation_.Location;

    auto callback = this->OnChangeRoleCallback;
    if (callback)
    {
        auto root = shared_from_this();
        auto cmServiceSPtr = dynamic_pointer_cast<ClusterManagerReplica>(root);
        ASSERT_IF(!cmServiceSPtr, "{0} failed to cast CM replica", this->TraceId);
        callback(serviceLocation_, cmServiceSPtr);
    }

    return error;
}

ErrorCode ClusterManagerReplica::OnClose()
{
    CMEvents::Trace->ReplicaClose(this->PartitionedReplicaId);

    bool success = federation_.Federation.UnRegisterMessageHandler(Transport::Actor::CM, messageFilterSPtr_);

    if (queryMessageHandler_)
    {
        queryMessageHandler_->Close();
    }

    if (!success)
    {
        WriteInfo(
            TraceComponent,
            "{0} unable to unregister message handler: actor = '{1}'",
            this->TraceId,
            serviceLocation_);

        // Best-effort close, so proceed instead of returning error
    }

    auto error = CloseHealthManagerReplica();

    if (rolloutManagerUPtr_)
    {
        rolloutManagerUPtr_->Close();
    }

    if (applicationTypeTrackerUPtr_)
    {
        applicationTypeTrackerUPtr_->Clear();
    }

    if (stringRequestTrackerUPtr_)
    {
        stringRequestTrackerUPtr_->Stop();
    }

    if (nameRequestTrackerUPtr_)
    {
        nameRequestTrackerUPtr_->Stop();
    }

    imageBuilder_.Disable();

    if (namingJobQueue_)
    {
        namingJobQueue_->Close();
    }

    CloseAutomaticCleanupApplicationType();

    auto callback = this->OnCloseReplicaCallback;
    if (callback)
    {
        callback(serviceLocation_);
    }

    fabricRoot_.reset();

    return error;
}

void ClusterManagerReplica::OnAbort()
{
    CMEvents::Trace->ReplicaAbort(this->PartitionedReplicaId);

    OnClose();
}

// *******
// private
// *******

GlobalWString ClusterManagerReplica::RejectReasonMissingActivityHeader = make_global<wstring>(L"Missing Activity Header");
GlobalWString ClusterManagerReplica::RejectReasonMissingTimeoutHeader = make_global<wstring>(L"Missing Timeout Header");
GlobalWString ClusterManagerReplica::RejectReasonMissingGatewayRetryHeader = make_global<wstring>(L"Missing Gateway Retry Header");
GlobalWString ClusterManagerReplica::RejectReasonIncorrectActor = make_global<wstring>(L"Incorrect Actor");
GlobalWString ClusterManagerReplica::RejectReasonInvalidAction = make_global<wstring>(L"Invalid Action");

void ClusterManagerReplica::Initialize()
{
    if (ManagementConfig::GetConfig().TestComposeDeploymentTestMode)
    {
        this->dockerComposeAppTypeVersionGenerator_ = make_shared<TestAppTypeNameVersionGenerator>();
    }
    else
    {
        this->dockerComposeAppTypeVersionGenerator_ = make_shared<DockerComposeAppTypeNameVersionGenerator>();
        this->containerGroupAppTypeVersionGenerator_ = make_shared<ContainerGroupAppTypeNameVersionGenerator>();
    }

    this->InitializeRequestHandlers();
}

void ClusterManagerReplica::InitializeRequestHandlers()
{
    map<wstring, ProcessRequestHandler> t;

    this->AddHandler(t, ClusterManagerTcpMessage::ProvisionApplicationTypeAction, CreateHandler<ProvisionApplicationTypeAsyncOperation>);
    this->AddHandler(t, ClusterManagerTcpMessage::CreateApplicationAction, CreateHandler<CreateApplicationAsyncOperation>);
    this->AddHandler(t, ClusterManagerTcpMessage::UpdateApplicationAction, CreateHandler<UpdateApplicationAsyncOperation>);
    this->AddHandler(t, ClusterManagerTcpMessage::UpgradeApplicationAction, CreateHandler<UpgradeApplicationAsyncOperation>);
    this->AddHandler(t, ClusterManagerTcpMessage::UpdateApplicationUpgradeAction, CreateHandler<UpdateApplicationUpgradeAsyncOperation>);
    this->AddHandler(t, ClusterManagerTcpMessage::RollbackApplicationUpgradeAction, CreateHandler<RollbackApplicationUpgradeAsyncOperation>);
    this->AddHandler(t, ClusterManagerTcpMessage::CreateServiceAction, CreateHandler<CreateServiceAsyncOperation>);
    this->AddHandler(t, ClusterManagerTcpMessage::CreateServiceFromTemplateAction, CreateHandler<CreateServiceFromTemplateAsyncOperation>);
    this->AddHandler(t, ClusterManagerTcpMessage::UnprovisionApplicationTypeAction, CreateHandler<UnprovisionApplicationTypeAsyncOperation>);
    // For backward compatibility
    this->AddHandler(t, ClusterManagerTcpMessage::DeleteApplicationAction, CreateHandler<DeleteApplicationAsyncOperation>);
    this->AddHandler(t, ClusterManagerTcpMessage::DeleteApplicationAction2, CreateHandler<DeleteApplication2AsyncOperation>);
    this->AddHandler(t, ClusterManagerTcpMessage::DeleteServiceAction, CreateHandler<DeleteServiceAsyncOperation>);
    this->AddHandler(t, ClusterManagerTcpMessage::GetUpgradeStatusAction, CreateHandler<GetUpgradeStatusAsyncOperation>);
    this->AddHandler(t, ClusterManagerTcpMessage::ReportUpgradeHealthAction, CreateHandler<ReportUpgradeHealthAsyncOperation>);
    this->AddHandler(t, ClusterManagerTcpMessage::MoveNextUpgradeDomainAction, CreateHandler<MoveNextUpgradeDomainAsyncOperation>);
    this->AddHandler(t, ClusterManagerTcpMessage::ProvisionFabricAction, CreateHandler<ProvisionFabricAsyncOperation>);
    this->AddHandler(t, ClusterManagerTcpMessage::UpgradeFabricAction, CreateHandler<UpgradeFabricAsyncOperation>);
    this->AddHandler(t, ClusterManagerTcpMessage::UpdateFabricUpgradeAction, CreateHandler<UpdateFabricUpgradeAsyncOperation>);
    this->AddHandler(t, ClusterManagerTcpMessage::RollbackFabricUpgradeAction, CreateHandler<RollbackFabricUpgradeAsyncOperation>);
    this->AddHandler(t, ClusterManagerTcpMessage::GetFabricUpgradeStatusAction, CreateHandler<GetFabricUpgradeStatusAsyncOperation>);
    this->AddHandler(t, ClusterManagerTcpMessage::ReportFabricUpgradeHealthAction, CreateHandler<ReportFabricUpgradeHealthAsyncOperation>);
    this->AddHandler(t, ClusterManagerTcpMessage::MoveNextFabricUpgradeDomainAction, CreateHandler<MoveNextFabricUpgradeDomainAsyncOperation>);
    this->AddHandler(t, ClusterManagerTcpMessage::UnprovisionFabricAction, CreateHandler<UnprovisionFabricAsyncOperation>);
    this->AddHandler(t, ClusterManagerTcpMessage::StartInfrastructureTaskAction, CreateHandler<StartInfrastructureTaskAsyncOperation>);
    this->AddHandler(t, ClusterManagerTcpMessage::FinishInfrastructureTaskAction, CreateHandler<FinishInfrastructureTaskAsyncOperation>);
    this->AddHandler(t, ContainerOperationTcpMessage::CreateComposeDeploymentAction, CreateHandler<CreateComposeDeploymentAsyncOperation>);
    this->AddHandler(t, ContainerOperationTcpMessage::DeleteComposeDeploymentAction, CreateHandler<DeleteComposeDeploymentAsyncOperation>);
    this->AddHandler(t, ContainerOperationTcpMessage::UpgradeComposeDeploymentAction, CreateHandler<UpgradeComposeDeploymentAsyncOperation>);
    this->AddHandler(t, ContainerOperationTcpMessage::RollbackComposeDeploymentUpgradeAction, CreateHandler<RollbackComposeDeploymentAsyncOperation>);
    this->AddHandler(t, ContainerOperationTcpMessage::DeleteSingleInstanceDeploymentAction, CreateHandler<DeleteSingleInstanceDeploymentAsyncOperation>);
    this->AddHandler(t, ClusterManagerTcpMessage::CreateApplicationResourceAction, CreateHandler<CreateSingleInstanceApplicationAsyncOperation>);
    this->AddHandler(t, VolumeOperationTcpMessage::CreateVolumeAction, CreateHandler<CreateVolumeAsyncOperation>);
    this->AddHandler(t, VolumeOperationTcpMessage::DeleteVolumeAction, CreateHandler<DeleteVolumeAsyncOperation>);

    requestHandlers_.swap(t);
}

void ClusterManagerReplica::AddHandler(map<wstring, ProcessRequestHandler> & temp, wstring const & action, ProcessRequestHandler const & handler)
{
    temp.insert(make_pair(action, handler));
}

template <class TAsyncOperation>
AsyncOperationSPtr ClusterManagerReplica::CreateHandler(
    __in ClusterManagerReplica & replica,
    __in MessageUPtr & request,
    __in RequestReceiverContextUPtr & requestContext,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<TAsyncOperation>(
        replica,
        move(request),
        move(requestContext),
        timeout,
        callback,
        parent);
}

bool ClusterManagerReplica::ValidateClientMessage(__in Transport::MessageUPtr & message, __out wstring & rejectReason)
{
    bool success = true;
    {
        FabricActivityHeader header;
        success = message->Headers.TryReadFirst(header);

        if (!success)
        {
            rejectReason = *RejectReasonMissingActivityHeader;
        }
    }

    if (success)
    {
        TimeoutHeader header;
        success = message->Headers.TryReadFirst(header);

        if (!success)
        {
            rejectReason = *RejectReasonMissingTimeoutHeader;
        }
    }

    if (success)
    {
        GatewayRetryHeader header;
        success = message->Headers.TryReadFirst(header);

        if (!success)
        {
            rejectReason = *RejectReasonMissingGatewayRetryHeader;
        }
    }

    if (success)
    {
        success = (message->Actor == Transport::Actor::CM);

        if (!success)
        {
            StringWriter(rejectReason).Write("{0}: {1}", *RejectReasonIncorrectActor, message->Actor);
        }
    }

    return success;
}

void ClusterManagerReplica::InitializeMessageHandler()
{
    // The lifetime of this replica is managed by the runtime (FabricRuntime)
    // and not the fabric root (FabricNode)
    //
    auto selfRoot = this->CreateComponentRoot();

     // Create and register the query handlers
    queryMessageHandler_ = make_unique<QueryMessageHandler>(*selfRoot, QueryAddresses::CMAddressSegment);
    queryMessageHandler_->RegisterQueryHandler(
        [this](QueryNames::Enum queryName, QueryArgumentMap const & queryArgs, Common::ActivityId const & activityId, TimeSpan timeout, AsyncCallback const & callback, AsyncOperationSPtr const & parent)
        {
            return this->BeginProcessQuery(queryName, queryArgs, activityId, timeout, callback, parent);
        },
        [this](Common::AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
        {
            return this->EndProcessQuery(operation, reply);
        });

    queryMessageHandler_->RegisterQueryForwardHandler(
            [this] (wstring const & childAddressSegment, wstring const & childAddressSegmentMetadata, MessageUPtr & message, TimeSpan timeout, AsyncCallback const & callback, AsyncOperationSPtr const & parent)
                { return BeginForwardMessage(childAddressSegment, childAddressSegmentMetadata, message, timeout, callback, parent); },
            [this](AsyncOperationSPtr const & asyncOperation, __out MessageUPtr & reply)
                { return EndForwardMessage(asyncOperation, reply); });

    auto error = queryMessageHandler_->Open();
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0} QueryMessageHandler failed to open with error {1}",
            this->TraceId,
            error);
    }

    auto filter = messageFilterSPtr_;

    federation_.Federation.RegisterMessageHandler(
        Transport::Actor::CM,
        [this](MessageUPtr & request, OneWayReceiverContextUPtr & requestContext)
        {
            WriteError(
                TraceComponent,
                "{0} received a oneway message: {1}",
                this->TraceId,
                *request);
            requestContext->Reject(ErrorCodeValue::InvalidMessage);
        },
        [this, selfRoot](MessageUPtr & message, RequestReceiverContextUPtr & requestReceiverContext)
        {
            this->RequestMessageHandler(move(message), move(requestReceiverContext));
        },
        false/*dispatchOnTransportThread*/,
        move(filter));
}

Common::AsyncOperationSPtr ClusterManagerReplica::BeginProcessQuery(
    Query::QueryNames::Enum queryName,
    QueryArgumentMap const & queryArgs,
    Common::ActivityId const & activityId,
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ProcessQueryAsyncOperation>(
        *this,
        queryName,
        queryArgs,
        activityId,
        timeout,
        callback,
        parent);
}

Common::ErrorCode ClusterManagerReplica::EndProcessQuery(
    Common::AsyncOperationSPtr const & operation,
    __out Transport::MessageUPtr & reply)
{
    return ProcessQueryAsyncOperation::End(operation, reply);
}


ErrorCode ClusterManagerReplica::ProcessQuery(
    QueryNames::Enum queryName,
    QueryArgumentMap const & queryArgs,
    ActivityId const & activityId,
    TimeSpan const timeout,
    __out MessageUPtr & reply)
{
    Store::ReplicaActivityId replicaActivityId(this->PartitionedReplicaId, activityId);

    QueryResult queryResult;
    switch (queryName)
    {
    case QueryNames::GetApplicationList:
        queryResult = GetApplications(queryArgs, replicaActivityId);
        break;
    case QueryNames::GetApplicationTypeList:
        queryResult = GetApplicationTypes(queryArgs, replicaActivityId);
        break;
    case QueryNames::GetApplicationTypePagedList:
        queryResult = GetApplicationTypesPaged(queryArgs, replicaActivityId);
        break;
    case QueryNames::GetApplicationServiceList:
        queryResult = GetServices(queryArgs, replicaActivityId);
        break;
    case QueryNames::GetComposeDeploymentStatusList:
        queryResult = GetComposeDeploymentStatus(queryArgs, replicaActivityId);
        break;
    case QueryNames::GetComposeDeploymentUpgradeProgress:
        queryResult = GetComposeDeploymentUpgradeProgress(queryArgs, replicaActivityId);
        break;
    case QueryNames::GetServiceTypeList:
        queryResult = GetServiceTypes(queryArgs, replicaActivityId);
        break;
    case QueryNames::GetServiceGroupMemberTypeList:
        queryResult = GetServiceGroupMemberTypes(queryArgs, replicaActivityId);
        break;
    case QueryNames::GetApplicationManifest:
        queryResult = GetApplicationManifest(queryArgs, replicaActivityId);
        break;
    case QueryNames::GetClusterManifest:
        queryResult = GetClusterManifest(queryArgs, replicaActivityId, timeout);
        break;
    case QueryNames::GetNodeList:
        queryResult = GetNodes(queryArgs, replicaActivityId);
        break;
    case QueryNames::GetServiceManifest:
        queryResult = GetServiceManifest(queryArgs, replicaActivityId);
        break;
    case QueryNames::GetInfrastructureTask:
        queryResult = GetInfrastructureTask(queryArgs, replicaActivityId);
        break;
    case QueryNames::GetProvisionedFabricCodeVersionList:
        queryResult = GetProvisionedFabricCodeVersions(queryArgs, replicaActivityId);
        break;
    case QueryNames::GetProvisionedFabricConfigVersionList:
        queryResult = GetProvisionedFabricConfigVersions(queryArgs, replicaActivityId);
        break;
    case QueryNames::GetDeletedApplicationsList:
        queryResult = GetDeletedApplicationsList(queryArgs, replicaActivityId);
        break;
    case QueryNames::GetProvisionedApplicationTypePackages:
        queryResult = GetProvisionedApplicationTypePackages(queryArgs, replicaActivityId);
        break;
    case QueryNames::GetApplicationResourceList:
        queryResult = GetApplicationResources(queryArgs, replicaActivityId);
        break;
    case QueryNames::GetServiceResourceList:
        queryResult = GetServiceResources(queryArgs, replicaActivityId);
        break;
    case QueryNames::GetVolumeResourceList:
        queryResult = GetVolumeResources(queryArgs, replicaActivityId);
        break;
    case QueryNames::GetClusterVersion:
        queryResult = GetClusterVersion();
        break;
    default:
        reply = nullptr;
        return ErrorCodeValue::InvalidConfiguration;
    }

    auto error = queryResult.QueryProcessingError;
    reply = ClusterManagerMessage::GetQueryReply(move(queryResult));
    return error;
}

AsyncOperationSPtr ClusterManagerReplica::BeginForwardMessage(
    wstring const & childAddressSegment,
    wstring const & childAddressSegmentMetadata,
    MessageUPtr & message,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    if (childAddressSegment != Query::QueryAddresses::HMAddressSegment)
    {
        unique_ptr<Store::ReplicaActivityId> replicaActivityId;
        FabricActivityHeader activityHeader;
        if (message && message->Headers.TryReadFirst(activityHeader))
        {
             replicaActivityId = make_unique<Store::ReplicaActivityId>(this->PartitionedReplicaId, activityHeader.ActivityId);
        }

        WriteWarning(
            TraceComponent,
            "{0}: Query message sent to CM with invalid forwarding segment {1} and segment metadata {2}",
            replicaActivityId ? replicaActivityId->TraceId : PartitionedReplicaId.TraceId,
            childAddressSegment,
            childAddressSegmentMetadata);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::InvalidAddress, callback, parent);
    }

    HealthManagerReplicaSPtr hm = GetHealthManagerReplica();
    if (!hm)
    {
        WriteWarning(
            TraceComponent,
            "{0}: Query message sent to CM with forwarding segment {1} and segment metadata {2}: HM not set",
            this->PartitionedReplicaId.TraceId,
            childAddressSegment,
            childAddressSegmentMetadata);
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::InvalidState, callback, parent);
    }

    return hm->QueryMessageHandlerObj->BeginProcessQueryMessage(*message, timeout, callback, parent);
}

ErrorCode ClusterManagerReplica::EndForwardMessage(
    AsyncOperationSPtr const & asyncOperation,
    __out MessageUPtr & reply)
{
    reply = nullptr;

    auto operation = dynamic_pointer_cast<CompletedAsyncOperation>(asyncOperation);
    if (operation)
    {
        return CompletedAsyncOperation::End(asyncOperation);
    }

    HealthManagerReplicaSPtr hm = GetHealthManagerReplica();
    if (!hm)
    {
        WriteWarning(TraceComponent, "{0}: EndForwardMessage: HM not set", this->TraceId);
        return ErrorCode(ErrorCodeValue::NotPrimary);
    }

    return hm->QueryMessageHandlerObj->EndProcessQueryMessage(asyncOperation, reply);
}

ErrorCode ClusterManagerReplica::GetApplicationContextForQuery(
    wstring const & applicationName,
    Store::ReplicaActivityId const & replicaActivityId,
    __out unique_ptr<ApplicationContext> & applicationContext)
{
    Common::NamingUri applicationNameUri;
    if (!Common::NamingUri::TryParse(applicationName, applicationNameUri))
    {
        WriteInfo(
            TraceComponent,
            "{0}: ApplicationName passed to the query has invalid format. Application Name = {1}",
            replicaActivityId.TraceId,
            applicationName);
        return ErrorCodeValue::InvalidNameUri;
    }

    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId);
    // Search for application name
    auto tempApplicationContext = make_unique<ApplicationContext>(applicationNameUri);
    auto error = this->GetApplicationContext(storeTx, *tempApplicationContext);
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0}: GetApplicationContext failed with error = {1}",
            replicaActivityId.TraceId,
            error);
        return error;
    }

    applicationContext = move(tempApplicationContext);
    storeTx.CommitReadOnly();
    return error;
}

ServiceModel::QueryResult ClusterManagerReplica::GetApplications(QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId)
{
    ApplicationQueryDescription queryDescription;
    auto error = queryDescription.GetDescriptionFromQueryArgumentMap(queryArgs);

    if (!error.IsSuccess())
    {
        return QueryResult(move(error));
    }

    ListPager<ApplicationQueryResult> applicationList;
    applicationList.SetMaxResults(queryDescription.PagingDescription->MaxResults);

    bool hasContinuationToken = !queryDescription.PagingDescription->ContinuationToken.empty();

    wstring appName(L"");
    queryArgs.TryGetValue(QueryResourceProperties::Application::ApplicationName, appName);

    // If applicationNameArg isn't empty, then we expect only one result. Or none.
    // Filter by application name
    if (!appName.empty())
    {
        if (hasContinuationToken && (appName <= queryDescription.PagingDescription->ContinuationToken))
        {
            // Application name doesn't respect continuation token
            return QueryResult(move(applicationList));
        }

        unique_ptr<ApplicationContext> applicationContextPtr;
        error = GetApplicationContextForQuery(appName, replicaActivityId, applicationContextPtr);
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: get application context for '{1}' failed: error = {2}",
                replicaActivityId.TraceId,
                appName,
                error);

            if (error.IsError(ErrorCodeValue::ApplicationNotFound) || error.IsError(ErrorCodeValue::NotFound))
            {
                return QueryResult(move(applicationList));
            }

            return QueryResult(move(error));
        }

        error = applicationList.TryAdd(applicationContextPtr->ToQueryResult(queryDescription.ExcludeApplicationParameters));
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: failed to add app {1} to the pager: error = {2}{3}",
                replicaActivityId.TraceId,
                applicationContextPtr->ApplicationName,
                error,
                error.Message);
            return QueryResult(move(error));
        }

        return QueryResult(move(applicationList));
    }

    // If there is no argument to filter the applicationName, get all application contexts.
    vector<ApplicationContext> applicationContexts;
    error = GetCompletedOrUpgradingApplicationContexts(applicationContexts, true); // isQuery
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} GetCompletedOrUpgradingApplicationContexts failed with error = {1}",
            replicaActivityId.TraceId,
            error);

        if(error.IsError(ErrorCodeValue::ApplicationNotFound) || error.IsError(ErrorCodeValue::NotFound))
        {
            return QueryResult(move(applicationList));
        }

        return ServiceModel::QueryResult(move(error));
    }

    // The store key for ApplicationContext is application name (as string), so the results are ordered by application name string.
    for (auto itApplicationContext = applicationContexts.begin(); itApplicationContext != applicationContexts.end(); ++itApplicationContext)
    {
        wstring applicationName = itApplicationContext->ApplicationName.ToString();
        if (hasContinuationToken && applicationName <= queryDescription.PagingDescription->ContinuationToken)
        {
            // Item doesn't respect continuation token
            continue;
        }

        if (!queryDescription.ApplicationTypeNameFilter.empty() && StringUtility::Compare(itApplicationContext->TypeName.Value, queryDescription.ApplicationTypeNameFilter) != 0)
        {
            // ApplicationTypeNameFilter does not match
            continue;
        }

        if (!ApplicationDefinitionKind::IsMatch(queryDescription.ApplicationDefinitionKindFilter, itApplicationContext->ApplicationDefinitionKind))
        {
            continue;
        }

        error = applicationList.TryAdd(itApplicationContext->ToQueryResult(queryDescription.ExcludeApplicationParameters));
        if (error.IsError(ErrorCodeValue::EntryTooLarge) || error.IsError(ErrorCodeValue::MaxResultsReached))
        {
            WriteInfo(
                TraceComponent,
                "{0}: TryAdd ApplicationQueryResult {1} returned non-fatal: {2} {3}.",
                replicaActivityId.TraceId,
                applicationName,
                error,
                error.Message);
            break;
        }
        else if (!error.IsSuccess()) // unexpected error
        {
            WriteInfo(
                TraceComponent,
                "{0}: failed to add application {1} to the pager: error = {2} {3}",
                replicaActivityId.TraceId,
                applicationName,
                error,
                error.Message);
            return QueryResult(move(error));
        }
    }

    return ServiceModel::QueryResult(move(applicationList));
}

ServiceModel::QueryResult ClusterManagerReplica::GetApplicationTypes(QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId)
{
    vector<pair<ApplicationTypeContext, shared_ptr<StoreDataApplicationManifest>>> appTypeData;

    ErrorCode error;
    wstring appTypeName;
    if (queryArgs.TryGetValue(QueryResourceProperties::ApplicationType::ApplicationTypeName, appTypeName))
    {
        // Query is for a specific application type name
        //
        ServiceModelVersion appTypeVersion;
        ApplicationTypeContext applicationTypeContext(ServiceModelTypeName(appTypeName), appTypeVersion);
        error = GetApplicationManifestContexts(applicationTypeContext.GetTypeNameKeyPrefix(), false, appTypeData);
    }
    else
    {
        error = GetApplicationManifestContexts(L"", false, appTypeData);
    }

    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0}: GetApplicationManifestContexts failed in GetApplicationTypes: {1}",
            replicaActivityId.TraceId,
            error);

        return QueryResult(move(error));
    }

    vector<ApplicationTypeQueryResult> resultList;
    for (auto const & it : appTypeData)
    {
        resultList.push_back(ApplicationTypeQueryResult(
            it.first.TypeName.Value,
            it.first.TypeVersion.Value,
            it.second.get() ? it.second->DefaultParameters : map<wstring, wstring>(),
            it.first.QueryStatus,
            it.first.QueryStatusDetails,
            ApplicationTypeDefinitionKind::ToPublicApi(it.first.ApplicationTypeDefinitionKind)));
    }

    return QueryResult(move(resultList));
}

ServiceModel::QueryResult ClusterManagerReplica::GetApplicationTypesPaged(QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId)
{
    // ---------------------------------------------------------------------------------------------
    // Do set up / initialization work
    // ---------------------------------------------------------------------------------------------

    // Used later to check for errors.
    ErrorCode error;

    wstring continuationToken;

    // Check if the continuation token should be used (check if it's passed in).
    bool checkContinuationToken = queryArgs.TryGetValue(QueryResourceProperties::QueryMetadata::ContinuationToken, continuationToken);

    wstring continationTokenAppType;
    wstring continationTokenAppVersion;
    if (checkContinuationToken)
    {
        ApplicationTypeQueryContinuationToken continuationTokenObj;
        error = Parser::ParseContinuationToken<ApplicationTypeQueryContinuationToken>(continuationToken, continuationTokenObj);

        // If the continuation token is badly formed, return error
        // We won't have an ArgumentNull error because if a continuationToken value is passed through
        // to here in queryArgs it's not null
        if (!error.IsSuccess())
        {
            error = this->TraceAndGetErrorDetails(
                ErrorCodeValue::InvalidArgument,
                wformatString(
                    GET_COMMON_RC(Invalid_Continuation_Token),
                    continuationToken),
                    L"Info");

            return QueryResult(error);
        }

        continuationTokenObj.TakeContinuationTokenComponents(continationTokenAppType, continationTokenAppVersion);
    }

    wstring appTypeNameOuter;
    bool checkAppTypeName = queryArgs.TryGetValue(QueryResourceProperties::ApplicationType::ApplicationTypeName, appTypeNameOuter);

    wstring appTypeVersionString;
    bool checkAppTypeVersion = queryArgs.TryGetValue(QueryResourceProperties::ApplicationType::ApplicationTypeVersion, appTypeVersionString);

    // If you have app type version without a name, throw an error
    if (checkAppTypeVersion && !checkAppTypeName)
    {
        error = this->TraceAndGetErrorDetails(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                "{0} {1} {2}.",
                GET_CM_RC(Version_Given_Without_Type),
                L"GetApplicationTypesPaged",
                appTypeVersionString),
            L"Info");

        return QueryResult(error);
    }

    wstring applicationTypeDefinitionKindFilterArg;
    DWORD applicationTypeDefinitionKindFilter = static_cast<DWORD>(FABRIC_APPLICATION_TYPE_DEFINITION_KIND_FILTER_DEFAULT);
    bool checkDefinitionKindFilter = false;

    if (queryArgs.TryGetValue(QueryResourceProperties::Deployment::ApplicationTypeDefinitionKindFilter, applicationTypeDefinitionKindFilterArg))
    {
        if (!StringUtility::TryFromWString<DWORD>(applicationTypeDefinitionKindFilterArg, applicationTypeDefinitionKindFilter))
        {
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                GET_RC(Invalid_Application_Type_Definition_Kind_Filter));
        }
        else
        {
            checkDefinitionKindFilter = (applicationTypeDefinitionKindFilter != FABRIC_APPLICATION_TYPE_DEFINITION_KIND_FILTER_DEFAULT);
        }
    }

    // Filters are exclusive
    bool hasFilterSet = false;
    if (!IsExclusiveFilterHelper(checkAppTypeName, hasFilterSet)
        || !IsExclusiveFilterHelper(checkDefinitionKindFilter, hasFilterSet))
    {
        WriteInfo(
            TraceComponent,
            "At most one of ApplicationTypeName {0} or ApplicationTypeDefinitionKind {1} can be specified in GetApplicationTypes.",
            appTypeNameOuter,
            applicationTypeDefinitionKindFilterArg);

        return QueryResult(
            ErrorCode(
                ErrorCodeValue::InvalidArgument,
                GET_RC(Application_Type_Filter_Dup_Defined)));
    }

    bool excludeApplicationParameters = false;
    wstring excludeApplicationParametersString;
    bool checkExcludeApplicationParameters = queryArgs.TryGetValue(QueryResourceProperties::QueryMetadata::ExcludeApplicationParameters, excludeApplicationParametersString);

    if (checkExcludeApplicationParameters)
    {
        // returns a bool for success/failure
        if (!StringUtility::TryFromWString<bool>(excludeApplicationParametersString, excludeApplicationParameters))
        {
            // If parsing fails, return error
            WriteInfo(
                TraceComponent,
                "{0}: GetApplicationTypesPaged failed because the provided value for ExcludeApplicationParameters of \"{1}\" is invalid or cannot be parsed.",
                replicaActivityId.TraceId,
                excludeApplicationParametersString);

            return QueryResult(ErrorCode(
                ErrorCodeValue::InvalidArgument,
                GET_COMMON_RC(Invalid_String_Boolean_Value)
            ));
        }
    }

    // sets the default value of maxResults
    int64 maxResults = ServiceModel::QueryPagingDescription::MaxResultsDefaultValue;
    wstring maxResultsString;

    // See if maxResults exists at all in queryArgs. If it does this function puts the value in maxResultsString and returns true.
    // If not, it returns false.
    // checkMaxResults should only be false if maxResults is not provided (which FabricClientImpl won't if it's the default value of 0)
    bool checkMaxResults = queryArgs.TryGetValue(QueryResourceProperties::QueryMetadata::MaxResults, maxResultsString);

    // Try to parse maxResults
    if (checkMaxResults)
    {
        auto int64Parser = StringUtility::ParseIntegralString<int64, true>();

        if (!int64Parser.Try(maxResultsString, maxResults, 0) || maxResults <= 0)
        {
            // If parsing fails, return error
            WriteInfo(
                TraceComponent,
                "{0}: GetApplicationTypesPaged failed because the provided value for MaxResults of \"{1}\" is invalid or cannot be parsed.",
                replicaActivityId.TraceId,
                maxResultsString);

            return QueryResult(ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(GET_COMMON_RC(Invalid_Max_Results), maxResultsString)));
        }
    }

    // Create list of store all contexts (all of the return results ...  as a pair of applicationType data and the manifest, contains the application parameters)
    vector<pair<ApplicationTypeContext, shared_ptr<StoreDataApplicationManifest>>> appTypeData;

    // Create list to store only the results we want to return
    ListPager<ApplicationTypeQueryResult> applicationTypeResultList;

    // When we call "TryAdd", it will make sure to respect the max results.
    applicationTypeResultList.SetMaxResults(maxResults);

    // If we are returning only one application type (if there is a appTypeName provided)
    // Get all the raw information we need
    // Filter for app type name
    if (checkAppTypeName)
    {
        ServiceModelVersion appTypeVersion;
        ApplicationTypeContext applicationTypeContext(ServiceModelTypeName(appTypeNameOuter), appTypeVersion);

        // This fills out the appTypeData variable (appType and the manifest)
        error = GetApplicationManifestContexts(applicationTypeContext.GetTypeNameKeyPrefix(), excludeApplicationParameters, appTypeData);

        // If there is an application type version filter provided, then select only the entries which match the version
        if (checkAppTypeVersion)
        {
            // Put into this array the matching versions
            // If no versions match, return empty array.
            // For linux: 'auto' not allowed in lambda parameter,
            // so we are doing pair<ApplicationTypeContext, shared_ptr<StoreDataApplicationManifest>> & item for the lambda
            auto valuesEnd = move(std::remove_if(
                appTypeData.begin(),
                appTypeData.end(),
                [appTypeVersionString](pair<ApplicationTypeContext, shared_ptr<StoreDataApplicationManifest>> & item)
                    { return item.first.TypeVersion.Value != appTypeVersionString; }));
            appTypeData.erase(move(valuesEnd), move(appTypeData.end()));
        }
    }
    else
    {
        error = GetApplicationManifestContexts(L"", excludeApplicationParameters, appTypeData);

        if (error.IsSuccess() && checkDefinitionKindFilter)
        {
            appTypeData.erase(
                remove_if(
                    appTypeData.begin(),
                    appTypeData.end(),
                    [applicationTypeDefinitionKindFilter](auto const & item) { return !ApplicationTypeDefinitionKind::IsMatch(applicationTypeDefinitionKindFilter, item.first.ApplicationTypeDefinitionKind); }),
                appTypeData.end());
        }
    }

    // Check for errors
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0}: GetApplicationManifestContexts failed in GetApplicationTypePaged query: {1}",
            replicaActivityId.TraceId,
            error);

        return QueryResult(move(error));
    }

    // ---------------------------------------------------------------------------------------------
    // Take all the gathered raw data and process it (with continuation token) and return it as a list
    // of QueryResult objects
    // The continuation token for cluster manager for this should be the app type name + app version
    // ---------------------------------------------------------------------------------------------

    // Add what we can to the results list. We assume that this list is already ordered
    // So that we don't just return randomly ordered items, making the next continuation token useless.
    for (auto const & it : appTypeData)
    {
        auto const & appTypeName = it.first.TypeName.Value;
        auto const & appTypeVersion = it.first.get_TypeVersion().Value;

        // Make sure that the result complies with the continuation token
        // value is 1 or more if str1 is less than str2 for str1.compare(str2)
        // We check here to see if based on the appTypeName, that the current results might violate the continuationToken
        if (checkContinuationToken)
        {
            int appTypeNameComparisonValue = appTypeName.compare(continationTokenAppType);

            // The appTypeName doesn't respect the token
            if (appTypeNameComparisonValue < 0)
            {
                continue;
            }

            // If the code reached here, it means that either the appTypeName doesn't respect the continuationToken
            // OR that it is the same
            if ((appTypeNameComparisonValue == 0) && appTypeVersion.compare(continationTokenAppVersion) <= 0)
            {
                // If the appTypeName is the same as the continuationToken, then we compare using the version number
                continue;
            }
        }

        // Add only if ListPager determines there is enough room
        error = applicationTypeResultList.TryAdd(ApplicationTypeQueryResult(
            appTypeName,
            it.first.TypeVersion.Value,
            it.second.get() ? it.second->DefaultParameters : map<wstring, wstring>(),
            it.first.QueryStatus,
            it.first.QueryStatusDetails,
            ApplicationTypeDefinitionKind::ToPublicApi(it.first.ApplicationTypeDefinitionKind)));

        // This should not be treated as an unexpected error. It just means we've hit the max number of items that can be returned
        if (error.IsError(ErrorCodeValue::EntryTooLarge) || error.IsError(ErrorCodeValue::MaxResultsReached))
        {
            WriteInfo(
                TraceComponent,
                "{0}: TryAdd ApplicationTypeQueryResult {1} returned non-fatal: {2} {3}.",
                replicaActivityId.TraceId,
                appTypeName,
                error,
                error.Message);
            break;
        }
        else if (!error.IsSuccess()) // unexpected error
        {
            WriteInfo(
                TraceComponent,
                "{0}: failed to add application type {1} to the pager: error = {2} {3}",
                replicaActivityId.TraceId,
                appTypeName,
                error,
                error.Message);
            return QueryResult(error);
        }
    }

    return QueryResult(move(applicationTypeResultList));
}

ErrorCode ClusterManagerReplica::GetComposeDeploymentContextForQuery(
    wstring const & deploymentName,
    Store::ReplicaActivityId const & replicaActivityId,
    __out unique_ptr<ComposeDeploymentContext> & composeDeploymentContext)
{
    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId);
    // Search for deployment name
    auto tempComposeDeploymentContext = make_unique<ComposeDeploymentContext>(deploymentName);
    auto error = this->GetComposeDeploymentContext(storeTx, *tempComposeDeploymentContext);
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0}: GetComposeDeploymentContext failed with error = {1}",
            replicaActivityId.TraceId,
            error);
        return error;
    }

    composeDeploymentContext = move(tempComposeDeploymentContext);
    storeTx.CommitReadOnly();
    return error;
}

QueryResult ClusterManagerReplica::GetApplicationResources(QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId)
{
    ApplicationQueryDescription queryDescription;
    auto error = queryDescription.GetDescriptionFromQueryArgumentMap(queryArgs);
    if (!error.IsSuccess())
    {
        return QueryResult(move(error));
    }

    // For the simplified model(modelV2), the application name is the unqualified name(without fabric:/). Application Uri is the term used to indicate the full fabric uri.
    // TODO: Consistency: Make the name to uri translation happen only on the server side. Client side should be passing the name without fabric:
    wstring appUri = queryDescription.GetApplicationNameString();
    ListPager<ModelV2::ApplicationDescriptionQueryResult> applicationResourceList;
    applicationResourceList.SetMaxResults(queryDescription.PagingDescription->MaxResults);

    bool hasContinuationToken = !queryDescription.PagingDescription->ContinuationToken.empty();

    wstring deploymentName(L"");
    if (!appUri.empty())
    {
       deploymentName = GetDeploymentNameFromAppName(appUri);
    }

    if (!deploymentName.empty())
    {
        if (hasContinuationToken && (deploymentName <= queryDescription.PagingDescription->ContinuationToken))
        {
            // Application name doesn't respect continuation token
            return QueryResult(move(applicationResourceList));
        }

        auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId);
        unique_ptr<SingleInstanceDeploymentContext> singleInstanceDeploymentContextPtr = make_unique<SingleInstanceDeploymentContext>(deploymentName);

        error = this->GetSingleInstanceDeploymentContext(storeTx, *singleInstanceDeploymentContextPtr);

        if (error.IsError(ErrorCodeValue::NotFound))
        {
            return QueryResult(move(applicationResourceList));
        }
        else if (!error.IsSuccess())
        {
            return QueryResult(move(error));
        }

        ModelV2::ApplicationDescriptionQueryResult queryResult;
        error = GetApplicationResourceQueryResult(storeTx, *singleInstanceDeploymentContextPtr, replicaActivityId, queryResult);
        storeTx.CommitReadOnly();

        if (!error.IsSuccess())
        {
            return QueryResult(move(error));
        }

        error = applicationResourceList.TryAdd(move(queryResult));
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: failed to add single instance deployment {1} to the pager: error = ({2} {3})",
                replicaActivityId.TraceId,
                deploymentName,
                error,
                error.Message);
            return QueryResult(move(error));
        }
        else
        {
            return QueryResult(move(applicationResourceList));
        }
    }
    else
    {
        // We are here if Application name not specified, so return all application resources.

        vector<SingleInstanceDeploymentContext> singleInstanceDeploymentContexts;
        error = ReadPrefix<SingleInstanceDeploymentContext>(Constants::StoreType_SingleInstanceDeploymentContext, singleInstanceDeploymentContexts);
        if (error.IsError(ErrorCodeValue::NotFound))
        {
            return QueryResult(move(applicationResourceList));
        }
        else if (!error.IsSuccess())
        {
            WriteInfo(
                    TraceComponent,
                    "{0} ReadPrefix failed with error: {1} while reading single instance contexts",
                    this->TraceId,
                    error);
            return QueryResult(move(error));
        }

        auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId);
        for (auto const & context : singleInstanceDeploymentContexts)
        {
            deploymentName = context.DeploymentName;
            if ((context.DeploymentType != DeploymentType::Enum::V2Application) ||
                (hasContinuationToken && (deploymentName <= queryDescription.PagingDescription->ContinuationToken)))
            {
                continue;
            }

            ModelV2::ApplicationDescriptionQueryResult queryResult;
            error = this->GetApplicationResourceQueryResult(storeTx, context, replicaActivityId, queryResult);
            if (!error.IsSuccess())
            {
                return QueryResult(move(error));
            }

            error = applicationResourceList.TryAdd(move(queryResult));
            if (error.IsError(ErrorCodeValue::EntryTooLarge))
            {
                WriteInfo(
                        TraceComponent,
                        "{0} TryAdd ApplicationResource result for {1} failed {2} {3}",
                        replicaActivityId.TraceId,
                        deploymentName,
                        error,
                        error.Message);
                break;
            }
        }
        storeTx.CommitReadOnly();

        return QueryResult(move(applicationResourceList));
    }
}

QueryResult ClusterManagerReplica::GetServiceResources(QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId)
{
    // TODO: consistency. in passing Uri or name from client - it should be either name or uri.
    wstring applicationNameUriString;
    if (!queryArgs.TryGetValue(QueryResourceProperties::Application::ApplicationName, applicationNameUriString))
    {
        return ServiceModel::QueryResult(ErrorCodeValue::InvalidArgument);
    }

    Common::NamingUri applicationNameUri;
    if (!Common::NamingUri::TryParse(applicationNameUriString, applicationNameUri))
    {
        WriteInfo(
            TraceComponent,
            "{0}: ApplicationName passed as query argument to GetServices is invalid. ApplicationName = {1}",
            replicaActivityId.TraceId,
            applicationNameUriString);
        return QueryResult(ErrorCodeValue::InvalidNameUri);
    }

    wstring serviceNameUri;
    queryArgs.TryGetValue(QueryResourceProperties::Service::ServiceName, serviceNameUri);

    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId);
    unique_ptr<SingleInstanceDeploymentContext> deploymentContextPtr = make_unique<SingleInstanceDeploymentContext>(GetDeploymentNameFromAppName(applicationNameUriString));

    auto error = this->GetSingleInstanceDeploymentContext(storeTx, *deploymentContextPtr);
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0}: error while reading deployment context for application {1}. ErrorCode = {2}",
            replicaActivityId.TraceId,
            applicationNameUriString,
            error);
        return QueryResult(error);
    }

    auto deploymentStoreData = make_shared<StoreDataSingleInstanceApplicationInstance>(
        deploymentContextPtr->DeploymentName,
        deploymentContextPtr->TypeVersion);

    error = storeTx.ReadExact(*deploymentStoreData);
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0}: Error while reading storedata for application resource {1} error = {2}",
            replicaActivityId.TraceId,
            deploymentContextPtr->ApplicationId,
            error);
        return QueryResult(error);
    }

    wstring continuationToken;
    bool checkContinuationToken = queryArgs.TryGetValue(QueryResourceProperties::QueryMetadata::ContinuationToken, continuationToken);

    ListPager<ModelV2::ContainerServiceQueryResult> serviceResourceList;
    if (!serviceNameUri.empty())
    {
        if (checkContinuationToken && serviceNameUri <= continuationToken)
        {
            // Doesn't respect the continuation token, nothing to do
            return QueryResult(move(serviceResourceList));
        }

        Common::NamingUri uri;
        FABRIC_QUERY_SERVICE_STATUS status = FABRIC_QUERY_SERVICE_STATUS_UNKNOWN;
        if (Common::NamingUri::TryParse(serviceNameUri, uri))
        {
            ServiceContext serviceContext(
                deploymentContextPtr->ApplicationId,
                applicationNameUri,
                uri);

            error = storeTx.ReadExact(serviceContext);
            if (error.IsSuccess())
            {
                status = serviceContext.GetQueryStatus();
            }
            else
            {
                // We wont be able to populate the service status because of this. This is not fatal.
                WriteInfo(
                    TraceComponent,
                    "{0} Unable to read service context for service {0} application {1}, error : {2}, {3}",
                    replicaActivityId.TraceId,
                    serviceNameUri,
                    applicationNameUriString,
                    error,
                    error.Message);

                error = ErrorCodeValue::Success;
            }
        }

        for (auto const& serviceDescription : deploymentStoreData->ApplicationDescription.Services)
        {
            if (StringUtility::Compare(serviceDescription.ServiceName, ServiceContext::GetRelativeServiceName(applicationNameUriString, serviceNameUri).Value) == 0)
            {
                serviceResourceList.TryAdd(move(ModelV2::ContainerServiceQueryResult(
                    serviceNameUri,
                    applicationNameUriString,
                    serviceDescription,
                    status)));

                break;
            }
        }
    }
    else
    {
        vector<ServiceContext> serviceContexts;
        error = ServiceContext::ReadApplicationServices(
            storeTx,
            deploymentContextPtr->ApplicationId,
            serviceContexts);
        if (!error.IsSuccess())
        {
            // We wont be able to populate the service status because of this. This is not fatal.
            WriteInfo(
                TraceComponent,
                "{0}: Error while reading App: {1} ServiceContexts, error = {2}",
                replicaActivityId.TraceId,
                applicationNameUriString,
                error);

            error = ErrorCodeValue::Success;
        }

        for (auto const& serviceDescription : deploymentStoreData->ApplicationDescription.Services)
        {
            serviceNameUri = wformatString("{0}/{1}", applicationNameUriString, serviceDescription.ServiceName);

            FABRIC_QUERY_SERVICE_STATUS status = FABRIC_QUERY_SERVICE_STATUS_UNKNOWN;
            for (auto const& serviceContext : serviceContexts)
            {
                if (StringUtility::Compare(serviceContext.ServiceName.Value, serviceDescription.ServiceName) == 0)
                {
                    status = serviceContext.GetQueryStatus();
                    break;
                }
            }

            error = serviceResourceList.TryAdd(move(ModelV2::ContainerServiceQueryResult(
                        serviceNameUri,
                        applicationNameUriString,
                        serviceDescription,
                        status)));
            if (error.IsError(ErrorCodeValue::EntryTooLarge))
            {
                WriteInfo(
                    TraceComponent,
                    "{0} TryAdd ServiceResource result for {1} failed {2} {3}",
                    replicaActivityId.TraceId,
                    applicationNameUriString,
                    error,
                    error.Message);
                break;
            }
        }
    }

    storeTx.CommitReadOnly();
    return QueryResult(move(serviceResourceList));
}

QueryResult ClusterManagerReplica::GetVolumeResources(QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId)
{
    VolumeQueryDescription queryDescription;
    auto error = queryDescription.GetDescriptionFromQueryArgumentMap(queryArgs);
    if (!error.IsSuccess())
    {
        return QueryResult(move(error));
    }

    wstring volumeName = queryDescription.VolumeNameFilter;
    ListPager<VolumeQueryResult> volumeResourceList;
    volumeResourceList.SetMaxResults(queryDescription.QueryPagingDescriptionUPtr->MaxResults);

    bool hasContinuationToken = !queryDescription.QueryPagingDescriptionUPtr->ContinuationToken.empty();
    if (!volumeName.empty())
    {
        if (hasContinuationToken && (volumeName <= queryDescription.QueryPagingDescriptionUPtr->ContinuationToken))
        {
            // Volume name doesn't respect continuation token
            return QueryResult(move(volumeResourceList));
        }

        auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId);
        StoreDataVolume storeDataVolume(volumeName);
        error = storeTx.ReadExact(storeDataVolume);
        if (error.IsError(ErrorCodeValue::NotFound))
        {
            WriteInfo(
                TraceComponent,
                "{0} Volume {1} does not exist",
                replicaActivityId.TraceId,
                volumeName);
            return QueryResult(move(volumeResourceList));
        }
        else if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0} failed to read StoreDataVolume {1}: ({2} {3})",
                replicaActivityId.TraceId,
                volumeName,
                error,
                error.Message);
            return QueryResult(move(error));
        }
        storeTx.CommitReadOnly();

        if (storeDataVolume.VolumeDescription != nullptr)
        {
            storeDataVolume.VolumeDescription->RemoveSensitiveInformation();
        }
        VolumeQueryResult queryResult(storeDataVolume.VolumeDescription);
        error = volumeResourceList.TryAdd(move(queryResult));
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0}: failed to add volume {1} to the pager: error = ({2} {3})",
                replicaActivityId.TraceId,
                volumeName,
                error,
                error.Message);
            return QueryResult(move(error));
        }
        else
        {
            return QueryResult(move(volumeResourceList));
        }
    }
    else
    {
        // We are here if volume name not specified, so return all volume resources.
        vector<StoreDataVolume> volumes;
        error = ReadPrefix<StoreDataVolume>(Constants::StoreType_Volume, volumes);
        if (error.IsError(ErrorCodeValue::NotFound))
        {
            WriteInfo(
                TraceComponent,
                "{0} No volumes found.",
                replicaActivityId.TraceId);
            return QueryResult(move(volumeResourceList));
        }
        else if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0} ReadPrefix failed while reading volumes. Error = ({1} {2})",
                replicaActivityId.TraceId,
                error,
                error.Message);
            return QueryResult(move(error));
        }

        for (auto const & volume : volumes)
        {
            if ((hasContinuationToken &&
                (volume.VolumeName <= queryDescription.QueryPagingDescriptionUPtr->ContinuationToken)))
            {
                continue;
            }
            if (volume.VolumeDescription != nullptr)
            {
                volume.VolumeDescription->RemoveSensitiveInformation();
            }
            VolumeQueryResult queryResult(volume.VolumeDescription);
            error = volumeResourceList.TryAdd(move(queryResult));
            if (error.IsError(ErrorCodeValue::EntryTooLarge) || error.IsError(ErrorCodeValue::MaxResultsReached))
            {
                WriteInfo(
                    TraceComponent,
                    "{0}: failed to add volume {1} to the pager: error = ({2} {3})",
                    replicaActivityId.TraceId,
                    volumeName,
                    error,
                    error.Message);
                break;
            }
            else if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceComponent,
                    "{0}: failed to add volume {1} to the pager: error = ({2} {3})",
                    replicaActivityId.TraceId,
                    volumeName,
                    error,
                    error.Message);
                return QueryResult(move(error));
            }
        }

        return QueryResult(move(volumeResourceList));
    }
}

ServiceModel::QueryResult ClusterManagerReplica::GetComposeDeploymentStatus(QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId)
{
    wstring deploymentNameArg(L"");
    queryArgs.TryGetValue(QueryResourceProperties::Deployment::DeploymentName, deploymentNameArg);

    if (deploymentNameArg.empty())
    {
        // For backward compatibility
        wstring applicationNameArg(L"");
        queryArgs.TryGetValue(QueryResourceProperties::Application::ApplicationName, applicationNameArg);
        deploymentNameArg = GetDeploymentNameFromAppName(applicationNameArg);
    }

    wstring continuationToken;
    bool checkContinuationToken = queryArgs.TryGetValue(QueryResourceProperties::QueryMetadata::ContinuationToken, continuationToken);

    int64 maxResults = ServiceModel::QueryPagingDescription::MaxResultsDefaultValue;
    wstring maxResultsString;
    if (queryArgs.TryGetValue(QueryResourceProperties::QueryMetadata::MaxResults, maxResultsString))
    {
        auto int64Parser = StringUtility::ParseIntegralString<int64, true>();
        if (!int64Parser.Try(maxResultsString, maxResults, 0) || maxResults <= 0)
        {
            WriteInfo(
                TraceComponent,
                "{0} GetComposeDeploymentStatus failed because MaxResults is invalid {1}",
                replicaActivityId.TraceId,
                maxResultsString);

            return QueryResult(ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(GET_COMMON_RC(Invalid_Max_Results), maxResultsString)));
        }
    }

    ListPager<ComposeDeploymentStatusQueryResult> composeDeploymentList;
    composeDeploymentList.SetMaxResults(maxResults);

    // Query by deployment name
    if (!deploymentNameArg.empty())
    {
        if (checkContinuationToken && deploymentNameArg <= continuationToken)
        {
            return QueryResult(move(composeDeploymentList));
        }

        unique_ptr<ComposeDeploymentContext> composeDeploymentContextPtr;

        ErrorCode error = GetComposeDeploymentContextForQuery(deploymentNameArg, replicaActivityId, composeDeploymentContextPtr);

        if (error.IsError(ErrorCodeValue::NotFound) || error.IsError(ErrorCodeValue::ComposeDeploymentNotFound))
        {
            return QueryResult(move(composeDeploymentList));
        }
        else if (!error.IsSuccess())
        {
            return QueryResult(error);
        }

        error = composeDeploymentList.TryAdd(composeDeploymentContextPtr->ToQueryResult());
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: failed to add compose deployment status {1} to the pager: error = {2}{3}",
                replicaActivityId.TraceId,
                composeDeploymentContextPtr->DeploymentName,
                error,
                error.Message);
            return QueryResult(move(error));
        }

        return QueryResult(move(composeDeploymentList));
    }

    vector<ComposeDeploymentContext> composeDeploymentContexts;
    ErrorCode error = GetComposeDeploymentContexts(composeDeploymentContexts);

    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} GetComposeDeploymentContexts failed with error = {1}",
            replicaActivityId.TraceId,
            error);

        if (error.IsError(ErrorCodeValue::NotFound))
        {
            return QueryResult(move(composeDeploymentList));
        }

        return QueryResult(move(error));
    }

    for (auto const & composeDeploymentContext : composeDeploymentContexts)
    {
        wstring deploymentName = composeDeploymentContext.DeploymentName;
        if (checkContinuationToken && deploymentName <= continuationToken)
        {
            // Item doesn't respect continuation token
            continue;
        }

        error = composeDeploymentList.TryAdd(composeDeploymentContext.ToQueryResult());
        if (error.IsError(ErrorCodeValue::EntryTooLarge))
        {
            WriteInfo(
                TraceComponent,
                "{0}: TryAdd ComposeDeploymentStatusQueryResult for {1} returned error {2} {3}",
                replicaActivityId.TraceId,
                composeDeploymentContext.DeploymentName,
                error,
                error.Message);
            break;
        }
    }

    return QueryResult(move(composeDeploymentList));
}

QueryResult ClusterManagerReplica::GetComposeDeploymentUpgradeProgress(QueryArgumentMap const & queryArgs, ReplicaActivityId const & replicaActivityId)
{
    wstring deploymentNameArg;
    if (!queryArgs.TryGetValue(QueryResourceProperties::Deployment::DeploymentName, deploymentNameArg))
    {
        return QueryResult(ErrorCode(ErrorCodeValue::InvalidArgument, wformatString(GET_RC(MissingQueryParameter), "DeploymentName")));
    }

    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId);
    ComposeDeploymentContext composeDeploymentContext(deploymentNameArg);
    auto error = GetComposeDeploymentContext(storeTx, composeDeploymentContext);
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} GetComposeDeploymentContexts {1} failed with error = {2} {3}",
            replicaActivityId.TraceId,
            deploymentNameArg,
            error,
            error.Message);
        return error;
    }

    unique_ptr<ComposeDeploymentUpgradeProgress> result;

    NamingUri applicationName = composeDeploymentContext.ApplicationName;
    ComposeDeploymentUpgradeContext upgradeContext = ComposeDeploymentUpgradeContext(composeDeploymentContext.DeploymentName);
    error = storeTx.ReadExact(upgradeContext);

    if (error.IsError(ErrorCodeValue::NotFound))
    {
        if (composeDeploymentContext.IsComplete || composeDeploymentContext.IsFailed)
        {
            result = make_unique<ComposeDeploymentUpgradeProgress>(
                composeDeploymentContext.DeploymentName,
                move(applicationName),
                RollingUpgradeMode::Invalid,
                0,
                RollingUpgradeMonitoringPolicy(),
                nullptr, // ApplicationHealthPolicy
                composeDeploymentContext.TypeVersion.Value,
                ComposeDeploymentUpgradeState::Enum::CompletedRollforward,
                wstring());
            error = ErrorCodeValue::Success;
            return QueryResult(move(result));
        }
        else
        {
            return ErrorCodeValue::CMRequestAlreadyProcessing;
        }
    }
    else if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0}: Failed to get record for ComposeDeploymentUpgradeContext '{1}': Error {2} {3}",
            replicaActivityId,TraceId,
            composeDeploymentContext.DeploymentName,
            error,
            error.Message);
        return QueryResult(error);
    }

    auto targetVersion = upgradeContext.TargetTypeVersion;
    auto upgradeState = upgradeContext.CurrentStatus;
    wstring statusDetails = upgradeContext.StatusDetails;

    if (upgradeState == ComposeDeploymentUpgradeState::Enum::ProvisioningTarget
        || upgradeState == ComposeDeploymentUpgradeState::Enum::Failed)
    {
        ApplicationUpgradeDescription upgradeDescription = upgradeContext.TakeUpgradeDescription();
        result = make_unique<ComposeDeploymentUpgradeProgress>(
            composeDeploymentContext.DeploymentName,
            move(applicationName),
            upgradeDescription.RollingUpgradeMode,
            UpgradeHelper::ToPublicReplicaSetCheckTimeoutInSeconds(upgradeDescription.ReplicaSetCheckTimeout),
            upgradeDescription.MonitoringPolicy,
            upgradeDescription.IsHealthPolicyValid ? make_shared<ApplicationHealthPolicy>(upgradeDescription.HealthPolicy) : nullptr,
            targetVersion.Value,
            upgradeState,
            move(statusDetails));
    }
    else if (upgradeState == ComposeDeploymentUpgradeState::Enum::Invalid)
    {
        WriteWarning(
            TraceComponent,
            "{0} invalid compose deployment state {1}",
            this->TraceId,
            upgradeState);

        Assert::TestAssert();
        return ErrorCodeValue::OperationFailed;
    }
    else
    {
        ApplicationUpgradeContext applicationUpgradeContext = ApplicationUpgradeContext(composeDeploymentContext.ApplicationName);
        error = storeTx.ReadExact(applicationUpgradeContext);
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: Failed to get record for ApplicationUpgradeContext '{1}': Error {2} {3}",
                replicaActivityId.TraceId,
                composeDeploymentContext.ApplicationName,
                error,
                error.Message);
            return QueryResult(error);
        }
        // Take from ApplicationUpgradeContext after provision target;
        ApplicationUpgradeDescription upgradeDescription = applicationUpgradeContext.TakeUpgradeDescription();
        auto upgradeMode = upgradeDescription.RollingUpgradeMode;

        if (upgradeContext.IsFailed)
        {
            WriteInfo(
                TraceComponent,
                "{0} overriding {1} upgrade state {2} with Failed",
                this->TraceId,
                deploymentNameArg,
                upgradeState);
            upgradeState = ComposeDeploymentUpgradeState::Enum::Failed;
        }

        switch (upgradeState)
        {
            case ComposeDeploymentUpgradeState::Enum::RollingForward:
            case ComposeDeploymentUpgradeState::Enum::UnprovisioningCurrent:
            case ComposeDeploymentUpgradeState::Enum::CompletedRollforward:
            case ComposeDeploymentUpgradeState::Enum::Failed:
                // no-op: keep values from upgrade description
                break;

            case ComposeDeploymentUpgradeState::Enum::RollingBack:
            case ComposeDeploymentUpgradeState::Enum::UnprovisioningTarget:
            case ComposeDeploymentUpgradeState::Enum::CompletedRollback:
                targetVersion = upgradeContext.CurrentTypeVersion;
                upgradeDescription.SetTargetVersion(targetVersion.Value);
                upgradeDescription.SetUpgradeMode(upgradeMode);
                upgradeDescription.SetReplicaSetCheckTimeout(
                    this->GetRollbackReplicaSetCheckTimeout(upgradeDescription.ReplicaSetCheckTimeout));
                break;

            default:
                WriteWarning(
                    TraceComponent,
                    "{0} invalid upgrade state {1}",
                    this->TraceId,
                    upgradeState);
                Assert::TestAssert();
                return ErrorCodeValue::OperationFailed;
        }

        result = make_unique<ComposeDeploymentUpgradeProgress>(
            composeDeploymentContext.DeploymentName,
            move(applicationName),
            upgradeDescription.UpgradeType,
            upgradeDescription.RollingUpgradeMode,
            UpgradeHelper::ToPublicReplicaSetCheckTimeoutInSeconds(upgradeDescription.ReplicaSetCheckTimeout),
            upgradeDescription.MonitoringPolicy,
            upgradeDescription.IsHealthPolicyValid ? make_shared<ApplicationHealthPolicy>(upgradeDescription.HealthPolicy) : nullptr,
            targetVersion.Value,
            upgradeState,
            move(const_cast<wstring&>(applicationUpgradeContext.InProgressUpgradeDomain)),
            move(const_cast<vector<wstring>&>(applicationUpgradeContext.CompletedUpgradeDomains)),
            move(const_cast<vector<wstring>&>(applicationUpgradeContext.PendingUpgradeDomains)),
            applicationUpgradeContext.RollforwardInstance,
            applicationUpgradeContext.OverallUpgradeElapsedTime,
            applicationUpgradeContext.UpgradeDomainElapsedTime,
            applicationUpgradeContext.TakeUnhealthyEvaluations(),
            applicationUpgradeContext.TakeCurrentUpgradeDomainProgress(),
            move(statusDetails),
            applicationUpgradeContext.TakeCommonUpgradeContextData());
    }

    if (error.IsSuccess())
    {
        storeTx.CommitReadOnly();
    }

    return QueryResult(move(result));
}

ServiceModel::QueryResult ClusterManagerReplica::GetServices(QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId)
{
    int64 maxResults;
    auto error = QueryPagingDescription::TryGetMaxResultsFromArgumentMap(queryArgs, maxResults, replicaActivityId.TraceId, L"ClusterManagerReplica::GetNodes");
    if (!error.IsSuccess())
    {
        return QueryResult(move(error));
    }

    ListPager<ServiceQueryResult> serviceResultList;
    serviceResultList.SetMaxResults(maxResults);

    // Find the ApplicationName argument. We expect this to be available as it is mandatory argument.
    wstring applicationNameArgument;
    if (!queryArgs.TryGetValue(QueryResourceProperties::Application::ApplicationName, applicationNameArgument))
    {
        return ServiceModel::QueryResult(ErrorCodeValue::InvalidArgument);
    }

    // Parse the application name. Return error if parsing fails.
    Common::NamingUri applicationNameUri;
    if (!Common::NamingUri::TryParse(applicationNameArgument, applicationNameUri))
    {
        WriteInfo(
            TraceComponent,
            "{0}: ApplicationName passed as query argument to GetServices is invalid. ApplicationName = {1}",
            replicaActivityId.TraceId,
            applicationNameUri);
        return QueryResult(ErrorCodeValue::InvalidNameUri);
    }

    wstring serviceNameArgument;
    queryArgs.TryGetValue(QueryResourceProperties::Service::ServiceName, serviceNameArgument);

    wstring serviceTypeNameArgument;
    queryArgs.TryGetValue(QueryResourceProperties::ServiceType::ServiceTypeName, serviceTypeNameArgument);

    // Filtering by both service name and service type name is not allowed
    if (!serviceNameArgument.empty() && !serviceTypeNameArgument.empty())
    {
        WriteInfo(
            TraceComponent,
            "Both ServiceName {0} and ServiceTypeName {1} are specified in GetServices.",
            serviceNameArgument,
            serviceTypeNameArgument);
        return QueryResult(
            ErrorCode(
                ErrorCodeValue::InvalidArgument,
                GET_RC(Service_Name_Type_Dup_Defined)
            ));
    }

    // Create applicationContext with the application name and ReadExact using it.
    ApplicationContext appContext(applicationNameUri);
    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId);
    error = this->GetValidApplicationContext(storeTx, appContext);
    if (!error.IsSuccess())
    {
        storeTx.Rollback();

        WriteInfo(
            TraceComponent,
            "{0}: Error while obtaining services for application. ErrorCode = {1}",
            replicaActivityId.TraceId,
            error);
        return ServiceModel::QueryResult(error);
    }

    wstring continuationToken;
    bool checkContinuationToken = queryArgs.TryGetValue(QueryResourceProperties::QueryMetadata::ContinuationToken, continuationToken);

    // Check if the service name has been specified as query argument. If so, filter by service name
    vector<ServiceContext> serviceContexts;
    if (!serviceNameArgument.empty())
    {
        if (checkContinuationToken && serviceNameArgument <= continuationToken)
        {
            // Doesn't respect the continuation token, nothing to do
            return QueryResult(move(serviceResultList));
        }

        Common::NamingUri serviceNameUri;
        if (Common::NamingUri::TryParse(serviceNameArgument, serviceNameUri))
        {
            ServiceContext serviceContext(
                appContext.ApplicationId,
                applicationNameUri,
                serviceNameUri);

            error = storeTx.ReadExact(serviceContext);
            if (error.IsSuccess())
            {
                serviceContexts.push_back(move(serviceContext));
            }
            else
            {
                WriteInfo(
                    TraceComponent,
                    "{0}: Error while obtaining the ServiceContext. ErrorCode = {1}",
                    replicaActivityId.TraceId,
                    error);
            }
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0}: ServiceName passed as the query argument to the GetServices is invalid. ServiceName = {1}",
                replicaActivityId.TraceId,
                serviceNameArgument);

            error = ErrorCodeValue::InvalidNameUri;
        }
    }
    else
    {
        // If we need all the services, use ReadApplicationServices to read prefix
        // ServiceContext has as primary key ApplicationId + Service name (as string).
        // The results are ordered by service name string so we can add them to list pager without extra sorting.
        error = ServiceContext::ReadApplicationServices(
            storeTx,
            appContext.ApplicationId,
            serviceContexts);

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: Error while obtaining the ServiceContexts. ErrorCode = {1}",
                replicaActivityId.TraceId,
                error);
        }
    }

    if(!error.IsSuccess())
    {
        storeTx.Rollback();

        // In case of error NotFound, we should return an empty list.
        if(error.IsError(ErrorCodeValue::ServiceNotFound) || error.IsError(ErrorCodeValue::NotFound))
        {
            return ServiceModel::QueryResult(move(serviceResultList));
        }

        return ServiceModel::QueryResult(error);
    }

    // ApplicationTypeContext is needed to retrieve the Service Type Version.
    StoreDataServiceManifest serviceManifest(ServiceModelTypeName(appContext.TypeName), appContext.TypeVersion);
    error = storeTx.ReadExact(serviceManifest);
    if (!error.IsSuccess())
    {
        storeTx.Rollback();
        return ServiceModel::QueryResult(error);
    }

    // store object is no longer needed.
    storeTx.CommitReadOnly();

    // Add services to the result.
    wstring serviceTypeManifestVersion;
    for (auto serviceContext = serviceContexts.begin(); serviceContext != serviceContexts.end(); ++serviceContext)
    {
        if (checkContinuationToken && serviceContext->ServiceDescriptor.Service.Name <= continuationToken)
        {
            // Doesn't respect continuation token, continue
            continue;
        }

        if (!serviceTypeNameArgument.empty() && StringUtility::Compare(serviceContext->ServiceTypeName.Value, serviceTypeNameArgument) != 0)
        {
            // ServiceTypeNameFilter does not match
            continue;
        }

        auto serviceManifestImports = serviceManifest.ServiceManifests;
        // Find the manifest that the service type belongs to.
        auto result = std::find_if(serviceManifestImports.begin(),serviceManifestImports.end(),
            [& serviceContext](ServiceModelServiceManifestDescription & item)
            {
                return (serviceContext->PackageName.Value == item.Name);
            }
        );

        if(result != serviceManifestImports.end())
        {
            serviceTypeManifestVersion = result->Version;
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0}: Package Name:{1} was not found in the service manifests of Application Type:{2} and Version:{3}",
                replicaActivityId.TraceId,
                serviceContext->PackageName.Value,
                appContext.TypeName,
                appContext.TypeVersion);

            // No error here because other fields have already been fetched.
            serviceTypeManifestVersion = L"";
        }

        error = serviceResultList.TryAdd(
            serviceContext->ToQueryResult(serviceTypeManifestVersion));

        if (!error.IsSuccess() && serviceResultList.IsBenignError(error))
        {
            serviceResultList.TraceErrorFromTryAdd(error, TraceComponent, replicaActivityId.TraceId, L"GetServices@" + serviceContext->ServiceName.Value);
            break;
        }
    }

    return ServiceModel::QueryResult(move(serviceResultList));
}

ErrorCode ClusterManagerReplica::GetServiceTypesFromStore(
    wstring applicationTypeName,
    wstring applicationTypeVersion,
    Store::ReplicaActivityId const & replicaActivityId,
    __out vector<ServiceModelServiceManifestDescription> & serviceManifests)
{
       // Read the application types using the prefix
    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId);

    ServiceModelVersion version(applicationTypeVersion);
    StoreDataServiceManifest storedData(ServiceModelTypeName(applicationTypeName), version);
    ErrorCode error = storeTx.ReadExact(storedData);
    if (!error.IsSuccess())
    {
        storeTx.Rollback();

        WriteInfo(
            TraceComponent,
            "{0} No provisioned ApplicationTypeContext was found for ApplicationTypeName:{1} and ApplicationTypeVersion:{2}",
            replicaActivityId.TraceId,
            applicationTypeName,
            applicationTypeVersion);

        if(error.IsError(ErrorCodeValue::NotFound))
        {
            error = ErrorCodeValue::ApplicationTypeNotFound;
        }
        return error;
    }

    storeTx.CommitReadOnly();

    serviceManifests = move(storedData.ServiceManifests);
    return ErrorCode::Success();
}

ServiceModel::QueryResult ClusterManagerReplica::GetServiceTypes(QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId)
{
    // Validate that the arguments contain both "ApplicationTypeName" and "ApplicationTypeVersion"
    wstring applicationTypeNameArgument;
    wstring applicationTypeVersionArgument;
    if (!queryArgs.TryGetValue(QueryResourceProperties::ApplicationType::ApplicationTypeName, applicationTypeNameArgument)
        || !queryArgs.TryGetValue(QueryResourceProperties::ApplicationType::ApplicationTypeVersion, applicationTypeVersionArgument)
        )
    {
        return ServiceModel::QueryResult(ErrorCodeValue::InvalidArgument);
    }

    vector<ServiceModelServiceManifestDescription> serviceManifests;
    ErrorCode error = GetServiceTypesFromStore(
        applicationTypeNameArgument,
        applicationTypeVersionArgument,
        replicaActivityId,
        serviceManifests);
    if (!error.IsSuccess())
    {
        return QueryResult(error);
    }

    wstring serviceTypeNameArgument;
    bool filterServiceType = queryArgs.TryGetValue(QueryResourceProperties::ServiceType::ServiceTypeName, serviceTypeNameArgument);
    vector<ServiceTypeQueryResult> resultList;
    // Query from services
    for(auto it1 = serviceManifests.begin(); it1 != serviceManifests.end(); ++it1)
    {
        vector<ServiceTypeDescription> serviceTypeDescriptions = it1->ServiceTypeNames;
        for(auto it2 = serviceTypeDescriptions.begin(); it2 != serviceTypeDescriptions.end(); ++it2)
        {
            if (!filterServiceType || it2->ServiceTypeName == serviceTypeNameArgument)
            {
                resultList.push_back(ServiceTypeQueryResult(*it2, it1->Version, it1->Name));
            }
        }
    }

    // Query from service groups
    for (auto it1 = serviceManifests.begin(); it1 != serviceManifests.end(); ++it1)
    {
        vector<ServiceGroupTypeDescription> serviceGroupTypeDescriptions = it1->ServiceGroupTypeNames;
        for (auto it2 = serviceGroupTypeDescriptions.begin(); it2 != serviceGroupTypeDescriptions.end(); ++it2)
        {
            if (!filterServiceType || it2->Description.ServiceTypeName == serviceTypeNameArgument)
            {
                ServiceTypeQueryResult serviceGroupTypeQueryResult (it2->Description, it1->Version, it1->Name);
                serviceGroupTypeQueryResult.IsServiceGroup = true;
                resultList.push_back(serviceGroupTypeQueryResult);
            }
        }
    }

    if(filterServiceType && resultList.empty())
    {
        // No match for the ServiceTypeName
        WriteInfo(
        TraceComponent,
        "{0} No ServiceTypes were found for ServiceTypeName:{1}",
        replicaActivityId.TraceId,
        serviceTypeNameArgument);
    }

    return ServiceModel::QueryResult(move(resultList));
}

ServiceModel::QueryResult ClusterManagerReplica::GetServiceGroupMemberTypes(QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId)
{
    // Validate that the arguments contain both "ApplicationTypeName" and "ApplicationTypeVersion"
    wstring applicationTypeNameArgument;
    wstring applicationTypeVersionArgument;
    if (!queryArgs.TryGetValue(QueryResourceProperties::ApplicationType::ApplicationTypeName, applicationTypeNameArgument)
        || !queryArgs.TryGetValue(QueryResourceProperties::ApplicationType::ApplicationTypeVersion, applicationTypeVersionArgument)
        )
    {
        return ServiceModel::QueryResult(ErrorCodeValue::InvalidArgument);
    }

    vector<ServiceModelServiceManifestDescription> serviceManifests;
    ErrorCode error = GetServiceTypesFromStore(
        applicationTypeNameArgument,
        applicationTypeVersionArgument,
        replicaActivityId,
        serviceManifests);
    if (!error.IsSuccess())
    {
        return QueryResult(error);
    }

    wstring serviceGroupTypeNameArgument;
    bool filterServiceGroupType = queryArgs.TryGetValue(QueryResourceProperties::ServiceType::ServiceGroupTypeName, serviceGroupTypeNameArgument);
    vector<ServiceGroupMemberTypeQueryResult> resultList;
    for (auto it1 = serviceManifests.begin(); it1 != serviceManifests.end(); ++it1)
    {
        vector<ServiceGroupTypeDescription> serviceGroupTypeDescriptions = it1->ServiceGroupTypeNames;
        for (auto it2 = serviceGroupTypeDescriptions.begin(); it2 != serviceGroupTypeDescriptions.end(); ++it2)
        {
            if (!filterServiceGroupType || it2->Description.ServiceTypeName == serviceGroupTypeNameArgument)
            {
                resultList.push_back(ServiceGroupMemberTypeQueryResult(it2->Members, it1->Version, it1->Name));
            }
        }
    }

    if (filterServiceGroupType && resultList.empty())
    {
        // No match for the ServiceGroupTypeName
        WriteInfo(
            TraceComponent,
            "{0} No ServiceGroupTypes were found for ServiceGroupTypeName:{1}",
            replicaActivityId.TraceId,
            serviceGroupTypeNameArgument);
    }

    return ServiceModel::QueryResult(move(resultList));
}

ServiceModel::QueryResult ClusterManagerReplica::GetApplicationManifest(QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId)
{
    wstring applicationTypeName, applicationTypeVersion;
    if (!queryArgs.TryGetValue(QueryResourceProperties::ApplicationType::ApplicationTypeName, applicationTypeName) ||
        !queryArgs.TryGetValue(QueryResourceProperties::ApplicationType::ApplicationTypeVersion, applicationTypeVersion))
    {
        return QueryResult(ErrorCodeValue::InvalidArgument);
    }

    ServiceModelTypeName typeName(applicationTypeName);
    ServiceModelVersion typeVersion(applicationTypeVersion);
    StoreDataApplicationManifest applicationManifest(typeName, typeVersion);
    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId);
    ErrorCode error = storeTx.ReadExact(applicationManifest);
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0}: Application Type {1}:{2} was not found",
            replicaActivityId.TraceId,
            applicationTypeName,
            applicationTypeVersion);
        storeTx.Rollback();
        return QueryResult(ErrorCodeValue::ApplicationTypeNotFound);
    }

    storeTx.CommitReadOnly();

    // column is not used anywhere. Hence empty string.
    return ServiceModel::QueryResult(make_unique<wstring>(applicationManifest.ApplicationManifest));
}

void ClusterManagerReplica::InitializeCachedFilePtrs()
{
    wstring dataRoot;
    auto error = FabricEnvironment::GetFabricDataRoot(dataRoot);
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0}: InitializeCachedFilePtrs: GetFabricDataRoot returned error {1}",
            this->TraceId,
            error);

        return;
    }

    ImageModel::FabricDeploymentSpecification deploymentSpecification(dataRoot);
    wstring clusterManifestFilePath;
    clusterManifestFilePath = deploymentSpecification.GetCurrentClusterManifestFile(nodeName_);
    if (!File::Exists(clusterManifestFilePath))
    {
        WriteInfo(
            TraceComponent,
            "{0}: InitializeCachedFilePtrs: current cluster manifest '{1}' does not exist",
            this->TraceId,
            clusterManifestFilePath);

        return;
    }

    error = CachedFile::Create(
        clusterManifestFilePath,
        [](__in wstring const & filePath, __out wstring & content)
        {
            return XmlReader::ReadXmlFile(
                filePath,
                *ServiceModel::SchemaNames::Element_ClusterManifest,
                *ServiceModel::SchemaNames::Namespace,
                content);
        },
        currentClusterManifest_);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0}: InitializeCachedFilePtrs: CachedFile initialization failed with error {1}",
            this->TraceId,
            error);
    }
}

ServiceModel::QueryResult ClusterManagerReplica::GetClusterVersion()
{
    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId);
    auto upgradeContext = make_unique<FabricUpgradeContext>();
    ErrorCode error = storeTx.ReadExact(*upgradeContext);

    // An item should be found any time after a cluster has been upgraded - either code or config.
    // Will return ErrorCodeValue::NotFound if there is no entry.
    if (error.IsSuccess())
    {
        storeTx.CommitReadOnly();
        bool upgradeCompleted = upgradeContext->IsComplete;
        auto & currentVersion = upgradeContext->CurrentVersion;
        auto & targetVersion = upgradeContext->UpgradeDescription.Version;

        // If the value is 0.0.0.0, then it should not be valid. This happens when an upgrade has started for the first time in the cluster, but not yet completed.
        // In production clusters, there is a baseline upgrade, so this should not be an issue.
        // In dev clusters, while possible, it is not a supported case, so return an error telling the user to check back after the upgrade.
        if (currentVersion.IsValid)
        {
            // Return the lesser of the code versions.
            // Type FabricCodeVersion has custom compare operators (overridden > == and <)
            // Type FabricCodeVersion has a overridden to string method
            if ((currentVersion < targetVersion || upgradeCompleted))
            {
                return QueryResult(make_unique<wstring>(currentVersion.CodeVersion.ToString()));
            }
            else
            {
                return QueryResult(make_unique<wstring>(targetVersion.CodeVersion.ToString()));
            }
        }        
    }
    else if (!error.IsError(ErrorCodeValue::NotFound))
    {
        return QueryResult(move(error));
    }

    // If code reaches here, it means that error code was NotFound

    // If version is not valid, then it means that an upgrade is happening - the first ever upgrade, so that
    // the cluster version is 0.0.0.0 according to CM.
    // In this case, return NotFound. Only dev clusters should reach this path, and fabric upgrades are not supported.
    // Since it will be a config only upgrade, all nodes will have the same version.
    wstring currentVersion;
    error = FabricEnvironment::GetFabricVersion(currentVersion);

    if (error.IsSuccess())
    {
        return QueryResult(make_unique<wstring>(currentVersion));
    }
    return QueryResult(move(error));
}

ServiceModel::QueryResult ClusterManagerReplica::GetClusterManifest(
    QueryArgumentMap const & queryArgs,
    Store::ReplicaActivityId const & replicaActivityId,
    TimeSpan const timeout)
{
    wstring versionString;
    if (queryArgs.TryGetValue(QueryResourceProperties::Cluster::ConfigVersionFilter, versionString))
    {
        FabricConfigVersion targetConfigVersion;
        auto error = FabricConfigVersion::FromString(versionString, targetConfigVersion);
        if (!error.IsSuccess())
        {
            return QueryResult(error);
        }

        // Verify that requested version is provisioned before attempting to read from image store.
        //
        auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId);
        auto provisionContext = make_unique<FabricProvisionContext>();
        error = storeTx.ReadExact(*provisionContext);

        if (error.IsError(ErrorCodeValue::NotFound))
        {
            WriteInfo(
                TraceComponent,
                "{0}: GetClusterManifest('{1}'): FabricProvisionContext not found",
                replicaActivityId.TraceId,
                targetConfigVersion);

            return ServiceModel::QueryResult(ErrorCodeValue::FabricVersionNotFound);
        }
        else if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: GetClusterManifest('{1}'): error={2}",
                replicaActivityId.TraceId,
                targetConfigVersion,
                error);

            return ServiceModel::QueryResult(error);
        }
        else
        {
            bool found = false;
            for (auto const & provisionedVersion : provisionContext->ProvisionedConfigVersions)
            {
                if (provisionedVersion == targetConfigVersion)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                WriteInfo(
                    TraceComponent,
                    "{0}: GetClusterManifest('{1}'): version not found in {2}",
                    replicaActivityId.TraceId,
                    targetConfigVersion,
                    provisionContext->ProvisionedConfigVersions);

                return ServiceModel::QueryResult(ErrorCodeValue::FabricVersionNotFound);
            }
        }

        storeTx.CommitReadOnly();

        wstring clusterManifestContents;
        error = imageBuilder_.GetClusterManifestContents(
            targetConfigVersion,
            timeout,
            clusterManifestContents);

        if (error.IsSuccess())
        {
            return QueryResult(make_unique<wstring>(clusterManifestContents));
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0}: Reading cluster manifest version '{1}' failed with error {2}",
                replicaActivityId.TraceId,
                targetConfigVersion,
                error);

            return QueryResult(error);
        }
    }
    else if (!currentClusterManifest_)
    {
        TRACE_ERROR_AND_TESTASSERT(
            TraceComponent,
            "{0} : current cluster manifest cached file pointer should not be null",
            this->TraceId);

        return QueryResult(ErrorCodeValue::InvalidState);
    }
    else
    {
        wstring clusterManifestContents;
        auto error = currentClusterManifest_->ReadFileContent(clusterManifestContents);
        if (error.IsSuccess())
        {
            return QueryResult(make_unique<wstring>(clusterManifestContents));
        }
        else
        {
            WriteWarning(
                TraceComponent,
                "{0}: Reading current cluster manifest failed with error {1}",
                replicaActivityId.TraceId,
                error);

            return QueryResult(error);
        }
    }
}

bool ClusterManagerReplica::IsExclusiveFilterHelper(bool const isValid, bool & hasFilterSet)
{
    if (!isValid) { return true; }

    if (hasFilterSet)
    {
        return false;
    }
    else
    {
        hasFilterSet = true;
        return true;
    }
}

bool ClusterManagerReplica::IsStatusFilterDefaultOrAll(DWORD replicaStatusFilter)
{
    if ((replicaStatusFilter == (DWORD)FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_DEFAULT) ||
        (replicaStatusFilter == (DWORD)FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_ALL))
    {
        return true;
    }
    return false;
}

ServiceModel::QueryResult ClusterManagerReplica::GetNodes(QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId)
{
    wstring infrastructureManifestPath;
    ErrorCode error;

    wstring dataRoot;
    error = FabricEnvironment::GetFabricDataRoot(dataRoot);
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0}: FabricEnvironment::GetFabricDataRoot returned error - {1}",
            replicaActivityId.TraceId,
            error);

        return QueryResult(move(error));
    }

    ImageModel::FabricDeploymentSpecification deploymentSpecification(dataRoot);
    infrastructureManifestPath = deploymentSpecification.GetInfrastructureManfiestFile(nodeName_);

    // infrastructureDescription has a member variable that contains a list of the nodes on the cluster.
    InfrastructureDescription infrastructureDescription;
    error = Parser::ParseInfrastructureDescription(infrastructureManifestPath, infrastructureDescription);
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0}: Parser::ParseInfrastructureDescription failed for the file name - {1} with error - {2}",
            replicaActivityId.TraceId,
            infrastructureManifestPath,
            error);

        return QueryResult(move(error));
    }

    wstring continuationTokenString;
    NodeId lastNodeId;
    bool checkContinuationToken = queryArgs.TryGetValue(QueryResourceProperties::QueryMetadata::ContinuationToken, continuationTokenString);
    if (checkContinuationToken)
    {
        error = PagingStatus::GetContinuationTokenData<NodeId>(continuationTokenString, lastNodeId);
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: error interpreting the continuation token {1} as nodeid: {2} {3}",
                replicaActivityId.TraceId,
                continuationTokenString,
                error,
                error.Message);
            return QueryResult(move(error));
        }

        WriteInfo(
            TraceComponent,
            "{0}: GetNodes with continuation token {1}",
            replicaActivityId.TraceId,
            continuationTokenString);
    }

    // Get max results value
    int64 maxResults;
    error = QueryPagingDescription::TryGetMaxResultsFromArgumentMap(queryArgs, maxResults, replicaActivityId.TraceId, L"ClusterManagerReplica::GetNodes");
    if (!error.IsSuccess())
    {
        return QueryResult(move(error));
    }

    ListPager<NodeQueryResult> nodeQueryResultList;
    nodeQueryResultList.SetMaxResults(maxResults);
    wstring nodeStatusFilterString;
    DWORD nodeStatusFilter = (DWORD)FABRIC_QUERY_NODE_STATUS_FILTER_DEFAULT;
    if (queryArgs.TryGetValue(QueryResourceProperties::Node::NodeStatusFilter, nodeStatusFilterString))
    {
        if (!StringUtility::TryFromWString<DWORD>(nodeStatusFilterString, nodeStatusFilter))
        {
            WriteInfo(
                TraceComponent,
                "Invalid value for nodeStatusFilter argument: {0}",
                nodeStatusFilterString);
            return ErrorCode(ErrorCodeValue::InvalidArgument, wformatString("Invalid value for nodeStatusFilter argument: {0}", nodeStatusFilterString));
        }
        else
        {
            if (!IsStatusFilterDefaultOrAll(nodeStatusFilter))
            {
                // return empty list.
                return QueryResult(move(nodeQueryResultList));
            }
        }
    }

    wstring desiredNodeName;
    if (queryArgs.TryGetValue(QueryResourceProperties::Node::Name, desiredNodeName))
    {
        for (auto iter = infrastructureDescription.NodeList.begin(); iter < infrastructureDescription.NodeList.end(); iter++)
        {
            if (iter->NodeName == desiredNodeName)
            {
                NodeId nodeId;
                error = Federation::NodeIdGenerator::GenerateFromString(iter->NodeName, nodeId);
                if (!error.IsSuccess())
                {
                    WriteInfo(
                        TraceComponent,
                        "{0}: Error getting nodeId for nodeName {1}: {2}",
                        replicaActivityId.TraceId,
                        iter->NodeName,
                        error);
                }
                else if (!checkContinuationToken || nodeId > lastNodeId)
                {
                    // Node respects the continuation token, return
                    error = nodeQueryResultList.TryAdd(NodeQueryResult(
                        iter->NodeName,
                        iter->IPAddressOrFQDN,
                        iter->NodeTypeRef,
                        iter->IsSeedNode,
                        iter->UpgradeDomain,
                        iter->FaultDomain,
                        nodeId));
                    if (!error.IsSuccess())
                    {
                        return QueryResult(move(error));
                    }

                    // We found the desired node name, return
                    break;
                }
            }
        }

        return QueryResult(move(nodeQueryResultList));
    }

    // Query merging expects the nodes to be sorted ASC and continuation token to be the highest NodeId.
    // Sort the nodes by node id and select only the ones that respect the token.
    map<NodeId, InfrastructureNodeDescription> sortedNodes;
    for (auto iter = infrastructureDescription.NodeList.begin(); iter < infrastructureDescription.NodeList.end(); iter++)
    {
        NodeId nodeId;
        error = Federation::NodeIdGenerator::GenerateFromString(iter->NodeName, nodeId);
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: Error getting nodeId for nodeName {1}: {2}",
                replicaActivityId.TraceId,
                iter->NodeName,
                error);

            // Ignore this node, move to next one
            continue;
        }

        if (checkContinuationToken && nodeId <= lastNodeId)
        {
            // Node doesn't respect the continuation token, continue search
            continue;
        }

        sortedNodes.insert(make_pair(move(nodeId), *iter));
    }

    for (auto iter = sortedNodes.begin(); iter != sortedNodes.end(); ++iter)
    {
        error = nodeQueryResultList.TryAdd(NodeQueryResult(
            move(iter->second.NodeName),
            move(iter->second.IPAddressOrFQDN),
            move(iter->second.NodeTypeRef),
            iter->second.IsSeedNode,
            move(iter->second.UpgradeDomain),
            move(iter->second.FaultDomain),
            iter->first));

        if (!error.IsSuccess() && nodeQueryResultList.IsBenignError(error))
        {
            nodeQueryResultList.TraceErrorFromTryAdd(error, TraceComponent, replicaActivityId.TraceId, L"GetNodes");
            break;
        }
        else if (!error.IsSuccess())
        {
            return QueryResult(move(error));
        }
    }

    return QueryResult(move(nodeQueryResultList));
}

ServiceModel::QueryResult ClusterManagerReplica::GetServiceManifest(QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId)
{
    wstring applicationTypeName, applicationTypeVersion, serviceManifestName;
    if (!queryArgs.TryGetValue(QueryResourceProperties::ApplicationType::ApplicationTypeName, applicationTypeName) ||
        !queryArgs.TryGetValue(QueryResourceProperties::ApplicationType::ApplicationTypeVersion, applicationTypeVersion) ||
        !queryArgs.TryGetValue(QueryResourceProperties::ServiceManifest::ServiceManifestName, serviceManifestName))
    {
        return QueryResult(ErrorCodeValue::InvalidArgument);
    }

    ServiceModelTypeName typeName(applicationTypeName);
    ServiceModelVersion typeVersion(applicationTypeVersion);
    StoreDataServiceManifest storedData(typeName, typeVersion);
    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId);
    ErrorCode error = storeTx.ReadExact(storedData);
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0}: Application Type {1}:{2} was not found",
            replicaActivityId.TraceId,
            applicationTypeName,
            applicationTypeVersion);
        storeTx.Rollback();
        return QueryResult(ErrorCodeValue::ApplicationTypeNotFound);
    }

    storeTx.CommitReadOnly();

    auto result = std::find_if(storedData.ServiceManifests.begin(),storedData.ServiceManifests.end(),
                [& serviceManifestName](const ServiceModelServiceManifestDescription & serviceManifest)
                {
                    return (serviceManifestName == serviceManifest.Name);
                }
            );

    if(result == storedData.ServiceManifests.end())
    {
        WriteInfo(
            TraceComponent,
            "{0}: ServiceManifestName {1} was not found for Application Type {2}:{3}",
            replicaActivityId.TraceId,
            serviceManifestName,
            applicationTypeName,
            applicationTypeVersion);

        return QueryResult(ErrorCodeValue::ServiceManifestNotFound);
    }

    // column is not used anywhere. Hence empty string.
    return ServiceModel::QueryResult(make_unique<wstring>(result->Content));
}

ServiceModel::QueryResult ClusterManagerReplica::GetInfrastructureTask(QueryArgumentMap const &, Store::ReplicaActivityId const & replicaActivityId)
{
    vector<InfrastructureTaskContext> contexts;
    auto error = this->ReadPrefix<InfrastructureTaskContext>(Constants::StoreType_InfrastructureTaskContext, contexts);

    vector<InfrastructureTaskQueryResult> resultList;
    for (auto it = contexts.begin(); it != contexts.end(); ++it)
    {
        WriteNoise(
            TraceComponent,
            "{0} queried {1}:{2}",
            replicaActivityId.TraceId,
            it->Description,
            it->State);

        resultList.push_back(InfrastructureTaskQueryResult(
            make_shared<InfrastructureTaskDescription>(it->Description),
            it->State));
    }

    return ServiceModel::QueryResult(move(resultList));
}

ServiceModel::QueryResult ClusterManagerReplica::GetProvisionedFabricCodeVersions(QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId)
{
    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId);
    auto provisionContext = make_unique<FabricProvisionContext>();
    auto error = storeTx.ReadExact(*provisionContext);
    if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::NotFound))
    {
        WriteInfo(
            TraceComponent,
            "{0}: GetProvisionedFabricCodeVersions: ReadExact returned error:{1}",
            replicaActivityId.TraceId,
            error);

        storeTx.Rollback();
        return ServiceModel::QueryResult(error);
    }
    else
    {
        storeTx.CommitReadOnly();
    }

    wstring codeVersionFilter;
    vector<ProvisionedFabricCodeVersionQueryResultItem> resultList;
    if (!error.IsError(ErrorCodeValue::NotFound))
    {
        if (queryArgs.TryGetValue(QueryResourceProperties::Cluster::CodeVersionFilter, codeVersionFilter))
        {
            for(auto it = provisionContext->ProvisionedCodeVersions.begin();
                it != provisionContext->ProvisionedCodeVersions.end();
                ++it)
            {
                auto codeVersion = it->ToString();
                if (StringUtility::Compare(codeVersionFilter, codeVersion) == 0)
                {
                    resultList.push_back(codeVersion);
                }
            }
        }
        else
        {
            for(auto it = provisionContext->ProvisionedCodeVersions.begin();
                it != provisionContext->ProvisionedCodeVersions.end();
                ++it)
            {
                resultList.push_back(it->ToString());
            }
        }
    }

    return ServiceModel::QueryResult(move(resultList));
}


ServiceModel::QueryResult ClusterManagerReplica::GetProvisionedFabricConfigVersions(QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId)
{
    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId);
    auto provisionContext = make_unique<FabricProvisionContext>();
    auto error = storeTx.ReadExact(*provisionContext);
    if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::NotFound))
    {
        WriteInfo(
            TraceComponent,
            "{0}: GetProvisionedFabricConfigVersions: ReadExact returned error:{1}",
            replicaActivityId.TraceId,
            error);

        storeTx.Rollback();
        return ServiceModel::QueryResult(error);
    }
    else
    {
        storeTx.CommitReadOnly();
    }

    vector<ProvisionedFabricConfigVersionQueryResultItem> resultList;
    wstring configVersionFilter;

    if (!error.IsError(ErrorCodeValue::NotFound))
    {
        if (queryArgs.TryGetValue(QueryResourceProperties::Cluster::ConfigVersionFilter, configVersionFilter))
        {
            for(auto it = provisionContext->ProvisionedConfigVersions.begin();
                it != provisionContext->ProvisionedConfigVersions.end();
                ++it)
            {
                auto configVersion = it->ToString();
                if (StringUtility::Compare(configVersionFilter, configVersion) == 0)
                {
                    resultList.push_back(configVersion);
                }
            }
        }
        else
        {
            for(auto it = provisionContext->ProvisionedConfigVersions.begin();
                it != provisionContext->ProvisionedConfigVersions.end();
                ++it)
            {
                resultList.push_back(it->ToString());
            }
        }
    }

    return ServiceModel::QueryResult(move(resultList));
}


void ClusterManagerReplica::RequestMessageHandler(
    MessageUPtr && request,
    RequestReceiverContextUPtr && requestContext)
{
    wstring rejectReason;
    if (!ValidateClientMessage(request, rejectReason))
    {
        CMEvents::Trace->RequestValidationFailed(
            this->PartitionedReplicaId,
            request->Action,
            request->MessageId,
            rejectReason);

        requestContext->Reject(ErrorCodeValue::InvalidMessage);
        return;
    }

    auto timeout = TimeoutHeader::FromMessage(*request).Timeout;
    auto activityId = FabricActivityHeader::FromMessage(*request).ActivityId;
    auto messageId = request->MessageId;

    CMEvents::Trace->RequestReceived(
        ReplicaActivityId(this->PartitionedReplicaId, activityId),
        request->Action,
        request->MessageId,
        timeout);

    // Processing can continue asynchronously after unregistering the message handler
    auto selfRoot = this->CreateComponentRoot();

    BeginProcessRequest(
        move(request),
        move(requestContext),
        activityId,
        timeout,
        [this, selfRoot, activityId, messageId](AsyncOperationSPtr const & asyncOperation) mutable
        {
            this->OnProcessRequestComplete(activityId, messageId, asyncOperation);
        },
        this->Root.CreateAsyncOperationRoot());
}

AsyncOperationSPtr ClusterManagerReplica::BeginProcessRequest(
    MessageUPtr && request,
    RequestReceiverContextUPtr && requestContext,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto activityId = FabricActivityHeader::FromMessage(*request).ActivityId;

    return BeginProcessRequest(
        move(request),
        move(requestContext),
        activityId,
        timeout,
        callback,
        parent);
}

AsyncOperationSPtr ClusterManagerReplica::BeginProcessRequest(
    MessageUPtr && request,
    RequestReceiverContextUPtr && requestContext,
    Common::ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ErrorCode error(ErrorCodeValue::Success);

    // rolloutManagerUPtr_ is set once during the first ChangeRole() to primary and not
    // reset until destruction of this class
    //
    if (!rolloutManagerUPtr_ || !rolloutManagerUPtr_->IsActive)
    {
        error = ErrorCodeValue::NotPrimary;

        CMEvents::Trace->RequestRejected(
            ReplicaActivityId(this->PartitionedReplicaId, activityId),
            request->MessageId,
            error);
    }
    else
    {
        auto it = requestHandlers_.find(request->Action);
        if (it != requestHandlers_.end())
        {
            return (it->second)(
                *this,
                request,
                requestContext,
                timeout,
                callback,
                parent);
        }
        else if (request->Action == QueryTcpMessage::QueryAction)
        {
            return AsyncOperation::CreateAndStart<ClusterManagerQueryMessageHandler>(
                *this,
                queryMessageHandler_,
                move(request),
                move(requestContext),
                timeout,
                callback,
                parent);
        }
        else
        {
            CMEvents::Trace->RequestValidationFailed2(
                ReplicaActivityId(this->PartitionedReplicaId, activityId),
                request->Action,
                request->MessageId,
                RejectReasonInvalidAction);

            error = ErrorCodeValue::InvalidMessage;
        }
    }

    return AsyncOperation::CreateAndStart<ClientRequestAsyncOperation>(
        *this,
        error,
        move(request),
        move(requestContext),
        timeout,
        callback,
        parent);
}

ErrorCode ClusterManagerReplica::EndProcessRequest(
    AsyncOperationSPtr const & asyncOperation,
    __out Transport::MessageUPtr & reply,
    __out Federation::RequestReceiverContextUPtr & requestContext)
{
    return ClientRequestAsyncOperation::End(asyncOperation, reply, requestContext);
}

void ClusterManagerReplica::OnProcessRequestComplete(
    Common::ActivityId const & activityId,
    MessageId const & messageId,
    AsyncOperationSPtr const & asyncOperation)
{
    MessageUPtr reply;
    RequestReceiverContextUPtr requestContext;
    auto error = EndProcessRequest(asyncOperation, reply, requestContext);

    if (error.IsSuccess())
    {
        CMEvents::Trace->RequestSucceeded(
            ReplicaActivityId(this->PartitionedReplicaId, activityId),
            messageId);

        reply->Headers.Replace(FabricActivityHeader(activityId));
        requestContext->Reply(std::move(reply));
    }
    else
    {
        CMEvents::Trace->RequestFailed(
            ReplicaActivityId(this->PartitionedReplicaId, activityId),
            messageId,
            error);

        requestContext->Reject(ToNamingGatewayReply(error));
    }
}

ErrorCode ClusterManagerReplica::ToNamingGatewayReply(Common::ErrorCode const & error)
{
    switch (error.ReadValue())
    {
        // Convert these error codes to an error that can be retried by the Naming Gateway
        //
        case ErrorCodeValue::ObjectClosed:
        case ErrorCodeValue::InvalidState:
            return ErrorCodeValue::ReconfigurationPending;

        default:
            return error;
    }
}

// *****************
// Utility functions
// *****************

TimeSpan ClusterManagerReplica::GetRandomizedOperationRetryDelay(ErrorCode const & error)
{
    return StoreTransaction::GetRandomizedOperationRetryDelay(
        error,
        ManagementConfig::GetConfig().MaxOperationRetryDelay);
}

// ************
// Test Helpers
// ************

void ClusterManagerReplica::Test_SetProcessingDelay(TimeSpan const & delay)
{
    WriteNoise(
        TraceComponent,
        "{0} test set processing delay to {1}",
        this->TraceId,
        delay);

    contextProcessingDelay_ = delay;
}

ErrorCode ClusterManagerReplica::Test_GetApplicationId(NamingUri const & appName, __out ServiceModelApplicationId & appId) const
{
    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId);

    auto appContext = make_unique<ApplicationContext>(appName);
    ErrorCode error = this->GetCompletedApplicationContext(storeTx, *appContext);

    if (error.IsSuccess())
    {
        storeTx.CommitReadOnly();

        appId = appContext->ApplicationId;
    }
    else
    {
        storeTx.Rollback();
    }

    return error;
}

ErrorCode ClusterManagerReplica::Test_GetComposeDeployment(wstring const & deploymentName, __out ServiceModelApplicationId & appId, __out wstring & typeName, __out wstring & typeVersion) const
{
    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId);

    auto composeContext = make_unique<ComposeDeploymentContext>(deploymentName);
    ErrorCode error = this->GetCompletedComposeDeploymentContext(storeTx, *composeContext);

    if (error.IsSuccess())
    {
        storeTx.CommitReadOnly();

        appId = composeContext->ApplicationId;

        typeName = composeContext->TypeName.Value;

        typeVersion = composeContext->TypeVersion.Value;
    }
    else
    {
        storeTx.Rollback();
    }

    return error;
}

ErrorCode ClusterManagerReplica::Test_GetComposeDeploymentUpgradeState(
    wstring const & deploymentName,
    __out ComposeDeploymentUpgradeState::Enum & state) const
{
    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId);

    auto upgradeContext = ComposeDeploymentUpgradeContext(deploymentName);
    ErrorCode error = storeTx.ReadExact(upgradeContext);
    if (!error.IsSuccess())
    {
        return error;
    }

    state = upgradeContext.CurrentStatus;
    return error;
}

ErrorCode ClusterManagerReplica::ScheduleNamingAsyncWork(
    BeginAsyncWorkCallback const & beginCallback,
    EndAsyncWorkCallback const & endCallback) const
{
    return namingJobQueue_->Enqueue(AsyncOperationWorkJobItem(beginCallback, endCallback));
}

ErrorCode ClusterManagerReplica::GetApplicationContexts(__out std::vector<ApplicationContext> & contexts) const
{
    return ReadPrefix<ApplicationContext>(Constants::StoreType_ApplicationContext, contexts);
}

ErrorCode ClusterManagerReplica::GetCompletedOrUpgradingApplicationContexts(
    __out std::vector<ApplicationContext> & contexts,
    bool isQuery) const
{
    ErrorCode error = ReadPrefix<ApplicationContext>(Constants::StoreType_ApplicationContext, contexts);

    if(!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} ReadPrefix failed with error:{1} while reading application contexts",
            this->TraceId,
            error);

        return error;
    }

    // Remove all the application contexts that are not completed
    contexts.erase(remove_if(contexts.begin(), contexts.end(),
        [this, isQuery](ApplicationContext const & appContext) -> bool
        {
            return (!isQuery && !this->CheckCompletedOrUpgradingApplicationContext(appContext).IsSuccess());
        }),
        contexts.end());

    return ErrorCodeValue::Success;
}

ErrorCode ClusterManagerReplica::GetComposeDeploymentContexts(
    __out vector<ComposeDeploymentContext> & contexts) const
{
    ErrorCode error = ReadPrefix<ComposeDeploymentContext>(Constants::StoreType_ComposeDeploymentContext, contexts);

    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} ReadPrefix failed with error: {1} while reading compose deployment contexts",
            this->TraceId,
            error);
    }

    return error;
}

// This method has an option to leave the StoreDataApplicationManifest value as null in the returned result.
// Set StoreDataApplicationManifest to null using parameter excludeManifest.
ErrorCode ClusterManagerReplica::GetApplicationManifestContexts(
    wstring const & keyPrefix,
    bool excludeManifest,
    __out vector<pair<ApplicationTypeContext, shared_ptr<StoreDataApplicationManifest>>> & result) const
{
    result.clear();

    vector<ApplicationTypeContext> appTypeContexts;
    auto error = ReadPrefix<ApplicationTypeContext>(Constants::StoreType_ApplicationTypeContext, keyPrefix, appTypeContexts);

    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} ReadPrefix failed with error: {1} while reading application type contexts for prefix {2}",
            this->TraceId,
            error,
            keyPrefix);

        return error;
    }

    // leave the manifest as empty
    if (excludeManifest)
    {
        for (auto & typeContext : appTypeContexts)
        {
            result.push_back(make_pair(move(typeContext), nullptr));
        }

        return ErrorCode::Success();
    }

    // return with the manifest
    else
    {
        vector<StoreDataApplicationManifest> appManifests;
        error = ReadPrefix<StoreDataApplicationManifest>(Constants::StoreType_ApplicationManifest, keyPrefix, appManifests);
        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0} ReadPrefix failed with error: {1} while reading StoreDataApplicationManifest for prefix {2}",
                this->TraceId,
                error,
                keyPrefix);

            return error;
        }

        for (auto & typeContext : appTypeContexts)
        {
            auto findIt = find_if(appManifests.begin(), appManifests.end(),
                [&typeContext](StoreDataApplicationManifest const & manifest)
            {
                return (typeContext.TypeName == manifest.TypeName && typeContext.TypeVersion == manifest.TypeVersion);
            });

            if (findIt != appManifests.end())
            {
                result.push_back(make_pair(move(typeContext), make_shared<StoreDataApplicationManifest>(move(*findIt))));
            }
            else
            {
                // There will be no persisted manifest data if provisioning fails
                result.push_back(make_pair(move(typeContext), nullptr));
            }
        }
        return error;
    }
}

ServiceModel::QueryResult ClusterManagerReplica::GetDeletedApplicationsList(QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId)
{
    vector<wstring> deletedAppIds;
    // Find the ApplicationId argument if it exists.
    wstring deletedAppsQueryObjString;

    if (queryArgs.TryGetValue(QueryResourceProperties::Application::ApplicationIds, deletedAppsQueryObjString))
    {
        InternalDeletedApplicationsQueryObject queryArg;
        auto error = JsonHelper::Deserialize<InternalDeletedApplicationsQueryObject>(queryArg, deletedAppsQueryObjString);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0}: Failed to deserialize DeletedApplicationsQueryObject: Error {1}",
                replicaActivityId.TraceId,
                error);
            return QueryResult(error);
        }
        if(!queryArg.ApplicationIds.empty())
        {
            auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId, replicaActivityId.ActivityId);
            for(auto iter = queryArg.ApplicationIds.begin(); iter != queryArg.ApplicationIds.end(); ++iter)
            {
                ServiceModelApplicationId appId(*iter);
                unique_ptr<StoreDataApplicationIdMap> appIdUPtr = make_unique<StoreDataApplicationIdMap>(appId);

                error = storeTx.ReadExact(*appIdUPtr);
                if(error.IsSuccess())
                {
                    WriteNoise(
                        TraceComponent,
                        "{0}:ApplicationId exists '{1}'",
                        replicaActivityId.TraceId,
                        appIdUPtr->ApplicationId);
                    continue;
                }
                else if (error.IsError(ErrorCodeValue::NotFound))
                {
                    WriteNoise(
                        TraceComponent,
                        "{0}: ApplicationId not found '{1}' Add to deleted apps list",
                        replicaActivityId.TraceId,
                        appIdUPtr->ApplicationId);
                    deletedAppIds.push_back(*iter);
                }
                else
                {
                    WriteInfo(
                        TraceComponent,
                        "{0}: Failed to get record for ApplicationId '{1}': Error {2}",
                        replicaActivityId.TraceId,
                        appIdUPtr->ApplicationId,
                        error);
                    return QueryResult(error);
                }
            }
            storeTx.CommitReadOnly();
        }
    }

    InternalDeletedApplicationsQueryObject queryResult(deletedAppIds);

    return QueryResult(make_unique<InternalDeletedApplicationsQueryObject>(move(queryResult)));
}

ServiceModel::QueryResult ClusterManagerReplica::GetProvisionedApplicationTypePackages(QueryArgumentMap const & queryArgs, Store::ReplicaActivityId const & replicaActivityId)
{
    wstring appTypeNameString;
    if (queryArgs.TryGetValue(QueryResourceProperties::ApplicationType::ApplicationTypeName, appTypeNameString))
    {
        vector<StoreDataServiceManifest> serviceManifestStoreData;

        // Windows filesystem is case-insensitive. We do not allow provisioning multiple
        // versions of the same application type/version with different casing concurrently,
        // but the user may unprovision/re-provision using different casing before Hosting
        // cleans up the previous version in the file system.
        //
        auto error = ReadPrefix<StoreDataServiceManifest>(Constants::StoreType_ServiceManifest, L"", serviceManifestStoreData);

        if (!error.IsSuccess())
        {
            return QueryResult(error);
        }

        // With one application type there could be multiple versions.
        vector<wstring> codepackages;
        vector<wstring> configpackages;
        vector<wstring> datapackages;
        set<wstring> serviceManifestMap;
        set<wstring> packagesMap;
        vector<wstring> serviceManifests;

        ServiceModelTypeName appTypeName(appTypeNameString);

        for (auto const & storeData : serviceManifestStoreData)
        {
            // Case insensitive
            //
            if (storeData.ApplicationTypeName != appTypeName)
            {
                continue;
            }

            for (auto const & manifest : storeData.ServiceManifests)
            {
                wstring smKey = wformatString("{0}:{1}", manifest.Name, manifest.Version);

                auto iter = serviceManifestMap.find(smKey);
                if(iter == serviceManifestMap.end())
                {
                    serviceManifestMap.insert(smKey);
                    serviceManifests.push_back(smKey);
                    CheckAddUniquePackages(manifest.Name, manifest.CodePackages, packagesMap, codepackages);
                    CheckAddUniquePackages(manifest.Name, manifest.ConfigPackages, packagesMap, configpackages);
                    CheckAddUniquePackages(manifest.Name, manifest.DataPackages, packagesMap, datapackages);
                }
            }
        }

        InternalProvisionedApplicationTypeQueryResult appTypeResources(appTypeNameString, serviceManifests, codepackages, configpackages, datapackages);
        return QueryResult(make_unique<InternalProvisionedApplicationTypeQueryResult>(move(appTypeResources)));
    }
    else
    {
        WriteWarning(
            TraceComponent,
            "{0}: Failed to get ApplicationTypeName from ArgumentsMap",
            replicaActivityId.TraceId);
        return QueryResult(ErrorCode(ErrorCodeValue::InvalidArgument));
    }
}

ErrorCode ClusterManagerReplica::Test_GetServiceContexts(__out std::vector<ServiceContext> & contexts) const
{
    return ReadPrefix<ServiceContext>(Constants::StoreType_ServiceContext, contexts);
}

ErrorCode ClusterManagerReplica::Test_GetComposeDeploymentContexts(
    __out vector<ComposeDeploymentContext> &composeApplicationContexts,
    __out vector<StoreDataComposeDeploymentInstance> &composeDeploymentInstances) const
{
    auto error = ReadPrefix<ComposeDeploymentContext>(Constants::StoreType_ComposeDeploymentContext, composeApplicationContexts);
    if (error.IsSuccess())
    {
        error = ReadPrefix<StoreDataComposeDeploymentInstance>(Constants::StoreType_ComposeDeploymentInstance, composeDeploymentInstances);
    }

    return error;
}

ErrorCode ClusterManagerReplica::Test_GetComposeDeploymentInstanceData(
    wstring const & deploymentName,
    __out vector<StoreDataComposeDeploymentInstance> &composeDeploymentInstances) const
{
    StoreDataComposeDeploymentInstance instance(deploymentName, ServiceModelVersion());

    return ReadPrefix<StoreDataComposeDeploymentInstance>(
        Constants::StoreType_ComposeDeploymentInstance,
        instance.GetDeploymentNameKeyPrefix(),
        composeDeploymentInstances);
}

ErrorCode ClusterManagerReplica::Test_GetServiceTemplates(__out std::vector<StoreDataServiceTemplate> & templates) const
{
    return ReadPrefix<StoreDataServiceTemplate>(Constants::StoreType_ServiceTemplate, templates);
}

ErrorCode ClusterManagerReplica::Test_GetServicePackages(__out std::vector<StoreDataServicePackage> & packages) const
{
    return ReadPrefix<StoreDataServicePackage>(Constants::StoreType_ServicePackage, packages);
}

HealthManagerReplicaSPtr ClusterManagerReplica::Test_GetHealthManagerReplica() const
{
    AcquireReadLock lock(lock_);
    return healthManagerReplica_;
}

bool ClusterManagerReplica::SetMockImageBuilderParameter(
    wstring const & appName,
    wstring && mockAppBuildPath,
    wstring && mockTypeName,
    wstring && mockTypeVersion)
{
    shared_ptr<TestAppTypeNameVersionGenerator> testGenerator = dynamic_pointer_cast<TestAppTypeNameVersionGenerator>(this->dockerComposeAppTypeVersionGenerator_);

    if (testGenerator == nullptr)
    {
        return false;
    }

    if (!testGenerator->SetMockType(appName, ServiceModelTypeName(move(mockTypeName)), ServiceModelVersion(move(mockTypeVersion)))) { return false; }

    TestDockerComposeImageBuilderProxy & testImageBuilder = static_cast<TestDockerComposeImageBuilderProxy &>(imageBuilder_);
    return testImageBuilder.SetMockAppBuildPath(appName, move(mockAppBuildPath));
}

void ClusterManagerReplica::EraseMockImageBuilderParameter(wstring const & appName)
{
    shared_ptr<TestAppTypeNameVersionGenerator> testGenerator = dynamic_pointer_cast<TestAppTypeNameVersionGenerator>(this->dockerComposeAppTypeVersionGenerator_);

    if (testGenerator == nullptr)
    {
        return;
    }

    testGenerator->EraseMockType(appName);

    TestDockerComposeImageBuilderProxy & testImageBuilder = static_cast<TestDockerComposeImageBuilderProxy &>(imageBuilder_);
    testImageBuilder.EraseMockAppBuildPath(appName);
}

template <class T>
ErrorCode ClusterManagerReplica::ReadPrefix(std::wstring const & storeType, __out std::vector<T> & contexts) const
{
    return ReadPrefix(storeType, L"", contexts);
}

template <class T>
ErrorCode ClusterManagerReplica::ReadPrefix(std::wstring const & storeType, std::wstring const & keyPrefix, __out std::vector<T> & contexts) const
{
    std::vector<T> result;

    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId);
    Common::ErrorCode error = storeTx.ReadPrefix(storeType, keyPrefix, result);

    if (error.IsSuccess())
    {
        storeTx.CommitReadOnly();
        contexts.swap(result);
    }
    else
    {
        storeTx.Rollback();
    }

    return error;
}

template <class T>
ErrorCode ClusterManagerReplica::ReadExact(__inout T & context) const
{
    auto storeTx = StoreTransaction::Create(this->ReplicatedStore, this->PartitionedReplicaId);
    Common::ErrorCode error = storeTx.ReadExact(context);

    if (error.IsSuccess())
    {
        storeTx.CommitReadOnly();
    }
    else
    {
        storeTx.Rollback();
    }

    return error;
}


// ****************
// Helper functions
// ****************

ErrorCode ClusterManagerReplica::GetSingleInstanceDeploymentContext(
    StoreTransaction const & storeTx,
    __inout SingleInstanceDeploymentContext & context) const
{
    ErrorCode error = storeTx.ReadExact(context);
    if (error.IsError(ErrorCodeValue::NotFound))
    {
        WriteInfo(
            TraceComponent,
            "{0} SingleInstanceDeployment {1} not found",
            this->TraceId,
            context.DeploymentName);
    }
    else if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} failed to read SingleInstaceDeployment {1} due to {2}",
            this->TraceId,
            context.DeploymentName,
            error);
    }
    return error;
}

ErrorCode ClusterManagerReplica::GetComposeDeploymentContext(
    StoreTransaction const & storeTx,
    __inout ComposeDeploymentContext & composeDeploymentContext) const
{
    ErrorCode error = storeTx.ReadExact(composeDeploymentContext);

    if (error.IsError(ErrorCodeValue::NotFound))
    {
        WriteInfo(
            TraceComponent,
            "{0} ComposeDeployment {1} not found",
            this->TraceId,
            composeDeploymentContext.DeploymentName);

        error = ErrorCodeValue::ComposeDeploymentNotFound;
    }
    else if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} failed to read ComposeDeploymentContext {1} due to {2}",
            this->TraceId,
            composeDeploymentContext.DeploymentName,
            error);
    }

    return error;
}

ErrorCode ClusterManagerReplica::GetApplicationContext(
    StoreTransaction const & storeTx,
    __out ApplicationContext & appContext) const
{
    ErrorCode error = storeTx.ReadExact(appContext);

    if (error.IsError(ErrorCodeValue::NotFound))
    {
        WriteInfo(
            TraceComponent,
            "{0} application {1} not found",
            this->TraceId,
            appContext.ApplicationName);

        error = ErrorCodeValue::ApplicationNotFound;
    }
    else if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} failed to read application context {1} due to {2}",
            this->TraceId,
            appContext.ApplicationName,
            error);
    }

    return error;
}

ErrorCode ClusterManagerReplica::CheckCompletedApplicationContext(ApplicationContext const & appContext) const
{
    if (appContext.IsUpgrading)
    {
        WriteInfo(
            TraceComponent,
            "{0} application {1} is being upgraded ({2})",
            this->TraceId,
            appContext.ApplicationName,
            appContext.Status);

        return ErrorCodeValue::ApplicationUpgradeInProgress;
    }
    else if (!appContext.IsComplete)
    {
        WriteNoise(
            TraceComponent,
            "{0} application {1} still being processed ({2})",
            this->TraceId,
            appContext.ApplicationName,
            appContext.Status);

        return ErrorCodeValue::CMRequestAlreadyProcessing;
    }

    return ErrorCodeValue::Success;
}

ErrorCode ClusterManagerReplica::CheckDeletableApplicationContext(ApplicationContext const & appContext) const
{
    if (appContext.IsUpgrading)
    {
        WriteInfo(
            TraceComponent,
            "{0} application {1} is being upgraded ({2})",
            this->TraceId,
            appContext.ApplicationName,
            appContext.Status);

        return ErrorCodeValue::ApplicationUpgradeInProgress;
    }
    else if (appContext.IsDeleting || appContext.IsFailed)
    {
        // Allow updating the timeout of a pending application delete operation
        return ErrorCodeValue::Success;
    }
    else if (!appContext.IsComplete)
    {
        WriteNoise(
            TraceComponent,
            "{0} application {1} still being processed ({2})",
            this->TraceId,
            appContext.ApplicationName,
            appContext.Status);

        return ErrorCodeValue::CMRequestAlreadyProcessing;
    }

    return ErrorCodeValue::Success;
}

ErrorCode ClusterManagerReplica::CheckCompletedOrUpgradingApplicationContext(ApplicationContext const & appContext) const
{
    if (appContext.IsUpgrading)
    {
        WriteInfo(
            TraceComponent,
            "{0} application {1} is being upgraded ({2})",
            this->TraceId,
            appContext.ApplicationName,
            appContext.Status);

        return ErrorCodeValue::Success;
    }
    else if (!appContext.IsComplete)
    {
        WriteNoise(
            TraceComponent,
            "{0} application {1} still being processed ({2})",
            this->TraceId,
            appContext.ApplicationName,
            appContext.Status);

        return ErrorCodeValue::CMRequestAlreadyProcessing;
    }

    return ErrorCodeValue::Success;
}

ErrorCode ClusterManagerReplica::CheckValidApplicationContext(ApplicationContext const & appContext) const
{
    if (appContext.IsUnknown)
    {
        WriteNoise(
            TraceComponent,
            "{0} application {1} is unknown ({2})",
            this->TraceId,
            appContext.ApplicationName,
            appContext.Status);

        return ErrorCodeValue::ApplicationNotFound;
    }
    return ErrorCodeValue::Success;
}

ErrorCode ClusterManagerReplica::CheckApplicationContextForServiceProcessing(ApplicationContext const & appContext) const
{
    if (appContext.IsUpgrading)
    {
        WriteInfo(
            TraceComponent,
            "{0} application {1} is being upgraded ({2})",
            this->TraceId,
            appContext.ApplicationName,
            appContext.Status);

        return ErrorCodeValue::Success;
    }
    else if (appContext.IsDeleting)
    {
        // Application deletion will wait for pending service contexts to finish
        // if they have already started processing.
        //
        // Allow creations and deletions to complete. DeleteApplication will delete all
        // remaining completed services.
        //
        WriteNoise(
            TraceComponent,
            "{0} application {1} is pending deletion",
            this->TraceId,
            appContext.ApplicationName);

        return ErrorCodeValue::Success;
    }
    else if (!appContext.IsComplete)
    {
        WriteNoise(
            TraceComponent,
            "{0} application {1} still being processed ({2})",
            this->TraceId,
            appContext.ApplicationName,
            appContext.Status);

        return ErrorCodeValue::CMRequestAlreadyProcessing;
    }

    return ErrorCodeValue::Success;
}

ErrorCode ClusterManagerReplica::GetCompletedApplicationContext(
    StoreTransaction const & storeTx,
    __out ApplicationContext & result) const
{
    ErrorCode error = this->GetApplicationContext(storeTx, result);

    if (error.IsSuccess())
    {
        error = this->CheckCompletedApplicationContext(result);
    }

    return error;
}

ErrorCode ClusterManagerReplica::GetDeletableApplicationContext(
    StoreTransaction const & storeTx,
    __out ApplicationContext & result) const
{
    ErrorCode error = this->GetApplicationContext(storeTx, result);

    if (error.IsSuccess())
    {
        error = this->CheckDeletableApplicationContext(result);
    }

    return error;
}

ErrorCode ClusterManagerReplica::GetCompletedOrUpgradingApplicationContext(
    StoreTransaction const & storeTx,
    __inout ApplicationContext & result) const
{
    ErrorCode error = this->GetApplicationContext(storeTx, result);

    if (error.IsSuccess())
    {
        error = this->CheckCompletedOrUpgradingApplicationContext(result);
    }

    return error;
}

ErrorCode ClusterManagerReplica::GetValidApplicationContext(
    StoreTransaction const & storeTx,
    __inout ApplicationContext & result) const
{
    ErrorCode error = this->GetApplicationContext(storeTx, result);

    if (error.IsSuccess())
    {
        error = this->CheckValidApplicationContext(result);
    }

    return error;
}

ErrorCode ClusterManagerReplica::GetApplicationContextForServiceProcessing(
    StoreTransaction const & storeTx,
    __out ApplicationContext & result) const
{
    ErrorCode error = this->GetApplicationContext(storeTx, result);

    if (error.IsSuccess())
    {
        error = this->CheckApplicationContextForServiceProcessing(result);
    }

    return error;
}

ErrorCode ClusterManagerReplica::ValidateServiceTypeDuringUpgrade(
    StoreTransaction const & storeTx,
    ApplicationContext const & appContext,
    ServiceModelTypeName const & serviceTypeName) const
{
    if (appContext.IsUpgrading)
    {
        // Verify that the service type is still supported after upgrade completes
        //
        auto upgradeContext = make_unique<ApplicationUpgradeContext>(appContext.ApplicationName);
        ErrorCode error = storeTx.ReadExact(*upgradeContext);

        if (!error.IsSuccess())
        {
            return error;
        }

        // While preparing the upgrade, we validate that service types are not removed
        // while they are still being used by some active service. Do not create any new services
        // until this validation is complete.
        //
        // Do not try to resolve the conflict by updating the upgrade context since a constant
        // stream of new service creations could effectively prevent the upgrade from starting.
        //
        if (upgradeContext->IsPreparingUpgrade)
        {
            return this->TraceAndGetErrorDetails(
                ErrorCodeValue::ApplicationUpgradeInProgress,
                wformatString("{0} {1}", GET_RC( Preparing_Upgrade ), appContext.ApplicationName));
        }

        if (upgradeContext->ContainsBlockedServiceType(serviceTypeName))
        {
            return this->TraceAndGetErrorDetails(
                ErrorCodeValue::ServiceTypeNotFound,
                wformatString("{0} {1}", GET_RC( Blocked_Service_Type ), serviceTypeName));
        }

        WriteNoise(
            TraceComponent,
            "{0} allowing create service against upgrade: {1}",
            this->TraceId,
            *upgradeContext);
    }

    return ErrorCodeValue::Success;
}

ErrorCode ClusterManagerReplica::GetStoreData(
    StoreTransaction const & storeTx,
    DeploymentType::Enum deploymentType,
    wstring const & deploymentName,
    ServiceModelVersion const & version,
    shared_ptr<StoreData> & storeData) const
{
    ErrorCode error = ErrorCode::Success();

    switch (deploymentType)
    {
    case DeploymentType::Enum::V2Application:
        storeData = make_shared<StoreDataSingleInstanceApplicationInstance>(
            deploymentName,
            version);
        error = storeTx.ReadExact(*storeData);
        if (!error.IsSuccess())
        {
            error = ErrorCode(error.ReadValue(), wformatString(GET_RC(Store_Data_Single_Instance_Application_Read_Failed), deploymentName, version, error.ReadValue()));
        }
        break;
    default:
            Assert::CodingError("Unknown single instance store data instance {0}", static_cast<int>(deploymentType));
    }

    return move(error);
}

template
ErrorCode ClusterManagerReplica::ClearVerifiedUpgradeDomains<StoreDataVerifiedUpgradeDomains>(
    StoreTransaction const & storeTx,
    __inout StoreDataVerifiedUpgradeDomains & storeData) const;

template
ErrorCode ClusterManagerReplica::ClearVerifiedUpgradeDomains<StoreDataVerifiedFabricUpgradeDomains>(
    StoreTransaction const & storeTx,
    __inout StoreDataVerifiedFabricUpgradeDomains & storeData) const;

template <class TStoreData>
ErrorCode ClusterManagerReplica::ClearVerifiedUpgradeDomains(
    StoreTransaction const & storeTx,
    __inout TStoreData & storeData) const
{
    auto error = storeTx.ReadExact(storeData);

    // Could also delete, but leave around to avoid generating tombstone
    // since this state is small and won't grow
    //
    if (error.IsSuccess())
    {
        storeData.Clear();

        error = storeTx.Update(storeData);
    }
    else if (error.IsError(ErrorCodeValue::NotFound))
    {
        error = ErrorCodeValue::Success;
    }

    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} could not clear verified domains: error={1}",
            this->TraceId,
            error);
    }

    return error;
}

TimeSpan ClusterManagerReplica::GetRollbackReplicaSetCheckTimeout(TimeSpan const timeout) const
{
    // ServiceModel\Management\ClusterManager\*UpgradeDescription.cpp
    // converts numeric_limits<DWORD>::max() to TimeSpan::MaxValue() when
    // parsing from the public API struct.
    //
    if (timeout == TimeSpan::MaxValue)
    {
        // Override for rollback to minimize the situations where rollback will itself
        // get stuck on safety checks.
        //
        return ManagementConfig::GetConfig().ReplicaSetCheckTimeoutRollbackOverride;
    }
    else
    {
        return timeout;
    }
}

ErrorCode ClusterManagerReplica::GetApplicationTypeContext(
    StoreTransaction const & storeTx,
    __inout ApplicationTypeContext & context) const
{
    auto error = storeTx.ReadExact(context);

    if (error.IsError(ErrorCodeValue::NotFound))
    {
        WriteInfo(
            TraceComponent,
            "{0} application type ({1},{2}) not found",
            this->TraceId,
            context.TypeName,
            context.TypeVersion);

        return ErrorCodeValue::ApplicationTypeNotFound;
    }
    else
    {
        return error;
    }
}

ErrorCode ClusterManagerReplica::GetCompletedApplicationTypeContext(
    StoreTransaction const & storeTx,
    __inout ApplicationTypeContext & context)
{
    // Create/Upgrade application is case-sensitive on application type name+version
    // since there are a lot of dependent database objects indexed by type name+version
    // (e.g. StoreDataApplicationManifest)
    //
    ErrorCode error = this->GetApplicationTypeContext(storeTx, context);

    if (error.IsSuccess() && !context.IsComplete)
    {
        WriteInfo(
            TraceComponent,
            "{0} application type {1}:{2} still being processed ({3})",
            this->TraceId,
            context.TypeName,
            context.TypeVersion,
            context.Status);

        error = ErrorCodeValue::CMRequestAlreadyProcessing;
    }

    return error;
}

ErrorCode ClusterManagerReplica::GetCompletedOrUpgradingComposeDeploymentContext(
    StoreTransaction const & storeTx,
    __inout ComposeDeploymentContext & context) const
{
    ErrorCode error = this->GetComposeDeploymentContext(storeTx, context);

    if (error.IsSuccess() && !context.IsComplete && !context.IsUpgrading)
    {
        WriteInfo(
            TraceComponent,
            "{0} compose deployment {1}:{2}:{3} still being processed (Status: {4}, Context rollout status: {5})",
            this->TraceId,
            context.DeploymentName,
            context.TypeName,
            context.TypeVersion,
            context.ComposeDeploymentStatus,
            context.Status);

        error = ErrorCodeValue::CMRequestAlreadyProcessing;
    }

    return error;
}

ErrorCode ClusterManagerReplica::GetCompletedComposeDeploymentContext(
    StoreTransaction const & storeTx,
    __inout ComposeDeploymentContext & context) const
{
    ErrorCode error = this->GetComposeDeploymentContext(storeTx, context);

    if (error.IsSuccess() && !context.IsComplete)
    {
        WriteInfo(
            TraceComponent,
            "{0} compose deployment {1}:{2}:{3} still being processed (Status: {4}, Context rollout status: {5})",
            this->TraceId,
            context.DeploymentName,
            context.TypeName,
            context.TypeVersion,
            context.ComposeDeploymentStatus,
            context.Status);

        error = ErrorCodeValue::CMRequestAlreadyProcessing;
    }

    return error;
}

ErrorCode ClusterManagerReplica::GetDeletableComposeDeploymentContext(
    StoreTransaction const & storeTx,
    __inout ComposeDeploymentContext & context) const
{
    ErrorCode error = this->GetComposeDeploymentContext(storeTx, context);

    if (error.IsSuccess())
    {
        if (context.IsUpgrading)
        {
            WriteInfo(
                TraceComponent,
                "{0} Compose deployment {1}:{2}:{3} still being upgraded ({4})",
                this->TraceId,
                context.DeploymentName,
                context.TypeName,
                context.TypeVersion,
                context.Status);

            // Use this error code directly as we are merging to single instance
            return ErrorCodeValue::SingleInstanceApplicationUpgradeInProgress;
        }
        else if (!context.IsComplete && !context.IsFailed)
        {
            WriteInfo(
                TraceComponent,
                "{0} Compose deployment {1}:{2}:{3} still being processed (Status: {4}, Context rollout status: {5})",
                this->TraceId,
                context.DeploymentName,
                context.TypeName,
                context.TypeVersion,
                context.ComposeDeploymentStatus,
                context.Status);

            return ErrorCodeValue::CMRequestAlreadyProcessing;
        }
    }

    return error;
}

ErrorCode ClusterManagerReplica::GetDeletableSingleInstanceDeploymentContext(
    StoreTransaction const & storeTx,
    __inout SingleInstanceDeploymentContext & context) const
{
    ErrorCode error = this->GetSingleInstanceDeploymentContext(storeTx, context);
    if (error.IsSuccess() && !context.IsComplete && !context.IsFailed)
    {
        WriteInfo(
            TraceComponent,
            "{0} Single Instance deployment {1}:{2}:{3} still being processed (Status: {4}, Context rollout status: {5})",
            this->TraceId,
            context.DeploymentName,
            context.TypeName,
            context.TypeVersion,
            context.DeploymentStatus,
            context.Status);

        error = ErrorCodeValue::CMRequestAlreadyProcessing;
    }

    return error;
}

ErrorCode ClusterManagerReplica::GetFabricVersionInfo(
    wstring const & codeFilepath,
    wstring const & clusterManifestFilepath,
    TimeSpan const timeout,
    __out FabricVersion & version) const
{
    if (isFabricImageBuilderOperationActive_.exchange(true))
    {
        return ErrorCodeValue::CMRequestAlreadyProcessing;
    }

    auto error = imageBuilder_.GetFabricVersionInfo(
        codeFilepath,
        clusterManifestFilepath,
        timeout,
        version);

    isFabricImageBuilderOperationActive_.store(false);

    return error;
}

ErrorCode ClusterManagerReplica::GetProvisionedFabricContext(
    StoreTransaction const & storeTx,
    Common::FabricVersion const & fabricVersion,
    __inout FabricProvisionContext & context,
    __out bool & containsVersion) const
{
    ErrorCode error = storeTx.ReadExact(context);

    if (error.IsError(ErrorCodeValue::NotFound))
    {
        return ErrorCodeValue::FabricVersionNotFound;
    }
    else if (error.IsSuccess())
    {
        containsVersion = false;

        if (context.IsComplete || context.IsFailed)
        {
            if (fabricVersion.CodeVersion.IsValid &&
                !context.ContainsCodeVersion(fabricVersion))
            {
                WriteInfo(
                    TraceComponent,
                    "{0} Fabric code version {1} not found: {2}",
                    this->TraceId,
                    fabricVersion.CodeVersion,
                    context.FormatProvisionedCodeVersions());
            }
            else if (fabricVersion.ConfigVersion.IsValid &&
                !context.ContainsConfigVersion(fabricVersion))
            {
                WriteInfo(
                    TraceComponent,
                    "{0} Fabric config version {1} not found: {2}",
                    this->TraceId,
                    fabricVersion.ConfigVersion,
                    context.FormatProvisionedConfigVersions());
            }
            else
            {
                containsVersion = true;
            }
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0} Fabric {1} still being processed ({2})",
                this->TraceId,
                context.IsProvision ? "provision": "unprovision",
                context.Status);

            error = ErrorCodeValue::CMRequestAlreadyProcessing;
        }
    }

    return error;
}

ErrorCode ClusterManagerReplica::TraceAndGetErrorDetails(
    ErrorCodeValue::Enum error,
    std::wstring && msg,
    std::wstring const & level) const
{
    return this->TraceAndGetErrorDetails(TraceComponent, error, move(msg), level);
}

ErrorCode ClusterManagerReplica::TraceAndGetErrorDetails(
    StringLiteral const & traceComponent,
    ErrorCodeValue::Enum error,
    std::wstring && msg,
    std::wstring const & level) const
{
    if (level == L"Info")
    {
        WriteInfo(
            traceComponent,
            "{0} {1} ({2})",
            this->TraceId,
            msg,
            error);
    }
    else if (level == L"Error")
    {
        WriteError(
            traceComponent,
            "{0} {1} ({2})",
            this->TraceId,
            msg,
            error);
    }
    else
    {
        WriteWarning(
            traceComponent,
            "{0} {1} ({2})",
            this->TraceId,
            msg,
            error);
    }

    return ErrorCode(error, move(msg));
}

wstring ClusterManagerReplica::ToWString(FABRIC_REPLICA_ROLE role)
{
    switch (role)
    {
        case FABRIC_REPLICA_ROLE_NONE:
            return L"None";
        case FABRIC_REPLICA_ROLE_PRIMARY:
            return L"Primary";
        case FABRIC_REPLICA_ROLE_IDLE_SECONDARY:
            return L"Idle Secondary";
        case FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY:
            return L"Active Secondary";
        default:
            return L"Unknown";
    }
}

// **************
// Helper classes
// **************

AsyncOperationSPtr ClusterManagerReplica::FinishAcceptDeleteContext(
    StoreTransaction && storeTx,
    ClientRequestSPtr && clientRequest,
    shared_ptr<RolloutContext> && context,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    ErrorCode error(ErrorCodeValue::Success);
    bool shouldCommit = true;

    if (!context->IsDeleting || context->IsConvertToForceDelete)
    {
        context->ReInitializeContext(*this, clientRequest);

        error = context->UpdateStatus(storeTx, RolloutStatus::DeletePending);
    }
    else
    {
        error = this->UpdateOperationTimeout(storeTx, *context, clientRequest->Timeout, shouldCommit);
    }

    return FinishAcceptRequest(
        move(storeTx),
        move(context),
        move(error),
        shouldCommit,
        timeout,
        callback,
        root);
}

AsyncOperationSPtr ClusterManagerReplica::FinishAcceptRequest(
    StoreTransaction && storeTx,
    shared_ptr<RolloutContext> && context,
    ErrorCode && error,
    bool shouldCommit,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    return this->FinishAcceptRequest(
        move(storeTx),
        move(context),
        move(error),
        shouldCommit,
        false,  // shouldCompleteClient
        timeout,
        callback,
        root);
}

AsyncOperationSPtr ClusterManagerReplica::FinishAcceptRequest(
    StoreTransaction && storeTx,
    shared_ptr<RolloutContext> && context,
    ErrorCode && error,
    bool shouldCommit,
    bool shouldCompleteClient,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    return AsyncOperation::CreateAndStart<FinishAcceptRequestAsyncOperation>(
        *this,
        move(storeTx),
        move(context),
        move(error),
        shouldCommit,
        shouldCompleteClient,
        timeout,
        callback,
        root);
}

bool ClusterManagerReplica::ShouldEnqueue(ErrorCode const & error)
{
    switch (error.ReadValue())
    {
    case ErrorCodeValue::Success:

    // Asynchronous replication can complete successfully after the timeout,
    // which will put the persisted context in the Pending state. We must
    // ensure that the context is enqueued for processing.
    //
    case ErrorCodeValue::CMRequestAlreadyProcessing:

    // Persisted context state indicates that work is still ongoing.
    // In these cases, we want to return either an error or some status
    // to the client before this work is actually complete (background
    // processing), so ensure the context is enqueued for processing
    // on these errors.
    //
    case ErrorCodeValue::ApplicationUpgradeInProgress:
    case ErrorCodeValue::FabricUpgradeInProgress:
    case ErrorCodeValue::InfrastructureTaskInProgress:
        return true;

    default:
        return false;
    }
}

AsyncOperationSPtr ClusterManagerReplica::RejectRequest(
    ClientRequestSPtr && clientRequest,
    ErrorCode && error,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    if (!error.IsSuccess())
    {
        CMEvents::Trace->RequestRejected(
            clientRequest->ReplicaActivityId,
            clientRequest->RequestMessageId,
            error);
    }

    return AsyncOperation::CreateAndStart<FinishAcceptRequestAsyncOperation>(
        *this,
        move(error),
        callback,
        root);
}

ErrorCode ClusterManagerReplica::RejectRequest(
    ClientRequestSPtr && clientRequest,
    ErrorCode && error)
{
    if (!error.IsSuccess())
    {
        CMEvents::Trace->RequestRejected(
            clientRequest->ReplicaActivityId,
            clientRequest->RequestMessageId,
            error);
    }

    return error;
}

AsyncOperationSPtr ClusterManagerReplica::RejectInvalidMessage(
    ClientRequestSPtr && clientRequest,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    return this->RejectRequest(move(clientRequest), ErrorCodeValue::InvalidMessage, callback, root);
}

// Allow a request retry from the client to update the operation timeout, but
// optimize the case when we are retrying internally due to CMRequestAlreadyProcessing
// since the operation timeout has not changed and commit is not necessary.
//
ErrorCode ClusterManagerReplica::UpdateOperationTimeout(
    StoreTransaction const & storeTx,
    __in RolloutContext & context,
    TimeSpan const timeout,
    __out bool & shouldCommit) const
{
    ErrorCode error(ErrorCodeValue::Success);

    shouldCommit = context.TrySetOperationTimeout(timeout);
    if (shouldCommit)
    {
        error = storeTx.Update(context);
    }

    if (error.IsSuccess())
    {
        error = ErrorCodeValue::CMRequestAlreadyProcessing;
    }

    return error;
}

ErrorCode ClusterManagerReplica::EndAcceptRequest(Common::AsyncOperationSPtr const & operation)
{
    return FinishAcceptRequestAsyncOperation::End(operation);
}

ClusterManagerReplica::FinishAcceptRequestAsyncOperation::FinishAcceptRequestAsyncOperation(
    __in ClusterManagerReplica & owner,
    ErrorCode && error,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : AsyncOperation(callback, root)
    , owner_(owner)
    , storeTx_(StoreTransaction::CreateInvalid(owner.ReplicatedStore, owner.PartitionedReplicaId))
    , context_()
    , error_(move(error))
    , shouldCommit_(false)
    , shouldCompleteClient_(false)
    , timeoutHelper_(TimeSpan::Zero)
    , innerAsyncOperationSPtr_()
{
}

ClusterManagerReplica::FinishAcceptRequestAsyncOperation::FinishAcceptRequestAsyncOperation(
    __in ClusterManagerReplica & owner,
    StoreTransaction && storeTx,
    shared_ptr<RolloutContext> && context,
    ErrorCode && error,
    bool shouldCommit,
    bool shouldCompleteClient,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : AsyncOperation(callback, root)
    , owner_(owner)
    , storeTx_(move(storeTx))
    , context_(move(context))
    , error_(move(error))
    , shouldCommit_(shouldCommit)
    , shouldCompleteClient_(shouldCompleteClient)
    , timeoutHelper_(timeout)
    , innerAsyncOperationSPtr_()
{
}

ErrorCode ClusterManagerReplica::FinishAcceptRequestAsyncOperation::End(AsyncOperationSPtr const & operation)
{
    return AsyncOperation::End<FinishAcceptRequestAsyncOperation>(operation)->Error;
}

void ClusterManagerReplica::FinishAcceptRequestAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    if (this->ShouldCommit())
    {
        auto operation = StoreTransaction::BeginCommit(
            move(storeTx_),
            *context_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnCommitComplete(operation, false); },
            thisSPtr);
        this->OnCommitComplete(operation, true);
    }
    else
    {
        this->FinishAcceptRequest(thisSPtr);
    }
}

void ClusterManagerReplica::FinishAcceptRequestAsyncOperation::OnCommitComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    ErrorCode innerError = StoreTransaction::EndCommit(operation);

    if (!innerError.IsSuccess())
    {
        error_ = innerError;
    }
    else if (shouldCompleteClient_ && context_)
    {
        context_->CompleteClientRequest();
    }

    this->FinishAcceptRequest(operation->Parent);
}

void ClusterManagerReplica::FinishAcceptRequestAsyncOperation::FinishAcceptRequest(AsyncOperationSPtr const & thisSPtr)
{
    if (this->ShouldEnqueue())
    {
        ErrorCode innerError = owner_.EnqueueRolloutContext(move(context_));

        // The RolloutManager will detect duplicate contexts.
        // CMRequestAlreadyProcessing is expected and indicates the context
        // was already enqueued so keep the original error (if any).
        //
        if (!innerError.IsSuccess() && (error_.IsSuccess() || !innerError.IsError(ErrorCodeValue::CMRequestAlreadyProcessing)))
        {
            error_ = innerError;
        }
    }

    this->TryComplete(thisSPtr, error_);
}

// CMRequestAlreadyProcessing indicates that the persisted version of the context is
// in the Pending state. If we made any updates to the in-memory version
// while accepting the request (e.g. updating the context's processing timeout),
// then the shouldCommit_ flag will be set.
//
bool ClusterManagerReplica::FinishAcceptRequestAsyncOperation::ShouldCommit()
{
    return (this->ShouldEnqueue() && shouldCommit_);
}

bool ClusterManagerReplica::FinishAcceptRequestAsyncOperation::ShouldEnqueue()
{
    switch (error_.ReadValue())
    {
        case ErrorCodeValue::Success:

        // Asynchronous replication can complete successfully after the timeout,
        // which will put the persisted context in the Pending state. We must
        // ensure that the context is enqueued for processing.
        //
        case ErrorCodeValue::CMRequestAlreadyProcessing:

        // Persisted context state indicates that work is still ongoing.
        // In these cases, we want to return either an error or some status
        // to the client before this work is actually complete (background
        // processing), so ensure the context is enqueued for processing
        // on these errors.
        //
        case ErrorCodeValue::ApplicationUpgradeInProgress:
        case ErrorCodeValue::FabricUpgradeInProgress:
        case ErrorCodeValue::InfrastructureTaskInProgress:
            return (context_ != nullptr);

        default:
            return false;

    }
}

void ClusterManagerReplica::CheckAddUniquePackages(
    wstring const & serviceManifestName,
    map<wstring, wstring> const & packages,
    __inout set<wstring> & packageMap,
    __inout vector<wstring> & uniquePackages)
{
    for(auto itPkg = packages.begin(); itPkg != packages.end(); itPkg++)
    {
        wstring pkgKey = wformatString("{0}:{1}:{2}", serviceManifestName, itPkg->first, itPkg->second);
        auto pkIt = packageMap.find(pkgKey);
        if(pkIt == packageMap.end())
        {
            uniquePackages.push_back(pkgKey);
            packageMap.insert(pkgKey);
        }
    }
}

bool ClusterManagerReplica::IsDnsServiceEnabled()
{
    //
    // Is DNS feature enabled?
    //
    DNS::DnsServiceConfig& config = DNS::DnsServiceConfig::GetConfig();
    return config.IsEnabled;
}


bool ClusterManagerReplica::IsPartitionedDnsQueryFeatureEnabled()
{
    DNS::DnsServiceConfig& config = DNS::DnsServiceConfig::GetConfig();
    return config.EnablePartitionedQuery;
}


Common::ErrorCode ClusterManagerReplica::ValidateServiceDnsName(std::wstring const & serviceDnsName)
{
    DNS::DNSNAME_STATUS status = DNS::IsDnsNameValid(serviceDnsName.c_str());
    if ((status != DNS::DNSNAME_VALID) && (status != DNS::DNSNAME_NON_RFC_NAME))
    {
        return ErrorCode::FromWin32Error(status);
    }

    return ValidateServiceDnsNameForPartitionedQueryCompliance(serviceDnsName);
}

Common::ErrorCode ClusterManagerReplica::ValidateServiceDnsNameForPartitionedQueryCompliance(std::wstring const & serviceDnsName)
{
    // Check if partitioned dns query feature is enabled or not? 
    // If not, return early.
    // If yes, we need to make sure original DNS name doesn't meet partitioned dns query format.
    if (!IsPartitionedDnsQueryFeatureEnabled())
    {
        return ErrorCodeValue::Success;
    }

    wstring const & prefix = DNS::DnsServiceConfig::GetConfig().PartitionPrefix;
    wstring const & suffix = DNS::DnsServiceConfig::GetConfig().PartitionSuffix;

    if (prefix.empty())
    {
        return ErrorCodeValue::Success;
    }

    wstring dnsName;
    wstring labelDelimiter(L".");

    // Locate first Label
    if (StringUtility::Contains<wstring>(serviceDnsName, labelDelimiter))
    {
        std::vector<wstring> labels;
        StringUtility::SplitOnString(serviceDnsName, labels, labelDelimiter);
        dnsName = labels[0]; //Starting with dot is taken care off by ValidDnsName check
    }
    else
    {
        dnsName = serviceDnsName;
    }

    // First label has to end with suffix, if suffix is non-empty.
    size_t suffixPos = dnsName.length();
    if (!suffix.empty())
    {
        suffixPos = dnsName.rfind(suffix);
        if ((suffixPos == wstring::npos) || ((suffixPos + suffix.length()) != dnsName.length()))
        {
            return ErrorCodeValue::Success;
        }
    }
 
    size_t prefixPos = dnsName.rfind(prefix);
    if (prefixPos != wstring::npos)
    {
        // check for partition name of non-zero length.
        if ((prefixPos != 0) && (suffixPos > (prefixPos + prefix.length())))
        {
            auto msg = wformatString(GET_CM_RC(InvalidDnsName_PatitionedQueryFormatInCompliance), serviceDnsName);
            return ErrorCode(ErrorCodeValue::InvalidDnsName, move(msg));
        }
    }

    return ErrorCodeValue::Success;
}

ErrorCode ClusterManagerReplica::ValidateAndGetComposeDeploymentName(wstring const &applicationName, __out NamingUri &appNameUri)
{
    wstring applicationNameAuthority;
    NamingUri::FabricNameToId(applicationName, applicationNameAuthority);
    if (StringUtility::Contains<wstring>(applicationNameAuthority, L"/"))
    {
        return this->TraceAndGetErrorDetails(
                ErrorCodeValue::InvalidNameUri,
                wformatString(GET_RC( Compose_App_Name_Invalid ), applicationName));
    }

    if (!NamingUri::TryParse(applicationName, appNameUri))
    {
        return ErrorCodeValue::InvalidNameUri;
    }

    return ErrorCodeValue::Success;
}

wstring ClusterManagerReplica::GetDeploymentNameFromAppName(wstring const &applicationName)
{
    wstring applicationNameAuthority;
    NamingUri::FabricNameToId(applicationName, applicationNameAuthority);
    return applicationNameAuthority;
}



ErrorCode ClusterManagerReplica::GetApplicationResourceQueryResult(
    Store::StoreTransaction const & storeTx,
    SingleInstanceDeploymentContext const &deploymentContext,
    Store::ReplicaActivityId const & replicaActivityId,
    ModelV2::ApplicationDescriptionQueryResult &result)
{
    auto storeData = make_shared<StoreDataSingleInstanceApplicationInstance>(
        deploymentContext.DeploymentName,
        deploymentContext.TypeVersion);

    auto error = storeTx.ReadExact(*storeData);
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0}: Error while reading storedata for application resource {1} error = {2}",
            replicaActivityId.TraceId,
            deploymentContext.ApplicationId,
            error);
        return error;
    }

    vector<ServiceContext> serviceContexts;
    error = ServiceContext::ReadApplicationServices(
        storeTx,
        deploymentContext.ApplicationId,
         serviceContexts);

    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0}: Error while reading App: {1} ServiceContexts, error = {2}",
            replicaActivityId.TraceId,
            deploymentContext.ApplicationId,
            error);
    }

    vector<wstring> serviceNames;
    for (auto const & service : serviceContexts)
    {
        serviceNames.push_back(service.ServiceName.Value);
    }

    SingleInstanceDeploymentQueryResult baseResult = deploymentContext.ToQueryResult();

    result = ModelV2::ApplicationDescriptionQueryResult(
        move(baseResult),
        storeData->ApplicationDescription.Description,
        move(serviceNames));

    return ErrorCodeValue::Success;
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Naming;
using namespace Reliability;
using namespace ServiceModel;
using namespace Store;
using namespace Transport;
using namespace Management::ClusterManager;
using namespace Management::NetworkInventoryManager;

StringLiteral const ApplicationUpgradeParallelRetryableAsyncOperationTraceComponent("ApplicationUpgradeParallelRetryableAsyncOperationTraceComponent");

//
// *** class used to execute parallel operations with retries
//
class ProcessApplicationUpgradeContextAsyncOperation::ApplicationUpgradeParallelRetryableAsyncOperation
    : public ProcessRolloutContextAsyncOperationBase::ParallelRetryableAsyncOperation
{
    DENY_COPY(ApplicationUpgradeParallelRetryableAsyncOperation);

public:
    ApplicationUpgradeParallelRetryableAsyncOperation(
        __in ProcessApplicationUpgradeContextAsyncOperation & owner,
        __in RolloutContext & context,
        long count,
        ParallelOperationCallback const & operationCallback,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
        : ProcessRolloutContextAsyncOperationBase::ParallelRetryableAsyncOperation(
            owner, 
            context, 
            count, 
            //
            // Wait for all pending operations to complete when encountering a fatal error
            // since this will trigger corresponding rollbacks for all parallel operations. 
            // This will minimize the chances of a rollback failing because the corresponding
            // rollforward operation hasn't executed yet.
            //
            // If we encounter issues where some rollforward operations get dropped when
            // rollback occurs (in practice), then we can add logic to perform another
            // diff to determine which operations need to actually be rolled back.
            // The only consequence of not performing the more complex pre-rollback diff
            // is that the upgrade may fail rather than rolling back cleanly.
            // This is mainly a concern for updating default services (repartitioning in
            // particular), so the rollforward UD walk will not have happened yet in this case
            // because updating default services happens before the UD walk.
            //
            true,
            operationCallback, 
            callback, 
            parent)
        , owner_(owner)
    {
    }

protected:
    virtual bool IsInnerErrorRetryable(Common::ErrorCode const & error) const override
    {
        return owner_.IsRetryable(error) && !owner_.IsServiceOperationFatalError(error);
    }

    virtual void ScheduleRetryOnInnerOperationFailed(
        AsyncOperationSPtr const & thisSPtr,
        RetryCallback const & callback,
        Common::ErrorCode && error) override
    {
        ErrorCode innerError = owner_.RefreshUpgradeContext();

        if (!innerError.IsSuccess())
        {
            this->TryComplete(thisSPtr, move(innerError));
            return;
        }
        owner_.TryScheduleRetry(error, thisSPtr, [callback, thisSPtr](AsyncOperationSPtr const &) { callback(thisSPtr); });
    }

private:
    ProcessApplicationUpgradeContextAsyncOperation & owner_;
};

//
// *** class InitializeUpgradeAsyncOperation;
//

ProcessApplicationUpgradeContextAsyncOperation::InitializeUpgradeAsyncOperation::InitializeUpgradeAsyncOperation(
    __in ProcessApplicationUpgradeContextAsyncOperation & owner,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent)
    , upgradeContextLock_()
    , owner_(owner)
{
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::InitializeUpgradeAsyncOperation::End(AsyncOperationSPtr const & operation)
{
    return AsyncOperation::End<InitializeUpgradeAsyncOperation>(operation)->Error;
}

void ProcessApplicationUpgradeContextAsyncOperation::InitializeUpgradeAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    this->InitializeUpgrade(thisSPtr);
}

void ProcessApplicationUpgradeContextAsyncOperation::InitializeUpgradeAsyncOperation::InitializeUpgrade(AsyncOperationSPtr const & thisSPtr)
{
    // The sequence of loading matters because fields are populated for use in subsequent load calls
    //
    auto error = owner_.LoadApplicationContext();
    if (!error.IsSuccess()) { this->TryComplete(thisSPtr, error); return; }

    error = owner_.RefreshUpgradeContext();
    if (!error.IsSuccess()) { this->TryComplete(thisSPtr, error); return; }

    auto operation = owner_.BeginLoadApplicationDescriptions(
        [&](AsyncOperationSPtr const & operation) { this->OnLoadApplicationDescriptionsComplete(operation, false); },
        thisSPtr);
    this->OnLoadApplicationDescriptionsComplete(operation, true);
}

void ProcessApplicationUpgradeContextAsyncOperation::InitializeUpgradeAsyncOperation::OnLoadApplicationDescriptionsComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    auto error = owner_.EndLoadApplicationDescriptions(operation);

    // This is a validation error thrown from IB proxy
    if (error.IsError(ErrorCodeValue::DnsServiceNotFound))
    {
        error = ErrorCodeValue::ApplicationUpgradeValidationError;
    }

    if (!error.IsSuccess()) { this->TryComplete(thisSPtr, error); return; }

    this->ValidateServices(thisSPtr);
}

void ProcessApplicationUpgradeContextAsyncOperation::InitializeUpgradeAsyncOperation::ValidateServices(AsyncOperationSPtr const & thisSPtr)
{
    auto error = owner_.LoadUpgradePolicies();
    if (!error.IsSuccess()) { this->TryComplete(thisSPtr, error); return; }

    // Only need to perform network validation and active service validation
    // while initially preparing the upgrade (not on failover)
    //
    if (!owner_.UpgradeContext.IsPreparingUpgrade)
    {
        this->TryComplete(thisSPtr, error);
    }
    else
    {
        if(Management::NetworkInventoryManager::NetworkInventoryManagerConfig::IsNetworkInventoryManagerEnabled())
        {
            this->ValidateNetworks(thisSPtr);
        }
        else
        {
            this->ValidateActiveDefaultServices(thisSPtr);
        }
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::InitializeUpgradeAsyncOperation::ValidateNetworks(AsyncOperationSPtr const & thisSPtr)
{
    // When target version is referring to non-existing container networks, we should directly fail the upgrade request by the client.
    // This is achieved by sending a request to NIM as soon as image builder upgradeApplication operation completes, when the application's network references in target version is parsed.
    // The validation is only needed when the target version refers to any container network.
    // It requires a new message exchange between CM and FM since existing workflow only sends the first message to FM after upgrade is accepted.
    if (owner_.IsNetworkValidationNeeded())
    {
        WriteNoise(
            owner_.TraceComponent,
            "{0} validating network references",
            owner_.TraceId);

        auto operation = owner_.BeginValidateNetworks(
            [&](AsyncOperationSPtr const & operation) { this->OnValidateNetworksComplete(operation, false); },
            thisSPtr);

        this->OnValidateNetworksComplete(operation, true);
    }
    else
    {
        this->ValidateActiveDefaultServices(thisSPtr);
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::InitializeUpgradeAsyncOperation::OnValidateNetworksComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    auto error = owner_.EndValidateNetworks(operation);
    if (!error.IsSuccess()) { this->TryComplete(thisSPtr, error); return; }

    this->ValidateActiveDefaultServices(thisSPtr);
}

void ProcessApplicationUpgradeContextAsyncOperation::InitializeUpgradeAsyncOperation::ValidateActiveDefaultServices(AsyncOperationSPtr const & thisSPtr)
{
    auto error = owner_.LoadActiveServices();
    if (!error.IsSuccess()) { this->TryComplete(thisSPtr, error); return; }

    error = owner_.LoadDefaultServices();
    if (!error.IsSuccess()) { this->TryComplete(thisSPtr, error); return; }

    error = owner_.ValidateRemovedServiceTypes();
    if (!error.IsSuccess()) { this->TryComplete(thisSPtr, error); return; }

    // Commit upgrade context before invoking retryable async operation which would potentially refresh store data.
    auto storeTx = owner_.CreateTransaction();
    error = storeTx.Update(owner_.UpgradeContext);

    if (error.IsSuccess())
    {
        auto operation = owner_.BeginCommitUpgradeContext(
            move(storeTx),
            [this](AsyncOperationSPtr const & operation) { this->OnValidateServicesComplete(operation, false); },
            thisSPtr);
        this->OnValidateServicesComplete(operation, true);
    }
    else
    {
        WriteWarning(
            owner_.TraceComponent,
            "{0} could not update validated upgrade context due to {1}",
            owner_.TraceId,
            error);
        this->TryComplete(thisSPtr, move(error));
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::InitializeUpgradeAsyncOperation::OnValidateServicesComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) return;

    ErrorCode error = owner_.EndCommitUpgradeContext(operation);
    auto const & thisSPtr = operation->Parent;

    if (!error.IsSuccess())
    {
        WriteWarning(
            owner_.TraceComponent,
            "{0} could not commit validated upgrade context due to {1}",
            owner_.TraceId,
            error);
        this->TryComplete(thisSPtr, move(error));
    }
    else
    {
        this->LoadActiveDefaultServiceUpdates(thisSPtr);
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::InitializeUpgradeAsyncOperation::LoadActiveDefaultServiceUpdates(AsyncOperationSPtr const & thisSPtr)
{
    WriteNoise(
        owner_.TraceComponent,
        "{0} loading active default service descriptions",
        owner_.TraceId);

    owner_.UpgradeContext.ClearDefaultServiceUpdateDescriptions();
    owner_.UpgradeContext.ClearRollbackDefaultServiceUpdateDescriptions();

    auto operation = AsyncOperation::CreateAndStart<ApplicationUpgradeParallelRetryableAsyncOperation>(
        owner_,
        owner_.UpgradeContext,
        static_cast<long>(owner_.activeDefaultServiceDescriptions_.size()),
        [this](AsyncOperationSPtr const & parallelAsyncOperation, size_t operationIndex, ParallelOperationsCompletedCallback const & callback)->ErrorCode
        {
            return this->ScheduleLoadActiveDefaultService(parallelAsyncOperation, operationIndex, callback);
        },
        [this](AsyncOperationSPtr const & operation) { this->OnLoadActiveDefaultServiceUpdatesComplete(operation, false); },
        thisSPtr);
    this->OnLoadActiveDefaultServiceUpdatesComplete(operation, true);
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::InitializeUpgradeAsyncOperation::ScheduleLoadActiveDefaultService(
    AsyncOperationSPtr const & parallelAsyncOperation,
    size_t operationIndex,
    ParallelOperationsCompletedCallback const & callback)
{
    return owner_.Replica.ScheduleNamingAsyncWork(
        [parallelAsyncOperation, this, operationIndex](AsyncCallback const & jobQueueCallback)
        {
        return this->BeginLoadActiveDefaultService(parallelAsyncOperation, operationIndex, jobQueueCallback);
        },
        [this, operationIndex, callback](AsyncOperationSPtr const & operation) { this->EndLoadActiveDefaultService(operation, operationIndex, callback); });
}

AsyncOperationSPtr ProcessApplicationUpgradeContextAsyncOperation::InitializeUpgradeAsyncOperation::BeginLoadActiveDefaultService(
    AsyncOperationSPtr const & parallelAsyncOperation,
    size_t operationIndex,
    AsyncCallback const & jobQueueCallback)
{
    ASSERT_IF(
        operationIndex >= owner_.activeDefaultServiceDescriptions_.size(),
        "LoadActiveDefaultService {0}: operationIndex {1} is out of bounds, {2} active default services",
        owner_.UpgradeContext.ActivityId,
        operationIndex,
        owner_.activeDefaultServiceDescriptions_.size());

    NamingUri serviceName = StringToNamingUri(owner_.activeDefaultServiceDescriptions_[operationIndex].Service.Name);

    Common::ActivityId innerActivityId(owner_.UpgradeContext.ActivityId, static_cast<uint64>(operationIndex));
    return owner_.Client.BeginGetServiceDescription(
        serviceName,
        innerActivityId,
        owner_.GetCommunicationTimeout(),
        jobQueueCallback,
        parallelAsyncOperation);
}

void ProcessApplicationUpgradeContextAsyncOperation::InitializeUpgradeAsyncOperation::EndLoadActiveDefaultService(
    AsyncOperationSPtr const & operation,
    size_t operationIndex,
    ParallelOperationsCompletedCallback const & callback)
{
    PartitionedServiceDescriptor activePSD;
    ErrorCode error = owner_.Client.EndGetServiceDescription(operation, activePSD);
    if (error.IsSuccess())
    {
        // Service description is updated based on active
        //
        auto & targetDescription = owner_.activeDefaultServiceDescriptions_[operationIndex];

        shared_ptr<ServiceUpdateDescription> serviceUpdateDescription;
        shared_ptr<ServiceUpdateDescription> rollbackServiceUpdateDescription;

        error = ServiceUpdateDescription::TryDiffForUpgrade(
            targetDescription, 
            activePSD, 
            serviceUpdateDescription,  // out 
            rollbackServiceUpdateDescription); // out

        if (!error.IsSuccess())
        {
            WriteWarning(
                owner_.TraceComponent, 
                "{0} default service update failed ({1}): error={2} {3}",
                owner_.TraceId,
                activePSD.Service.Name,
                error,
                error.Message);
        }
        else if (serviceUpdateDescription.get() != nullptr && rollbackServiceUpdateDescription.get() != nullptr)
        {
            WriteInfo(
                owner_.TraceComponent,
                "{0} modified default service: {1} -> {2}",
                owner_.TraceId,
                activePSD,
                targetDescription);

            AcquireWriteLock lock(upgradeContextLock_);

            owner_.UpgradeContext.InsertDefaultServiceUpdateDescriptions(StringToNamingUri(activePSD.Service.Name), move(*serviceUpdateDescription));
            owner_.UpgradeContext.InsertRollbackDefaultServiceUpdateDescriptions(StringToNamingUri(activePSD.Service.Name), move(*rollbackServiceUpdateDescription));
        }
    }
    else if (error.IsError(ErrorCodeValue::UserServiceNotFound) || error.IsError(ErrorCodeValue::NameNotFound))
    {
        error = owner_.TraceAndGetErrorDetails(ErrorCodeValue::ApplicationUpgradeValidationError, wformatString(GET_RC(Upgrade_Load_Service_Description), error));
    }

    callback(operation->Parent, error);
}

void ProcessApplicationUpgradeContextAsyncOperation::InitializeUpgradeAsyncOperation::OnLoadActiveDefaultServiceUpdatesComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = AsyncOperation::End<ApplicationUpgradeParallelRetryableAsyncOperation>(operation)->Error;

    auto const & thisSPtr = operation->Parent;

    if (error.IsSuccess())
    {
        this->InitializeHealthPolicies(thisSPtr);
    }
    else if (error.IsError(ErrorCodeValue::ApplicationUpgradeValidationError))
    {
        this->TryComplete(thisSPtr, error);
    }
    else
    {
        WriteInfo(
            owner_.TraceComponent,
            "{0} retrying to load active default service descriptions due to {1}",
            owner_.TraceId,
            error);

        owner_.StartTimer(
            thisSPtr,
            [this, thisSPtr](AsyncOperationSPtr const &) { this->LoadActiveDefaultServiceUpdates(thisSPtr); },
            owner_.Replica.GetRandomizedOperationRetryDelay(error));
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::InitializeUpgradeAsyncOperation::InitializeHealthPolicies(AsyncOperationSPtr const & thisSPtr)
{
    auto operation = owner_.BeginUpdateHealthPolicyForHealthCheck(
        [this](AsyncOperationSPtr const & operation) { this->OnUpdateHealthPolicyComplete(operation, false); },
        thisSPtr);
    this->OnUpdateHealthPolicyComplete(operation, true);
}

void ProcessApplicationUpgradeContextAsyncOperation::InitializeUpgradeAsyncOperation::OnUpdateHealthPolicyComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = owner_.EndUpdateHealthPolicy(operation);

    owner_.SetCommonContextDataForUpgrade();

    this->TryComplete(operation->Parent, error);
}

//
// *** class ImageBuilderRollbackAsyncOperation;
//

class ProcessApplicationUpgradeContextAsyncOperation::ImageBuilderRollbackAsyncOperation : public AsyncOperation
{
public:
    ImageBuilderRollbackAsyncOperation(
        ProcessApplicationUpgradeContextAsyncOperation & owner,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<ImageBuilderRollbackAsyncOperation>(operation)->Error;
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        this->DoImageBuilderRollback(thisSPtr);
    }

private:

    void DoImageBuilderRollback(AsyncOperationSPtr const & thisSPtr)
    {
        //
        // When rolling back from V2 -> V1, we internally generate another
        // application instance version. So the upgrade sequence becomes
        // R1 -> R2 -> R3 where R2 -> R3 is the rollback using internal
        // RolloutVersions.
        //
        // This is necessary to avoid conflicts in generating package
        // files for subsequent upgrades. If we did R1 -> R2 -> R1, then
        // a subsequent upgrade would again be R1 -> R2' and any package files
        // previously generated by ImageBuilder for R2 would be overwritten
        // by R2'. However, the package files for R2 (with potentially different
        // content) may already be in use.
        //
        auto operation = owner_.ImageBuilder.BeginUpgradeApplication(
            owner_.ActivityId,
            owner_.appContextUPtr_->ApplicationName,
            owner_.appContextUPtr_->ApplicationId,
            owner_.appContextUPtr_->TypeName,
            owner_.appContextUPtr_->TypeVersion,
            owner_.targetApplication_.Application.Version,
            owner_.appContextUPtr_->Parameters,
            owner_.GetImageBuilderTimeout(),
            [&](AsyncOperationSPtr const & operation) { this->OnUpgradeApplicationComplete(operation, false); },
            thisSPtr);
        this->OnUpgradeApplicationComplete(operation, true);
    }

    void OnUpgradeApplicationComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        DigestedApplicationDescription unusedCurrentApp;
        auto error = owner_.ImageBuilder.EndUpgradeApplication(
            operation,
            unusedCurrentApp, // out
            owner_.currentApplication_); // out: Use the updated RolloutVersions in "rollback" request (which must also be persisted)

        this->TryComplete(operation->Parent, error);
    }

    ProcessApplicationUpgradeContextAsyncOperation & owner_;
};

// *** Rollforward
//
// Rollforward occurs in the following phases. Once a phase completes successfully,
// upgrade proceeds onto the next phase.
//
// 1) Upgrade is prepared by calculating the delta via ImageBuilder and persisting
//    the necessary metadata to continue upgrade if failover occurs
//      - This phase also includes some upgrade validation checks
//      - Active description of default services are persisted for rollback
//
// 2) Update default services
//      - Differ based on active service description with target manifest.
//      - Values not defined in the manifest are considered as default values.
//      - Design doc: https://microsoft.sharepoint.com/teams/WindowsFabric/_layouts/OneNote.aspx?id=%2Fteams%2FWindowsFabric%2FNotebooks%2FWindows%20Fabric%20Design%20Notes&wd=target%28ClusterManager.one%7CBE48945C-624A-4B0C-AF5C-4F4EF0315951%2FRDBug%206528112%20%3A%20%5BDCR%5D%20Changes%20to%20default%20service%20definitions%7CDC5CAA6D-1EEB-456A-BA17-21C24026ACF0%2F%29
//
// 3) Upgrade requests are sent to FM
//      - The rollforward request message is loaded during upgrade preparation
//      - This phase is skipped if there are no active services to upgrade
//      - This is the only phase that can be interrupted to start a different upgrade
//
// 4) Create new default services calculated during upgrade preparation
//
// 5) Delete removed default services calculated during upgrade preparation (can't recover)
//
// 6) Persist application version, service templates, and service packages to complete upgrade
//
//
// *** Rollback
//
// Rollback occurs in the following phases. Again, rollback only proceeds onto the
// next phase once the current phase completes sucessfully.
//
// 1) Prepare the rollback application instance via ImageBuilder and persist rollback state
//
// 2) Ensure that new default services calculated during upgrade preparation have been deleted
//
// 3) Send rollback requests to FM
//      - The rollback request message is loaded during rollback preparation
//      - This phase is skipped if the rollforward process did not send any upgrade requests
//
// 4) Rollback updated default services
//      - Rollback default service descriptions to the status before upgrade
//
// 5) Persist application version to complete rollback
//      - Note that there is no persisted state to roll back, but
//        all service packages must be persisted with new RolloutVersions
//
ProcessApplicationUpgradeContextAsyncOperation::ProcessApplicationUpgradeContextAsyncOperation(
    __in RolloutManager & rolloutManager,
    __in ApplicationUpgradeContext & upgradeContext,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : ProcessUpgradeContextAsyncOperationBase(
        rolloutManager,
        upgradeContext,
        "ApplicationUpgradeContext",
        TimeSpan::MaxValue,
        callback, 
        root)
    , appContextUPtr_()
    , currentApplication_()
    , targetApplication_()
    , removedTypes_()
    , activeServiceTypes_()
    , activeDefaultServiceDescriptions_()
    , allActiveServices_()
    , manifestHealthPolicy_()
{
}

ProcessApplicationUpgradeContextAsyncOperation::ProcessApplicationUpgradeContextAsyncOperation(
    __in RolloutManager & rolloutManager,
    __in ApplicationUpgradeContext & upgradeContext,
    string const & traceComponent,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : ProcessUpgradeContextAsyncOperationBase(
        rolloutManager,
        upgradeContext,
        traceComponent,
        TimeSpan::MaxValue,
        callback,
        root)
    , appContextUPtr_()
    , currentApplication_()
    , targetApplication_()
    , removedTypes_()
    , activeServiceTypes_()
    , activeDefaultServiceDescriptions_()
    , allActiveServices_()
    , manifestHealthPolicy_()
{
}

void ProcessApplicationUpgradeContextAsyncOperation::OnStart(AsyncOperationSPtr const & operation)
{
    this->PerfCounters.NumberOfAppUpgrades.Increment();
    ProcessUpgradeContextAsyncOperationBase<ApplicationUpgradeContext, Reliability::UpgradeApplicationRequestMessageBody>::OnStart(operation);
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::End(AsyncOperationSPtr const & operation)
{
    return AsyncOperation::End<ProcessApplicationUpgradeContextAsyncOperation>(operation)->Error;
}

Reliability::RSMessage const & ProcessApplicationUpgradeContextAsyncOperation::GetUpgradeMessageTemplate()
{
    return Reliability::RSMessage::GetApplicationUpgradeRequest();
}

TimeSpan ProcessApplicationUpgradeContextAsyncOperation::GetUpgradeStatusPollInterval()
{
    return ManagementConfig::GetConfig().UpgradeStatusPollInterval;
}

TimeSpan ProcessApplicationUpgradeContextAsyncOperation::GetHealthCheckInterval()
{
    return ManagementConfig::GetConfig().UpgradeHealthCheckInterval;
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::LoadRollforwardMessageBody(
    __out UpgradeApplicationRequestMessageBody & requestBody)
{
    vector<StoreDataServicePackage> affectedTypes;
    vector<StoreDataServicePackage> movedTypes;
    DigestedApplicationDescription::CodePackageNameMap codePackages;
    bool shouldRestartPackages;

    auto error = currentApplication_.ComputeAffectedServiceTypes(
        targetApplication_, 
        affectedTypes, // out
        movedTypes, // out
        codePackages, // out
        shouldRestartPackages); // out

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent, 
            "{0} could not compute affected service types due to {1}",
            this->TraceId,
            error);

        return error;
    }

    if (this->UpgradeContext.IsPreparingUpgrade)
    {
        error = ValidateMovedServiceTypes(movedTypes);

        if (!error.IsSuccess())
        {
            return error;
        }

        this->UpgradeContext.ClearTargetServicePackages();

        for (auto const & package : affectedTypes)
        {
            this->UpgradeContext.AddTargetServicePackage(
                package.TypeName,
                package.PackageName,
                package.PackageVersion);
        }

        for (auto const & package : movedTypes)
        {
            this->UpgradeContext.AddTargetServicePackage(
                package.TypeName,
                package.PackageName,
                package.PackageVersion);
        }
    }

    this->IsConfigOnly = !shouldRestartPackages;

    ServiceModel::ServicePackageResourceGovernanceMap spRGSettings = targetApplication_.ResourceGovernanceDescriptions;
    ServiceModel::CodePackageContainersImagesMap cpContainersImages = targetApplication_.CodePackageContainersImages;

    vector<wstring> networks(targetApplication_.Networks);

    return LoadMessageBody(
        targetApplication_.Application,
        this->UpgradeContext.UpgradeDescription.IsInternalMonitored,
        this->UpgradeContext.UpgradeDescription.IsUnmonitoredManual,
        false, // IsRollback
        affectedTypes, 
        removedTypes_,
        codePackages,
        this->GetRollforwardReplicaSetCheckTimeout(),
        this->UpgradeContext.RollforwardInstance,
        move(spRGSettings),
        move(cpContainersImages),
        move(networks),
        requestBody);
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::LoadRollbackMessageBody(
    __out UpgradeApplicationRequestMessageBody & requestBody)
{
    vector<StoreDataServicePackage> affectedTypes;
    vector<StoreDataServicePackage> movedTypes;
    DigestedApplicationDescription::CodePackageNameMap codePackages;
    bool shouldRestartPackages;

    // Reverse the target and current for optimized rollback
    //
    auto error = targetApplication_.ComputeAffectedServiceTypes(
        currentApplication_, 
        affectedTypes, // out
        movedTypes, // out, unused
        codePackages, // out
        shouldRestartPackages); // out

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent, 
            "{0} could not compute affected rollback service types due to {1}",
            this->TraceId,
            error);

        // Don't fail rollback, fallback to non-optimized values.
        // Assume all services and code packages are affected.
        //
        affectedTypes = currentApplication_.ServicePackages;
        codePackages = currentApplication_.GetAllCodePackageNames();
    }

    ServiceModel::ServicePackageResourceGovernanceMap spRGSettings = currentApplication_.ResourceGovernanceDescriptions;
    ServiceModel::CodePackageContainersImagesMap cpContainersImages = currentApplication_.CodePackageContainersImages;

    vector<wstring> networks(currentApplication_.Networks);

    return LoadMessageBody(
        currentApplication_.Application, 
        this->UpgradeContext.UpgradeDescription.IsInternalMonitored,
        this->UpgradeContext.UpgradeDescription.IsUnmonitoredManual,
        true, // IsRollback
        affectedTypes,
        vector<StoreDataServicePackage>(), // added types not available until upgrade finishes 
        codePackages,
        this->GetRollbackReplicaSetCheckTimeout(),
        this->UpgradeContext.RollbackInstance,
        move(spRGSettings),
        move(cpContainersImages),
        move(networks),
        requestBody);
}

void ProcessApplicationUpgradeContextAsyncOperation::TraceQueryableUpgradeStart()
{
    CMEvents::Trace->ApplicationUpgradeStart(
        appContextUPtr_->ApplicationName.ToString(),
        this->UpgradeContext.RollforwardInstance,
        0, // dca_version
        this->ReplicaActivityId, 
        this->UpgradeContext.UpgradeDescription.TargetApplicationTypeVersion,
        this->appContextUPtr_->ManifestId,
        this->UpgradeContext.TargetApplicationManifestId);

    CMEvents::Trace->ApplicationUpgradeStartOperational(
        this->ReplicaActivityId.ActivityId.Guid,
        appContextUPtr_->ApplicationName.ToString(),
        appContextUPtr_->TypeName.Value,
        appContextUPtr_->TypeVersion.Value,
        this->UpgradeContext.UpgradeDescription.TargetApplicationTypeVersion,
        this->UpgradeContext.UpgradeDescription.UpgradeType,
        this->UpgradeContext.UpgradeDescription.RollingUpgradeMode,
        this->UpgradeContext.UpgradeDescription.MonitoringPolicy.FailureAction);
}

void ProcessApplicationUpgradeContextAsyncOperation::TraceQueryableUpgradeDomainComplete(std::vector<std::wstring> & recentlyCompletedUDs)
{
    CMEvents::Trace->ApplicationUpgradeDomainComplete(
        appContextUPtr_->ApplicationName.ToString(),
        this->UpgradeContext.RollforwardInstance,
        0, // dca_version
        this->ReplicaActivityId, 
        (this->UpgradeContext.UpgradeState == ApplicationUpgradeState::RollingBack ?
            this->UpgradeContext.RollbackApplicationTypeVersion.Value :
            this->UpgradeContext.UpgradeDescription.TargetApplicationTypeVersion),
        wformatString(recentlyCompletedUDs));

    CMEvents::Trace->ApplicationUpgradeDomainCompleteOperational(
        this->ReplicaActivityId.ActivityId.Guid,
        appContextUPtr_->ApplicationName.ToString(),
        appContextUPtr_->TypeName.Value,
        appContextUPtr_->TypeVersion.Value,
        (this->UpgradeContext.UpgradeState == ApplicationUpgradeState::RollingBack ?
            this->UpgradeContext.RollbackApplicationTypeVersion.Value :
            this->UpgradeContext.UpgradeDescription.TargetApplicationTypeVersion),
        this->UpgradeContext.UpgradeState,
        wformatString(recentlyCompletedUDs),
        this->UpgradeContext.UpgradeDomainElapsedTime.TotalMillisecondsAsDouble());
}

std::wstring ProcessApplicationUpgradeContextAsyncOperation::GetTraceAnalysisContext()
{
    return wformatString(
        "mode={0} target={1} current={2} state={3}",
        this->UpgradeContext.UpgradeDescription.RollingUpgradeMode,
        this->UpgradeContext.UpgradeDescription.TargetApplicationTypeVersion,
        this->UpgradeContext.RollbackApplicationTypeVersion.Value,
        this->UpgradeContext.UpgradeState);
}

AsyncOperationSPtr ProcessApplicationUpgradeContextAsyncOperation::BeginInitializeUpgrade(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<InitializeUpgradeAsyncOperation>(*this, callback, parent);
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::EndInitializeUpgrade(
    AsyncOperationSPtr const & operation)
{
    return InitializeUpgradeAsyncOperation::End(operation);
}

AsyncOperationSPtr ProcessApplicationUpgradeContextAsyncOperation::BeginOnValidationError(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto const & currentApp = currentApplication_.Application;
    auto const & targetApp = targetApplication_.Application;

    auto const & currentAppPackageRolloutVersion = currentApp.ApplicationPackageReference.RolloutVersionValue;
    auto const & targetAppPackageRolloutVersion = targetApp.ApplicationPackageReference.RolloutVersionValue;

    return this->ImageBuilder.BeginCleanupApplicationInstance(
        this->ActivityId,
        targetApp,
        (currentAppPackageRolloutVersion != targetAppPackageRolloutVersion), // deleteApplicationPackage
        addedPackages_,
        this->GetImageBuilderTimeout(),
        callback,
        parent);
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::EndOnValidationError(
    AsyncOperationSPtr const & operation)
{
    return this->ImageBuilder.EndCleanupApplicationInstance(operation);
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::OnRefreshUpgradeContext(StoreTransaction const & storeTx)
{
    switch (this->UpgradeContext.UpgradeState)
    {
    case ApplicationUpgradeState::CompletedRollforward:
    case ApplicationUpgradeState::RollingForward:
        return ErrorCodeValue::Success;
    };

    auto goalStateContext = make_unique<GoalStateApplicationUpgradeContext>(this->UpgradeContext.UpgradeDescription.ApplicationName);

    auto error = storeTx.ReadExact(*goalStateContext);

    if (error.IsError(ErrorCodeValue::NotFound))
    {
        return ErrorCodeValue::Success;
    }
    else if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent, 
            "{0} failed to read goal state upgrade on refresh: error={1}",
            this->TraceId,
            error);

        return error;
    }
    else
    {
        this->SetDynamicUpgradeStatusDetails(wformatString(
            GET_RC( Goal_State_App_Upgrade ),
            goalStateContext->UpgradeDescription.TargetApplicationTypeVersion));

        return ErrorCodeValue::Success;
    }
}

AsyncOperationSPtr ProcessApplicationUpgradeContextAsyncOperation::BeginUpdateHealthPolicyForHealthCheck(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & thisSPtr)
{
    // Always update the application instance on the HM health entity regardless of whether
    // the health policy was specified or not. Note that as a side-effect, this will also 
    // clear all existing health reports on the application in preparation for the upgrade.
    //
    ApplicationHealthPolicy const & healthPolicy = this->UpgradeContext.UpgradeDescription.IsHealthPolicyValid 
        ? this->UpgradeContext.UpgradeDescription.HealthPolicy 
        : manifestHealthPolicy_;

    WriteInfo(
        TraceComponent, 
        "{0} rollforward: updating application health check policy: {1} (fromAPI={2})",
        this->TraceId,
        healthPolicy,
        this->UpgradeContext.UpgradeDescription.IsHealthPolicyValid);

    return this->BeginUpdateHealthPolicy(
        healthPolicy,
        callback,
        thisSPtr);
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::EvaluateHealth(
    AsyncOperationSPtr const &,
    __out bool & isHealthy, 
    __out std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations)
{
    return this->HealthAggregator.IsApplicationHealthy(
        this->ActivityId,
        appContextUPtr_->ApplicationName.ToString(),
        this->UpgradeContext.CompletedUpgradeDomains,
        nullptr,
        isHealthy,
        unhealthyEvaluations);
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::LoadVerifiedUpgradeDomains(
    __in StoreTransaction & storeTx,
    __out std::vector<std::wstring> & verifiedDomains)
{
    auto storeData = make_unique<StoreDataVerifiedUpgradeDomains>(appContextUPtr_->ApplicationId);
    auto error = storeTx.ReadExact(*storeData);

    if (error.IsSuccess())
    {
        verifiedDomains = move(storeData->UpgradeDomains);
    }

    return error;
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::UpdateVerifiedUpgradeDomains(
    __in StoreTransaction & storeTx,
    std::vector<std::wstring> && verifiedDomains)
{
    auto verifiedDomainsStoreData = make_unique<StoreDataVerifiedUpgradeDomains>(
        appContextUPtr_->ApplicationId,
        move(verifiedDomains));
    return storeTx.UpdateOrInsertIfNotFound(*verifiedDomainsStoreData);
}

AsyncOperationSPtr ProcessApplicationUpgradeContextAsyncOperation::BeginUpdateHealthPolicy(
    ApplicationHealthPolicy const & policy, 
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    vector<ServiceModel::HealthReport> reports;
    this->Replica.CreateApplicationHealthReport(
        appContextUPtr_->ApplicationName,
        Common::SystemHealthReportCode::CM_ApplicationUpdated,
        policy.ToString(),
        appContextUPtr_->TypeName.Value,
        appContextUPtr_->ApplicationDefinitionKind,
        appContextUPtr_->GlobalInstanceCount,
        this->UpgradeContext.SequenceNumber,
        reports);

    return this->HealthAggregator.BeginUpdateApplicationsHealth(
        this->ActivityId,
        move(reports),
        this->GetCommunicationTimeout(),
        callback,
        parent);
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::EndUpdateHealthPolicy(AsyncOperationSPtr const & operation)
{
    auto error = this->HealthAggregator.EndUpdateApplicationsHealth(operation);
    
    if (error.IsError(ErrorCodeValue::HealthStaleReport))
    {
        error = ErrorCodeValue::Success;
    }

    return error;
}

void ProcessApplicationUpgradeContextAsyncOperation::OnCompleted()
{
    this->PerfCounters.NumberOfAppUpgrades.Decrement();
}

void ProcessApplicationUpgradeContextAsyncOperation::StartSendRequest(
    AsyncOperationSPtr const & thisSPtr)
{
    if (this->UpgradeContext.IsDefaultServiceUpdated)
    {
        WriteNoise(
            TraceComponent,
            "{0} updating default services",
            this->TraceId);

        auto storeTx = this->CreateTransaction();

        this->UpgradeContext.CommonUpgradeContextData.UpgradeStatusDetails = GET_RC(Update_Default_Service);
        ErrorCode error = storeTx.Update(this->UpgradeContext);

        if (error.IsSuccess())
        {
            auto operation = this->BeginCommitUpgradeContext(
                move(storeTx),
                [this](AsyncOperationSPtr const & operation) { this->OnPrepareUpdateDefaultServicesComplete(operation, false); },
                thisSPtr);
            this->OnPrepareUpdateDefaultServicesComplete(operation, true);
        }
        else
        {
            WriteWarning(
                TraceComponent,
                "{0} could not update status details for update default services due to {1}",
                TraceId,
                error);

            this->TryRefreshContextAndScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->StartSendRequest(thisSPtr); });
        }
    }
    else
    {
        this->ScheduleRequestToFM(thisSPtr);
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::OnPrepareUpdateDefaultServicesComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode error = this->EndCommitUpgradeContext(operation);

    auto const & thisSPtr = operation->Parent;

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0} could not commit update default service status details due to {1}",
            TraceId,
            error);

        this->TryRefreshContextAndScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->StartSendRequest(thisSPtr); });
    }
    else
    {
        this->UpdateDefaultServices(thisSPtr);
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::UpdateDefaultServices(
    AsyncOperationSPtr const & thisSPtr)
{
    auto error = this->RefreshUpgradeContext();
    if (!error.IsSuccess())
    {
        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->UpdateDefaultServices(thisSPtr); });
        return;
    }

    if (this->UpgradeContext.IsInterrupted)
    {
        CMEvents::Trace->UpgradeInterruption(this->ReplicaActivityId, this->GetUpgradeContextStringTrace());
        this->PrepareStartRollback(thisSPtr);
        return;
    }

    WriteNoise(
        TraceComponent,
        "{0} updating {1} default service(s) for upgrade",
        TraceId,
        this->UpgradeContext.DefaultServiceUpdateDescriptions.size());

    auto operation = AsyncOperation::CreateAndStart<ApplicationUpgradeParallelRetryableAsyncOperation>(
        *this,
        this->UpgradeContext,
        static_cast<long>(this->UpgradeContext.DefaultServiceUpdateDescriptions.size()),
        [this](AsyncOperationSPtr const & parallelAsyncOperation, size_t operationIndex, ParallelOperationsCompletedCallback const & callback)->ErrorCode
        {
            return this->ScheduleUpdateDefaultService(parallelAsyncOperation, operationIndex, false, callback);
        },
        [this](AsyncOperationSPtr const & operation) { this->OnUpdateDefaultServicesComplete(operation, false); },
        thisSPtr);
    this->OnUpdateDefaultServicesComplete(operation, true);
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::ScheduleUpdateDefaultService(
    AsyncOperationSPtr const & parallelAsyncOperation,
    size_t operationIndex,
    bool isRollback,
    ParallelOperationsCompletedCallback const & callback)
{
    return this->Replica.ScheduleNamingAsyncWork(
        [parallelAsyncOperation, this, operationIndex, isRollback](AsyncCallback const & jobQueueCallback)
        {
            return this->BeginUpdateDefaultService(parallelAsyncOperation, operationIndex, isRollback, jobQueueCallback); 
        },
        [this, callback](AsyncOperationSPtr const & operation) { this->EndUpdateDefaultService(operation, callback); });
}

AsyncOperationSPtr ProcessApplicationUpgradeContextAsyncOperation::BeginUpdateDefaultService(
    AsyncOperationSPtr const & parallelAsyncOperation,
    size_t operationIndex,
    bool isRollback,
    AsyncCallback const & jobQueueCallback)
{
    auto const & serviceUpdateDescriptions = isRollback ? this->UpgradeContext.RollbackDefaultServiceUpdateDescriptions : this->UpgradeContext.DefaultServiceUpdateDescriptions;
    ASSERT_IF(
        operationIndex >= serviceUpdateDescriptions.size(),
        "UpdateDefaultService: {0}: operation index {1} is out of bounds, only {2} items in map",
        this->UpgradeContext.ActivityId,
        operationIndex,
        serviceUpdateDescriptions.size());
    auto it = serviceUpdateDescriptions.begin();
    advance(it, operationIndex);
    Common::ActivityId innerActivityId(this->UpgradeContext.ActivityId, static_cast<uint64>(operationIndex));
    return this->Client.BeginUpdateService(
        it->first,
        it->second,
        innerActivityId,
        this->GetCommunicationTimeout(),
        jobQueueCallback,
        parallelAsyncOperation);
}

void ProcessApplicationUpgradeContextAsyncOperation::EndUpdateDefaultService(
    Common::AsyncOperationSPtr const & operation,
    ParallelOperationsCompletedCallback const & callback)
{
    auto error = this->Client.EndUpdateService(operation);
    callback(operation->Parent, error);
}

void ProcessApplicationUpgradeContextAsyncOperation::OnUpdateDefaultServicesComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = AsyncOperation::End<ApplicationUpgradeParallelRetryableAsyncOperation>(operation)->Error;
    
    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    if (error.IsSuccess())
    {
        this->PrepareRequestToFM(thisSPtr);
        return;
    }

    auto storeTx = this->CreateTransaction();

    if (this->IsServiceOperationFatalError(error))
    {
        WriteWarning(
            TraceComponent,
            "{0} update default service failed due to fatal error {1}. Upgrade will rollback",
            this->TraceId,
            error);

        this->UpgradeContext.CommonUpgradeContextData.FailureReason = UpgradeFailureReason::ProcessingFailure;
        this->UpgradeContext.CommonUpgradeContextData.UpgradeStatusDetailsAtFailure = wformatString(GET_RC( Update_Default_Service_Failed ), error);
        this->UpgradeContext.CommonUpgradeContextData.IsSkipRollbackUD = true;

        this->SetDynamicUpgradeStatusDetails(L"");

        error = storeTx.Update(this->UpgradeContext);
    }

    if (error.IsSuccess())
    {
        auto commitUpgradeContextOperation = this->BeginCommitUpgradeContext(
            move(storeTx),
            [this, error](AsyncOperationSPtr const & operation) { this->OnUpdateDefaultServicesFailureComplete(operation, false); },
            thisSPtr);
        this->OnUpdateDefaultServicesFailureComplete(commitUpgradeContextOperation, true);
    }
    else
    {
        this->TryRefreshContextAndScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->UpdateDefaultServices(thisSPtr); });
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::PrepareRequestToFM(AsyncOperationSPtr const & thisSPtr)
{
    auto storeTx = this->CreateTransaction();

    // UD progress reflects the status
    this->UpgradeContext.CommonUpgradeContextData.UpgradeStatusDetails = L"";
    ErrorCode error = storeTx.Update(this->UpgradeContext);

    if (error.IsSuccess())
    {
        auto operation = this->BeginCommitUpgradeContext(
            move(storeTx),
            [this](AsyncOperationSPtr const & operation) { this->OnPrepareRequestToFMComplete(operation, false); },
            thisSPtr);
        this->OnPrepareRequestToFMComplete(operation, true);
    }
    else
    {
        this->TryRefreshContextAndScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->PrepareRequestToFM(thisSPtr); });
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::OnPrepareRequestToFMComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode error = this->EndCommitUpgradeContext(operation);

    auto const & thisSPtr = operation->Parent;

    if (error.IsSuccess())
    {
        this->ScheduleRequestToFM(thisSPtr);
    }
    else
    {
        WriteWarning(
            TraceComponent,
            "{0} could not commit update default service complete status details due to {1}",
            TraceId,
            error);

        this->TryRefreshContextAndScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->UpdateDefaultServices(thisSPtr); });
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::OnUpdateDefaultServicesFailureComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode innerError = this->EndCommitUpgradeContext(operation);

    auto const & thisSPtr = operation->Parent;

    if (innerError.IsSuccess())
    {
        this->PrepareStartRollback(thisSPtr);
    }
    else
    {
        this->TryRefreshContextAndScheduleRetry(innerError, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->UpdateDefaultServices(thisSPtr); });
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::FinalizeUpgrade(AsyncOperationSPtr const & thisSPtr)
{
    if (!this->UpgradeContext.AddedDefaultServices.empty())
    {
        auto storeTx = this->CreateTransaction();
        this->UpgradeContext.CommonUpgradeContextData.UpgradeStatusDetails = GET_RC(Create_Default_Service);
        ErrorCode error = storeTx.Update(this->UpgradeContext);

        if (error.IsSuccess())
        {
            auto operation = this->BeginCommitUpgradeContext(
                move(storeTx),
                [this](AsyncOperationSPtr const & operation) { this->OnPrepareCreateDefaultServicesComplete(operation, false); },
                thisSPtr);
            this->OnPrepareCreateDefaultServicesComplete(operation, true);
        }
        else
        {
            WriteWarning(
                TraceComponent,
                "{0} could not update status details for create default services due to {1}",
                this->TraceId,
                error);

            this->TryRefreshContextAndScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->FinalizeUpgrade(thisSPtr); });
        }
    }
    // Skip creating default service(s)
    else
    {
        this->PrepareDeleteDefaultServices(thisSPtr);
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::OnPrepareCreateDefaultServicesComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode error = this->EndCommitUpgradeContext(operation);

    auto const & thisSPtr = operation->Parent;

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0} could not commit create default service status details due to {1}",
            TraceId,
            error);

        this->TryRefreshContextAndScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->FinalizeUpgrade(thisSPtr); });
    }
    else
    {
        this->CreateDefaultServices(thisSPtr);
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::CreateDefaultServices(AsyncOperationSPtr const & thisSPtr)
{
    WriteInfo(
        TraceComponent,
        "{0} creating {1} default service(s) for upgrade",
        TraceId,
        this->UpgradeContext.AddedDefaultServices.size());

    auto operation = AsyncOperation::CreateAndStart<ApplicationUpgradeParallelRetryableAsyncOperation>(
        *this,
        this->UpgradeContext,
        static_cast<long>(this->UpgradeContext.AddedDefaultServices.size()),
        [this](AsyncOperationSPtr const & parallelAsyncOperation, size_t operationIndex, ParallelOperationsCompletedCallback const & callback)->ErrorCode
        {
            return this->ScheduleCreateService(parallelAsyncOperation, operationIndex, callback);
        },
        [this](AsyncOperationSPtr const & operation) { this->OnCreateDefaultServicesComplete(operation, false); },
        thisSPtr);
    this->OnCreateDefaultServicesComplete(operation, true);
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::ScheduleCreateService(
    AsyncOperationSPtr const & parallelAsyncOperation,
    size_t operationIndex,
    ParallelOperationsCompletedCallback const & callback)
{
    return this->Replica.ScheduleNamingAsyncWork(
        [parallelAsyncOperation, this, operationIndex](AsyncCallback const & jobQueueCallback)
        {
            return this->BeginCreateService(parallelAsyncOperation, operationIndex, jobQueueCallback);
        },
        [this, callback](AsyncOperationSPtr const & operation) { this->EndCreateService(operation, callback); });
}

AsyncOperationSPtr ProcessApplicationUpgradeContextAsyncOperation::BeginCreateService(
    AsyncOperationSPtr const & parallelAsyncOperation,
    size_t operationIndex,
    AsyncCallback const & jobQueueCallback)
{
    ASSERT_IF(
        operationIndex >= this->UpgradeContext.AddedDefaultServices.size(),
        "CreateService {0}: operationIndex {1} is out of bounds, {2} added default services",
        this->UpgradeContext.ActivityId,
        operationIndex,
        this->UpgradeContext.AddedDefaultServices.size());
    Common::ActivityId innerActivityId(this->UpgradeContext.ActivityId, static_cast<uint64>(operationIndex));
    auto &defaultServiceContext = this->UpgradeContext.AddedDefaultServices[operationIndex];
    return AsyncOperation::CreateAndStart<CreateDefaultServiceWithDnsNameIfNeededAsyncOperation>(
        defaultServiceContext,
        *this,
        innerActivityId,
        this->GetCommunicationTimeout(),
        jobQueueCallback,
        parallelAsyncOperation);
}

void ProcessApplicationUpgradeContextAsyncOperation::EndCreateService(
    Common::AsyncOperationSPtr const & operation,
    ParallelOperationsCompletedCallback const & callback)
{
    auto error = CreateDefaultServiceWithDnsNameIfNeededAsyncOperation::End(operation);

    callback(operation->Parent, error);
}

void ProcessApplicationUpgradeContextAsyncOperation::OnCreateDefaultServicesComplete(
    AsyncOperationSPtr const & operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = AsyncOperation::End<ApplicationUpgradeParallelRetryableAsyncOperation>(operation)->Error;

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    if (error.IsSuccess())
    {
        this->PrepareDeleteDefaultServices(thisSPtr);
    }
    else
    {
        // Retry the entire step
        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->CreateDefaultServices(thisSPtr); });
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::PrepareDeleteDefaultServices(AsyncOperationSPtr const & thisSPtr)
{
    if (!this->UpgradeContext.DeletedDefaultServices.empty())
    {
        auto storeTx = this->CreateTransaction();

        // Rollback would never occur as soon as delete default service starts, even after failover.
        this->UpgradeContext.CommonUpgradeContextData.IsRollbackAllowed = false;
        this->UpgradeContext.CommonUpgradeContextData.UpgradeStatusDetails = GET_RC(Delete_Default_Service);

        ErrorCode error = storeTx.Update(this->UpgradeContext);

        if (error.IsSuccess())
        {
            auto operation = this->BeginCommitUpgradeContext(
                move(storeTx),
                [this](AsyncOperationSPtr const & operation) { this->OnPrepareDeleteDefaultServicesComplete(operation, false); },
                thisSPtr);
            this->OnPrepareDeleteDefaultServicesComplete(operation, true);
        }
        else
        {
            WriteWarning(
                TraceComponent,
                "{0} could not update status details for delete default services due to {1}",
                TraceId,
                error);

            this->TryRefreshContextAndScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->PrepareDeleteDefaultServices(thisSPtr); });
        }
    }
    // Skip deleting default service(s)
    else if (this->UpgradeContext.CommonUpgradeContextData.UpgradeStatusDetails != L"")
    {
        this->CommitDeleteDefaultServices(thisSPtr);
    }
    else
    {
        this->PrepareFinishUpgrade(thisSPtr);
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::OnPrepareDeleteDefaultServicesComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode error = this->EndCommitUpgradeContext(operation);

    auto const & thisSPtr = operation->Parent;

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0} could not commit delete default service status details due to {1}",
            TraceId,
            error);

        this->TryRefreshContextAndScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->PrepareDeleteDefaultServices(thisSPtr); });
    }
    else
    {
        this->DeleteDefaultServices(thisSPtr);
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::DeleteDefaultServices(AsyncOperationSPtr const & thisSPtr)
{
    WriteInfo(
        TraceComponent,
        "{0} deleting {1} default service(s) for upgrade",
        TraceId,
        this->UpgradeContext.DeletedDefaultServices.size());

    auto operation = AsyncOperation::CreateAndStart<ApplicationUpgradeParallelRetryableAsyncOperation>(
        *this,
        this->UpgradeContext,
        static_cast<long>(this->UpgradeContext.DeletedDefaultServices.size()),
        [this](AsyncOperationSPtr const & parallelAsyncOperation, size_t operationIndex, ParallelOperationsCompletedCallback const & callback)->ErrorCode
        {
            return this->ScheduleDeleteService(parallelAsyncOperation, operationIndex, false, callback);
        },
        [this](AsyncOperationSPtr const & operation) { this->OnDeleteDefaultServicesComplete(operation, false); },
        thisSPtr);
    this->OnDeleteDefaultServicesComplete(operation, true);
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::ScheduleDeleteService(
    AsyncOperationSPtr const & parallelAsyncOperation,
    size_t operationIndex,
    bool isRollback,
    ParallelOperationsCompletedCallback const & callback)
{
    return this->Replica.ScheduleNamingAsyncWork(
        [parallelAsyncOperation, this, operationIndex, isRollback](AsyncCallback const & jobQueueCallback)
        {
            return this->BeginDeleteService(parallelAsyncOperation, operationIndex, isRollback, jobQueueCallback);
        },
        [this, callback](AsyncOperationSPtr const & operation) { this->EndDeleteService(operation, callback); });
}

AsyncOperationSPtr ProcessApplicationUpgradeContextAsyncOperation::BeginDeleteService(
    AsyncOperationSPtr const & parallelAsyncOperation,
    size_t operationIndex,
    bool isRollback,
    AsyncCallback const & jobQueueCallback)
{
    DeleteServiceDescription serviceDescription;
    if (isRollback)
    {
        ASSERT_IF(
            operationIndex >= this->UpgradeContext.AddedDefaultServices.size(),
            "DeleteService for rollback {0}: operationIndex {1} is out of bounds, {2} default services",
            this->UpgradeContext.ActivityId,
            operationIndex,
            this->UpgradeContext.AddedDefaultServices.size());
        serviceDescription = DeleteServiceDescription(this->UpgradeContext.AddedDefaultServices[operationIndex].AbsoluteServiceName);
    }
    else
    {
        ASSERT_IF(
            operationIndex >= this->UpgradeContext.DeletedDefaultServices.size(),
            "DeleteService {0}: operationIndex {1} is out of bounds, {2} default services",
            this->UpgradeContext.ActivityId,
            operationIndex,
            this->UpgradeContext.DeletedDefaultServices.size());
        serviceDescription = DeleteServiceDescription(this->UpgradeContext.DeletedDefaultServices[operationIndex]);
    }

    Common::ActivityId innerActivityId(this->UpgradeContext.ActivityId, static_cast<uint64>(operationIndex));
    return this->Client.BeginDeleteService(
        this->UpgradeContext.UpgradeDescription.ApplicationName,
        serviceDescription,
        innerActivityId,
        this->GetCommunicationTimeout(),
        jobQueueCallback,
        parallelAsyncOperation);
}

void ProcessApplicationUpgradeContextAsyncOperation::EndDeleteService(
    Common::AsyncOperationSPtr const & operation,
    ParallelOperationsCompletedCallback const & callback)
{
    auto error = this->Client.EndDeleteService(operation);
    if (error.IsError(ErrorCodeValue::UserServiceNotFound))
    {
        error = ErrorCode::Success();
    }

    callback(operation->Parent, error);
}

void ProcessApplicationUpgradeContextAsyncOperation::OnDeleteDefaultServicesComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = AsyncOperation::End<ApplicationUpgradeParallelRetryableAsyncOperation>(operation)->Error;

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    if (error.IsSuccess())
    {
        this->CommitDeleteDefaultServices(thisSPtr);
    }
    else
    {
        this->TryRefreshContextAndScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->DeleteDefaultServices(thisSPtr); });
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::CommitDeleteDefaultServices(AsyncOperationSPtr const & thisSPtr)
{
    auto storeTx = this->CreateTransaction();

    this->UpgradeContext.CommonUpgradeContextData.UpgradeStatusDetails = L"";
    ErrorCode error = storeTx.Update(this->UpgradeContext);

    if (error.IsSuccess())
    {
        auto operation = this->BeginCommitUpgradeContext(
            move(storeTx),
            [this](AsyncOperationSPtr const & operation) { this->OnCommitDeleteDefaultServicesComplete(operation, false); },
            thisSPtr);
        this->OnCommitDeleteDefaultServicesComplete(operation, true);
    }
    else
    {
        this->TryRefreshContextAndScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->CommitDeleteDefaultServices(thisSPtr); });
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::OnCommitDeleteDefaultServicesComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode error = this->EndCommitUpgradeContext(operation);

    auto const & thisSPtr = operation->Parent;

    if (error.IsSuccess())
    {
        this->PrepareFinishUpgrade(thisSPtr);
    }
    else
    {
        WriteWarning(
            TraceComponent,
            "{0} could not commit finish upgrade status details due to {1}",
            TraceId,
            error);

        this->TryRefreshContextAndScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->CommitDeleteDefaultServices(thisSPtr); });
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::PrepareFinishUpgrade(AsyncOperationSPtr const & thisSPtr)
{
    ApplicationHealthPolicy healthPolicy;
    auto error = this->LoadHealthPolicy(ServiceModelVersion(this->UpgradeContext.UpgradeDescription.TargetApplicationTypeVersion), healthPolicy);
    
    if (error.IsSuccess())
    {
        WriteInfo(
            TraceComponent, 
            "{0} finished upgrade: updating application health check policy: {1}",
            this->TraceId,
            healthPolicy);

        auto operation = this->BeginUpdateHealthPolicy(
            healthPolicy,
            [this](AsyncOperationSPtr const & operation){ this->OnPrepareFinishUpgradeComplete(operation, false); },
            thisSPtr);
        this->OnPrepareFinishUpgradeComplete(operation, true);
    }
    else
    {
        this->TrySchedulePrepareFinishUpgrade(error, thisSPtr);
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::OnPrepareFinishUpgradeComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    auto error = this->EndUpdateHealthPolicy(operation);

    if (error.IsSuccess())
    {
        this->FinishUpgrade(thisSPtr);
    }
    else
    {
        WriteInfo(
            TraceComponent, 
            "{0} failed to update application health policy: {1}",
            this->TraceId,
            error);

        this->TrySchedulePrepareFinishUpgrade(error, thisSPtr);
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::FinishUpgrade(AsyncOperationSPtr const & thisSPtr)
{
    WriteNoise(
        TraceComponent, 
        "{0} finishing upgrade",
        this->TraceId);

    auto error = RefreshUpgradeContext();
    if (!error.IsSuccess())
    {
        this->TrySchedulePrepareFinishUpgrade(error, thisSPtr);
        return;
    }

    // Persist all state that completes the upgrade in a single transaction. This greatly simplifies the
    // rollback process as there will be no partial state to cleanup.
    // 
    auto storeTx = this->CreateTransaction();

    error = FinishUpgradeServicePackages(storeTx);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent, 
            "{0} could not finish upgrading service packages due to {1}",
            this->TraceId,
            error);

        this->TrySchedulePrepareFinishUpgrade(error, thisSPtr);
        return;
    }

    error = FinishUpgradeServiceTemplates(storeTx);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent, 
            "{0} could not finish upgrading service templates due to {1}",
            this->TraceId,
            error);

        this->TrySchedulePrepareFinishUpgrade(error, thisSPtr);
        return;
    }

    error = FinishDeleteGoalState(storeTx);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent, 
            "{0} could not finish deleting goal state due to {1}",
            this->TraceId,
            error);

        this->TrySchedulePrepareFinishUpgrade(error, thisSPtr);
        return;
    }

    error = FinishCreateDefaultServices(storeTx);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent, 
            "{0} could not finish creating default services due to {1}",
            this->TraceId,
            error);

        this->TrySchedulePrepareFinishUpgrade(error, thisSPtr);
        return;
    }

    error = FinishDeleteDefaultServices(storeTx);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0} could not finish deleting default services due to {1}",
            this->TraceId,
            error);

        this->TrySchedulePrepareFinishUpgrade(error, thisSPtr);
        return;
    }

    appContextUPtr_->SetApplicationAndTypeVersion(
        targetApplication_.Application.Version,
        ServiceModelVersion(targetApplication_.Application.ApplicationTypeVersion));
    appContextUPtr_->SetApplicationParameters(this->UpgradeContext.UpgradeDescription.ApplicationParameters);

    error = this->OnFinishUpgrade(storeTx);
    if (!error.IsSuccess())
    {
        this->TrySchedulePrepareFinishUpgrade(error, thisSPtr);
        return;
    }
    // saving current manifestid for tracing in OnFinishUpgradeCommitComplete
    currentManifestId_ = appContextUPtr_->ManifestId;

    // updating the current manifestid to the upgraded manifestid
    appContextUPtr_->ManifestId = this->UpgradeContext.TargetApplicationManifestId;

    error = this->UpgradeContext.CompleteRollforward(storeTx);
    if (!error.IsSuccess())
    {
        this->TrySchedulePrepareFinishUpgrade(error, thisSPtr);
        return;
    }

    error = CompleteApplicationContext(storeTx);
    if (!error.IsSuccess())
    {
        this->TrySchedulePrepareFinishUpgrade(error, thisSPtr);
        return;
    }

    error = this->ClearVerifiedUpgradeDomains(storeTx, *make_unique<StoreDataVerifiedUpgradeDomains>(appContextUPtr_->ApplicationId));
    if (!error.IsSuccess())
    {
        this->TrySchedulePrepareFinishUpgrade(error, thisSPtr);
        return;
    }

    auto operation = this->BeginCommitUpgradeContext(
        move(storeTx),
        [this](AsyncOperationSPtr const & operation) { this->OnFinishUpgradeCommitComplete(operation, false); },
        thisSPtr);
    this->OnFinishUpgradeCommitComplete(operation, true);
}

void ProcessApplicationUpgradeContextAsyncOperation::OnFinishUpgradeCommitComplete(
    AsyncOperationSPtr const & operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto const & thisSPtr = operation->Parent;

    auto error = this->EndCommitUpgradeContext(operation);

    if (error.IsSuccess())
    {
        CMEvents::Trace->UpgradeComplete(this->ReplicaActivityId, this->GetUpgradeContextStringTrace());

        CMEvents::Trace->ApplicationUpgradeComplete(
            appContextUPtr_->ApplicationName.ToString(),
            this->UpgradeContext.RollforwardInstance,
            0, // dca_version
            this->ReplicaActivityId, 
            this->UpgradeContext.UpgradeDescription.TargetApplicationTypeVersion,
            currentManifestId_,
            this->UpgradeContext.TargetApplicationManifestId);

        CMEvents::Trace->ApplicationUpgradeCompleteOperational(
            this->ReplicaActivityId.ActivityId.Guid,
            appContextUPtr_->ApplicationName.ToString(),
            appContextUPtr_->TypeName.Value,
            this->UpgradeContext.UpgradeDescription.TargetApplicationTypeVersion,
            this->UpgradeContext.OverallUpgradeElapsedTime.TotalMillisecondsAsDouble());

        this->TryComplete(thisSPtr, error);
    }
    else
    {
        WriteInfo(
            TraceComponent, 
            "{0} could not commit to finish upgrade due to {1}",
            this->TraceId,
            error);

        this->TrySchedulePrepareFinishUpgrade(error, thisSPtr);
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::TrySchedulePrepareFinishUpgrade(
    ErrorCode const & error, 
    AsyncOperationSPtr const & thisSPtr)
{
    auto errorToCheck = error;

    // upgrades can be interrupted by another upgrade, which will change the
    // sequence number on the corresponding application context
    //
    auto refreshError = this->LoadApplicationContext();

    if (!refreshError.IsSuccess())
    {
        errorToCheck.Overwrite(refreshError);
    }

    this->TryScheduleRetry(errorToCheck, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->PrepareFinishUpgrade(thisSPtr); });
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::FinishUpgradeServiceTemplates(StoreTransaction const & storeTx)
{
    ErrorCode error(ErrorCodeValue::Success);

    auto removedServiceTemplates = currentApplication_.ComputeRemovedServiceTemplates(targetApplication_);
    for (auto iter = removedServiceTemplates.begin(); iter != removedServiceTemplates.end(); ++iter)
    {
        error = storeTx.DeleteIfFound(*iter);

        if (!error.IsSuccess())
        {
            return error;
        }
    }

    auto modifiedServiceTemplates = currentApplication_.ComputeModifiedServiceTemplates(targetApplication_);
    for (auto iter = modifiedServiceTemplates.begin(); iter != modifiedServiceTemplates.end(); ++iter)
    {
        error = storeTx.UpdateOrInsertIfNotFound(*iter);

        if (!error.IsSuccess())
        {
            return error;
        }
    }

    auto addedServiceTemplates = currentApplication_.ComputeAddedServiceTemplates(targetApplication_);
    for (auto iter = addedServiceTemplates.begin(); iter != addedServiceTemplates.end(); ++iter)
    {
        error = storeTx.UpdateOrInsertIfNotFound(*iter);

        if (!error.IsSuccess())
        {
            return error;
        }
    }

    return error;
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::FinishUpgradeServicePackages(StoreTransaction const & storeTx)
{
    ErrorCode error(ErrorCodeValue::Success);

    auto removedServicePackages = currentApplication_.ComputeRemovedServiceTypes(targetApplication_);
    for (auto iter = removedServicePackages.begin(); iter != removedServicePackages.end(); ++iter)
    {
        error = storeTx.DeleteIfFound(*iter);

        if (!error.IsSuccess())
        {
            return error;
        }
    }

    vector<StoreDataServicePackage> modifiedServicePackages;
    vector<StoreDataServicePackage> movedServicePackages;
    DigestedApplicationDescription::CodePackageNameMap codePackages;
    bool unused;
    error = currentApplication_.ComputeAffectedServiceTypes(
        targetApplication_, 
        modifiedServicePackages, 
        movedServicePackages, 
        codePackages, // unused
        unused);
    if (!error.IsSuccess())
    {
        return error;
    }

    // merge the two lists for update
    modifiedServicePackages.insert(modifiedServicePackages.end(), movedServicePackages.begin(), movedServicePackages.end());

    for (auto iter = modifiedServicePackages.begin(); iter != modifiedServicePackages.end(); ++iter)
    {
        error = storeTx.UpdateOrInsertIfNotFound(*iter);

        if (!error.IsSuccess())
        {
            return error;
        }
    }

    auto addedServicePackages = currentApplication_.ComputeAddedServiceTypes(targetApplication_);
    for (auto iter = addedServicePackages.begin(); iter != addedServicePackages.end(); ++iter)
    {
        error = storeTx.UpdateOrInsertIfNotFound(*iter);

        if (!error.IsSuccess())
        {
            return error;
        }
    }

    return error;
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::FinishDeleteGoalState(StoreTransaction const & storeTx)
{
    // Typically don't expect goal state to exist anymore since we delete it 
    // after rollback when the goal state upgrade is restarted and when
    // accepting a new upgrade to replace a previously failed upgrade.
    //
    auto goalStateContext = make_unique<GoalStateApplicationUpgradeContext>(this->UpgradeContext.UpgradeDescription.ApplicationName);

    return storeTx.DeleteIfFound(*goalStateContext);
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::FinishCreateDefaultServices(StoreTransaction const & storeTx)
{
    vector<ServiceContext> & defaultServices = const_cast<vector<ServiceContext> &>(this->UpgradeContext.AddedDefaultServices);

    for (auto iter = defaultServices.begin(); iter != defaultServices.end(); ++iter)
    {
        auto error = iter->InsertCompletedIfNotFound(storeTx);

        if (!error.IsSuccess())
        {
            return error;
        }
    }

    return ErrorCodeValue::Success;
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::FinishDeleteDefaultServices(StoreTransaction const & storeTx)
{
    vector<NamingUri> const & deletedDefaultServices = this->UpgradeContext.DeletedDefaultServices;

    for (auto iter = deletedDefaultServices.begin(); iter != deletedDefaultServices.end(); ++iter)
    {
        ServiceContext serviceContextDeleted(
            appContextUPtr_->ApplicationId,
            appContextUPtr_->ApplicationName,
            *iter);
        auto error = storeTx.DeleteIfFound(serviceContextDeleted);

        if (!error.IsSuccess())
        {
            return error;
        }
    }

    return ErrorCodeValue::Success;
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::CompleteApplicationContext(StoreTransaction const & storeTx)
{
    auto error = appContextUPtr_->SetUpgradeComplete(storeTx, this->UpgradeContext.UpgradeCompletionInstance);

    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent, 
            "{0} could not set application status to Completed {1}",
            this->TraceId,
            error);
    }

    return error;
}

void ProcessApplicationUpgradeContextAsyncOperation::RemoveCreatedDefaultServices(AsyncOperationSPtr const & thisSPtr)
{
    WriteNoise(
        TraceComponent, 
        "{0} deleting {1} default service(s) for rollback",
        TraceId,
        this->UpgradeContext.AddedDefaultServices.size());

    auto operation = AsyncOperation::CreateAndStart<ApplicationUpgradeParallelRetryableAsyncOperation>(
        *this,
        this->UpgradeContext,
        static_cast<long>(this->UpgradeContext.AddedDefaultServices.size()),
        [this](AsyncOperationSPtr const & parallelAsyncOperation, size_t operationIndex, ParallelOperationsCompletedCallback const & callback)->ErrorCode
        {
            return this->ScheduleDeleteService(parallelAsyncOperation, operationIndex, true, callback);
        },
        [this](AsyncOperationSPtr const & operation) { this->OnRemoveCreatedDefaultServicesComplete(operation, false); },
        thisSPtr);
    this->OnRemoveCreatedDefaultServicesComplete(operation, true);
}

void ProcessApplicationUpgradeContextAsyncOperation::OnRemoveCreatedDefaultServicesComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = AsyncOperation::End<ApplicationUpgradeParallelRetryableAsyncOperation>(operation)->Error;
    
    auto const & thisSPtr = operation->Parent;

    if (error.IsSuccess())
    {
        this->CommitRemoveCreatedDefaultServices(thisSPtr);
    }
    else
    {
        this->TryScheduleRetry(error, operation->Parent, [this](AsyncOperationSPtr const & thisSPtr) { this->RemoveCreatedDefaultServices(thisSPtr); });
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::CommitRemoveCreatedDefaultServices(AsyncOperationSPtr const & thisSPtr)
{
    auto storeTx = this->CreateTransaction();

    // UD progress reflects the status
    this->UpgradeContext.CommonUpgradeContextData.UpgradeStatusDetails = L"";
    ErrorCode error = storeTx.Update(this->UpgradeContext);

    if (error.IsSuccess())
    {
        auto operation = this->BeginCommitUpgradeContext(
            move(storeTx),
            [this](AsyncOperationSPtr const & operation) { this->OnCommitRemoveCreatedDefaultServicesComplete(operation, false); },
            thisSPtr);
        this->OnCommitRemoveCreatedDefaultServicesComplete(operation, true);
    }
    else
    {
        this->TryRefreshContextAndScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->CommitRemoveCreatedDefaultServices(thisSPtr); });
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::OnCommitRemoveCreatedDefaultServicesComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode error = this->EndCommitUpgradeContext(operation);

    auto const & thisSPtr = operation->Parent;

    if (error.IsSuccess())
    {
        this->ScheduleRequestToFM(thisSPtr);
    }
    else
    {
        WriteWarning(
            TraceComponent,
            "{0} could not commit remove created default services status details due to {1}",
            TraceId,
            error);

        this->TryRefreshContextAndScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->CommitRemoveCreatedDefaultServices(thisSPtr); });
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::FinalizeRollback(AsyncOperationSPtr const & thisSPtr)
{
    if (this->UpgradeContext.IsDefaultServiceUpdated && !Get_SkipRollbackUpdateDefaultService())
    {
        auto storeTx = this->CreateTransaction();

        this->AppendDynamicUpgradeStatusDetails(GET_RC(Update_Default_Service));

        auto error = storeTx.Update(this->UpgradeContext);

        if (error.IsSuccess())
        {
            auto operation = this->BeginCommitUpgradeContext(
                move(storeTx),
                [this](AsyncOperationSPtr const & operation) { this->OnPrepareRollbackUpdatedDefaultServicesComplete(operation, false); },
                thisSPtr);
            this->OnPrepareRollbackUpdatedDefaultServicesComplete(operation, true);
            return;
        }
        else
        {
            WriteWarning(
                TraceComponent,
                "{0} failed to start rollback updated default service due to {1}",
                this->TraceId,
                error);
            
            this->TryRefreshContextAndScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->FinalizeRollback(thisSPtr); });
            return;
        }
    }
    else
    {
        if (Get_SkipRollbackUpdateDefaultService())
        {
            WriteInfo(
                TraceComponent,
                "{0} rollback updated default services is skipped",
                this->TraceId);
        }
        this->FinishRollback(thisSPtr);
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::OnPrepareRollbackUpdatedDefaultServicesComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode error = this->EndCommitUpgradeContext(operation);

    auto const & thisSPtr = operation->Parent;

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0} could not commit rollback updated default service status details due to {1}",
            this->TraceId,
            error);

        this->TryRefreshContextAndScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->FinalizeRollback(thisSPtr); });
    }
    else
    {
        this->RollbackUpdatedDefaultServices(thisSPtr);
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::RollbackUpdatedDefaultServices(AsyncOperationSPtr const & thisSPtr)
{
    WriteInfo(
        TraceComponent,
        "{0} updating {1} default service(s) for rollback",
        TraceId,
        this->UpgradeContext.RollbackDefaultServiceUpdateDescriptions.size());

    auto operation = AsyncOperation::CreateAndStart<ApplicationUpgradeParallelRetryableAsyncOperation>(
        *this,
        this->UpgradeContext,
        static_cast<long>(this->UpgradeContext.RollbackDefaultServiceUpdateDescriptions.size()),
        [this](AsyncOperationSPtr const & parallelAsyncOperation, size_t operationIndex, ParallelOperationsCompletedCallback const & callback)->ErrorCode
        {
            return this->ScheduleUpdateDefaultService(parallelAsyncOperation, operationIndex, true /*isRollback*/, callback);
        },
        [this](AsyncOperationSPtr const & operation) { this->OnRollbackUpdatedDefaultServicesComplete(operation, false); },
        thisSPtr);
    this->OnRollbackUpdatedDefaultServicesComplete(operation, true);
}

void ProcessApplicationUpgradeContextAsyncOperation::OnRollbackUpdatedDefaultServicesComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = AsyncOperation::End<ApplicationUpgradeParallelRetryableAsyncOperation>(operation)->Error;

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    if (error.IsSuccess())
    {
        PrepareFinishRollback(thisSPtr);
        return;
    }

    ErrorCode innerError = error;

    auto storeTx = this->CreateTransaction();

    if (this->IsServiceOperationFatalError(error))
    {
        WriteWarning(
            TraceComponent,
            "{0} rollback updated default service failed due to fatal error {1}",
            this->TraceId,
            error);

        this->UpgradeContext.CommonUpgradeContextData.FailureReason = UpgradeFailureReason::ProcessingFailure;

        this->AppendDynamicUpgradeStatusDetails(wformatString(GET_RC( Update_Default_Service_Rollback_Failed ), error));

        innerError = storeTx.Update(this->UpgradeContext);
    }

    // If rollback fails, the packageInstance will be mismatch between CM (N + 1) and FM (N + 2). So set packageInstance to N + 2 before fail the upgrade.
    if (innerError.IsSuccess())
    {
        innerError = appContextUPtr_->SetUpgradePending(storeTx, this->UpgradeContext.RollbackInstance);
    }

    // Prepare to fail rollback
    if (innerError.IsSuccess())
    {
         auto commitUpgradeContextOperation = this->BeginCommitUpgradeContext(
            move(storeTx),
            [this, error](AsyncOperationSPtr const & operation) { this->OnRollbackUpdatedDefaultServicesFailureComplete(error, operation, false); },
            thisSPtr);
        this->OnRollbackUpdatedDefaultServicesFailureComplete(error, commitUpgradeContextOperation, true);
    }
    else
    {
        this->TryRefreshContextAndScheduleRetry(innerError, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->RollbackUpdatedDefaultServices(thisSPtr); });
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::OnRollbackUpdatedDefaultServicesFailureComplete(
    ErrorCode const & error,
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode innerError = this->EndCommitUpgradeContext(operation);

    auto const & thisSPtr = operation->Parent;

    if (innerError.IsSuccess())
    {
        this->TryComplete(thisSPtr, error);
    }
    else
    {
        this->TryRefreshContextAndScheduleRetry(innerError, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->RollbackUpdatedDefaultServices(thisSPtr); });
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::PrepareFinishRollback(AsyncOperationSPtr const & thisSPtr)
{
    auto storeTx = this->CreateTransaction();

    auto error = storeTx.Update(this->UpgradeContext);

    if (error.IsSuccess())
    {
         auto operation = this->BeginCommitUpgradeContext(
            move(storeTx),
            [this](AsyncOperationSPtr const & operation) { this->OnPrepareFinishRollbackComplete(operation, false); },
            thisSPtr);
        this->OnPrepareFinishRollbackComplete(operation, true);
    }
    else
    {
        this->TryRefreshContextAndScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->PrepareFinishRollback(thisSPtr); });
    }

}

void ProcessApplicationUpgradeContextAsyncOperation::OnPrepareFinishRollbackComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ErrorCode error = this->EndCommitUpgradeContext(operation);

    auto const & thisSPtr = operation->Parent;

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0} could not commit rollback updated default service status details due to {1}",
            TraceId,
            error);

        this->TryRefreshContextAndScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->PrepareFinishRollback(thisSPtr); });
    }
    else
    {
        this->FinishRollback(thisSPtr);
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::FinishRollback(AsyncOperationSPtr const & thisSPtr)
{
    WriteNoise(
        TraceComponent, 
        "{0} finishing rollback",
        this->TraceId);

    auto error = RefreshUpgradeContext();
    if (!error.IsSuccess())
    {
        TryScheduleFinishRollback(error, thisSPtr);
        return;
    }

    auto storeTx = this->CreateTransaction();

    // We must update all service packages in the application since the rollback
    // could have changed the rollout versions on the packages
    //
    error = FinishRollbackServicePackages(storeTx);
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent, 
            "{0} could not finish rolling back service packages due to {1}",
            this->TraceId,
            error);

        TryScheduleFinishRollback(error, thisSPtr);
        return;
    }

    appContextUPtr_->SetApplicationAndTypeVersion(
        currentApplication_.Application.Version,
        ServiceModelVersion(currentApplication_.Application.ApplicationTypeVersion));

    error = this->OnFinishRollback(storeTx);
    if (!error.IsSuccess())
    {
        this->TryScheduleFinishRollback(error, thisSPtr);
        return;
    }

    error = this->ClearVerifiedUpgradeDomains(storeTx, *make_unique<StoreDataVerifiedUpgradeDomains>(appContextUPtr_->ApplicationId));
    if (!error.IsSuccess())
    {
        this->TryScheduleFinishRollback(error, thisSPtr);
        return;
    }

    if (!this->TryAcceptGoalStateUpgrade(thisSPtr, storeTx))
    {
        error = this->UpgradeContext.CompleteRollback(storeTx);
        if (!error.IsSuccess())
        {
            this->TryScheduleFinishRollback(error, thisSPtr);
            return;
        }

        error = CompleteApplicationContext(storeTx);
        if (!error.IsSuccess())
        {
            TryScheduleFinishRollback(error, thisSPtr);
            return;
        }

        auto operation = this->BeginCommitUpgradeContext(
            move(storeTx),
            [this](AsyncOperationSPtr const & operation) { this->OnFinishRollbackCommitComplete(operation, false); },
            thisSPtr);
        this->OnFinishRollbackCommitComplete(operation, true);
    }
}

bool ProcessApplicationUpgradeContextAsyncOperation::TryAcceptGoalStateUpgrade(AsyncOperationSPtr const & thisSPtr, __in StoreTransaction & storeTx)
{
    auto goalStateContext = make_unique<GoalStateApplicationUpgradeContext>(this->UpgradeContext.UpgradeDescription.ApplicationName);

    auto error = storeTx.ReadExact(*goalStateContext);

    if (error.IsError(ErrorCodeValue::NotFound))
    {
        WriteInfo(
            TraceComponent, 
            "{0} no goal state upgrade found on rollback completion",
            this->TraceId);

        return false;
    }
    else if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent, 
            "{0} failed to read goal state upgrade: error={1}",
            this->TraceId,
            error);

        this->TryScheduleFinishRollback(error, thisSPtr);

        // Intentionally fall through and return "true" since
        // the retry is being scheduled here and we do not want
        // the caller to proceed as if the goal state doesn't
        // exist.
    }
    else
    {
        auto rollbackInstance = this->UpgradeContext.RollbackInstance;

        auto newUpgradeContext = make_unique<ApplicationUpgradeContext>(
            this->UpgradeContext.ReplicaActivityId,
            goalStateContext->UpgradeDescription,
            goalStateContext->RollbackApplicationTypeVersion,
            goalStateContext->TargetApplicationManifestId,
            ApplicationUpgradeContext::GetNextUpgradeInstance(rollbackInstance),
            goalStateContext->ApplicationDefinitionKind);
        newUpgradeContext->SetSequenceNumber(this->UpgradeContext.SequenceNumber);

        StoreDataApplicationManifest appManifest(appContextUPtr_->TypeName, appContextUPtr_->TypeVersion);
        error = storeTx.ReadExact(appManifest);

        if (error.IsSuccess())
        {
            if (appContextUPtr_->CanAcceptUpgrade(goalStateContext->UpgradeDescription, appManifest))
            {
                WriteInfo(
                    TraceComponent, 
                    "{0} accepted goal state upgrade: {1}",
                    this->TraceId,
                    *goalStateContext);

                error = storeTx.Update(*newUpgradeContext);

                if (error.IsSuccess())
                {
                    error = appContextUPtr_->SetUpgradePending(storeTx, rollbackInstance);
                }
            }
            else
            {
                WriteInfo(
                    TraceComponent, 
                    "{0} already in target version - goal state ignored: {1}",
                    this->TraceId,
                    *goalStateContext);

                newUpgradeContext->UpdateCompletedUpgradeDomains(this->UpgradeContext.TakeCompletedUpgradeDomains());

                error = newUpgradeContext->CompleteRollforward(storeTx);

                if (error.IsSuccess())
                {
                    error = appContextUPtr_->SetUpgradeComplete(storeTx, newUpgradeContext->UpgradeCompletionInstance);
                }
            }
        }

        if (error.IsSuccess())
        {
            this->ResetUpgradeContext(move(*newUpgradeContext));

            error = storeTx.Delete(*goalStateContext);
        }

        if (error.IsSuccess())
        {
            auto operation = this->BeginCommitUpgradeContext(
                move(storeTx),
                [this](AsyncOperationSPtr const & operation) { this->OnFinishAcceptGoalStateUpgrade(operation, false); },
                thisSPtr);
            this->OnFinishAcceptGoalStateUpgrade(operation, true);
        }
        else
        {
            WriteInfo(
                TraceComponent, 
                "{0} failed to apply goal state upgrade: error={1}",
                this->TraceId,
                error);

            this->TryScheduleFinishRollback(error, thisSPtr);
        }
    }

    return true;
}

void ProcessApplicationUpgradeContextAsyncOperation::OnFinishAcceptGoalStateUpgrade(
    AsyncOperationSPtr const & operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }
    
    auto const & thisSPtr = operation->Parent;

    auto error = this->EndCommitUpgradeContext(operation);

    if (error.IsSuccess())
    {
        if (this->UpgradeContext.IsPending)
        {
            this->RestartUpgrade(thisSPtr);
        }
        else
        {
            CMEvents::Trace->UpgradeComplete(this->ReplicaActivityId, this->GetUpgradeContextStringTrace());

            CMEvents::Trace->ApplicationUpgradeComplete(
                appContextUPtr_->ApplicationName.ToString(),
                this->UpgradeContext.RollforwardInstance,
                0, // dca_version
                this->ReplicaActivityId, 
                this->UpgradeContext.UpgradeDescription.TargetApplicationTypeVersion,
                currentManifestId_,
                this->UpgradeContext.TargetApplicationManifestId);

            CMEvents::Trace->ApplicationUpgradeCompleteOperational(
                this->ReplicaActivityId.ActivityId.Guid,
                appContextUPtr_->ApplicationName.ToString(),
                appContextUPtr_->TypeName.Value,
                this->UpgradeContext.UpgradeDescription.TargetApplicationTypeVersion,
                this->UpgradeContext.OverallUpgradeElapsedTime.TotalMillisecondsAsDouble());

            this->TryComplete(thisSPtr, error);
        }
    }
    else
    {
        WriteInfo(
            TraceComponent, 
            "{0} failed to commit goal state upgrade: error={1}",
            this->TraceId,
            error);

        this->TryScheduleFinishRollback(error, thisSPtr);
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::OnFinishRollbackCommitComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto const & thisSPtr = operation->Parent;

    auto error = this->EndCommitUpgradeContext(operation);

    if (error.IsSuccess())
    {
        CMEvents::Trace->RollbackComplete(this->ReplicaActivityId, this->GetUpgradeContextStringTrace());

        CMEvents::Trace->ApplicationUpgradeRollbackComplete(
            appContextUPtr_->ApplicationName.ToString(),
            this->UpgradeContext.RollbackInstance,
            0, // dca_version
            this->ReplicaActivityId, 
            appContextUPtr_->TypeVersion.Value,
            this->UpgradeContext.TargetApplicationManifestId,
            this->appContextUPtr_->ManifestId);

        CMEvents::Trace->ApplicationUpgradeRollbackCompleteOperational(
            this->ReplicaActivityId.ActivityId.Guid,
            appContextUPtr_->ApplicationName.ToString(),
            appContextUPtr_->TypeName.Value,
            appContextUPtr_->TypeVersion.Value,
            this->UpgradeContext.CommonUpgradeContextData.FailureReason,
            this->UpgradeContext.OverallUpgradeElapsedTime.TotalMillisecondsAsDouble());

        this->TryComplete(thisSPtr, error);
    }
    else
    {
        WriteInfo(
            TraceComponent, 
            "{0} could not commit to finish rollback due to {1}",
            this->TraceId,
            error);

        TryScheduleFinishRollback(error, thisSPtr);
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::TryScheduleFinishRollback(
    ErrorCode const & error, 
    AsyncOperationSPtr const & thisSPtr)
{
    auto errorToCheck = error;

    auto refreshError = this->LoadApplicationContext();

    if (!refreshError.IsSuccess())
    {
        errorToCheck.Overwrite(refreshError);
    }

    if (this->ShouldScheduleRetry(errorToCheck, thisSPtr))
    {
        this->StartTimer(
            thisSPtr, 
            [this](AsyncOperationSPtr const & thisSPtr){ this->FinishRollback(thisSPtr); }, 
            this->Replica.GetRandomizedOperationRetryDelay(error));
    }
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::FinishRollbackServicePackages(StoreTransaction const & storeTx)
{
    ErrorCode error(ErrorCodeValue::Success);

    vector<StoreDataServicePackage> & modifiedServicePackages = currentApplication_.ServicePackages;

    for (auto iter = modifiedServicePackages.begin(); iter != modifiedServicePackages.end(); ++iter)
    {
        error = storeTx.UpdateOrInsertIfNotFound(*iter);

        if (!error.IsSuccess())
        {
            return error;
        }
    }

    return error;
}

void ProcessApplicationUpgradeContextAsyncOperation::PrepareStartRollback(AsyncOperationSPtr const & thisSPtr)
{
    WriteInfo(
        TraceComponent, 
        "{0} rollback: reverting application health check policy: {1}",
        this->TraceId,
        manifestHealthPolicy_);

    auto operation = this->BeginUpdateHealthPolicy(
        manifestHealthPolicy_,
        [this](AsyncOperationSPtr const & operation){ this->OnPrepareStartRollbackComplete(operation, false); },
        thisSPtr);
    this->OnPrepareStartRollbackComplete(operation, true);
}

void ProcessApplicationUpgradeContextAsyncOperation::OnPrepareStartRollbackComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    auto error = this->EndUpdateHealthPolicy(operation);

    if (error.IsSuccess())
    {
        this->StartRollback(thisSPtr);
    }
    else
    {
        WriteInfo(
            TraceComponent, 
            "{0} failed to update application health policy: {1}",
            this->TraceId,
            error);

        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->PrepareStartRollback(thisSPtr); });
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::StartRollback(AsyncOperationSPtr const & thisSPtr)
{
    auto storeTx = this->CreateTransaction();

    auto error = RefreshUpgradeContext();

    if (error.IsSuccess())
    {
        if (this->UpgradeContext.UpgradeState != ApplicationUpgradeState::RollingBack)
        {
            auto goalStateContext = make_unique<GoalStateApplicationUpgradeContext>(this->UpgradeContext.UpgradeDescription.ApplicationName);

            error = storeTx.ReadExact(*goalStateContext);

            bool goalStateExists = error.IsSuccess();

            if (error.IsError(ErrorCodeValue::NotFound))
            {
                error = ErrorCodeValue::Success;
            }

            if (error.IsSuccess())
            {
                this->SetCommonContextDataForRollback(goalStateExists);

                if (!this->UpgradeContext.AddedDefaultServices.empty() && !this->UpgradeContext.CommonUpgradeContextData.IsSkipRollbackUD)
                {
                    this->AppendDynamicUpgradeStatusDetails(GET_RC(Delete_Default_Service));
                }

                error = this->ClearVerifiedUpgradeDomains(storeTx, *make_unique<StoreDataVerifiedUpgradeDomains>(appContextUPtr_->ApplicationId));
            }
        }

        if (error.IsSuccess())
        {
            error = this->OnStartRollback(storeTx);
        }

        if (error.IsSuccess())
        {
            error = storeTx.Update(this->UpgradeContext);
        }
    }

    if (error.IsSuccess())
    {
        auto rollbackCommitOperation = this->BeginCommitUpgradeContext(
            move(storeTx),
            [this](AsyncOperationSPtr const & operation) { this->OnStartRollbackCommitComplete(operation, false); },
            thisSPtr);
        this->OnStartRollbackCommitComplete(rollbackCommitOperation, true);
    }
    else
    {
        WriteWarning(
            TraceComponent, 
            "{0} failed to start rollback due to {1}",
            this->TraceId,
            error);

        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->StartRollback(thisSPtr); });
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::OnStartRollbackCommitComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;
    
    auto error = this->EndCommitUpgradeContext(operation);

    if (error.IsSuccess())
    {
        this->StartImageBuilderRollback(thisSPtr);
    }
    else
    {
        WriteWarning(
            TraceComponent, 
            "{0} failed to commit start rollback due to {1}",
            this->TraceId,
            error);

        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->StartRollback(thisSPtr); });
    }
}

void ProcessApplicationUpgradeContextAsyncOperation::StartImageBuilderRollback(AsyncOperationSPtr const & thisSPtr)
{
    auto operation = AsyncOperation::CreateAndStart<ImageBuilderRollbackAsyncOperation>(
        *this,
        [&](AsyncOperationSPtr const & operation) { this->OnImageBuilderRollbackComplete(operation, false); },
        thisSPtr);
    this->OnImageBuilderRollbackComplete(operation, true);
}

void ProcessApplicationUpgradeContextAsyncOperation::OnImageBuilderRollbackComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;

    auto error = ImageBuilderRollbackAsyncOperation::End(operation);

    if (error.IsSuccess())
    {
        this->ResetImageStoreErrorCount();

        error = ProcessUpgradeContextAsyncOperationBase::LoadRollbackMessageBody();
    }

    if (error.IsSuccess())
    {
        CMEvents::Trace->RollbackStart(this->ReplicaActivityId, this->GetUpgradeContextStringTrace());

        CMEvents::Trace->ApplicationUpgradeRollback(
            appContextUPtr_->ApplicationName.ToString(),
            this->UpgradeContext.RollforwardInstance,
            0, // dca_version
            this->ReplicaActivityId, 
            appContextUPtr_->TypeVersion.Value,
            this->UpgradeContext.TargetApplicationManifestId,
            this->appContextUPtr_->ManifestId);

        CMEvents::Trace->ApplicationUpgradeRollbackStartOperational(
            this->ReplicaActivityId.ActivityId.Guid,
            appContextUPtr_->ApplicationName.ToString(),
            appContextUPtr_->TypeName.Value,
            appContextUPtr_->TypeVersion.Value,
            this->UpgradeContext.RollbackApplicationTypeVersion.Value,
            this->UpgradeContext.CommonUpgradeContextData.FailureReason,
            this->UpgradeContext.OverallUpgradeElapsedTime.TotalMillisecondsAsDouble());

        if (!this->UpgradeContext.AddedDefaultServices.empty() && !this->UpgradeContext.CommonUpgradeContextData.IsSkipRollbackUD)
        {
            this->RemoveCreatedDefaultServices(thisSPtr);
        }
        else
        {
            WriteNoise(
                TraceComponent, 
                "{0} rolling back request to FM",
                this->TraceId);

            // Must still send request to FM even if IsSkipRollbackUD is set so
            // that FM has a chance to observe the new application instance corresponding
            // to this rollback.
            //
            this->ScheduleRequestToFM(thisSPtr);
        }
    }
    else
    {
        WriteInfo(
            TraceComponent, 
            "{0} failed to start Image Builder rollback due to {1}",
            this->TraceId,
            error);
        this->TryScheduleRetry(error, thisSPtr, [this](AsyncOperationSPtr const & thisSPtr) { this->StartImageBuilderRollback(thisSPtr); });
    }
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::LoadApplicationContext()
{
    WriteNoise(
        TraceComponent, 
        "{0} loading application context",
        this->TraceId);
    
    auto tempAppContextUPtr = make_unique<ApplicationContext>(this->UpgradeContext.UpgradeDescription.ApplicationName);
    auto storeTx = this->CreateTransaction();

    auto error = storeTx.ReadExact(*tempAppContextUPtr);

    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent, 
            "{0} could not read application context for application '{1}' due to {2}",
            this->TraceId,
            this->UpgradeContext.UpgradeDescription.ApplicationName,
            error);

        storeTx.Rollback();
    }
    else
    {
        appContextUPtr_.swap(tempAppContextUPtr);

        storeTx.CommitReadOnly();
    }

    return error;
}

AsyncOperationSPtr ProcessApplicationUpgradeContextAsyncOperation::BeginLoadApplicationDescriptions(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    WriteNoise(
        TraceComponent, 
        "{0} loading application descriptions",
        this->TraceId);

    return this->ImageBuilder.BeginUpgradeApplication(
        this->ActivityId,
        appContextUPtr_->ApplicationName,
        appContextUPtr_->ApplicationId,
        appContextUPtr_->TypeName,
        ServiceModelVersion(this->UpgradeContext.UpgradeDescription.TargetApplicationTypeVersion),
        appContextUPtr_->ApplicationVersion,
        this->UpgradeContext.UpgradeDescription.ApplicationParameters,
        this->GetImageBuilderTimeout(),
        callback,
        parent);
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::EndLoadApplicationDescriptions(
    AsyncOperationSPtr const & operation)
{
    auto error = this->ImageBuilder.EndUpgradeApplication(
        operation,
        /*out*/ currentApplication_,
        /*out*/ targetApplication_);

    if (error.IsSuccess())
    {
        this->ResetImageStoreErrorCount();

        removedTypes_ = currentApplication_.ComputeRemovedServiceTypes(targetApplication_);

        // This must happen before any validations since any added packages must be cleaned up
        // on validation failure
        //
        addedPackages_ = currentApplication_.ComputeAddedServicePackages(targetApplication_);

        wstring errorDetails;
        if (!targetApplication_.TryValidate(errorDetails) || !currentApplication_.TryValidate(errorDetails))
        {
            error = this->TraceAndGetErrorDetails(ErrorCodeValue::ApplicationUpgradeValidationError, move(errorDetails));
        }
    }
    else
    {
        WriteInfo(
            TraceComponent, 
            "{0} failed UpgradeApplication() call to ImageBuilder for application '{1}' due to {2}",
            this->TraceId,
            this->UpgradeContext.UpgradeDescription.ApplicationName,
            error);
    }

    return error;
}

bool ProcessApplicationUpgradeContextAsyncOperation::IsNetworkValidationNeeded()
{
    return targetApplication_.Networks.size() > 0;
}

AsyncOperationSPtr ProcessApplicationUpgradeContextAsyncOperation::BeginValidateNetworks(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ValidateNetworkMessageBody body(targetApplication_.Networks);

    WriteNoise(
        TraceComponent,
        "{0} sending request to FM to validate networks referred to by application {1} : {2}",
        this->TraceId,
        appContextUPtr_->ApplicationName,
        body);

    MessageUPtr validateNetworkMessage = NIMMessage::GetValidateNetwork().CreateMessage<ValidateNetworkMessageBody>(body);

    NIMMessage::AddActivityHeader(*validateNetworkMessage, FabricActivityHeader(this->ActivityId));
    validateNetworkMessage->Idempotent = true;

    return this->Router.BeginRequestToFM(
        move(validateNetworkMessage),
        TimeSpan::MaxValue,
        this->GetCommunicationTimeout(),
        callback,
        parent);
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::EndValidateNetworks(
    AsyncOperationSPtr const & operation)
{
    MessageUPtr reply;
    auto error = this->Router.EndRequestToFM(operation, reply);

    if (error.IsSuccess())
    {
        BasicFailoverReplyMessageBody body;
        if (reply->GetBody(body))
        {
            error = body.ErrorCodeValue;
        }
        else
        {
            error = ErrorCode::FromNtStatus(reply->GetStatus());
        }

        WriteNoise(
            TraceComponent,
            "{0} request to FM to validate networks referred to by application {1} completed with error : {2}",
            this->TraceId,
            appContextUPtr_->ApplicationName,
            error);
    }
    else
    {
        WriteNoise(
            TraceComponent,
            "{0} request to FM to validate networks referred to by application {1} failed with : {2}",
            this->TraceId,
            appContextUPtr_->ApplicationName,
            error);
    }

    return error;
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::LoadUpgradePolicies()
{
    if (!this->UpgradeContext.UpgradeDescription.IsHealthMonitored)
    {
        return ErrorCodeValue::Success;
    }

    WriteNoise(
        TraceComponent, 
        "{0} loading upgrade policies",
        this->TraceId);

    ApplicationHealthPolicy healthPolicy;
    auto error = this->LoadHealthPolicy(appContextUPtr_->TypeVersion, healthPolicy);

    if (error.IsSuccess())
    {
        manifestHealthPolicy_ = move(healthPolicy);
    }

    return error;
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::LoadHealthPolicy(ServiceModelVersion const & typeVersion, __out ApplicationHealthPolicy & healthPolicy)
{
    StoreDataApplicationManifest manifestData(appContextUPtr_->TypeName, typeVersion);

    auto storeTx = this->CreateTransaction();

    auto error = storeTx.ReadExact(manifestData);

    if (error.IsError(ErrorCodeValue::NotFound))
    {
        WriteInfo(
            TraceComponent, 
            "{0} application type context {1}:{2} not found",
            this->TraceId, 
            appContextUPtr_->TypeName,
            typeVersion);

        error = ErrorCodeValue::Success;
    }

    if (error.IsSuccess())
    {
        healthPolicy = manifestData.HealthPolicy;

        storeTx.CommitReadOnly();
    }

    return error;
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::LoadActiveServices()
{
    WriteNoise(
        TraceComponent, 
        "{0} loading active service types",
        this->TraceId);

    vector<ServiceContext> serviceContexts;
    auto storeTx = this->CreateTransaction();

    auto error = ServiceContext::ReadApplicationServices(storeTx, appContextUPtr_->ApplicationId, /*out*/ serviceContexts);

    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent, 
            "{0} could not read service contexts due to {1}",
            this->TraceId,
            error);

        storeTx.Rollback();
    }
    else
    {
        wstring types(L"[");
        wstring serviceNames(L"[");
        for (auto iter = serviceContexts.begin(); iter != serviceContexts.end(); ++iter)
        {
            if (!iter->IsComplete && !iter->IsPending)
            {
                // Services still undergoing deletion or that have failed
                // creation are treated as inactive for the purposes of
                // upgrade validation.
                //
                WriteInfo(
                    TraceComponent, 
                    "{0} handle pending service context {1} as inactive", 
                    this->TraceId, 
                    (*iter));

                continue;
            }

            if (iter->IsPending)
            {
                // Services still undergoing creation are treated as
                // active for the purposes of upgrade validation.
                //
                WriteInfo(
                    TraceComponent, 
                    "{0} handle pending service context {1} as active", 
                    this->TraceId, 
                    (*iter));
            }

            if (activeServiceTypes_.insert(iter->ServiceTypeName).second == true)
            {
                types.append(iter->ServiceTypeName.Value);
                types.append(L" ");
            }

            if (allActiveServices_.insert(
                pair<NamingUri, PartitionedServiceDescriptor>(iter->AbsoluteServiceName, iter->ServiceDescriptor)).second == true)
            {
                serviceNames.append(iter->ServiceName.Value);
                serviceNames.append(L" ");
            }
        }

        if (error.IsSuccess())
        {
            types.append(L"]");
            serviceNames.append(L"]");

            WriteNoise(
                TraceComponent, 
                "{0} added active services: types={1} serviceNames={2}",
                this->TraceId,
                types,
                serviceNames);

            // Need to send upgrade request to FM regardless of whether or not there
            // are active services since they may still be in the ToBeDeleted state.
            // The client may also delete all services after the upgrade has started.

            storeTx.CommitReadOnly();
        }
    }

    return error;
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::ValidateRemovedServiceTypes()
{
    WriteNoise(
        TraceComponent, 
        "{0} validating removed service types: [{1}]",
        this->TraceId,
        removedTypes_);

    for (auto iter = removedTypes_.begin(); iter != removedTypes_.end(); ++iter)
    {
        auto findIter = activeServiceTypes_.find(iter->TypeName);
        if (findIter != activeServiceTypes_.end())
        {
            return this->TraceAndGetErrorDetails( ErrorCodeValue::ApplicationUpgradeValidationError, wformatString("{0} {1}", GET_RC( ServiceType_Removal ),
                *iter));
        }

        this->UpgradeContext.AddBlockedServiceType(iter->TypeName);
    }

    return ErrorCodeValue::Success;
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::ValidateMovedServiceTypes(vector<StoreDataServicePackage> const & movedTypes)
{
    WriteNoise(
        TraceComponent, 
        "{0} validating moved service types: [{1}]",
        this->TraceId,
        movedTypes);

    for (auto iter = movedTypes.begin(); iter != movedTypes.end(); ++iter)
    {
        auto findIter = activeServiceTypes_.find(iter->TypeName);
        if (findIter != activeServiceTypes_.end())
        {
            return this->TraceAndGetErrorDetails( ErrorCodeValue::ApplicationUpgradeValidationError, wformatString("{0} {1}", GET_RC( ServiceType_Move ),
                *iter));
        }

        this->UpgradeContext.AddBlockedServiceType(iter->TypeName);
    }

    return ErrorCodeValue::Success;
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::LoadMessageBody(
    ApplicationInstanceDescription const & appDescription,
    bool isMonitored,
    bool isManual,
    bool isRollback,
    vector<StoreDataServicePackage> const & packageUpgrades,
    vector<StoreDataServicePackage> const & removedTypes,
    map<ServiceModelPackageName, vector<wstring>> const & codePackages,
    TimeSpan const replicaSetCheckTimeout,
    uint64 instanceId,
    ServiceModel::ServicePackageResourceGovernanceMap && spRGSettings,
    ServiceModel::CodePackageContainersImagesMap && cpContainersImages,
    vector<wstring> && networks,
    __out UpgradeApplicationRequestMessageBody & requestMessage)
{
    auto sequenceNumber = this->UpgradeContext.SequenceNumber;

    if (sequenceNumber < 0)
    {
        TRACE_ERROR_AND_TESTASSERT(TraceComponent, "{0} invalid upgrade context sequence number: {1}", this->TraceId, sequenceNumber);

        auto error = this->RefreshUpgradeContext();
        if (!error.IsSuccess())
        {
            WriteWarning(TraceComponent, "{0} refresh upgrade context failed: {1}", this->TraceId, error);
        }

        return ErrorCodeValue::OperationFailed;
    }

    WriteInfo(
        TraceComponent, 
        "{0} loading message body: vers={1} type.vers={2} rollout.vers={3} replicaSetCheckTimeout={4} instance={5}",
        this->TraceId,
        appDescription.Version,
        appDescription.ApplicationTypeVersion,
        appDescription.ApplicationPackageReference.RolloutVersionValue,
        replicaSetCheckTimeout,
        instanceId);

    WriteInfo(
        TraceComponent, 
        "{0} loading message body: isMonitored={1} isManual={2} seq={3} upgrades=[{4}] removals=[{5}] code=[{6}]",
        this->TraceId,
        isMonitored,
        isManual,
        sequenceNumber,
        packageUpgrades,
        removedTypes,
        codePackages);

    map<ServiceModelPackageName, RolloutVersion> packageVersions;    
    for (auto iter = packageUpgrades.begin(); iter != packageUpgrades.end(); ++iter)
    {
        ServiceModelPackageName const & packageName = iter->PackageName;        

        auto versionIter = packageVersions.find(packageName);
        if (versionIter == packageVersions.end())
        {
            ServicePackageVersion packageVersion;
            auto error = ServicePackageVersion::FromString(iter->PackageVersion.Value, /*out*/ packageVersion);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceComponent, 
                    "{0} could not parse '{1}' as a service package version",
                    this->TraceId,
                    iter->PackageVersion.Value);

                return error;
            }

            packageVersions[packageName] = packageVersion.RolloutVersionValue;            
        }     
    }

    vector<ServicePackageUpgradeSpecification> packageUpgradeSpecifications;
    for (auto iter = packageVersions.begin(); iter != packageVersions.end(); ++iter)
    {
        ServiceModelPackageName const & packageName = iter->first;
        RolloutVersion const & packageVersion = iter->second;

        vector<wstring> codePackageNames;
        auto findIter = codePackages.find(packageName);
        if (findIter != codePackages.end())
        {
            codePackageNames = findIter->second;
        }

        packageUpgradeSpecifications.push_back(ServicePackageUpgradeSpecification(
            packageName.Value,
            packageVersion,
            codePackageNames));
    }

    map<ServiceModelPackageName, ServiceTypeRemovalSpecification> typeRemovalSpecificationsMap;
    for (auto it = removedTypes.begin(); it != removedTypes.end(); ++it)
    {
        ServiceModelPackageName const & packageName = it->PackageName;        

        auto specificationIter = typeRemovalSpecificationsMap.find(packageName);
        if (specificationIter == typeRemovalSpecificationsMap.end())
        {
            RolloutVersion version;

            auto versionIter = packageVersions.find(packageName);
            if (versionIter == packageVersions.end())
            {
                ServicePackageVersion packageVersion;
                auto error = ServicePackageVersion::FromString(it->PackageVersion.Value, /*out*/ packageVersion);
                if (!error.IsSuccess())
                {
                    WriteWarning(
                        TraceComponent, 
                        "{0} could not parse '{1}' as a service package version",
                        this->TraceId,
                        it->PackageVersion.Value);

                    return error;
                }

                version = packageVersion.RolloutVersionValue;

                packageVersions[packageName] = version;
            }     
            else
            {
                version = versionIter->second;
            }

            ServiceTypeRemovalSpecification specification(it->PackageName.Value, version);
            specification.AddServiceTypeName(it->TypeName.Value);

            typeRemovalSpecificationsMap.insert(pair<ServiceModelPackageName, ServiceTypeRemovalSpecification>(it->PackageName, move(specification)));
        }
        else
        {
            specificationIter->second.AddServiceTypeName(it->TypeName.Value);
        }
    }

    vector<ServiceTypeRemovalSpecification> typeRemovalSpecifications;
    for (auto it=typeRemovalSpecificationsMap.begin(); it!=typeRemovalSpecificationsMap.end(); ++it)
    {
        typeRemovalSpecifications.push_back(move(it->second));
    }
    
    ApplicationIdentifier appId;
    auto error = ApplicationIdentifier::FromString(appDescription.ApplicationId, /*out*/ appId);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent, 
            "{0} could not parse '{1}' as an application identifier",
            this->TraceId,
            appDescription.ApplicationId);

        return error;
    }

    ApplicationVersion appVersion = ApplicationVersion(appDescription.ApplicationPackageReference.RolloutVersionValue);

    wstring applicationName = appContextUPtr_->ApplicationName.ToString();

    ApplicationUpgradeSpecification upgradeSpecification(
        appId,
        appVersion, 
        applicationName,
        instanceId,
        this->GetUpgradeType(),
        isMonitored,
        isManual,
        move(packageUpgradeSpecifications),
        move(typeRemovalSpecifications),
        move(spRGSettings),
        move(cpContainersImages),
        move(networks));

    requestMessage = UpgradeApplicationRequestMessageBody(
        move(upgradeSpecification),
        replicaSetCheckTimeout,
        vector<wstring>(),
        sequenceNumber,
        isRollback);

    return ErrorCodeValue::Success;
}

NamingUri ProcessApplicationUpgradeContextAsyncOperation::StringToNamingUri(wstring const & name)
{
    NamingUri namingUri;
    ASSERT_IFNOT(NamingUri::TryParse(name, namingUri), "Could not parse {0} into a NamingUri", name);
    return namingUri;
}

void ProcessApplicationUpgradeContextAsyncOperation::AppendDynamicUpgradeStatusDetails(wstring const & statusToAppend)
{
    // Status details causing upgrade failure should always be preserved
    //
    wstring statusDetails(this->UpgradeContext.CommonUpgradeContextData.UpgradeStatusDetailsAtFailure);

    if (!this->UpgradeContext.CommonUpgradeContextData.UpgradeStatusDetails.empty())
    {
        if (!statusDetails.empty())
        {
            statusDetails.append(L"\n");
        }

        statusDetails.append(this->UpgradeContext.CommonUpgradeContextData.UpgradeStatusDetails);
    }

    if (!statusToAppend.empty())
    {
        if (!statusDetails.empty())
        {
            statusDetails.append(L"\n");
        }

        statusDetails.append(statusToAppend);
    }

    this->UpgradeContext.CommonUpgradeContextData.UpgradeStatusDetails = statusDetails;
}

void ProcessApplicationUpgradeContextAsyncOperation::SetDynamicUpgradeStatusDetails(wstring const & statusDetails)
{
    this->UpgradeContext.CommonUpgradeContextData.UpgradeStatusDetails = L"";

    this->AppendDynamicUpgradeStatusDetails(statusDetails);
}

ErrorCode ProcessApplicationUpgradeContextAsyncOperation::LoadDefaultServices()
{
    // Update and deletion of default services are enabled only when this switch equals true.
    bool isDefaultServicesUpgradeEnabled = this->Get_EnableDefaultServicesUpgrade();

    WriteNoise(
        TraceComponent, 
        "{0} loading default services. isDefaultServicesUpgradeEnabled = {1}",
        this->TraceId,
        isDefaultServicesUpgradeEnabled);

    this->UpgradeContext.ClearAddedDefaultServices();
    this->UpgradeContext.ClearDeletedDefaultServices();
    activeDefaultServiceDescriptions_.clear();

    vector<PartitionedServiceDescriptor> const & targetDefaultServices = targetApplication_.DefaultServices;
    vector<PartitionedServiceDescriptor> const & currentDefaultServices = currentApplication_.DefaultServices;

    for (auto iter = targetDefaultServices.begin(); iter != targetDefaultServices.end(); ++iter)
    {
        // the service type is not yet fully qualified
        Naming::PartitionedServiceDescriptor const & incompletePSD = (*iter);

        bool packageFound = false;

        ServiceContext serviceContextToAdd;
        for (auto iter2 = targetApplication_.ServicePackages.begin(); iter2 != targetApplication_.ServicePackages.end(); ++iter2)
        {
            if (ServiceModelTypeName(incompletePSD.Service.Type.ServiceTypeName) == iter2->TypeName)
            {
                packageFound = true;

                serviceContextToAdd = ServiceContext(
                    this->UpgradeContext.ReplicaActivityId,
                    appContextUPtr_->ApplicationId,
                    iter2->PackageName,
                    iter2->PackageVersion,
                    this->UpgradeContext.UpgradeCompletionInstance,
                    incompletePSD);

                break;
            }
        }

        // validate against default services for service types that do not exist
        if (!packageFound)
        {
            return this->TraceAndGetErrorDetails( ErrorCodeValue::ApplicationUpgradeValidationError, wformatString("{0} ({1}, {2})", GET_RC( Default_ServiceType ),
                incompletePSD.Service.Name,
                incompletePSD.Service.Type));
        }

        // the PSD in the service context has a qualified service type
        Naming::PartitionedServiceDescriptor const & completePSD = serviceContextToAdd.ServiceDescriptor;

        // compare to active default services
        auto findIter = allActiveServices_.find(serviceContextToAdd.AbsoluteServiceName);
        // Creating default service is always allowed
        if (findIter == allActiveServices_.end())
        {
            WriteNoise(
                TraceComponent, 
                "{0} adding default service: {1}",
                this->TraceId,
                serviceContextToAdd.ServiceName);

            this->UpgradeContext.PushAddedDefaultService(move(serviceContextToAdd));
        }
        else if (isDefaultServicesUpgradeEnabled)
        {
            ErrorCode error = PartitionedServiceDescriptor::IsUpdateServiceAllowed(
                    findIter->second,
                    completePSD,
                    TraceComponent,
                    this->TraceId);

            if (!error.IsSuccess())
            {
                return this->TraceAndGetErrorDetails(
                    ErrorCodeValue::ApplicationUpgradeValidationError,
                    wformatString(GET_RC(Validate_Update_Default_Service_Fail), error.TakeMessage()));
            }

            activeDefaultServiceDescriptions_.push_back(move(completePSD));
        }
        else
        {
            auto error = findIter->second.Equals(completePSD);

            if(!error.IsSuccess())
            {
                WriteWarning(
                    TraceComponent,
                    "{0} modified default service: {1} -> {2}",
                    this->TraceId,
                    findIter->second,
                    completePSD);

                return this->TraceAndGetErrorDetails(ErrorCodeValue::ApplicationUpgradeValidationError, wformatString(GET_RC(Default_Service_Description), completePSD.Service.Name, error.TakeMessage()));
            }
        }
    } // for all default service types in target application

    if (isDefaultServicesUpgradeEnabled)
    {
        for (auto iter = currentDefaultServices.begin(); iter != currentDefaultServices.end(); ++iter)
        {
            auto findIter = allActiveServices_.find(StringToNamingUri(iter->Service.Name));
            if (findIter == allActiveServices_.end())
            {
                continue;
            }

            auto targetIter = find_if(targetDefaultServices.begin(), targetDefaultServices.end(),
                [&iter](PartitionedServiceDescriptor const & psd)
                {
                    return psd.Service.Name == iter->Service.Name;
                });

            if (targetIter == targetDefaultServices.end())
            {
                WriteNoise(
                    TraceComponent,
                    "{0} deleting default services: {1}",
                    this->TraceId,
                    iter->Service.Name);

                this->UpgradeContext.PushDeletedDefaultServices(StringToNamingUri(iter->Service.Name));
            }
        } // for all default services in current application
    }
    
    return ErrorCodeValue::Success;
}

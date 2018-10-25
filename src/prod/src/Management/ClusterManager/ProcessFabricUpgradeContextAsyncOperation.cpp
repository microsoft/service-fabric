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
using namespace Management::HealthManager;

using namespace Management::ClusterManager;

class ProcessFabricUpgradeContextAsyncOperation::InitializeUpgradeAsyncOperation : public AsyncOperation
{
public:
    InitializeUpgradeAsyncOperation(
        __in ProcessFabricUpgradeContextAsyncOperation & owner,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<InitializeUpgradeAsyncOperation>(operation)->Error;
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = this->InitializeUpgrade();

        if (error.IsSuccess() && owner_.UpgradeContext.IsPreparingUpgrade)
        {
            this->InitializeHealthPolicies(thisSPtr);

            owner_.SetCommonContextDataForUpgrade();
        }
        else
        {
            this->TryComplete(thisSPtr, error);
        }
    }

private:

    ErrorCode InitializeUpgrade()
    {
        auto error = owner_.RefreshUpgradeContext();
        if (!error.IsSuccess()) { return error; }

        bool isConfigOnly = false;

        error = owner_.ImageBuilder.UpgradeFabric(
            owner_.UpgradeContext.CurrentVersion,
            owner_.UpgradeContext.UpgradeDescription.Version,
            owner_.GetImageBuilderTimeout(),
            isConfigOnly);
        if (!error.IsSuccess()) { return error; }

        auto currentCodeVersion = owner_.UpgradeContext.CurrentVersion.CodeVersion;
        auto targetCodeVersion = owner_.UpgradeContext.UpgradeDescription.Version.CodeVersion;

        if (currentCodeVersion.IsValid && currentCodeVersion == targetCodeVersion)
        {
            owner_.IsConfigOnly = isConfigOnly;
        }
        else
        {
            owner_.IsConfigOnly = false;
        }

        owner_.ResetImageStoreErrorCount();

        return ErrorCodeValue::Success;
    }

    void InitializeHealthPolicies(AsyncOperationSPtr const & thisSPtr)
    {
        // Ensure that HM has the correct policy throughout the upgrade since it could failover to nodes
        // with different cluster manifests.
        //
        auto healthPolicy = make_shared<ClusterHealthPolicy>(
            owner_.UpgradeContext.UpgradeDescription.IsHealthPolicyValid 
                ? owner_.UpgradeContext.UpgradeDescription.HealthPolicy 
                : owner_.UpgradeContext.ManifestHealthPolicy);

        auto upgradeHealthPolicy = make_shared<ClusterUpgradeHealthPolicy>(
            owner_.UpgradeContext.UpgradeDescription.IsUpgradeHealthPolicyValid
                ? owner_.UpgradeContext.UpgradeDescription.UpgradeHealthPolicy
                : owner_.UpgradeContext.ManifestUpgradeHealthPolicy);

        auto applicationHealthPolicies = owner_.UpgradeContext.UpgradeDescription.ApplicationHealthPolicies;
        
        WriteInfo(
            owner_.TraceComponent, 
            "{0} rollforward: initializing cluster health check policies: {1} (fromAPI={2}) {3} (fromAPI={4}), {5} application health policies",
            owner_.TraceId,
            *healthPolicy,
            owner_.UpgradeContext.UpgradeDescription.IsHealthPolicyValid,
            *upgradeHealthPolicy,
            owner_.UpgradeContext.UpgradeDescription.IsUpgradeHealthPolicyValid,
            applicationHealthPolicies ? applicationHealthPolicies->size() : 0u);

        auto operation = owner_.BeginUpdateHealthPolicy(
            healthPolicy,
            upgradeHealthPolicy,
            applicationHealthPolicies,
            [this](AsyncOperationSPtr const & operation) { this->OnUpdateHealthPolicyComplete(operation, false); },
            thisSPtr);
        this->OnUpdateHealthPolicyComplete(operation, true);
    }

private:

    void OnUpdateHealthPolicyComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto error = owner_.EndUpdateHealthPolicy(operation);

        this->TryComplete(operation->Parent, error);
    }

    ProcessFabricUpgradeContextAsyncOperation & owner_;
};

//
// ProcessFabricUpgradeContextAsyncOperation
//

ProcessFabricUpgradeContextAsyncOperation::ProcessFabricUpgradeContextAsyncOperation(
    __in RolloutManager & rolloutManager,
    __in FabricUpgradeContext & upgradeContext,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : ProcessUpgradeContextAsyncOperationBase(
        rolloutManager,
        upgradeContext,
        "FabricUpgradeContext",
        TimeSpan::MaxValue,
        callback, 
        root)
    , clusterStateSnapshotData_(make_unique<StoreDataClusterUpgradeStateSnapshot>())
{
}

ErrorCode ProcessFabricUpgradeContextAsyncOperation::End(AsyncOperationSPtr const & operation)
{
    return AsyncOperation::End<ProcessFabricUpgradeContextAsyncOperation>(operation)->Error;
}

Reliability::RSMessage const & ProcessFabricUpgradeContextAsyncOperation::GetUpgradeMessageTemplate()
{
    return Reliability::RSMessage::GetFabricUpgradeRequest();
}

TimeSpan ProcessFabricUpgradeContextAsyncOperation::GetUpgradeStatusPollInterval()
{
    return ManagementConfig::GetConfig().FabricUpgradeStatusPollInterval;
}

TimeSpan ProcessFabricUpgradeContextAsyncOperation::GetHealthCheckInterval()
{
    return ManagementConfig::GetConfig().FabricUpgradeHealthCheckInterval;
}

ErrorCode ProcessFabricUpgradeContextAsyncOperation::LoadRollforwardMessageBody(
    __out UpgradeFabricRequestMessageBody & requestBody)
{
    auto upgradeType = this->UpgradeContext.UpgradeDescription.UpgradeType;

    switch (upgradeType)
    {
        case UpgradeType::Rolling:
            // Intentional no-op: ImageBuilder will analyze FabricSettings
            // to determine if any static settings have changed.
            //
            break;

        case UpgradeType::Rolling_ForceRestart:
            // Intentional no-op
            //
            break;

        case UpgradeType::Rolling_NotificationOnly:
            // Intentional fall-through: NotificationOnly is not exposed through the 
            // public API so we don't expect to hit this case
            //
        default:
            WriteWarning(
                TraceComponent, 
                "{0} unsupported upgrade type {1}",
                this->TraceId,
                upgradeType);

            return ErrorCodeValue::FabricUpgradeValidationError;
    }

    return LoadMessageBody(
        this->UpgradeContext.UpgradeDescription.Version,
        this->GetRollforwardReplicaSetCheckTimeout(),
        this->UpgradeContext.RollforwardInstance,
        this->UpgradeContext.UpgradeDescription.IsInternalMonitored,
        this->UpgradeContext.UpgradeDescription.IsUnmonitoredManual,
        false, // IsRollback
        requestBody);
}

ErrorCode ProcessFabricUpgradeContextAsyncOperation::LoadRollbackMessageBody(
    __out UpgradeFabricRequestMessageBody & requestBody)
{
    return LoadMessageBody(
        this->UpgradeContext.CurrentVersion,
        this->GetRollbackReplicaSetCheckTimeout(),
        this->UpgradeContext.RollbackInstance,
        //
        // UnmonitoredManual is supported during rollback after 5.2 CU2, but not during the
        // the rollback of an upgrade from a lower version. A CM primary on versions >= 5.2CU2
        // will end up defaulting to manual rather than auto. The workaround is simple
        // (either modify the upgrade back to auto explicitly or move the CM primary to an
        // old version node) but this check will avoid having to use the workaround at all. 
        //
        (this->UpgradeContext.CurrentVersion.CodeVersion < FabricCodeVersion(5, 2, 210, 9590))
            ? false // always UnmonitoredAuto
            : this->UpgradeContext.UpgradeDescription.IsInternalMonitored,
        this->UpgradeContext.UpgradeDescription.IsUnmonitoredManual,
        true, // IsRollback
        requestBody);
}

void ProcessFabricUpgradeContextAsyncOperation::TraceQueryableUpgradeStart()
{
    CMEvents::Trace->ApplicationUpgradeStart(
        SystemServiceApplicationNameHelper::SystemServiceApplicationName,
        this->UpgradeContext.RollforwardInstance,
        0, // dca_version
        this->ReplicaActivityId, 
        this->UpgradeContext.UpgradeDescription.Version.ToString(),
        L"", // currentManifestId
        L""); // targetManifestId

    CMEvents::Trace->ClusterUpgradeStartOperational(
        this->ReplicaActivityId.ActivityId.Guid,
        this->UpgradeContext.CurrentVersion.ToString(),
        this->UpgradeContext.UpgradeDescription.Version.ToString(),
        this->UpgradeContext.UpgradeDescription.UpgradeType,
        this->UpgradeContext.UpgradeDescription.RollingUpgradeMode,
        this->UpgradeContext.UpgradeDescription.MonitoringPolicy.FailureAction); 
}

void ProcessFabricUpgradeContextAsyncOperation::TraceQueryableUpgradeDomainComplete(std::vector<std::wstring> & recentlyCompletedUDs)
{
    CMEvents::Trace->ApplicationUpgradeDomainComplete(
        SystemServiceApplicationNameHelper::SystemServiceApplicationName,
        this->UpgradeContext.RollforwardInstance,
        0, // dca_version
        this->ReplicaActivityId, 
        (this->UpgradeContext.UpgradeState == FabricUpgradeState::RollingBack ?
            this->UpgradeContext.CurrentVersion.ToString() :
            this->UpgradeContext.UpgradeDescription.Version.ToString()),
        wformatString(recentlyCompletedUDs));

    CMEvents::Trace->ClusterUpgradeDomainCompleteOperational(
        this->ReplicaActivityId.ActivityId.Guid,
        (this->UpgradeContext.UpgradeState == FabricUpgradeState::RollingBack ?
            this->UpgradeContext.CurrentVersion.ToString() :
            this->UpgradeContext.UpgradeDescription.Version.ToString()),
        this->UpgradeContext.UpgradeState,
        wformatString(recentlyCompletedUDs),
        this->UpgradeContext.UpgradeDomainElapsedTime.TotalMillisecondsAsDouble());
}

std::wstring ProcessFabricUpgradeContextAsyncOperation::GetTraceAnalysisContext()
{
    return wformatString(
        "mode={0} target={1} current={2} state={3}",
        this->UpgradeContext.UpgradeDescription.RollingUpgradeMode,
        this->UpgradeContext.UpgradeDescription.Version,
        this->UpgradeContext.CurrentVersion,
        this->UpgradeContext.UpgradeState);
}

AsyncOperationSPtr ProcessFabricUpgradeContextAsyncOperation::BeginInitializeUpgrade(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<InitializeUpgradeAsyncOperation>(*this, callback, parent);
}

ErrorCode ProcessFabricUpgradeContextAsyncOperation::EndInitializeUpgrade(
    AsyncOperationSPtr const & operation)
{
    return InitializeUpgradeAsyncOperation::End(operation);
}

AsyncOperationSPtr ProcessFabricUpgradeContextAsyncOperation::BeginOnValidationError(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
        ErrorCodeValue::Success,
        callback,
        parent);
}

ErrorCode ProcessFabricUpgradeContextAsyncOperation::EndOnValidationError(
    AsyncOperationSPtr const & operation)
{
    return CompletedAsyncOperation::End(operation);
}

ErrorCode ProcessFabricUpgradeContextAsyncOperation::OnFinishStartUpgrade(StoreTransaction const & storeTx)
{
    auto isPreparing = this->UpgradeContext.IsPreparingUpgrade;
    bool enableDeltas = this->UpgradeContext.UpgradeDescription.EnableDeltaHealthEvaluation;

    if (isPreparing && enableDeltas)
    {
        ClusterUpgradeStateSnapshot snapshot;
        auto error = this->HealthAggregator.GetClusterUpgradeSnapshot(
            this->ActivityId,
            snapshot);
        if (error.IsSuccess())
        {
            WriteInfo(
                TraceComponent, 
                "{0} loaded cluster upgrade snapshot: {1}",
                this->TraceId,
                snapshot);
        }
        else
        {
            WriteInfo(
                TraceComponent, 
                "{0} failed to get cluster upgrade snapshot: error={1}",
                this->TraceId,
                error);
            return error;
        }

        // Update is committed by base class
        //
        clusterStateSnapshotData_ = make_unique<StoreDataClusterUpgradeStateSnapshot>(move(snapshot));
        return storeTx.UpdateOrInsertIfNotFound(*clusterStateSnapshotData_);
    }
    else
    {
        WriteInfo(
            TraceComponent, 
            "{0} skip loading cluster upgrade snapshot: preparing={1} enabledDeltas={2}",
            this->TraceId,
            isPreparing,
            enableDeltas);

        return ErrorCodeValue::Success;
    }
}

ErrorCode ProcessFabricUpgradeContextAsyncOperation::OnProcessFMReply(StoreTransaction const & storeTx)
{
    bool enableDeltas = this->UpgradeContext.UpgradeDescription.EnableDeltaHealthEvaluation;
    auto upgradeState = this->UpgradeContext.UpgradeState;

    if (!enableDeltas || upgradeState == FabricUpgradeState::RollingBack)
    {
        return ErrorCodeValue::Success;
    }

    // This is normally not needed, but UDs are dynamically detected by the FM.
    // Handle the edge-case where the FM discovers new UDs after the cluster
    // upgrade has already started.
    //
    vector<wstring> unknownUDs;

    for (auto const & ud : this->UpgradeContext.CompletedUpgradeDomains)
    {
        if (!clusterStateSnapshotData_->StateSnapshot.HasUpgradeDomainEntry(ud))
        {
            unknownUDs.push_back(ud);
        }
    }

    for (auto const & ud : this->UpgradeContext.PendingUpgradeDomains)
    {
        if (!clusterStateSnapshotData_->StateSnapshot.HasUpgradeDomainEntry(ud))
        {
            unknownUDs.push_back(ud);
        }
    }

    if (unknownUDs.empty())
    {
        return ErrorCodeValue::Success;
    }
    else
    {
        auto error = this->HealthAggregator.AppendToClusterUpgradeSnapshot(
            this->ActivityId,
            unknownUDs,
            clusterStateSnapshotData_->StateSnapshot);
        if (error.IsSuccess())
        {
            WriteInfo(
                TraceComponent, 
                "{0} appended new UDs to cluster upgrade snapshot: {1}",
                this->TraceId,
                clusterStateSnapshotData_->StateSnapshot);
        }
        else
        {
            WriteInfo(
                TraceComponent, 
                "{0} failed to append new UDs to cluster upgrade snapshot: UDs={1} error={2}",
                this->TraceId,
                unknownUDs,
                error);
            return error;
        }

        // Update is committed by base class
        //
        return storeTx.UpdateOrInsertIfNotFound(*clusterStateSnapshotData_);
    }
}

ErrorCode ProcessFabricUpgradeContextAsyncOperation::OnRefreshUpgradeContext(StoreTransaction const & storeTx)
{
    bool enableDeltas = this->UpgradeContext.UpgradeDescription.EnableDeltaHealthEvaluation;
    if (!enableDeltas)
    {
        return ErrorCodeValue::Success;
    }

    // State snapshot needs to be reloaded on CM failover 
    // and when a monitored upgrade fails and gets resumed.
    //
    auto error = storeTx.ReadExact(*clusterStateSnapshotData_);

    if (error.IsError(ErrorCodeValue::NotFound))
    {
        error = ErrorCodeValue::Success;
    }

    return error;
}

void ProcessFabricUpgradeContextAsyncOperation::FinalizeUpgrade(AsyncOperationSPtr const & thisSPtr)
{
    this->PrepareFinishUpgrade(thisSPtr);
}

AsyncOperationSPtr ProcessFabricUpgradeContextAsyncOperation::BeginUpdateHealthPolicyForHealthCheck(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & thisSPtr)
{
    ClusterHealthPolicySPtr healthPolicy;
    if (this->UpgradeContext.UpgradeDescription.IsHealthPolicyValid)
    { 
        healthPolicy = make_shared<ClusterHealthPolicy>(this->UpgradeContext.UpgradeDescription.HealthPolicy);
    }

    ClusterUpgradeHealthPolicySPtr upgradeHealthPolicy;
    if (this->UpgradeContext.UpgradeDescription.IsUpgradeHealthPolicyValid)
    {
        upgradeHealthPolicy = make_shared<ClusterUpgradeHealthPolicy>(this->UpgradeContext.UpgradeDescription.UpgradeHealthPolicy);
    }

    auto applicationHealthPolicies = this->UpgradeContext.UpgradeDescription.ApplicationHealthPolicies;

    WriteInfo(
        TraceComponent, 
        "{0} rollforward: updating cluster health check policy: {1} {2} {3} applicationHealthPolicies",
        this->TraceId,
        healthPolicy ? wformatString("{0}", *healthPolicy) : L"nullptr",
        upgradeHealthPolicy ? wformatString("{0}", *upgradeHealthPolicy) : L"nullptr",
        applicationHealthPolicies ? applicationHealthPolicies->size() : 0u);

    return this->BeginUpdateHealthPolicy(
        healthPolicy,
        upgradeHealthPolicy,
        applicationHealthPolicies,
        callback,
        thisSPtr);
}

ErrorCode ProcessFabricUpgradeContextAsyncOperation::EvaluateHealth(
    AsyncOperationSPtr const & thisSPtr,
    __out bool & isHealthy, 
    __out std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations)
{
    ClusterUpgradeStateSnapshot invalidSnapshot;
    bool enableDeltas = this->UpgradeContext.UpgradeDescription.EnableDeltaHealthEvaluation;

    vector<wstring> applicationsWithoutAppType;
    auto error = this->HealthAggregator.IsClusterHealthy(
        this->ActivityId,
        this->UpgradeContext.CompletedUpgradeDomains,
        (enableDeltas ? clusterStateSnapshotData_->StateSnapshot : invalidSnapshot),
        isHealthy,
        unhealthyEvaluations,
        applicationsWithoutAppType);

    if (error.IsError(ErrorCodeValue::ApplicationTypeNotFound))
    {
        this->Replica.ReportApplicationsType(TraceComponent, this->ActivityId, thisSPtr, applicationsWithoutAppType, this->GetCommunicationTimeout());
    }

    return error;
}

ErrorCode ProcessFabricUpgradeContextAsyncOperation::LoadVerifiedUpgradeDomains(
    __in StoreTransaction & storeTx,
    __out std::vector<std::wstring> & verifiedDomains)
{
    auto storeData = make_unique<StoreDataVerifiedFabricUpgradeDomains>();
    auto error = storeTx.ReadExact(*storeData);

    if (error.IsSuccess())
    {
        verifiedDomains = move(storeData->UpgradeDomains);
    }

    return error;
}

ErrorCode ProcessFabricUpgradeContextAsyncOperation::UpdateVerifiedUpgradeDomains(
    __in StoreTransaction & storeTx,
    std::vector<std::wstring> && verifiedDomains)
{
    auto storeData = make_unique<StoreDataVerifiedFabricUpgradeDomains>(move(verifiedDomains));
    return storeTx.UpdateOrInsertIfNotFound(*storeData);
}

AsyncOperationSPtr ProcessFabricUpgradeContextAsyncOperation::BeginUpdateHealthPolicy(
    ClusterHealthPolicySPtr const & policy, 
    ClusterUpgradeHealthPolicySPtr const & upgradePolicy,
    ApplicationHealthPolicyMapSPtr const & applicationHealthPolicies,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return this->HealthAggregator.BeginUpdateClusterHealthPolicy(
        this->ActivityId,
        policy,
        upgradePolicy,
        applicationHealthPolicies,
        this->GetCommunicationTimeout(),
        callback,
        parent);
}

ErrorCode ProcessFabricUpgradeContextAsyncOperation::EndUpdateHealthPolicy(AsyncOperationSPtr const & operation)
{
    auto error = this->HealthAggregator.EndUpdateClusterHealthPolicy(operation);
    
    if (error.IsError(ErrorCodeValue::HealthStaleReport))
    {
        error = ErrorCodeValue::Success;
    }

    return error;
}

void ProcessFabricUpgradeContextAsyncOperation::PrepareFinishUpgrade(AsyncOperationSPtr const & thisSPtr)
{
    // the cluster manifest will be consistent throughout the cluster after upgrade has finished
    //
    auto const & config = Management::ManagementConfig::GetConfig();
    auto healthPolicy = make_shared<ClusterHealthPolicy>(config.GetClusterHealthPolicy());
    auto upgradeHealthPolicy = make_shared<ClusterUpgradeHealthPolicy>(config.GetClusterUpgradeHealthPolicy());
    
    WriteInfo(
        TraceComponent, 
        "{0} finished upgrade: updating cluster health check policy: {1}, upgrade {2}",
        this->TraceId,
        healthPolicy,
        upgradeHealthPolicy);

    auto operation = this->BeginUpdateHealthPolicy(
        healthPolicy,
        upgradeHealthPolicy,
        nullptr, // no ApplicationHealthPolicies set outside the upgrade
        [this](AsyncOperationSPtr const & operation){ this->OnPrepareFinishUpgradeComplete(operation, false); },
        thisSPtr);
    this->OnPrepareFinishUpgradeComplete(operation, true);
}

void ProcessFabricUpgradeContextAsyncOperation::OnPrepareFinishUpgradeComplete(
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
            "{0} failed to update cluster health policy: {1}",
            this->TraceId,
            error);

        this->TrySchedulePrepareFinishUpgrade(error, thisSPtr);
    }
}

void ProcessFabricUpgradeContextAsyncOperation::FinishUpgrade(AsyncOperationSPtr const & thisSPtr)
{
    WriteNoise(
        TraceComponent, 
        "{0} finishing Fabric upgrade",
        this->TraceId);

    auto error = this->RefreshUpgradeContext();
    if (!error.IsSuccess())
    {
        this->TrySchedulePrepareFinishUpgrade(error, thisSPtr);
        return;
    }

    auto storeTx = this->CreateTransaction();

    error = this->UpgradeContext.CompleteRollforward(storeTx);
    if (!error.IsSuccess())
    {
        this->TrySchedulePrepareFinishUpgrade(error, thisSPtr);
        return;
    }

    error = this->ClearVerifiedUpgradeDomains<StoreDataVerifiedFabricUpgradeDomains>(storeTx, *make_unique<StoreDataVerifiedFabricUpgradeDomains>());
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

void ProcessFabricUpgradeContextAsyncOperation::OnFinishUpgradeCommitComplete(
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
            SystemServiceApplicationNameHelper::SystemServiceApplicationName,
            this->UpgradeContext.RollforwardInstance,
            0, // dca_version
            this->ReplicaActivityId, 
            this->UpgradeContext.UpgradeDescription.Version.ToString(),
            L"", // currentManifestId
            L""); // targetManifestId

        CMEvents::Trace->ClusterUpgradeCompleteOperational(
            this->ReplicaActivityId.ActivityId.Guid,
            this->UpgradeContext.UpgradeDescription.Version.ToString(),
            this->UpgradeContext.OverallUpgradeElapsedTime.TotalMillisecondsAsDouble());

        this->TryComplete(thisSPtr, error);
    }
    else
    {
        WriteInfo(
            TraceComponent, 
            "{0} could not commit to finish Fabric upgrade due to {1}",
            this->TraceId,
            error);

        this->TrySchedulePrepareFinishUpgrade(error, thisSPtr);
    }
}

void ProcessFabricUpgradeContextAsyncOperation::TrySchedulePrepareFinishUpgrade(
    ErrorCode const & error, 
    AsyncOperationSPtr const & thisSPtr)
{
    if (this->ShouldScheduleRetry(error, thisSPtr))
    {
        this->StartTimer(
            thisSPtr, 
            [this](AsyncOperationSPtr const & thisSPtr){ this->PrepareFinishUpgrade(thisSPtr); }, 
            this->Replica.GetRandomizedOperationRetryDelay(error));
    }
}

void ProcessFabricUpgradeContextAsyncOperation::FinalizeRollback(AsyncOperationSPtr const & thisSPtr)
{
    WriteNoise(
        TraceComponent, 
        "{0} finishing Fabric rollback",
        this->TraceId);

    auto error = this->RefreshUpgradeContext();
    if (!error.IsSuccess())
    {
        this->TryScheduleFinalizeRollback(error, thisSPtr);
        return;
    }

    auto storeTx = this->CreateTransaction();

    error = this->UpgradeContext.CompleteRollback(storeTx);
    if (!error.IsSuccess())
    {
        this->TryScheduleFinalizeRollback(error, thisSPtr);
        return;
    }

    error = this->ClearVerifiedUpgradeDomains<StoreDataVerifiedFabricUpgradeDomains>(storeTx, *make_unique<StoreDataVerifiedFabricUpgradeDomains>());
    if (!error.IsSuccess())
    {
        this->TryScheduleFinalizeRollback(error, thisSPtr);
        return;
    }

    auto operation = this->BeginCommitUpgradeContext(
        move(storeTx),
        [this](AsyncOperationSPtr const & operation) { this->OnFinalizeRollbackCommitComplete(operation, false); },
        thisSPtr);
    this->OnFinalizeRollbackCommitComplete(operation, true);
}

void ProcessFabricUpgradeContextAsyncOperation::OnFinalizeRollbackCommitComplete(
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
            SystemServiceApplicationNameHelper::SystemServiceApplicationName,
            this->UpgradeContext.RollforwardInstance,
            0, // dca_version
            this->ReplicaActivityId, 
            this->UpgradeContext.CurrentVersion.ToString(),
            L"", // currentManifestId before rollback
            L""); // targetManifestId after rollback

        CMEvents::Trace->ClusterUpgradeRollbackCompleteOperational(
            this->ReplicaActivityId.ActivityId.Guid,
            this->UpgradeContext.CurrentVersion.ToString(),
            this->UpgradeContext.CommonUpgradeContextData.FailureReason,
            this->UpgradeContext.OverallUpgradeElapsedTime.TotalMillisecondsAsDouble());

        this->TryComplete(thisSPtr, error);
    }
    else
    {
        WriteInfo(
            TraceComponent, 
            "{0} could not commit to finish Fabric rollback due to {1}",
            this->TraceId,
            error);

        this->TryScheduleFinalizeRollback(error, thisSPtr);
    }
}

void ProcessFabricUpgradeContextAsyncOperation::TryScheduleFinalizeRollback(
    ErrorCode const & error, 
    AsyncOperationSPtr const & thisSPtr)
{
    if (this->ShouldScheduleRetry(error, thisSPtr))
    {
        this->StartTimer(
            thisSPtr, 
            [this](AsyncOperationSPtr const & thisSPtr){ this->FinalizeRollback(thisSPtr); }, 
            this->Replica.GetRandomizedOperationRetryDelay(error));
    }
}

void ProcessFabricUpgradeContextAsyncOperation::PrepareStartRollback(AsyncOperationSPtr const & thisSPtr)
{
    auto healthPolicy = make_shared<ClusterHealthPolicy>(this->UpgradeContext.ManifestHealthPolicy);
    auto upgradeHealthPolicy = make_shared<ClusterUpgradeHealthPolicy>(this->UpgradeContext.ManifestUpgradeHealthPolicy);
    
    WriteInfo(
        TraceComponent, 
        "{0} rollback: reverting cluster health check policy: {1}",
        this->TraceId,
        healthPolicy);

    auto operation = this->BeginUpdateHealthPolicy(
        healthPolicy,
        upgradeHealthPolicy,
        nullptr, // no ApplicationHealthPolicyMap defined outside the upgrade
        [this](AsyncOperationSPtr const & operation){ this->OnPrepareStartRollbackComplete(operation, false); },
        thisSPtr);
    this->OnPrepareStartRollbackComplete(operation, true);
}

void ProcessFabricUpgradeContextAsyncOperation::OnPrepareStartRollbackComplete(
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
            "{0} failed to update cluster health policy: {1}",
            this->TraceId,
            error);

        this->TrySchedulePrepareStartRollback(error, thisSPtr);
    }
}

void ProcessFabricUpgradeContextAsyncOperation::StartRollback(AsyncOperationSPtr const & thisSPtr)
{
    auto error = this->RefreshUpgradeContext();

    auto storeTx = this->CreateTransaction();

    if (error.IsSuccess())
    {
        if (!this->UpgradeContext.CurrentVersion.IsValid)
        {
            WriteWarning(
                TraceComponent, 
                "{0} cannot rollback: failing upgrade",
                this->TraceId);

            this->TryComplete(thisSPtr, ErrorCodeValue::OperationFailed);
            return;
        }
        else if (this->UpgradeContext.UpgradeState != FabricUpgradeState::RollingBack)
        {
            this->SetCommonContextDataForRollback(false); // goalStateExists

            error = storeTx.Update(this->UpgradeContext);

            if (error.IsSuccess())
            {
                error = this->ClearVerifiedUpgradeDomains<StoreDataVerifiedFabricUpgradeDomains>(storeTx, *make_unique<StoreDataVerifiedFabricUpgradeDomains>());
            }
        }
    }

    if (error.IsSuccess())
    {
        error = ProcessUpgradeContextAsyncOperationBase::LoadRollbackMessageBody();
    }

    if (error.IsSuccess())
    {
        auto operation = this->BeginCommitUpgradeContext(
            move(storeTx),
            [this](AsyncOperationSPtr const & operation) { this->OnStartRollbackCommitComplete(operation, false); },
            thisSPtr);
        this->OnStartRollbackCommitComplete(operation, true);
    }
    else
    {
        WriteInfo(
            TraceComponent, 
            "{0} failed to start rollback due to {1}",
            this->TraceId,
            error);

        this->TrySchedulePrepareStartRollback(error, thisSPtr);
    }
}

void ProcessFabricUpgradeContextAsyncOperation::OnStartRollbackCommitComplete(
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
        CMEvents::Trace->RollbackStart(this->ReplicaActivityId, this->GetUpgradeContextStringTrace());

        CMEvents::Trace->ApplicationUpgradeRollback(
            SystemServiceApplicationNameHelper::SystemServiceApplicationName,
            this->UpgradeContext.RollforwardInstance,
            0, // dca_version
            this->ReplicaActivityId, 
            this->UpgradeContext.CurrentVersion.ToString(),
            L"", // currentManifestId before rollback
            L""); // targetManifestId after rollback

        CMEvents::Trace->ClusterUpgradeRollbackStartOperational(
            this->ReplicaActivityId.ActivityId.Guid,
            this->UpgradeContext.CurrentVersion.ToString(),  // rolling back to this version.
            this->UpgradeContext.CommonUpgradeContextData.FailureReason,
            this->UpgradeContext.OverallUpgradeElapsedTime.TotalMillisecondsAsDouble());

        this->ScheduleRequestToFM(thisSPtr);
    }
    else
    {
        this->TrySchedulePrepareStartRollback(error, thisSPtr);
    }
}

void ProcessFabricUpgradeContextAsyncOperation::TrySchedulePrepareStartRollback(ErrorCode const & error, AsyncOperationSPtr const & thisSPtr)
{
    if (this->ShouldScheduleRetry(error, thisSPtr))
    {
        this->StartTimer(
            thisSPtr, 
            [this](AsyncOperationSPtr const & thisSPtr){ this->PrepareStartRollback(thisSPtr); }, 
            this->Replica.GetRandomizedOperationRetryDelay(error));
    }
}

ErrorCode ProcessFabricUpgradeContextAsyncOperation::LoadMessageBody(
    FabricVersion const & version,
    TimeSpan const replicaSetCheckTimeout,
    uint64 instanceId,
    bool isMonitored,
    bool isManual,
    bool isRollback,
    __out UpgradeFabricRequestMessageBody & requestMessage)
{
    auto sequenceNumber = this->UpgradeContext.SequenceNumber;

    WriteInfo(
        TraceComponent, 
        "{0} loading message body: vers={1} replicaSetCheckTimeout={2} instance={3} isMonitored={4} seq={5}",
        this->TraceId,
        version,
        replicaSetCheckTimeout,
        instanceId,
        isMonitored,
        sequenceNumber);

    FabricUpgradeSpecification upgradeSpecification(
        version,
        instanceId,
        this->GetUpgradeType(),
        isMonitored,
        isManual);

    requestMessage = UpgradeFabricRequestMessageBody(
        move(upgradeSpecification),
        replicaSetCheckTimeout,
        vector<wstring>(),
        sequenceNumber,
        isRollback);

    return ErrorCodeValue::Success;
}

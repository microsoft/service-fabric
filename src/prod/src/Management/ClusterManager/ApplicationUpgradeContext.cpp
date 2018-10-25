// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Naming;
using namespace ServiceModel;
using namespace Store;
using namespace Management::ClusterManager;

StringLiteral const TraceComponent("ApplicationUpgradeContext");

RolloutContextType::Enum const ApplicationUpgradeContextType(RolloutContextType::ApplicationUpgrade);

ApplicationUpgradeContext::ApplicationUpgradeContext()
    : RolloutContext(ApplicationUpgradeContextType)
    , upgradeDescription_()
    , upgradeState_()
    , rollbackApplicationTypeVersion_()
    , targetApplicationManifestId_()
    , targetServicePackages_()
    , inProgressUpgradeDomain_()
    , completedUpgradeDomains_()
    , pendingUpgradeDomains_()
    , upgradeInstance_(0)
    , isInterrupted_(false)
    , isPreparingUpgrade_(false)
    , blockedServiceTypes_()
    , addedDefaultServices_()
    , deletedDefaultServices_()
    , defaultServiceUpdateDescriptions_()
    , rollbackDefaultServiceUpdateDescriptions_()
    , healthCheckElapsedTime_(TimeSpan::Zero)
    , overallUpgradeElapsedTime_(TimeSpan::Zero)
    , upgradeDomainElapsedTime_(TimeSpan::Zero)
    , lastHealthCheckResult_(false)
    , isFMRequestModified_(false)
    , isHealthPolicyModified_(false)
    , unhealthyEvaluations_()
    , currentUpgradeDomainProgress_()
    , commonUpgradeContextData_()
    , applicationDefinitionKind_(ApplicationDefinitionKind::Enum::ServiceFabricApplicationDescription)
{
}

ApplicationUpgradeContext::ApplicationUpgradeContext(
    Common::NamingUri const & appName)
    : RolloutContext(ApplicationUpgradeContextType)
    , upgradeDescription_(appName)
    , upgradeState_(ApplicationUpgradeState::RollingForward)
    , rollbackApplicationTypeVersion_()
    , targetApplicationManifestId_()
    , targetServicePackages_()
    , inProgressUpgradeDomain_()
    , completedUpgradeDomains_()
    , pendingUpgradeDomains_()
    , upgradeInstance_(0)
    , isInterrupted_(false)
    , isPreparingUpgrade_(false)
    , blockedServiceTypes_()
    , addedDefaultServices_()
    , deletedDefaultServices_()
    , defaultServiceUpdateDescriptions_()
    , rollbackDefaultServiceUpdateDescriptions_()
    , healthCheckElapsedTime_(TimeSpan::Zero)
    , overallUpgradeElapsedTime_(TimeSpan::Zero)
    , upgradeDomainElapsedTime_(TimeSpan::Zero)
    , lastHealthCheckResult_(false)
    , isFMRequestModified_(false)
    , isHealthPolicyModified_(false)
    , unhealthyEvaluations_()
    , currentUpgradeDomainProgress_()
    , commonUpgradeContextData_()
    , applicationDefinitionKind_(ApplicationDefinitionKind::Enum::ServiceFabricApplicationDescription)
{
}

ApplicationUpgradeContext::ApplicationUpgradeContext(
    ComponentRoot const & replica,
    ClientRequestSPtr const & request,
    ApplicationUpgradeDescription const & upgradeDescription,
    ServiceModelVersion const & rollbackApplicationTypeVersion,
    std::wstring const & targetApplicationManifestId,
    uint64 upgradeInstance)
    : RolloutContext(ApplicationUpgradeContextType, replica, request)
    , upgradeDescription_(upgradeDescription)
    , upgradeState_(ApplicationUpgradeState::RollingForward)
    , rollbackApplicationTypeVersion_(rollbackApplicationTypeVersion)
    , targetApplicationManifestId_(targetApplicationManifestId)
    , targetServicePackages_()
    , inProgressUpgradeDomain_()
    , completedUpgradeDomains_()
    , pendingUpgradeDomains_()
    , upgradeInstance_(upgradeInstance)
    , isInterrupted_(false)
    , isPreparingUpgrade_(true)
    , blockedServiceTypes_()
    , addedDefaultServices_()
    , deletedDefaultServices_()
    , defaultServiceUpdateDescriptions_()
    , rollbackDefaultServiceUpdateDescriptions_()
    , healthCheckElapsedTime_(TimeSpan::Zero)
    , overallUpgradeElapsedTime_(TimeSpan::Zero)
    , upgradeDomainElapsedTime_(TimeSpan::Zero)
    , lastHealthCheckResult_(false)
    , unhealthyEvaluations_()
    , currentUpgradeDomainProgress_()
    , commonUpgradeContextData_()
    , applicationDefinitionKind_(ApplicationDefinitionKind::Enum::ServiceFabricApplicationDescription)
{
    WriteNoise(
        GetTraceComponent(), 
        "{0} ctor({1})",
        this->TraceId,
        upgradeDescription);
}

ApplicationUpgradeContext::ApplicationUpgradeContext(
    ComponentRoot const & replica,
    ClientRequestSPtr const & request,
    ApplicationUpgradeDescription const & upgradeDescription,
    ServiceModelVersion const & rollbackApplicationTypeVersion,
    uint64 upgradeInstance,
    ApplicationDefinitionKind::Enum applicationDefinitionKind)
    : RolloutContext(ApplicationUpgradeContextType, replica, request)
    , upgradeDescription_(upgradeDescription)
    , upgradeState_(ApplicationUpgradeState::RollingForward)
    , rollbackApplicationTypeVersion_(rollbackApplicationTypeVersion)
    , targetServicePackages_()
    , inProgressUpgradeDomain_()
    , completedUpgradeDomains_()
    , pendingUpgradeDomains_()
    , upgradeInstance_(upgradeInstance)
    , isInterrupted_(false)
    , isPreparingUpgrade_(true)
    , blockedServiceTypes_()
    , addedDefaultServices_()
    , deletedDefaultServices_()
    , defaultServiceUpdateDescriptions_()
    , rollbackDefaultServiceUpdateDescriptions_()
    , healthCheckElapsedTime_(TimeSpan::Zero)
    , overallUpgradeElapsedTime_(TimeSpan::Zero)
    , upgradeDomainElapsedTime_(TimeSpan::Zero)
    , lastHealthCheckResult_(false)
    , unhealthyEvaluations_()
    , currentUpgradeDomainProgress_()
    , commonUpgradeContextData_()
    , applicationDefinitionKind_(applicationDefinitionKind)
{
    WriteNoise(
        GetTraceComponent(), 
        "{0} ctor({1}, applicationDefinitionKind {2})",
        this->TraceId,
        upgradeDescription,
        applicationDefinitionKind);
}

ApplicationUpgradeContext::ApplicationUpgradeContext(
    Store::ReplicaActivityId const &activityId,
    ApplicationUpgradeDescription const & upgradeDescription,
    ServiceModelVersion const & rollbackApplicationTypeVersion,
    wstring const & targetApplicationManifestId,
    uint64 upgradeInstance,
    ApplicationDefinitionKind::Enum applicationDefinitionKind)
    : RolloutContext(ApplicationUpgradeContextType, activityId)
    , upgradeDescription_(upgradeDescription)
    , upgradeState_(ApplicationUpgradeState::RollingForward)
    , rollbackApplicationTypeVersion_(rollbackApplicationTypeVersion)
    , targetApplicationManifestId_(targetApplicationManifestId)
    , targetServicePackages_()
    , inProgressUpgradeDomain_()
    , completedUpgradeDomains_()
    , pendingUpgradeDomains_()
    , upgradeInstance_(upgradeInstance)
    , isInterrupted_(false)
    , isPreparingUpgrade_(true)
    , blockedServiceTypes_()
    , addedDefaultServices_()
    , deletedDefaultServices_()
    , defaultServiceUpdateDescriptions_()
    , rollbackDefaultServiceUpdateDescriptions_()
    , healthCheckElapsedTime_(TimeSpan::Zero)
    , overallUpgradeElapsedTime_(TimeSpan::Zero)
    , upgradeDomainElapsedTime_(TimeSpan::Zero)
    , lastHealthCheckResult_(false)
    , unhealthyEvaluations_()
    , currentUpgradeDomainProgress_()
    , commonUpgradeContextData_()
    , applicationDefinitionKind_(applicationDefinitionKind)
{
    WriteNoise(
        GetTraceComponent(),
        "{0} ctor({1}, applicationDefinitionKind {2})",
        this->TraceId,
        upgradeDescription,
        applicationDefinitionKind);
}

ApplicationUpgradeContext::~ApplicationUpgradeContext()
{
    // Will be null for contexts instantiated for reading from disk.
    // Do not trace dtor for these contexts as it is noisy and not useful.
    //
    if (replicaSPtr_)
    {
        WriteNoise(
            GetTraceComponent(),
            "{0} ~dtor",
            this->TraceId);
    }
}

std::wstring const & ApplicationUpgradeContext::get_Type() const
{
    return Constants::StoreType_ApplicationUpgradeContext;
}

StringLiteral const & ApplicationUpgradeContext::GetTraceComponent() const
{
    return TraceComponent;
}

std::wstring ApplicationUpgradeContext::ConstructKey() const
{
    wstring temp;
    StringWriter writer(temp);
    writer.Write("{0}",
        upgradeDescription_.ApplicationName);
    return temp;
}

void ApplicationUpgradeContext::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write(
        "{0}({1})[{2}, {3}, {4}, ({5}) interrupted = {6} modified[fm={7} health={8}], preparingUpgrade = {9}, applicationDefinitionKind = {10}]",
        this->get_Type(),
        this->Status, 
        upgradeDescription_, 
        rollbackApplicationTypeVersion_,
        upgradeState_, 
        upgradeInstance_,
        isInterrupted_,
        isFMRequestModified_,
        isHealthPolicyModified_,
        isPreparingUpgrade_,
        applicationDefinitionKind_);

    w.Write("[{0}]", commonUpgradeContextData_);

    w.Write("[blocked = {0} in progress = {1} completed = {2} pending = {3}]",
        blockedServiceTypes_,
        inProgressUpgradeDomain_,
        completedUpgradeDomains_,
        pendingUpgradeDomains_);

    if (upgradeDescription_.IsHealthMonitored)
    {
        w.Write("elapsed[overall = {0} UD = {1} health = {2} lastCheck={3}]",
            overallUpgradeElapsedTime_,
            upgradeDomainElapsedTime_,
            healthCheckElapsedTime_,
            lastHealthCheckResult_);
    }
     
    for (auto const & reason : unhealthyEvaluations_)
    {
        w.Write("[health reason = {0}]", reason.Evaluation->Description);
    }
}

void ApplicationUpgradeContext::AddBlockedServiceType(ServiceModelTypeName const & serviceTypeName)
{
    if (!this->ContainsBlockedServiceType(serviceTypeName))
    {
        blockedServiceTypes_.push_back(serviceTypeName.Value);
    }
}

bool ApplicationUpgradeContext::ContainsBlockedServiceType(ServiceModelTypeName const & serviceTypeName)
{
    return (find(blockedServiceTypes_.begin(), blockedServiceTypes_.end(), serviceTypeName.Value) != blockedServiceTypes_.end());
}

void ApplicationUpgradeContext::ClearBlockedServiceTypes()
{
    blockedServiceTypes_.clear();
}

void ApplicationUpgradeContext::AddTargetServicePackage(
    ServiceModelTypeName const & typeName, 
    ServiceModelPackageName const & packageName, 
    ServiceModelVersion const & packageVersion)
{
    targetServicePackages_.insert(make_pair(
        typeName, 
        TargetServicePackage(packageName, packageVersion)));
}

bool ApplicationUpgradeContext::TryGetTargetServicePackage(
    ServiceModelTypeName const & typeName, 
    __out ServiceModelPackageName & packageName,
    __out ServiceModelVersion & packageVersion)
{
    auto findIt = targetServicePackages_.find(typeName);

    if (findIt == targetServicePackages_.end())
    {
        return false;
    }
    else
    {
        packageName = findIt->second.PackageName;
        packageVersion = findIt->second.PackageVersion;
        
        return true;
    }
}

void ApplicationUpgradeContext::ClearTargetServicePackages()
{
    targetServicePackages_.clear();
}

void ApplicationUpgradeContext::UpdateInProgressUpgradeDomain(std::wstring && upgradeDomain)
{ 
    inProgressUpgradeDomain_.swap(upgradeDomain);
}

void ApplicationUpgradeContext::UpdateCompletedUpgradeDomains(std::vector<std::wstring> && upgradeDomains) 
{ 
    completedUpgradeDomains_.swap(upgradeDomains); 
}

void ApplicationUpgradeContext::UpdatePendingUpgradeDomains(std::vector<std::wstring> && upgradeDomains) 
{ 
    pendingUpgradeDomains_.swap(upgradeDomains); 
}

std::vector<std::wstring> ApplicationUpgradeContext::TakeCompletedUpgradeDomains()
{
    return move(completedUpgradeDomains_);
}

void ApplicationUpgradeContext::UpdateUpgradeProgress(Reliability::UpgradeDomainProgress && currentUpgradeDomainProgress)
{
    currentUpgradeDomainProgress_ = move(currentUpgradeDomainProgress);
}

ErrorCode ApplicationUpgradeContext::CompleteRollforward(StoreTransaction const & storeTx)
{
    upgradeState_ = ApplicationUpgradeState::CompletedRollforward;
    return this->CompleteUpgrade(storeTx);
}

ErrorCode ApplicationUpgradeContext::CompleteRollback(StoreTransaction const & storeTx)
{
    upgradeState_ = this->IsInterrupted ? ApplicationUpgradeState::Interrupted : ApplicationUpgradeState::CompletedRollback;
    return this->CompleteUpgrade(storeTx);
}

ErrorCode ApplicationUpgradeContext::CompleteUpgrade(StoreTransaction const & storeTx)
{
    this->ClearBlockedServiceTypes();

    // Trim non-completed UDs from the list if the upgrade completes at the FM 
    // (returning ApplicationNotFound) before the CM can get an updated list 
    // because the FM does not keep application information around after
    // the upgrade has completed. This is not necessary for cluster
    // upgrades since there is no equivalent "cluster not found"
    // error from the FM.
    //
    this->UpdatePendingUpgradeDomains(vector<wstring>());
    this->UpdateInProgressUpgradeDomain(wstring());

    this->UpdateUpgradeProgress(Reliability::UpgradeDomainProgress());

    return this->Complete(storeTx);
}

uint64 ApplicationUpgradeContext::get_RollforwardInstance() const 
{ 
    return upgradeInstance_; 
}

uint64 ApplicationUpgradeContext::get_RollbackInstance() const 
{ 
    return (upgradeInstance_ + 1); 
}

uint64 ApplicationUpgradeContext::get_UpgradeCompletionInstance() const
{
    // Depends on ProcessApplicationUpgradeContextAsyncOperation updating the 
    // upgrade state first before accessing this field to complete the corresponding
    // ApplicationContext.
    //
    switch (upgradeState_)
    {
        case ApplicationUpgradeState::RollingForward:
        case ApplicationUpgradeState::CompletedRollforward:
        case ApplicationUpgradeState::Failed:
            return this->RollforwardInstance;

        case ApplicationUpgradeState::RollingBack:
        case ApplicationUpgradeState::CompletedRollback:
        case ApplicationUpgradeState::Interrupted:
            return this->RollbackInstance;

        default:
            TRACE_ERROR_AND_TESTASSERT(
                GetTraceComponent(), 
                "get_UpgradeCompletionInstance: unexpected upgrade state={0}", 
                upgradeState_);

            // Fallback to legacy instance generation algorithm
            //
            return this->RollforwardInstance + 2;
    }
}

uint64 ApplicationUpgradeContext::GetNextUpgradeInstance(uint64 applicationPackageInstance)
{
    // The sequence of versions used by the old (V1) CM is as follows. 
    // Where S = stable (no upgrades/rollbacks), U = upgrading, R = rolling back.
    // 
    // S1(i), U1(i+1), R1(i+2), S2(i+3), U2(i+4), R2(i+5), S3(i+6)
    // 
    // The CM only persists stable instances (i, i+3, i+6). The upgrade and 
    // rollback instances are only calculated in memory depending on the state 
    // of the application as needed. This was because the CM originally only 
    // sent create service requests with stable instances, which were persisted 
    // for create service requests to read from persisted state. Package version 
    // data for upgrade instances (i+1) was not persisted until the upgrade completed, 
    // so a create service request could not have read the needed version information to 
    // send a request with the upgrade instance. Not persisting the target package 
    // version data until the upgrade completes makes the rollback path simpler 
    // and more robust (no persisted package version data to rollback).
    // 
    // The new proposed sequence of versions used by the CM will be as follows.
    // 
    // S1(i), U1(i+1), R1(i+2), S2(i+1 | i+2), U2(i+2 | i+3), R2(i+3 | i+4), S3(i+3 | I+4)
    // 
    // The subsequent upgrade and rollback versions cannot be generated in such a way to 
    // be match the old CM sequence stable versions since an upgrade may complete on
    // a new CM, while the next upgrade starts on an old CM. i.e. We cannot guarantee
    // the following sequence:
    //
    // S1(i), U1(i+1), R1(i+2), S2(i+1 | i+2), U2(i+4), R2(i+5), S3(i+4 | I+5)
    // 
    // An alternate sequence could be:
    // 
    // S1(i), U1(i+1), S2(i+1), U2(i+2), R2(i+3), S3(i+3)
    // 
    // With the instance persisted at every increment. However, this will cause backwards 
    // compatibility issues because the old CM protocol would calculate in-memory values 
    // from the persisted values, corrupting the sequence if an upgrade or rollback 
    // starts on a new CM, but continues on an old CM.
    //
    return (applicationPackageInstance + 1);
}

uint64 ApplicationUpgradeContext::GetDeleteApplicationInstance(uint64 applicationPackageInstance)
{
    // PackageInstance(i) = N
    // RollforwardInstance = N + 1
    // RollbackInstance = N + 2
    // UpgradeCompletionInstance = N + 1 | N + 2
    //
    // PackageInstance(i + 1) = UpgradeCompletionInstance = N + 1 | N + 2
    //
    // GetDeleteApplicationInstance returns (arg + 2), which equals
    // RollbackInstance in this case to ensure that FM accepts the
    // request during upgrade/rollback since PackageInstance is only
    // persisted to the ApplicationContext when the upgrade/rollback completes.
    //
    return (applicationPackageInstance + 2);
}

bool ApplicationUpgradeContext::TryInterrupt()
{
    if (!isInterrupted_)
    {
        isInterrupted_ = true;
        return true;
    }
    else
    {
        return false;
    }
}

bool ApplicationUpgradeContext::TryModifyUpgrade(
    ApplicationUpgradeUpdateDescription const & update,
    __out wstring & validationErrorMessage)
{
    // Converting from public to internal API already performs these
    // checks, but serializing from JSON does not.
    //

    if (update.ApplicationName != upgradeDescription_.ApplicationName)
    {
        validationErrorMessage = wformatString(GET_RC( Mismatched_Application ),
            update.ApplicationName,
            upgradeDescription_.ApplicationName);

        return false;
    }

    if (update.UpgradeType != UpgradeType::Rolling)
    {
        validationErrorMessage = wformatString("{0} {1}", GET_RC( Invalid_Upgrade_Kind ),
            update.UpgradeType);

        return false;
    }

    if (!update.UpdateDescription && !update.HealthPolicy)
    {
        validationErrorMessage = GET_RC( No_Updates );

        return false;
    }

    switch (upgradeState_)
    {
    case ApplicationUpgradeState::RollingForward:
        // no special checks, allow modification
        break;

    case ApplicationUpgradeState::RollingBack:
    case ApplicationUpgradeState::Interrupted:
        if (update.HealthPolicy ||
            (update.UpdateDescription && !update.UpdateDescription->IsValidForRollback()))
        {
            validationErrorMessage = wformatString("{0} {1}", GET_RC( Allowed_Rollback_Updates ),
                upgradeState_);

            return false;
        }

        break;

    case ApplicationUpgradeState::CompletedRollforward:
    case ApplicationUpgradeState::CompletedRollback:
    default:
        validationErrorMessage = wformatString("{0} {1}", GET_RC( Invalid_Upgrade_State ),
            upgradeState_);

        return false;
    }

    if (!upgradeDescription_.TryModify(update, validationErrorMessage))
    {
        return false;
    }

    isFMRequestModified_ = (update.UpdateDescription != nullptr);
    isHealthPolicyModified_ = (update.HealthPolicy != nullptr);

    return true;
}

void ApplicationUpgradeContext::SetHealthCheckElapsedTimeToWaitDuration(TimeSpan const elapsed)
{
    auto resetElapsed = upgradeDescription_.MonitoringPolicy.HealthCheckWaitDuration.SubtractWithMaxAndMinValueCheck(elapsed);
    if (resetElapsed < TimeSpan::Zero)
    {
        resetElapsed = TimeSpan::Zero;
    }

    if (healthCheckElapsedTime_ > resetElapsed)
    {
        healthCheckElapsedTime_ = resetElapsed;
    }
}

void ApplicationUpgradeContext::SetLastHealthCheckResult(bool isHealthy)
{
    lastHealthCheckResult_ = isHealthy;
}

void ApplicationUpgradeContext::UpdateHealthCheckElapsedTime(TimeSpan const elapsed)
{
    healthCheckElapsedTime_ = healthCheckElapsedTime_.AddWithMaxAndMinValueCheck(elapsed);
}

void ApplicationUpgradeContext::UpdateHealthMonitoringTimeouts(TimeSpan const elapsed)
{
    overallUpgradeElapsedTime_ = overallUpgradeElapsedTime_.AddWithMaxAndMinValueCheck(elapsed);
    upgradeDomainElapsedTime_ = upgradeDomainElapsedTime_.AddWithMaxAndMinValueCheck(elapsed);
}

void ApplicationUpgradeContext::ResetUpgradeDomainTimeouts()
{
    healthCheckElapsedTime_ = TimeSpan::Zero;
    upgradeDomainElapsedTime_ = TimeSpan::Zero;
    lastHealthCheckResult_ = false;
}

void ApplicationUpgradeContext::SetRollingBack(bool goalStateExists)
{
    upgradeState_ = ApplicationUpgradeState::RollingBack;

    if (goalStateExists || upgradeDescription_.RollingUpgradeMode == ServiceModel::RollingUpgradeMode::Monitored)
    {
        upgradeDescription_.SetUpgradeMode(ServiceModel::RollingUpgradeMode::UnmonitoredAuto);
    }
}

void ApplicationUpgradeContext::RevertToManualUpgrade()
{
    upgradeDescription_.SetUpgradeMode(ServiceModel::RollingUpgradeMode::UnmonitoredManual);
}

void ApplicationUpgradeContext::UpdateHealthEvaluationReasons(std::vector<ServiceModel::HealthEvaluation> && unhealthyEvaluations)
{
    unhealthyEvaluations_ = move(unhealthyEvaluations);
}

void ApplicationUpgradeContext::ClearHealthEvaluationReasons()
{
    unhealthyEvaluations_.clear();
}

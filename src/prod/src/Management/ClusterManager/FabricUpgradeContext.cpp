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

StringLiteral const TraceComponent("FabricUpgradeContext");

RolloutContextType::Enum const FabricUpgradeContextType(RolloutContextType::FabricUpgrade);

FabricUpgradeContext::FabricUpgradeContext()
    : RolloutContext(FabricUpgradeContextType)
    , upgradeDescription_()
    , upgradeState_()
    , currentVersion_()
    , inProgressUpgradeDomain_()
    , completedUpgradeDomains_()
    , pendingUpgradeDomains_()
    , upgradeInstance_(0)
    , isInterrupted_(false)
    , isPreparingUpgrade_(false)
    , healthCheckElapsedTime_(TimeSpan::Zero)
    , overallUpgradeElapsedTime_(TimeSpan::Zero)
    , upgradeDomainElapsedTime_(TimeSpan::Zero)
    , manifestHealthPolicy_()
    , manifestUpgradeHealthPolicy_()
    , lastHealthCheckResult_(false)
    , isFMRequestModified_(false)
    , isHealthPolicyModified_(false)
    , unhealthyEvaluations_()
    , currentUpgradeDomainProgress_()
    , commonUpgradeContextData_()
    , isAutoBaseline_ (false)
{
}

FabricUpgradeContext::FabricUpgradeContext(
    FabricUpgradeContext && other)
    : RolloutContext(move(other))
    , upgradeDescription_(move(other.upgradeDescription_))
    , upgradeState_(move(other.upgradeState_))
    , currentVersion_(move(other.currentVersion_))
    , inProgressUpgradeDomain_(move(other.inProgressUpgradeDomain_))
    , completedUpgradeDomains_(move(other.completedUpgradeDomains_))
    , pendingUpgradeDomains_(move(other.pendingUpgradeDomains_))
    , upgradeInstance_(move(other.upgradeInstance_))
    , isInterrupted_(move(other.isInterrupted_))
    , isPreparingUpgrade_(move(other.isPreparingUpgrade_))
    , healthCheckElapsedTime_(move(other.healthCheckElapsedTime_))
    , overallUpgradeElapsedTime_(move(other.overallUpgradeElapsedTime_))
    , upgradeDomainElapsedTime_(move(other.upgradeDomainElapsedTime_))
    , manifestHealthPolicy_(move(other.manifestHealthPolicy_))
    , manifestUpgradeHealthPolicy_(move(other.manifestUpgradeHealthPolicy_))
    , lastHealthCheckResult_(move(other.lastHealthCheckResult_))
    , isFMRequestModified_(move(other.isFMRequestModified_))
    , isHealthPolicyModified_(move(other.isHealthPolicyModified_))
    , unhealthyEvaluations_(move(other.unhealthyEvaluations_))
    , currentUpgradeDomainProgress_(move(other.currentUpgradeDomainProgress_))
    , commonUpgradeContextData_(move(other.commonUpgradeContextData_))
    , isAutoBaseline_(false)
{
}

FabricUpgradeContext & FabricUpgradeContext::operator=(
    FabricUpgradeContext && other)
{
    if (this != &other)
    {
        __super::operator = (move(other));
        upgradeDescription_ = move(other.upgradeDescription_);
        upgradeState_ = move(other.upgradeState_);
        currentVersion_ = move(other.currentVersion_);
        inProgressUpgradeDomain_ = move(other.inProgressUpgradeDomain_);
        completedUpgradeDomains_ = move(other.completedUpgradeDomains_);
        pendingUpgradeDomains_ = move(other.pendingUpgradeDomains_);
        upgradeInstance_ = move(other.upgradeInstance_);
        isInterrupted_ = move(other.isInterrupted_);
        isPreparingUpgrade_ = move(other.isPreparingUpgrade_);
        healthCheckElapsedTime_ = move(other.healthCheckElapsedTime_);
        overallUpgradeElapsedTime_ = move(other.overallUpgradeElapsedTime_);
        upgradeDomainElapsedTime_ = move(other.upgradeDomainElapsedTime_);
        manifestHealthPolicy_ = move(other.manifestHealthPolicy_);
        manifestUpgradeHealthPolicy_ = move(other.manifestUpgradeHealthPolicy_);
        lastHealthCheckResult_ = move(other.lastHealthCheckResult_);
        isFMRequestModified_ = move(other.isFMRequestModified_);
        isHealthPolicyModified_ = move(other.isHealthPolicyModified_);
        unhealthyEvaluations_ = move(other.unhealthyEvaluations_);
        currentUpgradeDomainProgress_ = move(other.currentUpgradeDomainProgress_);
        commonUpgradeContextData_ = move(other.commonUpgradeContextData_);
        isAutoBaseline_ = move(other.isAutoBaseline_);
    }

    return *this;
}

FabricUpgradeContext::FabricUpgradeContext(
    ComponentRoot const & replica,
    ClientRequestSPtr const & request,
    FabricUpgradeDescription const & upgradeDescription,
    Common::FabricVersion const & currentVersion,
    uint64 upgradeInstance)
    : RolloutContext(FabricUpgradeContextType, replica, request)
    , upgradeDescription_(upgradeDescription)
    , upgradeState_(FabricUpgradeState::RollingForward)
    , currentVersion_(currentVersion)
    , inProgressUpgradeDomain_()
    , completedUpgradeDomains_()
    , pendingUpgradeDomains_()
    , upgradeInstance_(upgradeInstance)
    , isInterrupted_(false)
    , isPreparingUpgrade_(false)
    , healthCheckElapsedTime_(TimeSpan::Zero)
    , overallUpgradeElapsedTime_(TimeSpan::Zero)
    , upgradeDomainElapsedTime_(TimeSpan::Zero)
    , manifestHealthPolicy_()
    , manifestUpgradeHealthPolicy_()
    , lastHealthCheckResult_(false)
    , isFMRequestModified_(false)
    , isHealthPolicyModified_(false)
    , unhealthyEvaluations_()
    , currentUpgradeDomainProgress_()
    , commonUpgradeContextData_()
    , isAutoBaseline_(false)
{
    WriteNoise(
        TraceComponent, 
        "{0} ctor({1})",
        this->TraceId,
        upgradeDescription);
}

FabricUpgradeContext::FabricUpgradeContext(
    ComponentRoot const &,
    Store::ReplicaActivityId const & activityId,
    FabricUpgradeDescription const & upgradeDescription,
    Common::FabricVersion const & currentVersion,
    uint64 upgradeInstance)
    : RolloutContext(FabricUpgradeContextType, activityId)
    , upgradeDescription_(upgradeDescription)
    , upgradeState_(FabricUpgradeState::RollingForward)
    , currentVersion_(currentVersion)
    , inProgressUpgradeDomain_()
    , completedUpgradeDomains_()
    , pendingUpgradeDomains_()
    , upgradeInstance_(upgradeInstance)
    , isInterrupted_(false)
    , isPreparingUpgrade_(false)
    , healthCheckElapsedTime_(TimeSpan::Zero)
    , overallUpgradeElapsedTime_(TimeSpan::Zero)
    , upgradeDomainElapsedTime_(TimeSpan::Zero)
    , manifestHealthPolicy_()
    , manifestUpgradeHealthPolicy_()
    , lastHealthCheckResult_(false)
    , isFMRequestModified_(false)
    , isHealthPolicyModified_(false)
    , unhealthyEvaluations_()
    , currentUpgradeDomainProgress_()
    , commonUpgradeContextData_()
    , isAutoBaseline_(false)
{
    WriteNoise(
        TraceComponent, 
        "{0} ctor({1})",
        this->TraceId,
        upgradeDescription);
}

FabricUpgradeContext::~FabricUpgradeContext()
{
    // Will be null for contexts instantiated for reading from disk.
    // Do not trace dtor for these contexts as it is noisy and not useful.
    //
    if (replicaSPtr_)
    {
        WriteNoise(
            TraceComponent,
            "{0} ~dtor",
            this->TraceId);
    }
}

std::wstring const & FabricUpgradeContext::get_Type() const
{
    return Constants::StoreType_FabricUpgradeContext;
}

std::wstring FabricUpgradeContext::ConstructKey() const
{
    return Constants::StoreKey_FabricUpgradeContext;
}

void FabricUpgradeContext::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write(
        "FabricUpgradeContext({0})[{1}, {2}, {3}, ({4}) interrupted = {5} modified[fm={6} health={7}] preparingUpgrade = {8} isautobaseline={9}]",
        this->Status, 
        upgradeDescription_, 
        currentVersion_,
        upgradeState_, 
        upgradeInstance_,
        isInterrupted_,
        isFMRequestModified_,
        isHealthPolicyModified_,
        isPreparingUpgrade_,
        isAutoBaseline_);

    w.Write("[{0}]", commonUpgradeContextData_);

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

void FabricUpgradeContext::UpdateInProgressUpgradeDomain(std::wstring && upgradeDomain)
{ 
    inProgressUpgradeDomain_.swap(upgradeDomain);
}

void FabricUpgradeContext::UpdateCompletedUpgradeDomains(std::vector<std::wstring> && upgradeDomains) 
{ 
    completedUpgradeDomains_.swap(upgradeDomains); 
}

void FabricUpgradeContext::UpdatePendingUpgradeDomains(std::vector<std::wstring> && upgradeDomains) 
{ 
    pendingUpgradeDomains_.swap(upgradeDomains); 
}

void FabricUpgradeContext::UpdateUpgradeProgress(Reliability::UpgradeDomainProgress && currentUpgradeDomainProgress)
{
    currentUpgradeDomainProgress_ = move(currentUpgradeDomainProgress);
}

ErrorCode FabricUpgradeContext::StartUpgrading(
    StoreTransaction const & storeTx, 
    Common::ComponentRoot const & replica,
    ClientRequestSPtr const & request,
    FabricUpgradeDescription const & upgradeDescription)
{
    upgradeDescription_ = upgradeDescription;
    upgradeState_ = FabricUpgradeState::RollingForward;

    inProgressUpgradeDomain_.clear();
    completedUpgradeDomains_.clear();
    pendingUpgradeDomains_.clear();
    unhealthyEvaluations_.clear();

    currentUpgradeDomainProgress_ = Reliability::UpgradeDomainProgress();
    commonUpgradeContextData_.Reset();

    upgradeInstance_ = this->UpgradeCompletionInstance + 1;
    isInterrupted_ = false;
    isPreparingUpgrade_ = true;
    isAutoBaseline_ = false;

    healthCheckElapsedTime_ = TimeSpan::Zero;
    overallUpgradeElapsedTime_ = TimeSpan::Zero;
    upgradeDomainElapsedTime_ = TimeSpan::Zero;

    // At this time we are starting a new upgrade, which means that the cluster manifest is
    // consistent throughout the cluster so save the health policy now. We cannot read the value 
    // dynamically during upgrade since the cluster manifest itself may be upgrading.
    //
    manifestHealthPolicy_ = Management::ManagementConfig::GetConfig().GetClusterHealthPolicy();
    manifestUpgradeHealthPolicy_ = Management::ManagementConfig::GetConfig().GetClusterUpgradeHealthPolicy();

    this->ReInitializeContext(replica, request);

    return this->UpdateStatus(storeTx, RolloutStatus::Pending);
}

ErrorCode FabricUpgradeContext::CompleteRollforward(StoreTransaction const & storeTx)
{
    currentVersion_ = upgradeDescription_.Version;
    upgradeState_ = FabricUpgradeState::CompletedRollforward;

    this->UpdateUpgradeProgress(Reliability::UpgradeDomainProgress());

    return this->Complete(storeTx);
}

ErrorCode FabricUpgradeContext::CompleteRollback(StoreTransaction const & storeTx)
{
    upgradeState_ = this->IsInterrupted ? FabricUpgradeState::Interrupted : FabricUpgradeState::CompletedRollback;

    this->UpdateUpgradeProgress(Reliability::UpgradeDomainProgress());

    return this->Complete(storeTx);
}

void FabricUpgradeContext::RefreshUpgradeInstance(uint64 instance)
{
    if (upgradeInstance_ < instance)
    {
        upgradeInstance_ = instance;
    }
}

bool FabricUpgradeContext::TryInterrupt()
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

bool FabricUpgradeContext::TryUpdateHealthPolicies(FabricUpgradeDescription const & other)
{
    // We allow switching from any upgrade mode to monitored as long as all
    // other aspects of the upgrade (other than health policies) remain the same.
    //
    if (upgradeDescription_.EqualsIgnoreHealthPolicies(other) && other.RollingUpgradeMode == RollingUpgradeMode::Monitored)
    {
        FabricUpgradeDescription description(
            upgradeDescription_.Version,
            upgradeDescription_.UpgradeType,
            ServiceModel::RollingUpgradeMode::Monitored,
            upgradeDescription_.ReplicaSetCheckTimeout,
            other.MonitoringPolicy,
            other.HealthPolicy,
            other.IsHealthPolicyValid,
            other.EnableDeltaHealthEvaluation,
            other.UpgradeHealthPolicy,
            other.IsUpgradeHealthPolicyValid,
            other.ApplicationHealthPolicies);

        // When switching from unmonitored manual to monitored, the next health check occurs
        // immediately upon UD completion since the caller is expected to complete the
        // current unmonitored manual health check work before changing the upgrade mode.
        //
        healthCheckElapsedTime_ = description.MonitoringPolicy.HealthCheckWaitDuration;
        lastHealthCheckResult_ = false;

        // Remaining timeouts are reset since this could be the first time we are going from
        // unmonitored manual to monitored, in which case there is no previous timeout state.
        // This implies that when continuing a failed monitored upgrade, the new overall
        // upgrade timeout being set should account for any elapsed time.
        //
        overallUpgradeElapsedTime_ = TimeSpan::Zero;
        upgradeDomainElapsedTime_ = TimeSpan::Zero;

        upgradeDescription_ = move(description);

        return true;
    }

    return false;
}

bool FabricUpgradeContext::TryModifyUpgrade(
    FabricUpgradeUpdateDescription const & update,
    __out wstring & validationErrorMessage)
{
    // Converting from public to internal API already performs these
    // checks, but serializing from JSON does not.
    //

    if (update.UpgradeType != UpgradeType::Rolling)
    {
        validationErrorMessage = wformatString("{0} {1}", GET_RC( Invalid_Upgrade_Kind ),
            update.UpgradeType);

        return false;
    }

    if (!update.HasUpdates())
    {
        validationErrorMessage = GET_RC( No_Updates );

        return false;
    }

    switch (upgradeState_)
    {
    case FabricUpgradeState::RollingForward:
        // no special checks, allow modification
        break;

    case FabricUpgradeState::RollingBack:
    case FabricUpgradeState::Interrupted:
        // Only ReplicaSetCheckTimeout, ForceRestart, and RollingUpgradeMode UnmonitoredManual/UnmonitoredAuto are allowed during rollback
        //
        if (update.HealthPolicy || 
            update.EnableDeltaHealthEvaluation ||
            update.UpgradeHealthPolicy ||
            update.ApplicationHealthPolicies ||
            (update.UpdateDescription && !update.UpdateDescription->IsValidForRollback()))
        {
            validationErrorMessage = wformatString("{0} {1}", GET_RC( Allowed_Rollback_Updates ),
                upgradeState_);

            return false;
        }

        break;

    case FabricUpgradeState::CompletedRollforward:
    case FabricUpgradeState::CompletedRollback:
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
    isHealthPolicyModified_ = (update.HealthPolicy != nullptr || update.UpgradeHealthPolicy != nullptr || update.ApplicationHealthPolicies != nullptr);

    return true;
}

void FabricUpgradeContext::SetHealthCheckElapsedTimeToWaitDuration(TimeSpan const elapsed)
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

void FabricUpgradeContext::SetLastHealthCheckResult(bool isHealthy)
{
    lastHealthCheckResult_ = isHealthy;
}

void FabricUpgradeContext::UpdateHealthCheckElapsedTime(TimeSpan const elapsed)
{
    healthCheckElapsedTime_ = healthCheckElapsedTime_.AddWithMaxAndMinValueCheck(elapsed);
}

void FabricUpgradeContext::UpdateHealthMonitoringTimeouts(TimeSpan const elapsed)
{
    overallUpgradeElapsedTime_ = overallUpgradeElapsedTime_.AddWithMaxAndMinValueCheck(elapsed);
    upgradeDomainElapsedTime_ = upgradeDomainElapsedTime_.AddWithMaxAndMinValueCheck(elapsed);
}

void FabricUpgradeContext::ResetUpgradeDomainTimeouts()
{
    healthCheckElapsedTime_ = TimeSpan::Zero;
    upgradeDomainElapsedTime_ = TimeSpan::Zero;
    lastHealthCheckResult_ = false;
}

void FabricUpgradeContext::SetRollingBack(bool goalStateExists)
{
    upgradeState_ = FabricUpgradeState::RollingBack;

    if (goalStateExists || upgradeDescription_.RollingUpgradeMode == ServiceModel::RollingUpgradeMode::Monitored)
    {
        upgradeDescription_.SetUpgradeMode(ServiceModel::RollingUpgradeMode::UnmonitoredAuto);
    }
}

void FabricUpgradeContext::RevertToManualUpgrade()
{
    upgradeDescription_.SetUpgradeMode(ServiceModel::RollingUpgradeMode::UnmonitoredManual);
}

void FabricUpgradeContext::MarkAsBaseline()
{
    isAutoBaseline_ = true;
}

void FabricUpgradeContext::UpdateHealthEvaluationReasons(std::vector<ServiceModel::HealthEvaluation> && unhealthyEvaluations)
{
    unhealthyEvaluations_ = move(unhealthyEvaluations);
}

void FabricUpgradeContext::ClearHealthEvaluationReasons()
{
    unhealthyEvaluations_.clear();
}

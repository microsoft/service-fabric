// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;

using namespace Management::ClusterManager;

FabricUpgradeStatusDescription::FabricUpgradeStatusDescription()
    : targetVersion_()
    , upgradeState_(FabricUpgradeState::Invalid)
    , inProgressUpgradeDomain_()
    , nextUpgradeDomain_()
    , completedUpgradeDomains_()
    , pendingUpgradeDomains_()
    , upgradeInstance_()
    , rollingUpgradeMode_(RollingUpgradeMode::Invalid)
    , upgradeDescription_()
    , upgradeDuration_()
    , currentUpgradeDomainDuration_()
    , unhealthyEvaluations_()
    , currentUpgradeDomainProgress_()
    , commonUpgradeContextData_()
{ 
}

FabricUpgradeStatusDescription::FabricUpgradeStatusDescription(FabricUpgradeStatusDescription && other) 
    : targetVersion_(move(other.targetVersion_))
    , upgradeState_(move(other.upgradeState_))
    , inProgressUpgradeDomain_(move(other.inProgressUpgradeDomain_))
    , nextUpgradeDomain_(move(other.nextUpgradeDomain_))
    , completedUpgradeDomains_(move(other.completedUpgradeDomains_))
    , pendingUpgradeDomains_(move(other.pendingUpgradeDomains_))
    , upgradeInstance_(move(other.upgradeInstance_))
    , rollingUpgradeMode_(move(other.rollingUpgradeMode_))
    , upgradeDescription_(move(other.upgradeDescription_))
    , upgradeDuration_(move(other.upgradeDuration_))
    , currentUpgradeDomainDuration_(move(other.currentUpgradeDomainDuration_))
    , unhealthyEvaluations_(move(other.unhealthyEvaluations_))
    , currentUpgradeDomainProgress_(move(other.currentUpgradeDomainProgress_))
    , commonUpgradeContextData_(move(other.commonUpgradeContextData_))
{ 
}

FabricUpgradeStatusDescription & FabricUpgradeStatusDescription::operator=(FabricUpgradeStatusDescription && other) 
{
    if (this != &other)
    {
        targetVersion_ = move(other.targetVersion_);
        upgradeState_ = move(other.upgradeState_);
        inProgressUpgradeDomain_ = move(other.inProgressUpgradeDomain_);
        nextUpgradeDomain_ = move(other.nextUpgradeDomain_);
        completedUpgradeDomains_ = move(other.completedUpgradeDomains_);
        pendingUpgradeDomains_ = move(other.pendingUpgradeDomains_);
        upgradeInstance_ = move(other.upgradeInstance_);
        rollingUpgradeMode_ = move(other.rollingUpgradeMode_);
        upgradeDescription_ = move(other.upgradeDescription_);
        upgradeDuration_ = move(other.upgradeDuration_);
        currentUpgradeDomainDuration_ = move(other.currentUpgradeDomainDuration_);
        unhealthyEvaluations_ = move(other.unhealthyEvaluations_);
        currentUpgradeDomainProgress_ = move(other.currentUpgradeDomainProgress_);
        commonUpgradeContextData_ = move(other.commonUpgradeContextData_);
    }

    return *this;
}

FabricUpgradeStatusDescription::FabricUpgradeStatusDescription(
    Common::FabricVersion const & targetVersion,
    FabricUpgradeState::Enum upgradeState,
    std::wstring && inProgressUpgradeDomain,
    std::vector<std::wstring> && completedUpgradeDomains,
    std::vector<std::wstring> && pendingUpgradeDomains,
    uint64 upgradeInstance, 
    ServiceModel::RollingUpgradeMode::Enum rollingUpgradeMode,
    std::shared_ptr<FabricUpgradeDescription> && upgradeDescription,
    Common::TimeSpan const upgradeDuration,
    Common::TimeSpan const currentUpgradeDomainDuration,
    std::vector<ServiceModel::HealthEvaluation> && unhealthyEvaluations, 
    Reliability::UpgradeDomainProgress && currentUpgradeDomainProgress,
    Management::ClusterManager::CommonUpgradeContextData && commonUpgradeContextData)
    : targetVersion_(targetVersion)
    , upgradeState_(upgradeState)
    , inProgressUpgradeDomain_(inProgressUpgradeDomain)
    , nextUpgradeDomain_(pendingUpgradeDomains.empty() ? L"" : pendingUpgradeDomains.front())
    , completedUpgradeDomains_(move(completedUpgradeDomains))
    , pendingUpgradeDomains_(move(pendingUpgradeDomains))
    , upgradeInstance_(upgradeInstance)
    , rollingUpgradeMode_(rollingUpgradeMode)
    , upgradeDescription_(move(upgradeDescription))
    , upgradeDuration_(upgradeDuration)
    , currentUpgradeDomainDuration_(currentUpgradeDomainDuration)
    , unhealthyEvaluations_(move(unhealthyEvaluations))
    , currentUpgradeDomainProgress_(move(currentUpgradeDomainProgress))
    , commonUpgradeContextData_(move(commonUpgradeContextData))
{ 
}

FabricCodeVersion const& FabricUpgradeStatusDescription::GetTargetCodeVersion() const
{
    return targetVersion_.CodeVersion;
}

FabricConfigVersion const& FabricUpgradeStatusDescription::GetTargetConfigVersion() const
{
    return targetVersion_.ConfigVersion;
}

FabricUpgradeState::Enum FabricUpgradeStatusDescription::GetUpgradeState() const
{
    return upgradeState_;
}

void FabricUpgradeStatusDescription::GetUpgradeDomains(
    __out wstring &inProgressDomain,
    __out vector<wstring> &pendingDomains, 
    __out vector<wstring> &completedDomains) const
{
    inProgressDomain = inProgressUpgradeDomain_;
    pendingDomains = pendingUpgradeDomains_;
    completedDomains = completedUpgradeDomains_;
}

void FabricUpgradeStatusDescription::GetChangedUpgradeDomains(
    wstring const & prevInProgress,
    vector<wstring> const & prevCompleted,
    __out wstring &changedInProgressDomain,
    __out vector<wstring> &changedCompletedDomains) const
{
    UpgradeHelper::GetChangedUpgradeDomains(
        inProgressUpgradeDomain_,
        completedUpgradeDomains_,
        prevInProgress,
        prevCompleted,
        changedInProgressDomain,
        changedCompletedDomains);
}

RollingUpgradeMode::Enum FabricUpgradeStatusDescription::GetRollingUpgradeMode() const
{
    return rollingUpgradeMode_;
}

wstring const& FabricUpgradeStatusDescription::GetNextUpgradeDomain() const
{
    return nextUpgradeDomain_;
}

uint64 FabricUpgradeStatusDescription::GetUpgradeInstance() const
{
    return upgradeInstance_;
}

shared_ptr<FabricUpgradeDescription> const & FabricUpgradeStatusDescription::GetUpgradeDescription() const
{
    return upgradeDescription_;
}

TimeSpan const FabricUpgradeStatusDescription::GetUpgradeDuration() const
{
    return upgradeDuration_;
}

TimeSpan const FabricUpgradeStatusDescription::GetCurrentUpgradeDomainDuration() const
{
    return currentUpgradeDomainDuration_;
}

vector<HealthEvaluation> const & FabricUpgradeStatusDescription::GetHealthEvaluations() const
{
    return unhealthyEvaluations_;
}

Reliability::UpgradeDomainProgress const& FabricUpgradeStatusDescription::GetCurrentUpgradeDomainProgress() const
{
    return currentUpgradeDomainProgress_;
}

Common::DateTime FabricUpgradeStatusDescription::GetStartTime() const
{
    return commonUpgradeContextData_.StartTime;
}

Common::DateTime FabricUpgradeStatusDescription::GetFailureTime() const
{
    return commonUpgradeContextData_.FailureTime;
}

Management::ClusterManager::UpgradeFailureReason::Enum FabricUpgradeStatusDescription::GetFailureReason() const
{
    return commonUpgradeContextData_.FailureReason;
}

Reliability::UpgradeDomainProgress const& FabricUpgradeStatusDescription::GetUpgradeDomainProgressAtFailure() const
{
    return commonUpgradeContextData_.UpgradeProgressAtFailure;
}

FABRIC_UPGRADE_STATE FabricUpgradeStatusDescription::ToPublicUpgradeState() const
{
    switch (upgradeState_)
    {
    case FabricUpgradeState::CompletedRollforward:
        return FABRIC_UPGRADE_STATE_ROLLING_FORWARD_COMPLETED;

    case FabricUpgradeState::CompletedRollback:
    case FabricUpgradeState::Interrupted:
        return FABRIC_UPGRADE_STATE_ROLLING_BACK_COMPLETED;

    case FabricUpgradeState::RollingForward:
        //
        // For UnmonitoredManual, "pending" indicates that the system is 
        // waiting for an explicit move next.
        //
        // For Monitored, "pending" indicates that a health check is pending.
        //
        if (inProgressUpgradeDomain_.empty() && !pendingUpgradeDomains_.empty() && this->GetRollingUpgradeMode() != RollingUpgradeMode::Enum::UnmonitoredAuto)
        {
            return FABRIC_UPGRADE_STATE_ROLLING_FORWARD_PENDING;
        }
        else
        {
            return FABRIC_UPGRADE_STATE_ROLLING_FORWARD_IN_PROGRESS;
        }

    case FabricUpgradeState::RollingBack:
        if (inProgressUpgradeDomain_.empty() && !pendingUpgradeDomains_.empty() && this->GetRollingUpgradeMode() != RollingUpgradeMode::Enum::UnmonitoredAuto)
        {
            return FABRIC_UPGRADE_STATE_ROLLING_BACK_PENDING;
        }
        else
        {
            return FABRIC_UPGRADE_STATE_ROLLING_BACK_IN_PROGRESS;
        }

    case FabricUpgradeState::Failed:
        return FABRIC_UPGRADE_STATE_FAILED;

    default:
        return FABRIC_UPGRADE_STATE_INVALID;
    }
}

void FabricUpgradeStatusDescription::ToPublicUpgradeDomains(
    __in ScopedHeap & heap,
    __out ReferenceArray<FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION> & result) const
{
    UpgradeHelper::ToPublicUpgradeDomains(
        heap,
        inProgressUpgradeDomain_,
        pendingUpgradeDomains_,
        completedUpgradeDomains_,
        result);
}

ErrorCode FabricUpgradeStatusDescription::ToPublicChangedUpgradeDomains(
    __in ScopedHeap & heap,
    FabricCodeVersion const & prevTargetCodeVersion,
    FabricConfigVersion const & prevTargetConfigVersion,
    wstring const & prevInProgress,
    vector<wstring> const & prevCompleted,
    __out ReferenceArray<FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION> & result) const
{
    if (prevTargetCodeVersion != this->GetTargetCodeVersion())
    {
        return ErrorCodeValue::InvalidArgument;
    }

    if (prevTargetConfigVersion != this->GetTargetConfigVersion())
    {
        return ErrorCodeValue::InvalidArgument;
    }

    wstring changedInProgressDomain;
    vector<wstring> changedCompletedDomains;

    this->GetChangedUpgradeDomains(
        prevInProgress,
        prevCompleted,
        changedInProgressDomain,
        changedCompletedDomains);

    UpgradeHelper::ToPublicChangedUpgradeDomains(
        heap,
        changedInProgressDomain,
        changedCompletedDomains,
        result);

    return ErrorCodeValue::Success;
}

LPCWSTR FabricUpgradeStatusDescription::ToPublicNextUpgradeDomain() const
{
    return nextUpgradeDomain_.empty() ? NULL : nextUpgradeDomain_.c_str();
}

void FabricUpgradeStatusDescription::ToPublicApi(
    __in ScopedHeap & heap, 
    __out FABRIC_UPGRADE_PROGRESS & progressResult) const
{
    if (upgradeDescription_)
    {
        auto descriptionPtr = heap.AddItem<FABRIC_UPGRADE_DESCRIPTION>();

        upgradeDescription_->ToPublicApi(heap, *descriptionPtr);

        progressResult.UpgradeDescription = descriptionPtr.GetRawPointer();
    }

    progressResult.UpgradeState = this->ToPublicUpgradeState();

    progressResult.UpgradeMode = RollingUpgradeMode::ToPublicApi(rollingUpgradeMode_);

    progressResult.NextUpgradeDomain = this->ToPublicNextUpgradeDomain();

    ReferenceArray<FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION> domains;
    this->ToPublicUpgradeDomains(heap, domains);

    auto domainListPtr = heap.AddItem<FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION_LIST>();
    domainListPtr->Count = static_cast<ULONG>(domains.GetCount());
    domainListPtr->Items = domains.GetRawArray();

    progressResult.UpgradeDomains = domainListPtr.GetRawPointer();

    progressResult.UpgradeDurationInSeconds = static_cast<DWORD>(upgradeDuration_.TotalSeconds());

    progressResult.CurrentUpgradeDomainDurationInSeconds = static_cast<DWORD>(currentUpgradeDomainDuration_.TotalSeconds());

    auto publicHealthEvaluationsPtr = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
    auto error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(
        heap, 
        unhealthyEvaluations_, 
        *publicHealthEvaluationsPtr); 
    if (error.IsSuccess())
    {
        progressResult.UnhealthyEvaluations = publicHealthEvaluationsPtr.GetRawPointer();
    }
    else
    {
        Trace.WriteError("FabricUpgradeStatusDescription", "Unhealthy evaluations to public API failed: error={0}", error);

        progressResult.UnhealthyEvaluations = NULL;
    }

    auto upgradeDomainProgressPtr = heap.AddItem<FABRIC_UPGRADE_DOMAIN_PROGRESS>();
    currentUpgradeDomainProgress_.ToPublicApi(heap, *upgradeDomainProgressPtr);
    progressResult.CurrentUpgradeDomainProgress = upgradeDomainProgressPtr.GetRawPointer();

    auto progressResultEx1Ptr = heap.AddItem<FABRIC_UPGRADE_PROGRESS_EX1>();
    progressResultEx1Ptr->StartTimestampUtc = commonUpgradeContextData_.StartTime.AsFileTime;
    progressResultEx1Ptr->FailureTimestampUtc = commonUpgradeContextData_.FailureTime.AsFileTime;
    progressResultEx1Ptr->FailureReason = UpgradeFailureReason::ToPublicApi(commonUpgradeContextData_.FailureReason);

    if (commonUpgradeContextData_.FailureTime != DateTime::Zero)
    {
        auto upgradeProgressAtFailurePtr = heap.AddItem<FABRIC_UPGRADE_DOMAIN_PROGRESS>();
        commonUpgradeContextData_.UpgradeProgressAtFailure.ToPublicApi(heap, *upgradeProgressAtFailurePtr);
        progressResultEx1Ptr->UpgradeDomainProgressAtFailure = upgradeProgressAtFailurePtr.GetRawPointer();
    }

    progressResult.Reserved = progressResultEx1Ptr.GetRawPointer();
}

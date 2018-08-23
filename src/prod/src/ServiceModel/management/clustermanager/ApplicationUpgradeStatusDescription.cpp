// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;

using namespace Management::ClusterManager;

ApplicationUpgradeStatusDescription::ApplicationUpgradeStatusDescription()
    : applicationName_()
    , applicationTypeName_()
    , targetApplicationTypeVersion_()
    , upgradeState_(ApplicationUpgradeState::Invalid)
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

ApplicationUpgradeStatusDescription::ApplicationUpgradeStatusDescription(ApplicationUpgradeStatusDescription && other) 
    : applicationName_(move(other.applicationName_))
    , applicationTypeName_(move(other.applicationTypeName_))
    , targetApplicationTypeVersion_(move(other.targetApplicationTypeVersion_))
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

ApplicationUpgradeStatusDescription & ApplicationUpgradeStatusDescription::operator=(ApplicationUpgradeStatusDescription && other) 
{
    if (this != &other)
    {
        applicationName_ = move(other.applicationName_);
        applicationTypeName_ = move(other.applicationTypeName_);
        targetApplicationTypeVersion_ = move(other.targetApplicationTypeVersion_);
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

ApplicationUpgradeStatusDescription::ApplicationUpgradeStatusDescription(
    NamingUri const & applicationName,
    wstring const & applicationTypeName,
    wstring const & targetApplicationTypeVersion,
    ApplicationUpgradeState::Enum upgradeState,
    wstring && inProgressUpgradeDomain,
    vector<wstring> && completedUpgradeDomains,
    vector<wstring> && pendingUpgradeDomains,
    uint64 upgradeInstance, 
    ServiceModel::RollingUpgradeMode::Enum rollingUpgradeMode,
    shared_ptr<ApplicationUpgradeDescription> && upgradeDescription,
    TimeSpan const upgradeDuration,
    TimeSpan const currentUpgradeDomainDuration,
    vector<ServiceModel::HealthEvaluation> && unhealthyEvaluations,
    Reliability::UpgradeDomainProgress && currentUpgradeDomainProgress,
    Management::ClusterManager::CommonUpgradeContextData && commonUpgradeContextData)
    : applicationName_(applicationName)
    , applicationTypeName_(applicationTypeName)
    , targetApplicationTypeVersion_(targetApplicationTypeVersion)
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

NamingUri const& ApplicationUpgradeStatusDescription::GetApplicationName() const
{
    return applicationName_;
}

wstring const& ApplicationUpgradeStatusDescription::GetApplicationTypeName() const
{
    return applicationTypeName_;
}

wstring const& ApplicationUpgradeStatusDescription::GetTargetApplicationTypeVersion() const
{
    return targetApplicationTypeVersion_;
}

ApplicationUpgradeState::Enum ApplicationUpgradeStatusDescription::GetUpgradeState() const
{
    return upgradeState_;
}

void ApplicationUpgradeStatusDescription::GetUpgradeDomains(
    __out wstring &inProgressDomain,
    __out vector<wstring> &pendingDomains, 
    __out vector<wstring> &completedDomains) const
{
    inProgressDomain = inProgressUpgradeDomain_;
    pendingDomains = pendingUpgradeDomains_;
    completedDomains = completedUpgradeDomains_;
}

void ApplicationUpgradeStatusDescription::GetChangedUpgradeDomains(
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

RollingUpgradeMode::Enum ApplicationUpgradeStatusDescription::GetRollingUpgradeMode() const
{
    return rollingUpgradeMode_;
}

wstring const& ApplicationUpgradeStatusDescription::GetNextUpgradeDomain() const
{
    return nextUpgradeDomain_;
}

uint64 ApplicationUpgradeStatusDescription::GetUpgradeInstance() const
{
    return upgradeInstance_;
}

shared_ptr<ApplicationUpgradeDescription> const & ApplicationUpgradeStatusDescription::GetUpgradeDescription() const
{
    return upgradeDescription_;
}

TimeSpan const ApplicationUpgradeStatusDescription::GetUpgradeDuration() const
{
    return upgradeDuration_;
}

TimeSpan const ApplicationUpgradeStatusDescription::GetCurrentUpgradeDomainDuration() const
{
    return currentUpgradeDomainDuration_;
}

vector<HealthEvaluation> const & ApplicationUpgradeStatusDescription::GetHealthEvaluations() const
{
    return unhealthyEvaluations_;
}

Reliability::UpgradeDomainProgress const& ApplicationUpgradeStatusDescription::GetCurrentUpgradeDomainProgress() const
{
    return currentUpgradeDomainProgress_;
}

Common::DateTime ApplicationUpgradeStatusDescription::GetStartTime() const
{
    return commonUpgradeContextData_.StartTime;
}

Common::DateTime ApplicationUpgradeStatusDescription::GetFailureTime() const
{
    return commonUpgradeContextData_.FailureTime;
}

Management::ClusterManager::UpgradeFailureReason::Enum ApplicationUpgradeStatusDescription::GetFailureReason() const
{
    return commonUpgradeContextData_.FailureReason;
}

Reliability::UpgradeDomainProgress const& ApplicationUpgradeStatusDescription::GetUpgradeDomainProgressAtFailure() const
{
    return commonUpgradeContextData_.UpgradeProgressAtFailure;
}

wstring const & ApplicationUpgradeStatusDescription::GetUpgradeStatusDetails() const
{
    return commonUpgradeContextData_.UpgradeStatusDetails;
}

FABRIC_APPLICATION_UPGRADE_STATE ApplicationUpgradeStatusDescription::ToPublicUpgradeState() const
{
    switch (upgradeState_)
    {
    case ApplicationUpgradeState::CompletedRollforward:
        return FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_FORWARD_COMPLETED;

    case ApplicationUpgradeState::CompletedRollback:
    case ApplicationUpgradeState::Interrupted:
        return FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_BACK_COMPLETED;

    case ApplicationUpgradeState::RollingForward:
        // UnmonitoredAuto mode always returns IN_PROGRESS since there is no waiting or pending health check.
        // For UnmonitoredManual, "pending" indicates that the system is 
        // waiting for an explicit move next.
        //
        // For Monitored, "pending" indicates that a health check is pending.
        //
        if (inProgressUpgradeDomain_.empty() && !pendingUpgradeDomains_.empty() && this->GetRollingUpgradeMode() != RollingUpgradeMode::Enum::UnmonitoredAuto)
        {
            return FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_FORWARD_PENDING;
        }
        else
        {
            return FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_FORWARD_IN_PROGRESS;
        }

    case ApplicationUpgradeState::RollingBack:
        if (inProgressUpgradeDomain_.empty() && !pendingUpgradeDomains_.empty() && this->GetRollingUpgradeMode() != RollingUpgradeMode::Enum::UnmonitoredAuto)
        {
            return FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_BACK_PENDING;
        }
        else
        {
            return FABRIC_APPLICATION_UPGRADE_STATE_ROLLING_BACK_IN_PROGRESS;
        }

    case ApplicationUpgradeState::Failed:
        return FABRIC_APPLICATION_UPGRADE_STATE_FAILED;

    default:
        return FABRIC_APPLICATION_UPGRADE_STATE_INVALID;
    }
}

void ApplicationUpgradeStatusDescription::ToPublicUpgradeDomains(
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

ErrorCode ApplicationUpgradeStatusDescription::ToPublicChangedUpgradeDomains(
    __in ScopedHeap & heap,
    NamingUri const & prevAppName,
    wstring const & prevAppTypeName,
    wstring const & prevTargetAppTypeVersion,
    wstring const & prevInProgress,
    vector<wstring> const & prevCompleted,
    __out ReferenceArray<FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION> & result) const
{
    if (prevAppName != this->GetApplicationName())
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString("{0} ({1}, {2})", GET_CM_RC( Mismatched_Application ), this->GetApplicationName(), prevAppName));
    }

    if (prevAppTypeName != this->GetApplicationTypeName())
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString("{0} ({1}, {2})", GET_CM_RC( Mismatched_Application_Type ), this->GetApplicationTypeName(), prevAppTypeName));
    }

    if (prevTargetAppTypeVersion != this->GetTargetApplicationTypeVersion())
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString("{0} ({1}, {2})", GET_CM_RC( Mismatched_Application_Version ), this->GetTargetApplicationTypeVersion(), prevTargetAppTypeVersion));
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

LPCWSTR ApplicationUpgradeStatusDescription::ToPublicNextUpgradeDomain() const
{
    return nextUpgradeDomain_.empty() ? NULL : nextUpgradeDomain_.c_str();
}

void ApplicationUpgradeStatusDescription::ToPublicApi(
    __in ScopedHeap & heap, 
    __out FABRIC_APPLICATION_UPGRADE_PROGRESS & progressResult) const
{
    if (upgradeDescription_)
    {
        auto descriptionPtr = heap.AddItem<FABRIC_APPLICATION_UPGRADE_DESCRIPTION>();

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
        Trace.WriteError("ApplicationUpgradeStatusDescription", "Unhealthy evaluations to public API failed: error={0}", error);

        progressResult.UnhealthyEvaluations = NULL;
    }

    auto upgradeDomainProgressPtr = heap.AddItem<FABRIC_UPGRADE_DOMAIN_PROGRESS>();
    currentUpgradeDomainProgress_.ToPublicApi(heap, *upgradeDomainProgressPtr);
    progressResult.CurrentUpgradeDomainProgress = upgradeDomainProgressPtr.GetRawPointer();

    auto progressResultEx1Ptr = heap.AddItem<FABRIC_APPLICATION_UPGRADE_PROGRESS_EX1>();
    progressResultEx1Ptr->StartTimestampUtc = commonUpgradeContextData_.StartTime.AsFileTime;
    progressResultEx1Ptr->FailureTimestampUtc = commonUpgradeContextData_.FailureTime.AsFileTime;
    progressResultEx1Ptr->FailureReason = UpgradeFailureReason::ToPublicApi(commonUpgradeContextData_.FailureReason);

    if (commonUpgradeContextData_.FailureTime != DateTime::Zero)
    {
        auto upgradeProgressAtFailurePtr = heap.AddItem<FABRIC_UPGRADE_DOMAIN_PROGRESS>();
        commonUpgradeContextData_.UpgradeProgressAtFailure.ToPublicApi(heap, *upgradeProgressAtFailurePtr);
        progressResultEx1Ptr->UpgradeDomainProgressAtFailure = upgradeProgressAtFailurePtr.GetRawPointer();
    }

    auto progressResultEx2Ptr = heap.AddItem<FABRIC_APPLICATION_UPGRADE_PROGRESS_EX2>();
    progressResultEx2Ptr->UpgradeStatusDetails = commonUpgradeContextData_.UpgradeStatusDetails.empty() ? NULL : heap.AddString(commonUpgradeContextData_.UpgradeStatusDetails);

    progressResultEx1Ptr->Reserved = progressResultEx2Ptr.GetRawPointer();
    progressResult.Reserved = progressResultEx1Ptr.GetRawPointer();
}

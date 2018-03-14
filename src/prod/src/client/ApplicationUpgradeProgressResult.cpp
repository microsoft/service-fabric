// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace ServiceModel;
using namespace Management::ClusterManager;
using namespace Naming;
using namespace Client;

ApplicationUpgradeProgressResult::ApplicationUpgradeProgressResult(
    Management::ClusterManager::ApplicationUpgradeStatusDescription &&desc)
    : upgradeDescription_(move(desc))
{
}

NamingUri const& ApplicationUpgradeProgressResult::GetApplicationName()
{
    return upgradeDescription_.GetApplicationName();
}

wstring const& ApplicationUpgradeProgressResult::GetApplicationTypeName()
{
    return upgradeDescription_.GetApplicationTypeName();
}

wstring const& ApplicationUpgradeProgressResult::GetTargetApplicationTypeVersion()
{
    return upgradeDescription_.GetTargetApplicationTypeVersion();
}

ApplicationUpgradeState::Enum ApplicationUpgradeProgressResult::GetUpgradeState()
{
    return upgradeDescription_.GetUpgradeState();
}

ErrorCode ApplicationUpgradeProgressResult::GetUpgradeDomains(
    __out wstring &inProgressDomain,
    __out vector<wstring> &pendingDomains, 
    __out vector<wstring> &completedDomains)
{
    upgradeDescription_.GetUpgradeDomains(inProgressDomain, pendingDomains, completedDomains);

    return ErrorCode::Success();
}

ErrorCode ApplicationUpgradeProgressResult::GetChangedUpgradeDomains(
    IApplicationUpgradeProgressResultPtr const &previousProgress,
    __out wstring &changedInProgressDomain,
    __out vector<wstring> &changedCompletedDomains)
{
    wstring prevInProgress;
    vector<wstring> prevPending;
    vector<wstring> prevCompleted;
    auto error = previousProgress->GetUpgradeDomains(prevInProgress, prevPending, prevCompleted);
    if (!error.IsSuccess()) { return error; }

    upgradeDescription_.GetChangedUpgradeDomains(
        prevInProgress,
        prevCompleted,
        changedInProgressDomain,
        changedCompletedDomains);

    return ErrorCode::Success();
}

RollingUpgradeMode::Enum ApplicationUpgradeProgressResult::GetRollingUpgradeMode()
{
    return upgradeDescription_.GetRollingUpgradeMode();
}

wstring const& ApplicationUpgradeProgressResult::GetNextUpgradeDomain()
{
    return upgradeDescription_.GetNextUpgradeDomain();
}

uint64 ApplicationUpgradeProgressResult::GetUpgradeInstance()
{
    return upgradeDescription_.GetUpgradeInstance();
}

shared_ptr<ApplicationUpgradeDescription> const & ApplicationUpgradeProgressResult::GetUpgradeDescription()
{
    return upgradeDescription_.GetUpgradeDescription();
}

TimeSpan const ApplicationUpgradeProgressResult::GetUpgradeDuration()
{
    return upgradeDescription_.GetUpgradeDuration();
}

TimeSpan const ApplicationUpgradeProgressResult::GetCurrentUpgradeDomainDuration()
{
    return upgradeDescription_.GetCurrentUpgradeDomainDuration();
}

vector<HealthEvaluation> const & ApplicationUpgradeProgressResult::GetHealthEvaluations() const
{
    return upgradeDescription_.GetHealthEvaluations();
}

Reliability::UpgradeDomainProgress const& ApplicationUpgradeProgressResult::GetCurrentUpgradeDomainProgress() const
{
    return upgradeDescription_.GetCurrentUpgradeDomainProgress();
}

Common::DateTime ApplicationUpgradeProgressResult::GetStartTime() const
{
    return upgradeDescription_.GetStartTime();
}

Common::DateTime ApplicationUpgradeProgressResult::GetFailureTime() const
{
    return upgradeDescription_.GetFailureTime();
}

Management::ClusterManager::UpgradeFailureReason::Enum ApplicationUpgradeProgressResult::GetFailureReason() const
{
    return upgradeDescription_.GetFailureReason();
}

Reliability::UpgradeDomainProgress const& ApplicationUpgradeProgressResult::GetUpgradeDomainProgressAtFailure() const
{
    return upgradeDescription_.GetUpgradeDomainProgressAtFailure();
}

wstring const & ApplicationUpgradeProgressResult::GetUpgradeStatusDetails() const
{
    return upgradeDescription_.GetUpgradeStatusDetails();
}

FABRIC_APPLICATION_UPGRADE_STATE ApplicationUpgradeProgressResult::ToPublicUpgradeState() const
{
    return upgradeDescription_.ToPublicUpgradeState();
}

void ApplicationUpgradeProgressResult::ToPublicUpgradeDomains(
    __in Common::ScopedHeap & heap,
    __out Common::ReferenceArray<FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION> & result) const
{
    upgradeDescription_.ToPublicUpgradeDomains(heap, result);
}

Common::ErrorCode ApplicationUpgradeProgressResult::ToPublicChangedUpgradeDomains(
    __in Common::ScopedHeap & heap, 
    IApplicationUpgradeProgressResultPtr const & previousProgress,
    __out Common::ReferenceArray<FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION> & result) const
{
    wstring prevInProgress;
    vector<wstring> prevPending;
    vector<wstring> prevCompleted;
    auto error = previousProgress->GetUpgradeDomains(prevInProgress, prevPending, prevCompleted);
    if (!error.IsSuccess()) { return error; }

    return upgradeDescription_.ToPublicChangedUpgradeDomains(
        heap,
        previousProgress->GetApplicationName(),
        previousProgress->GetApplicationTypeName(),
        previousProgress->GetTargetApplicationTypeVersion(),
        prevInProgress,
        prevCompleted,
        result);
}

LPCWSTR ApplicationUpgradeProgressResult::ToPublicNextUpgradeDomain() const
{
    return upgradeDescription_.ToPublicNextUpgradeDomain();
}

void ApplicationUpgradeProgressResult::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_APPLICATION_UPGRADE_PROGRESS & result) const
{
    return upgradeDescription_.ToPublicApi(heap, result);
}

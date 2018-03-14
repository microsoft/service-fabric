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
using namespace Client;

UpgradeProgressResult::UpgradeProgressResult(
    Management::ClusterManager::FabricUpgradeStatusDescription &&desc)
    : upgradeDescription_(move(desc))
{
}

FabricCodeVersion const& UpgradeProgressResult::GetTargetCodeVersion()
{
    return upgradeDescription_.GetTargetCodeVersion();
}

FabricConfigVersion const& UpgradeProgressResult::GetTargetConfigVersion()
{
    return upgradeDescription_.GetTargetConfigVersion();
}

FabricUpgradeState::Enum UpgradeProgressResult::GetUpgradeState()
{
    return upgradeDescription_.GetUpgradeState();
}

ErrorCode UpgradeProgressResult::GetUpgradeDomains(
    __out wstring &inProgressDomain,
    __out vector<wstring> &pendingDomains, 
    __out vector<wstring> &completedDomains)
{
    inProgressDomain = upgradeDescription_.InProgressUpgradeDomain;
    pendingDomains = upgradeDescription_.PendingUpgradeDomains;
    completedDomains = upgradeDescription_.CompletedUpgradeDomains;
    return ErrorCode::Success();
}

ErrorCode UpgradeProgressResult::GetChangedUpgradeDomains(
    IUpgradeProgressResultPtr const &previousProgress,
    __out wstring &changedInProgressDomain,
    __out vector<wstring> &changedCompletedDomains)
{
    wstring prevInProgress;
    vector<wstring> prevPending;
    vector<wstring> prevCompleted;

    auto err = previousProgress->GetUpgradeDomains(prevInProgress, prevPending, prevCompleted);
    if (!err.IsSuccess()) { return err; }

    upgradeDescription_.GetChangedUpgradeDomains(
        prevInProgress,
        prevCompleted,
        changedInProgressDomain,
        changedCompletedDomains);

    return ErrorCode::Success();
}

RollingUpgradeMode::Enum UpgradeProgressResult::GetRollingUpgradeMode()
{
    return upgradeDescription_.GetRollingUpgradeMode();
}

wstring const& UpgradeProgressResult::GetNextUpgradeDomain()
{
    return upgradeDescription_.GetNextUpgradeDomain();
}

uint64 UpgradeProgressResult::GetUpgradeInstance()
{
    return upgradeDescription_.GetUpgradeInstance();
}

shared_ptr<FabricUpgradeDescription> const & UpgradeProgressResult::GetUpgradeDescription()
{
    return upgradeDescription_.GetUpgradeDescription();
}

TimeSpan const UpgradeProgressResult::GetUpgradeDuration()
{
    return upgradeDescription_.GetUpgradeDuration();
}

TimeSpan const UpgradeProgressResult::GetCurrentUpgradeDomainDuration()
{
    return upgradeDescription_.GetCurrentUpgradeDomainDuration();
}

vector<HealthEvaluation> const & UpgradeProgressResult::GetHealthEvaluations() const
{
    return upgradeDescription_.GetHealthEvaluations();
}

Reliability::UpgradeDomainProgress const& UpgradeProgressResult::GetCurrentUpgradeDomainProgress() const
{
    return upgradeDescription_.GetCurrentUpgradeDomainProgress();
}

Common::DateTime UpgradeProgressResult::GetStartTime() const
{
    return upgradeDescription_.GetStartTime();
}

Common::DateTime UpgradeProgressResult::GetFailureTime() const
{
    return upgradeDescription_.GetFailureTime();
}

Management::ClusterManager::UpgradeFailureReason::Enum UpgradeProgressResult::GetFailureReason() const
{
    return upgradeDescription_.GetFailureReason();
}

Reliability::UpgradeDomainProgress const& UpgradeProgressResult::GetUpgradeDomainProgressAtFailure() const
{
    return upgradeDescription_.GetUpgradeDomainProgressAtFailure();
}

FABRIC_UPGRADE_STATE UpgradeProgressResult::ToPublicUpgradeState() const
{
    return upgradeDescription_.ToPublicUpgradeState();
}

void UpgradeProgressResult::ToPublicUpgradeDomains(
    __in Common::ScopedHeap & heap,
    __out Common::ReferenceArray<FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION> & result) const
{
    upgradeDescription_.ToPublicUpgradeDomains(heap, result);
}

Common::ErrorCode UpgradeProgressResult::ToPublicChangedUpgradeDomains(
    __in Common::ScopedHeap & heap, 
    IUpgradeProgressResultPtr const & previousProgress,
    __out Common::ReferenceArray<FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION> & result) const
{
    wstring prevInProgress;
    vector<wstring> prevPending;
    vector<wstring> prevCompleted;
    auto error = previousProgress->GetUpgradeDomains(prevInProgress, prevPending, prevCompleted);
    if (!error.IsSuccess()) { return error; }

    return upgradeDescription_.ToPublicChangedUpgradeDomains(
        heap,
        previousProgress->GetTargetCodeVersion(),
        previousProgress->GetTargetConfigVersion(),
        prevInProgress,
        prevCompleted,
        result);
}

LPCWSTR UpgradeProgressResult::ToPublicNextUpgradeDomain() const
{
    return upgradeDescription_.ToPublicNextUpgradeDomain();
}

void UpgradeProgressResult::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_UPGRADE_PROGRESS & result) const
{
    return upgradeDescription_.ToPublicApi(heap, result);
}

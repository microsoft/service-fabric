// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace ServiceModel;
using namespace std;
using namespace Transport;

FabricUpgradeManager::FabricUpgradeManager(
    FailoverManager & fm,
    FabricVersionInstance && currentVersionInstance,
    FabricUpgradeUPtr && upgrade)
    : fm_(fm)
    , currentVersionInstance_(move(currentVersionInstance))
    , upgrade_(move(upgrade))
{
}

FabricVersionInstance const& FabricUpgradeManager::get_CurrentVersionInstance() const
{
    AcquireReadLock grab(lockObject_);

    return currentVersionInstance_;
}

FabricUpgradeUPtr const& FabricUpgradeManager::get_Upgrade() const
{
    AcquireReadLock grab(lockObject_);

    return upgrade_;
}

ErrorCode FabricUpgradeManager::ProcessFabricUpgrade(UpgradeFabricRequestMessageBody && requestBody, UpgradeFabricReplyMessageBody & replyBody)
{
    AcquireWriteLock grab(lockObject_);

    // Check if this is a stale request.
    if (requestBody.Specification.InstanceId < currentVersionInstance_.InstanceId ||
        (upgrade_ && requestBody.Specification.InstanceId < upgrade_->Description.InstanceId))
    {
        fm_.WriteInfo(FailoverManager::TraceFabricUpgrade, "Upgrade request {0} is stale.", requestBody);
        return ErrorCode(ErrorCodeValue::StaleRequest);
    }

    ErrorCode error = ErrorCode::Success();

    // If an upgrade is already in progress, check if the request is a duplicate.
    if (upgrade_ && requestBody.Specification.InstanceId == upgrade_->Description.InstanceId)
    {
        FabricUpgradeUPtr newUpgrade = make_unique<FabricUpgrade>(*upgrade_);

        if (newUpgrade->UpdateUpgrade(
            fm_,
            move(const_cast<FabricUpgradeSpecification &>(requestBody.Specification)),
            requestBody.UpgradeReplicaSetCheckTimeout,
            move(const_cast<vector<wstring> &>(requestBody.VerifiedUpgradeDomains)),
            requestBody.SequenceNumber))
        {
            newUpgrade->PersistenceState = PersistenceState::ToBeUpdated;
            error = PersistUpgradeUpdateCallerHoldsWriteLock(move(newUpgrade));
        }

        if (error.IsSuccess())
        {
            upgrade_->GetProgress(replyBody);
            fm_.WriteInfo(FailoverManager::TraceFabricUpgrade, "Upgrade request processed: {0}", requestBody);
        }

        return error;
    }

    UpgradeDomainSortPolicy::Enum sortPolicy =
        (FailoverConfig::GetConfig().SortUpgradeDomainNamesAsNumbers ? UpgradeDomainSortPolicy::DigitsAsNumbers : UpgradeDomainSortPolicy::Lexicographical);
    UpgradeDomainsCSPtr upgradeDomains = fm_.NodeCacheObj.GetUpgradeDomains(sortPolicy);

    // Check if the requested upgrade has already completed.
    if (requestBody.Specification.InstanceId == currentVersionInstance_.InstanceId)
    {
        vector<wstring> uds = upgradeDomains->UDs;
        replyBody.SetCompletedUpgradeDomains(move(uds));
        fm_.WriteInfo(FailoverManager::TraceFabricUpgrade, "Upgrade request is already completed: {0}", requestBody);

        return ErrorCode::Success();
    }

    //
    // This is a new upgrade request. Create and persist the corresponding FabricUpgrade object.
    //

    FabricUpgradeUPtr upgrade = make_unique<FabricUpgrade>(
        move(const_cast<FabricUpgradeSpecification &>(requestBody.Specification)),
        requestBody.UpgradeReplicaSetCheckTimeout,
        upgradeDomains,
        requestBody.SequenceNumber,
        requestBody.IsRollback);

    if (upgrade_)
    {
        upgrade->PersistenceState = PersistenceState::ToBeUpdated;
    }

    // Persist the FabricUpgrade object.
    error = PersistUpgradeUpdateCallerHoldsWriteLock(move(upgrade));
    if (error.IsSuccess())
    {
        upgrade_->GetProgress(replyBody);

        fm_.WriteInfo(
            FailoverManager::TraceFabricUpgrade,
            "Upgrade request accepted: Version={0}:{1}, Type={2}, IsMonitored={3}, IsManual={4}, IsRollback={5}, CheckTimeout={6}, SeqNum={7}",
            upgrade_->Description.Version,
            upgrade_->Description.InstanceId,
            upgrade_->Description.UpgradeType,
            upgrade_->Description.IsMonitored,
            upgrade_->Description.IsManual,
            requestBody.IsRollback,
            requestBody.UpgradeReplicaSetCheckTimeout,
            requestBody.SequenceNumber);
    }

    return error;
}

void FabricUpgradeManager::ProcessNodeFabricUpgradeReplyAsync(NodeFabricUpgradeReplyMessageBody && replyBody, NodeInstance const& from)
{
    NodeInfoSPtr nodeInfo = fm_.NodeCacheObj.GetNode(from.Id);

    FabricVersionInstance targetVersionInstance;

    {
        AcquireWriteLock grab(lockObject_);

        if (upgrade_)
        {
            if (upgrade_->CheckUpgradeDomain(nodeInfo->ActualUpgradeDomainId))
            {
                targetVersionInstance = FabricVersionInstance(upgrade_->Description.Version, upgrade_->Description.InstanceId);
            }
            else if (replyBody.VersionInstance.InstanceId > currentVersionInstance_.InstanceId)
            {
                targetVersionInstance = replyBody.VersionInstance;
            }
        }
        else
        {
            targetVersionInstance = currentVersionInstance_;
        }
    }

    if (replyBody.VersionInstance == targetVersionInstance)
    {
        fm_.NodeCacheObj.UpdateVersionInstanceAsync(from, targetVersionInstance);
    }
}

void FabricUpgradeManager::ProcessCancelFabricUpgradeReply(NodeFabricUpgradeReplyMessageBody && replyBody, NodeInstance const& from) const
{
    AcquireReadLock grab(lockObject_);

    if (!upgrade_)
    {
        return;
    }

    FabricVersionInstance targetVersionInstance = FabricVersionInstance(upgrade_->Description.Version, upgrade_->Description.InstanceId);

    if (replyBody.VersionInstance == targetVersionInstance)
    {
        NodeInfoSPtr nodeInfo = fm_.NodeCacheObj.GetNode(from.Id);
        fm_.NodeCacheObj.BeginSetPendingFabricUpgrade(
            *nodeInfo,
            false,
            [this](AsyncOperationSPtr const& operation) { fm_.NodeCacheObj.EndSetPendingFabricUpgrade(operation); },
            fm_.CreateAsyncOperationRoot());
    }
}

void FabricUpgradeManager::UpdateFabricUpgradeProgress(
    FabricUpgrade const& upgrade,
    map<NodeId, NodeUpgradeProgress> && pendingNodes,
    map<NodeId, pair<NodeUpgradeProgress, NodeInfoSPtr>> && readyNodes,
    map<NodeId, NodeUpgradeProgress> && waitingNodes,
    bool isCurrentDomainComplete)
{
    AcquireWriteLock grab(lockObject_);

    if (upgrade.Description.InstanceId == upgrade_->Description.InstanceId)
    {
        FabricUpgradeUPtr newUpgrade = make_unique<FabricUpgrade>(*upgrade_);
        bool isUpdated = newUpgrade->UpdateProgress(fm_, readyNodes, true, isCurrentDomainComplete);
        bool isUpgradeComplete = newUpgrade->IsCompleted();

        ErrorCode error = ErrorCode::Success();
        if (!isUpgradeComplete && (isCurrentDomainComplete || isUpdated))
        {
            newUpgrade->PersistenceState = PersistenceState::ToBeUpdated;
            error = PersistUpgradeUpdateCallerHoldsWriteLock(move(newUpgrade));
        }

        upgrade_->UpdateProgressDetails(move(pendingNodes), move(readyNodes), move(waitingNodes), isCurrentDomainComplete);

        if (error.IsSuccess() && isUpgradeComplete)
        {
            CompleteFabricUpgradeCallerHoldsWriteLock();
        }
    }
}

Common::ErrorCode FabricUpgradeManager::PersistUpgradeUpdateCallerHoldsWriteLock(FabricUpgradeUPtr && newUpgrade)
{
    ErrorCode error = fm_.Store.UpdateFabricUpgrade(*newUpgrade);
    if (error.IsSuccess())
    {
        upgrade_ = move(newUpgrade);

        if (upgrade_)
        {
            fm_.PLB.UpdateClusterUpgrade(true, upgrade_->GetCompletedDomains());

            fm_.WriteInfo(FailoverManager::TraceFabricUpgrade, "Upgrade updated: {0}", upgrade_);
        }
        else
        {
            fm_.PLB.UpdateClusterUpgrade(false, set<wstring>());
        }
    }
    else
    {
        fm_.WriteWarning(
            FailoverManager::TraceFabricUpgrade,
            "Failed to persist FabricUpgrade with error '{0}': {1}", error, newUpgrade);
    }

    return error;
}

void FabricUpgradeManager::CompleteFabricUpgradeCallerHoldsWriteLock()
{
    ErrorCode error = ErrorCode::Success();

    // Create the new FabricVersionInstance.
    FabricVersionInstance versionInstance(upgrade_->Description.Version, upgrade_->Description.InstanceId);

    fm_.WriteInfo(FailoverManager::TraceFabricUpgrade, "Completing upgrade for version {0}", versionInstance);

    // Persist and update the current FabricVersionInstance.
    if (currentVersionInstance_ != versionInstance)
    {
        error = UpdateCurrentVersionInstanceCallerHoldsWriteLock(versionInstance);
    }

    // Delete the FabricUpgrade object.
    if (error.IsSuccess())
    {
        upgrade_->PersistenceState = PersistenceState::ToBeDeleted;
        error = fm_.Store.UpdateFabricUpgrade(*upgrade_);
        if (error.IsSuccess())
        {
            upgrade_ = nullptr;

            fm_.PLB.UpdateClusterUpgrade(false, set<wstring>());
        }
    }

    if (error.IsSuccess())
    {
        fm_.WriteInfo(FailoverManager::TraceFabricUpgrade, "Upgrade to version {0} completed.", versionInstance);
    }
    else
    {
        fm_.WriteInfo(FailoverManager::TraceFabricUpgrade, "Upgrade completion to version {0} failed with '{1}'.", versionInstance, error);
    }
}

bool FabricUpgradeManager::IsFabricUpgradeNeeded(
    FabricVersionInstance const& incomingVersionInstance,
    wstring const& upgradeDomain,
    __out FabricVersionInstance & targetVersionInstance)
{
    if (fm_.IsMaster || !FailoverConfig::GetConfig().IsFabricUpgradeGatekeepingEnabled)
    {
        targetVersionInstance = incomingVersionInstance;
        return false;
    }

    AcquireWriteLock grab(lockObject_);

    if (upgrade_)
    {
        if (upgrade_->IsDomainStarted(upgradeDomain))
        {
            // Node belongs to the current UD
            if (incomingVersionInstance.Version == upgrade_->Description.Version)
            {
                targetVersionInstance = FabricVersionInstance(incomingVersionInstance.Version, upgrade_->Description.InstanceId);
            }
            else
            {
                targetVersionInstance = (incomingVersionInstance < currentVersionInstance_ ? currentVersionInstance_ : incomingVersionInstance);
            }
        }
        else if (upgrade_->CheckUpgradeDomain(upgradeDomain))
        {
            // Node belongs to the UD that has been upgraded
            targetVersionInstance = FabricVersionInstance(upgrade_->Description.Version, upgrade_->Description.InstanceId);
        }
        else
        {
            // Node belongs to a UD for which upgrade has not started yet
            targetVersionInstance = (incomingVersionInstance < currentVersionInstance_ ? currentVersionInstance_ : incomingVersionInstance);
        }
    }
    else
    {
        targetVersionInstance = currentVersionInstance_;
    }

    bool upgradeNeeded = (incomingVersionInstance != targetVersionInstance);

    return upgradeNeeded;
}

ErrorCode FabricUpgradeManager::UpdateCurrentVersionInstance(FabricVersionInstance const& currentVersionInstance)
{
    AcquireWriteLock lock(lockObject_);

    return UpdateCurrentVersionInstanceCallerHoldsWriteLock(currentVersionInstance);
}

ErrorCode FabricUpgradeManager::UpdateCurrentVersionInstanceCallerHoldsWriteLock(FabricVersionInstance const& currentVersionInstance)
{
    ErrorCode error = fm_.Store.UpdateFabricVersionInstance(currentVersionInstance);
    if (error.IsSuccess())
    {
        fm_.WriteInfo(FailoverManager::TraceFabricUpgrade, "Fabric version updated to {0}", currentVersionInstance);
        currentVersionInstance_ = currentVersionInstance;
    }

    return error;
}

void FabricUpgradeManager::AddFabricUpgradeContext(vector<BackgroundThreadContextUPtr> & contexts) const
{
    unique_ptr<FabricUpgradeContext> context = nullptr;

    {
        AcquireWriteLock grab(lockObject_);

        if (upgrade_)
        {
            if (upgrade_->UpgradeDomains->IsStale)
            {
                upgrade_->UpdateDomains(fm_.NodeCacheObj.GetUpgradeDomains(upgrade_->UpgradeDomainSortPolicy));
            }

            context = upgrade_->CreateContext(fm_.Id);
        }
    }

    if (context)
    {
        if (context->Initialize(fm_))
        {
            contexts.push_back(move(context));
        }
    }
}

bool FabricUpgradeManager::GetUpgradeStartTime(Common::DateTime & startTime) const
{
    AcquireReadLock grab(lockObject_);

    if (upgrade_)
    {
        startTime = upgrade_->ClusterStartedTime;
        return true;
    }

    return false;
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace ServiceModel;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

FabricUpgradeContext::FabricUpgradeContext(
    wstring const& fmId,
    FabricUpgrade const& upgrade,
    wstring const & currentDomain)
    : UpgradeContext(wformatString("UpgradeContext.{0}.{1}", fmId, upgrade.Description.InstanceId), currentDomain),
      fmId_(fmId),
      upgrade_(upgrade),
      isSafeToUpgrade_(true),
      voterCount_(0)
{
}

FabricUpgradeContext::FabricUpgradeContext(
    wstring const& fmId,
    FabricUpgrade const& upgrade,
    wstring const & currentDomain,
    int voterCount)
    : UpgradeContext(wformatString("UpgradeContext.{0}", fmId), currentDomain),
    fmId_(fmId),
    upgrade_(upgrade),
    isSafeToUpgrade_(true),
    voterCount_(voterCount)
{
}

BackgroundThreadContextUPtr FabricUpgradeContext::CreateNewContext() const
{
    return make_unique<FabricUpgradeContext>(fmId_, upgrade_, currentDomain_, voterCount_);
}

bool FabricUpgradeContext::Initialize(FailoverManager & fm)
{
    vector<NodeInfoSPtr> nodeInfos;
    fm.NodeCacheObj.GetNodes(nodeInfos);

    for (NodeInfoSPtr const& nodeInfo : nodeInfos)
    {
        if (nodeInfo->ActualUpgradeDomainId == currentDomain_)
        {
            LockedNodeInfo lockedNodeInfo;
            ErrorCode error = fm.NodeCacheObj.GetLockedNode(nodeInfo->Id, lockedNodeInfo, TimeSpan::Zero);
            if (!error.IsSuccess())
            {
                fm.WriteInfo(
                    FailoverManager::TraceFabricUpgrade,
                    "Unable to lock node {0} during FabricUpgradeContext::Initialize: {1}", nodeInfo->Id, error);
                return false;
            }

            if (lockedNodeInfo->IsUp)
            {
                if (lockedNodeInfo->VersionInstance.Version != upgrade_.Description.Version ||
                    lockedNodeInfo->VersionInstance.InstanceId != upgrade_.Description.InstanceId)
                {
                    auto upgradeProgress = NodeUpgradeProgress(lockedNodeInfo->Id, lockedNodeInfo->NodeName, NodeUpgradePhase::Upgrading);
                    AddReadyNode(lockedNodeInfo->Id, upgradeProgress, lockedNodeInfo.Get());
                }
                else if (!lockedNodeInfo->IsReplicaUploaded)
                {
                    notReadyNodes_.insert(nodeInfo->Id);
                }
            }
            else if (!lockedNodeInfo->IsNodeStateRemoved &&
                !lockedNodeInfo->IsUnknown &&
                lockedNodeInfo->NodeDownTime > upgrade_.CurrentDomainStartedTime &&
                DateTime::Now() - lockedNodeInfo->NodeDownTime < FailoverConfig::GetConfig().ExpectedNodeFabricUpgradeDuration)
            {
                notReadyNodes_.insert(lockedNodeInfo->Id);
            }
        }
    }

    ProcessSeedNodes(fm);

    return true;
}

void FabricUpgradeContext::ProcessSeedNodes(FailoverManager & fm)
{
    if (upgrade_.Description.UpgradeType == UpgradeType::Rolling_NotificationOnly)
    {
        return;
    }

    // Seed node can only start arbitration after some initital wait
    FederationConfig const & federationConfig = FederationConfig::GetConfig();
    TimeSpan seedNodeWaitInterval = TimeSpan::FromTicks((federationConfig.LeaseDuration + federationConfig.ArbitrationTimeout).Ticks * FailoverConfig::GetConfig().SeedNodeWaitSafetyFactor);

    vector<NodeId> seedNodeIds;
    voterCount_ = fm.Federation.GetSeedNodes(seedNodeIds);

    ASSERT_IF(voterCount_ < 1, "Voter count cannot be less than 1. Current count is {0}.", voterCount_);

    if (voterCount_ < 3)
    {
        // If the voter count is less than 3, upgrade cannot proceed without bringing down the entire cluster.
        // We treat this as a special case and do not do any safety checks when the voter count is less than 3.

        fm.WriteWarning(
            FailoverManager::TraceFabricUpgrade,
            "Unsafe upgrade detected: VoterCount={0}",
            voterCount_);

        return;
    }

    int upCount = voterCount_ - static_cast<int>(seedNodeIds.size());
    int minRequired = voterCount_ / 2 + 1;
    vector<NodeInfoSPtr> candidates;
	wstring nodes = L"(";

    for (NodeId const& seedNodeId : seedNodeIds)
    {
        NodeInfoSPtr const& seedNodeInfo = fm.NodeCacheObj.GetNode(seedNodeId);

        if (!seedNodeInfo)
        {
            continue;
        }

        if (seedNodeInfo->IsUp &&
            seedNodeInfo->ActualUpgradeDomainId == currentDomain_ &&
            seedNodeInfo->VersionInstance.Version != upgrade_.Description.Version)
        {
            if (!seedNodeInfo->IsPendingNodeClose(fm))
            {
                candidates.push_back(seedNodeInfo);
            }
        }
        else if (seedNodeInfo->IsPendingFabricUpgrade &&
                 !upgrade_.CheckUpgradeDomain(seedNodeInfo->ActualUpgradeDomainId))
        {
            AddCancelNode(seedNodeId);
        }

        if (seedNodeInfo->IsUp && !seedNodeInfo->IsPendingNodeClose(fm) && DateTime::Now() > seedNodeInfo->NodeUpTime + seedNodeWaitInterval)
        {
            upCount++;
			nodes += seedNodeId.ToString() + L" ";
        }
		else
		{
			nodes += seedNodeId.ToString() + L"* "; //marks nodes that are down with an asterisk
		}
    }

    if (!candidates.empty())
    {
		nodes.pop_back();
		nodes += L")";

        sort(candidates.begin(), candidates.end(), [](NodeInfoSPtr const& left, NodeInfoSPtr const& right) { return left->Id < right->Id; });

        int readyCount = upCount - minRequired;

        if (readyCount <= 0)
        {
            fm.WriteWarning(
                FailoverManager::TraceFabricUpgrade,
                "Upgrade blocked. Upgrading seed nodes can result in global lease loss: Candidates={0}, SeedNodes={1}, VoterCount={2}, EffectiveUpCount={3}",
                candidates, nodes, voterCount_, upCount);
        }

        for (size_t i = 0; i < candidates.size(); i++)
        {
            if (static_cast<int>(i) >= readyCount)
            {
                auto upgradeProgress = NodeUpgradeProgress(candidates[i]->Id, candidates[i]->NodeName, NodeUpgradePhase::PreUpgradeSafetyCheck, UpgradeSafetyCheckKind::EnsureSeedNodeQuorum);
                AddPendingNode(candidates[i]->Id, upgradeProgress);
            }
        }

        candidates.clear();
    }
}

void FabricUpgradeContext::Process(FailoverManager const& fm, FailoverUnit const& failoverUnit)
{
    if (failoverUnit.ServiceInfoObj->ServiceDescription.Type.IsSystemServiceType())
    {
        bool isSafeToUpgrade = true;
        if (IsReplicaSetCheckNeeded())
        {
            isSafeToUpgrade = failoverUnit.IsSafeToUpgrade(upgrade_.ClusterStartedTime);
            if (!isSafeToUpgrade)
            {
                TraceInfo(TraceTaskCodes::FM, FailoverManager::TraceFabricUpgrade, "It's not safe to upgrade FT: {0}", failoverUnit);
            }
        }

        isSafeToUpgrade_ &= isSafeToUpgrade;
    }

    for (auto it = failoverUnit.BeginIterator; it != failoverUnit.EndIterator; it++)
    {
        if (it->NodeInfoObj->ActualUpgradeDomainId == currentDomain_)
        {
            if (it->IsNodeUp &&
                (it->NodeInfoObj->IsPendingFabricUpgrade ||
                 it->NodeInfoObj->VersionInstance.Version != upgrade_.Description.Version ||
                 it->NodeInfoObj->VersionInstance.InstanceId != upgrade_.Description.InstanceId))
            {
                if (it->NodeInfoObj->IsPendingFabricUpgrade ||
                    it->NodeInfoObj->VersionInstance.Version == upgrade_.Description.Version)
                {
                    auto upgradeProgress = NodeUpgradeProgress(it->FederationNodeId, it->NodeInfoObj->NodeName, NodeUpgradePhase::Upgrading);
                    AddReadyNode(it->FederationNodeId, upgradeProgress, it->NodeInfoObj);
                }
                else
                {
                    ProcessReplica(failoverUnit, *it);
                }
            }
            else if (IsReplicaWaitNeeded(*it))
            {
                auto upgradeProgress = NodeUpgradeProgress(it->FederationNodeId, it->NodeInfoObj->NodeName, NodeUpgradePhase::PostUpgradeSafetyCheck, UpgradeSafetyCheckKind::WaitForPrimaryPlacement, failoverUnit.Id.Guid);
                if (AddWaitingNode(it->FederationNodeId, upgradeProgress))
                {
                    failoverUnit.TraceState();
                }
            }
        }
        else if (it->NodeInfoObj->IsPendingFabricUpgrade &&
                 !upgrade_.CheckUpgradeDomain(it->NodeInfoObj->ActualUpgradeDomainId))
        {
            AddCancelNode(it->FederationNodeId);
        }
    }

    ApplicationInfoSPtr applicationInfo = failoverUnit.ServiceInfoObj->ServiceType->Application;
    ProcessCandidates(fm, *applicationInfo, failoverUnit);
}

void FabricUpgradeContext::Merge(BackgroundThreadContext const& context)
{
    UpgradeContext::Merge(context);

    FabricUpgradeContext const& other = dynamic_cast<FabricUpgradeContext const&>(context);

    isSafeToUpgrade_ &= other.isSafeToUpgrade_;
}

void FabricUpgradeContext::Complete(FailoverManager & fm, bool isContextCompleted, bool isEnumerationAborted)
{
    fm.Events.FabricUpgradeInProgress(
        currentDomain_,
        isContextCompleted,
        UnprocessedFailoverUnitCount,
        readyNodes_.size(),
        pendingNodes_.size(),
        waitingNodes_.size(),
        cancelNodes_.size(),
        notReadyNodes_.size(),
        isSafeToUpgrade_);

    PruneReadyNodes();

    if (!isContextCompleted && UnprocessedFailoverUnitCount == 0u)
    {
        TraceNodes(fm, FailoverManager::TraceFabricUpgrade);
    }

    if (!isEnumerationAborted && UnprocessedFailoverUnitCount == 0u)
    {
        fm.FabricUpgradeManager.UpdateFabricUpgradeProgress(
            upgrade_,
            move(pendingNodes_),
            move(readyNodes_),
            move(waitingNodes_),
            isContextCompleted && isSafeToUpgrade_);
    }

    // Send CancelFabricUpgradeRequest messages(s) to cancel any previous upgrade
    for (NodeId nodeId : cancelNodes_)
    {
        NodeInfoSPtr nodeInfo = fm.NodeCacheObj.GetNode(nodeId);

        if (nodeInfo && nodeInfo->IsUp)
        {
            Transport::MessageUPtr message = RSMessage::GetCancelFabricUpgradeRequest().CreateMessage(upgrade_.Description);

            GenerationHeader header(fm.Generation, false);
            message->Headers.Add(header);

            fm.SendToNodeAsync(move(message), nodeInfo->NodeInstance);
        }
    }
}

bool FabricUpgradeContext::IsReplicaSetCheckNeeded() const
{
    if (voterCount_ < 3)
    {
        return false;
    }

    return upgrade_.IsReplicaSetCheckNeeded();
}

bool FabricUpgradeContext::IsReplicaWaitNeeded(Replica const& replica) const
{
    if (replica.FailoverUnitObj.IsToBeDeleted ||
        !IsReplicaSetCheckNeeded())
    {
        return false;
    }

    if (DateTime::Now() - upgrade_.CurrentDomainStartedTime < FailoverConfig::GetConfig().ExpectedNodeFabricUpgradeDuration)
    {
        return replica.IsPreferredPrimaryLocation;
    }

    return false;
}

bool FabricUpgradeContext::IsReplicaMoveNeeded(Replica const& replica) const
{
    if (DateTime::Now() - upgrade_.CurrentDomainStartedTime < FailoverConfig::GetConfig().SwapPrimaryRequestTimeout)
    {
        return replica.FailoverUnitObj.IsReplicaMoveNeededDuringUpgrade(replica);
    }

    return false;
}

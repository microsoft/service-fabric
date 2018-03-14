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

UpgradeContext::UpgradeContext(wstring const& contextId, wstring const& currentDomain)
    : BackgroundThreadContext(contextId),
      currentDomain_(currentDomain),
      notReadyNodes_()
{
}

void UpgradeContext::AddCancelNode(NodeId nodeId)
{
    cancelNodes_.insert(nodeId);
}

void UpgradeContext::AddReadyNode(NodeId nodeId, NodeUpgradeProgress const& upgradeProgress, NodeInfoSPtr const& nodeInfo)
{
    if (pendingNodes_.find(nodeId) != pendingNodes_.end() ||
        readyNodes_.find(nodeId) != readyNodes_.end())
    {
        return;
    }

    readyNodes_[nodeId] = make_pair(upgradeProgress, nodeInfo);
    waitingNodes_.erase(nodeId);
}

bool UpgradeContext::AddPendingNode(NodeId nodeId, NodeUpgradeProgress const& upgradeProgress)
{
    if (pendingNodes_.find(nodeId) != pendingNodes_.end())
    {
        return false;
    }

    pendingNodes_[nodeId] = upgradeProgress;
    readyNodes_.erase(nodeId);

    return true;
}

bool UpgradeContext::AddWaitingNode(NodeId nodeId, NodeUpgradeProgress const& upgradeProgress)
{
    if (readyNodes_.find(nodeId) != readyNodes_.end() ||
        waitingNodes_.find(nodeId) != waitingNodes_.end())
    {
        return false;
    }

    waitingNodes_[nodeId] = upgradeProgress;

    return true;
}

void UpgradeContext::PruneReadyNodes()
{
    if (FailoverConfig::GetConfig().MaxParallelNodeUpgradeCount <= 0)
    {
        return;
    }

    int count = FailoverConfig::GetConfig().MaxParallelNodeUpgradeCount - static_cast<int>(notReadyNodes_.size());

    if (count > 0)
    {
        if (count < readyNodes_.size())
        {
            auto it = readyNodes_.begin();
            for (int i = 0; i < count; i++)
            {
                it++;
            }

            readyNodes_.erase(it, readyNodes_.end());
        }
    }
    else
    {
        readyNodes_.clear();
    }
}

void UpgradeContext::Merge(BackgroundThreadContext const & context)
{
    UpgradeContext const & other = dynamic_cast<UpgradeContext const &>(context);

    for (auto it = other.cancelNodes_.begin(); it != other.cancelNodes_.end(); ++it)
    {
        AddCancelNode(*it);
    }

    for (auto it = other.readyNodes_.begin(); it != other.readyNodes_.end(); ++it)
    {
        AddReadyNode(it->first, it->second.first, it->second.second);
    }

    for (auto it = other.pendingNodes_.begin(); it != other.pendingNodes_.end(); ++it)
    {
        AddPendingNode(it->first, it->second);
    }

    for (auto it = other.waitingNodes_.begin(); it != other.waitingNodes_.end(); ++it)
    {
        AddWaitingNode(it->first, it->second);
    }
}

void UpgradeContext::ProcessReplica(FailoverUnit const & failoverUnit, Replica const& replica)
{
    if (replica.IsUp)
    {
        if (IsReplicaSetCheckNeeded())
        {
            if (IsReplicaMoveNeeded(replica))
            {
                auto kind = (failoverUnit.InBuildReplicaCount == 0 ? UpgradeSafetyCheckKind::WaitForPrimarySwap : UpgradeSafetyCheckKind::WaitForInBuildReplica);
                auto upgradeProgress = NodeUpgradeProgress(replica.FederationNodeId, replica.NodeInfoObj->NodeName, NodeUpgradePhase::PreUpgradeSafetyCheck, kind, failoverUnit.Id.Guid);
                if (AddPendingNode(replica.FederationNodeId, upgradeProgress))
                {
                    failoverUnit.TraceState();
                }
            }
            else
            {
                bool isQuorumCheckNeeded = false;
                UpgradeSafetyCheckKind::Enum kind;
                bool isSafeToCloseReplica = failoverUnit.IsSafeToCloseReplica(replica, isQuorumCheckNeeded, kind);

                if (isQuorumCheckNeeded)
                {
                    candidates_.push_back(pair<NodeId, wstring>(replica.FederationNodeId, replica.NodeInfoObj->NodeName));
                }
                else if (isSafeToCloseReplica)
                {
                    auto upgradeProgress = NodeUpgradeProgress(replica.FederationNodeId, replica.NodeInfoObj->NodeName, NodeUpgradePhase::Upgrading);
                    AddReadyNode(replica.FederationNodeId, upgradeProgress, replica.NodeInfoObj);
                }
                else
                {
                    auto upgradeProgress = NodeUpgradeProgress(replica.FederationNodeId, replica.NodeInfoObj->NodeName, NodeUpgradePhase::PreUpgradeSafetyCheck, kind, failoverUnit.Id.Guid);
                    if (AddPendingNode(replica.FederationNodeId, upgradeProgress))
                    {
                        failoverUnit.TraceState();
                    }
                }
            }
        }
        else
        {
            auto upgradeProgress = NodeUpgradeProgress(replica.FederationNodeId, replica.NodeInfoObj->NodeName, NodeUpgradePhase::Upgrading);
            AddReadyNode(replica.FederationNodeId, upgradeProgress, replica.NodeInfoObj);
        }
    }
    else
    {
        if (!replica.IsDropped)
        {
            auto upgradeProgress = NodeUpgradeProgress(replica.FederationNodeId, replica.NodeInfoObj->NodeName, NodeUpgradePhase::Upgrading);
            AddReadyNode(replica.FederationNodeId, upgradeProgress, replica.NodeInfoObj);
        }
        else if (IsReplicaWaitNeeded(replica))
        {
            auto upgradeProgress = NodeUpgradeProgress(replica.FederationNodeId, replica.NodeInfoObj->NodeName, NodeUpgradePhase::PostUpgradeSafetyCheck, UpgradeSafetyCheckKind::WaitForPrimaryPlacement, failoverUnit.Id.Guid);
            if (AddWaitingNode(replica.FederationNodeId, upgradeProgress))
            {
                failoverUnit.TraceState();
            }
        }
    }
}

void UpgradeContext::ProcessCandidates(FailoverManager const& fm, ApplicationInfo const& applicationInfo, FailoverUnit const& failoverUnit)
{
    if (!candidates_.empty())
    {
        int readyCount = failoverUnit.GetSafeReplicaCloseCount(fm, applicationInfo);

        sort(candidates_.begin(), candidates_.end(), [](pair<NodeId, wstring> left, pair<NodeId, wstring> right) { return left.first < right.first; });

        for (int i = 0; i < candidates_.size(); i++)
        {
            if (i < readyCount)
            {
                auto const& replica = *failoverUnit.GetReplica(candidates_[i].first);
                
                bool isQuorumCheckNeeded = false;
                UpgradeSafetyCheckKind::Enum kind;
                bool isSafeToCloseReplica = failoverUnit.IsSafeToCloseReplica(replica, isQuorumCheckNeeded, kind);

                if (isSafeToCloseReplica)
                {
                    auto upgradeProgress = NodeUpgradeProgress(candidates_[i].first, candidates_[i].second, NodeUpgradePhase::Upgrading);
                    AddReadyNode(candidates_[i].first, upgradeProgress, replica.NodeInfoObj);
                }
                else
                {
                    auto upgradeProgress = NodeUpgradeProgress(replica.FederationNodeId, replica.NodeInfoObj->NodeName, NodeUpgradePhase::PreUpgradeSafetyCheck, kind, failoverUnit.Id.Guid);
                    if (AddPendingNode(replica.FederationNodeId, upgradeProgress))
                    {
                        failoverUnit.TraceState();
                    }
                }
            }
            else
            {
                UpgradeSafetyCheckKind::Enum kind = (failoverUnit.IsStateful ? UpgradeSafetyCheckKind::EnsurePartitionQuorum : UpgradeSafetyCheckKind::EnsureAvailability);
                auto upgradeProgress = NodeUpgradeProgress(candidates_[i].first, candidates_[i].second, NodeUpgradePhase::PreUpgradeSafetyCheck, kind, failoverUnit.Id.Guid);
                if (AddPendingNode(candidates_[i].first, upgradeProgress))
                {
                    failoverUnit.TraceState();
                }
            }
        }

        candidates_.clear();
    }
}

bool UpgradeContext::ReadyToComplete()
{
    return (waitingNodes_.size() == 0 &&
            pendingNodes_.size() == 0 &&
            readyNodes_.size() == 0);
}

void UpgradeContext::TraceNodes(FailoverManager & fm, StringLiteral const& traceSource, wstring const& type) const
{
    for (auto it = readyNodes_.begin(); it != readyNodes_.end(); ++it)
    {
        fm.WriteInfo(traceSource, type, "{0}", it->second.first);
    }

    for (auto it = pendingNodes_.begin(); it != pendingNodes_.end(); ++it)
    {
        fm.WriteInfo(traceSource, type, "{0}", it->second);
    }

    for (auto it = waitingNodes_.begin(); it != waitingNodes_.end(); ++it)
    {
        fm.WriteInfo(traceSource, type, "{0}", it->second);
    }

    for (auto it = cancelNodes_.begin(); it != cancelNodes_.end(); ++it)
    {
        fm.WriteInfo(traceSource, type, "Cancelling upgrade on node {0}", *it);
    }

    if (!notReadyNodes_.empty())
    {
        fm.WriteInfo(traceSource, type, "Not ready nodes: {0}", notReadyNodes_);
    }
}

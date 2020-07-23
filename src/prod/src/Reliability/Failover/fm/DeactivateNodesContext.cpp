// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace std;

DeactivateNodesContext::DeactivateNodesContext(
    map<NodeId, NodeDeactivationIntent::Enum> const& nodesToDeactivate,
    wstring const& batchId)
    : nodesToDeactivate_(nodesToDeactivate),
      batchId_(batchId),
      nodesWithUpReplicas_(),
      failoverUnitId_(Guid::Empty()),
      UpgradeContext(L"DeactivateNodesContext." + batchId, L"")
{
}

BackgroundThreadContextUPtr DeactivateNodesContext::CreateNewContext() const
{
    return make_unique<DeactivateNodesContext>(nodesToDeactivate_, batchId_);
}

void DeactivateNodesContext::Merge(BackgroundThreadContext const& context)
{
    UpgradeContext::Merge(context);

    DeactivateNodesContext const& other = dynamic_cast<DeactivateNodesContext const&>(context);

    nodesWithUpReplicas_.insert(other.nodesWithUpReplicas_.begin(), other.nodesWithUpReplicas_.end());

    if (!other.nodesWithUpReplicas_.empty())
    {
        failoverUnitId_ = other.failoverUnitId_;
    }
}

bool DeactivateNodesContext::IsReplicaSetCheckNeeded() const
{
    return true;
}

bool DeactivateNodesContext::IsReplicaWaitNeeded(Replica const& replica) const
{
    UNREFERENCED_PARAMETER(replica);

    return false;
}

bool DeactivateNodesContext::IsReplicaMoveNeeded(Replica const& replica) const
{
    if (DateTime::Now() - replica.NodeInfoObj->DeactivationInfo.StartTime < FailoverConfig::GetConfig().SwapPrimaryRequestTimeout)
    {
        return replica.FailoverUnitObj.IsReplicaMoveNeededDuringDeactivateNode(replica);
    }

    return false;
}

bool DeactivateNodesContext::IsRemoveNodeOrDataReplicaWaitNeeded(NodeInfoSPtr const & nodeInfo) const
{
    return FailoverConfig::GetConfig().RemoveNodeOrDataUpReplicaTimeout == Common::TimeSpan::MaxValue || 
        DateTime::Now() - nodeInfo->DeactivationInfo.StartTime < FailoverConfig::GetConfig().RemoveNodeOrDataUpReplicaTimeout;
}

void DeactivateNodesContext::Process(FailoverManager const& fm, FailoverUnit const& failoverUnit)
{
    for (auto replica = failoverUnit.BeginIterator; replica != failoverUnit.EndIterator; ++replica)
    {
        if (nodesToDeactivate_.find(replica->FederationNodeId) != nodesToDeactivate_.end() &&
            replica->NodeInfoObj->DeactivationInfo.ContainsBatch(batchId_))
        {
            // this we need to do in practise only for stateful replicas
            // currently we will do it for stateless in case the config is not turned on
            // TODO: In the future make the config on by default and change this to nodesWithUpStatefulReplicas_
            if (failoverUnit.IsStateful || !FailoverConfig::GetConfig().RemoveNodeOrDataCloseStatelessInstanceAfterSafetyCheckComplete)
            {
                if (replica->IsUp)
                {
                    if (nodesWithUpReplicas_.empty())
                    {
                        failoverUnitId_ = failoverUnit.Id;
                    }

                    nodesWithUpReplicas_.insert(replica->NodeInfoObj->NodeInstance.Id);
                }
            }

            bool isRemoveNodeOrData = nodesToDeactivate_[replica->FederationNodeId] == NodeDeactivationIntent::RemoveData ||
                nodesToDeactivate_[replica->FederationNodeId] == NodeDeactivationIntent::RemoveNode;

            bool shouldAddToPendingDueToRemove = false;

            if (isRemoveNodeOrData)
            {
                // for stateful these checks are always done
                // for stateless we dont need them if the replicas are supposed to be closed after safety checks are complete
                if (failoverUnit.IsStateful || !FailoverConfig::GetConfig().RemoveNodeOrDataCloseStatelessInstanceAfterSafetyCheckComplete)
                {
                    //we should not proceed further in case it is not safe to remove this replica
                    //this is data loss check we always need to do
                    shouldAddToPendingDueToRemove = (replica->IsUp || replica->IsOffline) && !failoverUnit.IsSafeToRemove(batchId_);
                    if (!shouldAddToPendingDueToRemove)
                    {
                        //in case it is safe to remove it we will wait for RemoveNodeOrDataUpReplicaTimeout for an up replica to move out to a different node
                        //once that time is up we are ok with killing the replica
                        //but we first need to ensure that other safety checks are respected
                        shouldAddToPendingDueToRemove = replica->IsUp && IsRemoveNodeOrDataReplicaWaitNeeded(replica->NodeInfoObj);
                    }
                }
            }

            if (isRemoveNodeOrData && shouldAddToPendingDueToRemove)
            {
                auto upgradeProgress = NodeUpgradeProgress(replica->FederationNodeId, replica->NodeInfoObj->NodeName, NodeUpgradePhase::PreUpgradeSafetyCheck, UpgradeSafetyCheckKind::EnsurePartitionQuorum, failoverUnit.Id.Guid);
                if (AddPendingNode(replica->FederationNodeId, upgradeProgress))
                {
                    failoverUnit.TraceState();
                }
            }
            else if (replica->NodeInfoObj->IsPendingDeactivateNode)
            {
                auto upgradeProgress = NodeUpgradeProgress(replica->FederationNodeId, replica->NodeInfoObj->NodeName, NodeUpgradePhase::Upgrading);
                AddReadyNode(replica->FederationNodeId, upgradeProgress, replica->NodeInfoObj);
            }
            else
            {
                ProcessReplica(failoverUnit, *replica);
            }
        }
    }

    ApplicationInfoSPtr applicationInfo = failoverUnit.ServiceInfoObj->ServiceType->Application;
    ProcessCandidates(fm, *applicationInfo, failoverUnit);
}

bool DeactivateNodesContext::ReadyToComplete()
{
    bool isAllPause = true;
    bool nonPauseNodeWithUpReplicasExists = false;

    for (auto const& nodeToDeactivate : nodesToDeactivate_)
    {
        if (nodeToDeactivate.second != NodeDeactivationIntent::Pause)
        {
            isAllPause = false;
            if (nodesWithUpReplicas_.find(nodeToDeactivate.first) != nodesWithUpReplicas_.end())
            {
                nonPauseNodeWithUpReplicasExists = true;
                break;
            }
        }
    }

    if (isAllPause)
    {
        return (pendingNodes_.empty());
    }
    else
    {
        return (pendingNodes_.empty() && !nonPauseNodeWithUpReplicasExists);
    }
}

void DeactivateNodesContext::Initialize(FailoverManager & fm)
{
    for (auto const& nodeToDeactivate : nodesToDeactivate_)
    {
        if (nodeToDeactivate.second != NodeDeactivationIntent::Pause)
        {
            NodeInfoSPtr const& nodeInfo = fm.NodeCacheObj.GetNode(nodeToDeactivate.first);

            if (nodeInfo->IsUp &&
                nodeInfo->DeactivationInfo.IsSafetyCheckComplete)
            {
                auto upgradeProgress = NodeUpgradeProgress(nodeInfo->Id, nodeInfo->NodeName, NodeUpgradePhase::Upgrading);
                AddReadyNode(nodeInfo->Id, upgradeProgress, nodeInfo);
            }
        }
    }

    CheckSeedNodes(fm);
}

// TODO, MMohsin: Consider refactoring with FabricUpgradeContext
void DeactivateNodesContext::CheckSeedNodes(FailoverManager & fm)
{
    // Seed node can only start arbitration after some initital wait
    FederationConfig const & federationConfig = FederationConfig::GetConfig();
    TimeSpan seedNodeWaitInterval = TimeSpan::FromTicks((federationConfig.LeaseDuration + federationConfig.ArbitrationTimeout).Ticks * FailoverConfig::GetConfig().SeedNodeWaitSafetyFactor);

    vector<NodeId> seedNodeIds;
    int voterCount = fm.Federation.GetSeedNodes(seedNodeIds);

    int upCount = voterCount - static_cast<int>(seedNodeIds.size());
    int minRequired = voterCount / 2 + 1;
    vector<NodeInfoSPtr> candidates;
	wstring nodes = L"(";

    for (NodeId const& seedNodeId : seedNodeIds)
    {
        NodeInfoSPtr const& seedNodeInfo = fm.NodeCacheObj.GetNode(seedNodeId);

        if (!seedNodeInfo)
        {
            continue;
        }
        
        auto it = nodesToDeactivate_.find(seedNodeId);
        if (it != nodesToDeactivate_.end())
        {
            if (it->second == NodeDeactivationIntent::RemoveNode)
            {
                auto upgradeProgress = NodeUpgradeProgress(seedNodeInfo->Id, seedNodeInfo->NodeName, NodeUpgradePhase::PreUpgradeSafetyCheck, UpgradeSafetyCheckKind::EnsureSeedNodeQuorum);
                AddPendingNode(seedNodeInfo->Id, upgradeProgress);
            }
            else if (seedNodeInfo->IsUp && !seedNodeInfo->IsPendingNodeClose(fm))
            {
                candidates.push_back(seedNodeInfo);
            }
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
                fm.TraceDeactivateNode, batchId_,
                "Node deactivation blocked for batch {0}. Deactivating seed nodes can result in global lease loss: Candidates={1}, SeedNodes={2}, VoterCount={3}, EffectiveUpCount={4}",
                batchId_, candidates, nodes, voterCount, upCount);
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

void DeactivateNodesContext::Complete(FailoverManager & fm, bool isContextCompleted, bool isEnumerationAborted)
{
    if (!isContextCompleted)
    {
        fm.NodeEvents.DeactivateNodeStatus(
            batchId_,
            nodesWithUpReplicas_.size(),
            UnprocessedFailoverUnitCount,
            failoverUnitId_,
            isContextCompleted);

        TraceNodes(fm, FailoverManager::TraceDeactivateNode, batchId_);
    }

    if (isEnumerationAborted || UnprocessedFailoverUnitCount > 0u)
    {
        return;
    }

    for (auto const& nodeToDeactivate : nodesToDeactivate_)
    {
        NodeDeactivationStatus::Enum deactivationStatus = NodeDeactivationStatus::DeactivationSafetyCheckInProgress;

        if (isContextCompleted)
        {
            switch (nodeToDeactivate.second)
            {
            case NodeDeactivationIntent::Pause:
                deactivationStatus = NodeDeactivationStatus::DeactivationComplete;
                break;
            case NodeDeactivationIntent::Restart:
            case NodeDeactivationIntent::RemoveData:
            case NodeDeactivationIntent::RemoveNode:
                deactivationStatus = NodeDeactivationStatus::DeactivationSafetyCheckComplete;
                break;
            default:
                break;
            }
        }
        else if (pendingNodes_.find(nodeToDeactivate.first) == pendingNodes_.end())
        {
            switch (nodeToDeactivate.second)
            {
            case NodeDeactivationIntent::Pause:
                TESTASSERT_IF(pendingNodes_.empty(), "Pending node list is empty for node deactivation batch {0}", batchId_);
                deactivationStatus = NodeDeactivationStatus::DeactivationSafetyCheckComplete;
                break;
            case NodeDeactivationIntent::Restart:
                deactivationStatus = NodeDeactivationStatus::DeactivationSafetyCheckComplete;
                break;
            case NodeDeactivationIntent::RemoveData:
            case NodeDeactivationIntent::RemoveNode:
            {
                NodeInfoSPtr const& nodeInfo = fm.NodeCacheObj.GetNode(nodeToDeactivate.first);
                bool removeNodeReplicaWaitExpired = !IsRemoveNodeOrDataReplicaWaitNeeded(nodeInfo);
                if (nodesWithUpReplicas_.find(nodeToDeactivate.first) == nodesWithUpReplicas_.end() || removeNodeReplicaWaitExpired)
                {
                    deactivationStatus = NodeDeactivationStatus::DeactivationSafetyCheckComplete;
                }
                break;
            }
            default:
                break;
            }
        }

        ErrorCode error = fm.NodeCacheObj.UpdateNodeDeactivationStatus(
            nodeToDeactivate.first,
            nodeToDeactivate.second,
            deactivationStatus,
            move(pendingNodes_),
            move(readyNodes_),
            move(waitingNodes_));

        if (!error.IsSuccess())
        {
            fm.NodeEvents.DeactivateNodeFailure(nodeToDeactivate.first, error);
        }
    }

    if (pendingNodes_.empty())
    {
        for (auto const& node : readyNodes_)
        {
            NodeInfoSPtr const& nodeInfo = fm.NodeCacheObj.GetNode(node.first);

            if ((nodeInfo->DeactivationInfo.IsRestart || nodeInfo->DeactivationInfo.IsRemove) &&
                nodeInfo->DeactivationInfo.IsSafetyCheckComplete)
            {
                SendNodeDeactivateMessage(fm, *nodeInfo);
            }
        }
    }
}

void DeactivateNodesContext::SendNodeDeactivateMessage(FailoverManager & fm, NodeInfo const& nodeInfo) const
{
    fm.WriteInfo(
        fm.TraceDeactivateNode, batchId_,
        "Sending NodeDeactivateRequest message to node {0}", nodeInfo.Id);

    // Send NodeDeactivateRequest message to RA
    NodeDeactivateRequestMessageBody body(nodeInfo.DeactivationInfo.SequenceNumber, fm.IsMaster);
    Transport::MessageUPtr request = RSMessage::GetNodeDeactivateRequest().CreateMessage(move(body));
    GenerationHeader header(fm.Generation, fm.IsMaster);
    request->Headers.Add(header);

    fm.SendToNodeAsync(move(request), nodeInfo.NodeInstance);
}

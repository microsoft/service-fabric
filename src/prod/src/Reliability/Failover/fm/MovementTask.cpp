// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace Reliability::LoadBalancingComponent;

MovementTask::MovementTask(
    FailoverManager const& fm,
    INodeCache const& nodeCache,
    FailoverUnitMovement && movement,
    LoadBalancingComponent::DecisionToken && decisionToken)
    : fm_(fm), nodeCache_(nodeCache), movement_(move(movement)), decisionToken_(move(decisionToken))
{
}

vector<MovementTask::Action> MovementTask::GetActions() const
{
    vector<Action> actions;

    for (auto it = movement_.Actions.begin(); it != movement_.Actions.end(); ++it)
    {
        if (it->Action == FailoverUnitMovementType::AddPrimary)
        {
            // if there is an AddPrimary, make it the only action
            actions.clear();
            actions.push_back(make_pair(it->TargetNode, ActionType::AddPrimaryReplica));
            break;
        }
        else if (it->Action == FailoverUnitMovementType::AddSecondary)
        {
            auto actionType = ActionType::AddReplica;
            actions.push_back(make_pair(it->TargetNode, actionType));
        }
        else if (it->Action == FailoverUnitMovementType::AddInstance)
        {
            auto actionType = ActionType::AddInstance;
            actions.push_back(make_pair(it->TargetNode, actionType));
        }
        else if (it->Action == FailoverUnitMovementType::MoveSecondary ||
            it->Action == FailoverUnitMovementType::MoveInstance)
        {
            actions.push_back(make_pair(it->SourceNode, ActionType::MoveReplica));

            auto actionType = (it->Action == FailoverUnitMovementType::MoveSecondary ? ActionType::AddReplica : ActionType::AddInstance);
            actions.push_back(make_pair(it->TargetNode, actionType));
        }
        else if (it->Action == FailoverUnitMovementType::SwapPrimarySecondary ||
            it->Action == FailoverUnitMovementType::PromoteSecondary)
        {
            actions.push_back(make_pair(it->TargetNode, ActionType::PromoteReplica));
        }
        else if (it->Action == FailoverUnitMovementType::MovePrimary)
        {
            actions.push_back(make_pair(it->SourceNode, ActionType::MoveReplica));

            actions.push_back(make_pair(it->TargetNode, ActionType::AddReplicaAndPromote));
        }
        else if (it->Action == FailoverUnitMovementType::RequestedPlacementNotPossible)
        {
            actions.push_back(make_pair(it->SourceNode, ActionType::ClearFlags));
        }
        else if (it->Action == FailoverUnitMovementType::DropPrimary ||
            it->Action == FailoverUnitMovementType::DropSecondary ||
            it->Action == FailoverUnitMovementType::DropInstance)
        {
            actions.push_back(make_pair(it->SourceNode, ActionType::DropReplica));
        }
        else
        {
            Assert::CodingError("Unknown Replica Move Type");
        }
    }

    if (actions.size() <= 1)
    {
        return actions;
    }

    sort(actions.begin(), actions.end(), [](Action const& a1, Action const& a2) -> bool
    {
        if (a1.first != a2.first)
        {
            return a1.first < a2.first;
        }
        else
        {
            return a1.second < a2.second;
        }
    });

    size_t index = 0;

    while (actions.size() > 1 && index < actions.size() - 1)
    {
        size_t next = index;

        while (next < actions.size() && actions[next].first == actions[index].first)
        {
            ++next;
        }

        if (next - index == 1)
        {
            index = next;
            continue;
        }

        if (actions[index].second == ActionType::MoveReplica &&
            actions[index+1].second == ActionType::AddReplica)
        {
            actions.erase(actions.begin() + index, actions.begin() + next);
        }
        else if (actions[index].second == ActionType::MoveReplica &&
            actions[index+1].second == ActionType::AddInstance)
        {
            actions.erase(actions.begin() + index, actions.begin() + next);
        }
        else if (actions[index].second == ActionType::MoveReplica &&
            actions[index+1].second == ActionType::AddReplicaAndPromote)
        {
            actions[index].second = ActionType::PromoteReplica;
            actions.erase(actions.begin() + index + 1, actions.begin() + next);
            ++index;
        }
        else
        {
            actions.erase(actions.begin() + index + 1, actions.begin() + next);
            ++index;
        }
    }

    return actions;
}

vector<MovementTask::Action> MovementTask::GetActionsForEmptyFT() const
{
    vector<Action> actions;
    for (auto it = movement_.Actions.begin(); it != movement_.Actions.end(); ++it)
    {
        if (it->Action == FailoverUnitMovementType::AddSecondary || it->Action == FailoverUnitMovementType::AddInstance)
        {
            // Make it the only action
            actions.push_back(make_pair(it->TargetNode, ActionType::AddPrimaryReplica));
            break;
        }
    }

    return actions;
}

bool MovementTask::AddPrimary(LockedFailoverUnitPtr & failoverUnit, NodeId nodeId)
{
    if (!failoverUnit->CurrentConfiguration.IsEmpty)
    {
        bool reconfigurationNeeded = false;
        return AddReplica(failoverUnit, nodeId, true, reconfigurationNeeded);
    }

    Replica * existing = failoverUnit->GetReplica(nodeId);
    if (!existing || (existing->IsDeleted && !existing->IsInConfiguration))
    {
        NodeInfoSPtr nodeInfo = nodeCache_.GetNode(nodeId);
        if (!failoverUnit->HasPersistedState || nodeInfo->IsReplicaUploaded)
        {
            if (!nodeInfo->IsPendingNodeClose(fm_))
            {
                failoverUnit.EnableUpdate();

                auto replica = failoverUnit->CreateReplica(move(nodeInfo));
                failoverUnit->StartReconfiguration(true /*isPrimaryChange*/);
                failoverUnit->NoData = true;
                failoverUnit->UpdateReplicaCurrentConfigurationRole(replica, ReplicaRole::Primary);
                replica->IsPrimaryToBePlaced = false;
                return true;
            }
            else
            {
                fm_.PLB.OnDroppedPLBMovement(movement_.FailoverUnitId, PlbMovementIgnoredReasons::NodePendingClose, decisionToken_.DecisionId);
            }
        }
        else
        {
            fm_.PLB.OnDroppedPLBMovement(movement_.FailoverUnitId, PlbMovementIgnoredReasons::NodeIsUploadingReplicas, decisionToken_.DecisionId);
        }
    }
    else
    {
        fm_.PLB.OnDroppedPLBMovement(movement_.FailoverUnitId, PlbMovementIgnoredReasons::ReplicaNotDeleted, decisionToken_.DecisionId);
    }

    return false;
}

bool MovementTask::AddReplica(LockedFailoverUnitPtr & failoverUnit, NodeId nodeId, bool isToBePromoted, __out bool & reconfigurationNeeded)
{
    if (failoverUnit->CurrentConfiguration.IsEmpty)
    {
        fm_.PLB.OnDroppedPLBMovement(movement_.FailoverUnitId, PlbMovementIgnoredReasons::CurrentConfigurationEmpty, decisionToken_.DecisionId);
        return false;
    }

    if (isToBePromoted && failoverUnit->ToBePromotedReplicaExists)
    {
        fm_.PLB.OnDroppedPLBMovement(movement_.FailoverUnitId, PlbMovementIgnoredReasons::ToBePromotedReplicaAlreadyExists, decisionToken_.DecisionId);
        return false;
    }

    auto existing = failoverUnit->GetReplicaIterator(nodeId);
    if ((existing == failoverUnit->EndIterator) || existing->IsDeleted)
    {
        NodeInfoSPtr nodeInfo = nodeCache_.GetNode(nodeId);
        if (failoverUnit->HasPersistedState && !nodeInfo->IsReplicaUploaded)
        {
            fm_.PLB.OnDroppedPLBMovement(movement_.FailoverUnitId, PlbMovementIgnoredReasons::NodeIsUploadingReplicas, decisionToken_.DecisionId);
            return false;
        }
        else if (nodeInfo->IsPendingNodeClose(fm_))
        {
            fm_.PLB.OnDroppedPLBMovement(movement_.FailoverUnitId, PlbMovementIgnoredReasons::NodePendingClose, decisionToken_.DecisionId);
            return false;
        }

        ReplicaRole::Enum role = ReplicaRole::Idle;
        if ((existing != failoverUnit->EndIterator) && existing->IsInConfiguration)
        {
            if (failoverUnit->IsChangingConfiguration ||
                existing->CurrentConfigurationRole != ReplicaRole::Secondary)
            {
                fm_.PLB.OnDroppedPLBMovement(movement_.FailoverUnitId, PlbMovementIgnoredReasons::ReplicaConfigurationChanging, decisionToken_.DecisionId);
                return false;
            }

            failoverUnit.EnableUpdate();

            failoverUnit->UpdateReplicaCurrentConfigurationRole(existing, ReplicaRole::None);
            role = ReplicaRole::Secondary;
            reconfigurationNeeded = true;
        }

        failoverUnit.EnableUpdate();

        auto replica = failoverUnit->CreateReplica(move(nodeInfo));
        replica->IsToBePromoted = isToBePromoted;
        replica->IsPrimaryToBePlaced = false;
        replica->IsReplicaToBePlaced = false;

        if (isToBePromoted)
        {
            failoverUnit->Primary->IsPrimaryToBeSwappedOut = false;
        }

        failoverUnit->UpdateReplicaCurrentConfigurationRole(replica, role);

        return true;
    }
    else if (existing->IsUp)
    {
        if (isToBePromoted && existing->IsToBeDropped)
        {
            fm_.PLB.OnDroppedPLBMovement(movement_.FailoverUnitId, PlbMovementIgnoredReasons::ReplicaToBeDropped, decisionToken_.DecisionId);
            return false;
        }
        else if (existing->IsCurrentConfigurationPrimary)
        {
            failoverUnit.EnableUpdate();

            existing->IsToBeDroppedByPLB = false;
            return true;
        }
        else if (existing->IsCurrentConfigurationSecondary)
        {
            if (!isToBePromoted)
            {
                failoverUnit.EnableUpdate();

                existing->IsToBeDroppedByPLB = false;
                return true;
            }
            else if (!existing->IsToBeDropped)
            {
                failoverUnit.EnableUpdate();

                if (existing->IsStandBy)
                {
                    existing->ReplicaState = ReplicaStates::InBuild;
                }

                existing->IsToBeDroppedByPLB = false;
                existing->IsToBePromoted = true;
                failoverUnit->Primary->IsPrimaryToBeSwappedOut = false;
                return true;
            }
            else
            {
                fm_.PLB.OnDroppedPLBMovement(movement_.FailoverUnitId, PlbMovementIgnoredReasons::ReplicaToBeDropped, decisionToken_.DecisionId);
                return false;
            }
        }
        else if (existing->IsStandBy && !existing->IsInCurrentConfiguration)
        {
            failoverUnit.EnableUpdate();

            existing->IsToBeDroppedByPLB = false;
            existing->ReplicaState = ReplicaStates::InBuild;
            existing->IsToBePromoted = isToBePromoted;

            if (isToBePromoted)
            {
                failoverUnit->Primary->IsPrimaryToBeSwappedOut = false;
            }

            return true;
        }
        else
        {
            fm_.PLB.OnDroppedPLBMovement(movement_.FailoverUnitId, PlbMovementIgnoredReasons::InvalidReplicaState, decisionToken_.DecisionId);
            return false;
        }
    }
    else
    {
        fm_.PLB.OnDroppedPLBMovement(movement_.FailoverUnitId, PlbMovementIgnoredReasons::ReplicaNotDeleted, decisionToken_.DecisionId);
        return false;
    }
}

bool MovementTask::AddInstance(LockedFailoverUnitPtr & failoverUnit, NodeId nodeId)
{
    Replica const * existing = failoverUnit->GetReplica(nodeId);
    if (!existing || existing->IsDeleted)
    {
        NodeInfoSPtr nodeInfo = nodeCache_.GetNode(nodeId);
        if (!nodeInfo->IsPendingDeactivateNode)
        {
            failoverUnit.EnableUpdate();

            failoverUnit->CreateReplica(move(nodeInfo));
            return true;
        }
        else
        {
            fm_.PLB.OnDroppedPLBMovement(movement_.FailoverUnitId, PlbMovementIgnoredReasons::NodePendingDeactivate, decisionToken_.DecisionId);
        }
    }
    else
    {
        fm_.PLB.OnDroppedPLBMovement(movement_.FailoverUnitId, PlbMovementIgnoredReasons::ReplicaNotDeleted, decisionToken_.DecisionId);
    }

    return false;
}

bool MovementTask::DropReplica(LockedFailoverUnitPtr & failoverUnit, NodeId nodeId)
{
    failoverUnit.EnableUpdate();

    auto replica = failoverUnit->GetReplica(nodeId);
    replica->IsToBeDroppedByPLB = true;
    replica->IsMoveInProgress = false;
    replica->IsPreferredPrimaryLocation = false;

    return true;
}

bool MovementTask::PromoteReplica(LockedFailoverUnitPtr & failoverUnit, NodeId nodeId)
{
    if (failoverUnit->ToBePromotedReplicaExists)
    {
        fm_.PLB.OnDroppedPLBMovement(movement_.FailoverUnitId, PlbMovementIgnoredReasons::ToBePromotedReplicaAlreadyExists, decisionToken_.DecisionId);
        return false;
    }

    auto replica = failoverUnit->GetReplica(nodeId);

    if (!replica)
    {
        fm_.PLB.OnDroppedPLBMovement(movement_.FailoverUnitId, PlbMovementIgnoredReasons::ReplicaNotFound, decisionToken_.DecisionId);
        return false;
    }

    if (!replica->IsUp || replica->IsStandBy)
    {
        fm_.PLB.OnDroppedPLBMovement(movement_.FailoverUnitId, PlbMovementIgnoredReasons::InvalidReplicaState, decisionToken_.DecisionId);
        return false;
    }

    failoverUnit.EnableUpdate();

    replica->IsToBePromoted = true;
    replica->IsPrimaryToBePlaced = false;
    replica->IsReplicaToBePlaced = false;
    failoverUnit->Primary->IsPrimaryToBeSwappedOut = false;

    return true;
}

bool MovementTask::MoveReplica(LockedFailoverUnitPtr & failoverUnit, NodeId nodeId)
{
    failoverUnit.EnableUpdate();

    auto replica = failoverUnit->GetReplica(nodeId);
    replica->IsMoveInProgress = true;
    replica->IsPreferredPrimaryLocation = false;
    replica->IsPreferredReplicaLocation = false;

    return true;
}

bool MovementTask::ClearFlags(LockedFailoverUnitPtr & failoverUnit, NodeId nodeId)
{
    Replica * existing = failoverUnit->GetReplica(nodeId);

    if (!existing)
    {
        fm_.PLB.OnDroppedPLBMovement(movement_.FailoverUnitId, PlbMovementIgnoredReasons::ReplicaNotFound, decisionToken_.DecisionId);
        return false;
    }

    failoverUnit.EnableUpdate();

    existing->IsPrimaryToBeSwappedOut = false;
    existing->IsPrimaryToBePlaced = false;
    existing->IsReplicaToBePlaced = false;
    existing->IsPreferredPrimaryLocation = false;
    existing->IsPreferredReplicaLocation = false;
    existing->IsMoveInProgress = false;

    return true;
}

void MovementTask::CheckFailoverUnit(LockedFailoverUnitPtr & failoverUnit, vector<StateMachineActionUPtr> & actions)
{
    if (failoverUnit->IsToBeDeleted)
    {
        fm_.PLB.OnDroppedPLBMovement(movement_.FailoverUnitId, PlbMovementIgnoredReasons::FailoverUnitIsToBeDeleted, decisionToken_.DecisionId);
        return;
    }

    auto movementActions = GetActions();

    if (failoverUnit->CurrentConfiguration.IsEmpty &&
        failoverUnit->PreferredPrimaryLocationExists &&
        (movementActions.size() != 1 || movementActions.begin()->second != ActionType::AddPrimaryReplica))
    {
        movementActions = GetActionsForEmptyFT();
    }

    if (movementActions.empty())
    {
        fm_.PLB.OnDroppedPLBMovement(movement_.FailoverUnitId, PlbMovementIgnoredReasons::PrimaryNotFound, decisionToken_.DecisionId);
        return;
    }

    bool ret = false;
    bool reconfigurationNeeded = false;

    for (auto const& action : movementActions)
    {
        if (action.second == ActionType::AddInstance)
        {
            ret |= AddInstance(failoverUnit, action.first);
        }
        else if (action.second == ActionType::AddPrimaryReplica)
        {
            ret |= AddPrimary(failoverUnit, action.first);
        }
        else if (action.second == ActionType::AddReplica)
        {
            ret |= AddReplica(failoverUnit, action.first, false, reconfigurationNeeded);
        }
        else if (action.second == ActionType::AddReplicaAndPromote)
        {
            ret |= AddReplica(failoverUnit, action.first, true, reconfigurationNeeded);
        }
        else if (action.second == ActionType::DropReplica)
        {
            ret |= DropReplica(failoverUnit, action.first);
        }
        else if (action.second == ActionType::PromoteReplica)
        {
            ret |= PromoteReplica(failoverUnit, action.first);
        }
        else if (action.second == ActionType::ClearFlags)
        {
            ret |= ClearFlags(failoverUnit, action.first);
        }
        else if (action.second == ActionType::MoveReplica)
        {
            ret |= MoveReplica(failoverUnit, action.first);
        }
    }

    if (reconfigurationNeeded)
    {
        failoverUnit->StartReconfiguration(false);
    }

    if (ret)
    {
        // Copying this because the plb method takes a lock, and I don't want to block this push back.
        auto guid = movement_.FailoverUnitId;
        actions.push_back(make_unique<TraceMovementAction>(move(movement_), decisionToken_.DecisionId));

        // Call a method here that will remove this failoverunit from the plbdiagnostics table that keeps track of consecutive drops
        fm_.PLB.OnExecutePLBMovement(guid);
    }
}


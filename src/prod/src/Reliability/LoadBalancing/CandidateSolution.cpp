// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "CandidateSolution.h"
#include "Placement.h"
#include "PLBConfig.h"
#include "TempSolution.h"
#include "Searcher.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

CandidateSolution::CandidateSolution(
    Placement const * originalPlacement,
    vector<Movement> && creations,
    vector<Movement> && migrations,
    PLBSchedulerActionType::Enum currentSchedulerAction,
    SearchInsight&& searchInsight)
    : originalPlacement_(originalPlacement),
    creations_(move(creations)),
    migrations_(move(migrations)),
    nodePlacements_(&(originalPlacement->NodePlacements)),
    nodeMovingInPlacements_(&(originalPlacement->NodeMovingInPlacements)),
    partitionPlacements_(&(originalPlacement->PartitionPlacements)),
    inBuildCountsPerNode_(&(originalPlacement->InBuildCountsPerNode)),
    applicationPlacements_(&(originalPlacement->ApplicationPlacements)),
    applicationNodeCounts_(&(originalPlacement->ApplicationNodeCounts)),
    applicationTotalLoad_(&(originalPlacement->ApplicationsTotalLoads)),
    applicationNodeLoads_(&(originalPlacement->ApplicationNodeLoad)),
    applicationReservedLoads_(&(originalPlacement->ApplicationReservedLoads)),
    faultDomainStructures_(&(originalPlacement->FaultDomainStructures), true),
    upgradeDomainStructures_(&(originalPlacement->UpgradeDomainStructures), false),
    nodeChanges_(originalPlacement_->TotalMetricCount, 0, true), // Use all metrics
    nodeMovingInChanges_(originalPlacement_->TotalMetricCount, 0, true),    // Use all metrics
    moveCost_(0),
    validSwapCount_(0),
    validMoveCount_(0),
    validAddCount_(0),
    dynamicNodeLoads_(
        originalPlacement_->BalanceCheckerObj->Nodes,
        originalPlacement_->BalanceCheckerObj->LBDomains,
        originalPlacement_->Settings.NodesWithReservedLoadOverlap),
    score_(
        originalPlacement_->TotalMetricCount,
        originalPlacement_->LBDomains,
        originalPlacement_->TotalReplicaCount,
        originalPlacement_->BalanceCheckerObj->FaultDomainLoads,
        originalPlacement_->BalanceCheckerObj->UpgradeDomainLoads,
        originalPlacement_->BalanceCheckerObj->ExistDefragMetric,
        originalPlacement_->BalanceCheckerObj->ExistScopedDefragMetric,
        originalPlacement->Settings,
        &dynamicNodeLoads_),
    currentSchedulerAction_(currentSchedulerAction),
    servicePackagePlacements_(&(originalPlacement_->ServicePackagePlacements)),
    solutionSearchInsight_(move(searchInsight))
{
    for (size_t i = 0; i < MaxNumberOfCreationAndMigration; i++)
    {
        Movement const& movement = GetMovement(i);
        if (!movement.IsValid)
        {
            continue;
        }

        ModifyAll(Movement::Invalid, movement, i);
    }

    if (originalPlacement_->BalanceCheckerObj->ExistScopedDefragMetric)
    {
        score_.ResetDynamicNodeLoads();
    }

    // calculate scores
    score_.UpdateMetricScores(nodeChanges_);
    if (originalPlacement_->Settings.UseMoveCostReports)
    {
        score_.Calculate(originalPlacement_->Settings.SwapCost * validSwapCount_ + moveCost_);
    }
    else
    {
        score_.Calculate(static_cast<double>(validMoveCount_));
    }
}

CandidateSolution::CandidateSolution(CandidateSolution && other)
    : originalPlacement_(other.originalPlacement_),
    creations_(move(other.creations_)),
    migrations_(move(other.migrations_)),
    replicaMovementPositions_(move(other.replicaMovementPositions_)),
    nodePlacements_(move(other.nodePlacements_)),
    nodeMovingInPlacements_(move(other.nodeMovingInPlacements_)),
    partitionPlacements_(move(other.partitionPlacements_)),
    inBuildCountsPerNode_(move(other.inBuildCountsPerNode_)),
    applicationPlacements_(move(other.applicationPlacements_)),
    applicationNodeCounts_(move(other.applicationNodeCounts_)),
    applicationTotalLoad_(move(other.applicationTotalLoad_)),
    applicationNodeLoads_(move(other.applicationNodeLoads_)),
    applicationReservedLoads_(move(other.applicationReservedLoads_)),
    faultDomainStructures_(move(other.faultDomainStructures_)),
    upgradeDomainStructures_(move(other.upgradeDomainStructures_)),
    nodeChanges_(move(other.nodeChanges_)),
    nodeMovingInChanges_(move(other.nodeMovingInChanges_)),
    moveCost_(other.moveCost_),
    validSwapCount_(other.validSwapCount_),
    validMoveCount_(other.validMoveCount_),
    validAddCount_(other.validAddCount_),
    score_(move(other.score_)),
    currentSchedulerAction_(move(other.currentSchedulerAction_)),
    servicePackagePlacements_(move(other.servicePackagePlacements_)),
    dynamicNodeLoads_(move(other.dynamicNodeLoads_)),
    solutionSearchInsight_(move(other.solutionSearchInsight_))
{
    score_.DynamicNodeLoads = &dynamicNodeLoads_;
}

CandidateSolution& CandidateSolution::operator=(CandidateSolution && other)
{
    if (this != &other)
    {
        originalPlacement_ = other.originalPlacement_;
        creations_ = move(other.creations_);
        migrations_ = move(other.migrations_);
        replicaMovementPositions_ = move(other.replicaMovementPositions_);
        nodePlacements_ = move(other.nodePlacements_);
        nodeMovingInPlacements_ = move(other.nodeMovingInPlacements_);
        partitionPlacements_ = move(other.partitionPlacements_);
        inBuildCountsPerNode_ = move(other.inBuildCountsPerNode_);
        applicationPlacements_ = move(other.applicationPlacements_);
        applicationNodeCounts_ = move(other.applicationNodeCounts_);
        applicationTotalLoad_ = move(other.applicationTotalLoad_);
        applicationNodeLoads_ = move(other.applicationNodeLoads_);
        applicationReservedLoads_ = move(other.applicationReservedLoads_);
        faultDomainStructures_ = move(other.faultDomainStructures_);
        upgradeDomainStructures_ = move(other.upgradeDomainStructures_);
        nodeChanges_ = move(other.nodeChanges_);
        nodeMovingInChanges_ = move(other.nodeMovingInChanges_);
        moveCost_ = other.moveCost_;
        validSwapCount_ = other.validSwapCount_;
        validMoveCount_ = other.validMoveCount_;
        validAddCount_ = other.validAddCount_;
        score_ = move(other.score_);
        currentSchedulerAction_ = move(other.currentSchedulerAction_);
        servicePackagePlacements_ = move(other.servicePackagePlacements_);
        dynamicNodeLoads_ = other.dynamicNodeLoads_;
        score_.DynamicNodeLoads = &dynamicNodeLoads_;
        solutionSearchInsight_ = move(other.solutionSearchInsight_);
    }
    return *this;
}

CandidateSolution::CandidateSolution(CandidateSolution const& other)
    : originalPlacement_(other.originalPlacement_),
    creations_(other.creations_),
    migrations_(other.migrations_),
    replicaMovementPositions_(other.replicaMovementPositions_),
    nodePlacements_(other.nodePlacements_),
    nodeMovingInPlacements_(other.nodeMovingInPlacements_),
    partitionPlacements_(other.partitionPlacements_),
    inBuildCountsPerNode_(other.inBuildCountsPerNode_),
    applicationPlacements_(other.applicationPlacements_),
    applicationNodeCounts_(other.applicationNodeCounts_),
    applicationTotalLoad_(other.applicationTotalLoad_),
    applicationNodeLoads_(other.applicationNodeLoads_),
    applicationReservedLoads_(other.applicationReservedLoads_),
    faultDomainStructures_(other.faultDomainStructures_),
    upgradeDomainStructures_(other.upgradeDomainStructures_),
    nodeChanges_(other.nodeChanges_),
    nodeMovingInChanges_(other.nodeMovingInChanges_),
    moveCost_(other.moveCost_),
    validSwapCount_(other.validSwapCount_),
    validMoveCount_(other.validMoveCount_),
    validAddCount_(other.validAddCount_),
    score_(other.score_),
    currentSchedulerAction_(other.currentSchedulerAction_),
    servicePackagePlacements_(other.servicePackagePlacements_),
    dynamicNodeLoads_(other.dynamicNodeLoads_),
    solutionSearchInsight_(other.solutionSearchInsight_)
{
    score_.DynamicNodeLoads = &dynamicNodeLoads_;
}

Score CandidateSolution::TryChange(TempSolution const& tempSolution) const
{
    Score score(score_);

    // If PLB initiated move from node A to node B, node A load in NodeChanges will be decreased for moved-out replica load
    score.UpdateMetricScores(tempSolution.NodeChanges, nodeChanges_);
    if (originalPlacement_->Settings.UseMoveCostReports)
    {
        score.Calculate(originalPlacement_->Settings.SwapCost * tempSolution.ValidSwapCount + tempSolution.MoveCost);
    }
    else
    {
        score.Calculate(static_cast<double>(tempSolution.ValidMoveCount));
    }

    return score;
}

void CandidateSolution::UndoChange(TempSolution const& tempSolution) const
{
    Score score(score_);

    score.UndoMetricScoresForDynamicLoads(tempSolution.NodeChanges, nodeChanges_);
}

void CandidateSolution::ApplyChange(TempSolution const& tempSolution, Score && newScore)
{
    ApplyChange(tempSolution.MoveChanges);

    score_ = move(newScore);

    if (tempSolution.HasSearchInsight())
    {
        solutionSearchInsight_ = move(const_cast<TempSolution&>(tempSolution).SolutionSearchInsight);
    }
}

void CandidateSolution::ApplyChange(std::map<size_t, Movement> const& moveChanges)
{
    for (auto itTempMove = moveChanges.begin(); itTempMove != moveChanges.end(); ++itTempMove)
    {
        Movement & oldMove = GetMovement(itTempMove->first);

        ModifyAll(oldMove, Movement::Invalid, itTempMove->first);
    }

    for (auto itTempMove = moveChanges.begin(); itTempMove != moveChanges.end(); ++itTempMove)
    {
        Movement & oldMove = GetMovement(itTempMove->first);
        Movement const & newMove = itTempMove->second;

        ModifyAll(Movement::Invalid, newMove, itTempMove->first);

        oldMove = newMove;
    }
}

void CandidateSolution::ApplyChange(TempSolution const& tempSolution)
{
    ASSERT_IFNOT(this == &(tempSolution.BaseSolution), "Temp solution belongs to a different base solution");

    if (!tempSolution.IsEmpty)
    {
        Score newScore = TryChange(tempSolution);
        ApplyChange(tempSolution, move(newScore));
    }
}

void CandidateSolution::ApplyMoveChange(TempSolution const& tempSolution)
{
    ASSERT_IFNOT(this == &(tempSolution.BaseSolution), "Temp solution belongs to a different base solution");

    if (!tempSolution.IsEmpty)
    {
        std::map<size_t, Movement> moveChanges;
        for (auto itTempMove = tempSolution.MoveChanges.begin(); itTempMove != tempSolution.MoveChanges.end(); ++itTempMove)
        {
            if (!itTempMove->second.IsAdd)
            {
                moveChanges.insert(make_pair(itTempMove->first, itTempMove->second));
            }
        }

        ApplyChange(moveChanges);
    }
}

size_t CandidateSolution::GetReplicaMovementPosition(PlacementReplica const* replica) const
{
    auto it = replicaMovementPositions_.find(replica);
    if (it != replicaMovementPositions_.end())
    {
        return it->second;
    }
    else
    {
        return SIZE_MAX;
    }
}

NodeEntry const* CandidateSolution::GetReplicaLocation(PlacementReplica const* replica) const
{
    size_t position = GetReplicaMovementPosition(replica);

    if (position == SIZE_MAX)
    {
        return replica->Node;
    }

    Movement const& movement = GetMovement(position);

    if (!movement.IsValid)
    {
        return replica->Node;
    }

    if (movement.IsSwap)
    {
        if (replica == movement.SourceToBeDeletedReplica)
        {
            return movement.TargetNode;
        }
        else
        {
            ASSERT_IFNOT(replica == movement.SourceToBeAddedReplica, "Replica mismatch for a swap movement");
            return movement.SourceNode;
        }
    }
    else if (movement.IsMove || movement.IsAdd || movement.IsPromote)
    {
        return movement.TargetNode;
    }
    else if (movement.IsDrop)
    {
        return nullptr;
    }
    else
    {
        Assert::CodingError("Invalid movement type {0}", static_cast<int>(movement.MoveType));
    }
}

BoundedSet<PlacementReplica const*> CandidateSolution::GetUnplacedReplicas() const
{
    BoundedSet<PlacementReplica const*> unplacedReplicas(PLBConfig::GetConfig().MaxInvalidReplicasToTrace);

    for (size_t i = 0; i < originalPlacement_->NewReplicaCount && unplacedReplicas.CanAdd(); ++i)
    {
        PlacementReplica const* replica = originalPlacement_->SelectNewReplica(i);
        if (GetReplicaMovementPosition(replica) == SIZE_MAX)
        {
            unplacedReplicas.Add(replica);
        }
    }

    return unplacedReplicas;
}

////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////

void CandidateSolution::ModifyAll(Movement const & oldMove, Movement const & newMove, size_t index)
{
    ModifyReplicaMovementPositions(oldMove, newMove, index);
    nodePlacements_.ChangeMovement(oldMove, newMove);
    partitionPlacements_.ChangeMovement(oldMove, newMove);
    applicationPlacements_.ChangeMovement(oldMove, newMove);
    applicationTotalLoad_.ChangeMovement(oldMove, newMove);

    if (originalPlacement_->IsThrottlingConstraintNeeded)
    {
        inBuildCountsPerNode_.ChangeMovement(oldMove, newMove);
    }

    // ApplicationReservedLoads will make the change in node loads and in node counts.
    applicationReservedLoads_.ChangeMovement(oldMove, newMove, applicationNodeLoads_, applicationNodeCounts_);

    nodeChanges_.ChangeMovement(oldMove, newMove);

    if (originalPlacement_->Settings.PreventTransientOvercommit)
    {
        nodeMovingInPlacements_.ChangeMovingIn(oldMove, newMove);
        nodeMovingInChanges_.ChangeMovingIn(oldMove, newMove);
    }

    servicePackagePlacements_.ChangeMovement(oldMove, newMove, nodeChanges_, nodeMovingInChanges_);

    if (!originalPlacement_->BalanceCheckerObj->FaultDomainStructure.IsEmpty)
    {
        faultDomainStructures_.ChangeMovement(oldMove, newMove);
    }

    if (!originalPlacement_->BalanceCheckerObj->UpgradeDomainStructure.IsEmpty)
    {
        upgradeDomainStructures_.ChangeMovement(oldMove, newMove);
    }

    ModifyValidMoveCount(oldMove, newMove);
}

void CandidateSolution::ModifyReplicaMovementPositions(Movement const & oldMove, Movement const & newMove, size_t index)
{
    if (oldMove.IsValid)
    {
        oldMove.ForEachReplica([&](PlacementReplica const* r)
        {
            auto itReplicaMovement = replicaMovementPositions_.find(r);
            ASSERT_IF(itReplicaMovement == replicaMovementPositions_.end(), "Replica {0} in an old movement {1} should exist", *r, oldMove);

            replicaMovementPositions_.erase(itReplicaMovement);
        });
    }

    if (newMove.IsValid)
    {
        newMove.ForEachReplica([&](PlacementReplica const* r)
        {
            auto itReplicaMovement = replicaMovementPositions_.find(r);
            ASSERT_IFNOT(itReplicaMovement == replicaMovementPositions_.end(), "Replica {0} in a new movement {1} should not exist", *r, newMove);

            replicaMovementPositions_.insert(std::make_pair(r, index));
        });
    }
}

void CandidateSolution::ModifyValidMoveCount(Movement const & oldMove, Movement const & newMove)
{
    if (newMove.IsValid)
    {
        validMoveCount_++;
        if (newMove.IsMove)
        {
            // Add move cost of the new move to the sum.
            moveCost_ += newMove.TargetToBeAddedReplica->Partition->GetMoveCost(newMove.TargetToBeAddedReplica->Role);
        }
        else if (newMove.IsAdd)
        {
            validAddCount_++;
        }
        else if (newMove.IsSwap)
        {
            validSwapCount_++;
        }
    }

    if (oldMove.IsValid)
    {
        ASSERT_IF(validMoveCount_ == 0, "Solution doesn't have any valid movement when removing an old move");
        validMoveCount_--;
        if (oldMove.IsMove)
        {
            // If a move is canceled, decrease move cost accordingly
            ASSERT_IF(moveCost_ < oldMove.TargetToBeAddedReplica->Partition->GetMoveCost(oldMove.TargetToBeAddedReplica->Role),
                "Current move cost is smaller than then cost to be subtracted");
            moveCost_ -= oldMove.TargetToBeAddedReplica->Partition->GetMoveCost(oldMove.TargetToBeAddedReplica->Role);
        }
        else if (oldMove.IsAdd)
        {
            ASSERT_IF(validAddCount_ == 0, "Solution doesn't have any valid Add when removing an old Add");
            validAddCount_--;
        }
        else if (oldMove.IsSwap)
        {
            ASSERT_IF(validSwapCount_ == 0, "Current swap count is zero when removing valid swap");
            validSwapCount_--;
        }
    }
}


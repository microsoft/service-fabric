// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Checker.h"
#include "Placement.h"
#include "PLBConfig.h"
#include "TempSolution.h"
#include "ViolationList.h"

#include "DiagnosticSubspace.h"
#include "ReplicaExclusionConstraint.h"
#include "NodeBlockListConstraint.h"
#include "PreferredLocationConstraint.h"
#include "FaultDomainConstraint.h"
#include "AffinityConstraint.h"
#include "NodeCapacityConstraint.h"
#include "ScaleoutCountConstraint.h"
#include "ApplicationCapacityConstraint.h"
#include "ThrottlingConstraint.h"

#include "PLBDiagnostics.h"


using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::LoadBalancingComponent;

Checker::Checker(Placement const* pl, PLBEventSource const& trace, PLBDiagnosticsSPtr const& plbDiagnosticsSPtr)
    : affinityConstraintIndex_(-1),
    maxPriority_(-1),
    trace_(trace),
    plbDiagnosticsSPtr_(plbDiagnosticsSPtr),
    settings_(pl->Settings),
    batchIndex_(0),
    maxPriorityToUse_(-1),
    maxMovementSlots_(SIZE_MAX)
{
    PLBConfig const& config = PLBConfig::GetConfig();

    constraints_.push_back(make_unique<ReplicaExclusionStaticConstraint>(-1));
    staticConstraintIndexes_.push_back(constraints_.size() - 1);

    if (config.PlacementConstraintPriority >= 0)
    {
        constraints_.push_back(make_unique<NodeBlockListConstraint>(config.PlacementConstraintPriority));
        staticConstraintIndexes_.push_back(constraints_.size() - 1);
    }

    if (config.PreferredLocationConstraintPriority >= 0)
    {
        constraints_.push_back(make_unique<PreferredLocationConstraint>(config.PreferredLocationConstraintPriority));
        staticConstraintIndexes_.push_back(constraints_.size() - 1);
    }

    if (pl->IsThrottlingConstraintNeeded)
    {
        constraints_.push_back(make_unique<ThrottlingConstraint>(pl->ThrottlingConstraintPriority));
        if (0 == config.ThrottlingConstraintPriority)
        {
            // Limit the number of movement slots only if thrittling constraint is hard.
            maxMovementSlots_ = pl->GetThrottledMoveCount();
        }
    }

    if (config.FaultDomainConstraintPriority >= 0 && !pl->BalanceCheckerObj->FaultDomainStructure.IsEmpty)
    {
        constraints_.push_back(make_unique<FaultDomainConstraint>(IConstraint::FaultDomain, config.FaultDomainConstraintPriority));
    }

    bool allNodesHaveItsOwnUD = true;
    pl->BalanceCheckerObj->UpgradeDomainStructure.ForEachNodeInPreOrder([&](BalanceChecker::DomainTree::Node const & node)
    {
        if (node.Children.size() == 0)
        {
            if (node.Data.NodeCount > 1)
            {
                allNodesHaveItsOwnUD = false;
            }
        }
    });

    if (config.UpgradeDomainConstraintPriority >= 0 && !pl->BalanceCheckerObj->UpgradeDomainStructure.IsEmpty && !allNodesHaveItsOwnUD)
    {
        constraints_.push_back(make_unique<FaultDomainConstraint>(IConstraint::UpgradeDomain, config.UpgradeDomainConstraintPriority));
    }

    constraints_.push_back(make_unique<ReplicaExclusionDynamicConstraint>(-1));

    if (config.CapacityConstraintPriority >= 0)
    {
        constraints_.push_back(make_unique<NodeCapacityConstraint>(config.CapacityConstraintPriority));
    }

    if (config.ScaleoutCountConstraintPriority >= 0)
    {
        constraints_.push_back(make_unique<ScaleoutCountConstraint>(config.ScaleoutCountConstraintPriority));
    }

    if (config.ApplicationCapacityConstraintPriority >= 0)
    {
        constraints_.push_back(make_unique<ApplicationCapacityConstraint>(config.ApplicationCapacityConstraintPriority));
    }

    // Note : Affinity must be last constraint to be checked!
    // when placing parent replicas there are no restrictions, so it can violate other constraints
    if (config.AffinityConstraintPriority >= 0)
    {
        affinityConstraintIndex_ = static_cast<int>(constraints_.size());
        constraints_.push_back(make_unique<AffinityConstraint>(config.AffinityConstraintPriority));
    }

    for (size_t i = 0; i < constraints_.size(); ++i)
    {
        maxPriority_ = max(maxPriority_, constraints_[i]->Priority);
        constraintPriorities_.insert(make_pair(constraints_[i]->Type, constraints_[i]->Priority));
    }

}

void Checker::PlaceNewReplicas(
    TempSolution & solution,
    Random & random,
    int maxConstraintPriority,
    bool enableVerboseTracing /* = false */,
    bool relaxAffinity,
    bool modifySolutionWhenTracing /* = true */) const
{
    ASSERT_IF(!PLBConfig::GetConfig().MoveExistingReplicaForPlacement && !solution.IsEmpty, "Solution for placement should be empty");

    if (settings_.IsTestMode)
    {
        ViolationList intialViolations = CheckSolution(solution, maxConstraintPriority, true, random);
        InnerPlaceNewReplicas(solution, random, maxConstraintPriority, relaxAffinity);
        ViolationList changedViolations = CheckSolution(solution, maxConstraintPriority, true, random);
        ASSERT_IFNOT(changedViolations <= intialViolations, "Violation becomes worse");
    }
    else if (!enableVerboseTracing)
    {
        InnerPlaceNewReplicas(solution, random, maxConstraintPriority, relaxAffinity);
    }
    else
    {
        TraceInnerPlaceNewReplicas(solution, random, maxConstraintPriority, relaxAffinity, modifySolutionWhenTracing);
    }
}

void Checker::DropExtraReplicas(TempSolution & tempSolution, Common::Random & random, int maxConstraintPriority) const
{
    std::vector<ISubspaceUPtr> subspaces;
    for (auto it = constraints_.begin(); it != constraints_.end(); ++it)
    {
        if ((*it)->Priority > maxConstraintPriority)
        {
            continue;
        }
        ConstraintCheckResult result = (*it)->CheckSolutionAndGenerateSubspace(tempSolution, true, false, false, random);
        subspaces.push_back(move(result.Subspace));
    }

    Placement const* placement = tempSolution.OriginalPlacement;

    // Partitions are already ordered by from parent to child, hence we select starting from the back
    // In this way we will first drop child services, and after them parent services
    for (size_t i = 0; i < placement->PartitionCount; ++i)
    {
        PartitionEntry const& partition = placement->SelectPartition(placement->PartitionCount - 1 - i);
        // We want to allow drop of extra replica even if partition is in upgrade
        if (partition.NumberOfExtraReplicas == 0)
        {
            continue;
        }

        int count = partition.NumberOfExtraReplicas;

        CandidateSolution baseSolution = tempSolution.BaseSolution;

        while (count > 0)
        {
            PlacementReplica const* candidateReplica = nullptr;
            Score bestScore = baseSolution.TryChange(tempSolution);

            NodeSet candidateNodes(tempSolution.OriginalPlacement, false);
            GetSourceNodesForReplicaDrop(tempSolution, partition, subspaces, candidateNodes);

            partition.ForEachExistingReplica([&](PlacementReplica const* r)
            {
                NodeEntry const* node = tempSolution.GetReplicaLocation(r);
                if (nullptr == node || !candidateNodes.Check(node) || r->IsPrimary || r->IsToBeDropped)
                {
                    // Replica is primary, or has already been dropped, or its drop will violate some constraints
                    return;
                }

                size_t dropIndex = tempSolution.DropReplica(r, random, r->Node);
                Score newScore = baseSolution.TryChange(tempSolution);
                if (nullptr == candidateReplica || newScore.AvgStdDev < bestScore.AvgStdDev)
                {
                    candidateReplica = r;
                    bestScore = move(newScore);
                }

                baseSolution.UndoChange(tempSolution);
                tempSolution.CancelMovement(dropIndex);
            }, false, true);

            if (nullptr == candidateReplica)
            {
                break;
            }

            tempSolution.DropReplica(candidateReplica, random, candidateReplica->Node);
            count--;
        }
    }
}

bool Checker::CorrectSolution(
    TempSolution & solution,
    Random & random,
    int maxConstraintPriority,
    bool constraintFixLight,
    size_t currentTransition,
    size_t transitionsPerRound)
{
    ASSERT_IFNOT(solution.IsEmpty, "Solution for constraint check should be empty");

    bool ret;

    if (affinityConstraintIndex_ != -1)
    {
        AffinityConstraint& affinityConstraint = dynamic_cast<AffinityConstraint&>(*constraints_[affinityConstraintIndex_]);

        if (solution.OriginalPlacement->Settings.MoveParentToFixAffinityViolation || solution.OriginalPlacement->Settings.IsAffinityBidirectional)
        {
            if (currentTransition >= (1 - solution.OriginalPlacement->Settings.MoveParentToFixAffinityViolationTransitionPercentage) * transitionsPerRound)
            {
                affinityConstraint.AllowParentToMove = true;
            }
            else
            {
                affinityConstraint.AllowParentToMove = false;
            }
        }
    }

    if (settings_.IsTestMode)
    {
        ViolationList intialViolations = CheckSolution(solution, maxConstraintPriority, false, random);
        ret = InnerCorrectSolution(solution, random, maxConstraintPriority, constraintFixLight);

        if (ret)
        {
            ViolationList changedViolations = CheckSolution(solution, maxConstraintPriority, false, random);
            ASSERT_IFNOT(changedViolations <= intialViolations, "Violation becomes worse");
        }
    }
    else
    {
        ret = InnerCorrectSolution(solution, random, maxConstraintPriority, constraintFixLight);
    }

    if (affinityConstraintIndex_ != -1)
    {
        AffinityConstraint& affinityConstraint = dynamic_cast<AffinityConstraint&>(*constraints_[affinityConstraintIndex_]);
        affinityConstraint.AllowParentToMove = false;
    }
    return ret;
}

bool Checker::MoveSolutionRandomly(
    TempSolution & solution,
    bool swapOnly,
    bool useNodeLoadAsHeuristic,
    int maxConstraintPriority,
    bool useRestrictedDefrag,
    Common::Random & random,
    bool forPlacement,
    bool usePartialClosure) const
{
    ASSERT_IFNOT(solution.IsEmpty, "Solution for random move should be empty");

    bool ret;
    if (settings_.IsTestMode)
    {
        ViolationList intialViolations = CheckSolution(solution, maxConstraintPriority, false, random);
        ret = InnerMoveSolutionRandomly(solution, swapOnly, useNodeLoadAsHeuristic, maxConstraintPriority, useRestrictedDefrag, random, forPlacement, usePartialClosure);

        if (ret)
        {
            ViolationList changedViolations = CheckSolution(solution, maxConstraintPriority, false, random);
            ASSERT_IFNOT(changedViolations <= intialViolations, "Violation becomes worse");
        }
    }
    else
    {
        ret = InnerMoveSolutionRandomly(solution, swapOnly, useNodeLoadAsHeuristic, maxConstraintPriority, useRestrictedDefrag, random, forPlacement, usePartialClosure);
    }

    return ret;
}

int Checker::GetMaxPriority() const
{
    return (maxPriorityToUse_ >= 0) ? maxPriorityToUse_ : maxPriority_;
}

int Checker::GetPriority(IConstraint::Enum type) const
{
    auto it = constraintPriorities_.find(type);

    if (it != constraintPriorities_.end())
    {
        return it->second;
    }
    else
    {
        return INT_MAX;
    }
}

ViolationList Checker::CheckSolution(
    TempSolution const& tempSolution,
    int maxConstraintPriority,
    bool useNodeBufferCapacity,
    Random& random) const
{
    ViolationList resultViolations;

    for (auto it = constraints_.begin(); it != constraints_.end(); ++it)
    {
        if ((*it)->Priority > maxConstraintPriority)
        {
            continue;
        }

        IViolationUPtr violation = (*it)->GetViolations(tempSolution, false, false, useNodeBufferCapacity, random);

        if (!violation->IsEmpty())
        {
            resultViolations.AddViolations((*it)->Priority, (*it)->Type, move(violation));
        }
    }

    return resultViolations;
}

set<Guid> Checker::CheckSolutionForInvalidPartitions(
    TempSolution const& tempSolution,
    DiagnosticsOption option,
    __out std::vector<std::shared_ptr<IConstraintDiagnosticsData>> & constraintsDiagnosticsData,
    Random& random) const
{

    set<Guid> partitionList;

    for (auto it = constraints_.begin(); it != constraints_.end(); ++it)
    {
        std::shared_ptr<IConstraintDiagnosticsData> diagnosticsDataPtr = IConstraintDiagnosticsData::MakeSharedChildOfType((*it)->Type);
        diagnosticsDataPtr->plbNodesPtr_ = &(plbDiagnosticsSPtr_->PLBNodes);
        //In the polymorphic functions, if the passed pointer is null, don't do the diagnostics
        ConstraintCheckResult result = (*it)->CheckSolutionAndGenerateSubspace(tempSolution, false, false, false, random, diagnosticsDataPtr);

        if (diagnosticsDataPtr->changed_)
        {
            constraintsDiagnosticsData.push_back(diagnosticsDataPtr);
        }

        if (!result.IsValid)
        {
            for (auto itReplica = result.InvalidReplicas.begin(); itReplica != result.InvalidReplicas.end(); ++itReplica)
            {
                bool addToList = true;


                //Health and Diagnostics
                switch(option)
                {
                    case DiagnosticsOption::None:
                        break;

                    case DiagnosticsOption::ConstraintCheck:
                        plbDiagnosticsSPtr_->TrackConstraintViolation(*itReplica, (*it), diagnosticsDataPtr);
                        break;

                    case DiagnosticsOption::ConstraintFixUnsuccessful:
                        if (!plbDiagnosticsSPtr_->UpdateConstraintFixFailureLedger(*itReplica, (*it)))
                        {
                            addToList = false;
                        }
                        break;

                    default:
                        break;
                }

                if (addToList)
                {
                    partitionList.insert((*itReplica)->Partition->PartitionId);
                }

            }
        }
    }

    return partitionList;
}

bool Checker::CheckSolutionForTriggerMigrateReplicaValidity(TempSolution const& tempSolution, PlacementReplica const* candidateReplica, std::wstring& errMsg)
{
    errMsg = L"";
    Random random;

    for (auto it = constraints_.begin(); it != constraints_.end(); ++it)
    {
        //ReplicaExclusionDynamic will always be generated for swaps between primaries and secondaries, so ignore it, and preferred location isn't relevant here
        //We should only care about this seriously if the customer configs it to be a hard constraint
        if (((*it)->Type == IConstraint::ReplicaExclusionDynamic) || ((*it)->Priority > 0))
        {
            continue;
        }

        ConstraintCheckResult result = (*it)->CheckSolutionAndGenerateSubspace(tempSolution, false, false, false, random);

        if (!result.IsValid)
        {
            for (auto itReplica = result.InvalidReplicas.begin(); itReplica != result.InvalidReplicas.end(); ++itReplica)
            {
                if ((*itReplica)->Partition->PartitionId == candidateReplica->Partition->PartitionId)
                {
                    errMsg = StringUtility::ToWString((*it)->Type);
                    return false;
                }
                else if ((*it)->Type == IConstraint::Affinity)
                {
                    if (((*itReplica)->Partition->Service->DependedService) && ((*itReplica)->Partition->Service->DependedService->Name == candidateReplica->Partition->Service->Name))
                    {
                        errMsg = wformatString("Affinity Violation for Dependent Service {0} with PartitionID {1}", (*itReplica)->Partition->Service->Name, (*itReplica)->Partition->PartitionId);
                    }
                    else
                    {
                        errMsg = wformatString("Affinity Violation for Dependent Service {0}", (*itReplica)->Partition->Service->Name);
                    }
                    return false;
                }
            }
        }
    }

    return true;
}

///////////////////////////////////////////
// private functions
///////////////////////////////////////////


bool Checker::MoveSolution(
    TempSolution & tempSolution,
    Random & random,
    int maxConstraintPriority) const
{
    bool ret = true;
    vector<ISubspaceUPtr> subspaces;
    subspaces.reserve(constraints_.size());
    NodeSet candidateNodes(tempSolution.OriginalPlacement, false);

    for (auto itConstraint = constraints_.begin(); itConstraint != constraints_.end(); ++itConstraint)
    {
        if ((*itConstraint)->Priority > maxConstraintPriority)
        {
            continue;
        }

        ConstraintCheckResult result = (*itConstraint)->CheckSolutionAndGenerateSubspace(tempSolution, true, true, false, random);
        subspaces.push_back(move(result.Subspace));

        if (!result.IsValid)
        {
            for (auto itReplica = result.InvalidReplicas.begin(); itReplica != result.InvalidReplicas.end(); ++itReplica)
            {
                candidateNodes.SelectAll();
                for (auto itSubspace = subspaces.begin(); itSubspace != subspaces.end() && !candidateNodes.IsEmpty; ++itSubspace)
                {
                    (*itSubspace)->GetTargetNodes(tempSolution, *itReplica, candidateNodes, false);
                }

                NodeEntry const* targetNode = candidateNodes.SelectRandom(random);
                if (targetNode == nullptr || !tempSolution.MoveReplica(*itReplica, targetNode, random))
                {
                    ret = false;
                    break;
                }
            }
        }
    }

    return ret;
}

bool Checker::TraceMoveSolution(
    TempSolution & tempSolution,
    Common::Random & random) const
{
    bool ret = true;
    NodeSet candidateNodes(tempSolution.OriginalPlacement, false);
    NodeSet differenceNodes(tempSolution.OriginalPlacement, false);

    for (auto itReplica = plbDiagnosticsSPtr_->ConstraintDiagnosticsReplicas.begin(); itReplica != plbDiagnosticsSPtr_->ConstraintDiagnosticsReplicas.end(); ++itReplica)
    {
        //This loop is moved inside the itReplica loop because each constraint violation's constraint has a different constraint priority.
        std::vector<DiagnosticSubspace> subspaces;
        subspaces.reserve(constraints_.size());

        for (auto it = constraints_.begin(); it != constraints_.end(); ++it)
        {
            if ((*it)->Priority > itReplica->Priority)
            {
                subspaces.push_back(DiagnosticSubspace( (ISubspaceUPtr) nullptr, std::numeric_limits<size_t>::max(), L""));
                continue;
            }

            ConstraintCheckResult result = (*it)->CheckSolutionAndGenerateSubspace(tempSolution, true, true, false, random);
            subspaces.push_back(DiagnosticSubspace(move(result.Subspace), 0, L""));
        }

        candidateNodes.SelectAll();
        DiagnosticSubspace::RefreshSubspacesDiagnostics(subspaces);
        size_t totalNodeCount = candidateNodes.Count, last = candidateNodes.Count;
        bool const trackEliminatedNodes = plbDiagnosticsSPtr_->TrackConstraintFixEliminatedNodes(itReplica->Replica, itReplica->Constraint);
        NodeToConstraintDiagnosticsDataMapSPtr nodeToConstraintDiagnosticsDataMapSPtr = trackEliminatedNodes ? make_shared<NodeToConstraintDiagnosticsDataMap>() : nullptr;
        bool rxdBecauseofDroppedPLBMovements = false;

        for (auto it = subspaces.begin(); it != subspaces.end() && !candidateNodes.IsEmpty; ++it)
        {
            if (it->Count == std::numeric_limits<size_t>::max())
            {
                continue;
            }

            if (trackEliminatedNodes)
            {
                differenceNodes.Copy(candidateNodes);
            }

            last = candidateNodes.Count;
            (it->Subspace)->GetTargetNodes(tempSolution, itReplica->Replica, candidateNodes, false, nodeToConstraintDiagnosticsDataMapSPtr);
            it->Count  = last - candidateNodes.Count;

            if ((it->Subspace)->Type == IConstraint::ReplicaExclusionDynamic)
            {
                if ((it->Count != 0) && (it->Count != std::numeric_limits<size_t>::max()))
                {
                    rxdBecauseofDroppedPLBMovements = plbDiagnosticsSPtr_->IsPLBMovementBeingDropped(itReplica->Replica->Partition->PartitionId);
                }
            }

            if (trackEliminatedNodes)
            {
                //Keeping only the nodes that were eliminated
                differenceNodes.Filter([&] (NodeEntry const * n) -> bool { return !candidateNodes.Check(n);});

                if (it->Count > 0)
                {
                    it->NodeDetail = plbDiagnosticsSPtr_->NodeSetDetailString(differenceNodes, nodeToConstraintDiagnosticsDataMapSPtr);
                }
            }
        }

        NodeEntry const* targetNode = candidateNodes.SelectRandom(random);
        if (targetNode == nullptr || ( (targetNode->NodeId == itReplica->Replica->Node->NodeId) && candidateNodes.Count == 1) || !tempSolution.MoveReplica(itReplica->Replica, targetNode, random))
        {
            ret = false;

            std::wstring traceMessage;
            std::wstring nodeDetailMessage;

            PLBDiagnostics::GenerateUnplacedTraceandNodeDetailMessages(
                constraints_, subspaces, itReplica->Replica, totalNodeCount,
                traceMessage, nodeDetailMessage, trackEliminatedNodes, rxdBecauseofDroppedPLBMovements);

            //Chooses whether to trace in a detailed fashion or not. Throttle the detailed tracing because of size issues, especially for large clusters
            //Trace message is copied because it is moved and consumed in the TrackUnfixableConstraint method
            std::wstring traceCopy = Environment::NewLine + traceMessage;
            if (plbDiagnosticsSPtr_->TrackUnfixableConstraint(*itReplica, traceMessage, nodeDetailMessage))
            {
                trace_.UnfixableConstraintViolationDetails(itReplica->Replica->Partition->PartitionId, itReplica->Replica->Partition->Service->Name, StringUtility::ToWString(itReplica->Replica->Role), traceCopy, nodeDetailMessage);
            }
            else
            {
                trace_.UnfixableConstraintViolation(itReplica->Replica->Partition->PartitionId, itReplica->Replica->Partition->Service->Name, StringUtility::ToWString(itReplica->Replica->Role), traceCopy);
            }
        }
    }

    return ret;
}

std::vector<ISubspaceUPtr> Checker::GenerateStaticSubspacesForRandomMove(
    TempSolution & solution,
    Common::Random & random,
    int maxConstraintPriority) const
{
    std::vector<ISubspaceUPtr> subspaces;
    for (auto staticConstraintIndex : staticConstraintIndexes_)
    {
        if (constraints_[staticConstraintIndex]->Priority > maxConstraintPriority)
        {
            continue;
        }

        ConstraintCheckResult result = constraints_[staticConstraintIndex]->CheckSolutionAndGenerateSubspace(solution, true, true, false, random);
        subspaces.push_back(move(result.Subspace));
    }
    return subspaces;
}

bool Checker::CheckPrimaryToBePlacedUpgradePartition(
    PartitionEntry const& partition,
    TempSolution & tempSolution,
    std::vector<ISubspaceUPtr> const& subspaces,
    Random & random) const
{
    bool ret = false;
    NodeSet candidateNodes(tempSolution.OriginalPlacement, false);

    NodeEntry const* targetNode = partition.PrimaryUpgradeLocation;
    PlacementReplica const* primary = partition.PrimaryReplica;
    if (!primary || primary->Node == targetNode || primary->IsMoveInProgress)
    {
        // Primary was already swapped in, ignore the J flag
        // Or primary is marked as MoveInProgress, so wait for that replica to be placed on prefered location
        return true;
    }
    candidateNodes.Add(targetNode);
    if (partition.GetReplica(targetNode) != nullptr)
    {
        GetTargetNodesForReplicaSwapBack(tempSolution, &partition, subspaces, candidateNodes);
    }
    else
    {
        GetTargetNodesForPlacedReplica(tempSolution, primary, subspaces, candidateNodes, false, true);
    }
    // Check if target is in the solution
    if (candidateNodes.Check(targetNode))
    {
        PlacementReplica const* replicaOnTarget = partition.GetReplica(targetNode);
        // If already has the primary, move it to the preferred location; otherwise, place it.
        if (replicaOnTarget && replicaOnTarget->IsSecondary)
        {
            tempSolution.PromoteSecondary(&partition, random, targetNode);
        }
        else if (primary && primary->IsNew)
        {
            tempSolution.PlaceReplica(primary, targetNode);
        }
        else
        {
            // There is no secondary on target,
            // Add primary on target node if replica diff > 0 or ignore
            PlacementReplica * newReplica = partition.NewReplica();
            if (newReplica)
            {
                tempSolution.PlaceReplicaAndPromote(newReplica, targetNode);
            }

        }

        ret = true;
    }

    return ret;
}

void Checker::CheckSecondaryToBePlacedUpgradePartition(
    PartitionEntry const& partition,
    size_t newReplicaId,
    std::vector<NodeEntry const*>& remainingSecondaryUpgradeLocations,
    TempSolution & tempSolution,
    std::vector<ISubspaceUPtr> const& subspaces,
    Random & random) const
{
    UNREFERENCED_PARAMETER(random);

    PlacementReplica const* newReplica = partition.SelectNewReplica(newReplicaId);
    if (!newReplica)
    {
        // If there is no new replica, don't try to place it on a specific node
        remainingSecondaryUpgradeLocations.clear();
        return;
    }

    NodeSet candidateNodes(tempSolution.OriginalPlacement, true);
    GetTargetNodesForPlacedReplica(tempSolution, newReplica, subspaces, candidateNodes, false, true);

    bool placedOnPreferredLocation = false;
    int locationId = 0;
    for (auto it = remainingSecondaryUpgradeLocations.begin(); it != remainingSecondaryUpgradeLocations.end(); ++it)
    {
        if (candidateNodes.Check((*it)))
        {
            // The randomWalkPlacement might already place replica on one of the secondary upgrade location
            if (tempSolution.GetReplicaLocation(newReplica) != (*it))
            {
                tempSolution.PlaceReplica(newReplica, (*it));
            }

            placedOnPreferredLocation = true;
            break;
        }

        locationId++;
    }

    if (placedOnPreferredLocation)
    {
        remainingSecondaryUpgradeLocations.erase(remainingSecondaryUpgradeLocations.begin() + locationId);
    }
}

void Checker::CheckSwapPrimaryUpgradePartition(
    TempSolution & tempSolution,
    std::vector<PartitionEntry const*> const & partitions,
    Random & random,
    std::vector<PartitionEntry const*> & unplacedPartitions,
    bool relaxCapacity) const
{
    unplacedPartitions.clear();

    std::vector<ISubspaceUPtr> subspaces;
    for (auto it = constraints_.begin(); it != constraints_.end(); ++it)
    {
        if (relaxCapacity && ((*it)->Type == IConstraint::NodeCapacity || (*it)->Type == IConstraint::ApplicationCapacity))
        {
            continue;
        }

        ConstraintCheckResult result = (*it)->CheckSolutionAndGenerateSubspace(tempSolution, true, false, true, random);
        subspaces.push_back(move(result.Subspace));
    }

    Placement const* placement = tempSolution.OriginalPlacement;
    CandidateSolution baseSolution = tempSolution.BaseSolution;
    NodeSet candidateNodes(placement, false);

    std::set<ServiceEntry const*> checkedAffinityParentServices;

    bool foundSolution;
    for (size_t i = 0; i < partitions.size(); ++i)
    {
        foundSolution = false;
        PartitionEntry const* p = partitions[i];
        ASSERT_IFNOT(p->IsInUpgrade, "Invalid partition {0} for upgrade SwapPrimary", p);

        // Check align affinity if configured and
        // it is not primary swap out of singleton replica in upgrade,
        // and relaxed affinity is required singleton upgrade scenario
        if (settings_.CheckAlignedAffinityForUpgrade &&
            !(p->IsTargetOne && settings_.RelaxAlignAffinityConstraintDuringSingletonUpgrade))
        {
            size_t movementUpgradeIndex = p->UpgradeIndex + tempSolution.BaseSolution.MaxNumberOfCreation;
            Movement const& m = tempSolution.GetMove(movementUpgradeIndex);
            if (m.IsValid && m.IsSwap)
            {
                // Has been checked, skip
                continue;
            }

            ServiceEntry const* parentService = nullptr;
            if (!p->Service->DependentServices.empty())
            {
                parentService = p->Service;
            }
            else if (p->Service->DependedService)
            {
                parentService = p->Service->DependedService;
            }

            candidateNodes.SelectAll();
            if (parentService && !parentService->AffinityAssociatedPartitions().empty() &&
                checkedAffinityParentServices.find(parentService) == checkedAffinityParentServices.end())
            {
                checkedAffinityParentServices.insert(parentService);

                std::vector<PartitionEntry const*> const& associatedPartitions = parentService->AffinityAssociatedPartitions();

                // Do the PromoteSecondary check for this group of partitions
                for (auto it = subspaces.begin(); it != subspaces.end() && !candidateNodes.IsEmpty; ++it)
                {
                    (*it)->PromoteSecondaryForPartitions(tempSolution, associatedPartitions, candidateNodes);
                }

                NodeEntry const* node = (settings_.DummyPLBEnabled) ? candidateNodes.SelectHighestNodeID() : candidateNodes.SelectRandom(random);
                if (node != nullptr)
                {
                    tempSolution.PromoteSecondaryForPartitions(associatedPartitions, random, node);
                    foundSolution = true;

                    // Found a solution, continue with the next partition
                    continue;
                }
                else if (!relaxCapacity)
                {
                    // Didn't find a solution
                    unplacedPartitions.push_back(p);

                    // if did not find solution when the capacity is not relaxed, continue;
                    // This partition will be checked again with capacity relaxed.
                    continue;
                }
            }
        }

        candidateNodes.SelectAll();

        // The primary could be null when the primary is being deactivated
        if (p->GetExistingSecondaryCount() != 0)
        {
            bool sendVoid = false;

            for (auto it = subspaces.begin(); it != subspaces.end() && !candidateNodes.IsEmpty; ++it)
            {
                sendVoid = sendVoid || (*it)->PromoteSecondary(tempSolution, p, candidateNodes);
            }

            NodeEntry const* promoteNode = nullptr;
            if (settings_.DummyPLBEnabled)
            {
                promoteNode = candidateNodes.SelectHighestNodeID();
            }
            else
            {
                Score bestScore = baseSolution.TryChange(tempSolution);
                // Choose the best candiate based on score
                p->ForEachExistingReplica([&](PlacementReplica const* r)
                {
                    NodeEntry const* replicaNode = tempSolution.GetReplicaLocation(r);
                    if (nullptr == replicaNode || !candidateNodes.Check(replicaNode) || r->IsPrimary)
                    {
                        return;
                    }

                    size_t upgradeIndex = tempSolution.PromoteSecondary(p, random, replicaNode);
                    Score newScore = baseSolution.TryChange(tempSolution);
                    if (nullptr == promoteNode || newScore.AvgStdDev < bestScore.AvgStdDev)
                    {
                        promoteNode = replicaNode;
                        bestScore = move(newScore);
                    }
                    
                    baseSolution.UndoChange(tempSolution);
                    tempSolution.CancelMovement(upgradeIndex);
                }, false, true);
            }

            if (promoteNode != nullptr)
            {
                tempSolution.PromoteSecondary(p, random, promoteNode);
                foundSolution = true;
            }
            else if (sendVoid && p->IsTargetOne && placement->IsSingletonReplicaMoveAllowedDuringUpgrade)
            {
                // Swap is not possible to replacement replica due to some of the constraints - cancel the FM flags for primary replica
                tempSolution.AddVoidMovement(p, random, p->PrimarySwapOutLocation);
            }
        }
        // Allow singleton primary replica to be swapped out to "MoveInProgress" secondary
        // If it is not dropped by FM, an upgrade could be blocked, hence clear the V flag by sending void movement 
        // Scenario: (P/LI S/V), repDiff = 0
        else if (p->IsTargetOne &&
            p->ExistingReplicaCount == 2 &&
            p->SecondaryReplicaCount == 0 &&
            p->NewReplicaCount == 0)
        {
            PlacementReplica const* replica = p->SelectExistingReplica(0)->IsSecondary ? p->SelectExistingReplica(0) : p->SelectExistingReplica(1);
            if (replica->IsMoveInProgress && !replica->IsToBeDropped)
            {
                tempSolution.AddVoidMovement(p, random, replica->Node);
            }
        }
        else
        {
            // If there is no existing secondary, instead of generating VOID movment, 'I' flag should be just ignored;
            foundSolution = true;
        }

        if (foundSolution)
        {
            plbDiagnosticsSPtr_->TrackUpgradeSwap(p);
        }
        else
        {
            // Didn't find a solution
            unplacedPartitions.push_back(p);
        }
    }
}

void Checker::TraceCheckSwapPrimaryUpgradePartition(
    TempSolution & tempSolution,
    std::vector<PartitionEntry const*> const & partitions,
    Random & random,
    std::vector<PartitionEntry const*> & unplacedPartitions,
    bool relaxCapacity) const
{
    unplacedPartitions.clear();

    std::vector<DiagnosticSubspace> subspaces;
    auto const& config = PLBConfig::GetConfig();
    bool isDummyPLB = settings_.DummyPLBEnabled;

    for (auto it = constraints_.begin(); it != constraints_.end(); ++it)
    {
        if (relaxCapacity && ((*it)->Type == IConstraint::NodeCapacity || (*it)->Type == IConstraint::ApplicationCapacity))
        {
            subspaces.push_back(DiagnosticSubspace( (ISubspaceUPtr) nullptr, std::numeric_limits<size_t>::max(), L""));
            continue;
        }

        ConstraintCheckResult result = (*it)->CheckSolutionAndGenerateSubspace(tempSolution, true, false, true, random);
        subspaces.push_back(DiagnosticSubspace(move(result.Subspace), 0, L""));
    }

    Placement const* placement = tempSolution.OriginalPlacement;
    NodeSet candidateNodes(placement, false);
    NodeSet differenceNodes(placement, false);

    for (size_t i = 0; i < partitions.size(); ++i)
    {
        PartitionEntry const* p = partitions[i];
        ASSERT_IFNOT(p->IsInUpgrade, "Invalid partition {0} for upgrade SwapPrimary", p);

        candidateNodes.SelectAll();
        DiagnosticSubspace::RefreshSubspacesDiagnostics(subspaces);

        auto rex = find_if(subspaces.begin(), subspaces.end(), [&](DiagnosticSubspace & q) -> bool
        {
            return  (q.Subspace)->Type == IConstraint::ReplicaExclusionStatic;
        });

        ASSERT_IF(rex == subspaces.end(), "ReplicaExclusionStatic constraint not found.");

        (rex->Subspace)->PromoteSecondary(tempSolution, p, candidateNodes, ISubspace::Enum::OnlySecondaries);

        size_t totalNodeCount = candidateNodes.Count, last = candidateNodes.Count;
        bool const trackEliminatedNodes = plbDiagnosticsSPtr_->TrackUpgradeSwapEliminatedNodes(p);
        bool rxdBecauseofDroppedPLBMovements = false;

        // The primary could be null when the primary is being deactivated
        if (p->GetExistingSecondaryCount() != 0)
        {
            for (auto it = subspaces.begin(); it != subspaces.end() && !candidateNodes.IsEmpty; ++it)
            {
                if (it->Count == std::numeric_limits<size_t>::max())
                {
                    continue;
                }

                if (trackEliminatedNodes)
                {
                    differenceNodes.Copy(candidateNodes);
                }

                last = candidateNodes.Count;

                if ((it->Subspace)->Type == IConstraint::ReplicaExclusionStatic)
                {
                    (it->Subspace)->PromoteSecondary(tempSolution, p, candidateNodes, ISubspace::Enum::IsInTransition);
                }
                else if ((it->Subspace)->Type == IConstraint::PlacementConstraint)
                {
                    if (config.PlacementConstraintPriority >= 0)
                    {
                        (it->Subspace)->PromoteSecondary(tempSolution, p, candidateNodes, ISubspace::Enum::NodeBlockList);
                    }
                    else
                    {
                        (it->Subspace)->PromoteSecondary(tempSolution, p, candidateNodes);
                    }
                }
                else
                {
                    (it->Subspace)->PromoteSecondary(tempSolution, p, candidateNodes);
                }


                it->Count  = last - candidateNodes.Count;

                if ((it->Subspace)->Type == IConstraint::ReplicaExclusionDynamic)
                {
                    if ((it->Count != 0) && (it->Count != std::numeric_limits<size_t>::max()))
                    {
                        rxdBecauseofDroppedPLBMovements = plbDiagnosticsSPtr_->IsPLBMovementBeingDropped(p->PartitionId);
                    }
                }

                if (trackEliminatedNodes)
                {
                    //Keeping only the nodes that were eliminated
                    differenceNodes.Filter([&] (NodeEntry const * n) -> bool { return !candidateNodes.Check(n);});

                    if (it->Count > 0)
                    {
                        it->NodeDetail = plbDiagnosticsSPtr_->NodeSetDetailString(differenceNodes);
                    }
                }
            }

            NodeEntry const* node = (isDummyPLB) ? candidateNodes.SelectHighestNodeID() : candidateNodes.SelectRandom(random);

            if (node == nullptr)
            {
                std::wstring traceMessage;
                std::wstring nodeDetailMessage;

                PLBDiagnostics::GenerateUnswappedUpgradeTraceMessage(constraints_, subspaces, p, totalNodeCount, traceMessage, nodeDetailMessage, trackEliminatedNodes, rxdBecauseofDroppedPLBMovements);

                //Chooses whether to trace in a detailed fashion or not. Throttle the detailed tracing because of size issues, especially for large clusters
                //Trace message is copied because it is moved and consumed in the TrackUnplacedReplica method
                std::wstring traceCopy = Environment::NewLine + traceMessage;
                if (plbDiagnosticsSPtr_->TrackUpgradeUnswappedPartition(p, traceMessage, nodeDetailMessage))
                {
                    trace_.UnswappableUpgradePartitionDetails(p->PartitionId, p->Service->Name, traceCopy, nodeDetailMessage);
                }
                else
                {
                    trace_.UnswappableUpgradePartition(p->PartitionId, p->Service->Name,  traceCopy);
                }
                // Didn't find a solution
                unplacedPartitions.push_back(p);
            }
        }
    }
}

void Checker::CheckUpgradePartitionsToBePlaced(
    TempSolution & tempSolution,
    std::vector<PartitionEntry const*> const & partitionsToBePlaced,
    Random & random,
    int currentConstraintPriority,
    std::vector<PartitionEntry const*> & unplacedPartitions) const
{
    unplacedPartitions.clear();
    // The replicas are already ordered so that the replicas that are affinitized to other replicas will be processed later
    std::vector<ISubspaceUPtr> subspaces;
    for (auto it = constraints_.begin(); it != constraints_.end(); ++it)
    {
        // Don't include the dynamic replica exclustion since we need a swap in
        if ((*it)->Priority > currentConstraintPriority || (*it)->Type == IConstraint::ReplicaExclusionDynamic)
        {
            continue;
        }
        ConstraintCheckResult result = (*it)->CheckSolutionAndGenerateSubspace(tempSolution, true, false, true, random);
        subspaces.push_back(move(result.Subspace));
    }

    bool foundSolution;
    for (size_t i = 0; i < partitionsToBePlaced.size(); ++i)
    {
        foundSolution = false;
        PartitionEntry const* partition = partitionsToBePlaced[i];
        ASSERT_IFNOT(partition->IsInUpgrade, "Invalid partition {0} for upgrade to be placed", *partition);
        ASSERT_IF(!partition->ExistsUpgradeLocation, "Partition {0} is not ToBePlaced", *partition);

        // For replica to be placed in upgrade, it is either IsPrimaryToBePlaced or IsReplicaToBePlaced
        if (partition->PrimaryUpgradeLocation != nullptr)
        {
            if (!CheckPrimaryToBePlacedUpgradePartition(*partition, tempSolution, subspaces, random))
            {
                // Didn't place primary replica on the same location where it was before upgrade
                unplacedPartitions.push_back(partition);
                if (0 == currentConstraintPriority)
                {
                    // Couldn't find a solution in the last try (priority == 0), cancel the FM flags for primary replica
                    tempSolution.AddVoidMovement(partition, random, partition->PrimaryUpgradeLocation);
                }
            }
        }
        else
        {
            std::vector<NodeEntry const*> remainingSecondaryUpgradeLocations(partition->SecondaryUpgradeLocations);

            for (size_t newReplicaId = 0; newReplicaId < partition->SecondaryUpgradeLocations.size(); newReplicaId++)
            {
                CheckSecondaryToBePlacedUpgradePartition(
                    *partition,
                    newReplicaId,
                    remainingSecondaryUpgradeLocations,
                    tempSolution,
                    subspaces,
                    random);

                if (remainingSecondaryUpgradeLocations.empty())
                {
                    break;
                }
            }

            if (!remainingSecondaryUpgradeLocations.empty())
            {
                unplacedPartitions.push_back(partition);

                if (0 == currentConstraintPriority)
                {
                    // Couldn't find the solution to place all missing secondary replicas on the previous location
                    // As this was last try (priority == 0), cancel the FM flags for secondary replicas that were not placed
                    for (auto it = remainingSecondaryUpgradeLocations.begin(); it != remainingSecondaryUpgradeLocations.end(); ++it)
                    {
                        tempSolution.AddVoidMovement(partition, random, (*it));
                    }
                }
            }
        }
    }
}

std::vector<Federation::NodeId> Checker::NodesAvailableForTriggerRandomMove(TempSolution const& tempSolution, PlacementReplica const* candidateReplica, Random & random, bool force)
{
    std::vector<ISubspaceUPtr> subspaces;
    std::vector<Federation::NodeId> candidateNodeIds;

    for (auto it = constraints_.begin(); it != constraints_.end(); ++it)
    {
        if (force)
        {
            if ((*it)->Type != IConstraint::ReplicaExclusionStatic)
            {
                continue;
            }
        }
        else
        {
            if (((*it)->Priority > 0) || ((*it)->Type == IConstraint::ReplicaExclusionDynamic))
            {
                continue;
            }
        }

        ConstraintCheckResult result = (*it)->CheckSolutionAndGenerateSubspace(tempSolution, false, false, false, random);
        subspaces.push_back(move(result.Subspace));
    }

    Placement const* placement = tempSolution.OriginalPlacement;
    NodeSet candidateNodes(placement, false);

    candidateNodes.SelectAll();

    for (auto it = subspaces.begin(); it != subspaces.end() && !candidateNodes.IsEmpty; ++it)
    {
        (*it)->GetTargetNodes(tempSolution, candidateReplica, candidateNodes, true);
        trace_.MoveReplicaOrInstance(wformatString("After {0} there are {1} nodes left.", (*it)->Type, candidateNodes.Count));
    }

    candidateNodeIds.reserve(candidateNodes.Count);
    candidateNodes.ForEach([&candidateNodeIds](NodeEntry const * eachOne) -> bool { candidateNodeIds.push_back(eachOne->NodeId); return true;} );

    return candidateNodeIds;
}

void Checker::GetPartitionStartIndexes(Placement const* placement, size_t & startIndex, size_t & endIndex) const
{
    std::vector<size_t> const& batchIndexVec = placement->PartitionBatchIndexVec;

    if (batchIndexVec.empty())
    {
        return;
    }

    if (batchIndex_ > 0)
    {
        startIndex = batchIndexVec[batchIndex_ - 1];
    }

    if (batchIndex_ < batchIndexVec.size())
    {
        endIndex = batchIndexVec[batchIndex_];
    }
}

void Checker::InnerPlaceNewReplicas(TempSolution & solution, Random & random, int maxConstraintPriority, bool relaxAffinity) const
{
    /*Important: Any changes to this method should also be mirrored in TraceInnerPlaceNewReplicas*/

    // initiate target nodes for each new replicas one by one, if some replica cannot find target node, skip that replica
    // the replicas are already ordered so that the replicas that are affinitized to other replicas will be processed later
    std::vector<ISubspaceUPtr> subspaces;
    bool isDummyPLB = settings_.DummyPLBEnabled;

    for (auto it = constraints_.begin(); it != constraints_.end(); ++it)
    {
        if ((*it)->Priority > maxConstraintPriority || (relaxAffinity && (*it)->Type == IConstraint::Affinity))
        {
            continue;
        }
        ConstraintCheckResult result = (*it)->CheckSolutionAndGenerateSubspace(solution, true, false, true, random);
        subspaces.push_back(move(result.Subspace));
    }

    // TODO: call GetTargetNodeForUnplacedReplica directly for each constraint
    Placement const* placement = solution.OriginalPlacement;
    NodeSet candidateNodes(placement, false);
    std::set<PlacementReplica const*> processedParentReplicas;
    std::set<ApplicationEntry const*> processedScaleoutOneApplications;

    size_t partitionStartIdx(0);
    size_t partitionEndIdx(placement->PartitionCount);
    GetPartitionStartIndexes(placement, partitionStartIdx, partitionEndIdx);

    for (size_t i = partitionStartIdx; i < partitionEndIdx; ++i)
    {
        // the partitions are already ordered by its parent/child level, so parent are always before child
        PartitionEntry const& partition = placement->SelectPartition(i);
        std::vector<size_t> newReplicaIndex;

        if (partition.NewReplicaCount == 0)
        {
            continue;
        }

        // If partition is in singleton replica upgrade and specific optimization should be performed
        if ((!relaxAffinity && partition.IsInCheckAffinityOptimization()) ||
            partition.IsInRelaxedScaleoutOptimization())
        {
            SingletonReplicaUpgrade(
                partition,
                processedParentReplicas,
                processedScaleoutOneApplications,
                solution,
                candidateNodes,
                nullptr,
                &subspaces,
                nullptr,
                newReplicaIndex,
                random,
                maxConstraintPriority,
                isDummyPLB,
                false);
        }
        else
        {
            partition.ForEachNewReplica([&](PlacementReplica const* r)
            {
                if (maxPriorityToUse_ >= 0 && solution.GetReplicaLocation(r) != nullptr)
                {
                    // NewReplicaPlacement run with relaxed after regular run
                    return;
                }

                candidateNodes.SelectAll();

                for (auto it = subspaces.begin(); it != subspaces.end() && !candidateNodes.IsEmpty; ++it)
                {
                    (*it)->GetTargetNodes(solution, r, candidateNodes, true);
                }

                NodeEntry const* node = ChooseNodeForPlacement(placement, r, candidateNodes, random, isDummyPLB);

                if (node != nullptr)
                {
                    solution.PlaceReplica(r, node);
                }
            });

            CorrectNonPartiallyPlacement(solution, partition);
        }
    }
}

void Checker::TraceInnerPlaceNewReplicas(
    TempSolution & solution,
    Random & random,
    int maxConstraintPriority,
    bool relaxAffinity /*= false*/,
    bool modifySolutionWhenTracing /* = true */) const
{
    /*Important: This method should mirror any changes in InnerPlaceNewReplicas*/

    // initiate target nodes for each new replicas one by one, if some replica cannot find target node, skip that replica
    // the replicas are already ordered so that the replicas that are affinitized to other replicas will be processed later
    std::vector<DiagnosticSubspace> subspaces;
    bool isDummyPLB = settings_.DummyPLBEnabled;

    for (auto it = constraints_.begin(); it != constraints_.end(); ++it)
    {
        if ((*it)->Priority > maxConstraintPriority || (relaxAffinity && (*it)->Type == IConstraint::Affinity))
        {
            subspaces.push_back(DiagnosticSubspace( (ISubspaceUPtr) nullptr, std::numeric_limits<size_t>::max(), L""));
            continue;
        }
        ConstraintCheckResult result = (*it)->CheckSolutionAndGenerateSubspace(solution, true, false, true, random);
        subspaces.push_back(DiagnosticSubspace(move(result.Subspace), 0, L""));
    }

    // TODO: call GetTargetNodeForUnplacedReplica directly for each constraint
    Placement const* placement = solution.OriginalPlacement;
    NodeSet candidateNodes(placement, false);
    NodeSet differenceNodes(placement, false);
    std::set<PlacementReplica const*> processedParentReplicas;
    std::set<ApplicationEntry const*> processedScaleoutOneApplications;

    auto& tracePtr = trace_; //used for the lambda expression

    size_t partitionStartIdx(0);
    size_t partitionEndIdx(placement->PartitionCount);
    GetPartitionStartIndexes(placement, partitionStartIdx, partitionEndIdx);

    for (size_t i = partitionStartIdx; i < partitionEndIdx; ++i)
    {
        // the partitions are already ordered by its parent/child level, so parent are always before child
        PartitionEntry const& partition = placement->SelectPartition(i);
        std::vector<size_t> newReplicaIndex;

        if (partition.NewReplicaCount == 0)
        {
            continue;
        }

        // If partition is in singleton replica upgrade and specific optimization should be performed
        if ((!relaxAffinity && partition.IsInCheckAffinityOptimization()) ||
            partition.IsInRelaxedScaleoutOptimization())
        {
            SingletonReplicaUpgrade(
                partition,
                processedParentReplicas,
                processedScaleoutOneApplications,
                solution,
                candidateNodes,
                &differenceNodes,
                nullptr,
                &subspaces,
                newReplicaIndex,
                random,
                maxConstraintPriority,
                isDummyPLB,
                true,
                modifySolutionWhenTracing);
        }
        else
        {
            partition.ForEachNewReplica([&](PlacementReplica const* r)
            {
                candidateNodes.SelectAll();
                DiagnosticSubspace::RefreshSubspacesDiagnostics(subspaces);
                size_t totalNodeCount = candidateNodes.Count;
                size_t last = candidateNodes.Count;
                bool const trackEliminatedNodes = plbDiagnosticsSPtr_->TrackEliminatedNodes(r);
                NodeToConstraintDiagnosticsDataMapSPtr nodeToConstraintDiagnosticsDataMapSPtr = trackEliminatedNodes ? make_shared<NodeToConstraintDiagnosticsDataMap>() : nullptr;
                bool rxdBecauseofDroppedPLBMovements = false;

                for (auto it = subspaces.begin(); it != subspaces.end() && !candidateNodes.IsEmpty; ++it)
                {
                    if (it->Count == std::numeric_limits<size_t>::max())
                    {
                        continue;
                    }

                    if (trackEliminatedNodes)
                    {
                        differenceNodes.Copy(candidateNodes);
                    }

                    last = candidateNodes.Count;
                    //Can Always call the diagnostics version because it's compatible with nullptr.
                    //IConstraint::GetTargetNodes forwards the diagnostics version to the regular version, unless overriden.
                    (it->Subspace)->GetTargetNodes(solution, r, candidateNodes, true, nodeToConstraintDiagnosticsDataMapSPtr);
                    it->Count = last - candidateNodes.Count;

                    if ((it->Subspace)->Type == IConstraint::ReplicaExclusionDynamic)
                    {
                        if ((it->Count != 0) && (it->Count != std::numeric_limits<size_t>::max()))
                        {
                            rxdBecauseofDroppedPLBMovements = plbDiagnosticsSPtr_->IsPLBMovementBeingDropped(partition.PartitionId);
                        }
                    }

                    if (trackEliminatedNodes)
                    {
                        //Keeping only the nodes that were eliminated
                        differenceNodes.Filter([&](NodeEntry const * n) -> bool { return !candidateNodes.Check(n); });

                        if (it->Count > 0)
                        {
                            it->NodeDetail = plbDiagnosticsSPtr_->NodeSetDetailString(differenceNodes, nodeToConstraintDiagnosticsDataMapSPtr);
                        }
                    }

                }

                NodeEntry const* node = ChooseNodeForPlacement(placement, r, candidateNodes, random, isDummyPLB);

                if (node != nullptr)
                {
                    //We would ordinarily place the replica here, but the edge case of where a secondary placement succeeds, blocks the
                    //placement of the next replica, and then the placed replica gets dropped by the FM is a case that we'll address via another mechanism

                    if (modifySolutionWhenTracing)
                    {
                        solution.PlaceReplica(r, node);
                    }
                }
                else
                {
                    if (maxConstraintPriority == 0)
                    {
                        std::wstring traceMessage;
                        std::wstring nodeDetailMessage;

                        PLBDiagnostics::GenerateUnplacedTraceandNodeDetailMessages(
                            constraints_, subspaces, r, totalNodeCount,
                            traceMessage, nodeDetailMessage, trackEliminatedNodes, rxdBecauseofDroppedPLBMovements);

                        //Chooses whether to trace in a detailed fashion or not. Throttle the detailed tracing because of size issues, especially for large clusters
                        //Trace message is copied because it is moved and consumed in the TrackUnplacedReplica method
                        std::wstring traceCopy = Environment::NewLine + traceMessage;
                        if (plbDiagnosticsSPtr_->TrackUnplacedReplica(r, traceMessage, nodeDetailMessage))
                        {
                            tracePtr.UnplacedReplicaDetails(r->Partition->PartitionId, r->Partition->Service->Name, StringUtility::ToWString(r->Role), traceCopy, nodeDetailMessage);
                        }
                        else
                        {
                            tracePtr.UnplacedReplica(r->Partition->PartitionId, r->Partition->Service->Name, StringUtility::ToWString(r->Role), traceCopy);
                        }
                    }
                }
            });

            CorrectNonPartiallyPlacement(solution, partition, true);
        }
    }
}

void Checker::AddOrMoveSingletonRelatedReplicasDuringUpgrade(
    NodeEntry const* node,
    std::vector<PlacementReplica const*> const& replicas,
    std::vector<size_t>& newReplicaIndex,
    TempSolution & solution,
    Common::Random & random,
    std::vector<Common::Guid>& nonCreatedMovements) const
{
    for (auto replica : replicas)
    {
        if (replica->IsNew)
        {
            newReplicaIndex.push_back(replica->NewReplicaIndex);
            // If primary replica is not in upgrade (not 'I' flag),
            // then partition is in return after upgrade process,
            // add primary should be initiated
            if (!replica->Partition->PrimaryReplica->IsInUpgrade)
            {
                const_cast<PlacementReplica*>(replica)->Role = ReplicaRole::Enum::Primary;
            }
            solution.PlaceReplica(replica, node);
        }
        else if (replica->Node != node)
        {
            if (!solution.MoveReplica(replica, node, random, SIZE_MAX, true))
            {
                nonCreatedMovements.push_back(replica->Partition->PartitionId);
            };
        }
    }

}

void Checker::SingletonReplicaUpgrade(
    PartitionEntry const& partition,
    std::set<PlacementReplica const*>& processedParentReplicas,
    std::set<ApplicationEntry const*>& processedScaleoutOneApplications,
    TempSolution & solution,
    NodeSet & candidateNodes,
    NodeSet* differenceNodes,
    std::vector<ISubspaceUPtr>* subspaces,
    std::vector<DiagnosticSubspace>* traceSubspace,
    std::vector<size_t>& newReplicaIndex,
    Common::Random & random,
    int maxConstraintPriority,
    bool isDummyPLB,
    bool isInTraceMode,
    bool modifySolutionWhenTracing) const
{
    Placement const* placement = solution.OriginalPlacement;

    // If application has already been processed skip the run, otherwise marked it as processed
    if (partition.IsInRelaxedScaleoutOptimization())
    {
        if (processedScaleoutOneApplications.find(partition.Service->Application) != processedScaleoutOneApplications.end())
        {
            return;
        }
        else
        {
            processedScaleoutOneApplications.insert(partition.Service->Application);
        }
    }

    // Replica is already placed, ignore it
    PlacementReplica const* curReplica = partition.SelectNewReplica(0);
    if (solution.GetReplicaLocation(curReplica) != nullptr)
    {
        return;
    }

    // Find all related replicas with respect to the upgrade optimization mechanism
    std::vector<PlacementReplica const*> allRelatedReplicas =
        partition.IsInCheckAffinityOptimization() ?
        partition.Service->AllAffinityAssociatedReplicasWithTarget1() :
        partition.Service->Application->CorelatedSingletonReplicasInUpgrade;

    // If parent replica has already been processed skip the run, otherwise marked it as processed
    if (partition.IsInCheckAffinityOptimization() && allRelatedReplicas.size() > 0)
    {
        if (processedParentReplicas.find(allRelatedReplicas[0]) != processedParentReplicas.end())
        {
            return;
        }
        else
        {
            processedParentReplicas.insert(allRelatedReplicas[0]);
        }
    }

    if (allRelatedReplicas.size() > 0)
    {
        // If partition has preferred location (i.e. restoring replica location after the upgrade),
        // then set preferred location for all related replicas, which are not in upgrade
        if (partition.PrimaryUpgradeLocation != nullptr)
        {
            for (PlacementReplica const* replica : allRelatedReplicas)
            {
                if (!replica->Partition->IsInSingleReplicaUpgrade)
                {
                    const_cast<PartitionEntry*>(replica->Partition)->PrimaryUpgradeLocation = partition.PrimaryUpgradeLocation;
                }
            }
        }

        // Set upgrade insight into temp solution
        solution.SolutionSearchInsight.HasInsight = true;
        solution.SolutionSearchInsight.SingleReplicaUpgradeInfo.insert(make_pair(&partition, allRelatedReplicas));

        candidateNodes.SelectAll();

        if (!isInTraceMode)
        {
            for (auto it = subspaces->begin(); it != subspaces->end() && !candidateNodes.IsEmpty; ++it)
            {
                (*it)->GetTargetNodesForReplicas(solution, allRelatedReplicas, candidateNodes, true);
            }

            NodeEntry const* node = ChooseNodeForPlacement(placement, allRelatedReplicas, candidateNodes, random, isDummyPLB);

            if (node != nullptr)
            {
                solution.SolutionSearchInsight.SingleReplicaTargetNode.insert(make_pair(&partition, node->NodeId));
                std::vector<Common::Guid> nonCreatedMovements;
                AddOrMoveSingletonRelatedReplicasDuringUpgrade(
                    node,
                    allRelatedReplicas,
                    newReplicaIndex,
                    solution,
                    random,
                    nonCreatedMovements);
                if (nonCreatedMovements.size() > 0)
                {
                    solution.SolutionSearchInsight.SingleReplicaNonCreatedMoves.insert(make_pair(&partition, nonCreatedMovements));
                }
            }
        }
        else if (traceSubspace != nullptr)
        {
            DiagnosticSubspace::RefreshSubspacesDiagnostics(*traceSubspace);
            size_t totalNodeCount = candidateNodes.Count;
            size_t last = candidateNodes.Count;
            bool const trackEliminatedNodes = plbDiagnosticsSPtr_->TrackEliminatedNodes(curReplica);
            NodeToConstraintDiagnosticsDataMapSPtr nodeToConstraintDiagnosticsDataMapSPtr = trackEliminatedNodes ? make_shared<NodeToConstraintDiagnosticsDataMap>() : nullptr;
            bool rxdBecauseofDroppedPLBMovements = false;
            auto& tracePtr = trace_;

            for (auto it = traceSubspace->begin(); it != traceSubspace->end() && !candidateNodes.IsEmpty; ++it)
            {
                if (it->Count == std::numeric_limits<size_t>::max())
                {
                    continue;
                }

                if (trackEliminatedNodes)
                {
                    differenceNodes->Copy(candidateNodes);
                }

                last = candidateNodes.Count;
                it->Subspace->GetTargetNodesForReplicas(solution, allRelatedReplicas, candidateNodes, true);
                it->Count = last - candidateNodes.Count;

                if ((it->Subspace)->Type == IConstraint::ReplicaExclusionDynamic)
                {
                    if ((it->Count != 0) && (it->Count != std::numeric_limits<size_t>::max()))
                    {
                        rxdBecauseofDroppedPLBMovements = plbDiagnosticsSPtr_->IsPLBMovementBeingDropped(partition.PartitionId);
                    }
                }

                if (trackEliminatedNodes)
                {
                    //Keeping only the nodes that were eliminated
                    differenceNodes->Filter([&](NodeEntry const * n) -> bool { return !candidateNodes.Check(n); });

                    if (it->Count > 0)
                    {
                        it->NodeDetail = plbDiagnosticsSPtr_->NodeSetDetailString(*differenceNodes, nodeToConstraintDiagnosticsDataMapSPtr);
                    }
                }
            }

            NodeEntry const* node = ChooseNodeForPlacement(placement, allRelatedReplicas, candidateNodes, random, isDummyPLB);

            if (node != nullptr)
            {
                solution.SolutionSearchInsight.SingleReplicaTargetNode.insert(make_pair(&partition, node->NodeId));
                std::vector<Common::Guid> nonCreatedMovements;
                if (modifySolutionWhenTracing)
                {
                    AddOrMoveSingletonRelatedReplicasDuringUpgrade(
                        node,
                        allRelatedReplicas,
                        newReplicaIndex,
                        solution,
                        random,
                        nonCreatedMovements);
                }
                if (nonCreatedMovements.size() > 0)
                {
                    solution.SolutionSearchInsight.SingleReplicaNonCreatedMoves.insert(make_pair(&partition, nonCreatedMovements));
                }
            }
            else
            {
                if (maxConstraintPriority == 0)
                {
                    std::wstring traceMessage;
                    std::wstring nodeDetailMessage;
                    std::wstring partitionGuids = L"[";

                    bool firstGuid = true;
                    for (auto replica : allRelatedReplicas)
                    {
                        if (replica == curReplica)
                        {
                            // Do not include the current replica - it will be printed separately.
                            continue;
                        }
                        if (!firstGuid)
                        {
                            partitionGuids += L" ";
                        }
                        partitionGuids += replica->Partition->PartitionId.ToString();
                        firstGuid = false;
                    }
                    partitionGuids += L"]";

                    PLBDiagnostics::GenerateUnplacedTraceandNodeDetailMessages(
                        constraints_, *traceSubspace, curReplica, totalNodeCount,
                        traceMessage, nodeDetailMessage, trackEliminatedNodes, rxdBecauseofDroppedPLBMovements);

                    //Chooses whether to trace in a detailed fashion or not. Throttle the detailed tracing because of size issues, especially for large clusters
                    //Trace message is copied because it is moved and consumed in the TrackUnplacedReplica method
                    std::wstring traceCopy = Environment::NewLine + traceMessage;
                    if (plbDiagnosticsSPtr_->TrackUnplacedReplica(curReplica, traceMessage, nodeDetailMessage))
                    {
                        tracePtr.UnplacedReplicaInUpgradeDetails(curReplica->Partition->PartitionId, curReplica->Partition->Service->Name, StringUtility::ToWString(curReplica->Role), partitionGuids, traceCopy, nodeDetailMessage);
                    }
                    else
                    {
                        tracePtr.UnplacedReplicaInUpgrade(curReplica->Partition->PartitionId, curReplica->Partition->Service->Name, StringUtility::ToWString(curReplica->Role), partitionGuids, traceCopy);
                    }
                }
            }
        }
    }

}

bool Checker::InnerCorrectSolution(
    TempSolution & solution,
    Random & random,
    int maxConstraintPriority,
    bool constraintFixLight)
{
    ASSERT_IFNOT(solution.IsEmpty, "Solution for random move should be empty");

    // first correct each constraint separately
    // TODO: correcting constraints based on static constraints
    for (auto it = constraints_.begin(); it != constraints_.end(); ++it)
    {
        if ((*it)->Priority > maxConstraintPriority ||
                (constraintFixLight &&
                    ((*it)->Type == IConstraint::FaultDomain || (*it)->Type == IConstraint::UpgradeDomain)))
        {
            continue;
        }

        (*it)->CorrectViolations(solution, vector<ISubspaceUPtr>(), random);
    }

    // finally correct all constraints together
    return MoveSolution(solution, random, maxConstraintPriority);
}

bool Checker::DiagnoseCorrectSolution(
    TempSolution & tempSolution,
    Common::Random & random)
{
    ASSERT_IFNOT(tempSolution.IsEmpty, "Solution for random move should be empty");

    //This empty method is here to preserve the parallelism between the untraced and traced APIs so that
    //if changes are made to the untraced algorithms, the traced ones can be updated here

    return TraceMoveSolution(tempSolution, random);
}

bool Checker::InnerMoveSolutionRandomly(
    TempSolution & solution,
    bool swapOnly,
    bool useNodeLoadAsHeuristic,
    int maxConstraintPriority,
    bool useRestrictedDefrag,
    Common::Random & random,
    bool forPlacement,
    bool usePartialClosure) const
{
    ASSERT_IFNOT(solution.IsEmpty, "Solution for random move should be empty");

    size_t position = static_cast<size_t>(random.Next(static_cast<int>(solution.BaseSolution.MaxNumberOfCreationAndMigration)));
    ASSERT_IFNOT(position < solution.BaseSolution.MaxNumberOfCreationAndMigration, "Movement index {0} out of bound {1}", position, solution.BaseSolution.MaxNumberOfCreationAndMigration);

    NodeSet candidateNodes(solution.OriginalPlacement, false);
    Movement const & oldMovement = solution.BaseSolution.GetMovement(position);

    bool targetEmptyNodesAchieved = solution.BaseSolution.SolutionScore.DefragTargetEmptyNodesAchieved;

    if (position < solution.BaseSolution.MaxNumberOfCreation)
    {
        if (!oldMovement.IsValid)
        {
            return false;
        }

        ASSERT_IFNOT(oldMovement.IsAdd, "Invalid creation {0} on index {1}", oldMovement, position);

        PlacementReplica const* replica = oldMovement.TargetToBeAddedReplica;
        ASSERT_IFNOT(replica != nullptr && replica->IsNew && replica->NewReplicaIndex == position, "Invalid new replica");

        // During singleton replica upgrade, there could be replicas which are not in upgrade,
        // but are correlated to the ones that are in upgrade (affinity or scaleout), hence movement
        // is generated for them. This movements should not be canceled by balancing phase,
        // as it will violate optimization mechanism
        if (replica->Partition->IsInSingletonReplicaUpgradeOptimization())
        {
            return false;
        }

        vector<ISubspaceUPtr> subspaces = GenerateStaticSubspacesForRandomMove(solution, random, maxConstraintPriority);
        GetTargetNodesForPlacedReplica(solution, replica, subspaces, candidateNodes, false, false);

        NodeEntry const* targetNode = candidateNodes.SelectRandom(random);

        if (targetNode == nullptr || !solution.MoveReplica(replica, targetNode, random))
        {
            return false;
        }
    }
    else if (oldMovement.IsValid && oldMovement.IsDrop)
    {
        // Drops were already selected by best energy, so we're not changing them.
        return false;
    }
    else if (oldMovement.IsValid)
    {
        // During singleton replica upgrade, there could be replicas which are not in upgrade,
        // but are correlated to the ones that are in upgrade (affinity or scaleout), hence movement
        // is generated for them. This movements should not be canceled by balancing phase,
        // as it will violate optimization mechanism
        if (oldMovement.Partition->IsInSingletonReplicaUpgradeOptimization())
        {
            return false;
        }

        solution.CancelMovement(position);
    }
    else
    {
        Placement const * pl = solution.OriginalPlacement;
        if (swapOnly || random.NextDouble() < settings_.SwapPrimaryProbability) //generate a swap
        {
            // TODO: use constraint to implement swap-only?
            PlacementReplica const* replica = pl->SelectRandomPrimary(random, useNodeLoadAsHeuristic, useRestrictedDefrag, targetEmptyNodesAchieved);

            if (replica == nullptr)
            {
                return false;
            }

            ASSERT_IF(replica->IsNew || !replica->IsPrimary, "Invalid replica for random swap");

            PartitionEntry const* partition = replica->Partition;

            PlacementReplica const* replicaToSwap = partition->SelectSecondary(random);

            if (replicaToSwap->IsNew || !replicaToSwap->IsMovable || nullptr == solution.GetReplicaLocation(replicaToSwap))
            {
                // Do not swap with new replicas, replicas that can't be moved or replicas that were dropped.
                return false;
            }

            // TODO: use constraint to implement overflow to underflow only?
            if (useNodeLoadAsHeuristic)
            {
                // Chosen primary replica is marked as beneficial but we need to check will swap to random secondary replica be beneficial
                if (!Placement::IsSwapBeneficial(replicaToSwap))
                {
                    return false;
                }
            }

            if (!(solution.MoveReplica(replica, replicaToSwap->Node, random, position) &&
                solution.MoveReplica(replicaToSwap, replica->Node, random, position)))
            {
                return false;
            }
        }
        else //generate a movement
        {
            PlacementReplica const* replica = forPlacement ?
                pl->SelectRandomExistingReplica(random, usePartialClosure) : pl->SelectRandomReplica(random, useNodeLoadAsHeuristic, useRestrictedDefrag, targetEmptyNodesAchieved);

            if (replica == nullptr || solution.GetReplicaLocation(replica) == nullptr)
            {
                // If replica does not exist, or if it was dropped then do not make the move.
                return false;
            }

            // During singleton replica upgrade, there could be replicas which are not in upgrade,
            // but are correlated to the ones that are in upgrade (affinity or scaleout), hence movement
            // is generated for them. This movements should not be canceled by balancing phase,
            // as it will violate optimization mechanism
            if (replica->Partition->IsInSingletonReplicaUpgradeOptimization())
            {
                return false;
            }

            ASSERT_IF(replica->IsNew || replica->Role == ReplicaRole::None, "Invalid replica {0} for random move", *replica);

            // if the replica is already moved, get its position
            // TODO: use constraint to implement overflow to underflow only?
            vector<ISubspaceUPtr> subspaces = GenerateStaticSubspacesForRandomMove(solution, random, maxConstraintPriority);
            GetTargetNodesForPlacedReplica(solution, replica, subspaces, candidateNodes, useNodeLoadAsHeuristic, false);
            NodeEntry const* targetNode = candidateNodes.SelectRandom(random);

            if (targetNode == nullptr || !solution.MoveReplica(replica, targetNode, random, position))
            {
                return false;
            }
        }
    }

    if (affinityConstraintIndex_ > 0)
    {
        ConstraintCheckResult result = constraints_[affinityConstraintIndex_]->
            CheckSolutionAndGenerateSubspace(solution, true, true, false, random);

        if (!result.IsValid)
        {
            for (auto itReplica = result.InvalidReplicas.begin(); itReplica != result.InvalidReplicas.end(); ++itReplica)
            {
                candidateNodes.SelectAll();
                result.Subspace->GetTargetNodes(solution, *itReplica, candidateNodes, false);

                NodeEntry const* targetNode = candidateNodes.SelectRandom(random);
                if (targetNode == nullptr || !solution.MoveReplica(*itReplica, targetNode, random))
                {
                    return false;
                }
            }
        }
    }

    return MoveSolution(solution, random, maxConstraintPriority);
}

void Checker::GetTargetNodesForReplicaSwapBack(
    TempSolution const& tempSolution,
    PartitionEntry const* partition,
    vector<ISubspaceUPtr> const& subspaces,
    NodeSet & candidateNodes) const
{
    for (auto it = subspaces.begin(); it != subspaces.end() && !candidateNodes.IsEmpty; ++it)
    {
        (*it)->PrimarySwapback(tempSolution, partition, candidateNodes);
    }
}

void Checker::GetTargetNodesForPlacedReplica(
    TempSolution const& tempSolution,
    PlacementReplica const* replica,
    vector<ISubspaceUPtr> const& subspaces,
    NodeSet & candidateNodes,
    bool useNodeLoadAsHeuristic,
    bool useNodeBufferCapacity) const
{
    Placement const* pl = tempSolution.OriginalPlacement;
    if (useNodeLoadAsHeuristic)
    {
        candidateNodes.Clear();
        replica->ForEachBeneficialMetric([&](size_t metricIndex, bool forDefrag) -> bool
        {
            UNREFERENCED_PARAMETER(forDefrag);

            NodeSet const* nodes = pl->GetBeneficialTargetNodesForMetric(metricIndex);
            if (nodes != nullptr)
            {
                candidateNodes.Union(*nodes);
            }
            return true;
        });
    }
    else
    {
        candidateNodes.SelectAll();
    }

    for (auto it = subspaces.begin(); it != subspaces.end() && !candidateNodes.IsEmpty; ++it)
    {
        (*it)->GetTargetNodes(tempSolution, replica, candidateNodes, useNodeBufferCapacity);
    }
}

void Checker::GetSourceNodesForReplicaDrop(
    TempSolution const& tempSolution,
    PartitionEntry const& partition,
    std::vector<ISubspaceUPtr> const& subspaces,
    NodeSet & candidateNodes) const
{
    candidateNodes.Clear();

    partition.ForEachReplica([&](PlacementReplica const* replica)
    {
        NodeEntry const* tempNode = tempSolution.GetReplicaLocation(replica);

        if (tempNode == nullptr || !replica->IsDropable)
        {
            return;
        }

        candidateNodes.Add(tempNode);
    }, false, true);

    for (auto it = subspaces.begin(); it != subspaces.end() && !candidateNodes.IsEmpty; ++it)
    {
        (*it)->GetNodesForReplicaDrop(tempSolution, partition, candidateNodes);
    }
}

void Checker::CorrectNonPartiallyPlacement(
    TempSolution & tempSolution,
    PartitionEntry const& partition,
    bool forTracing /* = false*/) const
{
    if (partition.Service->PartiallyPlace == true)
    {
        return;
    }

    auto partitionPlacement = tempSolution.PartitionPlacements[&partition];
    std::wstring nodeDetailMessage = L"";
    std::wstring traceMessage = L"";

    if (partitionPlacement.ReplicaCount < partition.TotalReplicaCount)
    {
        ServiceEntry const* service = partition.Service;

        if (partition.ExistingReplicaCount == 0)
        {
            if (forTracing)
            {
                traceMessage = wformatString("Service {0}: existing replica {1} required nodes {2} available nodes {3} cannot place any replica since NonPartially flag is set",
                    service->Name,
                    partition.ExistingReplicaCount,
                    StringUtility::ToWString(partition.TotalReplicaCount),
                    StringUtility::ToWString(partitionPlacement.ReplicaCount));

                trace_.UnplacedPartition(partition.PartitionId, service->Name, traceMessage);
            }

            partition.ForEachNewReplica([&](PlacementReplica const* r)
            {
                if (forTracing)
                {
                    plbDiagnosticsSPtr_->TrackUnplacedReplica(r, traceMessage, nodeDetailMessage);
                }

                if (tempSolution.GetReplicaLocation(r) != nullptr)
                {
                    tempSolution.CancelMovement(r->NewReplicaIndex);
                }
            });

        }
        else if (forTracing)
        {
            traceMessage = wformatString("Service {0}: existing replica {1} required nodes {2} available nodes {3}  NonPartially flag is neglected since existring replica > 0",
                service->Name,
                partition.ExistingReplicaCount,
                StringUtility::ToWString(partition.TotalReplicaCount),
                StringUtility::ToWString(partitionPlacement.ReplicaCount));

            trace_.UnplacedPartition(partition.PartitionId, service->Name, traceMessage);
        }
    }
}

const NodeEntry* Checker::ChooseNodeForPlacement(
    Placement const* placement,
    PlacementReplica const* replica,
    NodeSet& candidateNodes,
    Random& random,
    bool isDummyPLB) const
{
    vector<PlacementReplica const*> replicas;
    replicas.push_back(replica);

    return ChooseNodeForPlacement(placement, replicas, candidateNodes, random, isDummyPLB);
}

const NodeEntry* Checker::ChooseNodeForPlacement(
    Placement const* placement,
    vector<PlacementReplica const*> const& replicas,
    NodeSet& candidateNodes,
    Random& random,
    bool isDummyPLB) const
{
    NodeEntry const* node = nullptr;

    if (placement->UseHeuristicDuringPlacement)
    {
        NodeSet heuristicsNodes(placement, true);

        for (auto replica : replicas)
        {
            replica->ForEachWeightedDefragMetric([&](size_t metricIndex) -> bool
            {
                // Iterate through all replica metrics to find good nodes for this replica placement
                NodeSet const* beneficialNodes = placement->GetBeneficialTargetNodesForPlacementForMetric(metricIndex);
                if (beneficialNodes != nullptr)
                {
                    heuristicsNodes.Intersect(*beneficialNodes);
                }
                return true;
            }, settings_);
        }

        // Constraint heuristc nodes
        heuristicsNodes.Intersect(candidateNodes);

        NodeSet *candidateSet = &heuristicsNodes;
        // Fallback to the list of all possible candidate nodes if there is no any beneficial candidate node
        if (heuristicsNodes.Count == 0)
        {
            candidateSet = &candidateNodes;
        }

        node = (isDummyPLB) ? candidateSet->SelectHighestNodeID() : candidateSet->SelectRandom(random);
    }
    else
    {
        node = (isDummyPLB) ? candidateNodes.SelectHighestNodeID() : candidateNodes.SelectRandom(random);
    }

    return node;
}

void Checker::SetMaxPriorityToUse(int maxPriorityToUse)
{
    maxPriorityToUse_ = maxPriorityToUse;
}


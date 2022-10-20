// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Checker.h"
#include "NodeCapacityConstraint.h"
#include "NodeBlockListConstraint.h"
#include "SearcherSettings.h"
#include "TempSolution.h"
#include "NodeEntry.h"
#include "NodeSet.h"
#include "IViolation.h"
#include "DiagnosticsDataStructures.h"
#include "Node.h"


using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

void NodeCapacitySubspace::GetTargetNodes(
    TempSolution const& tempSolution,
    PlacementReplica const* replica,
    NodeSet & candidateNodes,
    bool useNodeBufferCapacity,
    NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr) const
{
    if (!replica->Partition->Service->MetricHasCapacity)
    {
        return;
    }

    std::vector<PlacementReplica const*> replicas;
    replicas.push_back(replica);
    GetTargetNodesForReplicas(tempSolution, replicas, candidateNodes, useNodeBufferCapacity, nodeToConstraintDiagnosticsDataMapSPtr);
}

void NodeCapacitySubspace::GetTargetNodesForReplicas(
    TempSolution const& tempSolution,
    std::vector<PlacementReplica const*> const& replicas,
    NodeSet & candidateNodes,
    bool useNodeBufferCapacity,
    NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr /* = nullptr */) const
{
    ASSERT_IF(replicas.size() == 0, "Replica empty.");
    NodeEntry const* currentNode = replicas.size() == 1 ? tempSolution.GetReplicaLocation(replicas[0]) : nullptr;

    //Choose the function here to avoid the runtime speeed hit of boolean checks for every node in the filter
    auto FilterCandidateNodesForCapacityConstraintsWithOrWithoutDiagnostics = (nodeToConstraintDiagnosticsDataMapSPtr == nullptr) ?
            static_cast<std::function<bool(NodeEntry const *)>>(
                [&](NodeEntry const *node) -> bool
                {
                    return FilterCandidateNodesForCapacityConstraints(node, tempSolution, replicas, useNodeBufferCapacity, false);
                }
            ):
            static_cast<std::function<bool(NodeEntry const *)>>(
                [&](NodeEntry const *node) -> bool
                {
                    NodeCapacityViolationDiagnosticsDataSPtr constraintDiagnosticsDataSPtr =
                        make_shared<NodeCapacityViolationDiagnosticsData>(L"", 0, 0, 0, 0, 0, 0, false);

                    return FilterCandidateNodesForCapacityConstraints(node,
                        tempSolution,
                        replicas,
                        useNodeBufferCapacity,
                        constraintDiagnosticsDataSPtr, false) ? true :
                            [&node, &nodeToConstraintDiagnosticsDataMapSPtr, &constraintDiagnosticsDataSPtr]() -> bool
                            {
                                nodeToConstraintDiagnosticsDataMapSPtr->insert(move(std::pair<Federation::NodeId, NodeCapacityViolationDiagnosticsDataSPtr>(node->NodeId, move(constraintDiagnosticsDataSPtr))));  // with diagnostics
                                return false;
                            }();
                }
            );

    candidateNodes.Filter([&](NodeEntry const *node) -> bool
    {
        if (!node->HasCapacity)
        {
            return true;
        }
        else if (node == currentNode)
        {
            auto itNode = nodeExtraLoads_.find(currentNode);
            if (itNode != nodeExtraLoads_.end())
            {
                auto itReplica = itNode->second.find(replicas[0]);
                if (itReplica != itNode->second.end())
                {
                    // if the replica is one of invalid replicas on this node
                    return false;
                }
            }

            return true;
        }
        else if (replicas[0]->Partition->Service->Application
                 && replicas[0]->Partition->Service->Application->HasReservation)
        {
            // If replica was already on this node, it will be allowed to return because it will be evaluated in the previous
            // else-if branch above. Here we check if application was on this node, and we don't allow new replicas from that app.
            // This won't happen during search for upgrade (replicas.size() > 1), so we're safe checking replicas[0]
            auto const& nodeIt = nodeExtraApplications_.find(node);
            if (   nodeIt != nodeExtraApplications_.end()
                && nodeIt->second.find(replicas[0]->Partition->Service->Application) != nodeIt->second.end())
            {
                // We are moving the entire application from this node, so it's not allowed as target. If we had allowed it, and if 
                // replica load was below reservation then there is no other mechanism for ensuring that application will be moved.
                return false;
            }
            return FilterCandidateNodesForCapacityConstraintsWithOrWithoutDiagnostics(node);
        }
        else if (replicas[0]->Partition->Service->ServicePackage)
        {
            // This won't happen during search for upgrade (replicas.size() > 1), so we're safe checking replicas[0]
            auto const& nodeIt = nodeExtraServicePackages_.find(node);
            if (   nodeIt != nodeExtraServicePackages_.end()
                && nodeIt->second.find(replicas[0]->Partition->Service->ServicePackage) != nodeIt->second.end())
            {
                // We are moving the entire service package from this node, so it's not allowed as target. If we had allowed it, and if 
                // replica load was below reservation then there is no other mechanism for ensuring that application will be moved.
                return false;
            }
            return FilterCandidateNodesForCapacityConstraintsWithOrWithoutDiagnostics(node);
        }
        else
        {
            return FilterCandidateNodesForCapacityConstraintsWithOrWithoutDiagnostics(node);
        }
    });
}

bool NodeCapacitySubspace::FilterByNodeCapacity(
    NodeEntry const *node,
    TempSolution const& tempSolution,
    std::vector<PlacementReplica const*> const& replicas,
    int64 replicaLoadValue,
    bool useNodeBufferCapacity,
    size_t capacityIndex,
    size_t globalMetricIndex,
    IConstraintDiagnosticsDataSPtr const constraintDiagnosticsDataSPtr /* = nullptr */) const
{
    LoadEntry const& shouldDisappearLoads = node->ShouldDisappearLoads;

    LoadEntry const& baseLoads = tempSolution.BaseSolution.NodeChanges[node];

    //if we want to prevent transient overcommit we should used node changes because our load is already calculated in NodeMoveInChanges
    //but we should still check if by returning this replica we will make the solution worse
    LoadEntry const& tempLoads = (tempSolution.OriginalPlacement->Settings.PreventTransientOvercommit && replicas[0]->Node != node) ?
        tempSolution.NodeMovingInChanges[node] : 
        tempSolution.NodeChanges[node];

    ASSERT_IFNOT(tempSolution.OriginalPlacement->TotalMetricCount == tempLoads.Length &&
        tempSolution.OriginalPlacement->TotalMetricCount == baseLoads.Length, "Invalid temp loads or base loads");

    int64 capacity = useNodeBufferCapacity ? node->TotalCapacities.Values[capacityIndex] : node->BufferedCapacities.Values[capacityIndex];

    if (capacity >= 0)
    {
        if (relaxed_)
        {
            int64 loadInBaseSolution = node->Loads.Values[globalMetricIndex] +
                baseLoads.Values[globalMetricIndex] +
                tempSolution.GetBaseApplicationReservedLoad(node, capacityIndex);
            capacity = max(capacity, loadInBaseSolution);
        }

        int64 totalLoad = node->Loads.Values[globalMetricIndex] +   // This was on the node before the beginning of the run.
            shouldDisappearLoads.Values[globalMetricIndex] +        // Dissapearing load (not counted in reservation)
            tempLoads.Values[globalMetricIndex] +                   // Changes in the temp solution
            tempSolution.GetApplicationReservedLoad(node, capacityIndex);  // Reserved load in the temp solution.

        if (replicaLoadValue > 0 &&
            totalLoad + replicaLoadValue > capacity)
        {
            //Nullcheck here in case the overloads are called by people who don't want diagnostics
            //Since we return in this block anyways, there's no real perf hit from the additional nullcheck
            if (constraintDiagnosticsDataSPtr != nullptr)
            {
                int64 originalLoadValue = node->Loads.Values[globalMetricIndex] +
                    tempSolution.GetOriginalApplicationReservedLoad(node, capacityIndex);
                size_t globalMetricCount = tempSolution.OriginalPlacement->GlobalMetricCount;
                size_t totalMetricCount = tempSolution.OriginalPlacement->TotalMetricCount;
                ASSERT_IFNOT(totalMetricCount >= globalMetricCount, "Invalid metric count");
                size_t globalMetricStartIndex = totalMetricCount - globalMetricCount;


                LoadBalancingDomainEntry const& globalDomain = tempSolution.OriginalPlacement->BalanceCheckerObj->LBDomains.back();

                //Need to Static cast back into NodeCapacityViolationDiagnosticsDataSPtr

                auto castedPtr = (static_pointer_cast<NodeCapacityViolationDiagnosticsData>(constraintDiagnosticsDataSPtr));
                castedPtr->metricName_ = globalDomain.Metrics[globalMetricIndex-globalMetricStartIndex].Name;
                castedPtr->replicaLoad_ = replicaLoadValue;
                castedPtr->currentLoad_ = originalLoadValue;
                castedPtr->disappearingLoad_ = shouldDisappearLoads.Values[globalMetricIndex];
                castedPtr->appearingLoad_ = tempLoads.Values[globalMetricIndex];
                castedPtr->reservedLoad_ = tempSolution.GetApplicationReservedLoad(node, capacityIndex);
                castedPtr->capacity_ = capacity;
                castedPtr->diagnosticsSucceeded_ = true;
            }

            return false;
        }
    }

    return true;
}

bool NodeCapacitySubspace::PromoteSecondary(
    TempSolution const& tempSolution,
    PartitionEntry const* partition,
    NodeSet & candidateNodes) const
{
    if (partition == nullptr || !partition->Service->MetricHasCapacity)
    {
        return false;
    }

    std::vector<PartitionEntry const*> partitions;
    partitions.push_back(partition);

    PromoteSecondaryForPartitions(tempSolution, partitions, candidateNodes);

    return false;
}

void NodeCapacitySubspace::PromoteSecondaryForPartitions(TempSolution const& tempSolution, std::vector<PartitionEntry const*> const& partitions, NodeSet & candidateNodes) const
{

    size_t globalMetricCount = tempSolution.OriginalPlacement->GlobalMetricCount;
    size_t totalMetricCount = tempSolution.OriginalPlacement->TotalMetricCount;
    ASSERT_IFNOT(totalMetricCount >= globalMetricCount, "Invalid metric count");
    size_t globalMetricStartIndex = totalMetricCount - globalMetricCount;

    bool applyOnlyMoveInChanges = tempSolution.OriginalPlacement->Settings.PreventTransientOvercommit;

    candidateNodes.Filter([&](NodeEntry const *node) -> bool
    {
        if (!node->IsUp)
        {
            return false;
        }
        if (!node->HasCapacity)
        {
            return true;
        }
        else
        {
            LoadEntry const& originalLoads = node->Loads;
            LoadEntry const& shouldDisappearLoads = node->ShouldDisappearLoads;
            LoadEntry const& baseLoads = tempSolution.BaseSolution.NodeChanges[node];
            LoadEntry const& tempLoads = applyOnlyMoveInChanges ? tempSolution.NodeMovingInChanges[node] : tempSolution.NodeChanges[node];

            ASSERT_IFNOT(totalMetricCount == originalLoads.Length && totalMetricCount == tempLoads.Length &&
                totalMetricCount == baseLoads.Length, "Invalid temp loads or base loads");

            bool ret = true;
            LoadEntry swapLoadDiffEntry(globalMetricCount);

            // We rely on static subspace to filter out nodes without secondaries, so we assume that there are replicas on the node.
            std::map<ApplicationEntry const*, LoadEntry> appLoadOnNode;

            for (auto pIt = partitions.begin(); pIt != partitions.end(); ++pIt)
            {
                auto partition = *pIt;
                ServiceEntry const* replicaService = partition->Service;
                ApplicationEntry const* application = replicaService->Application;
                bool hasReservation = application && application->HasReservation;

                std::map<ApplicationEntry const*, LoadEntry>::iterator appLoadOnNodeIt;

                if (hasReservation)
                {
                    appLoadOnNodeIt = appLoadOnNode.find(application);
                    if (appLoadOnNodeIt == appLoadOnNode.end())
                    {
                        appLoadOnNodeIt = appLoadOnNode.insert(make_pair(application, ((NodeMetrics const&)tempSolution.ApplicationNodeLoad[application])[node])).first;
                    }
                }

                LoadEntry const& primaryEntry = partition->GetReplicaEntry(ReplicaRole::Primary);
                LoadEntry const& secondaryEntry = partition->GetReplicaEntry(ReplicaRole::Secondary, true, node->NodeId);

                for (size_t metricIndex = 0; metricIndex < replicaService->MetricCount; metricIndex++)
                {
                    ASSERT_IFNOT(metricIndex < secondaryEntry.Length,
                        "Metric index should be < replica entry size: {0} {1}",
                        metricIndex,
                        secondaryEntry.Length);

                    int64 swapLoadDiffValue = primaryEntry.Values[metricIndex] - secondaryEntry.Values[metricIndex];

                    size_t globalMetricIndex = replicaService->GlobalMetricIndices[metricIndex];
                    size_t capacityIndex = globalMetricIndex - globalMetricStartIndex;

                    if (hasReservation)
                    {
                        // There are already replicas on the node: this replica may fit into reservation that already exists
                        auto applicationLoadOnNode =  appLoadOnNodeIt->second.Values[capacityIndex];
                        appLoadOnNodeIt->second.AddLoad(capacityIndex, swapLoadDiffValue);
                        auto applicationReservation = application->Reservation.Values[capacityIndex];
                        auto remainingReservation = applicationLoadOnNode < applicationReservation ? applicationReservation - applicationLoadOnNode : 0;
                        swapLoadDiffValue = swapLoadDiffValue < remainingReservation ? 0 : swapLoadDiffValue - remainingReservation;
                    }

                    swapLoadDiffEntry.AddLoad(capacityIndex, swapLoadDiffValue);
                }
            }

            for (size_t i = 0; i < globalMetricCount; ++i)
            {
                int64 partitionsSwapDiffValue = swapLoadDiffEntry.Values[i];
                int64 capacity = node->TotalCapacities.Values[i];

                if (partitionsSwapDiffValue <= 0 || capacity < 0)
                {
                    continue;
                }

                size_t totalIndex = globalMetricStartIndex + i;

                int64 totalLoadWithPartitionsSwap = originalLoads.Values[totalIndex] +
                    tempSolution.GetApplicationReservedLoad(node, i) +
                    shouldDisappearLoads.Values[totalIndex] +
                    tempLoads.Values[totalIndex] +
                    partitionsSwapDiffValue;

                if (totalLoadWithPartitionsSwap > capacity)
                {
                    ret = false;
                    break;
                }
            }

            return ret;
        }
    });
}

void NodeCapacitySubspace::PrimarySwapback(TempSolution const& tempSolution, PartitionEntry const* partition, NodeSet & candidateNodes) const
{
    PromoteSecondary(tempSolution, partition, candidateNodes);
}

NodeCapacityConstraint::NodeCapacityConstraint(int priority)
    : IDynamicConstraint(priority)
{
}

void NodeCapacityConstraint::CorrectViolations(TempSolution & solution, std::vector<ISubspaceUPtr> const& subspaces, Common::Random & random) const
{
    UNREFERENCED_PARAMETER(subspaces);

    ConstraintCheckResult result = CheckSolutionAndGenerateSubspace(solution, false, false, false, random);

    if (!result.IsValid)
    {
        NodeCapacitySubspace const& subspace = dynamic_cast<NodeCapacitySubspace const&>(*(result.Subspace));
        NodeSet candidateNodes(solution.OriginalPlacement, false);
        for (auto itNode = subspace.NodeExtraLoads.begin(); itNode != subspace.NodeExtraLoads.end(); ++itNode)
        {
            PlacementReplicaSet const& replicas = itNode->second;
            for (auto itReplica = replicas.begin(); itReplica != replicas.end(); ++itReplica)
            {
                candidateNodes.SelectEligible(*itReplica, solution.IsSwapPreferred);
                candidateNodes.Delete(itNode->first);

                NodeBlockListConstraint::ExcludeServiceBlockList(candidateNodes, *itReplica);

                NodeEntry const* targetNode = candidateNodes.SelectRandom(random);
                if (targetNode != nullptr)
                {
                    bool shouldTryMove = true;
                    if (solution.IsSwapPreferred)
                    {
                        shouldTryMove = !solution.TrySwapMovement(*itReplica, targetNode, random, false);
                    }
                    if (shouldTryMove)
                    {
                        solution.MoveReplica(*itReplica, targetNode, random);
                    }
                }
            }
        }
    }
}

IViolationUPtr NodeCapacityConstraint::GetViolations(
    TempSolution const& tempSolution,
    bool changedOnly,
    bool relaxed,
    bool useNodeBufferCapacity,
    Random& random) const
{
    UNREFERENCED_PARAMETER(random);
    size_t globalMetricCount = tempSolution.OriginalPlacement->GlobalMetricCount;
    size_t totalMetricCount = tempSolution.OriginalPlacement->TotalMetricCount;
    ASSERT_IFNOT(totalMetricCount >= globalMetricCount, "Invalid metric count");
    size_t globalMetricStartIndex = totalMetricCount - globalMetricCount;

    // Constraint check phase is trying to move things out of the node in order to fix violations.
    // In constraint check case we need to count on freed up space in order to see that violations are fixed.
    bool applyOnlyMoveInChanges = tempSolution.OriginalPlacement->Settings.PreventTransientOvercommit &&
        tempSolution.BaseSolution.CurrentSchedulerAction != PLBSchedulerActionType::ConstraintCheck;

    LoadBalancingDomainEntry const& globalDomain = tempSolution.OriginalPlacement->BalanceCheckerObj->LBDomains.back();
    map<NodeEntry const*, LoadEntry> nodeLoadOverCapacity;
    int64 totalLoadOverCapacity = 0;

    tempSolution.ForEachNode(changedOnly, [&](NodeEntry const* node) -> bool
    {
        if (!node->IsUp || !node->HasCapacity)
        {
            return true;
        }

        LoadEntry loadOverCapacity(globalMetricCount);
        LoadEntry const& originalLoads = node->Loads;
        LoadEntry const& baseLoads = tempSolution.BaseSolution.NodeChanges[node];
        LoadEntry const& tempLoads = applyOnlyMoveInChanges ? tempSolution.NodeMovingInChanges[node] : tempSolution.NodeChanges[node];

        ASSERT_IFNOT(globalDomain.MetricStartIndex == globalMetricStartIndex, "Metrics are not aligned");
        ASSERT_IFNOT(totalMetricCount == originalLoads.Length && totalMetricCount == tempLoads.Length &&
            totalMetricCount == baseLoads.Length, "Invalid temp loads or base loads");

        bool overCapacity = false;
        for (size_t i = 0; i < globalMetricCount; ++i)
        {
            size_t totalIndex = globalMetricStartIndex + i;
            int64 capacity = useNodeBufferCapacity ? node->TotalCapacities.Values[i] : node->BufferedCapacities.Values[i];
            if (capacity >= 0)
            {
                int64 originalLoadValue = node->Loads.Values[totalIndex];
                // load = original load + changes in temp solution + reserved load in temp solution
                int64 load = originalLoadValue +
                    tempLoads.Values[totalIndex] +
                    tempSolution.GetApplicationReservedLoad(node, i);

                if (relaxed)
                {
                    int64 baseSolutionLoad = originalLoadValue +
                        baseLoads.Values[totalIndex] +
                        tempSolution.GetBaseApplicationReservedLoad(node, i);
                    capacity = max(capacity, baseSolutionLoad);
                }

                if (load > capacity)
                {
                    overCapacity = true;
                    loadOverCapacity.Set(i, load - capacity);
                    totalLoadOverCapacity += load - capacity; // TODO: check overflow
                }
            }
        }

        if (overCapacity)
        {
            nodeLoadOverCapacity.insert(make_pair(node, move(loadOverCapacity)));
        }

        return true;
    },
        true,   // Iterate only over nodes with capacity
        false);

    return make_unique<NodeLoadViolation>(totalLoadOverCapacity, move(nodeLoadOverCapacity), &globalDomain);
}

ConstraintCheckResult NodeCapacityConstraint::CheckSolutionAndGenerateSubspace(
    TempSolution const& tempSolution,
    bool changedOnly,
    bool relaxed,
    bool useNodeBufferCapacity,
    Random & random,
    std::shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr /* = nullptr */) const
{
    PlacementReplicaSet invalidReplicas;

    size_t globalMetricCount = tempSolution.OriginalPlacement->GlobalMetricCount;
    size_t totalMetricCount = tempSolution.OriginalPlacement->TotalMetricCount;
    ASSERT_IFNOT(totalMetricCount >= globalMetricCount, "Invalid metric count");
    size_t globalMetricStartIndex = totalMetricCount - globalMetricCount;

    bool preventTransientOvercommit = tempSolution.OriginalPlacement->Settings.PreventTransientOvercommit;
    bool countDisappearingLoad = preventTransientOvercommit && tempSolution.OriginalPlacement->Settings.CountDisappearingLoadForSimulatedAnnealing;
    bool applyOnlyMoveInChanges = preventTransientOvercommit &&
        tempSolution.BaseSolution.CurrentSchedulerAction != PLBSchedulerActionType::ConstraintCheck;

    map<NodeEntry const*, PlacementReplicaSet> nodeChanges;
    map<NodeEntry const*, std::set<ApplicationEntry const*>> nodeExtraApplications;
    map<NodeEntry const*, set<ServicePackageEntry const*>> nodeExtraServicePackages;

    tempSolution.ForEachNode(changedOnly, [&](NodeEntry const* node) -> bool
    {
        // Check for node capacity and calculate the diff (load - capacity)
        // Node is over capacity if:
        //      PreventTransientOvercommit is false:
        //          Original load on the node + load changes > node capacity.
        //      PreventTransientOvercommit is true:
        //          Original load on node + load changes > node capacity
        //          or Load changes > 0 and Original load on node + load changes + should disappear load > capacity.

        if (!node->IsUp || !node->HasCapacity)
        {
            return true;
        }

        PlacementReplicaSet nodeInvalidReplicas;
        std::set<ApplicationEntry const*> nodeInvalidApplications;
        std::set<ServicePackageEntry const*> nodeInvalidServicePackages;

        LoadEntry diff(globalMetricCount);
        LoadEntry const& originalLoads = node->Loads;
        LoadEntry const& baseLoads = tempSolution.BaseSolution.NodeChanges[node];
        LoadEntry const& tempLoads = applyOnlyMoveInChanges ? tempSolution.NodeMovingInChanges[node] : tempSolution.NodeChanges[node];
        LoadEntry const& shouldDisappearLoads = node->ShouldDisappearLoads;

        ASSERT_IFNOT(totalMetricCount == originalLoads.Length && totalMetricCount == tempLoads.Length &&
            totalMetricCount == baseLoads.Length, "Invalid temp loads or base loads");

        bool overCapacity = false;
        for (size_t globalMetricIndex = 0; globalMetricIndex < globalMetricCount; ++globalMetricIndex)
        {
            size_t totalIndex = globalMetricStartIndex + globalMetricIndex;
            int64 nodeCapacity = useNodeBufferCapacity ? node->TotalCapacities.Values[globalMetricIndex] : node->BufferedCapacities.Values[globalMetricIndex];
            int64 capacity = nodeCapacity;

            if (nodeCapacity >= 0)
            {
                int64 originalLoadValue = node->Loads.Values[totalIndex];
                auto tempReservedLoad = tempSolution.GetApplicationReservedLoad(node, globalMetricIndex);
                int64 load = originalLoadValue +
                    tempLoads.Values[totalIndex] +
                    tempReservedLoad;

                if (relaxed)
                {
                    int64 baseLoadWithReservation = originalLoadValue +
                        baseLoads.Values[totalIndex] +
                        tempSolution.GetBaseApplicationReservedLoad(node, globalMetricIndex);
                    capacity = max(nodeCapacity, baseLoadWithReservation);
                }

                if (load > capacity)
                {
                    overCapacity = true;
                }
                else if (countDisappearingLoad)
                {
                    // If we are preventing transient overcommit, and if there is incoming load or reservation to the node,
                    // then we also need to account for disappearing load when calculating capacity violation.
                    // Otherwise, it is possible the incoming replicas will use the load that is occupied by disappearing replicas.
                    if (   tempLoads.Values[totalIndex] > 0
                        || tempReservedLoad > tempSolution.BaseSolution.GetApplicationReservedLoad(node, globalMetricIndex))
                    {
                        auto sdLoad = shouldDisappearLoads.Values[globalMetricIndex];
                        load += sdLoad;
                        if (load > capacity)
                        {
                            overCapacity = true;
                        }
                    }
                }

                if (overCapacity)
                {
                    diff.Set(globalMetricIndex, load - capacity);

                    //Diagnostics
                    if (diagnosticsDataPtr != nullptr)
                    {
                        diagnosticsDataPtr->changed_ = true;

                        LoadBalancingDomainEntry const& globalDomain = tempSolution.OriginalPlacement->BalanceCheckerObj->LBDomains.back();
                        auto nCap = std::static_pointer_cast<NodeCapacityConstraintDiagnosticsData>(diagnosticsDataPtr);

                        MetricCapacityDiagnosticInfo info(
                            globalDomain.Metrics[globalMetricIndex].Name,
                            node->NodeId,
                            (diagnosticsDataPtr->plbNodesPtr_ != nullptr) ? (*(diagnosticsDataPtr->plbNodesPtr_)).at(node->NodeIndex).NodeDescriptionObj.NodeName : L"",
                            load,
                            capacity);

                        nCap->AddMetricDiagnosticInfo(move(info));
                    }
                }
            }
        }

        if (!overCapacity)
        {
            return true;
        }

        // ----------------------------------

        // If transient overcommit should be prevented, newly created node capacity violation
        // should not be fixed by moving out the replicas that have been on the node before initiated moves.
        ReplicaSet const* candidatesToMoveOut = preventTransientOvercommit ?
            &tempSolution.NodeMovingInPlacements[node] : &tempSolution.NodePlacements[node];

        // If node is over capacity and there is no planned replicas for moving in, node was over capacity before PLB run.
        // Constraint check phase should fix that by moving any replica out of the node, but other phases should not.
        if (preventTransientOvercommit &&
            candidatesToMoveOut->Data.empty() &&
            tempSolution.BaseSolution.CurrentSchedulerAction != PLBSchedulerActionType::ConstraintCheck)
        {
            return true;
        }

        // During constraint check phase we can have node over capacity at the very beginning,
        // before any move is initiated or during simulated annealing run after initiated fixes.
        // In second case we should prefer already initiated moved in order not to make node chain violations.
        bool needAnotherPass = true;
        if (preventTransientOvercommit && tempSolution.BaseSolution.CurrentSchedulerAction == PLBSchedulerActionType::ConstraintCheck)
        {
            needAnotherPass = false;

            // First try only with move ins and if there is still a diff go with replicas that are already on a node
            if (candidatesToMoveOut->Data.empty() ||
                !FindAllInvalidReplicas(
                    tempSolution,
                    candidatesToMoveOut->Data,
                    tempSolution.OriginalPlacement->LBDomains.back().MetricStartIndex,
                    random,
                    diff,
                    invalidReplicas,
                    nodeInvalidReplicas,
                    nodeInvalidApplications,
                    nodeInvalidServicePackages,
                    node))
            {
                candidatesToMoveOut = &tempSolution.NodePlacements[node];
                needAnotherPass = true;
            }
        }

        if (needAnotherPass)
        {
            FindAllInvalidReplicas(
                tempSolution,
                candidatesToMoveOut->Data,
                tempSolution.OriginalPlacement->LBDomains.back().MetricStartIndex,
                random,
                diff,
                invalidReplicas,
                nodeInvalidReplicas,
                nodeInvalidApplications,
                nodeInvalidServicePackages,
                node);
        }

        if (nodeInvalidApplications.size() > 0)
        {
            nodeExtraApplications.insert(make_pair(node, move(nodeInvalidApplications)));
        }
        if (nodeInvalidServicePackages.size() > 0)
        {
            nodeExtraServicePackages.insert(make_pair(node, move(nodeInvalidServicePackages)));
        }
        nodeChanges.insert(make_pair(node, move(nodeInvalidReplicas)));

        return true;
    },
        true,   // Iterate only over nodes with capacity
        false);

    return ConstraintCheckResult(make_unique<NodeCapacitySubspace>(relaxed, move(nodeChanges), move(nodeExtraApplications), move(nodeExtraServicePackages)), move(invalidReplicas));
}


bool NodeCapacityConstraint::FindAllInvalidReplicas(
    TempSolution const& tempSolution,
    PlacementReplicaSet const& candidatesToMoveOut,
    size_t const globalMetricStartIndex,
    Random & random,
    LoadEntry & diff,
    PlacementReplicaSet & invalidReplicas,
    PlacementReplicaSet & nodeInvalidReplicas,
    set<ApplicationEntry const*> & nodeInvalidApplications,
    std::set<ServicePackageEntry const*>& nodeInvalidServicePackages,
    NodeEntry const* node,
    bool forApplicationCapacity)
{
    bool allViolationsFound = false;

    ASSERT_IF(candidatesToMoveOut.empty(),
        "TempSolution should have some replicas placed on node {0} when that node's capacity constraint is violated", *node);

    auto myFunc = [&random](size_t n) -> int
    {
        return random.Next(static_cast<int>(n));
    };

    vector<PlacementReplica const*> replicaList;
    replicaList.reserve(candidatesToMoveOut.size());
    for (auto itCandidate = candidatesToMoveOut.begin(); itCandidate != candidatesToMoveOut.end(); ++itCandidate)
    {
        //we will consider this replica only if it was not set as invalid by the previous call of this function (with moving in candidates)
        if (nodeInvalidReplicas.find(*itCandidate) == nodeInvalidReplicas.end())
        {
            replicaList.push_back(*itCandidate);
        }
    }
    random_shuffle(replicaList.begin(), replicaList.end(), myFunc);

    // Used for tracking application reserved load while fixing violations.
    std::map<ApplicationEntry const*, std::pair<LoadEntry, size_t>> remainingApplicationLoadCount;
    std::vector<ServicePackageEntry const *> remainingServicePackages;

    for (auto itReplica = replicaList.begin(); itReplica != replicaList.end(); ++itReplica)
    {
        PlacementReplica const* replica = *itReplica;
        ApplicationEntry const* application = replica->Partition->Service->Application;

        // We don't want to move out standBy or moveInProgress or moveInProgress or toBeDropped replicas
        if (replica->IsStandBy || replica->ShouldDisappear)
        {
            continue;
        }

        bool doesReplicaContributeToCapacityViolation = false;
        ServiceEntry const* replicaService = replica->Partition->Service;

        PlacementReplica const* standByReplicaOnNode = nullptr;
        replica->Partition->ForEachStandbyReplica([&](PlacementReplica const* r)
        {
            if (r->Node == node)
            {
                standByReplicaOnNode = r;
            }
        });

        PlacementReplica const* existingReplicaOnNode = nullptr;
        replica->Partition->ForEachExistingReplica([&](PlacementReplica const* r)
        {
            if (replica->Index != r->Index && r->Node == node)
            {
                existingReplicaOnNode = r;
            }
        });

        TESTASSERT_IF(standByReplicaOnNode != nullptr && existingReplicaOnNode != nullptr, "Node shouldn't have an existing replica on the same node with a standby replica");

        bool checkReservation = !forApplicationCapacity && application && application->HasReservation;

        if (checkReservation)
        {
            if (remainingApplicationLoadCount.find(application) == remainingApplicationLoadCount.end())
            {
                auto const & applicationNodeCounts = tempSolution.ApplicationNodeCounts[application];
                auto const & nodeCountIt = applicationNodeCounts.find(node);
                if (nodeCountIt != applicationNodeCounts.end())
                {
                    size_t numReplicas = nodeCountIt->second;
                    LoadEntry const& applicationLoad = ((NodeMetrics const&)tempSolution.ApplicationNodeLoad[application])[node];
                    remainingApplicationLoadCount.insert(make_pair(application, make_pair(applicationLoad, numReplicas)));
                }
            }
        }

        for (size_t metricIndex = 0; metricIndex < replicaService->MetricCount; metricIndex++)
        {
            ASSERT_IFNOT(metricIndex < replica->ReplicaEntry->Length,
                "Metric index should be < replica entry size: {0} {1}",
                metricIndex,
                replica->ReplicaEntry->Length);

            size_t capacityIndex = replicaService->GlobalMetricIndices[metricIndex] - globalMetricStartIndex;
            ASSERT_IFNOT(capacityIndex < diff.Length,
                "Global metric index index should be < diff size: {0} {1}",
                capacityIndex,
                diff.Length);

            int64 diffValue = diff.Values[capacityIndex];
            int64 loadToBeAddedValue = replica->ReplicaEntry->Values[metricIndex];

            // check if standBy is on this node, as standby is not movable we need to account its load if we move this replica out of this node
            if (standByReplicaOnNode != nullptr)
            {
                loadToBeAddedValue -= standByReplicaOnNode->ReplicaEntry->Values[metricIndex];
            }

            if (checkReservation)
            {
                int64 applicationReservation = application->Reservation.Values[capacityIndex];
                int64 applicationLoadOnNode = remainingApplicationLoadCount[application].first.Values[capacityIndex];
                size_t numReplicasOnNode = remainingApplicationLoadCount[application].second;
                // If there are more application replicas on the node
                if (numReplicasOnNode > 1)
                {
                    if (applicationLoadOnNode < applicationReservation)
                    {
                        // Removing this replica won't help, reservation will remain.
                        loadToBeAddedValue = 0;
                    }
                    else
                    {
                        int64 loadOverReservation = applicationLoadOnNode - applicationReservation;
                        if (loadToBeAddedValue > loadOverReservation)
                        {
                            // We can remove at most this much load.
                            loadToBeAddedValue = loadOverReservation;
                        }
                    }
                }
                // There is only one application replica on the node
                else
                {
                    loadToBeAddedValue = loadToBeAddedValue > applicationReservation ? loadToBeAddedValue: applicationReservation;
                }
            }

            // check whether there are any replicas on this node in the original placement
            if (tempSolution.BaseSolution.OriginalPlacement->Settings.PreventTransientOvercommit && existingReplicaOnNode != nullptr)
            {
                loadToBeAddedValue -= existingReplicaOnNode->ReplicaEntry->Values[metricIndex];
            }

            if (diffValue > 0 && loadToBeAddedValue > 0)
            {
                doesReplicaContributeToCapacityViolation = true;
                diff.Set(capacityIndex, diffValue - loadToBeAddedValue);
            }
        }

        if (doesReplicaContributeToCapacityViolation)
        {
            if (checkReservation)
            {
                auto & appLoadCountOnNode = remainingApplicationLoadCount[application];
                // Subtract load for this replica from the remaining application load.
                for (size_t metricIndex = 0; metricIndex < replicaService->MetricCount; metricIndex++)
                {
                    size_t globalMetricIndex = replicaService->GlobalMetricIndices[metricIndex] - globalMetricStartIndex;
                    int64 loadToBeRemovedValue = replica->ReplicaEntry->Values[metricIndex];
                    appLoadCountOnNode.first.Set(globalMetricIndex, appLoadCountOnNode.first.Values[globalMetricIndex] - loadToBeRemovedValue);
                }
                --appLoadCountOnNode.second;
            }

            invalidReplicas.insert(replica);
            nodeInvalidReplicas.insert(replica);

            auto itOverCapacity = find_if(diff.Values.begin(), diff.Values.end(), [](int64 v)
            {
                return v > 0;
            });

            if (itOverCapacity == diff.Values.end())
            {
                allViolationsFound = true;
                break;
            }
        }
    }

    // We did not manage to find all violations only with replicas. We need to check the reservation as well
    if (!allViolationsFound && !forApplicationCapacity && remainingApplicationLoadCount.size() > 0)
    {
        for (auto const & appCountIt : remainingApplicationLoadCount)
        {
            if (appCountIt.second.second > 0)
            {
                ApplicationEntry const* application = appCountIt.first;

                // We can remove the reservation only if we can remove all replicas from the node.
                LazyMap<NodeEntry const*, ReplicaSet> const& applicationPlacement = tempSolution.ApplicationPlacements[application];
                ReplicaSet const& applicationReplicasOnNode = applicationPlacement[node];

                // Entire application can be moved only if it does not have SB replicas or non-movable replicas. 
                // ShouldDisappear are fine as they do not contribute to reservation
                auto reservationNotMoveable = std::find_if(applicationReplicasOnNode.Data.begin(), applicationReplicasOnNode.Data.end(), [](PlacementReplica const* r) -> bool
                {
                    if (r->IsNew || r->ShouldDisappear)
                    {
                        return false;
                    }
                    if (!r->IsMovable)
                    {
                        return true;
                    }
                    if (r->IsStandBy)
                    {
                        return true;
                    }
                    return false;
                });

                bool canMoveApplication = reservationNotMoveable == applicationReplicasOnNode.Data.end();
                // Entire application can be moved only if we can move all of its replicas
                canMoveApplication = canMoveApplication &&
                    std::includes(candidatesToMoveOut.begin(),
                    candidatesToMoveOut.end(),
                    applicationReplicasOnNode.Data.begin(),
                    applicationReplicasOnNode.Data.end(),
                    PlacementReplicaPointerCompare());

                if (!canMoveApplication)
                {
                    continue;
                }

                bool doesReservationContributeToViolation = false;

                for (size_t appMetricIndex = 0; appMetricIndex < application->AppCapacities.Length; ++appMetricIndex)
                {
                    int64 diffValue = diff.Values[appMetricIndex];

                    int64 appReservationLoad = application->Reservation.Values[appMetricIndex];
                    if (diffValue > 0 && appReservationLoad > 0)
                    {
                        doesReservationContributeToViolation = true;
                        diff.Set(appMetricIndex, diffValue - appReservationLoad);
                    }
                }

                if (doesReservationContributeToViolation)
                {
                    // Insert all applications replicas
                    for (auto const& replica : applicationReplicasOnNode.Data)
                    {
                        invalidReplicas.insert(replica);
                        nodeInvalidReplicas.insert(replica);
                    }

                    nodeInvalidApplications.insert(application);

                    auto itOverCapacity = find_if(diff.Values.begin(), diff.Values.end(), [](int64 v)
                    {
                        return v > 0;
                    });

                    if (itOverCapacity == diff.Values.end())
                    {
                        allViolationsFound = true;
                        break;
                    }
                }
            }
        }
    }

    if (!allViolationsFound && !forApplicationCapacity)
    {
        //we want to capture all the service packages on this node
        for (auto itReplica = replicaList.begin(); itReplica != replicaList.end(); ++itReplica)
        {
            ServiceEntry const* replicaService = (*itReplica)->Partition->Service;

            if (replicaService->ServicePackage != nullptr && 
                find(remainingServicePackages.begin(), remainingServicePackages.end(), replicaService->ServicePackage) == remainingServicePackages.end())
            {
                remainingServicePackages.push_back(replicaService->ServicePackage);
            }
        }

        random_shuffle(remainingServicePackages.begin(), remainingServicePackages.end(), myFunc);

        for (auto itSP = remainingServicePackages.begin(); itSP != remainingServicePackages.end(); ++itSP)
        {
            auto const& replicasOnNode = (tempSolution.ServicePackagePlacements[node]).operator[] (*itSP);
            //only if all the replicas are present in the closure
            bool canMoveAllReplicas = replicasOnNode.first.Data.size() == replicasOnNode.second;
            auto nonMovableIterator = std::find_if(replicasOnNode.first.Data.begin(), replicasOnNode.first.Data.end(), [](PlacementReplica const* r) -> bool
            {
                if (r->IsNew)
                {
                    return false;
                }
                if (!r->IsMovable)
                {
                    return true;
                }
                if (r->IsStandBy)
                {
                    return true;
                }
                return false;
            });

            canMoveAllReplicas = canMoveAllReplicas && (nonMovableIterator == replicasOnNode.first.Data.end());


            //all replicas of the service package must be present on the node and moveable
            canMoveAllReplicas = canMoveAllReplicas && std::includes(candidatesToMoveOut.begin(),
                candidatesToMoveOut.end(),
                replicasOnNode.first.Data.begin(),
                replicasOnNode.first.Data.end(),
                PlacementReplicaPointerCompare());

            if (canMoveAllReplicas)
            {
                size_t capacityIndex = 0;
                bool contributesToViolation = false;
                for (size_t globalMetricIndex = globalMetricStartIndex; globalMetricIndex < (*itSP)->Loads.Values.size(); ++globalMetricIndex, ++capacityIndex)
                {
                    int64 diffValue = diff.Values[capacityIndex];
                    int64 spLoadValue = (*itSP)->Loads.Values[globalMetricIndex];

                    if (diffValue > 0 && spLoadValue > 0)
                    {
                        contributesToViolation = true;
                        diff.Set(capacityIndex, diffValue - spLoadValue);
                    }
                }

                if (contributesToViolation)
                {
                    invalidReplicas.insert(replicasOnNode.first.Data.begin(), replicasOnNode.first.Data.end());
                    nodeInvalidReplicas.insert(replicasOnNode.first.Data.begin(), replicasOnNode.first.Data.end());
                    nodeInvalidServicePackages.insert((*itSP));

                    auto itOverCapacity = find_if(diff.Values.begin(), diff.Values.end(), [](int64 v)
                    {
                        return v > 0;
                    });

                    if (itOverCapacity == diff.Values.end())
                    {
                        allViolationsFound = true;
                        break;
                    }
                }
            }
        }
    }

    return allViolationsFound;
}


// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "IConstraint.h"
#include "TempSolution.h"
#include "IViolation.h"
#include "DiagnosticsDataStructures.h"
#include "Node.h"
#include "SearcherSettings.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

void Reliability::LoadBalancingComponent::WriteToTextWriter(Common::TextWriter & w, IConstraint::Enum const & val)
{
    switch (val)
    {
        case IConstraint::ReplicaExclusionStatic:
            w << "ReplicaExclusionStatic"; return;
        case IConstraint::ReplicaExclusionDynamic:
            w << "ReplicaExclusionDynamic"; return;
        case IConstraint::PlacementConstraint:
            w << "PlacementConstraint"; return;
        case IConstraint::NodeCapacity:
            w << "NodeCapacity"; return;
        case IConstraint::Affinity:
            w << "Affinity"; return;
        case IConstraint::FaultDomain:
            w << "FaultDomain"; return;
        case IConstraint::UpgradeDomain:
            w << "UpgradeDomain"; return;
        case IConstraint::PreferredLocation:
            w << "PreferredLocation"; return;
        case IConstraint::ScaleoutCount:
            w << "ScaleoutCount"; return;
        case IConstraint::ApplicationCapacity:
            w << "ApplicationCapacity"; return;
        case IConstraint::Throttling:
            w << "Throttling"; return;
        default:
            Common::Assert::CodingError("Unknown Constraint Type {0}", val);
    }
}

void ISubspace::GetTargetNodesForReplicas(
    TempSolution const& tempSolution,
    std::vector<PlacementReplica const*> const& replicas,
    NodeSet & candidateNodes,
    bool useNodeBufferCapacity,
    NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr) const
{
    for (auto replica : replicas)
    {
        // In case of singleton replica upgrade optimization (set of replicas moved together),
        // it could happened that some replicas are moved, and for some movement is rejected.
        // In order to include initially chosen node into candidate nodes, exclusion constraint is 
        // skipped for this scenario, e.g.
        // Node:        N0            N1
        // Replicas:   [P/LI/+1]    [P,S]
        if (replicas.size() > 1 &&
            replica->Partition->TargetReplicaSetSize == 1 &&
            !replica->IsNew &&
            (Type == IConstraint::ReplicaExclusionStatic ||
                Type == IConstraint::ReplicaExclusionDynamic))
        {
            continue;
        }

        GetTargetNodes(tempSolution, replica, candidateNodes, useNodeBufferCapacity, nodeToConstraintDiagnosticsDataMapSPtr);
    }
}

bool ISubspace::PromoteSecondary(TempSolution const& tempSolution, PartitionEntry const* partition, NodeSet & candidateNodes) const
{
    UNREFERENCED_PARAMETER(tempSolution);
    UNREFERENCED_PARAMETER(partition);
    UNREFERENCED_PARAMETER(candidateNodes);

    return false;
}

void ISubspace::PromoteSecondaryForPartitions(TempSolution const& tempSolution, std::vector<PartitionEntry const*> const& partitions, NodeSet & candidateNodes) const
{
    for (auto it = partitions.begin(); it != partitions.end(); ++it)
    {
        PromoteSecondary(tempSolution, *it, candidateNodes);
    }
}

void ISubspace::PromoteSecondary(
    TempSolution const& tempSolution,
    PartitionEntry const* partition,
    NodeSet & candidateNodes,
    ISubspace::Enum filter) const
{
    UNREFERENCED_PARAMETER(tempSolution);
    UNREFERENCED_PARAMETER(partition);
    UNREFERENCED_PARAMETER(candidateNodes);
    UNREFERENCED_PARAMETER(filter);
}

void ISubspace::PrimarySwapback(TempSolution const& tempSolution, PartitionEntry const* partition, NodeSet & candidateNodes) const
{
    UNREFERENCED_PARAMETER(tempSolution);
    UNREFERENCED_PARAMETER(partition);
    UNREFERENCED_PARAMETER(candidateNodes);
}

void ISubspace::GetNodesForReplicaDrop(TempSolution const& tempSolution, PartitionEntry const& partition, NodeSet & candidateNodes) const
{
    // In general case, subspaces will not object to dropping replicas, the ones that will should implement this method.
    UNREFERENCED_PARAMETER(tempSolution);
    UNREFERENCED_PARAMETER(partition);
    UNREFERENCED_PARAMETER(candidateNodes);
}

bool ISubspace::FilterCandidateNodesForCapacityConstraints(
    NodeEntry const *node,
    TempSolution const& tempSolution,
    std::vector<PlacementReplica const*> const& replicas,
    bool useNodeBufferCapacity,
    bool forApplicationCapacity) const
{
    IConstraintDiagnosticsDataSPtr placeHolderConstraintDiagnosticsDataSPtr = nullptr;
    return FilterCandidateNodesForCapacityConstraints(node,
        tempSolution,
        replicas,
        useNodeBufferCapacity,
        placeHolderConstraintDiagnosticsDataSPtr,
        forApplicationCapacity);
}

bool ISubspace::FilterCandidateNodesForCapacityConstraints(
    NodeEntry const *node,
    TempSolution const& tempSolution,
    std::vector<PlacementReplica const*> const& replicas,
    bool useNodeBufferCapacity,
    IConstraintDiagnosticsDataSPtr const & constraintDiagnosticsDataSPtr,
    bool forApplicationCapacity) const
{

    size_t globalMetricCount = tempSolution.OriginalPlacement->GlobalMetricCount;
    size_t totalMetricCount = tempSolution.OriginalPlacement->TotalMetricCount;
    ASSERT_IFNOT(totalMetricCount >= globalMetricCount, "Invalid metric count");
    size_t globalMetricStartIndex = totalMetricCount - globalMetricCount;

    bool ret = true;
    LoadEntry loadDifference(globalMetricCount);

    std::map<ApplicationEntry const*, LoadEntry> applicationLoads;
    std::set<ServicePackageEntry const*> addedServicePackages;

    for (auto replica : replicas)
    {
        ServiceEntry const* replicaService = replica->Partition->Service;
        ServicePackageEntry const* servicePackage = replicaService->ServicePackage;
        ApplicationEntry const * application = replicaService->Application;

        // In case of singleton replica upgrade, correlated replicas that are not new,
        // can be moved to other nodes or not moved at all. Hence, when checking a node
        // capacity for a set of replicas, the ones that are already placed should be excluded,
        // to avoid double load counting
        if (replicas.size() > 1 &&
            replica->Partition->IsTargetOne &&
            !replica->IsNew &&
            replica->Node == node &&
            (replica->Partition->Service->HasAffinityAssociatedSingletonReplicaInUpgrade ||
                (replica->Partition->Service->Application != nullptr &&
                 replica->Partition->Service->Application->HasPartitionsInSingletonReplicaUpgrade)))
        {
            continue;
        }

        if (!forApplicationCapacity && servicePackage)
        {
            if (addedServicePackages.find(servicePackage) == addedServicePackages.end())
            {
                addedServicePackages.insert(servicePackage);
                if (!tempSolution.ServicePackagePlacements.HasServicePackageOnNode(servicePackage, node))
                {
                    // If SP is not present on the node, we need to account for it.
                    for (int capacityIndex = 0; capacityIndex < globalMetricCount; ++capacityIndex)
                    {
                        loadDifference.AddLoad(capacityIndex, servicePackage->Loads.Values[capacityIndex + globalMetricStartIndex]);
                    }
                }
            }
        }

        // Application can be nullptr in case when there is no scaleout and no capacity. nullptr is a valid key for std::map.
        auto appIterator = applicationLoads.find(application);

        if (appIterator == applicationLoads.end())
        {
            appIterator = applicationLoads.insert(make_pair(application, LoadEntry(globalMetricCount))).first;
        }

        for (size_t metricIndex = 0; metricIndex < replicaService->MetricCount; ++metricIndex)
        {
            ASSERT_IFNOT(metricIndex < replica->ReplicaEntry->Length,
                "Metric index should be < replica entry size: {0} {1}",
                metricIndex,
                replica->ReplicaEntry->Length);

            size_t globalMetricIndex = replicaService->GlobalMetricIndices[metricIndex];

            int64 loadValue(0);
            vector<NodeEntry const*> const& SBLocations = replica->Partition->StandByLocations;
            if (find(SBLocations.begin(), SBLocations.end(), node) != SBLocations.end())
            {
                LoadEntry const& standByEntry = replica->Partition->GetReplicaEntry(ReplicaRole::Secondary, true, node->NodeId);
                loadValue = replica->ReplicaEntry->Values[metricIndex] - standByEntry.Values[metricIndex];
            }
            else
            {
                loadValue = replica->ReplicaEntry->Values[metricIndex];
            }

            appIterator->second.AddLoad(globalMetricIndex - globalMetricStartIndex, loadValue);
        }
    }

    for (auto newApplicationLoad : applicationLoads)
    {
        for (size_t capacityIndex = 0; capacityIndex < globalMetricCount; ++capacityIndex)
        {
            int64 loadValue = newApplicationLoad.second.Values[capacityIndex];

            ApplicationEntry const* application = newApplicationLoad.first;
            // Check if we are bringing reservation - if it's the first one on the node
            if (!forApplicationCapacity && application != nullptr && application->HasReservation)
            {
                auto const& nodeCounts = tempSolution.ApplicationNodeCounts[application];
                auto nodeCountsIt = nodeCounts.find(node);
                auto reservationOnNode = application->Reservation.Values[capacityIndex];
                if (nodeCountsIt == nodeCounts.end() || nodeCountsIt->second == 0)
                {
                    // This is the first replica from the application on this node - it may bring reservation
                    if (loadValue < reservationOnNode)
                    {
                        loadValue = reservationOnNode;
                    }
                }
                else
                {
                    // There are already replicas on the node: they may fit into reservation that already exists
                    auto applicationLoadOnNode = ((NodeMetrics const&)tempSolution.ApplicationNodeLoad[application])[node].Values[capacityIndex];
                    auto remainingReservation = applicationLoadOnNode < reservationOnNode ? reservationOnNode - applicationLoadOnNode : 0;
                    loadValue = loadValue < remainingReservation ? 0 : loadValue - remainingReservation;
                }
            }

            loadDifference.AddLoad(capacityIndex, loadValue);
        }
    }

    for (size_t metricIndex = 0; metricIndex < globalMetricCount; ++metricIndex)
    {
        size_t capacityIndex = metricIndex;

        ASSERT_IFNOT(capacityIndex < node->TotalCapacities.Length,
            "Global metric index index should be < total diff size: {0} {1}",
            capacityIndex,
            node->TotalCapacities.Length);
        ASSERT_IFNOT(capacityIndex < node->BufferedCapacities.Length,
            "Global metric index index should be < buffered diff size: {0} {1}",
            capacityIndex,
            node->BufferedCapacities.Length);

        //We break out of this loop in this function if it's false, and the Filter function only conducts diagnostics
        //at the very end only if the end result will be false, so using the diagnostics method has no unnecessary perf hit.
        //So no need for function redirection like in GetTargetNodes calling this.
        if (!FilterByNodeCapacity(node, tempSolution, replicas, loadDifference.Values[metricIndex], useNodeBufferCapacity, capacityIndex, metricIndex + globalMetricStartIndex, constraintDiagnosticsDataSPtr))
        {
            ret = false;

            break;
        }
    }

    return ret;
}



bool ISubspace::FilterByNodeCapacity(
    NodeEntry const *node,
    TempSolution const& tempSolution,
    std::vector<PlacementReplica const*> const& replicas,
    int64 replicaLoadValue,
    bool useNodeBufferCapacity,
    size_t capacityIndex,
    size_t globalMetricIndex,
    IConstraintDiagnosticsDataSPtr const constraintDiagnosticsDataSPtr /* = nullptr */) const
{
    UNREFERENCED_PARAMETER(node);
    UNREFERENCED_PARAMETER(tempSolution);
    UNREFERENCED_PARAMETER(replicas);
    UNREFERENCED_PARAMETER(replicaLoadValue);
    UNREFERENCED_PARAMETER(useNodeBufferCapacity);
    UNREFERENCED_PARAMETER(capacityIndex);
    UNREFERENCED_PARAMETER(globalMetricIndex);
    UNREFERENCED_PARAMETER(constraintDiagnosticsDataSPtr);

    return true;
}

void IConstraint::CorrectViolations(TempSolution & solution, std::vector<ISubspaceUPtr> const& subspaces, Common::Random & random) const
{
    UNREFERENCED_PARAMETER(subspaces);

    //all constraints other than AppCapacity,NodeCapacity,Affinity do not benefit from correcting violations using swaps
    //therefore do not try to fix other violations
    if (solution.IsSwapPreferred)
    {
        return;
    }
    ConstraintCheckResult result = CheckSolutionAndGenerateSubspace(solution, false, false, false, random);

    if (!result.IsValid)
    {
        NodeSet candidateNodes(solution.OriginalPlacement, true);

        for (auto itReplica = result.InvalidReplicas.begin(); itReplica != result.InvalidReplicas.end(); ++itReplica)
        {
            candidateNodes.SelectAll();
            result.Subspace->GetTargetNodes(solution, (*itReplica), candidateNodes, false);
            NodeEntry const* targetNode = candidateNodes.SelectRandom(random);
            if (targetNode != nullptr)
            {
                solution.MoveReplica(*itReplica, targetNode, random);
            }
        }
    }
}

IViolationUPtr IStaticConstraint::GetViolations(
    TempSolution const& tempSolution,
    bool changedOnly,
    bool relaxed,
    bool useNodeBufferCapacity,
    Random& random) const
{
    UNREFERENCED_PARAMETER(useNodeBufferCapacity);
    UNREFERENCED_PARAMETER(random);
    return make_unique<ReplicaSetViolation>(GetInvalidReplicas(tempSolution, changedOnly, relaxed));
}

ConstraintCheckResult IStaticConstraint::CheckSolutionAndGenerateSubspace(
    TempSolution const& tempSolution,
    bool changedOnly,
    bool relaxed,
    bool useNodeBufferCapacity,
    Random & random,
    std::shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr /* = nullptr */) const
{
    UNREFERENCED_PARAMETER(random);
    UNREFERENCED_PARAMETER(useNodeBufferCapacity);

    auto invalidReplicas = GetInvalidReplicas(tempSolution, changedOnly, relaxed, diagnosticsDataPtr);

    //Diagnostics
    if (diagnosticsDataPtr != nullptr)
    {
        diagnosticsDataPtr->changed_ = !invalidReplicas.empty();
        TESTASSERT_IF(!((std::dynamic_pointer_cast<ReplicaExclusionStaticConstraintDiagnosticsData>(diagnosticsDataPtr) != nullptr)
            || (std::dynamic_pointer_cast<PlacementConstraintDiagnosticsData> (diagnosticsDataPtr) != nullptr)),
            "Invalid Constraint Type for Static Constraint Diagnostics");

        auto rxS = std::dynamic_pointer_cast<ReplicaExclusionStaticConstraintDiagnosticsData>(diagnosticsDataPtr);
        auto nC = std::dynamic_pointer_cast<PlacementConstraintDiagnosticsData> (diagnosticsDataPtr);

        for (auto const & r : invalidReplicas)
        {
            BasicDiagnosticInfo info;

            info.serviceName_ = r->Partition->Service->Name;
            info.AddPartition(r->Partition->PartitionId);
            info.AddNode(r->Node->NodeId,
                (diagnosticsDataPtr->plbNodesPtr_ != nullptr) 
                ? (*(diagnosticsDataPtr->plbNodesPtr_)).at(r->Node->NodeIndex).NodeDescriptionObj.NodeName 
                : L"");

            if (rxS != nullptr)
            {
                rxS->AddBasicDiagnosticsInfo(move(info));
            }
            else if (nC != nullptr)
            {
                info.miscellanious_ = r->Partition->Service->DiagnosticsData.placementConstraints_;
                nC->AddBasicDiagnosticsInfo(move(info));
            }
        }
    }

    return ConstraintCheckResult(make_unique<StaticSubspace>(this, relaxed), move(invalidReplicas));
}

bool IStaticConstraint::AllowedPlacementOnDeactivatedNode(
    TempSolution const& solution,
    NodeEntry const* currentNode,
    PlacementReplica const* replica) const
{
    vector<int> const& deactivatedNodes = solution.OriginalPlacement->BalanceCheckerObj->DeactivatedNodes;
    vector<int> const& deactivatingNodesAllowPlacement = solution.OriginalPlacement->BalanceCheckerObj->DeactivatingNodesAllowPlacement;
    vector<int> const& deactivatingNodesAllowServiceOnEveryNode =
        solution.OriginalPlacement->BalanceCheckerObj->DeactivatingNodesAllowServiceOnEveryNode;

    if (find(deactivatedNodes.begin(), deactivatedNodes.end(), currentNode->NodeIndex) != deactivatedNodes.end())
    {
        // No replica cannot be placed or moved to Deactivated nodes
        return false;
    }
    else if (!replica->IsNew)
    {
        // Existing replica cannot be placed or moved to nodes with any deactivation intent
        if (find(deactivatingNodesAllowPlacement.begin(),
                deactivatingNodesAllowPlacement.end(),
                currentNode->NodeIndex) != deactivatingNodesAllowPlacement.end() ||
            find(deactivatingNodesAllowServiceOnEveryNode.begin(),
                deactivatingNodesAllowServiceOnEveryNode.end(),
                currentNode->NodeIndex) != deactivatingNodesAllowServiceOnEveryNode.end())
        {
            return false;
        }
    }
    else
    {
        ServiceEntry const* service = replica->Partition->Service;

        if (replica->Partition->IsTargetOne && solution.OriginalPlacement->IsSingletonReplicaMoveAllowedDuringUpgrade)
        {
            // New replicas for stateful volatile services with only one replica should not be created on deactivating nodes
            if (find(deactivatingNodesAllowPlacement.begin(),
                deactivatingNodesAllowPlacement.end(),
                currentNode->NodeIndex) != deactivatingNodesAllowPlacement.end())
            {
                return false;
            }
        }

        if (service->IsStateful || !service->OnEveryNode)
        {
            // Only stateless service with replicas on every node can be placed or moved to deactivatingNodesAllowServiceOnEveryNode nodes.
            // Statefull or stateless services with specified number of replicas should not.
            if (find(deactivatingNodesAllowServiceOnEveryNode.begin(),
                deactivatingNodesAllowServiceOnEveryNode.end(),
                currentNode->NodeIndex) != deactivatingNodesAllowServiceOnEveryNode.end())
            {
                return false;
            }
        }
    }

    return true;
}

PlacementReplicaSet IStaticConstraint::GetInvalidReplicas(
	TempSolution const& solution,
	bool changedOnly,
	bool relaxed,
	std::shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr /* = nullptr */) const
{
    PlacementReplicaSet invalidReplicas;

    solution.ForEachReplica(changedOnly, [&](PlacementReplica const* replica) -> bool
    {
        NodeEntry const* currentNode = solution.GetReplicaLocation(replica);

        if (currentNode == nullptr)
        {
            return true;
        }

        TESTASSERT_IF(!currentNode->IsUp, "Replica should not exist on down node!");

        if (relaxed && currentNode == solution.BaseSolution.GetReplicaLocation(replica))
        {
            return true;
        }

        if (!replica->IsNew && (!replica->IsMovable || !replica->Partition->IsMovable))
        {
            return true;
        }

        // Do not allow moving existing replicas to DeactivatedNodes/DeactivatingNodesAllowPlacement nodes
        // during constraint check and balancing phase.
        // Allow placing new replicas on DeactivatingNodesAllowPlacement nodes and
        // new replicas of stateless services on DeactivatingNodesAllowServiceOnEveryNode.
        if (!CheckReplica(solution, replica, currentNode) || (relaxed && !AllowedPlacementOnDeactivatedNode(solution, currentNode, replica)))
        {
            invalidReplicas.insert(replica);
        }

        return true;
    });

    return invalidReplicas;
}

StaticSubspace::StaticSubspace(IStaticConstraint const* constraint, bool relaxed)
    : constraint_(constraint),
      relaxed_(relaxed)
{
}

void StaticSubspace::GetTargetNodes(
    TempSolution const& tempSolution,
    PlacementReplica const* replica,
    NodeSet & candidateNodes,
    bool useNodeBufferCapacity,
    NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr /* = nullptr */) const
{
    UNREFERENCED_PARAMETER(useNodeBufferCapacity);
    UNREFERENCED_PARAMETER(nodeToConstraintDiagnosticsDataMapSPtr);

    // NodeBlocklistConstraint should allow the replica to return to original node when relaxed.
    // If this is not done, then we can fix either all violations or none.
    // In case when throttling is enabled, or when there is no capacity then we will not allow partial fix.
    bool allowReturnToOriginalNode = false;
    NodeEntry const* baseNode = tempSolution.BaseSolution.GetReplicaLocation(replica);

    if (relaxed_ && constraint_->Type == IConstraint::Enum::PlacementConstraint && baseNode != nullptr)
    {
        allowReturnToOriginalNode = candidateNodes.Check(baseNode);
    }

    if (!replica->IsNew)
    {
        // Existing replica cannot be placed or moved to nodes with any deactivation intent
        candidateNodes.DeleteNodeVecWithIndex(tempSolution.OriginalPlacement->BalanceCheckerObj->DeactivatingNodesAllowPlacement);
        candidateNodes.DeleteNodeVecWithIndex(tempSolution.OriginalPlacement->BalanceCheckerObj->DeactivatingNodesAllowServiceOnEveryNode);
    }
    else
    {
        ServiceEntry const* service = replica->Partition->Service;

        if (replica->Partition->IsTargetOne && tempSolution.OriginalPlacement->IsSingletonReplicaMoveAllowedDuringUpgrade)
        {
            // New replicas for stateful volatile services with only one replica should not be created on deactivating nodes
            candidateNodes.DeleteNodeVecWithIndex(tempSolution.OriginalPlacement->BalanceCheckerObj->DeactivatingNodesAllowPlacement);
        }

        if (service->IsStateful || !service->OnEveryNode)
        {
            // Only stateless service with replicas on every node can be placed or moved to deactivatingNodesAllowServiceOnEveryNode nodes.
            // Statefull or stateless services with specified number of replicas should not.
            candidateNodes.DeleteNodeVecWithIndex(tempSolution.OriginalPlacement->BalanceCheckerObj->DeactivatingNodesAllowServiceOnEveryNode);
        }
    }

    constraint_->GetTargetNodes(tempSolution, replica, candidateNodes);

    if (allowReturnToOriginalNode)
    {
        candidateNodes.Add(baseNode);
    }
}

bool StaticSubspace::PromoteSecondary(TempSolution const& tempSolution, PartitionEntry const* partition, NodeSet & candidateNodes) const
{
    UNREFERENCED_PARAMETER(tempSolution);

    size_t secondaryCandidates = 0;
    size_t deactivatedReplicas = 0;

    candidateNodes.Intersect([&](function<void(NodeEntry const *)> f)
    {
        partition->ForEachExistingReplica([&](PlacementReplica const* r)
        {
            if (r->IsSecondary && !r->Partition->Service->IsNodeInBlockList(r->Node))
            {
                secondaryCandidates++;

                if (r->IsOnDeactivatedNode)
                {
                    deactivatedReplicas++;
                }

                if (!r->IsInTransition)
                {
                    f(r->Node);
                }
            }
        }, true, false);
    });

    return deactivatedReplicas == secondaryCandidates;
}

void StaticSubspace::PromoteSecondary(
    TempSolution const& tempSolution,
    PartitionEntry const* partition,
    NodeSet & candidateNodes,
    ISubspace::Enum filter) const
{
    UNREFERENCED_PARAMETER(tempSolution);

    switch(filter)
    {
        case ISubspace::Enum::OnlySecondaries:
            candidateNodes.Intersect([=](function<void(NodeEntry const *)> f)
            {
                partition->ForEachExistingReplica([&f](PlacementReplica const* r)
                {
                    if (r->IsSecondary)
                    {
                        f(r->Node);
                    }
                }, true, false);
            });
        break;

        case ISubspace::Enum::IsInTransition:
            candidateNodes.Intersect([=](function<void(NodeEntry const *)> f)
            {
                partition->ForEachExistingReplica([&f](PlacementReplica const* r)
                {
                    if (!r->IsInTransition)
                    {
                        f(r->Node);
                    }
                }, true, false);
            });
        break;

        case ISubspace::Enum::NodeBlockList:
            candidateNodes.Intersect([=](function<void(NodeEntry const *)> f)
            {
                partition->ForEachExistingReplica([&f](PlacementReplica const* r)
                {
                    if (!r->Partition->Service->IsNodeInBlockList(r->Node))
                    {
                        f(r->Node);
                    }
                }, true, false);
            });
        break;
    }

}

void StaticSubspace::PrimarySwapback(TempSolution const& tempSolution,
    PartitionEntry const* partition,
    NodeSet & candidateNodes) const
{
    UNREFERENCED_PARAMETER(tempSolution);
    candidateNodes.Filter([&](NodeEntry const *node) -> bool
    {
        if (partition->Service->IsNodeInBlockList(node))
        {
            return false;
        }
        else
        {
            return true;
        }
    });
}

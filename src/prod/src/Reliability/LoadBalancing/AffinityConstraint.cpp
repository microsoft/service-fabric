// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Checker.h"
#include "AffinityConstraint.h"
#include "PlacementReplica.h"
#include "PlacementReplicaSet.h"
#include "PartitionEntry.h"
#include "ServiceEntry.h"
#include "TempSolution.h"
#include "CandidateSolution.h"
#include "SearcherSettings.h"
#include "IViolation.h"
#include "Node.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

AffinitySubspace::AffinitySubspace(bool relaxed)
    : relaxed_(relaxed)
{
}

int const AffinitySubspace::GetColocatedReplicaCount(
    TempSolution const& tempSolution,
    bool colocatePrimary,
    PartitionEntry const* parentPartition,
    ReplicaPlacement const& childPlacement,
    PlacementReplica const* replicaToExclude /* = nullptr */,
    bool useBaseSolution /* = false */)
{
    int colocatedCount = 0;
    parentPartition->ForEachReplica([&](PlacementReplica const* parentReplica)
    {
        NodeEntry const* parentNode;

        if (useBaseSolution)
        {
            parentNode = tempSolution.BaseSolution.GetReplicaLocation(parentReplica);
        }
        else
        {
            parentNode = tempSolution.GetReplicaLocation(parentReplica);
        }

        if (parentNode == nullptr)
        {
            return;
        }

        if (childPlacement.HasReplicaOnNode(parentNode, colocatePrimary ? parentReplica->IsPrimary : true, colocatePrimary ? parentReplica->IsSecondary : true, replicaToExclude))
        {
            colocatedCount++;
        }

    });

    return colocatedCount;
}

void AffinitySubspace::GetTargetNodes(
    TempSolution const& tempSolution,
    PlacementReplica const* replica,
    NodeSet & candidateNodes,
    bool useNodeBufferCapacity,
    NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr /* = nullptr */) const
{
    UNREFERENCED_PARAMETER(useNodeBufferCapacity);
    UNREFERENCED_PARAMETER(nodeToConstraintDiagnosticsDataMapSPtr);

    ServiceEntry const* service = replica->Partition->Service;

    // Skip getting target nodes for StandBy replicas as affinity constraint
    // doesn't care about them and it can happen only when diagnostics goes through all constraints and invalid replicas
    if (replica->IsStandBy ||
        !service->HasAffinity ||
        (tempSolution.OriginalPlacement->Settings.PlaceChildWithoutParent &&
        (service->DependedService == NULL || service->DependedService->PartitionCount == 0)))
    {
        //don't change candidate nodes if it's not a child service or we can place it without parent
        return;
    }

    if (service->DependedService != NULL)
    {
        if (service->DependedService->PartitionCount != 1)
        {
            //we still have not received FT update or replicas are dropped or on down nodes for parent so child should not be placed anywhere
            //or the user specified that the parent is multi partitioned which is not supported
            candidateNodes.Clear();
            return;
        }

        CandidateSolution const& baseSolution = tempSolution.BaseSolution;
        NodeEntry const* baseNode = baseSolution.GetReplicaLocation(replica);
        ASSERT_IF(relaxed_ && baseNode == nullptr, "Base node should not be null while relaxed");

        // if it's primary replica, get the parent primary replicas' current location
        // if it's secondary replica, get all the parent secondary replicas' location
        PartitionEntry const& parentPartition = service->DependedService->SelectPartition(0);

        // don't place child if no parent service replica is placed
        if (!tempSolution.OriginalPlacement->Settings.PlaceChildWithoutParent && parentPartition.ExistingReplicaCount == 0)
        {
            candidateNodes.Clear();
            return;
        }

        bool colocatePrimary =
            replica->Partition->PrimaryReplica != nullptr &&
            parentPartition.PrimaryReplica != nullptr &&
            service->AlignedAffinity &&
            !AffinityConstraint::IsRelaxAlignAffinityDuringSingletonReplicaUpgrade(&parentPartition, replica->Partition, tempSolution);

        ReplicaPlacement const& parentBasePlacement = baseSolution.PartitionPlacements[&parentPartition];

        bool includePrimary = colocatePrimary ? replica->IsPrimary : true;
        bool includeSecondary = colocatePrimary ? replica->IsSecondary : true;

        // In case when child is in singleton replicas upgrade (and parent is not),
        // FM will temporally increase target replica size for child to 2,
        // but it will not be explicitly visible by PLB. For that reason additional condition is required,
        // to allow child service to create new secondary and have more replicas then parent service 
        if (tempSolution.OriginalPlacement->Settings.AllowHigherChildTargetReplicaCountForAffinity && 
            ((parentPartition.TargetReplicaSetSize < replica->Partition->TargetReplicaSetSize) ||
            (!parentPartition.IsInSingleReplicaUpgrade && replica->Partition->IsInSingleReplicaUpgrade)))
        {
            ReplicaPlacement const& childPlacement = tempSolution.PartitionPlacements[replica->Partition];

            // We can place child replica anywhere if all parent replicas are collocated with child replicas
            // or if a parent wasn't collocated with a child in the base solution
            if (parentPartition.ActiveReplicaCount <= GetColocatedReplicaCount(tempSolution, colocatePrimary, &parentPartition, childPlacement, replica)
                || (relaxed_ && !parentBasePlacement.HasReplicaOnNode(baseNode, includePrimary, includeSecondary)))
            {
                // If affinity was in violation in the base solution, we moved all non-colocated child replicas to the invalid replicas list.
                // Afterwards, if moving one of the invalid replicas fixed affinity in some of the previous iterations, we should avoid
                // moving the rest of them, because they will now see all target nodes as valid and they would be randomly moved around without benefit.
                // Basically, they are no longer invalid replicas at that point, so they shouldn't be moved.
                if (!relaxed_ && !replica->IsNew
                    && (parentPartition.ActiveReplicaCount > GetColocatedReplicaCount(
                        tempSolution,
                        colocatePrimary,
                        &parentPartition,
                        baseSolution.PartitionPlacements[replica->Partition],
                        nullptr,
                        true)))
                {
                    candidateNodes.Clear();
                }
                return;
            }
        }

        if (!relaxed_ || parentBasePlacement.HasReplicaOnNode(baseNode, includePrimary, includeSecondary))
        {
            if (colocatePrimary && replica->IsPrimary)
            {
                NodeEntry const* node = tempSolution.GetReplicaLocation(parentPartition.PrimaryReplica);
                if (node != nullptr)
                {
                    candidateNodes.CheckAndSet(node);
                }
            }
            else
            {
                NodeSet tempNodes(candidateNodes);
                candidateNodes.Clear();
                parentPartition.ForEachReplica([&](PlacementReplica const* r)
                {
                    if (!colocatePrimary || !r->IsPrimary)
                    {
                        NodeEntry const* target = tempSolution.GetReplicaLocation(r);
                        if (target && tempNodes.Check(target))
                        {
                            candidateNodes.Add(target);
                        }
                    }
                });
            }
        }
    }
    else
    {
        candidateNodes.Clear();
    }
}

void AffinitySubspace::GetTargetNodesForReplicas(
    TempSolution const& tempSolution,
    std::vector<PlacementReplica const*> const& replicas,
    NodeSet & candidateNodes,
    bool useNodeBufferCapacity,
    NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr) const
{
    // We don't do anything here: we are moving parent + children replicas so they will stay together.
    // Logic in checker will make sure that all replicas will go to the same node!
    UNREFERENCED_PARAMETER(tempSolution);
    UNREFERENCED_PARAMETER(replicas);
    UNREFERENCED_PARAMETER(candidateNodes);
    UNREFERENCED_PARAMETER(useNodeBufferCapacity);
    UNREFERENCED_PARAMETER(nodeToConstraintDiagnosticsDataMapSPtr);
}

void AffinitySubspace::GetNodesForReplicaDrop(TempSolution const& tempSolution, PartitionEntry const& partition, NodeSet & candidateNodes) const
{
    ServiceEntry const* service = partition.Service;
    size_t serviceToDropTargetReplicaCount = partition.TargetReplicaSetSize;

    if (service->DependentServices.size() == 0)
    {
        if (service->DependedService == nullptr)
        {
            return;
        }

        if (service->DependedService->PartitionCount != 1)
        {
            return;
        }

        PartitionEntry const& parentPartition = service->DependedService->SelectPartition(0);
        size_t parentTargetReplicaCount = parentPartition.TargetReplicaSetSize;

        if (tempSolution.OriginalPlacement->Settings.AllowHigherChildTargetReplicaCountForAffinity
            && serviceToDropTargetReplicaCount > parentTargetReplicaCount)
        {
            ReplicaPlacement const& childPlacement = tempSolution.PartitionPlacements[&partition];
            std::vector<NodeEntry const*> nodesWithParent;

            parentPartition.ForEachReplica([&](PlacementReplica const* parentReplica)
            {
                NodeEntry const* parentNode = tempSolution.GetReplicaLocation(parentReplica);

                if (parentNode == nullptr)
                {
                    return;
                }

                if (childPlacement.HasReplicaOnNode(parentNode, true, true))
                {
                    nodesWithParent.push_back(parentNode);

                }
            });

            if (parentTargetReplicaCount <= nodesWithParent.size())
            {
                if (partition.ActiveReplicaCount > nodesWithParent.size())
                {
                    for (NodeEntry const * node : nodesWithParent)
                    {
                        candidateNodes.Delete(node);
                    }
                    return;
                }
                else
                {
                    return;
                }
            }
        }
        else
        {
            return;
        }
    }

    for (size_t childIndex = 0; childIndex < service->DependentServices.size(); ++childIndex)
    {
        ServiceEntry const* childService = service->DependentServices[childIndex];

        for (size_t partitionIndex = 0; partitionIndex < childService->PartitionCount; ++partitionIndex)
        {
            PartitionEntry const& childPartition = childService->SelectPartition(partitionIndex);
            std::vector<NodeEntry const*> nodesWithChild;

            size_t childTargetReplicaCount = childPartition.TargetReplicaSetSize;

            childPartition.ForEachExistingReplica([&](PlacementReplica const* r)
            {
                NodeEntry const *node = tempSolution.GetReplicaLocation(r);
                if (node)
                {
                    nodesWithChild.push_back(node);
                }
            }, false, true);

            if (serviceToDropTargetReplicaCount < childTargetReplicaCount
                && tempSolution.OriginalPlacement->Settings.AllowHigherChildTargetReplicaCountForAffinity)
            {
                continue;
            }

            for (NodeEntry const * node : nodesWithChild)
            {
                candidateNodes.Delete(node);
            }
        }
    }
}

AffinityConstraint::AffinityConstraint(int priority)
    : IDynamicConstraint(priority),
      allowParentToMove_(false)
{
}

bool AffinityConstraint::IsRelaxAlignAffinityDuringSingletonReplicaUpgrade(
    PartitionEntry const* parentPartition,
    PartitionEntry const* childPartition,
    TempSolution const& tempSolution)
{
    if (!(tempSolution.OriginalPlacement->Settings.RelaxAlignAffinityConstraintDuringSingletonUpgrade &&
        parentPartition->IsTargetOne &&
        childPartition->IsTargetOne))
    {
        return false;
    }

    // Relaxing align-affinity (to non-aligned) during singleton replica upgrade,
    // when the upgrade has initiated, as well as during transition state in which
    // there are two replicas and primary to be swapped-out/back is in progress
    bool IsInSingletonReplicaTransition = parentPartition->ExistingReplicaCount > 1 &&
        parentPartition->NumberOfExtraReplicas == 0 &&
        childPartition->ExistingReplicaCount > 1 &&
        childPartition->NumberOfExtraReplicas == 0;

    return parentPartition->IsInUpgrade ||
        childPartition->IsInUpgrade ||
        IsInSingletonReplicaTransition;
}

IViolationUPtr AffinityConstraint::GetViolations(
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

void AffinityConstraint::CorrectViolations(TempSolution & solution, std::vector<ISubspaceUPtr> const& subspaces, Common::Random & random) const
{
    UNREFERENCED_PARAMETER(subspaces);

    ConstraintCheckResult result = CheckSolutionAndGenerateSubspace(solution, false, false, false, random);

    if (!result.IsValid)
    {
        NodeSet candidateNodes(solution.OriginalPlacement, true);

        for (auto itReplica = result.InvalidReplicas.begin(); itReplica != result.InvalidReplicas.end(); ++itReplica)
        {
            candidateNodes.SelectEligible(*itReplica, solution.IsSwapPreferred);
            result.Subspace->GetTargetNodes(solution, (*itReplica), candidateNodes, false);
            NodeEntry const* targetNode = candidateNodes.SelectRandom(random);
            if (targetNode != nullptr)
            {
                // If affinity is aligned try to perform a swap
                // If it is a swap preferred solution always try a swap
                // Otherwise generate a move
                bool shouldTryMove = true;
                if (solution.IsSwapPreferred || (*itReplica)->Partition->Service->IsAlignedChild)
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

ConstraintCheckResult AffinityConstraint::CheckSolutionAndGenerateSubspace(
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

        auto aff = std::static_pointer_cast<AffinityConstraintDiagnosticsData>(diagnosticsDataPtr);

        for (auto const & r : invalidReplicas)
        {
            BasicDiagnosticInfo info;

            info.serviceName_ = r->Partition->Service->Name;
            info.AddPartition(r->Partition->PartitionId);
            info.AddNode(r->Node->NodeId,
                (diagnosticsDataPtr->plbNodesPtr_ != nullptr)
                ? (*(diagnosticsDataPtr->plbNodesPtr_)).at(r->Node->NodeIndex).NodeDescriptionObj.NodeName
                : L"");

            aff->AddBasicDiagnosticsInfo(move(info));
        }
    }

    return ConstraintCheckResult(make_unique<AffinitySubspace>(relaxed), move(invalidReplicas));
}

PlacementReplicaSet AffinityConstraint::GetInvalidReplicas(
    TempSolution const& tempSolution,
    bool changedOnly,
    bool relaxed,
    std::shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr /* = nullptr */) const
{
    PlacementReplicaSet invalidReplicas;

    if (changedOnly)
    {
        set<PartitionEntry const*> parentPartitions;
        tempSolution.ForEachPartition(true, [&](PartitionEntry const* partition) -> bool
        {
            ServiceEntry const* service = partition->Service;

            if (!service->DependentServices.empty() && service->PartitionCount == 1)
            {
                parentPartitions.insert(partition);
            }
            else if (service->DependedService != nullptr && service->DependedService->PartitionCount == 1)
            {
                parentPartitions.insert(&(service->DependedService->SelectPartition(0)));
            }

            return true;
        });

        for (PartitionEntry const* parentPartition : parentPartitions)
        {
            GetInvalidReplicas(tempSolution, parentPartition, invalidReplicas, relaxed, diagnosticsDataPtr);
        }
    }
    else
    {
        for (PartitionEntry const* parentPartition : tempSolution.OriginalPlacement->ParentPartitions)
        {
            GetInvalidReplicas(tempSolution, parentPartition, invalidReplicas, relaxed, diagnosticsDataPtr);
        }
    }

    return invalidReplicas;
}

void AffinityConstraint::GetInvalidReplicas(
    TempSolution const& tempSolution,
    PartitionEntry const* parentPartition,
    PlacementReplicaSet & invalidReplicas,
    bool relaxed,
    std::shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr /* = nullptr */) const
{
    if (parentPartition->Service->PartitionCount != 1)
    {
        return;
    }

    ReplicaPlacement const& parentPlacement = tempSolution.PartitionPlacements[parentPartition];

    CandidateSolution const& baseSolution = tempSolution.BaseSolution;

    ReplicaPlacement const& parentBasePlacement = baseSolution.PartitionPlacements[parentPartition];

    PlacementReplicaSet invalidChildReplicasPerParent;

    vector<ServiceEntry const*> const& childServices = parentPartition->Service->DependentServices;
    bool isParentMovable = (tempSolution.OriginalPlacement->Settings.MoveParentToFixAffinityViolation
        || tempSolution.OriginalPlacement->Settings.IsAffinityBidirectional) && allowParentToMove_;
    bool allowHigherChildTargetReplicaCount = tempSolution.OriginalPlacement->Settings.AllowHigherChildTargetReplicaCountForAffinity;

    // add child services which are without parent service to invalidReplicas
    for (ServiceEntry const* childService : childServices)
    {
        for (size_t i = 0; i < childService->PartitionCount; ++i)
        {
            PartitionEntry const& childPartition = childService->SelectPartition(i);
            int targetReplicaDif = childPartition.TargetReplicaSetSize - parentPartition->TargetReplicaSetSize;
            PlacementReplicaSet invalidReplicasForChildPartition;

            // for each partition, first check if primary replica is colocated
            // then check secondary replicas. If a secondary is colocated with any one secondary replicas,it's considered valid
            // when the affinity is bi-directional, push both parent and child replicas to the invalid replicas
            bool colocatePrimary =
                childPartition.PrimaryReplica != nullptr &&
                parentPartition->PrimaryReplica != nullptr &&
                childPartition.Service->AlignedAffinity &&
                !IsRelaxAlignAffinityDuringSingletonReplicaUpgrade(parentPartition, &childPartition, tempSolution);

            // In case when child is in singleton replicas upgrade (and parent is not),
            // FM will temporally increase target replica size for child to 2,
            // but it will not be explicitly visible by PLB. For that reason additional condition is required,
            // to allow child service to create new secondary and have more replicas then parent service 
            if (!parentPartition->IsInSingleReplicaUpgrade && childPartition.IsInSingleReplicaUpgrade)
            {
                targetReplicaDif = 1;
            }

            if (allowHigherChildTargetReplicaCount && targetReplicaDif > 0)
            {
                ReplicaPlacement const& childPlacement = tempSolution.PartitionPlacements[&childPartition];

                int colocatedCount = AffinitySubspace::GetColocatedReplicaCount(tempSolution, colocatePrimary, parentPartition, childPlacement);

                if (parentPartition->TargetReplicaSetSize <= colocatedCount || parentPartition->ActiveReplicaCount == colocatedCount)
                {
                    continue;
                }
            }

            childPartition.ForEachReplica([&](PlacementReplica const* r)
            {
                // only primary or secondary will be processed
                NodeEntry const* tempNode = tempSolution.GetReplicaLocation(r);
                if (tempNode == nullptr)
                {
                    return;
                }

                NodeEntry const* baseNode = baseSolution.GetReplicaLocation(r);
                if (nullptr == baseNode)
                {
                    // child replica has been dropped in one of the previous stages
                    return;
                }

                bool includePrimary = colocatePrimary ? r->IsPrimary : true;
                bool includeSecondary = colocatePrimary ? r->IsSecondary : true;

                if ((!relaxed || parentBasePlacement.HasReplicaOnNode(baseNode, includePrimary, includeSecondary)) &&
                    !parentPlacement.HasReplicaOnNode(tempNode, includePrimary, includeSecondary))
                {
                    invalidReplicasForChildPartition.insert(r);
                }
            });

            if (invalidReplicasForChildPartition.size() || !isParentMovable)
            {
                for (auto it = invalidReplicasForChildPartition.begin(); it != invalidReplicasForChildPartition.end(); ++it)
                {
                    invalidChildReplicasPerParent.insert(*it);
                    invalidReplicas.insert(*it);
                }
            }
        }
    }

    // if parent is movable and at least one child is not on the same node as parent, add parent replica and all child replicas on that node to invalidReplicas
    if (isParentMovable)
    {
        PlacementReplicaSet invalidChildReplicas = invalidChildReplicasPerParent;

        for (auto childReplica = invalidChildReplicas.begin(); childReplica != invalidChildReplicas.end(); ++childReplica)
        {
            const PartitionEntry* childPartition = (*childReplica)->Partition;

            parentPartition->ForEachReplica([&](PlacementReplica const* parentReplica)
            {
                ReplicaPlacement const& childPlacement = tempSolution.PartitionPlacements[childPartition];
                ReplicaPlacement const& childBasePlacement = baseSolution.PartitionPlacements[childPartition];


                bool colocatePrimary =
                    childPartition->PrimaryReplica != nullptr &&
                    parentPartition->PrimaryReplica != nullptr &&
                    childPartition->Service->AlignedAffinity &&
                    !IsRelaxAlignAffinityDuringSingletonReplicaUpgrade(parentPartition, childPartition, tempSolution);

                NodeEntry const* tempNode = tempSolution.GetReplicaLocation(parentReplica);
                if (tempNode == nullptr)
                {
                    return;
                }

                NodeEntry const* baseNode = baseSolution.GetReplicaLocation(parentReplica);
                ASSERT_IF(relaxed && baseNode == nullptr, "Base node should not be null while relaxed");

                bool includePrimary = colocatePrimary ? parentReplica->IsPrimary : true;
                bool includeSecondary = colocatePrimary ? parentReplica->IsSecondary : true;

                // if parent replica does not have replica of invalid children on the same node, add parent to invalidReplicas
                if ((!relaxed || childBasePlacement.HasReplicaOnNode(baseNode, includePrimary, includeSecondary)) &&
                    !childPlacement.HasReplicaOnNode(tempNode, includePrimary, includeSecondary))
                {
                    invalidReplicas.insert(parentReplica);

                    for (ServiceEntry const* childService : childServices)
                    {
                        for (size_t i = 0; i < childService->PartitionCount; ++i)
                        {
                            PartitionEntry const& childPartitionAll = childService->SelectPartition(i);

                            // parent replica is invalid, add all its child replicas which are on the same node to invalidReplicas
                            childPartitionAll.ForEachReplica([&](PlacementReplica const* child)
                            {
                                NodeEntry const* tempChildNode = tempSolution.GetReplicaLocation(child);
                                if (tempChildNode == nullptr)
                                {
                                    return;
                                }

                                if (tempChildNode == tempNode)
                                {
                                    invalidReplicas.insert(child);
                                }
                            });
                        }
                    }
                }
            });
        }
    }
}

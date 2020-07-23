// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TempSolution.h"
#include "Movement.h"
#include "Placement.h"
#include "CandidateSolution.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

TempSolution::TempSolution(CandidateSolution const& baseSolution)
    : baseSolution_(baseSolution),
    nodePlacements_(&(baseSolution.NodePlacements)),
    nodeMovingInPlacements_(&(baseSolution.NodeMovingInPlacements)),
    partitionPlacements_(&(baseSolution.PartitionPlacements)),
    inBuildCountsPerNode_(&(baseSolution.InBuildCountsPerNode)),
    applicationPlacements_(&(baseSolution.ApplicationPlacements)),
    applicationNodeCounts_(&(baseSolution.ApplicationNodeCounts)),
    applicationTotalLoad_(&(baseSolution.ApplicationTotalLoads)),
    applicationNodeLoads_(&(baseSolution.ApplicationNodeLoad)),
    applicationReservedLoads_(&(baseSolution.ApplicationReservedLoads)),
    faultDomainStructures_(&(baseSolution.FaultDomainStructures), true),
    upgradeDomainStructures_(&(baseSolution.UpgradeDomainStructures), false),
    nodeChanges_(&(baseSolution.NodeChanges)),
    nodeMovingInChanges_(&(baseSolution.NodeMovingInChanges)),
    validSwapCount_(baseSolution.ValidSwapCount),
    validMoveCount_(baseSolution.ValidMoveCount),
    validAddCount_(baseSolution.ValidAddCount),
    moveCost_(baseSolution.MoveCost),
    servicePackagePlacements_(&(baseSolution_.ServicePackagePlacements)),
    solutionSearchInsight_(),
    isSwapPreferred_(false)
{
}

TempSolution::TempSolution(TempSolution && other)
    : baseSolution_(other.baseSolution_), 
    moveChanges_(move(other.moveChanges_)),
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
    validSwapCount_(other.validSwapCount_),
    validMoveCount_(other.validMoveCount_),
    validAddCount_(other.validAddCount_),
    moveCost_(other.moveCost_),
    servicePackagePlacements_(move(other.servicePackagePlacements_)),
    solutionSearchInsight_(move(other.solutionSearchInsight_)),
    isSwapPreferred_(move(other.isSwapPreferred_))
{
}

TempSolution& TempSolution::operator=(TempSolution && other)
{
    if (this != &other)
    {
        moveChanges_ = move(other.moveChanges_);
        replicaMovementPositions_ = move(other.replicaMovementPositions_);
        nodePlacements_ = move(other.nodePlacements_);
        nodeMovingInPlacements_ = move(other.nodeMovingInPlacements_);
        partitionPlacements_ = move(other.partitionPlacements_);
        inBuildCountsPerNode_ = move(other.inBuildCountsPerNode_);
        applicationPlacements_ = move(other.applicationPlacements_);
        applicationNodeCounts_ = move(other.applicationNodeCounts_);
        applicationTotalLoad_ = move(other.applicationTotalLoad_);
        applicationNodeLoads_ = move(other.applicationNodeLoads_);
        applicationReservedLoads_ = move(other.applicationReservedLoads_),
        faultDomainStructures_ = move(other.faultDomainStructures_);
        upgradeDomainStructures_ = move(other.upgradeDomainStructures_);
        nodeChanges_ = move(other.nodeChanges_);
        nodeMovingInChanges_ = move(other.nodeMovingInChanges_);
        validSwapCount_ = other.validSwapCount_;
        validMoveCount_ = other.validMoveCount_;
        validAddCount_ = other.validAddCount_;
        moveCost_ = other.moveCost_;
        servicePackagePlacements_ = move(other.servicePackagePlacements_);
        solutionSearchInsight_ = move(other.solutionSearchInsight_);
        isSwapPreferred_ = move(other.isSwapPreferred_);
    }
    return *this;
}

void TempSolution::ForEachPartition(bool changedOnly, std::function<bool(PartitionEntry const *)> processor) const
{
    if (changedOnly)
    {
        partitionPlacements_.ForEachKey([&](PartitionEntry const* p)
        {
            return processor(p);
        });
    }
    else
    {
        Placement const* pl = OriginalPlacement;
        for (size_t i = 0; i < pl->PartitionCount; ++i)
        {
            if (!processor(&(pl->SelectPartition(i))))
            {
                break;
            }
        }
    }
}

void TempSolution::ForEachApplication(bool changedOnly, std::function<bool(ApplicationEntry const *)> processor) const
{
    if (changedOnly)
    {
        applicationPlacements_.ForEachKey([&](ApplicationEntry const* p)
        {
            return processor(p);
        });
    }
    else
    {
        Placement const* pl = OriginalPlacement;
        for (size_t i = 0; i < pl->ApplicationCount; ++i)
        {
            if (!processor(&(pl->SelectApplication(i))))
            {
                break;
            }
        }
    }
}

void TempSolution::ForEachNode(ApplicationEntry const* app, bool changedOnly, std::function<bool(NodeEntry const *)> processor) const
{
    if (changedOnly)
    {
        if (!applicationPlacements_.HasKey(app))
        {
            // No change of application placement in this temp solution.
            return;
        }
        // Iterate only through nodes that changed.
        applicationPlacements_[app].ForEachKey([&](NodeEntry const* n)
        {
            return processor(n);
        });
    }
    else
    {
        // Iterate through all nodes of the application.
        auto const& appNodeCounts = applicationNodeCounts_[app];
        for (auto node : appNodeCounts)
        {
            if (!processor(node.first))
            {
                break;
            }
        }
    }
}

void TempSolution::ForEachNode(bool changedOnly,
    std::function<bool(NodeEntry const *)> processor,
    bool requireCapacity,
    bool requireThrottling) const
{
    if (changedOnly)
    {
        nodePlacements_.ForEachKey([&](NodeEntry const* n)
        {
            if (requireCapacity && !n->HasCapacity)
            {
                // Skip nodes without capacity if capacity is required
                return true;
            }
            if (requireThrottling && !n->IsThrottled)
            {
                // Skip nodes without throttling, if throttling is required
                return true;
            }
            return processor(n);
        });
    }
    else
    {
        Placement const* pl = OriginalPlacement;
        for (size_t node = 0; node < pl->NodeCount; ++node)
        {
            if (!processor(&(pl->SelectNode(node))))
            {
                break;
            }
        }
    }
}

void TempSolution::ForEachReplica(bool changedOnly, std::function<bool(PlacementReplica const *)> processor) const
{
    if (changedOnly)
    {
        for (auto itReplica = replicaMovementPositions_.begin(); itReplica != replicaMovementPositions_.end(); ++itReplica)
        {
            if (!processor(itReplica->first))
            {
                break;
            }
        }
    }
    else
    {
        size_t replicaCount = OriginalPlacement->TotalReplicaCount;
        for (size_t replica = 0; replica < replicaCount; ++replica)
        {
            if (!processor(OriginalPlacement->SelectReplica(replica)))
            {
                break;
            }
        }
    }
}

NodeEntry const* TempSolution::GetReplicaLocation(PlacementReplica const* replica) const
{
    size_t position = GetReplicaMovementPosition(replica, true);

    if (position == SIZE_MAX)
    {
        return replica->Node;
    }

    Movement const& movement = GetMove(position);

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
            ASSERT_IFNOT(replica == movement.SourceToBeAddedReplica, "Replica {0} mismatch for a swap movement", *replica);
            return movement.SourceNode;
        }
    }
    else if (movement.IsMove || movement.IsAdd || movement.IsPromote)
    {
        return movement.TargetNode;
    }
    else if (movement.IsVoid)
    {
        return movement.SourceNode;
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

void TempSolution::PlaceReplica(PlacementReplica const* replica, NodeEntry const* targetNode)
{
    ASSERT_IF(replica == nullptr || targetNode == nullptr || !replica->IsNew ||
        replica->NewReplicaIndex >= baseSolution_.MaxNumberOfCreation, 
        "Replica invalid or node invalid");
    TESTASSERT_IF(!targetNode->IsUp, "Moving to a down node");

    Movement newMove = Movement::Create(replica, targetNode);

    AddMovement(replica->NewReplicaIndex, move(newMove));
}

void TempSolution::PlaceReplicaAndPromote(PlacementReplica * replica, NodeEntry const* targetNode)
{
    ASSERT_IF(replica == nullptr || targetNode == nullptr || !replica->IsNew || replica->IsPrimary ||
        replica->NewReplicaIndex >= baseSolution_.MaxNumberOfCreation, 
        "Replica invalid or node invalid");
    TESTASSERT_IF(!targetNode->IsUp, "Moving to a down node");

    replica->Role = ReplicaRole::Primary;
    Movement newMove = Movement::Create(replica, targetNode);

    AddMovement(replica->NewReplicaIndex, move(newMove));
}

void TempSolution::PromoteSecondaryForPartitions(std::vector<PartitionEntry const*> const& partitions, Common::Random & random, NodeEntry const* targetNode)
{
    for (auto it = partitions.begin(); it != partitions.end(); ++it)
    {
        PromoteSecondary(*it, random, targetNode);
    }
}

size_t TempSolution::PromoteSecondary(PartitionEntry const* partition, Random & random, NodeEntry const* targetNode)
{
    UNREFERENCED_PARAMETER(random);

    size_t movementUpgradeIndex = SIZE_MAX;
    ASSERT_IF(partition == nullptr || targetNode == nullptr, "Replica invalid or node invalid");
    TESTASSERT_IF(!targetNode->IsUp, "Moving to a down node");

    PlacementReplica const* targetReplica = partition->GetReplica(targetNode);
    //replica is on the same node as in the inital placement
    if (GetReplicaLocation(targetReplica) == targetNode)
    {
        Movement newMove = Movement::CreatePromoteSecondaryMovement(partition, targetNode, targetReplica, OriginalPlacement->Settings.IsTestMode);

        if (partition->UpgradeIndex != SIZE_MAX)
        {
             movementUpgradeIndex = partition->UpgradeIndex + BaseSolution.MaxNumberOfCreation;
            AddMovement(movementUpgradeIndex, move(newMove));
        }
    }

    return movementUpgradeIndex;
}

void TempSolution::AddVoidMovement(PartitionEntry const* partition, Random & random, NodeEntry const* sourceNode)
{
    UNREFERENCED_PARAMETER(random);
    // The preferred location cannot be used, set the partition movement to VOID so that FM can clear the flag
    Movement newMove = Movement::CreateVoidMovement(partition, sourceNode);
    if (partition->UpgradeIndex != SIZE_MAX)
    {
        size_t movementUpgradeIndex = partition->UpgradeIndex + BaseSolution.MaxNumberOfCreation;
        AddMovement(movementUpgradeIndex, move(newMove));
    }
}

bool TempSolution::TrySwapMovement(
    PlacementReplica const* sourceReplica,
    NodeEntry const* targetNode,
    Random & random,
    bool forUpgrade)
{
    UNREFERENCED_PARAMETER(random);

    // Initial parameter check
    if (sourceReplica == nullptr || targetNode == nullptr)
    {
        return false;
    }

    // If there is a replica on a target node (in the initial placement),
    // belonging to the same partition, there is a possibility that swap can be performed
    // Note: The case where replica was not on a target node in the initial placement,
    // but moved to it later (in the candidate or temp solution), is not preferred by this function,
    // and it is covered by generating two movements
    PlacementReplica const* targetReplica = sourceReplica->Partition->GetReplica(targetNode);
    if (targetReplica == nullptr)
    {
        return false;
    }

    // Check if all other prerequisites are satisfied, in order to create a swap
    if (!sourceReplica->IsMovable ||                                // Source replica exists and it is movable
        !targetReplica->IsMovable ||                                // Target replica exists and it is movable
        (sourceReplica->Role != ReplicaRole::Primary  &&            // Source replica is neither Primary, nor Secondary
            sourceReplica->Role != ReplicaRole::Secondary) ||
        (targetReplica->Role != ReplicaRole::Primary &&             // Target replica is neither Primary, nor Secondary
            targetReplica->Role != ReplicaRole::Secondary) ||
        sourceReplica->Role == targetReplica->Role ||               // Source replica role is equal to target replica role
        GetReplicaLocation(targetReplica) != targetNode)            // Target replica is not on a target node in a current temp solution
    {
        return false;
    }

    // If source replica has other movements in the solution,
    // cancel the last one (the valid one)
    const size_t originalPosition = GetReplicaMovementPosition(sourceReplica, true);
    if (originalPosition != SIZE_MAX)
    {
        CancelMovement(originalPosition);
    }

    // Create a new swap movement and add it to the solution
    const size_t swapPosition = FindEmptyMigration(random);
    if (swapPosition != SIZE_MAX)
    {
        AddMovement(swapPosition, sourceReplica->IsPrimary ?
            Movement::Create(sourceReplica, targetReplica, forUpgrade) :
            Movement::Create(targetReplica, sourceReplica, forUpgrade));
    }

    return true;
}

size_t TempSolution::DropReplica(PlacementReplica const* replica, Random & random, NodeEntry const* sourceNode)
{
    ASSERT_IF(replica == nullptr || sourceNode == nullptr, "Replica is invalid, or node is invalid");

    Movement newDrop = Movement::CreateDrop(replica, sourceNode);

    size_t dropIndex = FindEmptyMigration(random, true);
    AddMovement(dropIndex, move(newDrop));
    return dropIndex;
}

bool TempSolution::MoveReplica(PlacementReplica const* replica, NodeEntry const* targetNode, Random & random, size_t position, bool forUpgrade)
{
    ASSERT_IF(replica == nullptr || targetNode == nullptr, "Replica invalid or node invalid");
    TESTASSERT_IF(!targetNode->IsUp, "Moving to a down node");

    NodeEntry const* currentLocation = GetReplicaLocation(replica);
    ASSERT_IF(currentLocation == nullptr, "Current location of the replica should not be null");

    if (currentLocation == targetNode)
    {
        return true;
    }

    if (replica->IsNew)
    {
        ASSERT_IFNOT(position == SIZE_MAX, "Invalid position for new replica {0}", *replica);

        Movement newMove = Movement::Create(replica, targetNode, forUpgrade);

        AddMovement(replica->NewReplicaIndex, move(newMove));
        return true;
    }

    // If the MoveReplica is called from checkUpgradeReplica, the move should be allowed
    if ((!replica->Partition->IsMovable || !replica->IsMovable) && !forUpgrade)
    {
        return false;
    }

    // this is to ensure solution not changed if we can't make any of the movements
    vector<pair<size_t, Movement>> movements;
    bool ret = false;

    // check if the movement generate a swap
    if (replica->Node != targetNode)
    {
        PlacementReplica const* replicaToSwap = replica->Partition->GetReplica(targetNode);

        if (replicaToSwap != nullptr && 
            replicaToSwap->Role != replica->Role && 
            replicaToSwap->Role != ReplicaRole::None &&
            GetReplicaLocation(replicaToSwap) == replica->Node)
        {
            // if another replica of this partition happens to be moved to the location of current replica
            ASSERT_IF(!replicaToSwap->IsMovable && !forUpgrade, "The other replica was moved so it {0} should be movable", *replicaToSwap);
            size_t swapPosition = FindMovementPosition(replicaToSwap, movements, random);
            ASSERT_IF(swapPosition == SIZE_MAX, "The replica to swap should be moved");
            size_t originalPosition = GetReplicaMovementPosition(replica, true);
            if (originalPosition != SIZE_MAX)
            {
                Movement const& originalMove = GetMove(originalPosition);
                if (originalMove.IsSwap)
                {
                    PlacementReplica const* anotherReplica = originalMove.SourceToBeDeletedReplica == replica ? originalMove.TargetToBeDeletedReplica : originalMove.SourceToBeDeletedReplica;
                    ASSERT_IFNOT(anotherReplica->IsMovable, "Another replica was moved so it {0} should be movable", *anotherReplica);
                    movements.push_back(make_pair(originalPosition, Movement::Create(anotherReplica, replica->Node, forUpgrade)));
                }
                else
                {
                    movements.push_back(make_pair(originalPosition, Movement::Invalid));
                }
            }

            movements.push_back(make_pair(swapPosition, replica->IsPrimary ? Movement::Create(replica, replicaToSwap, forUpgrade) : Movement::Create(replicaToSwap, replica, forUpgrade)));
            ret = true;
            goto exit_function;
        }
    }

    if (position == SIZE_MAX)
    {
        position = FindMovementPosition(replica, movements, random);

        if (position == SIZE_MAX)
        {
            position = FindEmptyMigration(random);
        }
    }
    else
    {
        size_t replicaPosition = FindMovementPosition(replica, movements, random);
        if (replicaPosition != SIZE_MAX)
        {
            position = replicaPosition;
        }
    }

    if (position != SIZE_MAX)
    {
        movements.push_back(make_pair(position, replica->Node == targetNode ? Movement::Invalid : Movement::Create(replica, targetNode, forUpgrade)));
        ret = true;
    }

exit_function:
    if (ret && !movements.empty())
    {
        for (size_t i = 0; i < movements.size(); ++i)
        {
            AddMovement(movements[i].first, move(movements[i].second));
        }
    }
    return ret;
}

void TempSolution::CancelMovement(size_t index)
{
    AddMovement(index, Movement(Movement::Invalid));
}

void TempSolution::Clear()
{
    moveChanges_.clear();
    replicaMovementPositions_.clear();
    nodePlacements_.Clear();
    nodeMovingInPlacements_.Clear();
    partitionPlacements_.Clear();
    inBuildCountsPerNode_.Clear();
    servicePackagePlacements_.Clear();
    applicationPlacements_.Clear();
    applicationNodeCounts_.Clear();
    applicationTotalLoad_.Clear();
    applicationNodeLoads_.Clear();
    applicationReservedLoads_.Clear();
    faultDomainStructures_.Clear();
    upgradeDomainStructures_.Clear();
    nodeChanges_.Clear();
    nodeMovingInChanges_.Clear();
    solutionSearchInsight_.Clear();
    validSwapCount_ = baseSolution_.ValidSwapCount;
    validMoveCount_ = baseSolution_.ValidMoveCount;
    validAddCount_ = baseSolution_.ValidAddCount;
    moveCost_ = baseSolution_.MoveCost;
}

// private members
Movement const& TempSolution::GetMove(size_t index) const
{
    ASSERT_IFNOT(index < baseSolution_.MaxNumberOfCreationAndMigration, "Movement index {0} out of bound {1}", index, baseSolution_.MaxNumberOfCreationAndMigration);

    auto itChange = moveChanges_.find(index);
    Movement const& movement = itChange == moveChanges_.end() ? baseSolution_.GetMovement(index) : itChange->second;
    return movement;
}

size_t TempSolution::GetReplicaMovementPosition(PlacementReplica const* replica, bool checkBase) const
{
    auto it = replicaMovementPositions_.find(replica);

    if (it != replicaMovementPositions_.end())
    {
        return it->second;
    }
    else
    {
        return checkBase ? baseSolution_.GetReplicaMovementPosition(replica) : SIZE_MAX;
    }
}

size_t TempSolution::FindMovementPosition(PlacementReplica const* replica, vector<pair<size_t, Movement>> & additionalMovements, Random & random) const
{
    ASSERT_IF(replica->IsNew, "This method doesn't handle new replicas {0}", *replica);

    size_t position = GetReplicaMovementPosition(replica, false);
    if (position != SIZE_MAX)
    {
        Movement const& move = GetMove(position);
        if (move.IsSwap)
        {
            size_t anotherPosition = FindEmptyMigration(random);
            if (anotherPosition == SIZE_MAX)
            {
                position = SIZE_MAX;
            }
            else
            {
                PlacementReplica const* anotherReplica = move.SourceToBeDeletedReplica == replica ? move.TargetToBeDeletedReplica : move.SourceToBeDeletedReplica;
                additionalMovements.push_back(make_pair(position, Movement::Invalid));
                additionalMovements.push_back(make_pair(anotherPosition, Movement::Create(anotherReplica, replica->Node)));
            }
        }
    }
    else
    {
        position = baseSolution_.GetReplicaMovementPosition(replica);
        if (position != SIZE_MAX)
        {
            // a replica is not moved in tempsolution, but is moved in base solution
            // and we can find another move (invalid or valid) in the position
            auto itMove = moveChanges_.find(position);

            if (itMove == moveChanges_.end())
            {
                Movement const& move = baseSolution_.GetMovement(position);
                if (move.IsSwap)
                {
                    size_t anotherPosition = FindEmptyMigration(random);
                    if (anotherPosition == SIZE_MAX)
                    {
                        position = SIZE_MAX;
                    }
                    else
                    {
                        PlacementReplica const* anotherReplica = move.SourceToBeDeletedReplica == replica ? move.TargetToBeDeletedReplica : move.SourceToBeDeletedReplica;
                        additionalMovements.push_back(make_pair(position, Movement::Invalid));
                        additionalMovements.push_back(make_pair(anotherPosition, Movement::Create(anotherReplica, replica->Node)));
                    }
                }
            }
            else if (itMove->second.IsValid)
            {
                size_t anotherPosition = FindEmptyMigration(random);

                if (anotherPosition == SIZE_MAX)
                {
                    position = SIZE_MAX;
                }
                else
                {
                    Movement originalMovement(itMove->second);
                    additionalMovements.push_back(make_pair(position, Movement::Invalid));
                    additionalMovements.push_back(make_pair(anotherPosition, std::move(originalMovement)));
                }
            }
        }
    }

    return position;
}

size_t TempSolution::FindEmptyMigration(Random & random, bool forDroppingExtraReplica) const
{
    size_t position = SIZE_MAX;
    if (baseSolution_.MaxNumberOfMigration > 0)
    {
        size_t availableMovementSlots = baseSolution_.MaxNumberOfMigration;
        size_t startIndex = baseSolution_.MaxNumberOfCreation;

        if (forDroppingExtraReplica)
        {
            availableMovementSlots -= baseSolution_.OriginalPlacement->PartitionsInUpgradeCount;
            startIndex += baseSolution_.OriginalPlacement->PartitionsInUpgradeCount;
        }

        size_t index = random.Next(static_cast<int>(availableMovementSlots));
        for (size_t count = 0; count < availableMovementSlots; ++count)
        {
            size_t totalIndex = startIndex + index;
            if (moveChanges_.find(totalIndex) == moveChanges_.end())
            {
                Movement const& current = GetMove(totalIndex);

                if (!current.IsValid)
                {
                    position = totalIndex;
                    break;
                }
            }

            ++index;
            if (index == availableMovementSlots)
            {
                index = 0;
            }
        }
    }

    return position;
}

void TempSolution::AddMovement(size_t index, Movement && movement)
{
    auto it = moveChanges_.find(index);
    bool changeExists = (it != moveChanges_.end());

    Movement const& baseMove = baseSolution_.GetMovement(index);
    Movement const& oldMove = changeExists ? it->second : baseMove;

    if (oldMove != movement)
    {
        if (oldMove.IsValid)
        {
            oldMove.ForEachReplica([&](PlacementReplica const* r)
            {
                auto itReplicaMovement = replicaMovementPositions_.find(r);

                size_t basePosition = baseSolution_.GetReplicaMovementPosition(r);
                if (basePosition == SIZE_MAX)
                {
                    // old movement is in tempsolution
                    ASSERT_IF(itReplicaMovement == replicaMovementPositions_.end(), "Replica {0} movement position for an old movement {1} should exist", *r, oldMove);
                    replicaMovementPositions_.erase(itReplicaMovement);
                }
                else if (itReplicaMovement == replicaMovementPositions_.end())
                {
                    replicaMovementPositions_.insert(std::make_pair(r, SIZE_MAX));
                }
                else
                {
                    itReplicaMovement->second = SIZE_MAX;
                }
            });
        }

        if (movement.IsValid)
        {
            movement.ForEachReplica([&](PlacementReplica const* r)
            {
                auto itReplicaMovement = replicaMovementPositions_.find(r);
                if(itReplicaMovement == replicaMovementPositions_.end())
                {
                    replicaMovementPositions_.insert(std::make_pair(r, index));
                }
                else
                {
                    ASSERT_IFNOT(itReplicaMovement->second == SIZE_MAX, "A new move's replica already exist in replica movement position; replica {0}, movement {1}", *r, movement);
                    itReplicaMovement->second = index;
                }
            });
        }

        nodePlacements_.ChangeMovement(oldMove, movement);
        partitionPlacements_.ChangeMovement(oldMove, movement);
        servicePackagePlacements_.ChangeMovement(oldMove, movement, nodeChanges_, nodeMovingInChanges_);
        applicationPlacements_.ChangeMovement(oldMove, movement);
        applicationTotalLoad_.ChangeMovement(oldMove, movement);

        if (baseSolution_.OriginalPlacement->IsThrottlingConstraintNeeded)
        {
            inBuildCountsPerNode_.ChangeMovement(oldMove, movement);
        }

        // ApplicationReservedLoads will make the change in node loads and in node counts.
        applicationReservedLoads_.ChangeMovement(oldMove, movement, applicationNodeLoads_, applicationNodeCounts_);

        if (!OriginalPlacement->BalanceCheckerObj->FaultDomainStructure.IsEmpty)
        {
            faultDomainStructures_.ChangeMovement(oldMove, movement);
        }

        if (!OriginalPlacement->BalanceCheckerObj->UpgradeDomainStructure.IsEmpty)
        {
            upgradeDomainStructures_.ChangeMovement(oldMove, movement);
        }

        nodeChanges_.ChangeMovement(oldMove, movement);

        if (baseSolution_.OriginalPlacement->Settings.PreventTransientOvercommit)
        {
            nodeMovingInChanges_.ChangeMovingIn(oldMove, movement);
            nodeMovingInPlacements_.ChangeMovingIn(oldMove, movement);
        }

        ModifyValidMoveCount(oldMove, movement);

        if (!changeExists)
        {
            moveChanges_.insert(std::make_pair(index, std::move(movement))).first;
        }
        else if (baseMove != movement)
        {
            it->second = std::move(movement);
        }
        else
        {
            moveChanges_.erase(it);
        }
    }
    //Trace.WriteInfo("AddMovement", "changeExists = {0} --- Old move: IsValid = {1} ---  New move: IsValid = {2}", changeExists, oldMove.IsValid, movement.IsValid);  // TODO
}

void TempSolution::ModifyValidMoveCount(Movement const & oldMove, Movement const & newMove)
{
    if (newMove.IsValid)
    {
        validMoveCount_++;
        if (newMove.IsMove)
        {
            // A new move is added - update move cost accordingly.
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
        ASSERT_IF(validMoveCount_ == 0, "Valid movement empty when removing an old move {0}, new move {1}", oldMove, newMove);
        validMoveCount_--;
        if (oldMove.IsMove)
        {
            // Existing move is cancelled - update move cost accordingly.
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
            ASSERT_IF(validSwapCount_ == 0, "Valid sswap count is zero when removing swap.");
            validSwapCount_--;
        }
    }
}


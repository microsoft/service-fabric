// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Checker.h"
#include "PartitionPlacement.h"
#include "PlacementReplica.h"
#include "PlacementReplicaSet.h"
#include "PartitionEntry.h"
#include "TempSolution.h"
#include "Placement.h"
#include "FaultDomainConstraint.h"
#include "PLBConfig.h"
#include "IViolation.h"
#include "math.h"
#include "Node.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

FaultDomainSubspace::FaultDomainSubspace(bool isFaultDomain, bool relaxed)
    : isFaultDomain_(isFaultDomain), relaxed_(relaxed)
{
}

bool FaultDomainSubspace::IsQuorumBasedDomain(TempSolution const& tempSolution, bool isFaultDomain, ServiceEntry const* service)
{
    if (service->AutoSwitchToQuorumBasedLogic)
    {
        return true;
    }

    if (isFaultDomain && tempSolution.OriginalPlacement->Settings.QuorumBasedReplicaDistributionPerFaultDomains)
    {
        return true;
    }

    if (!isFaultDomain && tempSolution.OriginalPlacement->Settings.QuorumBasedReplicaDistributionPerUpgradeDomains)
    {
        return true;
    }

    return false;
}

bool FaultDomainSubspace::PromoteSecondary(
    TempSolution const& tempSolution,
    PartitionEntry const* partition,
    NodeSet & candidateNodes) const
{
    const PlacementReplica* primary = partition->PrimaryReplica;
    if (isFaultDomain_ || !primary || primary->IsNew)
    {
        // PromoteSecondary is only for UD
        return false;
    }

    NodeEntry const* primaryNode = tempSolution.GetReplicaLocation(primary);
    ASSERT_IF(primaryNode == nullptr, "Null primary node when check UD promote secondary for partition {0}", *partition)

    PartitionDomainTree const* replicaTreeBase = nullptr;
    BalanceChecker::DomainTree const* nodeTreeBase = nullptr;
    FaultDomainConstraint::GetBaseTrees(tempSolution, partition, isFaultDomain_, false, replicaTreeBase, nodeTreeBase);

    PartitionDomainTree const& replicaTreeTemp = tempSolution.UpgradeDomainStructures[partition];
    ASSERT_IF(replicaTreeTemp.IsEmpty, "Domain structure is empty");

    TreeNodeIndex primaryDomain = primaryNode->UpgradeDomainIndex;

    candidateNodes.Filter([&](NodeEntry const *node) -> bool
    {
        TreeNodeIndex const& index = node->UpgradeDomainIndex;
        ASSERT_IF(index.Length == 0, "Domain index should not be empty");

        bool sameDomain = true;
        for (size_t i = 0; i < index.Length; ++i)
        {
            size_t segment = index.GetSegment(i);

            if (segment != primaryDomain.GetSegment(i))
            {
                sameDomain = false;
                break;
            }
        }
        return !sameDomain;
    });

    return false;
}

void FaultDomainSubspace::PrimarySwapback(TempSolution const& tempSolution, PartitionEntry const* partition, NodeSet & candidateNodes) const
{
    PromoteSecondary(tempSolution, partition, candidateNodes);
}

void FaultDomainSubspace::GetTargetNodes(
    TempSolution const& tempSolution,
    PlacementReplica const* replica,
    NodeSet & candidateNodes,
    bool useNodeBufferCapacity,
    NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr) const
{
    UNREFERENCED_PARAMETER(useNodeBufferCapacity);
    UNREFERENCED_PARAMETER(nodeToConstraintDiagnosticsDataMapSPtr);

    PartitionEntry const* partition = replica->Partition;
    ServiceEntry const* service = partition->Service;

    if (service->OnEveryNode || (isFaultDomain_ && service->FDDistribution == Service::Type::Ignore))
    {
        return;
    }

    bool nonPacking = isFaultDomain_ && service->FDDistribution == Service::Type::Nonpacking;
    PartitionDomainTree const* replicaTreeBase = nullptr;
    BalanceChecker::DomainTree const* nodeTreeBase = nullptr;
    FaultDomainConstraint::GetBaseTrees(tempSolution, partition, isFaultDomain_, false, replicaTreeBase, nodeTreeBase);

    if (nonPacking || !FaultDomainSubspace::IsQuorumBasedDomain(tempSolution, isFaultDomain_,service))
    {
        if (relaxed_ && FaultDomainConstraint::HasDomainViolation(replicaTreeBase->Root, nodeTreeBase->Root, nonPacking))
        {
            // if the base solution already violate the fault domain, skip it
            return;
        }
    }

    PartitionDomainTree const* replicaTreeTemp = nullptr;
    if (!FaultDomainConstraint::GetTempTrees(tempSolution, partition, isFaultDomain_, true, replicaTreeTemp, nodeTreeBase))
    {
        return;
    }

    map<TreeNodeIndex, NodeSet> const& treeIndexMapWithNodeSet = isFaultDomain_ ?
        tempSolution.OriginalPlacement->BalanceCheckerObj->FaultDomainNodeSet : tempSolution.OriginalPlacement->BalanceCheckerObj->UpgradeDomainNodeSet;

    bool useDomainFilter = (candidateNodes.Count > treeIndexMapWithNodeSet.size()) ? true : false;

    if (useDomainFilter && treeIndexMapWithNodeSet.empty())
    {
        return;
    }

    NodeEntry const* currentNode = tempSolution.GetReplicaLocation(replica);

    TreeNodeIndex treeIndex = currentNode == nullptr ? TreeNodeIndex() :
        isFaultDomain_ ? currentNode->FaultDomainIndex : currentNode->UpgradeDomainIndex;

    if (nonPacking || !FaultDomainSubspace::IsQuorumBasedDomain(tempSolution, isFaultDomain_, service))
    {
        if (useDomainFilter)
        {
            FilterCandidateDomains(
                treeIndexMapWithNodeSet,
                replicaTreeTemp->Root,
                nodeTreeBase->Root,
                move(treeIndex),
                candidateNodes,
                nonPacking);
        }
        else
        {
            FilterCandidateNodes(
                replicaTreeTemp->Root,
                nodeTreeBase->Root,
                move(treeIndex),
                candidateNodes,
                nonPacking);
        }
    }
    else
    {
        if (useDomainFilter)
        {
            FilterQuorumCandidateDomains(
                treeIndexMapWithNodeSet,
                replicaTreeBase,
                replicaTreeTemp,
                nodeTreeBase,
                move(treeIndex),
                candidateNodes,
                partition);
        }
        else
        {
            FilterQuorumCandidateNodes(
                replicaTreeBase,
                replicaTreeTemp,
                nodeTreeBase,
                move(treeIndex),
                candidateNodes,
                partition);
        }
    }
}

void FaultDomainSubspace::FilterCandidateDomains(
    map<TreeNodeIndex, NodeSet> const& treeIndexMapWithNodeSet,
    PartitionDomainTree::Node const& replicaTree,
    BalanceChecker::DomainTree::Node const& nodeTree,
    TreeNodeIndex && currentDomain,
    NodeSet & candidateNodes,
    bool nonPacking) const
{
    ASSERT_IFNOT(replicaTree.Children.size() == nodeTree.Children.size(), "Tree nodes should have same structure");
    bool isUnplacedReplica = (currentDomain.Length == 0);

    TreeNodeIndex const* sourceDomainIndex = nullptr;
    size_t sourceDomainParentLevel = 0;
    bool focusSourceDomain = false;

    NodeSet validNodes(candidateNodes.OriginalPlacement, false);

    for (auto it = treeIndexMapWithNodeSet.begin(); it != treeIndexMapWithNodeSet.end(); ++it)
    {
        TreeNodeIndex const& candidateNodeIndex = it->first;

        bool isValidDomain = IsValidDomain(
            candidateNodeIndex,
            replicaTree,
            nodeTree,
            currentDomain,
            isUnplacedReplica,
            nonPacking,
            focusSourceDomain,
            &sourceDomainIndex,
            sourceDomainParentLevel);

        if (isValidDomain)
        {
            validNodes.SimpleUnion(it->second);
        }
    }

    candidateNodes.SimpleIntersect(validNodes);

    if (focusSourceDomain)
    {
        candidateNodes.Filter([&](NodeEntry const *node) -> bool
        {
            for (size_t i = 0; i < sourceDomainParentLevel && i < sourceDomainIndex->Length; ++i)
            {
                size_t candidateNodeSegment = isFaultDomain_ ?
                    node->FaultDomainIndex.GetSegment(i) : node->UpgradeDomainIndex.GetSegment(i);

                if (sourceDomainIndex->GetSegment(i) != candidateNodeSegment)
                {
                    return false;
                }
            }

            return true;
        });
    }

}

void FaultDomainSubspace::FilterCandidateNodes(
    PartitionDomainTree::Node const& replicaTree,
    BalanceChecker::DomainTree::Node const& nodeTree,
    TreeNodeIndex && currentDomain,
    NodeSet & candidateNodes,
    bool nonPacking) const
{
    ASSERT_IFNOT(replicaTree.Children.size() == nodeTree.Children.size(), "Tree nodes should have same structure");
    bool isUnplacedReplica = (currentDomain.Length == 0);

    TreeNodeIndex const* sourceDomainIndex = nullptr;
    size_t sourceDomainParentLevel = 0;
    bool focusSourceDomain = false;

    candidateNodes.Filter([&](NodeEntry const *node) -> bool
    {
        TreeNodeIndex const& candidateNodeIndex = isFaultDomain_ ? node->FaultDomainIndex : node->UpgradeDomainIndex;
        ASSERT_IF(candidateNodeIndex.Length == 0, "Domain candidateNodeIndex should not be empty");

        return IsValidDomain(
            candidateNodeIndex,
            replicaTree,
            nodeTree,
            currentDomain,
            isUnplacedReplica,
            nonPacking,
            focusSourceDomain,
            &sourceDomainIndex,
            sourceDomainParentLevel
            );
    });

    if (focusSourceDomain)
    {
        candidateNodes.Filter([&](NodeEntry const *node) -> bool
        {
            for (size_t i = 0; i < sourceDomainParentLevel && i < sourceDomainIndex->Length; ++i)
            {
                size_t candidateNodeSegment = isFaultDomain_ ?
                    node->FaultDomainIndex.GetSegment(i) : node->UpgradeDomainIndex.GetSegment(i);

                if (sourceDomainIndex->GetSegment(i) != candidateNodeSegment)
                {
                    return false;
                }
            }

            return true;
        });
    }
}

bool FaultDomainSubspace::IsValidDomain(
    TreeNodeIndex const& candidateNodeIndex,
    PartitionDomainTree::Node const& replicaTree,
    BalanceChecker::DomainTree::Node const& nodeTree,
    TreeNodeIndex const& currentDomain,
    bool isUnplacedReplica,
    bool nonPacking,
    bool & focusSourceDomain,
    TreeNodeIndex const ** sourceDomainIndex,
    size_t & sourceDomainParentLevel
    ) const
{
    bool isValidDomain = true;
    if (nonPacking)
    {
        // Keep max 1 replica per domain
        size_t segment = candidateNodeIndex.GetSegment(0);
        ASSERT_IFNOT(segment < replicaTree.Children.size(), "Index error");

        if (nodeTree.Children[segment].Data.NodeCount == 0)
        {
            isValidDomain = false;
        }
        else if (replicaTree.Children[segment].Data.ReplicaCount > ((candidateNodeIndex == currentDomain) ? 1 : 0))
        {
            isValidDomain = false;
        }
    }
    else
    {
        // Keep number of replicas per domain between floor and Ceil of value numberOfPartition'sReplicas / numberOfPartition'sDomains
        PartitionDomainTree::Node const* currentReplicaTree = &(replicaTree);
        BalanceChecker::DomainTree::Node const* currentNodeTree = &(nodeTree);

        bool sameDomain = isUnplacedReplica ? false : true;
        bool sameParentDomain = sameDomain;
        for (size_t i = 0; i < candidateNodeIndex.Length; ++i)
        {
            size_t targetSegment = candidateNodeIndex.GetSegment(i);
            size_t sourceSegment = isUnplacedReplica ? 0 : currentDomain.GetSegment(i);

            sameParentDomain = sameDomain;
            if (sameDomain && targetSegment != sourceSegment)
            {
                sameDomain = false;
            }

            ASSERT_IFNOT(currentReplicaTree->Children.size() == currentNodeTree->Children.size() &&
                targetSegment < currentReplicaTree->Children.size(), "Index error");

            size_t targetChildDomainExistingReplicas = currentReplicaTree->Children[targetSegment].Data.ReplicaCount;
            size_t targetChildDomainExistingNodes = currentNodeTree->Children[targetSegment].Data.NodeCount;

            // If target domain doesn't have nodes ==> don't allow the move or replica add
            if (targetChildDomainExistingNodes == 0)
            {
                isValidDomain = false;
                break;
            }

            // If target domain doesn't have replicas ==> skip checks for sub-domains and allow the move or replica add
            if (targetChildDomainExistingReplicas == 0)
            {
                break;
            }

            size_t currentDomainCount = count_if(
                currentNodeTree->Children.begin(),
                currentNodeTree->Children.end(),
                [](BalanceChecker::DomainTree::Node const& n)
            {
                return n.Data.NodeCount > 0;
            });

            // If child sub-domains are equally filled up it is only allowed to add new replicas to parent domain.
            // In case we have only one child sub-domain we should continue investigating replicas distribution across child domains.
            if (currentDomainCount == 1)
            {
                currentReplicaTree = &(currentReplicaTree->Children[targetSegment]);
                currentNodeTree = &(currentNodeTree->Children[targetSegment]);
                continue;
            }

            size_t currentReplicaCount = currentReplicaTree->Data.ReplicaCount;

            for (size_t j = 0; j < currentReplicaTree->Children.size(); ++j)
            {
                if (currentNodeTree->Children[j].Data.NodeCount == 0)
                {
                    //we need to remove these replicas from the count because they should not be used in the calculations
                    //these will be considered by other constraints
                    currentReplicaCount -= currentReplicaTree->Children[j].Data.ReplicaCount;
                }
            }

            ASSERT_IFNOT(currentDomainCount > 0,
                "Invalid currentDomainCount at the beginning");

            size_t baseCeil = static_cast<size_t>(ceil((double)currentReplicaCount / currentDomainCount));
            size_t baseFloor = static_cast<size_t>(floor((double)currentReplicaCount / currentDomainCount));

            // Find underloaded child domain - if domain has valid nodes and if number of replicas is smaller than floor
            bool existDomainWithLessThanFloorReplicas = false;
            size_t childDomainIndex = 0;
            currentReplicaTree->ForEachChild([&](PartitionDomainTree::Node const& n)
            {
                if (currentNodeTree->Children[childDomainIndex].Data.NodeCount > 0 &&
                    n.Data.ReplicaCount < baseFloor)
                {
                    existDomainWithLessThanFloorReplicas = true;
                }
                childDomainIndex++;
            });

            // Is move allowed based on source domain state?
            if (!focusSourceDomain && existDomainWithLessThanFloorReplicas && sameParentDomain)
            {
                *sourceDomainIndex = &candidateNodeIndex;
                sourceDomainParentLevel = i;
                focusSourceDomain = true;
            }

            // Move-out to the target domain which is under the same parent as source domain is not allowed if
            // source domain will be underloaded after the move.
            if (!isUnplacedReplica && sameParentDomain)
            {
                if (currentReplicaTree->Children[sourceSegment].Data.ReplicaCount <= baseFloor && !sameDomain)
                {
                    isValidDomain = false;
                    break;
                }
            }

            // Is move allowed based on target domain state?
            if (targetChildDomainExistingReplicas > baseCeil ||
                targetChildDomainExistingReplicas >= baseFloor && existDomainWithLessThanFloorReplicas)
            {
                isValidDomain = false;
                break;
            }
            else if (targetChildDomainExistingReplicas == baseCeil)
            {
                // Move inside the same domain are allowed.
                if (!sameDomain)
                {
                    // We allow only next 2 things in case where every child domain has the same number of replicas:
                    //  - adding new replica
                    //  - moving replica to target domain outside of parent domain
                    // Everything else is not valid.
                    if (!(baseCeil == baseFloor && (isUnplacedReplica || !sameParentDomain)))
                    {
                        isValidDomain = false;
                        break;
                    }
                }
            }

            currentReplicaTree = &(currentReplicaTree->Children[targetSegment]);
            currentNodeTree = &(currentNodeTree->Children[targetSegment]);
        }
    }

    return isValidDomain;

}

bool FaultDomainSubspace::IsValidQuorumDomain(
    Common::TreeNodeIndex const& candidateNodeIndex,
    PartitionDomainTree const* replicaTreeBase,
    PartitionDomainTree const* replicaTreeTemp,
    BalanceChecker::DomainTree const* nodeTree,
    TreeNodeIndex const& currentDomain,
    PartitionEntry const* partition) const
{
    bool isUnplacedReplica = (currentDomain.Length == 0);
    ASSERT_IF(candidateNodeIndex.Length == 0, "Domain candidateNodeIndex should not be empty");

    size_t level = 0;
    size_t targetSegment = 0;
    size_t childDomainsCount = 0;
    size_t targetDomainReplicasTemp = 0;
    size_t targetDomainReplicasBase = 0;

    if (!FaultDomainConstraint::GetFirstChildSubDomain(
        replicaTreeBase,
        replicaTreeTemp,
        nodeTree,
        candidateNodeIndex,
        level,
        targetSegment,
        childDomainsCount,
        targetDomainReplicasTemp,
        targetDomainReplicasBase))
    {
        // all levels have only one sub-domain
        return true;
    }

    size_t replicaLimit = partition->MaxReplicasPerDomain;

    // In case we don't have enough domains to place all needed replicas, we should allow one more replica per domain
    // Examples:
    //    - targetReplicaCount = 4, allowedDomainsCount = 3 -> it is OK if one domain has 2 replicas
    //    - targetReplicaCount = X, allowedDomainsCount = 2 -> it is OK if one domain has X - (Floor(X/2) + 1) + 1 = Ceil(X/2) replicas
    if (childDomainsCount * replicaLimit < partition->TargetReplicaSetSize)
    {
        ++replicaLimit;
    }

    if (isUnplacedReplica)
    {
        // no relax mode
        ++targetDomainReplicasTemp;
    }
    else
    {
        if (relaxed_)
        {
            replicaLimit = max(targetDomainReplicasBase, replicaLimit);
        }

        if (currentDomain.GetSegment(level) != targetSegment)
        {
            // inter-domain move
            ++targetDomainReplicasTemp;
        }
    }

    if (targetDomainReplicasTemp > replicaLimit)
    {
        return false;
    }

    return true;
}

void FaultDomainSubspace::FilterQuorumCandidateNodes(
    PartitionDomainTree const* replicaTreeBase,
    PartitionDomainTree const* replicaTreeTemp,
    BalanceChecker::DomainTree const* nodeTree,
    TreeNodeIndex && currentDomain,
    NodeSet & candidateNodes,
    PartitionEntry const* partition) const
{
    ASSERT_IFNOT(replicaTreeTemp->Root.Children.size() == nodeTree->Root.Children.size(), "Tree nodes should have same structure");

    candidateNodes.Filter([&](NodeEntry const *node) -> bool
    {
        TreeNodeIndex const& candidateNodeIndex = isFaultDomain_ ? node->FaultDomainIndex : node->UpgradeDomainIndex;
        ASSERT_IF(candidateNodeIndex.Length == 0, "Domain candidateNodeIndex should not be empty");

        return IsValidQuorumDomain(
            candidateNodeIndex,
            replicaTreeBase,
            replicaTreeTemp,
            nodeTree,
            currentDomain,
            partition);
    });
}

void FaultDomainSubspace::FilterQuorumCandidateDomains(
    map<TreeNodeIndex, NodeSet> const& treeIndexMapWithNodeSet,
    PartitionDomainTree const* replicaTreeBase,
    PartitionDomainTree const* replicaTreeTemp,
    BalanceChecker::DomainTree const* nodeTree,
    TreeNodeIndex && currentDomain,
    NodeSet & candidateNodes,
    PartitionEntry const* partition) const
{
    ASSERT_IFNOT(replicaTreeTemp->Root.Children.size() == nodeTree->Root.Children.size(), "Tree nodes should have same structure");

    NodeSet validNodes(candidateNodes.OriginalPlacement, false);

    for (auto it = treeIndexMapWithNodeSet.begin(); it != treeIndexMapWithNodeSet.end(); ++it)
    {
        TreeNodeIndex const& candidateNodeIndex = it->first;

        bool isValidDomain = IsValidQuorumDomain(
            candidateNodeIndex,
            replicaTreeBase,
            replicaTreeTemp,
            nodeTree,
            currentDomain,
            partition);

        if (isValidDomain)
        {
            validNodes.SimpleUnion(it->second);
        }
    }

    candidateNodes.SimpleIntersect(validNodes);

}

void FaultDomainSubspace::GetNodesForReplicaDrop(
    TempSolution const& tempSolution,
    PartitionEntry const& partition,
    NodeSet & candidateNodes) const
{
    ServiceEntry const* service = partition.Service;

    if (service->OnEveryNode || (isFaultDomain_ && service->FDDistribution == Service::Type::Ignore))
    {
        return;
    }

    if (isFaultDomain_ && partition.Service->FDDistribution == Service::Type::Nonpacking)
    {
        // In this case we can have 0 or 1 replica per domain, so we can drop any of them.
        return;
    }

    // if partition should follow quorum based replica distribution - any replica could be dropped
    if (FaultDomainSubspace::IsQuorumBasedDomain(tempSolution, isFaultDomain_, service))
    {
        return;
    }

    PartitionDomainTree const* replicaTreeTemp = nullptr;
    BalanceChecker::DomainTree const* nodeTreeTemp = nullptr;
    FaultDomainConstraint::GetTempTrees(tempSolution, &partition, isFaultDomain_, false, replicaTreeTemp, nodeTreeTemp);

    ASSERT_IF(nodeTreeTemp->IsEmpty, "Domain structures are empty");

    FilterCandidateNodesForDrop(replicaTreeTemp->Root, nodeTreeTemp->Root, candidateNodes, partition);
}

void FaultDomainSubspace::FilterCandidateNodesForDrop(
    PartitionDomainTree::Node const& replicaTree,
    BalanceChecker::DomainTree::Node const& nodeTree,
    NodeSet & candidateNodes,
    PartitionEntry const& partition) const
{
    // drop replicas from domains with replica count > base (or from any if all domains have equal count)
    ASSERT_IFNOT(replicaTree.Children.size() == nodeTree.Children.size(), "Tree nodes should have same structure");

    candidateNodes.Filter([&](NodeEntry const *node) -> bool
    {
        TreeNodeIndex const& index = isFaultDomain_ ? node->FaultDomainIndex : node->UpgradeDomainIndex;
        ASSERT_IF(index.Length == 0, "Domain index should not be empty");

        // If node doesn't have replica that could be dropped, node should not be filtered out from candidate nodes list
        if (partition.GetReplica(node) == nullptr)
        {
            return true;
        }

        bool isValidDomain = true;
        PartitionDomainTree::Node const* currentReplicaTree = &replicaTree;
        BalanceChecker::DomainTree::Node const* currentNodeTree = &nodeTree;
        size_t parentReplicaCount = currentReplicaTree->Data.ReplicaCount;
        size_t parentDomainCount = count_if(currentNodeTree->Children.begin(), currentNodeTree->Children.end(), [](BalanceChecker::DomainTree::Node const& n)
        {
            return n.Data.NodeCount > 0;
        });

        for (size_t i = 0; i < index.Length; ++i)
        {
            size_t segment = index.GetSegment(i);

            ASSERT_IFNOT(currentReplicaTree->Children.size() == currentNodeTree->Children.size() && segment < currentReplicaTree->Children.size(), "Index error");
            currentReplicaTree = &(currentReplicaTree->Children[segment]);
            currentNodeTree = &(currentNodeTree->Children[segment]);

            // If there are no eligible nodes in this domain, any replicas left on any of them should be good for drop
            if (currentNodeTree->Data.NodeCount == 0)
            {
                isValidDomain = true;
                break;
            }

            if (currentReplicaTree->Data.ReplicaCount == 0)
            {
                break;
            }

            ASSERT_IFNOT(parentReplicaCount > 0 && parentDomainCount > 0, "Invalid parentReplicaCount or parentDomainCount");

            size_t base = parentReplicaCount / parentDomainCount;
            bool allEqual = (0 == parentReplicaCount % parentDomainCount);

            if (!allEqual && currentReplicaTree->Data.ReplicaCount <= base)
            {
                // We cannot drop from this domain, since there are other domains with more replicas.
                isValidDomain = false;
                break;
            }

            parentReplicaCount = currentReplicaTree->Data.ReplicaCount;
            parentDomainCount = count_if(currentNodeTree->Children.begin(), currentNodeTree->Children.end(), [](BalanceChecker::DomainTree::Node const& n)
            {
                return n.Data.NodeCount > 0;
            });
        }

        return isValidDomain;
    });
}

FaultDomainConstraint::FaultDomainConstraint(Enum name, int priority)
    : IDynamicConstraint(priority), domainName_(name)
{
}

IViolationUPtr FaultDomainConstraint::GetViolations(
    TempSolution const& tempSolution,
    bool changedOnly,
    bool relaxed,
    bool useNodeBufferCapacity,
    Random& random) const
{
    UNREFERENCED_PARAMETER(useNodeBufferCapacity);
    UNREFERENCED_PARAMETER(random);
    set<PartitionEntry const*> invalidPartitions;
    tempSolution.ForEachPartition(changedOnly, [&](PartitionEntry const* partition) -> bool
    {
        GetViolations(tempSolution, partition, IsFaultDomain, relaxed, invalidPartitions);

        return true;
    });

    return make_unique<PartitionSetViolation>(move(invalidPartitions));
}

bool FaultDomainConstraint::GetViolations(
    TempSolution const& tempSolution,
    PartitionEntry const* partition,
    bool isFaultDomain,
    bool relaxed,
    set<PartitionEntry const*> & invalidPartitions)
{
    ServiceEntry const* service = partition->Service;

    // if the service is for place on every node, skip the partition
    if (service->OnEveryNode || (isFaultDomain && service->FDDistribution == Service::Type::Ignore))
    {
        return true;
    }

    bool nonPacking = isFaultDomain && service->FDDistribution == Service::Type::Nonpacking;
    PartitionDomainTree const* replicaTreeBase = nullptr;
    BalanceChecker::DomainTree const* nodeTreeBase = nullptr;
    FaultDomainConstraint::GetBaseTrees(tempSolution, partition, isFaultDomain, false, replicaTreeBase, nodeTreeBase);

    if (nonPacking || !FaultDomainSubspace::IsQuorumBasedDomain(tempSolution, isFaultDomain, service))
    {
        if (relaxed && HasDomainViolation(replicaTreeBase->Root, nodeTreeBase->Root, nonPacking))
        {
            // if the base solution already violate the fault domain, skip it
            return true;
        }
    }

    PartitionDomainTree const* replicaTreeTemp = nullptr;
    FaultDomainConstraint::GetTempTrees(tempSolution, partition, isFaultDomain, false, replicaTreeTemp, nodeTreeBase);

    if (nonPacking || !FaultDomainSubspace::IsQuorumBasedDomain(tempSolution, isFaultDomain, service))
    {
        if (HasDomainViolation(replicaTreeTemp->Root, nodeTreeBase->Root, nonPacking))
        {
            invalidPartitions.insert(partition);
        }
    }
    else
    {
        if (HasQuorumDomainViolation(
            replicaTreeBase,
            replicaTreeTemp,
            partition,
            nodeTreeBase,
            relaxed,
            [&](vector<PlacementReplica const*> const& replicas, size_t extraReplicaCount) -> bool
        {
            UNREFERENCED_PARAMETER(replicas);
            UNREFERENCED_PARAMETER(extraReplicaCount);
            return false;
        }))
        {
            invalidPartitions.insert(partition);
        }
    }

    return true;
}

ConstraintCheckResult FaultDomainConstraint::CheckSolutionAndGenerateSubspace(
    TempSolution const& tempSolution,
    bool changedOnly,
    bool relaxed,
    bool useNodeBufferCapacity,
    Random & random,
    std::shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr) const
{
    UNREFERENCED_PARAMETER(useNodeBufferCapacity);

    PlacementReplicaSet invalidReplicas;

    tempSolution.ForEachPartition(changedOnly, [&](PartitionEntry const* partition) -> bool
    {
        GetInvalidReplicas(tempSolution, partition, IsFaultDomain, relaxed, random, diagnosticsDataPtr, invalidReplicas);

        return true;
    });

    //Diagnostics
    if (diagnosticsDataPtr != nullptr)
    {
        auto fD = std::dynamic_pointer_cast<FaultDomainConstraintDiagnosticsData>(diagnosticsDataPtr);
        auto uD = std::dynamic_pointer_cast<UpgradeDomainConstraintDiagnosticsData>(diagnosticsDataPtr);

        diagnosticsDataPtr->changed_ = !invalidReplicas.empty();
        TESTASSERT_IF(!((fD != nullptr) || (uD != nullptr)), "Invalid Constraint Type for Static Constraint Diagnostics");

        for (auto const & r : invalidReplicas)
        {
            BasicDiagnosticInfo info;

            info.serviceName_ = r->Partition->Service->Name;
            info.AddPartition(r->Partition->PartitionId);
            info.AddNode(r->Node->NodeId,
                (diagnosticsDataPtr->plbNodesPtr_ != nullptr)
                ? (*(diagnosticsDataPtr->plbNodesPtr_)).at(r->Node->NodeIndex).NodeDescriptionObj.NodeName
                : L"");

            if (IsFaultDomain && (fD != nullptr))
            {
                fD->AddBasicDiagnosticsInfo(move(info));
            }
            else if (uD != nullptr)
            {
                uD->AddBasicDiagnosticsInfo(move(info));
            }
        }
    }

    return ConstraintCheckResult(make_unique<FaultDomainSubspace>(IsFaultDomain, relaxed), move(invalidReplicas));
}

void FaultDomainConstraint::GetInvalidReplicas(
    TempSolution const& tempSolution,
    PartitionEntry const* partition,
    bool isFaultDomain,
    bool relaxed,
    Random & random,
    shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr,
    PlacementReplicaSet & invalidReplicas)
{
    // if the service is for place on every node, skip the partition
    if (partition->Service->OnEveryNode || (isFaultDomain && partition->Service->FDDistribution == Service::Type::Ignore))
    {
        return;
    }

    bool nonPacking = isFaultDomain && partition->Service->FDDistribution == Service::Type::Nonpacking;
    PartitionDomainTree const* replicaTreeBase = nullptr;
    BalanceChecker::DomainTree const* nodeTreeBase = nullptr;
    FaultDomainConstraint::GetBaseTrees(tempSolution, partition, isFaultDomain, false, replicaTreeBase, nodeTreeBase);

    if (nonPacking || !FaultDomainSubspace::IsQuorumBasedDomain(tempSolution, isFaultDomain, partition->Service))
    {
        if (relaxed && HasDomainViolation(replicaTreeBase->Root, nodeTreeBase->Root, nonPacking))
        {
            // if the base solution already violate the fault domain, skip it
            return;
        }
    }

    PartitionDomainTree const* replicaTreeTemp = nullptr;
    if (!FaultDomainConstraint::GetTempTrees(tempSolution, partition, isFaultDomain, true, replicaTreeTemp, nodeTreeBase))
    {
        return;
    }

    if (nonPacking || !FaultDomainSubspace::IsQuorumBasedDomain(tempSolution, isFaultDomain, partition->Service))
    {
        GetInvalidReplicas(replicaTreeTemp->Root, nodeTreeBase->Root, invalidReplicas, 0, random, nonPacking, diagnosticsDataPtr);
    }
    else
    {
        GetQuorumInvalidReplicas(
            replicaTreeBase,
            replicaTreeTemp,
            partition,
            nodeTreeBase,
            invalidReplicas,
            relaxed,
            random,
            diagnosticsDataPtr);
    }
}

bool FaultDomainConstraint::HasDomainViolation(
    PartitionDomainTree::Node const& replicaTree,
    BalanceChecker::DomainTree::Node const& nodeTree,
    bool nonPacking)
{
    ASSERT_IFNOT(replicaTree.Children.size() == nodeTree.Children.size(), "Tree nodes should have same structure");

    if (replicaTree.Children.empty())
    {
        return false;
    }

    size_t replicaCount = replicaTree.Data.ReplicaCount;

    if (replicaCount <= 1)
    {
        return false;
    }

    bool violation = false;
    if (nonPacking)
    {
        for (size_t i = 0; i < replicaTree.Children.size(); ++i)
        {
            if (nodeTree.Children[i].Data.NodeCount > 0 &&
                replicaTree.Children[i].Data.ReplicaCount > 1)
            {
                violation = true;
                break;
            }
        }
    }
    else
    {
        size_t domainCount = count_if(nodeTree.Children.begin(), nodeTree.Children.end(), [](BalanceChecker::DomainTree::Node const& n)
        {
            return (n.Data.NodeCount > 0);
        });

        if (domainCount == 0)
        {
            return false;
        }

        for (size_t i = 0; i < replicaTree.Children.size(); ++i)
        {
            if (nodeTree.Children[i].Data.NodeCount == 0)
            {
                //we need to remove these replicas from the count because they should not be used in the calculations
                //these will be considered by other constraints
                replicaCount -= replicaTree.Children[i].Data.ReplicaCount;
            }
        }

        size_t base = replicaCount / domainCount;

        for (size_t i = 0; i < replicaTree.Children.size(); ++i)
        {
            size_t currentReplicaCount = replicaTree.Children[i].Data.ReplicaCount;

            if (nodeTree.Children[i].Data.NodeCount == 0)
            {
                //do nothing fault domain should not report this
                //this can only happen if the whole domain is block-listed or deactivated
                continue;
            }

            if (currentReplicaCount > base + 1 || currentReplicaCount < base)
            {
                violation = true;
                break;
            }

            if (HasDomainViolation(replicaTree.Children[i], nodeTree.Children[i], false))
            {
                violation = true;
                break;
            }
        }
    }
    return violation;
}


bool FaultDomainConstraint::HasQuorumDomainViolation(
    PartitionDomainTree const* replicaTreeBase,
    PartitionDomainTree const* replicaTreeTemp,
    PartitionEntry const* partition,
    BalanceChecker::DomainTree const* nodeTree,
    bool relaxed,
    function<bool(vector<PlacementReplica const*> const&, size_t)> processor)
{
    PartitionDomainTree::Node const* currentReplicaTreeBase = &(replicaTreeBase->Root);
    PartitionDomainTree::Node const* currentReplicaTreeTemp = &(replicaTreeTemp->Root);
    BalanceChecker::DomainTree::Node const* currentNodeTree = &(nodeTree->Root);

    size_t childDomainsCount = 0;

    do
    {
        if (nullptr == currentNodeTree)
        {
            return false;
        }

        size_t lastChildWithNodes = 0;
        childDomainsCount = 0;
        for (size_t childId = 0; childId < currentNodeTree->Children.size(); childId++)
        {
            if (currentNodeTree->Children[childId].Data.NodeCount != 0)
            {
                childDomainsCount++;
                lastChildWithNodes = childId;
            }
        }

        if (childDomainsCount == 1)
        {
            currentReplicaTreeBase = &(currentReplicaTreeBase->Children[lastChildWithNodes]);
            currentReplicaTreeTemp = &(currentReplicaTreeTemp->Children[lastChildWithNodes]);
            currentNodeTree = &(currentNodeTree->Children[lastChildWithNodes]);
        }

    } while (childDomainsCount == 1);

    if (0 == childDomainsCount)
    {
        // all levels have only one sub-domain
        return false;
    }

    bool hasViolation = false;
    auto childBaseIt = currentReplicaTreeBase->Children.begin();

    for (auto childTempIt = currentReplicaTreeTemp->Children.begin();
        childTempIt != currentReplicaTreeTemp->Children.end();
        ++childTempIt)
    {
        TESTASSERT_IF(childBaseIt == currentReplicaTreeBase->Children.end(),
            "Temp and base partition domain trees have different children sub-domains");

        size_t replicaLimit = partition->MaxReplicasPerDomain;

        // In case we don't have enough domains to place all needed replicas, we should allow one more replica per domain
        // Examples:
        //    - targetReplicaCount = 4, allowedDomainsCount = 3 -> it is OK if one domain has 2 replicas
        //    - targetReplicaCount = X, allowedDomainsCount = 2 -> it is OK if one domain has X - (Floor(X/2) + 1) + 1 = Ceil(X/2) replicas
        if (childDomainsCount * replicaLimit < partition->TargetReplicaSetSize)
        {
            ++replicaLimit;
        }

        if (relaxed)
        {
            replicaLimit = max(childBaseIt->Data.ReplicaCount, replicaLimit);
        }

        if (childTempIt->Data.ReplicaCount > replicaLimit)
        {
            hasViolation = true;
            std::vector<PlacementReplica const*> invalidCandidates;

            childTempIt->ForEachNodeInPreOrder([&](PartitionDomainTree::Node const& domain)
            {
                if (domain.Children.size() == 0)
                {
                    for (auto replicaIt = domain.Data.Replicas.begin(); replicaIt != domain.Data.Replicas.end(); ++replicaIt)
                    {
                        invalidCandidates.push_back(*replicaIt);
                    }
                }
            });

            if (!processor(invalidCandidates, childTempIt->Data.ReplicaCount - replicaLimit))
            {
                break;
            }
        }

        ++childBaseIt;
    }

    return hasViolation;
}

void FaultDomainConstraint::GetInvalidReplicas(
    PartitionDomainTree::Node const& replicaTree,
    BalanceChecker::DomainTree::Node const& nodeTree,
    PlacementReplicaSet & invalidReplicas,
    size_t countToRemove, Random & random,
    bool nonPacking,
    std::shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr)
{
    // This method return the replicas that should be removed from the subtree in order to correct fault/upgrade domain violation
    // The countToRemove is the number of replicas to be removed because of upper level domain
    // Do not take into consideration replicas that are on invalid nodes unless their domain is invalid
    size_t replicaCount = replicaTree.Data.ReplicaCount;
    ASSERT_IFNOT(replicaCount >= countToRemove && replicaCount > 0, "Replica count should be >= to be removed replica and should be > 0");
    ASSERT_IFNOT(replicaTree.Children.size() == nodeTree.Children.size(), "Tree nodes should have same structure");

    if (replicaTree.Children.empty())
    {
        // it's the leaf node, get the replicas
        vector<PlacementReplica const*> const& replicas = replicaTree.Data.Replicas;
        ASSERT_IFNOT(replicaCount == replicas.size(), "Replica size doesn't match");
        GetReplicas(replicas, invalidReplicas, countToRemove, random);
        return;
    }

    if (replicaCount == countToRemove)
    {
        // if all replicas should be removed
        for (size_t i = 0; i < nodeTree.Children.size(); ++i)
        {
            size_t count = replicaTree.Children[i].Data.ReplicaCount;
            if (count > 0)
            {
                GetInvalidReplicas(replicaTree.Children[i], nodeTree.Children[i], invalidReplicas, count, random, false);
            }
        }
        return;
    }

    if (nonPacking)
    {
        // it's non packing and it's the first level of domain
        ASSERT_IFNOT(countToRemove == 0, "countToRemove should be 0");
        for (size_t i = 0; i < nodeTree.Children.size(); ++i)
        {
            // remove those replicas in those invalid domains
            size_t count = replicaTree.Children[i].Data.ReplicaCount;

            // fault domain violation should only trigger when node count > 0
            if (nodeTree.Children[i].Data.NodeCount > 0)
            {
                if (count > 1)
                {
                    GetInvalidReplicas(replicaTree.Children[i], nodeTree.Children[i], invalidReplicas, count - 1, random, false);
                }
            }
        }
    }

    int validDomainIndex = -1; // the index of the last valid domain
    size_t domainCount = 0;
    size_t invalidDomainReplicaCount = 0;
    PlacementReplicaSet zeroNodeReplicas;

    for (size_t i = 0; i < nodeTree.Children.size(); ++i)
    {
        if (nodeTree.Children[i].Data.NodeCount > 0)
        {
            ++domainCount;
            validDomainIndex = static_cast<int>(i);
        }
        else
        {
            size_t count = replicaTree.Children[i].Data.ReplicaCount;
            if (count > 0)
            {
                invalidDomainReplicaCount += count;
                GetInvalidReplicas(replicaTree.Children[i], nodeTree.Children[i], zeroNodeReplicas, count, random, false);
            }
        }
    }

    replicaCount -= invalidDomainReplicaCount;

    auto invalidReplicaIterator = zeroNodeReplicas.begin();

    //first remove the invalid replicas from count always
    while (countToRemove > 0 && invalidDomainReplicaCount > 0 && invalidReplicaIterator != zeroNodeReplicas.end())
    {
        invalidReplicas.insert(*invalidReplicaIterator);
        ++invalidReplicaIterator;
        --countToRemove;
        --invalidDomainReplicaCount;
    }

    if (replicaCount == 0 || replicaCount == 1 && countToRemove == 0)
    {
        return;
    }

    ASSERT_IFNOT(domainCount > 0 && replicaCount > countToRemove, "Domain count should be > 0, and replica count should > countToRemove");

    if (domainCount == 1)
    {
        ASSERT_IF(validDomainIndex < 0 || validDomainIndex >= static_cast<int>(replicaTree.Children.size()), "Invalid domainIndex value");
        GetInvalidReplicas(replicaTree.Children[validDomainIndex], nodeTree.Children[validDomainIndex], invalidReplicas, countToRemove, random, false);
    }
    else if (countToRemove == 0 && replicaCount <= domainCount)
    {
        for (size_t i = 0; i < replicaTree.Children.size(); ++i)
        {
            if (nodeTree.Children[i].Data.NodeCount == 0)
            {
                continue;
            }

            size_t count = replicaTree.Children[i].Data.ReplicaCount;
            if (count > 1)
            {
                GetInvalidReplicas(replicaTree.Children[i], nodeTree.Children[i], invalidReplicas, count - 1, random, false);
            }
        }
    }
    else
    {
        size_t overflowCount = 0, underflowCount = 0, highCount = 0;

        size_t base = (replicaCount - countToRemove) / domainCount;

        for (size_t i = 0; i < replicaTree.Children.size(); ++i)
        {
            if (nodeTree.Children[i].Data.NodeCount == 0)
            {
                continue;
            }

            size_t count = replicaTree.Children[i].Data.ReplicaCount;

            if (count < base)
            {
                underflowCount += base - count;
            }
            else if (count > base)
            {
                ++highCount;
                if (count > base + 1)
                {
                    overflowCount += count - base - 1;
                }
            }
        }

        size_t invalidCount = max(overflowCount, underflowCount + countToRemove);

        // first remove overflow count
        // then remove invalidCount - overflow count, randomly selected from highCount
        size_t numberToSelect = invalidCount - overflowCount;
        ASSERT_IFNOT(numberToSelect <= highCount, "numberToSelect should <= highCount");
        for (size_t i = 0; i < replicaTree.Children.size(); ++i)
        {
            if (nodeTree.Children[i].Data.NodeCount == 0)
            {
                continue;
            }

            size_t count = replicaTree.Children[i].Data.ReplicaCount;

            if (count == 0)
            {
                continue;
            }

            size_t diff = 0;
            if (count > base)
            {
                diff = count - base - 1;

                if (numberToSelect > 0 && random.NextDouble() < static_cast<double>(numberToSelect) / highCount)
                {
                    ++diff;
                    --numberToSelect;
                }

                --highCount;
            }

            GetInvalidReplicas(replicaTree.Children[i], nodeTree.Children[i], invalidReplicas, diff, random, false);
        }
    }
}

void FaultDomainConstraint::GetQuorumInvalidReplicas(
    PartitionDomainTree const* replicaTreeBase,
    PartitionDomainTree const* replicaTreeTemp,
    PartitionEntry const* partition,
    BalanceChecker::DomainTree const* nodeTree,
    PlacementReplicaSet & invalidReplicas,
    bool relaxed,
    Random & random,
    std::shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr)
{
    HasQuorumDomainViolation(
        replicaTreeBase,
        replicaTreeTemp,
        partition,
        nodeTree,
        relaxed,
        [&](vector<PlacementReplica const*> const& replicas, size_t extraReplicaCount) -> bool
    {
        GetReplicas(replicas, invalidReplicas, extraReplicaCount, random);

        return true;
    });
}

bool FaultDomainConstraint::GetFirstChildSubDomain(
    PartitionDomainTree const* replicaTreeBase,
    PartitionDomainTree const* replicaTreeTemp,
    BalanceChecker::DomainTree const* nodeTree,
    TreeNodeIndex const& candidateNodeIndex,
    size_t & level,
    size_t & targetSegment,
    size_t & childDomainsCount,
    size_t & replicaCountTemp,
    size_t & replicaCountBase)
{
    if (nullptr == replicaTreeTemp || nullptr == replicaTreeBase)
    {
        return false;
    }

    PartitionDomainTree::Node const* currentReplicaTreeBase = &(replicaTreeBase->Root);
    PartitionDomainTree::Node const* currentReplicaTreeTemp = &(replicaTreeTemp->Root);
    BalanceChecker::DomainTree::Node const* currentNodeTree = &(nodeTree->Root);

    level = 0;
    targetSegment = candidateNodeIndex.GetSegment(level);
    replicaCountTemp = 0;
    replicaCountBase = 0;

    do
    {
        if (nullptr == currentNodeTree)
        {
            return false;
        }

        if (!(level < candidateNodeIndex.Length))
        {
            break;
        }

        childDomainsCount = 0;
        for (size_t childId = 0; childId < currentNodeTree->Children.size(); childId++)
        {
            if (currentNodeTree->Children[childId].Data.NodeCount != 0)
            {
                childDomainsCount++;
            }
        }
        if (childDomainsCount == 1)
        {
            currentReplicaTreeBase = &(currentReplicaTreeBase->Children[targetSegment]);
            currentReplicaTreeTemp = &(currentReplicaTreeTemp->Children[targetSegment]);
            currentNodeTree = &(currentNodeTree->Children[targetSegment]);

            ++level;
            targetSegment = candidateNodeIndex.GetSegment(level);
        }

    } while (childDomainsCount == 1);

    if (0 == childDomainsCount || 1 == childDomainsCount)
    {
        // all levels have only one sub-domain
        return false;
    }

    currentReplicaTreeBase = &(currentReplicaTreeBase->Children[targetSegment]);
    currentReplicaTreeTemp = &(currentReplicaTreeTemp->Children[targetSegment]);
    replicaCountBase = currentReplicaTreeBase->Data.ReplicaCount;
    replicaCountTemp = currentReplicaTreeTemp->Data.ReplicaCount;

    return true;
}

bool FaultDomainConstraint::GetTempTrees(
    TempSolution const& tempSolution,
    PartitionEntry const* partition,
    bool isFaultDomain,
    bool validateReplicaCount,
    PartitionDomainTree const* &replicaTree,
    BalanceChecker::DomainTree const* &nodeTree)
{
    return GetTrees(
        tempSolution.FaultDomainStructures,
        tempSolution.UpgradeDomainStructures,
        tempSolution.OriginalPlacement,
        partition,
        isFaultDomain,
        validateReplicaCount,
        replicaTree,
        nodeTree);
}

bool FaultDomainConstraint::GetBaseTrees(
    TempSolution const& tempSolution,
    PartitionEntry const* partition,
    bool isFaultDomain,
    bool validateReplicaCount,
    PartitionDomainTree const* &replicaTree,
    BalanceChecker::DomainTree const* &nodeTree)
{
    CandidateSolution const& baseSolution = tempSolution.BaseSolution;

    bool returnValue = GetTrees(
        baseSolution.FaultDomainStructures,
        baseSolution.UpgradeDomainStructures,
        baseSolution.OriginalPlacement,
        partition,
        isFaultDomain,
        validateReplicaCount,
        replicaTree,
        nodeTree);

    ASSERT_IF(nodeTree->IsEmpty, "Node domain tree is empty");

    return returnValue;
}

bool FaultDomainConstraint::GetTempAndBaseTrees(
    TempSolution const& tempSolution,
    PartitionEntry const* partition,
    bool isFaultDomain,
    bool validateReplicaCount,
    PartitionDomainTree const* &replicaTempTree,
    PartitionDomainTree const* &replicaBaseTree,
    BalanceChecker::DomainTree const* &nodeTree)
{
    if (!GetReplicaTrees(
        tempSolution.FaultDomainStructures,
        tempSolution.UpgradeDomainStructures,
        partition,
        isFaultDomain,
        validateReplicaCount,
        replicaTempTree))
    {
        return false;
    }

    CandidateSolution const& baseSolution = tempSolution.BaseSolution;

    if (!GetReplicaTrees(
        baseSolution.FaultDomainStructures,
        baseSolution.UpgradeDomainStructures,
        partition,
        isFaultDomain,
        validateReplicaCount,
        replicaBaseTree))
    {
        return false;
    }

    GetNodeTrees(tempSolution.OriginalPlacement, partition, isFaultDomain, nodeTree);

    ASSERT_IF(nodeTree->IsEmpty, "Node domain tree is empty");

    return true;
}

bool FaultDomainConstraint::GetTrees(
    PartitionDomainStructure const& faultDomainStructures,
    PartitionDomainStructure const& upgradeDomainStructures,
    Placement const * originalPlacement,
    PartitionEntry const* partition,
    bool isFaultDomain,
    bool validateReplicaCount,
    PartitionDomainTree const* &replicaTree,
    BalanceChecker::DomainTree const* &nodeTree)
{
    if (!GetReplicaTrees(
        faultDomainStructures,
        upgradeDomainStructures,
        partition,
        isFaultDomain,
        validateReplicaCount,
        replicaTree))
    {
        return false;
    }

    if (nullptr == nodeTree)
    {
        GetNodeTrees(originalPlacement, partition, isFaultDomain, nodeTree);
    }

    return true;
}

bool FaultDomainConstraint::GetReplicaTrees(
    PartitionDomainStructure const& faultDomainStructures,
    PartitionDomainStructure const& upgradeDomainStructures,
    PartitionEntry const* partition,
    bool isFaultDomain,
    bool validateReplicaCount,
    PartitionDomainTree const* &replicaTree)
{
    if (isFaultDomain)
    {
        replicaTree = &faultDomainStructures[partition];
    }
    else
    {
        replicaTree = &upgradeDomainStructures[partition];
    }

    ASSERT_IF(replicaTree->IsEmpty, "Replica domain tree is empty");

    if (validateReplicaCount)
    {
        size_t replicaCount = replicaTree->Root.Data.ReplicaCount;

        if (replicaCount == 0)
        {
            return false;
        }
    }

    return true;
}

bool FaultDomainConstraint::GetNodeTrees(
    Placement const * originalPlacement,
    PartitionEntry const* partition,
    bool isFaultDomain,
    BalanceChecker::DomainTree const* &nodeTree)
{
    if (isFaultDomain)
    {
        nodeTree = partition->Service->FaultDomainStructure.IsEmpty ?
            &originalPlacement->BalanceCheckerObj->FaultDomainStructure :
            &partition->Service->FaultDomainStructure;
    }
    else
    {
        nodeTree = partition->Service->UpgradeDomainStructure.IsEmpty ?
            &originalPlacement->BalanceCheckerObj->UpgradeDomainStructure :
            &partition->Service->UpgradeDomainStructure;
    }

    return true;
}

void FaultDomainConstraint::GetReplicas(
    vector<PlacementReplica const*> const& replicas,
    PlacementReplicaSet & invalidReplicas,
    size_t countToRemove,
    Random & random)
{
    TESTASSERT_IF(replicas.size() < countToRemove, "Not enough replicas to remove.");

    size_t replicaCount = replicas.size();
    if (countToRemove == 0)
    {
        return;
    }
    else if (countToRemove == 1)
    {
        size_t indexToRemove = static_cast<size_t>(random.Next(static_cast<int>(replicaCount)));
        invalidReplicas.insert(replicas[indexToRemove]);
    }
    else if (countToRemove == replicaCount - 1)
    {
        size_t indexToKeep = static_cast<size_t>(random.Next(static_cast<int>(replicaCount)));

        for (size_t i = 0; i < replicaCount; ++i)
        {
            if (i != indexToKeep)
            {
                invalidReplicas.insert(replicas[i]);
            }
        }
    }
    else if (countToRemove == replicaCount)
    {
        for (size_t i = 0; i < replicaCount; ++i)
        {
            invalidReplicas.insert(replicas[i]);
        }
    }
    else
    {
        int i = 0;
        int size = static_cast<int>(replicas.size());
        while (countToRemove > 0 && i < size)
        {
            if (random.NextDouble() < static_cast<double>(countToRemove) / (size - i))
            {
                invalidReplicas.insert(replicas[i]);
                --countToRemove;
            }
            ++i;
        }
    }
}

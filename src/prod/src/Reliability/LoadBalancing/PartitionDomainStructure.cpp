// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Movement.h"
#include "PartitionDomainStructure.h"
#include "ServiceEntry.h"
#include "Service.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

PartitionDomainStructure::PartitionDomainStructure(bool isFaultDomain)
    : LazyMap<PartitionEntry const*, PartitionDomainTree>(PartitionDomainTree()), isFaultDomain_(isFaultDomain)
{
}

PartitionDomainStructure::PartitionDomainStructure(PartitionDomainStructure && other)
    : LazyMap<PartitionEntry const*, PartitionDomainTree>(move(other)), isFaultDomain_(other.isFaultDomain_)
{
}

PartitionDomainStructure::PartitionDomainStructure(PartitionDomainStructure const& other)
    : LazyMap<PartitionEntry const*, PartitionDomainTree>(other), isFaultDomain_(other.isFaultDomain_)
{
}

PartitionDomainStructure & PartitionDomainStructure::operator = (PartitionDomainStructure && other)
{
    return (PartitionDomainStructure &)LazyMap<PartitionEntry const*, PartitionDomainTree>::operator = (move(other));
}

PartitionDomainStructure::PartitionDomainStructure(PartitionDomainStructure const* basePlacement, bool isFaultDomain)
    : LazyMap<PartitionEntry const*, PartitionDomainTree>(basePlacement), isFaultDomain_(isFaultDomain)
{
}

void PartitionDomainStructure::ChangeMovement(Movement const& oldMovement, Movement const& newMovement)
{
    if (oldMovement.IsValid)
    {
        ServiceEntry const* service = oldMovement.Partition->Service;

        if (!(service->OnEveryNode || isFaultDomain_ && service->FDDistribution == Service::Type::Ignore))
        {
            PartitionDomainTree & partitionDomainTree = this->operator[](oldMovement.Partition);
            if (oldMovement.IsMove)
            {
                PlacementReplica const* replica = oldMovement.TargetToBeAddedReplica;
                TreeNodeIndex const& sourceNodeIndex = isFaultDomain_ ? oldMovement.SourceNode->FaultDomainIndex : oldMovement.SourceNode->UpgradeDomainIndex;
                TreeNodeIndex const& targetNodeIndex = isFaultDomain_ ? oldMovement.TargetNode->FaultDomainIndex : oldMovement.TargetNode->UpgradeDomainIndex;
                DeleteReplica(partitionDomainTree, targetNodeIndex, replica);
                AddReplica(partitionDomainTree, sourceNodeIndex, replica);
            }
            else if (oldMovement.IsAdd)
            {
                PlacementReplica const* replica = oldMovement.TargetToBeAddedReplica;
                TreeNodeIndex const& index = isFaultDomain_ ? oldMovement.TargetNode->FaultDomainIndex : oldMovement.TargetNode->UpgradeDomainIndex;
                DeleteReplica(partitionDomainTree, index, replica);
            }
            else if (oldMovement.IsSwap)
            {
                TreeNodeIndex const& index1 = isFaultDomain_ ? oldMovement.SourceNode->FaultDomainIndex : oldMovement.SourceNode->UpgradeDomainIndex;
                TreeNodeIndex const& index2 = isFaultDomain_ ? oldMovement.TargetNode->FaultDomainIndex : oldMovement.TargetNode->UpgradeDomainIndex;
                SwapReplica(partitionDomainTree, index1, oldMovement.SourceToBeAddedReplica, index2, oldMovement.TargetToBeAddedReplica);
            }
            else if (oldMovement.IsDrop)
            {
                PlacementReplica const* replica = oldMovement.SourceToBeDeletedReplica;
                // MoveInProgress replicas should not be in FailoverUnit/UpgradeDomain tables as they should not create FailoverUnit/UpgradeDomain violations
                if (!replica->IsMoveInProgress)
                {
                    TreeNodeIndex const& index = isFaultDomain_ ? oldMovement.SourceNode->FaultDomainIndex : oldMovement.SourceNode->UpgradeDomainIndex;
                    AddReplica(partitionDomainTree, index, replica);
                }
            }
        }
    }

    if (newMovement.IsValid)
    {
        ServiceEntry const* service = newMovement.Partition->Service;
        if (!(service->OnEveryNode || isFaultDomain_ && service->FDDistribution == Service::Type::Ignore))
        {
            PartitionDomainTree & partitionDomainTree = this->operator[](newMovement.Partition);
            if (newMovement.IsMove)
            {
                PlacementReplica const* replica = newMovement.TargetToBeAddedReplica;
                TreeNodeIndex const& sourceNodeIndex = isFaultDomain_ ? newMovement.SourceNode->FaultDomainIndex : newMovement.SourceNode->UpgradeDomainIndex;
                TreeNodeIndex const& targetNodeIndex = isFaultDomain_ ? newMovement.TargetNode->FaultDomainIndex : newMovement.TargetNode->UpgradeDomainIndex;
                DeleteReplica(partitionDomainTree, sourceNodeIndex, replica);
                AddReplica(partitionDomainTree, targetNodeIndex, replica);
            }
            else if (newMovement.IsAdd)
            {
                PlacementReplica const* replica = newMovement.TargetToBeAddedReplica;
                TreeNodeIndex const& index = isFaultDomain_ ? newMovement.TargetNode->FaultDomainIndex : newMovement.TargetNode->UpgradeDomainIndex;
                AddReplica(partitionDomainTree, index, replica);
            }
            else if (newMovement.IsSwap)
            {
                TreeNodeIndex const& index1 = isFaultDomain_ ? newMovement.SourceNode->FaultDomainIndex : newMovement.SourceNode->UpgradeDomainIndex;
                TreeNodeIndex const& index2 = isFaultDomain_ ? newMovement.TargetNode->FaultDomainIndex : newMovement.TargetNode->UpgradeDomainIndex;
                SwapReplica(partitionDomainTree, index1, newMovement.SourceToBeDeletedReplica, index2, newMovement.TargetToBeDeletedReplica);
            }
            else if (newMovement.IsDrop)
            {
                PlacementReplica const* replica = newMovement.SourceToBeDeletedReplica;
                // MoveInProgress replicas should not be in FailoverUnit/UpgradeDomain tables as they should not create FailoverUnit/UpgradeDomain violations
                if (!replica->IsMoveInProgress)
                {
                    TreeNodeIndex const& index = isFaultDomain_ ? newMovement.SourceNode->FaultDomainIndex : newMovement.SourceNode->UpgradeDomainIndex;
                    DeleteReplica(partitionDomainTree, index, replica);
                }
            }
        }
    }
}

void PartitionDomainStructure::AddReplica(PartitionDomainTree & tree, TreeNodeIndex const& toDomain, PlacementReplica const* replica)
{
    tree.ForEachNodeInPath(toDomain, [=](PartitionDomainTree::Node & n)
    {
        ++n.DataRef.ReplicaCount;
        if (n.Children.empty())
        {
            n.DataRef.Replicas.push_back(replica);
        }
    });
}

void PartitionDomainStructure::DeleteReplica(PartitionDomainTree & tree, TreeNodeIndex const& fromDomain, PlacementReplica const* replica)
{
    tree.ForEachNodeInPath(fromDomain, [=](PartitionDomainTree::Node & n)
    {
        ASSERT_IFNOT(n.Data.ReplicaCount > 0, "Node count should be greater than 0");
        --n.DataRef.ReplicaCount;
        if (n.Children.empty())
        {
            auto it = find(n.Data.Replicas.begin(), n.Data.Replicas.end(), replica);
            ASSERT_IF(it == n.Data.Replicas.end(), "Replica not found in partition domain tree");
            n.DataRef.Replicas.erase(it);
        }
    });
}

void PartitionDomainStructure::SwapReplica(PartitionDomainTree & tree, TreeNodeIndex const& index1, PlacementReplica const* replica1, TreeNodeIndex const& index2, PlacementReplica const* replica2)
{
    PartitionDomainTree::Node & node1 = tree.GetNodeByIndex(index1);
    PartitionDomainTree::Node & node2 = tree.GetNodeByIndex(index2);

    ASSERT_IFNOT(node1.Children.empty() && node2.Children.empty(), "Nodes should be leaf nodes");

    vector<PlacementReplica const*>::iterator it1 = find(node1.DataRef.Replicas.begin(), node1.DataRef.Replicas.end(), replica1);
    ASSERT_IF(it1 == node1.Data.Replicas.end(), "Replica {0} not found in partition domain tree", *replica1);

    vector<PlacementReplica const*>::iterator it2 = find(node2.DataRef.Replicas.begin(), node2.DataRef.Replicas.end(), replica2);
    ASSERT_IF(it2 == node2.Data.Replicas.end(), "Replica {0} not found in partition domain tree", *replica2);

    (*it1) = replica2;
    (*it2) = replica1;
}

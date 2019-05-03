// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Movement.h"
#include "InBuildCountPerNode.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

InBuildCountPerNode::InBuildCountPerNode()
    : LazyMap <NodeEntry const*, uint64_t> ((uint64_t)(0))
{
}

InBuildCountPerNode::InBuildCountPerNode(InBuildCountPerNode && other)
    : LazyMap <NodeEntry const*, uint64_t>(move(other))
{
}

InBuildCountPerNode::InBuildCountPerNode(InBuildCountPerNode const& other)
    : LazyMap <NodeEntry const*, uint64_t>(other)
{
}

InBuildCountPerNode::InBuildCountPerNode(InBuildCountPerNode const* baseNodeCounts)
    : LazyMap <NodeEntry const*, uint64_t>(baseNodeCounts)
{
}

InBuildCountPerNode & InBuildCountPerNode::operator = (InBuildCountPerNode && other)
{
    return (InBuildCountPerNode &)LazyMap<NodeEntry const*, uint64_t>::operator = (move(other));
}

void InBuildCountPerNode::ChangeMovement(Movement const& oldMovement, Movement const& newMovement)
{
    // Only Add and Move can generate InBuild replicas, only replicas that are added to target node can be throttled.
    // Builds of primary replicas are not throttled.
    if (   oldMovement.IsValid 
        && (oldMovement.IsAdd || oldMovement.IsMove || oldMovement.IsAddAndPromote)
        && oldMovement.TargetNode != nullptr
        && oldMovement.TargetToBeAddedReplica != nullptr
        && oldMovement.TargetToBeAddedReplica->Partition != nullptr
        && oldMovement.TargetToBeAddedReplica->Partition->Service != nullptr
        && oldMovement.TargetToBeAddedReplica->Partition->Service->IsStateful)
    {
        bool isPrimary = oldMovement.TargetToBeAddedReplica->IsPrimary;
        if (isPrimary)
        {
            // In some cases, PLB will treat new secondary as primary because of load, but it does count towards IB replica count.
            // Replica is a true primary only if it is the first one in partition to be placed.
            isPrimary = oldMovement.TargetToBeAddedReplica->Partition->ExistingReplicaCount == 0;
        }
        if (!isPrimary)
        {
            // Change count only if there is no pre-existing replica on the node
            if (!oldMovement.TargetToBeAddedReplica->Partition->HasReplicaOnNode(oldMovement.TargetNode))
            {
                DecreaseCount(oldMovement.TargetNode);
            }
        }
    }

    if (newMovement.IsValid
        && (newMovement.IsAdd || newMovement.IsMove || newMovement.IsAddAndPromote)
        && newMovement.TargetNode != nullptr
        && newMovement.TargetToBeAddedReplica != nullptr
        && newMovement.TargetToBeAddedReplica->Partition != nullptr
        && newMovement.TargetToBeAddedReplica->Partition->Service != nullptr
        && newMovement.TargetToBeAddedReplica->Partition->Service->IsStateful)
    {
        bool isPrimary = newMovement.TargetToBeAddedReplica->IsPrimary;
        if (isPrimary)
        {
            // In some cases, PLB will treat new secondary as primary because of load, but it does count towards IB replica count.
            // Replica is a true primary only if it is the first one in partition to be placed.
            isPrimary = newMovement.TargetToBeAddedReplica->Partition->ExistingReplicaCount == 0;
        }
        if (!isPrimary)
        {
            // Change count only if there is no pre-existing replica on the node
            if (!newMovement.TargetToBeAddedReplica->Partition->HasReplicaOnNode(newMovement.TargetNode))
            {
                IncreaseCount(newMovement.TargetNode);
            }
        }
    }
}

void InBuildCountPerNode::IncreaseCount(NodeEntry const* node)
{
    if (node)
    {
        uint64_t & counts = this->operator[](node);
        ++counts;
    }
}

void InBuildCountPerNode::DecreaseCount(NodeEntry const* node)
{
    if (node)
    {
        uint64_t & counts = this->operator[](node);
        ASSERT_IF(counts <= 0, "Decreasing in build counts when there is nothing to decrease.");
        --counts;
    }
}

void InBuildCountPerNode::SetCount(NodeEntry const* node, uint64_t count)
{
    if (node)
    {
        uint64_t & counts = this->operator[](node);
        counts = count;
    }
}

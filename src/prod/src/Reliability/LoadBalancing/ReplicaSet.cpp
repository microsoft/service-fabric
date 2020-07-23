// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Movement.h"
#include "ReplicaSet.h"
#include "NodeEntry.h"
#include "ServiceEntry.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

ReplicaSet::ReplicaSet()
    : replicas_()
{
}

ReplicaSet::ReplicaSet(ReplicaSet && other)
    : replicas_(move(other.replicas_))
{
}

ReplicaSet::ReplicaSet(ReplicaSet const& other)
    : replicas_(other.replicas_)
{
}

ReplicaSet & ReplicaSet::operator = (ReplicaSet && other)
{
    if (this != &other)
    {
        replicas_ = move(other.replicas_);
    }

    return *this;
}

void ReplicaSet::Add(PlacementReplica const* replica)
{
    if (replica != nullptr)
    {
        auto it = replicas_.find(replica);
        ASSERT_IF(it != replicas_.end(), "Replica should not exist when adding a replica");
        replicas_.insert(replica);
    }
}

void ReplicaSet::Delete(PlacementReplica const* replica)
{
    if (replica != nullptr)
    {
        auto it = replicas_.find(replica);
        ASSERT_IF(it == replicas_.end(), "Replica should exist when deleting a replica");
        replicas_.erase(replica);
    }
}

void ReplicaSet::Clear()
{
    replicas_.clear();
}

//------------------------
// NodePlacement methods

NodePlacement::NodePlacement()
    : LazyMap<NodeEntry const*, ReplicaSet>(ReplicaSet())
{
}

NodePlacement::NodePlacement(NodePlacement && other)
    : LazyMap<NodeEntry const*, ReplicaSet>(move(other))
{
}

NodePlacement::NodePlacement(NodePlacement const& other)
    : LazyMap<NodeEntry const*, ReplicaSet>(other)
{
}

NodePlacement & NodePlacement::operator = (NodePlacement && other)
{
    return (NodePlacement &)LazyMap<NodeEntry const*, ReplicaSet>::operator = (move(other));
}

NodePlacement::NodePlacement(NodePlacement const* basePlacement)
    : LazyMap<NodeEntry const*, ReplicaSet>(basePlacement)
{
}

void NodePlacement::ChangeMovement(Movement const& oldMovement, Movement const& newMovement)
{
    if (oldMovement.IsValid)
    {
        if (oldMovement.TargetNode != nullptr && (oldMovement.TargetNode->HasCapacity || oldMovement.TargetNode->IsThrottled))
        {
            ReplicaSet & rset = this->operator[](oldMovement.TargetNode);
            rset.Delete(oldMovement.TargetToBeAddedReplica);
            rset.Add(oldMovement.TargetToBeDeletedReplica);
        }
        if (oldMovement.SourceNode != nullptr && (oldMovement.SourceNode->HasCapacity || oldMovement.SourceNode->IsThrottled))
        {
            ReplicaSet & rset = this->operator[](oldMovement.SourceNode);
            rset.Add(oldMovement.SourceToBeDeletedReplica);
            rset.Delete(oldMovement.SourceToBeAddedReplica);
        }
    }

    if (newMovement.IsValid)
    {
        if (newMovement.SourceNode != nullptr && (newMovement.SourceNode->HasCapacity || newMovement.SourceNode->IsThrottled))
        {
            ReplicaSet & rset = this->operator[](newMovement.SourceNode);
            rset.Delete(newMovement.SourceToBeDeletedReplica);
            rset.Add(newMovement.SourceToBeAddedReplica);
        }
        if (newMovement.TargetNode != nullptr && (newMovement.TargetNode->HasCapacity || newMovement.TargetNode->IsThrottled))
        {
            ReplicaSet & rset = this->operator[](newMovement.TargetNode);
            rset.Add(newMovement.TargetToBeAddedReplica);
            rset.Delete(newMovement.TargetToBeDeletedReplica);
        }
    }
}

void NodePlacement::ChangeMovingIn(Movement const& oldMovement, Movement const& newMovement)
{
    if (oldMovement.IsValid)
    {
        auto & service = *(oldMovement.Partition->Service);

        if (oldMovement.SourceNode != nullptr &&
            oldMovement.SourceNode->HasCapacity &&
            oldMovement.IsSwap &&
            IsMakingPositiveDifference(
                service,
                *(oldMovement.SourceToBeAddedReplica->ReplicaEntry),
                *(oldMovement.TargetToBeAddedReplica->ReplicaEntry)))
        {
            ReplicaSet & rset = this->operator[](oldMovement.SourceNode);
            rset.Delete(oldMovement.SourceToBeAddedReplica);
        }

        if (oldMovement.TargetNode != nullptr &&
            oldMovement.TargetNode->HasCapacity &&
                (oldMovement.IncreasingTargetLoad ||
                    (oldMovement.IsSwap || oldMovement.IsPromote) &&
                    oldMovement.TargetToBeAddedReplica != nullptr &&
                    IsMakingPositiveDifference(
                        service,
                        *(oldMovement.TargetToBeAddedReplica->ReplicaEntry),
                        *(oldMovement.TargetToBeDeletedReplica->ReplicaEntry))
                )
            )
        {
            ReplicaSet & rset = this->operator[](oldMovement.TargetNode);
            rset.Delete(oldMovement.TargetToBeAddedReplica);
        }
    }

    if (newMovement.IsValid)
    {
        auto & service = *(newMovement.Partition->Service);

        if (newMovement.SourceNode != nullptr &&
            newMovement.SourceNode->HasCapacity &&
            newMovement.IsSwap &&
            IsMakingPositiveDifference(
                service,
                *(newMovement.SourceToBeAddedReplica->ReplicaEntry),
                *(newMovement.TargetToBeAddedReplica->ReplicaEntry)))
        {
            ReplicaSet & rset = this->operator[](newMovement.SourceNode);
            rset.Add(newMovement.SourceToBeAddedReplica);
        }

        if (newMovement.TargetNode != nullptr &&
            newMovement.TargetNode->HasCapacity &&
                (newMovement.IncreasingTargetLoad ||
                    (newMovement.IsSwap || newMovement.IsPromote) &&
                    newMovement.TargetToBeAddedReplica != nullptr &&
                    IsMakingPositiveDifference(
                        service,
                        *(newMovement.TargetToBeAddedReplica->ReplicaEntry),
                        *(newMovement.TargetToBeDeletedReplica->ReplicaEntry))
                )
            )
        {
            ReplicaSet & rset = this->operator[](newMovement.TargetNode);
            rset.Add(newMovement.TargetToBeAddedReplica);
        }
    }
}

bool NodePlacement::IsMakingPositiveDifference(ServiceEntry const& service, LoadEntry const& loadChange1, LoadEntry const& loadChange2)
{
    ASSERT_IFNOT(service.MetricCount == loadChange1.Values.size(),
        "Size of changes should be equal to the corresponding service metric count");
    ASSERT_IFNOT(loadChange1.Values.size() == loadChange2.Values.size(),
        "Size of two load changes should be equal");

    bool isPositiveDifference = false;
    for (size_t i = 0; i < service.MetricCount; i++)
    {
        if (loadChange1.Values[i] - loadChange2.Values[i] > 0)
        {
            isPositiveDifference = true;
            break;
        }
    }

    return isPositiveDifference;
}

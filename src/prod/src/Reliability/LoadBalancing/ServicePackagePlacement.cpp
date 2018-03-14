// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Movement.h"
#include "ServicePackagePlacement.h"
#include "ServicePackageEntry.h"
#include "NodeEntry.h"
#include "NodeMetrics.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

ServicePackagePlacement::ServicePackagePlacement()
    : LazyMap<NodeEntry const*, LazyMap<ServicePackageEntry const*, pair<ReplicaSet, int>>> (LazyMap<ServicePackageEntry const*, pair<ReplicaSet, int>> (pair<ReplicaSet, int>(ReplicaSet(), 0)))
{
}

ServicePackagePlacement::ServicePackagePlacement(ServicePackagePlacement && other)
    : LazyMap<NodeEntry const*, LazyMap<ServicePackageEntry const*, pair<ReplicaSet, int>>>(other)
{
}

ServicePackagePlacement::ServicePackagePlacement(ServicePackagePlacement const & other)
    : LazyMap<NodeEntry const*, LazyMap<ServicePackageEntry const*, pair<ReplicaSet, int>>>(other)
{
}

ServicePackagePlacement & Reliability::LoadBalancingComponent::ServicePackagePlacement::operator=(ServicePackagePlacement && other)
{
    if (this != &other)
    {
        LazyMap<NodeEntry const*, LazyMap<ServicePackageEntry const*, pair<ReplicaSet, int>>>::operator=(move(other));
    }
    return *this;
}

ServicePackagePlacement::ServicePackagePlacement(ServicePackagePlacement const * basePlacement)
    : LazyMap<NodeEntry const*, LazyMap<ServicePackageEntry const*, pair<ReplicaSet, int>>>(basePlacement)
{
}

LazyMap<ServicePackageEntry const*, pair<ReplicaSet, int>> const & Reliability::LoadBalancingComponent::ServicePackagePlacement::operator[](NodeEntry const * key) const
{
    return LazyMap<NodeEntry const*, LazyMap<ServicePackageEntry const*, pair<ReplicaSet, int>>>::operator[](key);
}

LazyMap<ServicePackageEntry const*, pair<ReplicaSet, int>>& Reliability::LoadBalancingComponent::ServicePackagePlacement::operator[](NodeEntry const * key)
{
    auto it = data_.find(key);

    if (it != data_.end())
    {
        return it->second;
    }
    else if (baseMap_ != nullptr)
    {
        // Initialize new member of the LazyMap with pointer to base, instead of with copy of base as in LazyMap
        it = data_.insert(std::make_pair(key, LazyMap<ServicePackageEntry const*, pair<ReplicaSet, int>>(&(baseMap_->operator[] (key))))).first;
        return it->second;
    }
    else
    {
        it = data_.insert(std::make_pair(key, LazyMap<ServicePackageEntry const*, pair<ReplicaSet, int>>(pair<ReplicaSet, int>(ReplicaSet(), 0)))).first;
        return it->second;
    }
}

void ServicePackagePlacement::ChangeMovement(Movement const & oldMovement,
    Movement const & newMovement,
    NodeMetrics & nodeChanges,
    NodeMetrics & nodeMovingInChanges)
{
    if (oldMovement.IsValid && oldMovement.Partition->Service->ServicePackage)
    {
        ServicePackageEntry const* servicePackage = oldMovement.Partition->Service->ServicePackage;

        if (oldMovement.SourceNode != nullptr)
        {
            LazyMap<ServicePackageEntry const*, pair<ReplicaSet, int>> & sourceNodePlacement = this->operator[](oldMovement.SourceNode);
            DeleteReplica(sourceNodePlacement, oldMovement.SourceNode, servicePackage, oldMovement.SourceToBeAddedReplica, nodeChanges, nodeMovingInChanges);
            AddReplica(sourceNodePlacement, oldMovement.SourceNode, servicePackage, oldMovement.SourceToBeDeletedReplica, nodeChanges, nodeMovingInChanges);
        }

        if (oldMovement.TargetNode != nullptr)
        {
            LazyMap<ServicePackageEntry const*, pair<ReplicaSet, int>> & targetNodePlacement = this->operator[](oldMovement.TargetNode);
            DeleteReplica(targetNodePlacement, oldMovement.TargetNode, servicePackage, oldMovement.TargetToBeAddedReplica, nodeChanges, nodeMovingInChanges);
            AddReplica(targetNodePlacement, oldMovement.TargetNode, servicePackage, oldMovement.TargetToBeDeletedReplica, nodeChanges, nodeMovingInChanges);
        }
    }

    if (newMovement.IsValid && newMovement.Partition->Service->ServicePackage)
    {
        ServicePackageEntry const* servicePackage = newMovement.Partition->Service->ServicePackage;

        if (newMovement.SourceNode != nullptr)
        {
            LazyMap<ServicePackageEntry const*, pair<ReplicaSet, int>> & sourceNodePlacement = this->operator[](newMovement.SourceNode);
            DeleteReplica(sourceNodePlacement, newMovement.SourceNode, servicePackage, newMovement.SourceToBeDeletedReplica, nodeChanges, nodeMovingInChanges);
            AddReplica(sourceNodePlacement, newMovement.SourceNode, servicePackage, newMovement.SourceToBeAddedReplica, nodeChanges, nodeMovingInChanges);
        }

        if (newMovement.TargetNode != nullptr)
        {
            LazyMap<ServicePackageEntry const*, pair<ReplicaSet, int>> & targetNodePlacement = this->operator[](newMovement.TargetNode);
            DeleteReplica(targetNodePlacement, newMovement.TargetNode, servicePackage, newMovement.TargetToBeDeletedReplica, nodeChanges, nodeMovingInChanges);
            AddReplica(targetNodePlacement, newMovement.TargetNode, servicePackage, newMovement.TargetToBeAddedReplica, nodeChanges, nodeMovingInChanges);
        }
    }
}

void Reliability::LoadBalancingComponent::ServicePackagePlacement::AddReplicaToNode(ServicePackageEntry const* servicePackage,
    NodeEntry const* node,
    PlacementReplica const* replica)
{
    //we do not increase the count here as we already set these counts in the SetCountToNode
    LazyMap<ServicePackageEntry const*, pair<ReplicaSet, int>> & placement = this->operator[] (node);
    auto & replicas = placement[servicePackage];
    replicas.first.Add(replica);
}

void Reliability::LoadBalancingComponent::ServicePackagePlacement::SetCountToNode(ServicePackageEntry const* servicePackage,
    NodeEntry const* node,
    int count)
{
    LazyMap<ServicePackageEntry const*, pair<ReplicaSet, int>> & placement = this->operator[] (node);
    auto & replicas = placement[servicePackage];
    replicas.second = count;
}

void ServicePackagePlacement::AddReplica(LazyMap<ServicePackageEntry const*, pair<ReplicaSet, int>> & placement,
    NodeEntry const* node,
    ServicePackageEntry const* servicePackage,
    PlacementReplica const* replica,
    NodeMetrics & nodeChanges,
    NodeMetrics & nodeMovingInChanges)
{
    // Discard None replicas as they shouldn't be moved or dropped
    // No need for checking dropped replicas as they aren't kept in backend logic - we don't consider them in allReplicas_
    if (nullptr == replica || nullptr == node || replica->IsNone)
    {
        return;
    }
    auto & replicas = placement[servicePackage];
    replicas.first.Add(replica);
    replicas.second++;

    if (replicas.second == 1)
    {
        // First replica on node, add load.
        nodeChanges.AddLoad(node, servicePackage->Loads);
        // We did not have it in the original placement so there was no load in nodeMovingIn
        if (!HasServicePackageOnNode(servicePackage, node, true))
        {
            nodeMovingInChanges.AddLoad(node, servicePackage->Loads);
        }
    }
}

void ServicePackagePlacement::DeleteReplica(LazyMap<ServicePackageEntry const*, pair<ReplicaSet, int>> & placement,
    NodeEntry const* node,
    ServicePackageEntry const* servicePackage,
    PlacementReplica const* replica,
    NodeMetrics & nodeChanges,
    NodeMetrics & nodeMovingInChanges)
{
    // Discard None replicas as they shouldn't be moved or dropped
    // No need for checking dropped replicas as they aren't kept in backend logic - we don't consider them in allReplicas_
    if (nullptr == replica || nullptr == node || replica->IsNone)
    {
        return;
    }
    auto & replicas = placement[servicePackage];
    replicas.first.Delete(replica);
    replicas.second--;

    TESTASSERT_IF(replicas.second < 0, "Replica count for service package {0} is less than 0", servicePackage->Name);

    if (replicas.second == 0)
    {
        // Last replica to move out of the node, remove the load.
        nodeChanges.DeleteLoad(node, servicePackage->Loads);
        // In moving in changes, we remove the load only if there was no SP on the node in base placement.
        if (!HasServicePackageOnNode(servicePackage, node, true))
        {
            nodeMovingInChanges.DeleteLoad(node, servicePackage->Loads);
        }
    }
}

bool Reliability::LoadBalancingComponent::ServicePackagePlacement::HasServicePackageOnNode(ServicePackageEntry const* servicePackage,
    NodeEntry const * node,
    bool inHighestBase) const
{
    LazyMap<NodeEntry const*, LazyMap<ServicePackageEntry const*, pair<ReplicaSet, int>>> const* placement = this;

    if (inHighestBase)
    {
        while (nullptr != placement->Base())
        {
            placement = placement->Base();
        }
    }

    LazyMap<ServicePackageEntry const*, pair<ReplicaSet, int>> const& replicasInSpMap = placement->operator[](node);
    auto const& replicasInSp = replicasInSpMap[servicePackage];
    return replicasInSp.second > 0;
}

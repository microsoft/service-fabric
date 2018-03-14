// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ApplicationPlacement.h"
#include "Movement.h"
#include "ServiceEntry.h"
#include "ApplicationEntry.h"

using namespace Reliability::LoadBalancingComponent;

ApplicationPlacement::ApplicationPlacement()
    : LazyMap<ApplicationEntry const*, LazyMap<NodeEntry const*, ReplicaSet>>(LazyMap<NodeEntry const*, ReplicaSet>(ReplicaSet()))
{
}

ApplicationPlacement::ApplicationPlacement(ApplicationPlacement && other)
    : LazyMap<ApplicationEntry const*, LazyMap<NodeEntry const*, ReplicaSet>>(std::move(other))
{
}

ApplicationPlacement::ApplicationPlacement(ApplicationPlacement const& other)
    : LazyMap<ApplicationEntry const*, LazyMap<NodeEntry const*, ReplicaSet>>(other)
{
}

ApplicationPlacement & ApplicationPlacement::operator = (ApplicationPlacement && other)
{
    return (ApplicationPlacement &)LazyMap<ApplicationEntry const*, LazyMap<NodeEntry const*, ReplicaSet>>::operator = (std::move(other));
}

ApplicationPlacement::ApplicationPlacement(ApplicationPlacement const* basePlacement)
    : LazyMap<ApplicationEntry const*, LazyMap<NodeEntry const*, ReplicaSet>>(basePlacement)
{
}

LazyMap<NodeEntry const*, ReplicaSet> const& ApplicationPlacement::operator[](ApplicationEntry const* key) const
{
    return LazyMap<ApplicationEntry const*, LazyMap<NodeEntry const*, ReplicaSet>>::operator[](key);
}

LazyMap<NodeEntry const*, ReplicaSet> & ApplicationPlacement::operator[](ApplicationEntry const* key)
{
    auto it = data_.find(key);

    if (it != data_.end())
    {
        return it->second;
    }
    else if (baseMap_ != nullptr)
    {
        // Initialize new member of the LazyMap with pointer to base, instead of with copy of base as in LazyMap
        it = data_.insert(std::make_pair(key, LazyMap<NodeEntry const*, ReplicaSet>(&(baseMap_->operator[] (key))))).first;
        return it->second;
    }
    else
    {
        it = data_.insert(std::make_pair(key, LazyMap<NodeEntry const*, ReplicaSet>(ReplicaSet()))).first;
        return it->second;
    }
}

void ApplicationPlacement::ChangeMovement(Movement const& oldMovement, Movement const& newMovement)
{
    if (oldMovement.IsValid && oldMovement.Partition->Service->Application)
    {
        ApplicationEntry const* appOldMove = oldMovement.Partition->Service->Application;
        if (!appOldMove->HasScaleoutOrCapacity)
        {
            return;
        }

        LazyMap<NodeEntry const*, ReplicaSet> & oldPlacement = this->operator[](appOldMove);

        DeleteReplica(oldPlacement, oldMovement.TargetNode, oldMovement.TargetToBeAddedReplica);
        AddReplica(oldPlacement, oldMovement.TargetNode, oldMovement.TargetToBeDeletedReplica);
        DeleteReplica(oldPlacement, oldMovement.SourceNode, oldMovement.SourceToBeAddedReplica);
        AddReplica(oldPlacement, oldMovement.SourceNode, oldMovement.SourceToBeDeletedReplica);
    }

    if (newMovement.IsValid && newMovement.Partition->Service->Application)
    {
        ApplicationEntry const* appNewMove = newMovement.Partition->Service->Application;
        if (!appNewMove->HasScaleoutOrCapacity)
        {
            return;
        }

        LazyMap<NodeEntry const*, ReplicaSet> & newPlacement = this->operator[](appNewMove);

        DeleteReplica(newPlacement, newMovement.SourceNode, newMovement.SourceToBeDeletedReplica);
        AddReplica(newPlacement, newMovement.SourceNode, newMovement.SourceToBeAddedReplica);
        DeleteReplica(newPlacement, newMovement.TargetNode, newMovement.TargetToBeDeletedReplica);
        AddReplica(newPlacement, newMovement.TargetNode, newMovement.TargetToBeAddedReplica);
    }
}

void ApplicationPlacement::AddReplica(LazyMap<NodeEntry const*, ReplicaSet> & placement,
    NodeEntry const* node,
    PlacementReplica const* replica)
{
    if (node && replica && !replica->ShouldDisappear)
    {
        placement[node].Add(replica);
    }
}

void ApplicationPlacement::DeleteReplica(LazyMap<NodeEntry const*, ReplicaSet> & placement,
    NodeEntry const* node,
    PlacementReplica const* replica)
{
    if (node && replica && !replica->ShouldDisappear)
    {
        placement[node].Delete(replica);
    }
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Movement.h"
#include "PartitionPlacement.h"
#include "ServiceEntry.h"
#include "ApplicationEntry.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

PartitionPlacement::PartitionPlacement()
    : LazyMap<PartitionEntry const*, ReplicaPlacement>(ReplicaPlacement())
{
}

PartitionPlacement::PartitionPlacement(PartitionPlacement && other)
    : LazyMap<PartitionEntry const*, ReplicaPlacement>(move(other))
{
}

PartitionPlacement::PartitionPlacement(PartitionPlacement const& other)
    : LazyMap<PartitionEntry const*, ReplicaPlacement>(other)
{
}

PartitionPlacement & PartitionPlacement::operator = (PartitionPlacement && other)
{
    return (PartitionPlacement &)LazyMap<PartitionEntry const*, ReplicaPlacement>::operator = (move(other));
}

PartitionPlacement::PartitionPlacement(PartitionPlacement const* basePlacement)
    : LazyMap<PartitionEntry const*, ReplicaPlacement>(basePlacement)
{
}

void PartitionPlacement::ChangeMovement(Movement const& oldMovement, Movement const& newMovement)
{
    if (oldMovement.IsValid)
    {
        ReplicaPlacement & partitionPlacement = this->operator[](oldMovement.Partition);
        partitionPlacement.Delete(oldMovement.TargetNode, oldMovement.TargetToBeAddedReplica);
        partitionPlacement.Add(oldMovement.TargetNode, oldMovement.TargetToBeDeletedReplica);
        partitionPlacement.Delete(oldMovement.SourceNode, oldMovement.SourceToBeAddedReplica);
        partitionPlacement.Add(oldMovement.SourceNode, oldMovement.SourceToBeDeletedReplica);
    }

    if (newMovement.IsValid)
    {
        ReplicaPlacement & partitionPlacement = this->operator[](newMovement.Partition);
        partitionPlacement.Delete(newMovement.SourceNode, newMovement.SourceToBeDeletedReplica);
        partitionPlacement.Add(newMovement.SourceNode, newMovement.SourceToBeAddedReplica);
        partitionPlacement.Delete(newMovement.TargetNode, newMovement.TargetToBeDeletedReplica);
        partitionPlacement.Add(newMovement.TargetNode, newMovement.TargetToBeAddedReplica);
    }
}


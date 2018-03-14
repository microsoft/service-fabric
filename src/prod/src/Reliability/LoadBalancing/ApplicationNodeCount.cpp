// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Movement.h"
#include "ApplicationNodeCount.h"
#include "ServiceEntry.h"
#include "ApplicationEntry.h"
#include "Checker.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

ApplicationNodeCount::ApplicationNodeCount()
    : LazyMap < ApplicationEntry const*, std::map<NodeEntry const*, size_t>>(std::map<NodeEntry const*, size_t>())
{
}

ApplicationNodeCount::ApplicationNodeCount(ApplicationNodeCount && other)
    : LazyMap < ApplicationEntry const*, std::map<NodeEntry const*, size_t>>(move(other))
{
}

ApplicationNodeCount::ApplicationNodeCount(ApplicationNodeCount const& other)
    : LazyMap < ApplicationEntry const*, std::map<NodeEntry const*, size_t>>(other)
{
}

ApplicationNodeCount::ApplicationNodeCount(ApplicationNodeCount const* baseNodeCounts)
    : LazyMap < ApplicationEntry const*, std::map<NodeEntry const*, size_t>>(baseNodeCounts)
{
}

ApplicationNodeCount & ApplicationNodeCount::operator = (ApplicationNodeCount && other)
{
    return (ApplicationNodeCount &)LazyMap<ApplicationEntry const*, std::map<NodeEntry const*, size_t>>::operator = (move(other));
}

void ApplicationNodeCount::ChangeMovement(Movement const& oldMovement, Movement const& newMovement)
{
    if (oldMovement.IsValid && oldMovement.Partition->Service->Application)
    {
        ApplicationEntry const* appOldMove = oldMovement.Partition->Service->Application;
        if (!appOldMove->HasScaleoutOrCapacity)
        {
            return;
        }

        map<NodeEntry const*, size_t> & nodeCounts = this->operator[](appOldMove);

        RemoveReplica(nodeCounts, oldMovement.TargetNode, oldMovement.TargetToBeAddedReplica);
        AddReplica(nodeCounts, oldMovement.TargetNode, oldMovement.TargetToBeDeletedReplica);
        RemoveReplica(nodeCounts, oldMovement.SourceNode, oldMovement.SourceToBeAddedReplica);
        AddReplica(nodeCounts, oldMovement.SourceNode, oldMovement.SourceToBeDeletedReplica);
    }

    if (newMovement.IsValid && newMovement.Partition->Service->Application)
    {
        ApplicationEntry const* appNewMove = newMovement.Partition->Service->Application;
        if (!appNewMove->HasScaleoutOrCapacity)
        {
            return;
        }

        map<NodeEntry const*, size_t> & nodeCounts = this->operator[](appNewMove);

        RemoveReplica(nodeCounts, newMovement.SourceNode, newMovement.SourceToBeDeletedReplica);
        AddReplica(nodeCounts, newMovement.SourceNode, newMovement.SourceToBeAddedReplica);
        RemoveReplica(nodeCounts, newMovement.TargetNode, newMovement.TargetToBeDeletedReplica);
        AddReplica(nodeCounts, newMovement.TargetNode, newMovement.TargetToBeAddedReplica);
    }
}

void ApplicationNodeCount::IncreaseCount(ApplicationEntry const* app, NodeEntry const* node)
{
    if (app && node)
    {
        map<NodeEntry const*, size_t> & nodeCounts = this->operator[](app);
        nodeCounts[node] = nodeCounts[node] + 1;
    }
}

void ApplicationNodeCount::SetCount(ApplicationEntry const* app, NodeEntry const* node, size_t count)
{
    if (app && node)
    {
        map<NodeEntry const*, size_t> & nodeCounts = this->operator[](app);
        nodeCounts[node] = count;
    }
}

void ApplicationNodeCount::AddReplica(map<NodeEntry const*, size_t> & nodeCounts, NodeEntry const* node, PlacementReplica const* replica)
{
    if (node && replica && !replica->ShouldDisappear)
    {
        if (nodeCounts.find(node) == nodeCounts.end())
        {
            nodeCounts.insert(make_pair(node, 1));
        }
        else
        {
            nodeCounts[node] = nodeCounts[node] + 1;
        }
    }
}

void ApplicationNodeCount::RemoveReplica(map<NodeEntry const*, size_t> & nodeCounts, NodeEntry const* node, PlacementReplica const* replica)
{
    if (node && replica && !replica->ShouldDisappear)
    {
        ASSERT_IF(nodeCounts.find(node) == nodeCounts.end(), "Node does not exist when changing application node counts.");
        ASSERT_IF(nodeCounts[node] <= 0, "Count should be greater than zero when removing replica from application node.");
        nodeCounts[node] = nodeCounts[node] - 1;
        if (0 == nodeCounts[node])
        {
            nodeCounts.erase(node);
        }
    }
}


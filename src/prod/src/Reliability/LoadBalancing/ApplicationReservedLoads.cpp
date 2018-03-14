// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ApplicationNodeCount.h"
#include "ApplicationNodeLoads.h"
#include "ApplicationReservedLoads.h"
#include "Movement.h"
#include "ServiceEntry.h"
#include "ApplicationEntry.h"

using namespace Reliability::LoadBalancingComponent;

ApplicationReservedLoad::ApplicationReservedLoad(size_t globalMetricCount)
    : LazyMap<NodeEntry const*, LoadEntry>(LoadEntry(globalMetricCount)),
    globalMetricCount_(globalMetricCount)
{
}

ApplicationReservedLoad::ApplicationReservedLoad(ApplicationReservedLoad && other)
    : LazyMap<NodeEntry const*, LoadEntry>(move(other)),
    globalMetricCount_(other.globalMetricCount_)
{
}

ApplicationReservedLoad::ApplicationReservedLoad(ApplicationReservedLoad const* base)
    : LazyMap<NodeEntry const*, LoadEntry>(base),
    globalMetricCount_(base->globalMetricCount_)
{
}

ApplicationReservedLoad & ApplicationReservedLoad::operator = (ApplicationReservedLoad && other)
{
    return (ApplicationReservedLoad &)LazyMap<NodeEntry const*, LoadEntry>::operator = (move(other));
}

void ApplicationReservedLoad::ChangeMovement(Movement const& oldMovement,
    Movement const& newMovement,
    ApplicationNodeLoads & appLoads,
    ApplicationNodeCount & appNodeCounts)
{
    ApplicationEntry const* appOldMove = oldMovement.IsValid ? oldMovement.Partition->Service->Application : nullptr;
    ApplicationEntry const* appNewMove = newMovement.IsValid ? newMovement.Partition->Service->Application : nullptr;

    if (appNewMove == nullptr && appOldMove == nullptr)
    {
        return;
    }

    if (appNewMove == appOldMove)
    {
        ChangeMovementInternal(appNewMove, oldMovement, newMovement, appLoads, appNodeCounts);
    }
    else
    {
        // Handle the case when two movements are from different application.
        if (appOldMove != nullptr)
        {
            ChangeMovementInternal(appOldMove, oldMovement, Movement::Invalid, appLoads, appNodeCounts);
        }
        if (appNewMove != nullptr)
        {
            ChangeMovementInternal(appNewMove, Movement::Invalid, newMovement, appLoads, appNodeCounts);
        }
    }
}

void ApplicationReservedLoad::ChangeMovementInternal(ApplicationEntry const* application,
    Movement const& oldMovement,
    Movement const& newMovement,
    ApplicationNodeLoads & appLoads,
    ApplicationNodeCount & appNodeCounts)
{
    // Calculate the set of nodes that are affected by both moves.
    std::set<NodeEntry const*> nodes;
    if (newMovement.IsValid)
    {
        if (newMovement.SourceNode != nullptr)
        {
            nodes.insert(newMovement.SourceNode);
        }
        if (newMovement.TargetNode != nullptr)
        {
            nodes.insert(newMovement.TargetNode);
        }
    }
    if (oldMovement.IsValid)
    {
        if (oldMovement.SourceNode != nullptr)
        {
            nodes.insert(oldMovement.SourceNode);
        }
        if (oldMovement.TargetNode != nullptr)
        {
            nodes.insert(oldMovement.TargetNode);
        }
    }

    // Remove reserved load for these nodes
    UpdateReservedLoad(nodes, application, appLoads, appNodeCounts, false);

    // Update application node loads and node counts
    appLoads.ChangeMovement(oldMovement, newMovement);
    appNodeCounts.ChangeMovement(oldMovement, newMovement);

    // Add reserved load for these nodes with new node counts and application node loads.
    UpdateReservedLoad(nodes, application, appLoads, appNodeCounts, true);
}

void ApplicationReservedLoad::Set(NodeEntry const* node, LoadEntry && reservedLoad)
{
    // This is allowed only from placement (hence base_ == nullptr) and only if node does not exist.
    TESTASSERT_IF(baseMap_ != nullptr || data_.find(node) != data_.end(),
        "Adding reserved load for node {0} when it already exists",
        node->NodeId);
    data_.insert(make_pair(node, move(reservedLoad)));
}

void ApplicationReservedLoad::UpdateReservedLoad(std::set<NodeEntry const*> nodes,
    ApplicationEntry const* application,
    ApplicationNodeLoads const& appLoads,
    ApplicationNodeCount const& appNodeCounts,
    bool isAdd)
{
    if (nodes.size() > 0 && application && application->HasReservation)
    {
        for (auto node : nodes)
        {
            std::map<NodeEntry const*, size_t> const& nodeCounts = appNodeCounts[application];
            auto const& nodeCountIt = nodeCounts.find(node);
            size_t numReplicasOnNode = 0;
            if (nodeCountIt != nodeCounts.end())
            {
                numReplicasOnNode = nodeCountIt->second;
            }
            if (numReplicasOnNode != 0)
            {
                LoadEntry appMinNodeLoadDiff(globalMetricCount_);
                LoadEntry const& applicationLoadOnNode = ((NodeMetrics const&)appLoads[application])[node];

                for (size_t globalIndex = 0; globalIndex < application->AppCapacities.Length; ++globalIndex)
                {
                    int64 loadValue = applicationLoadOnNode.Values[globalIndex];
                    int64 minLoadDiff = application->GetReservationDiff(globalIndex, loadValue);

                    appMinNodeLoadDiff.Set(globalIndex, minLoadDiff);
                }

                LoadEntry & currentReservation = this->operator[](node);
                if (isAdd)
                {
                    currentReservation += appMinNodeLoadDiff;
                }
                else
                {
                    currentReservation -= appMinNodeLoadDiff;
                }

                // In case we go below zero we have removed all reservation!
                for (int i = 0; i < currentReservation.Values.size(); ++i)
                {
                    if (currentReservation.Values[i] < 0)
                    {
                        TESTASSERT_IF(isAdd, "Adding reserved load for application {0} resulted in undeflow.", application->Name);
                        // We don't have any reservation any more!
                        currentReservation.Set(i, 0);
                    }
                }
            }
        }
    }
}


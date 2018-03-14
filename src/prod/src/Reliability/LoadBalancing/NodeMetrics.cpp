// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "NodeMetrics.h"
#include "Movement.h"
#include "NodeEntry.h"
#include "ServiceEntry.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

NodeMetrics::NodeMetrics(size_t totalMetricCount, size_t metricStartIndex, bool useAllMetrics)
    : LazyMap<NodeEntry const*, LoadEntry>(LoadEntry(totalMetricCount)),
    totalMetricCount_(totalMetricCount),
    useAllMetrics_(useAllMetrics),
    metricStartIndex_(metricStartIndex)
{
}

NodeMetrics::NodeMetrics(size_t totalMetricCount,
    std::map<NodeEntry const*, LoadEntry> && data,
    bool useAllMetrics,
    size_t metricStartIndex)
    : LazyMap<NodeEntry const*, LoadEntry>(LoadEntry(totalMetricCount), move(data)),
    totalMetricCount_(totalMetricCount),
    useAllMetrics_(useAllMetrics),
    metricStartIndex_(metricStartIndex)
{
}

NodeMetrics::NodeMetrics(NodeMetrics && other)
    : LazyMap<NodeEntry const*, LoadEntry>(move(other)),
    totalMetricCount_(other.totalMetricCount_),
    useAllMetrics_(other.useAllMetrics_),
    metricStartIndex_(other.metricStartIndex_)
{
}

NodeMetrics::NodeMetrics(NodeMetrics const& other)
    : LazyMap<NodeEntry const*, LoadEntry>(other),
    totalMetricCount_(other.totalMetricCount_),
    useAllMetrics_(other.useAllMetrics_),
    metricStartIndex_(other.metricStartIndex_)
{
}

NodeMetrics & NodeMetrics::operator = (NodeMetrics && other)
{
    if (this != &other)
    {
        LazyMap<NodeEntry const*, LoadEntry>::operator = (move(other));
        totalMetricCount_ = other.totalMetricCount_;
        useAllMetrics_ = other.useAllMetrics_;
        metricStartIndex_ = other.metricStartIndex_;
    }

    return *this;
}

NodeMetrics::NodeMetrics(NodeMetrics const* baseChanges)
    : LazyMap<NodeEntry const*, LoadEntry>(baseChanges),
    totalMetricCount_(baseChanges->totalMetricCount_),
    useAllMetrics_(baseChanges->useAllMetrics_),
    metricStartIndex_(baseChanges->metricStartIndex_)
{
}

void NodeMetrics::ChangeMovement(Movement const& oldMovement, Movement const& newMovement)
{
    UpdateWithOldMovement(oldMovement);

    UpdateWithNewMovement(newMovement);
}

void NodeMetrics::UpdateWithOldMovement(Movement const& oldMovement)
{
    if (oldMovement.IsValid)
    {
        auto & service = *(oldMovement.Partition->Service);

        if (oldMovement.IsSwap)
        {
            MoveLoad(oldMovement.TargetNode, oldMovement.SourceNode, service, *(oldMovement.SourceToBeDeletedReplica->ReplicaEntry));
            MoveLoad(oldMovement.SourceNode, oldMovement.TargetNode, service, *(oldMovement.TargetToBeDeletedReplica->ReplicaEntry));
        }
        else if (oldMovement.IsMove)
        {
            MoveLoad(oldMovement.TargetNode, oldMovement.SourceNode, service, *(oldMovement.SourceToBeDeletedReplica->ReplicaEntry));
            AddSbLoad(oldMovement.TargetNode, oldMovement.Partition);
        }
        else if (oldMovement.IsAdd)
        {
            DeleteLoad(oldMovement.TargetNode, service, *(oldMovement.TargetToBeAddedReplica->ReplicaEntry));
            AddSbLoad(oldMovement.TargetNode, oldMovement.Partition);
        }
        else if (oldMovement.IsPromote)
        {
            DeleteLoad(oldMovement.TargetNode, service, oldMovement.Partition->GetReplicaEntry(ReplicaRole::Enum::Primary));
            AddLoad(oldMovement.TargetNode, service, oldMovement.Partition->GetReplicaEntry(ReplicaRole::Enum::Secondary));
        }
        else if (oldMovement.IsDrop)
        {
            AddLoad(oldMovement.SourceNode, service, *(oldMovement.SourceToBeDeletedReplica->ReplicaEntry));
        }
    }
}

void NodeMetrics::UpdateWithNewMovement(Movement const& newMovement)
{
    if (newMovement.IsValid)
    {
        auto & service = *(newMovement.Partition->Service);

        if (newMovement.IsSwap)
        {
            MoveLoad(newMovement.SourceNode, newMovement.TargetNode, service, *(newMovement.SourceToBeDeletedReplica->ReplicaEntry));
            MoveLoad(newMovement.TargetNode, newMovement.SourceNode, service, *(newMovement.TargetToBeDeletedReplica->ReplicaEntry));
        }
        else if (newMovement.IsMove)
        {
            DeleteSbLoad(newMovement.TargetNode, newMovement.Partition);
            MoveLoad(newMovement.SourceNode, newMovement.TargetNode, service, *(newMovement.SourceToBeDeletedReplica->ReplicaEntry));
        }
        else if (newMovement.IsAdd)
        {
            DeleteSbLoad(newMovement.TargetNode, newMovement.Partition);
            AddLoad(newMovement.TargetNode, service, *(newMovement.TargetToBeAddedReplica->ReplicaEntry));
        }
        else if (newMovement.IsPromote)
        {
            DeleteLoad(newMovement.TargetNode, service, newMovement.Partition->GetReplicaEntry(ReplicaRole::Enum::Secondary));
            AddLoad(newMovement.TargetNode, service, newMovement.Partition->GetReplicaEntry(ReplicaRole::Enum::Primary));
        }
        else if (newMovement.IsDrop)
        {
            DeleteLoad(newMovement.SourceNode, service, *(newMovement.SourceToBeDeletedReplica->ReplicaEntry));
        }
    }
}

void Reliability::LoadBalancingComponent::NodeMetrics::AddLoad(NodeEntry const* node, LoadEntry const & load)
{
    LoadEntry & nodeChange = this->operator[](node);
    nodeChange += load;
}

void Reliability::LoadBalancingComponent::NodeMetrics::DeleteLoad(NodeEntry const* node, LoadEntry const & load)
{
    LoadEntry & nodeChange = this->operator[](node);
    nodeChange -= load;
}

void NodeMetrics::ChangeMovingIn(Movement const& oldMovement, Movement const& newMovement)
{
    UpdateWithOldMoveIn(oldMovement);
    UpdateWithNewMoveIn(newMovement);
}

void NodeMetrics::UpdateWithOldMoveIn(Movement const& oldMovement)
{
    if (oldMovement.IsValid)
    {
        auto & service = *(oldMovement.Partition->Service);

        if (oldMovement.IsSwap)
        {
            ApplyPositiveDifference(
                oldMovement.SourceNode,
                service,
                *(oldMovement.TargetToBeDeletedReplica->ReplicaEntry),
                *(oldMovement.SourceToBeDeletedReplica->ReplicaEntry),
                true);

            ApplyPositiveDifference(
                oldMovement.TargetNode,
                service,
                *(oldMovement.SourceToBeDeletedReplica->ReplicaEntry),
                *(oldMovement.TargetToBeDeletedReplica->ReplicaEntry),
                true);
        }
        else if (oldMovement.IsMove)
        {
            LoadEntry const* standByEntry = StandByReplicaLoadOnTargetNode(oldMovement.TargetNode, oldMovement.Partition);
            LoadEntry const* existingEntry = ExistingReplicaLoadOnTargetNode(oldMovement.TargetNode, oldMovement.Partition);

            TESTASSERT_IF(standByEntry != nullptr && existingEntry != nullptr, "Node shouldn't have an existing replica on the same node with a standby replica");

            if (existingEntry != nullptr)
            {
                ApplyPositiveDifference(
                    oldMovement.TargetNode,
                    service,
                    *(oldMovement.SourceToBeDeletedReplica->ReplicaEntry),
                    *existingEntry,
                    true);
            }
            else if (standByEntry == nullptr)
            {
                DeleteLoad(oldMovement.TargetNode, service, *(oldMovement.SourceToBeDeletedReplica->ReplicaEntry));
            }
            else
            {
                ApplyPositiveDifference(
                    oldMovement.TargetNode,
                    service,
                    *(oldMovement.SourceToBeDeletedReplica->ReplicaEntry),
                    *standByEntry,
                    true);
            }
        }
        else if (oldMovement.IsAdd)
        {
            LoadEntry const* standByEntry = StandByReplicaLoadOnTargetNode(oldMovement.TargetNode, oldMovement.Partition);
            LoadEntry const* existingEntry = ExistingReplicaLoadOnTargetNode(oldMovement.TargetNode, oldMovement.Partition);

            TESTASSERT_IF(standByEntry != nullptr && existingEntry != nullptr, "Node shouldn't have an existing replica on the same node with a standby replica");

            if (existingEntry != nullptr)
            {
                ApplyPositiveDifference(
                    oldMovement.TargetNode,
                    service,
                    *(oldMovement.TargetToBeAddedReplica->ReplicaEntry),
                    *existingEntry,
                    true);
            }
            else if (standByEntry == nullptr)
            {
                DeleteLoad(oldMovement.TargetNode, service, *(oldMovement.TargetToBeAddedReplica->ReplicaEntry));
            }
            else
            {
                ApplyPositiveDifference(
                    oldMovement.TargetNode,
                    service,
                    *(oldMovement.TargetToBeAddedReplica->ReplicaEntry),
                    *standByEntry,
                    true);
            }
        }
        else if (oldMovement.IsPromote)
        {
            ApplyPositiveDifference(
                oldMovement.TargetNode,
                service,
                oldMovement.Partition->GetReplicaEntry(ReplicaRole::Enum::Primary),
                oldMovement.Partition->GetReplicaEntry(ReplicaRole::Enum::Secondary),
                true);
        }
        else if (oldMovement.IsDrop)
        {
        }
    }
}

void NodeMetrics::UpdateWithNewMoveIn(Movement const& newMovement)
{
    if (newMovement.IsValid)
    {
        auto & service = *(newMovement.Partition->Service);

        if (newMovement.IsSwap)
        {
            ApplyPositiveDifference(
                newMovement.SourceNode,
                service,
                *(newMovement.TargetToBeDeletedReplica->ReplicaEntry),
                *(newMovement.SourceToBeDeletedReplica->ReplicaEntry),
                false);

            ApplyPositiveDifference(
                newMovement.TargetNode,
                service,
                *(newMovement.SourceToBeDeletedReplica->ReplicaEntry),
                *(newMovement.TargetToBeDeletedReplica->ReplicaEntry),
                false);
        }
        else if (newMovement.IsMove)
        {
            LoadEntry const* standByEntry = StandByReplicaLoadOnTargetNode(newMovement.TargetNode, newMovement.Partition);
            LoadEntry const* existingEntry = ExistingReplicaLoadOnTargetNode(newMovement.TargetNode, newMovement.Partition);

            TESTASSERT_IF(standByEntry != nullptr && existingEntry != nullptr, "Node shouldn't have an existing replica on the same node with a standby replica");

            if (existingEntry != nullptr)
            {
                ApplyPositiveDifference(
                    newMovement.TargetNode,
                    service,
                    *(newMovement.SourceToBeDeletedReplica->ReplicaEntry),
                    *existingEntry,
                    false);
            }
            else if (standByEntry == nullptr)
            {
                AddLoad(newMovement.TargetNode, service, *(newMovement.SourceToBeDeletedReplica->ReplicaEntry));
            }
            else
            {
                ApplyPositiveDifference(
                    newMovement.TargetNode,
                    service,
                    *(newMovement.SourceToBeDeletedReplica->ReplicaEntry),
                    *standByEntry,
                    false);
            }
        }
        else if (newMovement.IsAdd)
        {
            LoadEntry const* standByEntry = StandByReplicaLoadOnTargetNode(newMovement.TargetNode, newMovement.Partition);
            LoadEntry const* existingEntry = ExistingReplicaLoadOnTargetNode(newMovement.TargetNode, newMovement.Partition);

            TESTASSERT_IF(standByEntry != nullptr && existingEntry != nullptr, "Node shouldn't have an existing replica on the same node with a standby replica");

            if (existingEntry != nullptr)
            {
                ApplyPositiveDifference(
                    newMovement.TargetNode,
                    service,
                    *(newMovement.TargetToBeAddedReplica->ReplicaEntry),
                    *existingEntry,
                    false);
            }
            else if (standByEntry == nullptr)
            {
                AddLoad(newMovement.TargetNode, service, *(newMovement.TargetToBeAddedReplica->ReplicaEntry));
            }
            else
            {
                ApplyPositiveDifference(
                    newMovement.TargetNode,
                    service,
                    *(newMovement.TargetToBeAddedReplica->ReplicaEntry),
                    *standByEntry,
                    false);
            }
        }
        else if (newMovement.IsPromote)
        {
            ApplyPositiveDifference(
                newMovement.TargetNode,
                service,
                newMovement.Partition->GetReplicaEntry(ReplicaRole::Enum::Primary),
                newMovement.Partition->GetReplicaEntry(ReplicaRole::Enum::Secondary),
                false);
        }
        else if (newMovement.IsDrop)
        {
            // Although replica is marked to be dropped, we don't know when is it going to actually be dropped.
            // From pessimistic point of view - that space is still occupied.
        }
    }
}

void NodeMetrics::AddLoad(NodeEntry const* node, ServiceEntry const& service, LoadEntry const& loadChange)
{
    ASSERT_IFNOT(service.MetricCount == loadChange.Values.size(), "Size of changes should be equal to the corresponding service metric count");
    LoadEntry & nodeChange = this->operator[](node);

    for (size_t i = 0; i < service.MetricCount; i++)
    {
        int64 metricValue = loadChange.Values[i];
        if (metricValue != 0)
        {
            // update local load balancing domain metric value
            if (useAllMetrics_ && service.LBDomain != nullptr)
            {
                size_t index = i + service.LBDomain->MetricStartIndex;
                nodeChange.AddLoad(index, metricValue);
            }

            // update global load balancing domain metric value
            size_t globalIndex = service.GlobalMetricIndices[i] - metricStartIndex_;
            nodeChange.AddLoad(globalIndex, metricValue);
        }
    }
}

void NodeMetrics::ApplyPositiveDifference(
    NodeEntry const* node,
    ServiceEntry const& service,
    LoadEntry const& loadChange1,
    LoadEntry const& loadChange2,
    bool removeDifference)
{
    ASSERT_IFNOT(service.MetricCount == loadChange1.Values.size(),
        "Size of changes should be equal to the corresponding service metric count");
    ASSERT_IFNOT(loadChange1.Values.size() == loadChange2.Values.size(),
        "Size of two load changes should be equal");
    LoadEntry & nodeChange = this->operator[](node);

    for (size_t i = 0; i < service.MetricCount; i++)
    {
        int64 difference = loadChange1.Values[i] - loadChange2.Values[i];
        if (difference > 0)
        {
            // update local load balancing domain metric value
            if (useAllMetrics_ && service.LBDomain != nullptr)
            {
                size_t index = i + service.LBDomain->MetricStartIndex;
                nodeChange.AddLoad(index, removeDifference ? -difference : difference);
            }

            // update global load balancing domain metric value
            size_t globalIndex = service.GlobalMetricIndices[i] - metricStartIndex_;
            nodeChange.AddLoad(globalIndex, removeDifference ? -difference : difference);
        }
    }
}

void NodeMetrics::DeleteLoad(NodeEntry const* node, ServiceEntry const& service, LoadEntry const& loadChange)
{
    ASSERT_IFNOT(service.MetricCount == loadChange.Values.size(), "Size of changes should be equal to the corresponding service metric count");

    LoadEntry & nodeChange = this->operator[](node);

    for (size_t i = 0; i < service.MetricCount; i++)
    {
        int64 metricValue = loadChange.Values[i];
        if (metricValue != 0)
        {
            // update local load balancing domain metric value
            if (useAllMetrics_ && service.LBDomain != nullptr)
            {
                size_t index = i + service.LBDomain->MetricStartIndex;
                nodeChange.AddLoad(index, -metricValue);
            }

            // update global load balancing domain metric value
            size_t globalIndex = service.GlobalMetricIndices[i] - metricStartIndex_;
            nodeChange.AddLoad(globalIndex, -metricValue);
        }
    }
}

void NodeMetrics::MoveLoad(NodeEntry const* sourceNode, NodeEntry const* targetNode, ServiceEntry const& service, LoadEntry const& loadChange)
{
    ASSERT_IFNOT(service.MetricCount == loadChange.Values.size(), "Size of changes should be equal to the corresponding service metric count");

    LoadEntry & sourceChange = this->operator[](sourceNode);
    LoadEntry & targetChange = this->operator[](targetNode);
    for (size_t i = 0; i < service.MetricCount; i++)
    {
        int64 metricValue = loadChange.Values[i];
        if (metricValue != 0)
        {
            // update local load balancing domain metric value
            if (useAllMetrics_ && service.LBDomain != nullptr)
            {
                size_t index = i + service.LBDomain->MetricStartIndex;
                sourceChange.AddLoad(index, -metricValue);
                targetChange.AddLoad(index, metricValue);
            }

            // update global load balancing domain metric value
            size_t globalIndex = service.GlobalMetricIndices[i] - metricStartIndex_;
            sourceChange.AddLoad(globalIndex, -metricValue);
            targetChange.AddLoad(globalIndex, metricValue);
        }
    }
}

LoadEntry const* NodeMetrics::StandByReplicaLoadOnTargetNode(NodeEntry const* node, PartitionEntry const* partition)
{
    LoadEntry const* standByEntry = nullptr;
    vector<NodeEntry const*> const& SBLocations = partition->StandByLocations;

    if (find(SBLocations.begin(), SBLocations.end(), node) != SBLocations.end())
    {
        standByEntry = &partition->GetReplicaEntry(ReplicaRole::Secondary, true, node->NodeId);
    }

    return standByEntry;
}

LoadEntry const* NodeMetrics::ExistingReplicaLoadOnTargetNode(NodeEntry const* node, PartitionEntry const* partition)
{
    LoadEntry const* existingReplicaEntry = nullptr;

    partition->ForEachExistingReplica([&](PlacementReplica const* r)
    {
        if (r->Node->NodeId == node->NodeId && r->Role != ReplicaRole::StandBy)
        {
            existingReplicaEntry = r->ReplicaEntry;
        }
    }, false, false);

    return existingReplicaEntry;
}

void NodeMetrics::AddSbLoad(NodeEntry const* node, PartitionEntry const* partition)
{
    LoadEntry const* standByEntry = StandByReplicaLoadOnTargetNode(node, partition);
    if (standByEntry)
    {
        AddLoad(node, *partition->Service, *standByEntry);
    }
}

void NodeMetrics::DeleteSbLoad(NodeEntry const* node, PartitionEntry const* partition)
{
    LoadEntry const* standByEntry = StandByReplicaLoadOnTargetNode(node, partition);
    if (standByEntry)
    {
        DeleteLoad(node, *partition->Service, *standByEntry);
    }
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Metric.h"
#include "PlacementReplica.h"
#include "PlacementReplicaSet.h"
#include "PartitionEntry.h"
#include "PlacementAndLoadBalancing.h"
#include "DynamicNodeLoadSet.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

PlacementReplica::PlacementReplica(size_t index,
    ReplicaRole::Enum role,
    bool canBeThrottled)
    : index_(index),
    partitionEntry_(nullptr),
    role_(role),
    newReplicaIndex_(SIZE_MAX),
    isMovable_(false),
    isDropable_(false),
    isInTransition_(true),
    nodeEntry_(nullptr),
    isMoveInProgress_(false),
    isToBeDropped_(false),
    isInUpgrade_(false),
    isSingletonReplicaMovableDuringUpgrade_(true),
    canBeThrottled_(canBeThrottled)
{
}

// constructor for an existing replica
PlacementReplica::PlacementReplica(
    size_t index,
    ReplicaRole::Enum role,
    bool isMovable,
    bool isDropable,
    bool isInTransition,
    NodeEntry const* nodeEntry,
    bool isMoveInProgress,
    bool isToBeDropped,
    bool isInUpgrade,
    bool isSingletonReplicaMovableDuringUpgrade,
    bool canBeThrottled)
    : index_(index),
    partitionEntry_(nullptr),
    role_(role),
    newReplicaIndex_(SIZE_MAX),
    isMovable_(isMovable),
    isDropable_(isDropable),
    isInTransition_(isInTransition),
    nodeEntry_(nodeEntry),
    isMoveInProgress_(isMoveInProgress),
    isToBeDropped_(isToBeDropped),
    isInUpgrade_(isInUpgrade),
    isSingletonReplicaMovableDuringUpgrade_(isSingletonReplicaMovableDuringUpgrade),
    canBeThrottled_(canBeThrottled)
{
}

LoadEntry const* PlacementReplica::get_ReplicaEntry() const
{
    if (IsNew)
    {
        return &(partitionEntry_->GetReplicaEntry(role_));
    }
    else
    {
        return &(partitionEntry_->GetReplicaEntry(role_, true, nodeEntry_->NodeId));
    }
}

void PlacementReplica::SetPartitionEntry(PartitionEntry const* partitionEntry)
{
    partitionEntry_ = partitionEntry;
}

void PlacementReplica::SetNewReplicaIndex(size_t newReplicaIndex)
{
    newReplicaIndex_ = newReplicaIndex;
}

void PlacementReplica::ForEachBeneficialMetric(function<bool(size_t, bool forDefrag)> processor) const
{
    size_t metricCount = partitionEntry_->Service->MetricCount;
    ASSERT_IFNOT(metricCount == ReplicaEntry->Values.size(), "Metric count not same");
    auto servicePackage = partitionEntry_->Service->ServicePackage;

    LoadBalancingDomainEntry const* lbDomain = partitionEntry_->Service->LBDomain;
    LoadBalancingDomainEntry const* globalDomain = partitionEntry_->Service->GlobalLBDomain;

    for (size_t metricIndex = 0; metricIndex < metricCount; metricIndex++)
    {
        size_t totalMetricIndexInGlobalDomain = partitionEntry_->Service->GlobalMetricIndices[metricIndex];
        int64 load = ReplicaEntry->Values[metricIndex];

        //we will consider these replicas to carry load of their service package
        //only for purpose of calculating beneficial replicas
        if (load == 0)
        {
            if (servicePackage != nullptr)
            {
                load = (servicePackage->Loads).Values[totalMetricIndexInGlobalDomain];
            }
        }

        if (load == 0)
        {
            continue;
        }
        
        if (lbDomain != nullptr)
        {
            size_t totalMetricIndex = lbDomain->MetricStartIndex + metricIndex;
            auto& metric = lbDomain->Metrics[metricIndex];
            auto & loadStat = lbDomain->GetLoadStat(metricIndex);
            int64 coefficient = metric.IsDefrag ? (metric.DefragmentationScopedAlgorithmEnabled ? 1 : -1) : 1;

            bool scopedDefrag = false;
            double loadDiff;
            if (metric.BalancingByPercentage)
            {
                // When balancing by percentage, loadStat.Average is average of percents of load.
                // In case there are different node capacities, average of percentages can change by making moves; therefore, to find beneficial
                // replicas, beneficial moves, etc. average should be calculated as average in ideal balanced situation
                double average = loadStat.AbsoluteSum / loadStat.CapacitySum;
                if (metric.IsDefrag && metric.DefragmentationScopedAlgorithmEnabled && metric.DefragNodeCount < loadStat.Count)
                {
                    average = loadStat.AbsoluteSum / (loadStat.CapacitySum - metric.DefragNodeCount * metric.ReservationLoad);
                    scopedDefrag = true;
                }

                auto nodeCapacity = nodeEntry_->GetNodeCapacity(metric.IndexInGlobalDomain);
                auto nodeLoad = nodeEntry_->GetLoadLevel(totalMetricIndex);

                loadDiff = nodeLoad - nodeCapacity * average;
            }
            else
            {
                double average = loadStat.Average;
                if (metric.IsDefrag && metric.DefragmentationScopedAlgorithmEnabled && metric.DefragNodeCount < loadStat.Count)
                {
                    // Since we want to empty out a certain number of nodes,
                    // the amount of load we want to have on all nodes changes
                    // (we try to distribute the load on all nodes, except on the empty nodes).
                    // Here we calculate the total load from the average and use it to calculate the desired load (new average)
                    average = loadStat.Average * loadStat.Count / (loadStat.Count - metric.DefragNodeCount);
                    scopedDefrag = true;
                }
                loadDiff = nodeEntry_->GetLoadLevel(totalMetricIndex) - average;
            }
            // If scoped defrag is enabled, mark the replica as beneficial and
            // let the processor decide whether it is beneficial for balancing or for defrag
            if (!metric.IsBalanced && (coefficient * loadDiff > coefficient * 1e-9 || scopedDefrag))
            {
                if (!processor(totalMetricIndex, !(loadDiff > 1e-9) && scopedDefrag))
                {
                    break;
                }
            }
        }

        size_t metricIndexInGlobalDomain = totalMetricIndexInGlobalDomain - globalDomain->MetricStartIndex;
        auto& metric = globalDomain->Metrics[metricIndexInGlobalDomain];
        auto & loadStat = globalDomain->GetLoadStat(metricIndexInGlobalDomain);
        int64 coefficient = metric.IsDefrag ? (metric.DefragmentationScopedAlgorithmEnabled ? 1 : -1) : 1;

        bool scopedDefrag = false;
        double loadDiff;
        if (metric.BalancingByPercentage)
        {
            double average = loadStat.AbsoluteSum / loadStat.CapacitySum;
            if (metric.IsDefrag && metric.DefragmentationScopedAlgorithmEnabled && metric.DefragNodeCount < loadStat.Count)
            {
                average = loadStat.AbsoluteSum * 1.0 / (loadStat.CapacitySum - metric.DefragNodeCount * metric.ReservationLoad);
                scopedDefrag = true;
            }

            auto nodeCapacity = nodeEntry_->GetNodeCapacity(metric.IndexInGlobalDomain);
            auto nodeLoad = nodeEntry_->GetLoadLevel(totalMetricIndexInGlobalDomain);

            loadDiff = nodeLoad - nodeCapacity * average;
        }
        else
        {
            double average = loadStat.Average;
            if (metric.IsDefrag && metric.DefragmentationScopedAlgorithmEnabled && metric.DefragNodeCount < loadStat.Count)
            {
                // Since we want to empty out a certain number of nodes,
                // the amount of load we want to have on all nodes changes
                // (we try to distribute the load on all nodes, except on the empty nodes).
                // Here we calculate the total load from the average and use it to calculate the desired load (new average)
                average = loadStat.Average * loadStat.Count / (loadStat.Count - metric.DefragNodeCount);
                scopedDefrag = true;
            }
            loadDiff = nodeEntry_->GetLoadLevel(totalMetricIndexInGlobalDomain) - average;
        }

        if (!metric.IsBalanced && (coefficient * loadDiff > coefficient * 1e-9 || scopedDefrag))
        {
            // If scoped defrag is enabled, mark the replica as beneficial and
            // let the processor decide whether it is beneficial for balancing or for defrag
            if (!processor(totalMetricIndexInGlobalDomain, !(loadDiff > 1e-9) && scopedDefrag))
            {
                break;
            }
        }
    }
}

void PlacementReplica::ForEachWeightedDefragMetric(std::function<bool(size_t)> processor, SearcherSettings const& settings) const
{
    size_t metricCount = partitionEntry_->Service->MetricCount;
    ASSERT_IFNOT(metricCount == ReplicaEntry->Values.size(), "Metric count not same");

    LoadBalancingDomainEntry const* lbDomain = partitionEntry_->Service->LBDomain;
    LoadBalancingDomainEntry const* globalDomain = partitionEntry_->Service->GlobalLBDomain;
    TESTASSERT_IF(globalDomain == nullptr, "Global domain shouldn't be null");

    for (size_t metricIndex = 0; metricIndex < metricCount; metricIndex++)
    {
        if (lbDomain != nullptr && settings.LocalDomainWeight > 0 && lbDomain->Metrics[metricIndex].Weight > 0)
        {
            size_t totalMetricIndex = lbDomain->MetricStartIndex + metricIndex;
            if (!processor(totalMetricIndex))
            {
                break;
            }
        }

        size_t totalMetricIndexInGlobalDomain = partitionEntry_->Service->GlobalMetricIndices[metricIndex];
        size_t metricIndexInGlobalDomain = totalMetricIndexInGlobalDomain - globalDomain->MetricStartIndex;
        if (globalDomain != nullptr && globalDomain->Metrics[metricIndexInGlobalDomain].Weight > 0)
        {
            if (!processor(totalMetricIndexInGlobalDomain))
            {
                break;
            }
        }
    }
}

void PlacementReplica::WriteTo(TextWriter& writer, FormatOptions const &) const
{
    writer.Write("partition:{0} node:{1} role:{2} movable:{3} dropable:{4} moveInProgress:{5} toBeDropped:{6} isNew:{7} isMovableDuringUpgrade:{8} isInSingletonUpgrade:{9}",
        partitionEntry_->PartitionId,
        nodeEntry_ != nullptr ? nodeEntry_->NodeId : Federation::NodeId(LargeInteger::Zero),
        role_,
        isMovable_,
        isDropable_,
        isMoveInProgress_,
        isToBeDropped_,
        IsNew,
        isSingletonReplicaMovableDuringUpgrade_,
        partitionEntry_->IsInSingleReplicaUpgrade);
}

std::wstring PlacementReplica::ReplicaHash(bool alreadyExists /* = false */) const
{
    //This ReplicaHash() function exists because the WriteTo function above creates a new hash for the same replica each time. This gives a consistent hash for maps up to partition role etc

    if (alreadyExists && Node)
    {
        return wformatString("partition:{0} role:{1} node: {2}",
            partitionEntry_->PartitionId,
            ReplicaRole::ToString(role_),
            Node->NodeId);
    }
    else
    {
        return wformatString("partition:{0} role:{1}",
            partitionEntry_->PartitionId,
            ReplicaRole::ToString(role_));
    }
}

void PlacementReplica::WriteToEtw(uint16 contextSequenceId) const
{
    PlacementAndLoadBalancing::PLBTrace->PLBDump(contextSequenceId, wformatString(*this));
}

bool PlacementReplicaPointerCompare::operator() (PlacementReplica const * lhs, PlacementReplica const * rhs) const
{
    // Do not check for nullptr. PlacementReplica object should not be null during engine run!
    if (lhs->Partition->Order == rhs->Partition->Order)
    {
        return lhs->Index < rhs->Index;
    }
    else
    {
        return lhs->Partition->Order < rhs->Partition->Order;
    }
}

int64 PlacementReplica::GetReplicaLoadValue(
    PartitionEntry const* partition,
    LoadEntry const& replicaLocalLoad,
    size_t globalMetricIndex,
    size_t globalMetricIndexStart)
{
    for (size_t metricIndex = 0; metricIndex < partition->Service->MetricCount; ++metricIndex)
    {
        size_t curGlobalMetricIndex = partition->Service->GlobalMetricIndices[metricIndex] - globalMetricIndexStart;
        if (curGlobalMetricIndex == globalMetricIndex)
        {
            return replicaLocalLoad.Values[metricIndex];
        }
    }
    return 0;
}

bool PlacementReplica::CheckIfSingletonReplicaIsMovableDuringUpgrade(ReplicaDescription const& replica)
{
    return (replica.CurrentRole == ReplicaRole::Enum::Primary ||
        replica.CurrentRole == ReplicaRole::Enum::Secondary) &&
        replica.CurrentState == ReplicaStates::Enum::Ready &&
        replica.IsUp &&
        !(replica.IsToBeDroppedByPLB ||
            replica.IsToBePromoted ||
            replica.IsPendingRemove ||
            replica.IsDeleted ||
            replica.IsPrimaryToBePlaced ||
            replica.IsReplicaToBePlaced ||
            replica.IsMoveInProgress);
}

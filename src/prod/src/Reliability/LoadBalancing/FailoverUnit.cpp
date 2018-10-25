// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FailoverUnit.h"
#include "PLBConfig.h"
#include "SearcherSettings.h"
#include "Service.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

FailoverUnit::FailoverUnit(
    FailoverUnitDescription && fuDescription,
    vector<uint> && primaryEntries,
    vector<uint> && secondaryEntries,
    std::map<Federation::NodeId, std::vector<uint>> && secondaryEntriesMap,
    uint primaryMoveCost,
    uint secondaryMoveCost,
    bool isServiceOnEveryNode,
    map<Federation::NodeId, ResourceLoad> && resourceMap,
    Common::StopwatchTime nextScalingCheck)
    : fuDescription_(std::move(fuDescription)),
    primaryEntries_(std::move(primaryEntries)),
    isPrimaryLoadReported_(primaryEntries_.size(), false),
    secondaryEntries_(std::move(secondaryEntries)),
    isSecondaryLoadReported_(secondaryEntries_.size(), false),
    secondaryEntriesMap_(std::move(secondaryEntriesMap)),
    primaryMoveCost_(0),
    isPrimaryMoveCostReported_(false),
    secondaryMoveCost_(0),
    isSecondaryMoveCostReported_(false),
    actualReplicaDifference_(fuDescription_.ReplicaDifference),
    isServiceOnEveryNode_(isServiceOnEveryNode),
    resourceLoadMap_(std::move(resourceMap)),
    nextScalingCheck_(nextScalingCheck)
{
    for (auto it = secondaryEntriesMap_.begin(); it != secondaryEntriesMap_.end(); ++it)
    {
        isSecondaryLoadMapReported_.insert(std::make_pair(it->first, std::vector<bool> (it->second.size(), false)));
    }

    updateCount_ = 1;
    UpdateMoveCost(ReplicaRole::Primary, primaryMoveCost, false);
    UpdateMoveCost(ReplicaRole::Secondary, secondaryMoveCost, false);
}

FailoverUnit::FailoverUnit(FailoverUnit && other)
    : fuDescription_(std::move(other.fuDescription_)),
    primaryEntries_(std::move(other.primaryEntries_)),
    isPrimaryLoadReported_(std::move(other.isPrimaryLoadReported_)),
    secondaryEntries_(std::move(other.secondaryEntries_)),
    isSecondaryLoadReported_(std::move(other.isSecondaryLoadReported_)),
    primaryMoveCost_(other.primaryMoveCost_),
    isPrimaryMoveCostReported_(other.isPrimaryMoveCostReported_),
    secondaryMoveCost_(other.secondaryMoveCost_),
    isSecondaryMoveCostReported_(other.isSecondaryMoveCostReported_),
    actualReplicaDifference_(other.actualReplicaDifference_),
    updateCount_(other.updateCount_),
    secondaryEntriesMap_(std::move(other.secondaryEntriesMap_)),
    isSecondaryLoadMapReported_(std::move(other.isSecondaryLoadMapReported_)),
    isServiceOnEveryNode_(other.isServiceOnEveryNode_),
    resourceLoadMap_(std::move(other.resourceLoadMap_)),
    nextScalingCheck_(other.nextScalingCheck_)
{
}

FailoverUnit::FailoverUnit(FailoverUnit const & other)
    : fuDescription_(other.fuDescription_),
    primaryEntries_(other.primaryEntries_),
    isPrimaryLoadReported_(other.isPrimaryLoadReported_),
    secondaryEntries_(other.secondaryEntries_),
    isSecondaryLoadReported_(other.isSecondaryLoadReported_),
    primaryMoveCost_(other.primaryMoveCost_),
    isPrimaryMoveCostReported_(other.isPrimaryMoveCostReported_),
    secondaryMoveCost_(other.secondaryMoveCost_),
    isSecondaryMoveCostReported_(other.isSecondaryMoveCostReported_),
    actualReplicaDifference_(other.actualReplicaDifference_),
    updateCount_(other.updateCount_),
    secondaryEntriesMap_(other.secondaryEntriesMap_),
    isSecondaryLoadMapReported_(other.isSecondaryLoadMapReported_),
    isServiceOnEveryNode_(other.isServiceOnEveryNode_),
    resourceLoadMap_(other.resourceLoadMap_),
    nextScalingCheck_(other.nextScalingCheck_)
{
}

void FailoverUnit::UpdateDescription(FailoverUnitDescription && description)
{
    fuDescription_ = move(description);
}

void FailoverUnit::UpdateActualReplicaDifference(int value)
{
    actualReplicaDifference_ = value;
}

void FailoverUnit::UpdateSecondaryLoadMap(FailoverUnitDescription const & newFailoverUnitDescription, SearcherSettings const & settings)
{
    bool hasNewSecondary = false;

    if (!SecondaryEntriesMap.empty())
    {
        // Update the secondary map with new replica set in the FT description
        newFailoverUnitDescription.ForEachReplica([&](ReplicaDescription const & replica)
        {
            if (replica.CurrentRole != ReplicaRole::Secondary)
            {
                return;
            }

            // Case when secondary replica move is started:
            if (SecondaryEntriesMap.find(replica.NodeId) == SecondaryEntriesMap.end())
            {
                // If secondary replica is being moved
                auto itSecondaryMove = secondaryMoves_.find(replica.NodeId);
                if (itSecondaryMove != secondaryMoves_.end())
                {
                    // And if source replica had specific secondary load
                    Federation::NodeId & sourceReplicaNode = itSecondaryMove->second;
                    auto itSecondarySource = SecondaryEntriesMap.find(sourceReplicaNode);
                    if (itSecondarySource != SecondaryEntriesMap.end())
                    {
                        // Move is initiated: add specific secondary load for target replica into the map
                        secondaryEntriesMap_.insert(make_pair(replica.NodeId, itSecondarySource->second));

                        // Update isSecondaryLoadMap in target replica with value from source replica
                        auto itMap = isSecondaryLoadMapReported_.find(sourceReplicaNode);
                        if (itMap != isSecondaryLoadMapReported_.end())
                        {
                            isSecondaryLoadMapReported_.insert(make_pair(replica.NodeId, itMap->second));
                        }
                    }
                    else
                    {
                        hasNewSecondary = true;
                    }
                }
                else
                {
                    hasNewSecondary = true;
                }
            }
        });

        // Only keep entries for existing replicas in the FT update
        for (auto itEntries = SecondaryEntriesMap.begin(); itEntries != SecondaryEntriesMap.end();)
        {
            if (!newFailoverUnitDescription.HasReplicaWithSecondaryLoadOnNode(itEntries->first))
            {
                auto itReportedMap = isSecondaryLoadMapReported_.find(itEntries->first);
                if (itReportedMap != isSecondaryLoadMapReported_.end())
                {
                    itReportedMap = isSecondaryLoadMapReported_.erase(itReportedMap);
                }

                itEntries = secondaryEntriesMap_.erase(itEntries);
            }
            else
            {
                ++itEntries;
            }
        }

        if (hasNewSecondary && !SecondaryEntriesMap.empty())
        {
            size_t numMetrics = SecondaryEntries.size();
            vector<uint> newSecondaryLoad(numMetrics, 0);
            vector<bool> newSecondaryLoadReport(numMetrics, false);
            bool useDefaultLoad = isServiceOnEveryNode_ && settings.UseDefaultLoadForServiceOnEveryNode;
            //if there is a secondary replica that doesn't have a load entry in the map, use average load
            //special case for -1 services to use default if config is set
            if (!useDefaultLoad)
            {
                for (int i = 0; i < numMetrics; ++i)
                {
                    uint64 loadSum = 0;
                    for (auto itMap = SecondaryEntriesMap.begin(); itMap != SecondaryEntriesMap.end(); ++itMap)
                    {
                        loadSum += itMap->second[i];
                    }
                    newSecondaryLoad[i] = static_cast<uint>(floor(static_cast<double>(loadSum) / SecondaryEntriesMap.size() + 0.5));

                    bool hasReported = false;
                    for (auto itMap = IsSecondaryLoadMapReported.begin(); itMap != IsSecondaryLoadMapReported.end(); ++itMap)
                    {
                        if (itMap->second[i])
                        {
                            hasReported = true;
                            break;
                        }
                    }
                    newSecondaryLoadReport[i] = hasReported;
                }
            }
            else
            {
                newSecondaryLoad = secondaryEntries_;
                newSecondaryLoadReport = isSecondaryLoadReported_;
            }
            newFailoverUnitDescription.ForEachReplica([&](ReplicaDescription const & replica)
            {
                if (replica.CurrentRole != ReplicaRole::Secondary)
                {
                    return;
                }

                // Case when this is a new secondary
                if (SecondaryEntriesMap.find(replica.NodeId) == SecondaryEntriesMap.end())
                {
                    secondaryEntriesMap_.insert(make_pair(replica.NodeId, newSecondaryLoad));
                    isSecondaryLoadMapReported_.insert(make_pair(replica.NodeId, newSecondaryLoadReport));
                }
            });
        }
    }

    secondaryMoves_.clear();
}

bool FailoverUnit::UpdateLoad(ReplicaRole::Enum role, size_t index, uint value, SearcherSettings const & settings,
    bool useNodeId, Federation::NodeId const& nodeId, bool isReset,bool isSingletonReplicaService)
{
    // Update routine for load reporting from nodeId
    bool updated = false;
    bool secondaryUpdate = (role == ReplicaRole::Primary || !settings.UseSeparateSecondaryLoad || !useNodeId);

    if (secondaryUpdate)
    {
        vector<uint> & metrics = (role == ReplicaRole::Primary ? primaryEntries_ : secondaryEntries_);
        vector<bool> & isLoadReported = (role == ReplicaRole::Primary ? isPrimaryLoadReported_ : isSecondaryLoadReported_);

        ASSERT_IF(index >= metrics.size(), "Reporting primary/secondary load: Index should be <= metric size");

        if (metrics[index] != value)
        {
            metrics[index] = value;
            updated = true;
        }

        isLoadReported[index] = !isReset;
    }

    // If service is singleton replica, align primary and secondary metric loads
    // This is needed to avoid issues during singleton replica movement,
    // as first new secondary is created, and then promoted to primary
    if (role == ReplicaRole::Primary && isSingletonReplicaService)
    {
        ASSERT_IF(index >= secondaryEntries_.size(), "Index should be <= metric size");

        if (secondaryEntries_[index] != value)
        {
            secondaryEntries_[index] = value;
        }
        isSecondaryLoadReported_[index] = !isReset;
    }

    if (useNodeId)
    {
        // loads is secondary default load metric vector
        vector<uint> loads = secondaryEntries_;
        bool hasReplicaOnNode = FuDescription.HasReplicaWithSecondaryLoadOnNode(nodeId);
        if (secondaryEntriesMap_.empty() && hasReplicaOnNode)
        {
            // First load update, add replicas using default loads
            this->fuDescription_.ForEachReplica([&](ReplicaDescription const & replica)
            {
                if (replica.UseSecondaryLoad())
                {
                    secondaryEntriesMap_.insert(make_pair(replica.NodeId, vector<uint>(secondaryEntries_)));
                    isSecondaryLoadMapReported_.insert(make_pair(replica.NodeId, vector<bool>(isSecondaryLoadReported_)));
                }
            });
        }

        auto it = secondaryEntriesMap_.find(nodeId);
        if (it != secondaryEntriesMap_.end())
        {
            updated = FindAndUpdateLoad(it->second, index, value) || updated;
        }

        auto itReported = isSecondaryLoadMapReported_.find(nodeId);
        if (itReported != isSecondaryLoadMapReported_.end())
        {
            ASSERT_IF(index >= itReported->second.size(), "Reporting secondary load from secondary entries: Index should be <= metric size");
            itReported->second[index] = !isReset;
        }

    }
    else if (role == ReplicaRole::Secondary)
    {
        // No node id, update all secondary map with the load: sweep updates
        for (auto mapIt = secondaryEntriesMap_.begin(); mapIt != secondaryEntriesMap_.end(); ++mapIt)
        {
            updated = FindAndUpdateLoad(mapIt->second, index, value) || updated;
        }

        for (auto mapIt = isSecondaryLoadMapReported_.begin(); mapIt != isSecondaryLoadMapReported_.end(); ++mapIt)
        {
            ASSERT_IF(index >= mapIt->second.size(), "Reporting secondary entries: Index should be <= metric size");
            mapIt->second[index] = !isReset;
        }
    }

    return updated;
}

uint FailoverUnit::GetSecondaryLoad(size_t metricIndex, Federation::NodeId const& nodeId, SearcherSettings const & settings) const
{
    uint load = SecondaryEntries[metricIndex];
    bool useDefaultLoad = isServiceOnEveryNode_ && settings.UseDefaultLoadForServiceOnEveryNode;

    if (settings.UseSeparateSecondaryLoad && !SecondaryEntriesMap.empty())
    {
        auto it = SecondaryEntriesMap.find(nodeId);
        // If there is replica on nodeId, move map shouldn't be used
        // The replica should already have entry during FT update
        if (it == SecondaryEntriesMap.end() && !fuDescription_.HasAnyReplicaOnNode(nodeId))
        {
            // nodeId isn't in the map
            auto itNodeMove = secondaryMoves_.find(nodeId);
            if (itNodeMove != secondaryMoves_.end())
            {
                // Use the src node saved in the move map
                it = SecondaryEntriesMap.find(itNodeMove->second);
            }
        }

        if (it != SecondaryEntriesMap.end())
        {
            load = (it->second)[metricIndex];
        }
        else if (!SecondaryEntriesMap.empty() && !useDefaultLoad)
        {
            // secondary map is not empty, but this replica is not in the map, use average load
            //we special case for -1 services
            load = GetSecondaryLoadAverage(metricIndex);
        }
    }

    return load;
}

uint FailoverUnit::GetSecondaryLoadAverage(size_t metricIndex) const
{
    uint64 averageLoad(0);
    for (auto it = SecondaryEntriesMap.begin(); it != SecondaryEntriesMap.end(); ++it)
    {
        averageLoad += it->second[metricIndex];
    }

    return static_cast<uint>(floor(static_cast<double>(averageLoad) / SecondaryEntriesMap.size() + 0.5));
}

void FailoverUnit::UpdateSecondaryMoves(Federation::NodeId const& src, Federation::NodeId const& dst)
{
    // Move: secondaryMoves_[TargetNode] = SourceNode
    // Swap: secondaryMoves_[PrimaryNode] = SecondaryNode
    auto itNodeMove = secondaryMoves_.find(dst);
    if (itNodeMove != secondaryMoves_.end())
    {
        itNodeMove->second = src;
    }
    else
    {
        secondaryMoves_.insert(make_pair(dst, src));
    }
}

bool FailoverUnit::HasSecondaryOnNode(Federation::NodeId const& src) const
{
    return fuDescription_.HasReplicaWithRoleOnNode(src, ReplicaRole::Secondary);
}

bool FailoverUnit::FindAndUpdateLoad(vector<uint>& metrics, size_t metricIndex, uint value)
{
    ASSERT_IF(metricIndex >= metrics.size(), "Index should be <= metric size");

    if (metrics[metricIndex] != value)
    {
        metrics[metricIndex] = value;
        return true;
    }

    return false;
}

bool FailoverUnit::UpdateMoveCost(ReplicaRole::Enum role, uint value, bool isMoveCostReported)
{
    bool updated = false;

    if (!IsMoveCostValid(value))
    {
        return false;
    }

    if (role == ReplicaRole::Primary)
    {
        if (primaryMoveCost_ != value)
        {
            primaryMoveCost_ = value;
            updated = true;
        }

        isPrimaryMoveCostReported_ = isMoveCostReported;
    }
    else
    {
        if (secondaryMoveCost_ != value)
        {
            secondaryMoveCost_ = value;
            updated = true;
        }

        isSecondaryMoveCostReported_ = isMoveCostReported;
    }

    return updated;
}

bool FailoverUnit::IsMoveCostValid(uint moveCost) const
{
    if (FABRIC_MOVE_COST::FABRIC_MOVE_COST_ZERO == moveCost
        || FABRIC_MOVE_COST::FABRIC_MOVE_COST_LOW == moveCost
        || FABRIC_MOVE_COST::FABRIC_MOVE_COST_MEDIUM == moveCost
        || FABRIC_MOVE_COST::FABRIC_MOVE_COST_HIGH == moveCost)
    {
        return true;
    }
    return false;
}

bool FailoverUnit::IsMoveCostReported(ReplicaRole::Enum role) const
{
    return role == ReplicaRole::Primary ? isPrimaryMoveCostReported_ : isSecondaryMoveCostReported_;
}

uint FailoverUnit::GetMoveCostValue(ReplicaRole::Enum role, SearcherSettings const & settings) const
{
    uint moveCostValue = 0;

    switch (role == ReplicaRole::Primary ? primaryMoveCost_ : secondaryMoveCost_)
    {
    case FABRIC_MOVE_COST::FABRIC_MOVE_COST_HIGH:
        moveCostValue = settings.MoveCostHighValue;
        break;
    case FABRIC_MOVE_COST::FABRIC_MOVE_COST_MEDIUM:
        moveCostValue = settings.MoveCostMediumValue;
        break;
    case FABRIC_MOVE_COST::FABRIC_MOVE_COST_LOW:
        moveCostValue = settings.MoveCostLowValue;
        break;
    case FABRIC_MOVE_COST::FABRIC_MOVE_COST_ZERO:
        moveCostValue = settings.MoveCostZeroValue;
        break;
    default:
        // Primary move cost value is validated when partition is created,
        // so we should not end up here.
        TESTASSERT_IF(true, "{0} move cost for partition entry {1} is invalid", role == ReplicaRole::Primary ? L"Primary" : L"Secondary", FUId);
    }

    return moveCostValue;
}

void FailoverUnit::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("{0} {1} {2} {3} {4} {5} {6} {7}", fuDescription_, primaryEntries_, secondaryEntries_, primaryMoveCost_, secondaryMoveCost_, actualReplicaDifference_, updateCount_, nextScalingCheck_);
}

void FailoverUnit::ResetMovementMaps()
{
    secondaryEntriesMap_.clear();
    secondaryMoves_.clear();
}

bool FailoverUnit::UpdateResourceLoad(wstring const& metricName, int value, Federation::NodeId const& nodeId)
{
    if (!fuDescription_.HasAnyReplicaOnNode(nodeId))
    {
        return false;
    }
    if (metricName == *ServiceModel::Constants::SystemMetricNameCpuCores)
    {
        resourceLoadMap_[nodeId].CPUCores_ = value;
    }
    else if (metricName == *ServiceModel::Constants::SystemMetricNameMemoryInMB)
    {
        resourceLoadMap_[nodeId].MemoryInMb_ = value;
    }
    return true;
}

void FailoverUnit::CleanupResourceUsage(FailoverUnitDescription const & newFailoverUnitDescription)
{
    // Only keep entries for existing replicas in the FT update
    for (auto itEntries = resourceLoadMap_.begin(); itEntries != resourceLoadMap_.end();)
    {
        if (!newFailoverUnitDescription.HasAnyReplicaOnNode(itEntries->first))
        {
            itEntries = resourceLoadMap_.erase(itEntries);
        }
        else
        {
            ++itEntries;
        }
    }
}

uint FailoverUnit::GetResourcePrimaryLoad(std::wstring const & name) const
{
    auto primaryReplica = fuDescription_.PrimaryReplica;
    if (primaryReplica != nullptr)
    {
        bool isCpu = name == *ServiceModel::Constants::SystemMetricNameCpuCores;
        bool isMemory = name == *ServiceModel::Constants::SystemMetricNameMemoryInMB;
        auto resourceIt = resourceLoadMap_.find(primaryReplica->NodeId);
        if (resourceIt != resourceLoadMap_.end())
        {
            if (isCpu)
            {
                return resourceIt->second.CPUCores_;
            }
            else if (isMemory)
            {
                return resourceIt->second.MemoryInMb_;
            }
        }
    }
    return 0;
}

uint FailoverUnit::GetResourceAverageLoad(std::wstring const & metricName) const
{
    uint64 averageLoad(0);
    bool isCpu = metricName == *ServiceModel::Constants::SystemMetricNameCpuCores;
    bool isMemory = metricName == *ServiceModel::Constants::SystemMetricNameMemoryInMB;

    for (auto it : resourceLoadMap_)
    {
        if (isCpu)
        {
            averageLoad += it.second.CPUCores_;
        }
        else if (isMemory)
        {
            averageLoad += it.second.MemoryInMb_;
        }
    }

    return static_cast<uint>(floor(static_cast<double>(averageLoad) / resourceLoadMap_.size() + 0.5));
}

uint FailoverUnit::GetSecondaryLoadForAutoScaling(uint metricIndex) const
{
    if (secondaryEntriesMap_.size() > 0)
    {
        return GetSecondaryLoadAverage(metricIndex);
    }
    return secondaryEntries_[metricIndex];
}

uint FailoverUnit::GetAverageLoadForAutoScaling(uint metricIndex) const
{
    uint64 averageLoad(0);

    auto replicas = this->FuDescription.Replicas.size();

    if (replicas == 0)
    {
        return static_cast<uint>(averageLoad);
    }

    // Primary replica load
    averageLoad += primaryEntries_[metricIndex];

    if (replicas > 1) 
    {
        if (secondaryEntriesMap_.size() > 0)
        {
            for (auto it = SecondaryEntriesMap.begin(); it != SecondaryEntriesMap.end(); ++it)
            {
                averageLoad += it->second[metricIndex];
            }
        }
        else
        {
            // All secondaries have default load
            averageLoad += secondaryEntries_[metricIndex] * (replicas - 1);
        }
    }
    
    return static_cast<uint>(floor(static_cast<double>(averageLoad) / replicas + 0.5));
}

bool FailoverUnit::IsLoadAvailable(std::wstring const & metricName) const
{
    if (metricName == *ServiceModel::Constants::SystemMetricNameCpuCores || metricName == *ServiceModel::Constants::SystemMetricNameMemoryInMB)
    {
        return resourceLoadMap_.size() > 0;
    }
    else
    {
        // Default load is always available.
        // Check if metric is defined for this service is done outside.
        return true;
    }
}

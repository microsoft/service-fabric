// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "LoadOrMoveCostDescription.h"
#include "LoadMetric.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

LoadOrMoveCostDescription::LoadOrMoveCostDescription() :
    failoverUnitId_(Common::Guid::Empty()),
    serviceName_(),
    isStateful_(false),
    isReset_(false),
    primaryEntries_(),
    secondaryEntries_(),
    secondaryEntriesMap_()
{
}

LoadOrMoveCostDescription::LoadOrMoveCostDescription(
    Guid failoverUnitId,
    wstring && serviceName,
    bool isStateful,
    bool isReset) :
    failoverUnitId_(failoverUnitId),
    serviceName_(move(serviceName)),
    isStateful_(isStateful),
    isReset_(isReset)
{
}

LoadOrMoveCostDescription::LoadOrMoveCostDescription(LoadOrMoveCostDescription && other) :
    failoverUnitId_(other.failoverUnitId_),
    serviceName_(move(other.serviceName_)),
    isStateful_(other.isStateful_),
    isReset_(other.isReset_),
    primaryEntries_(move(other.primaryEntries_)),
    secondaryEntries_(move(other.secondaryEntries_)),
    secondaryEntriesMap_(move(other.secondaryEntriesMap_))
{
}

LoadOrMoveCostDescription & LoadOrMoveCostDescription::operator=(LoadOrMoveCostDescription && other)
{
    if (this != &other)
    {
        failoverUnitId_ = other.failoverUnitId_;
        serviceName_ = move(other.serviceName_);
        isStateful_ = other.isStateful_;
        isReset_ = other.isReset_;
        primaryEntries_ = move(other.primaryEntries_);
        secondaryEntries_ = move(other.secondaryEntries_);
        secondaryEntriesMap_ = move(other.secondaryEntriesMap_);
    }

    return *this;
}

void LoadOrMoveCostDescription::AdjustTimestamps(TimeSpan diff)
{
    if (diff != TimeSpan::Zero)
    {
        for (auto it = primaryEntries_.begin(); it != primaryEntries_.end(); ++it)
        {
            it->AdjustTimestamp(diff);
        }
        for (auto it = secondaryEntries_.begin(); it != secondaryEntries_.end(); ++it)
        {
            it->AdjustTimestamp(diff);
        }
        for (auto it = secondaryEntriesMap_.begin(); it != secondaryEntriesMap_.end(); ++it)
        {
            vector<LoadMetricStats>& loads = it->second;
            for (auto sIt = loads.begin(); sIt != loads.end(); ++sIt)
            {
                sIt->AdjustTimestamp(diff);
            }
        }
    }
}

bool LoadOrMoveCostDescription::CreateLoadReportForResources(
    Common::Guid const & ft, 
    std::wstring const & serviceName, 
    bool isStateful, 
    ReplicaRole::Enum replicaRole, 
    Federation::NodeId const& nodeId, 
    double cpuUsage, 
    uint64 memoryUsage,
    LoadOrMoveCostDescription & loadDescription)
{
    if (replicaRole == ReplicaRole::Primary || replicaRole == ReplicaRole::Secondary)
    {
        loadDescription.failoverUnitId_ = ft;
        loadDescription.serviceName_ = serviceName;
        loadDescription.isStateful_ = isStateful;

        vector<LoadMetric> loads;
        //memory is in MB once it reaches the RA already
        //cpu is corrected for PLB
        LoadMetric loadStatMemory(wstring(*ServiceModel::Constants::SystemMetricNameMemoryInMB), (uint)(memoryUsage));
        LoadMetric loadStatCpu(wstring(*ServiceModel::Constants::SystemMetricNameCpuCores), (uint)(ceil(cpuUsage * ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor)));
        loads.push_back(loadStatMemory);
        loads.push_back(loadStatCpu);

        loadDescription.MergeLoads(replicaRole, move(loads), Stopwatch::Now(), true, nodeId);

        return true;
    }
    else
    {
        return false;
    }
}

bool LoadOrMoveCostDescription::MergeLoads(
    ReplicaRole::Enum replicaRole,
    vector<LoadMetric> && loads,
    StopwatchTime timestamp,
    bool useNodeId,
    Federation::NodeId const& nodeId)
{
    // this method is called when RA aggregate a load report
    ASSERT_IF(replicaRole == ReplicaRole::None || !isStateful_ && replicaRole == ReplicaRole::Primary, "Invalid role {0} for failover unit with stateful/less {1}", replicaRole, isStateful_);

    bool changed = false;
    vector<LoadMetricStats> & entries = !isStateful_ || replicaRole == ReplicaRole::Secondary ? secondaryEntries_ : primaryEntries_;
    for (auto it = loads.begin(); it != loads.end(); ++it)
    {
        changed = FindAndUpdate(entries, move(*it), timestamp) || changed;
    }

    if (useNodeId && (!isStateful_ || replicaRole == ReplicaRole::Secondary))
    {
        changed = FindAndUpdateSecondaryLoad(secondaryEntriesMap_, nodeId, move(loads), timestamp);
    }

    return changed;
}

bool LoadOrMoveCostDescription::MergeLoads(LoadOrMoveCostDescription && loads)
{
    // this method is called when FM receives a load report from RA
    ASSERT_IF(failoverUnitId_ != loads.failoverUnitId_ || serviceName_ != loads.serviceName_ || isStateful_ != loads.isStateful_, "Merge of two different load description {0}, {1}", *this, loads);

    bool changed = false;
    for (auto it = loads.primaryEntries_.begin(); it != loads.primaryEntries_.end(); ++it)
    {
        changed = FindAndUpdate(primaryEntries_, move(*it)) || changed;
    }
    for (auto it = loads.secondaryEntries_.begin(); it != loads.secondaryEntries_.end(); ++it)
    {
        changed = FindAndUpdate(secondaryEntries_, move(*it)) || changed;
    }
    for (auto it = loads.secondaryEntriesMap_.begin(); it != loads.secondaryEntriesMap_.end(); ++it)
    {
        changed = FindAndUpdateSecondaryLoad(secondaryEntriesMap_, it->first, move(it->second)) || changed;
    }

    return changed;
}

void LoadOrMoveCostDescription::WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const
{
    writer.Write("{0} {1} {2}: ", failoverUnitId_, serviceName_, isStateful_);

    if (isStateful_)
    {
        writer.Write("Primary: ");
        for (LoadMetricStats const & loadMetric : primaryEntries_)
        {
            writer.Write("{0} ", loadMetric);
        }

        writer.Write("Secondary: ");
    }

    for (auto it = secondaryEntriesMap_.begin(); it != secondaryEntriesMap_.end(); ++it)
    {
        writer.Write("Node ID: {0}", it->first);
        for (LoadMetricStats const & loadMetric : it->second)
        {
            writer.Write("{0} ", loadMetric);
        }
    }
}

//------------------------------------------------------------
// private members
//------------------------------------------------------------

bool LoadOrMoveCostDescription::FindAndUpdate(vector<LoadMetricStats> & loadList, LoadMetric && newLoad, StopwatchTime timestamp)
{
    auto it = find_if(loadList.begin(), loadList.end(), [&](LoadMetricStats const & load) -> bool
    {
        return load.Name == newLoad.Name;
    });

    if(it == loadList.end())
    {
        loadList.push_back(LoadMetricStats(wstring(newLoad.Name), newLoad.Value, timestamp));
        return true;
    }
    else
    {
        return it->Update(newLoad.Value, timestamp);
    }
}

bool LoadOrMoveCostDescription::FindAndUpdate(vector<LoadMetricStats> & loadList, LoadMetricStats && newLoad)
{
    auto it = find_if(loadList.begin(), loadList.end(), [&](LoadMetricStats const & load) -> bool
    {
        return load.Name == newLoad.Name;
    });

    if(it == loadList.end())
    {
        loadList.push_back(move(newLoad));
        return true;
    }
    else
    {
        return it->Update(move(newLoad));
    }
}

bool LoadOrMoveCostDescription::FindAndUpdateSecondaryLoad(
    map<Federation::NodeId, std::vector<LoadMetricStats>> & secondaryLoad,
    Federation::NodeId const& nodeId,
    vector<LoadMetricStats> && newLoad)
{
    auto it = find_if(secondaryLoad.begin(), secondaryLoad.end(), [&](pair<Federation::NodeId, vector<LoadMetricStats>> const & load) -> bool
    {
        return load.first == nodeId;
    });

    if (it == secondaryLoad.end())
    {
        secondaryLoad.insert(pair<Federation::NodeId, vector<LoadMetricStats>>(nodeId, move(newLoad)));
        return true;
    }
    else
    {
        bool changed = false;
        vector<LoadMetricStats> & currentLoads = it->second;
        for (auto newIt = newLoad.begin(); newIt != newLoad.end(); ++newIt)
        {
            changed = FindAndUpdate(currentLoads, move(*newIt)) || changed;
        }
        return changed;
    }
}

bool LoadOrMoveCostDescription::FindAndUpdateSecondaryLoad(
    map<Federation::NodeId, std::vector<LoadMetricStats>> & secondaryLoad,
    Federation::NodeId const& nodeId,
    vector<LoadMetric> && newLoad,
    StopwatchTime timestamp)
{
    auto it = find_if(secondaryLoad.begin(), secondaryLoad.end(), [&](pair<Federation::NodeId, vector<LoadMetricStats>> const & load) -> bool
    {
        return load.first == nodeId;
    });

    if (it == secondaryLoad.end())
    {
        vector<LoadMetricStats> newLoadStats;
        for (auto itVec = newLoad.begin(); itVec != newLoad.end(); ++itVec)
        {
            newLoadStats.push_back(LoadMetricStats(wstring(itVec->Name), itVec->Value, timestamp));
        }
        secondaryLoad.insert(pair<Federation::NodeId, vector<LoadMetricStats>>(nodeId, move(newLoadStats)));
        return true;
    }
    else
    {
        bool changed = false;
        vector<LoadMetricStats> & currentLoads = it->second;
        for (auto newIt = newLoad.begin(); newIt != newLoad.end(); ++newIt)
        {
            changed = FindAndUpdate(currentLoads, move(*newIt), timestamp) || changed;
        }
        return changed;
    }
}

void LoadOrMoveCostDescription::AlignSingletonReplicaLoads()
{
    secondaryEntries_.clear();
    secondaryEntries_.insert(
        secondaryEntries_.begin(),
        primaryEntries_.begin(),
        primaryEntries_.end());
}


// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "LoadEntry.h"
#include "NodeEntry.h"
#include "PartitionEntry.h"
#include "ServiceEntry.h"
#include "PlacementReplica.h"
#include "LoadBalancingDomainEntry.h"
#include "PlacementAndLoadBalancing.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

LoadEntry NodeEntry::ConvertToFlatEntries(vector<LoadEntry> const& entries)
{
    vector<int64> newEntries;

    for (size_t i = 0; i < entries.size(); ++i)
    {
        newEntries.insert(newEntries.end(), entries[i].Values.begin(), entries[i].Values.end());
    }

    return LoadEntry(move(newEntries));
}

NodeEntry::NodeEntry(
    int nodeIndex,
    Federation::NodeId nodeId,
    LoadEntry && loads,
    LoadEntry && shouldDisappearLoads,
    LoadEntry && capacityRatios,
    LoadEntry && bufferedCapacities,
    LoadEntry && totalCapacities,
    TreeNodeIndex && faultDomainIndex,
    TreeNodeIndex && upgradeDomainIndex,
    bool isDeactivated,
    bool isUp,
    std::vector<std::wstring> && nodeImages,
    bool isValid,
    wstring && nodeTypeName)
    : nodeIndex_(nodeIndex),
    nodeId_(nodeId),
    loads_(move(loads)),
    shouldDisappearLoads_(move(shouldDisappearLoads)),
    capacityRatios_(move(capacityRatios)),
    bufferedCapacities_(move(bufferedCapacities)),
    totalCapacities_(move(totalCapacities)),
    hasCapacity_(false),
    faultDomainIndex_(move(faultDomainIndex)),
    upgradeDomainIndex_(move(upgradeDomainIndex)),
    isDeactivated_(isDeactivated),
    isUp_(isUp),
    nodeImages_(move(nodeImages)),
    maxConcurrentBuilds_(0),
    isValid_(isValid),
    nodeTypeName_(move(nodeTypeName))
{
    ASSERT_IFNOT(capacityRatios_.Values.size() == bufferedCapacities_.Values.size(),
        "Buffered capacities size {0} and CapacityRatios size {1} should be equal",
        bufferedCapacities_.Values.size(), capacityRatios_.Values.size());
    ASSERT_IFNOT(capacityRatios_.Values.size() == totalCapacities_.Values.size(),
        "Total capacities size {0} and CapacityRatios size {1} should be equal",
        totalCapacities_.Values.size(), capacityRatios_.Values.size());
    ASSERT_IFNOT(bufferedCapacities_.Values.size() <= loads_.Values.size(),
        "Global metric count {0} should be <= total metric count {1}",
        bufferedCapacities_.Values.size(), loads_.Values.size());
    ASSERT_IFNOT(loads_.Values.size() == shouldDisappearLoads_.Values.size(),
        "Total load metric count {0} and total shouldDisappear metric count should be equal",
        loads_.Values.size(), shouldDisappearLoads_.Values.size());

    auto it = find_if(bufferedCapacities_.Values.begin(), bufferedCapacities_.Values.end(), [](int64 value) { return value >= 0; });
    if (it != bufferedCapacities_.Values.end())
    {
        hasCapacity_ = true;
    }
}

NodeEntry::NodeEntry(NodeEntry && other)
    : nodeIndex_(other.nodeIndex_),
    nodeId_(move(other.nodeId_)),
    loads_(move(other.loads_)),
    shouldDisappearLoads_(move(other.shouldDisappearLoads_)),
    capacityRatios_(move(other.capacityRatios_)),
    bufferedCapacities_(move(other.bufferedCapacities_)),
    totalCapacities_(move(other.totalCapacities_)),
    hasCapacity_(other.hasCapacity_),
    faultDomainIndex_(move(other.faultDomainIndex_)),
    upgradeDomainIndex_(move(other.upgradeDomainIndex_)),
    isDeactivated_(other.isDeactivated_),
    isUp_(other.isUp_),
    nodeImages_(move(other.nodeImages_)),
    maxConcurrentBuilds_(other.maxConcurrentBuilds_),
    isValid_(other.isValid_),
    nodeTypeName_(move(other.nodeTypeName_))
{
}

NodeEntry & NodeEntry::operator = (NodeEntry && other)
{
    if (this != &other)
    {
        nodeIndex_ = other.nodeIndex_;
        nodeId_ = move(other.nodeId_);
        loads_ = move(other.loads_);
        shouldDisappearLoads_ = move(other.shouldDisappearLoads_);
        capacityRatios_ = move(other.capacityRatios_);
        bufferedCapacities_ = move(other.bufferedCapacities_);
        totalCapacities_ = move(other.totalCapacities_);
        hasCapacity_ = other.hasCapacity_;
        faultDomainIndex_ = move(other.faultDomainIndex_);
        upgradeDomainIndex_ = move(other.upgradeDomainIndex_);
        isDeactivated_ = other.isDeactivated_;
        isUp_ = other.isUp_;
        nodeImages_ = move(other.nodeImages_);
        isValid_ = other.isValid_;
        maxConcurrentBuilds_ = other.maxConcurrentBuilds_;
        nodeTypeName_ = move(other.nodeTypeName_);
    }

    return *this;
}

int64 NodeEntry::GetLoadLevel(size_t metricIndex) const
{
    ASSERT_IFNOT(metricIndex < loads_.Values.size(), "Metric index {0} out of bound {1}", metricIndex, loads_.Values.size());

    int64 load = loads_.Values[metricIndex];

    // note: we don't incorporate capacity ratios any more
    return load;
}

int64 NodeEntry::GetLoadLevel(size_t metricIndex, int64 diff) const
{
    int64 load = loads_.TryAddLoad(metricIndex, diff);

    // note: we don't incorporate capacity ratios any more
    return load;
}

int64 NodeEntry::GetNodeCapacity(size_t metricIndex) const
{
    TESTASSERT_IFNOT(metricIndex < totalCapacities_.Values.size(), "Metric index {0} out of bound {1}", metricIndex, totalCapacities_.Values.size());

    int64 capacity = totalCapacities_.Values[metricIndex];

    return capacity;
}

GlobalWString const NodeEntry::TraceDescription = make_global<wstring>(
    L"[Nodes]\r\n#nodeId #nodeIndex faultDomainIndex upgradeDomainIndex loads disappearingLoads capacityRatios bufferedCapacities totalCapacities isDeactivated isUp");

void NodeEntry::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    writer.Write("{0} {1} {2} {3} {4} {5} {6} {7} {8} {9} {10} {11} {12}",
        nodeId_,
        nodeIndex_,
        faultDomainIndex_,
        upgradeDomainIndex_,
        loads_,
        shouldDisappearLoads_,
        capacityRatios_,
        bufferedCapacities_,
        totalCapacities_,
        isDeactivated_,
        isUp_,
        nodeImages_,
        maxConcurrentBuilds_);
}

void NodeEntry::WriteToEtw(uint16 contextSequenceId) const
{
    PlacementAndLoadBalancing::PLBTrace->PLBDump(contextSequenceId, wformatString(*this));
}

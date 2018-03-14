// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Node.h"
#include "ServiceDomainMetric.h"
#include "ServiceDomainLocalMetric.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

GlobalWString const ServiceDomainMetric::FormatHeader = make_global<wstring>(L"Loads/DisappearingLoads/TotalCapacities");

ServiceDomainMetric::ServiceDomainMetric(wstring && name)
    : name_(move(name)),
    serviceCount_(0),
    notAffectingBalancingServiceCount_(0),
    applicationCount_(0),
    totalWeight_(0.0),
    nodeLoads_(),
    shouldDisappearNodeLoads_(),
    nodeLoadSum_(0),
    blockNodeMap_()
{
}

ServiceDomainMetric::ServiceDomainMetric(ServiceDomainMetric const& other)
    : name_(other.name_),
    serviceCount_(other.serviceCount_),
    notAffectingBalancingServiceCount_(other.notAffectingBalancingServiceCount_),
    applicationCount_(other.applicationCount_),
    totalWeight_(other.totalWeight_),
    nodeLoads_(other.nodeLoads_),
    shouldDisappearNodeLoads_(other.shouldDisappearNodeLoads_),
    nodeLoadSum_(other.nodeLoadSum_),
    blockNodeMap_(other.blockNodeMap_)
{
}

ServiceDomainMetric::ServiceDomainMetric(ServiceDomainMetric && other)
    : name_(move(other.name_)),
    serviceCount_(other.serviceCount_),
    notAffectingBalancingServiceCount_(other.notAffectingBalancingServiceCount_),
    applicationCount_(other.applicationCount_),
    totalWeight_(other.totalWeight_),
    nodeLoads_(move(other.nodeLoads_)),
    shouldDisappearNodeLoads_(move(other.shouldDisappearNodeLoads_)),
    nodeLoadSum_(move(other.nodeLoadSum_)),
    blockNodeMap_(move(other.blockNodeMap_))
{
}

void ServiceDomainMetric::IncreaseServiceCount(double metricWeightInService, bool affectsBalancing)
{
    ++serviceCount_;
    totalWeight_ += metricWeightInService;

    if (!affectsBalancing)
    {
        ++notAffectingBalancingServiceCount_;
    }
}

void ServiceDomainMetric::DecreaseServiceCount(double metricWeightInService, bool affectsBalancing)
{
    ASSERT_IFNOT(serviceCount_ > 0, "Decreasing service count when it is 0");

    --serviceCount_;
    totalWeight_ -= metricWeightInService;

    if (!affectsBalancing)
    {
        TESTASSERT_IFNOT(notAffectingBalancingServiceCount_ > 0, "Decreasing notAffectingBalancingServiceCount_ count when it is 0");
        --notAffectingBalancingServiceCount_;
    }
}

void ServiceDomainMetric::AddLoad(Federation::NodeId node, uint load, bool shouldDisappear)
{
    if (load > 0)
    {
        if (shouldDisappear)
        {
            AddLoadInternal(node, load, shouldDisappearNodeLoads_);
        }
        else
        {
            AddLoadInternal(node, load, nodeLoads_);
            nodeLoadSum_ += load;
        }

    }
}

void ServiceDomainMetric::AddTotalLoad(uint load)
{
    if (load > 0)
    {
        nodeLoadSum_ += load;
    }
}

void ServiceDomainMetric::AddLoadInternal(Federation::NodeId node, uint load, std::map<Federation::NodeId, int64> & loads)
{
    auto itNode = loads.find(node);
    if (itNode == loads.end())
    {
        itNode = loads.insert(make_pair(node, 0)).first;
    }

    itNode->second += load;
}

void ServiceDomainMetric::DeleteLoad(Federation::NodeId node, uint load, bool shouldDisappear)
{
    if (load > 0)
    {
        if (shouldDisappear)
        {
            DeleteLoadInternal(node, load, shouldDisappearNodeLoads_);
        }
        else
        {
            ASSERT_IF(nodeLoadSum_ < load, "Existing nodeLoadSum is smaller than load to be deleted");
            DeleteLoadInternal(node, load, nodeLoads_);
            nodeLoadSum_ -= load;
        }

    }
}

void ServiceDomainMetric::DeleteTotalLoad(uint load)
{
    if (load > 0)
    {
        ASSERT_IF(nodeLoadSum_ < load, "Existing nodeLoadSum is smaller than load to be deleted");
        nodeLoadSum_ -= load;
    }
}

void ServiceDomainMetric::DeleteLoadInternal(Federation::NodeId node, uint load, std::map<Federation::NodeId, int64> & loads)
{
    auto itNode = loads.find(node);
    ASSERT_IF(itNode == loads.end(), "Node {0} doesn't exist", node);

    ASSERT_IF(itNode->second < load, "Existing load is smaller than load to be deleted");
    if (itNode->second == load)
    {
        loads.erase(itNode);
    }
    else
    {
        itNode->second -= load;
    }
}

int64 ServiceDomainMetric::GetLoad(Federation::NodeId node, bool shouldDisappear) const
{
    std::map<Federation::NodeId, int64> const* loads = shouldDisappear ? &shouldDisappearNodeLoads_ : &nodeLoads_;

    auto itNode = loads->find(node);
    if (itNode != loads->end())
    {
        return itNode->second;
    }
    else
    {
        return 0;
    }
}

void ServiceDomainMetric::UpdateBlockNodeServiceCount(uint64 nodeIndex, int delta)
{
    auto it = blockNodeMap_.find(nodeIndex);
    if (it == blockNodeMap_.end())
    {
        ASSERT_IF(delta < 0, "delta shouldn't be negative when blockNodeMap_ dosen't contain the block node {0}; {1} received", nodeIndex, delta);
        it = blockNodeMap_.insert(make_pair(nodeIndex, 0)).first;
    }

    ASSERT_IF(it->second + delta < 0, "The end result of block node service count should not be negative: {0}, {1}", it->second, delta);
    it->second += delta;

    if (it->second == 0)
    {
        blockNodeMap_.erase(it);
    }
}

DynamicBitSet ServiceDomainMetric::GetBlockNodeList() const
{
    DynamicBitSet blockList;
    for (auto it = blockNodeMap_.begin(); it != blockNodeMap_.end(); ++it)
    {
        // all services that have this metric have bloclisted this node (except services that are spanned across all nodes),
        // so this metric should not consider this node
        if (it->second == static_cast<int64>(serviceCount_) - static_cast<int64>(notAffectingBalancingServiceCount_))
        {
            blockList.Add(it->first);
        }
    }
    return blockList;
}

void ServiceDomainMetric::VerifyLoads(ServiceDomainLocalMetric const& baseLoads, bool includeDisappearingReplicas) const
{
    std::map<Federation::NodeId, int64> const& loads = includeDisappearingReplicas ? shouldDisappearNodeLoads_ : nodeLoads_;

    for (auto it = loads.begin(); it != loads.end(); ++it)
    {
        int64 rhsLoad = baseLoads.GetLoad(it->first);
        ASSERT_IF(it->second != rhsLoad, "Verifying ServiceDomainMetric {0}loads failed: {1} does not match {2} on node {3}",
            includeDisappearingReplicas ? "includeDisappearingReplicas " : "", it->second, rhsLoad, it->first);
    }
}

void ServiceDomainMetric::VerifyAreZero(bool includeDisappearingReplicas) const
{
    std::map<Federation::NodeId, int64> const& loads = includeDisappearingReplicas ? shouldDisappearNodeLoads_ : nodeLoads_;

    for (auto it = loads.begin(); it != loads.end(); ++it)
    {
        ASSERT_IF(it->second != 0, "Verifying ServiceDomainMetric loads failed: {0}Load 0 expected on node {1}, {2} received",
            includeDisappearingReplicas ? "includeDisappearingReplicas" : "", it->first, it->second);
    }
}

bool ServiceDomainMetric::WriteFor(Node node, StringWriterA& w) const
{
    int64 load = GetLoad(node.NodeDescriptionObj.NodeId);
    int64 shouldDisappearLoad = GetLoad(node.NodeDescriptionObj.NodeId, true);

    auto & totalCapacities = node.NodeDescriptionObj.Capacities;
    auto itTotalCapacity = totalCapacities.find(name_);
    int64 totalCapacity = itTotalCapacity == totalCapacities.end() ? -1LL : static_cast<int64>(itTotalCapacity->second);

    w.Write(" ");
    if (totalCapacity < 0)
    {
        w.Write(Common::StringLiteral("{0}/{1}/-"), move(load), move(shouldDisappearLoad));
    }
    else if (load <= totalCapacity && shouldDisappearLoad <= totalCapacity)
    {
        w.Write("{0}/{1}/{2}", load, shouldDisappearLoad, totalCapacity);
    }
    else if (load <= totalCapacity && shouldDisappearLoad > totalCapacity)
    {
        w.Write("{0}/[{1}]/{2}", load, shouldDisappearLoad, totalCapacity);
    }
    else if (load > totalCapacity && shouldDisappearLoad <= totalCapacity)
    {
        w.Write("[{0}]/{1}/{2}", load, shouldDisappearLoad, totalCapacity);
    }
    else if (load > totalCapacity && shouldDisappearLoad > totalCapacity)
    {
        w.Write("[{0}]/[{1}]/{2}", load, shouldDisappearLoad, totalCapacity);
    }
    return (load > 0 || shouldDisappearLoad > 0);
}

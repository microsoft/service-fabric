// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ApplicationLoad.h"
#include "PlacementAndLoadBalancing.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

GlobalWString const ApplicationLoad::FormatHeader = make_global<wstring>(L"{Metric:Load}");

ApplicationLoad::ApplicationLoad(uint64 applicationId)
    : applicationId_(applicationId),
    loadTable_(),
    metricLoads_(),
    nodeCounts_()
{
}

ApplicationLoad::ApplicationLoad(ApplicationLoad && other)
    : applicationId_(move(other.applicationId_)),
    loadTable_(move(other.loadTable_)),
    metricLoads_(move(other.metricLoads_)),
    nodeCounts_(move(other.nodeCounts_))
{
}

void ApplicationLoad::AddLoad(std::wstring const& metricName, int64 load)
{
    auto it = loadTable_.find(metricName);
    if (it == loadTable_.end())
    {
        loadTable_.insert(make_pair(metricName, load));
    }
    else
    {
        it->second += load;
    }
}

void ApplicationLoad::DeleteLoad(std::wstring const& metricName, int64 load)
{
    auto it = loadTable_.find(metricName);

    if (it != loadTable_.end())
    {
        ASSERT_IF(it->second < load, "Application existing load {0} is smaller than load to be deleted {1}", it->second, load);

        it->second -= load;
        if (it->second == 0)
        {
            loadTable_.erase(it);
        }
    }
}

void ApplicationLoad::AddNodeLoad(std::wstring const& metricName, Federation::NodeId node, uint load, bool shouldDisappear)
{
    if (load > 0)
    {
        auto itApplicationLoad = metricLoads_.find(metricName);
        if (itApplicationLoad == metricLoads_.end())
        {
            // Insert the load if it does not exist for this metric
            itApplicationLoad = metricLoads_.insert(make_pair(metricName, ServiceDomainMetric(wstring(metricName)))).first;
        }
        itApplicationLoad->second.AddLoad(node, load, shouldDisappear);
    }
}

void ApplicationLoad::DeleteNodeLoad(std::wstring const& metricName, Federation::NodeId node, uint load, bool shouldDisappear)
{
    if (load > 0)
    {
        auto itApplicationLoad = metricLoads_.find(metricName);
        ASSERT_IF(itApplicationLoad == metricLoads_.end(), "ServiceDomainMetric does not exist when removing application load.");
        itApplicationLoad->second.DeleteLoad(node, load, shouldDisappear);
    }
}

void ApplicationLoad::AddReplicaToNode(Federation::NodeId node)
{
    auto itNodeCount = nodeCounts_.find(node);
    if (itNodeCount == nodeCounts_.end())
    {
        itNodeCount = nodeCounts_.insert(make_pair(node, 0)).first;
    }
    nodeCounts_[node]++;
}

void ApplicationLoad::DeleteReplicaFromNode(Federation::NodeId node)
{
    auto itNodeCount = nodeCounts_.find(node);
    ASSERT_IF(itNodeCount == nodeCounts_.end(), "Node should exist when decreasing node count");
    ASSERT_IF(nodeCounts_[node] == 0, "Number of replicas on the node should be greater than zero when removing replica");
    nodeCounts_[node]--;

    if (nodeCounts_[node] == 0)
    {
        nodeCounts_.erase(itNodeCount);
    }
}

// return actual load if less than capacity; otherwise, return capacity
int64 ApplicationLoad::GetReservedLoadUsed(std::wstring const& metricName, int64 totalReservedCapacity) const
{
    if (loadTable_.empty())
    {
        return 0;
    }

    auto loadIt = loadTable_.find(metricName);
    if (loadIt == loadTable_.end())
    {
        return 0;
    }
    else
    {
        auto actualLoad = loadIt->second;
        return actualLoad < totalReservedCapacity ? actualLoad : totalReservedCapacity;
    }

}

void ApplicationLoad::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    writer.Write("{0}: {1}", applicationId_, loadTable_);
}

bool ApplicationLoad::WriteFor(Application const& app, StringWriterA& w) const
{
    //If we're in the wrong ApplicationLoad, just return...
    if (app.ApplicationDesc.ApplicationId != applicationId_ || loadTable_.empty()) { return false; }

    //Printing ApplicationLoad as per the FormatHeader...
    for (auto itMetric = loadTable_.begin(); itMetric != loadTable_.end(); itMetric++)
    {
        w.Write("{0}:{1}", itMetric->first, itMetric->second);
    }
    return true;
}

void ApplicationLoad::Merge(ApplicationLoad& other)
{
    // merge loads
    for (auto itOtherLoad = other.loadTable_.begin(); itOtherLoad != other.loadTable_.end(); ++itOtherLoad)
    {
        auto itLoad = loadTable_.find(itOtherLoad->first);
        if (itLoad == loadTable_.end())
        {
            loadTable_.insert(make_pair(move(itOtherLoad->first), move(itOtherLoad->second)));
        }
        else
        {
            PlacementAndLoadBalancing::PLBTrace->MergingApplicationLoad(L"loadTable", applicationId_, itOtherLoad->first);
            TESTASSERT_IF(itLoad != loadTable_.end(), 
                "Merging loadTable while merging ApplicationLoad for metric {0} which is contained in both domains. Domains weren't merged well.", 
                itOtherLoad->first);
        }
    }

    // merge node counts
    for (auto itOtherNodeCount = other.NodeCounts.begin(); itOtherNodeCount != other.NodeCounts.end(); ++itOtherNodeCount)
    {
        auto itNodeCount = nodeCounts_.find(itOtherNodeCount->first);
        if (itNodeCount != nodeCounts_.end())
        {
            itNodeCount->second += itOtherNodeCount->second;
        }
        else
        {
            nodeCounts_.insert(make_pair(move(itOtherNodeCount->first), move(itOtherNodeCount->second)));
        }
    }

    // merge metric loads
    for (auto itOtherMetricLoad = other.MetricLoads.begin(); itOtherMetricLoad != other.MetricLoads.end(); ++itOtherMetricLoad)
    {
        auto itMetricLoad = metricLoads_.find(itOtherMetricLoad->first);
        if (itMetricLoad == metricLoads_.end())
        {
            metricLoads_.insert(make_pair(move(itOtherMetricLoad->first), itOtherMetricLoad->second));
        }
        else
        {
            PlacementAndLoadBalancing::PLBTrace->MergingApplicationLoad(L"metricLoads", applicationId_, itOtherMetricLoad->first);
            TESTASSERT_IF(itMetricLoad != metricLoads_.end(),
                "Merging metricTable while merging ApplicationLoad for metric {0} which is contained in both domains. Domains weren't merged well.",
                itOtherMetricLoad->first);
        }
    }
}

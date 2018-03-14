// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ServiceDomain.h"
#include "PLBConfig.h"
#include "PlacementAndLoadBalancing.h"
#include "ServiceMetric.h"
#include "MetricGraph.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

MetricGraph::MetricGraph()
:
plb_(nullptr)
{
}

MetricGraph::MetricGraph(MetricGraph && other)
: 
metricConnectionIds_(move(other.metricConnectionIds_)),
metricConnections_(move(other.metricConnections_)),
metrics_(move(other.metrics_)),
plb_(other.plb_)
{
}

MetricGraph::MetricGraph(MetricGraph const& other)
: 
metricConnectionIds_(other.metricConnectionIds_),
metricConnections_(other.metricConnections_),
metrics_(other.metrics_),
plb_(other.plb_)
{
}

MetricGraph & MetricGraph::operator = (MetricGraph && other)
{
    if (this != &other) 
    {
        metricConnectionIds_ = move(other.metricConnectionIds_);
        metricConnections_ = move(other.metricConnections_);
        metrics_ = move(other.metrics_);
    }

    return *this;
}

/// <summary>
/// Sets the PLB.
/// </summary>
/// <param name="plb">The PLB.</param>
void MetricGraph::SetPLB(PlacementAndLoadBalancing * plb)
{
    plb_ = plb;
}

/// <summary>
/// Gets the metric connection identifier.
/// </summary>
/// <param name="metricName">Name of the metric.</param>
/// <returns>the metric connection identifier</returns>
int MetricGraph::GetMetricConnectionId(wstring const& metricName)
{
    if (metricConnectionIds_.find(metricName) == metricConnectionIds_.end())
    {
        metricConnectionIds_.insert(make_pair(metricName, static_cast<int>(metricConnectionIds_.size())));
        metricConnections_.push_back(new unordered_map<int, uint>());
        metrics_.push_back(metricName);
        DBGASSERT_IF(metricConnectionIds_.size() != metricConnections_.size() || metricConnections_.size() != metrics_.size(), "Improper insertion");
    }
    return metricConnectionIds_[metricName];
}

/// <summary>
/// Adds Or removes the metric affinity. In essence, adds of removes the metric connection between the lowest metrics of both metric sets
/// </summary>
/// <param name="metrics1">Metrics of first set.</param>
/// <param name="metrics2">Metrics of first set.</param>
/// <param name="add">If set to <c>true</c> [add]. If set to <c>false</c> [remove].</param>
/// <returns>whether a domain merge/split was queued.</returns>
bool MetricGraph::AddOrRemoveMetricAffinity(vector<ServiceMetric> const& metrics1, vector<ServiceMetric> const& metrics2, bool add)
{
    ASSERT_IF(plb_ == nullptr, "PLB not declared");
    if (metrics1.empty() || metrics2.empty()) return false;
    int minId1 = -1;
    int minId2 = -1;

    for (auto itMetric = metrics1.begin(); itMetric != metrics1.end(); ++itMetric)
    {
        int id = GetMetricConnectionId(itMetric->Name);
        if (minId1 == -1 || id < minId1)
        {
            minId1 = id;
        }
    }
    for (auto itMetric = metrics2.begin(); itMetric != metrics2.end(); ++itMetric)
    {
        int id = GetMetricConnectionId(itMetric->Name);
        if (minId2 == -1 || id < minId2)
        {
            minId2 = id;
        }
    }

    if (add)
    {
        return AddMetricConnection(minId1, minId2);
    }
    else
    {
        return RemoveMetricConnection(minId1, minId2);
    }
}

/// <summary>
/// Adds Or removes the metric affinity. In essence, adds of removes the metric connection between the lowest metrics of both metric sets
/// </summary>
/// <param name="metrics1">Metrics of first set.</param>
/// <param name="metrics2">Metrics of second set.</param>
/// <param name="add">If set to <c>true</c> [add]. If set to <c>false</c> [remove].</param>
/// <returns>whether a domain merge/split was queued.</returns>
bool MetricGraph::AddOrRemoveMetricAffinity(vector<ServiceMetric> const& metrics1, vector<wstring> const& metrics2, bool add)
{
    ASSERT_IF(plb_ == nullptr, "PLB not declared");
    if (metrics1.empty() || metrics2.empty()) return false;
    int minId1 = -1;
    int minId2 = -1;

    for (auto itMetric = metrics1.begin(); itMetric != metrics1.end(); ++itMetric)
    {
        int id = GetMetricConnectionId(itMetric->Name);
        if (minId1 == -1 || id < minId1)
        {
            minId1 = id;
        }
    }
    for (auto itMetric = metrics2.begin(); itMetric != metrics2.end(); ++itMetric)
    {
        int id = GetMetricConnectionId(*itMetric);
        if (minId2 == -1 || id < minId2)
        {
            minId2 = id;
        }
    }

    if (add)
    {
        return AddMetricConnection(minId1, minId2);
    }
    else
    {
        return RemoveMetricConnection(minId1, minId2);
    }
}

/// <summary>
/// Adds/Removes metric connections amongst metrics of a single service. In essence, it connects.disconnects lowest metricId to all other metricIds
/// </summary>
/// <param name="metrics">The metrics.</param>
/// <param name="add">if set to <c>true</c> [add]. If set to <c>false</c>, remove</param>
/// <returns>whether a domain merge/split was queued.</returns>
bool MetricGraph::AddOrRemoveMetricConnectionsForService(vector<ServiceMetric> const& metrics, bool add)
{
    ASSERT_IF(plb_ == nullptr, "PLB not declared");
    bool ret = false;
    if (metrics.size() <= 1) return ret;
    vector<int> metricIds;
    auto itMetric = metrics.begin();
    int minId = GetMetricConnectionId(itMetric->Name);
    itMetric++;
    while (itMetric != metrics.end())
    {
        int id = GetMetricConnectionId(itMetric->Name);
        if (id < minId)
        {
            metricIds.push_back(minId);
            minId = id;
        }
        else
        {
            metricIds.push_back(id);
        }
        itMetric++;
    }
    for (auto itId = metricIds.begin(); itId != metricIds.end(); ++itId)
    {
        if (add)
        {
            ret |= AddMetricConnection(minId, *itId);
        }
        else
        {
            ret |= RemoveMetricConnection(minId, *itId);
        }
    }
    return ret;
}

/// <summary>
/// Adds/Removes metric connections amongst metrics of a single service. In essence, it connects.disconnects lowest metricId to all other metricIds
/// </summary>
/// <param name="metrics">The metrics.</param>
/// <param name="add">if set to <c>true</c> [add]. If set to <c>false</c>, remove</param>
/// <returns>whether a domain merge/split was queued.</returns>
bool MetricGraph::AddOrRemoveMetricConnection(vector<wstring> const& metrics, bool add)
{
    ASSERT_IF(plb_ == nullptr, "PLB not declared");
    bool ret = false;
    if (metrics.size() <= 1) return ret;
    vector<int> metricIds;
    auto itMetric = metrics.begin();
    int minId = GetMetricConnectionId(*itMetric);
    itMetric++;
    while (itMetric != metrics.end())
    {
        int id = GetMetricConnectionId(*itMetric);
        if (id < minId)
        {
            metricIds.push_back(minId);
            minId = id;
        }
        else
        {
            metricIds.push_back(id);
        }
        itMetric++;
    }
    for (auto itId = metricIds.begin(); itId != metricIds.end(); ++itId)
    {
        if (add)
        {
            ret |= AddMetricConnection(minId, *itId);
        }
        else
        {
            ret |= RemoveMetricConnection(minId, *itId);
        }
    }
    return ret;
}

/// <summary>
///  Adds a single metric connection.
/// </summary>
/// <param name="metric1">The metric1.</param>
/// <param name="metric2">The metric2.</param>
/// <returns>Returns whether a domain merge was queued</returns>
bool MetricGraph::AddMetricConnection(wstring const& metric1, wstring const& metric2)
{
    return AddMetricConnection(GetMetricConnectionId(metric1), GetMetricConnectionId(metric2));
}

/// <summary>
/// Adds a single metric connection. If the two metrics being connected belong to different servicedomains, we queue a domain merge. Also, we update the metricToDomainTable_ if it is missing some entries...
/// </summary>
/// <param name="metricId1">The metric id1.</param>
/// <param name="metricId2">The metric id2.</param>
/// <returns>Returns whether a domain merge was queued</returns>
bool MetricGraph::AddMetricConnection(int metricId1, int metricId2)
{
    ASSERT_IF(plb_ == nullptr, "PLB not declared");
    if (metricId1 == metricId2)
    {
        return false;
    }

    bool toMerge = false;
    if (metricId1 > metricId2)
    {
        swap(metricId1, metricId2);
    }
    //pair<int, int> index = metricId1 < metricId2 ? make_pair(metricId1, metricId2) : make_pair(metricId2, metricId1);

    auto& row = *metricConnections_[metricId1];
    auto column = metricId2;
    if (row.find(column) == row.end())
    {
        //might be time to merge servicedomains
        auto itDomain1 = plb_->metricToDomainTable_.find(metrics_[metricId1]);
        auto itDomain2 = plb_->metricToDomainTable_.find(metrics_[metricId2]);
        if (itDomain1 != plb_->metricToDomainTable_.end() && itDomain2 != plb_->metricToDomainTable_.end() && itDomain1->second != itDomain2->second)
        {
            plb_->QueueDomainChange(itDomain1->second->second.Id, true);
            plb_->QueueDomainChange(itDomain2->second->second.Id, true);
            toMerge = true;
        }
        else if (itDomain1 != plb_->metricToDomainTable_.end() && itDomain2 == plb_->metricToDomainTable_.end())
        {
            plb_->AddMetricToDomain(metrics_[metricId2], itDomain1->second);
        }
        else if (itDomain2 != plb_->metricToDomainTable_.end() && itDomain1 == plb_->metricToDomainTable_.end())
        {
            plb_->AddMetricToDomain(metrics_[metricId1], itDomain2->second);
        }

        row.insert(make_pair(column, 1));
    }
    else
    {
        row[column]++;
    }
    return toMerge;
}

/// <summary>
/// Removes a single metric connection in the connections graph.
/// </summary>
/// <param name="metric1">The metric1.</param>
/// <param name="metric2">The metric2.</param>
/// <returns>Returns whether a domain split was queued</returns>
bool MetricGraph::RemoveMetricConnection(wstring const& metric1, wstring const& metric2)
{
    return RemoveMetricConnection(GetMetricConnectionId(metric1), GetMetricConnectionId(metric2));
}

/// <summary>
/// Removes a single metric connection in the connections graph. If removal results in two metrics being disconnected, we queue a domain split.
/// </summary>
/// <param name="metricId1">The metric id1.</param>
/// <param name="metricId2">The metric id2.</param>
/// <returns>Returns whether a domain split was queued</returns>
bool MetricGraph::RemoveMetricConnection(int metricId1, int metricId2)
{
    ASSERT_IF(plb_ == nullptr, "PLB not declared");
    if (metricId1 == metricId2)
    {
        return false;
    }

    bool toSplit = false;
    if (metricId1 > metricId2)
    {
        swap(metricId1, metricId2);
    }
    //pair<int, int> index = metricId1 < metricId2 ? make_pair(metricId1, metricId2) : make_pair(metricId2, metricId1);

    auto& row = *metricConnections_[metricId1];
    auto column = metricId2;
    if (row.find(column) == row.end())
    {
        DBGASSERT_IF(!metricConnections_.empty(), "Unexpected MetricConnection Delete: {0}-{1}", metricId1, metricId2);
        return false;
    }
    else
    {
        row[column]--;
    }
    if (row[column] == 0)
    {
        row.erase(column);
        auto itDomain = plb_->metricToDomainTable_.find(metrics_[metricId1]);
        bool toSplitDomain = PLBConfig::GetConfig().SplitDomainEnabled;
        //might be time to split domains
        if (toSplitDomain && itDomain != plb_->metricToDomainTable_.end())
        {
            plb_->QueueDomainChange(itDomain->second->second.Id, false);
            toSplit = true;
        }
    }
    return toSplit;
}

/// <summary>
/// Determines whether the metrics are connected overall. Uses Dijkstra's algorithm, minus the min priority queues.
/// </summary>
/// <param name="metricsVector">Vector of metrics.</param>
/// <returns>Whether the metrics are all connected or not</returns>
bool MetricGraph::AreMetricsConnected(map<wstring, ServiceDomainMetric> const& metrics)
{
    ASSERT_IF(plb_ == nullptr, "PLB not declared");
    if (metrics.size() <= 1) return true;
    set<int> discoveredGraph;
    set<int> remainingGraph;
    set<int> toTraverse;
    int minId = -1;
    for (auto it = metrics.begin(); it != metrics.end(); it++)
    {
        int metric = GetMetricConnectionId(it->second.Name);
        remainingGraph.insert(metric);
        if (minId == -1 || metric < minId)
        {
            minId = metric;
        }
    }
    toTraverse.insert(minId);

    while (!toTraverse.empty())
    {
        //current Node
        int node = *(toTraverse.begin());
        //basic operations..
        discoveredGraph.insert(node);
        if (remainingGraph.count(node) > 0)
        {
            remainingGraph.erase(node);
        }
        //adding neighbors ->
        //we take const& because we dont plan to change the vector...
        auto const& row = *metricConnections_[node];
        for (auto itNode2 = row.begin(); itNode2 != row.end(); itNode2++)
        {
            TESTASSERT_IF(itNode2->second == 0, "Node {0}:{1} has empty entries in the metric graph", node, metrics_[node]);
            //we are safe to insert here because the node graph has edges in sorted numberical order. hence each node will only be traversed once.
            toTraverse.insert(itNode2->first);
        }
        //adding neighbors <-
        for (int i = 0; i < node; i++)
        {
            if (discoveredGraph.find(i) != discoveredGraph.end())
            {
                continue;
            }

            if (metricConnections_[i]->find(node) != metricConnections_[i]->end())
            {
                toTraverse.insert(i);
                discoveredGraph.insert(i);
            }
        }
        //done traversing
        toTraverse.erase(node);
    }

    //if we traversed all the nodes, then the set is connected.
    return remainingGraph.empty();
}

//this is not currently used, but I'm keeping it in. TODO: perform cleanup if metric graph gets too big.
void MetricGraph::Clean()
{
    int metric = 0;
    while (metric < metrics_.size())
    {
        auto metricName = metrics_[metric];
        //has outgoing connections...
        if (metricConnections_[metric]->size() > 0) 
        {
            metric++;
        }
        else
        {
            bool isUsed = false;
            for (int i = 0; i < metric; i++)
            {
                if (metricConnections_[i]->find(metric) != metricConnections_[i]->end())
                {
                    isUsed = true;
                    break;
                }
            }
            //has incoming connections...
            if (isUsed)
            {
                metric++;
            }
            //has no connections...
            else
            {
                metricConnections_.erase(metricConnections_.begin() + metric);
                metrics_.erase(metrics_.begin() + metric);
                metricConnectionIds_.erase(metricName);

                for (int i = 0; i < metricConnections_.size(); i++)
                {
                    auto map = metricConnections_[i];
                    unordered_map<int, uint>* map2 = new unordered_map<int, uint>();
                    for (auto itMap = map->begin(); itMap != map->end(); itMap++)
                    {
                        //decrement all indices after this...
                        if (itMap->first > metric)
                        {
                            map2->insert(make_pair(itMap->first - 1, itMap->second));
                        }
                        else
                        {
                            map2->insert(make_pair(itMap->first, itMap->second));
                        }
                    }
                    free(map);
                    metricConnections_[i] = map2;
                }

                for (auto itId = metricConnectionIds_.begin(); itId != metricConnectionIds_.end(); itId++)
                {
                    if (itId->second > metric)
                    {
                        itId->second--;
                    }
                }
            }
        }
    }
}

void MetricGraph::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    for (int i = 0; i < metricConnections_.size(); i++)
    {
        writer.Write("{\r\n");
        for (auto it2 = metricConnections_[i]->begin(); it2 != metricConnections_[i]->end(); it2++)
        {
            writer.Write("({0}<->{1}):{2}\r\n", metrics_[i], metrics_[it2->first],it2->second);
        }
        writer.Write("}");
    }
}

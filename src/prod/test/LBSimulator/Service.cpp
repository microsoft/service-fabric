// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Node.h"
#include "Service.h"

using namespace std;
using namespace Common;
using namespace LBSimulator;

std::wstring const Service::PrimaryCountName(L"PrimaryCount");
std::wstring const Service::SecondaryCountName(L"SecondaryCount");
std::wstring const Service::ReplicaCountName(L"ReplicaCount");
std::wstring const Service::InstanceCountName(L"InstanceCount");
std::wstring const Service::CountName(L"Count");

bool Service::IsBuiltInMetric(std::wstring const& metricName)
{
    return
        metricName == PrimaryCountName ||
        metricName == SecondaryCountName ||
        metricName == ReplicaCountName ||
        metricName == InstanceCountName ||
        metricName == CountName;
}

Service::Service(int index, wstring serviceName, bool isStateful, int partitionCount, int replicaCountPerPartition,
    wstring affinitizedService, set<int> && blockList, vector<Metric> && metrics, uint defaultMoveCost,
    wstring placementConstraints, int minReplicaSetSize, int startFailoverUnitIndex, bool hasPersistedState)
    : index_(index),
      serviceName_(move(serviceName)),
      isStateful_(isStateful),
      partitionCount_(partitionCount),
      targetReplicaSetSize_(replicaCountPerPartition),
      affinitizedService_(affinitizedService),
      blockList_(move(blockList)),
      startFailoverUnitIndex_(startFailoverUnitIndex),
      metrics_(move(metrics)),
      defaultMoveCost_(defaultMoveCost),
      placementConstraints_(move(placementConstraints)),
      minReplicaSetSize_(move(minReplicaSetSize)),
      hasPersistedState_(hasPersistedState)
{
}

Service::Service(Service && other)
    : index_(other.index_),
      serviceName_(other.serviceName_),
      isStateful_(other.isStateful_),
      partitionCount_(other.partitionCount_),
      targetReplicaSetSize_(other.targetReplicaSetSize_),
      affinitizedService_(other.affinitizedService_),
      blockList_(move(other.blockList_)),
      startFailoverUnitIndex_(other.startFailoverUnitIndex_),
      metrics_(move(other.metrics_)),
      defaultMoveCost_(other.defaultMoveCost_),
      placementConstraints_(other.placementConstraints_),
      minReplicaSetSize_(other.minReplicaSetSize_),
      hasPersistedState_(other.hasPersistedState_)
{
}

Service & Service::operator = (Service && other)
{
    if (this != &other)
    {
        index_ = other.index_;
        serviceName_ = other.serviceName_;
        isStateful_= other.isStateful_;
        partitionCount_ = other.partitionCount_;
        targetReplicaSetSize_ = other.targetReplicaSetSize_;
        affinitizedService_ = other.affinitizedService_;
        blockList_ = move(other.blockList_);
        startFailoverUnitIndex_ = other.startFailoverUnitIndex_;
        metrics_ = move(other.metrics_);
        defaultMoveCost_ = other.defaultMoveCost_;
        placementConstraints_ = other.placementConstraints_;
        hasPersistedState_ = other.hasPersistedState_;
    }

    return *this;
}

bool Service::InBlockList(int nodeIndex) const
{
    return (blockList_.find(nodeIndex) != blockList_.end());
}

Reliability::LoadBalancingComponent::ServiceDescription Service::GetPLBServiceDescription() const
{
    vector<Reliability::LoadBalancingComponent::ServiceMetric> tempMetrics;
    for each (Metric const& metric in metrics_)
    {
        tempMetrics.push_back(Reliability::LoadBalancingComponent::ServiceMetric(wstring(metric.Name),
            metric.Weight, metric.PrimaryDefaultLoad, metric.SecondaryDefaultLoad));
    }

    return Reliability::LoadBalancingComponent::ServiceDescription(
            StringUtility::ToWString(serviceName_),
            StringUtility::ToWString(index_) + L"_type",
            wstring(L""),
            isStateful_,
            wstring(placementConstraints_),
            wstring(affinitizedService_),
            true,
            move(tempMetrics),
            defaultMoveCost_,
            false,
            PartitionCount,
            TargetReplicaSetSize
            );
}
Reliability::LoadBalancingComponent::ServiceTypeDescription Service::GetPLBServiceTypeDescription() const
{
    return Reliability::LoadBalancingComponent::ServiceTypeDescription(
        StringUtility::ToWString(index_) + L"_type",
        set<Federation::NodeId>()
        );

}
Reliability::LoadBalancingComponent::ServiceTypeDescription Service::GetPLBServiceTypeDescription(map<int, Node> const& nodes) const
{
    set<Federation::NodeId> tempBlockList;
    for each (int nodeIndex in blockList_)
    {
        auto itNode = nodes.find(nodeIndex);
        if (itNode != nodes.end() && itNode->second.IsUp)
        {
            tempBlockList.insert(Node::CreateNodeId(nodeIndex));
        }
    }

    return Reliability::LoadBalancingComponent::ServiceTypeDescription(
            StringUtility::ToWString(index_) + L"_type",
            move(tempBlockList)
            );
}

void Service::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("{0} {1} {2} {3} {4} {5} {6}",
        index_, isStateful_, partitionCount_, targetReplicaSetSize_, blockList_, startFailoverUnitIndex_, metrics_);
}

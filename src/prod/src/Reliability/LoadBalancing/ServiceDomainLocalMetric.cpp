// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ServiceDomainLocalMetric.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

ServiceDomainLocalMetric::ServiceDomainLocalMetric()
    : nodeLoads_()
{
}

ServiceDomainLocalMetric::ServiceDomainLocalMetric(ServiceDomainLocalMetric && other)
    : nodeLoads_(move(other.nodeLoads_))
{
}

ServiceDomainLocalMetric & ServiceDomainLocalMetric::operator=(ServiceDomainLocalMetric && other)
{
    if (this != &other)
    {
        nodeLoads_ = move(other.nodeLoads_);
    }

    return *this;
}

void ServiceDomainLocalMetric::AddLoad(Federation::NodeId node, uint load)
{
    auto itNode = nodeLoads_.find(node);
    if (itNode == nodeLoads_.end())
    {
        itNode = nodeLoads_.insert(make_pair(node, 0u)).first;
    }

    itNode->second += load;
}

void ServiceDomainLocalMetric::DeleteLoad(Federation::NodeId node, uint load)
{
    auto itNode = nodeLoads_.find(node);
    if (itNode == nodeLoads_.end())
    {
        ASSERT_IF(load > 0, "Node {0} doesn't exist when deleting a positive load {1} from that node", node, load);
        return;
    }

    ASSERT_IF(itNode->second < load, "Existing load is smaller than load to be deleted");
    itNode->second -= load;

    if (itNode->second == 0)
    {
        nodeLoads_.erase(itNode);
    }
}

int64 ServiceDomainLocalMetric::GetLoad(Federation::NodeId node) const
{
    auto itNode = nodeLoads_.find(node);
    return itNode == nodeLoads_.end() ? 0LL : itNode->second;
}

void ServiceDomainLocalMetric::VerifyLoads(ServiceDomainLocalMetric const& baseLoads) const
{
    auto &rhsLoads = baseLoads.nodeLoads_;
    for (auto itNode = rhsLoads.begin(); itNode != rhsLoads.end(); ++itNode)
    {
        int64 lhsLoad = GetLoad(itNode->first);
        ASSERT_IF(lhsLoad != itNode->second, "Verifying ServiceDomainLocalMetric loads failed: {0} does not match {1} on node {2}", lhsLoad, itNode->second, itNode->first);
    }

    for (auto itNode = nodeLoads_.begin(); itNode != nodeLoads_.end(); ++itNode)
    {
        int64 rhsLoad = baseLoads.GetLoad(itNode->first);
        ASSERT_IF(itNode->second != rhsLoad, "Verifying ServiceDomainLocalMetric loads failed: {0} does not match {1} on node {2}", itNode->second, rhsLoad, itNode->first);
    }
}

void ServiceDomainLocalMetric::VerifyAreZero() const
{
    for (auto itNode = nodeLoads_.begin(); itNode != nodeLoads_.end(); ++itNode)
    {
        ASSERT_IF(itNode->second != 0, "Verifying ServiceDomainLocalMetric loads failed: load should be 0 on node {0}, {1} received", itNode->first, itNode->second);
    }
}

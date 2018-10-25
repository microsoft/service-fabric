// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("ClusterHealthQueryDescription");

ClusterHealthQueryDescription::ClusterHealthQueryDescription()
    : healthPolicy_()
    , eventsFilter_()
    , nodesFilter_()
    , applicationsFilter_()
    , applicationHealthPolicies_()
    , healthStatsFilter_()
{
}

ClusterHealthQueryDescription::~ClusterHealthQueryDescription()
{
}

ErrorCode ClusterHealthQueryDescription::FromPublicApi(FABRIC_CLUSTER_HEALTH_QUERY_DESCRIPTION const & publicClusterHealthQueryDescription)
{
    // HealthPolicy
    if (publicClusterHealthQueryDescription.HealthPolicy != NULL)
    {
        ClusterHealthPolicy clusterPolicy;
        auto error = clusterPolicy.FromPublicApi(*publicClusterHealthQueryDescription.HealthPolicy);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "HealthPolicy::FromPublicApi error: {0}", error);
            return error;
        }

        healthPolicy_ = make_unique<ClusterHealthPolicy>(move(clusterPolicy));
    }

    // EventsFilter
    if (publicClusterHealthQueryDescription.EventsFilter != NULL)
    {
        HealthEventsFilter eventsFilter;
        auto error = eventsFilter.FromPublicApi(*publicClusterHealthQueryDescription.EventsFilter);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "EventsFilter::FromPublicApi error: {0}", error);
            return error;
        }

        eventsFilter_ = make_unique<HealthEventsFilter>(move(eventsFilter));
    }

    // NodesFilter
    if (publicClusterHealthQueryDescription.NodesFilter != NULL)
    {
        NodeHealthStatesFilter nodesFilter;
        auto error = nodesFilter.FromPublicApi(*publicClusterHealthQueryDescription.NodesFilter);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "NodesFilter::FromPublicApi error: {0}", error);
            return error;
        }

        nodesFilter_ = make_unique<NodeHealthStatesFilter>(move(nodesFilter));
    }

    // ApplicationsFilter
    if (publicClusterHealthQueryDescription.ApplicationsFilter != NULL)
    {
        ApplicationHealthStatesFilter applicationsFilter;
        auto error = applicationsFilter.FromPublicApi(*publicClusterHealthQueryDescription.ApplicationsFilter);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "ApplicationsFilter::FromPublicApi error: {0}", error);
            return error;
        }

        applicationsFilter_ = make_unique<ApplicationHealthStatesFilter>(move(applicationsFilter));
    }

    // ApplicationHealthPolicyMap
    if (publicClusterHealthQueryDescription.ApplicationHealthPolicyMap != NULL)
    {
        auto error = applicationHealthPolicies_.FromPublicApi(*publicClusterHealthQueryDescription.ApplicationHealthPolicyMap);
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    if (publicClusterHealthQueryDescription.Reserved != nullptr)
    {
        auto ex1 = static_cast<FABRIC_CLUSTER_HEALTH_QUERY_DESCRIPTION_EX1*>(publicClusterHealthQueryDescription.Reserved);

        // ClusterHealthStatisticsFilter
        if (ex1->HealthStatisticsFilter != nullptr)
        {
            healthStatsFilter_ = make_unique<ClusterHealthStatisticsFilter>();
            auto error = healthStatsFilter_->FromPublicApi(*ex1->HealthStatisticsFilter);
            if (!error.IsSuccess())
            {
                Trace.WriteInfo(TraceSource, "HealthStatisticsFilter::FromPublicApi error: {0}", error);
                return error;
            }
        }
    }

    return ErrorCode::Success();
}

void ClusterHealthQueryDescription::SetQueryArguments(__in QueryArgumentMap & argMap) const
{
    if (healthPolicy_)
    {
        argMap.Insert(
            Query::QueryResourceProperties::Health::ClusterHealthPolicy, 
            healthPolicy_->ToString());
    }

    if (eventsFilter_)
    {
        argMap.Insert(
            Query::QueryResourceProperties::Health::EventsFilter,
            eventsFilter_->ToString());
    }

    if (nodesFilter_)
    {
        argMap.Insert(
            Query::QueryResourceProperties::Health::NodesFilter,
            nodesFilter_->ToString());
    }

    if (applicationsFilter_)
    {
        argMap.Insert(
            Query::QueryResourceProperties::Health::ApplicationsFilter,
            applicationsFilter_->ToString());
    }

    if (!applicationHealthPolicies_.empty())
    {
        argMap.Insert(
            Query::QueryResourceProperties::Health::ApplicationHealthPolicyMap,
            applicationHealthPolicies_.ToString());
    }

    if (healthStatsFilter_)
    {
        argMap.Insert(
            Query::QueryResourceProperties::Health::HealthStatsFilter,
            healthStatsFilter_->ToString());
    }
}


// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("ServiceHealthQueryDescription");

ServiceHealthQueryDescription::ServiceHealthQueryDescription()
    : serviceName_()
    , healthPolicy_()
    , eventsFilter_()
    , partitionsFilter_()
    , healthStatsFilter_()
{
}

ServiceHealthQueryDescription::ServiceHealthQueryDescription(
    Common::NamingUri && serviceName,
    std::unique_ptr<ApplicationHealthPolicy> && healthPolicy)
    : serviceName_(move(serviceName))
    , healthPolicy_(move(healthPolicy))
    , eventsFilter_()
    , partitionsFilter_()
    , healthStatsFilter_()
{
}

ServiceHealthQueryDescription::~ServiceHealthQueryDescription()
{
}

ErrorCode ServiceHealthQueryDescription::FromPublicApi(FABRIC_SERVICE_HEALTH_QUERY_DESCRIPTION const & publicServiceHealthQueryDescription)
{
    // ServiceName
    {
        auto error = NamingUri::TryParse(
            publicServiceHealthQueryDescription.ServiceName,
            "ServiceName",
            serviceName_);
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    // HealthPolicy
    if (publicServiceHealthQueryDescription.HealthPolicy != NULL)
    {
        ApplicationHealthPolicy appPolicy;
        auto error = appPolicy.FromPublicApi(*publicServiceHealthQueryDescription.HealthPolicy);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "HealthPolicy::FromPublicApi error: {0}", error);
            return error;
        }

        healthPolicy_ = make_unique<ApplicationHealthPolicy>(move(appPolicy));
    }

    // EventsFilter
    if (publicServiceHealthQueryDescription.EventsFilter != NULL)
    {
        HealthEventsFilter eventsFilter;
        auto error = eventsFilter.FromPublicApi(*publicServiceHealthQueryDescription.EventsFilter);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "EventsFilter::FromPublicApi error: {0}", error);
            return error;
        }

        eventsFilter_ = make_unique<HealthEventsFilter>(move(eventsFilter));
    }

    // PartitionsFilter
    if (publicServiceHealthQueryDescription.PartitionsFilter != NULL)
    {
        PartitionHealthStatesFilter partitionsFilter;
        auto error = partitionsFilter.FromPublicApi(*publicServiceHealthQueryDescription.PartitionsFilter);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "PartitionsFilter::FromPublicApi error: {0}", error);
            return error;
        }

        partitionsFilter_ = make_unique<PartitionHealthStatesFilter>(move(partitionsFilter));
    }

    if (publicServiceHealthQueryDescription.Reserved != nullptr)
    {
        auto ex1 = static_cast<FABRIC_SERVICE_HEALTH_QUERY_DESCRIPTION_EX1*>(publicServiceHealthQueryDescription.Reserved);

        // ClusterHealthStatisticsFilter
        if (ex1->HealthStatisticsFilter != nullptr)
        {
            healthStatsFilter_ = make_unique<ServiceHealthStatisticsFilter>();
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

void ServiceHealthQueryDescription::SetQueryArguments(__in QueryArgumentMap & argMap) const
{
    argMap.Insert(Query::QueryResourceProperties::Service::ServiceName, serviceName_.Name);

    if (healthPolicy_)
    {
        argMap.Insert(
            Query::QueryResourceProperties::Health::ApplicationHealthPolicy,
            healthPolicy_->ToString());
    }

    if (eventsFilter_)
    {
        argMap.Insert(
            Query::QueryResourceProperties::Health::EventsFilter,
            eventsFilter_->ToString());
    }

    if (partitionsFilter_)
    {
        argMap.Insert(
            Query::QueryResourceProperties::Health::PartitionsFilter,
            partitionsFilter_->ToString());
    }

    if (healthStatsFilter_)
    {
        argMap.Insert(
            Query::QueryResourceProperties::Health::HealthStatsFilter,
            healthStatsFilter_->ToString());
    }
}

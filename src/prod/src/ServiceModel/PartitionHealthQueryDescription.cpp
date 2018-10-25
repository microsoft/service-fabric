// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("PartitionHealthQueryDescription");

PartitionHealthQueryDescription::PartitionHealthQueryDescription()
    : partitionId_(Guid::Empty())
    , healthPolicy_()
    , eventsFilter_()
    , replicasFilter_()
    , healthStatsFilter_()
{
}

PartitionHealthQueryDescription::PartitionHealthQueryDescription(
    Common::Guid partitionId,
    std::unique_ptr<ApplicationHealthPolicy> && healthPolicy)
    : partitionId_(partitionId)
    , healthPolicy_(move(healthPolicy))
    , eventsFilter_()
    , replicasFilter_()
    , healthStatsFilter_()
{
}

PartitionHealthQueryDescription::~PartitionHealthQueryDescription()
{
}

ErrorCode PartitionHealthQueryDescription::FromPublicApi(FABRIC_PARTITION_HEALTH_QUERY_DESCRIPTION const & publicPartitionHealthQueryDescription)
{
    partitionId_ = Guid(publicPartitionHealthQueryDescription.PartitionId);

    // HealthPolicy
    if (publicPartitionHealthQueryDescription.HealthPolicy != NULL)
    {
        ApplicationHealthPolicy appPolicy;
        auto error = appPolicy.FromPublicApi(*publicPartitionHealthQueryDescription.HealthPolicy);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "HealthPolicy::FromPublicApi error: {0}", error);
            return error;
        }

        healthPolicy_ = make_unique<ApplicationHealthPolicy>(move(appPolicy));
    }

    // EventsFilter
    if (publicPartitionHealthQueryDescription.EventsFilter != NULL)
    {
        HealthEventsFilter eventsFilter;
        auto error = eventsFilter.FromPublicApi(*publicPartitionHealthQueryDescription.EventsFilter);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "EventsFilter::FromPublicApi error: {0}", error);
            return error;
        }

        eventsFilter_ = make_unique<HealthEventsFilter>(move(eventsFilter));
    }

    // ReplicasFilter
    if (publicPartitionHealthQueryDescription.ReplicasFilter != NULL)
    {
        ReplicaHealthStatesFilter replicasFilter;
        auto error = replicasFilter.FromPublicApi(*publicPartitionHealthQueryDescription.ReplicasFilter);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "ReplicasFilter::FromPublicApi error: {0}", error);
            return error;
        }

        replicasFilter_ = make_unique<ReplicaHealthStatesFilter>(move(replicasFilter));
    }

    if (publicPartitionHealthQueryDescription.Reserved != nullptr)
    {
        auto ex1 = static_cast<FABRIC_PARTITION_HEALTH_QUERY_DESCRIPTION_EX1*>(publicPartitionHealthQueryDescription.Reserved);

        // PartitionHealthStatisticsFilter
        if (ex1->HealthStatisticsFilter != nullptr)
        {
            healthStatsFilter_ = make_unique<PartitionHealthStatisticsFilter>();
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

void PartitionHealthQueryDescription::SetQueryArguments(__in QueryArgumentMap & argMap) const
{
    argMap.Insert(Query::QueryResourceProperties::Partition::PartitionId, wformatString(partitionId_));

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

    if (replicasFilter_)
    {
        argMap.Insert(
            Query::QueryResourceProperties::Health::ReplicasFilter,
            replicasFilter_->ToString());
    }

    if (healthStatsFilter_)
    {
        argMap.Insert(
            Query::QueryResourceProperties::Health::HealthStatsFilter,
            healthStatsFilter_->ToString());
    }
}

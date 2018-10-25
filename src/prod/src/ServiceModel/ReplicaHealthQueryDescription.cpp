// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("ReplicaHealthQueryDescription");

ReplicaHealthQueryDescription::ReplicaHealthQueryDescription()
    : partitionId_(Guid::Empty())
    , replicaOrInstanceId_(FABRIC_INVALID_REPLICA_ID)
    , healthPolicy_()
    , eventsFilter_()
{
}

ReplicaHealthQueryDescription::ReplicaHealthQueryDescription(
    Common::Guid partitionId,
    FABRIC_REPLICA_ID replicaOrInstanceId,
    std::unique_ptr<ApplicationHealthPolicy> && healthPolicy)
    : partitionId_(partitionId)
    , replicaOrInstanceId_(replicaOrInstanceId)
    , healthPolicy_(move(healthPolicy))
    , eventsFilter_()
{
}

ReplicaHealthQueryDescription::~ReplicaHealthQueryDescription()
{
}

ErrorCode ReplicaHealthQueryDescription::FromPublicApi(FABRIC_REPLICA_HEALTH_QUERY_DESCRIPTION const & publicReplicaHealthQueryDescription)
{
    partitionId_ = Guid(publicReplicaHealthQueryDescription.PartitionId);

    replicaOrInstanceId_ = publicReplicaHealthQueryDescription.ReplicaOrInstanceId;

    // HealthPolicy
    if (publicReplicaHealthQueryDescription.HealthPolicy != NULL)
    {
        ApplicationHealthPolicy appPolicy;
        auto error = appPolicy.FromPublicApi(*publicReplicaHealthQueryDescription.HealthPolicy);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "HealthPolicy::FromPublicApi error: {0}", error);
            return error;
        }

        healthPolicy_ = make_unique<ApplicationHealthPolicy>(move(appPolicy));
    }

    // EventsFilter
    if (publicReplicaHealthQueryDescription.EventsFilter != NULL)
    {
        HealthEventsFilter eventsFilter;
        auto error = eventsFilter.FromPublicApi(*publicReplicaHealthQueryDescription.EventsFilter);
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceSource, "EventsFilter::FromPublicApi error: {0}", error);
            return error;
        }

        eventsFilter_ = make_unique<HealthEventsFilter>(move(eventsFilter));
    }

    return ErrorCode::Success();
}

void ReplicaHealthQueryDescription::SetQueryArguments(__in QueryArgumentMap & argMap) const
{
    argMap.Insert(Query::QueryResourceProperties::Partition::PartitionId, wformatString(partitionId_));
    argMap.Insert(Query::QueryResourceProperties::Replica::ReplicaId, wformatString(replicaOrInstanceId_));

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
}

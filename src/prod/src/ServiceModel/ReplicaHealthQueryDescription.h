// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ReplicaHealthQueryDescription
    {
        DENY_COPY(ReplicaHealthQueryDescription)

    public:
        ReplicaHealthQueryDescription();

        ReplicaHealthQueryDescription(
            Common::Guid partitionId, 
            FABRIC_REPLICA_ID replicaOrInstanceId,
            std::unique_ptr<ApplicationHealthPolicy> && healthPolicy);

        ReplicaHealthQueryDescription(ReplicaHealthQueryDescription && other) = default;
        ReplicaHealthQueryDescription & operator = (ReplicaHealthQueryDescription && other) = default;

        ~ReplicaHealthQueryDescription();

        __declspec(property(get=get_PartitionId)) Common::Guid PartitionId;
        Common::Guid get_PartitionId() const { return partitionId_; }

        __declspec(property(get=get_ReplicaOrInstanceId)) FABRIC_REPLICA_ID ReplicaOrInstanceId;
        FABRIC_REPLICA_ID get_ReplicaOrInstanceId() const { return replicaOrInstanceId_; }

        __declspec(property(get=get_HealthPolicy)) std::unique_ptr<ApplicationHealthPolicy> const & HealthPolicy;
        std::unique_ptr<ApplicationHealthPolicy> const & get_HealthPolicy() const { return healthPolicy_; }

        __declspec(property(get=get_EventsFilter)) std::unique_ptr<HealthEventsFilter> const & EventsFilter;
        std::unique_ptr<HealthEventsFilter> const & get_EventsFilter() const { return eventsFilter_; }

        void SetEventsFilter(std::unique_ptr<HealthEventsFilter> && eventsFilter) { eventsFilter_ = move(eventsFilter); }

        Common::ErrorCode FromPublicApi(FABRIC_REPLICA_HEALTH_QUERY_DESCRIPTION const & publicReplicaHealthQueryDescription);
        
        void SetQueryArguments(__in QueryArgumentMap & argMap) const;

    private:
        Common::Guid partitionId_;
        FABRIC_REPLICA_ID replicaOrInstanceId_;
        std::unique_ptr<ApplicationHealthPolicy> healthPolicy_;
        std::unique_ptr<HealthEventsFilter> eventsFilter_;
    };
}


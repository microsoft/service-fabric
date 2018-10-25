// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class PartitionHealthQueryDescription
    {
        DENY_COPY(PartitionHealthQueryDescription)

    public:
        PartitionHealthQueryDescription();

        PartitionHealthQueryDescription(
            Common::Guid partitionId,
            std::unique_ptr<ApplicationHealthPolicy> && healthPolicy);

        PartitionHealthQueryDescription(PartitionHealthQueryDescription && other) = default;
        PartitionHealthQueryDescription & operator = (PartitionHealthQueryDescription && other) = default;

        ~PartitionHealthQueryDescription();

        __declspec(property(get=get_PartitionId)) Common::Guid PartitionId;
        Common::Guid get_PartitionId() const { return partitionId_; }

        __declspec(property(get=get_HealthPolicy)) std::unique_ptr<ApplicationHealthPolicy> const & HealthPolicy;
        std::unique_ptr<ApplicationHealthPolicy> const & get_HealthPolicy() const { return healthPolicy_; }

        __declspec(property(get=get_EventsFilter)) std::unique_ptr<HealthEventsFilter> const & EventsFilter;
        std::unique_ptr<HealthEventsFilter> const & get_EventsFilter() const { return eventsFilter_; }

        __declspec(property(get=get_ReplicasFilter)) std::unique_ptr<ReplicaHealthStatesFilter> const & ReplicasFilter;
        std::unique_ptr<ReplicaHealthStatesFilter> const & get_ReplicasFilter() const { return replicasFilter_; }

        __declspec(property(get = get_StatisticsFilter)) PartitionHealthStatisticsFilterUPtr const & StatisticsFilter;
        PartitionHealthStatisticsFilterUPtr const & get_StatisticsFilter() const { return healthStatsFilter_; }

        void SetEventsFilter(std::unique_ptr<HealthEventsFilter> && eventsFilter) { eventsFilter_ = move(eventsFilter); }
        void SetReplicasFilter(std::unique_ptr<ReplicaHealthStatesFilter> && replicasFilter) { replicasFilter_ = move(replicasFilter); }
        void SetStatisticsFilter(PartitionHealthStatisticsFilterUPtr && healthStatsFilter) { healthStatsFilter_ = std::move(healthStatsFilter); };

        Common::ErrorCode FromPublicApi(FABRIC_PARTITION_HEALTH_QUERY_DESCRIPTION const & publicPartitionHealthQueryDescription);

        void SetQueryArguments(__in QueryArgumentMap & argMap) const;

    private:
        Common::Guid partitionId_;
        std::unique_ptr<ApplicationHealthPolicy> healthPolicy_;
        std::unique_ptr<HealthEventsFilter> eventsFilter_;
        std::unique_ptr<ReplicaHealthStatesFilter> replicasFilter_;
        PartitionHealthStatisticsFilterUPtr healthStatsFilter_;
    };
}


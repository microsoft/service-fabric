// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ServiceHealthQueryDescription
    {
        DENY_COPY(ServiceHealthQueryDescription)

    public:
        ServiceHealthQueryDescription();

        ServiceHealthQueryDescription(
            Common::NamingUri && serviceName,
            std::unique_ptr<ApplicationHealthPolicy> && healthPolicy);

        ServiceHealthQueryDescription(ServiceHealthQueryDescription && other) = default;
        ServiceHealthQueryDescription & operator = (ServiceHealthQueryDescription && other) = default;

        ~ServiceHealthQueryDescription();

        __declspec(property(get=get_ServiceName)) Common::NamingUri const & ServiceName;
        Common::NamingUri const & get_ServiceName() const { return serviceName_; }

        __declspec(property(get=get_HealthPolicy)) std::unique_ptr<ApplicationHealthPolicy> const & HealthPolicy;
        std::unique_ptr<ApplicationHealthPolicy> const & get_HealthPolicy() const { return healthPolicy_; }

        __declspec(property(get=get_EventsFilter)) std::unique_ptr<HealthEventsFilter> const & EventsFilter;
        std::unique_ptr<HealthEventsFilter> const & get_EventsFilter() const { return eventsFilter_; }

        __declspec(property(get=get_PartitionsFilter)) std::unique_ptr<PartitionHealthStatesFilter> const & PartitionsFilter;
        std::unique_ptr<PartitionHealthStatesFilter> const & get_PartitionsFilter() const { return partitionsFilter_; }

        __declspec(property(get = get_StatisticsFilter)) ServiceHealthStatisticsFilterUPtr const & StatisticsFilter;
        ServiceHealthStatisticsFilterUPtr const & get_StatisticsFilter() const { return healthStatsFilter_; }

        void SetEventsFilter(std::unique_ptr<HealthEventsFilter> && eventsFilter) { eventsFilter_ = move(eventsFilter); }
        void SetPartitionsFilter(std::unique_ptr<PartitionHealthStatesFilter> && partitionsFilter) { partitionsFilter_ = move(partitionsFilter); }
        void SetStatisticsFilter(ServiceHealthStatisticsFilterUPtr && healthStatsFilter) { healthStatsFilter_ = std::move(healthStatsFilter); };

        Common::ErrorCode FromPublicApi(FABRIC_SERVICE_HEALTH_QUERY_DESCRIPTION const & publicServiceHealthQueryDescription);
        
        void SetQueryArguments(__in QueryArgumentMap & argMap) const;

    private:
        Common::NamingUri serviceName_;
        std::unique_ptr<ApplicationHealthPolicy> healthPolicy_;
        std::unique_ptr<HealthEventsFilter> eventsFilter_;
        std::unique_ptr<PartitionHealthStatesFilter> partitionsFilter_;
        ServiceHealthStatisticsFilterUPtr healthStatsFilter_;
    };
}


// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ApplicationHealthQueryDescription
    {
        DENY_COPY(ApplicationHealthQueryDescription)

    public:
        ApplicationHealthQueryDescription();

        ApplicationHealthQueryDescription(
            Common::NamingUri && applicationName,
            std::unique_ptr<ApplicationHealthPolicy> && healthPolicy);

        ApplicationHealthQueryDescription(ApplicationHealthQueryDescription && other) = default;
        ApplicationHealthQueryDescription & operator = (ApplicationHealthQueryDescription && other) = default;

        ~ApplicationHealthQueryDescription();

        __declspec(property(get=get_ApplicationName)) Common::NamingUri const & ApplicationName;
        Common::NamingUri const & get_ApplicationName() const { return applicationName_; }

        __declspec(property(get=get_HealthPolicy)) std::unique_ptr<ApplicationHealthPolicy> const & HealthPolicy;
        std::unique_ptr<ApplicationHealthPolicy> const & get_HealthPolicy() const { return healthPolicy_; }

        __declspec(property(get=get_EventsFilter)) std::unique_ptr<HealthEventsFilter> const & EventsFilter;
        std::unique_ptr<HealthEventsFilter> const & get_EventsFilter() const { return eventsFilter_; }
        
        __declspec(property(get=get_ServicesFilter)) std::unique_ptr<ServiceHealthStatesFilter> const & ServicesFilter;
        std::unique_ptr<ServiceHealthStatesFilter> const & get_ServicesFilter() const { return servicesFilter_; }

        __declspec(property(get=get_DeployedApplicationsFilter)) std::unique_ptr<DeployedApplicationHealthStatesFilter> const & DeployedApplicationsFilter;
        std::unique_ptr<DeployedApplicationHealthStatesFilter> const & get_DeployedApplicationsFilter() const { return deployedApplicationsFilter_; }

        __declspec(property(get = get_StatisticsFilter)) ApplicationHealthStatisticsFilterUPtr const & StatisticsFilter;
        ApplicationHealthStatisticsFilterUPtr const & get_StatisticsFilter() const { return healthStatsFilter_; }

        void SetEventsFilter(std::unique_ptr<HealthEventsFilter> && eventsFilter) { eventsFilter_ = move(eventsFilter); }
        void SetServicesFilter(std::unique_ptr<ServiceHealthStatesFilter> && servicesFilter) { servicesFilter_ = move(servicesFilter); }
        void SetDeployedApplicationsFilter(std::unique_ptr<DeployedApplicationHealthStatesFilter> && deployedApplicationsFilter) { deployedApplicationsFilter_ = move(deployedApplicationsFilter); }
        void SetStatisticsFilter(ApplicationHealthStatisticsFilterUPtr && healthStatsFilter) { healthStatsFilter_ = std::move(healthStatsFilter); };

        Common::ErrorCode FromPublicApi(FABRIC_APPLICATION_HEALTH_QUERY_DESCRIPTION const & publicApplicationHealthQueryDescription);
        
        void SetQueryArguments(__in QueryArgumentMap & argMap) const;

    private:
        Common::NamingUri applicationName_;
        std::unique_ptr<ApplicationHealthPolicy> healthPolicy_;
        std::unique_ptr<HealthEventsFilter> eventsFilter_;
        std::unique_ptr<ServiceHealthStatesFilter> servicesFilter_;
        std::unique_ptr<DeployedApplicationHealthStatesFilter> deployedApplicationsFilter_;
        ApplicationHealthStatisticsFilterUPtr healthStatsFilter_;
    };
}


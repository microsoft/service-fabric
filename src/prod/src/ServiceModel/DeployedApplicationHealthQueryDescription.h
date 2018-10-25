// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class DeployedApplicationHealthQueryDescription
    {
        DENY_COPY(DeployedApplicationHealthQueryDescription)

    public:
        DeployedApplicationHealthQueryDescription();

        DeployedApplicationHealthQueryDescription(
            Common::NamingUri && applicationName,
            std::wstring && nodeName,
            std::unique_ptr<ApplicationHealthPolicy> && healthPolicy);

        DeployedApplicationHealthQueryDescription(DeployedApplicationHealthQueryDescription && other) = default;
        DeployedApplicationHealthQueryDescription & operator = (DeployedApplicationHealthQueryDescription && other) = default;

        ~DeployedApplicationHealthQueryDescription();

        __declspec(property(get=get_ApplicationName)) Common::NamingUri const & ApplicationName;
        Common::NamingUri const & get_ApplicationName() const { return applicationName_; }

        __declspec(property(get=get_NodeName)) std::wstring const & NodeName;
        std::wstring const & get_NodeName() const { return nodeName_; }

        __declspec(property(get=get_HealthPolicy)) std::unique_ptr<ApplicationHealthPolicy> const & HealthPolicy;
        std::unique_ptr<ApplicationHealthPolicy> const & get_HealthPolicy() const { return healthPolicy_; }

        __declspec(property(get=get_EventsFilter)) std::unique_ptr<HealthEventsFilter> const & EventsFilter;
        std::unique_ptr<HealthEventsFilter> const & get_EventsFilter() const { return eventsFilter_; }

        __declspec(property(get=get_DeployedServicePackagesFilter)) std::unique_ptr<DeployedServicePackageHealthStatesFilter> const & DeployedServicePackagesFilter;
        std::unique_ptr<DeployedServicePackageHealthStatesFilter> const & get_DeployedServicePackagesFilter() const { return deployedServicePackagesFilter_; }

        __declspec(property(get = get_StatisticsFilter)) DeployedApplicationHealthStatisticsFilterUPtr const & StatisticsFilter;
        DeployedApplicationHealthStatisticsFilterUPtr const & get_StatisticsFilter() const { return healthStatsFilter_; }

        void SetEventsFilter(std::unique_ptr<HealthEventsFilter> && eventsFilter) { eventsFilter_ = move(eventsFilter); }
        void SetDeployedServicePackagesFilter(std::unique_ptr<DeployedServicePackageHealthStatesFilter> && deployedServicePackagesFilter) { deployedServicePackagesFilter_ = move(deployedServicePackagesFilter); }
        void SetStatisticsFilter(DeployedApplicationHealthStatisticsFilterUPtr && healthStatsFilter) { healthStatsFilter_ = std::move(healthStatsFilter); };

        Common::ErrorCode FromPublicApi(FABRIC_DEPLOYED_APPLICATION_HEALTH_QUERY_DESCRIPTION const & publicDeployedApplicationHealthQueryDescription);

        void SetQueryArguments(__in QueryArgumentMap & argMap) const;

    private:
        Common::NamingUri applicationName_;
        std::wstring nodeName_;
        std::unique_ptr<ApplicationHealthPolicy> healthPolicy_;
        std::unique_ptr<HealthEventsFilter> eventsFilter_;
        std::unique_ptr<DeployedServicePackageHealthStatesFilter> deployedServicePackagesFilter_;
        DeployedApplicationHealthStatisticsFilterUPtr healthStatsFilter_;
    };
}


// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ClusterHealthQueryDescription
    {
        DENY_COPY(ClusterHealthQueryDescription)

    public:
        ClusterHealthQueryDescription();
        
        ClusterHealthQueryDescription(ClusterHealthQueryDescription && other) = default;
        ClusterHealthQueryDescription & operator = (ClusterHealthQueryDescription && other) = default;

        ~ClusterHealthQueryDescription();

        __declspec(property(get=get_HealthPolicy)) std::unique_ptr<ClusterHealthPolicy> const & HealthPolicy;
        std::unique_ptr<ClusterHealthPolicy> const & get_HealthPolicy() const { return healthPolicy_; }

        __declspec(property(get=get_EventsFilter)) std::unique_ptr<HealthEventsFilter> const & EventsFilter;
        std::unique_ptr<HealthEventsFilter> const & get_EventsFilter() const { return eventsFilter_; }

        __declspec(property(get=get_NodesFilter)) std::unique_ptr<NodeHealthStatesFilter> const & NodesFilter;
        std::unique_ptr<NodeHealthStatesFilter> const & get_NodesFilter() const { return nodesFilter_; }

        __declspec(property(get = get_StatisticsFilter)) ClusterHealthStatisticsFilterUPtr const & StatisticsFilter;
        ClusterHealthStatisticsFilterUPtr const & get_StatisticsFilter() const { return healthStatsFilter_; }

        __declspec(property(get=get_ApplicationsFilter)) std::unique_ptr<ApplicationHealthStatesFilter> const & ApplicationsFilter;
        std::unique_ptr<ApplicationHealthStatesFilter> const & get_ApplicationsFilter() const { return applicationsFilter_; }
        
        void SetClusterHealthPolicy(std::unique_ptr<ClusterHealthPolicy> && healthPolicy) { healthPolicy_ = std::move(healthPolicy); }
        void SetApplicationHealthPolicies(std::map<std::wstring, ApplicationHealthPolicy> && applicationHealthPolicies) { applicationHealthPolicies_.SetMap(std::move(applicationHealthPolicies)); }
        void SetEventsFilter(std::unique_ptr<HealthEventsFilter> && eventsFilter) { eventsFilter_ = move(eventsFilter); }
        void SetNodesFilter(std::unique_ptr<NodeHealthStatesFilter> && nodesFilter) { nodesFilter_ = move(nodesFilter); }
        void SetApplicationsFilter(std::unique_ptr<ApplicationHealthStatesFilter> && applicationsFilter) { applicationsFilter_ = move(applicationsFilter); }
        void SetStatisticsFilter(ClusterHealthStatisticsFilterUPtr && healthStatsFilter) { healthStatsFilter_ = std::move(healthStatsFilter); };

        Common::ErrorCode FromPublicApi(FABRIC_CLUSTER_HEALTH_QUERY_DESCRIPTION const & publicClusterHealthQueryDescription);

        void SetQueryArguments(__in QueryArgumentMap & argMap) const;

    private:
        std::unique_ptr<ClusterHealthPolicy> healthPolicy_;
        std::unique_ptr<HealthEventsFilter> eventsFilter_;
        std::unique_ptr<NodeHealthStatesFilter> nodesFilter_;
        std::unique_ptr<ApplicationHealthStatesFilter> applicationsFilter_;
        ApplicationHealthPolicyMap applicationHealthPolicies_;
        ClusterHealthStatisticsFilterUPtr healthStatsFilter_;
    };
}


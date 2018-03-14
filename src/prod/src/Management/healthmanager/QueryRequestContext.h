// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        // Context that keeps track of the incoming queries that are pending completion.
        // It stores all the query filters as well as the query result.
        class QueryRequestContext
            : public Store::ReplicaActivityTraceComponent<Common::TraceTaskCodes::HealthManager>
        {
            DENY_COPY(QueryRequestContext);

        public:
            QueryRequestContext(
                Store::ReplicaActivityId const & replicaActivityId,
                __in ServiceModel::QueryArgumentMap & queryArgs);

            QueryRequestContext(QueryRequestContext && other);
            QueryRequestContext & operator = (QueryRequestContext && other);

            ~QueryRequestContext();

            __declspec(property(get=get_ContextId,put=set_ContextId)) std::wstring const & ContextId;
            std::wstring const & get_ContextId() const { return contextId_; }
            void set_ContextId(std::wstring && value) { contextId_ = move(value); }
            void set_ContextId(std::wstring const & value) { contextId_ = value; }

            __declspec(property(get=get_ContextKind,put=set_ContextKind)) RequestContextKind::Enum ContextKind;
            RequestContextKind::Enum get_ContextKind() const;
            void set_ContextKind(RequestContextKind::Enum value) { contextKind_ = value; }

            __declspec(property(get=get_HealthPolicyConsiderWarningAsError)) bool HealthPolicyConsiderWarningAsError;
            bool get_HealthPolicyConsiderWarningAsError() const;

            __declspec(property(get = get_CanCacheResult)) bool CanCacheResult;
            bool get_CanCacheResult() const { return canCacheResult_; }

            __declspec(property(get=get_ApplicationPolicy,put=set_ApplicationPolicy)) std::shared_ptr<ServiceModel::ApplicationHealthPolicy> const & ApplicationPolicy;
            std::shared_ptr<ServiceModel::ApplicationHealthPolicy> const & get_ApplicationPolicy() const { return applicationHealthPolicy_; }
            void set_ApplicationPolicy(std::shared_ptr<ServiceModel::ApplicationHealthPolicy> && value) { applicationHealthPolicy_ = std::move(value); }

            __declspec(property(get=get_ClusterPolicy,put=set_ClusterPolicy)) ServiceModel::ClusterHealthPolicySPtr const & ClusterPolicy;
            ServiceModel::ClusterHealthPolicySPtr const & get_ClusterPolicy() const { return clusterHealthPolicy_; }
            void set_ClusterPolicy(ServiceModel::ClusterHealthPolicySPtr && value) { clusterHealthPolicy_ = std::move(value); }

            __declspec(property(get=get_ApplicationHealthPolicies, put=set_ApplicationHealthPolicies)) ServiceModel::ApplicationHealthPolicyMapSPtr const & ApplicationHealthPolicies;
            ServiceModel::ApplicationHealthPolicyMapSPtr const & get_ApplicationHealthPolicies() const { return applicationHealthPolicies_; }
            void set_ApplicationHealthPolicies(ServiceModel::ApplicationHealthPolicyMapSPtr && value) { applicationHealthPolicies_ = std::move(value); }

            __declspec(property(get=get_Filters,put=set_Filters)) AttributesMap const & Filters;
            AttributesMap const & get_Filters() const { return filters_; }
            void set_Filters(AttributesMap && value) { filters_ = value; }

            __declspec(property(get = get_ContinuationToken)) std::wstring const & ContinuationToken;
            std::wstring const & get_ContinuationToken() const { return continuationToken_; }

            __declspec(property(get = get_maxResults)) int64 MaxResults;
            int64 get_maxResults() const { return maxResults_; }

            __declspec(property(get=get_ChildrenKind,put=set_ChildrenKind)) HealthEntityKind::Enum ChildrenKind;
            HealthEntityKind::Enum get_ChildrenKind() const { return childrenKind_; }
            void set_ChildrenKind(HealthEntityKind::Enum value) { childrenKind_ = value; }

            Common::ErrorCode SetApplicationHealthPolicy(std::wstring const & healthPolicy);
            Common::ErrorCode SetClusterHealthPolicy(std::wstring const & healthPolicy);

            Common::ErrorCode SetApplicationHealthPolicy();
            Common::ErrorCode SetClusterHealthPolicy();
            Common::ErrorCode SetApplicationHealthPolicyMap();

            // For paging related set operations, return success if there is nothing to set.
            Common::ErrorCode SetContinuationToken();
            Common::ErrorCode SetPagingStatus();
            Common::ErrorCode SetMaxResults();
            Common::ErrorCode ClearContinuationToken();
            Common::ErrorCode ClearMaxResults();

            Common::ErrorCode GetEventsFilter(__inout std::unique_ptr<ServiceModel::HealthEventsFilter> & eventsFilter) const;
            Common::ErrorCode GetServicesFilter(__inout std::unique_ptr<ServiceModel::ServiceHealthStatesFilter> & servicesFilter) const;
            Common::ErrorCode GetDeployedApplicationsFilter(__inout std::unique_ptr<ServiceModel::DeployedApplicationHealthStatesFilter> & deployedApplicationsFilter) const;
            Common::ErrorCode GetPartitionsFilter(__inout std::unique_ptr<ServiceModel::PartitionHealthStatesFilter> & partitionsFilter) const;
            Common::ErrorCode GetNodesFilter(__inout std::unique_ptr<ServiceModel::NodeHealthStatesFilter> & nodesFilter) const;
            Common::ErrorCode GetApplicationsFilter(__inout std::unique_ptr<ServiceModel::ApplicationHealthStatesFilter> & applicationsFilter) const;
            Common::ErrorCode GetReplicasFilter(__inout std::unique_ptr<ServiceModel::ReplicaHealthStatesFilter> & replicasFilter) const;
            Common::ErrorCode GetDeployedServicePackagesFilter(__inout std::unique_ptr<ServiceModel::DeployedServicePackageHealthStatesFilter> & deployedServicePackagesFilter) const;

            Common::ErrorCode GetPartitionHealthStatsFilter(__inout ServiceModel::PartitionHealthStatisticsFilterUPtr & filter) const;
            Common::ErrorCode GetServiceHealthStatsFilter(__inout ServiceModel::ServiceHealthStatisticsFilterUPtr & filter) const;
            Common::ErrorCode GetApplicationHealthStatsFilter(__inout ServiceModel::ApplicationHealthStatisticsFilterUPtr & filter) const;
            Common::ErrorCode GetDeployedApplicationHealthStatsFilter(__inout ServiceModel::DeployedApplicationHealthStatisticsFilterUPtr & filter) const;
            Common::ErrorCode GetClusterHealthStatsFilter(__inout ServiceModel::ClusterHealthStatisticsFilterUPtr & filter) const;

            Common::ErrorCode GetNodesFilter(__inout ServiceModel::NodeHealthStateFilterListWrapper & nodesFilter) const;
            Common::ErrorCode GetApplicationsFilter(__inout ServiceModel::ApplicationHealthStateFilterListWrapper & applicationsFilter) const;

            ServiceModel::QueryResult MoveQueryResult() { return std::move(queryResult_); }

            void SetQueryResult(ServiceModel::QueryResult && queryResult) { std::swap(queryResult_, queryResult); }

            std::wstring GetFilterDescription() const;

            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
            void WriteToEtw(uint16 contextSequenceId) const;

            template<class TAggregatedChild>
            void AddQueryListResults(ServiceModel::ListPager<TAggregatedChild> && pager);

            void InitializeEmptyQueryResult();

            static size_t GetMaxAllowedSize();

        private:
            // Kept alive by ProcessInnerQueryRequestAsyncOperation
            ServiceModel::QueryArgumentMap & queryArgs_;

            // Identifies the type of the query
            RequestContextKind::Enum contextKind_;

            ServiceModel::ApplicationHealthPolicySPtr applicationHealthPolicy_;
            ServiceModel::ClusterHealthPolicySPtr clusterHealthPolicy_;
            ServiceModel::ApplicationHealthPolicyMapSPtr applicationHealthPolicies_;
            ServiceModel::QueryResult queryResult_;
            AttributesMap filters_;
            HealthEntityKind::Enum childrenKind_;
            std::wstring contextId_;
            std::wstring continuationToken_;
            int64 maxResults_;
            mutable bool canCacheResult_;
        };
    }
}

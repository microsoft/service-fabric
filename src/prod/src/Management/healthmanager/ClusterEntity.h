// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class ClusterEntity
            : public HealthEntity
            , public std::enable_shared_from_this<ClusterEntity>
        {
            DENY_COPY(ClusterEntity);

        public:
            ClusterEntity(
                AttributesStoreDataSPtr && attributes,
                __in HealthManagerReplica & healthManagerReplica,
                HealthEntityState::Enum entityState);

            virtual ~ClusterEntity();

            virtual HealthEntityKind::Enum get_EntityKind() const override { return HealthEntityKind::Cluster; }

            __declspec (property(get = get_EntityId)) ClusterHealthId const & EntityId;
            ClusterHealthId const & get_EntityId() const { return *Constants::StoreType_ClusterTypeString; }

            Common::ErrorCode GetClusterUpgradeSnapshot(
                Common::ActivityId const & activityId,
                __inout ClusterUpgradeStateSnapshot & snapshot);

            Common::ErrorCode AppendToClusterUpgradeSnapshot(
                Common::ActivityId const & activityId,
                std::vector<std::wstring> const & upgradeDomains,
                __inout ClusterUpgradeStateSnapshot & snapshot);

            // Called by CM during cluster upgrades
            Common::ErrorCode EvaluateHealth(
                Common::ActivityId const & activityId,
                std::vector<std::wstring> const & upgradeDomains,
                ServiceModel::ClusterHealthPolicySPtr const & policy,
                ServiceModel::ClusterUpgradeHealthPolicySPtr const & upgradePolicy,
                ServiceModel::ApplicationHealthPolicyMapSPtr const & applicationHealthPolicies,
                ClusterUpgradeStateSnapshot const & baseline,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __inout ServiceModel::HealthEvaluationList & unhealthyEvaluations,
                __inout std::vector<std::wstring> & applicationsWithoutAppType);

            Common::ErrorCode GetClusterHealthPolicy(
                __inout ServiceModel::ClusterHealthPolicySPtr & healthPolicy);

            // Called by CM through IHealthAggregator. Used to update the policies in preparation for upgrade.
            // The policies are used for cluster evaluation on queries and upgrade health checking during upgrade.
            Common::AsyncOperationSPtr BeginUpdateClusterHealthPolicy(
                Common::ActivityId const & activityId,
                ServiceModel::ClusterHealthPolicySPtr const & healthPolicy,
                ServiceModel::ClusterUpgradeHealthPolicySPtr const & upgradeHealthPolicy,
                ServiceModel::ApplicationHealthPolicyMapSPtr const & applicationHealthPolicies,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            Common::ErrorCode EndUpdateClusterHealthPolicy(
                Common::AsyncOperationSPtr const & operation);

            Common::ErrorCode Close() override;

            HEALTH_ENTITY_TEMPLATED_METHODS_DECLARATIONS( ClusterAttributesStoreData )

        protected:
            virtual Common::ErrorCode UpdateContextHealthPoliciesCallerHoldsLock(
                __inout QueryRequestContext & context) override;

            virtual Common::ErrorCode SetDetailQueryResult(
                __inout QueryRequestContext & context,
                FABRIC_HEALTH_STATE entityEventsState,
                std::vector<ServiceModel::HealthEvent> && queryEvents,
                ServiceModel::HealthEvaluationList && unhealthyEvaluations) override;

            virtual Common::ErrorCode SetChildrenAggregatedHealthQueryResult(
                __inout QueryRequestContext & context) override;

            bool CleanupChildren();

        private:
            class UpdateClusterHealthPolicyAsyncOperation;

            static bool CanIgnoreChildEvaluationError(Common::ErrorCode const & error);

            Common::ErrorCode GetClusterAndApplicationHealthPoliciesIfNotSet(
                __inout ServiceModel::ClusterHealthPolicySPtr & healthPolicy,
                __inout ServiceModel::ApplicationHealthPolicyMapSPtr & applicationHealthPolicies);

            Common::ErrorCode GetClusterUpgradeHealthPolicy(
                __inout ServiceModel::ClusterUpgradeHealthPolicySPtr & upgradeHealthPolicy);

            Common::ErrorCode GetNodesAggregatedHealthStates(
                __inout QueryRequestContext & context);

            Common::ErrorCode EvaluateNodesForClusterUpgrade(
                Common::ActivityId const & activityId,
                ServiceModel::ClusterHealthPolicy const & healthPolicy,
                ServiceModel::ClusterUpgradeHealthPolicySPtr const & upgradeHealthPolicy,
                std::vector<std::wstring> const & upgradeDomains,
                ClusterUpgradeStateSnapshot const & baseline,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __inout ServiceModel::HealthEvaluationList & unhealthyEvaluations);

            Common::ErrorCode EvaluateNodeChunks(
                Common::ActivityId const & activityId,
                ServiceModel::ClusterHealthPolicy const & healthPolicy,
                ServiceModel::NodeHealthStateFilterList const & nodeFilters,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __inout ServiceModel::NodeHealthStateChunkList & childrenHealthStates);

            Common::ErrorCode EvaluateNodes(
                Common::ActivityId const & activityId,
                ServiceModel::ClusterHealthPolicy const & healthPolicy,
                ServiceModel::NodeHealthStatesFilterUPtr const & nodesFilter,
                HealthStatisticsUPtr const & healthStats,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __inout ServiceModel::HealthEvaluationList & unhealthyEvaluations,
                __inout ServiceModel::NodeAggregatedHealthStateList & childrenHealthStates);

            Common::ErrorCode EvaluateApplicationsForClusterUpgrade(
                Common::ActivityId const & activityId,
                ServiceModel::ClusterHealthPolicy const & healthPolicy,
                ServiceModel::ApplicationHealthPolicyMap const & applicationHealthPolicies,
                std::vector<std::wstring> const & upgradeDomains,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __inout ServiceModel::HealthEvaluationList & unhealthyEvaluations,
                __inout std::vector<std::wstring> & applicationsWithoutAppType);

            Common::ErrorCode EvaluateApplicationChunks(
                Common::ActivityId const & activityId,
                ServiceModel::ClusterHealthPolicy const & healthPolicy,
                ServiceModel::ApplicationHealthPolicyMap const & applicationHealthPolicies,
                ServiceModel::ApplicationHealthStateFilterList const & applicationFilters,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __inout ServiceModel::ApplicationHealthStateChunkList & childrenHealthStates);

            Common::ErrorCode EvaluateApplications(
                Common::ActivityId const & activityId,
                ServiceModel::ClusterHealthPolicy const & healthPolicy,
                ServiceModel::ApplicationHealthPolicyMap const & applicationHealthPolicies,
                ServiceModel::ApplicationHealthStatesFilterUPtr const & applicationsFilter,
                HealthStatisticsUPtr const & healthStats,
                ServiceModel::ClusterHealthStatisticsFilterUPtr const & healthStatsFilter,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __inout ServiceModel::HealthEvaluationList & unhealthyEvaluations,
                __inout ServiceModel::ApplicationAggregatedHealthStateList & childrenHealthStates);

            Common::ErrorCode ComputeApplicationsHealth(
                Common::ActivityId const & activityId,
                ServiceModel::HealthEvaluationList && systemAppUnhealthyChildren,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __in GroupHealthStateCount & applicationsHealthCount,
                __in std::map<std::wstring, GroupHealthStateCount> & appsPerAppType,
                __inout ServiceModel::HealthEvaluationList & unhealthyEvaluations);

            Common::ErrorCode ComputeDetailQueryResult(
                __inout QueryRequestContext & context,
                FABRIC_HEALTH_STATE entityEventsState,
                std::vector<ServiceModel::HealthEvent> && queryEvents,
                ServiceModel::HealthEvaluationList && unhealthyEvaluations);

            Common::ErrorCode ComputeQueryEntityHealthStateChunkResult(
                __inout QueryRequestContext & context,
                FABRIC_HEALTH_STATE entityEventsState);

            Common::ErrorCode GetApplicationsAggregatedHealthStates(
                __inout QueryRequestContext & context);

            // Get application aggregated health state and unhealthy evaluations
            Common::ErrorCode GetApplicationUnhealthyEvaluations(
                QueryRequestContext & context);

            Common::ErrorCode GetDeployedApplicationsOnNodeAggregatedHealthStates(
                __inout QueryRequestContext & context);

            void HealthStatsTimerCallback();

            // Disables the caching of the partial cluster health.
            // Returns true if it was previously enabled, false otherwise.
            bool DisableCachedEntityStats();

            struct CachedClusterHealth;
            using CachedClusterHealthSPtr = std::shared_ptr<CachedClusterHealth>;
            Common::ErrorCode ComputeClusterHealthWithoutEvents(
                bool canCache,
                Common::ActivityId const & activityId,
                ServiceModel::ClusterHealthPolicy const & healthPolicy,
                ServiceModel::ApplicationHealthPolicyMap const & applicationHealthPolicies,
                ServiceModel::NodeHealthStatesFilterUPtr const & nodesFilter,
                ServiceModel::ApplicationHealthStatesFilterUPtr const & applicationsFilter,
                ServiceModel::ClusterHealthStatisticsFilterUPtr const & healthStatsFilter,
                __inout ServiceModel::NodeAggregatedHealthStateList & nodeHealthStates,
                __inout ServiceModel::ApplicationAggregatedHealthStateList & applicationHealthStates,
                __inout CachedClusterHealthSPtr & partialClusterHealth,
                __inout bool & cached,
                __inout Common::TimeSpan & elapsed);

        private:
            // Flag set when the result of previous query that respects caching restrictions is cached.
            // If set, queries that respect caching restrictions can return the result immediately.
            Common::atomic_bool useCachedStats_;

            // Timer that runs periodically to get the cluster statistics.
            Common::TimerSPtr statsTimer_;

            struct CachedClusterHealth
            {
            public:
                FABRIC_HEALTH_STATE AggregatedHealthState;
                // Health evaluations without description
                ServiceModel::HealthEvaluationList UnhealthyEvaluation;
                HealthStatisticsUPtr HealthStats;
            };
            CachedClusterHealthSPtr cachedClusterHealth_;

            MUTABLE_RWLOCK(ClusterEntityStats, statsLock_);
        };
    }
}

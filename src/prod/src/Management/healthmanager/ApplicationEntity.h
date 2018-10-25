// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class ApplicationEntity
            : public HealthEntity
            , public std::enable_shared_from_this<ApplicationEntity>
        {
            DENY_COPY(ApplicationEntity);

        public:
            ApplicationEntity(
                AttributesStoreDataSPtr && attributes,
                __in HealthManagerReplica & healthManagerReplica,
                HealthEntityState::Enum entityState);

            virtual ~ApplicationEntity();

            virtual HealthEntityKind::Enum get_EntityKind() const { return HealthEntityKind::Application; }

            __declspec (property(get = get_EntityId)) ApplicationHealthId const & EntityId;
            ApplicationHealthId const & get_EntityId() const { return entityId_; }

            __declspec (property(get = get_IsSystemApp)) bool IsSystemApp;
            bool get_IsSystemApp() const { return isSystemApp_; }

            bool IsAdHocApp() const { return this->EntityIdString.empty(); }

            void AddService(
                ServiceEntitySPtr const & service);
            std::set<ServiceEntitySPtr> GetServices();

            void AddDeployedApplication(
                DeployedApplicationEntitySPtr const & deployedApplication);
            std::set<DeployedApplicationEntitySPtr> GetDeployedApplications();

            Common::ErrorCode IsApplicationHealthy(
                Common::ActivityId const & activityId,
                ServiceModel::ApplicationHealthPolicy const *policy,
                std::vector<std::wstring> const & upgradeDomains,
                __inout bool & healthy,
                __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations);

            Common::ErrorCode EvaluateFilteredHealth(
                Common::ActivityId const & activityId,
                ServiceModel::ApplicationHealthPolicySPtr const & appHealthPolicy,
                std::vector<std::wstring> const & upgradeDomains,
                ServiceModel::ServiceHealthStateFilterList const & serviceFilters,
                ServiceModel::DeployedApplicationHealthStateFilterList const & deployedApplicationFilters,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __inout ServiceModel::ServiceHealthStateChunkList & services,
                __inout ServiceModel::DeployedApplicationHealthStateChunkList & deployedApplications);

            // When isDeployedChildrenPrecedent is one, the deployed application is evluated first such that if service unhealthy evaluations are triggered by Hosting reporting unhealthy events, we will see deployed view still. This is turned on for mesh application/code package because the deployed ones are more helpful.
            Common::ErrorCode EvaluateHealth(
                Common::ActivityId const & activityId,
                ServiceModel::ApplicationHealthPolicySPtr const & appHealthPolicy,
                std::vector<std::wstring> const & upgradeDomains,
                HealthStatisticsUPtr const & healthStats,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations,
                bool isDeployedChildrenPrecedent = false);

            Common::ErrorCode GetApplicationHealthPolicy(
                __inout std::shared_ptr<ServiceModel::ApplicationHealthPolicy> & appHealthPolicy);

            Common::ErrorCode GetApplicationTypeName(__inout std::wstring & appTypeName) const;
            bool NeedsApplicationTypeName() const;

            HEALTH_ENTITY_TEMPLATED_METHODS_DECLARATIONS( ApplicationAttributesStoreData )

        protected:

            virtual Common::ErrorCode UpdateContextHealthPoliciesCallerHoldsLock(
                __in QueryRequestContext & context);

            virtual Common::ErrorCode SetDetailQueryResult(
                __in QueryRequestContext & context,
                FABRIC_HEALTH_STATE entityEventsState,
                std::vector<ServiceModel::HealthEvent> && queryEvents,
                std::vector<ServiceModel::HealthEvaluation> && unhealthyEvaluations);

            virtual Common::ErrorCode SetChildrenAggregatedHealthQueryResult(
                __in QueryRequestContext & context);

            bool CleanupChildren();

            virtual bool ShouldBeDeletedDueToChildrenState();

        private:
            Common::ErrorCode GetServicesAggregatedHealthStates(
                __in QueryRequestContext & context);

            // Gets all deployed applications on all nodes for a given application
            Common::ErrorCode GetDeployedApplicationsAggregatedHealthStates(
                __in QueryRequestContext & context);

            void EvaluateServiceChunks(
                Common::ActivityId const & activityId,
                ServiceModel::ApplicationHealthPolicy const & healthPolicy,
                ServiceModel::ServiceHealthStateFilterList const & serviceFilters,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __inout ServiceModel::ServiceHealthStateChunkList & childrenHealthStates);

            void EvaluateServices(
                Common::ActivityId const & activityId,
                ServiceModel::ApplicationHealthPolicy const & healthPolicy,
                std::unique_ptr<ServiceModel::ServiceHealthStatesFilter> const & servicesFilter,
                HealthStatisticsUPtr const & healthStats,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations,
                __inout std::vector<ServiceModel::ServiceAggregatedHealthState> & childrenHealthStates);

            void EvaluateDeployedApplicationChunks(
                Common::ActivityId const & activityId,
                ServiceModel::ApplicationHealthPolicy const & healthPolicy,
                std::vector<std::wstring> const & upgradeDomains,
                ServiceModel::DeployedApplicationHealthStateFilterList const & deployedApplicationFilters,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __inout ServiceModel::DeployedApplicationHealthStateChunkList & childrenHealthStates);

            void EvaluateDeployedApplications(
                Common::ActivityId const & activityId,
                ServiceModel::ApplicationHealthPolicy const & healthPolicy,
                std::vector<std::wstring> const & upgradeDomains,
                std::unique_ptr<ServiceModel::DeployedApplicationHealthStatesFilter> const & deployedApplicationsFilter,
                HealthStatisticsUPtr const & healthStats,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations,
                __inout std::vector<ServiceModel::DeployedApplicationAggregatedHealthState> & deployedApplicationHealthStates);

        private:
            ApplicationHealthId entityId_;
            // Keep a vector of children.
            // Used to compute the aggregated health of the parent.
            HealthEntityChildren<ServiceEntity> services_;
            HealthEntityChildren<DeployedApplicationEntity> deployedApplications_;

            bool expectSystemReports_;

            // Cache system app flag to avoid comparing the app name to system app name
            bool isSystemApp_;
        };
    }
}

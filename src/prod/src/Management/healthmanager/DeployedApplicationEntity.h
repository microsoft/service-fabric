// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class DeployedApplicationEntity 
            : public HealthEntity
            , public std::enable_shared_from_this<DeployedApplicationEntity>
        {
            DENY_COPY(DeployedApplicationEntity);

        public:
            DeployedApplicationEntity(
                AttributesStoreDataSPtr && attributes,
                __in HealthManagerReplica & healthManagerReplica,
                HealthEntityState::Enum entityState);

            virtual ~DeployedApplicationEntity();

            virtual HealthEntityKind::Enum get_EntityKind() const { return HealthEntityKind::DeployedApplication; }

            __declspec (property(get = get_EntityId)) DeployedApplicationHealthId const & EntityId;
            DeployedApplicationHealthId const & get_EntityId() const { return entityId_; }

            void AddDeployedServicePackage(DeployedServicePackageEntitySPtr const & deployedServicePackage);
            std::set<DeployedServicePackageEntitySPtr> GetDeployedServicePackages();

            Common::ErrorCode EvaluateHealth(
                Common::ActivityId const & activityId,
                ServiceModel::ApplicationHealthPolicy const & healthPolicy,
                HealthStatisticsUPtr const & healthStats,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations);

            Common::ErrorCode EvaluateFilteredHealth(
                Common::ActivityId const & activityId,
                ServiceModel::ApplicationHealthPolicy const & healthPolicy,
                ServiceModel::DeployedServicePackageHealthStateFilterList const & deployedServicePackageFilters,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __inout ServiceModel::DeployedServicePackageHealthStateChunkList & deployedServicePackages);

            Common::ErrorCode GetCachedApplicationHealthPolicy(
                Common::ActivityId const & activityId,
                __inout std::shared_ptr<ServiceModel::ApplicationHealthPolicy> & appHealthPolicy);

            Common::ErrorCode GetUpgradeDomain(
                Common::ActivityId const & activityId,
                __inout std::wstring & upgradeDomain);

            HEALTH_ENTITY_TEMPLATED_METHODS_DECLARATIONS( DeployedApplicationAttributesStoreData )

        protected:
            virtual bool get_HasHierarchySystemReport() const;

            virtual Common::ErrorCode HasAttributeMatchCallerHoldsLock(
                std::wstring const & attributeName,
                std::wstring const & attributeValue,
                __out bool & hasMatch) const;

            virtual void UpdateParents(Common::ActivityId const & activityId) override;
            
            virtual bool DeleteDueToParentsCallerHoldsLock() const override;

            virtual void OnEntityReadyToAcceptRequests(Common::ActivityId const & activityId);

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

        private:
            Common::ErrorCode GetDeployedServicePackagesAggregatedHealthStates(
                __in QueryRequestContext & context);
            
            Common::ErrorCode EvaluateDeployedServicePackages(
                Common::ActivityId const & activityId,
                ServiceModel::ApplicationHealthPolicy const & healthPolicy,
                std::unique_ptr<ServiceModel::DeployedServicePackageHealthStatesFilter> const & filter,
                HealthStatisticsUPtr const & healthStats,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations,
                __inout std::vector<ServiceModel::DeployedServicePackageAggregatedHealthState> & childrenHealthStates);

            Common::ErrorCode EvaluateDeployedServicePackageChunks(
                Common::ActivityId const & activityId,
                ServiceModel::ApplicationHealthPolicy const & healthPolicy,
                ServiceModel::DeployedServicePackageHealthStateFilterList const & deployedServicePackageFilters,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __inout ServiceModel::DeployedServicePackageHealthStateChunkList & childrenHealthStates);

        private:
            DeployedApplicationHealthId entityId_;

            // Keep a vector of children.
            // Used to compute the aggregated health of the parent.
            HealthEntityChildren<DeployedServicePackageEntity> deployedServicePackages_;

            HealthEntityNodeParent parentNode_;
            HealthEntityParent parentApplication_;
        };
    }
}

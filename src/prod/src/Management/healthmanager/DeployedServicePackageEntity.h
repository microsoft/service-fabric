// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class DeployedServicePackageEntity 
            : public HealthEntity
            , public std::enable_shared_from_this<DeployedServicePackageEntity>
        {
            DENY_COPY(DeployedServicePackageEntity);

        public:
            DeployedServicePackageEntity(
                AttributesStoreDataSPtr && attributes,
                __in HealthManagerReplica & healthManagerReplica,
                HealthEntityState::Enum entityState);

            virtual ~DeployedServicePackageEntity();

            virtual HealthEntityKind::Enum get_EntityKind() const { return HealthEntityKind::DeployedServicePackage; }

            __declspec (property(get = get_EntityId)) DeployedServicePackageHealthId const & EntityId;
            DeployedServicePackageHealthId const & get_EntityId() const { return entityId_; }

            Common::ErrorCode EvaluateHealth(
                Common::ActivityId const & activityId,
                ServiceModel::ApplicationHealthPolicy const & healthPolicy,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations);

            HEALTH_ENTITY_TEMPLATED_METHODS_DECLARATIONS( DeployedServicePackageAttributesStoreData )

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

        private:
            DeployedServicePackageHealthId entityId_;
            HealthEntityParent parentDeployedApplication_;
            HealthEntityNodeParent parentNode_;
        };
    }
}

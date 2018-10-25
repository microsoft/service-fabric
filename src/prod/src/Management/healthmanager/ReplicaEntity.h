// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class ReplicaEntity
            : public HealthEntity
            , public std::enable_shared_from_this<ReplicaEntity>
        {
            DENY_COPY(ReplicaEntity);

        public:
            ReplicaEntity(
                AttributesStoreDataSPtr && attributes,
                __in HealthManagerReplica & healthManagerReplica,
                HealthEntityState::Enum entityState);

            ~ReplicaEntity();

            virtual HealthEntityKind::Enum get_EntityKind() const { return HealthEntityKind::Replica; }

            __declspec (property(get = get_EntityId)) ReplicaHealthId const & EntityId;
            ReplicaHealthId const & get_EntityId() const { return entityId_; }

            FABRIC_SERVICE_KIND GetKind() const { return (this->GetAttributesCopy()->UseInstance) ? FABRIC_SERVICE_KIND_STATEFUL : FABRIC_SERVICE_KIND_STATELESS; };

            Common::ErrorCode EvaluateHealth(
                Common::ActivityId const & activityId,
                ServiceModel::ApplicationHealthPolicy const & healthPolicy,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations);

            HEALTH_ENTITY_TEMPLATED_METHODS_DECLARATIONS( ReplicaAttributesStoreData )

        protected:
            bool get_HasHierarchySystemReport() const;

            virtual Common::ErrorCode HasAttributeMatchCallerHoldsLock(
                std::wstring const & attributeName,
                std::wstring const & attributeValue,
                __out bool & hasMatch) const;

            virtual void UpdateParents(Common::ActivityId const & activityId) override;

            virtual bool DeleteDueToParentsCallerHoldsLock() const override;

            void OnEntityReadyToAcceptRequests(Common::ActivityId const & activityId);

            virtual Common::ErrorCode UpdateContextHealthPoliciesCallerHoldsLock(
                __in QueryRequestContext & context);

            virtual Common::ErrorCode SetDetailQueryResult(
                __in QueryRequestContext & context,
                FABRIC_HEALTH_STATE entityEventsState,
                std::vector<ServiceModel::HealthEvent> && queryEvents,
                std::vector<ServiceModel::HealthEvaluation> && unhealthyEvaluations);

        private:
            HealthEntitySPtr GetParent();

            ReplicaHealthId entityId_;
            HealthEntityParent parentPartition_;
            HealthEntityNodeParent parentNode_;
        };
    }
}

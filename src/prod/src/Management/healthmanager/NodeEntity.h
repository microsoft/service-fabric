// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class NodeEntity 
            : public HealthEntity
            , public std::enable_shared_from_this<NodeEntity>
        {
            DENY_COPY(NodeEntity);

        public:
            NodeEntity(
                AttributesStoreDataSPtr && attributes,
                __in HealthManagerReplica & healthManagerReplica,
                HealthEntityState::Enum entityState);

            virtual ~NodeEntity();

            virtual HealthEntityKind::Enum get_EntityKind() const { return HealthEntityKind::Node; }

            __declspec (property(get = get_EntityId)) NodeHealthId const & EntityId;
            NodeHealthId const & get_EntityId() const { return entityId_; }

            Common::ErrorCode EvaluateHealth(
                Common::ActivityId const & activityId,
                ServiceModel::ClusterHealthPolicy const & healthPolicy,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations);

            HEALTH_ENTITY_TEMPLATED_METHODS_DECLARATIONS( NodeAttributesStoreData )

        protected:
            virtual Common::ErrorCode UpdateContextHealthPoliciesCallerHoldsLock(
                __in QueryRequestContext & context);

            virtual Common::ErrorCode SetDetailQueryResult(
                __in QueryRequestContext & context,
                FABRIC_HEALTH_STATE entityEventsState,
                std::vector<ServiceModel::HealthEvent> && queryEvents,
                std::vector<ServiceModel::HealthEvaluation> && unhealthyEvaluations);

        private:
            NodeHealthId entityId_;
        };
    }
}

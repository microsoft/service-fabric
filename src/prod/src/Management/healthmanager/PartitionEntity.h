// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class PartitionEntity 
            : public HealthEntity
            , public std::enable_shared_from_this<PartitionEntity>
        {
            DENY_COPY(PartitionEntity);

        public:
            PartitionEntity(
                AttributesStoreDataSPtr && attributes,
                __in HealthManagerReplica & healthManagerReplica,
                HealthEntityState::Enum entityState);

            virtual ~PartitionEntity();

            virtual HealthEntityKind::Enum get_EntityKind() const { return HealthEntityKind::Partition; }
            
            __declspec (property(get = get_EntityId)) PartitionHealthId const & EntityId;
            PartitionHealthId const & get_EntityId() const { return entityId_; }

            void AddReplica(ReplicaEntitySPtr const & replica);
            std::set<ReplicaEntitySPtr> GetReplicas();

            Common::ErrorCode EvaluateHealth(
                Common::ActivityId const & activityId,
                ServiceModel::ApplicationHealthPolicy const & healthPolicy,
                HealthStatisticsUPtr const & healthStats,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations);

            Common::ErrorCode EvaluateFilteredHealth(
                Common::ActivityId const & activityId,
                ServiceModel::ApplicationHealthPolicy const & healthPolicy,
                ServiceModel::ReplicaHealthStateFilterList const & replicaFilters,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __inout ServiceModel::ReplicaHealthStateChunkList & replicas);

            Common::ErrorCode GetCachedApplicationHealthPolicy(
                Common::ActivityId const & activityId,
                __inout std::shared_ptr<ServiceModel::ApplicationHealthPolicy> & appHealthPolicy);

            HEALTH_ENTITY_TEMPLATED_METHODS_DECLARATIONS( PartitionAttributesStoreData )

        protected:
            bool get_HasHierarchySystemReport() const override;

            virtual Common::ErrorCode HasAttributeMatchCallerHoldsLock(
                std::wstring const & attributeName,
                std::wstring const & attributeValue,
                __out bool & hasMatch) const override;

            virtual void UpdateParents(Common::ActivityId const & activityId) override;
            
            virtual void OnEntityReadyToAcceptRequests(Common::ActivityId const & activityId);

            virtual Common::ErrorCode UpdateContextHealthPoliciesCallerHoldsLock(
                __in QueryRequestContext & context) override;

            virtual Common::ErrorCode SetDetailQueryResult(
                __in QueryRequestContext & context,
                FABRIC_HEALTH_STATE entityEventsState,
                std::vector<ServiceModel::HealthEvent> && queryEvents,
                std::vector<ServiceModel::HealthEvaluation> && unhealthyEvaluations) override;

            virtual Common::ErrorCode SetEntityHealthState(
                __in QueryRequestContext & context,
                FABRIC_HEALTH_STATE entityEventsState) override;

            virtual Common::ErrorCode SetChildrenAggregatedHealthQueryResult(
                __in QueryRequestContext & context) override;
            
            // Called to create or update the parent when attributes are changed
            void CreateOrUpdateNonPersistentParents(Common::ActivityId const & activityId) override;
            
            bool CleanupChildren() override;

        private: 
            HealthEntitySPtr GetParent();

            Common::ErrorCode GetReplicasAggregatedHealthStates(
                __in QueryRequestContext & context);

            Common::ErrorCode GetServiceTypeName(
                Common::ActivityId const & activityId,
                __inout std::wstring & serviceTypeName);

            Common::ErrorCode EvaluateReplicas(
                Common::ActivityId const & activityId,
                std::wstring const & serviceTypeName, 
                ServiceModel::ApplicationHealthPolicy const & healthPolicy,
                std::unique_ptr<ServiceModel::ReplicaHealthStatesFilter> const & filter,
                HealthStatisticsUPtr const & healthStats,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations,
                __inout std::vector<ServiceModel::ReplicaAggregatedHealthState> & childrenHealthStates);

            Common::ErrorCode EvaluateReplicaChunks(
                Common::ActivityId const & activityId,
                std::wstring const & serviceTypeName,
                ServiceModel::ApplicationHealthPolicy const & healthPolicy,
                ServiceModel::ReplicaHealthStateFilterList const & replicasFilter,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __inout ServiceModel::ReplicaHealthStateChunkList & childrenHealthStates);

        private:
            PartitionHealthId entityId_;

            // Keep a vector or replicas that are children of the partition.
            // Used to compute the aggregated health of the parent.
            HealthEntityChildren<ReplicaEntity> replicas_;
            MUTABLE_RWLOCK(HM.PartitionChild, replicasLock_);

            // Because the parent service is not persisted and not reported on,
            // it must be created by the fist partition child when the required attributes are set
            Common::atomic_bool hasUpdatedParentService_;

            HealthEntityParent parentService_;
        };
    }
}

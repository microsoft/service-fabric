// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class ServiceEntity 
            : public HealthEntity
            , public std::enable_shared_from_this<ServiceEntity>
        {
            DENY_COPY(ServiceEntity);

        public:
            ServiceEntity(
                AttributesStoreDataSPtr && attributes,
                __in HealthManagerReplica & healthManagerReplica,
                HealthEntityState::Enum entityState);

            ~ServiceEntity();

            virtual HealthEntityKind::Enum get_EntityKind() const { return HealthEntityKind::Service; }

            __declspec (property(get = get_EntityId)) ServiceHealthId const & EntityId;
            ServiceHealthId const & get_EntityId() const { return entityId_; }

            void AddPartition(PartitionEntitySPtr const & partition);
            std::set<PartitionEntitySPtr> GetPartitions();

            Common::ErrorCode EvaluateHealth(
                Common::ActivityId const & activityId,
                ServiceModel::ApplicationHealthPolicy const & healthPolicy,
                HealthStatisticsUPtr const & healthStats,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations);

            Common::ErrorCode EvaluateFilteredHealth(
                Common::ActivityId const & activityId,
                ServiceModel::ApplicationHealthPolicy const & healthPolicy,
                ServiceModel::PartitionHealthStateFilterList const & partitionFilters,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __inout ServiceModel::PartitionHealthStateChunkList & partitions);

            Common::ErrorCode GetCachedApplicationHealthPolicy(
                Common::ActivityId const & activityId,
                __inout std::shared_ptr<ServiceModel::ApplicationHealthPolicy> & appHealthPolicy);
            
            HEALTH_ENTITY_TEMPLATED_METHODS_DECLARATIONS( ServiceAttributesStoreData )

        protected:
            virtual bool get_HasHierarchySystemReport() const;
            
            virtual Common::ErrorCode HasAttributeMatchCallerHoldsLock(
                std::wstring const & attributeName,
                std::wstring const & attributeValue,
                __out bool & hasMatch) const;

            virtual void UpdateParents(Common::ActivityId const & activityId) override;
            
            // Called to create or update the parent when attributes are changed
            void CreateOrUpdateNonPersistentParents(Common::ActivityId const & activityId);

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
            HealthEntitySPtr GetParent();

            Common::ErrorCode GetPartitionsAggregatedHealthStates(
                __in QueryRequestContext & context);

            Common::ErrorCode GetServiceTypeName(
                Common::ActivityId const & activityId,
                __inout std::wstring & serviceTypeName);

            Common::ErrorCode EvaluatePartitions(
                Common::ActivityId const & activityId,
                std::wstring const & serviceTypeName,
                ServiceModel::ApplicationHealthPolicy const & healthPolicy,
                std::unique_ptr<ServiceModel::PartitionHealthStatesFilter> const & partitionsFilter,
                HealthStatisticsUPtr const & healthStats,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations,
                __inout std::vector<ServiceModel::PartitionAggregatedHealthState> & childrenHealthStates);
            
            Common::ErrorCode EvaluatePartitionChunks(
                Common::ActivityId const & activityId,
                std::wstring const & serviceTypeName,
                ServiceModel::ApplicationHealthPolicy const & healthPolicy,
                ServiceModel::PartitionHealthStateFilterList const & partitionFilters,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState,
                __inout ServiceModel::PartitionHealthStateChunkList & childrenHealthStates);

        private:
            ServiceHealthId entityId_;

            // Keep a vector of children.
            // Used to compute the aggregated health of the parent.
            HealthEntityChildren<PartitionEntity> partitions_;

            // ServiceEntity is created by partition entity when the service name attribute is set
            // When the app name and service type attributes are set on the partition,
            // update the local attributes
            // and create the parent application entity
            Common::atomic_bool hasUpdatedParent_;

            HealthEntityParent parentApplication_;
        };
    }
}

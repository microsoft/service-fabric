// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class PartitionHealth
        : public EntityHealthBase
    {
        DENY_COPY(PartitionHealth)
    public:
        PartitionHealth();

        PartitionHealth(
            Common::Guid const & partitionId,
            std::vector<HealthEvent> && events,
            FABRIC_HEALTH_STATE aggregatedHealthState,
            std::vector<HealthEvaluation> && unhealthyEvaluations,
            std::vector<ReplicaAggregatedHealthState> && replicasAggregatedHealthStates,
            Management::HealthManager::HealthStatisticsUPtr && healthStatistics);
        
        PartitionHealth(PartitionHealth && other) = default;
        PartitionHealth & operator= (PartitionHealth && other) = default;

        virtual ~PartitionHealth();

        __declspec(property(get=get_PartitionId)) Common::Guid const& PartitionId;
        Common::Guid const& get_PartitionId() const { return partitionId_; }

        __declspec(property(get=get_ReplicasAggregatedHealthStates)) std::vector<ReplicaAggregatedHealthState> const & ReplicasAggregatedHealthStates;
        std::vector<ReplicaAggregatedHealthState> const & get_ReplicasAggregatedHealthStates() const { return replicasAggregatedHealthStates_; }

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_PARTITION_HEALTH & publicPartitionHealth) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_PARTITION_HEALTH const & publicPartitionHealth);
        
        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const override;
        void WriteToEtw(uint16 contextSequenceId) const override;
        
        FABRIC_FIELDS_06(partitionId_, events_, aggregatedHealthState_, replicasAggregatedHealthStates_, unhealthyEvaluations_, healthStats_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(Constants::PartitionId, partitionId_)
            SERIALIZABLE_PROPERTY(Constants::ReplicaAggregatedHealthState, replicasAggregatedHealthStates_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(replicasAggregatedHealthStates_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        Common::Guid partitionId_;
        std::vector<ReplicaAggregatedHealthState> replicasAggregatedHealthStates_;
    };    
}

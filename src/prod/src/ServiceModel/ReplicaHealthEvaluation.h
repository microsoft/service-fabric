// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ReplicaHealthEvaluation
        : public HealthEvaluationWithChildrenBase
    {
        DENY_COPY(ReplicaHealthEvaluation)

    public:
        ReplicaHealthEvaluation();

        ReplicaHealthEvaluation(
            Common::Guid const & partitionId,
            FABRIC_REPLICA_ID replicaId,
            FABRIC_HEALTH_STATE aggregatedHealthState,
            std::vector<HealthEvaluation> && evaluations);

        ReplicaHealthEvaluation(ReplicaHealthEvaluation && other) = default;
        ReplicaHealthEvaluation & operator = (ReplicaHealthEvaluation && other) = default;

        virtual ~ReplicaHealthEvaluation();

        __declspec(property(get=get_PartitionId)) Common::Guid const& PartitionId;
        Common::Guid const& get_PartitionId() const { return partitionId_; }

        __declspec(property(get=get_ReplicaOrInstanceId)) FABRIC_REPLICA_ID ReplicaOrInstanceId;
        FABRIC_REPLICA_ID get_ReplicaOrInstanceId() const { return replicaId_; }

        virtual void SetDescription() override;

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_HEALTH_EVALUATION & publicHealthEvaluation) const;
        
        Common::ErrorCode FromPublicApi(
            FABRIC_HEALTH_EVALUATION const & publicHealthEvaluation);

        FABRIC_FIELDS_06(kind_, description_, unhealthyEvaluations_, aggregatedHealthState_, partitionId_, replicaId_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(Constants::PartitionId, partitionId_)
            SERIALIZABLE_PROPERTY(Constants::ReplicaOrInstanceId, replicaId_)
            SERIALIZABLE_PROPERTY(Constants::UnhealthyEvaluations, unhealthyEvaluations_)
        END_JSON_SERIALIZABLE_PROPERTIES()     

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(unhealthyEvaluations_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(partitionId_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        Common::Guid partitionId_;
        FABRIC_REPLICA_ID replicaId_;
    };

    DEFINE_HEALTH_EVALUATION_ACTIVATOR( ReplicaHealthEvaluation, FABRIC_HEALTH_EVALUATION_KIND_REPLICA )
}

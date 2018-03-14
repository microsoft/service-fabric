// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ReplicaHealth
        : public EntityHealthBase
    {
    public:
        ReplicaHealth();

        ReplicaHealth(
            FABRIC_SERVICE_KIND kind,
            Common::Guid const & partitionId,
            FABRIC_REPLICA_ID replicaId,
            std::vector<HealthEvent> && events,
            FABRIC_HEALTH_STATE aggregatedHealthState,
            std::vector<HealthEvaluation> && unhealthyEvaluations);

        ReplicaHealth(ReplicaHealth && other) = default;
        ReplicaHealth & operator = (ReplicaHealth && other) = default;

        virtual ~ReplicaHealth();

        __declspec(property(get=get_Kind)) FABRIC_SERVICE_KIND  Kind;
        FABRIC_SERVICE_KIND get_Kind() const { return kind_; }

        __declspec(property(get=get_PartitionId)) Common::Guid const& PartitionId;
        Common::Guid const& get_PartitionId() const { return partitionId_; }

        __declspec(property(get=get_ReplicaId)) FABRIC_REPLICA_ID ReplicaId;
        FABRIC_REPLICA_ID get_ReplicaId() const { return replicaId_; }

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_REPLICA_HEALTH & publicReplicaHealth) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_REPLICA_HEALTH const & publicReplicaHealth);

        FABRIC_FIELDS_06(kind_, partitionId_, replicaId_, events_, aggregatedHealthState_, unhealthyEvaluations_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(Constants::ServiceKind, kind_)
            SERIALIZABLE_PROPERTY(Constants::PartitionId, partitionId_)
            SERIALIZABLE_PROPERTY_IF(Constants::ReplicaId, replicaId_, kind_ == FABRIC_SERVICE_KIND_STATEFUL)
            SERIALIZABLE_PROPERTY_IF(Constants::InstanceId, replicaId_, kind_ == FABRIC_SERVICE_KIND_STATELESS)
            SERIALIZABLE_PROPERTY(Constants::HealthEvents, events_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::AggregatedHealthState, aggregatedHealthState_)
            SERIALIZABLE_PROPERTY(Constants::UnhealthyEvaluations, unhealthyEvaluations_)
        END_JSON_SERIALIZABLE_PROPERTIES()
        
        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_ENUM_ESTIMATION_MEMBER(kind_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(partitionId_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        FABRIC_SERVICE_KIND kind_;
        Common::Guid partitionId_;
        FABRIC_REPLICA_ID replicaId_;
    };    
}

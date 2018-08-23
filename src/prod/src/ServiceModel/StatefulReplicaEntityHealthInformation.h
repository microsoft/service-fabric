// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class StatefulReplicaEntityHealthInformation
        : public EntityHealthInformation
    {
        DENY_COPY(StatefulReplicaEntityHealthInformation)

    public:
        StatefulReplicaEntityHealthInformation();

        StatefulReplicaEntityHealthInformation(
            Common::Guid const& partitionId,
            FABRIC_REPLICA_ID replicaId,
            FABRIC_INSTANCE_ID replicaInstanceId);

        StatefulReplicaEntityHealthInformation(StatefulReplicaEntityHealthInformation && other) = default;
        StatefulReplicaEntityHealthInformation & operator = (StatefulReplicaEntityHealthInformation && other) = default;
        
         __declspec(property(get=get_PartitionId)) Common::Guid const& PartitionId;
        Common::Guid const& get_PartitionId() const { return partitionId_; }

        __declspec(property(get=get_ReplicaId)) FABRIC_REPLICA_ID ReplicaId;
        FABRIC_REPLICA_ID get_ReplicaId() const { return replicaId_; }

        __declspec(property(get=get_ReplicaInstanceId)) FABRIC_INSTANCE_ID  ReplicaInstanceId;
        FABRIC_INSTANCE_ID get_ReplicaInstanceId() const { return replicaInstanceId_; }

        std::wstring const& get_EntityId() const;
        DEFINE_INSTANCE_FIELD(replicaInstanceId_); 

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __in FABRIC_HEALTH_INFORMATION * commonHealthInformation,
            __out FABRIC_HEALTH_REPORT & healthReport) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_HEALTH_REPORT const & healthReport,
            __inout HealthInformation & commonHealthInformation,
            __out AttributeList & attributes);

        FABRIC_FIELDS_04(kind_, partitionId_, replicaId_, replicaInstanceId_);

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(partitionId_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        Common::Guid partitionId_;
        FABRIC_REPLICA_ID replicaId_;
        FABRIC_INSTANCE_ID replicaInstanceId_;
    };

    DEFINE_HEALTH_ENTITY_ACTIVATOR(StatefulReplicaEntityHealthInformation, FABRIC_HEALTH_REPORT_KIND_STATEFUL_SERVICE_REPLICA)
}

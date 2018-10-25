// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class StatelessInstanceEntityHealthInformation
        : public EntityHealthInformation
    {
        DENY_COPY(StatelessInstanceEntityHealthInformation)

    public:
        StatelessInstanceEntityHealthInformation();
        
        StatelessInstanceEntityHealthInformation(
            Common::Guid const& partitionId,
            FABRIC_REPLICA_ID replicaId);

        StatelessInstanceEntityHealthInformation(StatelessInstanceEntityHealthInformation && other) = default;
        StatelessInstanceEntityHealthInformation & operator = (StatelessInstanceEntityHealthInformation && other) = default;
        
         __declspec(property(get=get_PartitionId)) Common::Guid const& PartitionId;
        Common::Guid const& get_PartitionId() const { return partitionId_; }

        __declspec(property(get=get_ReplicaId)) FABRIC_REPLICA_ID ReplicaId;
        FABRIC_REPLICA_ID get_ReplicaId() const { return replicaId_; }

        std::wstring const& get_EntityId() const;
        
        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __in FABRIC_HEALTH_INFORMATION * commonHealthInformation,
            __out FABRIC_HEALTH_REPORT & healthReport) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_HEALTH_REPORT const & healthReport,
            __inout HealthInformation & commonHealthInformation,
            __out AttributeList & attributes);

        FABRIC_FIELDS_03(kind_, partitionId_, replicaId_);

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(partitionId_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        Common::Guid partitionId_;
        FABRIC_REPLICA_ID replicaId_;
    };

    DEFINE_HEALTH_ENTITY_ACTIVATOR(StatelessInstanceEntityHealthInformation, FABRIC_HEALTH_REPORT_KIND_STATELESS_SERVICE_INSTANCE)
}

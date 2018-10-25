// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ReplicaHealthStateChunk
        : public Common::IFabricJsonSerializable
        , public Serialization::FabricSerializable
        , public Common::ISizeEstimator
    {
        DENY_COPY(ReplicaHealthStateChunk)

    public:
        ReplicaHealthStateChunk();

        ReplicaHealthStateChunk(
            FABRIC_REPLICA_ID replicaOrInstanceId,
            FABRIC_HEALTH_STATE healthState);

        ReplicaHealthStateChunk(ReplicaHealthStateChunk && other) = default;
        ReplicaHealthStateChunk & operator = (ReplicaHealthStateChunk && other) = default;

        virtual ~ReplicaHealthStateChunk();

        __declspec(property(get=get_ReplicaOrInstanceId)) FABRIC_REPLICA_ID ReplicaOrInstanceId;
        FABRIC_REPLICA_ID get_ReplicaOrInstanceId() const { return replicaOrInstanceId_; }

        __declspec(property(get=get_HealthState)) FABRIC_HEALTH_STATE HealthState;
        FABRIC_HEALTH_STATE get_HealthState() const { return healthState_; }

        Common::ErrorCode FromPublicApi(FABRIC_REPLICA_HEALTH_STATE_CHUNK const & publicReplicaHealthStateChunk);

        Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __inout FABRIC_REPLICA_HEALTH_STATE_CHUNK & publicReplicaHealthStateChunk) const;

        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(healthState_)
        END_DYNAMIC_SIZE_ESTIMATION()

        FABRIC_FIELDS_02(replicaOrInstanceId_, healthState_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ReplicaOrInstanceId, replicaOrInstanceId_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::HealthState, healthState_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        FABRIC_REPLICA_ID replicaOrInstanceId_;
        FABRIC_HEALTH_STATE healthState_;
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ReplicaHealthStateChunk);

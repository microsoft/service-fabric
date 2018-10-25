// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class PartitionHealthStateChunk
        : public Common::IFabricJsonSerializable
        , public Serialization::FabricSerializable
        , public Common::ISizeEstimator
    {
        DENY_COPY(PartitionHealthStateChunk)

    public:
        PartitionHealthStateChunk();

        PartitionHealthStateChunk(
            Common::Guid partitionId,
            FABRIC_HEALTH_STATE healthState,
            ReplicaHealthStateChunkList && replicaHealthStateChunks);

        PartitionHealthStateChunk(PartitionHealthStateChunk && other) = default;
        PartitionHealthStateChunk & operator = (PartitionHealthStateChunk && other) = default;

        virtual ~PartitionHealthStateChunk();

        __declspec(property(get=get_PartitionId)) Common::Guid PartitionId;
        Common::Guid get_PartitionId() const { return partitionId_; }

        __declspec(property(get=get_HealthState)) FABRIC_HEALTH_STATE HealthState;
        FABRIC_HEALTH_STATE get_HealthState() const { return healthState_; }

        __declspec(property(get=get_ReplicaHealthStateChunks)) ReplicaHealthStateChunkList const & ReplicaHealthStateChunks;
        ReplicaHealthStateChunkList const & get_ReplicaHealthStateChunks() const { return replicaHealthStateChunks_; }

        Common::ErrorCode FromPublicApi(FABRIC_PARTITION_HEALTH_STATE_CHUNK const & publicPartitionHealthStateChunk);

        Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __inout FABRIC_PARTITION_HEALTH_STATE_CHUNK & publicPartitionHealthStateChunk) const;

        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(partitionId_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(healthState_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(replicaHealthStateChunks_)
        END_DYNAMIC_SIZE_ESTIMATION()

        FABRIC_FIELDS_03(partitionId_, healthState_, replicaHealthStateChunks_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::PartitionId, partitionId_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::HealthState, healthState_)
            SERIALIZABLE_PROPERTY(Constants::ReplicaHealthStateChunks, replicaHealthStateChunks_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        Common::Guid partitionId_;
        FABRIC_HEALTH_STATE healthState_;
        ReplicaHealthStateChunkList replicaHealthStateChunks_;
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::PartitionHealthStateChunk);

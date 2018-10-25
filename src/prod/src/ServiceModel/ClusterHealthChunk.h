// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ClusterHealthChunk
        : public Common::IFabricJsonSerializable
        , public Serialization::FabricSerializable
        , public Common::ISizeEstimator
    {
        DENY_COPY(ClusterHealthChunk)

    public:
        ClusterHealthChunk();

        ClusterHealthChunk(
            FABRIC_HEALTH_STATE healthState,
            NodeHealthStateChunkList && nodeHealthStateChunks,
            ApplicationHealthStateChunkList && applicationHealthStateChunks);

        ClusterHealthChunk(ClusterHealthChunk && other) = default;
        ClusterHealthChunk & operator = (ClusterHealthChunk && other) = default;

        virtual ~ClusterHealthChunk();

        __declspec(property(get=get_HealthState)) FABRIC_HEALTH_STATE HealthState;
        FABRIC_HEALTH_STATE get_HealthState() const { return healthState_; }

        __declspec(property(get=get_NodeHealthStateChunks)) NodeHealthStateChunkList const & NodeHealthStateChunks;
        NodeHealthStateChunkList const & get_NodeHealthStateChunks() const { return nodeHealthStateChunks_; }

        __declspec(property(get=get_ApplicationHealthStateChunks)) ApplicationHealthStateChunkList const & ApplicationHealthStateChunks;
        ApplicationHealthStateChunkList const & get_ApplicationHealthStateChunks() const { return applicationHealthStateChunks_; }

        // Used by FabricTest
        Common::ErrorCode FromPublicApi(FABRIC_CLUSTER_HEALTH_CHUNK const & publicClusterStateChunk);

        Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __inout FABRIC_CLUSTER_HEALTH_CHUNK & publicClusterStateChunk) const;

        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(healthState_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(nodeHealthStateChunks_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationHealthStateChunks_)
        END_DYNAMIC_SIZE_ESTIMATION()

        FABRIC_FIELDS_03(healthState_, nodeHealthStateChunks_, applicationHealthStateChunks_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(Constants::HealthState, healthState_)
            SERIALIZABLE_PROPERTY(Constants::NodeHealthStateChunks, nodeHealthStateChunks_)
            SERIALIZABLE_PROPERTY(Constants::ApplicationHealthStateChunks, applicationHealthStateChunks_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        FABRIC_HEALTH_STATE healthState_;
        NodeHealthStateChunkList nodeHealthStateChunks_;
        ApplicationHealthStateChunkList applicationHealthStateChunks_;
    };
}


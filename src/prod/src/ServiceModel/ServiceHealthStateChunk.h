// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ServiceHealthStateChunk
        : public Common::IFabricJsonSerializable
        , public Serialization::FabricSerializable
        , public Common::ISizeEstimator
    {
        DENY_COPY(ServiceHealthStateChunk)

    public:
        ServiceHealthStateChunk();

        ServiceHealthStateChunk(
            std::wstring const & serviceName,
            FABRIC_HEALTH_STATE healthState,
            PartitionHealthStateChunkList && partitionHealthStateChunks);

        ServiceHealthStateChunk(ServiceHealthStateChunk && other) = default;
        ServiceHealthStateChunk & operator = (ServiceHealthStateChunk && other) = default;

        virtual ~ServiceHealthStateChunk();

        __declspec(property(get=get_ServiceName)) std::wstring const & ServiceName;
        std::wstring const & get_ServiceName() const { return serviceName_; }

        __declspec(property(get=get_HealthState)) FABRIC_HEALTH_STATE HealthState;
        FABRIC_HEALTH_STATE get_HealthState() const { return healthState_; }

        __declspec(property(get=get_PartitionHealthStateChunks)) PartitionHealthStateChunkList const & PartitionHealthStateChunks;
        PartitionHealthStateChunkList const & get_PartitionHealthStateChunks() const { return partitionHealthStateChunks_; }

        Common::ErrorCode FromPublicApi(FABRIC_SERVICE_HEALTH_STATE_CHUNK const & publicServiceHealthStateChunk);

        Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __inout FABRIC_SERVICE_HEALTH_STATE_CHUNK & publicServiceHealthStateChunk) const;

        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(serviceName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(healthState_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(partitionHealthStateChunks_)
        END_DYNAMIC_SIZE_ESTIMATION()

        FABRIC_FIELDS_03(serviceName_, healthState_, partitionHealthStateChunks_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ServiceName, serviceName_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::HealthState, healthState_)
            SERIALIZABLE_PROPERTY(Constants::PartitionHealthStateChunks, partitionHealthStateChunks_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring serviceName_;
        FABRIC_HEALTH_STATE healthState_;
        PartitionHealthStateChunkList partitionHealthStateChunks_;
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ServiceHealthStateChunk);

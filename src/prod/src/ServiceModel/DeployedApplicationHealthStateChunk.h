// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class DeployedApplicationHealthStateChunk
        : public Common::IFabricJsonSerializable
        , public Serialization::FabricSerializable
        , public Common::ISizeEstimator
    {
        DENY_COPY(DeployedApplicationHealthStateChunk)

    public:
        DeployedApplicationHealthStateChunk();

        DeployedApplicationHealthStateChunk(
            std::wstring const & nodeName,
            FABRIC_HEALTH_STATE healthState,
            DeployedServicePackageHealthStateChunkList && deployedServicePackageHealthStateChunks);

        DeployedApplicationHealthStateChunk(DeployedApplicationHealthStateChunk && other) = default;
        DeployedApplicationHealthStateChunk & operator = (DeployedApplicationHealthStateChunk && other) = default;

        virtual ~DeployedApplicationHealthStateChunk();

        __declspec(property(get=get_NodeName)) std::wstring const & NodeName;
        std::wstring const & get_NodeName() const { return nodeName_; }

        __declspec(property(get=get_HealthState)) FABRIC_HEALTH_STATE HealthState;
        FABRIC_HEALTH_STATE get_HealthState() const { return healthState_; }

        __declspec(property(get=get_DeployedServicePackageHealthStateChunks)) DeployedServicePackageHealthStateChunkList const & DeployedServicePackageHealthStateChunks;
        DeployedServicePackageHealthStateChunkList const & get_DeployedServicePackageHealthStateChunks() const { return deployedServicePackageHealthStateChunks_; }

        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;

        Common::ErrorCode FromPublicApi(FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_CHUNK const & publicDeployedApplicationHealthStateChunk);

        Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __inout FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_CHUNK & publicDeployedApplicationHealthStateChunk) const;

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(nodeName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(healthState_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(deployedServicePackageHealthStateChunks_)
        END_DYNAMIC_SIZE_ESTIMATION()

        FABRIC_FIELDS_03(nodeName_, healthState_, deployedServicePackageHealthStateChunks_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::NodeName, nodeName_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::HealthState, healthState_)
            SERIALIZABLE_PROPERTY(Constants::DeployedServicePackageHealthStateChunks, deployedServicePackageHealthStateChunks_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring nodeName_;
        FABRIC_HEALTH_STATE healthState_;
        DeployedServicePackageHealthStateChunkList deployedServicePackageHealthStateChunks_;
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::DeployedApplicationHealthStateChunk)

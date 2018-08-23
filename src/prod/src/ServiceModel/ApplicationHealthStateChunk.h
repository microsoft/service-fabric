// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ApplicationHealthStateChunk
        : public Common::IFabricJsonSerializable
        , public Serialization::FabricSerializable
        , public Common::ISizeEstimator
    {
        DENY_COPY(ApplicationHealthStateChunk)

    public:
        ApplicationHealthStateChunk();

        ApplicationHealthStateChunk(
            std::wstring const & applicationName,
            std::wstring const & applicationTypeName,
            FABRIC_HEALTH_STATE healthState,
            ServiceHealthStateChunkList && serviceHealthStateChunks,
            DeployedApplicationHealthStateChunkList && deployedApplicationHealthStateChunks);

        ApplicationHealthStateChunk(ApplicationHealthStateChunk && other) = default;
        ApplicationHealthStateChunk & operator = (ApplicationHealthStateChunk && other) = default;

        virtual ~ApplicationHealthStateChunk();

        __declspec(property(get=get_ApplicationName)) std::wstring const & ApplicationName;
        std::wstring const & get_ApplicationName() const { return applicationName_; }

        __declspec(property(get = get_ApplicationTypeName)) std::wstring const & ApplicationTypeName;
        std::wstring const & get_ApplicationTypeName() const { return applicationTypeName_; }

        __declspec(property(get=get_HealthState)) FABRIC_HEALTH_STATE HealthState;
        FABRIC_HEALTH_STATE get_HealthState() const { return healthState_; }

        __declspec(property(get=get_ServiceHealthStateChunks)) ServiceHealthStateChunkList const & ServiceHealthStateChunks;
        ServiceHealthStateChunkList const & get_ServiceHealthStateChunks() const { return serviceHealthStateChunks_; }

        __declspec(property(get=get_DeployedApplicationHealthStateChunks)) DeployedApplicationHealthStateChunkList const & DeployedApplicationHealthStateChunks;
        DeployedApplicationHealthStateChunkList const & get_DeployedApplicationHealthStateChunks() const { return deployedApplicationHealthStateChunks_; }

        Common::ErrorCode FromPublicApi(FABRIC_APPLICATION_HEALTH_STATE_CHUNK const & publicApplicationHealthStateChunk);

        Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __inout FABRIC_APPLICATION_HEALTH_STATE_CHUNK & publicApplicationHealthStateChunk) const;

        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationTypeName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(healthState_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(serviceHealthStateChunks_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(deployedApplicationHealthStateChunks_)
        END_DYNAMIC_SIZE_ESTIMATION()

        FABRIC_FIELDS_05(applicationName_, healthState_, serviceHealthStateChunks_, deployedApplicationHealthStateChunks_, applicationTypeName_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ApplicationName, applicationName_)
            SERIALIZABLE_PROPERTY(Constants::ApplicationTypeName, applicationTypeName_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::HealthState, healthState_)
            SERIALIZABLE_PROPERTY(Constants::ServiceHealthStateChunks, serviceHealthStateChunks_)
            SERIALIZABLE_PROPERTY(Constants::DeployedApplicationHealthStateChunks, deployedApplicationHealthStateChunks_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring applicationName_;
        std::wstring applicationTypeName_;
        FABRIC_HEALTH_STATE healthState_;
        ServiceHealthStateChunkList serviceHealthStateChunks_;
        DeployedApplicationHealthStateChunkList deployedApplicationHealthStateChunks_;
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ApplicationHealthStateChunk);

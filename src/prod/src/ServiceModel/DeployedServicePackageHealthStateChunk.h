// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class DeployedServicePackageHealthStateChunk
        : public Common::IFabricJsonSerializable
        , public Serialization::FabricSerializable
        , public Common::ISizeEstimator
    {
        DENY_COPY(DeployedServicePackageHealthStateChunk)

    public:
        DeployedServicePackageHealthStateChunk();

        DeployedServicePackageHealthStateChunk(
            std::wstring const & serviceManifestName,
            std::wstring const & servicePackageActivationId,
            FABRIC_HEALTH_STATE healthState);

        DeployedServicePackageHealthStateChunk(DeployedServicePackageHealthStateChunk && other) = default;
        DeployedServicePackageHealthStateChunk & operator = (DeployedServicePackageHealthStateChunk && other) = default;

        virtual ~DeployedServicePackageHealthStateChunk();

        __declspec(property(get=get_ServiceManifestName)) std::wstring const & ServiceManifestName;
        std::wstring const & get_ServiceManifestName() const { return serviceManifestName_; }

        __declspec(property(get = get_ServicePackageActivationId)) std::wstring const & ServicePackageActivationId;
        std::wstring const & get_ServicePackageActivationId() const { return servicePackageActivationId_; }

        __declspec(property(get=get_HealthState)) FABRIC_HEALTH_STATE HealthState;
        FABRIC_HEALTH_STATE get_HealthState() const { return healthState_; }

        Common::ErrorCode FromPublicApi(FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_CHUNK const & publicDeployedServicePackageHealthStateChunk);

        Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __inout FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_CHUNK & publicDeployedServicePackageHealthStateChunk) const;

        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(serviceManifestName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(healthState_)
			DYNAMIC_SIZE_ESTIMATION_MEMBER(servicePackageActivationId_)
        END_DYNAMIC_SIZE_ESTIMATION()

        FABRIC_FIELDS_03(serviceManifestName_, healthState_, servicePackageActivationId_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ServiceManifestName, serviceManifestName_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::HealthState, healthState_)
            SERIALIZABLE_PROPERTY(Constants::ServicePackageActivationId, servicePackageActivationId_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring serviceManifestName_;
        std::wstring servicePackageActivationId_;
        FABRIC_HEALTH_STATE healthState_;
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::DeployedServicePackageHealthStateChunk);


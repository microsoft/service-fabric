// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class DeployedServicePackageHealth
        : public EntityHealthBase
    {
        DENY_COPY(DeployedServicePackageHealth)
    public:
        DeployedServicePackageHealth();

        DeployedServicePackageHealth(
            std::wstring const & applicationName,
            std::wstring const & serviceManifestName,
            std::wstring const & servicePackageActivationId,
            std::wstring const & nodeName,
            std::vector<HealthEvent> && events,
            FABRIC_HEALTH_STATE aggregatedHealthState,
            std::vector<HealthEvaluation> && unhealthyEvaluations);

        DeployedServicePackageHealth(DeployedServicePackageHealth && other) = default;
        DeployedServicePackageHealth & operator = (DeployedServicePackageHealth && other) = default;

        virtual ~DeployedServicePackageHealth();

        __declspec(property(get=get_ApplicationName)) std::wstring const& ApplicationName;
        std::wstring const& get_ApplicationName() const { return applicationName_; }

        __declspec(property(get=get_ServiceManifestName)) std::wstring const& ServiceManifestName;
        std::wstring const& get_ServiceManifestName() const { return serviceManifestName_; }

        __declspec(property(get = get_ServicePackageActivationId)) std::wstring const & ServicePackageActivationId;
        std::wstring const & get_ServicePackageActivationId() const { return servicePackageActivationId_; }

        __declspec(property(get=get_NodeName)) std::wstring const& NodeName;
        std::wstring const& get_NodeName() const { return nodeName_; }
         
        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH & publicDeployedServicePackageHealth) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH const & publicDeployedServicePackageHealth);

        FABRIC_FIELDS_07(applicationName_, serviceManifestName_, nodeName_, events_, aggregatedHealthState_, unhealthyEvaluations_, servicePackageActivationId_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ApplicationName, applicationName_)
            SERIALIZABLE_PROPERTY(Constants::ServiceManifestName, serviceManifestName_)
            SERIALIZABLE_PROPERTY(Constants::NodeName, nodeName_)
            SERIALIZABLE_PROPERTY(Constants::ServicePackageActivationId, servicePackageActivationId_)
            SERIALIZABLE_PROPERTY_CHAIN()
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(nodeName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(serviceManifestName_)
			DYNAMIC_SIZE_ESTIMATION_MEMBER(servicePackageActivationId_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        std::wstring applicationName_;
        std::wstring serviceManifestName_;
        std::wstring servicePackageActivationId_;
        std::wstring nodeName_;
    };    
}

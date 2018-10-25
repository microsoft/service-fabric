// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class DeployedServicePackageEntityHealthInformation
        : public EntityHealthInformation
    {
        DENY_COPY(DeployedServicePackageEntityHealthInformation)

    public:
        DeployedServicePackageEntityHealthInformation();

        DeployedServicePackageEntityHealthInformation(
            std::wstring const & applicationName,
            std::wstring const & serviceManifestName,
            std::wstring const & servicePackageActivationId,
            Common::LargeInteger const & nodeId,
            std::wstring const & nodeName,
            FABRIC_INSTANCE_ID applicationInstanceId);

        DeployedServicePackageEntityHealthInformation(DeployedServicePackageEntityHealthInformation && other) = default;
        DeployedServicePackageEntityHealthInformation & operator = (DeployedServicePackageEntityHealthInformation && other) = default;
        
        __declspec(property(get=get_ApplicationName)) std::wstring const & ApplicationName;
        std::wstring const & get_ApplicationName() const { return applicationName_; }

        __declspec(property(get=get_ServiceManifestName)) std::wstring const & ServiceManifestName;
        std::wstring const & get_ServiceManifestName() const { return serviceManifestName_; }

        __declspec(property(get = get_ServicePackageActivationId)) std::wstring const & ServicePackageActivationId;
        std::wstring const & get_ServicePackageActivationId() const { return servicePackageActivationId_; }

        __declspec(property(get=get_NodeName)) std::wstring const & NodeName;
        std::wstring const & get_NodeName() const { return nodeName_; }

        __declspec(property(get=get_NodeId)) Common::LargeInteger const & NodeId;
        Common::LargeInteger const & get_NodeId() const { return nodeId_; }

        __declspec(property(get=get_ServicePackageInstanceId)) FABRIC_INSTANCE_ID ServicePackageInstanceId;
        FABRIC_INSTANCE_ID get_ServicePackageInstanceId() const { return servicePackageInstanceId_; }

        std::wstring const & get_EntityId() const;
        DEFINE_INSTANCE_FIELD(servicePackageInstanceId_)
        
        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __in FABRIC_HEALTH_INFORMATION * commonHealthInformation,
            __out FABRIC_HEALTH_REPORT & healthReport) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_HEALTH_REPORT const & healthReport,
            __inout HealthInformation & commonHealthInformation,
            __out AttributeList & attributes);

        Common::ErrorCode GenerateNodeId();

        FABRIC_FIELDS_07(kind_, applicationName_, serviceManifestName_, nodeId_, nodeName_, servicePackageInstanceId_, servicePackageActivationId_);

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_CHAIN()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(serviceManifestName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(nodeId_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(nodeName_)
			DYNAMIC_SIZE_ESTIMATION_MEMBER(servicePackageActivationId_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        std::wstring applicationName_;
        std::wstring serviceManifestName_;
        std::wstring servicePackageActivationId_;
        Common::LargeInteger nodeId_;
        std::wstring nodeName_;
        FABRIC_INSTANCE_ID servicePackageInstanceId_;
    };

    DEFINE_HEALTH_ENTITY_ACTIVATOR(DeployedServicePackageEntityHealthInformation, FABRIC_HEALTH_REPORT_KIND_DEPLOYED_SERVICE_PACKAGE)
}

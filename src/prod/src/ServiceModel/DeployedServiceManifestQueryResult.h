// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class DeployedServiceManifestQueryResult
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        DeployedServiceManifestQueryResult();
        DeployedServiceManifestQueryResult(
            std::wstring const & serviceManifestName,
            std::wstring const & servicePackageActivationId,
            std::wstring const & serviceManifestVersion,
            DeploymentStatus::Enum deployedServiceManifestStatus);

        DeployedServiceManifestQueryResult(DeployedServiceManifestQueryResult && other) = default;
        DeployedServiceManifestQueryResult(DeployedServiceManifestQueryResult const & other) = default;

        DeployedServiceManifestQueryResult & operator = (DeployedServiceManifestQueryResult const & other) = default;
        DeployedServiceManifestQueryResult & operator = (DeployedServiceManifestQueryResult && other) = default;
        
        __declspec(property(get=get_ServiceManifestName, put = put_ServiceManifestName)) std::wstring const & ServiceManifestName;
        std::wstring const& get_ServiceManifestName() const { return serviceManifestName_; }
        void put_ServiceManifestName(std::wstring const& serviceManifestName) { serviceManifestName_ = serviceManifestName; }

        __declspec(property(get = get_ServicePackageActivationId)) std::wstring const & ServicePackageActivationId;
        std::wstring const & get_ServicePackageActivationId() const { return servicePackageActivationId_; }

        __declspec(property(get=get_ServiceManifestVersion)) std::wstring const & ServiceManifestVersion;
        std::wstring const& get_ServiceManifestVersion() const { return serviceManifestVersion_; }

        __declspec(property(get=get_DeploymentStatus)) DeploymentStatus::Enum DeployedServicePackageStatus;
        DeploymentStatus::Enum get_DeploymentStatus() const { return deployedServiceManifestStatus_; }

        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_DEPLOYED_SERVICE_PACKAGE_QUERY_RESULT_ITEM & publicDeployedServiceManifestQueryResult) const ;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        Common::ErrorCode FromPublicApi(__in FABRIC_DEPLOYED_SERVICE_PACKAGE_QUERY_RESULT_ITEM const & publicDeployedServiceManifestQueryResult);

        FABRIC_FIELDS_04(
            serviceManifestName_, 
            serviceManifestVersion_, 
            deployedServiceManifestStatus_, 
            servicePackageActivationId_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::Name, serviceManifestName_)
            SERIALIZABLE_PROPERTY(Constants::Version, serviceManifestVersion_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::Status, deployedServiceManifestStatus_)
            SERIALIZABLE_PROPERTY(Constants::ServicePackageActivationId, servicePackageActivationId_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring serviceManifestName_;
        std::wstring servicePackageActivationId_;
        std::wstring serviceManifestVersion_;
        DeploymentStatus::Enum deployedServiceManifestStatus_;
    };
}

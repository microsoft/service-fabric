// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class DeployedServiceTypeQueryResult
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        DeployedServiceTypeQueryResult();
        DeployedServiceTypeQueryResult(
            std::wstring const & serviceTypeName,
            std::wstring const & codePackageName,
            std::wstring const & serviceManifestName,
            std::wstring const & servicePackageActivationId_,
            FABRIC_SERVICE_TYPE_REGISTRATION_STATUS registrationStatus);

        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_DEPLOYED_SERVICE_TYPE_QUERY_RESULT_ITEM & publicDeployedServiceTypeQueryResult) const ;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        Common::ErrorCode FromPublicApi(__in FABRIC_DEPLOYED_SERVICE_TYPE_QUERY_RESULT_ITEM const & publicDeployedServiceTypeQueryResult);

        __declspec(property(get = get_ServiceTypeName)) std::wstring const & ServiceTypeName;
        std::wstring const& get_ServiceTypeName() const { return serviceTypeName_; }

        __declspec(property(get = get_ServiceManifestName)) std::wstring const & ServiceManifestName;
        std::wstring const& get_ServiceManifestName() const { return serviceManifestName_; }

        __declspec(property(get = get_CodePackageName)) std::wstring const & CodePackageName;
        std::wstring const& get_CodePackageName() const { return codePackageName_; }

        __declspec(property(get = get_ServicePackageActivationId)) std::wstring const & servicePackageActivationId;
        std::wstring const & get_ServicePackageActivationId() const { return servicePackageActivationId_; }

        FABRIC_FIELDS_05(serviceTypeName_, codePackageName_, serviceManifestName_, registrationStatus_, servicePackageActivationId_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ServiceTypeName, serviceTypeName_)
            SERIALIZABLE_PROPERTY(Constants::CodePackageName, codePackageName_)
            SERIALIZABLE_PROPERTY(Constants::ServiceManifestName, serviceManifestName_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::Status, registrationStatus_)
            SERIALIZABLE_PROPERTY(Constants::ServicePackageActivationId, servicePackageActivationId_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring serviceTypeName_;
        std::wstring codePackageName_;
        std::wstring serviceManifestName_;
        std::wstring servicePackageActivationId_;
        FABRIC_SERVICE_TYPE_REGISTRATION_STATUS registrationStatus_;
    };

    
}

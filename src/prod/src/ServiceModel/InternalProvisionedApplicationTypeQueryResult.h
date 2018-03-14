// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class InternalProvisionedApplicationTypeQueryResult
        : public Serialization::FabricSerializable
    {
    public:
        InternalProvisionedApplicationTypeQueryResult();
        
        InternalProvisionedApplicationTypeQueryResult(
            std::wstring const & appTypeName, 
            std::vector<std::wstring> const & sericeManifests,
            std::vector<std::wstring> const & codePackages,
            std::vector<std::wstring> const & configPackages,
            std::vector<std::wstring> const & dataPackages);

        InternalProvisionedApplicationTypeQueryResult( InternalProvisionedApplicationTypeQueryResult && other);


        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        __declspec(property(get=get_ApplicationTypeName)) std::wstring const & ApplicationTypeName;
        std::wstring const& get_ApplicationTypeName() const { return appTypeName_; }

        __declspec(property(get=get_ServiceManifests)) std::vector<std::wstring> const &ServiceManifests;
        std::vector<std::wstring> const& get_ServiceManifests() const { return serviceManifests_; }

        __declspec(property(get=get_CodePackages)) std::vector<std::wstring> const & CodePackages;
        std::vector<std::wstring> const& get_CodePackages() const { return codePackages_; }

        __declspec(property(get=get_ConfigPackages)) std::vector<std::wstring> const & ConfigPackages;
        std::vector<std::wstring> const& get_ConfigPackages() const { return configPackages_; }

        __declspec(property(get=get_DataPackages)) std::vector<std::wstring> const & DataPackages;
        std::vector<std::wstring> const& get_DataPackages() const { return dataPackages_; }        

        InternalProvisionedApplicationTypeQueryResult const & operator= (InternalProvisionedApplicationTypeQueryResult && other);

        FABRIC_FIELDS_05(appTypeName_, serviceManifests_, codePackages_, configPackages_, dataPackages_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ApplicationTypeName, appTypeName_)
            SERIALIZABLE_PROPERTY(Constants::ServiceManifestName, serviceManifests_)
            SERIALIZABLE_PROPERTY(Constants::CodePackageIds, codePackages_)
            SERIALIZABLE_PROPERTY(Constants::ConfigPackageIds, configPackages_)
            SERIALIZABLE_PROPERTY(Constants::DataPackageIds, dataPackages_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring appTypeName_;
        std::vector<std::wstring> serviceManifests_;
        std::vector<std::wstring> codePackages_;
        std::vector<std::wstring> configPackages_;
        std::vector<std::wstring> dataPackages_;
    };
}

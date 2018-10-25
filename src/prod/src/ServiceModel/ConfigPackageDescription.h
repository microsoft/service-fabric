// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // ServiceManifest/ConfigPackage
    struct DigestedConfigPackageDescription;
    struct ServiceManifestDescription;
    struct ConfigPackageDescription : public Serialization::FabricSerializable
    {
    public:
        ConfigPackageDescription();
        ConfigPackageDescription(ConfigPackageDescription const & other);
        ConfigPackageDescription(ConfigPackageDescription && other);

        ConfigPackageDescription const & operator = (ConfigPackageDescription const & other);
        ConfigPackageDescription const & operator = (ConfigPackageDescription && other);

        bool operator == (ConfigPackageDescription const & other) const;
        bool operator != (ConfigPackageDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        Common::ErrorCode FromPublicApi(FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION const & publicDescription);
        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __in std::wstring const& serviceManifestName, 
            __in std::wstring const& serviceManifestVersion, 
            __out FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION & publicDescription) const;

        void clear();

        FABRIC_FIELDS_02(Name, Version);

    public:
        std::wstring Name;
        std::wstring Version;

    private:
        friend struct DigestedConfigPackageDescription;
        friend struct ServiceManifestDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ConfigPackageDescription);

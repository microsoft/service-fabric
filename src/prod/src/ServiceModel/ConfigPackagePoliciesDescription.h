// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ConfigPackagePoliciesDescription> element.
    struct ConfigPackagePoliciesDescription
    {
    public:
        ConfigPackagePoliciesDescription(); 

        ConfigPackagePoliciesDescription(ConfigPackagePoliciesDescription const & other) = default;
        ConfigPackagePoliciesDescription(ConfigPackagePoliciesDescription && other) = default;

        ConfigPackagePoliciesDescription & operator = (ConfigPackagePoliciesDescription const & other) = default;
        ConfigPackagePoliciesDescription & operator = (ConfigPackagePoliciesDescription && other) = default;

        bool operator == (ConfigPackagePoliciesDescription const & other) const;
        bool operator != (ConfigPackagePoliciesDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        std::vector<ConfigPackageSettingDescription> ConfigPackages;
        std::wstring CodePackageRef;

    private:
        friend struct DigestedCodePackageDescription;
        friend struct ServiceManifestImportDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}

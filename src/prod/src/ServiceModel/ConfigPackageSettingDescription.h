// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // ApplicationManifest/ConfigPackage
    struct ConfigPackageSettingDescription : public Serialization::FabricSerializable
    {
    public:
        ConfigPackageSettingDescription();
        ConfigPackageSettingDescription(ConfigPackageSettingDescription const & other) = default;
        ConfigPackageSettingDescription(ConfigPackageSettingDescription && other) = default;

        ConfigPackageSettingDescription & operator = (ConfigPackageSettingDescription const & other) = default;
        ConfigPackageSettingDescription & operator = (ConfigPackageSettingDescription && other) = default ;

        bool operator == (ConfigPackageSettingDescription const & other) const;
        bool operator != (ConfigPackageSettingDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

        FABRIC_FIELDS_04(Name, SectionName, MountPoint, EnvironmentVariableName);

    public:
        std::wstring Name;
        std::wstring SectionName;
        std::wstring MountPoint;
        std::wstring EnvironmentVariableName;

    private:
        friend struct ConfigPackagePoliciesDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ConfigPackageSettingDescription);

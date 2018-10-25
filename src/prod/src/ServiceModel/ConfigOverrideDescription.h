// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // ServicePackageConfigOverrides/ConfigOverride digested from 
    // ApplicationManifest/ServiceManifestImport/ConfigOverrides/ConfigOverride
    struct ConfigOverrideDescription
    {
    public:
        ConfigOverrideDescription();
        ConfigOverrideDescription(ConfigOverrideDescription const & other);
        ConfigOverrideDescription(ConfigOverrideDescription && other);

        ConfigOverrideDescription const & operator = (ConfigOverrideDescription const & other);
        ConfigOverrideDescription const & operator = (ConfigOverrideDescription && other);

        bool operator == (ConfigOverrideDescription const & other) const;
        bool operator != (ConfigOverrideDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
		
        void clear();

    public:
        std::wstring ConfigPackageName;
        Common::ConfigSettingsOverride SettingsOverride;

    private:
        friend struct DigestedConfigPackageDescription;
		friend struct ServiceManifestImportDescription;
        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);

        static Common::ConfigSettingsOverride ReadSettingsOverride(Common::XmlReader &);
        static Common::ConfigSectionOverride ReadSectionOverride(Common::XmlReader &);
        static Common::ConfigParameterOverride ReadParameterOverride(Common::XmlReader &);

        static Common::ErrorCode WriteSettingsOverride(Common::ConfigSettingsOverride const &, Common::XmlWriterUPtr const &);
        static Common::ErrorCode WriteSectionOverride(Common::ConfigSectionOverride  const &, Common::XmlWriterUPtr const &);
        static Common::ErrorCode WriteParameterOverride(Common::ConfigParameterOverride const &, Common::XmlWriterUPtr const &);
    };
}


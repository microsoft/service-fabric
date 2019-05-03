// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    struct DigestedConfigPackageDescription;
	struct ServiceManifestImportDescription;
}

namespace Common
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

        void WriteTo(TextWriter & w, FormatOptions const &) const;
		
        void clear();

    public:
        std::wstring ConfigPackageName;
        ConfigSettingsOverride SettingsOverride;

    private:
        friend struct ServiceModel::DigestedConfigPackageDescription;
		friend struct ServiceModel::ServiceManifestImportDescription;
        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
    };
}


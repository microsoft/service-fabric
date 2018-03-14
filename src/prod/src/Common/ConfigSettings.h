// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    struct Parser;
}

namespace Common
{
    // <Settings>
    struct ConfigSettings
    {
    public:
        ConfigSettings();
        ConfigSettings(ConfigSettings const & other);
        ConfigSettings(ConfigSettings && other);

        ConfigSettings const & operator = (ConfigSettings const & other);
        ConfigSettings const & operator = (ConfigSettings && other);

        bool operator == (ConfigSettings const & other) const;
        bool operator != (ConfigSettings const & other) const;

        void WriteTo(TextWriter & w, FormatOptions const &) const;

        void ToPublicApi(__in ScopedHeap & heap, __out FABRIC_CONFIGURATION_SETTINGS & publicSettings) const;

        void ApplyOverrides(ConfigSettingsOverride const& configSettingsOverride);
        void Merge(ConfigSettings const & other);

        void Set(std::wstring const & sectionName, ConfigParameter && parameter);

        void Remove(std::wstring const & sectionName);

        void clear();
    public:
        std::map<std::wstring, ConfigSection, IsLessCaseInsensitiveComparer<std::wstring>> Sections;

    private:
        friend struct ServiceModel::Parser;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        bool TryAddSection(ConfigSection && section);
    };
}


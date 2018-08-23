// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Common
{
    // <ConfigOverrides ...><Settings>
    struct ConfigSettingsOverride
    {
    public:
        typedef std::map<std::wstring, ConfigSectionOverride, IsLessCaseInsensitiveComparer<std::wstring>> SectionMapType;

        ConfigSettingsOverride();
        ConfigSettingsOverride(SectionMapType && sections);
        ConfigSettingsOverride(ConfigSettingsOverride const & other);
        ConfigSettingsOverride(ConfigSettingsOverride && other);

        ConfigSettingsOverride const & operator = (ConfigSettingsOverride const & other);
        ConfigSettingsOverride const & operator = (ConfigSettingsOverride && other);

        bool operator == (ConfigSettingsOverride const & other) const;
        bool operator != (ConfigSettingsOverride const & other) const;

        void WriteTo(TextWriter & w, FormatOptions const &) const;

        void clear();
    public:
        SectionMapType Sections;
    };
}

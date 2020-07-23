// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Common
{
    // <ConfigOverride ...><Settings><Section ...>
    struct ConfigSettingsOverride;
    struct ConfigSectionOverride
    {
    public:
        ConfigSectionOverride();
        ConfigSectionOverride(ConfigSectionOverride const & other);
        ConfigSectionOverride(ConfigSectionOverride && other);

        ConfigSectionOverride const & operator = (ConfigSectionOverride const & other);
        ConfigSectionOverride const & operator = (ConfigSectionOverride && other);

        bool operator == (ConfigSectionOverride const & other) const;
        bool operator != (ConfigSectionOverride const & other) const;

        void WriteTo(TextWriter & w, FormatOptions const &) const;

        void clear();
    public:
        std::wstring Name;
        std::map<std::wstring, ConfigParameterOverride, IsLessCaseInsensitiveComparer<std::wstring>> Parameters;

    private:
        friend struct ConfigSettingsOverride;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
        bool TryAddParameter(ConfigParameterOverride && parameter);
    };
}

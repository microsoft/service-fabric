// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Common
{
    // <ConfigOverride ..><Settings><Section ..><Parameter .../>
    struct ConfigSectionOverride;
    struct ConfigParameterOverride
    {
    public:
        ConfigParameterOverride();
        ConfigParameterOverride(ConfigParameterOverride const & other);
        ConfigParameterOverride(ConfigParameterOverride && other);

        ConfigParameterOverride & operator = (ConfigParameterOverride const & other) = default;
        ConfigParameterOverride & operator = (ConfigParameterOverride && other) = default;

        bool operator == (ConfigParameterOverride const & other) const;
        bool operator != (ConfigParameterOverride const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

    public:
        std::wstring Name;
        std::wstring Value;
        bool IsEncrypted;
		std::wstring Type;

    private:
        friend struct ConfigSectionOverride;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}

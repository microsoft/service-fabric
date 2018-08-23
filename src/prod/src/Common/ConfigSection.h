// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Common
{
    // <Settings><Section ...>
    struct ConfigSection
    {
    public:
        typedef std::map<std::wstring, ConfigParameter, Common::IsLessCaseInsensitiveComparer<std::wstring>> ParametersMapType;

        ConfigSection();
        ConfigSection(std::wstring && name, ParametersMapType && parameters);
        ConfigSection(ConfigSection const & other);
        ConfigSection(ConfigSection && other);

        ConfigSection const & operator = (ConfigSection const & other);
        ConfigSection const & operator = (ConfigSection && other);

        bool operator == (ConfigSection const & other) const;
        bool operator != (ConfigSection const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_CONFIGURATION_SECTION & section) const;
        void Merge(ConfigSection const & other);

        void Set(ConfigParameter && parameter);

        void clear();
    public:
        std::wstring Name;
        ParametersMapType Parameters;

    private:
    };
}

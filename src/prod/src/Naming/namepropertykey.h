// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class NamePropertyKey
    {
    public:
        NamePropertyKey(Common::NamingUri const & uri, std::wstring const & propertyName);

        __declspec(property(get=get_Name)) Common::NamingUri const & Name;
        __declspec(property(get=get_PropertyName)) std::wstring const & PropertyName;
        __declspec(property(get=get_Key)) std::wstring const & Key;

        inline Common::NamingUri const & get_Name() const { return uri_; }
        inline std::wstring const & get_PropertyName() const { return propertyName_; }
        inline std::wstring const & get_Key() const { return key_; }

        bool operator == (NamePropertyKey const & other) const;
        bool operator < (NamePropertyKey const & other) const;

        static std::wstring CreateKey(Common::NamingUri const &, std::wstring const & propertyName);

        void WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const;

    private:
        Common::NamingUri uri_;
        std::wstring propertyName_;
        std::wstring key_;
    };
}

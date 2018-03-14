// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EnumerateSubNamesToken : public Serialization::FabricSerializable
    {
    public:
        EnumerateSubNamesToken();

        EnumerateSubNamesToken(
           Common::NamingUri const & lastEnumeratedName,
            _int64 subnamesVersion);

        static Common::ErrorCode Create(std::wstring const & escapedToken, __out EnumerateSubNamesToken & token);

        __declspec(property(get=get_NamingUri)) Common::NamingUri const & LastEnumeratedName;
        __declspec(property(get=get_SubnamesVersion)) _int64 SubnamesVersion;
        __declspec(property(get=get_IsValid)) bool IsValid;

        Common::NamingUri const & get_NamingUri() const { return lastEnumeratedName_; }
        _int64 get_SubnamesVersion() const { return subnamesVersion_; }
        bool get_IsValid() const { return isValid_; }

        Common::ErrorCode ToEscapedString(__out std::wstring & escapedToken) const;

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_03(lastEnumeratedName_, subnamesVersion_, isValid_);

    private:
        static std::wstring const Delimiter;

        Common::NamingUri lastEnumeratedName_;
        _int64 subnamesVersion_;
        bool isValid_;
    };
}

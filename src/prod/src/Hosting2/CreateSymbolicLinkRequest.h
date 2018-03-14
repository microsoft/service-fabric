// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{

    class CreateSymbolicLinkRequest : public Serialization::FabricSerializable
    {
    public:
        CreateSymbolicLinkRequest();
        CreateSymbolicLinkRequest(
            std::vector<ArrayPair<std::wstring, std::wstring>> const & symbolicLinks,
            DWORD flags);

        __declspec(property(get=get_SymbolicLinks)) std::vector<ArrayPair<std::wstring, std::wstring>> const & SymbolicLinks;
        std::vector<ArrayPair<std::wstring, std::wstring>> const & get_SymbolicLinks() const { return symbolicLinks_; }

        __declspec(property(get=get_Flags)) DWORD Flags;
        DWORD get_Flags() const { return flags_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(symbolicLinks_, flags_);

    private:
        std::vector<ArrayPair<std::wstring, std::wstring>> symbolicLinks_;
        DWORD flags_;
    };
}

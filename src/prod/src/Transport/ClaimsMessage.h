// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class ClaimsMessage : public Serialization::FabricSerializable
    {
    public:
        ClaimsMessage();
        ClaimsMessage(std::wstring const & claims);

        static MessageUPtr CreateClaimsMessage(std::wstring const& token);

        std::wstring const & Claims() const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_01(claims_);

    private:
        std::wstring claims_;
    };
}

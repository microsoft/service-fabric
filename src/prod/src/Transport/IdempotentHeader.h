// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class IdempotentHeader : public MessageHeader<MessageHeaderId::Idempotent>, public Serialization::FabricSerializable
    {
    public:
        IdempotentHeader() {}
        IdempotentHeader(bool idempotent) : idempotent_(idempotent) {}

        __declspec(property(get=get_Idempotent)) bool Idempotent;
        bool get_Idempotent() const { return idempotent_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_01(idempotent_);
    private:
        bool idempotent_ = true;
    };
}

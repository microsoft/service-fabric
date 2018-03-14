// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class BroadcastRelatesToHeader : public Transport::MessageHeader<Transport::MessageHeaderId::BroadcastRelatesTo>, public Serialization::FabricSerializable
    {
    public:
        BroadcastRelatesToHeader()
        {
        }

        BroadcastRelatesToHeader(Transport::MessageId const & messageId)
            : messageId_(messageId)
        {
        }

        __declspec(property(get=get_MessageId)) Transport::MessageId const & MessageId;

        Transport::MessageId const & get_MessageId() const { return messageId_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const 
        { 
            w << messageId_;
        }

        FABRIC_FIELDS_01(messageId_);

    private:
        Transport::MessageId messageId_;
    };
}

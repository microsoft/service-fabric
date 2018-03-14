// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class MulticastHeader : public Transport::MessageHeader<Transport::MessageHeaderId::Multicast>, public Serialization::FabricSerializable
    {
    public:
        MulticastHeader()
        {
        }

        MulticastHeader(NodeInstance const & from, Transport::MessageId const & multicastId)
             : from_(from), multicastId_(multicastId)
        {
        }

        __declspec(property(get=get_From)) NodeInstance const & From;
        __declspec(property(get=get_MulticastId)) Transport::MessageId const & MulticastId;

        NodeInstance const & get_From() const { return this->from_; }
        Transport::MessageId const & get_MulticastId() const { return this->multicastId_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const 
        { 
            w << "[From: " << this->from_ <<
                 ", MulticastId_: " << this->multicastId_ <<
                 "]";
        }

        FABRIC_FIELDS_02(from_, multicastId_);

    private:
        NodeInstance from_;
        Transport::MessageId multicastId_;
    };
}

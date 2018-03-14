// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class BroadcastRangeHeader : public Transport::MessageHeader<Transport::MessageHeaderId::BroadcastRange>, public Serialization::FabricSerializable
    {
    public:
        BroadcastRangeHeader()
        {
        }

        BroadcastRangeHeader(NodeIdRange const & range)
             : range_(range)
        {
        }

        __declspec(property(get=get_Range)) NodeIdRange const & Range;

        NodeIdRange const & get_Range() const { return this->range_; }


        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const 
        { 
            w << "[Range: " << this->range_ << "]";
        }

        FABRIC_FIELDS_01(range_);

    private:
        NodeIdRange range_;
    };
}

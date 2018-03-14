// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class ExpectsReplyHeader : public MessageHeader<MessageHeaderId::ExpectsReply>, public Serialization::FabricSerializable
    {
    public:
        ExpectsReplyHeader() : expectsReply_(false) { }
        ExpectsReplyHeader(bool expectsReply) : expectsReply_(expectsReply) { }

        __declspec(property(get=get_ExpectsReply)) bool ExpectsReply;

        bool get_ExpectsReply() const { return expectsReply_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
        {
            w << expectsReply_;
        }

        FABRIC_FIELDS_01(expectsReply_);

    private:
        //DENY_COPY_ASSIGNMENT(ExpectsReplyHeader); there is only a bool, copy should be allowed
        bool expectsReply_;
    };
}

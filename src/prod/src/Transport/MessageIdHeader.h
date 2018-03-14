// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class MessageIdHeader : public MessageHeader<MessageHeaderId::MessageId>, public Serialization::FabricSerializable
    {
    public:
        MessageIdHeader();
        MessageIdHeader(Transport::MessageId const & messageId);

        __declspec(property(get=get_MessageId)) Transport::MessageId const & MessageId;
        Transport::MessageId const & get_MessageId() const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_01(messageId_);
    private:
        //DENY_COPY_ASSIGNMENT(MessageIdHeader); TODO: deny later
        Transport::MessageId messageId_;
    };
}

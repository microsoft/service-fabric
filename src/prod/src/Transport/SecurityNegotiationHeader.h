// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class SecurityNegotiationHeader : public MessageHeader<MessageHeaderId::SecurityNegotiation>, public Serialization::FabricSerializable
    {
    public:
        SecurityNegotiationHeader();
        SecurityNegotiationHeader(
            bool framingProtectionEnabled,
            ListenInstance const & listenInstance,
            size_t maxIncomingFrameSize);

        bool FramingProtectionEnabled() const;
        size_t MaxIncomingFrameSize() const { return maxIncomingFrameSize_; }

        bool operator == (SecurityNegotiationHeader const & other) const;
        bool operator != (SecurityNegotiationHeader const & other) const { return !(*this == other); }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_04(x509ExtraFramingEnabled_, framingProtectionEnabled_, listenInstance_, maxIncomingFrameSize_);

    private:
        bool x509ExtraFramingEnabled_ = true; //deprecated
        bool framingProtectionEnabled_;
        ListenInstance listenInstance_;
        size_t maxIncomingFrameSize_ = 0;
    };
}

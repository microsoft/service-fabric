// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class UncorrelatedReplyHeader : public MessageHeader<MessageHeaderId::UncorrelatedReply>, public Serialization::FabricSerializable
    {
    public:
        UncorrelatedReplyHeader() : isUncorrelatedReply_(true) { }

        __declspec(property(get=get_IsUncorrelatedReply)) bool IsUncorrelatedReply;
        bool get_IsUncorrelatedReply() const { return isUncorrelatedReply_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const { w << isUncorrelatedReply_; }

        FABRIC_FIELDS_01(isUncorrelatedReply_);

    private:
        bool isUncorrelatedReply_;
    };
}

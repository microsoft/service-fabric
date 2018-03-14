// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class BroadcastHeader : public Transport::MessageHeader<Transport::MessageHeaderId::Broadcast>, public Serialization::FabricSerializable
    {
    public:
        BroadcastHeader()
            : expectsReply_(false), expectsAck_(false)
        {
        }

        BroadcastHeader(
            NodeInstance const & from,
            Transport::MessageId const & broadcastId,
            bool expectsReply,
            bool expectsAck,
            std::wstring const & fromRing)
             :  from_(from),
                broadcastId_(broadcastId),
                expectsReply_(expectsReply),
                expectsAck_(expectsAck),
                fromRing_(fromRing)
        {
        }

        __declspec(property(get=get_From)) NodeInstance const & From;
        __declspec(property(get=get_BroadcastId)) Transport::MessageId const & BroadcastId;
        __declspec(property(get=get_ExpectsReply)) bool ExpectsReply;
        __declspec(property(get=get_ExpectsAck)) bool ExpectsAck;
        __declspec(property(get=get_FromRing)) std::wstring const & FromRing;

        NodeInstance const & get_From() const { return this->from_; }
        Transport::MessageId const & get_BroadcastId() const { return this->broadcastId_; }
        bool get_ExpectsReply() const { return this->expectsReply_; }
        bool get_ExpectsAck() const { return this->expectsAck_; }
        std::wstring const & get_FromRing() const { return fromRing_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const 
        { 
            w << "[From: " << this->from_;
            if (!fromRing_.empty())
            {
                w << L"@" << fromRing_;
            }

            w << ", BroadcastId: " << this->broadcastId_ <<
                 ", ExpectsReply: " << this->expectsReply_ <<
                 ", ExpectsAck: " << this->expectsAck_ <<
                 "]";
        }

        FABRIC_FIELDS_05(from_, broadcastId_, expectsReply_, expectsAck_, fromRing_);

    private:
        NodeInstance from_;
        Transport::MessageId broadcastId_;
        bool expectsReply_;
        bool expectsAck_;
        std::wstring fromRing_;
    };
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class RoutingHeader : public Transport::MessageHeader<Transport::MessageHeaderId::Routing>, public Serialization::FabricSerializable
    {
    public:
        RoutingHeader()
        {
        }

        RoutingHeader(
            NodeInstance const & from,
            std::wstring const & fromRing,
            NodeInstance const & to,
            std::wstring const & toRing,
            Transport::MessageId const & messageId,
            Common::TimeSpan expiration,
            Common::TimeSpan retryTimeout,
            bool useExactRouting,
            bool expectsReply)
            :
                from_(from),
                fromRing_(fromRing),
                to_(to),
                toRing_(toRing),
                messageId_(messageId),
                expiration_(expiration),
                retryTimeout_(retryTimeout),
                useExactRouting_(useExactRouting),
                expectsReply_(expectsReply)
        {
        }

        __declspec(property(get=get_From)) NodeInstance const & From;
        __declspec(property(get=get_To)) NodeInstance const & To;
        __declspec(property(get=get_MessageId)) Transport::MessageId const & MessageId;
        __declspec(property(get=get_Expiration, put=set_Expiration)) Common::TimeSpan Expiration;
        __declspec(property(get=get_RetryTimeout)) Common::TimeSpan RetryTimeout;
        __declspec(property(get=get_UseExactRouting)) bool UseExactRouting;
        __declspec(property(get=get_ExpectsReply)) bool ExpectsReply;
        __declspec(property(get=get_FromRing)) std::wstring const & FromRing;
        __declspec(property(get=get_ToRing)) std::wstring const & ToRing;

        NodeInstance const & get_From() const { return this->from_; }
        NodeInstance const & get_To() const { return this->to_; }
        Transport::MessageId const & get_MessageId() const { return this->messageId_; }
        Common::TimeSpan get_Expiration() const { return this->expiration_; }
        Common::TimeSpan get_RetryTimeout() const { return this->retryTimeout_; }
        bool get_UseExactRouting() const { return this->useExactRouting_; }
        bool get_ExpectsReply() const { return this->expectsReply_; }
        std::wstring const & get_FromRing() const { return fromRing_; }
        std::wstring const & get_ToRing() const { return toRing_; }

        void set_Expiration(Common::TimeSpan expiration) { this->expiration_ = expiration; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const 
        { 
            w << "[From: " << this->from_;
            if (!fromRing_.empty())
            {
                w << L"@" << fromRing_;
            }

            w << ", To: " << this->to_;
            if (!toRing_.empty())
            {
                w << L"@" << toRing_;
            }

            w << ", MessageId: " <<  this->messageId_ <<
                 ", Expiration: " << this->expiration_ <<
                 ", RetryTimeout: " << this->retryTimeout_ <<
                 ", UseExactRouting: " << this->useExactRouting_ <<
                 ", ExpectsReply: " << this->expectsReply_ <<
                 "]";
        }

        FABRIC_FIELDS_09(from_, to_, messageId_, useExactRouting_, expectsReply_, expiration_, retryTimeout_, fromRing_, toRing_);

    private:
        NodeInstance from_;
        std::wstring fromRing_;
        NodeInstance to_;
        std::wstring toRing_;
        Transport::MessageId messageId_;
        Common::TimeSpan expiration_;
        Common::TimeSpan retryTimeout_;
        bool useExactRouting_;
        bool expectsReply_;
    };
}

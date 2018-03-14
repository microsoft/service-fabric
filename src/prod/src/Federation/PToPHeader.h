// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class PToPHeader : public Transport::MessageHeader<Transport::MessageHeaderId::PToP>, public Serialization::FabricSerializable
    {
    public:
        PToPHeader()
            : exactInstance_(true)
        {
        }

        PToPHeader(
            NodeInstance const & from, 
            NodeInstance const & to,
            PToPActor::Enum actor,
            std::wstring const & fromRing,
            std::wstring const & toRing,
            bool exactInstance)
            : from_(from), to_(to), actor_(actor), fromRing_(fromRing), toRing_(toRing), exactInstance_(exactInstance)
        {
        }

        __declspec(property(get=get_From)) NodeInstance const & From;
        __declspec(property(get=get_To)) NodeInstance const & To;
        __declspec(property(get=get_Actor)) PToPActor::Enum Actor;
        __declspec(property(get=get_FromRing)) std::wstring const & FromRing;
        __declspec(property(get=get_ToRing)) std::wstring const & ToRing;
        __declspec(property(get=get_ExactInstance)) bool ExactInstance;

        NodeInstance const & get_From() const { return this->from_; }
        NodeInstance const & get_To() const { return this->to_; }
        PToPActor::Enum get_Actor() const { return static_cast<PToPActor::Enum>(this->actor_); }
        std::wstring const & get_FromRing() const { return fromRing_; }
        std::wstring const & get_ToRing() const { return toRing_; }
        bool get_ExactInstance() const { return exactInstance_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const 
        {
            w << L"[From: " << this->from_;
            if (!fromRing_.empty())
            {
                w << L"@" << fromRing_;
            }

            w << L", To: " << this->to_;
            if (!toRing_.empty())
            {
                w << L"@" << toRing_;
            }

            if (exactInstance_)
            {
                w << L", exact";
            }

            w << L", Actor: " << this->Actor <<
                 L"]";
        }

        FABRIC_FIELDS_06(from_, to_, actor_, fromRing_, toRing_, exactInstance_);

    private:
        NodeInstance from_;
        NodeInstance to_;
        PToPActor::Enum actor_;
        std::wstring fromRing_;
        std::wstring toRing_;
        bool exactInstance_;
    };
}

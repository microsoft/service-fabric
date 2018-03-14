// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class ForwardMessageHeader
        : public MessageHeader<MessageHeaderId::ForwardMessage>
        , public Serialization::FabricSerializable
    {
    public:
        ForwardMessageHeader()
        {
        }

        ForwardMessageHeader(Actor::Enum const actor, std::wstring const& action)
            : originalActor_(actor)
            , originalAction_(action)
        {
        }

        ForwardMessageHeader(ForwardMessageHeader const & other)
            : originalActor_(other.originalActor_)
            , originalAction_(other.originalAction_)
        {
        }

        __declspec(property(get=get_Actor)) Actor::Enum Actor;
        Actor::Enum get_Actor() const { return originalActor_; }

        __declspec(property(get=get_Action)) std::wstring const& Action;
        std::wstring const& get_Action() const { return originalAction_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w.Write(
            "{0}:{1}",
            originalActor_,
            originalAction_);
        }

        FABRIC_FIELDS_02(originalActor_, originalAction_);

    private:
        Actor::Enum originalActor_;
        std::wstring originalAction_;
    };
}

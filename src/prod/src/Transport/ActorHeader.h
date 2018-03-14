// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class ActorHeader : public MessageHeader<MessageHeaderId::Actor>, public Serialization::FabricSerializable
    {
    public:
        ActorHeader ();
        ActorHeader (Actor::Enum actor);

        __declspec(property(get=get_Actor)) Actor::Enum Actor;
        Actor::Enum const & get_Actor() const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_01(actor_);

    private:
        Actor::Enum actor_;
    };
}

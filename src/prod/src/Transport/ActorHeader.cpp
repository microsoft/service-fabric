// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;

ActorHeader::ActorHeader ()
    : actor_(Actor::Enum::Empty)
{
}

ActorHeader::ActorHeader (Actor::Enum actor)
    : actor_(actor)
{
}

Actor::Enum const & ActorHeader::get_Actor() const
{
    return actor_;
}

void ActorHeader::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w << actor_;
}

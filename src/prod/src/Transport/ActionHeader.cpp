// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;

ActionHeader::ActionHeader()
{
}

ActionHeader::ActionHeader(std::wstring const & action) : action_(action)
{
}

ActionHeader::ActionHeader(std::wstring && action) : action_(std::move(action))
{
}

ActionHeader::ActionHeader(ActionHeader const & rhs) : action_(rhs.action_)
{
}

ActionHeader::ActionHeader(ActionHeader && rhs) : action_(std::move(rhs.action_))
{
}

std::wstring const & ActionHeader::get_Action() const
{
    return action_;
}

void ActionHeader::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w << action_;
}

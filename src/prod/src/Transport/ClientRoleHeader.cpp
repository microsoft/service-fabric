// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Transport
{
    using namespace Common;

    ClientRoleHeader::ClientRoleHeader()
    {
    }

    ClientRoleHeader::ClientRoleHeader(RoleMask::Enum role)
        : role_(role)
    {
    }

    RoleMask::Enum ClientRoleHeader::Role()
    {
        return role_;
    }

    void ClientRoleHeader::WriteTo(TextWriter & w, FormatOptions const &) const
    {
        w << role_;
    }
}

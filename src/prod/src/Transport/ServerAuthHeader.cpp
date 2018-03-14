// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;

ServerAuthHeader::ServerAuthHeader() : willSendConnectionAuthStatus_(false)
{
}

ServerAuthHeader::ServerAuthHeader(bool willSendConnectionAuthStatus) : willSendConnectionAuthStatus_(willSendConnectionAuthStatus)
{
}

bool ServerAuthHeader::WillSendConnectionAuthStatus() const
{
    return willSendConnectionAuthStatus_;
}

void ServerAuthHeader::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w << "willSendConnectionAuthStatus=" << willSendConnectionAuthStatus_;
    if (metadata_) 
    { 
        w << ",metadata=" << *metadata_; 
    }
    else
    {
        w << ",metadata=none";
    }
}

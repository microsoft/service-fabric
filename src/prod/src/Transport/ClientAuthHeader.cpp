// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Transport
{
    ClientAuthHeader::ClientAuthHeader()
    {
    }

    ClientAuthHeader::ClientAuthHeader(SecurityProvider::Enum providerType)
        : providerType_(providerType)
    {
    }

    SecurityProvider::Enum ClientAuthHeader::ProviderType()
    {
        return providerType_;
    }

    void ClientAuthHeader::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
    {
        w << providerType_;
    }
}

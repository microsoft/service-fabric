// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    interface INetworkParams
    {
        K_SHARED_INTERFACE(INetworkParams);

    public:
        virtual const KArray<ULONG>& DnsServers() const = 0;
        virtual const KArray<KString::SPtr> DnsSearchList() const = 0;
    };
}

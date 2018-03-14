// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

void DnsFlags::SetFlag(__out USHORT& flags, DNS_FLAG flag)
{
    flags |= flag;
}

void DnsFlags::UnsetFlag(__out USHORT& flags, DNS_FLAG flag)
{
    flags &= ~flag;
}

bool DnsFlags::IsFlagSet(__in USHORT flags, DNS_FLAG flag)
{
    return ((flags & flag) == flag);
}

void DnsFlags::SetResponseCode(__out USHORT& flags, DNS_RESPONSE_CODE code)
{
    flags &= ~0xf;
    flags |= (code & 0xf);
}

USHORT DnsFlags::GetResponseCode(__in USHORT flags)
{
    flags &= 0xf;
    return flags;
}

bool DnsFlags::IsSuccessResponseCode(__in USHORT rc)
{
    return (rc == DnsFlags::RC_NOERROR || rc == DnsFlags::RC_NXDOMAIN);
}

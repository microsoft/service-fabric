// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "DnsMessageHeader.h"

bool DnsMessageHeader::Deserialize(
    __out USHORT& id,
    __out USHORT& flags,
    __out USHORT& qdCount,
    __out USHORT& anCount,
    __out USHORT& nsCount,
    __out USHORT& adCount,
    __in BinaryReader& reader
)
{
    if (!reader.ReadUInt16(/*out*/id))
    {
        return false;
    }

    if (!reader.ReadUInt16(/*out*/flags))
    {
        return false;
    }

    if (!reader.ReadUInt16(/*out*/qdCount))
    {
        return false;
    }

    if (!reader.ReadUInt16(/*out*/anCount))
    {
        return false;
    }

    if (!reader.ReadUInt16(/*out*/nsCount))
    {
        return false;
    }

    if (!reader.ReadUInt16(/*out*/adCount))
    {
        return false;
    }

    return true;
}

bool DnsMessageHeader::Serialize(
    __in BinaryWriter& writer,
    __in USHORT id,
    __in USHORT flags,
    __in USHORT qdCount,
    __in USHORT anCount,
    __in USHORT nsCount,
    __in USHORT adCount
)
{
    if (!writer.WriteUInt16(id))
    {
        return false;
    }

    if (!writer.WriteUInt16(flags))
    {
        return false;
    }

    if (!writer.WriteUInt16(qdCount))
    {
        return false;
    }

    if (!writer.WriteUInt16(anCount))
    {
        return false;
    }

    if (!writer.WriteUInt16(nsCount))
    {
        return false;
    }

    if (!writer.WriteUInt16(adCount))
    {
        return false;
    }

    return true;
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "BinaryReader.h"
#include "BinaryWriter.h"

namespace DNS
{
    namespace DnsMessageHeader
    {
        bool Deserialize(
            __out USHORT& id,
            __out USHORT& flags,
            __out USHORT& qdCount,
            __out USHORT& anCount,
            __out USHORT& nsCount,
            __out USHORT& adCount,
            __in BinaryReader& reader
        );

        bool Serialize(
            __in BinaryWriter& writer,
            __in USHORT id,
            __in USHORT flags,
            __in USHORT qdCount,
            __in USHORT anCount,
            __in USHORT nsCount,
            __in USHORT adCount
        );
    };
}

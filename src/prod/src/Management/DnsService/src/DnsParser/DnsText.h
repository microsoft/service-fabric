// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "BinaryReader.h"
#include "BinaryWriter.h"

namespace DNS
{
    namespace DnsText
    {
        bool Skip(
            __in BinaryReader& reader
        );

        bool Deserialize(
            __out KString::SPtr& spText,
            __out DnsTextEncodingType& encodingType,
            __in KAllocator& allocator,
            __in BinaryReader& reader
        );

        bool Serialize(
            __out KStringA::SPtr& spResult,
            __in KAllocator& allocator,
            __in const KStringView& text,
            __in DnsTextEncodingType encodingType
        );
    }
}

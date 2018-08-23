// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "DnsAnswerRecordOpt.h"

/*static*/
void DnsAnswerRecordOpt::Create(
    __out DnsAnswerRecordOpt::SPtr& spRecord,
    __in KAllocator& allocator,
    __in IDnsRecord& question,
    __in USHORT size
)
{
    spRecord = _new(TAG, allocator) DnsAnswerRecordOpt(question, size);
    KInvariant(spRecord != nullptr);
}

DnsAnswerRecordOpt::DnsAnswerRecordOpt(
    __in IDnsRecord& question,
    __in USHORT size
) : DnsAnswerRecordBase(DnsRecordTypeOpt, question),
_size(size)
{
}

DnsAnswerRecordOpt::~DnsAnswerRecordOpt()
{
}

bool DnsAnswerRecordOpt::Serialize(
    __in BinaryWriter& writer,
    __in ULONG ttl
) const
{
    // OPT is special, it repurposes the header.
    // TTL is actually an extended RCODE.
    // CLASS is the max payload size.
    // RFC 6891

    if (!__super::Serialize(writer, ttl, _size))
    {
        return false;
    }

    USHORT rdLength = 0;
    if (!writer.WriteUInt16(rdLength))
    {
        return false;
    }

    return true;
}

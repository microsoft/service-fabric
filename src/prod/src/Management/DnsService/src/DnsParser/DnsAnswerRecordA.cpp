// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "DnsAnswerRecordA.h"

/*static*/
void DnsAnswerRecordA::Create(
    __out DnsAnswerRecordA::SPtr& spRecord,
    __in KAllocator& allocator,
    __in IDnsRecord& question,
    __in ULONG address
)
{
    spRecord = _new(TAG, allocator) DnsAnswerRecordA(question, address);
    KInvariant(spRecord != nullptr);
}

DnsAnswerRecordA::DnsAnswerRecordA(
    __in IDnsRecord& question,
    __in ULONG address
) : DnsAnswerRecordBase(DnsRecordTypeA, question),
    _address(address)
{
}

DnsAnswerRecordA::~DnsAnswerRecordA()
{
}

bool DnsAnswerRecordA::Serialize(
    __in BinaryWriter& writer,
    __in ULONG ttl
) const
{
    if (!__super::Serialize(writer, ttl))
    {
        return false;
    }

    USHORT rdLength = sizeof(_address);
    if (!writer.WriteUInt16(rdLength))
    {
        return false;
    }

    if (!writer.WriteUInt32(_address))
    {
        return false;
    }

    return true;
}

/*static*/
bool DnsAnswerRecordA::Deserialize(
    __out DnsAnswerRecordA::SPtr& spRecord,
    __in KAllocator& allocator,
    __in BinaryReader& reader,
    __in IDnsRecord& question
)
{
    ULONG address;
    if (!reader.ReadUInt32(address))
    {
        return false;
    }

    DnsAnswerRecordA::Create(/*out*/spRecord, allocator, question, address);

    return true;
}

KString::SPtr DnsAnswerRecordA::DebugString() const
{
    WCHAR temp[256];

    IN_ADDR inaddr;
#if !defined(PLATFORM_UNIX)
    inaddr.S_un.S_addr = htonl(_address);
    InetNtop(AF_INET, &inaddr, temp, ARRAYSIZE(temp));
#else
    CHAR tempAscii[ARRAYSIZE(temp)];
    inaddr.s_addr = htonl(_address);
    inet_ntop(AF_INET, &inaddr, tempAscii, ARRAYSIZE(tempAscii));

    for (ULONG i = 0; i < ARRAYSIZE(temp); i++)
    {
        temp[i] = WCHAR(tempAscii[i]);
    }
#endif

    return KString::Create(temp, GetThisAllocator());
}

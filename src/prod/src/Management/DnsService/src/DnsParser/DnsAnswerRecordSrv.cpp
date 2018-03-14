// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "DnsAnswerRecordSrv.h"
#include "DnsText.h"

/*static*/
void DnsAnswerRecordSrv::Create(
    __out DnsAnswerRecordSrv::SPtr& spRecord,
    __in KAllocator& allocator,
    __in IDnsRecord& question,
    __in KString& strHost,
    __in USHORT port
)
{
    spRecord = _new(TAG, allocator) DnsAnswerRecordSrv(question, strHost, port);
    KInvariant(spRecord != nullptr);
}

DnsAnswerRecordSrv::DnsAnswerRecordSrv(
    __in IDnsRecord& question,
    __in KString& strHost,
    __in USHORT port
) : DnsAnswerRecordBase(DnsRecordTypeSrv, question),
_port(port),
_spHost(&strHost)
{
}

DnsAnswerRecordSrv::~DnsAnswerRecordSrv()
{
}

bool DnsAnswerRecordSrv::Serialize(
    __in BinaryWriter& writer,
    __in ULONG ttl
) const
{
    if (!__super::Serialize(writer, ttl))
    {
        return false;
    }

    KStringA::SPtr spDnsText;
    if (!DnsText::Serialize(/*out*/spDnsText, GetThisAllocator(), Host(), _question.Encoding()))
    {
        return false;
    }

    USHORT rdLength = static_cast<USHORT>((3 * sizeof(USHORT)) + spDnsText->LengthInBytes());

    if (!writer.WriteUInt16(rdLength))
    {
        return false;
    }

    if (!writer.WriteUInt16(static_cast<USHORT>(0)))
    {
        return false;
    }

    if (!writer.WriteUInt16(static_cast<USHORT>(0)))
    {
        return false;
    }

    if (!writer.WriteUInt16(Port()))
    {
        return false;
    }

    if (!writer.WriteBytes(
        static_cast<PVOID>(*spDnsText),
        spDnsText->LengthInBytes(),
        spDnsText->LengthInBytes())
        )
    {
        return false;
    }

    return true;
}

KString::SPtr DnsAnswerRecordSrv::DebugString() const
{
    KString::SPtr spOutStr;
    KString::Create(/*out*/spOutStr, GetThisAllocator(), Host());

    WCHAR temp[256];
#if !defined(PLATFORM_UNIX)                
    _snwprintf_s(temp, ARRAYSIZE(temp), L":%d", Port());
#else
    swprintf(temp, ARRAYSIZE(temp), L":%d", Port());
#endif
    spOutStr->Concat(temp);

    spOutStr->SetNullTerminator();

    return spOutStr;
}

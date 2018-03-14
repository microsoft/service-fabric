// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "DnsAnswerRecordBase.h"
#include "DnsText.h"

DnsAnswerRecordBase::DnsAnswerRecordBase(
    __in USHORT type,
    __in IDnsRecord& question
) : _type(type),
_question(question)
{
}

DnsAnswerRecordBase::~DnsAnswerRecordBase()
{
}

bool DnsAnswerRecordBase::Serialize(
    __in BinaryWriter& writer,
    __in ULONG ttl,
    __in USHORT nClass
) const
{
    KStringA::SPtr spDnsText;
    if (!DnsText::Serialize(/*out*/spDnsText, GetThisAllocator(), _question.Name(), _question.Encoding()))
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

    if (!writer.WriteUInt16(Type()))
    {
        return false;
    }

    if (!writer.WriteUInt16(nClass))
    {
        return false;
    }

    if (!writer.WriteUInt32(ttl))
    {
        return false;
    }

    return true;
}

/*static*/
bool DnsAnswerRecordBase::DeserializeHeader(
    __out USHORT& type,
    __out USHORT& rdLength,
    __in BinaryReader& reader
)
{
    // Skip text
    if (!DnsText::Skip(reader))
    {
        return false;
    }

    if (!reader.ReadUInt16(/*out*/type))
    {
        return false;
    }

    USHORT nClass;
    if (!reader.ReadUInt16(/*out*/nClass))
    {
        return false;
    }

    ULONG ttl;
    if (!reader.ReadUInt32(/*out*/ttl))
    {
        return false;
    }

    if (!reader.ReadUInt16(/*out*/rdLength))
    {
        return false;
    }

    return true;
}

void DnsAnswerUnsupportedRecord::Create(
    __out DnsAnswerUnsupportedRecord::SPtr& spRecord,
    __in KAllocator& allocator,
    __in IDnsRecord& question,
    __in USHORT type
)
{
    spRecord = _new(TAG, allocator) DnsAnswerUnsupportedRecord(question, type);
    KInvariant(spRecord != nullptr);
}

DnsAnswerUnsupportedRecord::DnsAnswerUnsupportedRecord(
    __in IDnsRecord& question,
    __in USHORT type
) : DnsAnswerRecordBase(type, question)
{
}

DnsAnswerUnsupportedRecord::~DnsAnswerUnsupportedRecord()
{
}

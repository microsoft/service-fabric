// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "DnsQuestionRecord.h"
#include "DnsText.h"

/*static*/
void DnsQuestionRecord::Create(
    __out DnsQuestionRecord::SPtr& spRecord,
    __in KAllocator& allocator,
    __in DnsRecordType type,
    __in KString& strName,
    __in DnsTextEncodingType encodingType
)
{
    spRecord = _new(TAG, allocator) DnsQuestionRecord(type, strName, encodingType);
    KInvariant(spRecord != nullptr);
}

DnsQuestionRecord::DnsQuestionRecord(
    __in DnsRecordType type,
    __in KString& strName,
    __in DnsTextEncodingType encodingType
) : _type(type),
_spName(&strName),
_encodingType(encodingType)
{
}

DnsQuestionRecord::~DnsQuestionRecord()
{
}

bool DnsQuestionRecord::Serialize(
    __in BinaryWriter& writer
) const
{
    KStringA::SPtr spDnsText;
    if (!DnsText::Serialize(/*out*/spDnsText, GetThisAllocator(), Name(), Encoding()))
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

    USHORT qClass = 1;
    if (!writer.WriteUInt16(qClass))
    {
        return false;
    }

    return true;
}

/*static*/
bool DnsQuestionRecord::Deserialize(
    __out DnsQuestionRecord::SPtr& spRecord,
    __in KAllocator& allocator,
    __in BinaryReader& reader
)
{
    spRecord = nullptr;

    KString::SPtr spName;
    DnsTextEncodingType encodingType;
    if (!DnsText::Deserialize(/*out*/spName, /*out*/encodingType, allocator, reader))
    {
        return false;
    }

    CharLowerBuff(static_cast<PWCHAR>(*spName), spName->Length());

    USHORT qType;
    if (!reader.ReadUInt16(/*out*/qType))
    {
        return false;
    }

    USHORT qClass;
    if (!reader.ReadUInt16(/*out*/qClass))
    {
        return false;
    }

    DnsQuestionRecord::Create(
        /*out*/spRecord,
        allocator,
        static_cast<DnsRecordType>(qType),
        *spName,
        encodingType
    );

    return true;
}

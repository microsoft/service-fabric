// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "DnsParser.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"

#include "DnsMessage.h"
#include "DnsMessageHeader.h"

#include "DnsQuestionRecord.h"
#include "DnsAnswerRecordA.h"
#include "DnsAnswerRecordSrv.h"
#include "DnsAnswerRecordTxt.h"
#include "DnsAnswerRecordOpt.h"

void DNS::CreateDnsParser(
    __out IDnsParser::SPtr& spDnsParser,
    __in KAllocator& allocator
)
{
    DnsParser::SPtr sp;
    DnsParser::Create(/*out*/sp, allocator);

    spDnsParser = sp.RawPtr();
}

/*static*/
void DnsParser::Create(
    __out DnsParser::SPtr& spDnsParser,
    __in KAllocator& allocator
)
{
    spDnsParser = _new(TAG, allocator) DnsParser();
    KInvariant(spDnsParser != nullptr);
}

DnsParser::DnsParser()
{
}

DnsParser::~DnsParser()
{
}

bool DnsParser::GetHeader(
    __out USHORT& id,
    __out USHORT& flags,
    __out USHORT& questionCount,
    __out USHORT& answerCount,
    __out USHORT& nsCount,
    __out USHORT& adCount,
    __in KBuffer& buffer,
    __in ULONG numberOfBytes
) const
{
    id = 0;
    flags = 0;
    questionCount = 0;
    answerCount = 0;
    nsCount = 0;
    adCount = 0;

    BinaryReader reader(buffer, numberOfBytes);

    if (!DnsMessageHeader::Deserialize(
        /*out*/id,
        /*out*/flags,
        /*out*/questionCount,
        /*out*/answerCount,
        /*out*/nsCount,
        /*out*/adCount,
        reader))
    {
        return false;
    }

    return true;
}

bool DnsParser::SetHeader(
    __in KBuffer& buffer,
    __in USHORT id,
    __in USHORT flags,
    __in USHORT qdCount,
    __in USHORT anCount,
    __in USHORT nsCount,
    __in USHORT adCount
) const
{
    BinaryWriter writer(buffer);

    return DnsMessageHeader::Serialize(writer, id, flags, qdCount, anCount, nsCount, adCount);
}

bool DnsParser::Deserialize(
    __out IDnsMessage::SPtr& spMessage,
    __in KBuffer& buffer,
    __in ULONG numberOfBytes,
    __in bool fIncludeUnsupportedRecords
) const
{
    BinaryReader reader(buffer, numberOfBytes);

    USHORT id, flags, qdCount, anCount, nsCount, adCount;
    if (!DnsMessageHeader::Deserialize(/*out*/id, /*out*/flags, /*out*/qdCount,
        /*out*/anCount, /*out*/nsCount, /*out*/adCount, reader))
    {
        return false;
    }

    DnsMessage::SPtr spMessageInternal;
    DnsMessage::Create(/*out*/spMessageInternal, GetThisAllocator(), id, flags);

    for (USHORT i = 0; i < qdCount; i++)
    {
        DnsQuestionRecord::SPtr spRecord;
        if (!DnsQuestionRecord::Deserialize(/*out*/spRecord, GetThisAllocator(), reader))
        {
            return false;
        }

        if (STATUS_SUCCESS != spMessageInternal->Questions().Append(spRecord.RawPtr()))
        {
            return false;
        }
    }

    for (USHORT i = 0; i < anCount && qdCount > 0; i++)
    {
        USHORT type, rdLength;
        if (!DnsAnswerRecordBase::DeserializeHeader(/*out*/type, /*out*/rdLength, reader))
        {
            return false;
        }

        IDnsRecord::SPtr spQuestionRecord = spMessageInternal->Questions()[0];

        // We care only about A records
        if (type == DnsRecordTypeA)
        {
            DnsAnswerRecordA::SPtr spRecord;
            if (!DnsAnswerRecordA::Deserialize(/*out*/spRecord, GetThisAllocator(), reader, *spQuestionRecord))
            {
                return false;
            }

            if (STATUS_SUCCESS != spMessageInternal->Answers().Append(spRecord.RawPtr()))
            {
                return false;
            }
        }
        else
        {
            // Skip
            if (!reader.Skip(rdLength))
            {
                return false;
            }

            if (fIncludeUnsupportedRecords)
            {
                DnsAnswerUnsupportedRecord::SPtr spUnsupportedRecord;
                DnsAnswerUnsupportedRecord::Create(/*out*/spUnsupportedRecord, GetThisAllocator(), *spQuestionRecord, type);
                if (STATUS_SUCCESS != spMessageInternal->Answers().Append(spUnsupportedRecord.RawPtr()))
                {
                    return false;
                }
            }
        }
    }

    spMessage = spMessageInternal.RawPtr();

    return true;
}

bool DnsParser::Serialize(
    __out KBuffer& buffer,
    __out ULONG& numberOfBytesWritten,
    __in IDnsMessage& message,
    __in ULONG ttl,
    __in DnsTextEncodingType encodingType
) const
{
    numberOfBytesWritten = 0;

    BinaryWriter writer(buffer);

    KArray<IDnsRecord::SPtr>& arrQuestions = message.Questions();
    ULONG qdCount = arrQuestions.Count();

    KArray<IDnsRecord::SPtr>& arrAnswers = message.Answers();
    ULONG anCount = arrAnswers.Count();

    //
    // OPT record is always there to ensure UDP package larger than 512 bytes
    //
    const USHORT adCount = 1;

    if (!DnsMessageHeader::Serialize(writer, message.Id(), message.Flags(),
        static_cast<USHORT>(qdCount), static_cast<USHORT>(anCount), 0, adCount))
    {
        return false;
    }

    for (ULONG i = 0; i < arrQuestions.Count(); i++)
    {
        DnsQuestionRecord& record = *(arrQuestions[i].DownCast<DnsQuestionRecord>());

        if (!record.Serialize(writer))
        {
            return false;
        }
    }

    for (ULONG i = 0; i < arrAnswers.Count(); i++)
    {
        IDnsRecord::SPtr spRecord = arrAnswers[i];

        switch (spRecord->Type())
        {
        case DnsRecordTypeA:
        {
            DnsAnswerRecordA& arec = *spRecord.DownCast<DnsAnswerRecordA>();
            if (!arec.Serialize(writer, ttl))
            {
                return false;
            }
        }
        break;

        case DnsRecordTypeSrv:
        {
            DnsAnswerRecordSrv& srvrec = *spRecord.DownCast<DnsAnswerRecordSrv>();
            if (!srvrec.Serialize(writer, ttl))
            {
                return false;
            }
        }
        break;

        case DnsRecordTypeTxt:
        {
            DnsAnswerRecordTxt& txtrec = *spRecord.DownCast<DnsAnswerRecordTxt>();
            if (!txtrec.Serialize(writer, ttl))
            {
                return false;
            }
        }
        break;
        }
    }

    //
    // Additional OPT record to increase the UDP message size
    //
    KString::SPtr spEmptyString;
    KString::Create(/*out*/spEmptyString, GetThisAllocator());
    DnsQuestionRecord::SPtr spEmptyQuestion;
    DnsQuestionRecord::Create(/*out*/spEmptyQuestion, GetThisAllocator(), DnsRecordTypeOpt, *spEmptyString, encodingType);

    DnsAnswerRecordOpt::SPtr spRecordOpt;
    DnsAnswerRecordOpt::Create(/*out*/spRecordOpt, GetThisAllocator(), *spEmptyQuestion, (USHORT)buffer.QuerySize());
    if (!spRecordOpt->Serialize(writer))
    {
        return false;
    }

    numberOfBytesWritten = writer.GetWritePosition();

    return true;
}

IDnsMessage::SPtr DnsParser::CreateQuestionMessage(
    __in USHORT id,
    __in KString& strQuestion,
    __in DnsRecordType type,
    __in DnsTextEncodingType encodingType
) const
{
    USHORT flags = 0;
    DnsFlags::SetFlag(/*out*/flags, DnsFlags::RECURSION_DESIRED);

    DnsMessage::SPtr spInternal;
    DnsMessage::Create(/*out*/spInternal, GetThisAllocator(), id, flags);

    DnsQuestionRecord::SPtr spRecord;
    DnsQuestionRecord::Create(/*out*/spRecord, GetThisAllocator(), type, strQuestion, encodingType);
    spInternal->Questions().Append(spRecord.RawPtr());

    IDnsMessage::SPtr spMessage = spInternal.RawPtr();

    return spMessage;
}

IDnsRecord::SPtr DnsParser::CreateRecordA(
    __in IDnsRecord& question,
    __in ULONG address
) const
{
    DnsAnswerRecordA::SPtr spRecordInternal;
    DnsAnswerRecordA::Create(/*out*/spRecordInternal, GetThisAllocator(), question, address);
    IDnsRecord::SPtr spRecord = spRecordInternal.RawPtr();

    return spRecord;
}

IDnsRecord::SPtr DnsParser::CreateRecordTxt(
    __in IDnsRecord& question,
    __in KString& strText
) const
{
    DnsAnswerRecordTxt::SPtr spInternal;
    DnsAnswerRecordTxt::Create(/*out*/spInternal, GetThisAllocator(), question, strText);
    IDnsRecord::SPtr spRecord = spInternal.RawPtr();

    return spRecord;
}

IDnsRecord::SPtr DnsParser::CreateRecordSrv(
    __in IDnsRecord& question,
    __in KString& strHost,
    __in USHORT port
) const
{
    DnsAnswerRecordSrv::SPtr spInternal;
    DnsAnswerRecordSrv::Create(/*out*/spInternal, GetThisAllocator(), question, strHost, port);
    IDnsRecord::SPtr spRecord = spInternal.RawPtr();

    return spRecord;
}

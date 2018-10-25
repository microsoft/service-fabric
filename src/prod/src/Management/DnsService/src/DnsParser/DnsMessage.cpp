// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "DnsMessage.h"
#include "DnsAnswerRecordA.h"
#include "DnsAnswerRecordSrv.h"
#include "DnsAnswerRecordTxt.h"
#include "DnsQuestionRecord.h"

/*static*/
void DnsMessage::Create(
    __out DnsMessage::SPtr& spMessage,
    __in KAllocator& allocator,
    __in USHORT id,
    __in USHORT flags
)
{
    spMessage = _new(TAG, allocator) DnsMessage(id, flags);
    KInvariant(spMessage != nullptr);
}

DnsMessage::DnsMessage(
    __in USHORT id,
    __in USHORT flags
) : _id(id),
_flags(flags),
_arrQuestions(GetThisAllocator()),
_arrAnswers(GetThisAllocator())
{
    _time = KDateTime::Now();
}

DnsMessage::~DnsMessage()
{
}

class AnswersForSingleQuestion :
    public KShared<AnswersForSingleQuestion>
{
    K_FORCE_SHARED(AnswersForSingleQuestion);

public:
    static void Create(
        __out AnswersForSingleQuestion::SPtr& spAnswers,
        __in KAllocator& allocator,
        __in IDnsRecord& question
    )
    {
        spAnswers = _new(TAG, allocator) AnswersForSingleQuestion(question);
        KInvariant(spAnswers != nullptr);
    }

private:
    AnswersForSingleQuestion(
        __in IDnsRecord& question
    );

public:
    IDnsRecord& Question;
    KDynString A;
    KDynString Srv;
    KDynString Txt;
};

AnswersForSingleQuestion::AnswersForSingleQuestion(
    __in IDnsRecord& question
) : Question(question),
    A(GetThisAllocator()),
    Srv(GetThisAllocator()),
    Txt(GetThisAllocator())
{
}

AnswersForSingleQuestion::~AnswersForSingleQuestion()
{
}

KString::SPtr DnsMessage::ToString() const
{
    KString::SPtr spOutStr = KString::Create(GetThisAllocator(), 2048);
    KInvariant(spOutStr != nullptr);

    KString& strOut = *spOutStr;

    KDuration duration = KDateTime::Now() - _time;
    WCHAR temp[256];
#if !defined(PLATFORM_UNIX)    
    _snwprintf_s(temp, ARRAYSIZE(temp), L"ID( 0x%x ) RC( %d ) Flags ( %x ) Duration ( %d ms ) ", _id, DnsFlags::GetResponseCode(_flags), _flags, (LONG)duration.Milliseconds());
#else
    swprintf(temp, ARRAYSIZE(temp), L"ID( 0x%x ) RC( %d ) Flags ( %x ) Duration ( %d ms ) ", _id, DnsFlags::GetResponseCode(_flags), _flags, (LONG)duration.Milliseconds());
#endif    
    strOut.Concat(temp);

    KHashTable<KString::SPtr, AnswersForSingleQuestion::SPtr> ht(256, K_DefaultHashFunction, CompareKString, GetThisAllocator());
    for (ULONG i = 0; i < _arrQuestions.Count(); i++)
    {
        DnsQuestionRecord& record = static_cast<DnsQuestionRecord&>(*_arrQuestions[i]);
        KString::SPtr spNameStr(&record.Name());
        AnswersForSingleQuestion::SPtr spAnswers;
        AnswersForSingleQuestion::Create(/*out*/spAnswers, GetThisAllocator(), record);
        NTSTATUS status = ht.Put(spNameStr, spAnswers);
        if (status != STATUS_SUCCESS && status != STATUS_OBJECT_NAME_EXISTS)
        {
            KInvariant(false);
        }
    }

    for (ULONG i = 0; i < _arrAnswers.Count(); i++)
    {
        IDnsRecord& record = *_arrAnswers[i];

        KString::SPtr spNameStr(&record.Name());
        AnswersForSingleQuestion::SPtr spAnswers;
        if (STATUS_SUCCESS != ht.Get(spNameStr, /*out*/spAnswers))
        {
            continue;
        }

        AnswersForSingleQuestion& answers = *spAnswers;
        switch (record.Type())
        {
        case DnsRecordTypeA:
        {
            DnsAnswerRecordA& arec = (DnsAnswerRecordA&)record;        
            IN_ADDR inaddr;
#if !defined(PLATFORM_UNIX)               
            inaddr.S_un.S_addr = htonl(arec.Address());
            InetNtop(AF_INET, &inaddr, temp, ARRAYSIZE(temp));
#else
            inaddr.s_addr = htonl(arec.Address());

            CHAR tempAscii[ARRAYSIZE(temp)];
            inet_ntop(AF_INET, &inaddr, tempAscii, ARRAYSIZE(tempAscii));

            for (ULONG i = 0; i < ARRAYSIZE(temp); i++)
            {
                temp[i] = WCHAR(tempAscii[i]);
            }
#endif
        
            answers.A.Concat(L" ");
            answers.A.Concat(temp);
            break;
        }

        case DnsRecordTypeSrv:
        {
            DnsAnswerRecordSrv& srvrec = (DnsAnswerRecordSrv&)record;
            answers.Srv.Concat(L" ");
            answers.Srv.Concat(srvrec.Host());
#if !defined(PLATFORM_UNIX)                
            _snwprintf_s(temp, ARRAYSIZE(temp), L":%d", srvrec.Port());
#else
            swprintf(temp, ARRAYSIZE(temp), L":%d", srvrec.Port());
#endif    
            answers.Srv.Concat(temp);
            break;
        }

        case DnsRecordTypeTxt:
        {
            DnsAnswerRecordTxt& txtrec = (DnsAnswerRecordTxt&)record;
            answers.Txt.Concat(L" ");
            answers.Txt.Concat(txtrec.Text());
            break;
        }
        }
    }

    KString::SPtr spQuestionStr;
    AnswersForSingleQuestion::SPtr spAnswers;
    ht.Reset();
    while (ht.Next(/*out*/spQuestionStr, /*out*/spAnswers) == STATUS_SUCCESS)
    {
        AnswersForSingleQuestion& answers = *spAnswers;
        strOut.Concat(*spQuestionStr);
#if !defined(PLATFORM_UNIX)           
        _snwprintf_s(temp, ARRAYSIZE(temp), L" Type ( %d )", answers.Question.Type());
#else
        swprintf(temp, ARRAYSIZE(temp), L" Type ( %d )", answers.Question.Type());
#endif    
        strOut.Concat(temp);
        strOut.Concat(L" A(");
        strOut.Concat(answers.A);
        strOut.Concat(L" ) SRV(");
        strOut.Concat(answers.Srv);
        strOut.Concat(L" ) TXT(");
        strOut.Concat(answers.Txt);
        strOut.Concat(L" ) ");
    }

    return spOutStr;
}

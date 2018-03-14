// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "DnsAnswerRecordTxt.h"

/*static*/
void DnsAnswerRecordTxt::Create(
    __out DnsAnswerRecordTxt::SPtr& spRecord,
    __in KAllocator& allocator,
    __in IDnsRecord& question,
    __in KString& strText
)
{
    spRecord = _new(TAG, allocator) DnsAnswerRecordTxt(question, strText);
    KInvariant(spRecord != nullptr);
}

DnsAnswerRecordTxt::DnsAnswerRecordTxt(
    __in IDnsRecord& question,
    __in KString& strText
) : DnsAnswerRecordBase(DnsRecordTypeTxt, question),
_spText(&strText)
{
}

DnsAnswerRecordTxt::~DnsAnswerRecordTxt()
{
}

bool DnsAnswerRecordTxt::Serialize(
    __in BinaryWriter& writer,
    __in ULONG ttl
) const
{
    if (!__super::Serialize(writer, ttl))
    {
        return false;
    }

    KStringA::SPtr spUTF8 = KStringA::Create(GetThisAllocator(), Text().BufferSizeInChars());

    int numberOfChars = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS,
        static_cast<LPCWSTR>(Text()),  static_cast<int>(Text().Length()),
        static_cast<LPSTR>(*spUTF8), static_cast<int>(spUTF8->BufferSizeInChars() - 1),
        nullptr, nullptr
        );

    if (numberOfChars == 0)
    {
        return false;
    }

    spUTF8->SetLength(numberOfChars);
    spUTF8->SetNullTerminator();

    USHORT textLength = static_cast<USHORT>(spUTF8->Length());

    // Text is an array of 255 chars separated by a single byte (length)
    USHORT arraySize = textLength / 255;
    if (textLength % 255 > 0)
    {
        arraySize++;
    }

    USHORT rdLength = static_cast<USHORT>(textLength + arraySize);

    if (!writer.WriteUInt16(rdLength))
    {
        return false;
    }

    PVOID pBuffer = static_cast<PVOID>(*spUTF8);
    for (USHORT i = 0; i < arraySize; i++)
    {
        USHORT count = (i < arraySize - 1) ? 255 : (textLength % 255);
        if (!writer.WriteByte((char)count))
        {
            return false;
        }

        for (USHORT j = 0; j < count; j++)
        {
            if (!writer.WriteByte((char)static_cast<char*>(pBuffer)[i * 256 + j]))
            {
                return false;
            }
        }
    }

    return true;
}

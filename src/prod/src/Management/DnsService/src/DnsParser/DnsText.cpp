// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "DnsText.h"
#if defined(PLATFORM_UNIX)
#include "IDNConversionUtility.h"
#endif

const ULONG MAX_DNS_LABEL_SIZE = 63;
const ULONG MAX_DNS_NAME_SIZE = 255;

bool ReadDnsText(
    __out KStringA::SPtr& spText,
    __in KAllocator& allocator,
    __in BinaryReader& reader
);

bool WriteDnsLabel(
    __out KStringA::SPtr& spAsciiText,
    __in const KStringView& text,
    __in DnsTextEncodingType encodingType
);

bool DnsText::Skip(
    __in BinaryReader& reader
)
{
    char length = 1;
    while (length > 0)
    {
        if (!reader.ReadByte(/*out*/length))
        {
            return false;
        }

        // label has to start with 0 0 bits (length < 64)
        // pointer has to start with 1 1 bits (length >= 192)
        // combinations 0 1 and 1 0 are unused

        if (length > MAX_DNS_LABEL_SIZE)
        {
            // Compression offset is one byte in size
            if (!reader.Skip(sizeof(char)))
            {
                return false;
            }
        }
        else
        {
            if (!reader.Skip(length))
            {
                return false;
            }
        }
    }

    return true;
}

bool DnsText::Deserialize(
    __out KString::SPtr& spText,
    __out DnsTextEncodingType& encodingType,
    __in KAllocator& allocator,
    __in BinaryReader& reader
)
{
    encodingType = DnsTextEncodingTypeInvalid;

    KString::Create(/*out*/spText, allocator, MAX_DNS_NAME_SIZE + 1);
    spText->SetNullTerminator();

    char length;
    if (!reader.PeekByte(/*out*/length))
    {
        return false;
    }

    // label has to start with 0 0 bits (length < 64)
    // pointer has to start with 1 1 bits (length >= 192)
    // combinations 0 1 and 1 0 are unused
    // WE DON'T SUPPORT COMPRESSION POINTERS DUE TO SECURITY

    if (length > MAX_DNS_LABEL_SIZE)
    {
        return false;
    }

    KStringA::SPtr spAsciiText;
    if (!ReadDnsText(/*out*/spAsciiText, allocator, reader))
    {
        return false;
    }

    // ASCII can be IDN encoded (punycode)
    // Decode it here.
    KString::SPtr spIdnText;
    KString::Create(/*out*/spIdnText, allocator, spAsciiText->Length());
    if (!spIdnText->CopyFromAnsi(*spAsciiText, spAsciiText->Length()))
    {
        return false;
    }

    int numberOfCharsReceived = 0;

    DWORD flags = 0;
    numberOfCharsReceived = IdnToUnicode(
        flags,
        *spIdnText, spIdnText->Length(),
        *spText, spText->BufferSizeInChars() - 1
    );

    DWORD dwError = 0;
    if (numberOfCharsReceived == 0)
    {
        dwError = GetLastError();

        // Try UTF-8
        numberOfCharsReceived = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
            *spAsciiText, spAsciiText->Length(),
            *spText, spText->BufferSizeInChars() - 1);

        if (numberOfCharsReceived != 0)
        {
            encodingType = DnsTextEncodingTypeUTF8;
        }
        else
        {
            dwError = GetLastError();
            return false;
        }
    }
    else
    {
        encodingType = DnsTextEncodingTypeIDN;
    }

    spText->SetLength(static_cast<ULONG>(numberOfCharsReceived));
    if (!spText->SetNullTerminator())
    {
        return false;
    }

    return true;
}

bool DnsText::Serialize(
    __out KStringA::SPtr& spResult,
    __in KAllocator& allocator,
    __in const KStringView& text,
    __in DnsTextEncodingType encodingType
)
{
    KStringA::Create(/*out*/spResult, allocator);

    if (!text.IsEmpty())
    {
        LPCWSTR pBuffer = static_cast<LPCWSTR>(text);
        KStringView strDot(L".");

        ULONG prevPos = 0;
        ULONG pos = 0;
        while (text.Search(strDot, /*out*/pos, prevPos))
        {
            const ULONG length = pos - prevPos;
            KStringView strView(PWCHAR(pBuffer + prevPos), length, length);

            if (!WriteDnsLabel(/*out*/spResult, strView, encodingType))
            {
                return false;
            }

            prevPos = pos + 1;
        }

        if (prevPos < text.Length())
        {
            const ULONG length = text.Length() - prevPos;
            KStringView strView(PWCHAR(pBuffer + prevPos), length, length);

            if (!WriteDnsLabel(/*out*/spResult, strView, encodingType))
            {
                return false;
            }
        }
    }

    if (!spResult->AppendChar((char)(0)))
    {
        return false;
    }

    return true;
}

bool ReadDnsText(
    __out KStringA::SPtr& spAsciiText,
    __in KAllocator& allocator,
    __in BinaryReader& reader
)
{
    char length;
    if (!reader.ReadByte(/*out*/length))
    {
        return false;
    }

    KStringA::Create(/*out*/spAsciiText, allocator);

    while (length > 0)
    {
        if (spAsciiText->Length() > 0)
        {
            if (!spAsciiText->AppendChar('.'))
            {
                return false;
            }
        }

        KBuffer::SPtr spBuffer;
        KBuffer::Create(length, /*out*/spBuffer, allocator);

        if (!reader.ReadBytes(/*out*/*spBuffer, spBuffer->QuerySize()))
        {
            return false;
        }

        KStringViewA strView(
            static_cast<char*>(spBuffer->GetBuffer()),
            spBuffer->QuerySize(),
            spBuffer->QuerySize()
        );

        if (!spAsciiText->Concat(strView))
        {
            return false;
        }

        if (!reader.ReadByte(/*out*/length))
        {
            return false;
        }
    }

    return true;
}

bool WriteDnsLabel(
    __out KStringA::SPtr& spAsciiText,
    __in const KStringView& text,
    __in DnsTextEncodingType encodingType
)
{
    if (text.Length() == 0)
    {
        if (!spAsciiText->AppendChar((char)0))
        {
            return false;
        }

        return true;
    }

    // Convert the label from IDN to ACSII
    if (encodingType == DnsTextEncodingTypeIDN)
    {
        WCHAR wszAsciiLabel[MAX_DNS_LABEL_SIZE];

        DWORD flags = 0;
        int numberOfChars = 0;
        numberOfChars = IdnToAscii(flags,
            text, text.Length(),
            wszAsciiLabel, ARRAYSIZE(wszAsciiLabel)
        );

        if (0 == numberOfChars)
        {
            return false;
        }

        if (!spAsciiText->AppendChar((char)numberOfChars))
        {
            return false;
        }

        for (int i = 0; i < numberOfChars; i++)
        {
            if (!spAsciiText->AppendChar((char)wszAsciiLabel[i]))
            {
                return false;
            }
        }
    }
    else if (encodingType == DnsTextEncodingTypeUTF8)
    {
        char szAsciiLabel[MAX_DNS_LABEL_SIZE];

        int numberOfChars = WideCharToMultiByte(
            CP_UTF8, WC_ERR_INVALID_CHARS,
            text, text.Length(),
            szAsciiLabel, ARRAYSIZE(szAsciiLabel),
            nullptr, nullptr 
        );

        if (0 == numberOfChars)
        {
            return false;
        }

        if (!spAsciiText->AppendChar((char)numberOfChars))
        {
            return false;
        }

        for (int i = 0; i < numberOfChars; i++)
        {
            if (!spAsciiText->AppendChar(szAsciiLabel[i]))
            {
                return false;
            }
        }
    }

    return true;
}

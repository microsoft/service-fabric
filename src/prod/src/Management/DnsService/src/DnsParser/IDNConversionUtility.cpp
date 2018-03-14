// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#if defined(PLATFORM_UNIX)
#include "stdafx.h"
#define NULL nullptr
#include "unicode/idna.h"
#include "IDNConversionUtility.h"

void WideCharToChar(
    __in_ecount(length) LPCWSTR wideCharInput,
    __in int length,
    __out LPSTR output
);

void UnsignedcharToWideChar(
    __in_ecount(length) UChar* input,
    __in int length,
    __out LPWSTR wideCharOutput
);

bool IsUTF8(
    __in_ecount(inputSize) LPCWSTR input,
    __in int inputSize
);

int DNS::IdnToAscii(
    __in unsigned long dwFlags,
    __in_ecount(unicodeLength) LPCWSTR unicodestr,
    __in int unicodeLength,
    __out_ecount(asciisize) LPWSTR asciistr,
    __in int asciisize
)
{
    //pre-processing check
    //if the input is empty, the output buffer is NULL without the outputsize set to 0
    //or if output buffer size is negative, the function returns 0 without processing
    if (unicodeLength == 0 || 
        unicodestr == NULL || 
        (asciistr == NULL && asciisize > 0) ||
        asciisize < 0)
    {
        return 0;
    }
    
    bool nullTerminated = false;
    //the input should be considered nullterminated if unicodeLength == -1
    //if the input string is null terminated, the output string should be too.
    if (unicodeLength == -1)
    {
        nullTerminated = true;
        int i = 0;
        while (unicodestr[i] != L'\0')
        {
            i++;
        }
        unicodeLength = i;
        
    }
    else if (unicodestr[unicodeLength - 1] == L'\0')
    {
        nullTerminated = true;
        unicodeLength--;
    }

    int numberOfChars = 0;
    //fromUTF32 function only accepts int* as 32-bit input
    int char32bit[unicodeLength];

    for (int i = 0; i < unicodeLength; i++)
    {
        char32bit[i] = (int) unicodestr[i];
    }

    //the constructors with 16 bit characters are not available in this library
    //so this a way to make UnicodeString from widechar input
    UnicodeString unicodeInput = UnicodeString::fromUTF32(char32bit, unicodeLength);
    UnicodeString asciiOutput;
    IDNAInfo info;
    UErrorCode error = U_ZERO_ERROR;
    IDNA* converter = IDNA::createUTS46Instance(0, error);
    KInvariant(converter != NULL);
    converter->nameToASCII(unicodeInput, asciiOutput, info, error);
    //converter needs to be deleted after use according to the documentation
    //as createUTS46Instance uses new to allocate the object
    delete converter;
    
    if (!info.hasErrors()) 
    {
        numberOfChars = asciiOutput.length();
        numberOfChars = nullTerminated ? numberOfChars + 1 : numberOfChars;
        //return 0 if the output buffer size is smaller than required
        //and the output buffer size is not set to 0 (in which case, just return the required size)
        if (numberOfChars > asciisize && asciisize != 0)
        {
            numberOfChars = 0;
        }
        else if (asciistr != NULL)
        {
            UnsignedcharToWideChar((UChar*) asciiOutput.getTerminatedBuffer(), numberOfChars, asciistr);
            if (nullTerminated) 
            {
                asciistr[numberOfChars] = '\0';
            }
        }
    }
    else
    {
        numberOfChars = 0;
    }

    return numberOfChars;
}

int DNS::IdnToUnicode (
    __in unsigned long dwFlags,
    __in_ecount(cchASCIIChar) LPCWSTR lpASCIICharStr,
    __in int cchASCIIChar,
    __out_ecount(cchUnicodeChar) LPWSTR lpUnicodeCharStr,
    __in int cchUnicodeChar
)
{
    //pre-processing check
    //if the input is empty, the output buffer is NULL without the outputsize set to 0
    //or if output buffer size is negative, the function returns 0 without processing
    if (cchASCIIChar == 0 || 
        lpASCIICharStr == NULL || 
        (lpUnicodeCharStr == NULL && cchUnicodeChar != 0) ||
        cchUnicodeChar < 0)
    {
        return 0;
    }
    bool nullTerminated = false;
    //the input should be considered nullterminated if unicodeLength == -1
    //if the input string is null terminated, the output string should be too.
    if (cchASCIIChar == -1)
    {
        nullTerminated = true;
        int i = 0;
        while (lpASCIICharStr[i] != L'\0') 
        {
            i++;
        }
        cchASCIIChar = i;
    }
    if (lpASCIICharStr[cchASCIIChar - 1] == L'\0') 
    {
        nullTerminated = true;
        cchASCIIChar--;
    }

    char charstr[cchASCIIChar];
    int numberOfCharsReceived = 0;

    WideCharToChar (lpASCIICharStr, cchASCIIChar, charstr);

    //check if input is UTF-8, as windows' version of this function
    //does not convert UTF-8 to unicode this function will not either.
    if (IsUTF8 (lpASCIICharStr, cchASCIIChar)) 
    {
        return numberOfCharsReceived;
    }

    UnicodeString asciiInput(charstr, cchASCIIChar);
    UnicodeString unicodeOutput;
    IDNAInfo info;
    UErrorCode error = U_ZERO_ERROR;
    IDNA* converter = IDNA::createUTS46Instance(0, error);
    unicodeOutput =  converter->nameToUnicode(asciiInput, unicodeOutput, info, error);
    KInvariant(converter != NULL);
    //converter needs to be deleted after use according to the documentation
    //as createUTS46Instance uses new to allocate the object
    delete converter;
    
    if (!info.hasErrors()) 
    {
        numberOfCharsReceived = unicodeOutput.length();
        numberOfCharsReceived = nullTerminated ? numberOfCharsReceived + 1 : numberOfCharsReceived;
        //return 0 if the output buffer size is smaller than required
        //and the output buffer size is not set to 0 (in which case, just return the required size)
        if (numberOfCharsReceived > cchUnicodeChar && cchUnicodeChar != 0)
        {
            numberOfCharsReceived = 0;           
        }
        else if (lpUnicodeCharStr != NULL) 
        {
            UnsignedcharToWideChar((UChar*) unicodeOutput.getTerminatedBuffer(), numberOfCharsReceived, lpUnicodeCharStr);

            if (nullTerminated) 
            {
                lpUnicodeCharStr[numberOfCharsReceived] = L'\0';
            }
        }
    } 
    else 
    {
        numberOfCharsReceived = 0;
    }

    return numberOfCharsReceived;
}

void WideCharToChar(
    __in_ecount(length) LPCWSTR wideCharInput,
    __in int length,
    __out LPSTR output
)
{
    for (int i = 0; i < length; i++) 
    {
        output[i] = (char)wideCharInput[i];
    }

}

void UnsignedcharToWideChar(
    __in_ecount(length) UChar* input,
    __in int length,
    __out LPWSTR wideCharOutput
)
{
    for (int i = 0; i < length; i++) 
    {
        wideCharOutput[i] = (wchar_t)input[i];
    }

}

bool IsUTF8(
    __in_ecount(inputSize) LPCWSTR input,
    __in int inputSize
)
{
    for (int i = 0; i < inputSize; i++) 
    {
        if (input[i] & 128) 
        {
            return true;
        }
    }
    return false;
}
#endif

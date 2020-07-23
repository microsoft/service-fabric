// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Base64Encoder.h"

const char * Base64Encoder::Base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const char Base64Encoder::paddingChar = '=';
std::wstring_convert<std::codecvt_utf8<wchar_t>> Base64Encoder::utf8Converter;

std::string Base64Encoder::ConvertToUTF8AndEncode(std::wstring const & inputString)
{
    return Base64Encoder::Encode(Base64Encoder::utf8Converter.to_bytes(inputString));
}

std::wstring Base64Encoder::ConvertToUTF8AndEncodeW(std::wstring const & inputString)
{
    return Base64Encoder::utf8Converter.from_bytes(Base64Encoder::Encode(Base64Encoder::utf8Converter.to_bytes(inputString)));
}

std::string Base64Encoder::Encode(std::string const & inputString)
{
    size_t inputStringLength = inputString.length();
    size_t outputStringLength = (inputStringLength / 3 * 4) + ((inputStringLength % 3) > 0 ? 4 : 0);
    std::string outputString = std::string(outputStringLength, 0);

    size_t outputStringIndex = 0;
    size_t inputStringIndex = 0;

    // Process next 3 input characters in each iteration to generate 4 output characters.
    while (inputStringIndex < inputStringLength)
    {
        if ((outputStringLength - outputStringIndex) < 4)
        {
            CODING_ASSERT(
                "Coding error in Base64 Encoding logic. InputString length: {0}, OutputString length: {1} InputString Index: {2} OutputString Index: {3}"
                , inputStringLength
                , outputStringLength
                , inputStringIndex
                , outputStringIndex
            );
        }

        int encodeCharLen = 0;

        BYTE b1 = inputString[inputStringIndex + encodeCharLen++];
        BYTE b2 = ((inputStringIndex + encodeCharLen) < inputStringLength) ? inputString[inputStringIndex + encodeCharLen++] : 0;
        BYTE b3 = ((inputStringIndex + encodeCharLen) < inputStringLength) ? inputString[inputStringIndex + encodeCharLen++] : 0;

        inputStringIndex += encodeCharLen;

        outputString[outputStringIndex++] = Base64Chars[(b1 >> 2) & 0x3F];
        outputString[outputStringIndex++] = Base64Chars[(((b1 & 0x3) << 4) | ((b2 >> 4) & 0xF)) & 0x3F];
        outputString[outputStringIndex++] = Base64Chars[(((b2 & 0xF) << 2) | ((b3 >> 6) & 0x3)) & 0x3F];
        outputString[outputStringIndex++] = Base64Chars[b3 & 0x3F];

        if (encodeCharLen < 3)
        {
            outputString[outputStringIndex - 1] = Base64Encoder::paddingChar;

            if (encodeCharLen < 2)
            {
                outputString[outputStringIndex - 2] = Base64Encoder::paddingChar;
            }
        }
    }
    return outputString;
}

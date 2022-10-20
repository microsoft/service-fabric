// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
class Base64Encoder
{
public:
    static std::string Encode(std::string const & inputString);
    static std::string ConvertToUTF8AndEncode(std::wstring const & inputString);
    static std::wstring ConvertToUTF8AndEncodeW(std::wstring const & inputString);
private:
    static const char *Base64Chars;
    static std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8Converter;
    static const char paddingChar;
};


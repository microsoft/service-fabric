// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "pal_string_util.h"
#include <codecvt>
#include <locale>

using namespace std;
using namespace Pal;

wstring Pal::utf8to16(const char *str)
{
    wstring_convert<codecvt_utf8_utf16<char16_t>, char16_t> conv;
    u16string u16str = conv.from_bytes(str);
    basic_string<wchar_t> result;
    for(int index = 0; index < u16str.length(); index++)
    {
        result.push_back((wchar_t)u16str[index]);
    }
    return result;
}

string Pal::utf16to8(const wchar_t *wstr)
{
    wstring_convert<codecvt_utf8_utf16<char16_t>, char16_t> conv;
    return conv.to_bytes((const char16_t *) wstr);
}

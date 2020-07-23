// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <iostream>
#include <iomanip>

using namespace std;
using namespace Common;
using namespace Hosting2;

bool MACHelper::TryParse(const std::wstring & macString, uint64 & mac)
{
    uint64 macAddressParts[6];

    vector<wstring> macAddressStringParts;
    StringUtility::Split<wstring>(macString, macAddressStringParts, L"-");
    if (macAddressStringParts.size() != 6)
    {
        return false;
    }

    if (StringUtility::TryFromWString<uint64>(macAddressStringParts[0], macAddressParts[0], 16) &&
        StringUtility::TryFromWString<uint64>(macAddressStringParts[1], macAddressParts[1], 16) &&
        StringUtility::TryFromWString<uint64>(macAddressStringParts[2], macAddressParts[2], 16) &&
        StringUtility::TryFromWString<uint64>(macAddressStringParts[3], macAddressParts[3], 16) &&
        StringUtility::TryFromWString<uint64>(macAddressStringParts[4], macAddressParts[4], 16) &&
        StringUtility::TryFromWString<uint64>(macAddressStringParts[5], macAddressParts[5], 16))
    {
        if (macAddressParts[0] > 255 || 
            macAddressParts[1] > 255 ||
            macAddressParts[2] > 255 || 
            macAddressParts[3] > 255 ||
            macAddressParts[4] > 255 || 
            macAddressParts[5] > 255)
        {
            return false;
        }
    }

    mac = (macAddressParts[0]) << 40 |
          (macAddressParts[1]) << 32 |
          (macAddressParts[2]) << 24 |
          (macAddressParts[3]) << 16 |
          (macAddressParts[4]) << 8 |
          (macAddressParts[5]);

    return true;
}

static const FormatOptions hexByteFormat(2, true, "x");

std::wstring MACHelper::Format(uint64 mac)
{
    wstring macStr;
    StringWriter writer(macStr);

    for (int i = 40; i >= 0; i -= 8)
    {
        uint64 byteToWrite = (mac >> i) & 0xff;
        writer.WriteNumber(byteToWrite, hexByteFormat, false);
        if (i > 0) writer.WriteChar(L'-');
    }

    return macStr;
}

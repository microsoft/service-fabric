// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

bool IPHelper::TryParse(const wstring &ipString, UINT &ip)
{
    UINT dig1, dig2, dig3, dig4;

    vector<wstring> ipAddressParts;
    StringUtility::Split<wstring>(ipString, ipAddressParts, L".");
    if (ipAddressParts.size() != 4)
    {
        return false;
    }

    if (StringUtility::TryFromWString<UINT>(ipAddressParts[0], dig1) &&
        StringUtility::TryFromWString<UINT>(ipAddressParts[1], dig2) &&
        StringUtility::TryFromWString<UINT>(ipAddressParts[2], dig3) &&
        StringUtility::TryFromWString<UINT>(ipAddressParts[3], dig4))
    {
        if (dig1 > 255 || dig2 > 255 || dig3 > 255 || dig4 > 255)
        {
            return false;
        }

        ip = (dig1 <<= 24) | (dig2 <<= 16) | (dig3 <<= 8) | dig4;
    }
    else
    {
        return false;
    }

    return true;
}

wstring IPHelper::Format(UINT ip)
{
    return wformatString(L"{0}.{1}.{2}.{3}",
        (ip >> 24) & 0xFF,
        (ip >> 16) & 0xFF,
        (ip >> 8) & 0xFF,
        ip & 0xFF);
}

bool IPHelper::TryParseCIDR(const wstring &cidr, UINT &baseIp, int &maskNum)
{
    UINT dig1, dig2, dig3, dig4;

    // split cidr format string into base ip and mask
    vector<wstring> segments;
    StringUtility::Split<wstring>(cidr, segments, L"//");
    if (segments.size() != 2)
    {
        return false;
    }

    if (!StringUtility::TryFromWString<int>(segments[1], maskNum))
    {
        return false;
    }

    vector<wstring> ipAddressParts;
    StringUtility::Split<wstring>(segments[0], ipAddressParts, L".");
    if (ipAddressParts.size() != 4)
    {
        return false;
    }

    if (StringUtility::TryFromWString<UINT>(ipAddressParts[0], dig1) &&
        StringUtility::TryFromWString<UINT>(ipAddressParts[1], dig2) &&
        StringUtility::TryFromWString<UINT>(ipAddressParts[2], dig3) &&
        StringUtility::TryFromWString<UINT>(ipAddressParts[3], dig4))
    {
        if (dig1 > 255 || dig2 > 255 || dig3 > 255 || dig4 > 255)
        {
            return false;
        }

        baseIp = (dig1 <<= 24) | (dig2 <<= 16) | (dig3 <<= 8) | dig4;
    }
    else
    {
        return false;
    }

    return true;
}

wstring IPHelper::FormatCIDR(UINT ip, int maskNum)
{
    return wformatString(L"{0} / {1}", Format(ip), maskNum);
}

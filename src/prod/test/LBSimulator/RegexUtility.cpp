// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "RegexUtility.h"

using namespace std;
using namespace Common;
using namespace LBSimulator;

wstring const RegexUtility::KeyValueDelimiter = L":";


wstring RegexUtility::ExtractStringByKey(wstring const & text, wstring key)
{
    wstring value;
    wstring regexString = key + L":([^\\s]+)";
    wregex rgx(regexString, std::regex::ECMAScript);
    wsmatch match;
    if (regex_search(text, match, rgx) && match.size() > 1)
    {
        value = match.str(1);
        return value;
    }
    else
    {
        Assert::CodingError("Value cannot be extracted into a wstring from {0} by key {1}", text.c_str(), key.c_str());
    }
}

bool RegexUtility::ExtractBoolByKey(std::wstring const & text, std::wstring key)
{
    wstring stringValue = RegexUtility::ExtractStringByKey(text, key);
     
    if (!stringValue.compare(L"true"))
    {
        return true;
    }
    else if (!stringValue.compare(L"false"))
    {
        return false;
    }

    Assert::CodingError("Value cannot be extracted into a bool from {0} by key {1}", text.c_str(), key.c_str());
}

wstring RegexUtility::ExtractStringInBetween(wstring const & text, wstring string1, wstring string2, bool gready)
{
    wstring value;
    wregex rgx;
    if (gready == true)
    {
        rgx = wregex(string1 + L"(.*)" + string2);
    }
    else
    {
        rgx = wregex(string1 + L"(.*?)" + string2);
    }

    wsmatch match;
    if (regex_search(text, match, rgx) && match.size() > 1)
    {
        value = match.str(1);
        return value;
    }
    else
    {
        Assert::CodingError("Value cannot be extracted into a wstring: {0}", text.c_str());
    }
}

bool RegexUtility::ExtractBoolInBetween(std::wstring const & text, std::wstring string1, std::wstring string2, bool gready)
{
    wstring stringValue = RegexUtility::ExtractStringInBetween(text, string1, string2, gready);
 
    if (!stringValue.compare(L"true"))
    {
        return true;
    }
    else if (!stringValue.compare(L"false"))
    {
        return false;
    }
    else
    {
        Assert::CodingError("Value cannot be extracted into a bool: {0}", text.c_str());
    }
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;

void ContainerInfoType::WriteToTextWriter(Common::TextWriter & w, Enum const & val)
{
    switch (val)
    {
        case Invalid:
            w << L"Invalid";
            return;
        case Logs:
            w << L"Logs";
            return;
        case Stats:
            w << L"Stats";
            return;
        case Events:
            w << L"Events";
            return;
        case Inspect:
            w << L"Inspect";
            return;
        case RawAPI:
            w << L"RawAPI";
            return;
        default:
            w << Common::wformatString("Unknown ContainerInfoType value {0}", static_cast<int>(val));
    }
}
        
bool ContainerInfoType::FromWString(std::wstring const & string, __out Enum & val)
{
    if(Common::StringUtility::AreEqualCaseInsensitive(string, L"Invalid"))
    {
        val = Invalid;
    }
    else if(Common::StringUtility::AreEqualCaseInsensitive(string, L"Logs"))
    {
        val = Logs;
    }
    else if(Common::StringUtility::AreEqualCaseInsensitive(string, L"Stats"))
    {
        val = Stats;
    }
    else if(Common::StringUtility::AreEqualCaseInsensitive(string, L"Events"))
    {
        val = Events;
    }
    else if(Common::StringUtility::AreEqualCaseInsensitive(string, L"Inspect"))
    {
        val = Inspect;
    }
    else if (Common::StringUtility::AreEqualCaseInsensitive(string, L"RawAPI"))
    {
        val = RawAPI;
    }
    else
    {
        return false;
    }
            
    return true;
}

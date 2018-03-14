// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

void ConfigEntryUpgradePolicy::WriteToTextWriter(TextWriter & w, Enum const & val)
{
    switch (val)
    {
    case Dynamic: 
        w << L"Dynamic";
        return;
    case Static: 
        w << L"Static";
        return;
    case NotAllowed: 
        w << L"NotAllowed";        
        return;
    case SingleChange:
        w << L"SingleChange";
        return;
    default: 
        Assert::CodingError("Unknown UpgradePolicy");
    }
}

wstring ConfigEntryUpgradePolicy::ToString(ConfigEntryUpgradePolicy::Enum const & val)
{
    wstring upgradePolicy;
    StringWriter(upgradePolicy).Write(val);
    return upgradePolicy;
}

ConfigEntryUpgradePolicy::Enum ConfigEntryUpgradePolicy::FromString(wstring const & val)
{
    if(StringUtility::AreEqualCaseInsensitive(val, L"Dynamic"))
    {
        return ConfigEntryUpgradePolicy::Dynamic;
    }
    else if(StringUtility::AreEqualCaseInsensitive(val, L"Static"))
    {
        return ConfigEntryUpgradePolicy::Static;
    }
    else if(StringUtility::AreEqualCaseInsensitive(val, L"NotAllowed"))
    {
        return ConfigEntryUpgradePolicy::NotAllowed;
    }
    else if (StringUtility::AreEqualCaseInsensitive(val, L"SingleChange"))
    {
        return ConfigEntryUpgradePolicy::SingleChange;
    }
    else
    {
        Assert::CodingError("Unknown UpgradePolicy: {0}", val);
    }    
}

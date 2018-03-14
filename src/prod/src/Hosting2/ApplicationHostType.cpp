// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

void ApplicationHostType::WriteToTextWriter(TextWriter & w, Enum const & val)
{
    w << ToString(val);
}

wstring ApplicationHostType::ToString(Enum const & val)
{
    switch(val)
    {
    case ApplicationHostType::NonActivated:
        return L"NonActivated";
    case ApplicationHostType::Activated_SingleCodePackage:
        return L"Activated_SingleCodePackage";
    case ApplicationHostType::Activated_MultiCodePackage:
        return L"Activated_MultiCodePackage";
    case ApplicationHostType::Activated_InProcess:
        return L"Activated_InProcess";
    default:
        Assert::CodingError("Unknown ApplicationHostType value {0}", (int)val);
    }
}

ErrorCode ApplicationHostType::FromString(wstring const & str, __out Enum & val)
{
    if (StringUtility::AreEqualCaseInsensitive(str, L"NonActivated"))
    {
        val =  ApplicationHostType::NonActivated;
    } 
    else if (StringUtility::AreEqualCaseInsensitive(str, L"Activated_SingleCodePackage"))
    {
        val = ApplicationHostType::Activated_SingleCodePackage;
    }
    else if (StringUtility::AreEqualCaseInsensitive(str, L"Activated_MultiCodePackage"))
    {
        val = ApplicationHostType::Activated_MultiCodePackage;
    }
    else if (StringUtility::AreEqualCaseInsensitive(str, L"Activated_InProcess"))
    {
        val = ApplicationHostType::Activated_InProcess;
    }
    else 
    {
        return ErrorCode::FromHResult(E_INVALIDARG);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

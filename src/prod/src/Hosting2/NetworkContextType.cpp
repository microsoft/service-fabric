// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

wstring NetworkContextType::ToString(Enum const & val)
{
    switch(val)
    {
    case NetworkContextType::None:
        return L"None";
    case NetworkContextType::Flat:
        return L"Flat";
    case NetworkContextType::Overlay:
        return L"Overlay";
    default:
        Assert::CodingError("Unknown NetworkContextType value {0}", (int)val);
    }
}

ErrorCode NetworkContextType::FromString(wstring const & str, __out Enum & val)
{
    if (StringUtility::AreEqualCaseInsensitive(str, L"None"))
    {
        val = NetworkContextType::None;
    } 
    else if (StringUtility::AreEqualCaseInsensitive(str, L"Flat"))
    {
		val = NetworkContextType::Flat;
    }
    else if (StringUtility::AreEqualCaseInsensitive(str, L"Overlay"))
    {
        val = NetworkContextType::Overlay;
    } 
	else
    {
        return ErrorCode::FromHResult(E_INVALIDARG);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

void NetworkType::WriteToTextWriter(TextWriter & w, Enum const & val)
{
    switch (val)
    {
    case NetworkType::Open:
        w << L"Open";
        return;
    case NetworkType::Other:
        w << L"Other";
        return;

    default:
        Assert::CodingError("Unknown NetworkType value {0}", (int)val);
    }
}

ErrorCode NetworkType::FromString(wstring const & value, __out Enum & val)
{
    if (value == L"Open" || value == L"default")
    {
        val = NetworkType::Open;
        return ErrorCode(ErrorCodeValue::Success);
    } 
    else if (value == L"Other")
    {
        val = NetworkType::Other;
        return ErrorCode(ErrorCodeValue::Success);
    } 
    else
    {
        return ErrorCode::FromHResult(E_INVALIDARG);
    }
}

std::wstring NetworkType::EnumToString(
	ServiceModel::NetworkType::Enum val)
{
	switch (val)
	{
	case NetworkType::Open:
		return L"Open";
	case NetworkType::Other:
		return L"Other";
	default:
		Assert::CodingError("Unknown NetworkType value {0}", (int)val);
	}
}

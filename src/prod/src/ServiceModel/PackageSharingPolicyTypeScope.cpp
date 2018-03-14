// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

void PackageSharingPolicyTypeScope::WriteToTextWriter(TextWriter & w, Enum const & val)
{
    switch (val)
    {
	case PackageSharingPolicyTypeScope::None:
        w << L"None";
        return;
	case PackageSharingPolicyTypeScope::All:
        w << L"All";
        return;
	case PackageSharingPolicyTypeScope::Code:
        w << L"Code";
        return;
	case PackageSharingPolicyTypeScope::Config:
        w << L"Config";
        return;
	case PackageSharingPolicyTypeScope::Data:
        w << L"Data";
        return;
    default:
        Assert::CodingError("Unknown PackageSharingPolicyTypeScope value {0}", (int)val);
    }
}

ErrorCode PackageSharingPolicyTypeScope::FromString(wstring const & value, __out Enum & val)
{
    if (value == L"None")
    {
        val = PackageSharingPolicyTypeScope::None;
        return ErrorCode(ErrorCodeValue::Success);
    } 
    else if (value == L"All")
    {
        val = PackageSharingPolicyTypeScope::All;
        return ErrorCode(ErrorCodeValue::Success);
    } 
    else if (value == L"Code")
    {
        val = PackageSharingPolicyTypeScope::Code;
		return ErrorCode(ErrorCodeValue::Success);
    }
    else if (value == L"Config")
    {
        val = PackageSharingPolicyTypeScope::Config;
        return ErrorCode(ErrorCodeValue::Success);
    }
    else if (value == L"Data")
    {
        val = PackageSharingPolicyTypeScope::Data;
        return ErrorCode(ErrorCodeValue::Success);
    }
    else if (value == L"")
    {
        val = PackageSharingPolicyTypeScope::None;
        return ErrorCode(ErrorCodeValue::Success);
    }
    else
    {
        return ErrorCode::FromHResult(E_INVALIDARG);
    }
}

wstring PackageSharingPolicyTypeScope::EnumToString(PackageSharingPolicyTypeScope::Enum val)
{
	switch (val)
	{
	case PackageSharingPolicyTypeScope::None:
		return L"None";
	case PackageSharingPolicyTypeScope::All:
		return L"All";
	case PackageSharingPolicyTypeScope::Code:
		return L"Code";
	case PackageSharingPolicyTypeScope::Config:
		return L"Config";
	case PackageSharingPolicyTypeScope::Data:
		return L"Data";
	default:
		Assert::CodingError("Unknown PackageSharingPolicyTypeScope value {0}", (int)val);
	}
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

void RunAsPolicyTypeEntryPointType::WriteToTextWriter(TextWriter & w, Enum const & val)
{
    switch (val)
    {
    case RunAsPolicyTypeEntryPointType::Setup:
        w << L"Setup";
        return;
    case RunAsPolicyTypeEntryPointType::Main:
        w << L"Main";
        return;
    case RunAsPolicyTypeEntryPointType::All:
        w << L"All";
        return;
    default:
        Assert::CodingError("Unknown RunAsPolicyTypeEntryPointType value {0}", (int)val);
    }
}

ErrorCode RunAsPolicyTypeEntryPointType::FromString(wstring const & value, __out Enum & val)
{
    if (value == L"Setup")
    {
        val = RunAsPolicyTypeEntryPointType::Setup;
        return ErrorCode(ErrorCodeValue::Success);
    } 
    else if (value == L"Main")
    {
        val = RunAsPolicyTypeEntryPointType::Main;
        return ErrorCode(ErrorCodeValue::Success);
    } 
    else if (value == L"All")
    {
        val = RunAsPolicyTypeEntryPointType::All;
        return ErrorCode(ErrorCodeValue::Success);
    }
    else
    {
        return ErrorCode::FromHResult(E_INVALIDARG);
    }
}

std::wstring RunAsPolicyTypeEntryPointType::EnumToString(
	ServiceModel::RunAsPolicyTypeEntryPointType::Enum val)
{
	switch (val)
	{
	case RunAsPolicyTypeEntryPointType::Setup:
		return L"Setup";
	case RunAsPolicyTypeEntryPointType::Main:
		return L"Main";
	case RunAsPolicyTypeEntryPointType::All:
		return L"All";
	default:
		Assert::CodingError("Unknown RunAsPolicyTypeEntryPointType value {0}", (int)val);
	}
}

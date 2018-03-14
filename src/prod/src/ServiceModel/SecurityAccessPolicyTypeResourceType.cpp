// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

void SecurityAccessPolicyTypeResourceType::WriteToTextWriter(TextWriter & w, Enum const & val)
{
    switch (val)
    {
    case SecurityAccessPolicyTypeResourceType::Endpoint:
        w << L"Endpoint";
        return;
    case SecurityAccessPolicyTypeResourceType::Certificate:
        w << L"Certificate";
        return;
    default:
        w << L"Endpoint";
        return;
    }
}

ErrorCode SecurityAccessPolicyTypeResourceType::FromString(wstring const & value, __out Enum & val)
{
    if (value == L"Certificate")
    {
        val = SecurityAccessPolicyTypeResourceType::Certificate;
        return ErrorCode(ErrorCodeValue::Success);
    } 
    else if (value == L"Endpoint")
    {
        val = SecurityAccessPolicyTypeResourceType::Endpoint;
        return ErrorCode(ErrorCodeValue::Success);
    } 
    else
    {
        return ErrorCode::FromHResult(E_INVALIDARG);
    }
}

wstring SecurityAccessPolicyTypeResourceType::ToString(SecurityAccessPolicyTypeResourceType::Enum val)
{
	switch (val)
	{
	case SecurityAccessPolicyTypeResourceType::Endpoint:
		return L"Endpoint";
	case SecurityAccessPolicyTypeResourceType::Certificate:
		return L"Certificate";
	default:
		return L"Endpoint";
	}
}

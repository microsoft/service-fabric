// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

void ServicePackageSharingType::WriteToTextWriter(TextWriter & w, Enum const & val)
{
    switch (val)
    {
    case ServicePackageSharingType::None:
        w << L"None";
        return;
    case ServicePackageSharingType::All:
        w << L"All";
        return;
    case ServicePackageSharingType::Code:
        w << L"Code";
        return;
    case ServicePackageSharingType::Config:
        w << L"Config";
        return;
    case ServicePackageSharingType::Data:
        w << L"Data";
        return;
    default:
        Assert::CodingError("Unknown ServicePackageSharingType value {0}", (int)val);
    }
}

ErrorCode ServicePackageSharingType::FromString(wstring const & value, __out Enum & val)
{
    if (value == L"None")
    {
        val = ServicePackageSharingType::None;
        return ErrorCode(ErrorCodeValue::Success);
    } 
    else if (value == L"All")
    {
        val = ServicePackageSharingType::All;
        return ErrorCode(ErrorCodeValue::Success);
    } 
    else if (value == L"Code")
    {
        val = ServicePackageSharingType::Code;
        return ErrorCode(ErrorCodeValue::Success);
    }
    else if (value == L"Config")
    {
        val = ServicePackageSharingType::Config;
        return ErrorCode(ErrorCodeValue::Success);
    }
    else if (value == L"Data")
    {
        val = ServicePackageSharingType::Data;
        return ErrorCode(ErrorCodeValue::Success);
    }
    else if (value == L"")
    {
        val = ServicePackageSharingType::None;
        return ErrorCode(ErrorCodeValue::Success);
    }
    else
    {
        return ErrorCode::FromHResult(E_INVALIDARG);
    }
}

ErrorCode ServicePackageSharingType::FromPublic(FABRIC_PACKAGE_SHARING_POLICY_SCOPE value, __out Enum & val)
{
    if(value == FABRIC_PACKAGE_SHARING_POLICY_SCOPE_NONE)
    {
        val = ServicePackageSharingType::None;
        return ErrorCode(ErrorCodeValue::Success); 
    }
    else if(value == FABRIC_PACKAGE_SHARING_POLICY_SCOPE_ALL)
    {
        val = ServicePackageSharingType::All;
        return ErrorCode(ErrorCodeValue::Success);
    }
    else if (value == FABRIC_PACKAGE_SHARING_POLICY_SCOPE_CODE)
    {
        val = ServicePackageSharingType::Code;
        return ErrorCode(ErrorCodeValue::Success);
    }
    else if (value == FABRIC_PACKAGE_SHARING_POLICY_SCOPE_CONFIG)
    {
        val = ServicePackageSharingType::Config;
        return ErrorCode(ErrorCodeValue::Success);
    }
    else if (value == FABRIC_PACKAGE_SHARING_POLICY_SCOPE_DATA)
    {
        val = ServicePackageSharingType::Data;
        return ErrorCode(ErrorCodeValue::Success);
    }
    else
    {
        return ErrorCode::FromHResult(E_INVALIDARG);
    }
}

ErrorCode ServicePackageSharingType::ToPublic(Enum const & value, __out FABRIC_PACKAGE_SHARING_POLICY_SCOPE & val)
{
    switch (value)
    {
    case ServicePackageSharingType::None:
        val = FABRIC_PACKAGE_SHARING_POLICY_SCOPE_NONE;
        return ErrorCode(ErrorCodeValue::Success);
    case ServicePackageSharingType::All:
        val = FABRIC_PACKAGE_SHARING_POLICY_SCOPE_ALL;
        return ErrorCode(ErrorCodeValue::Success);;
    case ServicePackageSharingType::Code:
        val = FABRIC_PACKAGE_SHARING_POLICY_SCOPE_CODE;
        return ErrorCode(ErrorCodeValue::Success);;
    case ServicePackageSharingType::Config:
        val = FABRIC_PACKAGE_SHARING_POLICY_SCOPE_CONFIG;
        return ErrorCode(ErrorCodeValue::Success);;
    case ServicePackageSharingType::Data:
        val = FABRIC_PACKAGE_SHARING_POLICY_SCOPE_DATA;
        return ErrorCode(ErrorCodeValue::Success);;
    default:
        return ErrorCode::FromHResult(E_INVALIDARG);
    }
}

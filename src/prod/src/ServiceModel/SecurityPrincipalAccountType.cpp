// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

void SecurityPrincipalAccountType::WriteToTextWriter(TextWriter & w, Enum const & val)
{
    switch (val)
    {
    case SecurityPrincipalAccountType::LocalUser:
        w << L"LocalUser";
        return;
    case SecurityPrincipalAccountType::DomainUser:
        w << L"DomainUser";
        return;
    case SecurityPrincipalAccountType::NetworkService:
        w << L"NetworkService";
        return;
    case SecurityPrincipalAccountType::LocalService:
        w << L"LocalService";
        return;
    case SecurityPrincipalAccountType::ManagedServiceAccount:
        w << L"ManagedServiceAccount";
        return;
    case SecurityPrincipalAccountType::LocalSystem:
        w << L"LocalSystem";
        return;
    default:
        Assert::CodingError("Unknown SecurityPrincipalAccountType value {0}", (int)val);
    }
}

ErrorCode SecurityPrincipalAccountType::FromString(wstring const & value, __out Enum & val)
{
    if (value == L"LocalUser")
    {
        val = SecurityPrincipalAccountType::LocalUser;
        return ErrorCode(ErrorCodeValue::Success);
    }
    else if (value == L"DomainUser")
    {
        val = SecurityPrincipalAccountType::DomainUser;
        return ErrorCode(ErrorCodeValue::Success);
    }
    else if (value == L"NetworkService")
    {
        val = SecurityPrincipalAccountType::NetworkService;
        return ErrorCode(ErrorCodeValue::Success);
    }
    else if (value == L"LocalService")
    {
        val = SecurityPrincipalAccountType::LocalService;
        return ErrorCode(ErrorCodeValue::Success);
    }
    else if (value == L"ManagedServiceAccount")
    {
        val = SecurityPrincipalAccountType::ManagedServiceAccount;
        return ErrorCode(ErrorCodeValue::Success);
    }
    else if (value == L"LocalSystem")
    {
        val = SecurityPrincipalAccountType::LocalSystem;
        return ErrorCode(ErrorCodeValue::Success);
    }
    else
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(GET_FSS_RC(Invalid_SecurityPrincipalAccountType), value));
    }
}

wstring SecurityPrincipalAccountType::ToString(ServiceModel::SecurityPrincipalAccountType::Enum val)
{
    switch (val)
    {
    case SecurityPrincipalAccountType::LocalUser:
        return L"LocalUser";
    case SecurityPrincipalAccountType::DomainUser:
        return L"DomainUser";
    case SecurityPrincipalAccountType::NetworkService:
        return L"NetworkService";
    case SecurityPrincipalAccountType::LocalService:
        return L"LocalService";
    case SecurityPrincipalAccountType::ManagedServiceAccount:
        return L"ManagedServiceAccount";
    case SecurityPrincipalAccountType::LocalSystem:
        return L"LocalSystem";
    default:
        Assert::CodingError("Unknown SecurityPrincipalAccountType value {0}", (int)val);
    }
}

void SecurityPrincipalAccountType::Validate(ServiceModel::SecurityPrincipalAccountType::Enum val)
{
    switch (val)
    {
    case SecurityPrincipalAccountType::LocalUser:
    case SecurityPrincipalAccountType::DomainUser:
    case SecurityPrincipalAccountType::NetworkService:
    case SecurityPrincipalAccountType::LocalService:
    case SecurityPrincipalAccountType::ManagedServiceAccount:
    case SecurityPrincipalAccountType::LocalSystem:
        return;
    default:
        Assert::CodingError("Unknown SecurityPrincipalAccountType value {0}", (int)val);
    }
}

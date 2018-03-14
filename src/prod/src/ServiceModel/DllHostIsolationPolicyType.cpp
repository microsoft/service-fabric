// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

void CodePackageIsolationPolicyType::WriteToTextWriter(TextWriter & w, Enum const & val)
{
    w << ToString(val);
}

wstring CodePackageIsolationPolicyType::ToString(Enum const & val)
{
    switch (val)
    {
    case CodePackageIsolationPolicyType::Invalid:
        return L"Invalid";
    case CodePackageIsolationPolicyType::SharedDomain:
        return L"SharedDomain";
    case CodePackageIsolationPolicyType::DedicatedDomain:
        return L"DedicatedDomain";
    case CodePackageIsolationPolicyType::DedicatedProcess:
        return L"DedicatedProcess";
    case CodePackageIsolationPolicyType::Container:
        return L"Container";
    default:
        Assert::CodingError("Unknown CodePackageIsolationPolicyType value {0}", (int)val);
    }
}

ErrorCode CodePackageIsolationPolicyType::FromPublicApi(FABRIC_DLLHOST_ISOLATION_POLICY const & publicVal, __out Enum & val)
{
    switch(publicVal)
    {
    case FABRIC_DLLHOST_ISOLATION_POLICY_INVALID:
        val = CodePackageIsolationPolicyType::Invalid;
        break;
    case FABRIC_DLLHOST_ISOLATION_POLICY_SHARED_DOMAIN:
        val = CodePackageIsolationPolicyType::SharedDomain;
        break;
    case FABRIC_DLLHOST_ISOLATION_POLICY_DEDICATED_DOMAIN:
        val = CodePackageIsolationPolicyType::DedicatedDomain;
        break;
    case FABRIC_DLLHOST_ISOLATION_POLICY_DEDICATED_PROCESS:
        val = CodePackageIsolationPolicyType::DedicatedProcess;
        break;
    default:
        return ErrorCode::FromHResult(E_INVALIDARG);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

FABRIC_DLLHOST_ISOLATION_POLICY CodePackageIsolationPolicyType::ToPublicApi(Enum const & val)
{
    switch (val)
    {
    case CodePackageIsolationPolicyType::Invalid:
        return FABRIC_DLLHOST_ISOLATION_POLICY_INVALID;
    case CodePackageIsolationPolicyType::SharedDomain:
        return FABRIC_DLLHOST_ISOLATION_POLICY_SHARED_DOMAIN;
    case CodePackageIsolationPolicyType::DedicatedDomain:
        return FABRIC_DLLHOST_ISOLATION_POLICY_DEDICATED_DOMAIN;
    case CodePackageIsolationPolicyType::DedicatedProcess:
        return FABRIC_DLLHOST_ISOLATION_POLICY_DEDICATED_PROCESS;
    default:
        Assert::CodingError("Unknown CodePackageIsolationPolicyType value {0}", (int)val);
    }
}

bool CodePackageIsolationPolicyType::TryParseFromString(std::wstring const& string, __out Enum & val)
{
    val = CodePackageIsolationPolicyType::Invalid;

    if (string == L"SharedDomain")
    {
        val = CodePackageIsolationPolicyType::SharedDomain;
    }
    else if (string == L"DedicatedDomain")
    {
        val = CodePackageIsolationPolicyType::DedicatedDomain;
    }
    else if (string == L"DedicatedProcess")
    {
        val = CodePackageIsolationPolicyType::DedicatedProcess;
    }
    else if (string == L"Container")
    {
        val = CodePackageIsolationPolicyType::Container;
    }
    else
    {
        return false;
    }

    return true;
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

void ApplicationIsolationPolicyType::WriteToTextWriter(TextWriter & w, Enum const & val)
{
    switch (val)
    {
    case ApplicationIsolationPolicyType::Invalid:
        w << L"Invalid";
        return;
    case ApplicationIsolationPolicyType::SharedProcess:
        w << L"SharedProcess";
        return;
    case ApplicationIsolationPolicyType::DedicatedProcess:
        w << L"DedicatedProcess";
        return;
    default:
        Assert::CodingError("Unknown ApplicationIsolationPolicyType value {0}", (int)val);
    }
}

ErrorCode ApplicationIsolationPolicyType::FromString(wstring const & value, __out Enum & val)
{
    if (value == L"SharedProcess")
    {
        val = ApplicationIsolationPolicyType::SharedProcess;
        return ErrorCode(ErrorCodeValue::Success);
    } 
    else if (value == L"DedicatedProcess")
    {
        val = ApplicationIsolationPolicyType::DedicatedProcess;
        return ErrorCode(ErrorCodeValue::Success);
    }
    else
    {
        return ErrorCode::FromHResult(E_INVALIDARG);
    }
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

void DllHostHostedDllKind::WriteToTextWriter(TextWriter & w, Enum const & val)
{
    switch (val)
    {
   case DllHostHostedDllKind::Invalid:
        w << L"Invalid";
        return;
    case DllHostHostedDllKind::Unmanaged:
        w << L"UnmanagedDll";
        return;
    case DllHostHostedDllKind::Managed:
        w << L"ManagedAssembly";
        return;
    default:
        Assert::CodingError("Unknown DllHostHostedDllKind value {0}", (int)val);
    }
}

ErrorCode DllHostHostedDllKind::FromPublicApi(FABRIC_DLLHOST_HOSTED_DLL_KIND const & publicVal, __out Enum & val)
{
    switch (publicVal)
    {
    case FABRIC_DLLHOST_HOSTED_DLL_KIND_UNMANAGED:
        val = DllHostHostedDllKind::Unmanaged;
        break;
    case FABRIC_DLLHOST_HOSTED_DLL_KIND_MANAGED:
        val = DllHostHostedDllKind::Managed;
        break;
    default:
        return ErrorCode::FromHResult(E_INVALIDARG);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

FABRIC_DLLHOST_HOSTED_DLL_KIND DllHostHostedDllKind::ToPublicApi(Enum const & val)
{
    switch (val)
    {
    case DllHostHostedDllKind::Unmanaged :
        return FABRIC_DLLHOST_HOSTED_DLL_KIND_UNMANAGED;
    case DllHostHostedDllKind::Managed:
        return FABRIC_DLLHOST_HOSTED_DLL_KIND_MANAGED;
    default:
        Assert::CodingError("Unknown DllHostHostedDllKind value {0}", (int)val);
    }
}

void WriteToTextWriter(__in Common::TextWriter & w, ::FABRIC_DLLHOST_HOSTED_DLL_KIND const & val)
{
    switch (val)
    {
    case ::FABRIC_DLLHOST_HOSTED_DLL_KIND_INVALID:
            w << "FABRIC_DLLHOST_HOSTED_DLL_KIND_INVALID";
            return;
    case ::FABRIC_DLLHOST_HOSTED_DLL_KIND_UNMANAGED:
            w << "FABRIC_DLLHOST_HOSTED_DLL_KIND_UNMANAGED";
            return;
    case ::FABRIC_DLLHOST_HOSTED_DLL_KIND_MANAGED:
            w << "FABRIC_DLLHOST_HOSTED_DLL_KIND_MANAGED";
            return;
    default:
            Assert::CodingError("Unknown FABRIC_DLLHOST_HOSTED_DLL_KIND value {0}", (int)val);
    }
}

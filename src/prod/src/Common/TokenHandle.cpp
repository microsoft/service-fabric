// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

TokenHandle::TokenHandle(HANDLE hValue)
    : HandleBase(hValue)
{
}

TokenHandle::~TokenHandle()
{
    Cleanup(Value);
}

TokenHandleSPtr TokenHandle::CreateSPtr(HANDLE hValue)
{
    try
    {
        return std::make_shared<TokenHandle>(hValue);
    }
    catch(...)
    {
        Cleanup(hValue);
        throw;
    }
}

TokenHandleUPtr TokenHandle::CreateUPtr(HANDLE hValue)
{
    try
    {
        return make_unique<TokenHandle>(hValue);
    }
    catch(...)
    {
        Cleanup(hValue);
        throw;
    }
}

void TokenHandle::Cleanup(HANDLE hValue)
{
    auto success = ::CloseHandle(hValue);
    if (success == 0)
    {
        Common::TraceWarning(
            Common::TraceTaskCodes::Common,
            "TokenHandle",
            "CloseHandle failed: ErrorCode={0}",
            Common::ErrorCode::FromWin32Error());
    }
}

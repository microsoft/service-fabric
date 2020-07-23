// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

ProfileHandle::ProfileHandle(TokenHandleSPtr const & tokenHandle, HANDLE hValue)
    : HandleBase(hValue),
    tokenHandle_(tokenHandle)
{
    ASSERT_IF(tokenHandle_.get() == nullptr, "TokenHandle must not be null.");
}

ProfileHandle::~ProfileHandle()
{
    Cleanup(tokenHandle_, Value);
}

ProfileHandleSPtr ProfileHandle::CreateSPtr(TokenHandleSPtr const & tokenHandle, HANDLE hValue)
{
    try
    {
        return std::make_shared<ProfileHandle>(tokenHandle, hValue);
    }
    catch(...)
    {
        Cleanup(tokenHandle, hValue);
        throw;
    }
}

ProfileHandleUPtr ProfileHandle::CreateUPtr(TokenHandleSPtr const & tokenHandle, HANDLE hValue)
{
    try
    {
        return make_unique<ProfileHandle>(tokenHandle, hValue);
    }
    catch(...)
    {
        Cleanup(tokenHandle, hValue);
        throw;
    }
}

void ProfileHandle::Cleanup(TokenHandleSPtr const & tokenHandle, HANDLE hValue)
{
    Common::TraceNoise(
        Common::TraceTaskCodes::Common,
        "ProfileHandle",
        "Cleanup: UnloadUserProfile");
    if (!::UnloadUserProfile(tokenHandle->Value, hValue))
    {
        Common::TraceWarning(
            Common::TraceTaskCodes::Common,
            "ProfileHandle",
            "UnloadUserProfile failed: ErrorCode={0}",
            Common::ErrorCode::FromWin32Error());
    }
}

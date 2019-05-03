// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

WorkstationHandle::WorkstationHandle(HWINSTA hValue)
    : HandleBase(hValue)
{
}

WorkstationHandle::~WorkstationHandle()
{
    Cleanup(Value);
}

WorkstationHandleSPtr WorkstationHandle::CreateSPtr(HWINSTA hValue)
{
    try
    {
        return std::make_shared<WorkstationHandle>(hValue);
    }
    catch(...)
    {
        Cleanup(hValue);
        throw;
    }
}

WorkstationHandleUPtr WorkstationHandle::CreateUPtr(HWINSTA hValue)
{
    try
    {
        return make_unique<WorkstationHandle>(hValue);
    }
    catch(...)
    {
        Cleanup(hValue);
        throw;
    }
}

void WorkstationHandle::Cleanup(HWINSTA hValue)
{
    Common::TraceNoise(
        Common::TraceTaskCodes::Common,
        "WorkstationHandle",
        "Cleanup: CloseWindowStation");

    if (!::CloseWindowStation(hValue))
    {
        Common::TraceWarning(
            Common::TraceTaskCodes::Common,
            "WorkstationHandle",
            "CloseWindowStation failed: ErrorCode={0}",
            Common::ErrorCode::FromWin32Error());
    }
}

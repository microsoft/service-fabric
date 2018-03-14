// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

JobHandle::JobHandle(HANDLE hValue, bool autoTerminate)
    : HandleBase(hValue), autoTerminate_(autoTerminate)
{
}

JobHandle::~JobHandle()
{
    Cleanup(Value, autoTerminate_);
}

JobHandleSPtr JobHandle::CreateSPtr(HANDLE hValue, bool autoTerminate)
{
    try
    {
        return std::make_shared<JobHandle>(hValue, autoTerminate);
    }
    catch(...)
    {
        Cleanup(hValue, autoTerminate);
        throw;
    }
}

JobHandleUPtr JobHandle::CreateUPtr(HANDLE hValue, bool autoTerminate)
{
    try
    {
        return make_unique<JobHandle>(hValue, autoTerminate);
    }
    catch(...)
    {
        Cleanup(hValue, autoTerminate);
        throw;
    }
}

void JobHandle::Cleanup(HANDLE hValue, bool autoTerminate)
{
    if (autoTerminate)
    {
        ::TerminateJobObject(hValue, 1);
    }
   ::CloseHandle(hValue);
}

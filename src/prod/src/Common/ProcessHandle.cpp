// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

ProcessHandle::ProcessHandle(HANDLE hValue, bool autoTerminate)
    : HandleBase(hValue), autoTerminate_(autoTerminate)
{
}

ProcessHandle::~ProcessHandle()
{
    Cleanup(Value, autoTerminate_);
}

ProcessHandleSPtr ProcessHandle::CreateSPtr(HANDLE hValue, bool autoTerminate)
{
    try
    {
        return std::make_shared<ProcessHandle>(hValue, autoTerminate);
    }
    catch(...)
    {
        Cleanup(hValue, autoTerminate);
        throw;
    }
}

ProcessHandleUPtr ProcessHandle::CreateUPtr(HANDLE hValue, bool autoTerminate)
{
    try
    {
        return make_unique<ProcessHandle>(hValue, autoTerminate);
    }
    catch(...)
    {
        Cleanup(hValue, autoTerminate);
        throw;
    }
}

void ProcessHandle::Cleanup(HANDLE hValue, bool autoTerminate)
{
    if (autoTerminate)
    {
        ::TerminateProcess(hValue, ErrorCodeValue::ProcessDeactivated & 0x0000FFFF);
    }
   ::CloseHandle(hValue);
}

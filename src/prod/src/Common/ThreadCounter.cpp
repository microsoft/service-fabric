//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"

#if !defined(PLATFORM_UNIX)
#include "TlHelp32.h"
#endif

#include "ThreadCounter.h"

using namespace std;

using Common::ThreadCounter;

void ThreadCounter::SetThreadTestLimit(int limit)
{
    if (limit > 0)
    {
        limit_ = limit;
    }
}

void ThreadCounter::OnThreadEnter()
{
    if (count_ < 0)
    {
        InitializeThreadCount();
    }

    ++count_;

    FailFastIfThreadLimitHit();
}

void ThreadCounter::OnThreadExit()
{
    --count_;
}

void ThreadCounter::FailFastIfThreadLimitHit()
{
    if ((0 < limit_) && (limit_ < Threadpool::ActiveCallbackCount()))
    {
        // Crash as quickly as possible to preserve threadpool stacks, TestAssert may be too slow due to stack walking
        ::RaiseFailFastException(nullptr, nullptr, 0);
    }
}

void ThreadCounter::InitializeThreadCount()
{
#if !defined(PLATFORM_UNIX)
    DWORD processId = GetCurrentProcessId();

    Handle snapshotHandle(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (snapshotHandle.Value == INVALID_HANDLE_VALUE)
    {
        count_ = 1;
        return;
    }

    PROCESSENTRY32 processEntry = {};
    processEntry.dwSize = sizeof(processEntry);
    if (!Process32First(snapshotHandle.Value, &(processEntry)))
    {
        count_ = 1;
        return;
    }

    do
    {
        if (processEntry.th32ProcessID == processId)
        {
            count_ = processEntry.cntThreads;
            return;
        }
    } while (Process32Next(snapshotHandle.Value, &(processEntry)));
#endif
}

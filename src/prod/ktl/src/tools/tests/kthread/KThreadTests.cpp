/*++

Copyright (c) Microsoft Corporation

Module Name:

    KThreadTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KThreadTests.h.
    2. Add an entry to the gs_KuTestCases table in KThreadTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include "KThreadTests.h"
#include <CommandLineParser.h>

#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif

static LONG g_Global = 0;

VOID
RunThread(
    __in VOID* Parameter
    )

{
    LONG* l = (LONG*) Parameter;

    //
    // Set the value that the parameter points to, to 1.
    //

    *l = 1;
}

NTSTATUS
KThreadTestX(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    NTSTATUS status = STATUS_SUCCESS;
    KThread::SPtr thread;

    EventRegisterMicrosoft_Windows_KTL();

    KTestPrintf("Starting KThreadTest test");

    //
    // Create a thread that will set a global variable.
    //

    status = KThread::Create(RunThread, (VOID*) &g_Global, thread, KtlSystem::GlobalNonPagedAllocator());

    if (!NT_SUCCESS(status)) {
        KTestPrintf("Could not create the thread %x\n", status);
        goto Finish;
    }

    //
    // Wait for the thread to exit and check the global.
    //

    thread->Wait();

    if (g_Global != 1) {
        KTestPrintf("the thread did not run\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

Finish:

    EventUnregisterMicrosoft_Windows_KTL();

    return status;
}

NTSTATUS
KThreadTest(
    int argc, WCHAR* args[]
    )
{
    NTSTATUS status;

#if defined(PLATFORM_UNIX)
    status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif
    
    KTestPrintf("KThreadTest: STARTED\n");

    status = KtlSystem::Initialize();

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = KThreadTestX(argc, args);

    KtlSystem::Shutdown();

    KTestPrintf("KThreadTest: COMPLETED\n");

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif  
    
    return status;
}

#if CONSOLE_TEST
int
main(int argc, char* args[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    return RtlNtStatusToDosError(KThreadTest(0, NULL));
}
#endif

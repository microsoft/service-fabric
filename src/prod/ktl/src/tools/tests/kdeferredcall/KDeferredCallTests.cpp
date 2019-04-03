/*++

Copyright (c) Microsoft Corporation

Module Name:

    KDeferredCallTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KDeferredCallTests.h.
    2. Add an entry to the gs_KuTestCases table in KDeferredCallTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <ktl.h>
#include <ktrace.h>
#include "KDeferredCallTests.h"
#include <CommandLineParser.h>

#if KTL_USER_MODE
 extern volatile LONGLONG gs_AllocsRemaining;
#endif

KAllocator* g_Allocator = nullptr;

LONG g_CallCount = 0;
KAutoResetEvent* g_pEvent;

class Blah
{
public:
    Blah()
    {
    }

    VOID Test()
    {
        KTestPrintf("Test function\n");
        g_pEvent->SetEvent();
    }

    VOID Test1(int a)
    {
        KTestPrintf("Test function with value %d\n", a);
        InterlockedDecrement(&g_CallCount);
        g_pEvent->SetEvent();

    }


    VOID Test2(int a, int b)
    {
        KTestPrintf("The diff is a-b=%d\n", a-b);
        InterlockedDecrement(&g_CallCount);

        g_pEvent->SetEvent();

    }


    VOID Test3(int a, int b, int c)
    {
        KTestPrintf("The sum=%d\n", a+b+c);
        InterlockedDecrement(&g_CallCount);

        g_pEvent->SetEvent();

    }

};



VOID MyFunc(int x, int y)
{
    KTestPrintf("The total is x + y = %d\n", x + y);
    InterlockedDecrement(&g_CallCount);
    g_pEvent->SetEvent();
}

VOID
TestJumps()
{
    Blah b;
    KAutoResetEvent ev;
    g_CallCount = 6;
    g_pEvent = &ev;


    KDeferredJump<VOID(int)> t1(b, &Blah::Test1, *g_Allocator);
    t1(10);

    // Invoke simple functino

    KDeferredJump<VOID(int, int)> t(MyFunc, *g_Allocator);
    t(10, 78);

    // Invoke
    KDeferredJump<VOID(int, int)> t2(b, &Blah::Test2, *g_Allocator);
    t2(30, 10);
    t2(40, 7);

    KDeferredJump<VOID(int, int, int)> t3(b, &Blah::Test3, *g_Allocator);
    t3(30, 10, 3);
    t3(40, 7, 7);



    KTestPrintf("About to exec\n");

    for (;;)
    {
        KTestPrintf("Calls pending= %u\n", g_CallCount);
        ev.WaitUntilSet();
        if (g_CallCount == 0)
        {
            break;
        }
    }
}

VOID
TestEntryVoid()
{
    KTestPrintf("Test entry VOID\n");
    InterlockedDecrement(&g_CallCount);
    g_pEvent->SetEvent();
}

int
TestEntry0()
{
    KTestPrintf("Test entry 0\n");
    InterlockedDecrement(&g_CallCount);
    g_pEvent->SetEvent();
    return 0;
}


int
TestEntry1(int in)
{
    KTestPrintf("Test entry 1\n");
    InterlockedDecrement(&g_CallCount);
    g_pEvent->SetEvent();
    return in + 100;
}

VOID
TestEntryV1(int in)
{
    UNREFERENCED_PARAMETER(in);

    KTestPrintf("Test entry 1\n");
    InterlockedDecrement(&g_CallCount);
    g_pEvent->SetEvent();
}

int
TestEntry2(int in1, int in2)
{
    KTestPrintf("Test entry 2\n");
    InterlockedDecrement(&g_CallCount);
    g_pEvent->SetEvent();
    return in1 + in2 + 100;
}

VOID
TestEntryV2(int in1, int in2)
{
    UNREFERENCED_PARAMETER(in1);
    UNREFERENCED_PARAMETER(in2);

    KTestPrintf("Test entry 2\n");
    InterlockedDecrement(&g_CallCount);
    g_pEvent->SetEvent();
}

int
TestEntry3(int in1, int in2, int in3)
{
    KTestPrintf("Test entry 3\n");
    InterlockedDecrement(&g_CallCount);
    g_pEvent->SetEvent();
    return in1 + in2 + in3 + 100;
}

VOID
TestEntryV3(int in1, int in2, int in3)
{
    UNREFERENCED_PARAMETER(in1);
    UNREFERENCED_PARAMETER(in2);
    UNREFERENCED_PARAMETER(in3);

    KTestPrintf("Test entry 3\n");
    InterlockedDecrement(&g_CallCount);
    g_pEvent->SetEvent();
}



VOID
TestCalls()
{
    KAutoResetEvent ev;
    g_CallCount = 8;
    g_pEvent = &ev;
    KtlSystem& Sys = g_Allocator->GetKtlSystem();

    KDeferredCall<VOID()> cv(Sys);
    KDeferredCall<int()> c0(Sys);

    KDeferredCall<VOID(int)> cv1(Sys);
    KDeferredCall<int(int)> c1(Sys);

    KDeferredCall<int(int,int)> c2(Sys);
    KDeferredCall<VOID(int,int)> cv2(Sys);

    KDeferredCall<int(int,int,int)> c3(Sys);
    KDeferredCall<VOID(int,int,int)> cv3(Sys);


    cv.Bind(TestEntryVoid);
    c0.Bind(TestEntry0);

    c1.Bind(TestEntry1);
    cv1.Bind(TestEntryV1);

    c2.Bind(TestEntry2);
    cv2.Bind(TestEntryV2);

    c3.Bind(TestEntry3);
    cv3.Bind(TestEntryV3);

    cv();
    c0();
    c1(10);
    cv1(4);
    c2(23, 27);
    cv2(44, 2);
    c3(19, 1000, 418);
    cv3(1, 2, 3);

    for (;;)
    {
        KTestPrintf("Calls pending= %u\n", g_CallCount);
        ev.WaitUntilSet();
        if (g_CallCount == 0)
        {
            break;
        }
    }

    int res0 = c0.ReturnValue();
    int res1 = c1.ReturnValue();
    int res2 = c2.ReturnValue();
    int res3 = c3.ReturnValue();

    KTestPrintf("Results = %d %d %d %d\n", res0, res1, res2, res3);

}

NTSTATUS
TestSequence()

{
    TestJumps();
    TestCalls();
    return STATUS_SUCCESS;
}

NTSTATUS
KDeferredCallTest(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

#if defined(PLATFORM_UNIX)
    NTSTATUS status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif
    
    KTestPrintf("KDeferredCallTest: STARTED\n");

    KtlSystem* underlyingSystem;
    NTSTATUS Result = KtlSystem::Initialize(&underlyingSystem);
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("KtlSystem::Initializer() failure\n");
        return Result;
    }
    KFinally([&](){ KtlSystem::Shutdown(); });
    g_Allocator = &KtlSystem::GlobalNonPagedAllocator();

    ULONGLONG StartingAllocs = KAllocatorSupport::gs_AllocsRemaining;

    EventRegisterMicrosoft_Windows_KTL();

    underlyingSystem->SetDefaultSystemThreadPoolUsage(FALSE);       // Use only dedicated ktl threadpool for all Asyncs

    // NTSTATUS Res =
    TestSequence();

    ULONGLONG Leaks = KAllocatorSupport::gs_AllocsRemaining - StartingAllocs;
    if (Leaks)
    {
        KTestPrintf("Leaks = %u\n", Leaks);
        Result = STATUS_UNSUCCESSFUL;
    }
    else
    {
        KTestPrintf("No leaks.\n");
    }

    EventUnregisterMicrosoft_Windows_KTL();

    KTestPrintf("KDeferredCallTest: COMPLETED\n");

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif      
    
    return Result;
}



#if CONSOLE_TEST
int
#if !defined(PLATFORM_UNIX)
wmain(int argc, WCHAR* args[])
{
#else
main(int argc, char* cargs[])
{
    CONVERT_TO_ARGS(argc, cargs)
#endif
    if (argc > 0)
    {
        argc--;
        args++;
    }

    return KDeferredCallTest(argc, args);
}
#endif




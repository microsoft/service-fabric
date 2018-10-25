/*++

Copyright (c) Microsoft Corporation

Module Name:

    KTimerTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KAsyncQueueTests.h.
    2. Add an entry to the gs_KuTestCases table in KAsyncQueueTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include "KAsyncQueueTests.h"
#include <CommandLineParser.h>

#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif


#if KTL_USER_MODE
extern volatile LONGLONG gs_AllocsRemaining;
#endif


extern NTSTATUS LastStaticTestCallbackStatus;

struct StaticTestCallback;

// Get the allowed timer delay overhead based on the timer delay.
// This overhead accounts for allowable expected delays due to CPU contention during test execution.
static ULONGLONG AllowedTimerOverhead(ULONGLONG timerDelay)
{
    #if KTL_USER_MODE
        return (timerDelay / 2);
    #else
        UNREFERENCED_PARAMETER(timerDelay);
        return 40;
    #endif
}

//** Basic KTimer Test
class TestTimer : public KTimer
{
    K_FORCE_SHARED(TestTimer);

public:
    static NTSTATUS
    Create(TestTimer::SPtr& Result, KAllocator& Allocator, ULONG Tag);

    BOOLEAN TestOnStartWasCalled;
    BOOLEAN TestOnCancelWasCalled;
    BOOLEAN TestUnsafeOnTimerFiringWasCalled;

private:
    VOID
    OnStart();

    VOID
    OnCancel();

    VOID
    OnReuse();

    VOID
    UnsafeOnTimerFiring();
};

NTSTATUS
TestTimer::Create(TestTimer::SPtr& Result, KAllocator& Allocator, ULONG Tag)
{
    Result = _new(Tag, Allocator) TestTimer();
    if (Result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = Result->Status();
    if (!NT_SUCCESS(status))
    {
        Result.Reset();
    }
    return status;
}

TestTimer::TestTimer()
{
    TestOnStartWasCalled = FALSE;
    TestOnCancelWasCalled = FALSE;
    TestUnsafeOnTimerFiringWasCalled = FALSE;
}

TestTimer::~TestTimer()
{
}

VOID
TestTimer::OnReuse()
{
    TestOnStartWasCalled = FALSE;
    TestOnCancelWasCalled = FALSE;
    TestUnsafeOnTimerFiringWasCalled = FALSE;
}

VOID
TestTimer::OnStart()
{
    TestOnStartWasCalled = TRUE;
    ScheduleTimer();
}

VOID
TestTimer::OnCancel()
{
    TestOnCancelWasCalled = TRUE;
    CancelTimerAndComplete(STATUS_CANCELLED);
}

VOID
TestTimer::UnsafeOnTimerFiring()
{
    TestUnsafeOnTimerFiringWasCalled = TRUE;
    Complete(STATUS_SUCCESS);
}


NTSTATUS
BasicKTimerTest()
{
    NTSTATUS                                    status;
    KAllocator&                                 allocator = KtlSystem::GlobalNonPagedAllocator();
    KEvent                                      completionEvent(FALSE, FALSE);
    StaticTestCallback                          cb(&completionEvent);
    KAsyncContextBase::CompletionCallback       completionCallback(&cb, &StaticTestCallback::Callback);
    ULONGLONG const                             timerDelay = 2000;
    ULONGLONG const                             timerResolution = 20;
    ULONG const                                 numberOfInterations = 10;


    for (ULONG i = 0; i < numberOfInterations; i++)
    {
        KTimer::SPtr                                testKTimer;
        status = KTimer::Create(testKTimer, allocator, KTL_TAG_TEST);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("BasicKTimerTest: KTimer::Create Failed: %i\n", __LINE__);
            return status;
        }

        //* Prove basic delay functionality
        ULONGLONG sTime = KNt::GetTickCount64();
        testKTimer->StartTimer(timerDelay, nullptr, completionCallback);
        completionEvent.WaitUntilSet();
        ULONGLONG eTime = KNt::GetTickCount64();
        status = testKTimer->Status();
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("BasicKTimerTest: Timer Failed: %i\n", __LINE__);
            return status;
        }

        if ((eTime - sTime) < (timerDelay - timerResolution))
        {
            KTestPrintf("BasicKTimerTest: Timer completed too quickly (%I64u): %i\n", (eTime - sTime), __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        if ((eTime - sTime) > (timerDelay + timerResolution + AllowedTimerOverhead(timerDelay)))
        {
            KTestPrintf("BasicKTimerTest: Timer completed too late (%I64u): %i\n", (eTime - sTime), __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        //* Prove cancel behavior
        testKTimer->Reuse();
        testKTimer->StartTimer(timerDelay, nullptr, completionCallback);
        KNt::Sleep(timerDelay / 2);
        status = testKTimer->Status();
        if (status != STATUS_PENDING)
        {
            KTestPrintf("BasicKTimerTest: Unexpected Status() - 0x%08X: %i\n", status, __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        testKTimer->Cancel();
        completionEvent.WaitUntilSet();
        status = testKTimer->Status();
        if (status != STATUS_CANCELLED)
        {
            KTestPrintf("BasicKTimerTest: Unexpected Status() - 0x%08X: %i\n", status, __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        //* Prove reset behavior
        testKTimer->Reuse();
        sTime = KNt::GetTickCount64();
        testKTimer->StartTimer(timerDelay, nullptr, completionCallback);

        const ULONG resetCount = 10;
        for (ULONG r = 0; r < resetCount; r++)
        {
            KNt::Sleep(timerDelay / 2);

            status = testKTimer->Status();
            if (status != STATUS_PENDING)
            {
                KTestPrintf("BasicKTimerTest: Unexpected Status() - 0x%08X: %i\n", status, __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            if (testKTimer->ResetTimer(timerDelay) != TRUE)
            {
                KTestPrintf("BasicKTimerTest: Timer failed to reset: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }
        }

        completionEvent.WaitUntilSet();
        eTime = KNt::GetTickCount64();
        status = testKTimer->Status();
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("BasicKTimerTest: Unexpected Status() - 0x%08X: %i\n", status, __LINE__);
            return status;
        }

        const ULONGLONG expectedDelay = (timerDelay / 2) * resetCount + timerDelay;
        if ((eTime - sTime) < expectedDelay)
        {
            KTestPrintf("BasicKTimerTest: Timer was not properly reset and completed too quickly (%I64u): %i\n", (eTime - sTime), __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        //* Reset after the timer fires should not succeed
        if (testKTimer->ResetTimer(timerDelay) != FALSE)
        {
            KTestPrintf("BasicKTimerTest: Timer reset succeeded when it was supposed to fail (timer has completed): %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        //* Prove reset with cancel behavior
        for (int j = 0; j <= 3; j++)
        {
            testKTimer->Reuse();
            sTime = KNt::GetTickCount64();
            testKTimer->StartTimer(timerDelay, nullptr, completionCallback);

            KNt::Sleep(timerDelay / 2);

            if (j % 2 == 0)
            {
                testKTimer->ResetTimer(timerDelay);
                testKTimer->Cancel();
            }
            else
            {
                testKTimer->Cancel();
                testKTimer->ResetTimer(timerDelay);
            }

            completionEvent.WaitUntilSet();
            eTime = KNt::GetTickCount64();
            status = testKTimer->Status();
            if (status != STATUS_CANCELLED)
            {
                KTestPrintf("BasicKTimerTest: Unexpected Status() - 0x%08X: %i\n", status, __LINE__);
                return status;
            }

            if ((eTime - sTime) > timerDelay)
            {
                KTestPrintf("BasicKTimerTest: Timer cancelled too late (%I64u): %i\n", (eTime - sTime), __LINE__);
                return STATUS_UNSUCCESSFUL;
            }
        }
    }

    //** Test basic derivation behavior
    for (ULONG i = 0; i < numberOfInterations; i++)
    {
        TestTimer::SPtr                         testTimer;

        status = TestTimer::Create(testTimer, allocator, KTL_TAG_TEST);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("BasicKTimerTest: TestTimer::Create Failed: %i\n", __LINE__);
            return status;
        }

        //* Prove basic delay functionality
        ULONGLONG sTime = KNt::GetTickCount64();
        testTimer->StartTimer(timerDelay, nullptr, completionCallback);
        completionEvent.WaitUntilSet();
        ULONGLONG eTime = KNt::GetTickCount64();
        status = testTimer->Status();
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("BasicKTimerTest: TestTimer Failed: %i\n", __LINE__);
            return status;
        }

        if ((eTime - sTime) < (timerDelay - timerResolution))
        {
            KTestPrintf("BasicKTimerTest: TestTimer completed too quickly (%I64u): %i\n", (eTime - sTime), __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        if ((eTime - sTime) > (timerDelay + timerResolution + AllowedTimerOverhead(timerDelay)))
        {
            KTestPrintf("BasicKTimerTest: TestTimer completed too late (%I64u): %i\n", (eTime - sTime), __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        if (!testTimer->TestOnStartWasCalled)
        {
            KTestPrintf("BasicKTimerTest: TestTimer::TestOnStartWasCalled not set: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        if (!testTimer->TestUnsafeOnTimerFiringWasCalled)
        {
            KTestPrintf("BasicKTimerTest: TestTimer::TestUnsafeOnTimerFiringWasCalled not set: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        if (testTimer->TestOnCancelWasCalled)
        {
            KTestPrintf("BasicKTimerTest: TestTimer::TestOnCancelWasCalled was set - in error: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        //* Prove cancel behavior
        testTimer->Reuse();
        testTimer->StartTimer(timerDelay, nullptr, completionCallback);
        KNt::Sleep(timerDelay / 2);
        status = testTimer->Status();
        if (status != STATUS_PENDING)
        {
            KTestPrintf("BasicKTimerTest: Unexpected TestTimer Status() - 0x%08X: %i\n", status, __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        testTimer->Cancel();
        completionEvent.WaitUntilSet();
        status = testTimer->Status();
        if (status != STATUS_CANCELLED)
        {
            KTestPrintf("BasicKTimerTest: Unexpected TestTimer Status() - 0x%08X: %i\n", status, __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        if (!testTimer->TestOnStartWasCalled)
        {
            KTestPrintf("BasicKTimerTest: TestTimer::TestOnStartWasCalled not set: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        if (testTimer->TestUnsafeOnTimerFiringWasCalled)
        {
            KTestPrintf("BasicKTimerTest: TestTimer::TestUnsafeOnTimerFiringWasCalled was set - in error: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        if (!testTimer->TestOnCancelWasCalled)
        {
            KTestPrintf("BasicKTimerTest: TestTimer::TestOnCancelWasCalled was not set: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
AdaptiveTimerTest()
{
    NTSTATUS status;
    KAllocator& allocator = KtlSystem::GlobalNonPagedAllocator();
    KAdaptiveTimerRoot::SPtr rootTimer;
    KAdaptiveTimerRoot::TimerInstance::SPtr testKTimer;
    KEvent completionEvent(FALSE, FALSE);
    StaticTestCallback cb(&completionEvent);
    KAsyncContextBase::CompletionCallback completionCallback(&cb, &StaticTestCallback::Callback);
    ULONGLONG                                   timerDelay;
    ULONGLONG const                             timerResolution = 20;
    int i;

    KAdaptiveTimerRoot::Parameters timerParams;
    LPCWSTR newParams[] =
    {
        L"MaximumTimeout", L"1000",
        L"InitialTimeInterval", L"200"
    };

    //
    //  Initialize and modify some of the initial parameters before creating an
    //  AdaptiveTimer.
    //

    timerParams.InitializeParameters();
    timerParams.ModifyParameters( ARRAYSIZE(newParams ), newParams );

    status = KAdaptiveTimerRoot::Create(
        rootTimer,
        timerParams,
        allocator,
        KTL_TAG_TEST
        );

    if (!NT_SUCCESS(status ))
    {
        KTestPrintf("AdaptiveTimerTest: Allocate root timer failed. Status = 0x%08X: %i\n", status, __LINE__);
        return(status);
    }

    if (rootTimer->GetCurrentTimeInterval() != 200)
    {
        KTestPrintf("AdaptiveTimerTest: Initialization failed. %d: %i\n", rootTimer->GetCurrentTimeInterval(), __LINE__);
        return(STATUS_UNSUCCESSFUL);
    }

    status = rootTimer->AllocateTimerInstance( testKTimer );

    if (!NT_SUCCESS(status ))
    {
        KTestPrintf("AdaptiveTimerTest: Allocate instanse timer failed. Status = 0x%08X: %i\n", status, __LINE__);
        return(status);
    }

    timerDelay = rootTimer->GetCurrentTimeInterval() * 4;
    ULONGLONG sTime = KNt::GetTickCount64();
    testKTimer->StartTimer(nullptr, completionCallback);
    completionEvent.WaitUntilSet();
    ULONGLONG eTime = KNt::GetTickCount64();
    status = testKTimer->Status();
    testKTimer->StopTimer();
    testKTimer->Reuse();

    if (!NT_SUCCESS(status))
    {
        KTestPrintf("AdaptiveTimerTest:  Timer Failed: %i\n", __LINE__);
        return status;
    }

    if ((eTime - sTime) < (timerDelay - timerResolution))
    {
        KTestPrintf("AdaptiveTimerTest:  Timer completed too quickly (%I64u): %i\n", (eTime - sTime), __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    if ((eTime - sTime) > (timerDelay + timerResolution + AllowedTimerOverhead(timerDelay)))
    {
        KTestPrintf("AdaptiveTimerTest:  Timer completed too late (%I64u): %i\n", (eTime - sTime), __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Use very short time intervals.
    //

    for (i = 0; i < 16; i++)
    {
        testKTimer->StartTimer(nullptr, completionCallback);
        status = testKTimer->Status();
        if (status != STATUS_PENDING)
        {
            KTestPrintf("AdaptiveTimerTest:  Unexpected Status() - 0x%08X: %i\n", status, __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        testKTimer->StopTimer();
        completionEvent.WaitUntilSet();
        status = testKTimer->Status();
        if (status != STATUS_CANCELLED)
        {
            KTestPrintf("AdaptiveTimerTest:  Unexpected Status() - 0x%08X: %i\n", status, __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        testKTimer->Reuse();
    }

    if (rootTimer->GetCurrentTimeInterval() > 35)
    {
        KTestPrintf("AdaptiveTimerTest: AdpativeTime short cycle failed. %d: %i\n", rootTimer->GetCurrentTimeInterval(), __LINE__);
         return(STATUS_UNSUCCESSFUL);
    }

    timerDelay = rootTimer->GetCurrentTimeInterval() * 4;
    sTime = KNt::GetTickCount64();
    testKTimer->StartTimer(nullptr, completionCallback);
    completionEvent.WaitUntilSet();
    eTime = KNt::GetTickCount64();
    status = testKTimer->Status();
    testKTimer->StopTimer();
    testKTimer->Reuse();

    if (!NT_SUCCESS(status))
    {
        KTestPrintf("AdaptiveTimerTest:  Timer Failed: %i\n", __LINE__);
        return status;
    }

    if ((eTime - sTime) < (timerDelay - timerResolution))
    {
        KTestPrintf("AdaptiveTimerTest:  Timer completed too quickly (%I64u): %i\n", (eTime - sTime), __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    if ((eTime - sTime) > (timerDelay + timerResolution + AllowedTimerOverhead(timerDelay)))
    {
        KTestPrintf("AdaptiveTimerTest:  Timer completed too late (%I64u): %i\n", (eTime - sTime), __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Next verify retry time increases.
    //

    timerDelay = rootTimer->GetCurrentTimeInterval() * 4;
    for (i = 0; i < 16; i++)
    {
        sTime = KNt::GetTickCount64();
        testKTimer->StartTimer(nullptr, completionCallback);
        completionEvent.WaitUntilSet();
        eTime = KNt::GetTickCount64();
        status = testKTimer->Status();
        testKTimer->Reuse();

        // No StopTimer.

        if (!NT_SUCCESS(status))
        {
            KTestPrintf("AdaptiveTimerTest:  Timer Failed: %i\n", __LINE__);
            return status;
        }

        if ((eTime - sTime) < (timerDelay - timerResolution))
        {
            KTestPrintf("AdaptiveTimerTest:  Timer completed too quickly (%I64u): %i\n", (eTime - sTime), __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        if ((eTime - sTime) > (timerDelay + timerResolution + AllowedTimerOverhead(timerDelay)))
        {
            KTestPrintf("AdaptiveTimerTest:  Timer completed too late (%I64u): %i\n", (eTime - sTime), __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        timerDelay = timerDelay * 2;
        if (timerDelay > 1000)
        {
            //
            // The timer delay should not exceed the limit we set initially.
            //

            timerDelay = 1000;
        }
    }

    return(STATUS_SUCCESS);
}

NTSTATUS
KTimerTest(int argc, WCHAR* args[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    NTSTATUS status = STATUS_SUCCESS;

    EventRegisterMicrosoft_Windows_KTL();
    KTestPrintf("Starting KTimerTest test\n");

    KtlSystem::Initialize();
    if ((status = BasicKTimerTest()) != STATUS_SUCCESS)
    {
        KTestPrintf("KTimerTest Unit Test: FAILED\n");
    }
#if !defined(PLATFORM_UNIX)
    else if ((status = AdaptiveTimerTest()) != STATUS_SUCCESS)
    {
        KTestPrintf("AdaptiveTimerTest Unit Test: FAILED\n");
    }
#endif

    KTestPrintf("KTimerTest tests passed\n");

    KtlSystem::Shutdown();

    EventUnregisterMicrosoft_Windows_KTL();
    return status;
}


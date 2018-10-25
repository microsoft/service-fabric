/*++

Copyright (c) Microsoft Corporation

Module Name:

    KAsyncServiceTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KAsyncEventTests.h.
    2. Add an entry to the gs_KuTestCases table in KAsyncEventTestCases.cpp.
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

class TestService : public KAsyncServiceBase
{
    K_FORCE_SHARED(TestService);

public:
    static void
    Create(TestService::SPtr& Result, KAllocator& Allocator)
    {
        Result = _new(KTL_TAG_TEST, Allocator) TestService();
        KInvariant(Result != nullptr);
    }

    // Enable access to test for completing open and close
    using KAsyncServiceBase::StartOpen;
    using KAsyncServiceBase::StartClose;
    using KAsyncServiceBase::CompleteOpen;
    using KAsyncServiceBase::CompleteClose;
    using KAsyncServiceBase::IsOpen;
    using KAsyncServiceBase::SetDeferredCloseBehavior;
    using KAsyncServiceBase::TryAcquireServiceActivity;
    using KAsyncServiceBase::ReleaseServiceActivity;
    using KAsyncServiceBase::_IsOpen;
    using KAsyncServiceBase::DeferredCloseState;
#if DBG
    using KAsyncServiceBase::Test_TryAcquireServiceActivitySpinCount;
    using KAsyncServiceBase::Test_ScheduleOnServiceCloseSpinCount;
    using KAsyncServiceBase::Test_ReleaseServiceActivitySpinCount;
#endif
    KEvent          OnServiceReuseCalled;
    KEvent          OnServiceOpenCalled;
    KEvent          OnServiceCloseCalled;
    KEvent          OnDeferredClosingCalled;

    void*           ExtendedOpenCtx;
    void*           ExtendedCloseCtx;

    // Test open-delayed API
    NTSTATUS
    StartTestApi(WCHAR* InputTestString, WCHAR*& OutputTestString, KAsyncContextBase::CompletionCallback Callback);

private:
    VOID OnServiceOpen() override;
    VOID OnServiceClose() override;
    VOID OnDeferredClosing() override;
    VOID OnServiceReuse() override;
    VOID OnUnsafeOpenCompleteCalled(void* OpenedCompleteCtx) override;
    VOID OnUnsafeCloseCompleteCalled(void* ClosedCompleteCtx) override;
};

TestService::TestService()
    :   OnServiceReuseCalled(FALSE, FALSE),
        OnServiceOpenCalled(FALSE, FALSE),
        OnServiceCloseCalled(FALSE, FALSE),
        ExtendedOpenCtx(nullptr),
        ExtendedCloseCtx(nullptr)
{
}

TestService::~TestService()
{
}

VOID
TestService::OnServiceOpen()
{
    OnServiceOpenCalled.SetEvent();
}

VOID
TestService::OnServiceClose()
{
    OnServiceCloseCalled.SetEvent();
}

VOID
TestService::OnDeferredClosing()
{
    OnDeferredClosingCalled.SetEvent();
}

VOID
TestService::OnServiceReuse()
{
    ExtendedOpenCtx = nullptr;
    ExtendedCloseCtx = nullptr;
    OnServiceReuseCalled.SetEvent();
}

VOID 
TestService::OnUnsafeOpenCompleteCalled(void* OpenedCompleteCtx)
{
    ExtendedOpenCtx = OpenedCompleteCtx;
}

VOID
TestService::OnUnsafeCloseCompleteCalled(void* ClosedCompleteCtx)
{
    ExtendedCloseCtx = ClosedCompleteCtx;
}



//** AsyncService Test
NTSTATUS
DoBasicServiceTest(KAllocator& Allocator)
{
    TestService::SPtr       testSvc;
    KServiceSynchronizer    openSync;
    KServiceSynchronizer    closeSync;

    // Prove happy path open, close, and reuse
    TestService::Create(testSvc, Allocator);
    KInvariant(testSvc->Status() == K_STATUS_NOT_STARTED);
    KInvariant(!testSvc->IsOpen());

    KInvariant(testSvc->StartOpen(nullptr, openSync.OpenCompletionCallback()) == STATUS_PENDING);
    KInvariant(!testSvc->IsOpen());
    KInvariant(testSvc->StartOpen(nullptr, openSync.OpenCompletionCallback()) == STATUS_SHARING_VIOLATION); // Prove 2nd open fails correctly
    KInvariant(!testSvc->IsOpen());

    KInvariant(testSvc->OnServiceOpenCalled.WaitUntilSet());
    KInvariant(testSvc->CompleteOpen(STATUS_SUCCESS, (void*)1));
    KInvariant(NT_SUCCESS(openSync.WaitForCompletion()));
    KInvariant(testSvc->ExtendedOpenCtx == (void*)1);
    KInvariant(testSvc->IsOpen());

    KInvariant(testSvc->StartClose(nullptr, closeSync.CloseCompletionCallback()) == STATUS_PENDING);
    KInvariant(!testSvc->IsOpen());
    KInvariant(testSvc->StartClose(nullptr, closeSync.CloseCompletionCallback()) == STATUS_UNSUCCESSFUL);   // Prove 2nd close fails correctly
    KInvariant(!testSvc->IsOpen());

    KInvariant(testSvc->OnServiceCloseCalled.WaitUntilSet());
    KInvariant(testSvc->CompleteClose(STATUS_SUCCESS, (void*)2));
    KInvariant(NT_SUCCESS(closeSync.WaitForCompletion()));
    KInvariant(testSvc->ExtendedCloseCtx == (void*)2);
    KInvariant(!testSvc->IsOpen());

    KInvariant(testSvc->Status() == STATUS_SUCCESS);

    // Prove Reuse
    testSvc->Reuse();
    KInvariant(testSvc->OnServiceReuseCalled.WaitUntilSet());
    KInvariant(testSvc->ExtendedOpenCtx == nullptr);
    KInvariant(testSvc->ExtendedCloseCtx == nullptr);
    KInvariant(!testSvc->IsOpen());

    KInvariant(testSvc->Status() == K_STATUS_NOT_STARTED);

    KInvariant(testSvc->StartOpen(nullptr, openSync.OpenCompletionCallback()) == STATUS_PENDING);
    KInvariant(testSvc->OnServiceOpenCalled.WaitUntilSet());
    KInvariant(testSvc->CompleteOpen(STATUS_SUCCESS));
    KInvariant(NT_SUCCESS(openSync.WaitForCompletion()));
    KInvariant(testSvc->ExtendedOpenCtx == nullptr);

    KInvariant(testSvc->StartClose(nullptr, closeSync.CloseCompletionCallback()) == STATUS_PENDING);
    KInvariant(testSvc->OnServiceCloseCalled.WaitUntilSet());
    KInvariant(testSvc->CompleteClose(STATUS_SUCCESS));
    KInvariant(NT_SUCCESS(closeSync.WaitForCompletion()));
    KInvariant(testSvc->ExtendedCloseCtx == nullptr);

    KInvariant(testSvc->Status() == STATUS_SUCCESS);

    // Prove ref count
    KNt::Sleep(100);
    KInvariant(testSvc.Reset());

    // Prove simple open failure paths
    TestService::Create(testSvc, Allocator);
    KInvariant(testSvc->StartOpen(nullptr, openSync.OpenCompletionCallback()) == STATUS_PENDING);
    KInvariant(testSvc->OnServiceOpenCalled.WaitUntilSet());
    KInvariant(testSvc->CompleteOpen(STATUS_UNSUCCESSFUL));
    KInvariant(openSync.WaitForCompletion() == STATUS_UNSUCCESSFUL);

    KInvariant(testSvc->Status() == STATUS_UNSUCCESSFUL);

    // Prove ref count
    KNt::Sleep(100);
    KInvariant(testSvc.Reset());

    // Prove simple close failure paths
    TestService::Create(testSvc, Allocator);
    KInvariant(testSvc->StartOpen(nullptr, openSync.OpenCompletionCallback()) == STATUS_PENDING);
    KInvariant(testSvc->OnServiceOpenCalled.WaitUntilSet());
    KInvariant(testSvc->CompleteOpen(STATUS_SUCCESS));
    KInvariant(openSync.WaitForCompletion() == STATUS_SUCCESS);

    KInvariant(testSvc->Status() == STATUS_PENDING);

    KInvariant(testSvc->StartClose(nullptr, closeSync.CloseCompletionCallback()) == STATUS_PENDING);
    KInvariant(testSvc->OnServiceCloseCalled.WaitUntilSet());
    KInvariant(testSvc->CompleteClose(STATUS_UNSUCCESSFUL));
    KInvariant(closeSync.WaitForCompletion() == STATUS_UNSUCCESSFUL);

    KInvariant(testSvc->Status() == STATUS_UNSUCCESSFUL);

    // Prove ref count
    KNt::Sleep(100);
    KInvariant(testSvc.Reset());

    // Prove racing open and close works when no open error
    TestService::Create(testSvc, Allocator);
    KInvariant(testSvc->StartOpen(nullptr, openSync.OpenCompletionCallback()) == STATUS_PENDING);
    KInvariant(testSvc->StartClose(nullptr, closeSync.CloseCompletionCallback()) == STATUS_PENDING);

    KInvariant(testSvc->OnServiceOpenCalled.WaitUntilSet());
    KInvariant(testSvc->CompleteOpen(STATUS_SUCCESS));
    KInvariant(openSync.WaitForCompletion() == STATUS_SUCCESS);

    KInvariant(testSvc->Status() == STATUS_PENDING);

    KInvariant(testSvc->OnServiceCloseCalled.WaitUntilSet());
    KInvariant(testSvc->CompleteClose(STATUS_SUCCESS));
    KInvariant(closeSync.WaitForCompletion() == STATUS_SUCCESS);

    KInvariant(testSvc->Status() == STATUS_SUCCESS);

    // Prove ref count
    KNt::Sleep(100);
    KInvariant(testSvc.Reset());

    // Prove racing open and close works when with open error
    TestService::Create(testSvc, Allocator);
    KInvariant(testSvc->StartOpen(nullptr, openSync.OpenCompletionCallback()) == STATUS_PENDING);
    KInvariant(testSvc->StartClose(nullptr, closeSync.CloseCompletionCallback()) == STATUS_PENDING);

    KInvariant(testSvc->OnServiceOpenCalled.WaitUntilSet());
    KInvariant(testSvc->CompleteOpen(STATUS_UNSUCCESSFUL));
    KInvariant(openSync.WaitForCompletion() == STATUS_UNSUCCESSFUL);

    KInvariant(testSvc->Status() == STATUS_UNSUCCESSFUL);

    KInvariant(closeSync.WaitForCompletion() == STATUS_UNSUCCESSFUL);

    KInvariant(testSvc->Status() == STATUS_UNSUCCESSFUL);

    // Prove ref count
    KNt::Sleep(100);
    KInvariant(testSvc.Reset());

    // Prove deferred close behavior - Acquire races ahead of close
    TestService::Create(testSvc, Allocator);
    testSvc->SetDeferredCloseBehavior();
    KInvariant(testSvc->StartOpen(nullptr, openSync.OpenCompletionCallback()) == STATUS_PENDING);
    KInvariant(testSvc->OnServiceOpenCalled.WaitUntilSet());
    KInvariant(testSvc->CompleteOpen(STATUS_SUCCESS));
    KInvariant(openSync.WaitForCompletion() == STATUS_SUCCESS);

    KInvariant(testSvc->TryAcquireServiceActivity());       // Hold OnClose processing
    KInvariant(testSvc->TryAcquireServiceActivity());       // And again
    KInvariant(testSvc->StartClose(nullptr, closeSync.CloseCompletionCallback()) == STATUS_PENDING);
    KInvariant(testSvc->OnDeferredClosingCalled.WaitUntilSet(1000));
    KInvariant(!testSvc->OnServiceCloseCalled.WaitUntilSet(1000));
    KInvariant(!testSvc->ReleaseServiceActivity());         // Release OnClose processing
    KInvariant(testSvc->ReleaseServiceActivity());
    KInvariant(testSvc->OnServiceCloseCalled.WaitUntilSet());

    KInvariant(testSvc->CompleteClose(STATUS_SUCCESS));
    KInvariant(NT_SUCCESS(closeSync.WaitForCompletion()));

    // Prove deferred close behavior - Close races ahead of Acquire
    TestService::Create(testSvc, Allocator);
    testSvc->SetDeferredCloseBehavior();
    KInvariant(testSvc->StartOpen(nullptr, openSync.OpenCompletionCallback()) == STATUS_PENDING);
    KInvariant(testSvc->OnServiceOpenCalled.WaitUntilSet());
    KInvariant(testSvc->CompleteOpen(STATUS_SUCCESS));
    KInvariant(openSync.WaitForCompletion() == STATUS_SUCCESS);

    KInvariant(testSvc->IsOpen());
    KInvariant(testSvc->StartClose(nullptr, closeSync.CloseCompletionCallback()) == STATUS_PENDING);
    KInvariant(!testSvc->IsOpen());

    testSvc->_IsOpen = TRUE;            // Allow "racing but lagging close" code path to be excercised
    KInvariant(testSvc->IsOpen());
    KInvariant(testSvc->OnDeferredClosingCalled.WaitUntilSet(1000));
    KInvariant(testSvc->OnServiceCloseCalled.WaitUntilSet(1000));

    KInvariant(!testSvc->TryAcquireServiceActivity());       // Prove activity rejected
    testSvc->_IsOpen = FALSE;
    KInvariant(testSvc->CompleteClose(STATUS_SUCCESS));
    KInvariant(NT_SUCCESS(closeSync.WaitForCompletion()));

    #if DBG
        // prove spinning works in the Deferred Close logic
        ULONGLONG           closeWins = 0;
        ULONGLONG           acquireWins = 0;
        ULONGLONG           timeToStop = KNt::GetTickCount64() + (1000 * 60 * 10);      // 10 mins max
        BOOLEAN             timedOut = FALSE;
        ULONGLONG           passCount = 0;

        while (((closeWins < 5) || 
                (acquireWins < 5) ||
                (TestService::Test_TryAcquireServiceActivitySpinCount < 1) ||
                (TestService::Test_ScheduleOnServiceCloseSpinCount < 1) ||
                (TestService::Test_ReleaseServiceActivitySpinCount < 1))
                && !timedOut)
        {
            TestService::Create(testSvc, Allocator);
            testSvc->SetDeferredCloseBehavior();
            KInvariant(testSvc->StartOpen(nullptr, openSync.OpenCompletionCallback()) == STATUS_PENDING);
            KInvariant(testSvc->OnServiceOpenCalled.WaitUntilSet());
            KInvariant(testSvc->CompleteOpen(STATUS_SUCCESS));
            KInvariant(openSync.WaitForCompletion() == STATUS_SUCCESS);

            KInvariant(testSvc->StartClose(nullptr, closeSync.CloseCompletionCallback()) == STATUS_PENDING);
            if (passCount & 1)
            {
                // Random yield
                KNt::Sleep(0);
            }
            passCount++;
            testSvc->_IsOpen = TRUE;            // Allow "racing but lagging close" code path to be exercised
            MemoryBarrier();

            BOOLEAN     acquired = testSvc->TryAcquireServiceActivity();
            testSvc->_IsOpen = FALSE;
            MemoryBarrier();

            if (acquired)
            {
                // Acquire raced ahead
                testSvc->ReleaseServiceActivity();
                acquireWins++;
            }
            else
            {
                closeWins++;
            }
            KInvariant(testSvc->OnServiceCloseCalled.WaitUntilSet(1000));

            KInvariant(testSvc->CompleteClose(STATUS_SUCCESS));
            KInvariant(NT_SUCCESS(closeSync.WaitForCompletion()));

            timedOut = KNt::GetTickCount64() >= timeToStop;
        }

        if (timedOut)
        {
            KTestPrintf("WARNING.  Test was not able to hit enough contention cases after 10 minutes to fully validate deferred close.\n");
            KTestPrintf("  Close wins= %d\n", closeWins);
            KTestPrintf("  Acquire wins= %d\n", acquireWins);
            KTestPrintf("  TryAcquireServiceActivity spin count= %d\n", TestService::Test_TryAcquireServiceActivitySpinCount);
            KTestPrintf("  ScheduleOnServiceClose spin count= %d\n", TestService::Test_ScheduleOnServiceCloseSpinCount);
            KTestPrintf("  ReleaseServiceActivity spin count= %d\n", TestService::Test_ReleaseServiceActivitySpinCount);

            // Verify we hit contention at least once in each method
            KInvariant((TestService::Test_TryAcquireServiceActivitySpinCount > 0) && 
                       (TestService::Test_ScheduleOnServiceCloseSpinCount > 0) &&
                       (TestService::Test_ReleaseServiceActivitySpinCount > 0));
        }
    #endif

    return STATUS_SUCCESS;
}


NTSTATUS
KAsyncServiceTest(__in int argc, __in_ecount(argc) WCHAR* args[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    KTestPrintf("KAsyncServiceTest: Starting\n");

    EventRegisterMicrosoft_Windows_KTL();
    KFinally([]()
    {
        EventUnregisterMicrosoft_Windows_KTL();
    });

    KtlSystem*      ourSys = nullptr;

    NTSTATUS status = KtlSystem::Initialize(&ourSys);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("KAsyncServiceTest: KtlSystem::Initialize Failed: %i\n", __LINE__);
        return status;
    }

    KFinally([]()
    {
        KtlSystem::Shutdown();
    });

#if KTL_USER_MODE
    ((KtlSystemBase*)ourSys)->SetDefaultSystemThreadPoolUsage(FALSE);
#else
    ourSys;
#endif

    status = DoBasicServiceTest(KtlSystem::GlobalNonPagedAllocator());
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("KAsyncServiceTest Unit Test: FAILED: 0X%X\n", status);
    }

    KTestPrintf("KAsyncServiceTest: Completed\n");
    return status;
}


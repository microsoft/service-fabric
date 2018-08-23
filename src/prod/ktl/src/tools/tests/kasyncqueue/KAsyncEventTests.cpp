/*++

Copyright (c) Microsoft Corporation

Module Name:

    KAsyncEventTests.cpp

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


class CountingCompletionEvent : public KEvent
{
public:
    CountingCompletionEvent(
        __in LONG CallbackCount,
        __in BOOLEAN IsManualReset = FALSE,
        __in BOOLEAN InitialState = FALSE)
        :   _RemainingCallbacks(CallbackCount),
            _Status(STATUS_SUCCESS),
            KEvent(IsManualReset, InitialState)

    {
    }

    void
    Reset(LONG CallbackCount)
    {
        KAssert(CallbackCount >= 0);
        _RemainingCallbacks = CallbackCount;
        _Status = STATUS_SUCCESS;
    }

    void
    EventCallback(
        KAsyncContextBase* const  Parent,
        KAsyncContextBase& Context)
    {
        UNREFERENCED_PARAMETER(Parent);

        NTSTATUS status = Context.Status();
        InterlockedCompareExchange(&_Status, status, STATUS_SUCCESS);
        if (InterlockedDecrement(&_RemainingCallbacks) == 0)
        {
            SetEvent();
        }
    }

    NTSTATUS
    WaitUntilSet()
    {
        KEvent::WaitUntilSet();
        return(_Status);
    }

    NTSTATUS volatile   _Status;
    LONG volatile       _RemainingCallbacks;
};







NTSTATUS
DoBasicKAsyncTest(KAllocator& Allocator)
{
    KTestPrintf("KAsyncEventTest: DoBasicKAsyncTest Starting\n");

    // Prove basic automatic single producer and single waiter behavior
    {
        CountingCompletionEvent                 waitOpsDoneEvent(1);
        KAsyncEvent                             testKAsyncEvent;
        KAsyncEvent::WaitContext::SPtr          waitContext;
        NTSTATUS                                status;
        KAsyncContextBase::CompletionCallback   testCallback(&waitOpsDoneEvent, &CountingCompletionEvent::EventCallback);

        status = testKAsyncEvent.CreateWaitContext(KTL_TAG_TEST, Allocator, waitContext);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("KAsyncEventTest: CreateWaitContext() Failed: %i\n", __LINE__);
            return status;
        }

        // Post the async wait callback and guarentee that the wait will be suspened
        waitContext->StartWaitUntilSet(nullptr, testCallback);

        KNt::Sleep(1000);
        if (waitOpsDoneEvent._RemainingCallbacks != 1)
        {
            KTestPrintf("KAsyncEventTest: unexpected _RemainingCallbacks count: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        if (testKAsyncEvent.IsSignaled() || (testKAsyncEvent.CountOfWaiters() != 1))
        {
            KTestPrintf("KAsyncEventTest: testKAsyncEvent in unexpected state: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        // Prove we can cancel a wait
        if (waitContext->Status() != STATUS_PENDING)
        {
            KTestPrintf("KAsyncEventTest: waitContext in unexpected state: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        if (!waitContext->Cancel())
        {
            KTestPrintf("KAsyncEventTest: waitContext.Cancel() returned an unexpected value: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        status = waitOpsDoneEvent.WaitUntilSet();
        if (status != STATUS_CANCELLED)
        {
            KTestPrintf("KAsyncEventTest: WaitUntilSet() returned an unexpected status: %i\n", __LINE__);
            return status;
        }

        // Now prove that post-wait-signaling the event work
        waitOpsDoneEvent.Reset(1);
        waitContext->Reuse();

        if (testKAsyncEvent.IsSignaled() || (testKAsyncEvent.CountOfWaiters() != 0))
        {
            KTestPrintf("KAsyncEventTest: testKAsyncEvent in unexpected state: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        waitContext->StartWaitUntilSet(nullptr, testCallback);
        while (testKAsyncEvent.CountOfWaiters() != 1)
        {
            KNt::Sleep(0);
        }

        testKAsyncEvent.SetEvent();
        KNt::Sleep(1000);
        if (waitOpsDoneEvent._RemainingCallbacks != 0)
        {
            KTestPrintf("KAsyncEventTest: unexpected _RemainingCallbacks count: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        status = waitOpsDoneEvent.WaitUntilSet();
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("KAsyncEventTest: WaitUntilSet() Failed: %i\n", __LINE__);
            return status;
        }

        // Now prove that pre-wait-signaling the event work
        waitOpsDoneEvent.Reset(1);
        waitContext->Reuse();

        if (testKAsyncEvent.IsSignaled() || (testKAsyncEvent.CountOfWaiters() != 0))
        {
            KTestPrintf("KAsyncEventTest: testKAsyncEvent in unexpected state: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        testKAsyncEvent.SetEvent();
        if (!testKAsyncEvent.IsSignaled() || (testKAsyncEvent.CountOfWaiters() != 0))
        {
            KTestPrintf("KAsyncEventTest: testKAsyncEvent in unexpected state: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        waitContext->StartWaitUntilSet(nullptr, testCallback);
        status = waitOpsDoneEvent.WaitUntilSet();
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("KAsyncEventTest: WaitUntilSet() Failed: %i\n", __LINE__);
            return status;
        }

        if (testKAsyncEvent.IsSignaled() || (testKAsyncEvent.CountOfWaiters() != 0))
        {
            KTestPrintf("KAsyncEventTest: testKAsyncEvent in unexpected state: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        // Prove multiple waiters (many) and signalers async to each other - mini stress test
        {
            int const               NumberOfWaiters = 100000;

            waitOpsDoneEvent.Reset(NumberOfWaiters);
            for (int i = 0; i < NumberOfWaiters; i++)
            {
                KAsyncEvent::WaitContext::SPtr          tempContext;
                status = testKAsyncEvent.CreateWaitContext(KTL_TAG_TEST, Allocator, tempContext);
                if (!NT_SUCCESS(status))
                {
                    KTestPrintf("KAsyncEventTest: CreateWaitContext() Failed: %i\n", __LINE__);
                    return status;
                }

                // Post the async wait callback
                tempContext->StartWaitUntilSet(nullptr, testCallback);
            }

            while (testKAsyncEvent.CountOfWaiters() < NumberOfWaiters)
            {
                KNt::Sleep(0);
            }

            if (testKAsyncEvent.IsSignaled())
            {
                KTestPrintf("KAsyncEventTest: testKAsyncEvent in unexpected state: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            ULONG waiterCount;
            while ((waiterCount = testKAsyncEvent.CountOfWaiters()) > 0)
            {
                // Let one waiter go and then wait for it (being dispatched) to be reflected in CountOfWaiters()
                waiterCount--;
                testKAsyncEvent.SetEvent();
                while (testKAsyncEvent.CountOfWaiters() != waiterCount)
                {
                    KNt::Sleep(0);
                }
            }

            if (testKAsyncEvent.IsSignaled() || (testKAsyncEvent.CountOfWaiters() != 0))
            {
                KTestPrintf("KAsyncEventTest: testKAsyncEvent in unexpected state: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            // Wait for all to complete thru waitOpsDoneEvent
            status = waitOpsDoneEvent.WaitUntilSet();
            if (!NT_SUCCESS(status))
            {
                KTestPrintf("KAsyncEventTest: WaitUntilSet() returned failure: %i\n", __LINE__);
                return status;
            }
        }

        // Test Manual Reset
        {
            int const               NumberOfWaiters = 100000;

            waitOpsDoneEvent.Reset(NumberOfWaiters);
            testKAsyncEvent.ChangeMode(TRUE);           // Change to Manual event reset

            if (testKAsyncEvent.IsSignaled())
            {
                KTestPrintf("KAsyncEventTest: testKAsyncEvent in unexpected state: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            for (int i = 0; i < NumberOfWaiters; i++)
            {
                KAsyncEvent::WaitContext::SPtr          tempContext;
                status = testKAsyncEvent.CreateWaitContext(KTL_TAG_TEST, Allocator, tempContext);
                if (!NT_SUCCESS(status))
                {
                    KTestPrintf("KAsyncEventTest: CreateWaitContext() Failed: %i\n", __LINE__);
                    return status;
                }

                // Post the async wait callback
                tempContext->StartWaitUntilSet(nullptr, testCallback);
            }

            if (testKAsyncEvent.IsSignaled())
            {
                KTestPrintf("KAsyncEventTest: testKAsyncEvent in unexpected state: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            // Let them all go!
            testKAsyncEvent.SetEvent();

            // Wait for all to complete thru waitOpsDoneEvent
            status = waitOpsDoneEvent.WaitUntilSet();
            if (!NT_SUCCESS(status))
            {
                KTestPrintf("KAsyncEventTest: WaitUntilSet() returned failure: %i\n", __LINE__);
                return status;
            }

            if (!testKAsyncEvent.IsSignaled() || (testKAsyncEvent.CountOfWaiters() != 0))
            {
                KTestPrintf("KAsyncEventTest: testKAsyncEvent in unexpected state: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            testKAsyncEvent.ResetEvent();
            if (testKAsyncEvent.IsSignaled() || (testKAsyncEvent.CountOfWaiters() != 0))
            {
                KTestPrintf("KAsyncEventTest: testKAsyncEvent in unexpected state: %i\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            testKAsyncEvent.ChangeMode(FALSE);
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
KAsyncEventTest(__in int argc, __in_ecount(argc) WCHAR* args[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    KTestPrintf("KAsyncEventTest: Starting\n");

    EventRegisterMicrosoft_Windows_KTL();
    KFinally([]()
    {
        EventUnregisterMicrosoft_Windows_KTL();
    });

    NTSTATUS status = KtlSystem::Initialize();
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("KAsyncEventTest: KtlSystem::Initialize Failed: %i\n", __LINE__);
        return status;
    }

    KFinally([]()
    {
        KtlSystem::Shutdown();
    });

    status = DoBasicKAsyncTest(KtlSystem::GlobalNonPagedAllocator());
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("KAsyncEventTest Unit Test: FAILED: 0X%X\n", status);
    }

    KTestPrintf("KAsyncEventTest: Completed\n");
    return status;
}


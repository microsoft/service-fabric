/*++

Copyright (c) Microsoft Corporation

Module Name:

    KQuotaGateTests.cpp

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
SimpleQuotaGateTest(KtlSystem& KtlSystem)
{
    {
        KQuotaGate::SPtr                        testGate;
        NTSTATUS                                status;
        ULONGLONG const                         initialGateQuanta = 100000;
        CountingCompletionEvent                 deactivationEvent(1);
        KAsyncContextBase::CompletionCallback   testActivationCallback(&deactivationEvent, &CountingCompletionEvent::EventCallback);

        KtlSystem; //  Avoid Error C4100 (Unreferenced formal parameter) - for some reason the next line does not prevent it
        status = KQuotaGate::Create(KTL_TAG_TEST, KtlSystem.GlobalNonPagedAllocator(), testGate);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf(
                "KQuotaGateTest:SimpleQuotaGateTest: KQuotaGate::Create() failed with: 0x%08X; at line %u\n",
                status,
                __LINE__);

            return status;
        }

        //* Prove basic activate and deactivation works
        status = testGate->Activate(0, nullptr, testActivationCallback);
        if (!K_ASYNC_SUCCESS(status))
        {
            KTestPrintf(
                "KQuotaGateTest:SimpleQuotaGateTest: Activate() failed with: 0x%08X; at line %u\n",
                status,
                __LINE__);

            return status;
        }

        testGate->Deactivate();
        status = deactivationEvent.WaitUntilSet();
        if (!NT_SUCCESS(status))
        {
            KTestPrintf(
                "KQuotaGateTest:SimpleQuotaGateTest: Deactivate() failed with: 0x%08X; at line %u\n",
                status,
                __LINE__);

            return status;
        }

        //* Prove proper deactivation when there are queued acquires; also prove cancel
        testGate->Reuse();
        deactivationEvent.Reset(1);
        status = testGate->Activate(0, nullptr, testActivationCallback);
        if (!K_ASYNC_SUCCESS(status))
        {
            KTestPrintf(
                "KQuotaGateTest:SimpleQuotaGateTest: Activate() failed with: 0x%08X; at line %u\n",
                status,
                __LINE__);

            return status;
        }

        CountingCompletionEvent                 shutdownResultTestEvent(1);
        KAsyncContextBase::CompletionCallback   testShutdownCallback(&shutdownResultTestEvent, &CountingCompletionEvent::EventCallback);
        KQuotaGate::AcquireContext::SPtr        acquireContext;

        status = testGate->CreateAcquireContext(acquireContext);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf(
                "KQuotaGateTest:SimpleQuotaGateTest: CreateAcquireContext() failed with: 0x%08X; at line %u\n",
                status,
                __LINE__);

            return status;
        }

        // Prove cancel
        acquireContext->StartAcquire(1, nullptr, testShutdownCallback);
        acquireContext->Cancel();
        status = shutdownResultTestEvent.WaitUntilSet();
        if (status != STATUS_CANCELLED)
        {
            KTestPrintf(
                "KQuotaGateTest:SimpleQuotaGateTest: StartAcquire() completed with unexpected status: 0x%08X; at line %u\n",
                status,
                __LINE__);

            return STATUS_UNSUCCESSFUL;
        }

        // Prove deactivation causes pending waits to complete with K_STATUS_SHUTDOWN_PENDING
        acquireContext->Reuse();
        shutdownResultTestEvent.Reset(1);
        acquireContext->StartAcquire(1, nullptr, testShutdownCallback);
        KNt::Sleep(200);        // Allow the above StartAcquire to get queued

        testGate->Deactivate();
        status = deactivationEvent.WaitUntilSet();
        if (!NT_SUCCESS(status))
        {
            KTestPrintf(
                "KQuotaGateTest:SimpleQuotaGateTest: Deactivate() failed with: 0x%08X; at line %u\n",
                status,
                __LINE__);

            return status;
        }

        status = shutdownResultTestEvent.WaitUntilSet();
        if (status != K_STATUS_SHUTDOWN_PENDING)
        {
            KTestPrintf(
                "KQuotaGateTest:SimpleQuotaGateTest: StartAcquire() completed with unexpected status: 0x%08X; at line %u\n",
                status,
                __LINE__);

            return STATUS_UNSUCCESSFUL;
        }

        //* Prove that all signals are received via acquires
        CountingCompletionEvent                 acquireTestResultTestEvent(initialGateQuanta);
        KAsyncContextBase::CompletionCallback   testAcquireCallback(&acquireTestResultTestEvent, &CountingCompletionEvent::EventCallback);

        testGate->Reuse();
        deactivationEvent.Reset(1);
        status = testGate->Activate(0, nullptr, testActivationCallback);
        if (!K_ASYNC_SUCCESS(status))
        {
            KTestPrintf(
                "KQuotaGateTest:SimpleQuotaGateTest: Activate() failed with: 0x%08X; at line %u\n",
                status,
                __LINE__);

            return status;
        }

        for (ULONG i = 0; i < initialGateQuanta; i++)
        {
            KQuotaGate::AcquireContext::SPtr        acquireContext1;

            status = testGate->CreateAcquireContext(acquireContext1);
            if (!NT_SUCCESS(status))
            {
                KTestPrintf(
                    "KQuotaGateTest:SimpleQuotaGateTest: CreateAcquireContext() failed with: 0x%08X; at line %u\n",
                    status,
                    __LINE__);

                return status;
            }

            acquireContext1->StartAcquire(1, nullptr, testAcquireCallback);
        }

        // Prove no StartAcquire()s complete
        KNt::Sleep(1000);
        if (acquireTestResultTestEvent._RemainingCallbacks != initialGateQuanta)
        {
            KTestPrintf(
                "KQuotaGateTest:SimpleQuotaGateTest: StartAcquire() completion unexpected at line %u\n",
                __LINE__);

            return STATUS_UNSUCCESSFUL;
        }

        // Signal all of the waiting Acquires - should all complete
        testGate->ReleaseQuanta(initialGateQuanta);
        status = acquireTestResultTestEvent.WaitUntilSet();
        if (!NT_SUCCESS(status))
        {
            KTestPrintf(
                "KQuotaGateTest:SimpleQuotaGateTest: StartAcquire() completed with unexpected status: 0x%08X; at line %u\n",
                status,
                __LINE__);

            return STATUS_UNSUCCESSFUL;
        }

        //* Prove that pre-signalling works
        status = testGate->CreateAcquireContext(acquireContext);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf(
                "KQuotaGateTest:SimpleQuotaGateTest: CreateAcquireContext() failed with: 0x%08X; at line %u\n",
                status,
                __LINE__);

            return status;
        }

        acquireTestResultTestEvent.Reset(1);
        testGate->ReleaseQuanta(1);
        acquireContext->StartAcquire(1, nullptr, testAcquireCallback);
        status = acquireTestResultTestEvent.WaitUntilSet();
        if (!NT_SUCCESS(status))
        {
            KTestPrintf(
                "KQuotaGateTest:SimpleQuotaGateTest: StartAcquire() completed with unexpected status: 0x%08X; at line %u\n",
                status,
                __LINE__);

            return STATUS_UNSUCCESSFUL;
        }

        testGate->Deactivate();
        status = deactivationEvent.WaitUntilSet();
        if (!NT_SUCCESS(status))
        {
            KTestPrintf(
                "KQuotaGateTest:SimpleQuotaGateTest: Deactivate() failed with: 0x%08X; at line %u\n",
                status,
                __LINE__);

            return status;
        }
    }

    #if defined(K_UseResumable)
    {
        KCoQuotaGate::SPtr                      testGate;
        NTSTATUS                                status;
        ULONGLONG const                         initialGateQuanta = 100000;

        status = KCoQuotaGate::Create(KTL_TAG_TEST, KtlSystem.GlobalNonPagedAllocator(), testGate);
        KInvariant(NT_SUCCESS(status));

        //* Prove basic activate and deactivation works
        status = ktl::SyncAwait(testGate->ActivateAsync(0));
        KInvariant(K_ASYNC_SUCCESS(status));

        ktl::SyncAwait(testGate->DeactivateAsync());

        //* Prove proper deactivation when there are queued acquires; also prove cancel
        testGate->Reuse();
        status = ktl::SyncAwait(testGate->ActivateAsync(0));
        KInvariant(K_ASYNC_SUCCESS(status));

        CountingCompletionEvent                 shutdownResultTestEvent(1);
        KAsyncContextBase::CompletionCallback   testShutdownCallback(&shutdownResultTestEvent, &CountingCompletionEvent::EventCallback);
        KCoQuotaGate::AcquireContext::SPtr      acquireContext;

        status = testGate->CreateAcquireContext(acquireContext);
        KInvariant(NT_SUCCESS(status));

        // Prove cancel
        acquireContext->StartAcquire(1, nullptr, testShutdownCallback);
        acquireContext->Cancel();
        status = shutdownResultTestEvent.WaitUntilSet();
        KInvariant(status == STATUS_CANCELLED);

        // Prove deactivation causes pending waits to complete with K_STATUS_SHUTDOWN_PENDING
        acquireContext->Reuse();
        shutdownResultTestEvent.Reset(1);
        acquireContext->StartAcquire(1, nullptr, testShutdownCallback);
        KNt::Sleep(200);        // Allow the above StartAcquire to get queued

        ktl::SyncAwait(testGate->DeactivateAsync());

        status = shutdownResultTestEvent.WaitUntilSet();
        KInvariant(status == K_STATUS_SHUTDOWN_PENDING);
    }
    #endif

    return STATUS_SUCCESS;
}

NTSTATUS
KQuotaGateTest(int argc, WCHAR* args[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    NTSTATUS status = STATUS_SUCCESS;

    EventRegisterMicrosoft_Windows_KTL();
    KTestPrintf("Starting KQuotaGateTest test\n");

    KtlSystem*  defaultSys;
    KtlSystem::Initialize(&defaultSys);
    defaultSys->SetStrictAllocationChecks(TRUE);
	
    status = SimpleQuotaGateTest(*defaultSys);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("KQuotaGateTest: SimpleQuotaGateTest() failed with: 0x%08X; at line %u\n", status, __LINE__);
    }
    else
    {
        KTestPrintf("KQuotaGateTest tests passed\n");
    }

    KtlSystem::Shutdown();

    EventUnregisterMicrosoft_Windows_KTL();
    return status;
}


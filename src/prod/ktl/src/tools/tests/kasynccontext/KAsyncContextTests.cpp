/*++

Copyright (c) Microsoft Corporation

Module Name:

    KAsyncContextTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KAsyncContextTests.h.
    2. Add an entry to the gs_KuTestCases table in KAsyncContextTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
        this file or any other file.

--*/
#pragma once

#include "KAsyncContextTests.h"
#include <CommandLineParser.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(PLATFORM_UNIX)
#include <vector>
#endif
#include <ktl.h>
#include <ktrace.h>

#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif


//**** Common support
VOID
PauseThread(ULONG MSecs)
{
#if KTL_USER_MODE
    Sleep(MSecs);
#else
    LARGE_INTEGER ticksToWait;
    ticksToWait.QuadPart = (ULONGLONG)MSecs * -1 * (LONGLONG)10000;     // To100NanoTicksFromMSecs()
    KeDelayExecutionThread(KernelMode, FALSE, &ticksToWait);
#endif
}


//**** KAsyncLock Tests
VOID
StaticHandleAcquireTestCallback(
    PVOID ThisPtr,
    KAsyncContextBase* const Parent,
    KAsyncContextBase& CompletedContext)
{
    UNREFERENCED_PARAMETER(CompletedContext);

    KEvent* eventPtr = static_cast<KEvent*>(ThisPtr);
    KInvariant(Parent == nullptr);
    eventPtr->SetEvent();
}

//** Entry for KAsyncLock::Handle testing
NTSTATUS
BasicKAsyncLockTest()
{
    // Keep these declared in this order as dtor's are called in the reverse order
    // See NOTE at end of the function.
    KSharedAsyncLock::Handle::SPtr  testHandle1;
    KSharedAsyncLock::Handle::SPtr  testHandle2;
    KSharedAsyncLock::SPtr          testLock;
    NTSTATUS                        status;

    status = KSharedAsyncLock::Create(KTL_TAG_BASE, KtlSystem::GlobalNonPagedAllocator(), testLock);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("BasicKAsyncLockTest: %i: KSharedAsyncLock::Create failed\n", __LINE__);
        return status;
    }

    {
        KEvent                                  testHandle1Event(FALSE, FALSE);
        KEvent                                  testHandle2Event(FALSE, FALSE);
        KAsyncContextBase::CompletionCallback   handle1Callback;
        KAsyncContextBase::CompletionCallback   handle2Callback;

        handle1Callback.Bind(&testHandle1Event, &StaticHandleAcquireTestCallback);
        handle2Callback.Bind(&testHandle2Event, &StaticHandleAcquireTestCallback);

        status = testLock->CreateHandle(KTL_TAG_BASE, testHandle1);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("BasicKAsyncLockTest: %i: CreateHandle failed\n", __LINE__);
            return status;
        }

        status = testLock->CreateHandle(KTL_TAG_BASE, testHandle2);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("BasicKAsyncLockTest: %i: CreateHandle failed\n", __LINE__);
            return status;
        }

        // Prove simple acquire works - should complete with successful acquire
        testHandle1->StartAcquireLock(nullptr, handle1Callback);
        testHandle1Event.WaitUntilSet();
        if (testHandle1->Status() != STATUS_SUCCESS)
        {
            KTestPrintf("BasicKAsyncLockTest: %i: unexpected status\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        // 2nd acquire should stay pending until first is released
        testHandle2->StartAcquireLock(nullptr, handle2Callback);
        PauseThread(1500);
        if (testHandle2->Status() != STATUS_PENDING)
        {
            KTestPrintf("BasicKAsyncLockTest: %i: unexpected status\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        testHandle1->ReleaseLock();
        testHandle2Event.WaitUntilSet();

        // Handle2 should now own the lock
        if (testHandle2->Status() != STATUS_SUCCESS)
        {
            KTestPrintf("BasicKAsyncLockTest: %i: unexpected status\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        // Cancel test
        testHandle1->Reuse();
        testHandle1->StartAcquireLock(nullptr, handle1Callback);
        PauseThread(1500);
        if (testHandle1->Status() != STATUS_PENDING)
        {
            KTestPrintf("BasicKAsyncLockTest: %i: unexpected status\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        testHandle1->Cancel();
        testHandle1Event.WaitUntilSet();
        if (testHandle1->Status() != STATUS_CANCELLED)
        {
            KTestPrintf("BasicKAsyncLockTest: %i: unexpected status\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }
        PauseThread(1500);              // make sure the SM for testHandle1 has had time to release its self ref
                                        // See #1 below


        // Final release and destruction test.
        testHandle2->ReleaseLock();
        if (!testHandle1.Reset())       // #1
        {
            KTestPrintf("BasicKAsyncLockTest: %i: expected destruction did not occur\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }
        if (!testHandle2.Reset())
        {
            KTestPrintf("BasicKAsyncLockTest: %i: expected destruction did not occur\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }
    }

    // Prove that testLock reset causes destruction
    if (!testLock.Reset())
    {
        KTestPrintf("BasicKAsyncLockTest: %i: expected destruction did not occur\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

#if ! KTL_USER_MODE
//** Entry for KAsyncContext DPC testing

NTSTATUS VerifyIRQL(
    KIRQL ExpectedIrql
    )
{
    KIRQL actualIrql;

    actualIrql = KeGetCurrentIrql();
    if (actualIrql != ExpectedIrql)
    {
        KTestPrintf("Expected irql %d, current IRQL is %d\n", ExpectedIrql, actualIrql);

        KInvariant(FALSE);
        return(STATUS_UNSUCCESSFUL);
    }

    return(STATUS_SUCCESS);
}

class MySynchronizer;

class DPCAsyncContext : public KAsyncContextBase
{
    K_FORCE_SHARED(DPCAsyncContext);

    friend MySynchronizer;

    public:
        typedef enum {
            Unassigned,
            CompleteImmediately,
            WaitForLater,
            CompleteParentAsync,
            CompleteOnCancel,
            StartChildAndWait
        } DPCAsyncMode;

        DPCAsyncMode GetMode()
        {
            return(_Mode);
        }

        static NTSTATUS Create(
            PWCHAR Name,
            DPCAsyncContext::SPtr& Context
            );

        VOID SetExpectDPC(
            BOOLEAN ExpectDPC
            )
        {
            _ExpectDPC = ExpectDPC;
        }

        VOID SetMode(DPCAsyncMode Mode)
        {
            _Mode = Mode;
        }

        VOID StartMe(
            DPCAsyncMode Mode,
            DPCAsyncContext* const Child,
            KAsyncContextBase::CompletionCallback ChildCallback,
            DPCAsyncContext* const Parent,
            KAsyncContextBase::CompletionCallback Callback
            )
        {
            _Mode = Mode;
            _Child = Child;
            _ChildCallback = ChildCallback;
            _Parent = Parent;
            Start((KAsyncContextBase *const)Parent, Callback);
        }

        VOID CompleteMe(
            NTSTATUS Status
            )
        {
            Complete(Status);
        }

        VOID CompleteParent(
            NTSTATUS Status
            )
        {
            _Parent->CompleteMe(Status);
        }

    protected:
        NTSTATUS VerifyAtDPC();  // TODO: delme

        VOID OnStart()
        {
            KTestPrintf("OnStart %p at %d\n", this, __LINE__);

            VerifyIRQL(_ExpectDPC ? 2 : 0);

            switch(_Mode)
            {
                case CompleteImmediately:
                {
                    KTestPrintf("Complete %p at %d\n", this, __LINE__);
                    Complete(STATUS_SUCCESS);
                    break;
                }

                case CompleteParentAsync:
                {
                    KTestPrintf("CompleteParent %p %p at %d\n", this, _Parent, __LINE__);
                    _Parent->Complete(STATUS_SUCCESS);
                    break;
                }

                case StartChildAndWait:
                {
                    KTestPrintf("StartChildAndWait %p start %p at %d\n", this, _Child, __LINE__);
                    _Child->Start(this, _ChildCallback);
                    break;
                }

                default:
                {
                    KTestPrintf("Do not complete Mode %d at %d\n", _Mode, __LINE__);
                    break;
                }
            }
        }

        VOID OnCompleted()
        {
            KTestPrintf("OnCompleted %p at %d\n", this, __LINE__);

            VerifyIRQL(_ExpectDPC ? 2 : 0);
        }

        VOID OnReuse()
        {
            KTestPrintf("OnReuse %p at %d\n", this, __LINE__);
            _Mode = Unassigned;
        }

        VOID OnCancel()
        {
            KTestPrintf("OnCancel %p at %d\n", this, __LINE__);

            VerifyIRQL(_ExpectDPC ? 2 : 0);

            if (_Mode == DPCAsyncContext::DPCAsyncMode::CompleteOnCancel)
            {
                KTestPrintf("CompleteOnCancel %p at %d\n", this, __LINE__);
                Complete(STATUS_CANCELLED);
            }
        }

    private:

    private:
        PWCHAR _Name;
        BOOLEAN _ExpectDPC;
        DPCAsyncMode _Mode;
        DPCAsyncContext* _Child;
        KAsyncContextBase::CompletionCallback _ChildCallback;
        DPCAsyncContext* _Parent;
};

class MySynchronizer : public KSynchronizer
{
    public:
        VOID SetExpectDPC(BOOLEAN ExpectDPC)
        {
            _ExpectDPC = ExpectDPC;
        }

        VOID SetDPCAsyncContext(DPCAsyncContext* Context)
        {
            _Context = Context;
        }

        VOID SetAssociatedDPCAsyncContext(DPCAsyncContext* Context)
        {
            _AssociatedContext = Context;
        }

    protected:
        VOID
        OnCompletion(
            __in_opt KAsyncContextBase* const Parent,
            __in KAsyncContextBase& CompletingOperation
            ) override
        {
            UNREFERENCED_PARAMETER(Parent);
            UNREFERENCED_PARAMETER(CompletingOperation);

            KTestPrintf("CompletionCallback %p at %d\n", _AssociatedContext, __LINE__);

            VerifyIRQL(_ExpectDPC ? 2 : 0);

            if (_Context)
            {
                KTestPrintf("CompletionCallback %p completing %p at %d\n", _AssociatedContext,
                     _Context,
                    __LINE__);
                _Context->CompleteMe(STATUS_SUCCESS);
            }

            _Context = NULL;
        }

    private:
        BOOLEAN _ExpectDPC;
        DPCAsyncContext* _AssociatedContext;
        DPCAsyncContext* _Context;
};


DPCAsyncContext::DPCAsyncContext()
{
}

DPCAsyncContext::~DPCAsyncContext()
{
}

NTSTATUS DPCAsyncContext::Create(
    PWCHAR Name,
    DPCAsyncContext::SPtr& Context
    )
{
    NTSTATUS status;
    DPCAsyncContext::SPtr context;

    context = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) DPCAsyncContext();
    if (context == nullptr)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    status = context->SetDispatchOnDPCThread(TRUE);
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("SetDispatchOnDPCThread failed %x\n", status);
        return(status);
    }

    context->_Name = Name;

    Context.Attach(context.Detach());
    return(STATUS_SUCCESS);
}




NTSTATUS
DPCDispatchTest()
{
    NTSTATUS status;
    DPCAsyncContext::SPtr context;
    DPCAsyncContext::SPtr parentContext;
    MySynchronizer sync, syncParent;

    status = DPCAsyncContext::Create(L"Child", context);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    status = DPCAsyncContext::Create(L"Parent", parentContext);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    sync.SetAssociatedDPCAsyncContext(context.RawPtr());
    syncParent.SetAssociatedDPCAsyncContext(parentContext.RawPtr());

    //
    // Test 1: Single Async lifecycle
    //

    //     Start Async
    //     Verify OnStart() is at DPC
    //     Complete Async in OnStart()
    //     Verify OnCompleted() is at DPC
    //     Verify Completion Callback is at DPC
    //
    KTestPrintf("Test 1.1\n");
    sync.SetExpectDPC(TRUE);
    sync.SetDPCAsyncContext(NULL);

    status = context->SetDispatchOnDPCThread(TRUE);
    context->SetExpectDPC(TRUE);

    context->StartMe(DPCAsyncContext::DPCAsyncMode::CompleteImmediately, NULL, NULL, NULL, sync);
    status = sync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("CompleteImmediately failed %x\n", status);
        return(status);
    }

    //     StartAsync
    //     Verify OnStart() is at DPC
    //     Cancel Async in main thread
    //     Verify OnCancel is at DPC
    //     Complete Async in OnCancel()
    //     Verify OnCompleted is at DPC
    //     Verify Completion Callback is at DPC
    //
    KTestPrintf("Test 1.2\n");
    sync.SetExpectDPC(TRUE);
    sync.SetDPCAsyncContext(NULL);

    context->Reuse();
    status = context->SetDispatchOnDPCThread(TRUE);
    context->SetExpectDPC(TRUE);

    context->StartMe(DPCAsyncContext::DPCAsyncMode::WaitForLater, NULL, NULL, NULL, sync);
    context->SetMode(DPCAsyncContext::DPCAsyncMode::CompleteOnCancel);
    context->Cancel();
    status = sync.WaitForCompletion();

    if (status != STATUS_CANCELLED)
    {
        KTestPrintf("WaitForLater failed %x\n", status);
        return(status);
    }

    //     StartAsync
    //     Verify OnStart() is at DPC
    //     Cancel Async in main thread
    //     Verify OnCancel is at DPC
    //     Complete Async in main thread
    //     Verify OnCompleted is at DPC
    //     Verify Completion Callback is at DPC
    //
    KTestPrintf("Test 1.3\n");

    sync.SetExpectDPC(TRUE);
    sync.SetDPCAsyncContext(NULL);

    context->Reuse();
    status = context->SetDispatchOnDPCThread(TRUE);
    context->SetExpectDPC(TRUE);

    context->StartMe(DPCAsyncContext::DPCAsyncMode::WaitForLater, NULL, NULL, NULL, sync);
    context->Cancel();
    context->CompleteMe(STATUS_CANCELLED);
    status = sync.WaitForCompletion();

    if (status != STATUS_CANCELLED)
    {
        KTestPrintf("WaitForLater failed %x\n", status);
        return(status);
    }

    // Test 1.4: Start DPC async from DPC
    //     Start Async
    //     Verify OnStart() is at DPC
    //     Complete Async in OnStart()
    //     Verify OnCompleted() is at DPC
    //     Verify Completion Callback is at DPC
    //
    KTestPrintf("Test 1.4\n");
    sync.SetExpectDPC(TRUE);
    sync.SetDPCAsyncContext(NULL);

    context->Reuse();
    status = context->SetDispatchOnDPCThread(TRUE);
    context->SetExpectDPC(TRUE);

    KIRQL oldIrql;
    oldIrql = KeRaiseIrqlToDpcLevel();

    context->StartMe(DPCAsyncContext::DPCAsyncMode::CompleteImmediately, NULL, NULL, NULL, sync);
    status = sync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("CompleteImmediately failed %x at %d\n", status, __LINE__);
        KeLowerIrql(oldIrql);
        return(status);
    }
    KeLowerIrql(oldIrql);

    // Test 1.5: Start passive async from DPC
    //     Start Async
    //     Verify OnStart() is at DPC
    //     Complete Async in OnStart()
    //     Verify OnCompleted() is at DPC
    //     Verify Completion Callback is at DPC
    //
    KTestPrintf("Test 1.5\n");
    sync.SetExpectDPC(FALSE);
    sync.SetDPCAsyncContext(NULL);

    context->Reuse();
    status = context->SetDispatchOnDPCThread(FALSE);
    context->SetExpectDPC(FALSE);

    oldIrql = KeRaiseIrqlToDpcLevel();

    context->StartMe(DPCAsyncContext::DPCAsyncMode::CompleteImmediately, NULL, NULL, NULL, sync);
    status = sync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("CompleteImmediately failed %x at %d\n", status, __LINE__);
        KeLowerIrql(oldIrql);
        return(status);
    }
    KeLowerIrql(oldIrql);


    //
    // Test 2: Both Parent/Child are DPC
    //
    //     Start Parent
    //     Verify Parent OnStart() is at DPC
    //     Start Child
    //     Verify Child OnStart() is at DPC
    //     Complete Parent in child OnStart()
    //     Verify Parent OnCompleted is at DPC
    //     Verify Parent completion callback is at DPC
    //     Complete Child in main thread
    //     Verify child OnCompleted is at DPC
    //     Verify child completion callback is at DPC
    //
    KTestPrintf("Test 2.1\n");

    parentContext->Reuse();
    parentContext->SetDispatchOnDPCThread(TRUE);
    parentContext->SetExpectDPC(TRUE);

    syncParent.SetExpectDPC(TRUE);
    syncParent.SetDPCAsyncContext(NULL);
    parentContext->StartMe(DPCAsyncContext::DPCAsyncMode::WaitForLater, NULL, NULL, NULL, syncParent);

    sync.SetExpectDPC(TRUE);
    sync.SetDPCAsyncContext(NULL);

    context->Reuse();
    context->SetDispatchOnDPCThread(TRUE);
    context->SetExpectDPC(TRUE);

    context->StartMe(DPCAsyncContext::DPCAsyncMode::CompleteParentAsync, NULL, NULL, parentContext.RawPtr(), sync);
    context->CompleteMe(STATUS_SUCCESS);

    status = syncParent.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Parent failed %x at %d\n", status, __LINE__);
        return(status);
    }

    status = sync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("child failed %x at %d\n", status, __LINE__);
        return(status);
    }

    //     Start Parent
    //     Verify Parent OnStart() is at DPC
    //     Start Child
    //     Verify Child OnStart() is at DPC
    //     Complete Child in OnStart()
    //     Verify Child OnCompleted is at DPC
    //     Verify Parent Completion callback is at DPC
    //     Complete Parent in Parent Completion callback
    //     Verify Parent OnCompleted is at DPC
    //     Verify completion callback is at passive
    //
    KTestPrintf("Test 2.2\n");

    parentContext->Reuse();
    parentContext->SetDispatchOnDPCThread(TRUE);
    parentContext->SetExpectDPC(TRUE);

    syncParent.SetExpectDPC(TRUE);
    syncParent.SetDPCAsyncContext(NULL);
    parentContext->StartMe(DPCAsyncContext::DPCAsyncMode::WaitForLater, NULL, NULL, NULL, syncParent);

    context->Reuse();
    context->SetDispatchOnDPCThread(TRUE);
    context->SetExpectDPC(TRUE);

    sync.SetExpectDPC(TRUE);
    sync.SetDPCAsyncContext(parentContext.RawPtr());
    context->StartMe(DPCAsyncContext::DPCAsyncMode::CompleteImmediately, NULL, NULL, parentContext.RawPtr(), sync);

    status = syncParent.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Parent failed %x at %d\n", status, __LINE__);
        return(status);
    }

    status = sync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("child failed %x at %d\n", status, __LINE__);
        return(status);
    }

    //     Start Parent
    //     Verify Parent OnStart() is at DPC
    //     Start Child in Parent OnStart()
    //     Verify Child OnStart() is at DPC
    //     Complete Child in main thread
    //     Verify Child OnCompleted is at DPC
    //     Verify Parent Completion callback is at DPC
    //     Complete Parent in main thread
    //     Verify Parent OnCompleted is at DPC
    //     Verify completion callback is at DPC
    //
    KTestPrintf("Test 2.3\n");

    parentContext->Reuse();
    parentContext->SetDispatchOnDPCThread(TRUE);
    parentContext->SetExpectDPC(TRUE);

    syncParent.SetExpectDPC(TRUE);
    syncParent.SetDPCAsyncContext(NULL);
    parentContext->StartMe(DPCAsyncContext::DPCAsyncMode::WaitForLater, NULL, NULL, NULL, syncParent);

    context->Reuse();
    context->SetDispatchOnDPCThread(TRUE);
    context->SetExpectDPC(TRUE);

    sync.SetExpectDPC(TRUE);
    sync.SetDPCAsyncContext(NULL);

    context->StartMe(DPCAsyncContext::DPCAsyncMode::WaitForLater, NULL, NULL, parentContext.RawPtr(), sync);
    context->CompleteMe(STATUS_SUCCESS);
    parentContext->CompleteMe(STATUS_SUCCESS);

    status = syncParent.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Parent failed %x at %d\n", status, __LINE__);
        return(status);
    }

    status = sync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("child failed %x at %d\n", status, __LINE__);
        return(status);
    }

    //     Start Parent
    //     Verify Parent OnStart() is at DPC
    //     Start Child in Parent OnStart()
    //     Verify Child OnStart() is at DPC
    //     Complete Parent in main thread
    //     Verify Parent OnCompleted is at DPC
    //     Complete Child in main thread
    //     Verify Child OnCompleted is at DPC
    //     Verify Parent Completion callback is at DPC
    //     Verify completion callback is at DPC
    //
    KTestPrintf("Test 2.4\n");

    parentContext->Reuse();
    parentContext->SetDispatchOnDPCThread(TRUE);
    parentContext->SetExpectDPC(TRUE);

    syncParent.SetExpectDPC(TRUE);
    syncParent.SetDPCAsyncContext(NULL);
    parentContext->StartMe(DPCAsyncContext::DPCAsyncMode::WaitForLater, NULL, NULL, NULL, syncParent);

    context->Reuse();
    context->SetDispatchOnDPCThread(TRUE);
    context->SetExpectDPC(TRUE);

    sync.SetExpectDPC(TRUE);
    sync.SetDPCAsyncContext(NULL);

    context->StartMe(DPCAsyncContext::DPCAsyncMode::WaitForLater, NULL, NULL, parentContext.RawPtr(), sync);
    parentContext->CompleteMe(STATUS_SUCCESS);
    context->CompleteMe(STATUS_SUCCESS);

    status = syncParent.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Parent failed %x at %d\n", status, __LINE__);
        return(status);
    }

    status = sync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("child failed %x at %d\n", status, __LINE__);
        return(status);
    }


    //
    // Test 3: Parent is passive, Child is DPC
    //

    //     Start Parent
    //     Verify Parent OnStart() is at passive
    //     Start Child in Parent OnStart()
    //     Verify Child OnStart() is at DPC
    //     Complete Child in OnStart()
    //     Verify Child OnCompleted is at DPC
    //     Verify Parent Completion callback is at DPC
    //     Complete Parent in Parent Completion callback
    //     Verify Parent OnCompleted is at passive
    //     Verify main completion callback is at passive
    //
    KTestPrintf("Test 3.1\n");

    parentContext->Reuse();
    parentContext->SetDispatchOnDPCThread(FALSE);    // Passive
    parentContext->SetExpectDPC(FALSE);

    syncParent.SetExpectDPC(FALSE);
    syncParent.SetDPCAsyncContext(NULL);

    context->Reuse();
    context->SetDispatchOnDPCThread(TRUE);
    context->SetMode(DPCAsyncContext::DPCAsyncMode::CompleteImmediately);
    context->SetExpectDPC(TRUE);

    sync.SetDPCAsyncContext(parentContext.RawPtr());
    sync.SetExpectDPC(TRUE);

    parentContext->StartMe(DPCAsyncContext::DPCAsyncMode::StartChildAndWait, context.RawPtr(), sync, NULL, syncParent);

    status = syncParent.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Parent failed %x at %d\n", status, __LINE__);
        return(status);
    }

    status = sync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("child failed %x at %d\n", status, __LINE__);
        return(status);
    }

    //     Start Parent
    //     Verify Parent OnStart() is at passive
    //     Start Child in Parent OnStart()
    //     Verify Child OnStart() is at DPC
    //     Complete Child in main thread
    //     Verify Child OnCompleted is at DPC
    //     Verify Parent Completion callback is at DPC
    //     Complete Parent in main thread
    //     Verify Parent OnCompleted is at passive
    //     Verify completion callback is at passive
    //
    KTestPrintf("Test 3.2\n");

    parentContext->Reuse();
    parentContext->SetDispatchOnDPCThread(FALSE);    // Passive
    parentContext->SetExpectDPC(FALSE);

    syncParent.SetExpectDPC(FALSE);
    syncParent.SetDPCAsyncContext(NULL);

    context->Reuse();
    context->SetDispatchOnDPCThread(TRUE);
    context->SetMode(DPCAsyncContext::DPCAsyncMode::WaitForLater);
    context->SetExpectDPC(TRUE);

    sync.SetExpectDPC(TRUE);
    sync.SetDPCAsyncContext(NULL);

    parentContext->StartMe(DPCAsyncContext::DPCAsyncMode::StartChildAndWait, context.RawPtr(), sync, NULL, syncParent);

    //
    // Wait here for the parent to start the child
    //
    PauseThread(500);

    context->CompleteMe(STATUS_SUCCESS);
    parentContext->CompleteMe(STATUS_SUCCESS);

    status = syncParent.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Parent failed %x at %d\n", status, __LINE__);
        return(status);
    }

    status = sync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("child failed %x at %d\n", status, __LINE__);
        return(status);
    }

    //     Start Parent
    //     Verify Parent OnStart() is at passive
    //     Start Child in Parent OnStart()
    //     Verify Child OnStart() is at DPC
    //     Complete Parent in main thread
    //     Verify Parent OnCompleted is at passive
    //     Verify completion callback is at passive
    //     Complete Child in main thread
    //     Verify Child OnCompleted is at DPC
    //     Verify Parent Completion callback is at passive
    //
    KTestPrintf("Test 3.3\n");

    parentContext->Reuse();
    parentContext->SetDispatchOnDPCThread(FALSE);    // Passive
    parentContext->SetExpectDPC(FALSE);

    syncParent.SetExpectDPC(FALSE);
    syncParent.SetDPCAsyncContext(NULL);

    context->Reuse();
    context->SetDispatchOnDPCThread(TRUE);
    context->SetMode(DPCAsyncContext::DPCAsyncMode::WaitForLater);
    context->SetExpectDPC(TRUE);

    sync.SetExpectDPC(TRUE);
    sync.SetDPCAsyncContext(NULL);

    parentContext->StartMe(DPCAsyncContext::DPCAsyncMode::StartChildAndWait, context.RawPtr(), sync, NULL, syncParent);
    //
    // Wait here for the parent to start the child
    //
    PauseThread(500);

    parentContext->CompleteMe(STATUS_SUCCESS);
    context->CompleteMe(STATUS_SUCCESS);

    status = syncParent.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Parent failed %x at %d\n", status, __LINE__);
        return(status);
    }

    status = sync.WaitForCompletion();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("child failed %x at %d\n", status, __LINE__);
        return(status);
    }


    //
    // Test 4: Parent is DPC, Child is not
    //
    // This is an illegal combination and will cause KInvariant.
    //

    // TODO: Use KInvariant Hook to keep this from crashing
#if 0
    KTestPrintf("Test 4.1\n");

    parentContext->Reuse();
    parentContext->SetDispatchOnDPCThread(TRUE);
    parentContext->SetExpectDPC(TRUE);
    syncParent.SetExpectDPC(TRUE);
    syncParent.SetDPCAsyncContext(NULL);

    parentContext->StartMe(DPCAsyncContext::DPCAsyncMode::WaitForLater, NULL, NULL, NULL, syncParent);

    context->Reuse();
    context->SetDispatchOnDPCThread(FALSE);
    context->SetExpectDPC(FALSE);
    sync.SetExpectDPC(FALSE);
    sync.SetDPCAsyncContext(NULL);
    context->StartMe(DPCAsyncContext::DPCAsyncMode::WaitForLater, NULL, NULL, parentContext.RawPtr(), syncParent);
#endif

    return STATUS_SUCCESS;
}
#endif

//**** KAsyncContext Tests

//** Class that extends KAsyncGlobalContext - to verify the KAsyncGlobalContext support
//   in KAsyncContextBase works
class TestAsyncGlobalContext : public KAsyncGlobalContext
{
    K_FORCE_SHARED(TestAsyncGlobalContext);

public:
    typedef KSharedPtr<TestAsyncGlobalContext> SPtr;

    static TestAsyncGlobalContext::SPtr
    Create(BOOLEAN& ToSetWhenDestructed, KAllocator& Allocator)
    {
        return SPtr(_new(KTL_TAG_TEST, Allocator) TestAsyncGlobalContext(ToSetWhenDestructed));
    }

    VOID
    Set()
    {
        _SetCount++;
    }

    VOID
    Clear()
    {
        _SetCount = 0;
    }

    BOOLEAN
    IsSet()
    {
        return _SetCount > 0;
    }

private:
    TestAsyncGlobalContext(BOOLEAN& ToSetWhenDestructed)
        : _ToSetWhenDestructed(ToSetWhenDestructed)
    {
        _SetCount = 0;
        _ToSetWhenDestructed = FALSE;
    }

private:
    int         _SetCount;
    BOOLEAN&    _ToSetWhenDestructed;
};

TestAsyncGlobalContext::~TestAsyncGlobalContext()
{
    _ToSetWhenDestructed = TRUE;
}


//** Class that wraps KAsyncLock as an async operation context.
//
// An instance of a KAsyncLockContext is used to acquire ownership of one KAsyncLock.
// The user of this class is responsible for releasing ownership (ReleaseLock()) before
// attempting to Reuse() or allowing destruction of instances.
//
class KAsyncLockContext : public KAsyncContextBase
{
    K_FORCE_SHARED(KAsyncLockContext);

public:
    static KAsyncLockContext::SPtr
    Create()
    {
        return SPtr(_new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) KAsyncLockContext());
    }

    VOID
    StartAcquire(
        KAsyncLock& Lock,
        KAsyncContextBase* const Parent = nullptr,
        KAsyncContextBase::CompletionCallback Callback = nullptr,
        KAsyncGlobalContext* const GlobalContext = nullptr)
    {
        _Lock = &Lock;
        Start(Parent, Callback, GlobalContext);
    }

    VOID
    ReleaseLock()
    {
        KAsyncLock* lock = _Lock;
        _Lock = nullptr;
        if (lock != nullptr)
        {
            __super::ReleaseLock(*lock);
        }
    }

    KAsyncLock&
    GetLock() const
    {
        return *_Lock;
    }

protected:
    VOID
    OnReuse()
    {
        // Caller did not call ReleaseLock();
        KAssert(_Lock == nullptr);
    }

    VOID
    OnStart()
    {
        // Indicate context passed was accessiable - if present
        TestAsyncGlobalContext* gCtx = down_cast<TestAsyncGlobalContext, KAsyncGlobalContext>(GetGlobalContext());
        if (gCtx != nullptr)
        {
            gCtx->Set();
        }

        KAsyncContextBase::LockAcquiredCallback localCallback;
        localCallback.Bind(this, &KAsyncLockContext::LocalLockAcquiredCallback);
        AcquireLock(*_Lock, localCallback);
    }

    VOID
    LocalLockAcquiredCallback(BOOLEAN IsAcquired, KAsyncLock& Lock)
    {
        KInvariant(IsAcquired);
        KInvariant(&Lock == _Lock);
        BOOLEAN completeAccepted = Complete(STATUS_SUCCESS);
        KInvariant(completeAccepted);
    }

private:
    VOID
    HandleOutstandingLocksHeldAtComplete()
    {
        // NOTE: This allows this class to complete with locks still being held. See KAsyncContextBase.
        //       Only this class is allowed to override HandleOutstandingLocksHeldAtComplete().
    }

private:
    KAsyncLock* _Lock;
};

KAsyncLockContext::KAsyncLockContext()
{
    _Lock = nullptr;
}

KAsyncLockContext::~KAsyncLockContext()
{
}


//** Special Friend class implementation for all KAsyncContextBase tests
class KAsyncContextTestDerivation : public KAsyncContextBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(KAsyncContextTestDerivation);

public:
    // Re-expose some KAsyncContextBase members as public
    using KAsyncContextBase::IsOperatingState;
    using KAsyncContextBase::IsCompletingState;
    using KAsyncContextBase::IsCompletedState;
    using KAsyncContextBase::IsInitializedState;
    using KAsyncContextBase::Start;
    using KAsyncContextBase::Complete;
    using KAsyncContextBase::_RefCount;
    using KAsyncContextBase::QueryActivityCount;
    using KAsyncContextBase::AcquireActivities;
    using KAsyncContextBase::ReleaseActivities;

    // KAsyncContextTestDerivation instance factory.
    //
    // Parameters:
    //      ProgressEvent   - KEvent that is used at various stages to signal progress from within
    //                        the KAsyncContextBase statemachine.
    //
    //      DestructionDone - Reference to a BOOLEAN that is set TRUE iif the instance is destructed.
    //
    // Returns: SPtr to created instance.
    //
    static KAsyncContextTestDerivation::SPtr
    Create(KEvent* ProgressEvent, BOOLEAN& DestructionDone)
    {
        DestructionDone = FALSE;
        return SPtr(_new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator())
                    KAsyncContextTestDerivation(ProgressEvent, DestructionDone));
    }

    // Trigger a nested KAsyncContext test sequence.
    //
    // Description:
    //      Constructs a nested KAsyncLockContext to be used as a child operation by this instance. OnStart()
    //      calls KAsyncLockContext::StartAcquire() which completes when the underlying KAsyncLock (_NestedTestLock)
    //      is acquired by this instance. _NestedTestLock can be ccessed via GetNestedTestLock() for lock
    //      compete situations.
    //
    // Parameters:
    //      ParentAsyncContext  - Pointer to an optional parent KAsyncContext - typically == nullptr
    //      CallbackPtr         - Delegate of method called when this operation completes
    //      NestedTestDelegate  - Completion Delegate assigned to the nested KAsyncLockContest::StartAcquire
    //                            operation.
    VOID
    StartNestedTest(
        KAsyncContextBase* const ParentAsyncContext,
        CompletionCallback CallbackPtr,
        CompletionCallback NestedTestDelegate,
        TestAsyncGlobalContext* const GlobalContext = nullptr);

    //** Various callbacks used by higher level tests
    VOID
    CancelTestCallback(KAsyncContextBase* const Parent, KAsyncContextBase& CompletedContext);

    VOID
    OnStartCompleteTestCallback(KAsyncContextBase* const Parent, KAsyncContextBase& CompletedContext);

    static VOID
    KAsyncContextTestDerivation::StaticCancelTestCallback(
        PVOID ThisPtr,
        KAsyncContextBase* const Parent,
        KAsyncContextBase& CompletedContext);

    VOID
    WaitForStateMachineCompletion();

    VOID
    ResetForStateMachineCompletion();

    VOID
    OnSubOpCompleteCallback(KAsyncContextBase* const Parent, KAsyncContextBase& CompletedContext);

    VOID
    OnSubOpCallbackWithNoParentComplete(KAsyncContextBase* const Parent, KAsyncContextBase& CompletedContext);

    //** Accessors for various test-control and status properties
    KEvent&
    GetProgressEvent()
    {
        return *_ProgressEvent;
    }

    // Turns on simple sub-operation test - See NoParentTest()
    VOID
    SetTestAsyncLockContext(KAsyncLockContext* const Context)
    {
        _TestAsyncLockContext = Context;
        SetRawTestAsyncLock(&(Context->GetLock()));
    }

    KAsyncLockContext*
    GetTestAsyncLockContext()
    {
        return _TestAsyncLockContext;
    }

    // Turns on direct KAsyncLock tests - See NoParentTest()
    VOID
    SetRawTestAsyncLock(KAsyncLock* const Lock)
    {
        _TestAsyncLock = Lock;
    }

    KAsyncLock* const
    GetRawTestAsyncLock()
    {
        return _TestAsyncLock;
    }

    KAsyncContextBase* const
    GetRawTestAsyncLockOwner()
    {
        return (_TestAsyncLock != nullptr) ? (_TestAsyncLock->_OwnerSPtr.RawPtr()) : nullptr;
    }

    // Allow access to internal KAsyncLock so that lock acquires that are queued can be tested - see ParentTest
    KAsyncLock* const
    GetNestedTestLock()
    {
        return &_NestedTestLock;
    }

public:
    // Various "event" indicators
    BOOLEAN     _CompletionCallbackCalled;
    BOOLEAN     _CompletionCallbackStateCorrect;
    BOOLEAN     _CompletionWorked;
    volatile BOOLEAN    _LockAcquireWorked;
    BOOLEAN     _CallbackInCompletingState;
    BOOLEAN     _OnCompletedCalled;

    // Various specific test case controls - See OnStart()
    BOOLEAN     _DoCompleteOnStartTest;
    BOOLEAN     _DoAlertTest;
    BOOLEAN     _DoImdCancelTest;

protected:
    using KAsyncContextBase::_CallbackPtr;

    VOID
    OnStart();

    VOID
    OnCancel();

    VOID
    OnReuse();

    VOID
    OnCompleted();

private:
    KAsyncContextTestDerivation(KEvent* ProgressEventm, BOOLEAN& DestructionDone);
    VOID AsyncLockAcquireSuccessCallback(BOOLEAN IsAcquired, KAsyncLock& LockAttempted);
    VOID AsyncLockAcquireAlertedCallback(BOOLEAN IsAcquired, KAsyncLock& LockAttempted);
    VOID OnStartDoAlertTestCallback(KAsyncContextBase* const Parent, KAsyncContextBase& CompletedContext);
    VOID PrivateOnFinalize();

private:
    KEvent*                 _ProgressEvent;
    KEvent*                 _StateMachineCompletedEvent;
    BOOLEAN*                _DestructionDone;
    KAsyncLock*             _TestAsyncLock;
    KAsyncLockContext*      _TestAsyncLockContext;
    KAsyncLockContext::SPtr _AlertTestLockContext;
    KAsyncLock              _AlertTestLock;
    KAsyncLockContext::SPtr _NestedTestContext;
    KAsyncLock              _NestedTestLock;
    CompletionCallback      _NestedTestDelegate;
};


KAsyncContextTestDerivation::KAsyncContextTestDerivation(KEvent* ProgressEvent, BOOLEAN& DestructionDone)
{
    _StateMachineCompletedEvent = _new(KTL_TAG_TEST, GetThisAllocator()) KEvent(FALSE, FALSE);
    DestructionDone = FALSE;
    _ProgressEvent = ProgressEvent;
    _DestructionDone = &DestructionDone;
    _CompletionCallbackCalled = FALSE;
    _CompletionCallbackStateCorrect = TRUE;
    _CompletionWorked = FALSE;
    _LockAcquireWorked = FALSE;
    _TestAsyncLock = nullptr;
    _TestAsyncLockContext = nullptr;
    _DoCompleteOnStartTest = FALSE;
    _DoAlertTest = FALSE;
    _DoImdCancelTest = FALSE;
    _CallbackInCompletingState = FALSE;
    _OnCompletedCalled = FALSE;
}

KAsyncContextTestDerivation::~KAsyncContextTestDerivation()
{
    *_DestructionDone = TRUE;
    _delete(_StateMachineCompletedEvent);
    _StateMachineCompletedEvent = nullptr;
}

VOID
KAsyncContextTestDerivation::PrivateOnFinalize()
{
    // Overrides PrivateOnFinalize() in KAsyncContextBase.
    KEvent* e = _StateMachineCompletedEvent;
    __super::PrivateOnFinalize();

    e->SetEvent();          // Signal state machine has completed - including releasing its self-reference
}

VOID
KAsyncContextTestDerivation::WaitForStateMachineCompletion()
{
    _StateMachineCompletedEvent->WaitUntilSet();
}

VOID
KAsyncContextTestDerivation::ResetForStateMachineCompletion()
{
    _StateMachineCompletedEvent->ResetEvent();
}

VOID
KAsyncContextTestDerivation::StartNestedTest(
    KAsyncContextBase* const ParentAsyncContext,
    CompletionCallback CallbackPtr,
    CompletionCallback NestedTestDelegate,
    TestAsyncGlobalContext* const GlobalContext)
{
    _NestedTestContext = KAsyncLockContext::Create();
    _NestedTestDelegate = NestedTestDelegate;
    Start(ParentAsyncContext, CallbackPtr, GlobalContext);
}

VOID
KAsyncContextTestDerivation::OnStart()
{
    // Prove we are on the thread pool for the ktl system
    #if KTL_USER_MODE
    {
        KInvariant
        (
            (GetDefaultSystemThreadPoolUsage() == GetThisKtlSystem().GetDefaultSystemThreadPoolUsage()) &&
            (
                (!GetDefaultSystemThreadPoolUsage() && GetThisKtlSystem().DefaultThreadPool().IsCurrentThreadOwned()) ||
                (GetDefaultSystemThreadPoolUsage() && !GetThisKtlSystem().DefaultThreadPool().IsCurrentThreadOwned())
            )
        );
    }
    #else
    {
        KInvariant( GetThisKtlSystem().DefaultThreadPool().IsCurrentThreadOwned());
    }
    #endif

    if (_DoImdCancelTest)
    {
        // OnCancel() handles completion for the ImdCancelTest
        return;
    }

    // Indicate context passed was accessiable - if present
    TestAsyncGlobalContext* gCtx = down_cast<TestAsyncGlobalContext, KAsyncGlobalContext>(GetGlobalContext());
    if (gCtx != nullptr)
    {
        gCtx->Set();
    }

    if (_DoCompleteOnStartTest)
    {
        // Testing imd completion - see NoParentTest()
        Complete(STATUS_SUCCESS);
        return;
    }

    if (_DoAlertTest)
    {
        // Delay start indication until async lock is acquired - the attempt is alerted

        // Make a lock that is already owned so that an acquire will not be complete;
        // The call to Complete() in OnCancel() should cause an Alert to occur
        _AlertTestLockContext = KAsyncLockContext::Create();
        KAsyncContextBase::CompletionCallback callback;
        callback.Bind(this, &KAsyncContextTestDerivation::OnStartDoAlertTestCallback);
        _AlertTestLockContext->StartAcquire(_AlertTestLock, this, callback);
        return;
    }

    if (_TestAsyncLock != nullptr)
    {
        // Delay start indication until async lock is acquired
        KAsyncContextBase::LockAcquiredCallback callback;
        callback.Bind(this, &KAsyncContextTestDerivation::AsyncLockAcquireSuccessCallback);
        AcquireLock(*_TestAsyncLock, callback);
        return;
    }

    if (_NestedTestContext)
    {
        // Doing a nested test - start the sub-op
        _NestedTestContext->StartAcquire(_NestedTestLock, this, _NestedTestDelegate, GetGlobalContext().RawPtr());
        return;
    }

    _ProgressEvent->SetEvent();
}

VOID
KAsyncContextTestDerivation::OnCancel()
{
    // Cancel() is used in NoParentTest and ImdCancelTest to complete test instances

    // Prove we are on the thread pool for the ktl system
    #if KTL_USER_MODE
    {
        KInvariant
        (
            (GetDefaultSystemThreadPoolUsage() == GetThisKtlSystem().GetDefaultSystemThreadPoolUsage()) &&
            (
                (!GetDefaultSystemThreadPoolUsage() && GetThisKtlSystem().DefaultThreadPool().IsCurrentThreadOwned()) ||
                (GetDefaultSystemThreadPoolUsage() && !GetThisKtlSystem().DefaultThreadPool().IsCurrentThreadOwned())
            )
        );
    }
    #else
    {
        KInvariant( GetThisKtlSystem().DefaultThreadPool().IsCurrentThreadOwned());
    }
    #endif

    BOOLEAN haveCallback = _CallbackPtr;
    _CompletionWorked = Complete(STATUS_CANCELLED);
    if (_CompletionWorked)
    {
        if (_TestAsyncLock != nullptr)
        {
            // release async lock if we hold it
            ReleaseLock(*_TestAsyncLock);
        }

        if (!haveCallback)
        {
            // no callback is being tested - fake successful callback conditions
            _CompletionCallbackCalled = TRUE;
            _ProgressEvent->SetEvent();
            return;
        }
    }
}

VOID
KAsyncContextTestDerivation::OnCompleted()
{
    if (_DoAlertTest && !_LockAcquireWorked)
    {
        // BUG: This is needed because KAsync may not call out to LockAcquire comp when
        //      racing with a Complete() call - consider if this is desired behavior.
        _LockAcquireWorked = TRUE;
    }

    _OnCompletedCalled = TRUE;
}

VOID
KAsyncContextTestDerivation::OnReuse()
{
    _CompletionCallbackCalled = FALSE;
    _CompletionCallbackStateCorrect = TRUE;
    _CompletionWorked = FALSE;
    _LockAcquireWorked = FALSE;
    _TestAsyncLock = nullptr;
    _TestAsyncLockContext = nullptr;
    _DoCompleteOnStartTest = FALSE;
    _DoAlertTest = FALSE;
    _DoImdCancelTest = FALSE;
    _CallbackInCompletingState = FALSE;
    _OnCompletedCalled = FALSE;

    if (_AlertTestLockContext)
    {
        _AlertTestLockContext->ReleaseLock();
        _AlertTestLockContext.Reset();
    }
    if (_NestedTestContext)
    {
        _NestedTestContext->ReleaseLock();
        _NestedTestContext.Reset();
    }
    _NestedTestDelegate.Reset();
    _ProgressEvent->SetEvent();
}

// Various callbacks used in BasicStateMachine tests
VOID
KAsyncContextTestDerivation::OnSubOpCompleteCallback(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& CompletedContext)
{
    // Continues OnStart() when _NestedTestContext set
    KInvariant(Parent == this);
    KInvariant(_NestedTestContext);
    KInvariant(this->IsInApartment());
    KInvariant(!CompletedContext.IsInApartment());

    // Prove we are on the thread pool for the ktl system
    #if KTL_USER_MODE
    {
        KInvariant
        (
            (GetDefaultSystemThreadPoolUsage() == GetThisKtlSystem().GetDefaultSystemThreadPoolUsage()) &&
            (
                (!GetDefaultSystemThreadPoolUsage() && GetThisKtlSystem().DefaultThreadPool().IsCurrentThreadOwned()) ||
                (GetDefaultSystemThreadPoolUsage() && !GetThisKtlSystem().DefaultThreadPool().IsCurrentThreadOwned())
            )
        );
    }
    #else
    {
        KInvariant( GetThisKtlSystem().DefaultThreadPool().IsCurrentThreadOwned());
    }
    #endif

    // Lock is owned - the fact this method was called should allow this parent to complete
    _NestedTestContext->ReleaseLock();

    //* Prove nested sub-op Reuse() works
    CompletedContext.Reuse();

    Complete(STATUS_SUCCESS);
}

VOID
KAsyncContextTestDerivation::OnSubOpCallbackWithNoParentComplete(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& CompletedContext)
{
    UNREFERENCED_PARAMETER(CompletedContext);

    // Continues OnStart() with _NestedTestContext set; but...
    // This should only be called when the parent is in the Completing state
    KInvariant(IsCompletingState());
    KInvariant(this == Parent);
    _CallbackInCompletingState = TRUE;
}

VOID
KAsyncContextTestDerivation::OnStartDoAlertTestCallback(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& CompletedContext)
{
    UNREFERENCED_PARAMETER(CompletedContext);

    // Continues from OnStart() with _DoAlertTest == TRUE
    KInvariant(Parent == this);
    KInvariant(_DoAlertTest);

    // Prove we are on the thread pool for the ktl system
    #if KTL_USER_MODE
    {
        KInvariant
        (
            (GetDefaultSystemThreadPoolUsage() == GetThisKtlSystem().GetDefaultSystemThreadPoolUsage()) &&
            (
                (!GetDefaultSystemThreadPoolUsage() && GetThisKtlSystem().DefaultThreadPool().IsCurrentThreadOwned()) ||
                (GetDefaultSystemThreadPoolUsage() && !GetThisKtlSystem().DefaultThreadPool().IsCurrentThreadOwned())
            )
        );
    }
    #else
    {
        KInvariant( GetThisKtlSystem().DefaultThreadPool().IsCurrentThreadOwned());
    }
    #endif

    // Lock is owned - request an acquire - will be alerted when Cancel() is called
    KAsyncContextBase::LockAcquiredCallback callback;
    callback.Bind(this, &KAsyncContextTestDerivation::AsyncLockAcquireAlertedCallback);
    AcquireLock(_AlertTestLock, callback);
    _ProgressEvent->SetEvent();
}

VOID
KAsyncContextTestDerivation::AsyncLockAcquireAlertedCallback(BOOLEAN IsAcquired, KAsyncLock& LockAttempted)
{
    // NOTE: Continues from OnStartDoAlertTestCallback() when Cancel() is called
    _LockAcquireWorked = TRUE;          // assume the best
    if (IsAcquired)
    {
        KTestPrintf("BasicStateMachine:AsyncLockAcquireAlertCallback: %i: Unexpected IsAcquired value\n", __LINE__);
        _LockAcquireWorked = FALSE;
    }
    else
    {
        // Prove we can re-issue and still complete
        KAsyncContextBase::LockAcquiredCallback callback;
        callback.Bind(this, &KAsyncContextTestDerivation::AsyncLockAcquireAlertedCallback);
        AcquireLock(_AlertTestLock, callback);
    }

    if (&LockAttempted != &_AlertTestLock)
    {
        KTestPrintf("BasicStateMachine:AsyncLockAcquireAlertCallback: %i: Unexpected LockAttempted& value\n", __LINE__);
        _LockAcquireWorked = FALSE;
    }
}

VOID
KAsyncContextTestDerivation::CancelTestCallback(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& CompletedContext)
{
    // Continued from OnStart() when Cancel() is being used as the completion means in a test. Used to specifcally
    // test instance method KDelegates as completion routines.
    _CompletionCallbackCalled = TRUE;
    if (!IsCompletedState())
    {
        KTestPrintf("BasicStateMachine:CancelTestCallback: %i: Unexpected State\n", __LINE__);
        _CompletionCallbackStateCorrect = FALSE;
    }
    if (Parent != nullptr)
    {
        KTestPrintf("BasicStateMachine:CancelTestCallback: %i: Unexpected Parent value\n", __LINE__);
        _CompletionCallbackStateCorrect = FALSE;
    }
    if (&CompletedContext != this)
    {
        KTestPrintf("BasicStateMachine:CancelTestCallback: %i: Unexpected CompletedContext value\n", __LINE__);
        _CompletionCallbackStateCorrect = FALSE;
    }

    _ProgressEvent->SetEvent();
}

VOID
KAsyncContextTestDerivation::StaticCancelTestCallback(
    PVOID ThisPtr,
    KAsyncContextBase* const Parent,
    KAsyncContextBase& CompletedContext)
{
    // Continued from OnStart() when Cancel() is being used as the completion means in a test. Used to specifcally
    // test static method KDelegates as completion routines.
    KAsyncContextTestDerivation* thisPtr = static_cast<KAsyncContextTestDerivation*>(ThisPtr);
    thisPtr->_CompletionCallbackCalled = TRUE;
    if (!thisPtr->IsCompletedState())
    {
        KTestPrintf("BasicStateMachine:StaticCancelTestCallback: %i: Unexpected State\n", __LINE__);
        thisPtr->_CompletionCallbackStateCorrect = FALSE;
    }
    if (Parent != nullptr)
    {
        KTestPrintf("BasicStateMachine:StaticCancelTestCallback: %i: Unexpected Parent value\n", __LINE__);
        thisPtr->_CompletionCallbackStateCorrect = FALSE;
    }
    if (&CompletedContext != thisPtr)
    {
        KTestPrintf("BasicStateMachine:StaticCancelTestCallback: %i: Unexpected CompletedContext value\n", __LINE__);
        thisPtr->_CompletionCallbackStateCorrect = FALSE;
    }
    thisPtr->_ProgressEvent->SetEvent();
}

VOID
KAsyncContextTestDerivation::OnStartCompleteTestCallback(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& CompletedContext)
{
    // Continued from OnStart() when _DoCompleteOnStartTest is set
    _CompletionCallbackCalled = TRUE;
    if (!IsCompletedState())
    {
        KTestPrintf("BasicStateMachine:OnStartCompleteTestCallback: %i: Unexpected State\n", __LINE__);
        _CompletionCallbackStateCorrect = FALSE;
    }
    if (Parent != nullptr)
    {
        KTestPrintf("BasicStateMachine:OnStartCompleteTestCallback: %i: Unexpected Parent value\n", __LINE__);
        _CompletionCallbackStateCorrect = FALSE;
    }
    if (&CompletedContext != this)
    {
        KTestPrintf("BasicStateMachine:OnStartCompleteTestCallback: %i: Unexpected CompletedContext value\n", __LINE__);
        _CompletionCallbackStateCorrect = FALSE;
    }
    _ProgressEvent->SetEvent();
}

VOID
KAsyncContextTestDerivation::AsyncLockAcquireSuccessCallback(BOOLEAN IsAcquired, KAsyncLock& LockAttempted)
{
    // NOTE: Continues from OnStart() when _TestAsyncLock is set
    _LockAcquireWorked = TRUE;               // assume the best
    if (!IsAcquired)
    {
        KTestPrintf("BasicStateMachine:AsyncLockAcquireSuccessCallback: %i: Unexpected IsAcquired value\n", __LINE__);
        _LockAcquireWorked = FALSE;
    }

    if (&LockAttempted != _TestAsyncLock)
    {
        KTestPrintf("BasicStateMachine:AsyncLockAcquireSuccessCallback: %i: Unexpected LockAttempted& value\n", __LINE__);
        _LockAcquireWorked = FALSE;
    }
    _ProgressEvent->SetEvent();
}

//** KAsyncContext derivation to specifically test KAsyncLock completion while in the Completing State
class KAsyncLockDelayedCompleteContext : public KAsyncContextBase
{
    K_FORCE_SHARED(KAsyncLockDelayedCompleteContext);

public:
    static KAsyncLockDelayedCompleteContext::SPtr
    Create()
    {
        return SPtr(_new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) KAsyncLockDelayedCompleteContext());
    }

    VOID
    StartAcquire(
        KAsyncLock& Lock,
        KEvent& CompletionEvent,
        KAsyncContextBase* const Parent = nullptr,
        KAsyncContextBase::CompletionCallback Callback = nullptr)
    {
        _Lock = &Lock;
        _CompletionEvent = &CompletionEvent;
        Start(Parent, Callback);
    }

    VOID
    ReleaseLock()
    {
        KAsyncLock* lock = _Lock;
        _Lock = nullptr;
        if (lock != nullptr)
        {
            __super::ReleaseLock(*lock);
        }
    }

    KAsyncLock&
    GetLock() const
    {
        return *_Lock;
    }

protected:
    VOID
    OnReuse()
    {
        // Caller did not call ReleaseLock();
        KInvariant(_Lock == nullptr);
        KInvariant(_CompletionEvent == nullptr);
    }

    VOID
    OnStart()
    {
        KAsyncContextBase::LockAcquiredCallback localCallback;
        localCallback.Bind(this, &KAsyncLockDelayedCompleteContext::LocalLockAcquiredCallback);

        // Pre-complete the context - should cause sm to move to the Completing State when LocalLockAcquiredCallback
        // is called
        BOOLEAN completeAccepted = Complete(STATUS_SUCCESS);
        KInvariant(completeAccepted);

        AcquireLock(*_Lock, localCallback);
    }

    VOID
    LocalLockAcquiredCallback(BOOLEAN IsAcquired, KAsyncLock& Lock)
    {
        KInvariant(IsAcquired);
        KInvariant(&Lock == _Lock);

        // completeAccepted == FALSE indicates this callback was invoked from Completing State because OnStart()
        // already called Complete().
        BOOLEAN completeAccepted = Complete(STATUS_UNSUCCESSFUL);
        KInvariant(!completeAccepted);

        KEvent*     compEvent = _CompletionEvent;
        _CompletionEvent = nullptr;

        // Signal completion
        compEvent->SetEvent();
    }

private:
    VOID
    HandleOutstandingLocksHeldAtComplete()
    {
        // NOTE: This allows this class to complete with locks still being held. See KAsyncContextBase.
        //       Only this class is allowed to override HandleOutstandingLocksHeldAtComplete().
    }

private:
    KAsyncLock* _Lock;
    KEvent*     _CompletionEvent;
};

KAsyncLockDelayedCompleteContext::KAsyncLockDelayedCompleteContext()
{
    _Lock = nullptr;
    _CompletionEvent = nullptr;
}

KAsyncLockDelayedCompleteContext::~KAsyncLockDelayedCompleteContext()
{
}


// Common helper functions
VOID
OnSuccessfulCompleteCallback(
    PVOID KEventPtr,
    KAsyncContextBase* const Parent,
    KAsyncContextBase& CompletedContext)
{
    UNREFERENCED_PARAMETER(Parent);

    // Common static delegate that sets a passed event. Used in various nested KAsyncContext tests - see
    // ParentTest()
    KInvariant(CompletedContext.Status() == STATUS_SUCCESS);
    KEvent* eventPtr = static_cast<KEvent*>(KEventPtr);
    eventPtr->SetEvent();
}


//** Main Tests
BOOLEAN
ParentTest(
    KAsyncContextTestDerivation::SPtr&      TestInst,
    int                                     Cycle,
    TestAsyncGlobalContext* const           GlobalContext = nullptr)
{
    KEvent& testSyncEvent = TestInst->GetProgressEvent();

    // Test construction
    NTSTATUS status = TestInst->Status();
    if (status != K_STATUS_NOT_STARTED)
    {
        KTestPrintf("BasicStateMachine:ParentTest: %i: Unexpected Status(0x%08X) - cycle: %i\n", __LINE__, status, Cycle);
        return FALSE;
    }
    if (TestInst->_CompletionCallbackCalled)
    {
        KTestPrintf("BasicStateMachine:ParentTest: %i: Unexpected State - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }
    if (!TestInst->IsInitializedState())
    {
        KTestPrintf("BasicStateMachine:ParentTest: %i: Unexpected State - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }

    // Test basic sub-op completion thru a parent
    KAsyncContextBase::CompletionCallback callback;
    KAsyncContextBase::CompletionCallback subOpCompCallback;

    callback.Bind(&testSyncEvent, &OnSuccessfulCompleteCallback);
    subOpCompCallback.Bind(TestInst.RawPtr(), &KAsyncContextTestDerivation::OnSubOpCompleteCallback);

    TestInst->StartNestedTest(nullptr, callback, subOpCompCallback, GlobalContext);
    testSyncEvent.WaitUntilSet();        // Wait for completion - OnSuccessfulCompleteCallback called

    status = TestInst->Status();
    if (status != STATUS_SUCCESS)
    {
        KTestPrintf("BasicStateMachine:ParentTest: %i: Unexpected Status(0x%08X) - cycle: %i\n", __LINE__, status, Cycle);
        return FALSE;
    }
    if ((GlobalContext != nullptr) && !GlobalContext->IsSet())
    {
        KTestPrintf("BasicStateMachine:ParentTest: %i: GlobalContext Failure - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }

    if (!TestInst->_OnCompletedCalled)
    {
        KTestPrintf("BasicStateMachine:ParentTest: %i: OnCompleted() Failure - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }

    // Test Reuse()
    TestInst->WaitForStateMachineCompletion();          // Wait for the SM to catch up
    TestInst->Reuse();
    testSyncEvent.WaitUntilSet();
    status = TestInst->Status();
    if (status != K_STATUS_NOT_STARTED)
    {
        KTestPrintf("BasicStateMachine:ParentTest: %i: Unexpected Status(0x%08X) - cycle: %i\n", __LINE__, status, Cycle);
        return FALSE;
    }
    if (TestInst->_CompletionCallbackCalled)
    {
        KTestPrintf("BasicStateMachine:ParentTest %i: Unexpected State - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }
    if (!TestInst->IsInitializedState())
    {
        KTestPrintf("BasicStateMachine:ParentTest: %i: Unexpected State - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }
    if (TestInst->_RefCount != 1)
    {
        KTestPrintf("BasicStateMachine:ParentTest: %i: Unexpected RefCount State - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }

    //** Prove Completion of a Parent will hold at CompletionPending state until child completes
    {
        KEvent lockAcquiredEvent(FALSE, FALSE);
        KAsyncLockContext::CompletionCallback acquiredCallback;
        KAsyncLockContext::SPtr asyncLock = KAsyncLockContext::Create();

        // Cause a lock to be pre-acquired by a temp KAsyncLockContext - asyncLock. This is to
        // cause a parent that starts another acquire (StartAcquire) via another KAsyncLockContext to be
        // queued waiting for lock ownership.
        acquiredCallback.Bind(&lockAcquiredEvent, &OnSuccessfulCompleteCallback);
        asyncLock->StartAcquire(*(TestInst->GetNestedTestLock()), nullptr, acquiredCallback);
        lockAcquiredEvent.WaitUntilSet();       // OnSuccessfulCompleteCallback was called

        // Lock is held by asyncLock - should hold up completion of TestInst until asyncLock is released below.

        // Verify TestInst's construction
        NTSTATUS status1 = TestInst->Status();
        if (status1 != K_STATUS_NOT_STARTED)
        {
            KTestPrintf("BasicStateMachine:ParentTest: %i: Unexpected Status(0x%08X) - cycle: %i\n", __LINE__, status1, Cycle);
            return FALSE;
        }
        if (TestInst->_CompletionCallbackCalled)
        {
            KTestPrintf("BasicStateMachine:ParentTest: %i: Unexpected State - cycle: %i\n", __LINE__, Cycle);
            return FALSE;
        }
        if (!TestInst->IsInitializedState())
        {
            KTestPrintf("BasicStateMachine:ParentTest: %i: Unexpected State - cycle: %i\n", __LINE__, Cycle);
            return FALSE;
        }

        // Trigger a nested test and then start a completion seq on the parent (TestInst) with its underlying
        // child (created in StartNestedTest) still queued waiting to acquire lock currently owned by asyncLock.
        callback.Bind(&testSyncEvent, &OnSuccessfulCompleteCallback);
        subOpCompCallback.Bind(
            TestInst.RawPtr(),
            &KAsyncContextTestDerivation::OnSubOpCallbackWithNoParentComplete);

        TestInst->StartNestedTest(nullptr, callback, subOpCompCallback);
        PauseThread(500);
        TestInst->Complete(STATUS_SUCCESS);
        PauseThread(500);

        if (!TestInst->IsCompletingState())     // Parent must be held in CompletingState
        {
            KTestPrintf("BasicStateMachine:ParentTest: %i: Unexpected State - cycle: %i\n", __LINE__, Cycle);
            return FALSE;
        }

        if (TestInst->QueryActivityCount() != 1)
        {
            KTestPrintf("BasicStateMachine:ParentTest: %i: Unexpected QueryActivityCount() result - cycle: %i\n", __LINE__, Cycle);
            return FALSE;
        }

        // Prove basic ActivityCount control
        TestInst->AcquireActivities();
        if (TestInst->QueryActivityCount() != 2)
        {
            KTestPrintf("BasicStateMachine:ParentTest: %i: Unexpected QueryActivityCount() result - cycle: %i\n", __LINE__, Cycle);
            return FALSE;
        }

        // Cause the child to complete
        asyncLock->ReleaseLock();
        ULONGLONG stopTime = KNt::GetTickCount64() + 500;

        while (TestInst->QueryActivityCount() != 1)
        {
            KNt::Sleep(0);
            if (KNt::GetTickCount64() > stopTime)
            {
                KTestPrintf("BasicStateMachine:ParentTest: %i: Unexpected delay - cycle: %i\n", __LINE__, Cycle);
                return FALSE;
            }
        }

        TestInst->ReleaseActivities();
        testSyncEvent.WaitUntilSet();        // Wait for parent (callback) completion - OnSuccessfulCompleteCallback

        status1 = TestInst->Status();
        if (status1 != STATUS_SUCCESS)
        {
            KTestPrintf("BasicStateMachine:ParentTest: %i: Unexpected Status(0x%08X) - cycle: %i\n", __LINE__, status1, Cycle);
            return FALSE;
        }

        if (!TestInst->_CallbackInCompletingState)
        {
            KTestPrintf("BasicStateMachine:ParentTest: %i: Unexpected_CallbackInCompletingState value - cycle: %i\n", __LINE__, Cycle);
            return FALSE;
        }

        if (TestInst->QueryActivityCount() != 0)
        {
            KTestPrintf("BasicStateMachine:ParentTest: %i: Unexpected QueryActivityCount() result - cycle: %i\n", __LINE__, Cycle);
            return FALSE;
        }

        // Test Reuse()
        TestInst->WaitForStateMachineCompletion();          // Wait for the SM to catch up
        TestInst->Reuse();
        testSyncEvent.WaitUntilSet();
        status1 = TestInst->Status();
        if (status1 != K_STATUS_NOT_STARTED)
        {
            KTestPrintf("BasicStateMachine:ParentTest: %i: Unexpected Status(0x%08X) - cycle: %i\n", __LINE__, status1, Cycle);
            return FALSE;
        }
        if (TestInst->_CompletionCallbackCalled)
        {
            KTestPrintf("BasicStateMachine:ParentTest %i: Unexpected State - cycle: %i\n", __LINE__, Cycle);
            return FALSE;
        }
        if (!TestInst->IsInitializedState())
        {
            KTestPrintf("BasicStateMachine:ParentTest: %i: Unexpected State - cycle: %i\n", __LINE__, Cycle);
            return FALSE;
        }
        if (TestInst->_RefCount != 1)
        {
            KTestPrintf("BasicStateMachine:ParentTest: %i: Unexpected RefCount State - cycle: %i\n", __LINE__, Cycle);
            return FALSE;
        }
    }

    return TRUE;
}

BOOLEAN
NoParentTest(
    KAsyncContextTestDerivation::SPtr&      TestInst,
    int                                     Cycle,
    KAsyncContextBase::CompletionCallback   Callback,
    TestAsyncGlobalContext* const           GlobalContext = nullptr)
{
    KEvent& testSyncEvent = TestInst->GetProgressEvent();

    // Test construction
    NTSTATUS status = TestInst->Status();
    if (status != K_STATUS_NOT_STARTED)
    {
        KTestPrintf("BasicStateMachine:NoParentTest: %i: Unexpected Status(0x%08X) - cycle: %i\n", __LINE__, status, Cycle);
        return FALSE;
    }
    if (TestInst->_CompletionCallbackCalled)
    {
        KTestPrintf("BasicStateMachine:NoParentTest: %i: Unexpected State - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }
    if (!TestInst->IsInitializedState())
    {
        KTestPrintf("BasicStateMachine:NoParentTest: %i: Unexpected State - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }

    // Test start sequence
    testSyncEvent.ResetEvent();
    TestInst->ResetForStateMachineCompletion();

    TestInst->Start(nullptr, Callback, GlobalContext);
    if (TestInst->GetTestAsyncLockContext() != nullptr)
    {
        // Running a async lock acquire test - with lock pre held - should not call the acquired callback
        // until the lock is released
        PauseThread(2000);
        if (TestInst->_LockAcquireWorked)
        {
            KTestPrintf("BasicStateMachine:NoParentTest: %i: Unexpected Lock Acquired - cycle: %i\n", __LINE__, Cycle);
            return FALSE;
        }

        TestInst->GetTestAsyncLockContext()->ReleaseLock();     // Should allow ownership and the event to be signalled
    }

    testSyncEvent.WaitUntilSet();               // Wait for OnStart()
    status = TestInst->Status();
    if (!TestInst->IsOperatingState())
    {
        KTestPrintf("BasicStateMachine:NoParentTest: %i: Unexpected State - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }
    if (status != STATUS_PENDING)
    {
        KTestPrintf("BasicStateMachine:NoParentTest: %i: Unexpected Status(0x%08X) - cycle: %i\n", __LINE__, status, Cycle);
        return FALSE;
    }
    if (TestInst->_CompletionCallbackCalled)
    {
        KTestPrintf("BasicStateMachine:NoParentTest: %i: Unexpected State - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }
    if ((GlobalContext != nullptr) && !GlobalContext->IsSet())
    {
        KTestPrintf("BasicStateMachine:NoParentTest: %i: GlobalContext was not set - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }
    if (TestInst->GetRawTestAsyncLock() != nullptr)
    {
        // Should have acquired the lock - OnStart() does not signal - rather the
        // the lock acq callback does
        if (!TestInst->_LockAcquireWorked)
        {
            KTestPrintf("BasicStateMachine:NoParentTest: %i: Unexpected _LockAcquireWorked value - cycle: %i\n", __LINE__, Cycle);
            return FALSE;
        }
        if (TestInst->GetRawTestAsyncLockOwner() != TestInst.RawPtr())
        {
            KTestPrintf("BasicStateMachine:NoParentTest: %i: Unexpected Async Lock Owner value - cycle: %i\n", __LINE__, Cycle);
            return FALSE;
        }
    }

    // Test completion (Cancel) sequence
    TestInst->Cancel();
    testSyncEvent.WaitUntilSet();
    TestInst->WaitForStateMachineCompletion();          // Wait for the SM to catch up
    status = TestInst->Status();
    if (!TestInst->_CompletionCallbackCalled)
    {
        KTestPrintf("BasicStateMachine:NoParentTest: %i: Callback not called - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }
    if (!TestInst->_CompletionCallbackStateCorrect)
    {
        KTestPrintf("BasicStateMachine:NoParentTest: %i: Unexpected State - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }
    if (!TestInst->IsCompletedState())
    {
        KTestPrintf("BasicStateMachine:NoParentTest: %i: Unexpected State - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }
    if (!TestInst->_CompletionWorked)
    {
        KTestPrintf("BasicStateMachine:NoParentTest: %i: Complete() call failed - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }
    if (status != STATUS_CANCELLED)
    {
        KTestPrintf("BasicStateMachine:NoParentTest: %i: Unexpected Status(0x%08X) - cycle: %i\n", __LINE__, status, Cycle);
        return FALSE;
    }
    if (TestInst->_DoAlertTest)
    {
        if (!TestInst->_LockAcquireWorked)
        {
            KTestPrintf("BasicStateMachine:NoParentTest: %i: Unexpected DoAlertTest result - cycle: %i\n", __LINE__, Cycle);
            return FALSE;
        }
    }
    if (!TestInst->_OnCompletedCalled)
    {
        KTestPrintf("BasicStateMachine:NoParentTest: %i: OnCompleted() call failed - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }

    // Test Reuse()
    TestInst->Reuse();
    testSyncEvent.WaitUntilSet();
    status = TestInst->Status();
    if (status != K_STATUS_NOT_STARTED)
    {
        KTestPrintf("BasicStateMachine:NoParentTest: %i: Unexpected Status(0x%08X) - cycle: %i\n", __LINE__, status, Cycle);
        return FALSE;
    }
    if (TestInst->_CompletionCallbackCalled)
    {
        KTestPrintf("BasicStateMachine: %i: Unexpected State - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }
    if (!TestInst->IsInitializedState())
    {
        KTestPrintf("BasicStateMachine:NoParentTest: %i: Unexpected State - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }
    if (TestInst->_RefCount != 1)
    {
        KTestPrintf("BasicStateMachine:NoParentTest: %i: Unexpected RefCount State - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
ImdCancelTest(
    KAsyncContextTestDerivation::SPtr&      TestInst,
    int                                     Cycle)
{
    KEvent& testSyncEvent = TestInst->GetProgressEvent();
    TestInst->_DoImdCancelTest = TRUE;

    // Test construction
    NTSTATUS status = TestInst->Status();
    if (status != K_STATUS_NOT_STARTED)
    {
        KTestPrintf("BasicStateMachine:ImdCancelTest: %i: Unexpected Status(0x%08X) - cycle: %i\n", __LINE__, status, Cycle);
        return FALSE;
    }
    if (TestInst->_CompletionCallbackCalled)
    {
        KTestPrintf("BasicStateMachine:ImdCancelTest: %i: Unexpected State - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }
    if (!TestInst->IsInitializedState())
    {
        KTestPrintf("BasicStateMachine:ImdCancelTest: %i: Unexpected State - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }

    // Test start sequence
    TestInst->Start(nullptr, KAsyncContextBase::CompletionCallback());
    if (!TestInst->Cancel())
    {
        KTestPrintf("BasicStateMachine:ImdCancelTest: %i: Imd Cancel() failed - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }

    // Test completion (Cancel) sequence
    testSyncEvent.WaitUntilSet();
    status = TestInst->Status();
    if (!TestInst->_CompletionCallbackCalled)
    {
        KTestPrintf("BasicStateMachine:ImdCancelTest: %i: Callback not called - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }

    // Test Reuse()
    TestInst->WaitForStateMachineCompletion();          // Wait for the SM to catch up
    status = TestInst->Status();
    if (!TestInst->IsCompletedState())
    {
        KTestPrintf("BasicStateMachine:ImdCancelTest: %i: Unexpected State - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }
    if (status != STATUS_CANCELLED)
    {
        KTestPrintf("BasicStateMachine:ImdCancelTest: %i: Unexpected Status(0x%08X) - cycle: %i\n", __LINE__, status, Cycle);
        return FALSE;
    }
    if (!TestInst->_OnCompletedCalled)
    {
        KTestPrintf("BasicStateMachine:ImdCancelTest: %i: OnCompleted() call failed - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }

    TestInst->Reuse();
    testSyncEvent.WaitUntilSet();
    status = TestInst->Status();
    if (status != K_STATUS_NOT_STARTED)
    {
        KTestPrintf("BasicStateMachine:ImdCancelTest: %i: Unexpected Status(0x%08X) - cycle: %i\n", __LINE__, status, Cycle);
        return FALSE;
    }
    if (TestInst->_CompletionCallbackCalled)
    {
        KTestPrintf("BasicStateMachine:ImdCancelTest %i: Unexpected State - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }
    if (!TestInst->IsInitializedState())
    {
        KTestPrintf("BasicStateMachine:NoParentTest: %i: Unexpected State - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }
    if (TestInst->_RefCount != 1)
    {
        KTestPrintf("BasicStateMachine:ImdCancelTest: %i: Unexpected RefCount State - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
PhantomCycleTest(
    KAsyncContextTestDerivation::SPtr&      TestInst,
    int                                     Cycle)
{
    KEvent& testSyncEvent = TestInst->GetProgressEvent();
    TestInst->_DoImdCancelTest = FALSE;

    // Test construction
    NTSTATUS status = TestInst->Status();
    if (status != K_STATUS_NOT_STARTED)
    {
        KTestPrintf("BasicStateMachine:PhantomCycleTest: %i: Unexpected Status(0x%08X) - cycle: %i\n", __LINE__, status, Cycle);
        return FALSE;
    }
    if (TestInst->_CompletionCallbackCalled)
    {
        KTestPrintf("BasicStateMachine:PhantomCycleTest: %i: Unexpected State - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }
    if (!TestInst->IsInitializedState())
    {
        KTestPrintf("BasicStateMachine:PhantomCycleTest: %i: Unexpected State - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }

    // Test start sequence
    struct Local
    {
        Local::Local() : _LocalCallbackEvent(FALSE, FALSE) {}

        VOID
        PhantomOpCompleted(KAsyncContextBase* const ParentAsync, KAsyncContextBase& CompletingAsync)
        {
            UNREFERENCED_PARAMETER(ParentAsync);
            UNREFERENCED_PARAMETER(CompletingAsync);

            _LocalCallbackEvent.SetEvent();
        }

        KEvent          _LocalCallbackEvent;
    }                   local;

    KAsyncContextInterceptor::SPtr  interceptor;
    status = KAsyncContextInterceptor::Create(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator(), interceptor);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("BasicStateMachine:PhantomCycleTest: %i: KAsyncContextInterceptor::Create failed - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }

    interceptor->EnableIntercept(*TestInst);
    TestInst->Start(nullptr, KAsyncContextBase::CompletionCallback(&local, &Local::PhantomOpCompleted));
    if (TestInst->Status() != STATUS_PENDING)
    {
        KTestPrintf("BasicStateMachine:PhantomCycleTest: %i: StartPhantomCycle() failed - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }

    // Make sure the instance goes into the Operational state
    ULONGLONG       timeToDie = KNt::GetTickCount64() + 10000;      // 10secs from now
    while (!TestInst->IsOperatingState())
    {
        if (KNt::GetTickCount64() >= timeToDie)
        {
            KTestPrintf("BasicStateMachine:PhantomCycleTest: %i: Operating state not achieved in 10secs - cycle: %i\n", __LINE__, Cycle);
            return FALSE;
        }

        KNt::Sleep(10);
    }

    // Trigger completion; validate state change; and callback occured
    TestInst->Complete(STATUS_UNSUCCESSFUL);
    timeToDie = KNt::GetTickCount64() + 10000;      // 10secs from now
    while (!TestInst->IsCompletedState())
    {
        if (KNt::GetTickCount64() >= timeToDie)
        {
            KTestPrintf("BasicStateMachine:PhantomCycleTest: %i: Completed state not achieved in 10secs - cycle: %i\n", __LINE__, Cycle);
            return FALSE;
        }

        KNt::Sleep(10);
    }

    // Prove callback occured
    if (!local._LocalCallbackEvent.WaitUntilSet(10000))
    {
        KTestPrintf("BasicStateMachine:PhantomCycleTest: %i: Callback not called in 10secs - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }

    if (TestInst->Status() != STATUS_UNSUCCESSFUL)
    {
        KTestPrintf("BasicStateMachine:PhantomCycleTest: %i: Invalid completion status - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }

    // Test Reuse()
    if ((Cycle & 0x01) == 1)
    {
        interceptor->DisableIntercept();
    }
    TestInst->WaitForStateMachineCompletion();          // Wait for the SM to catch up
    if ((Cycle & 0x01) == 0)
    {
        interceptor->DisableIntercept();
    }

    TestInst->Reuse();
    testSyncEvent.WaitUntilSet();
    status = TestInst->Status();
    if (status != K_STATUS_NOT_STARTED)
    {
        KTestPrintf("BasicStateMachine:PhantomCycleTest: %i: Unexpected Status(0x%08X) - cycle: %i\n", __LINE__, status, Cycle);
        return FALSE;
    }
    if (TestInst->_CompletionCallbackCalled)
    {
        KTestPrintf("BasicStateMachine:PhantomCycleTest %i: Unexpected State - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }
    if (!TestInst->IsInitializedState())
    {
        KTestPrintf("BasicStateMachine:PhantomCycleTest: %i: Unexpected State - cycle: %i\n", __LINE__, Cycle);
        return FALSE;
    }
    if (TestInst->_RefCount != 1)
    {
        KNt::Sleep(100);        // In the case of intercepted KAsyncs (TestInst) there can still be a lag on dropping
                                // the ref count due to PrivateOnFinalize() behavior in the interceptor.
        if (TestInst->_RefCount != 1)
        {
            KTestPrintf("BasicStateMachine:PhantomCycleTest: %i: Unexpected RefCount State - cycle: %i\n", __LINE__, Cycle);
            return FALSE;
        }
    }

    return TRUE;
}

NTSTATUS
InternalBasicStateMachine(int argc, WCHAR* args[])
{
    KTestPrintf("KAsyncContextBase Unit Test: Start: BasicStateMachine\n");

    KtlSystem* underlyingSystem;
    NTSTATUS status = KtlSystem::Initialize(&underlyingSystem);

    if (!NT_SUCCESS(status))
    {
        KTestPrintf("BasicStateMachine: %i: KtlSystem::Initialize failed\n", __LINE__);
        return status;
    }

    // Turn on strict allocation tracking
    underlyingSystem->SetStrictAllocationChecks(TRUE);

    auto        Test = ([&] (int argc, WCHAR* args[]) -> NTSTATUS
    {
        UNREFERENCED_PARAMETER(argc);
        UNREFERENCED_PARAMETER(args);

        #if !KTL_USER_MODE
        // DPC dispatching test
        if (!NT_SUCCESS(DPCDispatchTest()))
        {
            KTestPrintf("BasicStateMachine: %i: DPCDispatchTest Failed\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }
        #endif

        //** Basic state transision and ref counting test - with no sub operations
        BOOLEAN testInstDestructed = FALSE;
        KEvent testSyncEvent(FALSE, FALSE);
        KAsyncContextTestDerivation::SPtr testInst = KAsyncContextTestDerivation::Create(&testSyncEvent, testInstDestructed);

        #if KTL_USER_MODE
        {
            // Prove the correct default thread pool propagates correctly
            BOOLEAN isSystemThreadPoolUsed = underlyingSystem->GetDefaultSystemThreadPoolUsage();
            KInvariant(isSystemThreadPoolUsed == testInst->GetDefaultSystemThreadPoolUsage());;
            testInst->SetDefaultSystemThreadPoolUsage(!isSystemThreadPoolUsed);
            KInvariant(isSystemThreadPoolUsed != testInst->GetDefaultSystemThreadPoolUsage());;
            testInst->SetDefaultSystemThreadPoolUsage(isSystemThreadPoolUsed);
            KInvariant(isSystemThreadPoolUsed == testInst->GetDefaultSystemThreadPoolUsage());;
        }
        #endif

        KAsyncContextBase::CompletionCallback callback;
        callback.Bind(testInst.RawPtr(), &KAsyncContextTestDerivation::CancelTestCallback);

        BOOLEAN globalCtxWasDestructed = FALSE;
        TestAsyncGlobalContext::SPtr testGlobalCtx = TestAsyncGlobalContext::Create(
            globalCtxWasDestructed,
            underlyingSystem->NonPagedAllocator());

        for (int cycle = 1; cycle <= 10000; cycle++)
        {
            testInstDestructed = FALSE;
            if (!NoParentTest(testInst, cycle, callback, testGlobalCtx.RawPtr()))
            {
                return STATUS_UNSUCCESSFUL;
            }

            testGlobalCtx->Clear();
        }

        // Imd Cancel Test
        for (int cycle = 1; cycle <= 10000; cycle++)
        {
            testInstDestructed = FALSE;
            if (!ImdCancelTest(testInst, cycle))
            {
                return STATUS_UNSUCCESSFUL;
            }
        }

        for (int cycle = 1; cycle <= 10000; cycle++)
        {
            testInstDestructed = FALSE;
            if (!PhantomCycleTest(testInst, cycle))
            {
                return STATUS_UNSUCCESSFUL;
            }
        }

        // Test static callback
        KTestPrintf("KAsyncContextBase Unit Test: Test static callback\n");
        callback.Bind((PVOID) testInst.RawPtr(), &KAsyncContextTestDerivation::StaticCancelTestCallback);
        if (!NoParentTest(testInst, 1, callback))
        {
            return STATUS_UNSUCCESSFUL;
        }

        // Test no callback
        KTestPrintf("KAsyncContextBase Unit Test: Test no callback\n");
        callback.Reset();

        for (int cycle = 1; cycle <= 10000; cycle++)
        {
            if (!NoParentTest(testInst, cycle, callback))
            {
                return STATUS_UNSUCCESSFUL;
            }
        }

        // Test completion from OnStart()
        KTestPrintf("KAsyncContextBase Unit Test: Test completion from OnStart()\n");
        callback.Bind(testInst.RawPtr(), &KAsyncContextTestDerivation::OnStartCompleteTestCallback);
        testInst->_DoCompleteOnStartTest = TRUE;
        testInst->Start(nullptr, callback);
        testSyncEvent.WaitUntilSet();
        testInst->WaitForStateMachineCompletion();          // Wait for the SM to catch up
        testInst->Reuse();
        testSyncEvent.WaitUntilSet();

        // Test simple KAsyncLock acquire/release support
        KTestPrintf("KAsyncContextBase Unit Test: Test simple KAsyncLock acquire/release support\n");
        {
            KAsyncLock testLock;
            testInst->SetRawTestAsyncLock(&testLock);

            callback.Bind((PVOID) testInst.RawPtr(), &KAsyncContextTestDerivation::StaticCancelTestCallback);
            if (!NoParentTest(testInst, 1, callback))
            {
                return STATUS_UNSUCCESSFUL;
            }
        }

        // Test Pending KAsyncLock acquire
        KTestPrintf("KAsyncContextBase Unit Test: Test Pending KAsyncLock acquire\n");
        {
            KAsyncLock testLock;
            KEvent lockAcquiredEvent(FALSE, FALSE);
            KAsyncLockContext::CompletionCallback acquiredCallback;
            acquiredCallback.Bind(&lockAcquiredEvent, &OnSuccessfulCompleteCallback);
            KAsyncLockContext::SPtr asyncLock = KAsyncLockContext::Create();
            asyncLock->StartAcquire(testLock, nullptr, acquiredCallback);
            lockAcquiredEvent.WaitUntilSet();

            // Lock is held - should hold up completion of until testLock is released
            testInst->SetTestAsyncLockContext(asyncLock.RawPtr());
            callback.Bind((PVOID) testInst.RawPtr(), &KAsyncContextTestDerivation::StaticCancelTestCallback);
            if (!NoParentTest(testInst, 1, callback))
            {
                return STATUS_UNSUCCESSFUL;
            }
        }

        // Test Alert behavior
        KTestPrintf("KAsyncContextBase Unit Test: Test Alert behavior\n");
        for (ULONG ix = 0; ix < 10000; ix++)
        {
            testInst->_DoAlertTest = TRUE;
            callback.Bind((PVOID) testInst.RawPtr(), &KAsyncContextTestDerivation::StaticCancelTestCallback);
            if (!NoParentTest(testInst, 1, callback))
            {
                return STATUS_UNSUCCESSFUL;
            }
        }

        // Test deferred completion of KAsyncContext with a lock acquire callback
        KTestPrintf("KAsyncContextBase Unit Test: Test KAsyncLock acquire callback from Completed State\n");
        {
            KAsyncLockDelayedCompleteContext::SPtr  delayedLockContext = KAsyncLockDelayedCompleteContext::Create();
            KAsyncLock                              testLock;
            KEvent                                  lockAcquiredEvent(FALSE, FALSE);
            NTSTATUS                                status;

            delayedLockContext->StartAcquire(testLock, lockAcquiredEvent);
            PauseThread(500);

            status = delayedLockContext->Status();
            KInvariant(status == STATUS_SUCCESS);

            delayedLockContext->ReleaseLock();
            lockAcquiredEvent.WaitUntilSet();
        }

        //** Test nested behavior
        KTestPrintf("KAsyncContextBase Unit Test: Test nested behavior\n");
        for (int cycle = 1; cycle <= 10; cycle++)
        {
            testInstDestructed = FALSE;
            if (!ParentTest(testInst, cycle, testGlobalCtx.RawPtr()))
            {
                return STATUS_UNSUCCESSFUL;
            }

            testGlobalCtx->Clear();
        }

        // Test destruction occurs
        testInst.Reset();
        if (!testInstDestructed)
        {
            KTestPrintf("BasicStateMachine: %i: Instance Not Destructed\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }
        if (globalCtxWasDestructed)
        {
            KTestPrintf("BasicStateMachine: %i: Unexpected globalCtxWasDestructed value\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }
        testGlobalCtx.Reset();
        if (!globalCtxWasDestructed)
        {
            KTestPrintf("BasicStateMachine: %i: Unexpected globalCtxWasDestructed value\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        if (!NT_SUCCESS(BasicKAsyncLockTest()))
        {
            KTestPrintf("BasicStateMachine: %i: BasicKAsyncLockTest Failed\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        return STATUS_SUCCESS;
    });

    #if KTL_USER_MODE
    {
        // Run the test with all thread pool configuratons and thread-level queuing
        underlyingSystem->SetDefaultSystemThreadPoolUsage(FALSE);
        status = Test(argc, args);
        if (NT_SUCCESS(status))
        {
            underlyingSystem->SetDefaultSystemThreadPoolUsage(TRUE);
            status = Test(argc, args);
        }
    }
    #else
    {
        status = Test(argc, args);
    }
    #endif

    KtlSystem::Shutdown();

    KTestPrintf("KAsyncContextBase Unit Test: Complete: BasicStateMachine\n");
    return status;
}


#if KTL_USER_MODE
class ReleaseApartmentTestAsync : public KAsyncContextBase
{
    K_FORCE_SHARED(ReleaseApartmentTestAsync);

public:
    using KAsyncContextBase::Start;

    static
    NTSTATUS Create(__in KAllocator& Allocator, __out ReleaseApartmentTestAsync::SPtr& Result)
    {
        Result = new(KTL_TAG_TEST, Allocator) ReleaseApartmentTestAsync();
        if (Result == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        return STATUS_SUCCESS;
    }

private:
    KEvent      deadLockEvent;

private:
    void CompletionCallback(
        KAsyncContextBase* const Parent,
        KAsyncContextBase&  CompletingSubOp)
    {
        UNREFERENCED_PARAMETER(Parent);
        UNREFERENCED_PARAMETER(CompletingSubOp);

        KInvariant(IsInApartment());
        deadLockEvent.SetEvent();
    }

    void OnStart() override
    {
        NTSTATUS status = CallbackAsyncContext(
            KTL_TAG_TEST,
            KAsyncContextBase::CompletionCallback(this, &ReleaseApartmentTestAsync::CompletionCallback));

        if (NT_SUCCESS(status))
        {
            KInvariant(IsInApartment());
            ReleaseApartment();
            KInvariant(!IsInApartment());
        }

        deadLockEvent.WaitUntilSet();
        Complete(status);
    }
};

ReleaseApartmentTestAsync::ReleaseApartmentTestAsync()
{
    SetDefaultSystemThreadPoolUsage(TRUE);
}

ReleaseApartmentTestAsync::~ReleaseApartmentTestAsync()
{
}


void
ReleaseApartmentTest()
{
    KTestPrintf("KAsyncContextBase Unit Test: Start: ReleaseApartmentTest\n");

    KtlSystem*      underlyingSystem;
    NTSTATUS status = KtlSystem::Initialize(&underlyingSystem);
    KInvariant(NT_SUCCESS(status));

    // Turn on strict allocation tracking
    underlyingSystem->SetStrictAllocationChecks(TRUE);
    underlyingSystem->SetDefaultSystemThreadPoolUsage(FALSE);

    {
        KAllocator&                     allocator = underlyingSystem->GlobalNonPagedAllocator();
        ReleaseApartmentTestAsync::SPtr testInst;
        KSynchronizer                   sync;

        status = ReleaseApartmentTestAsync::Create(allocator, testInst);
        KInvariant(NT_SUCCESS(status));

        testInst->Start(nullptr, sync);
        status = sync.WaitForCompletion();
        KInvariant(NT_SUCCESS(status));
    }

    KtlSystem::Shutdown();
    KTestPrintf("KAsyncContextBase Unit Test: Complete: ReleaseApartmentTest\n");
}

class EmbeddedDetachOnCompletionTestAsync : public KAsyncContextBase
{
    K_FORCE_SHARED(EmbeddedDetachOnCompletionTestAsync);

public:
    using KAsyncContextBase::Start;

    static
    NTSTATUS Create(__in KAllocator& Allocator, __out EmbeddedDetachOnCompletionTestAsync::SPtr& Result)
    {
        Result = new(KTL_TAG_TEST, Allocator) EmbeddedDetachOnCompletionTestAsync();
        if (Result == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        return STATUS_SUCCESS;
    }
};

EmbeddedDetachOnCompletionTestAsync::EmbeddedDetachOnCompletionTestAsync()
{
}

EmbeddedDetachOnCompletionTestAsync::~EmbeddedDetachOnCompletionTestAsync()
{
}


class DetachOnCompletionTestAsync : public KAsyncContextBase
{
    K_FORCE_SHARED(DetachOnCompletionTestAsync);

public:
    using KAsyncContextBase::Start;

    static
    NTSTATUS Create(__in KAllocator& Allocator, __out DetachOnCompletionTestAsync::SPtr& Result)
    {
        Result = new(KTL_TAG_TEST, Allocator) DetachOnCompletionTestAsync();
        if (Result == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        return STATUS_SUCCESS;
    }

    KEvent*     embeddedDoneEvent;
    BOOLEAN     isDetachedTest;
    BOOLEAN     useParent;

private:
    KThread::Id     onStartThread;

private:
    void CompletionCallback(
        KAsyncContextBase* const Parent,
        KAsyncContextBase&  CompletingSubOp)
    {
        UNREFERENCED_PARAMETER(CompletingSubOp);

        if (isDetachedTest)
        {
            if (Parent == nullptr)
            {
                KInvariant(KThread::GetCurrentThreadId() != onStartThread);
            }
            else
            {
                KInvariant(KThread::GetCurrentThreadId() == onStartThread);
            }
        }
        else
        {
            KInvariant(KThread::GetCurrentThreadId() == onStartThread);
        }

        KEvent&     e = *embeddedDoneEvent;
        e.SetEvent();
        Release();
    }

    void OnStart() override
    {
        onStartThread = KThread::GetCurrentThreadId();

        EmbeddedDetachOnCompletionTestAsync::SPtr embedded;

        NTSTATUS status = EmbeddedDetachOnCompletionTestAsync::Create(
            GetThisAllocator(),
            embedded);
        KInvariant(NT_SUCCESS(status));

        AddRef();
        embedded->SetDetachThreadOnCompletion(isDetachedTest);
        embedded->Start(
            useParent ? this : nullptr,
            KAsyncContextBase::CompletionCallback(this, &DetachOnCompletionTestAsync::CompletionCallback));

        KInvariant(NT_SUCCESS(status));
        KInvariant(IsInApartment());

        // If doing a detached test - make sure the scheduled "Start" above is made to run in parallel
        // when we complete
        SetDetachThreadOnCompletion(isDetachedTest);
        Complete(STATUS_SUCCESS);
    }
};

DetachOnCompletionTestAsync::DetachOnCompletionTestAsync()
{
    SetDefaultSystemThreadPoolUsage(TRUE);
}

DetachOnCompletionTestAsync::~DetachOnCompletionTestAsync()
{
}

KEvent eventSync(FALSE);
KEvent embeddedComplete(FALSE);

void DetachOnCompletionCallbackTestCompWithDelay(
    KAsyncContextBase* const Parent,
    KAsyncContextBase&  CompletingSubOp)
{
    UNREFERENCED_PARAMETER(Parent);
    UNREFERENCED_PARAMETER(CompletingSubOp);

    KNt::Sleep(500);
    eventSync.SetEvent();
}

void
DetachOnCompletionCallbackTest()
{
    KTestPrintf("KAsyncContextBase Unit Test: Start: DetachOnCompletionCallbackTest\n");

    KtlSystem*      underlyingSystem;
    NTSTATUS status = KtlSystem::Initialize(&underlyingSystem);
    KInvariant(NT_SUCCESS(status));

    // Turn on strict allocation tracking
    underlyingSystem->SetStrictAllocationChecks(TRUE);
    underlyingSystem->SetDefaultSystemThreadPoolUsage(TRUE);
    {
        //** Prove that an async can be auto-detached from its thread pool
        KAllocator&                     allocator = underlyingSystem->GlobalNonPagedAllocator();
        DetachOnCompletionTestAsync::SPtr testInst;

        status = DetachOnCompletionTestAsync::Create(allocator, testInst);
        KInvariant(NT_SUCCESS(status));
        testInst->embeddedDoneEvent = &embeddedComplete;

        //* No parent for the sub-op
        {
            // Prove sub-op completes on different thread than the starting async because its been told to complete with detach
            testInst->Reuse();
            testInst->isDetachedTest = TRUE;
            testInst->useParent = FALSE;
            testInst->Start(nullptr, KAsyncContextBase::CompletionCallback(&DetachOnCompletionCallbackTestCompWithDelay));
            eventSync.WaitUntilSet();
            embeddedComplete.WaitUntilSet();

            // Prove sub-op completes on the same thread - nothing broken because of detach logic
            testInst->Reuse();
            testInst->isDetachedTest = FALSE;
            testInst->useParent = FALSE;
            testInst->Start(nullptr, KAsyncContextBase::CompletionCallback(&DetachOnCompletionCallbackTestCompWithDelay));
            eventSync.WaitUntilSet();
            embeddedComplete.WaitUntilSet();
        }

        //* parent for the sub-op
        {
            // Prove sub-op completes on same thread as the starting async - prove detach ignored with parent
            testInst->Reuse();
            testInst->isDetachedTest = TRUE;
            testInst->useParent = TRUE;
            testInst->Start(nullptr, KAsyncContextBase::CompletionCallback(&DetachOnCompletionCallbackTestCompWithDelay));
            eventSync.WaitUntilSet();
            embeddedComplete.WaitUntilSet();

            // Prove sub-op completes on the same thread - nothing broken because of detach logic
            testInst->Reuse();
            testInst->isDetachedTest = FALSE;
            testInst->useParent = TRUE;
            testInst->Start(nullptr, KAsyncContextBase::CompletionCallback(&DetachOnCompletionCallbackTestCompWithDelay));
            eventSync.WaitUntilSet();
            embeddedComplete.WaitUntilSet();
        }
    }

    KtlSystem::Shutdown();
    KTestPrintf("KAsyncContextBase Unit Test: Complete: DetachOnCompletionCallbackTest\n");
}
#endif

//
// This template can be used to wrap a KAsyncContext and have its
// OnStart() delayed by a specific number of milliseconds.
//
template<class T>
class ForwardingInterceptor : public KAsyncContextInterceptor
{
    K_FORCE_SHARED_WITH_INHERITANCE(ForwardingInterceptor);


public:
    static NTSTATUS
    Create(
        __in ULONG AllocationTag,
        __in KAllocator& Allocator,
        __out KSharedPtr<ForwardingInterceptor>& Context)
    {
        NTSTATUS status;

        Context = _new(AllocationTag, Allocator) ForwardingInterceptor();
        if (Context == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = Context->Status();
        if (! NT_SUCCESS(status))
        {
            Context = nullptr;
        }

        return status;
    }

protected:
    //
    // When interceptor is enabled, the OnStart(), OnCancel(),
    // OnCompleted() and OnReuse() are redirected into the interceptor
    // class within the context of the async being intercepted. If
    // access to the intercepted class is needed, the GetIntercepted()
    // method is available. This may be useful for forwarding on the
    // call.
    //
    void OnStartIntercepted()
    {
        ForwardOnStart();
    }

    void OnCancelIntercepted()
    {
        ForwardOnCancel();
    }

    void OnCompletedIntercepted()
    {
        ForwardOnCompleted();
    }

    void OnReuseIntercepted()
    {
        ForwardOnReuse();
    }

protected:
};

template<class T>
ForwardingInterceptor<T>::ForwardingInterceptor()
{
}

template<class T>
ForwardingInterceptor<T>::~ForwardingInterceptor()
{
}

class ForwardTestAsync : public KAsyncContextBase
{
    K_FORCE_SHARED(ForwardTestAsync);

    public:
        static NTSTATUS Create(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag,
            __out ForwardTestAsync::SPtr& Async);

        VOID StartIt(
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
        {
            Start(ParentAsyncContext, CallbackPtr);
        }

        ULONG GetOnStartCount()
        {
            return(_OnStartCount);
        }

        ULONG GetOnCancelCount()
        {
            return(_OnCancelCount);
        }

        ULONG GetOnCompletedCount()
        {
            return(_OnCompletedCount);
        }

        ULONG GetOnReuseCount()
        {
            return(_OnReuseCount);
        }

    protected:
        VOID
        OnStart(
            ) override
        {
            _OnStartCount++;
        }

        VOID
        OnReuse(
            ) override
        {
            _OnReuseCount++;
        }

        VOID
        OnCancel(
            ) override
        {
            _OnCancelCount++;
            Complete(STATUS_CANCELLED);
        }

        VOID
        OnCompleted(
            )
        {
            _OnCompletedCount++;
        }

    private:
        ULONG _OnStartCount;
        ULONG _OnReuseCount;
        ULONG _OnCancelCount;
        ULONG _OnCompletedCount;

};

ForwardTestAsync::ForwardTestAsync()
{
     _OnStartCount = 0;
     _OnReuseCount = 0;
     _OnCancelCount = 0;
     _OnCompletedCount = 0;
}

ForwardTestAsync::~ForwardTestAsync()
{
}

NTSTATUS ForwardTestAsync::Create(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __out ForwardTestAsync::SPtr& Async)
{
    ForwardTestAsync::SPtr async;

    async = _new(AllocationTag, Allocator) ForwardTestAsync();
    if (! async)
    {
        KInvariant(FALSE);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (!NT_SUCCESS(async->Status()))
    {
        KInvariant(FALSE);
        return(async->Status());
    }

    Async = Ktl::Move(async);
    return(STATUS_SUCCESS);
}


VOID InterceptorTests()
{
    NTSTATUS status;

    KtlSystem*      underlyingSystem;
    status = KtlSystem::Initialize(&underlyingSystem);
    KInvariant(NT_SUCCESS(status));

    // Turn on strict allocation tracking
    underlyingSystem->SetStrictAllocationChecks(TRUE);
    KAllocator&                     allocator = underlyingSystem->GlobalNonPagedAllocator();

    ForwardTestAsync::SPtr forward;
    ForwardingInterceptor<ForwardTestAsync>::SPtr interceptor;

    status = ForwardTestAsync::Create(allocator,
                                      KTL_TAG_TEST,
                                      forward);
    KInvariant(NT_SUCCESS(status));

    status = ForwardingInterceptor<ForwardTestAsync>::Create(KTL_TAG_TEST,
                                                                     allocator,
                                                                     interceptor);
    KInvariant(NT_SUCCESS(status));


    interceptor->EnableIntercept(*forward);

    for (ULONG i = 0; i < 64; i++)
    {
        //
        // Verify OnStart, OnCancel and OnCompleted
        //
        forward->StartIt(NULL, NULL);
        KNt::Sleep(200);
        KInvariant(forward->GetOnStartCount() == i+1);

        forward->Cancel();
        KNt::Sleep(200);
        KInvariant(forward->GetOnCancelCount() == i+1);
        KInvariant(forward->GetOnCompletedCount() == i+1);

        forward->Reuse();
        KInvariant(forward->GetOnReuseCount() == i+1);
    }



    interceptor->DisableIntercept();
    interceptor = nullptr;
    forward = nullptr;

    KtlSystem::Shutdown();

}

NTSTATUS
BasicStateMachine(int argc, WCHAR* args[])
{
    KTestPrintf("KAsyncContextBase BasicStateMachine Test: STARTED\n");
    NTSTATUS result = STATUS_SUCCESS;

    EventRegisterMicrosoft_Windows_KTL();

#if defined(PLATFORM_UNIX)
    NTSTATUS status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif
    
    InterceptorTests();

    if ((result = InternalBasicStateMachine(argc, args)) != STATUS_SUCCESS)
    {
        KTestPrintf("KAsyncContextBase Unit Test: FAILED\n");
    }
    else
    {
        #if KTL_USER_MODE
        {
            ReleaseApartmentTest();
            DetachOnCompletionCallbackTest();
        }
        #endif
    }

    EventUnregisterMicrosoft_Windows_KTL();
    KTestPrintf("KAsyncContextBase BasicStateMachine Test: COMPLETED\n");

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif  

    return result;
}

#if CONSOLE_TEST
int
#if !defined(PLATFORM_UNIX)
main(int argc, WCHAR* args[])
{
#else
main(int argc, char* cargs[])
{
    CONVERT_TO_ARGS(argc, cargs)
#endif
    BasicStateMachine(argc - 1, args);
    return 0;
}
#endif

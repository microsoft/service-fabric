/*++

Copyright (c) Microsoft Corporation

Module Name:

    KAsyncQueueTests.cpp

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
#if defined(PLATFORM_UNIX)
#include <vector>
#endif
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


NTSTATUS LastStaticTestCallbackStatus;

StaticTestCallback::StaticTestCallback(PVOID EventPtr)
{
    _EventPtr = EventPtr;
}

void StaticTestCallback::Callback(KAsyncContextBase* const Parent, KAsyncContextBase& CompletedContext)
{
    UNREFERENCED_PARAMETER(Parent);

    KEvent* eventPtr = static_cast<KEvent*>(_EventPtr);
    LastStaticTestCallbackStatus = CompletedContext.Status();
    eventPtr->SetEvent();
}

NTSTATUS LastStaticDequeueCallbackStatus;

struct StaticDequeueCallback
{
    PVOID _EventPtr;

    StaticDequeueCallback(PVOID EventPtr)
    {
        _EventPtr = EventPtr;
    }

    VOID Callback(KAsyncContextBase* const Parent, KAsyncContextBase& CompletedContext)
    {
        UNREFERENCED_PARAMETER(Parent);

        KEvent* eventPtr = static_cast<KEvent*>(_EventPtr);
        LastStaticDequeueCallbackStatus = CompletedContext.Status();
        eventPtr->SetEvent();
    }
};

struct TestQueueItem
{
    KListEntry      _Links;
    ULONG           _ItemNumber;
    BOOLEAN         _WasProcessed;
};

ULONG CountOfRxdTestQueueItems;

struct StaticQueuePumpCallback
{
    PVOID _EventPtr;

    StaticQueuePumpCallback(PVOID EventPtr)
    {
        _EventPtr = EventPtr;
    }

    void Callback(KAsyncContextBase* const Parent, KAsyncContextBase& CompletedContext)
    {
        UNREFERENCED_PARAMETER(Parent);

        KEvent* eventPtr = static_cast<KEvent*>(_EventPtr);
        if (CompletedContext.Status() == STATUS_SUCCESS)
        {
            // have an event
            KAsyncQueue<TestQueueItem>::DequeueOperation* p = static_cast<KAsyncQueue<TestQueueItem>::DequeueOperation*>(&CompletedContext);
            TestQueueItem& item = p->GetDequeuedItem();
            CountOfRxdTestQueueItems++;     // process the item
            item._WasProcessed = TRUE;

            // Start the next op - keep the pump going
            p->Reuse();
            KAsyncContextBase::CompletionCallback   pumpCallback(this, &StaticQueuePumpCallback::Callback);

            p->StartDequeue(nullptr, pumpCallback);
            return;
        }
        else if (CompletedContext.Status() == K_STATUS_SHUTDOWN_PENDING)
        {
            // done
            eventPtr->SetEvent();
            return;
        }

        KFatal(FALSE);      // Bad status
    }
};

static VOID
DroppedItemCallback(
    KAsyncQueue<TestQueueItem>& DeactivatingQueue,
    TestQueueItem& DroppedItem)
{
    UNREFERENCED_PARAMETER(DeactivatingQueue);

    DroppedItem._WasProcessed = TRUE;
}


NTSTATUS
BasicKAsyncQueueTest()
{
    KAsyncQueue<TestQueueItem>::SPtr        testQueue;
    NTSTATUS                                status;
    KAllocator&                             allocator = KtlSystem::GlobalNonPagedAllocator();
    KEvent                                  deactivatedEvent(FALSE, FALSE);
    StaticTestCallback                      Cb(&deactivatedEvent);
    KAsyncContextBase::CompletionCallback   deactivatedCallback(&Cb, &StaticTestCallback::Callback);

    status = KAsyncQueue<TestQueueItem>::Create(KTL_TAG_TEST, allocator, FIELD_OFFSET(TestQueueItem, _Links), testQueue);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("BasicKAsyncQueueTest: KAsyncQueue<TestQueueItem>::Create Failed: %i\n", __LINE__);
        return status;
    }

    status = testQueue->Activate(nullptr, deactivatedCallback);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("BasicKAsyncQueueTest: Activate() Failed: %i\n", __LINE__);
        return status;
    }

    // Start a logical pump
    KAsyncQueue<TestQueueItem>::DequeueOperation::SPtr  pumpOp;
    status = testQueue->CreateDequeueOperation(pumpOp);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("BasicKAsyncQueueTest: CreateDequeueOperation() Failed: %i\n", __LINE__);
        return status;
    }

    CountOfRxdTestQueueItems = 0;
    KEvent                                  pumpDoneEvent(FALSE, FALSE);
    StaticQueuePumpCallback                 Cb2(&pumpDoneEvent);
    KAsyncContextBase::CompletionCallback   pumpCallback(&Cb2, &StaticQueuePumpCallback::Callback);


    pumpOp->StartDequeue(nullptr, pumpCallback);
    const ULONG                             NumberOfTestItems = 10;
    TestQueueItem*                          testItems = _newArray<TestQueueItem>(KTL_TAG_TEST, allocator, NumberOfTestItems);

    // Make a bunch of TestQueueItems and send them thru the queue
    for (ULONG i = 0; i < NumberOfTestItems; i++)
    {
        TestQueueItem* item = &testItems[i];
        item->_ItemNumber = i;
        item->_WasProcessed = FALSE;
        status = testQueue->Enqueue(*item);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("BasicKAsyncQueueTest: Enqueue() Failed: %i\n", __LINE__);
            return status;
        }
    }

    testQueue->Deactivate();

    // prove we get a rejected enqueue
    {
        TestQueueItem   t;
        status = testQueue->Enqueue(t);
        if (status != K_STATUS_SHUTDOWN_PENDING)
        {
            KTestPrintf("BasicKAsyncQueueTest: Enqueue() unexpected status: %i\n", __LINE__);
            return status;
        }
    }

    pumpDoneEvent.WaitUntilSet();

    if (CountOfRxdTestQueueItems != NumberOfTestItems)
    {
        KTestPrintf("BasicKAsyncQueueTest: Missing test items: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    for (ULONG i = 0; i < NumberOfTestItems; i++)
    {
        if (!testItems[i]._WasProcessed)
        {
            KTestPrintf("BasicKAsyncQueueTest: _WasProcessed not set: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }
    }

    _deleteArray(testItems);
    testItems = nullptr;

    //
    // Sleep so that async has a chance to finish cleaning up before we
    // check if it has been successfully freed
    //
    KNt::Sleep(500);
    if (!pumpOp.Reset())
    {
        KTestPrintf("BasicKAsyncQueueTest: Expected destruction failed: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    deactivatedEvent.WaitUntilSet();
    if (testQueue->Status() != STATUS_SUCCESS)
    {
        KTestPrintf("BasicKAsyncQueueTest: Deactivation Failed: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    // Test Reuse()
    testQueue->Reuse();
    status = testQueue->Activate(nullptr, deactivatedCallback);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("BasicKAsyncQueueTest: Activate() Failed: %i\n", __LINE__);
        return status;
    }

    // Prove that a pending dequeue op get a K_STATUS_SHUTDOWN_PENDING when
    // Deactivation is issued; this also proves that an Active queue can be
    // deactivated successfully while there is a referencing dequeue op.
    status = testQueue->CreateDequeueOperation(pumpOp);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("BasicKAsyncQueueTest: CreateDequeueOperation() Failed: %i\n", __LINE__);
        return status;
    }

    KEvent                                  rejectedDequeueDoneEvent(FALSE, FALSE);
    StaticDequeueCallback                   Cb3(&rejectedDequeueDoneEvent);
    KAsyncContextBase::CompletionCallback   rejectedDequeueCallback(&Cb3, &StaticDequeueCallback::Callback);

    pumpOp->StartDequeue(nullptr, rejectedDequeueCallback);
    while (testQueue->GetNumberOfWaiters() == 0)
    {
        KNt::Sleep(0);          // Wait for dequeue to be pending
    }

    testQueue->Deactivate();
    deactivatedEvent.WaitUntilSet();
    if (testQueue->Status() != STATUS_SUCCESS)
    {
        KTestPrintf("BasicKAsyncQueueTest: Deactivation Failed: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    rejectedDequeueDoneEvent.WaitUntilSet();
    if (pumpOp->Status() != K_STATUS_SHUTDOWN_PENDING)
    {
        KTestPrintf("BasicKAsyncQueueTest:unexpected status: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    // Prove that an old pumpOp gets a K_STATUS_SHUTDOWN_PENDING completion when
    // applied against a new version of a queue
    testQueue->Reuse();
    pumpOp->Reuse();
    status = testQueue->Activate(nullptr, deactivatedCallback);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("BasicKAsyncQueueTest: Activate() Failed: %i\n", __LINE__);
        return status;
    }

    pumpOp->StartDequeue(nullptr, rejectedDequeueCallback);
    rejectedDequeueDoneEvent.WaitUntilSet();
    if (pumpOp->Status() != K_STATUS_SHUTDOWN_PENDING)
    {
        KTestPrintf("BasicKAsyncQueueTest:unexpected status: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    // prove that we can cancel a posted dequeue
    status = testQueue->CreateDequeueOperation(pumpOp);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("BasicKAsyncQueueTest: CreateDequeueOperation() Failed: %i\n", __LINE__);
        return status;
    }

    pumpOp->StartDequeue(nullptr, rejectedDequeueCallback);
    while (testQueue->GetNumberOfWaiters() == 0)
    {
        KNt::Sleep(0);          // Wait for dequeue to be pending
    }

    pumpOp->Cancel();
    rejectedDequeueDoneEvent.WaitUntilSet();
    if (pumpOp->Status() != STATUS_CANCELLED)
    {
        KTestPrintf("BasicKAsyncQueueTest:unexpected status: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    testQueue->Deactivate();
    deactivatedEvent.WaitUntilSet();
    if (testQueue->Status() != STATUS_SUCCESS)
    {
        KTestPrintf("BasicKAsyncQueueTest: Deactivation Failed: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    // Prove clean up
    if (!pumpOp.Reset())
    {
        KTestPrintf("BasicKAsyncQueueTest: Expected destruction failed: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    // Prove that the DroppedItemCallback during deactivation works
    testQueue->Reuse();
    status = testQueue->Activate(nullptr, deactivatedCallback);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("BasicKAsyncQueueTest: Activate() Failed: %i\n", __LINE__);
        return status;
    }

    testItems = _newArray<TestQueueItem>(KTL_TAG_TEST, allocator, NumberOfTestItems);

    // Make a bunch of TestQueueItems and queue them
    for (ULONG i = 0; i < NumberOfTestItems; i++)
    {
        TestQueueItem* item = &testItems[i];
        item->_ItemNumber = i;
        item->_WasProcessed = FALSE;
        status = testQueue->Enqueue(*item);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("BasicKAsyncQueueTest: Enqueue() Failed: %i\n", __LINE__);
            return status;
        }
    }

    KAsyncQueue<TestQueueItem>::DroppedItemCallback droppedItemCallback( &DroppedItemCallback);

    testQueue->Deactivate(droppedItemCallback);
    deactivatedEvent.WaitUntilSet();
    if (testQueue->Status() != STATUS_SUCCESS)
    {
        KTestPrintf("BasicKAsyncQueueTest: Deactivation Failed: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    for (ULONG i = 0; i < NumberOfTestItems; i++)
    {
        if (!testItems[i]._WasProcessed)
        {
            KTestPrintf("BasicKAsyncQueueTest: _WasProcessed not set: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }
    }

    _deleteArray(testItems);
    testItems = nullptr;

    // allow testQueue SM to catch up.
    KNt::Sleep(1000);

    if (!testQueue.Reset())
    {
        KTestPrintf("BasicKAsyncQueueTest: Expected destruction failed: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
CancelAllEnqueuedTest()
{
    KAsyncQueue<TestQueueItem>::SPtr        testQueue;
    NTSTATUS                                status;
    KAllocator&                             allocator = KtlSystem::GlobalNonPagedAllocator();
    KEvent                                  deactivatedEvent(FALSE, FALSE);
    StaticTestCallback                      cb(&deactivatedEvent);
    KAsyncContextBase::CompletionCallback   deactivatedCallback(&cb, &StaticTestCallback::Callback);

    status = KAsyncQueue<TestQueueItem>::Create(KTL_TAG_TEST, allocator, FIELD_OFFSET(TestQueueItem, _Links), testQueue);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("CancelAllEnqueuedTest: KAsyncQueue<TestQueueItem>::Create Failed: %i\n", __LINE__);
        return status;
    }

    status = testQueue->Activate(nullptr, deactivatedCallback);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("CancelAllEnqueuedTest: Activate() Failed: %i\n", __LINE__);
        return status;
    }

    //*** Prove that all can be cancelled without a deactivation
    const ULONG                             NumberOfTestItems = 100000;
    TestQueueItem*                          testItems = _newArray<TestQueueItem>(KTL_TAG_TEST, allocator, NumberOfTestItems);

    // Make a bunch of TestQueueItems and send them thru the queue
    for (ULONG i = 0; i < NumberOfTestItems; i++)
    {
        TestQueueItem* item = &testItems[i];
        item->_ItemNumber = i;
        item->_WasProcessed = FALSE;
        status = testQueue->Enqueue(*item);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CancelAllEnqueuedTest: Enqueue() Failed: %i\n", __LINE__);
            return status;
        }
    }

    KAsyncQueue<TestQueueItem>::DroppedItemCallback droppedItemCallback( &DroppedItemCallback);
    status = testQueue->CancelAllEnqueued(droppedItemCallback);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("CancelAllEnqueuedTest: CancelAllEnqueued() Failed: %i\n", __LINE__);
        return status;
    }

    for (ULONG i = 0; i < NumberOfTestItems; i++)
    {
        TestQueueItem* item = &testItems[i];
        if (!item->_WasProcessed)
        {
            KTestPrintf("CancelAllEnqueuedTest: _WasProcessed not set: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }
    }

    _deleteArray(testItems);
    testItems = nullptr;

    //*** Prove that all can be cancelled after a deactivation (without the drop option)
    testItems = _newArray<TestQueueItem>(KTL_TAG_TEST, allocator, NumberOfTestItems);

    // Make a bunch of TestQueueItems and send them thru the queue
    for (ULONG i = 0; i < NumberOfTestItems; i++)
    {
        TestQueueItem* item = &testItems[i];
        item->_ItemNumber = i;
        item->_WasProcessed = FALSE;
        status = testQueue->Enqueue(*item);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CancelAllEnqueuedTest: Enqueue() Failed: %i\n", __LINE__);
            return status;
        }
    }

    testQueue->Deactivate();
    status = testQueue->CancelAllEnqueued(droppedItemCallback);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("CancelAllEnqueuedTest: CancelAllEnqueued() Failed: %i\n", __LINE__);
        return status;
    }

    for (ULONG i = 0; i < NumberOfTestItems; i++)
    {
        TestQueueItem* item = &testItems[i];
        if (!item->_WasProcessed)
        {
            KTestPrintf("CancelAllEnqueuedTest: _WasProcessed not set: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }
    }

    _deleteArray(testItems);
    testItems = nullptr;

    // allow testQueue SM to catch up.
    KNt::Sleep(1000);

    if (!testQueue.Reset())
    {
        KTestPrintf("CancelAllEnqueuedTest: Expected destruction failed: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
KAsyncQueuePauseResumeDequeueTest()
{
    KAsyncQueue<TestQueueItem>::SPtr        testQueue;
    NTSTATUS                                status;
    KAllocator&                             allocator = KtlSystem::GlobalNonPagedAllocator();

    status = KAsyncQueue<TestQueueItem>::Create(KTL_TAG_TEST, allocator, FIELD_OFFSET(TestQueueItem, _Links), testQueue);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("KAsyncQueuePauseResumeDequeueTest: KAsyncQueue<TestQueueItem>::Create Failed: %i\n", __LINE__);
        return status;
    }

    KSynchronizer syncDeactivate;
    status = testQueue->Activate(nullptr, syncDeactivate);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("KAsyncQueuePauseResumeDequeueTest: Activate() Failed: %i\n", __LINE__);
        return status;
    }

    //
    // Pause dequeue
    //
    testQueue->PauseDequeue();

    //
    // Start a logical pump
    //
    KAsyncQueue<TestQueueItem>::DequeueOperation::SPtr  pumpOp;
    status = testQueue->CreateDequeueOperation(pumpOp);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("KAsyncQueuePauseResumeDequeueTest: CreateDequeueOperation() Failed: %i\n", __LINE__);
        return status;
    }

    KSynchronizer syncDequeueOp;
    pumpOp->StartDequeue(nullptr, syncDequeueOp);

    const ULONG NumberOfTestItems = 10;
    TestQueueItem* testItems = _newArray<TestQueueItem>(KTL_TAG_TEST, allocator, NumberOfTestItems);

    // Make a bunch of TestQueueItems and send them thru the queue
    for (ULONG i = 0; i < NumberOfTestItems; i++)
    {
        TestQueueItem* item = &testItems[i];
        item->_ItemNumber = i;
        item->_WasProcessed = FALSE;
        status = testQueue->Enqueue(*item);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("KAsyncQueuePauseResumeDequeueTest: Enqueue() Failed: %i\n", __LINE__);
            return status;
        }
    }

    //
    // Verify that we did not dequeue a single item
    //
    BOOLEAN isCompleted = FALSE;
    syncDequeueOp.WaitForCompletion(500, &isCompleted);
    if (isCompleted)
    {
        KTestPrintf("KAsyncQueuePauseResumeDequeueTest: PauseDequeue Failed, pumpDoneEvent was received: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    if (testQueue->GetNumberOfQueuedItems() != NumberOfTestItems)
    {
        KTestPrintf("KAsyncQueuePauseResumeDequeueTest: Missing test items: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    for (ULONG i = 0; i < NumberOfTestItems; i++)
    {
        if (testItems[i]._WasProcessed)
        {
            KTestPrintf("KAsyncQueuePauseResumeDequeueTest: _WasProcessed is set: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }
    }

    //
    // Good, all items are still in the queue. Now resume dequeue.
    //
    testQueue->ResumeDequeue();

    //
    // We should receive the dequeue event
    //
    syncDequeueOp.WaitForCompletion(500, &isCompleted);
    if (!isCompleted)
    {
        KTestPrintf("KAsyncQueuePauseResumeDequeueTest: ResumeDequeue Failed, pumpDoneEvent was NOT received: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // There should be NumberOfTestItems - 1 more  items in the queue
    //
    if (testQueue->GetNumberOfQueuedItems() != (NumberOfTestItems - 1))
    {
        KTestPrintf("KAsyncQueuePauseResumeDequeueTest: Incorrect number of test items in the queue: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    KAsyncQueue<TestQueueItem>::DroppedItemCallback droppedItemCallback( &DroppedItemCallback);
    testQueue->Deactivate(DroppedItemCallback);

    syncDeactivate.WaitForCompletion(1000, &isCompleted);
    if (!isCompleted)
    {
        KTestPrintf("KAsyncQueuePauseResumeDequeueTest: Failed to deactivate the queue: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    if (testQueue->Status() != STATUS_SUCCESS)
    {
        KTestPrintf("KAsyncQueuePauseResumeDequeueTest: Deactivation Failed: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    _deleteArray(testItems);

    return STATUS_SUCCESS;
}


//** KAsyncOrderGate Tests
struct TestGateItem
{
    KListEntry      _Links;
    BOOLEAN         _WasProcessed;
    ULONG           _Key;

    ULONG
    GetAsyncOrderedGateKey()
    {
        return _Key;
    }

    ULONG
    GetNextAsyncOrderedGateKey()
    {
        return _Key + 1;
    }
};

const ULONG             NumberOfTestGateItems = 10000;
ULONG                   CountOfRxdTestGateItems;

struct StaticGatePumpCallback
{
    PVOID _EventPtr;

    StaticGatePumpCallback(PVOID EventPtr)
    {
        _EventPtr = EventPtr;
    }

    void Callback(KAsyncContextBase* const Parent, KAsyncContextBase& CompletedContext)
    {
        UNREFERENCED_PARAMETER(Parent);

        KEvent* eventPtr = static_cast<KEvent*>(_EventPtr);
        if (CompletedContext.Status() == STATUS_SUCCESS)
        {
            // have an event
            KAsyncOrderedGate<TestGateItem, ULONG>::OrderedDequeueOperation* p;
            p = static_cast<KAsyncOrderedGate<TestGateItem, ULONG>::OrderedDequeueOperation*>(&CompletedContext);

            TestGateItem& item = p->GetDequeuedItem();
            item._WasProcessed = TRUE;

            // Verify order
            KFatal(CountOfRxdTestGateItems == item._Key);

            CountOfRxdTestGateItems++;     // process the item

            // Start the next op - keep the pump going
            p->Reuse();
            KAsyncContextBase::CompletionCallback   pumpCallback(this, &StaticGatePumpCallback::Callback);
            p->StartDequeue(nullptr, pumpCallback);
            return;
        }
        else if (CompletedContext.Status() == K_STATUS_SHUTDOWN_PENDING)
        {
            // done
            eventPtr->SetEvent();
            return;
        }

        KFatal(FALSE);      // Bad status
    }
};


NTSTATUS
BasicAsyncOrderedGateTest()
{
    KAsyncOrderedGate<TestGateItem, ULONG>::SPtr    testGate;
    NTSTATUS                                        status;
    KAllocator&                                     allocator = KtlSystem::GlobalNonPagedAllocator();
    KEvent                                          deactivatedEvent(FALSE, FALSE);
    StaticTestCallback                              cb(&deactivatedEvent);
    KAsyncContextBase::CompletionCallback           deactivatedCallback(&cb, &StaticTestCallback::Callback);
    ULONG                                           startingOrderValue = 0;

    status = KAsyncOrderedGate<TestGateItem, ULONG>::Create(KTL_TAG_TEST, allocator, FIELD_OFFSET(TestGateItem, _Links), testGate);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("BasicAsyncOrderedGateTest: KAsyncQueue<TestQueueItem>::Create Failed: %i\n", __LINE__);
        return status;
    }

    status = testGate->ActivateGate(startingOrderValue, nullptr, deactivatedCallback);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("BasicAsyncOrderedGateTest: Activate() Failed: %i\n", __LINE__);
        return status;
    }

        // Start a logical pump
    KAsyncOrderedGate<TestGateItem, ULONG>::OrderedDequeueOperation::SPtr  pumpOp;
    status = testGate->CreateOrderedDequeueOperation(pumpOp);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("BasicAsyncOrderedGateTest: CreateOrderedDequeueOperation() Failed: %i\n", __LINE__);
        return status;
    }

    CountOfRxdTestGateItems = 0;
    KEvent                                  pumpDoneEvent(FALSE, FALSE);
    StaticGatePumpCallback                  cb2(&pumpDoneEvent);
    KAsyncContextBase::CompletionCallback   pumpCallback(&cb2, &StaticGatePumpCallback::Callback);

    pumpOp->StartDequeue(nullptr, pumpCallback);
    TestGateItem*                           testItems = _newArray<TestGateItem>(KTL_TAG_TEST, allocator, NumberOfTestGateItems);

    // Make a bunch of TestGateItems; send them thru the gate pseudo randomly and uniquely keyed thru the range of 0..(NumberOfTestItems-1)
    for (ULONG i = 0; i < NumberOfTestGateItems; i++)
    {
        TestGateItem*   item = &testItems[i];
        ULONG           key = (NumberOfTestGateItems - 1) - i;
        key = (key & ~0x03) | ((key & 0x01) << 1) | ((key & 0x02) >> 1);

        item->_Key = key;
        item->_WasProcessed = FALSE;
        status = testGate->EnqueueOrdered(*item);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("BasicAsyncOrderedGateTest: EnqueueOrdered() Failed: %i\n", __LINE__);
            return status;
        }
    }

    testGate->DeactivateGate();
    deactivatedEvent.WaitUntilSet();
    if (testGate->Status() != STATUS_SUCCESS)
    {
        KTestPrintf("BasicAsyncOrderedGateTest: Deactivation Failed: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    // verify all items were processed
    for (ULONG i = 0; i < NumberOfTestGateItems; i++)
    {
        TestGateItem*   item = &testItems[i];
        if (!item->_WasProcessed)
        {
            KTestPrintf("BasicAsyncOrderedGateTest: _WasProcessed found false: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }
    }

    _deleteArray(testItems);
    testItems = nullptr;

    // allow testGate SM to catch up.
    KNt::Sleep(1000);

    pumpOp.Reset();

    if (!testGate.Reset())
    {
        KTestPrintf("BasicAsyncOrderedGateTest: Expected destruction failed: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
KAsyncQueueTest(int argc, WCHAR* args[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    NTSTATUS status = STATUS_SUCCESS;

    EventRegisterMicrosoft_Windows_KTL();
    KTestPrintf("Starting KAsyncQueueTest test\n");

    KtlSystem::Initialize();
    if ((status = BasicKAsyncQueueTest()) != STATUS_SUCCESS)
    {
        KTestPrintf("KAsyncQueue Unit Test: FAILED\n");
    }
    else if ((status = BasicAsyncOrderedGateTest()) != STATUS_SUCCESS)
    {
        KTestPrintf("KAsyncOrderedGate Unit Test: FAILED\n");
    }
    else if ((status = CancelAllEnqueuedTest()) != STATUS_SUCCESS)
    {
        KTestPrintf("CancelAllEnqueuedTest Unit Test: FAILED\n");
    }
    else if ((status = KAsyncQueuePauseResumeDequeueTest()) != STATUS_SUCCESS)
    {
        KTestPrintf("KAsyncQueuePauseResumeDequeueTest Unit Test: FAILED\n");
    }

    KtlSystem::Shutdown();

    EventUnregisterMicrosoft_Windows_KTL();
    return status;
}

NTSTATUS
KAsyncHelperTests(int argc, WCHAR* args[])
{
    KTestPrintf("KAsyncQueueUserTests: STARTED\n");
    NTSTATUS status = STATUS_SUCCESS;

#if defined(PLATFORM_UNIX)
    status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif

	
    if ((status = KQuotaGateTest(argc, args)) != STATUS_SUCCESS)
    {
        KTestPrintf("KQuotaGateTest Unit Test: FAILED\n");
    }
    else if ((status = KAsyncQueueTest(argc, args)) != STATUS_SUCCESS)
    {
        KTestPrintf("KAsyncQueueTest Unit Test: FAILED\n");
    }
    else if ((status = KAsyncEventTest(argc, args)) != STATUS_SUCCESS)
    {
        KTestPrintf("KAsyncEventTest Unit Test: FAILED\n");
    }
    else if ((status = KTimerTest(argc, args)) != STATUS_SUCCESS)
    {
        KTestPrintf("KTimerTest Unit Test: FAILED\n");
    }
    else if ((status = KAsyncServiceTest(argc, args)) != STATUS_SUCCESS)
    {
        KTestPrintf("KAsyncServiceTest Unit Test: FAILED\n");
    }

    KTestPrintf("KAsyncQueueUserTests: COMPLETED\n");

#if defined(PLATFORM_UNIX)
	KtlTraceUnregister();
#endif	
	
    return status;
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
    KAsyncHelperTests(argc, args);
    return 0;
}
#endif

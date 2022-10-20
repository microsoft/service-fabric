/*++

Copyright (c) Microsoft Corporation

Module Name:

    KThreadPoolTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KThreadPoolTests.h.
    2. Add an entry to the gs_KuTestCases table in KThreadPoolTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include "KThreadPoolTests.h"
#include <CommandLineParser.h>

#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif


class TestWorkItem : public KThreadPool::WorkItem {

    public:

        NOFAIL
        TestWorkItem(
            );

        VOID
        Execute(
            );

        VOID
        Wait(
            );

    private:

        KEvent _Event;
        LONG _RefCount;

};

TestWorkItem::TestWorkItem(
    )
{
    _RefCount = 6;
}

VOID
TestWorkItem::Execute(
    )
{
    LONG r;

    r = InterlockedDecrement(&_RefCount);

    KtlSystem& system = KtlSystem::GetDefaultKtlSystem();

    switch (r) {
        case 0:
            KFatal(!system.DefaultSystemThreadPool().IsCurrentThreadOwned());
            KFatal(system.DefaultThreadPool().IsCurrentThreadOwned());
            _Event.SetEvent();
            break;

        case 1:
            KFatal(!system.DefaultThreadPool().IsCurrentThreadOwned());
            system.DefaultThreadPool().QueueWorkItem(*this);
            break;

        case 2:
            KFatal(!system.DefaultThreadPool().IsCurrentThreadOwned());
            system.DefaultSystemThreadPool().QueueWorkItem(*this);
            break;

        case 3:
            KFatal(!system.DefaultSystemThreadPool().IsCurrentThreadOwned());
            KFatal(system.DefaultThreadPool().IsCurrentThreadOwned());
            system.DefaultSystemThreadPool().QueueWorkItem(*this);
            break;

        case 4:
            KFatal(!system.DefaultSystemThreadPool().IsCurrentThreadOwned());
            KFatal(system.DefaultThreadPool().IsCurrentThreadOwned());
            system.DefaultThreadPool().QueueWorkItem(*this);
            break;

        case 5:
            system.DefaultThreadPool().QueueWorkItem(*this);
            break;

        default:
            KFatal(FALSE);
            break;

    }
}

VOID
TestWorkItem::Wait(
    )
{
    _Event.WaitUntilSet();
}



NTSTATUS
KThreadPoolTestX(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    NTSTATUS status = STATUS_SUCCESS;
    TestWorkItem testWorkItem;
    TestWorkItem testWorkItem2;

    EventRegisterMicrosoft_Windows_KTL();

    KTestPrintf("Starting KThreadPoolTest test\n");

    //
    // The regular and system thread pool are already created as part of KtlSystem.  Just use those and make sure
    // they are working properly.  The test will not complete unless the thread pool is working properly.
    //

    //
    // Do some jumping around the 2 thread pools.  Start on the default thread pool.
    //

    KtlSystem::GetDefaultKtlSystem().DefaultThreadPool().QueueWorkItem(testWorkItem);

    //
    // Do it again, starting on the default system thread pool.
    //

    KtlSystem::GetDefaultKtlSystem().DefaultSystemThreadPool().QueueWorkItem(testWorkItem2);

    testWorkItem2.Wait();

    testWorkItem.Wait();

    EventUnregisterMicrosoft_Windows_KTL();

    return status;
}


#if KTL_USER_MODE 
class NestedWorkItem : public KThreadPool::WorkItem, public KShared<NestedWorkItem>
{
    KThreadPool&        _ThreadPool;
    KEvent&             _CompEvent;
    ULONG               _DepthToDo;
    ULONG               _DepthDone;
    static KThread::Id      g_ThreadId;
    static UCHAR*           g_TestAddr;

public:
    static LONG volatile    g_NumberDone;

    NestedWorkItem(KThreadPool& ThreadPool, ULONG DepthToDo, ULONG DepthDone, KEvent& CompEvent)
        :   _CompEvent(CompEvent),
            _DepthToDo(DepthToDo),
            _DepthDone(DepthDone),
            _ThreadPool(ThreadPool)
    {
    }

    void
    Execute() override
    {
        UCHAR       testAddr = 0;


        InterlockedIncrement(&g_NumberDone);

        // Level 1 is note one of our thread pool threads - it's the test main thread.
        if (_DepthDone == 2)
        {
            g_ThreadId = KThread::GetCurrentThreadId();
            g_TestAddr = &testAddr;
        }

        if (_DepthDone > 2)
        {
            // Prove we are still executing on the same thread and at the same stack depth
            KInvariant(KThread::GetCurrentThreadId() == g_ThreadId);
            KInvariant(&testAddr == g_TestAddr);
        }

        if (_DepthToDo != _DepthDone)
        {
            UCHAR           bigStackObj[4096];
            bigStackObj;

            NestedWorkItem::SPtr  child;

            child = _new(KTL_TAG_TEST, _ThreadPool.GetThisKtlSystem().GlobalPagedAllocator()) 
                NestedWorkItem(_ThreadPool, _DepthToDo, _DepthDone + 1, _CompEvent);

            // Keep ref on child for itself
            _ThreadPool.QueueWorkItem((NestedWorkItem&)*child.Detach(), TRUE);
        }
        else
        {
            _CompEvent.SetEvent();
        }

        Release();
    }
};

KThread::Id     NestedWorkItem::g_ThreadId;
LONG volatile   NestedWorkItem::g_NumberDone;
UCHAR*          NestedWorkItem::g_TestAddr;



void
TestNestedWorkItems()
{
    EventUnregisterMicrosoft_Windows_KTL();
    KAllocator&         allocator = KtlSystem::GlobalNonPagedAllocator();

    auto                Test = ([&] (KThreadPool& ThreadPool, WCHAR* TestName) -> void
    {
#if !defined(PLATFORM_UNIX)
        KTestPrintf("Starting: %S\n", TestName);
#else
        KTestPrintf("Starting: %s\n", Utf16To8(TestName).c_str());
#endif
        LONG                depthToTest = 30000000;
        KEvent              childrenPending(FALSE);   

        NestedWorkItem::g_NumberDone = 0;

        NestedWorkItem::SPtr    child = _new(KTL_TAG_TEST, allocator) NestedWorkItem(ThreadPool, (ULONG)depthToTest, 1, childrenPending);

        KDateTime           startTime = KDateTime::Now();
        ThreadPool.QueueWorkItem(*(NestedWorkItem*)child.Detach(), TRUE);
        childrenPending.WaitUntilSet();
        KDateTime       stopTime = KDateTime::Now();
        KInvariant(NestedWorkItem::g_NumberDone == depthToTest);

        KDuration           runTime = stopTime - startTime;
        double              msecsPerChild = (double)runTime.Milliseconds() / (double)depthToTest;
        double              nsecsPerChild = msecsPerChild * 1000000;

#if !defined(PLATFORM_UNIX)
        KTestPrintf("Completed: %S: %uns per work item allocation and dispatch\n", TestName, (ULONG)nsecsPerChild);
#else
        KTestPrintf("Completed: %s: %uns per work item allocation and dispatch\n", Utf16To8(TestName).c_str(), (ULONG)nsecsPerChild);
#endif
    });

    Test(KtlSystem::GetDefaultKtlSystem().DefaultSystemThreadPool(), L"KThreadPoolTest: TestNestedWorkItems: DefaultSystemThreadPool");
    Test(KtlSystem::GetDefaultKtlSystem().DefaultThreadPool(), L"KThreadPoolTest: TestNestedWorkItems: DefaultThreadPool");

    EventRegisterMicrosoft_Windows_KTL();
}


class DetachingWorkItem : public KThreadPool::WorkItem, public KShared<DetachingWorkItem>
{
    KThreadPool&        _ThreadPool;
    KSemaphore&         _CompEvent;
    static KEvent*      g_LastDoneEvent;

public:
    static LONG volatile    g_NumberDone;
    static LONG volatile    g_NumberToDo;

    DetachingWorkItem(KThreadPool& ThreadPool, KSemaphore& CompEvent)
        :   _CompEvent(CompEvent),
            _ThreadPool(ThreadPool)
    {
    }

    void
    Execute() override
    {
        // Cause the first work item to wait until the entire test (last work item + 1) has scheduled
        // and dispatched. Every even WI will Detach itself after it has queued an odd child. Odd
        // child WIs do not detach and do not wait on their child. This pattern exercises all of the
        // logic paths in the ThreadPool wrt detaching threads in and in combination with the per-
        // thread queue logic in UM.
        KSemaphore  mySema4;
        KInvariant(NT_SUCCESS(mySema4.Status()));

        LONG        myOrder = InterlockedIncrement(&g_NumberDone) - 1;

        if (myOrder == g_NumberToDo)
        {
            InterlockedDecrement(&g_NumberDone);
            g_LastDoneEvent->SetEvent();
            return;
        }

        DetachingWorkItem::SPtr  child;

        child = _new(KTL_TAG_TEST, _ThreadPool.GetThisKtlSystem().GlobalPagedAllocator()) 
            DetachingWorkItem(_ThreadPool, mySema4);

        // Keep ref on child for itself
        _ThreadPool.QueueWorkItem((DetachingWorkItem&)*child.Detach(), TRUE);

        if ((myOrder & 0x01) == 0)
        {
            _ThreadPool.DetachCurrentThread();      // Let the children go
            mySema4.Subtract();

            if (myOrder == 0)
            {
                // Wait for the last one to finish
                KEvent      myEvent(FALSE);
                g_LastDoneEvent = &myEvent;
                myEvent.WaitUntilSet();
                g_LastDoneEvent = nullptr;
#if !defined(PLATFORM_UNIX)
            }

            _CompEvent.Add();       // Signal parent
#else
                _CompEvent.Add();       // Signal parent
            }
#endif
            Release();
            return;
        }

        if ((myOrder & 0x01) == 1)
        {
            _CompEvent.Add();       // Signal parent
            Release();
            return;
        }
    }
};

LONG volatile   DetachingWorkItem::g_NumberDone;
LONG volatile   DetachingWorkItem::g_NumberToDo;
KEvent*         DetachingWorkItem::g_LastDoneEvent;


void
TestDetachThread()
{
    EventUnregisterMicrosoft_Windows_KTL();
    KAllocator&         allocator = KtlSystem::GlobalNonPagedAllocator();
    WCHAR*              testName = L"KThreadPoolTest: TestDetachThread";
    KThreadPool&        threadPool = allocator.GetKtlSystem().DefaultSystemThreadPool();
    
#if !defined(PLATFORM_UNIX)
    KTestPrintf("Starting: %S\n", testName);
#else
    KTestPrintf("Starting: %s\n", Utf16To8(testName).c_str());
#endif
    KSemaphore          childrenPending;   

    DetachingWorkItem::g_NumberDone = 0;
    DetachingWorkItem::g_NumberToDo = 10000000;


    DetachingWorkItem::SPtr    child = _new(KTL_TAG_TEST, allocator) DetachingWorkItem(threadPool, childrenPending);

    KDateTime           startTime = KDateTime::Now();
    threadPool.QueueWorkItem(*(DetachingWorkItem*)child.Detach(), TRUE);
    childrenPending.Subtract();
    KDateTime       stopTime = KDateTime::Now();
    KInvariant(DetachingWorkItem::g_NumberDone ==  DetachingWorkItem::g_NumberToDo);

    KDuration           runTime = stopTime - startTime;
    double              msecsPerChild = (double)runTime.Milliseconds() / (double)DetachingWorkItem::g_NumberToDo;
    double              nsecsPerChild = msecsPerChild * 1000000;

#if !defined(PLATFORM_UNIX)
    KTestPrintf("Completed: %S: %uns per work item allocation and dispatch\n", testName, (ULONG)nsecsPerChild);
#else
    KTestPrintf("Completed: %s: %uns per work item allocation and dispatch\n", Utf16To8(testName).c_str(), (ULONG)nsecsPerChild);
#endif

    EventRegisterMicrosoft_Windows_KTL();
}
#endif

//** Test coroutine form of ParameterizedWorkItem
#if defined(K_UseResumable)

ktl::Awaitable<void> TestParameterizedWorkItemAsync(KtlSystem& System)
{
    KAllocator&         allocator = System.GlobalNonPagedAllocator();
    KThreadPool&        threadPool = System.DefaultSystemThreadPool();
    using               MyWorkItemType = KThreadPool::ParameterizedWorkItem<KAsyncEvent&>;

    KAsyncEvent                     workItemProcessed;
    KAsyncEvent::WaitContext::SPtr  waitContext;

    KInvariant(NT_SUCCESS(workItemProcessed.CreateWaitContext(KTL_TAG_TEST, allocator, waitContext)));

    MyWorkItemType::WorkItemProcessor worker = [](KAsyncEvent& CompletionEvent)
    {
        KNt::Sleep(1000);
        CompletionEvent.SetEvent();
    };

    MyWorkItemType      workItem(workItemProcessed, worker);
    ULONGLONG           minEndTimeExpected = KNt::GetTickCount64() + 1000;

    threadPool.QueueWorkItem(workItem);
    NTSTATUS status = co_await waitContext->StartWaitUntilSetAsync(nullptr);
    KInvariant(NT_SUCCESS(status));
    KInvariant(KNt::GetTickCount64() >= minEndTimeExpected);

    co_return;
}

#endif

void TestParameterizedWorkItem()
{
    EventUnregisterMicrosoft_Windows_KTL();
    KAllocator&         allocator = KtlSystem::GlobalNonPagedAllocator();
    KThreadPool&        threadPool = allocator.GetKtlSystem().DefaultSystemThreadPool();
    using               MyWorkItemType = KThreadPool::ParameterizedWorkItem<KAutoResetEvent&>;
    
    KAutoResetEvent     workItemProcessed;
    MyWorkItemType::WorkItemProcessor worker = [](KAutoResetEvent& CompletionEvent)
    {
        KNt::Sleep(1000);
        CompletionEvent.SetEvent();
    };

    MyWorkItemType      workItem(workItemProcessed, worker);
    ULONGLONG           minEndTimeExpected = KNt::GetTickCount64() + 1000;

    threadPool.QueueWorkItem(workItem);
    workItemProcessed.WaitUntilSet();

    KInvariant(KNt::GetTickCount64() >= minEndTimeExpected);

    #if defined(K_UseResumable)
    ktl::SyncAwait(TestParameterizedWorkItemAsync(allocator.GetKtlSystem()));
    #endif

    EventRegisterMicrosoft_Windows_KTL();
}

#if KTL_USER_MODE
__declspec(thread) int  tls_TestCoreLoadingCount = 0;
bool                    TestCoreLoadingDone = false;
volatile int            TestCoreLoadingThreadCount = 0;
volatile LONGLONG       TotalTestCoreLoadingThreadCount = 0;
KThreadPool*            TestCoreLoadingThreadPool = nullptr;

void TestCoreLoading()
{
    EventUnregisterMicrosoft_Windows_KTL();
    KAllocator&         allocator = KtlSystem::GlobalNonPagedAllocator();
    TestCoreLoadingThreadPool = &allocator.GetKtlSystem().DefaultThreadPool();
    TestCoreLoadingThreadCount = TestCoreLoadingThreadPool->GetThreadCount();

    class MyWorkItemType : public KThreadPool::WorkItem
    {
    public:
        void Execute() override
        {
            if (tls_TestCoreLoadingCount == 0)
            {
                KTestPrintf("TestCoreLoading: Thread Start: %llu\n", KThread::GetCurrentThreadId());
            }
            tls_TestCoreLoadingCount++;
            InterlockedIncrement64(&TotalTestCoreLoadingThreadCount);

            if (TestCoreLoadingDone)
            {
                KTestPrintf("TestCoreLoading: Thread Stop: %llu; Count: %i\n", KThread::GetCurrentThreadId(), tls_TestCoreLoadingCount);
            }
            else
            {
                // Not stopping - reprime workitem
                TestCoreLoadingThreadPool->QueueWorkItem(*this, FALSE);
            }
        }
    };

    MyWorkItemType*     workItems = _newArray<MyWorkItemType>(KTL_TAG_TEST, allocator, TestCoreLoadingThreadCount);

    // Prime 1 WI / Core
    for (int ix = 0; ix < TestCoreLoadingThreadCount; ix++)
    {
        TestCoreLoadingThreadPool->QueueWorkItem(workItems[ix], FALSE);
    }

    KNt::Sleep(1000);               // let all threads display their entre'
    KTestPrintf("TestCoreLoading: Observe CPU loading for next 3mins - all cores should be 100%% loaded\n");
    KNt::Sleep(3 * 60 * 1000);
    TestCoreLoadingDone = true;
    KNt::Sleep(1000);

    _deleteArray<MyWorkItemType>(workItems);

    KTestPrintf("TestCoreLoading: %llu IOCP events processed over 3mins\n", TotalTestCoreLoadingThreadCount);
    EventRegisterMicrosoft_Windows_KTL();
}
#endif

NTSTATUS
KThreadPoolTest(
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
    
    KTestPrintf("KThreadPoolTest: START\n");
    
    status = KtlSystem::Initialize();
    if (!NT_SUCCESS(status)) 
    {
        return status;
    }

    #if KTL_USER_MODE
        TestCoreLoading();
    #endif
    TestParameterizedWorkItem();
    status = KThreadPoolTestX(argc, args);
    if (NT_SUCCESS(status))
    {
        #if KTL_USER_MODE 
        {
            TestDetachThread();
            TestNestedWorkItems();
        }
        #endif
    }

    KtlSystem::Shutdown();

    KTestPrintf("KThreadPoolTest: COMPLETE: Status: %u\n", status);

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

    return RtlNtStatusToDosError(KThreadPoolTest(0, NULL));
}
#endif

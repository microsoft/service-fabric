/*++

Copyright (c) Microsoft Corporation

Module Name:

    KtlAwaitableTests.cpp - Unit tests that prove KTL++ co_routine awaitablity support

Abstract:

    This file contains test case implementations for some common KTL functionalities.

    To add a new unit test case:
    1. Declare your test case routine in KtlCommonTests.h.
    2. Add an entry to the gs_KuTestCases table in KtlCommonTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#define KCo$IncludeAllocationCounting
#define KCo$ForceTaskFrameAllocationFailure
#define KCo$ForceAwaitableTFrameAllocationFailure
#define KCo$UnitTest$AwaitableStateRaceTestCount
#define KtlEitherReady_TestFastPath
#define KtlEitherReady_TestHoldAwDelay

volatile long       _g_K$UnitTest$AwaitableStateRaceTestCount = 0;
volatile long       _g_K$UnitTest$AwaitableStateRaceTestResultReadyCount = 0;
volatile long       _g_K$UnitTest$AwaitableStateRaceTestSuspendedCount = 0;
volatile long       _g_K$UnitTest$AwaitableStateRaceTestFinalHeldCount = 0;
bool                _g_ForceTaskFrameAllocationFailure = false;
bool                _g_ForceAwaitableTFrameAllocationFailure = false;
bool                _g_KtlEitherReady_TestFastPath = true;
unsigned long       _g_KtlEitherReady_TestHoldAwDelay = 0;
unsigned long       _g_KCo$TotalObjects = 0;

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include "KtlAwaitableTests.h"
#include <CommandLineParser.h>
#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif
#include <KTpl.h>
#include <functional>

#if defined(PLATFORM_UNIX)
#include <pthread.h>
#endif

#if KTL_USER_MODE
extern volatile LONGLONG gs_AllocsRemaining;
#endif

using namespace ktl;
using namespace ktl::kasync;
using namespace ktl::kservice;

volatile ULONGLONG      AcsMonitorCount = 0;
AwaitableState      _g_AwaitableStateUnderTest;
KEvent              _g_EntryHoldEvent;
KEvent              _g_DoneHoldEvent;
volatile LONG       _g_ThreadsHeld;
bool                _g_EndRaceTest = false;


void SleepUntil(std::function<bool()> condition)
{
    static const int interval = 100;
    static const int totalTime = 10000;
    static const int iters = totalTime/interval;
    int cnt = 0;
    MemoryBarrier();
    while (!condition())
    {
        KNt::Sleep(interval);
        KInvariant(++cnt != iters);
        MemoryBarrier();
    }
}


//* Test Service used to verify ktl++ awaitable support for KAsyncServiceBase
class MyTestService : public KAsyncServiceBase
{
   K_FORCE_SHARED(MyTestService);

public:
   using KAsyncServiceBase::SetDeferredCloseBehavior;

   static NTSTATUS Create(MyTestService::SPtr& Result, KAllocator& Allocator)
   {
      Result = _new(KTL_TAG_TEST, Allocator) MyTestService();
      if (Result == nullptr)
      {
         return STATUS_INSUFFICIENT_RESOURCES;
      }

      return STATUS_SUCCESS;
   }

   Awaitable<NTSTATUS> StartFailedNullOpenTestAsync()
   {
        co_await suspend_never{};

        co_return{ STATUS_ABANDONED };
   }

   Awaitable<NTSTATUS> StartFailedNullCloseTestAsync()
   {
       co_await suspend_never{};
       
       co_return STATUS_ABANDONED_WAIT_0;
   }

   Awaitable<void> DoServiceFailedOpenCloseTest()
   {
      // TDO: Pull? a bogus test
      puts("MyTestService::DoServiceFailedOpenCloseTest: Entry");
      NTSTATUS status = co_await this->StartFailedNullOpenTestAsync();
      KInvariant(status == STATUS_ABANDONED);
      status = co_await this->StartFailedNullCloseTestAsync();
      KInvariant(status == STATUS_ABANDONED_WAIT_0);
      puts("MyTestService::DoServiceFailedOpenCloseTest: Exit");
   }

   Awaitable<NTSTATUS>
   StartOpenAsync(
       __in_opt KAsyncContextBase* const ParentAsync,
       __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr)
   {
       ktl::kservice::OpenAwaiter::SPtr awaiter;

       NTSTATUS status = ktl::kservice::OpenAwaiter::Create(
           GetThisAllocator(),
           GetThisAllocationTag(),
           *this,
           awaiter,
           ParentAsync,
           GlobalContextOverride);

       if (!NT_SUCCESS(status))
       {
           co_return{ status };
       }

       status = co_await *awaiter;
       co_return status;
   }

   Awaitable<NTSTATUS>
   StartCloseAsync(
        __in_opt KAsyncContextBase* const ParentAsync,
        __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr)
    {
       ktl::kservice::CloseAwaiter::SPtr awaiter;

       NTSTATUS status = ktl::kservice::CloseAwaiter::Create(
           GetThisAllocator(),
           GetThisAllocationTag(),
           *this,
           awaiter,
           ParentAsync,
           GlobalContextOverride);

       if (!NT_SUCCESS(status))
       {
           co_return{ status };
       }

       status = co_await *awaiter;
       co_return status;
   }

   /* Test APIs */
   Task TaskTest(KAsyncEvent& CompletionEvent, BOOLEAN HasDeferredClose)        // Task
   {
      KCoService$TaskEntry(HasDeferredClose);
      puts("MyTestService::TaskTest: Entry");

      puts("MyTestService::TaskTest: Entry: Waiting 2 secs");
      NTSTATUS status = co_await KTimer::StartTimerAsync(GetThisAllocator(), KTL_TAG_TEST, 2000, nullptr);
      KInvariant(status == STATUS_SUCCESS);
      puts("MyTestService::TaskTest: Exit: Signaling CompletionEvent");
      CompletionEvent.SetEvent();
   }

   Awaitable<NTSTATUS> AwaitableTest(BOOLEAN HasDeferredClose)      // Awaitable
   {
      puts("MyTestService::AwaitableTest: Entry");
      KCoService$ApiEntry(HasDeferredClose);

      puts("MyTestService::AwaitableTest: Entry: Waiting 2 secs");
      NTSTATUS status = co_await KTimer::StartTimerAsync(GetThisAllocator(), KTL_TAG_TEST, 2000, nullptr);
      puts("MyTestService::AwaitableTest: Exit");
      co_return status;
   }
};

MyTestService::MyTestService() {}
MyTestService::~MyTestService() {}

// Simple Service test
Awaitable<NTSTATUS> SimpleServiceTests(KAllocator& Alloc, char* Str)
{
   MyTestService::SPtr      service;
   KAsyncEvent              compEvent;
   KAsyncEvent::WaitContext::SPtr   eventWaitContext;

   NTSTATUS     status = MyTestService::Create(service, Alloc);
   if (!NT_SUCCESS(status))
   {
      co_return status;
   }

   status = compEvent.CreateWaitContext(KTL_TAG_TEST, Alloc, eventWaitContext);
   if (!NT_SUCCESS(status))
   {
      co_return status;
   }

   co_await service->DoServiceFailedOpenCloseTest();

   //* KAsyncService ktl::Task API tests

   // Test non-deferred close with a Task + standard entry macro
   status = co_await service->StartOpenAsync(nullptr, nullptr);
   if (!NT_SUCCESS(status))
   {
      co_return status;
   }

   service->TaskTest(compEvent, false);
   co_await eventWaitContext->StartWaitUntilSetAsync(nullptr);

   status = co_await service->StartCloseAsync(nullptr, nullptr);
   if (!NT_SUCCESS(status))
   {
      co_return status;
   }

   // Test deferred close with a Task + standard entry macro
   eventWaitContext->Reuse();
   service->Reuse();
   service->SetDeferredCloseBehavior();
   status = co_await service->StartOpenAsync(nullptr, nullptr);
   if (!NT_SUCCESS(status))
   {
      co_return status;
   }

   service->TaskTest(compEvent, true);
   co_await eventWaitContext->StartWaitUntilSetAsync(nullptr);

   status = co_await service->StartCloseAsync(nullptr, nullptr);
   if (!NT_SUCCESS(status))
   {
      co_return status;
   }

   //* KAsyncService Awaitable<NTSTATUS> API tests

   // Test non-deferred close with a Awaitable + standard entry macro
   service->Reuse();
   status = co_await service->StartOpenAsync(nullptr, nullptr);
   if (!NT_SUCCESS(status))
   {
      co_return status;
   }

   status = co_await service->AwaitableTest(false);
   if (!NT_SUCCESS(status))
   {
      co_return status;
   }

   status = co_await service->StartCloseAsync(nullptr, nullptr);
   if (!NT_SUCCESS(status))
   {
      co_return status;
   }

   // Test deferred close with a Awaitable + standard entry macro
   service->Reuse();
   service->SetDeferredCloseBehavior();
   status = co_await service->StartOpenAsync(nullptr, nullptr);
   if (!NT_SUCCESS(status))
   {
      co_return status;
   }

   status = co_await service->AwaitableTest(true);
   if (!NT_SUCCESS(status))
   {
      co_return status;
   }

   status = co_await service->StartCloseAsync(nullptr, nullptr);
   if (!NT_SUCCESS(status))
   {
      co_return status;
   }

   co_return STATUS_SUCCESS;
}


/* Test KAsync used to verify KAsyncContextBase awaitable behaviors */
class AsyncAwaiterTestContext : public KAsyncContextBase
{
   K_FORCE_SHARED_WITH_INHERITANCE(AsyncAwaiterTestContext);

public:
   static AsyncAwaiterTestContext::SPtr Create(KAllocator& Allocator)
   {
      return SPtr(_new(KTL_TAG_TEST, Allocator) AsyncAwaiterTestContext());
   }

   ktl::Awaitable<NTSTATUS> StartAwaiterAsync()
   {
       ktl::kasync::StartAwaiter::SPtr awaiter;

       NTSTATUS status = ktl::kasync::StartAwaiter::Create(
           GetThisAllocator(),
           GetThisAllocationTag(),
           *this,
           awaiter,
           nullptr,
           nullptr);

       if (!NT_SUCCESS(status))
       {
           co_return{ status };
       }

       status = co_await *awaiter;
       co_return status;
   }

   Awaitable<NTSTATUS> StartAsync()
   {
      KCoShared$ApiEntry();

      this->_testType = TestType::simpleKtaskTest;

#if !defined(PLATFORM_UNIX) // clang//llvm bug
      byte          bytes[32];
      for (int i = 0; i < 32; i++)
      {
         bytes[i] = (byte)i;
      }
#endif

      NTSTATUS status = co_await StartAwaiterAsync();

#if !defined(PLATFORM_UNIX) // clang//llvm bug
      for (int i = 0; i < 32; i++)
      {
         bytes[i]++;
      }
#endif

      co_return status;
   }

   ktl::Awaitable<NTSTATUS> StartAwaiterAsync1()
   {
       ktl::kasync::StartAwaiter::SPtr awaiter;

       NTSTATUS status = ktl::kasync::StartAwaiter::Create(
           GetThisAllocator(),
           GetThisAllocationTag(),
           *this,
           awaiter,
           nullptr,
           nullptr);

       if (!NT_SUCCESS(status))
       {
           co_return{ status };
       }

       status = co_await *awaiter;
       co_return status;
   }

   Awaitable<NTSTATUS> StartAsync1()
   {
      KCoShared$ApiEntry();

      this->_testType = simpleKtaskTest;
      co_return co_await StartAwaiterAsync1();
   }

   void StartApiTest(KAsyncContextBase* Parent, CompletionCallback Callback)
   {
      this->_testType = justStart;
      Start(Parent, Callback);
   }

   Awaitable<NTSTATUS> AwaitableMethod1()
   {
      KCoAsync$ApiEntry();
      NTSTATUS  status = STATUS_SUCCESS;

      KTimer::SPtr  localTimer;

      status = KTimer::Create(localTimer, GetThisAllocator(), KTL_TAG_TEST);
      if (!NT_SUCCESS(status))
      {
         co_return status;
      }

      puts("AwaitableMethod1: 1 sec delay");
      co_return co_await localTimer->StartTimerAsync(1000, nullptr);
   }

   ktl::Awaitable<NTSTATUS> StartFailedNullTestAsync()
   {
       ktl::kasync::StartAwaiter::SPtr awaiter;

       NTSTATUS status = ktl::kasync::StartAwaiter::Create(
           GetThisAllocator(),
           GetThisAllocationTag(),
           *this,
           awaiter,
           nullptr,
           nullptr);

       if (!NT_SUCCESS(status))
       {
           co_return{ status };
       }

       this->_testType = completeWithSTATUS_ABANDONED_WAIT_63;
       status = co_await *awaiter;
       co_return status;
   }

   Awaitable<void> DoNullFailedTest()
   {
      // Prove that a sync co_return works ok with a kasync::StartAwaiter
      puts("AsyncAwaiterTestContext::DoNullFailedTest: Entry");
      NTSTATUS status = co_await this->StartFailedNullTestAsync();
      KInvariant(status == STATUS_ABANDONED_WAIT_63);
      puts("AsyncAwaiterTestContext::DoNullFailedTest: Exit");
   }

private:
   KAsyncEvent _joinEvent;
   enum TestType
   {
      simpleKtaskTest,
      justStart,
      completeWithSTATUS_ABANDONED_WAIT_63,
      undefined
   };

   TestType _testType;


private:
   #pragma warning(disable:4701)           // VS C++ Update 2 bug
   Task OnStartAsync1(KAsyncEvent& JoinEvent)
   {
      KCoAsync$TaskEntry();

      KSpinLock     myLock;

      for (int ix2 = 0; ix2 < 100; ix2++)
      {
         KInvariant(!myLock.IsOwned());

         K_LOCK_BLOCK(myLock)
         {
            KInvariant(myLock.IsOwned());

            KTimer::SPtr    testTimer;
            NTSTATUS status = KTimer::Create(testTimer, GetThisAllocator(), KTL_TAG_TEST);

            if (!NT_SUCCESS(status))
            {
               Complete(status);
               co_return;
            }

            status = co_await testTimer->StartTimerAsync(20, this);
            if (!NT_SUCCESS(status))
            {
               Complete(status);
               co_return;
            }
            testTimer->Reuse();

            status = co_await testTimer->StartTimerAsync(30, this);
         }

         KInvariant(!myLock.IsOwned());
      }

      this->_joinEvent.SetEvent();
   }

   Task OnStartAsync()
   {
      KCoAsync$TaskEntry();

      // Form another parallel Task
      OnStartAsync1(_joinEvent);

      KTimer::SPtr  testTimer;
      NTSTATUS status = KTimer::Create(testTimer, GetThisAllocator(), KTL_TAG_TEST);
      if (!NT_SUCCESS(status))
      {
         Complete(status);
         co_return;
      }

      status = co_await testTimer->StartTimerAsync(5000, nullptr);
      if (!NT_SUCCESS(status))
      {
         Complete(status);
         co_return;
      }

      KSpinLock     myLock;

      for (int ix1 = 0; ix1 < 100; ix1++)
      {
         KInvariant(!myLock.IsOwned());

         K_LOCK_BLOCK(myLock)
         {
            KInvariant(myLock.IsOwned());

            testTimer->Reuse();
            status = co_await testTimer->StartTimerAsync(20, this);
            if (!NT_SUCCESS(status))
            {
               Complete(status);
               co_return;
            }

            testTimer->Reuse();
            status = co_await testTimer->StartTimerAsync(30, this);
         }

         KInvariant(!myLock.IsOwned());
      }

      KAsyncEvent::WaitContext::SPtr    waitContext;
      status = _joinEvent.CreateWaitContext(KTL_TAG_TEST, GetThisAllocator(), waitContext);
      if (!NT_SUCCESS(status))
      {
         Complete(status);
         co_return;
      }

      status = co_await waitContext->StartWaitUntilSetAsync(this);
      Complete(status);
   }
   #pragma warning(default:4701)

   void OnStart() override
   {
      switch (_testType)
      {
      case simpleKtaskTest: OnStartAsync();
         break;

      case justStart:
         break;

      case completeWithSTATUS_ABANDONED_WAIT_63:
          Complete(STATUS_ABANDONED_WAIT_63);
          break;

      default: KInvariant(false);
         break;
      }
   }

   void OnReuse()
   {
      _testType = undefined;
   }
};

AsyncAwaiterTestContext::AsyncAwaiterTestContext()
{
   _testType = TestType::undefined;
}

AsyncAwaiterTestContext::~AsyncAwaiterTestContext()
{
}

// Test Global parallel tasks with global method
Awaitable<NTSTATUS> ExecAwaiterParallelKAsync(AsyncAwaiterTestContext& async)
{
   async.Reuse();
   NTSTATUS status = co_await async.StartAsync1();
   if (!NT_SUCCESS(status))
   {
       co_return{ status };
   }

   KTimer::SPtr testTimer;
   status = KTimer::Create(testTimer, async.GetThisAllocator(), KTL_TAG_TEST);
   if (!NT_SUCCESS(status))
   {
       co_return{ status };
   }

   puts("ExecAwaiterParallelKAsync: starting WaitAsync: 2 sec wait");
   status = co_await testTimer->StartTimerAsync(2000, nullptr);
   co_return{ status };
}

//* Test KShared<> class used to verify the proper awaitable behavior on simple heap objects
//  with life time correctness enforced
//
class GClass : public KObject<GClass>, public KShared<GClass>, public KTagBase<GClass, KTL_TAG_TEST>
{
   K_FORCE_SHARED(GClass);

public:
   static GClass::SPtr  Create(KAllocator& Allocator)
   {
      return SPtr(_new(KTL_TAG_TEST, Allocator) GClass());
   }

   Awaitable<int> KSharedTypedAwaitableIMethod(int ValueToReturn)
   {
      KCoShared$ApiEntry();         // Test when caller drops refcount --- optional  

      printf("GClass::KSharedTypedAwaitableIMethod: ValueToReturn: %i\n", ValueToReturn);

      KTimer::SPtr  myTimer;
      KInvariant(NT_SUCCESS(KTimer::Create(myTimer, GetThisAllocator(), KTL_TAG_TEST)));

      NTSTATUS status = co_await myTimer->StartTimerAsync(1000, nullptr);
      KInvariant(status == STATUS_SUCCESS);
      puts("GClass::KSharedTypedAwaitableIMethod: After 1sec pause");

      myTimer->Reuse();
      co_await myTimer->StartTimerAsync(2000, nullptr);
      puts("GClass::KSharedTypedAwaitableIMethod: After 2sec pause");

      co_return ValueToReturn;
   }

   static bool  testGClassDtored;
};

GClass::GClass() {}
GClass::~GClass()
{
   testGClassDtored = true;
   puts("GClass::~GClass");
}

bool    GClass::testGClassDtored = false;

//* Simple unit tests for co_awaitable ktl::Awaitable<T> funtionality; also
//  proves simple KShared<> awaitable instance methods works; the KCoShared$ApiEntry
//  macro is also proven
//
Awaitable<int> BasicAwaitableTypeTestImp(KAllocator& AllocIn, int ValueToReturn)
{
   printf("BasicAwaitableTypeTestImp: Entry: ValueToReturn: %i\n", ValueToReturn);

   KAllocator& Alloc = AllocIn;

   auto& p = *co_await CorHelper::GetCurrentPromise<Awaitable<int>::promise_type>();
   auto h = coroutine_handle<Awaitable<int>::promise_type>::from_promise(p);
   h;

   auto ps = p.GetState();
   KInvariant(ps.IsUnusedBitsValid());

   co_await CorHelper::ThreadPoolThread(Alloc.GetKtlSystem().DefaultSystemThreadPool());

   GClass::testGClassDtored = false;
   GClass::SPtr g = GClass::Create(Alloc);
   auto x = g->KSharedTypedAwaitableIMethod(ValueToReturn);
   KInvariant(!GClass::testGClassDtored);

   int q = co_await x;
   printf("BasicAwaitableTypeTestImp: GClass::KSharedTypedAwaitableIMethod() returned: %i\n", q);
   g = nullptr;

   co_return q;
}

Awaitable<int> BasicAwaitableTypeTest(KAllocator& Alloc, int ValueToReturn)
{
   printf("BasicAwaitableTypeTest: ValueToReturn: %i\n", ValueToReturn);

   int r = co_await BasicAwaitableTypeTestImp(Alloc, ValueToReturn);
   printf("BasicAwaitableTypeTest: After co_await: result: %i\n", r);

   co_return{ r };
}

//* Simple unit tests for co_awaitable ktl::Awaitable<> funtionality; also proves 
//  KTimer awaitable funtionality
//
Awaitable<KTimer::SPtr> BasicAwaitableVoidTestImp(KAllocator& Alloc, char* Str)
{
   printf("BasicAwaitableVoidTestImp: Entry: %s\n", Str);

   KTimer::SPtr myTimer;
   KInvariant(NT_SUCCESS(KTimer::Create(myTimer, Alloc, KTL_TAG_TEST)));

   co_await myTimer->StartTimerAsync(1000, nullptr);
   puts("BasicAwaitableVoidTestImp: After 1sec pause");

   myTimer->Reuse();
   co_await myTimer->StartTimerAsync(2000, nullptr);
   puts("BasicAwaitableVoidTestImp: After 2sec pause");

   NTSTATUS status = co_await KTimer::StartTimerAsync(Alloc, KTL_TAG_TEST, 1500, nullptr);
   KInvariant(NT_SUCCESS(status));
   puts("BasicAwaitableVoidTestImp: After 1500 pause: static form of KTimer::StartTimerAsync()");

   printf("BasicAwaitableVoidTestImp: Exit: %s\n", Str);
   co_return{ myTimer };
}

Awaitable<void> BasicAwaitableVoidTest(KAllocator& Alloc, char* Str)
{
   puts("BasicAwaitableVoidTest: Entry");
   KTimer::SPtr myTimer = co_await BasicAwaitableVoidTestImp(Alloc, Str);
   puts("BasicAwaitableVoidTest: Exit");
}


//* Simple unit tests for co_awaitable ktl::Awaitable<NTSTATUS> funtionality
Awaitable<NTSTATUS> BasicFutureTypedTest(KAsyncEvent::WaitContext& AsyncEventWaitContext, BOOLEAN& HasStarted)
{
   puts("BasicFutureTypedTest: Entry");

   KInvariant(!HasStarted);
   HasStarted = true;

   NTSTATUS  status = co_await AsyncEventWaitContext.StartWaitUntilSetAsync(nullptr);
   if (!NT_SUCCESS(status))
   {
      co_return status;
   }

   KInvariant(!HasStarted);
   HasStarted = true;

   puts("BasicFutureTypedTest: Exit");

   co_return STATUS_WAIT_63;
}

//* Simple unit tests for co_awaitable ktl::Awaitable<> funtionality
Awaitable<void> BasicFutureTest(KAsyncEvent::WaitContext& AsyncEventWaitContext, BOOLEAN& HasStarted)
{
   puts("BasicFutureTest: Entry");

   KInvariant(!HasStarted);
   HasStarted = true;

   NTSTATUS  status = co_await AsyncEventWaitContext.StartWaitUntilSetAsync(nullptr);
   KInvariant(status == STATUS_SUCCESS);

   KInvariant(!HasStarted);
   HasStarted = true;

   puts("BasicFutureTest: Exit");
   co_return;
}

//* Simple unit tests for co_awaitable ktl::kasync::StartAwaiter funtionality
Task BasicStartAwaiterTest(KAsyncEvent::WaitContext& AsyncEventWaitContext, BOOLEAN& HasStarted, KEvent& CompEvent)
{
   puts("BasicStartAwaiterTest: Entry");

   KInvariant(!HasStarted);
   HasStarted = true;

   NTSTATUS  status = co_await AsyncEventWaitContext.StartWaitUntilSetAsync(nullptr);
   KInvariant(status == STATUS_SUCCESS);

   KInvariant(!HasStarted);
   HasStarted = true;

   puts("BasicStartAwaiterTest: Exit");
   CompEvent.SetEvent();
}


//* Simple unit tests for waitable ktl::Task funtionality
Task BasicTaskTest(KEvent& SyncEvent, BOOLEAN& SyncTestState)
{
   puts("BasicTaskTest: Entry");
   SyncTestState = true;
   SyncEvent.SetEvent();

   co_await suspend_never{};

   puts("BasicTaskTest: Exit");
}


//* Simple unit test for async function Exception propagation
class MyException : public ktl::Exception
{
   K_RUNTIME_TYPED(MyException);

public:
   MyException() = delete;

   template<typename _NestedT>
   MyException(NTSTATUS Status, const char* Desc, const _NestedT& InnerEx) noexcept : ktl::Exception(Status, InnerEx), _desc(Desc) {}

   MyException(NTSTATUS Status, const char* Desc) noexcept : ktl::Exception(Status), _desc(Desc) {}

   MyException(MyException const& Right) noexcept 
       : ktl::Exception(Right)
   {
       _desc = Right._desc;
   }
   
   MyException(MyException&& Right) noexcept
       : ktl::Exception(Ktl::Move(Right))
   {
       _desc = Right._desc;
   }

   MyException& operator=(MyException const& Right) noexcept
   {
      this->ktl::Exception::operator =(Right);
      _desc = Right._desc;
      return *this;
   }

   MyException& operator=(MyException&& Right) noexcept
   {
       this->ktl::Exception::operator=(Right);
       _desc = Right._desc;
       return *this;
   }

   ~MyException() noexcept {}
   
   const char* GetDesc()
   {
      return _desc;
   }

private:
   const char*  _desc;
};


#pragma warning(disable:4702)
Awaitable<int> ExpTypedTest2(KAllocator& Alloc, const char* EStr)
{
    auto result = co_await KTimer::StartTimerAsync(Alloc, KTL_TAG_TEST, 1000, nullptr);
    KInvariant(NT_SUCCESS(result));
    
   printf("ExpTypedTest2: %s: throwing: ktl::Exception(STATUS_TIMEOUT)\n", EStr);
   throw MyException(STATUS_TIMEOUT, EStr);
   
   co_return(212121);
}
#pragma warning(default:4702)

Awaitable<int> ExpTypedTest1(KAllocator& Alloc, const char* EStr)
{
   co_await suspend_never{};
   try
   {
      int result = co_await ExpTypedTest2(Alloc, EStr);
      result;
   }
   catch (...)
   {
      printf("ExpTypedTest1: %s: caught exception - rethrowing\n", EStr);
      throw;
   }
   co_await suspend_never{};

   co_return 5050;
}

#pragma warning(disable:4702)           // VS C++ Update 2 bug
Awaitable<void> ExpTest2(KAllocator& Alloc, const char* EStr)
{
    auto result = co_await KTimer::StartTimerAsync(Alloc, KTL_TAG_TEST, 1000, nullptr);
    KInvariant(NT_SUCCESS(result));

    printf("ExpTest2: %s: throwing: ktl::Exception(STATUS_TIMEOUT)\n", EStr);
    throw ktl::Exception(STATUS_TIMEOUT);
    co_return;
}
#pragma warning(default:4702)           // VS C++ Update 2 bug

Awaitable<void> ExpTest1(KAllocator& Alloc, const char* EStr)
{
    co_await suspend_never{};
    try
    {
        co_await ExpTest2(Alloc, EStr);
    }
    catch (ktl::Exception Ex)
    {
        printf("ExpTest1: %s: caught exception - rethrowing as nested Exception\n", EStr);
        throw MyException(Ex.GetStatus(), "From: ExpTest1", Ktl::Move(Ex));
    }
    co_await suspend_never{};
    co_return;
}

#pragma warning(disable:4702)           // VS C++ Update 2 bug
Awaitable<void> NullAwaitable()
{
   co_return co_await suspend_never{};
}
#pragma warning(default:4702)           // VS C++ Update 2 bug

#pragma warning(disable:4702)           // VS C++ Update 2 bug
Awaitable<int> ValuedAwaitable(int Value)
{
   co_await suspend_never{};
   co_return Value;
}
#pragma warning(default:4702)           // VS C++ Update 2 bug

Task BasicSyncCompAwaitableTests(KAllocator&)
{
   auto a = NullAwaitable();
   auto b = ValuedAwaitable(29942);

   KInvariant((co_await b) == 29942);
   co_await a;
}

#pragma warning(disable:4702)           // VS C++ Update 2 bug
Awaitable<int> SyncAwait_SyncCompAwaitValueTest(int Result)
{
   co_await suspend_never{};
   co_return Result;
}
#pragma warning(default:4702)           // VS C++ Update 2 bug

Awaitable<void> SyncAwait_SyncCompAwaitVoidTest(KAllocator& Alloc)
{
    auto timer = KTimer::StartTimerAsync(Alloc, KTL_TAG_TEST, 1000, nullptr);
    auto status = co_await timer;
    bool        result = NT_SUCCESS(status);
    KInvariant(result);

    co_return;
}

//** Test helpers for AwaitableCompletionSource<> support
Task AcsMonitor(AwaitableCompletionSource<int>& ACS, char* Name, KEvent& CompEvent)
{
   AwaitableCompletionSource<int>::SPtr acs = &ACS;

   printf("AcsMonitor: '%s' Starting\n", Name);
   int value = 0;
   try
   {
      value = co_await acs->GetAwaitable();
      printf("AcsMonitor: '%s' Value: %i\n", Name, value);

      //* Prove co_await on completed AwaitableCompletionSource works
      auto a = acs->GetAwaitable();
      co_await a;
      printf("AcsMonitor: Secondary: '%s'; Set() detected\n", Name);
   }
   catch (ktl::Exception Exp)
   {
      printf("AcsMonitor: '%s' Exception: %08I32X\n", Name, Exp.GetStatus());
   }
   
   printf("AcsMonitor: '%s' Ending\n", Name);
   if (InterlockedDecrement((volatile LONG*) &AcsMonitorCount) == 0)
   {
      CompEvent.SetEvent();
   }
}

Task AcsVoidMonitor(AwaitableCompletionSource<void>& ACS, char* Name, KEvent& CompEvent)
{
   AwaitableCompletionSource<void>::SPtr acs = &ACS;

   printf("AcsVoidMonitor: '%s' Starting\n", Name);
   try
   {
      co_await acs->GetAwaitable();
      printf("AcsVoidMonitor: '%s'; Set() detected\n", Name);

      //* Prove co_await on completed AwaitableCompletionSource works
      auto a = acs->GetAwaitable();
      co_await a;
      printf("AcsVoidMonitor: Secondary: '%s'; Set() detected\n", Name);
   }
   catch (ktl::Exception Exp)
   {
      printf("AcsVoidMonitor: '%s' Exception: %08I32X\n", Name, Exp.GetStatus());
   }

   printf("AcsVoidMonitor: '%s' Ending\n", Name);
   if (InterlockedDecrement((volatile LONG*) &AcsMonitorCount) == 0)
   {
      CompEvent.SetEvent();
   }
}

void SetRacerThreadStart(void* SubjectStateBit)
{
    AwaitableState      stateBit(*((AwaitableState*)&SubjectStateBit));

    while (!_g_EndRaceTest)
    {
        //* 1st We register this thread as waiting
        InterlockedIncrement(&_g_ThreadsHeld);
        _g_EntryHoldEvent.WaitUntilSet();

        if (stateBit.IsReady())
        {
            _g_AwaitableStateUnderTest.ResultReady();
        }
        else if (stateBit.IsSuspended())
        {
            _g_AwaitableStateUnderTest.TryToSuspend();
        }
        else if (stateBit.IsFinalHeld())
        {
            _g_AwaitableStateUnderTest.TryToFinalize();
        }
        else
        {
            KInvariant(false);
        }

        InterlockedIncrement(&_g_ThreadsHeld);
        _g_DoneHoldEvent.WaitUntilSet();
    }
}

// Test to prove Awaitable<> common state-machine state manipulation is corrent under all possible 
// race conditions
void ProveAwaitableState(KAllocator& Allocator)
{
    {
        AwaitableState      uut;

        uut.ResultReady();
        KInvariant(uut.IsReady());

        KInvariant(uut.TryToFinalize());
        KInvariant(uut.IsReady());
        KInvariant(uut.IsFinalHeld());
        KInvariant(!uut.IsSuspended());
        KInvariant(!uut.TryToSuspend());            // Should fail = TryToFinalize() won
        KInvariant(uut.IsReady());
        KInvariant(uut.IsFinalHeld());
        KInvariant(!uut.IsSuspended());
    }
    {
        AwaitableState      uut;

        uut.ResultReady();
        KInvariant(uut.IsReady());

        KInvariant(uut.TryToSuspend());
        KInvariant(uut.IsReady());
        KInvariant(!uut.IsFinalHeld());
        KInvariant(uut.IsSuspended());
        KInvariant(!uut.TryToFinalize());            // Should fail = TryToSuspend() won
        KInvariant(uut.IsReady());
        KInvariant(!uut.IsFinalHeld());
        KInvariant(uut.IsSuspended());
    }

    {
        KThread::SPtr       setReadyThread;
        KThread::SPtr       setSuspendedThread;
        KThread::SPtr       setFinalHeldThread;

        NTSTATUS            status;

        _g_ThreadsHeld = 0;
        _g_EndRaceTest = false;

        status = KThread::Create(SetRacerThreadStart, (void*)(AwaitableState::Make_ResultReady().GetValue()), setReadyThread, Allocator, KTL_TAG_TEST);
        KInvariant(NT_SUCCESS(status));
        status = KThread::Create(SetRacerThreadStart, (void*)(AwaitableState::Make_Suspended().GetValue()), setSuspendedThread, Allocator, KTL_TAG_TEST);
        KInvariant(NT_SUCCESS(status));
        status = KThread::Create(SetRacerThreadStart, (void*)(AwaitableState::Make_FinalHeld().GetValue()), setFinalHeldThread, Allocator, KTL_TAG_TEST);
        KInvariant(NT_SUCCESS(status));

        while (true)
        {
            // Wait for all 3 threads to suspend on the entry or next cycle 
            while (_g_ThreadsHeld != 3) KNt::Sleep(0);

            // All held
            _g_ThreadsHeld = 0;
            _g_DoneHoldEvent.ResetEvent();
            _g_AwaitableStateUnderTest = AwaitableState();      // clear state var for this cycle

            // Let the threads go and wait till they have all done
            _g_EntryHoldEvent.SetEvent();
            while (_g_ThreadsHeld != 3) KNt::Sleep(0);

            // All held at done event
            _g_ThreadsHeld = 0;
            _g_EntryHoldEvent.ResetEvent();         // ready for next cycle

            // verify state
            KInvariant(
                _g_AwaitableStateUnderTest.IsReady() &&
                ((_g_AwaitableStateUnderTest.IsFinalHeld() && !_g_AwaitableStateUnderTest.IsSuspended()) ||
                 (!_g_AwaitableStateUnderTest.IsFinalHeld() && _g_AwaitableStateUnderTest.IsSuspended())));

            // Check for completion
            if ((_g_K$UnitTest$AwaitableStateRaceTestResultReadyCount >= 4) &&
                (_g_K$UnitTest$AwaitableStateRaceTestSuspendedCount >= 4) &&
                (_g_K$UnitTest$AwaitableStateRaceTestFinalHeldCount >= 4))
            {
                _g_EndRaceTest = true;
                _g_DoneHoldEvent.SetEvent();
                _g_EntryHoldEvent.SetEvent();
                break;
            }

            _g_DoneHoldEvent.SetEvent();            // let threads go for another cycle
        }
    }
}

//** ExceptionDebugInfoProvider used to provide custom provider support behavior for Exception
class TestDebugInfoProvider : public ExceptionDebugInfoProvider
{
public:
    static KSpinLock        _g_Lock;
    static volatile LONG    _g_AcquireCount;
    static volatile LONG    _g_ReleaseCount;
    static volatile LONG    _g_UnsafeSymInitializeCount;
    static volatile LONG    _g_UnsafeSymFromAddrCount;
    static volatile LONG    _g_UnsafeSymGetLineFromAddr64Count;

public:
    void AcquireLock() noexcept override
    {
        InterlockedIncrement(&_g_AcquireCount);
        _g_Lock.Acquire();
    }

    void ReleaseLock() noexcept override
    {
        InterlockedIncrement(&_g_ReleaseCount);
        _g_Lock.Release();
    }

    // Assumes AcquireLock() was called
    BOOL UnsafeSymInitialize(_In_ HANDLE hProcess, _In_opt_ PCSTR UserSearchPath, _In_ BOOL fInvadeProcess) noexcept override
    {
        KInvariant(_g_Lock.IsOwned());
        InterlockedIncrement(&_g_UnsafeSymInitializeCount);
        return ::SymInitialize(hProcess, UserSearchPath, fInvadeProcess);
    }

    BOOL UnsafeSymFromAddr(_In_ HANDLE hProcess, _In_ DWORD64 Address, _Out_opt_ PDWORD64 Displacement, _Inout_ PSYMBOL_INFO Symbol) noexcept override
    {
        KInvariant(_g_Lock.IsOwned());
        InterlockedIncrement(&_g_UnsafeSymFromAddrCount);
        return ::SymFromAddr(hProcess, Address, Displacement, Symbol);
    }

    // Assumes AcquireLock() was called
    BOOL UnsafeSymGetLineFromAddr64(_In_ HANDLE hProcess, _In_ DWORD64 qwAddr, _Out_ PDWORD pdwDisplacement, _Out_ PIMAGEHLP_LINE64 Line64) noexcept override
    {
        KInvariant(_g_Lock.IsOwned());
        InterlockedIncrement(&_g_UnsafeSymGetLineFromAddr64Count);
        return ::SymGetLineFromAddr64(hProcess, qwAddr, pdwDisplacement, Line64);
    }
};

KSpinLock           TestDebugInfoProvider::_g_Lock;
volatile LONG       TestDebugInfoProvider::_g_AcquireCount = 0;
volatile LONG       TestDebugInfoProvider::_g_ReleaseCount = 0;
volatile LONG       TestDebugInfoProvider::_g_UnsafeSymInitializeCount = 0;
volatile LONG       TestDebugInfoProvider::_g_UnsafeSymFromAddrCount = 0;
volatile LONG       TestDebugInfoProvider::_g_UnsafeSymGetLineFromAddr64Count = 0;

TestDebugInfoProvider   _g_TestDebugInfoProvider;

void InitAll()
{
    _g_KCo$TotalObjects = 0;
    GClass::testGClassDtored = false;
    AcsMonitorCount = 0;
    _g_AwaitableStateUnderTest.AwaitableState::AwaitableState();
    _g_EndRaceTest = false;
    _g_ThreadsHeld = 0;
    _g_EntryHoldEvent.KEvent::KEvent();
    _g_DoneHoldEvent.KEvent::KEvent();
}

#if defined(K_UseResumableDbgExt)
Task EnumTestTask(KAsyncEvent& WaitOn, KAllocator& Alloc, volatile ULONG& TaskCount)
{
    InterlockedIncrement(&TaskCount);

    KAsyncEvent::WaitContext::SPtr  waiter;
    KInvariant(NT_SUCCESS(WaitOn.CreateWaitContext(KTL_TAG_TEST, Alloc, waiter)));

    NTSTATUS status = co_await waiter->StartWaitUntilSetAsync(nullptr);
    KInvariant(NT_SUCCESS(status));

    InterlockedDecrement(&TaskCount);
}
#endif

/// Fun test
static ULONG Bounces = 4096;
static const ULONG NC = 32;
ULONG State[NC];
ULONG State2[NC];
ULONG PingCount[NC];
ULONG PongCount[NC];
ULONG PingDoneCount = 0;
ULONG PongDoneCount = 0;

class AsyncFunWaitEvent : public KAsyncContextBase, public KThreadPool::WorkItem
{
    K_FORCE_SHARED(AsyncFunWaitEvent);

public:
    static NTSTATUS AsyncFunWaitEvent::Create(
        __in KAllocator& Allocator,
        __in ULONG AllocatorTag,
        __out AsyncFunWaitEvent::SPtr& Context);

protected:
    VOID
        Execute()
    {
        BOOLEAN b = FALSE;
        while (!b)
        {
            b = _Event->WaitUntilSet(5 * 1000);
            if (!b)
            {
                printf("Timeout %d State %d State2 %d\n", _Index, State[_Index], State2[_Index]);
            }
        }
        Complete(STATUS_SUCCESS);
    }

#if !defined(PLATFORM_UNIX)
    static DWORD WINAPI ThreadProc(_In_ LPVOID lpParameter);
#else
    static void* ThreadProc(void* context);
#endif


    VOID
        OnStart(
        )
    {
//        GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);
#if !defined(PLATFORM_UNIX)
        KInvariant(QueueUserWorkItem(ThreadProc, this, WT_EXECUTEDEFAULT));
#else
        int error;
        pthread_t thread;
        pthread_attr_t attr;
        int stack_size = 512 * 1024;
        void *sp;

        error = pthread_attr_init(&attr);
        KInvariant(error == 0);
        error = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        KInvariant(error == 0);

        error = posix_memalign(&sp, 4096, stack_size);
        KInvariant(error == 0);
        error = pthread_attr_setstack(&attr, sp, stack_size);
        KInvariant(error == 0);
        
        error = pthread_create(&thread, &attr, AsyncFunWaitEvent::ThreadProc, this);
        KInvariant(error == 0);
#endif
    }

    VOID
        OnReuse(
        )
    {
    }

    VOID
        OnCancel(
        )
    {
    }

public:
    KEvent* _Event;
    ULONG   _Index;
};

#if !defined(PLATFORM_UNIX)
DWORD WINAPI AsyncFunWaitEvent::ThreadProc(_In_ LPVOID lpParameter)
{
    ((AsyncFunWaitEvent*)lpParameter)->Execute();
    return 0;
}
#else
void* AsyncFunWaitEvent::ThreadProc(void* context)
{
    ((AsyncFunWaitEvent*)context)->Execute();
    return(nullptr);
}
#endif

AsyncFunWaitEvent::AsyncFunWaitEvent()
{
}

AsyncFunWaitEvent::~AsyncFunWaitEvent()
{
}

NTSTATUS AsyncFunWaitEvent::Create(
    __in KAllocator& Allocator,
    __in ULONG AllocatorTag,
    __out AsyncFunWaitEvent::SPtr& Context)
{
    AsyncFunWaitEvent::SPtr context;

    context = _new(AllocatorTag, Allocator) AsyncFunWaitEvent();
    if (!context)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES, nullptr, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (!NT_SUCCESS(context->Status()))
    {
        return(context->Status());
    }

    Context = Ktl::Move(context);
    return(STATUS_SUCCESS);

}

ktl::Awaitable<NTSTATUS> WaitForFunAsync(
    __in KAllocator& Alloc,
    __in KEvent* FunEvent,
    __in ULONG Index,
    __in_opt KAsyncContextBase* const ParentAsyncContext
)
{
    NTSTATUS status;
    //NTSTATUS status2;
    AsyncFunWaitEvent::SPtr context;

    status = AsyncFunWaitEvent::Create(
        Alloc,
        KTL_TAG_TEST,
        context);
    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    context->_Event = FunEvent;
    context->_Index = Index;

    StartAwaiter::SPtr awaiter;

    status = StartAwaiter::Create(
        Alloc,
        KTL_TAG_TEST,
        *context,
        awaiter,
        ParentAsyncContext,
        nullptr);               // GlobalContext

    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    State2[Index] = 1;
    status = co_await *awaiter;
    State2[Index] = 2;

    co_return status;
}


Task Ping(KAllocator& Alloc, ULONG index, KEvent* A, KEvent* B)
{
    NTSTATUS status;

    PingCount[index] = 0;
    for (ULONG i = 0; i < Bounces; i++)
    {
        A->SetEvent();
        State[index] = 11;
        status = co_await WaitForFunAsync(Alloc, B, index, nullptr);
        State[index] = 12;
        KInvariant(NT_SUCCESS(status));
        PingCount[index]++;
    }
    printf("Ping %d done\n", index);
    InterlockedIncrement((volatile LONG*) &PingDoneCount);
}

Task Pong(KAllocator& Alloc, ULONG index, KEvent* A, KEvent* B)
{
    NTSTATUS status;

    PongCount[index] = 0;
    for (ULONG i = 0; i < Bounces; i++)
    {
        State[index] = 21;
        status = co_await WaitForFunAsync(Alloc, A, index, nullptr);
        State[index] = 22;
        KInvariant(NT_SUCCESS(status));
        B->SetEvent();
        PongCount[index]++;
    }
    printf("Pong %d done\n", index);
    InterlockedIncrement((volatile LONG*) &PongDoneCount);
}

NTSTATUS
KAWIpcFunTest(KAllocator& Alloc)
{
    //* reinit
    Bounces = 4096;
    PingDoneCount = 0;
    PongDoneCount = 0;

    for (int ix = 0; ix < NC; ix++)
    {
        State[ix] = 0;
        State2[ix] = 0;
        PingCount[ix] = 0;
        PongCount[ix] = 0;
    }

    KEvent A[NC];
    KEvent B[NC];

    for (ULONG i = 0; i < NC / 2; i++)
    {
        Ping(Alloc, i, &A[i], &B[i]);
    }

    for (ULONG i = 0; i < NC / 2; i++)
    {
        Pong(Alloc, i, &A[i], &B[i]);
    }

    for (ULONG i = NC / 2; i < NC; i++)
    {
        Ping(Alloc, i, &A[i], &B[i]);
    }

    for (ULONG i = NC / 2; i < NC; i++)
    {
        Pong(Alloc, i, &A[i], &B[i]);
    }

    MemoryBarrier();
    while ((PingDoneCount < NC) && (PongDoneCount < NC))
    {
        KNt::Sleep(1000);
        MemoryBarrier();
        printf("PingDone %d PongDone %d\n", PingDoneCount, PongDoneCount);
    }
    return(STATUS_SUCCESS);
}

//* Prove ktl::CorHelper::ContinueOnNonKtlThread()
ktl::Awaitable<bool> ContinueOnNonKtlThreadTest(KAllocator& Alloc)
{
    // Force onto a ktl thread
    co_await KTimer::StartTimerAsync(Alloc, KTL_TAG_TEST, 100, nullptr);
    KInvariant(Alloc.GetKtlSystem().IsCurrentThreadOwned());
    bool result = co_await CorHelper::ContinueOnNonKtlThread(Alloc.GetKtlSystem());
    KInvariant(!Alloc.GetKtlSystem().IsCurrentThreadOwned());

    co_return result;
}


//* Prove ktl::KCoLock
ktl::Awaitable<void> KCoLockTest(KAllocator& Alloc)
{
    // Force onto a ktl thread
    co_await KTimer::StartTimerAsync(Alloc, KTL_TAG_TEST, 100, nullptr);
    KInvariant(Alloc.GetKtlSystem().IsCurrentThreadOwned());

    KCoLock      lock(Alloc.GetKtlSystem());

    co_await lock.AcquireAsync();

    auto waiter1 = lock.AcquireAsync();
    auto waiter2 = lock.AcquireAsync();

    KInvariant(lock.Release());
    co_await waiter1;
    KInvariant(lock.Release());
    co_await waiter2;
    KInvariant(lock.Release());

    co_return;
}

//** Acid test for KAsyncEvent::WaitContext::StartWaitUntilSetAsync()/KAsyncEvent::SetEvent() 
Awaitable<void> AcidTestOfKEventAsync(KAllocator& Allocator)
{
    puts("AcidTestOfKEventAsync: Entry");

    // DEBUG: Trap promise corruption
    auto& myPromise = *co_await ktl::CorHelper::GetCurrentPromise<ktl::Awaitable<void>::promise_type>();
    auto myState = myPromise.GetState();
    KInvariant(myState.IsUnusedBitsValid());
    KInvariant(!myState.IsReady());
    KInvariant(!myState.IsSuspended());
    KInvariant(!myState.IsFinalHeld());
    KInvariant(!myState.IsCalloutOnFinalHeld());

    KTimer::SPtr    timer;
    KInvariant(NT_SUCCESS(KTimer::Create(timer, Allocator, KTL_TAG_TEST)));

    const int todo = 200;
    const int maxWaiters = 500;
    KAsyncEvent     event(FALSE, FALSE);            // Autoreset

    KAsyncEvent::WaitContext::SPtr  waiters[maxWaiters];
    for (int ix = 0; ix < maxWaiters; ix++)
    {
        KInvariant(NT_SUCCESS(event.CreateWaitContext(KTL_TAG_TEST, Allocator, waiters[ix])));
    }

    KAsyncEvent                     ackEvent(FALSE, FALSE);            // Autoreset
    KAsyncEvent::WaitContext::SPtr  ackWaiter;
    KInvariant(NT_SUCCESS(ackEvent.CreateWaitContext(KTL_TAG_TEST, Allocator, ackWaiter)));

    auto WaitAndSignal = [&](int WaitersIx) -> Awaitable<NTSTATUS>
    {
        NTSTATUS status = co_await waiters[WaitersIx]->StartWaitUntilSetAsync(nullptr);
        KInvariant(NT_SUCCESS(status));
        waiters[WaitersIx]->Reuse();
        ackEvent.SetEvent();
        co_return{ status };
    };

    for (int ix = 0; ix < todo; ix++)
    {
        int numberOfWaitersThisCycle = rand() % maxWaiters;
        {
            Awaitable<NTSTATUS>      awaiters[maxWaiters];
            for (int ix1 = 0; ix1 < numberOfWaitersThisCycle; ix1++)
            {
                awaiters[ix1] = WaitAndSignal(ix1);
            }

            int leftToSignal = numberOfWaitersThisCycle;
            while (leftToSignal > 0)
            {
                int thisTime = __min(leftToSignal, leftToSignal % ((rand() % 90) + 1));
                for (int ix2 = 0; ix2 < thisTime; ix2++)
                {
                    NTSTATUS status = co_await timer->StartTimerAsync(rand() % 2, nullptr);
                    KInvariant(NT_SUCCESS(status));
                    timer->Reuse();
                    event.SetEvent();

                    status = co_await ackWaiter->StartWaitUntilSetAsync(nullptr);
                    KInvariant(NT_SUCCESS(status));
                    ackWaiter->Reuse();
                }

                leftToSignal -= thisTime;
            }

            for (int ix1 = 0; ix1 < numberOfWaitersThisCycle; ix1++)
            {
                NTSTATUS status = co_await awaiters[ix1];
                KInvariant(NT_SUCCESS(status));
                waiters[ix1]->Reuse();
            }
        }

        myState = myPromise.GetState();
        KInvariant(myState.IsUnusedBitsValid());
        KInvariant(!myState.IsReady());
        KInvariant(!myState.IsFinalHeld());
        KInvariant(!myState.IsCalloutOnFinalHeld());
    }

    puts("AcidTestOfKEventAsync: Exit");

    myState = myPromise.GetState();
    KInvariant(myState.IsUnusedBitsValid());
    KInvariant(!myState.IsReady());
    KInvariant(!myState.IsFinalHeld());
    KInvariant(!myState.IsCalloutOnFinalHeld());

    co_return;
}

//** Core tests
void
BasicAwaitableTests(KtlSystem& UnderlyingSystem)
{
   puts("BasicAwaitableTests: Starting");

   InitAll();

   LONGLONG     StartingAllocs = KAllocatorSupport::gs_AllocsRemaining;
   KAllocator&  allocator = UnderlyingSystem.NonPagedAllocator();

   //* Cause Exception::InitSymbols()'s call to SymInitialize() to fail - allows secondary callers
   SymInitialize(GetCurrentProcess(), NULL, TRUE);

   //** Prove SyncAwait<T> implemented within an on-demand created KShared<> derivation
   //   instance 
   {
       auto r = SyncAwait(BasicAwaitableTypeTest(allocator, 121212));
       printf("BasicAwaitTests: BasicAwaitableTypeTest returned: %u\n", r);
       KInvariant(r == 121212);
   }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});

   //** Acid Test KEvent in light of extreme load
   {
       // SyncAwait(AcidTestOfKEventAsync(allocator));
   }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});

   //** Prove basic SyncAwait with async and sync Awaitable<> 
   {
       puts("Starting: BasicSyncAwaitTest");

       auto result1 = SyncAwait(SyncAwait_SyncCompAwaitValueTest(98765));
       KInvariant(result1 == 98765);
       // Allow all background tasks to complete
       SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});

       auto x = SyncAwait_SyncCompAwaitVoidTest(allocator);
       KInvariant(x);             // Proves Awaiter<void>::operator(bool)
       SyncAwait(x);
       SyncAwait(x);

       puts("Completion of: BasicSyncAwaitTest");
   }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});

   //* Prove EitherReady() behavior
   {
       KTimer::SPtr     t1;
       KTimer::SPtr     t2;
       NTSTATUS         status;
       
       status = KTimer::Create(t1, allocator, KTL_TAG_TEST);
       KInvariant(NT_SUCCESS(status));
       status = KTimer::Create(t2, allocator, KTL_TAG_TEST);
       KInvariant(NT_SUCCESS(status));

       // Awaitable<value> form
       {
           // prove both active when EitherReady() called
           {
               ktl::Awaitable<NTSTATUS>     aw1 = t1->StartTimerAsync(2000, nullptr);
               ktl::Awaitable<NTSTATUS>     aw2 = t2->StartTimerAsync(1000, nullptr);

               SyncAwait(EitherReady(allocator.GetKtlSystem(), aw1, aw2));
               KInvariant(!aw1.IsReady() && aw2.IsReady());                     // aw2 should complete first

               t1->Cancel();                                                    // cause aw1 to complete quickly

               KInvariant(SyncAwait(aw2) == STATUS_SUCCESS);
               KInvariant(SyncAwait(aw1) == STATUS_CANCELLED);

               t1->Reuse();
               t2->Reuse();
           }

           // prove both complete when EitherReady() called (fast path)
           {
               ktl::Awaitable<NTSTATUS>     aw1 = t1->StartTimerAsync(0, nullptr);
               ktl::Awaitable<NTSTATUS>     aw2 = t2->StartTimerAsync(0, nullptr);

               KNt::Sleep(1000);

               SyncAwait(EitherReady(allocator.GetKtlSystem(), aw1, aw2));
               KInvariant(aw1.IsReady() && aw2.IsReady());

               KInvariant(SyncAwait(aw2) == STATUS_SUCCESS);
               KInvariant(SyncAwait(aw1) == STATUS_SUCCESS);

               t1->Reuse();
               t2->Reuse();
           }

           // prove both complete when EitherReady() called (non fast path)
           _g_KtlEitherReady_TestFastPath = false;
           {
               ktl::Awaitable<NTSTATUS>     aw1 = t1->StartTimerAsync(0, nullptr);
               ktl::Awaitable<NTSTATUS>     aw2 = t2->StartTimerAsync(0, nullptr);

               KNt::Sleep(1000);

               SyncAwait(EitherReady(allocator.GetKtlSystem(), aw1, aw2));
               KInvariant(aw1.IsReady() && aw2.IsReady());

               KInvariant(SyncAwait(aw2) == STATUS_SUCCESS);
               KInvariant(SyncAwait(aw1) == STATUS_SUCCESS);

               t1->Reuse();
               t2->Reuse();
           }

           // prove both complete when EitherReady() called - one at a time - aw1
           {
               ktl::Awaitable<NTSTATUS>     aw1 = t1->StartTimerAsync(0, nullptr);
               KNt::Sleep(1000);
               ktl::Awaitable<NTSTATUS>     aw2 = t2->StartTimerAsync(1000, nullptr);
               KInvariant(aw1.IsReady() && !aw2.IsReady());


               SyncAwait(EitherReady(allocator.GetKtlSystem(), aw1, aw2));
               KInvariant(aw1.IsReady() && !aw2.IsReady());
               t2->Cancel();

               KInvariant(SyncAwait(aw2) == STATUS_CANCELLED);
               KInvariant(SyncAwait(aw1) == STATUS_SUCCESS);

               t1->Reuse();
               t2->Reuse();
           }

           // prove both complete when EitherReady() called - one at a time - aw2
           {
               ktl::Awaitable<NTSTATUS>     aw2 = t2->StartTimerAsync(0, nullptr);
               KNt::Sleep(1000);
               ktl::Awaitable<NTSTATUS>     aw1 = t1->StartTimerAsync(1000, nullptr);
               KInvariant(!aw1.IsReady() && aw2.IsReady());


               SyncAwait(EitherReady(allocator.GetKtlSystem(), aw1, aw2));
               KInvariant(!aw1.IsReady() && aw2.IsReady());
               t1->Cancel();

               KInvariant(SyncAwait(aw2) == STATUS_SUCCESS);
               KInvariant(SyncAwait(aw1) == STATUS_CANCELLED);

               t1->Reuse();
               t2->Reuse();
           }

           // prove both complete when EitherReady() called - one at a time - aw1 -- forced test path #1
           {
               ktl::Awaitable<NTSTATUS>     aw1 = t1->StartTimerAsync(0, nullptr);
               KNt::Sleep(1000);
               ktl::Awaitable<NTSTATUS>     aw2 = t2->StartTimerAsync(1000, nullptr);
               KInvariant(aw1.IsReady() && !aw2.IsReady());

               _g_KtlEitherReady_TestHoldAwDelay = 1020;       // Cause test path #1 execute

               SyncAwait(EitherReady(allocator.GetKtlSystem(), aw1, aw2));
               KInvariant(aw1.IsReady() && aw2.IsReady());

               KInvariant(SyncAwait(aw2) == STATUS_SUCCESS);
               KInvariant(SyncAwait(aw1) == STATUS_SUCCESS);

               t1->Reuse();
               t2->Reuse();

               _g_KtlEitherReady_TestHoldAwDelay = 0;
           }

           // prove both complete when EitherReady() called - one at a time - aw2 -- forced test path #2
           {
               ktl::Awaitable<NTSTATUS>     aw2 = t2->StartTimerAsync(0, nullptr);
               KNt::Sleep(1000);
               ktl::Awaitable<NTSTATUS>     aw1 = t1->StartTimerAsync(1000, nullptr);
               KInvariant(!aw1.IsReady() && aw2.IsReady());

               _g_KtlEitherReady_TestHoldAwDelay = 1020;       // Cause test path #2 execute

               SyncAwait(EitherReady(allocator.GetKtlSystem(), aw1, aw2));
               KInvariant(aw1.IsReady() && aw2.IsReady());

               KInvariant(SyncAwait(aw2) == STATUS_SUCCESS);
               KInvariant(SyncAwait(aw1) == STATUS_SUCCESS);

               t1->Reuse();
               t2->Reuse();

               _g_KtlEitherReady_TestHoldAwDelay = 0;
           }

           _g_KtlEitherReady_TestFastPath = true;
       }

       // Awaitable<void> form
       {
           static auto StartTimerAsync = [](KTimer& Timer, ULONG Msecs, NTSTATUS Status) -> ktl::Awaitable<void>
           {
               NTSTATUS  expectedStatus = Status;
               NTSTATUS status = co_await Timer.StartTimerAsync(Msecs, nullptr);
               KInvariant(status == expectedStatus);
               co_return;
           };

           // prove both active when EitherReady() called
           {
               ktl::Awaitable<void>     aw1 = StartTimerAsync(*t1, 2000, STATUS_CANCELLED);
               ktl::Awaitable<void>     aw2 = StartTimerAsync(*t2, 1000, STATUS_SUCCESS);

               SyncAwait(EitherReady(allocator.GetKtlSystem(), aw1, aw2));
               KInvariant(!aw1.IsReady() && aw2.IsReady());                     // aw2 should complete first

               t1->Cancel();                                                    // cause aw1 to complete quickly

               SyncAwait(aw2);
               SyncAwait(aw1);

               t1->Reuse();
               t2->Reuse();
           }

           // prove both complete when EitherReady() called (fast path)
           {
               ktl::Awaitable<void>     aw1 = StartTimerAsync(*t1, 0, STATUS_SUCCESS);
               ktl::Awaitable<void>     aw2 = StartTimerAsync(*t2, 0, STATUS_SUCCESS);

               KNt::Sleep(1000);

               SyncAwait(EitherReady(allocator.GetKtlSystem(), aw1, aw2));
               KInvariant(aw1.IsReady() && aw2.IsReady());

               SyncAwait(aw2);
               SyncAwait(aw1);

               t1->Reuse();
               t2->Reuse();
           }

           // prove both complete when EitherReady() called (non fast path)
           _g_KtlEitherReady_TestFastPath = false;
           {
               ktl::Awaitable<void>     aw1 = StartTimerAsync(*t1, 0, STATUS_SUCCESS);
               ktl::Awaitable<void>     aw2 = StartTimerAsync(*t2, 0, STATUS_SUCCESS);

               KNt::Sleep(1000);

               SyncAwait(EitherReady(allocator.GetKtlSystem(), aw1, aw2));
               KInvariant(aw1.IsReady() && aw2.IsReady());

               SyncAwait(aw2);
               SyncAwait(aw1);

               t1->Reuse();
               t2->Reuse();
           }

           // prove both complete when EitherReady() called - one at a time - aw1
           {
               ktl::Awaitable<void>     aw1 = StartTimerAsync(*t1, 0, STATUS_SUCCESS);
               KNt::Sleep(1000);
               ktl::Awaitable<void>     aw2 = StartTimerAsync(*t2, 1000, STATUS_CANCELLED);
               KInvariant(aw1.IsReady() && !aw2.IsReady());


               SyncAwait(EitherReady(allocator.GetKtlSystem(), aw1, aw2));
               KInvariant(aw1.IsReady() && !aw2.IsReady());
               t2->Cancel();

               SyncAwait(aw2);
               SyncAwait(aw1);

               t1->Reuse();
               t2->Reuse();
           }

           // prove both complete when EitherReady() called - one at a time - aw2
           {
               ktl::Awaitable<void>     aw2 = StartTimerAsync(*t2, 0, STATUS_SUCCESS);
               KNt::Sleep(1000);
               ktl::Awaitable<void>     aw1 = StartTimerAsync(*t1, 1000, STATUS_CANCELLED);
               KInvariant(!aw1.IsReady() && aw2.IsReady());


               SyncAwait(EitherReady(allocator.GetKtlSystem(), aw1, aw2));
               KInvariant(!aw1.IsReady() && aw2.IsReady());
               t1->Cancel();

               SyncAwait(aw2);
               SyncAwait(aw1);

               t1->Reuse();
               t2->Reuse();
           }

           // prove both complete when EitherReady() called - one at a time - aw1 -- forced test path #1
           {
               ktl::Awaitable<void>     aw1 = StartTimerAsync(*t1, 0, STATUS_SUCCESS);
               KNt::Sleep(1000);
               ktl::Awaitable<void>     aw2 = StartTimerAsync(*t2, 1000, STATUS_SUCCESS);
               KInvariant(aw1.IsReady() && !aw2.IsReady());

               _g_KtlEitherReady_TestHoldAwDelay = 1020;       // Cause test path #1 execute

               SyncAwait(EitherReady(allocator.GetKtlSystem(), aw1, aw2));
               KInvariant(aw1.IsReady() && aw2.IsReady());

               SyncAwait(aw2);
               SyncAwait(aw1);

               t1->Reuse();
               t2->Reuse();

               _g_KtlEitherReady_TestHoldAwDelay = 0;
           }

           // prove both complete when EitherReady() called - one at a time - aw2 -- forced test path #2
           {
               ktl::Awaitable<void>     aw2 = StartTimerAsync(*t2, 0, STATUS_SUCCESS);
               KNt::Sleep(1000);
               ktl::Awaitable<void>     aw1 = StartTimerAsync(*t1, 1000, STATUS_SUCCESS);
               KInvariant(!aw1.IsReady() && aw2.IsReady());

               _g_KtlEitherReady_TestHoldAwDelay = 1020;       // Cause test path #2 execute

               SyncAwait(EitherReady(allocator.GetKtlSystem(), aw1, aw2));
               KInvariant(aw1.IsReady() && aw2.IsReady());

               SyncAwait(aw2);
               SyncAwait(aw1);

               t1->Reuse();
               t2->Reuse();

               _g_KtlEitherReady_TestHoldAwDelay = 0;
           }

           _g_KtlEitherReady_TestFastPath = true;
       }
   }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});

   //* Prove KCoLock behavior - not on ktl thread; no recursion
   {
       KCoLock      lock(UnderlyingSystem);

       SyncAwait(lock.AcquireAsync());

       auto waiter1 = lock.AcquireAsync();
       auto waiter2 = lock.AcquireAsync();

       KInvariant(lock.Release());
       SyncAwait(waiter1);
       KInvariant(lock.Release());
       SyncAwait(waiter2);
       KInvariant(lock.Release());
   }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});

   //* Prove KCoLock behavior - not on ktl thread; recursion
   {
       KCoLock      lock(UnderlyingSystem);

       SyncAwait(lock.AcquireAsync());

       auto waiter1 = lock.AcquireAsync();
       auto waiter2 = lock.AcquireAsync();

       KInvariant(lock.Release(true));
       SyncAwait(waiter1);
       KInvariant(lock.Release(true));
       SyncAwait(waiter2);
       KInvariant(lock.Release(true));
   }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});

   //* Prove KCoLock behavior - on ktl thread
   {
       SyncAwait(KCoLockTest(allocator));
   }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});

   //* Prove ContinueOnNonKtlThread behavior
   {
       auto x  = ContinueOnNonKtlThreadTest(allocator);
       KInvariant(SyncAwait(x));
   }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});

   //* Alan's test - found Sync comp race bug on non-ktl SytartAwaiter
   {
       KAWIpcFunTest(allocator);
       KNt::Sleep(1000);
   }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});

   //* Prove basic async natural of the Awaitable state-machine is correct
   ProveAwaitableState(allocator);

   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});

   //** Prove Awaitable<>::operator(bool) detected non-coroutine bound condition; and null ToTask()
   {
       Awaitable<int>   a1;
       KInvariant(!a1);
       ToTask(a1);


       Awaitable<void>   a2;
       KInvariant(!a2);
       ToTask(a2);
   }

#if defined(K_UseResumableDbgExt)
   //** Prove basic DBG Ext support
   {
        KAsyncEvent      event(TRUE);            // Manual reset
        volatile ULONG   taskActiveCount = 0;
        const ULONG      nbrOfTasksToStart = 10;

        for (int i = 0; i < nbrOfTasksToStart; i++)
        {
            EnumTestTask(event, allocator, taskActiveCount);
        }

        // Wait for all Tasks to start
        while (taskActiveCount < nbrOfTasksToStart) KNt::Sleep(0);

        // All tasks started - prove it
        #if defined(PLATFORM_UNIX)
            // _Resumable_frame_prefix is not defined
        #else
        auto  callback([] (Task::promise_type& Promise) -> void
        {
            coroutine_handle<Task::promise_type> coHandle;
            coHandle = coroutine_handle<Task::promise_type>::from_promise(Promise);

            coroutine_handle<Task::promise_type>::_Resumable_frame_prefix* framePreamble = 
            (coroutine_handle<Task::promise_type>::_Resumable_frame_prefix*)coHandle.address();

            const int       maxSymbolSize = 256;
            union
            {
                SYMBOL_INFO symbol;
                char        pad[sizeof(SYMBOL_INFO) + (maxSymbolSize * sizeof(char))];
            }               entry;

            entry.symbol.MaxNameLen = maxSymbolSize - 1;
            entry.symbol.SizeOfStruct = sizeof(SYMBOL_INFO);
            KInvariant(::SymFromAddr(GetCurrentProcess(), (DWORD64)framePreamble->_Fn, 0, &entry.symbol));
            KStringViewA        funcName((LPCSTR)(&entry.symbol.Name[0]));

            DWORD               lineDisplacement = 0;
            IMAGEHLP_LINE64     imageHelp; imageHelp.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
            ::SymGetLineFromAddr64(GetCurrentProcess(), (DWORD64)framePreamble->_Fn, &lineDisplacement, &imageHelp);

            printf("ActiveTask: Promise: 0x%llX; FramePreamble: 0x%llX; ResumePoint: %s (Line: %u)\n",
                (ULONGLONG)&Promise, 
                (ULONGLONG)framePreamble,
                (char*)funcName,
                imageHelp.LineNumber);
        });

        Task::EnumActiveTasks(callback);
        #endif

        auto  callback1([](PromiseCommon& Promise) -> void
        {
            coroutine_handle<Awaitable<void>::promise_type> coHandle;
            coHandle = coroutine_handle<Awaitable<void>::promise_type>::from_promise((Awaitable<void>::promise_type&)Promise);

            void* framePreamble = coHandle.address();

            const int       maxSymbolSize = 256;
            union
            {
                SYMBOL_INFO symbol;
                char        pad[sizeof(SYMBOL_INFO) + (maxSymbolSize * sizeof(char))];
            }               entry;

            entry.symbol.MaxNameLen = maxSymbolSize - 1;
            entry.symbol.SizeOfStruct = sizeof(SYMBOL_INFO);
            KInvariant(::SymFromAddr(GetCurrentProcess(), (DWORD64)Promise._ReturnAddress, 0, &entry.symbol));
            KStringViewA        funcName((LPCSTR)(&entry.symbol.Name[0]));

            DWORD               lineDisplacement = 0;
            IMAGEHLP_LINE64     imageHelp; imageHelp.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
            ::SymGetLineFromAddr64(GetCurrentProcess(), (DWORD64)Promise._ReturnAddress, &lineDisplacement, &imageHelp);

            printf("SuspendedAwaitable: Promise: 0x%llX; FramePreamble@: 0x%llX; Ret@: %s (Line: %u)\n",
                (ULONGLONG)&Promise,
                (ULONGLONG)framePreamble,
                (char*)funcName,
                imageHelp.LineNumber);
        });

        PromiseCommon::EnumActiveAwaitables(callback1);

        event.SetEvent();
       
        // Wait for all Tasks to complete
        while (taskActiveCount != 0) KNt::Sleep(0);
    }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});
#endif

   //** Prove ToTask()
   {
       auto awaiter = SyncAwait_SyncCompAwaitVoidTest(allocator);
       KInvariant(awaiter);             // Proves Awaiter<t>::operator(bool)
       KInvariant(_g_KCo$TotalObjects != 0);
       ToTask(awaiter);
       KInvariant(_g_KCo$TotalObjects != 0);
   }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});

   //** Very basic Exception and nested Exeception proofs
   {
       // Make sure a derived Exception can still produce an exception_ptr that
       // preserved the derived type - tests CRT basic compat
       try
       {
           MyException  xExp(STATUS_SUCCESS, "xExp");
           std::exception_ptr x1 = std::make_exception_ptr<MyException>(xExp);
        #if !defined(PLATFORM_UNIX)
           x1._RethrowException();
        #else
           rethrow_exception(x1);
        #endif
       }
       catch (MyException)
       {
       }
       catch (Exception ex)
       {
           KInvariant(false);
       }
       catch (...)
       {
           KInvariant(false);
       }

       // Prove basic nested exceptions work
       MyException  tExp(STATUS_SUCCESS, "tExp");
       Exception  tExp1(STATUS_SUCCESS, tExp);
       MyException  tExp2(STATUS_SUCCESS, "tExp2", tExp1);

       try
       {
           throw tExp2;
       }
       catch (MyException Ex)
       {
           KDynStringA   str(allocator);
           if (Ex.ToString(str))
           {
               puts(str);
           }
       }
       catch (...)
       {
           KInvariant(false);
       }

       // Prove GetInnerException()
       KUniquePtr<Exception, ktl::Exception::HeapDeleter<Exception>> x1(tExp2.GetInnerException<Exception>());
       KInvariant(x1);

       // Prove resulting nested exception is correct.
       try
       {
           throw *x1;
       }
       catch (MyException)
       {
           KInvariant(false);
       }
       catch (Exception) {}     // GOOD
       catch (...)
       {
           KInvariant(false);
       }

       KUniquePtr<MyException, ktl::Exception::HeapDeleter<MyException>> x(x1->GetInnerException<MyException>());
       KInvariant(x);

       // Prove resulting nested exception is correct.
       try
       {
           throw *x;
       }
       catch (MyException) {}
       catch (Exception)
       {
           KInvariant(false);
       }
       catch (...)
       {
           KInvariant(false);
       }
   }

   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});
   
   //** Prove CancellationToken/Source behavior
   {
      CancellationTokenSource::SPtr testSPtr;
      NTSTATUS      status = CancellationTokenSource::Create(allocator, KTL_TAG_TEST, testSPtr);
      KInvariant(NT_SUCCESS(status));
      KInvariant(testSPtr->_RefCount == 1);

      // Prove ref counting, copy, move, and reset semantics
      CancellationToken t1 = testSPtr->Token;
      KInvariant(!t1.IsCancellationRequested);
      KInvariant(testSPtr->_RefCount == 2);

      CancellationToken t2 = t1;
      KInvariant(testSPtr->_RefCount == 3);

      CancellationToken t3 = Ktl::Move(t2);
      KInvariant(testSPtr->_RefCount == 3);
      KInvariant(!t1.IsCancellationRequested);
      KInvariant(!t2.IsCancellationRequested);
      KInvariant(!t3.IsCancellationRequested);

      t1.ThrowIfCancellationRequested();
      testSPtr->Cancel(STATUS_CANCELLED);
      KInvariant(testSPtr->_RefCount == 3);
      KInvariant(t1.IsCancellationRequested);
      KInvariant(!t2.IsCancellationRequested);
      KInvariant(t3.IsCancellationRequested);

      CancellationToken t4 = testSPtr->Token;
      KInvariant(testSPtr->_RefCount == 4);
      KInvariant(t1.IsCancellationRequested);
      KInvariant(!t2.IsCancellationRequested);
      KInvariant(t3.IsCancellationRequested);
      KInvariant(t4.IsCancellationRequested);

      t4.Reset();
      KInvariant(testSPtr->_RefCount == 3);
      KInvariant(t1.IsCancellationRequested);
      KInvariant(!t2.IsCancellationRequested);
      KInvariant(t3.IsCancellationRequested);
      KInvariant(!t4.IsCancellationRequested);

      // Prove proper Exception status propagates
      try
      {
         t1.ThrowIfCancellationRequested();
         KInvariant(false);
      }
      catch (ktl::Exception& Ex)
      {
          KDynStringA   str(allocator);
          if (Ex.ToString(str))
          {
              puts(str);
          }
          KInvariant(Ex.GetStatus() == STATUS_CANCELLED);
      }

      // Prove ref counts are correct
      t1.Reset();
      t2.Reset();
      t3.Reset();
      t4.Reset();
      KInvariant(testSPtr->_RefCount == 1);

      CancellationToken t5 = CancellationToken::None;
      KInvariant(!t5.IsCancellationRequested);
      t5.ThrowIfCancellationRequested();
   }

   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});

   //** Prove basic Task behavior
   {
      puts("Starting: BasicTaskTest");
      KEvent        syncEvent{ false };
      BOOLEAN       syncState = false;              // set true when BasicTaskTest has made entry

      Task t = BasicTaskTest(syncEvent, syncState);
      KInvariant(t.IsTaskStarted());
      syncEvent.WaitUntilSet();
      KInvariant(syncState);
      puts("Completion of: BasicTaskTest");
   }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});

#if !defined(PLATFORM_UNIX)
   //** Prove basic Task behavior with allocation failure
   {
       puts("Starting: BasicTaskTest - alloc failure");
       KEvent       syncEvent{ false };
       BOOLEAN      syncState = false;              // set true when BasicTaskTest has made entry

       ::_g_ForceTaskFrameAllocationFailure = true;
       Task t = BasicTaskTest(syncEvent, syncState);
       ::_g_ForceTaskFrameAllocationFailure = false;
       KInvariant(!t.IsTaskStarted());
       Task t2 = t;
       KInvariant(!t2.IsTaskStarted());
       puts("Completion of: BasicTaskTest - alloc failure");
   }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});
#endif
   
   //** Prove sync completions of Awaitable<>
   {
      puts("Starting: BasicAwaitableTest");
      BasicSyncCompAwaitableTests(allocator);
      puts("Completion of: BasicAwaitableTest");
   }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});

   //** Prove basic SyncAwait with async and sync Awaitable<> 
   {
      puts("Starting: BasicSyncAwaitTest");

      auto result1 = SyncAwait(SyncAwait_SyncCompAwaitValueTest(98765));
      KInvariant(result1 == 98765);
      // Allow all background tasks to complete
      SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});

      auto x = SyncAwait_SyncCompAwaitVoidTest(allocator);
      KInvariant(x);             // Proves Awaiter<void>::operator(bool)
      SyncAwait(x);
      SyncAwait(x);

      puts("Completion of: BasicSyncAwaitTest");
   }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});

   //** Simple Exception propagation (void) test
   {
      ktl::Awaitable<void> awaitable = ExpTest1(allocator, "Test void Exception Timeout Str");
      ktl::Awaitable<void> a2 = Ktl::Move(awaitable);       // Prove copy ctor
      ktl::Awaitable<void> a1;                              // Prove default ctor
      a1 = Ktl::Move(a2);                                  // Prove copy 

      try
      {
         SyncAwait(a1);
      }
      catch (ktl::Exception& Excp)
      {
         printf("Main: caught void: %i\n", Excp.GetStatus());
         KInvariant(Excp.GetStatus() == STATUS_TIMEOUT);
         KDynStringA   str(allocator);
         if (Excp.ToString(str))
         {
             puts(str);
         }
      }

      //* Prove ktl::ExceptionHandle works
      ktl::ExceptionHandle eh1;
      try
      {
          eh1 = a1.GetExceptionHandle();
      #if !defined(PLATFORM_UNIX)
          eh1._RethrowException();
      #else
          rethrow_exception(eh1);
      #endif
      }
      catch (ktl::Exception& Excp)
      {
          printf("Main: caught void: %i\n", Excp.GetStatus());
          KInvariant(Excp.GetStatus() == STATUS_TIMEOUT);
          KDynStringA   str(allocator);
          if (Excp.ToString(str))
          {
              puts(str);
          }

          ktl::Exception excp1 = ktl::Exception::ToException(eh1);
          printf("Main: caught void: %i\n", excp1.GetStatus());
          KInvariant(excp1.GetStatus() == STATUS_TIMEOUT);
          KDynStringA   str1(allocator);
          if (excp1.ToString(str1))
          {
              puts(str);
          }

          KInvariant(str1.Compare(str) == 0);
      }
   }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});

   //** Simple Exception propagation (typed) test; custom exception; custom debug info provider
   Exception::SetDebugInfoProvider(_g_TestDebugInfoProvider);
   {
       char* testStr = "Test typed Exception Timeout Str";
       ktl::Awaitable<int> awaitable = ExpTypedTest1(allocator, testStr);
       ktl::Awaitable<int> a2 = Ktl::Move(awaitable);       // Prove copy ctor
       ktl::Awaitable<int> a1;                              // Prove default ctor
       a1 = Ktl::Move(a2);                                  // Prove copy 

       KArray<ktl::Awaitable<int>>  ta(allocator, 20);
       KInvariant(ta.SetCount(20));

       ta[0] = Ktl::Move(a1);

       a1 = Ktl::Move(ta[0]);

       try
       {
           int result = SyncAwait(a1);
           result;
       }
       catch (ktl::Exception& Excp)
       {
           // also prove that KRTT on Exception derivation works and we can catch on a base exception class
           KInvariant(is_type<MyException>(&Excp));
           MyException& myExcp = (MyException&)Excp;    // Downcast

           printf("Main: caught typed: %i; %s\n", myExcp.GetStatus(), myExcp.GetDesc());
           KInvariant(myExcp.GetStatus() == STATUS_TIMEOUT);
           KInvariant(myExcp.GetDesc() == testStr);

           KDynStringA   str(allocator, 1000);
           if (myExcp.ToString(str))
           {
               puts(str);
           }
       }

       //* Prove ktl::ExceptionHandle works
       try
       {
           ktl::ExceptionHandle eh1 = a1.GetExceptionHandle();
#if !defined(PLATFORM_UNIX)
           eh1._RethrowException();
#else
           rethrow_exception(eh1);
#endif
       }
       catch (ktl::Exception& Excp)
       {
           // also prove that KRTT on Exception derivation works and we can catch on a base exception class
           KInvariant(is_type<MyException>(&Excp));
           MyException& myExcp = (MyException&)Excp;    // Downcast

           printf("Main: caught typed: %i; %s\n", myExcp.GetStatus(), myExcp.GetDesc());
           KInvariant(myExcp.GetStatus() == STATUS_TIMEOUT);
           KInvariant(myExcp.GetDesc() == testStr);

           KDynStringA   str(allocator, 1000);
           if (myExcp.ToString(str))
           {
               puts(str);
           }
       }
   }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});

   Exception::ClearDebugInfoProvider();
   KInvariant(TestDebugInfoProvider::_g_AcquireCount > 0);
   KInvariant(TestDebugInfoProvider::_g_ReleaseCount == TestDebugInfoProvider::_g_AcquireCount);
   KInvariant(TestDebugInfoProvider::_g_UnsafeSymInitializeCount > 0);
   KInvariant(TestDebugInfoProvider::_g_UnsafeSymFromAddrCount > 0);
   KInvariant(TestDebugInfoProvider::_g_UnsafeSymGetLineFromAddr64Count > 0);

   TestDebugInfoProvider::_g_AcquireCount = 0;
   TestDebugInfoProvider::_g_ReleaseCount = 0;
   TestDebugInfoProvider::_g_UnsafeSymInitializeCount = 0;
   TestDebugInfoProvider::_g_UnsafeSymFromAddrCount = 0;
   TestDebugInfoProvider::_g_UnsafeSymGetLineFromAddr64Count = 0;

   //** Prove basic kasync::StartAwaiter in a Task behavior
   {
      puts("Starting: BasicStartAwaiterTest");
      KAsyncEvent           asyncEvent{ true };
      NTSTATUS              status;
      KAsyncEvent::WaitContext::SPtr    waitContext;
      BOOLEAN               hasStarted = false;
      KEvent                compEvent{ false };

      status = asyncEvent.CreateWaitContext(KTL_TAG_TEST, allocator, waitContext);
      KInvariant(NT_SUCCESS(status));

      BasicStartAwaiterTest(*waitContext, hasStarted, compEvent);
      KInvariant(hasStarted);

      hasStarted = false;
      asyncEvent.SetEvent();
      compEvent.WaitUntilSet();
      MemoryBarrier(); // work around bug in LLVM that is optimizing away the read below
      KInvariant(hasStarted);

      puts("Completion of: BasicStartAwaiterTest");
   }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});

   //** Prove basic kasync::StartAwaiter in a SyncAwait<void>() behavior
   {
      puts("Starting: BasicSyncAwaitVoidAwaiterTest");
      KAsyncEvent           asyncEvent{ true };
      NTSTATUS              status;
      KAsyncEvent::WaitContext::SPtr    waitContext;
      BOOLEAN               hasStarted = false;
      Awaitable<void>       future;

      {
          status = asyncEvent.CreateWaitContext(KTL_TAG_TEST, allocator, waitContext);
          KInvariant(NT_SUCCESS(status));

          future = BasicFutureTest(*waitContext, hasStarted);
          KInvariant(hasStarted);

          hasStarted = false;
          asyncEvent.SetEvent();
          SyncAwait(future);
          MemoryBarrier(); // work around bug in LLVM that is optimizing away the read below
          KInvariant(hasStarted);
      }

#if !defined(PLATFORM_UNIX)
      //** Prove out of memory behavior
      {
          ::_g_ForceAwaitableTFrameAllocationFailure = true;
          hasStarted = false;
          future = BasicFutureTest(*waitContext, hasStarted);
          KInvariant(!hasStarted);
          KInvariant((bool)future == false);
          KInvariant(future.await_ready() == true);
          KInvariant(future.await_suspend(std::experimental::coroutine_handle<void>{}) == false);
          KInvariant(future.IsComplete() == true);
          KInvariant(future.IsExceptionCompletion() == true);
          ExceptionHandle expPtr = future.GetExceptionHandle();
          try
          {
              #if defined(PLATFORM_UNIX)
              rethrow_exception(expPtr);
              #else
              expPtr._RethrowException();
              #endif
          }
          catch (ktl::FrameNotBoundException)
          {
          }
          catch (...)
          {
              KInvariant(false);
          }

          try
          {
              future.await_resume();
              KInvariant(false);
          }
          catch (ktl::FrameNotBoundException)
          {
          }
          catch (...)
          {
              KInvariant(false);
          }
          ::_g_ForceAwaitableTFrameAllocationFailure = false;
      }
#endif

      puts("Completion of: BasicSyncAwaitVoidAwaiterTest");
   }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});
   
   //** Prove basic kasync::StartAwaiter in a SyncAwait<T>() behavior
   {
      puts("Starting: BasicSyncAwaitTypedAwaiterTest");
      KAsyncEvent::WaitContext::SPtr    waitContext;
      BOOLEAN                           hasStarted = false;
      KAsyncEvent                       asyncEvent{ true };
      NTSTATUS                          status;
      Awaitable<NTSTATUS>               future;

      {
          status = asyncEvent.CreateWaitContext(KTL_TAG_TEST, allocator, waitContext);
          KInvariant(NT_SUCCESS(status));

          future = BasicFutureTypedTest(*waitContext, hasStarted);
          KInvariant(hasStarted);

          hasStarted = false;
          asyncEvent.SetEvent();
          status = SyncAwait(future);
          KInvariant(status == STATUS_WAIT_63);
          MemoryBarrier(); // work around bug in LLVM that is optimizing away the read below
          KInvariant(hasStarted);
      }

#if !defined(PLATFORM_UNIX)
      //** Prove out of memory behavior
      {
          ::_g_ForceAwaitableTFrameAllocationFailure = true;
          hasStarted = false;
          future = BasicFutureTypedTest(*waitContext, hasStarted);
          KInvariant(!hasStarted);
          KInvariant((bool)future == false);
          KInvariant(future.await_ready() == true);
          KInvariant(future.await_suspend(std::experimental::coroutine_handle<void>{}) == false);
          KInvariant(future.IsComplete() == true);
          KInvariant(future.IsExceptionCompletion() == true);
          ExceptionHandle expPtr = future.GetExceptionHandle();
          try
          {
              #if defined(PLATFORM_UNIX)
              rethrow_exception(expPtr);
              #else
              expPtr._RethrowException();
              #endif
          }
          catch (ktl::FrameNotBoundException)
          {
          }
          catch (...)
          {
              KInvariant(false);
          }

          try
          {
              future.await_resume();
              KInvariant(false);
          }
          catch (ktl::FrameNotBoundException)
          {
          }
          catch (...)
          {
              KInvariant(false);
          }
          ::_g_ForceAwaitableTFrameAllocationFailure = false;
      }
#endif
      
      puts("Completion of: BasicSyncAwaitTypedAwaiterTest");
   }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});

   //** Prove basic kasync::StartAwaiter in a SyncAwait<void> behavior
   {
      puts("BasicAwaitTests: BasicAwaitableVoidTest: Started");
      auto x = BasicAwaitableVoidTest(allocator, "Test String");
      SyncAwait(x);
      puts("BasicAwaitTests: BasicAwaitableVoidTest: completed");
   }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});

   //** Prove SyncAwait<T> implemented within an on-demand created KShared<> derivation
   //   instance 
   {
      auto r = SyncAwait(BasicAwaitableTypeTest(allocator, 121212));
      printf("BasicAwaitTests: BasicAwaitableTypeTest returned: %u\n", r);
      KInvariant(r == 121212);
   }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});

   //** Custom KAsyncContextBase derivation: prove instance methods for: 
   //   Tasks, Awaitable<>, and kasync::StartAwaitable; includes parallel
   //   executing private Tasks that hold completion until done
   {
      AsyncAwaiterTestContext::SPtr testInst = AsyncAwaiterTestContext::Create(allocator);

      // Prove sync return of NTSTATUS works on Start*Async methods
      SyncAwait(testInst->DoNullFailedTest());

      // Run multi threaded exerciser - completes via a Future<>
      testInst->Reuse();
      KNt::Sleep(1000);
      NTSTATUS status = SyncAwait(testInst->StartAsync());
      printf("StartAync: returned: status: %i\n", status);
      KInvariant(NT_SUCCESS(status));

      // Now run multi threaded exerciser but complete via a Awaitable<>
      testInst->Reuse();
      status = SyncAwait(ExecAwaiterParallelKAsync(*testInst));
      printf("ExecAwaiterParallelKAsync: returned: status: %i\n", status);
      KInvariant(NT_SUCCESS(status));
   }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});

   //* Simple test to prove all the awaitable KAsyncServiceBase support   
   {
      auto status = SyncAwait(SimpleServiceTests(allocator, "Service Test"));
      printf("BasicAwaitTests: SimpleServiceTests returned: %i\n", status);
      KInvariant(NT_SUCCESS(status));
   }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});
   
   //** Prove ktl::Exception assignment from move
   {
        ktl::Exception exception = ktl::Exception(0);
        ktl::Exception temp = ktl::Exception(K_STATUS_API_CLOSED);
        exception = Ktl::Move(temp);
        KInvariant(exception.GetStatus() == K_STATUS_API_CLOSED);
   }

   //** Prove AwaitableCompletionSource<T> behavior
   {
      AwaitableCompletionSource<int>::SPtr testSPtr0;
      NTSTATUS status = AwaitableCompletionSource<int>::Create(allocator, KTL_TAG_TEST, testSPtr0);
      KInvariant(NT_SUCCESS(status));

      AwaitableCompletionSource<int>::SPtr testSPtr1;
      status = AwaitableCompletionSource<int>::Create(allocator, KTL_TAG_TEST, testSPtr1);
      KInvariant(NT_SUCCESS(status));

      AwaitableCompletionSource<int>::SPtr testSPtr2;
      status = AwaitableCompletionSource<int>::Create(allocator, KTL_TAG_TEST, testSPtr2);
      KInvariant(NT_SUCCESS(status));

      KEvent        compEvent{ false };
      const int MaxMonitors = 1000;
      static char   names[MaxMonitors][128];

      AcsMonitorCount = MaxMonitors * 3;
      for (int x = 0; x < MaxMonitors; x++)
      {
         sprintf(&names[x][0], "Test Monitor %u", x + 1);
         AcsMonitor(*testSPtr0, &names[x][0], compEvent);
         AcsMonitor(*testSPtr1, &names[x][0], compEvent);
         AcsMonitor(*testSPtr2, &names[x][0], compEvent);
      }
      testSPtr0->SetResult(12345);
      testSPtr1->SetException(ktl::Exception(STATUS_ABANDONED));
      KInvariant(testSPtr1->IsExceptionSet());
      testSPtr2->SetCanceled();

      compEvent.WaitUntilSet();
   }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});

   //** Prove AwaitableCompletionSource<void> behavior
   {
      AwaitableCompletionSource<void>::SPtr testSPtr0;
      NTSTATUS      status = AwaitableCompletionSource<void>::Create(allocator, KTL_TAG_TEST, testSPtr0);
      KInvariant(NT_SUCCESS(status));

      AwaitableCompletionSource<void>::SPtr testSPtr1;
      status = AwaitableCompletionSource<void>::Create(allocator, KTL_TAG_TEST, testSPtr1);
      KInvariant(NT_SUCCESS(status));

      AwaitableCompletionSource<void>::SPtr testSPtr2;
      status = AwaitableCompletionSource<void>::Create(allocator, KTL_TAG_TEST, testSPtr2);
      KInvariant(NT_SUCCESS(status));

      KEvent        compEvent{ false };
      const int MaxMonitors = 1000;
      static char       names[MaxMonitors][128];

      AcsMonitorCount = MaxMonitors * 3;
      for (int x = 0; x < MaxMonitors; x++)
      {
         sprintf(&names[x][0], "Test Monitor %u", x + 1);
         AcsVoidMonitor(*testSPtr0, &names[x][0], compEvent);
         AcsVoidMonitor(*testSPtr1, &names[x][0], compEvent);
         AcsVoidMonitor(*testSPtr2, &names[x][0], compEvent);
      }
      testSPtr0->Set();
      testSPtr1->SetException(ktl::Exception(STATUS_ABANDONED));
      testSPtr2->SetCanceled();

      compEvent.WaitUntilSet();
   }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});

   /** Prove common KTL building blocks with awaitable wrapper methods */
   /*  KSharedAsyncLock */
   {
       puts("BasicAwaitTests: Awaitable KSharedAsyncLock Test: Started");
       KSharedAsyncLock::SPtr lock;
       NTSTATUS status = KSharedAsyncLock::Create(KTL_TAG_TEST, allocator, lock);
       KInvariant(NT_SUCCESS(status));

       KSharedAsyncLock::Handle::SPtr waitHandle;
       status = lock->CreateHandle(KTL_TAG_TEST, waitHandle);
       KInvariant(NT_SUCCESS(status));

       volatile bool    go = false;

       auto delayedSignaller = [&]() -> Task
       {
            while (!go)
            {
                NTSTATUS status = co_await KTimer::StartTimerAsync(allocator, KTL_TAG_TEST, 500, nullptr);
                KInvariant(NT_SUCCESS(status));
            }

            waitHandle->ReleaseLock();
       };

       delayedSignaller();

       // First acquire the lock
       status = SyncAwait(waitHandle->StartAcquireLockAsync(nullptr));
       KInvariant(NT_SUCCESS(status));

       go = true;
       KSharedAsyncLock::Handle::SPtr waitHandle1;

       status = SyncAwait(lock->StartAcquireLockAsync(KTL_TAG_TEST, waitHandle1, nullptr));
       KInvariant(NT_SUCCESS(status));

       waitHandle1->ReleaseLock();
       puts("BasicAwaitTests: Awaitable KSharedAsyncLock Test: Completed");
   }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});

   /*  KQuotaGate */
   {
       puts("BasicAwaitTests: Awaitable KQuotaGate Test: Started");
       KQuotaGate::SPtr gate;
       NTSTATUS status = KQuotaGate::Create(KTL_TAG_TEST, allocator, gate);
       KInvariant(NT_SUCCESS(status));

       status = gate->Activate(1000, nullptr, nullptr);
       KInvariant(NT_SUCCESS(status));

       KQuotaGate::AcquireContext::SPtr waitHandle;
       status = gate->CreateAcquireContext(waitHandle);
       KInvariant(NT_SUCCESS(status));

       volatile bool    go = false;

       auto delayedSignaller = [&]() -> Task
       {
           while (!go)
           {
               NTSTATUS status = co_await KTimer::StartTimerAsync(allocator, KTL_TAG_TEST, 1000, nullptr);
               KInvariant(NT_SUCCESS(status));
           }

           gate->ReleaseQuanta(500);
       };

       // Prove KQuotaGate::AcquireContext::StartAcquireAsync()
       delayedSignaller();

       go = true;
       status = SyncAwait(waitHandle->StartAcquireAsync(1500, nullptr));
       KInvariant(NT_SUCCESS(status));

       // Prove KQuotaGate::StartAcquireAsync()
       gate->ReleaseQuanta(1000);
       go = false;
       delayedSignaller();

       go = true;
       status = SyncAwait(gate->StartAcquireAsync(1500, nullptr));
       KInvariant(NT_SUCCESS(status));

       gate->Deactivate();

       puts("BasicAwaitTests: Awaitable KQuotaGate Test: Completed");
   }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});

   /*  KAsyncQueue<> */
   {
       puts("BasicAwaitTests: Awaitable KAsyncQueue<> Test: Started");

       struct QEntry
       {
           KListEntry   Links;
       };

       QEntry       qe1;
       QEntry       qe2;

       KAsyncQueue<QEntry>::SPtr   queue;
       NTSTATUS status = KAsyncQueue<QEntry>::Create(KTL_TAG_TEST, allocator, 0, queue);
       KInvariant(NT_SUCCESS(status));

       status = queue->Activate(nullptr, nullptr);
       KInvariant(NT_SUCCESS(status));

       KAsyncQueue<QEntry>::DequeueOperation::SPtr dqOp;
       status = queue->CreateDequeueOperation(dqOp);
       KInvariant(NT_SUCCESS(status));

       volatile bool    go = false;

       auto delayedSignaller = [&]() -> Task
       {
           while (!go)
           {
               NTSTATUS status = co_await KTimer::StartTimerAsync(allocator, KTL_TAG_TEST, 1000, nullptr);
               KInvariant(NT_SUCCESS(status));
           }

           queue->Enqueue(qe1);
       };

       delayedSignaller();

       go = true;
       status = SyncAwait(dqOp->StartDequeueAsync(nullptr));
       KInvariant(NT_SUCCESS(status));
       KInvariant(&qe1 == &dqOp->GetDequeuedItem());

       go = false;
       delayedSignaller();

       go = true;
       status = SyncAwait(queue->StartDequeueAsync(qe2, nullptr));
       KInvariant(NT_SUCCESS(status));

       queue->Deactivate();

       puts("BasicAwaitTests: Awaitable  KAsyncQueue<> Test: Completed");
   }
   // Allow all background tasks to complete
   SleepUntil([&]() {return _g_KCo$TotalObjects == 0;});
   puts("BasicAwaitTests:Completing: _g_KCo$TotalObjects == 0");

   SleepUntil([&]() {return GClass::testGClassDtored;});
   puts("BasicAwaitTests:Completing: GClass::testGClassDtored == true");

   KtlSystemBase::DebugOutputKtlSystemBases();
   SleepUntil([&]() {return (KAllocatorSupport::gs_AllocsRemaining == StartingAllocs);});
   puts("BasicAwaitTests:Completing: gs_AllocsRemaining == StartingAllocs");

   puts("BasicAwaitableTests: Completed");
}


NTSTATUS
KtlAwaitableTest(
    __in int argc,
    __in_ecount(argc) WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    NTSTATUS status;
#if defined(PLATFORM_UNIX)
    status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif
    
    EventRegisterMicrosoft_Windows_KTL();
    KTestPrintf("Starting KtlAwaitableTest test\n");

    KtlSystem* underlyingSystem;
    status = KtlSystem::Initialize(&underlyingSystem);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("%s: %i: KtlSystem::Initialize failed\n", __FUNCTION__, __LINE__);
        return status;
    }

    // Turn on strict allocation tracking
    underlyingSystem->SetStrictAllocationChecks(TRUE);

    // Run the tests with both TP options
#if !defined(PLATFORM_UNIX)
    underlyingSystem->SetDefaultSystemThreadPoolUsage(TRUE);        // Use only shared system threadpool for non IOCP Asyncs
    BasicAwaitableTests(*underlyingSystem);
#endif
    underlyingSystem->SetDefaultSystemThreadPoolUsage(FALSE);       // Use only dedicated ktl threadpool for all Asyncs
    BasicAwaitableTests(*underlyingSystem);

    KtlSystem::Shutdown();

    EventUnregisterMicrosoft_Windows_KTL();

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif  
    
    return STATUS_SUCCESS;
}

#if CONSOLE_TEST


int __cdecl
#if !defined(PLATFORM_UNIX)
wmain(
    __in int argc,
    __in_ecount(argc) WCHAR* args[]
)
{
#else
main(int argc, char* args[])
{
#endif
    NTSTATUS status;

    //
    // Adjust for the test name so CmdParseLine works.
    //
    if (argc > 0)
    {
        argc--;
        args++;
    }

    status = KtlAwaitableTest(argc, (WCHAR**)args);

    if (!NT_SUCCESS(status)) 
   {
        KTestPrintf("Ktl Awaitable tests failed Status = 0X%X\n", status);
        return RtlNtStatusToDosError(status);
    }

    return RtlNtStatusToDosError(status);
}
#endif

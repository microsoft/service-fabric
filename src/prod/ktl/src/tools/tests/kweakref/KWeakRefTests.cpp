/*++

Copyright (c) Microsoft Corporation

Module Name:

    KWeakRefTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KWeakRefTests.h.
    2. Add an entry to the gs_KuTestCases table in KWeakRefTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once

// Enable unit test specific code
#define EnableKWeakRefUnitTest 1

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(PLATFORM_UNIX)
#include <vector>
#endif
#include <ktl.h>
#include <ktrace.h>
#include "KWeakRefTests.h"
#include <CommandLineParser.h>

#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif

class TestObj : public KObject<TestObj>, public KShared<TestObj>
{
    K_FORCE_SHARED_WITH_INHERITANCE(TestObj);

public:
    static NTSTATUS
    CreateTestObj(__out TestObj::SPtr& Result)
    {
        Result = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) TestObj();
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
};

TestObj::TestObj()
{
    KFatal(_RefCount == 0);
}

TestObj::~TestObj()
{
}

class DerivedTestObj : public TestObj, public KWeakRefType<DerivedTestObj>
{
    K_FORCE_SHARED(DerivedTestObj);

public:
    static NTSTATUS
    CreateDerivedTestObj(__out DerivedTestObj::SPtr& Result)
    {
        Result = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) DerivedTestObj();
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
};

DerivedTestObj::DerivedTestObj()
{
    KFatal(_RefCount == 0);
}

DerivedTestObj::~DerivedTestObj()
{
}

//** Thread entry to test KWeakRef TakeRef race occurance - and proper behavior given that race
volatile LONG TakeRefRaceTestThreadCount;
volatile BOOLEAN AbortTakeRefRaceTest = FALSE;

VOID
TakeRefRaceTestThreadEntry(__inout_opt VOID* Parameter)
{
    DerivedTestObj*                 rawDerivedTestObj = (DerivedTestObj*)Parameter;
    KWeakRef<DerivedTestObj>::SPtr  testWeakRef = rawDerivedTestObj->GetWeakRef();

    while ((!rawDerivedTestObj->_TryTakeRefRaceDetected) && !AbortTakeRefRaceTest)
    {
        DerivedTestObj::SPtr testObj = testWeakRef->TryGetTarget();
        KFatal(testObj);
        testObj.Reset();
    }

    _InterlockedDecrement(&TakeRefRaceTestThreadCount);
}


NTSTATUS
TakeRefRaceTest()
{
    NTSTATUS                        status;
    DerivedTestObj::SPtr            testObj;
    KWeakRef<DerivedTestObj>::SPtr  testWeakRef;
    DerivedTestObj::SPtr            testObjRef1;

    //
    // There is an issue with the TakeRefRaceTest on single proc
    // machines, typically VMs. The test is trying to simulate a race
    // condition between multiple thread taking a weak ref count. Since
    // the window for the race is so small that on a single proc
    // machine it is never hit. Special test code is added to the
    // implementation to ensure that the race is hit in user mode
    // however this is not feasible in kernel mode as a spinlock is
    // held over the race window. Therefore the test is not run in
    // kernel mode on single proc machines
    //
#if !KTL_USER_MODE

    if (KeQueryActiveProcessorCountEx(0) == 1)
    {
        KTestPrintf("TakeRefRaceTest: Skipping test on single proc machine\n", __LINE__);
        return STATUS_SUCCESS;
    }
#endif

    status = DerivedTestObj::CreateDerivedTestObj(testObj);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("TakeRefRaceTest: CreateDerivedTestObj Failed: %i\n", __LINE__);
        return status;
    }

    testWeakRef = testObj->GetWeakRef();
    if (!testWeakRef->IsAlive())
    {
        KTestPrintf("TakeRefRaceTest: IsAlive Failed: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    const int       numberOfRaceTestThreads = 10;
    KThread::SPtr   raceTestThreads[numberOfRaceTestThreads];

    TakeRefRaceTestThreadCount = numberOfRaceTestThreads;
    for (int ix = 0; ix < numberOfRaceTestThreads; ix++)
    {
        if (!NT_SUCCESS(KThread::Create(TakeRefRaceTestThreadEntry, testObj.RawPtr(), raceTestThreads[ix],
                KtlSystem::GlobalNonPagedAllocator())))
        {
            KTestPrintf("TakeRefRaceTest: KThread::Create Failed: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }
    }

    // Wait for all the test threads to exit normally - having detected the race condition
    ULONGLONG       timeToStop = KNt::GetTickCount64() + (10 * 1000);   // 10 secs from now
    BOOLEAN         timedOut = FALSE;

    // The following loop waits for:
    //  1) At least one of the the threads (raceTestThreads) to detect the race condition within
    //     KWeakRef<>::TryGetTarget()
    //  2) All of the test threads see the detected condition and exit
    //  3) OR a timeout indicating the race path code not be caused
    //

    timedOut = !(timeToStop > KNt::GetTickCount64());
    while ((TakeRefRaceTestThreadCount > 0) && !timedOut)
    {
        KNt::Sleep(0);
        timedOut = !(timeToStop > KNt::GetTickCount64());
    }

    if (timedOut)
    {
        AbortTakeRefRaceTest = TRUE;
    }

    for (int ix = 0; ix < numberOfRaceTestThreads; ix++)
    {
        raceTestThreads[ix].Reset();
    }

    if (timedOut)
    {
        KTestPrintf("TakeRefRaceTest: timed out: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    testObjRef1 = testWeakRef->TryGetTarget();
    if (!testObjRef1)
    {
        KTestPrintf("TakeRefRaceTest: TryGetTarget Failed: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    if (testObjRef1.RawPtr() != testObj.RawPtr())
    {
        KTestPrintf("TakeRefRaceTest: Raw testObj pointers mis-match: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    // Test that the destruction occurs for DerivedTestObj
    if (testObj.Reset())
    {
        KTestPrintf("TakeRefRaceTest: Unexpected destruction: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    if (!testObjRef1.Reset())
    {
        KTestPrintf("TakeRefRaceTest: Expected destruction did not occur: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    // Test that we get an empty SPtr when we try to bind to an expired obj via a KWeakRef
    testObjRef1 = testWeakRef->TryGetTarget();
    if (testObjRef1)
    {
        KTestPrintf("TakeRefRaceTest: TryGetTarget returned unexpected non-empty SPtr: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    if (testWeakRef->IsAlive())
    {
        KTestPrintf("TakeRefRaceTest: IsAlive Failed: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    // Prove the detached weak ref destructs
    if (!testWeakRef.Reset())
    {
        KTestPrintf("TakeRefRaceTest: Expected destruction did not occur: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}


//** Thread entery to test the destructor racing code path
LONG DestructorRaceTestActiveCount;

VOID
DestructorRaceTestThreadEntry(__inout_opt VOID* Parameter)
{
    KWeakRef<DerivedTestObj>::SPtr  testWeakRef((KWeakRef<DerivedTestObj>*)Parameter);
    bool oneshot = false;

    while (testWeakRef->IsAlive())
    {
        if (!oneshot)
        {
            oneshot = true;
            _InterlockedIncrement(&DestructorRaceTestActiveCount);
        }
    }

    _InterlockedDecrement(&DestructorRaceTestActiveCount);
}

NTSTATUS
DestructorRaceTest()
{
    BOOLEAN         raceWasDetected = FALSE;
    ULONGLONG       timeToStop = KNt::GetTickCount64() + (60 * 1000);   // 60 secs from now

    //
    // There is an issue with the DestructorRaceTest on single proc
    // machines, typically VMs. The test is trying to simulate a race
    // condition between multiple thread taking a weak ref count. Since
    // the window for the race is so small that on a single proc
    // machine it is never hit. Special test code is added to the
    // implementation to ensure that the race is hit in user mode
    // however this is not feasible in kernel mode as a spinlock is
    // held over the race window. Therefore the test is not run in
    // kernel mode on single proc machines
    //
#if !KTL_USER_MODE

    if (KeQueryActiveProcessorCountEx(0) == 1)
    {
        KTestPrintf("DestructorRaceTest: Skipping test on single proc machine\n", __LINE__);
        return STATUS_SUCCESS;
    }
#endif

    while (!raceWasDetected && (timeToStop > KNt::GetTickCount64()))
    {
        DerivedTestObj::SPtr    testObj;
        NTSTATUS                status;

        status = DerivedTestObj::CreateDerivedTestObj(testObj);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("DestructorRaceTest: CreateDerivedTestObj Failed: %i\n", __LINE__);
            return status;
        }

        KWeakRef<DerivedTestObj>::SPtr testWeakRef = testObj->GetWeakRef();
        if (!testWeakRef->IsAlive())
        {
            KTestPrintf("DestructorRaceTest: IsAlive Failed: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        DestructorRaceTestActiveCount = 0;
        KThread::SPtr testThread;
        if (!NT_SUCCESS(KThread::Create(DestructorRaceTestThreadEntry, testWeakRef.RawPtr(), testThread,
                KtlSystem::GlobalNonPagedAllocator())))
        {
            KTestPrintf("TakeRefRaceTest: KThread::Create Failed: %i\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        // wait for the thread to come alive
        while (DestructorRaceTestActiveCount == 0)
        {
            KNt::Sleep(0);
        }

        // Do destruction and wait for thread to complete
        testObj.Reset();
        while (DestructorRaceTestActiveCount != 0)
        {
            KNt::Sleep(0);
        }

        raceWasDetected = testWeakRef->_TargetDestructingRaceDetected;
    }

    if (!raceWasDetected)
    {
        KTestPrintf("TakeRefRaceTest: Timed out attempting to create condition: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
MultiCoreWeakRefTest(int argc, WCHAR* args[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    NTSTATUS    status;
    status = TakeRefRaceTest();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = DestructorRaceTest();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
BasicWeakRefTest(int argc, WCHAR* args[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    NTSTATUS                status;
    DerivedTestObj::SPtr    testObj;

    status = DerivedTestObj::CreateDerivedTestObj(testObj);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("BasicWeakRefTest: CreateDerivedTestObj Failed: %i\n", __LINE__);
        return status;
    }

    KWeakRef<DerivedTestObj>::SPtr testWeakRef = testObj->GetWeakRef();
    if (!testWeakRef->IsAlive())
    {
        KTestPrintf("BasicWeakRefTest: IsAlive Failed: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    DerivedTestObj::SPtr testObjRef1 = testWeakRef->TryGetTarget();
    if (!testObjRef1)
    {
        KTestPrintf("BasicWeakRefTest: TryGetTarget Failed: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    if (testObjRef1.RawPtr() != testObj.RawPtr())
    {
        KTestPrintf("BasicWeakRefTest: Raw testObj pointers mis-match: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    // Test that the destruction occurs for DerivedTestObj
    if (testObj.Reset())
    {
        KTestPrintf("BasicWeakRefTest: Unexpected destruction: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    if (!testObjRef1.Reset())
    {
        KTestPrintf("BasicWeakRefTest: Expected destruction did not occur: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    // Test that we get an empty SPtr when we try to bind to an expired obj via a KWeakRef
    testObjRef1 = testWeakRef->TryGetTarget();
    if (testObjRef1)
    {
        KTestPrintf("BasicWeakRefTest: TryGetTarget returned unexpected non-empty SPtr: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    if (testWeakRef->IsAlive())
    {
        KTestPrintf("BasicWeakRefTest: IsAlive Failed: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    // Prove the detached weak ref destructs
    if (!testWeakRef.Reset())
    {
        KTestPrintf("BasicWeakRefTest: Expected destruction did not occur: %i\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}


//** Unit test for weak refs on shared interfaces; multiple and nested multiple inheritance
class TestWRefIf
{
    K_SHARED_INTERFACE(TestWRefIf);
    K_WEAKREF_INTERFACE(TestWRefIf);

public:
    virtual void TestMethod() = 0;
};


class TestWRefIf1
{
    K_SHARED_INTERFACE(TestWRefIf1);
    K_WEAKREF_INTERFACE(TestWRefIf1);

public:
    virtual void TestMethod1() = 0;
};

class TestWRefIfA
{
    K_SHARED_INTERFACE(TestWRefIfA);
    K_WEAKREF_INTERFACE(TestWRefIfA);

public:
    virtual void TestMethodA() = 0;
};

class TestWRefIfA1 : public TestWRefIfA
{
    K_SHARED_INTERFACE(TestWRefIfA1);
    K_WEAKREF_INTERFACE(TestWRefIfA1);

public:
    virtual void TestMethodA1() = 0;
};



class TestWRefIfImp :
    public KShared<TestWRefIfImp>,
    public KObject<TestWRefIfImp>,
    public KWeakRefType<TestWRefIfImp>,
    public TestWRefIf,
    public TestWRefIf1,
    public TestWRefIfA1
{
    K_FORCE_SHARED(TestWRefIfImp);
    K_SHARED_INTERFACE_IMP(TestWRefIf);
    K_SHARED_INTERFACE_IMP(TestWRefIf1);
    K_SHARED_INTERFACE_IMP(TestWRefIfA);
    K_SHARED_INTERFACE_IMP(TestWRefIfA1);
    K_WEAKREF_INTERFACE_IMP(TestWRefIf, TestWRefIfImp);
    K_WEAKREF_INTERFACE_IMP(TestWRefIf1, TestWRefIfImp);
    K_WEAKREF_INTERFACE_IMP(TestWRefIfA, TestWRefIfImp);
    K_WEAKREF_INTERFACE_IMP(TestWRefIfA1, TestWRefIfImp);

public:
    int     _TestMethodWasCalled;
    int     _TestMethod1WasCalled;
    int     _TestMethodAWasCalled;
    int     _TestMethodA1WasCalled;

    void TestMethod() override
    {
        _TestMethodWasCalled++;
    }

    void TestMethod1() override
    {
        _TestMethod1WasCalled++;
    }

    void TestMethodA() override
    {
        _TestMethodAWasCalled++;
    }

    void TestMethodA1() override
    {
        _TestMethodA1WasCalled++;
    }

    static KSharedPtr<TestWRefIfImp> Create(KAllocator& Allocator)
    {
        return new(KTL_TAG_TEST, Allocator) TestWRefIfImp();
    }
};

TestWRefIfImp::TestWRefIfImp()
{
    _TestMethodWasCalled = 13;
    _TestMethod1WasCalled = 7;
    _TestMethodAWasCalled = 11;
    _TestMethodA1WasCalled = 17;
}

TestWRefIfImp::~TestWRefIfImp()
{
}


void
SharedInterfaceWeakRefTest()
{
    LONGLONG        startAllocs = KAllocatorSupport::gs_AllocsRemaining;

    TestWRefIfImp::SPtr             out = TestWRefIfImp::Create(KtlSystem::GlobalNonPagedAllocator());
    TestWRefIf::SPtr                ifot = out.RawPtr();
    KInvariant(out.RawPtr() != nullptr);
    KInvariant(KAllocatorSupport::gs_AllocsRemaining == startAllocs + 2);   // Object and WeakRef

    //* Prove we can do a basic call thru a ref counted I/F
    ifot->TestMethod();
    KInvariant(out->_TestMethodWasCalled == 13+1);

    KWeakRef<TestWRefIfImp>::SPtr   wRefImp = out->GetWeakRef();
    KInvariant(wRefImp.RawPtr() != nullptr);
    KInvariant(KAllocatorSupport::gs_AllocsRemaining == startAllocs + 2);

    KWeakRef<TestWRefIf>::SPtr      wRefIf;
    KInvariant(NT_SUCCESS(ifot->GetWeakIfRef(wRefIf)));
    KInvariant(KAllocatorSupport::gs_AllocsRemaining == startAllocs + 2 + 1);   // +1 for per GetWeakIfRef() call type map object

    TestWRefIfImp::SPtr             out1 = wRefImp->TryGetTarget();
    KInvariant(out1.RawPtr() != nullptr);
    KInvariant(KAllocatorSupport::gs_AllocsRemaining == startAllocs + 2 + 1);

    TestWRefIf::SPtr                ifot1 = wRefIf->TryGetTarget();
    KInvariant(ifot1.RawPtr() != nullptr);
    KInvariant(KAllocatorSupport::gs_AllocsRemaining == startAllocs + 2 + 1);

    //* Prove we can do a call thru a recovered ref counted I/F from a weak ref
    ifot->TestMethod();
    KInvariant(out->_TestMethodWasCalled == 13+2);

    //* Prove we can access via 2nd interface
    TestWRefIf1::SPtr               i1fot = out.RawPtr();

    KWeakRef<TestWRefIf1>::SPtr     wRefIf1;
    KInvariant(NT_SUCCESS(i1fot->GetWeakIfRef(wRefIf1)));
    KInvariant(KAllocatorSupport::gs_AllocsRemaining == startAllocs + 2 + 1 + 1);

    TestWRefIf1::SPtr               i1fot1 = wRefIf1->TryGetTarget();
    KInvariant(i1fot1.RawPtr() != nullptr);

    i1fot1->TestMethod1();
    KInvariant(out->_TestMethod1WasCalled == 7+1);

    i1fot = nullptr;
    wRefIf1 = nullptr;
    i1fot1 = nullptr;
    KInvariant(KAllocatorSupport::gs_AllocsRemaining == startAllocs + 2 + 1);

    //* Prove we can access via 3rd interface's base interface
    TestWRefIfA::SPtr               iAfot = out.RawPtr();

    KWeakRef<TestWRefIfA>::SPtr     wRefIfA;
    KInvariant(NT_SUCCESS(iAfot->GetWeakIfRef(wRefIfA)));
    KInvariant(KAllocatorSupport::gs_AllocsRemaining == startAllocs + 2 + 1 + 1);

    TestWRefIfA::SPtr               iAfot1 = wRefIfA->TryGetTarget();
    KInvariant(iAfot1.RawPtr() != nullptr);

    iAfot1->TestMethodA();
    KInvariant(out->_TestMethodAWasCalled ==11+1);

    iAfot = nullptr;
    wRefIfA = nullptr;
    iAfot1 = nullptr;
    KInvariant(KAllocatorSupport::gs_AllocsRemaining == startAllocs + 2 + 1);

    //* Prove we can access via 3rd interface
    TestWRefIfA1::SPtr              iA1fot = out.RawPtr();

    KWeakRef<TestWRefIfA1>::SPtr    wRefIfA1;
    KInvariant(NT_SUCCESS(iA1fot->GetWeakIfRef(wRefIfA1)));
    KInvariant(KAllocatorSupport::gs_AllocsRemaining == startAllocs + 2 + 1 + 1);

    TestWRefIfA1::SPtr              iA1fot1 = wRefIfA1->TryGetTarget();
    KInvariant(iA1fot1.RawPtr() != nullptr);

    iA1fot1->TestMethodA1();
    KInvariant(out->_TestMethodA1WasCalled ==17+1);

    iA1fot = nullptr;
    wRefIfA1 = nullptr;
    iA1fot1 = nullptr;
    KInvariant(KAllocatorSupport::gs_AllocsRemaining == startAllocs + 2 + 1);

    BOOLEAN                         isThere = wRefImp->IsAlive();
    KInvariant(isThere);
    isThere = wRefIf->IsAlive();
    KInvariant(isThere);

    out = nullptr;
    KInvariant(wRefIf->IsAlive());
    KInvariant(wRefImp->IsAlive());

    ifot = nullptr;
    KInvariant(wRefIf->IsAlive());
    KInvariant(wRefImp->IsAlive());

    out1 = nullptr;
    KInvariant(wRefIf->IsAlive());
    KInvariant(wRefImp->IsAlive());

    KInvariant(KAllocatorSupport::gs_AllocsRemaining == startAllocs + 2 + 1);
    ifot1 = nullptr;
    KInvariant(!wRefIf->IsAlive());
    KInvariant(!wRefImp->IsAlive());
    KInvariant(KAllocatorSupport::gs_AllocsRemaining == startAllocs + 2 + 1 - 1);   // Underlying object dtor

    wRefImp = nullptr;
    KInvariant(KAllocatorSupport::gs_AllocsRemaining == startAllocs + 2 + 1 - 1);   // wRefIf keeping wr graph alive

    wRefIf = nullptr;

    KInvariant(KAllocatorSupport::gs_AllocsRemaining == startAllocs + 2 + 1 - 1 - 2);   // should be back where we started
    KInvariant(KAllocatorSupport::gs_AllocsRemaining == startAllocs);
}



NTSTATUS
KWeakRefTest(int argc, WCHAR* args[])
{

#if defined(PLATFORM_UNIX)
    NTSTATUS status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif
    
    KTestPrintf("KWeakRefTest: STARTED\n");

    NTSTATUS  result;

    EventRegisterMicrosoft_Windows_KTL();
    KtlSystem::Initialize();

    KTestPrintf("Starting KWeakRefTest test");
    if ((result = BasicWeakRefTest(argc, args)) != STATUS_SUCCESS)
    {
        KTestPrintf("KWeakRefTest Unit Test: FAILED\n");
    }

    if ((result = MultiCoreWeakRefTest(argc, args)) != STATUS_SUCCESS)
    {
        KTestPrintf("KWeakRefTest Unit Test: FAILED\n");
    }

    SharedInterfaceWeakRefTest();

    KtlSystem::Shutdown();
    EventUnregisterMicrosoft_Windows_KTL();

    KTestPrintf("KWeakRefTest: COMPLETED\n");

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
    KWeakRefTest(argc - 1, args);
    return 0;
}
#endif

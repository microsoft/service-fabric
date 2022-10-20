/*++

Copyright (c) Microsoft Corporation

Module Name:

    KtlCommonTests.cpp

Abstract:

    This file contains test case implementations for some common KTL functionalities.

    To add a new unit test case:
    1. Declare your test case routine in KtlCommonTests.h.
    2. Add an entry to the gs_KuTestCases table in KtlCommonTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#define UnitTest_KSPtrBroker 1

#include <wchar.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(PLATFORM_UNIX)
#include <vector>
#endif
#include <ktl.h>
#include <ktrace.h>
#include "KtlCommonTests.h"
#include <CommandLineParser.h>
#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif

#if KTL_USER_MODE
extern volatile LONGLONG gs_AllocsRemaining;
#endif

// hasTestElementWithContentCount detects the presence of TestElementWithContentCount in T.
template <class T, class = int>
struct hasTestElementWithContentCount
{
    static constexpr bool value = false;
};
template <class T>
struct hasTestElementWithContentCount<T, decltype((void)T::TestElementWithContentCount, 0)>
{
    static constexpr bool value = true;
};

// hasTestElementCount detects the presence of TestElementCount in T.
template <class T, class = int>
struct hasTestElementCount
{
    static constexpr bool value = false;
};
template <class T>
struct hasTestElementCount<T, decltype((void)T::TestElementCount, 0)>
{
    static constexpr bool value = true;
};

template <typename T>
void Validate_TestElementWithContentCountEquals(ULONG count, typename Ktl::enable_if<hasTestElementWithContentCount<T>::value>::type* = nullptr)
{
    KInvariant(T::TestElementWithContentCount == count);
}

template <typename T>
void Validate_TestElementWithContentCountEquals(ULONG, typename Ktl::enable_if<!hasTestElementWithContentCount<T>::value>::type* = nullptr)
{
}

template <typename T>
void Validate_TestElementCountEquals(ULONG count, typename Ktl::enable_if<hasTestElementCount<T>::value>::type* = nullptr)
{
    KInvariant(T::TestElementCount == count);
}

template <typename T>
void Validate_TestElementCountEquals(ULONG, typename Ktl::enable_if<!hasTestElementCount<T>::value>::type* = nullptr)
{
}

template <typename T>
void Validate_TestElementCountGE(ULONG count, typename Ktl::enable_if<hasTestElementCount<T>::value>::type* = nullptr)
{
    KInvariant(T::TestElementCount >= count);
}

template <typename T>
void Validate_TestElementCountGE(ULONG, typename Ktl::enable_if<!hasTestElementCount<T>::value>::type* = nullptr)
{
}

namespace KTypeTagTest1
{
    class AnonymousType : public KObject<AnonymousType>
    {
    public:
        AnonymousType() {}
        ~AnonymousType() {}
    };

    const ULONGLONG BaseTag = K_MAKE_TYPE_TAG(' gaT', 'esaB');

    class Base : public KObject<Base>, public KTagBase<Base, BaseTag>
    {
    public:
        Base() {}
        ~Base() {}
    };

    const ULONGLONG Derived1Tag = K_MAKE_TYPE_TAG('   1', 'dvrD');

    class Derived1 : public Base, public KTagBase<Derived1, Derived1Tag>
    {
    public:
        Derived1() {}
        ~Derived1() {}
    };

    const ULONGLONG Derived2Tag = K_MAKE_TYPE_TAG('   2', 'dvrD');

    class Derived2 : public Derived1, public KTagBase<Derived2, Derived2Tag>
    {
    public:
        Derived2() {}
        ~Derived2() {}
    };

    class Derived3 : public Derived2
    {
    public:
        Derived3() {}
        ~Derived3() {}
    };

    const ULONGLONG Derived4Tag = K_MAKE_TYPE_TAG('   4', 'dvrD');

    class Derived4 : public Derived3, public KTagBase<Derived4, Derived4Tag>
    {
    public:
        Derived4() {}
        ~Derived4() {}
    };

    VOID Execute()
    {
        AnonymousType noNameObj;
        KInvariant(noNameObj.GetTypeTag() == KDefaultTypeTag);

        Base baseObj;
        Derived1 derived1Obj;
        Derived2 derived2Obj;
        Derived3 derived3Obj;
        Derived4 derived4Obj;

        Base* basePtr;

        basePtr = &baseObj;
        KInvariant(basePtr->GetTypeTag() == BaseTag);

        basePtr = &derived1Obj;
        KInvariant(basePtr->GetTypeTag()== Derived1Tag);

        basePtr = &derived2Obj;
        KInvariant(basePtr->GetTypeTag() == Derived2Tag);

        basePtr = &derived3Obj;
        KInvariant(basePtr->GetTypeTag() == Derived2Tag);

        basePtr = &derived4Obj;
        KInvariant(basePtr->GetTypeTag() == Derived4Tag);

        KTestPrintf("%s succeeded\n", __FUNCTION__);
    }
}

namespace KTypeTagTest2
{
    using ::_delete;

    const ULONGLONG Async1Tag = K_MAKE_TYPE_TAG('   1', 'nysA');
    class AsyncContext1 : public KAsyncContextBase, public KTagBase<AsyncContext1, Async1Tag>
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AsyncContext1);
    public:
        static
        KAsyncContextBase::SPtr Create()
        {
            return _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) AsyncContext1();
        }
    };

    AsyncContext1::AsyncContext1() {}
    AsyncContext1::~AsyncContext1() {}

    const ULONGLONG Async2Tag = K_MAKE_TYPE_TAG('   2', 'nysA');
    class AsyncContext2 : public KAsyncContextBase, public KTagBase<AsyncContext2, Async2Tag>
    {
        K_FORCE_SHARED(AsyncContext2);
    public:
        static
        KAsyncContextBase::SPtr Create()
        {
            return _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) AsyncContext2();
        }
    };

    AsyncContext2::AsyncContext2() {}
    AsyncContext2::~AsyncContext2() {}

    VOID Execute()
    {
        KAsyncContextBase::SPtr basePtr;

        basePtr = AsyncContext1::Create();
        KInvariant(basePtr->GetTypeTag() == Async1Tag);

        basePtr = AsyncContext2::Create();
        KInvariant(basePtr->GetTypeTag() == Async2Tag);

        KTestPrintf("%s succeeded\n", __FUNCTION__);
    }
}

namespace KShiftArrayTest
{
    struct TestStruct
    {
        ULONG Value;
    };

    VOID Execute(KtlSystem& System)
    {
        #define TEST_ARRAY_SIZE     (4)

        KShiftArray<TestStruct, TEST_ARRAY_SIZE> shiftArray(System.NonPagedAllocator());

        TestStruct testObj;

        ULONG count = TEST_ARRAY_SIZE / 2;

        // Add some elements to the array but don't make the array full.
        for (ULONG i = 0; i < count; i++)
        {
            testObj.Value = i;
            shiftArray.Append(testObj);
        }

        KInvariant(shiftArray.Max() == TEST_ARRAY_SIZE);
        KInvariant(!shiftArray.IsEmpty());
        KInvariant(shiftArray.Count() == count);

        for (ULONG i = 0; i < count; i++)
        {
            testObj = shiftArray[i];
            KInvariant(testObj.Value == i);
        }

        // clear the array
        shiftArray.Clear();
        KInvariant(shiftArray.IsEmpty());
        KInvariant(shiftArray.Count() == 0);

        // Try to make the array shift
        count = TEST_ARRAY_SIZE * 2;

        for (ULONG i = 0; i < count; i++)
        {
            testObj.Value = i;
            shiftArray.Append(testObj);
        }

        KInvariant(shiftArray.Max() == TEST_ARRAY_SIZE);
        KInvariant(!shiftArray.IsEmpty());
        KInvariant(shiftArray.Count() == TEST_ARRAY_SIZE);

        for (ULONG i = 0; i < shiftArray.Count(); i++)
        {
            testObj = shiftArray[i];
            KInvariant(testObj.Value == count - TEST_ARRAY_SIZE + i);
        }

        KTestPrintf("%s succeeded\n", __FUNCTION__);
    }
}

namespace KtlSystemTest
{
    VOID
    Execute()
    {
        auto    test = [](KAllocator& testAllocator) -> void
        {
            ULONGLONG                   initialAllocationCount = testAllocator.GetAllocsRemaining();

            KtlSystemBase::DebugOutputKtlSystemBases();

            // Prove default instance ctor/dtor behavior
            {
                KGuid               testGuid;
                testGuid.CreateNew();

                KtlSystemBase*      testInst = _new(KTL_TAG_TEST, testAllocator) KtlSystemBase(testGuid);
                KtlSystemBase::DebugOutputKtlSystemBases();

                KInvariant(KtlSystemBase::TryFindKtlSystem(testGuid) == testInst);
                _delete(testInst);
                KInvariant(KtlSystemBase::TryFindKtlSystem(testGuid) == nullptr);
            }
            KInvariant(initialAllocationCount == testAllocator.GetAllocsRemaining());

            // Prove default instance Activation/Deactivation behavior
            {
                KGuid               testGuid;
                testGuid.CreateNew();

                KtlSystemBase*      testInst = _new(KTL_TAG_TEST, testAllocator) KtlSystemBase(testGuid);

                testInst->Activate(5000);
                KInvariant(testInst->Status() == STATUS_SUCCESS);

                UCHAR*              testAllocation = _newArray<UCHAR>(KTL_TAG_TEST, testInst->NonPagedAllocator(), 1024);
                _deleteArray(testAllocation);

                testAllocation = _newArray<UCHAR>(KTL_TAG_TEST, testInst->PagedAllocator(), 1024);
                // Comment out the following line to trigger a Deactivate timeout condition
                _deleteArray(testAllocation);

                testInst->Deactivate(5000);
                KInvariant(KtlSystemBase::TryFindKtlSystem(testGuid) == testInst);
                _delete(testInst);
                KInvariant(KtlSystemBase::TryFindKtlSystem(testGuid) == nullptr);
            }
            KInvariant(initialAllocationCount == testAllocator.GetAllocsRemaining());

            // Prove derived instance Activation/Deactivation behaviors
            {
                class MyKtlSystem : public KtlSystemBase
                {
                public:
                    MyKtlSystem(
                        GUID Id,
                        BOOLEAN& ActivateCalledIndicator,
                        BOOLEAN& DeactivateCalledIndicator,
                        BOOLEAN& TimeoutHandlerCalledIndicator)
                        :   KtlSystemBase(Id),
                            _ActivateCalledIndicator(ActivateCalledIndicator),
                            _DeactivateCalledIndicator(DeactivateCalledIndicator),
                            _TimeoutHandlerCalledIndicator(TimeoutHandlerCalledIndicator)
                    {
                        _ActivateCalledIndicator = FALSE;
                        _DeactivateCalledIndicator = FALSE;
                        _TimeoutHandlerCalledIndicator = FALSE;
                    }

                    //** Override activate/deactivate extension
                    NTSTATUS
                    StartSystemInstance(__in ULONG MaxActiveDurationInMsecs)
                    {
                        UNREFERENCED_PARAMETER(MaxActiveDurationInMsecs);

                        _ActivateCalledIndicator = TRUE;
                        return STATUS_SUCCESS;
                    }

                    NTSTATUS
                    StartSystemInstanceShutdown()
                    {
                        _DeactivateCalledIndicator = TRUE;
                        return STATUS_SUCCESS;
                    }

                    VOID
                    TimeoutDuringDeactivation()
                    {
                        _TimeoutHandlerCalledIndicator = TRUE;
                    }

                private:
                    BOOLEAN&        _ActivateCalledIndicator;
                    BOOLEAN&        _DeactivateCalledIndicator;
                    BOOLEAN&        _TimeoutHandlerCalledIndicator;
                };

                KGuid               testGuid;
                testGuid.CreateNew();

                BOOLEAN             activateExtensionCalled    = FALSE;
                BOOLEAN             deactivateExtensionCalled  = FALSE;
                BOOLEAN             timeoutHandlerCalled       = FALSE;

                // Prove basic extension works - no timeout on deactivate
                KtlSystemBase*      testInst = _new(KTL_TAG_TEST, testAllocator) MyKtlSystem(
                    testGuid,
                    activateExtensionCalled,
                    deactivateExtensionCalled,
                    timeoutHandlerCalled);

                testInst->Activate(5000);
                KInvariant(testInst->Status() == STATUS_SUCCESS);
                KInvariant(activateExtensionCalled);

                UCHAR*              testAllocation = _newArray<UCHAR>(KTL_TAG_TEST, testInst->NonPagedAllocator(), 1024);
                _deleteArray(testAllocation);
                testAllocation = nullptr;

                testAllocation = _newArray<UCHAR>(KTL_TAG_TEST, testInst->PagedAllocator(), 1024);
                _deleteArray(testAllocation);
                testAllocation = nullptr;

                testInst->Deactivate(5000);
                KInvariant(deactivateExtensionCalled);
                KInvariant(!timeoutHandlerCalled);
                KInvariant(testInst->Status() == STATUS_SUCCESS);

                KInvariant(KtlSystemBase::TryFindKtlSystem(testGuid) == testInst);
                _delete(testInst);
                KInvariant(KtlSystemBase::TryFindKtlSystem(testGuid) == nullptr);
                testInst = nullptr;

                // Prove basic extension works - with timeout on deactivate
                testInst = _new(KTL_TAG_TEST, testAllocator) MyKtlSystem(
                    testGuid,
                    activateExtensionCalled,
                    deactivateExtensionCalled,
                    timeoutHandlerCalled);

                testInst->Activate(5000);
                KInvariant(testInst->Status() == STATUS_SUCCESS);
                KInvariant(activateExtensionCalled);

                testAllocation = _newArray<UCHAR>(KTL_TAG_TEST, testInst->NonPagedAllocator(), 1024);
                _deleteArray(testAllocation);
                testAllocation = nullptr;

                testAllocation = _newArray<UCHAR>(KTL_TAG_TEST, testInst->PagedAllocator(), 1024);
                //_deleteArray(testAllocation);

                testInst->Deactivate(5000);
                KInvariant(deactivateExtensionCalled);
                KInvariant(timeoutHandlerCalled);
                KInvariant(testInst->Status() == STATUS_UNSUCCESSFUL);

                _deleteArray(testAllocation);
                testAllocation = nullptr;

                _delete(testInst);
                testInst = nullptr;
            }
            KInvariant(initialAllocationCount == testAllocator.GetAllocsRemaining());

            // Prove async dealloc work while instance being deactivated
            {
                KGuid               testGuid;
                testGuid.CreateNew();

                KtlSystemBase*      testInst = nullptr;

                //* KtlSystems can't be in paged memory if KTimers are created with them
                testInst = _new(KTL_TAG_TEST, testAllocator.GetKtlSystem().NonPagedAllocator()) KtlSystemBase(testGuid);

                testInst->Activate(5000);
                KInvariant(testInst->Status() == STATUS_SUCCESS);

                KTimer::SPtr        backgroundTimer;
                NTSTATUS            status = KTimer::Create(
                    backgroundTimer,
                    testInst->NonPagedAllocator(),
                    KTL_TAG_TEST);
                KInvariant(NT_SUCCESS(status));

                // Start the timer for 3 secs; and we'll allow deactivate to stall for up to 10 secs
                backgroundTimer->StartTimer(3000, nullptr, nullptr);        // Fire off a freestanding async op
                backgroundTimer.Reset();

                testInst->Deactivate(10000);
                KInvariant(testInst->Status() == STATUS_SUCCESS);
                _delete(testInst);
            }
            KInvariant(initialAllocationCount == testAllocator.GetAllocsRemaining());
        };

        test(KtlSystem::GlobalNonPagedAllocator());
        test(KtlSystem::GlobalPagedAllocator());
    }
}

namespace KtlSystemChildTest
{
    //
    // The root object in a child KTL system.
    // There is one such root object per child KTL system.
    // Its lifetime is longer than all other objects in the same child KTL system.
    // When this root object goes away, its child KTL system has no more use and should be cleaned up.
    //

    class ChildSystemRoot : public KShared<ChildSystemRoot>
    {
    public:

        ChildSystemRoot()
        {
        }

        ~ChildSystemRoot()
        {
            //
            // Kill its container child KTL system
            //
            KtlSystem& system = GetThisKtlSystem();

            KInvariant(system.GetTypeTag() == KtlSystemChild::ThisTypeTag);   // The container must be a KtlSystemChild
            KtlSystemChild& childSystem = static_cast<KtlSystemChild&>(system);

            childSystem.Shutdown(5000);
        }
    };

    VOID
    Execute(KtlSystem& System)
    {
        //
        // Create a parent KTL system
        //

        System;  //  Avoid Error C4100 (Unreferenced formal parameter) - for some reason the next line isn't preventing it
        KAllocator& testAllocator = System.GlobalNonPagedAllocator();

        KGuid parentGuid;
        parentGuid.CreateNew();

        KUniquePtr<KtlSystemBase> parentSystem = _new(KTL_TAG_TEST, testAllocator) KtlSystemBase(parentGuid);
        KAllocator& parentNonPagedAllocator = parentSystem->NonPagedAllocator();

        parentSystem->Activate(5000);
        KInvariant(parentSystem->Status() == STATUS_SUCCESS);

        parentSystem->SetStrictAllocationChecks(TRUE);

        //
        // Create a bunch of child KTL systems.
        // Each child system has one root object.
        // When the root object is gone, the child KTL system is no longer needed and should be killed.
        //

        const ULONG numberOfChildSystems = 3;
        KSharedPtr<ChildSystemRoot> childSystemRootObjectArray[numberOfChildSystems];

        for (ULONG i = 0; i < numberOfChildSystems; i++)
        {
            KtlSystemChildCleanup* cleanupObject = _new(KTL_TAG_TEST, parentNonPagedAllocator) KtlSystemChildCleanup(*parentSystem);
            KInvariant(cleanupObject);

            KGuid childGuid;
            childGuid.CreateNew();

            KtlSystemChild* childSystem = _new(KTL_TAG_TEST, parentNonPagedAllocator) KtlSystemChild(
                childGuid, KtlSystemChild::ShutdownCallback(cleanupObject, &KtlSystemChildCleanup::RunCleanup));
            KInvariant(childSystem);

            childSystem->Activate(5000);
            KInvariant(childSystem->Status() == STATUS_SUCCESS);

            childSystem->SetStrictAllocationChecks(TRUE);

            //
            // Allocate a root object for the child KTL system
            //
            KAllocator& childNonPagedAllocator = childSystem->NonPagedAllocator();
            childSystemRootObjectArray[i] = _new(KTL_TAG_TEST, childNonPagedAllocator) ChildSystemRoot();
        }

        for (ULONG i = 0; i < numberOfChildSystems; i++)
        {
            //
            // Release the ref count on the root objects. This will trigger the rundown of their container child KTL systems.
            //
            KInvariant(childSystemRootObjectArray[i]->IsUnique() == TRUE);
            childSystemRootObjectArray[i] = nullptr;
        }

        //
        // Shutdown the parent KTL system. This call blocks until all its child KTL systems are gone or it times out.
        //
        parentSystem->Deactivate(5000);
    }
}

namespace KAllocatorTest
{
    ULONG           S1Count = 0;
    ULONG           S2Count = 0;
    ULONG           TestObj1Count = 0;
    ULONG           SimpleStruct1Count = 0;

    class TestObj1 : public KObject<TestObj1>, public KShared<TestObj1>
    {
    public:
        typedef KSharedPtr<TestObj1>    SPtr;

        TestObj1(ULONG State)
            :   _State(State)
        {
            TestObj1Count++;
        }

        ~TestObj1();

    private:
        ULONG       _State;
    };

    TestObj1::~TestObj1()
    {
        _State = 25;
        TestObj1Count--;
    }

    VOID
    Execute(KtlSystem& System)
    {
        // Prove KAllocator::GetTotalAllocations() 
        #if KTL_USER_MODE
        #if DBG
        {
            GUID                sysId;
            KtlSystemBase       sys(sysId);
            KtlSysBaseNonPageAlignedAllocator<POOL_TYPE::NonPagedPool>  alloc1(&sys);

            for (int i = 0; i < 13; i++)
            {
                alloc1.Free(alloc1.Alloc(1017));
            }
            KInvariant(alloc1.GetAllocsRemaining() == 0);
            KInvariant(alloc1.GetTotalAllocations() == 13 * 1017);

            KtlSysBasePageAlignedAllocator  alloc2(&sys);

            for (int i = 0; i < 17; i++)
            {
                alloc2.Free(alloc2.Alloc(4096));
            }
            KInvariant(alloc2.GetAllocsRemaining() == 0);
            KInvariant(alloc2.GetTotalAllocations() == 17 * 4096);
        }
        #endif
        #endif

        ULONGLONG           initialAllocationCount = KAllocatorSupport::gs_AllocsRemaining;
        KAllocator&         allocator = System.NonPagedAllocator();
        ULONGLONG           initialNonPagedAllocationCount = allocator.GetAllocsRemaining();

        // Verify basic allocation/deallocation (_new/_delete) - includes ctor/dtor verification
        TestObj1*           testObj1 = _new(KTL_TAG_TEST + 2942, allocator) TestObj1(5);
        KAssert(testObj1 != nullptr);
        KAssert(TestObj1Count == 1);

        // Prove GetThisAllocationTag() works
        KInvariant(testObj1->GetThisAllocationTag() == (KTL_TAG_TEST + 2942));

        _delete(testObj1);
        KAssert(TestObj1Count == 0);
        KInvariant((KAllocatorSupport::gs_AllocsRemaining >= 0)
            && (static_cast<ULONGLONG>(KAllocatorSupport::gs_AllocsRemaining) == initialAllocationCount));

        void*               testObj1Void = _new(KTL_TAG_TEST, allocator) TestObj1(5);
        KAssert(TestObj1Count == 1);
        KAssert(testObj1Void != nullptr);

        _delete(testObj1Void);              // will not call dtor
        KAssert(TestObj1Count == 1);
        TestObj1Count = 0;
        KInvariant((KAllocatorSupport::gs_AllocsRemaining >= 0)
            && (static_cast<ULONGLONG>(KAllocatorSupport::gs_AllocsRemaining)  == initialAllocationCount));

        // Verify basic [] allocation/deallocation (_newArray/_delete[]) - both POD and objects with ctor/dtor
        TestObj1::SPtr*     testArray = _newArray<KSharedPtr<TestObj1>>(KTL_TAG_TEST, allocator, 25);      // TestObj1::SPtr[25];
        UCHAR*              testPodArray = _newArray<UCHAR>(KTL_TAG_TEST, allocator, 100, 10, 5);          // UCHAR[100][10][5];
        KAssert(testArray != nullptr);
        KAssert(testPodArray != nullptr);

        for (ULONG ix = 0; ix < 25; ix++)
        {
            KInvariant(testArray[ix].RawPtr() == nullptr);
        }

        _deleteArray(testArray);
        _deleteArray(testPodArray);
        KInvariant((KAllocatorSupport::gs_AllocsRemaining >= 0)
            && (static_cast<ULONGLONG>(KAllocatorSupport::gs_AllocsRemaining)  == initialAllocationCount));

        // Verify basic allocation/deallocation - objects with ctor/dtor and embedded objects w/ctor/dtor
        struct Str1
        {
            TestObj1::SPtr  t;
            UCHAR           x;
        };

        Str1*       testStr1 = _new(KTL_TAG_TEST, allocator) Str1();
        Str1*       testArray1 = _newArray<Str1>(KTL_TAG_TEST, allocator, 10);
        KAssert(testStr1 != nullptr);
        KAssert(testArray1 != nullptr);

        for (ULONG ix = 0; ix < 10; ix++)
        {
            KInvariant(testArray1[ix].t.RawPtr() == nullptr);
        }

        _deleteArray(testArray1);
        _delete(testStr1);
        KInvariant((KAllocatorSupport::gs_AllocsRemaining >= 0)
            && (static_cast<ULONGLONG>(KAllocatorSupport::gs_AllocsRemaining)  == initialAllocationCount));


        // Verify simple POD array allocation/deallocation using _new/_delete
        UCHAR*      charArray = _new(KTL_TAG_TEST, allocator) UCHAR[10];
        KAssert(charArray != nullptr);
        _delete(charArray);
        KInvariant((KAllocatorSupport::gs_AllocsRemaining >= 0)
            && (static_cast<ULONGLONG>(KAllocatorSupport::gs_AllocsRemaining)  == initialAllocationCount));

        // Verify basic allocation/deallocation of nested types w/ctors and dtors
        struct SimpleStruct1
        {
            SimpleStruct1()
            {
                _s1 = 0;
                SimpleStruct1Count++;
            }

            ~SimpleStruct1()
            {
                SimpleStruct1Count--;
            }

            ULONG       _s1;
        };

        struct SimpleStruct2 : public SimpleStruct1
        {
            ULONG               _s2;
            TestObj1::SPtr      _p;
        };

        SimpleStruct1*  simpleStruct1Array = _newArray<SimpleStruct1>(KTL_TAG_TEST, allocator, 10);    // SimpleStruct1[10];
        KAssert(simpleStruct1Array != nullptr);
        KInvariant(SimpleStruct1Count == 10);

        SimpleStruct2*  simpleStruct2Array = _newArray<SimpleStruct2>(KTL_TAG_TEST, allocator, 10);    // SimpleStruct2[10];
        KAssert(simpleStruct2Array != nullptr);
        KInvariant(SimpleStruct1Count == 20);

        _deleteArray(simpleStruct1Array);
        KInvariant(SimpleStruct1Count == 10);

        _deleteArray(simpleStruct2Array);
        KInvariant(SimpleStruct1Count == 0);
        KInvariant((KAllocatorSupport::gs_AllocsRemaining >= 0)
            && (static_cast<ULONGLONG>(KAllocatorSupport::gs_AllocsRemaining)  == initialAllocationCount));

        simpleStruct2Array = _newArray<SimpleStruct2>(KTL_TAG_TEST, allocator, 10, 20, 30);    // SimpleStruct2[10][20][30];
        KAssert(simpleStruct2Array != nullptr);
        KInvariant(SimpleStruct1Count == (10 * 20 * 30));

        _deleteArray(simpleStruct2Array);
        KInvariant(SimpleStruct1Count == 0);
        KInvariant((KAllocatorSupport::gs_AllocsRemaining >= 0)
            && (static_cast<ULONGLONG>(KAllocatorSupport::gs_AllocsRemaining)  == initialAllocationCount));

        // Prove raw contained array of large value type w/ctor but no dtor - allows use of _new/_delete because of no dtor
        KGuid*       guidArray = _new(KTL_TAG_TEST, allocator) KGuid[10];
        KAssert(guidArray != nullptr);
        _delete(guidArray);
        KInvariant((KAllocatorSupport::gs_AllocsRemaining >= 0)
            && (static_cast<ULONGLONG>(KAllocatorSupport::gs_AllocsRemaining)  == initialAllocationCount));

        // Prove array of large value type w/ctor but no dtor works ok w/_newArray/_deleteArray as well
        guidArray = _newArray<KGuid>(KTL_TAG_TEST, allocator, 10);
        KAssert(guidArray != nullptr);
        _deleteArray(guidArray);
        KInvariant((KAllocatorSupport::gs_AllocsRemaining >= 0)
            && (static_cast<ULONGLONG>(KAllocatorSupport::gs_AllocsRemaining)  == initialAllocationCount));

        // Prove nested objects w/virtual dtors work
        struct S1
        {
            S1()
            {
                _State = '-';
                S1Count++;
            }

            virtual void M1() {}

            virtual ~S1()
            {
                _State = '+';
                S1Count--;
            }

            UCHAR       _State;
        };

        struct S2 : public S1
        {
            void M1()
            {
                _S2State = '=';
            }

            S2()
            {
                _S2State = '-';
                S2Count++;
            }

            ~S2()
            {
                _S2State = '*';
                S2Count--;
            }

            UCHAR       _S2State;
        };

        // Test virtual dtor of S2 is called when dtor'ing as S1
        S1*         s2Test = _new(KTL_TAG_TEST, allocator) S2;
        KAssert(s2Test != nullptr);

        _delete(s2Test);
        KInvariant((KAllocatorSupport::gs_AllocsRemaining >= 0)
            && (static_cast<ULONGLONG>(KAllocatorSupport::gs_AllocsRemaining)  == initialAllocationCount));
        KInvariant(S1Count == 0);

        // Prove basic virtual dtor works for array of nested object w/virtual dtor
        // NOTE: it is not valid to use the standard delete[] (S1*) - it will fault as will _deleteArray(S1*) - in
        //       both cases an S2* must be deleted.
        S2*         s2TestArray = _newArray<S2>(KTL_TAG_TEST, allocator, 10);  // S2[10];
        KInvariant(S1Count == 10);
        KInvariant(S2Count == 10);
        KAssert(s2TestArray != nullptr);

        _deleteArray(s2TestArray);
        KInvariant(S2Count == 0);
        KInvariant(S1Count == 0);
        KInvariant((KAllocatorSupport::gs_AllocsRemaining >= 0)
            && (static_cast<ULONGLONG>(KAllocatorSupport::gs_AllocsRemaining)  == initialAllocationCount));

        // prove _delete and _deleteArray work with nullptr
        _delete((S2*)nullptr);
        _deleteArray((S2*)nullptr);

        KInvariant((KAllocatorSupport::gs_AllocsRemaining >= 0)
            && (static_cast<ULONGLONG>(KAllocatorSupport::gs_AllocsRemaining)  == initialAllocationCount));
        KInvariant(allocator.GetAllocsRemaining() == initialNonPagedAllocationCount);
    }
}

namespace KSharedBaseTest
{
    struct Type1 : public KShared<Type1>
    {
        typedef KSharedPtr<Type1> SPtr;

        Type1()
        {
            _InstanceId = InterlockedIncrement(&_NextInstanceId);
            InterlockedIncrement(&_InstanceCount);
            KTestPrintf("Creating Type1: %d\n", _InstanceId);
        }

        virtual ~Type1()
        {
            KTestPrintf("Destroying Type1: %d\n", _InstanceId);
            InterlockedDecrement(&_InstanceCount);
        }

        LONG GetThisRefCount()
        {
            return KSharedBase::_RefCount;
        }

        ULONG _InstanceId;
        static LONG _InstanceCount;
        static LONG _NextInstanceId;
    };

    LONG Type1::_InstanceCount = 0;
    LONG Type1::_NextInstanceId = 0;

    struct Type1Derived : public KObject<Type1Derived>, public Type1
    {
        typedef KSharedPtr<Type1Derived> SPtr;

        Type1Derived()
        {
            KTestPrintf("Creating Type1Derived: %d\n", _InstanceId);
        }

        ~Type1Derived()
        {
            KTestPrintf("Destroying Type1Derived: %d\n", _InstanceId);
        }

        ULONG _Value1;
    };

    struct Type2 : public KShared<Type2>
    {
        typedef KSharedPtr<Type2> SPtr;

        Type2()
        {
            _InstanceId = InterlockedIncrement(&_NextInstanceId);
            InterlockedIncrement(&_InstanceCount);
            KTestPrintf("Creating Type2: %d\n", _InstanceId);
        }

        virtual ~Type2()
        {
            KTestPrintf("Destroying Type2: %d\n", _InstanceId);
            InterlockedDecrement(&_InstanceCount);
        }

        LONG GetThisRefCount()
        {
            return KSharedBase::_RefCount;
        }

        ULONG _InstanceId;
        static LONG _InstanceCount;
        static LONG _NextInstanceId;
    };

    LONG Type2::_InstanceCount = 0;
    LONG Type2::_NextInstanceId = 0;

    struct Type2Derived : public KObject<Type2Derived>, public Type2
    {
        typedef KSharedPtr<Type2Derived> SPtr;

        Type2Derived()
        {
            KTestPrintf("Creating Type2Derived: %d\n", _InstanceId);
        }

        ~Type2Derived()
        {
            KTestPrintf("Destroying Type2Derived: %d\n", _InstanceId);
        }

        ULONG _Value1;
    };

    class MyAsync : public KAsyncContextBase
    {
        K_FORCE_SHARED(MyAsync);

    public:
        static MyAsync::SPtr Create(__in KAllocator& Allocator);

        ULONG _InstanceId;
        static LONG _InstanceCount;
        static LONG _NextInstanceId;
    };

    MyAsync::MyAsync()
    {
        _InstanceId = InterlockedIncrement(&_NextInstanceId);
        InterlockedIncrement(&_InstanceCount);
        KTestPrintf("Creating MyAsync: %d\n", _InstanceId);
    }

    MyAsync::~MyAsync()
    {
        KTestPrintf("Destroying MyAsync: %d\n", _InstanceId);
        InterlockedDecrement(&_InstanceCount);
    }

    MyAsync::SPtr MyAsync::Create(
        __in KAllocator& Allocator
        )
    {
        return _new(KTL_TAG_TEST, Allocator) MyAsync();
    }

    LONG MyAsync::_NextInstanceId = 0;
    LONG MyAsync::_InstanceCount = 0;

    VOID
    Execute(KtlSystem& System)
    {
        System;  //  Avoid Error C4100 (Unreferenced formal parameter) - for some reason the next line isn't preventing it
        KAllocator& testAllocator = System.GlobalNonPagedAllocator();

        KSharedBase::SPtr baseSPtr;

        Type1::SPtr type1Inst1 = _new(KTL_TAG_TEST, testAllocator) Type1();
        KInvariant(Type1::_InstanceCount == 1);

        //
        // Increase the ref count on type1Inst1.
        //

        Type1* rawType1Inst1 = type1Inst1.RawPtr();
        baseSPtr = rawType1Inst1;
        KInvariant(rawType1Inst1->GetThisRefCount() == 2);

        // Destroy the original SPTR.
        type1Inst1 = nullptr;
        KInvariant(rawType1Inst1->GetThisRefCount() == 1);

        //
        // Create another instance of Type1
        //
        Type1::SPtr type1Inst2 = _new(KTL_TAG_TEST, testAllocator) Type1();

        baseSPtr = type1Inst2.RawPtr();
        KInvariant(type1Inst2->GetThisRefCount() == 2);
        KInvariant(baseSPtr.RawPtr() == static_cast<KSharedBase*>(type1Inst2.RawPtr()));

        //
        // Create type2 instances then bind the same baseSPtr to type2.
        //

        Type2Derived::SPtr type2DerivedInst1 = _new(KTL_TAG_TEST, testAllocator) Type2Derived();

        baseSPtr = type2DerivedInst1.RawPtr();
        KInvariant(type2DerivedInst1->GetThisRefCount() == 2);
        KInvariant(baseSPtr.RawPtr() == static_cast<KSharedBase*>(type2DerivedInst1.RawPtr()));

        //
        // Because baseSPtr is now bound to type2 instances, all type1 instances should have been destroyed.
        //
        type1Inst2 = nullptr;
        KInvariant(Type1::_InstanceCount == 0);

        //
        // Test KAsyncContextBase derived types.
        //

        MyAsync::SPtr myAsyncInst1 = MyAsync::Create(testAllocator);
        baseSPtr = myAsyncInst1.RawPtr();
        KInvariant(baseSPtr.RawPtr() == static_cast<KSharedBase*>(myAsyncInst1.RawPtr()));

        type2DerivedInst1 = nullptr;
        KInvariant(Type2::_InstanceCount == 0);

        myAsyncInst1 = nullptr;
        baseSPtr = nullptr;
        KInvariant(MyAsync::_InstanceCount == 0);
    }


    class DClass : public KShared<DClass>
    {
        public:
            DClass(BOOLEAN* DestructorFlag);
            ~DClass();

            LONG GetRefCount()
            {
                return(_RefCount);
            }

        private:
            BOOLEAN* _DestructorFlag;
    };

    DClass::DClass(
        BOOLEAN* DestructorFlag
        )
    {
        _DestructorFlag = DestructorFlag;
    }

    DClass::~DClass()
    {
        *_DestructorFlag = TRUE;
    }


    VOID
    Execute2(KtlSystem& System)
    {
        System;  //  Avoid Error C4100 (Unreferenced formal parameter) - for some reason the next line isn't preventing it
        KAllocator& testAllocator = System.GlobalNonPagedAllocator();

        //
        // Case 1: Assign a SPtr to itself
        //
        DClass::SPtr d1;
        DClass::SPtr* d1Ptr;
        DClass* d1Raw;
        BOOLEAN destructorFlag = FALSE;

        d1 = _new(KTL_TAG_TEST, testAllocator) DClass(&destructorFlag);
        d1Ptr = &d1;
        d1Raw = (DClass*)d1.RawPtr();

        KTestPrintf("%d: DClass Ref: %d, IsDestructed: %s\n", __LINE__, d1Raw->GetRefCount(), destructorFlag ? "TRUE" : "FALSE");

        *d1Ptr = d1;

        KTestPrintf("%d: DClass Ref: %d, IsDestructed: %s\n", __LINE__, d1Raw->GetRefCount(), destructorFlag ? "TRUE" : "FALSE");
        KInvariant(! destructorFlag);

        //
        // Case 2: Reassign _Proxy to itself
        //
        KTestPrintf("%d: DClass Ref: %d, IsDestructed: %s\n", __LINE__, d1Raw->GetRefCount(), destructorFlag ? "TRUE" : "FALSE");
        d1 = d1Raw;
        KTestPrintf("%d: DClass Ref: %d, IsDestructed: %s\n", __LINE__, d1Raw->GetRefCount(), destructorFlag ? "TRUE" : "FALSE");
        KInvariant(! destructorFlag);

        //
        // Case 3: Ktl::Move
        //
        KTestPrintf("%d: DClass Ref: %d, IsDestructed: %s\n", __LINE__, d1Raw->GetRefCount(), destructorFlag ? "TRUE" : "FALSE");
        d1 = Ktl::Move(d1);
        KTestPrintf("%d: DClass Ref: %d, IsDestructed: %s\n", __LINE__, d1Raw->GetRefCount(), destructorFlag ? "TRUE" : "FALSE");
        KInvariant(! destructorFlag);
    }
}

namespace KSharedInterfaceTest
{
    using ::_delete;

    class TestIf
    {
        K_SHARED_INTERFACE(TestIf);

    public:
        virtual void TestIfInvoke() = 0;
    };

    class TestIfA
    {
        K_SHARED_INTERFACE(TestIfA);

    public:
        virtual void TestIfAInvoke() = 0;
    };

    class TestIf1 : public TestIf
    {
        K_SHARED_INTERFACE(TestIf1);

    public:
        virtual void TestIf1Invoke() = 0;
    };


    // Test KShared<> derivation imp of shared interfaces proved nested and multiple inheritance
    class TestImp : public KShared<TestImp>, public KObject<TestImp>, public TestIf1, public TestIfA
    {
        K_FORCE_SHARED(TestImp);
        K_SHARED_INTERFACE_IMP(TestIf);
        K_SHARED_INTERFACE_IMP(TestIf1);
        K_SHARED_INTERFACE_IMP(TestIfA);

    private:
        BOOLEAN&       _MyState;

    public:
        using KSharedBase::_RefCount;
        static TestImp::SPtr Create(KAllocator& Allocator, BOOLEAN& StateRef)
        {
            return TestImp::SPtr(_new(KTL_TAG_TEST, Allocator) TestImp(StateRef));
        }

        int         _TestIfInvokeCount;
        int         _TestIf1InvokeCount;
        int         _TestIfAInvokeCount;

        void TestIfInvoke() override
        {
            _TestIfInvokeCount++;
        }

        void TestIf1Invoke() override
        {
            _TestIf1InvokeCount++;
        }

        void TestIfAInvoke() override
        {
            _TestIfAInvokeCount++;
        }

    private:
        TestImp(BOOLEAN& StateRef);
    };

    TestImp::TestImp(BOOLEAN& StateRef)
        :   _MyState(StateRef)
    {
        _MyState = TRUE;
        _TestIfInvokeCount = 21;
        _TestIf1InvokeCount = 105;
        _TestIfAInvokeCount = 18;
    }

    TestImp::~TestImp()
    {
        _MyState = FALSE;
    }


    void Execute(KtlSystem& System)
    {
        UNREFERENCED_PARAMETER(System);

        BOOLEAN    outState = FALSE;

        TestImp::SPtr   tPtr = TestImp::Create(KtlSystem::GetDefaultKtlSystem().NonPagedAllocator(), outState);
        KInvariant(outState);
        KInvariant(tPtr->_RefCount == 1);

        // Prove SPtr assignment and proper ref-counting
        TestIf1::SPtr   i1Ptr = tPtr.RawPtr();
        KInvariant(tPtr->_RefCount == 2);
        TestIf::SPtr    iPtr = &(*i1Ptr);
        KInvariant(tPtr->_RefCount == 3);
        TestIfA::SPtr   aPtr = &(*tPtr);
        KInvariant(tPtr->_RefCount == 4);

        // Prove basic interface methods can be called and state manipulated correctly in the imp class
        iPtr->TestIfInvoke();
        i1Ptr->TestIf1Invoke();
        i1Ptr->TestIfInvoke();
        aPtr->TestIfAInvoke();
        KInvariant(tPtr->_TestIfInvokeCount == (2 + 21));
        KInvariant(tPtr->_TestIf1InvokeCount == (1 + 105));
        KInvariant(tPtr->_TestIfAInvokeCount == (1 + 18));

        // Prove correct AddRef/Release semantics via SPtr manipulation
        TestIfA*        rawRefCountedAPtr = aPtr.Detach();          // Take a raw ref counted ptr to an interface
        KInvariant(outState);
        KInvariant(tPtr->_RefCount == 4);
        KInvariant(aPtr.RawPtr() == nullptr);
        iPtr = nullptr;
        KInvariant(outState);
        KInvariant(tPtr->_RefCount == 3);
        i1Ptr = nullptr;

        KInvariant(outState);
        KInvariant(tPtr->_RefCount == 2);                              // tPtr and rawRefCountedAPtr
        aPtr.Attach(rawRefCountedAPtr);
            rawRefCountedAPtr = nullptr;
        KInvariant(tPtr->_RefCount == 2);                              // tPtr and aPtr

        aPtr = nullptr;
        KInvariant(outState);
        KInvariant(aPtr.RawPtr() == nullptr);
        KInvariant(tPtr->_RefCount == 1);                              // Only tPtr

        // Prove DTOR occurs
        tPtr = nullptr;
        KInvariant(!outState);
    }
}

namespace KConstSharedTest
{
    using ::_delete;

    class MyBaseClass;

    interface IMyInterface
    {
        K_SHARED_INTERFACE(IMyInterface);

    public:
        virtual int GetRefCount() const = 0;
        virtual MyBaseClass const* GetConstThis() const = 0;
        virtual MyBaseClass* GetNonConstThis() = 0;
    };

    class MyBaseClass : public KShared<MyBaseClass>, public IMyInterface
    {
        K_FORCE_SHARED_WITH_INHERITANCE(MyBaseClass);
        K_SHARED_INTERFACE_IMP(IMyInterface);

    public:
        MyBaseClass const* GetConstThis() const { return this; }
        MyBaseClass* GetNonConstThis() { return this; }

        int GetRefCount() const { return _RefCount; }

        static MyBaseClass::SPtr CreateNonConst(KAllocator& Alloc)
        {
            return _new(KTL_TAG_TEST, Alloc) MyBaseClass();
        }

        static MyBaseClass::CSPtr CreateConst(KAllocator& Alloc)
        {
            return _new(KTL_TAG_TEST, Alloc) MyBaseClass();
        }
    };

    MyBaseClass::MyBaseClass() {}
    MyBaseClass::~MyBaseClass() {}

    class MyDerivedClass : public MyBaseClass
    {
        K_FORCE_SHARED(MyDerivedClass);

    public:
        static MyDerivedClass::SPtr CreateNonConst(KAllocator& Alloc)
        {
            return _new(KTL_TAG_TEST, Alloc) MyDerivedClass();
        }

        static MyDerivedClass::CSPtr CreateConst(KAllocator& Alloc)
        {
            return _new(KTL_TAG_TEST, Alloc) MyDerivedClass();
        }
    };

    MyDerivedClass::MyDerivedClass() {}
    MyDerivedClass::~MyDerivedClass() {}

    //*** Test simple base class, derived class, and shared interface for non and const SPtrs
    void Execute(KtlSystem& System)
    {
        KAllocator&     alloc = System.NonPagedAllocator();

        // This is really a compile-time test to verify KSharedPtr<T const> support
        // It executes but should not fail at runtime... but...

        //* Base class (C)SPtr tests
        {
            // Prove typical factory pattern works for const and non-const forms
            MyBaseClass::SPtr   sptr1 = MyBaseClass::CreateNonConst(alloc);
            KInvariant(sptr1);

            MyBaseClass::CSPtr   csptr1 = MyBaseClass::CreateConst(alloc);
            KInvariant(csptr1);

            // Prove with casting from SPtr to CSPtr works and ref count stays correct
            KInvariant(sptr1->GetRefCount() == 1);
            MyBaseClass::CSPtr   csptr2 = (MyBaseClass::CSPtr&)sptr1;       // Prove cast to CSPtr &
            KInvariant((sptr1.RawPtr()) == (const_cast<MyBaseClass*>(csptr2.RawPtr())));
            KInvariant(sptr1->GetRefCount() == 2);

            // Prove with casting from CSPtr to SPtr works and ref count stays correct
            KInvariant(csptr1->GetRefCount() == 1);
            sptr1 = &const_cast<MyBaseClass&>(*csptr1);
            KInvariant(csptr1->GetRefCount() == 2);
            KInvariant((sptr1.RawPtr()) == (const_cast<MyBaseClass*>(csptr1.RawPtr())));

            // Prove access
            {
                MyBaseClass const*  cp1 = csptr1->GetConstThis();
                MyBaseClass*  p1 = sptr1->GetNonConstThis();
                KInvariant(cp1 != nullptr);
                KInvariant(p1 != nullptr);
            }

            //* Prove shared interface functionality
            {
                IMyInterface::SPtr   isptr1 = MyBaseClass::CreateNonConst(alloc).RawPtr();
                KInvariant(isptr1);

                IMyInterface::CSPtr   icsptr1 = MyBaseClass::CreateConst(alloc).RawPtr();
                KInvariant(icsptr1);

                // Prove with casting from SPtr to CSPtr works and ref count stays correct
                KInvariant(isptr1->GetRefCount() == 1);
                IMyInterface::CSPtr   icsptr2 = (IMyInterface::CSPtr&)isptr1;       // Prove cast to CSPtr &
                KInvariant((isptr1.RawPtr()) == (const_cast<IMyInterface*>(icsptr2.RawPtr())));
                KInvariant(isptr1->GetRefCount() == 2);

                // Prove with casting from CSPtr to SPtr works and ref count stays correct
                KInvariant(icsptr1->GetRefCount() == 1);
                isptr1 = &const_cast<IMyInterface&>(*icsptr1);
                KInvariant(icsptr1->GetRefCount() == 2);
                KInvariant((isptr1.RawPtr()) == (const_cast<IMyInterface*>(icsptr1.RawPtr())));

                // Prove access
                {
                    MyBaseClass const*  icp1 = icsptr1->GetConstThis();
                    KInvariant(icp1 != nullptr);

                    MyBaseClass*  ip1 = isptr1->GetNonConstThis();
                    KInvariant(ip1 != nullptr);
                }
            }
        }

        //* Derived class (C)SPtr tests
        {
            // Prove typical factory pattern works for const and non-const forms
            MyDerivedClass::SPtr   sptr1 = MyDerivedClass::CreateNonConst(alloc);
            KInvariant(sptr1);

            MyDerivedClass::CSPtr   csptr1 = MyDerivedClass::CreateConst(alloc);
            KInvariant(csptr1);

            // Prove with casting from SPtr to CSPtr works and ref count stays correct
            KInvariant(sptr1->GetRefCount() == 1);
            MyDerivedClass::CSPtr   csptr2 = (MyDerivedClass::CSPtr&)sptr1;       // Prove cast to CSPtr &
            KInvariant((sptr1.RawPtr()) == (const_cast<MyDerivedClass*>(csptr2.RawPtr())));
            KInvariant(sptr1->GetRefCount() == 2);

            // Prove with casting from CSPtr to SPtr works and ref count stays correct
            KInvariant(csptr1->GetRefCount() == 1);
            sptr1 = &const_cast<MyDerivedClass&>(*csptr1);
            KInvariant(csptr1->GetRefCount() == 2);
            KInvariant((sptr1.RawPtr()) == (const_cast<MyDerivedClass*>(csptr1.RawPtr())));

            // And base class behavior is still good
            {
                MyBaseClass::SPtr   sptr1a = MyDerivedClass::CreateNonConst(alloc).RawPtr();
                KInvariant(sptr1a);

                MyBaseClass::CSPtr   csptr1a = MyDerivedClass::CreateConst(alloc).RawPtr();
                KInvariant(csptr1a);

                // Prove with casting from SPtr to CSPtr works and ref count stays correct
                KInvariant(sptr1a->GetRefCount() == 1);
                MyBaseClass::CSPtr   csptr2a = (MyBaseClass::CSPtr&)sptr1a;       // Prove cast to CSPtr &
                KInvariant((sptr1a.RawPtr()) == (const_cast<MyBaseClass*>(csptr2a.RawPtr())));
                KInvariant(sptr1a->GetRefCount() == 2);

                // Prove with casting from CSPtr to SPtr works and ref count stays correct
                KInvariant(csptr1a->GetRefCount() == 1);
                sptr1a = &const_cast<MyBaseClass&>(*csptr1a);
                KInvariant(csptr1a->GetRefCount() == 2);
                KInvariant((sptr1a.RawPtr()) == (const_cast<MyBaseClass*>(csptr1a.RawPtr())));
            }
        }
    }
}

#if KTL_USER_MODE
namespace KReaderWriterSpinLockTest
{
    using ::_delete;

    struct TestThreadState
    {
        enum Cmd
        {
            NoOp,
            DoAcquireSharedLock,
            DoAcquireExclusiveLock,
        }           _Cmd;

        bool        _Waiting;
    };

    KReaderWriterSpinLock _g_RWSpinLock;
    KEvent              _g_CmdHoldEvent;
    KEvent              _g_DoneHoldEvent;
    volatile LONG       _g_ThreadsStartHeld = 0;
    volatile LONG       _g_ThreadsDoneHeld = 0;
    volatile LONG       _g_SharedAcquiredHeld = 0;
    volatile LONG       _g_ExclusiveAcquiredHeld = 0;
    bool                _g_EndRaceTest = false;

    void RacerThreadStart(void* SubjectStateBit)
    {
        TestThreadState&     myState = *((TestThreadState*)SubjectStateBit);

        while (!_g_EndRaceTest)
        {
            //* 1st We register this thread as waiting for a command
            InterlockedIncrement(&_g_ThreadsStartHeld);
            _g_CmdHoldEvent.WaitUntilSet();
            InterlockedDecrement(&_g_ThreadsStartHeld);

            switch (myState._Cmd)
            {
                case TestThreadState::Cmd::NoOp:
                    break;

                case TestThreadState::Cmd::DoAcquireSharedLock:
                {
                    InterlockedIncrement(&_g_SharedAcquiredHeld);
                    _g_RWSpinLock.AcquireShared();
                    InterlockedDecrement(&_g_SharedAcquiredHeld);
                    break;
                }

                case TestThreadState::Cmd::DoAcquireExclusiveLock:
                {
                    InterlockedIncrement(&_g_ExclusiveAcquiredHeld);
                    _g_RWSpinLock.AcquireExclusive();
                    InterlockedDecrement(&_g_ExclusiveAcquiredHeld);
                    break;
                }

                default:
                    KInvariant(false);
            }

            InterlockedIncrement(&_g_ThreadsDoneHeld);
            _g_DoneHoldEvent.WaitUntilSet();
            InterlockedDecrement(&_g_ThreadsDoneHeld);
        }
    }

    //*** Test that basic reader writer spinlock is correct
    void Execute(KtlSystem& System)
    {
        KAllocator&     alloc = System.NonPagedAllocator();
        TestThreadState threadsState[4];

        auto ClearAll = [&]() -> void
        {
            for (int ix = 0; ix < 4; ix++)
            {
                threadsState[ix]._Cmd = TestThreadState::Cmd::NoOp;
                threadsState[ix]._Waiting = false;
            }
        };

        auto AwakenAndWaitUntilAllActive = [&](KEvent& GateToSignal, volatile LONG& ValueRef, LONG UntilIncBy) -> void
        {
            ULONGLONG   stopTime = KNt::GetTickCount64() + 2000;

            while (ValueRef != UntilIncBy)
            {
                KNt::Sleep(0);
                KInvariant(KNt::GetTickCount64() < stopTime);
            }

            GateToSignal.SetEvent();

            while (ValueRef != 0)
            {
                KNt::Sleep(0);
                KInvariant(KNt::GetTickCount64() < stopTime);
            }

            GateToSignal.ResetEvent();
        };

        KThread::SPtr       thread0;
        KThread::SPtr       thread1;
        KThread::SPtr       thread2;
        KThread::SPtr       thread3;

        NTSTATUS            status;

        status = KThread::Create(RacerThreadStart, (void*)&threadsState[0], thread0, alloc, KTL_TAG_TEST);
        KInvariant(NT_SUCCESS(status));
        status = KThread::Create(RacerThreadStart, (void*)&threadsState[1], thread1, alloc, KTL_TAG_TEST);
        KInvariant(NT_SUCCESS(status));
        status = KThread::Create(RacerThreadStart, (void*)&threadsState[2], thread2, alloc, KTL_TAG_TEST);
        KInvariant(NT_SUCCESS(status));
        status = KThread::Create(RacerThreadStart, (void*)&threadsState[3], thread3, alloc, KTL_TAG_TEST);
        KInvariant(NT_SUCCESS(status));

        //** Prove the TryAcquire*() methods work
        KInvariant(_g_RWSpinLock.TryAcquireExclusive());
        KInvariant(!_g_RWSpinLock.TryAcquireExclusive());
        KInvariant(!_g_RWSpinLock.TryAcquireShared());
        _g_RWSpinLock.ReleaseExclusive();
        KInvariant(_g_RWSpinLock.TryAcquireShared());
        KInvariant(!_g_RWSpinLock.TryAcquireExclusive());
        KInvariant(_g_RWSpinLock.TryAcquireShared());
        _g_RWSpinLock.ReleaseShared();
        _g_RWSpinLock.ReleaseShared();
        KInvariant(_g_RWSpinLock.TryAcquireExclusive());
        _g_RWSpinLock.ReleaseExclusive();

        for (int i = 0; i < 1000000; i++)
        {
            // Generate random work roles to the threads
            ClearAll();

            // Make sure all reader and writer threads hold at the right place
            LONG        expectedShared = rand() & 0x03;         // rand(0-3)
            LONG        expectedExclusive = 4 - expectedShared;
            KInvariant((expectedShared + expectedExclusive) == 4);

            for (int ii = 0; ii < expectedShared; ii++)
            {
                threadsState[ii]._Cmd = TestThreadState::Cmd::DoAcquireSharedLock;
            }

            for (int ii = 0; ii < expectedExclusive; ii++)
            {
                threadsState[ii + expectedShared]._Cmd = TestThreadState::Cmd::DoAcquireExclusiveLock;
            }

            // Work assignments done - lock and let the workers go           
            KInvariant(_g_RWSpinLock.TryAcquireExclusive());
            AwakenAndWaitUntilAllActive(_g_CmdHoldEvent, _g_ThreadsStartHeld, 4);

            // wait for all threads to queue on lock
            ULONGLONG       stopTime = KNt::GetTickCount64() + 2000;
            while ((_g_SharedAcquiredHeld + _g_ExclusiveAcquiredHeld) != 4)
            {
                KInvariant(KNt::GetTickCount64() < stopTime);
                KNt::Sleep(0);
            }

            // All readers and writers held - let one through
            _g_RWSpinLock.ReleaseExclusive();

            stopTime = KNt::GetTickCount64() + 2000;
            while ((expectedShared + expectedExclusive) != 0)
            {
                if ((expectedShared + expectedExclusive) == (_g_SharedAcquiredHeld + _g_ExclusiveAcquiredHeld))
                {
                    // No progress
                    KInvariant(KNt::GetTickCount64() < stopTime);
                    KNt::Sleep(0);
                }
                else
                {
                    // Progress has been made
                    if (expectedShared != 0 && (expectedShared > _g_SharedAcquiredHeld))
                    {
                        // Have shared progress
                        expectedShared--;
                        _g_RWSpinLock.ReleaseShared();
                    }
                    else
                    {
                        KInvariant(expectedExclusive > _g_ExclusiveAcquiredHeld);

                        // We have Exclusive progress - must
                        expectedExclusive--;
                        _g_RWSpinLock.ReleaseExclusive();
                    }
                }
            }

            // All threads done - Let all threads cycle back to waiting for a cmd
            AwakenAndWaitUntilAllActive(_g_DoneHoldEvent, _g_ThreadsDoneHeld, 4);
        }

        // Cause all threads to terminate
        _g_EndRaceTest = true;
        ClearAll();
        AwakenAndWaitUntilAllActive(_g_CmdHoldEvent, _g_ThreadsStartHeld, 4);
        AwakenAndWaitUntilAllActive(_g_DoneHoldEvent, _g_ThreadsDoneHeld, 4);
    }
}
#endif

namespace KEnumeration
{
    using ::_delete;

    //*** Test common enumeration support
    void Execute(KtlSystem& System)
    {
        KAllocator&     alloc = System.NonPagedAllocator();

        //* Test KArray range-for support
        {
            KArray<int>     testArray(alloc);
            KArray<KSharedPtr<KSharedBase>>     testArray1(alloc);

            int     count = 0;
            testArray1.Append(KSharedPtr<KSharedBase>());
            for (auto x : testArray1)
            {
                count++;
                KInvariant(x.RawPtr() == nullptr);
            }
            KInvariant(count == 1);

            //* Prove empty default for-next support works - forward
            count = 0;
            for (auto x : testArray)
            {
                x;
                count++;
            }
            KInvariant(count == 0);

            //* Prove empty enum works - forward
            for (auto x : testArray.GetEnumerator())
            {
                x;
                count++;
            }
            KInvariant(count == 0);

            //* Prove empty enum works - backward
            for (auto x : testArray.GetEnumerator(testArray.Count(), -1, -1))
            {
                x;
                count++;
            }
            KInvariant(count == 0);

            //* Create and prove an array of 1000 items valued [1000..1]
            count = 1000;
            while (count > 0)
            {
                testArray.Append(count);
                count--;
            }
            KInvariant(count == 0);
            KInvariant(testArray.Count() == 1000);
            KInvariant(testArray[0] == 1000);
            KInvariant(testArray[testArray.Count() -1] == 1);

            // Prove simple enum works
            count = 0;
            for (auto x : testArray)
            {
                KInvariant(x == (1000 - count));
                KInvariant(testArray[count] == x);
                count++;
            }
            KInvariant(count == 1000);

            //* Prove in-place array element modification
            count = 0;
            for (int& x : testArray)
            {
                KInvariant(x == (1000 - count));
                KInvariant(&testArray[count] == &x);
                KInvariant(testArray[count] == x);
                x++;
                count++;
            }
            KInvariant(count == 1000);

            count = 0;
            for (int& x : testArray)
            {
                KInvariant(x == (1001 - count));
                KInvariant(&testArray[count] == &x);
                KInvariant(testArray[count] == x);
                count++;
            }
            KInvariant(count == 1000);

            // Prove default Enumerator behavior
            count = 0;
            for (int& x : testArray.GetEnumerator())
            {
                KInvariant(x == (1001 - count));
                KInvariant(&testArray[count] == &x);
                KInvariant(testArray[count] == x);
                count++;
            }
            KInvariant(count == 1000);

            // Prove range Enumerator behavior
            count = 10;
            for (int& x : testArray.GetEnumerator(10, 20))
            {
                KInvariant(x == (1001 - count));
                KInvariant(&testArray[count] == &x);
                KInvariant(testArray[count] == x);
                count++;
            }
            KInvariant(count == 20);

            // Prove range and scale Enumerator behavior
            count = 0;
            for (int& x : testArray.GetEnumerator(0, testArray.Count(), 2))
            {
                KInvariant(x == (1001 - count));
                KInvariant(&testArray[count] == &x);
                KInvariant(testArray[count] == x);
                count += 2;
            }
            KInvariant(count == 1000);

            count = 0;
            for (int& x : testArray.GetEnumerator(0, testArray.Count(), 3))
            {
                KInvariant(x == (1001 - count));
                KInvariant(&testArray[count] == &x);
                KInvariant(testArray[count] == x);
                count += 3;
            }
            KInvariant(count == 1002);

            // Prove reverse, range, and scale Enumerator behavior
            count = 999;
            for (int& x : testArray.GetEnumerator(testArray.Count() - 1, -1, -2))
            {
                KInvariant(x == (1001 - count));
                KInvariant(&testArray[count] == &x);
                KInvariant(testArray[count] == x);
                count -= 2;
            }
            KInvariant(count == -1);

            count = 999;
            for (int& x : testArray.GetEnumerator(testArray.Count() - 1, -1, -3))
            {
                KInvariant(x == (1001 - count));
                KInvariant(&testArray[count] == &x);
                KInvariant(testArray[count] == x);
                count -= 3;
            }
            KInvariant(count == -3);
        }
    }
}

namespace KtlTraceTests
{
#if KTL_USER_MODE
#define SOME_TEXT "The quick brown fox jumped over the lazy dog"

    VOID KtlTraceTestCallback(
        __in LPCWSTR TypeText,
        __in USHORT Level,
        __in LPCSTR Text
    )
    {
        KInvariant(Level == levelInfo);
        KInvariant(wcsncmp(TypeText, typeINFORMATIONAL, 512) == 0);
        KInvariant(strncmp(Text, SOME_TEXT, 512) == 0);
    }


    VOID KtlTraceTest()
    {
        RegisterKtlTraceCallback(KtlTraceTestCallback);

        KDbgPrintfInformational("%s", SOME_TEXT);


        RegisterKtlTraceCallback(nullptr);
    }
#endif
}

namespace KArrayBasicUnitTests
{
    using ::_delete;

    template<typename T>
    class FaultyAllocator : public KAllocator
    {
    public:

        FaultyAllocator(KAllocator& wrappedAllocator, T& shouldFailAllocation)
            : _shouldFailAllocation(shouldFailAllocation)
            , _wrappedAllocator(wrappedAllocator)

        {
        }

        PVOID Alloc(__in size_t Size) override
        {
            if (_shouldFailAllocation())
            {
                return nullptr;
            }

            return _wrappedAllocator.Alloc(Size);
        }

        PVOID AllocWithTag(__in size_t Size, __in ULONG Tag) override
        {
            if (_shouldFailAllocation())
            {
                return nullptr;
            }

            return _wrappedAllocator.AllocWithTag(Size, Tag);
        }

        VOID Free(__in PVOID Mem) override
        {
            _wrappedAllocator.Free(Mem);
        }
        KtlSystem& GetKtlSystem() override
        {
            return _wrappedAllocator.GetKtlSystem();
        }

        ULONGLONG GetAllocsRemaining() override
        {
            return _wrappedAllocator.GetAllocsRemaining();
        }

#if KTL_USER_MODE
#if DBG
        ULONGLONG GetTotalAllocations() override
        {
            return _wrappedAllocator.GetTotalAllocations();
        }
#endif
#endif

    private:

        T& _shouldFailAllocation;
        KAllocator& _wrappedAllocator;
    };

    struct BlittableStruct
    {
        KIs_BlittableType();

    public:

        BlittableStruct()
            : a((ULONG)rand())
            , b((char)rand())
        {
        }

        ~BlittableStruct()
        { 
            a = a + b + (ULONG)rand();
        }

    private:

        ULONG a;
        char b;
    };

    template <typename T>
    void Execute(KtlSystem& ktlSystem, T tA, T tB, T tC, T tD, T tDefault)
    {
        KAllocator& allocator = ktlSystem.PagedAllocator();
        NTSTATUS status;
        BOOLEAN res;

        // Default
        KArray<T> a(allocator);
        KInvariant(NT_SUCCESS(a.Status()));
        KInvariant(a.IsEmpty());
        KInvariant(a.Count() == 0);
        KInvariant(a.Max() != 0);
        status = a.Append(tA);
        KInvariant(NT_SUCCESS(status));
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 1);
        KInvariant(a.Max() >= 1);
        KInvariant(a[0] == tA);
        for (ULONG i = 1; i < a.Max(); i++)
        {
            KInvariant(a.Data()[i] == tDefault);
        }

        // Empty and non-expandable
        a = KArray<T>(allocator, 0, 0);
        KInvariant(NT_SUCCESS(a.Status()));
        KInvariant(a.IsEmpty());
        KInvariant(a.Count() == 0);
        KInvariant(a.Max() == 0);
        status = a.Append(tA);
        KInvariant(!NT_SUCCESS(status));
        KInvariant(a.IsEmpty());
        KInvariant(a.Count() == 0);
        KInvariant(a.Max() == 0);

        // Nonempty and non-expandable
        a = KArray<T>(allocator, 1, 1, 0);
        KInvariant(NT_SUCCESS(a.Status()));
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 1);
        KInvariant(a.Max() == 1);
        KInvariant(a[0] == tDefault);
        status = a.Append(tA);
        KInvariant(!NT_SUCCESS(status));
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 1);
        KInvariant(a.Max() == 1);
        KInvariant(a[0] == tDefault);

        // Empty, expandable, double on growth
        a = KArray<T>(allocator, 1, 100);
        KInvariant(NT_SUCCESS(a.Status()));
        KInvariant(a.IsEmpty());
        KInvariant(a.Count() == 0);
        KInvariant(a.Max() == 1);
        status = a.Append(tA);
        KInvariant(NT_SUCCESS(status));
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 1);
        KInvariant(a.Max() == 1);
        KInvariant(a[0] == tA);
        status = a.Append(tA);
        KInvariant(NT_SUCCESS(status));
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 2);
        KInvariant(a.Max() == 2);
        KInvariant(a[0] == tA);
        KInvariant(a[1] == tA);
        status = a.Append(tA);
        KInvariant(NT_SUCCESS(status));
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 3);
        KInvariant(a.Max() == 4);
        KInvariant(a[0] == tA);
        KInvariant(a[1] == tA);
        KInvariant(a[2] == tA);
        KInvariant(a.Data()[3] == tDefault);

        // Invalid (count > size)
        // Triggers assert rather than error status, so skip testing. todo: use a kinvariant callout to verify
        //a = KArray<T>(allocator, 1, 2, 1);
        //KInvariant(!NT_SUCCESS(a.Status()));

        // Minimum growth (+1 on growth)
        a = KArray<T>(allocator, 2, 1, 1);
        KInvariant(NT_SUCCESS(a.Status()));
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 1);
        KInvariant(a.Max() == 2);
        KInvariant(a[0] == tDefault);
        KInvariant(a.Data()[1] == tDefault);
        status = a.Append(tA);
        KInvariant(NT_SUCCESS(status));
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 2);
        KInvariant(a.Max() == 2);
        KInvariant(a[0] == tDefault);
        KInvariant(a[1] == tA);
        status = a.Append(tA);
        KInvariant(NT_SUCCESS(status));
        KInvariant(a.Count() == 3);
        KInvariant(a.Max() == 3);
        KInvariant(a[0] == tDefault);
        KInvariant(a[1] == tA);
        KInvariant(a[2] == tA);

        // Copy
        a = KArray<T>(allocator, 1, 1);
        KInvariant(NT_SUCCESS(a.Status()));
        KInvariant(a.IsEmpty());
        KInvariant(a.Count() == 0);
        KInvariant(a.Max() == 1);
        KArray<T> b(allocator, 1, 1, 0);
        KInvariant(NT_SUCCESS(b.Status()));
        KInvariant(!b.IsEmpty());
        KInvariant(b.Count() == 1);
        KInvariant(b.Max() == 1);
        KInvariant(b[0] == tDefault);
        a = b;
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 1);
        KInvariant(a.Max() == 1);
        KInvariant(a[0] == b[0]);
        KInvariant(tDefault == a[0]);
        KInvariant(a == a);
        KInvariant(b == b);
        KInvariant(a == b);
        KInvariant(b == a);
        KInvariant(!(a != b));
        KInvariant(!(b != a));

        // InsertAt 0
        a = KArray<T>(allocator, 2, 100);
        KInvariant(NT_SUCCESS(a.Status()));
        KInvariant(a.IsEmpty());
        KInvariant(a.Count() == 0);
        KInvariant(a.Max() == 2);
        status = a.Append(tA);
        KInvariant(NT_SUCCESS(status));
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 1);
        KInvariant(a.Max() == 2);
        KInvariant(a[0] == tA);
        status = a.InsertAt(0, tB);
        KInvariant(NT_SUCCESS(status));
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 2);
        KInvariant(a.Max() == 2);
        KInvariant(a[0] == tB);
        KInvariant(a[1] == tA);

        // InsertAt Count == append
        a = KArray<T>(allocator, 0, 100);
        KInvariant(NT_SUCCESS(a.Status()));
        KInvariant(a.IsEmpty());
        KInvariant(a.Count() == 0);
        KInvariant(a.Max() == 0);
        status = a.Append(tA);
        KInvariant(NT_SUCCESS(status));
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 1);
        KInvariant(a.Max() == 1);
        KInvariant(a[0] == tA);
        status = a.InsertAt(a.Count(), tB);
        KInvariant(NT_SUCCESS(status));
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 2);
        KInvariant(a.Max() == 2);
        KInvariant(a[0] == tA);
        KInvariant(a[1] == tB);
        // Insert in the middle
        status = a.InsertAt(1, tC);
        KInvariant(NT_SUCCESS(status));
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 3);
        KInvariant(a.Max() == 4);
        KInvariant(a[0] == tA);
        KInvariant(a[1] == tC);
        KInvariant(a[2] == tB);
        // Insert at the beginning
        status = a.InsertAt(0, tB);
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 4);
        KInvariant(a.Max() == 4);
        KInvariant(a[0] == tB);
        KInvariant(a[1] == tA);
        KInvariant(a[2] == tC);
        KInvariant(a[3] == tB);
        // Insert > count (fails)
        status = a.InsertAt(a.Count() + 1, tA);
        KInvariant(!NT_SUCCESS(status));
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 4);
        KInvariant(a.Max() == 4);
        KInvariant(a[0] == tB);
        KInvariant(a[1] == tA);
        KInvariant(a[2] == tC);
        KInvariant(a[3] == tB);
        status = a.InsertAt(ULONG_MAX, tA);
        KInvariant(!NT_SUCCESS(status));
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 4);
        KInvariant(a.Max() == 4);
        KInvariant(a[0] == tB);
        KInvariant(a[1] == tA);
        KInvariant(a[2] == tC);
        KInvariant(a[3] == tB);

        // Reserve and SetCount
        a = KArray<T>(allocator, 0, 100);
        KInvariant(NT_SUCCESS(a.Status()));
        KInvariant(a.IsEmpty());
        KInvariant(a.Count() == 0);
        KInvariant(a.Max() == 0);
        status = a.Reserve(3);
        KInvariant(NT_SUCCESS(status));
        KInvariant(a.IsEmpty());
        KInvariant(a.Count() == 0);
        KInvariant(a.Max() == 3);
        KInvariant(a.Data()[0] == tDefault);
        KInvariant(a.Data()[1] == tDefault);
        KInvariant(a.Data()[2] == tDefault);
        status = a.Append(tA);
        KInvariant(NT_SUCCESS(status));
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 1);
        KInvariant(a.Max() == 3);
        KInvariant(a[0] == tA);
        KInvariant(a.Data()[1] == tDefault);
        KInvariant(a.Data()[2] == tDefault);
        a.Data()[1] = tB;
        a.Data()[2] = tC;
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 1);
        KInvariant(a.Max() == 3);
        KInvariant(a[0] == tA);
        KInvariant(a.Data()[1] == tB);
        KInvariant(a.Data()[2] == tC);
        res = a.SetCount(4);
        KInvariant(!res);
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 1);
        KInvariant(a.Max() == 3);
        KInvariant(a[0] == tA);
        KInvariant(a.Data()[1] == tB);
        KInvariant(a.Data()[2] == tC);
        res = a.SetCount(ULONG_MAX);
        KInvariant(!res);
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 1);
        KInvariant(a.Max() == 3);
        KInvariant(a[0] == tA);
        KInvariant(a.Data()[1] == tB);
        KInvariant(a.Data()[2] == tC);
        res = a.SetCount(2);
        KInvariant(res);
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 2);
        KInvariant(a.Max() == 3);
        KInvariant(a[0] == tA);
        KInvariant(a[1] == tB);
        KInvariant(a.Data()[2] == tC);
        status = a.Append(tD);
        KInvariant(NT_SUCCESS(status));
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 3);
        KInvariant(a.Max() == 3);
        KInvariant(a[0] == tA);
        KInvariant(a[1] == tB);
        KInvariant(a[2] == tD);

        // skipping enumeration as it is covered by the KEnumeration test

        // Remove + RemoveRange(1 element)
        a = KArray<T>(allocator, 4, 3, 100);
        KInvariant(NT_SUCCESS(a.Status()));
        a[0] = tA;
        a[1] = tB;
        a[2] = tC;
        KInvariant(a.Data()[3] == tDefault);
        b = KArray<T>(a);
        KInvariant(NT_SUCCESS(b.Status()));
        res = a.Remove(3);
        KInvariant(!res);
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 3);
        KInvariant(a.Max() == 4);
        KInvariant(a[0] == tA);
        KInvariant(a[1] == tB);
        KInvariant(a[2] == tC);
        KInvariant(a.Data()[3] == tDefault);
        res = b.RemoveRange(3, 1);
        KInvariant(!res);
        KInvariant(b == a);
        res = a.Remove(1);
        KInvariant(res);
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 2);
        KInvariant(a.Max() == 4);
        KInvariant(a[0] == tA);
        KInvariant(a[1] == tC);
        KInvariant(a.Data()[2] == tDefault);
        KInvariant(a.Data()[3] == tDefault);
        res = b.RemoveRange(1, 1);
        KInvariant(res);
        KInvariant(a == b);
        a = KArray<T>(allocator, 4, 3, 100);
        KInvariant(NT_SUCCESS(a.Status()));
        a[0] = tA;
        a[1] = tB;
        a[2] = tC;
        KInvariant(a.Data()[3] == tDefault);
        b = KArray<T>(a);
        KInvariant(NT_SUCCESS(b.Status()));
        res = a.Remove(0);
        KInvariant(res);
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 2);
        KInvariant(a.Max() == 4);
        KInvariant(a[0] == tB);
        KInvariant(a[1] == tC);
        KInvariant(a.Data()[2] == tDefault);
        KInvariant(a.Data()[3] == tDefault);
        res = b.RemoveRange(0, 1);
        KInvariant(res);
        KInvariant(!(b != a)); // throw in a usage of the != as well
        a = KArray<T>(allocator, 4, 3, 100);
        KInvariant(NT_SUCCESS(a.Status()));
        a[0] = tA;
        a[1] = tB;
        a[2] = tC;
        KInvariant(a.Data()[3] == tDefault);
        b = KArray<T>(a);
        KInvariant(NT_SUCCESS(b.Status()));
        res = a.Remove(2);
        KInvariant(res);
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 2);
        KInvariant(a.Max() == 4);
        KInvariant(a[0] == tA);
        KInvariant(a[1] == tB);
        KInvariant(a.Data()[2] == tDefault);
        KInvariant(a.Data()[3] == tDefault);
        res = b.RemoveRange(2, 1);
        KInvariant(res);
        KInvariant(a == b);

        // Remove while iterating
        a = KArray<T>(allocator, 4, 3, 100);
        KInvariant(NT_SUCCESS(a.Status()));
        a[0] = tA;
        a[1] = tB;
        a[2] = tC;
        KInvariant(a.Data()[3] == tDefault);
        b = KArray<T>(a);
        KInvariant(NT_SUCCESS(b.Status()));
        res = TRUE;
        for (ULONG i = 0; i < a.Count(); i++)
        {
            if (i == 0 && res)
            {
                res = FALSE;
                a.Remove(i);
                b.RemoveRange(i, 1);
                i--;
            }
        }
        KInvariant(a.Count() == 2);
        KInvariant(a.Max() == 4);
        KInvariant(a[0] == tB);
        KInvariant(a[1] == tC);
        KInvariant(a.Data()[2] == tDefault);
        KInvariant(a.Data()[3] == tDefault);
        KInvariant(a == b);
        a = KArray<T>(allocator, 4, 3, 100);
        KInvariant(NT_SUCCESS(a.Status()));
        a[0] = tA;
        a[1] = tB;
        a[2] = tC;
        KInvariant(a.Data()[3] == tDefault);
        b = KArray<T>(a);
        KInvariant(NT_SUCCESS(b.Status()));
        res = TRUE;
        for (ULONG i = 0; i < a.Count(); i++)
        {
            if (i == 1 && res)
            {
                res = FALSE;
                a.Remove(i);
                b.RemoveRange(i, 1);
                i--;
            }
        }
        KInvariant(a.Count() == 2);
        KInvariant(a.Max() == 4);
        KInvariant(a[0] == tA);
        KInvariant(a[1] == tC);
        KInvariant(a.Data()[2] == tDefault);
        KInvariant(a.Data()[3] == tDefault);
        KInvariant(a == b);
        a = KArray<T>(allocator, 4, 3, 100);
        KInvariant(NT_SUCCESS(a.Status()));
        a[0] = tA;
        a[1] = tB;
        a[2] = tC;
        KInvariant(a.Data()[3] == tDefault);
        b = KArray<T>(a);
        KInvariant(NT_SUCCESS(b.Status()));
        res = TRUE;
        for (ULONG i = 0; i < a.Count(); i++)
        {
            if (i == 2 && res)
            {
                res = FALSE;
                a.Remove(i);
                b.RemoveRange(i, 1);
                i--;
            }
        }
        KInvariant(a.Count() == 2);
        KInvariant(a.Max() == 4);
        KInvariant(a[0] == tA);
        KInvariant(a[1] == tB);
        KInvariant(a.Data()[2] == tDefault);
        KInvariant(a.Data()[3] == tDefault);
        KInvariant(a == b);

        // RemoveRange
        a = KArray<T>(allocator, 5, 4, 100);
        KInvariant(NT_SUCCESS(a.Status()));
        a[0] = tA;
        a[1] = tB;
        a[2] = tC;
        a[3] = tD;
        KInvariant(a.Data()[4] == tDefault);
        b = KArray<T>(a);
        KInvariant(NT_SUCCESS(b.Status()));
        for (int i = 0; i < 2; i++)
        {
            res = a.Remove(4);
            KInvariant(!res);
        }
        KInvariant(a[0] == tA);
        KInvariant(a[1] == tB);
        KInvariant(a[2] == tC);
        KInvariant(a[3] == tD);
        KInvariant(a.Data()[4] == tDefault);
        res = b.RemoveRange(4, 2);
        KInvariant(!res);
        KInvariant(b == a);
        for (int i = 0; i < 2; i++)
        {
            res = a.Remove(4);
            KInvariant(!res);
        }
        KInvariant(a[0] == tA);
        KInvariant(a[1] == tB);
        KInvariant(a[2] == tC);
        KInvariant(a[3] == tD);
        KInvariant(a.Data()[4] == tDefault);
        res = b.RemoveRange(4, 2);
        KInvariant(!res);
        KInvariant(b == a);
        for (int i = 0; i < 2; i++)
        {
            res = a.Remove(ULONG_MAX);
            KInvariant(!res);
        }
        KInvariant(a[0] == tA);
        KInvariant(a[1] == tB);
        KInvariant(a[2] == tC);
        KInvariant(a[3] == tD);
        KInvariant(a.Data()[4] == tDefault);
        res = b.RemoveRange(ULONG_MAX, 2);
        KInvariant(!res);
        KInvariant(a == b);
        for (int i = 0; i < 2; i++)
        {
            res = a.Remove(1);
            KInvariant(res);
        }
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 2);
        KInvariant(a.Max() == 5);
        KInvariant(a[0] == tA);
        KInvariant(a[1] == tD);
        KInvariant(a.Data()[2] == tDefault);
        KInvariant(a.Data()[3] == tDefault);
        KInvariant(a.Data()[4] == tDefault);
        res = b.RemoveRange(1, 2);
        KInvariant(res);
        KInvariant(b == a);
        a = KArray<T>(allocator, 5, 4, 100);
        KInvariant(NT_SUCCESS(a.Status()));
        a[0] = tA;
        a[1] = tB;
        a[2] = tC;
        a[3] = tD;
        b = KArray<T>(a);
        KInvariant(NT_SUCCESS(b.Status()));
        for (int i = 0; i < 3; i++)
        {
            res = a.Remove(0);
            KInvariant(res);
        }
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 1);
        KInvariant(a.Max() == 5);
        KInvariant(a[0] == tD);
        KInvariant(a.Data()[1] == tDefault);
        KInvariant(a.Data()[2] == tDefault);
        KInvariant(a.Data()[3] == tDefault);
        KInvariant(a.Data()[4] == tDefault);
        res = b.RemoveRange(0, 3);
        KInvariant(res);
        KInvariant(b == a);
        a = KArray<T>(allocator, 5, 4, 100);
        KInvariant(NT_SUCCESS(a.Status()));
        a[0] = tA;
        a[1] = tB;
        a[2] = tC;
        a[3] = tD;
        b = KArray<T>(a);
        KInvariant(NT_SUCCESS(b.Status()));
        for (int i = 0; i < 2; i++)
        {
            res = a.Remove(2);
            KInvariant(res);
        }
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 2);
        KInvariant(a.Max() == 5);
        KInvariant(a[0] == tA);
        KInvariant(a[1] == tB);
        KInvariant(a.Data()[2] == tDefault);
        KInvariant(a.Data()[3] == tDefault);
        KInvariant(a.Data()[4] == tDefault);
        res = b.RemoveRange(2, 2);
        KInvariant(res);
        KInvariant(a == b);
        a = KArray<T>(allocator, 5, 4, 100);
        KInvariant(NT_SUCCESS(a.Status()));
        a[0] = tA;
        a[1] = tB;
        a[2] = tC;
        a[3] = tD;
        b = KArray<T>(a);
        KInvariant(NT_SUCCESS(b.Status()));
        for (int i = 0; i < 2; i++)
        {
            res = a.Remove(2);
            KInvariant(res);
        }
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 2);
        KInvariant(a.Max() == 5);
        KInvariant(a[0] == tA);
        KInvariant(a[1] == tB);
        KInvariant(a.Data()[2] == tDefault);
        KInvariant(a.Data()[3] == tDefault);
        KInvariant(a.Data()[4] == tDefault);
        res = b.RemoveRange(2, ULONG_MAX); // it will remove up to the end
        KInvariant(res);
        KInvariant(b == a);

        // Clear
        a = KArray<T>(allocator, 5, 4, 100);
        KInvariant(NT_SUCCESS(a.Status()));
        a[0] = tA;
        a[1] = tB;
        a[2] = tC;
        a[3] = tD;
        b = KArray<T>(a);
        KInvariant(NT_SUCCESS(b.Status()));
        a.Clear();
        KInvariant(a.IsEmpty());
        KInvariant(a.Count() == 0);
        KInvariant(a.Max() == 5);
        KInvariant(a.Data()[0] == tDefault);
        KInvariant(a.Data()[1] == tDefault);
        KInvariant(a.Data()[2] == tDefault);
        KInvariant(a.Data()[3] == tDefault);
        KInvariant(a.Data()[4] == tDefault);
        res = b.RemoveRange(0, b.Count());
        KInvariant(res);
        KInvariant(a == b);
        a.Clear();
        KInvariant(a.Count() == 0);
        KInvariant(a.Max() == 5);
        KInvariant(a == b);
        b.Clear();
        KInvariant(a == b);

        status = a.Append(tA); // append after clear
        KInvariant(NT_SUCCESS(status));
        KInvariant(!a.IsEmpty());
        KInvariant(a.Count() == 1);
        KInvariant(a.Max() == 5);
        KInvariant(a[0] == tA);
        KInvariant(a.Data()[1] == tDefault);
        KInvariant(a.Data()[2] == tDefault);
        KInvariant(a.Data()[3] == tDefault);
        KInvariant(a.Data()[4] == tDefault);
    }

    void DoFaultyAllocatorTests(KtlSystem& ktlSystem)
    {
        volatile bool shouldFail = false;
        auto shouldFailLambda = [&shouldFail]() { return shouldFail; };
        FaultyAllocator<decltype(shouldFailLambda)> faultyAllocator(ktlSystem.PagedAllocator(), shouldFailLambda);

        KArray<ULONG> a(faultyAllocator);
        KInvariant(NT_SUCCESS(a.Status()));

        shouldFail = true;
        a = KArray<ULONG>(faultyAllocator);
        shouldFail = false;
        KInvariant(!NT_SUCCESS(a.Status()));

        KArray<KString::SPtr> b(faultyAllocator);
        KInvariant(NT_SUCCESS(b.Status()));

        shouldFail = true;
        b = KArray<KString::SPtr>(faultyAllocator);
        shouldFail = false;
        KInvariant(!NT_SUCCESS(b.Status()));

        KArray<BlittableStruct> c(faultyAllocator);
        KInvariant(NT_SUCCESS(c.Status()));

        shouldFail = true;
        c = KArray<BlittableStruct>(faultyAllocator);
        shouldFail = false;
        KInvariant(!NT_SUCCESS(c.Status()));
    }

    void Execute(KtlSystem& ktlSystem)
    {
        NTSTATUS status;
        KAllocator& allocator = ktlSystem.PagedAllocator();

        KWString kWStringA(allocator, L"TestStringA");
        KInvariant(NT_SUCCESS(kWStringA.Status()));
        KWString kWStringB(allocator, L"BTestString");
        KInvariant(NT_SUCCESS(kWStringB.Status()));
        KWString kWStringC(allocator);
        KInvariant(NT_SUCCESS(kWStringC.Status()));
        KWString kWStringD(allocator, L"zzzzzzzzzzzzzzzzzzzzzzzzzzzz");
        KInvariant(NT_SUCCESS(kWStringD.Status()));

        KString::SPtr kStringA;
        status = KString::Create(kStringA, allocator, kWStringA.operator UNICODE_STRING &(), TRUE);
        KInvariant(NT_SUCCESS(status));
        KString::SPtr kStringB;
        status = KString::Create(kStringB, allocator, kWStringB.operator UNICODE_STRING &(), TRUE);
        KInvariant(NT_SUCCESS(status));
        KString::SPtr kStringC;
        status = KString::Create(kStringC, allocator, kWStringC.operator UNICODE_STRING &(), TRUE);
        KInvariant(NT_SUCCESS(status));
        KString::SPtr kStringD;
        status = KString::Create(kStringD, allocator, kWStringD.operator UNICODE_STRING &(), TRUE);
        KInvariant(NT_SUCCESS(status));

        KTestPrintf("KArray unit tests (UCHAR)\n");
        Execute<UCHAR>(ktlSystem, 1, 2, 0, UCHAR_MAX, UCHAR());                                         // Simple type
        KTestPrintf("KArray unit tests (ULONG)\n");
        Execute<ULONG>(ktlSystem, 1, 2, 0, ULONG_MAX, ULONG());                                         // Simple type
        KTestPrintf("KArray unit tests (ULONGLONG)\n");
        Execute<ULONGLONG>(ktlSystem, 1, 2, 0, ULONGLONG_MAX, ULONGLONG());                             // Simple type
        KTestPrintf("KArray unit tests (WCHAR)\n");
        Execute<WCHAR>(ktlSystem, 1, 2, 0, WCHAR_MAX, WCHAR());                                         // Simple type
        KTestPrintf("KArray unit tests (KWString)\n");
        Execute<KWString>(ktlSystem, kWStringA, kWStringB, kWStringC, kWStringD, KWString(allocator));  // KAllocatorRequired
        KTestPrintf("KArray unit tests (KString::SPtr)\n");
        Execute<KString::SPtr>(ktlSystem, kStringA, kStringB, kStringC, kStringD, KString::SPtr());     // SPtr

        KTestPrintf("KArray faulty allocator tests\n");
        DoFaultyAllocatorTests(ktlSystem);
    }
}

namespace KArrayBlit
{
    using ::_delete;

    // Test Element object types used for doing full accounting of how many total elements are
    // currently ctor and how many of those ctor'd objects have state - a _SimContent value
    // of MAXULONG indicates "no state". In the KArray<> interval [Count(), Size()] ctor'd 
    // elements exist but have no state for obvious reasons.
    //
    class ZerodefaultTestElement
    {
        KIs_BlittableType();
        KIs_BlittableType_DefaultIsZero();

    private:
        ULONGLONG       _SimContent;

    public:
        static ULONG    TestElementCount;

        ULONGLONG Value()
        {
            return (_SimContent == 0) ? MAXULONG : (_SimContent - 1);
        }

        ZerodefaultTestElement(ULONGLONG SimContent)
        {
            _SimContent = SimContent + 1;
            TestElementCount++;
        }

        ZerodefaultTestElement()
        {
            _SimContent = 0;
            TestElementCount++;
        }

        ZerodefaultTestElement(const ZerodefaultTestElement& Other)
        {
            _SimContent = Other._SimContent;
            TestElementCount++;
        }

        ZerodefaultTestElement(ZerodefaultTestElement&& Other)
        {
            _SimContent = Other._SimContent;
            TestElementCount++;
            Other._SimContent = 0;
        }

        ~ZerodefaultTestElement()
        {
            TestElementCount--;
            _SimContent = 0;
        }

        ZerodefaultTestElement& operator=(const ZerodefaultTestElement& Other)
        {
            _SimContent = Other._SimContent;

            return *this;
        }

        ZerodefaultTestElement& operator=(ZerodefaultTestElement&& Other)
        {
            _SimContent = Other._SimContent;
            Other._SimContent = 0;

            return *this;
        }
    };

    ULONG   ZerodefaultTestElement::TestElementCount = 0;

    class TestElement
    {
        KIs_BlittableType();
        //KIs_BlittableType_DefaultIsZero();

    private:
        ULONGLONG       _SimContent;

    public:
        static ULONG    TestElementCount;
        static ULONG    TestElementWithContentCount;

        ULONGLONG Value()
        {
            return (_SimContent == 0) ? MAXULONG : (_SimContent - 1);
        }

        TestElement(ULONGLONG SimContent)
        {
            _SimContent = SimContent + 1;
            TestElementCount++;
            if (_SimContent != 0)
            {
                TestElementWithContentCount++;
            }
        }

        TestElement()
        {
            _SimContent = 0;
            TestElementCount++;
        }

        TestElement(const TestElement& Other)
        {
            _SimContent = Other._SimContent;
            TestElementCount++;
            if (_SimContent != 0)
            {
                TestElementWithContentCount++;
            }
        }

        TestElement(TestElement&& Other)
        {
            _SimContent = Other._SimContent;
            TestElementCount++;
            if (Other._SimContent != 0)
            {
                TestElementWithContentCount--;
                Other._SimContent = 0;
            }
            if (_SimContent != 0)
            {
                TestElementWithContentCount++;
            }
        }

        ~TestElement()
        {
            TestElementCount--;
            if (_SimContent != 0)
            {
                _SimContent = 0;
                TestElementWithContentCount--;
            }
        }

        TestElement& operator=(const TestElement& Other)
        {
            if (_SimContent != 0)
            {
                TestElementWithContentCount--;
            }
            _SimContent = Other._SimContent;
            if (_SimContent != 0)
            {
                TestElementWithContentCount++;
            }

            return *this;
        }

        TestElement& operator=(TestElement&& Other)
        {
            if (_SimContent != 0)
            {
                TestElementWithContentCount--;
            }
            _SimContent = Other._SimContent;
            if (Other._SimContent != 0)
            {
                TestElementWithContentCount--;
                Other._SimContent = 0;
            }
            if (_SimContent != 0)
            {
                TestElementWithContentCount++;
            }

            return *this;
        }
    };

    ULONG   TestElement::TestElementCount = 0;
    ULONG   TestElement::TestElementWithContentCount = 0;

    class NonBlitTestElement
    {
        //KIs_BlittableType();
        //KIs_BlittableType_DefaultIsZero();

    private:
        ULONGLONG       _SimContent;

    public:
        static ULONG    TestElementCount;
        static ULONG    TestElementWithContentCount;

        ULONGLONG Value()
        {
            return (_SimContent == 0) ? MAXULONG : (_SimContent - 1);
        }

        NonBlitTestElement(ULONGLONG SimContent)
        {
            _SimContent = SimContent + 1;
            TestElementCount++;
            if (_SimContent != 0)
            {
                TestElementWithContentCount++;
            }
        }

        NonBlitTestElement()
        {
            _SimContent = 0;
            TestElementCount++;
        }

        NonBlitTestElement(const NonBlitTestElement& Other)
        {
            _SimContent = Other._SimContent;
            TestElementCount++;
            if (_SimContent != 0)
            {
                TestElementWithContentCount++;
            }
        }

        NonBlitTestElement(NonBlitTestElement&& Other)
        {
            _SimContent = Other._SimContent;
            TestElementCount++;
            if (Other._SimContent != 0)
            {
                TestElementWithContentCount--;
                Other._SimContent = 0;
            }
            if (_SimContent != 0)
            {
                TestElementWithContentCount++;
            }
        }

        ~NonBlitTestElement()
        {
            TestElementCount--;
            if (_SimContent != 0)
            {
                _SimContent = 0;
                TestElementWithContentCount--;
            }
        }

        NonBlitTestElement& operator=(const NonBlitTestElement& Other)
        {
            if (_SimContent != 0)
            {
                TestElementWithContentCount--;
            }
            _SimContent = Other._SimContent;
            if (_SimContent != 0)
            {
                TestElementWithContentCount++;
            }

            return *this;
        }

        NonBlitTestElement& operator=(NonBlitTestElement&& Other)
        {
            if (_SimContent != 0)
            {
                TestElementWithContentCount--;
            }
            _SimContent = Other._SimContent;
            if (Other._SimContent != 0)
            {
                TestElementWithContentCount--;
                Other._SimContent = 0;
            }
            if (_SimContent != 0)
            {
                TestElementWithContentCount++;
            }

            return *this;
        }
    };

    ULONG   NonBlitTestElement::TestElementCount = 0;
    ULONG   NonBlitTestElement::TestElementWithContentCount = 0;

    using KSharedTestObj = KSharedType<NonBlitTestElement>;

    template <class T>
    class TestKSharedPtr : public KSharedPtr<T>
    {
    public:
        static ULONG   TestElementCount;
        static ULONG   TestElementWithContentCount;

        NOFAIL TestKSharedPtr()
        {
            TestElementCount++;
        }

        NOFAIL TestKSharedPtr(__in_opt T* Obj)
        {
            TestElementCount++;
            *this = Obj;
        }

        NOFAIL TestKSharedPtr(__in TestKSharedPtr const& Src)
        {
            TestElementCount++;
            *this = Src;
        }

        NOFAIL TestKSharedPtr(__in TestKSharedPtr&& Src)
        {
            TestElementCount++;
            *this = Ktl::Move(Src);
        }

        ~TestKSharedPtr()
        {
            TestElementCount--;
            if (RawPtr() != nullptr)
            {
                TestElementWithContentCount--;
            }
        }

        NOFAIL TestKSharedPtr& operator=(__in TestKSharedPtr const & Src)
        {
            if (RawPtr() != nullptr)
            {
                TestElementWithContentCount--;
            }
            if (Src.RawPtr() != nullptr)
            {
                TestElementWithContentCount++;
            }
            *((KSharedPtr<T>*)this) = Src;

            return *this;
        }

        NOFAIL TestKSharedPtr& operator=(__in TestKSharedPtr&& Src)
        {
            if (RawPtr() != nullptr)
            {
                TestElementWithContentCount--;
            }
            *((KSharedPtr<T>*)this) = Ktl::Move(Src);

            return *this;
        }

        NOFAIL TestKSharedPtr& operator=(__in_opt T* Src)
        {
            if (RawPtr() != nullptr)
            {
                TestElementWithContentCount--;
            }
            if (Src != nullptr)
            {
                TestElementWithContentCount++;
            }
            *((KSharedPtr<T>*)this) = Src;

            return *this;
        }

        T& operator*() const { return __super::operator*(); }

        T* operator->() const { return __super::operator->(); }

        T* RawPtr() const { return __super::RawPtr(); }

        BOOLEAN Reset() 
        {
            if (RawPtr() != nullptr)
            {
                TestElementWithContentCount--;
            }
            return __super::Reset(); 
        }

        T* Detach() 
        {
            if (RawPtr() != nullptr)
            {
                TestElementWithContentCount--;
            }
            return __super::Detach(); 
        }

        void Attach(__in T* Src)
        {
            if (RawPtr() != nullptr)
            {
                TestElementWithContentCount--;
            }
            if (Src != nullptr)
            {
                TestElementWithContentCount++;
            }
            __super::Attach(Src);
        }

        BOOLEAN operator ==(__in TestKSharedPtr const & Comparand) const
        {
            return __super::operator==(Comparand);
        }

        BOOLEAN operator ==(__in_opt T* const Comparand) const
        {
            return __super::operator==(Comparand);
        }

        BOOLEAN operator !=(__in TestKSharedPtr const & Comparand) const
        {
            return __super::operator!=(Comparand);
        }

        BOOLEAN operator !=(__in_opt T* const Comparand) const
        {
            return __super::operator!=(Comparand);
        }

        operator BOOLEAN() const { return __super::operator BOOLEAN(); }
    };

    template <>
    ULONG TestKSharedPtr<KSharedTestObj>::TestElementCount = 0;

    template <>
    ULONG TestKSharedPtr<KSharedTestObj>::TestElementWithContentCount = 0;

    template<typename TTestElement>
    void TestElementExercise(KAllocator& Alloc, const ULONG NumberOfTestElements)
    {
        KArray<TTestElement>    testArray1(Alloc);

        KInvariant(TTestElement::TestElementCount == testArray1.Max());

        for (ULONG ix = 0; ix < NumberOfTestElements; ix++)
        {
            KInvariant(NT_SUCCESS(testArray1.InsertAt(0, TTestElement(ix))));
            Validate_TestElementWithContentCountEquals<TTestElement>(testArray1.Count());
        }

        //* Prove all elements are present through the physical array and that only Count() 
        //  elements have state
        KInvariant(testArray1.Max() >= testArray1.Count());
        KInvariant(TTestElement::TestElementCount >= NumberOfTestElements);
        KInvariant(TTestElement::TestElementCount == testArray1.Max());
        Validate_TestElementWithContentCountEquals<TTestElement>(NumberOfTestElements);

        // Make sure every element is exactly what it should be
        TTestElement* arrayBase = testArray1.Data();
        for (ULONG ix = 0; ix < testArray1.Count(); ix++)
        {
            KInvariant(arrayBase[ix].Value() == (ULONGLONG)testArray1.Count() - ix - 1);
        }
        for (ULONG ix = testArray1.Count(); ix < testArray1.Max(); ix++)
        {
            KInvariant(arrayBase[ix].Value() == MAXULONG);
        }
        
        for (ULONG ix = 0; ix < NumberOfTestElements; ix++)
        {
            KInvariant(testArray1[ix].Value() == (ULONGLONG)(NumberOfTestElements - 1) - ix);
        }

        KInvariant(NT_SUCCESS(testArray1.InsertAt(NumberOfTestElements, TTestElement(NumberOfTestElements))));
        KInvariant(testArray1[NumberOfTestElements].Value() == NumberOfTestElements);
        KInvariant(NT_SUCCESS(testArray1.InsertAt(NumberOfTestElements / 2, TTestElement(NumberOfTestElements + 1))));
        KInvariant(testArray1[NumberOfTestElements / 2].Value() == (NumberOfTestElements + 1));
        KInvariant(testArray1[(NumberOfTestElements + 1)].Value() == NumberOfTestElements);

        //* Prove all elements are present through the physical array and that only Count() 
        //  elements have state
        KInvariant(testArray1.Max() >= testArray1.Count());
        KInvariant(TTestElement::TestElementCount >= NumberOfTestElements);
        KInvariant(TTestElement::TestElementCount == testArray1.Max());
        Validate_TestElementWithContentCountEquals<TTestElement>(NumberOfTestElements + 2);

        //KInvariant(testArray1.Remove(NumberOfTestElements / 2));
        KInvariant(testArray1.RemoveRange(NumberOfTestElements / 2, 1));
        KInvariant(TTestElement::TestElementCount == testArray1.Max());
        KInvariant(testArray1[NumberOfTestElements].Value() == NumberOfTestElements);
        // KInvariant(testArray1.Remove(NumberOfTestElements));
        KInvariant(testArray1.RemoveRange(NumberOfTestElements, 1));
        KInvariant(TTestElement::TestElementCount == testArray1.Max());
        KInvariant(testArray1[NumberOfTestElements - 1].Value() == 0);

        // Make sure every element is exactly what it should be
        TTestElement* rawArrayBase = testArray1.Data();
        for (ULONG ix = 0; ix < testArray1.Count(); ix++)
        {
            KInvariant(rawArrayBase[ix].Value() == (ULONGLONG)testArray1.Count() - ix - 1);
        }
        for (ULONG ix = testArray1.Count(); ix < testArray1.Max(); ix++)
        {
            KInvariant(rawArrayBase[ix].Value() == MAXULONG);
        }

        for (ULONG ix = 0; ix < NumberOfTestElements; ix++)
        {
            KInvariant(testArray1[ix].Value() == (ULONGLONG)(NumberOfTestElements - 1) - ix);
        }

        //* Prove all elements are present through the physical array and that only Count() 
        //  elements have state
        KInvariant(testArray1.Max() >= testArray1.Count());
        KInvariant(TTestElement::TestElementCount >= NumberOfTestElements);
        KInvariant(TTestElement::TestElementCount == testArray1.Max());
        Validate_TestElementWithContentCountEquals<TTestElement>(NumberOfTestElements);

        rawArrayBase = testArray1.Data();
        for (ULONG ix = 0; ix < NumberOfTestElements; ix++)
        {
            KInvariant(testArray1[ix].Value() == (ULONGLONG)(NumberOfTestElements - 1) - ix);
            KInvariant(rawArrayBase[ix].Value() == (ULONGLONG)(NumberOfTestElements - 1) - ix);
        }

        ULONGLONG       topValue = testArray1[0].Value();
        while (testArray1.Count() > 0)
        {
            KInvariant(testArray1[0].Value() == topValue);
            Validate_TestElementWithContentCountEquals<TTestElement>(testArray1.Count());

            int     todo = __min(testArray1.Count(), 7);
            KInvariant(testArray1.Remove(0));
            Validate_TestElementWithContentCountEquals<TTestElement>(testArray1.Count());
 
            if (todo > 1)
            {
                KInvariant(testArray1.RemoveRange(0, todo-1));
                Validate_TestElementWithContentCountEquals<TTestElement>(testArray1.Count());
            }

            // Make sure every element is exactly what it should be
            rawArrayBase = testArray1.Data();
            for (ULONG ix = 0; ix < testArray1.Count(); ix++)
            {
                KInvariant(rawArrayBase[ix].Value() == (ULONGLONG)testArray1.Count() - ix - 1);
            }
            for (ULONG ix = testArray1.Count(); ix < testArray1.Max(); ix++)
            {
                KInvariant(rawArrayBase[ix].Value() == MAXULONG);
            }

            topValue -= todo;
        }

        //* Prove all elements are present through the physical array and that only Count() 
        //  elements have state
        KInvariant(testArray1.Max() >= testArray1.Count());
        KInvariant(TTestElement::TestElementCount >= NumberOfTestElements);
        KInvariant(TTestElement::TestElementCount == testArray1.Max());
        Validate_TestElementWithContentCountEquals<TTestElement>(0);
    }

    // Prove non-blittable(debug) and blittable(retail) behaviors for KArray<SPtr>
    template<typename TTestKSharedPtr, int NbrOfTestObjs = 5>
    void TestBlittableSPtrs(KAllocator& Alloc, const ULONG ArraySize)
    {
        using TSPtr = TTestKSharedPtr;
        ULONG numberOfTestElements = ArraySize;

        TSPtr           testObjs[NbrOfTestObjs];
        LONG            testObjMirrorRefCounts[NbrOfTestObjs];
        for (int ix = 0; ix < NbrOfTestObjs; ix++)
        {
            testObjs[ix] = _new(KTL_TAG_TEST, Alloc) KSharedTestObj;
            testObjMirrorRefCounts[ix] = 1;
        }

        KArray<TSPtr>   testArray1(Alloc);

        auto TotalRefCounts = [&]() -> ULONG
        {
            ULONG       result = 0;
            ULONG       mirrorCount = 0;
            for (int ix = 0; ix < NbrOfTestObjs; ix++)
            {
                KInvariant(testObjs[ix]->GetRefCount() == testObjMirrorRefCounts[ix]);
                result += testObjs[ix]->GetRefCount();
                mirrorCount += testObjMirrorRefCounts[ix];
            }

            return result;
        };

        auto RandomTestObjIx = [&]() -> int
        {
            return rand() % NbrOfTestObjs;
        };

        auto GetTestObjIx = [&](KSharedTestObj* InQuestion) -> int
        {
            for (int ix = 0; ix < NbrOfTestObjs; ix++)
            {
                if (testObjs[ix].RawPtr() == InQuestion)
                {
                    return ix;
                }
            }
            KInvariant(false);
            return -1;
        };

        LONG        counts[NbrOfTestObjs];
        auto ValidateEachElement = [&]() -> void
        {
            for (LONG& v : counts)          // allow for testObjs rooted refs
            {
                v = 1;
            }

            // Make sure every element is exactly what it should be
            TSPtr* arrayBase = testArray1.Data();
            for (ULONG ix = 0; ix < testArray1.Count(); ix++)
            {
                int objIx = GetTestObjIx(arrayBase[ix].RawPtr());
                counts[objIx]++;
            }
            for (int ix = 0; ix < NbrOfTestObjs; ix++)
            {
                KInvariant(testObjs[ix]->GetRefCount() == counts[ix]);
                KInvariant(testObjMirrorRefCounts[ix] == counts[ix]);
            }
            for (ULONG ix = testArray1.Count(); ix < testArray1.Max(); ix++)
            {
                KInvariant(arrayBase[ix] == nullptr);
            }
        };


        // NOTE: All of the "+ NbrOfTestObjs" on the KInvariants below is to allow for testObjs refs
        KInvariant(TotalRefCounts() == NbrOfTestObjs);
        Validate_TestElementCountEquals<TSPtr>(testArray1.Max() + NbrOfTestObjs);
        ValidateEachElement();  // Make sure every element is exactly what it should be

        // Build up initial test array
        for (ULONG ix = 0; ix < numberOfTestElements; ix++)
        {
            ULONG testObjIx = RandomTestObjIx();
            KInvariant(NT_SUCCESS(testArray1.InsertAt(0, testObjs[testObjIx])));
            testObjMirrorRefCounts[testObjIx]++;
            Validate_TestElementWithContentCountEquals<TSPtr>(testArray1.Count() + NbrOfTestObjs);
        }
        KInvariant(TotalRefCounts() == (numberOfTestElements + NbrOfTestObjs));

        // Make sure every element is exactly what it should be
        ValidateEachElement();

        //* Prove all elements are present through the physical array and that only Count() 
        //  elements have state
        KInvariant(testArray1.Max() >= testArray1.Count());
        Validate_TestElementCountGE<TSPtr>(numberOfTestElements + NbrOfTestObjs);
        Validate_TestElementCountEquals<TSPtr>(testArray1.Max() + NbrOfTestObjs);

        Validate_TestElementWithContentCountEquals<TSPtr>(numberOfTestElements + NbrOfTestObjs);

        {
            ULONG testObjIx = RandomTestObjIx();
            KInvariant(NT_SUCCESS(testArray1.InsertAt(numberOfTestElements, testObjs[testObjIx])));
            testObjMirrorRefCounts[testObjIx]++;
            Validate_TestElementCountEquals<TSPtr>(testArray1.Max() + NbrOfTestObjs);
            ValidateEachElement();  // Make sure every element is exactly what it should be
            KInvariant(testArray1[numberOfTestElements] == testObjs[testObjIx]);
        }

        {
            KSharedTestObj* endingElementValue = testArray1[numberOfTestElements].RawPtr();
            KSharedTestObj* topShiftedElementValue = testArray1[numberOfTestElements / 2].RawPtr();
            {
                ULONG testObjIx = RandomTestObjIx();
                KInvariant(NT_SUCCESS(testArray1.InsertAt(numberOfTestElements / 2, testObjs[testObjIx])));
                testObjMirrorRefCounts[testObjIx]++;
                KInvariant(TotalRefCounts() == (numberOfTestElements + NbrOfTestObjs + 2));
                KInvariant(testArray1[numberOfTestElements / 2] == testObjs[testObjIx]);
                KInvariant(testArray1[numberOfTestElements + 1].RawPtr() == endingElementValue);
                KInvariant(testArray1[((numberOfTestElements / 2) + 1)].RawPtr() == topShiftedElementValue);
                Validate_TestElementCountEquals<TSPtr>(testArray1.Max() + NbrOfTestObjs);
                ValidateEachElement();      // Make sure every element is exactly what it should be
            }

            testObjMirrorRefCounts[GetTestObjIx(testArray1[numberOfTestElements / 2].RawPtr())]--;
            KInvariant(testArray1.RemoveRange(numberOfTestElements / 2, 1));
            ValidateEachElement();      // Make sure every element is exactly what it should be
            KInvariant(testArray1[numberOfTestElements].RawPtr() == endingElementValue);
            KInvariant(testArray1[numberOfTestElements / 2].RawPtr() == topShiftedElementValue);

            endingElementValue = testArray1[numberOfTestElements - 1].RawPtr();
            testObjMirrorRefCounts[GetTestObjIx(testArray1[numberOfTestElements].RawPtr())]--;
            KInvariant(testArray1.RemoveRange(numberOfTestElements, 1));
            ValidateEachElement();      // Make sure every element is exactly what it should be
            KInvariant(testArray1[numberOfTestElements - 1].RawPtr() == endingElementValue);
            KInvariant(testArray1.Data()[numberOfTestElements].RawPtr() == nullptr);
            Validate_TestElementCountEquals<TSPtr>(testArray1.Max() + NbrOfTestObjs);
        }

        TSPtr*      rawArrayBase = testArray1.Data();
        for (ULONG ix = 0; ix < numberOfTestElements; ix++)
        {
            KInvariant(testArray1[ix] == rawArrayBase[ix]);
        }

        //* Prove all elements are present through the physical array and that only Count() 
        //  elements have state
        KInvariant(testArray1.Max() >= testArray1.Count());
        Validate_TestElementCountGE<TSPtr>(numberOfTestElements + NbrOfTestObjs);
        Validate_TestElementCountEquals<TSPtr>(testArray1.Max() + NbrOfTestObjs);

        Validate_TestElementWithContentCountEquals<TSPtr>(numberOfTestElements + NbrOfTestObjs);

        while (testArray1.Count() > 0)
        {
            KInvariant(testArray1[0] != nullptr);

            Validate_TestElementWithContentCountEquals<TSPtr>(testArray1.Count() + NbrOfTestObjs);

            ULONG     todo = __min(testArray1.Count(), 7);
            KSharedTestObj* afterRemoved = (todo == testArray1.Count()) ? nullptr : testArray1[todo].RawPtr();

            // Account for the elements to be deleted in the double-check ref counts table 
            for (ULONG ix = 0; ix < todo; ix++)
            {
                testObjMirrorRefCounts[GetTestObjIx(testArray1[ix].RawPtr())]--;
            }

            KInvariant(testArray1.Remove(0));
            Validate_TestElementWithContentCountEquals<TSPtr>(testArray1.Count() + NbrOfTestObjs);

            if (todo > 1)
            {
                KInvariant(testArray1.RemoveRange(0, todo - 1));
                Validate_TestElementWithContentCountEquals<TSPtr>(testArray1.Count() + NbrOfTestObjs);
            }
            KInvariant(testArray1.Data()[0].RawPtr() == afterRemoved);

            ValidateEachElement();      // Make sure every element is exactly what it should be
        }
        KInvariant(TotalRefCounts() == NbrOfTestObjs);

        //* Prove all elements are present through the physical array and that only Count() 
        //  elements have state
        KInvariant(testArray1.Max() >= testArray1.Count());
        Validate_TestElementCountGE<TSPtr>(numberOfTestElements + NbrOfTestObjs);
        Validate_TestElementCountEquals<TSPtr>(testArray1.Max() + NbrOfTestObjs);
        Validate_TestElementWithContentCountEquals<TSPtr>(NbrOfTestObjs);
    }


    template<typename TTestElement>
    void TestRemoveRange001(KAllocator& Alloc, ULONG InitialMax, ULONG GrowthPrecent, ULONG FillCount, ULONG RemoveIndex, ULONG ForCountOf)
    {
        KArray<TTestElement>  testArray1(Alloc, InitialMax, GrowthPrecent);

        for (ULONG ix = 0; ix < FillCount; ix++)
        {
            testArray1.Append(TTestElement(ix));
        }
        KInvariant(testArray1.Max() == InitialMax);
        KInvariant(testArray1.Count() == FillCount);
        KInvariant(TTestElement::TestElementCount == InitialMax);
        Validate_TestElementWithContentCountEquals<TTestElement>(FillCount);

        ULONG countBefore = testArray1.Count();
        KInvariant(testArray1.RemoveRange(RemoveIndex, ForCountOf));
        KInvariant(testArray1.Max() == InitialMax);
        KInvariant(testArray1.Count() == (FillCount - __min(ForCountOf, (countBefore - RemoveIndex))));
        Validate_TestElementWithContentCountEquals<TTestElement>(FillCount - __min(ForCountOf, (countBefore - RemoveIndex)));

        for (ULONG ix = 0; ix < testArray1.Count(); ix++)
        {
            KInvariant(testArray1[ix].Value() != MAXULONG);
        }
        for (ULONG ix = testArray1.Count(); ix < testArray1.Max(); ix++)
        {
            KInvariant(testArray1.Data()[ix].Value() == MAXULONG);
        }
    }


    //*** Test KArray blittable element support
    void Execute(KtlSystem& System)
    {
        KAllocator&     alloc = System.NonPagedAllocator();
        const ULONG     numberOfTestElements = 10101;

        // Prove various limits for RemoveRange
        {
            // Test Sumukh's specific failure case
            {
                TestRemoveRange001<NonBlitTestElement>(alloc, 64, 0, 49, 0, 48);
                TestRemoveRange001<TestElement>(alloc, 64, 0, 49, 0, 48);
                TestRemoveRange001<ZerodefaultTestElement>(alloc, 64, 0, 49, 0, 48);
            }
            // Remove the last 1
            {
                TestRemoveRange001<NonBlitTestElement>(alloc, 64, 0, 49, 48, 1);
                TestRemoveRange001<TestElement>(alloc, 64, 0, 49, 48, 1);
                TestRemoveRange001<ZerodefaultTestElement>(alloc, 64, 0, 49, 48, 1);
            }
            // Remove the last 3
            {
                TestRemoveRange001<NonBlitTestElement>(alloc, 64, 0, 49, 46, 3);
                TestRemoveRange001<TestElement>(alloc, 64, 0, 49, 46, 3);
                TestRemoveRange001<ZerodefaultTestElement>(alloc, 64, 0, 49, 46, 3);
            }
            // Remove the 3 before the last
            {
                TestRemoveRange001<NonBlitTestElement>(alloc, 64, 0, 49, 45, 3);
                TestRemoveRange001<TestElement>(alloc, 64, 0, 49, 45, 3);
                TestRemoveRange001<ZerodefaultTestElement>(alloc, 64, 0, 49, 45, 3);
            }
            // Remove all
            {
                TestRemoveRange001<NonBlitTestElement>(alloc, 64, 0, 49, 0, 49);
                TestRemoveRange001<TestElement>(alloc, 64, 0, 49, 0, 49);
                TestRemoveRange001<ZerodefaultTestElement>(alloc, 64, 0, 49, 0, 49);
            }
            // Remove the first 3
            {
                TestRemoveRange001<NonBlitTestElement>(alloc, 64, 0, 49, 0, 3);
                TestRemoveRange001<TestElement>(alloc, 64, 0, 49, 0, 3);
                TestRemoveRange001<ZerodefaultTestElement>(alloc, 64, 0, 49, 0, 3);
            }
            // Remove the 3 after the first
            {
                TestRemoveRange001<NonBlitTestElement>(alloc, 64, 0, 49, 1, 3);
                TestRemoveRange001<TestElement>(alloc, 64, 0, 49, 1, 3);
                TestRemoveRange001<ZerodefaultTestElement>(alloc, 64, 0, 49, 1, 3);
            }

            // Try to Remove the last and some after that
            {
                TestRemoveRange001<NonBlitTestElement>(alloc, 16, 0, 4, 3, 4);
                TestRemoveRange001<TestElement>(alloc, 16, 0, 4, 3, 4);
                TestRemoveRange001<ZerodefaultTestElement>(alloc, 16, 0, 4, 3, 4);
            }

            // Try to Remove all
            {
                TestRemoveRange001<NonBlitTestElement>(alloc, 16, 0, 4, 0, 5);
                TestRemoveRange001<TestElement>(alloc, 16, 0, 4, 0, 5);
                TestRemoveRange001<ZerodefaultTestElement>(alloc, 16, 0, 4, 0, 5);
            }

            // Try to Remove middle 2 from 4
            {
                TestRemoveRange001<NonBlitTestElement>(alloc, 16, 0, 4, 1, 2);
                TestRemoveRange001<TestElement>(alloc, 16, 0, 4, 1, 2);
                TestRemoveRange001<ZerodefaultTestElement>(alloc, 16, 0, 4, 1, 2);
            }

            // Try to Remove last 2 from 4
            {
                TestRemoveRange001<NonBlitTestElement>(alloc, 16, 0, 4, 2, 2);
                TestRemoveRange001<TestElement>(alloc, 16, 0, 4, 2, 2);
                TestRemoveRange001<ZerodefaultTestElement>(alloc, 16, 0, 4, 2, 2);
            }
        }

        // Prove non-blittable behaviors
        {
            ULONGLONG startTime = KNt::GetTickCount64();
            TestElementExercise<NonBlitTestElement>(alloc, numberOfTestElements);
            ULONGLONG stopTime = KNt::GetTickCount64();
            KTestPrintf("Non-Blittable type test: %li ms\n", stopTime - startTime);
        }

        // Prove blittable move behaviors
        {
            ULONGLONG startTime = KNt::GetTickCount64();
            TestElementExercise<TestElement>(alloc, numberOfTestElements);
            ULONGLONG stopTime = KNt::GetTickCount64();
            KTestPrintf("Blittable move type test: %li ms\n", stopTime - startTime);
        }

        // Prove blittable move with zero-value default behaviors
        {
            ULONGLONG startTime = KNt::GetTickCount64();
            TestElementExercise<ZerodefaultTestElement>(alloc, numberOfTestElements);
            ULONGLONG stopTime = KNt::GetTickCount64();
            KTestPrintf("Blittable move with zero-value default type test: %li ms\n", stopTime - startTime);
        }

        #if DBG
        // Prove non-blittable behaviors for KArray<SPtr>
        {
            ULONGLONG startTime = KNt::GetTickCount64();
            TestBlittableSPtrs<TestKSharedPtr<KSharedTestObj>>(alloc, numberOfTestElements);
            ULONGLONG stopTime = KNt::GetTickCount64();
            KTestPrintf("NonBlittable: TestKSharedPtr<KSharedTestObj>(SPtr) type test: %li ms\n", stopTime - startTime);
        }

        // Prove blittable retail-only behaviors for KArray<SPtr>; 
        {
            ULONGLONG startTime = KNt::GetTickCount64();
            TestBlittableSPtrs<KSharedPtr<KSharedTestObj>, 5>(alloc, numberOfTestElements / 3);
            ULONGLONG stopTime = KNt::GetTickCount64();
            KTestPrintf("Blittable raw KArray<SPtr> type test: (5): %u elements: %li ms\n", numberOfTestElements / 3, stopTime - startTime);

            startTime = KNt::GetTickCount64();
            TestBlittableSPtrs<KSharedPtr<KSharedTestObj>, 13>(alloc, numberOfTestElements / 3);
            stopTime = KNt::GetTickCount64();
            KTestPrintf("Blittable raw KArray<SPtr> type test: (13): %u elements: %li ms\n", numberOfTestElements / 3, stopTime - startTime);

            startTime = KNt::GetTickCount64();
            TestBlittableSPtrs<KSharedPtr<KSharedTestObj>, 89>(alloc, numberOfTestElements /3 );
            stopTime = KNt::GetTickCount64();
            KTestPrintf("Blittable raw KArray<SPtr> type test: (89): %u elements: %li ms\n", numberOfTestElements / 3, stopTime - startTime);
        }

        #else
        // Prove blittable retail-only behaviors for KArray<SPtr>
        {
            ULONG       nbrOfElementsInRun = numberOfTestElements;
            for (int ix = 0; ix < 10; ix++)
            {
                ULONGLONG startTime = KNt::GetTickCount64();
                TestBlittableSPtrs<KSharedPtr<KSharedTestObj>, 5>(alloc, nbrOfElementsInRun);
                ULONGLONG stopTime = KNt::GetTickCount64();
                KTestPrintf("Blittable raw KArray<SPtr> type test (5): %u elements: %li ms\n", nbrOfElementsInRun, stopTime - startTime);

                startTime = KNt::GetTickCount64();
                TestBlittableSPtrs<KSharedPtr<KSharedTestObj>, 13>(alloc, nbrOfElementsInRun);
                stopTime = KNt::GetTickCount64();
                KTestPrintf("Blittable raw KArray<SPtr> type test (13): %u elements: %li ms\n", nbrOfElementsInRun, stopTime - startTime);

                startTime = KNt::GetTickCount64();
                TestBlittableSPtrs<KSharedPtr<KSharedTestObj>, 89>(alloc, nbrOfElementsInRun);
                stopTime = KNt::GetTickCount64();
                KTestPrintf("Blittable raw KArray<SPtr> type test (89): %u elements: %li ms\n", nbrOfElementsInRun, stopTime - startTime);
                nbrOfElementsInRun = (static_cast<ULONG>(rand()) % 32767) + 1;
            }
        }
        #endif
    }
}


namespace KSPtrBrokerTest
{
    using ::_delete;

    //*** Test KSPtrBroker<> support
    void Execute(KtlSystem& System)
    {
        KTestPrintf("KSPtrBrokerTest: Started\n");

        KAllocator&                 alloc = System.NonPagedAllocator();
        KSharedArray<int>::SPtr     testArray1 = _new(KTL_TAG_TEST, alloc) KSharedArray<int>();
        KSharedArray<int>::SPtr     testArray2 = _new(KTL_TAG_TEST, alloc) KSharedArray<int>();
        KInvariant(testArray1->GetRefCount() == 1);
        KInvariant(testArray2->GetRefCount() == 1);

        // Prove basic non-racing behavior of AtomicRef and KSPtrBroker is correct
        {
            KInvariant(KSPtrBroker<KSharedBase>::Test_AtomicRead128SpinCount == 0);
            KSPtrBroker<KSharedArray<int>>   testBroker;
            KSPtrBroker<KSharedBase>::AtomicRef a;
            a._Raw._Value[0] = 0x55;
            a._Raw._Value[1] = 0xAA;
            KSPtrBroker<KSharedBase>::AtomicRef b;
            b = a;
            KInvariant(KSPtrBroker<KSharedBase>::Test_AtomicRead128SpinCount == 0);

            KInvariant(KSPtrBroker<KSharedArray<int>>::Test_AtomicRead128SpinCount == 0);
            KInvariant(testBroker.Get() == nullptr);
            testBroker.Put(testArray1, 100);
            KInvariant(testArray1->GetRefCount() == 101);
            testBroker.Put(nullptr, 100);
            KInvariant(testArray1->GetRefCount() == 1);
            KInvariant(testBroker.Get() == nullptr);

            testBroker.Put(testArray1, 100);
            KInvariant(testArray1->GetRefCount() == 101);
            testBroker.Put(testArray2, 100);
            KInvariant(testArray2->GetRefCount() == 101);
            KInvariant(testArray1->GetRefCount() == 1);
            testBroker.Put(nullptr, 100);
            KInvariant(testArray1->GetRefCount() == 1);
            KInvariant(testArray2->GetRefCount() == 1);

            testBroker.Put(testArray1, 100);
            KInvariant(testArray1->GetRefCount() == 101);
            for (int ix = 0; ix < 98; ix++)
            {
                (testBroker.Get(100)) = nullptr;
            }
            KInvariant(testArray1->GetRefCount() == 3);
            (testBroker.Get(100)) = nullptr;
            KInvariant(testArray1->GetRefCount() == 102);
            KInvariant(KSPtrBroker<KSharedArray<int>>::Test_AtomicRead128SpinCount == 0);
        }
        
        //* Prove AtomicRead() will retry in light of racing writes modify the state being atomically read
        {
            KSPtrBroker<KSharedBase>::Test_AtomicRead128SpinCount = 0;

            auto WiProcessor = [](KSPtrBroker<KSharedBase>::AtomicRef* Param) -> void
            {
                KSPtrBroker<KSharedBase>::AtomicRef copy;

                do
                {
                    KSPtrBroker<KSharedBase>::AtomicRef::AtomicRead128(*Param, copy);
                } while (KSPtrBroker<KSharedBase>::Test_AtomicRead128SpinCount < 50);
            };

            KSPtrBroker<KSharedBase>::AtomicRef a;
            KThreadPool::ParameterizedWorkItem<KSPtrBroker<KSharedBase>::AtomicRef*> wi(&a, WiProcessor);

            System.DefaultSystemThreadPool().QueueWorkItem(wi, FALSE);

            while (KSPtrBroker<KSharedBase>::Test_AtomicRead128SpinCount < 50)
            {
                a._Raw._Value[0] = rand();
                a._Raw._Value[1] = rand();
            }
        }

        //* Prove racing Put()s will retry and settle with a correct value
        {
            KSPtrBroker<KSharedBase>::Test_PutSpinCount = 0;

            struct TestParams
            {
                KSPtrBroker<KSharedBase>*   _testBroker;
                KSharedArray<int>*          _testRefCountObj;
            } testParams;

            auto WiProcessor = [](TestParams* Param) -> void
            {
                do
                {
                    Param->_testBroker->Put(Param->_testRefCountObj, 100);
                } while (KSPtrBroker<KSharedBase>::Test_PutSpinCount < 50);
            };

            KSPtrBroker<KSharedBase> testBroker;
            KThreadPool::ParameterizedWorkItem<TestParams*> wi(&testParams, WiProcessor);

            testParams._testBroker = &testBroker;
            testParams._testRefCountObj = testArray1.RawPtr();

            System.DefaultSystemThreadPool().QueueWorkItem(wi, FALSE);

            while (KSPtrBroker<KSharedBase>::Test_PutSpinCount < 50)
            {
                testBroker.Put(testArray2.RawPtr(), 100);
            }
            KNt::Sleep(500);

            KSharedBase::SPtr result = testBroker.Get(100);
            KInvariant((result.RawPtr() == testArray1.RawPtr()) || (result.RawPtr() == testArray2.RawPtr()));
        }

        //* Prove racing Get()s will retry and settle with a correct value
        {
            KSPtrBroker<KSharedBase>::Test_GetHoldAllocateCount = 0;
            KSPtrBroker<KSharedBase>::Test_GetSpinCount = 0;

            struct TestParams
            {
                KSPtrBroker<KSharedBase>*   _testBroker;
                KSharedArray<int>*          _testRefCountObj;
            } testParams;

            auto WiProcessor = [](TestParams* Param) -> void
            {
                do
                {
                    Param->_testBroker->Get(2);
                    Param->_testBroker->Get(2);
                    Param->_testBroker->Get(2);
                } while ((KSPtrBroker<KSharedBase>::Test_GetHoldAllocateCount < 2) || (KSPtrBroker<KSharedBase>::Test_GetSpinCount < 2));
            };

            KSPtrBroker<KSharedBase> testBroker;
            KThreadPool::ParameterizedWorkItem<TestParams*> wi(&testParams, WiProcessor);
            KThreadPool::ParameterizedWorkItem<TestParams*> wi1(&testParams, WiProcessor);
            KThreadPool::ParameterizedWorkItem<TestParams*> wi2(&testParams, WiProcessor);

            testParams._testBroker = &testBroker;
            testParams._testRefCountObj = testArray1.RawPtr();
            testBroker.Put(testArray1.RawPtr(), 2);

            System.DefaultThreadPool().QueueWorkItem(wi, FALSE);
            System.DefaultThreadPool().QueueWorkItem(wi1, FALSE);
            System.DefaultThreadPool().QueueWorkItem(wi2, FALSE);

            while ((KSPtrBroker<KSharedBase>::Test_GetHoldAllocateCount < 2) || (KSPtrBroker<KSharedBase>::Test_GetSpinCount < 2))
            {
                KNt::Sleep(10);
            }
            KNt::Sleep(500);

            KInvariant(testBroker.Get(5).RawPtr() == testArray1.RawPtr());
        }

        KInvariant(testArray1->GetRefCount() == 1);
        KInvariant(testArray2->GetRefCount() == 1);

        //* Prove racing Get()s and Put()s will retry and settle with a correct value
        {
            KSPtrBroker<KSharedBase>::Test_GetHoldAllocateCount = 0;
            KSPtrBroker<KSharedBase>::Test_GetSpinCount = 0;
            KSPtrBroker<KSharedBase>::Test_GetBackoutCount = 0;
            KSPtrBroker<KSharedBase>::Test_PutSpinCount = 0;

            struct TestParams
            {
                KSPtrBroker<KSharedBase>*   _testBroker;
                KSharedArray<int>*          _testRefCountObj;
            } testParams;

            auto WiProcessor = [](TestParams* Param) -> void
            {
                do
                {
                    Param->_testBroker->Get(2);
                    Param->_testBroker->Put(Param->_testRefCountObj, 2);

                } while ((KSPtrBroker<KSharedBase>::Test_GetHoldAllocateCount < 10) || (KSPtrBroker<KSharedBase>::Test_GetSpinCount < 10)
                    || (KSPtrBroker<KSharedBase>::Test_GetBackoutCount < 10) || (KSPtrBroker<KSharedBase>::Test_PutSpinCount < 10));
            };

            KSPtrBroker<KSharedBase> testBroker;
            KThreadPool::ParameterizedWorkItem<TestParams*> wi(&testParams, WiProcessor);
            KThreadPool::ParameterizedWorkItem<TestParams*> wi1(&testParams, WiProcessor);
            KThreadPool::ParameterizedWorkItem<TestParams*> wi2(&testParams, WiProcessor);

            testParams._testBroker = &testBroker;
            testParams._testRefCountObj = testArray1.RawPtr();
            testBroker.Put(testArray1.RawPtr(), 2);

            System.DefaultSystemThreadPool().QueueWorkItem(wi, FALSE);
            System.DefaultSystemThreadPool().QueueWorkItem(wi1, FALSE);
            System.DefaultSystemThreadPool().QueueWorkItem(wi2, FALSE);

            while ((KSPtrBroker<KSharedBase>::Test_GetHoldAllocateCount < 10) || (KSPtrBroker<KSharedBase>::Test_GetSpinCount < 10)
                || (KSPtrBroker<KSharedBase>::Test_GetBackoutCount < 10) || (KSPtrBroker<KSharedBase>::Test_PutSpinCount < 10))
            {
                testBroker.Put(testArray1.RawPtr(), 2);
                testBroker.Put(testArray2.RawPtr(), 2);
                testBroker.Get(2);
            }
            KNt::Sleep(500);

            KInvariant((testBroker.Get(2).RawPtr() == testArray1.RawPtr()) || (testBroker.Get(2).RawPtr() == testArray2.RawPtr()));
        }

        KInvariant(testArray1->GetRefCount() == 1);
        KInvariant(testArray2->GetRefCount() == 1);

        KTestPrintf("KSPtrBrokerTest: Completed\n");
    }
}

#if KTL_USER_MODE
namespace KAllocatorPerfTest
{
    using ::_delete;

    volatile LONG       cpuCount;
    volatile LONGLONG   totalAllocCount = 0;
    static const ULONG  testDurationInMins = 3;
    
    //*** Test Partitioned allocation counters perf
    void Execute(KtlSystem& System)
    {
        KTestPrintf("KAllocatorPerfTest: Started\n");
        LONGLONG    baseAllocations = KAllocatorSupport::AllocsRemaining();
        KEvent      processorsDone;

        struct Params
        {
            KtlSystem* system;
            KEvent*    compEvent;
        } params;

        auto WiProcessor = [](Params* Params) -> void
        {
            KAllocator&     alloc = Params->system->NonPagedAllocator();
            LONGLONG        allocatedCount = 0;
            ULONGLONG       stopTime = KNt::GetTickCount64() + (testDurationInMins * 60 * 1000);

            while (KNt::GetTickCount64() < stopTime)
            {
                allocatedCount++;

                UCHAR*   blk = _newArray<UCHAR>(KTL_TAG_TEST, alloc, rand() + 1);
                _deleteArray(blk);
            }

            InterlockedAdd64(&totalAllocCount, allocatedCount);
            if (InterlockedDecrement(&cpuCount) == 0)
            {
                Params->compEvent->SetEvent();
            }
        };

        params.compEvent = &processorsDone;
        params.system = &System;

        KThreadPool::ParameterizedWorkItem<Params*> wi(&params, WiProcessor);

        // Perf test with both allocate counters turned on
        {
            KAllocatorSupport::gs_AllocCountsDisabled = false;
            System.SetEnableAllocationCounters(true);
            processorsDone.ResetEvent();
            totalAllocCount = 0;
            cpuCount = System.DefaultThreadPool().GetThreadCount();

            for (LONG c = 0; c < cpuCount; c++)
            {
                System.DefaultThreadPool().QueueWorkItem(wi, FALSE);
            }
            processorsDone.WaitUntilSet();

            double allocsPerSec = (double)totalAllocCount / (60 * testDurationInMins);
            KTestPrintf("KAllocatorPerfTest: Both Counters on: %f\n", allocsPerSec);
            LONGLONG leaks = KAllocatorSupport::AllocsRemaining() - baseAllocations;
            KInvariant(leaks == 0);
        }

        // Perf test with only global allocate counter turned on
        {
            KAllocatorSupport::gs_AllocCountsDisabled = false;
            System.SetEnableAllocationCounters(false);
            processorsDone.ResetEvent();
            totalAllocCount = 0;
            cpuCount = System.DefaultThreadPool().GetThreadCount();

            for (LONG c = 0; c < cpuCount; c++)
            {
                System.DefaultThreadPool().QueueWorkItem(wi, FALSE);
            }
            processorsDone.WaitUntilSet();

            double allocsPerSec = (double)totalAllocCount / (60 * testDurationInMins);
            KTestPrintf("KAllocatorPerfTest: Global Only Counter on: %f\n", allocsPerSec);
            LONGLONG leaks = KAllocatorSupport::AllocsRemaining() - baseAllocations;
            KInvariant(leaks == 0);
        }

        // Perf test with only allocators' counters turned on
        {
            KAllocatorSupport::gs_AllocCountsDisabled = true;
            System.SetEnableAllocationCounters(true);
            processorsDone.ResetEvent();
            totalAllocCount = 0;
            cpuCount = System.DefaultThreadPool().GetThreadCount();

            for (LONG c = 0; c < cpuCount; c++)
            {
                System.DefaultThreadPool().QueueWorkItem(wi, FALSE);
            }
            processorsDone.WaitUntilSet();

            double allocsPerSec = (double)totalAllocCount / (60 * testDurationInMins);
            KTestPrintf("KAllocatorPerfTest: Allocators' Only Counter on: %f\n", allocsPerSec);
            LONGLONG leaks = KAllocatorSupport::AllocsRemaining() - baseAllocations;
            KInvariant(leaks == 0);
        }

        // Perf test with both counters turned off
        {
            KAllocatorSupport::gs_AllocCountsDisabled = true;
            System.SetEnableAllocationCounters(false);
            processorsDone.ResetEvent();
            totalAllocCount = 0;
            cpuCount = System.DefaultThreadPool().GetThreadCount();

            for (LONG c = 0; c < cpuCount; c++)
            {
                System.DefaultThreadPool().QueueWorkItem(wi, FALSE);
            }
            processorsDone.WaitUntilSet();

            double allocsPerSec = (double)totalAllocCount / (60 * testDurationInMins);
            KTestPrintf("KAllocatorPerfTest: No Counters on: %f\n", allocsPerSec);
            LONGLONG leaks = KAllocatorSupport::AllocsRemaining() - baseAllocations;
            KInvariant(leaks == 0);
        }

        KAllocatorSupport::gs_AllocCountsDisabled = false;
        System.SetEnableAllocationCounters(true);
        KTestPrintf("KAllocatorPerfTest: Complete\n");
    }
}
#endif

NTSTATUS
KtlCommonTest(
    __in int argc,
    __in_ecount(argc) WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    EventRegisterMicrosoft_Windows_KTL();
    KTestPrintf("Starting KtlCommonTest test\n");

    KtlSystem* underlyingSystem;
    NTSTATUS status = KtlSystem::Initialize(&underlyingSystem);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("%s: %i: KtlSystem::Initialize failed\n", __FUNCTION__, __LINE__);
        return status;
    }

    #if KTL_USER_MODE
        KAllocatorPerfTest::Execute(*underlyingSystem);
    #endif
    
    KSPtrBrokerTest::Execute(*underlyingSystem);
    KArrayBasicUnitTests::Execute(*underlyingSystem);
    KArrayBlit::Execute(*underlyingSystem);
    KEnumeration::Execute(*underlyingSystem);

    #if KTL_USER_MODE
        KtlTraceTests::KtlTraceTest();
        KReaderWriterSpinLockTest::Execute(*underlyingSystem);
    #endif
    KConstSharedTest::Execute(*underlyingSystem);
    KtlSystemTest::Execute();
    KtlSystemChildTest::Execute(*underlyingSystem);

    KAllocatorTest::Execute(*underlyingSystem);
    KSharedInterfaceTest::Execute(*underlyingSystem);
    KTypeTagTest1::Execute();
    KTypeTagTest2::Execute();

    KShiftArrayTest::Execute(*underlyingSystem);

    KSharedBaseTest::Execute(*underlyingSystem);
    KSharedBaseTest::Execute2(*underlyingSystem);

    //
    // Turn on strict allocation tracking
    underlyingSystem->SetStrictAllocationChecks(TRUE);

    KtlSystem::Shutdown();

    EventUnregisterMicrosoft_Windows_KTL();
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
main(int argc, char* cargs[])
{
    KTestPrintf("KCommonTests: STARTED\n");

    CONVERT_TO_ARGS(argc, cargs)
#endif
    NTSTATUS status;

#if defined(PLATFORM_UNIX)
    status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif
	
    //
    // Adjust for the test name so CmdParseLine works.
    //

    if (argc > 0)
    {
        argc--;
        args++;
    }

    status = KtlCommonTest(argc, args);

    if (!NT_SUCCESS(status)) {
        KTestPrintf("Ktl Common tests failed Status = 0X%X\n", status);
        return RtlNtStatusToDosError(status);
    }

    status = KtlTextTest(argc, args);
    if (!NT_SUCCESS(status)) {
        KTestPrintf("Ktl Common tests failed Status = 0X%X\n", status);
        return RtlNtStatusToDosError(status);
    }

#if !defined(PLATFORM_UNIX)
    KTestPrintf("Starting KNt Memory Map test\n");
    status = KNtMemoryMapTest(argc, args);
    if (!NT_SUCCESS(status)) {
        KTestPrintf("KNt Memory Map tests failed Status = 0X%X\n", status);
        return RtlNtStatusToDosError(status);
    }
#endif
    
    KTestPrintf("KCommonTests: COMPLETED\n");

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif  
	
    return RtlNtStatusToDosError(status);
}
#endif

/*++
Copyright (c) Microsoft Corporation

Module Name:    KPriorityQueueTests.cpp
Abstract:   This file contains test case implementations for KPriorityQueue.
--*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include "KPriorityQueueTests.h"
#include <CommandLineParser.h>

#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif

void IsEmpty_EmptyPriorityQueue_True(__in KAllocator & allocator)
{
    KPriorityQueue<int> kPriorityQueue(allocator);
    KTestAssert::IsTrue(kPriorityQueue.Status());

    VerifyIsEmpty(kPriorityQueue);
}

void IsEmpty_NonEmpty_False(__in KAllocator & allocator)
{
    KPriorityQueue<int> kPriorityQueue(allocator);
    KTestAssert::IsTrue(kPriorityQueue.Status());

    NTSTATUS status = kPriorityQueue.Push(1);
    KInvariant(NT_SUCCESS(status));

    BOOLEAN isEmpty = kPriorityQueue.IsEmpty();
    KTestAssert::IsFalse(isEmpty);
}


void Pop_NonEmpty_False(__in KAllocator & allocator)
{
    KPriorityQueue<int> kPriorityQueue(allocator);
    KTestAssert::IsTrue(kPriorityQueue.Status());

    int item;
    BOOLEAN popStatus = kPriorityQueue.Pop(item);
    KTestAssert::IsFalse(popStatus);

    VerifyIsEmpty(kPriorityQueue);
}

void Pop_Emptied_False(__in KAllocator & allocator)
{
    int inputArray[10] =            { 0, 2, 0, 1, 0, 5, 4, 5, 1, 3 };
    int expectedOutputArray[10] =   { 0, 0, 0, 1, 1, 2, 3, 4, 5, 5 };

    KPriorityQueue<int> kPriorityQueue(allocator);
    KTestAssert::IsTrue(kPriorityQueue.Status());

    InsertItems<int>(inputArray, 0, 10, kPriorityQueue);
    RemoveItems<int>(expectedOutputArray, 0, 10, kPriorityQueue);

    int item;
    BOOLEAN popStatus = kPriorityQueue.Pop(item);
    KTestAssert::IsFalse(popStatus);

    VerifyIsEmpty(kPriorityQueue);
}


void Peek_NonEmpty_False(__in KAllocator & allocator)
{
    KPriorityQueue<int> kPriorityQueue(allocator);
    KTestAssert::IsTrue(kPriorityQueue.Status());

    int item;
    BOOLEAN popStatus = kPriorityQueue.Peek(item);
    KTestAssert::IsFalse(popStatus);

    VerifyIsEmpty(kPriorityQueue);
}

void Peek_Emptied_False(__in KAllocator & allocator)
{
    int inputArray[10] =            { 0, 2, 0, 1, 0, 5, 4, 5, 1, 3 };
    int expectedOutputArray[10] =   { 0, 0, 0, 1, 1, 2, 3, 4, 5, 5 };

    KPriorityQueue<int> kPriorityQueue(allocator);
    KTestAssert::IsTrue(kPriorityQueue.Status());

    InsertItems<int>(inputArray, 0, 10, kPriorityQueue);
    RemoveItems<int>(expectedOutputArray, 0, 10, kPriorityQueue);

    int item;
    BOOLEAN popStatus = kPriorityQueue.Peek(item);
    KTestAssert::IsFalse(popStatus);

    VerifyIsEmpty(kPriorityQueue);
}

int Compare(__in KString::SPtr const & a, __in KString::SPtr const & b)
{
    return a->Compare(*b);
}

void PushPop_SPtrValue_OrderedOutput(__in KAllocator & allocator)
{
    NTSTATUS status;
    BOOLEAN isSuccess;

    KString::SPtr stringA;
    status = KString::Create(stringA, allocator, L"a");
    KInvariant(NT_SUCCESS(status));

    KString::SPtr stringB;
    status = KString::Create(stringB, allocator, L"b");
    KInvariant(NT_SUCCESS(status));

    KString::SPtr stringC;
    status = KString::Create(stringC, allocator, L"c");
    KInvariant(NT_SUCCESS(status));

    KPriorityQueue<KString::SPtr> kPriorityQueue(allocator, Compare);
    KTestAssert::IsTrue(kPriorityQueue.Status());

    status = kPriorityQueue.Push(stringC);
    KInvariant(NT_SUCCESS(status));

    status = kPriorityQueue.Push(stringB);
    KInvariant(NT_SUCCESS(status));

    status = kPriorityQueue.Push(stringA);
    KInvariant(NT_SUCCESS(status));

    KString::SPtr peek0;
    isSuccess = kPriorityQueue.Peek(peek0);
    KInvariant(isSuccess);
    KInvariant(stringA->Compare(*peek0) == 0);

    KString::SPtr out0;
    isSuccess = kPriorityQueue.Pop(out0);
    KInvariant(isSuccess);
    KInvariant(stringA->Compare(*out0) == 0);

    KString::SPtr out1;
    isSuccess = kPriorityQueue.Pop(out1);
    KInvariant(stringB->Compare(*out1) == 0);

    KString::SPtr out2;
    isSuccess = kPriorityQueue.Pop(out2);
    KInvariant(isSuccess);
    KInvariant(stringC->Compare(*out2) == 0);

    VerifyIsEmpty(kPriorityQueue);
}

void PushPop_PushsPops_OrderedOutput(__in KAllocator & allocator)
{
    int inputArray[10] =            {0, 2, 0, 1, 0, 5, 4, 5, 1, 3};
    int expectedOutputArray[10] =   {0, 0, 0, 1, 1, 2, 3, 4, 5, 5};

    KPriorityQueue<int> kPriorityQueue(allocator);
    KTestAssert::IsTrue(kPriorityQueue.Status());

    InsertItems<int>(inputArray, 0, 10, kPriorityQueue);
    RemoveItems<int>(expectedOutputArray, 0, 10, kPriorityQueue);

    VerifyIsEmpty(kPriorityQueue);
}

void PushPop_PushsPopsPushsPops_OrderedOutput(__in KAllocator & allocator)
{
    int inputArrayOne[10] =             { 0, 2, 0, 1, 0, 5, 4, 5, 1, 3 };
    int expectedOutputArrayOne[10] =    { 0, 0, 0, 1, 1, 2, 3, 4, 5, 5 };

    int inputArrayTwo[10] =             { 5, 2, 3, 1, 1, 1, 2, 3, 4, 0 };
    int expectedOutputArrayTwo[10] =    { 0, 1, 1, 1, 2, 2, 3, 3, 4, 5 };

    KPriorityQueue<int> kPriorityQueue(allocator);
    KTestAssert::IsTrue(kPriorityQueue.Status());

    InsertItems<int>(inputArrayOne, 0, 10, kPriorityQueue);
    RemoveItems<int>(expectedOutputArrayOne, 0, 10, kPriorityQueue);

    VerifyIsEmpty(kPriorityQueue);

    InsertItems<int>(inputArrayTwo, 0, 10, kPriorityQueue);
    RemoveItems<int>(expectedOutputArrayTwo, 0, 10, kPriorityQueue);

    VerifyIsEmpty(kPriorityQueue);
}

void PushPop_AllItemsSame_OrderedOutput(__in KAllocator & allocator)
{
    int expectedOutput[10] = { 7, 7, 7, 7, 7, 7, 7, 7, 7, 7};

    KPriorityQueue<int> kPriorityQueue(allocator);
    KTestAssert::IsTrue(kPriorityQueue.Status());

    InsertItems<int>(expectedOutput, 0, 10, kPriorityQueue);
    RemoveItems<int>(expectedOutput, 0, 10, kPriorityQueue);

    VerifyIsEmpty(kPriorityQueue);

    InsertItems<int>(expectedOutput, 0, 10, kPriorityQueue);
    RemoveItems<int>(expectedOutput, 0, 10, kPriorityQueue);

    VerifyIsEmpty(kPriorityQueue);
}

void PushPop_PushTwoPopOne_OrderedOutput(__in KAllocator & allocator)
{
    int inputArray[20] =            { 0, 10, 1, 11, 2, 12, 3, 13, 4, 14, 5, 15, 5, 15, 6, 16, 7, 17, 8, 18 };
    int expectedOutputArray[20] =   { 0, 1, 2, 3, 4, 5, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15, 15, 16, 17, 18 };

    KPriorityQueue<int> kPriorityQueue(allocator);
    KTestAssert::IsTrue(kPriorityQueue.Status());

    for(int i = 0; i < 10; i++)
    {
        InsertItems<int>(inputArray, i*2, 2, kPriorityQueue);
        RemoveItems<int>(expectedOutputArray, i, 1, kPriorityQueue);
    }

    RemoveItems<int>(expectedOutputArray, 10, 10, kPriorityQueue);

    VerifyIsEmpty(kPriorityQueue);
}

// This is a white-box test for bubble down.
void BubbleDown_LeftChildBiggerThanRightChildButSmallerThanParent_OrderedOutput(__in KAllocator & allocator)
{
    int inputArray[4] =             { 1, 6, 5, 4 };
    int expectedOutputArray[4] =    { 1, 4, 5, 6 };

    KPriorityQueue<int> kPriorityQueue(allocator);
    KTestAssert::IsTrue(kPriorityQueue.Status());

    InsertItems<int>(inputArray, 0, 4, kPriorityQueue);
    RemoveItems<int>(expectedOutputArray, 0, 4, kPriorityQueue);

    BOOLEAN isEmpty = kPriorityQueue.IsEmpty();
    KTestAssert::IsTrue(isEmpty);
}

int CompareNeverEqual(__in int const & a, __in int const & b)
{
    if (a < b) return -1;
    if (a > b) return 1;
    KTestAssert::IsTrue(a != b);
    return 0; 
}

void BubbleDown_ItemsNotComparedWithThemselves_OrderedOutput(__in KAllocator & allocator)
{
    KPriorityQueue<int> kPriorityQueue(allocator, CompareNeverEqual);
    KTestAssert::IsTrue(kPriorityQueue.Status());

    int inputArray[4] = { 1, 6, 5, 4 };
    int expectedOutputArray[4] = { 1, 4, 5, 6 };

    InsertItems<int>(inputArray, 0, 4, kPriorityQueue);
    RemoveItems<int>(expectedOutputArray, 0, 4, kPriorityQueue);

    BOOLEAN isEmpty = kPriorityQueue.IsEmpty();
    KTestAssert::IsTrue(isEmpty);
}


void Random_PushSomePopSome_OrderedOutput(__in KAllocator & allocator)
{
    KPriorityQueue<int> kPriorityQueue(allocator);
    KTestAssert::IsTrue(kPriorityQueue.Status());

    for(int i = 0; i < 256; i++)
    {
        InsertItems(1, kPriorityQueue);
        RemoveItems(1, kPriorityQueue);
    }

    for (int i = 0; i < 256; i++)
    {
        InsertItems(2, kPriorityQueue);
        RemoveItems(2, kPriorityQueue);
    }

    for (int i = 0; i < 256; i++)
    {
        InsertItems(4, kPriorityQueue);
        RemoveItems(4, kPriorityQueue);
    }

    for (int i = 0; i < 256; i++)
    {
        InsertItems(8, kPriorityQueue);
        RemoveItems(8, kPriorityQueue);
    }

    BOOLEAN isEmpty = kPriorityQueue.IsEmpty();
    KTestAssert::IsTrue(isEmpty);
}

void Random_PushSomePopSomeButLeaveSome_OrderedOutput(__in KAllocator & allocator)
{
    KPriorityQueue<int> kPriorityQueue(allocator);
    KTestAssert::IsTrue(kPriorityQueue.Status());

    for (int i = 0; i < 256; i++)
    {
        InsertItems(2, kPriorityQueue);
        RemoveItems(1, kPriorityQueue);
    }

    for (int i = 0; i < 256; i++)
    {
        InsertItems(4, kPriorityQueue);
        RemoveItems(2, kPriorityQueue);
    }

    for (int i = 0; i < 256; i++)
    {
        InsertItems(8, kPriorityQueue);
        RemoveItems(4, kPriorityQueue);
    }

    RemoveItems(1024 + 512 + 256, kPriorityQueue);

    BOOLEAN isEmpty = kPriorityQueue.IsEmpty();
    KTestAssert::IsTrue(isEmpty);
}

// Insert random items.
void InsertItems(int Count, KPriorityQueue<int>& PriorityQueue)
{
    for (int i = 0; i < Count; i++)
    {
        NTSTATUS status = PriorityQueue.Push(rand());
        KTestAssert::IsTrue(NT_SUCCESS(status));
    }

    BOOLEAN isEmpty = PriorityQueue.IsEmpty();
    KTestAssert::IsFalse(isEmpty);
}

// Remove items.
void RemoveItems(int Count, KPriorityQueue<int>& PriorityQueue)
{
    int lastValuePopped = INT_MIN;

    for (int i = 0; i < Count; i++)
    {
        int peekedItem;
        BOOLEAN peekSuccess = PriorityQueue.Peek(peekedItem);
        KTestAssert::IsTrue(peekSuccess);
        KTestAssert::IsTrue(lastValuePopped <= peekedItem);

        int popedItem;
        BOOLEAN popStatus = PriorityQueue.Pop(popedItem);
        KTestAssert::IsTrue(popStatus);
        KTestAssert::IsTrue(lastValuePopped <= popedItem);

        lastValuePopped = popedItem;
    }
}

NTSTATUS
KPriorityQueueTest(__in int argc, __in_ecount(argc) WCHAR* args[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

#if defined(PLATFORM_UNIX)
    NTSTATUS status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif
	
    KTestPrintf("KPriorityQueueTest started.\n");

    KtlSystem* system;
    NTSTATUS result = KtlSystem::Initialize(&system);
    KTestAssert::IsTrue(result);
    KFinally([&]() { KtlSystem::Shutdown(); });

    system->SetStrictAllocationChecks(TRUE);

    // Api behavior tests
    IsEmpty_EmptyPriorityQueue_True(system->NonPagedAllocator());
    IsEmpty_NonEmpty_False(system->NonPagedAllocator());

    Pop_NonEmpty_False(system->NonPagedAllocator());
    Pop_Emptied_False(system->NonPagedAllocator());

    Peek_NonEmpty_False(system->NonPagedAllocator());
    Peek_Emptied_False(system->NonPagedAllocator());

    // Basic SPtr
    PushPop_SPtrValue_OrderedOutput(system->NonPagedAllocator());

    // Basic scenarios.
    PushPop_PushsPops_OrderedOutput(system->NonPagedAllocator());
    PushPop_PushsPopsPushsPops_OrderedOutput(system->NonPagedAllocator());
    PushPop_AllItemsSame_OrderedOutput(system->NonPagedAllocator());
    PushPop_PushTwoPopOne_OrderedOutput(system->NonPagedAllocator());

    BubbleDown_LeftChildBiggerThanRightChildButSmallerThanParent_OrderedOutput(system->NonPagedAllocator());
    BubbleDown_ItemsNotComparedWithThemselves_OrderedOutput(system->NonPagedAllocator());

    Random_PushSomePopSome_OrderedOutput(system->NonPagedAllocator());
    Random_PushSomePopSomeButLeaveSome_OrderedOutput(system->NonPagedAllocator());

    KTestPrintf("KPriorityQueueTest completed successfully.\n");

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif  
	
    return STATUS_SUCCESS;
}

#if CONSOLE_TEST

int __cdecl
wmain(
    __in int argc,
    __in_ecount(argc) WCHAR* args[]
)
{
    NTSTATUS status;

    //
    // Adjust for the test name so CmdParseLine works.
    //

    if (argc > 0)
    {
        argc--;
        args++;
    }

    status = KPriorityQueueTest(argc, args);
    KTestAssert::IsTrue(status);

    return RtlNtStatusToDosError(status);
}
#endif


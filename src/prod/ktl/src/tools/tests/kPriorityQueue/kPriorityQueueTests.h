/*++
Copyright (c) Microsoft Corporation

Module Name:    KPriorityQueueTests.h
Abstract:       Contains unit test case routine declarations.
--*/

#pragma once
#include "KTestAssert.h"
#include "KPriorityQueue.h"

#if KTL_USER_MODE
#include <KmUser.h>
#else
#include <KmUnit.h>
#endif

NTSTATUS
KPriorityQueueTest(
    __in int argc, 
    __in_ecount(argc) WCHAR* args[]
    );

void InsertItems(__in int Count, __in KPriorityQueue<int>& PriorityQueue);
void RemoveItems(__in int Count, __in KPriorityQueue<int>& PriorityQueue);

template <class T>
void InsertItems(__in T* Items, __in int StartingOffset, __in int Count, __in KPriorityQueue<T>& PriorityQueue)
{
    for (int i = StartingOffset; i < StartingOffset + Count; i++)
    {
        NTSTATUS status = PriorityQueue.Push(Items[i]);
        KTestAssert::IsTrue(NT_SUCCESS(status));
    }

    BOOLEAN isEmpty = PriorityQueue.IsEmpty();
    KTestAssert::IsFalse(isEmpty);
}

template <class T>
void RemoveItems(__in T* Items, __in int StartingOffset, __in int Count, KPriorityQueue<T>& priorityQueueSPtr)
{
    for (int i = StartingOffset; i < StartingOffset + Count; i++)
    {
        T peekedItem;
        BOOLEAN peekSuccess = priorityQueueSPtr.Peek(peekedItem);
        KTestAssert::IsTrue(peekSuccess);
        KTestAssert::AreEqual(peekedItem, Items[i]);

        T poppedItem;
        BOOLEAN popStatus = priorityQueueSPtr.Pop(poppedItem);
        KTestAssert::IsTrue(popStatus);
        KTestAssert::AreEqual(poppedItem, Items[i]);
    }
}

template <class T>
void VerifyIsEmpty(__in KPriorityQueue<T>& PriorityQueue)
{
    BOOLEAN isEmpty = PriorityQueue.IsEmpty();
    KTestAssert::IsTrue(isEmpty);
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "../testcommon/TestCommon.Public.h"
#include "ConcurrentDictionaryTest.h"

using namespace ktl;
using namespace Data;
using namespace Data::Utilities;

void ConcurrentDictionaryTest::RunDictionaryTest_Add1(
    int cLevel,
    int initSize,
    int threads,
    int addsPerThread)
{
    UNREFERENCED_PARAMETER(cLevel);
    UNREFERENCED_PARAMETER(initSize);

    NTSTATUS status;
    volatile ULONG count = threads;

    KAllocator& allocator = GetAllocator();
    ConcurrentDictionary<int, int>::SPtr dict;
    status = ConcurrentDictionary<int, int>::Create(allocator, dict);
    KInvariant(NT_SUCCESS(status));

    KArray<KThreadPool::WorkItem*> itemArray(allocator);
    KFinally([&] {
        for (ULONG i = 0; i < itemArray.Count(); ++i)
        {
            _delete(itemArray[i]);
        }
    });

    KEvent event;
    for (int i = 0; i < threads; i++)
    {
        int ii = i;
        auto work = [&, ii] {
            for (int j = 0; j < addsPerThread; j++)
            {
                dict->TryAdd(j + ii * addsPerThread, -(j + ii * addsPerThread));
            }
            if (InterlockedDecrement((volatile LONG*)&count) == 0)
            {
                event.SetEvent();
            }
        };
        WorkItem<decltype(work)> *item = _new('xxxx', allocator) WorkItem<decltype(work)>(work);
        KInvariant(item);

        GetThreadPool().QueueWorkItem(*item);
        itemArray.Append(item);
    }
    event.WaitUntilSet();

    typename IEnumerator<KeyValuePair<int, int>>::SPtr enumerator = dict->GetEnumerator();
    while (enumerator->MoveNext())
    {
        KeyValuePair<int, int> entry = enumerator->Current();
        KInvariant(entry.Key == -entry.Value);
    }

    ULONG expectedCount = threads * addsPerThread;
    KInvariant(dict->Count == expectedCount);
}

void ConcurrentDictionaryTest::RunDictionaryTest_Update1(
    int cLevel,
    int threads,
    int updatesPerThread)
{
    UNREFERENCED_PARAMETER(cLevel);

    NTSTATUS status;

    KAllocator& allocator = GetAllocator();
    ConcurrentDictionary<int, int>::SPtr dict;
    status = ConcurrentDictionary<int, int>::Create(allocator, dict);
    KInvariant(NT_SUCCESS(status));

    for (int i = 1; i <= updatesPerThread; ++i)
    {
        KInvariant(dict->TryAdd(i, i));
    }

    KArray<KThreadPool::WorkItem*> itemArray(allocator);
    KFinally([&] {
        for (ULONG i = 0; i < itemArray.Count(); ++i)
        {
            _delete(itemArray[i]);
        }
    });

    volatile ULONG running = threads;
    KEvent event;
    for (int i = 0; i < threads; ++i)
    {
        int ii = i;
        auto work = [&, ii] {
            for (int j = 1; j <= updatesPerThread; ++j)
            {
                dict->TryUpdate(j, (ii + 2) * j, j);
            }
            if (InterlockedDecrement((volatile LONG*)&running) == 0)
            {
                event.SetEvent();
            }
        };
        WorkItem<decltype(work)> *item = _new(CONCURRENTDICTIONARY_TAG, allocator) WorkItem<decltype(work)>(work);
        KInvariant(item);

        GetThreadPool().QueueWorkItem(*item);
        itemArray.Append(item);
    }
    event.WaitUntilSet();

    typename IEnumerator<KeyValuePair<int, int>>::SPtr enumerator = dict->GetEnumerator();
    while (enumerator->MoveNext())
    {
        KeyValuePair<int, int> entry = enumerator->Current();
        int div = entry.Value / entry.Key;
        int rem = entry.Value % entry.Key;
        KInvariant(rem == 0);
        KInvariant(div >= 2);
        KInvariant(div <= threads + 1);
    }
}

void ConcurrentDictionaryTest::RunDictionaryTest_Read1(
    int cLevel,
    int threads,
    int readsPerThread)
{
    UNREFERENCED_PARAMETER(cLevel);

    NTSTATUS status;

    KAllocator& allocator = GetAllocator();
    ConcurrentDictionary<int, int>::SPtr dict;
    status = ConcurrentDictionary<int, int>::Create(allocator, dict);
    KInvariant(NT_SUCCESS(status));

    for (int i = 0; i < readsPerThread; i += 2)
    {
        KInvariant(dict->TryAdd(i, i));
    }

    KArray<KThreadPool::WorkItem*> itemArray(allocator);
    KFinally([&] {
        for (ULONG i = 0; i < itemArray.Count(); ++i)
        {
            _delete(itemArray[i]);
        }
    });

    volatile ULONG count = threads;
    KEvent event;
    for (int i = 0; i < threads; ++i)
    {
        auto work = [&] {
            for (int j = 0; j < readsPerThread; ++j)
            {
                int val = 0;
                if (dict->TryGetValue(j, val))
                {
                    KInvariant((j % 2 == 0) && (j == val));
                }
                else
                {
                    KInvariant(j % 2 != 0);
                }
            }
            if (InterlockedDecrement((volatile LONG*)&count) == 0)
            {
                event.SetEvent();
            }
        };
        WorkItem<decltype(work)> *item = _new(CONCURRENTDICTIONARY_TAG, allocator) WorkItem<decltype(work)>(work);
        KInvariant(item);

        GetThreadPool().QueueWorkItem(*item);
        itemArray.Append(item);
    }
    event.WaitUntilSet();
}

void ConcurrentDictionaryTest::RunDictionaryTest_Remove1(
    int cLevel,
    int threads,
    int removesPerThread)
{
    UNREFERENCED_PARAMETER(cLevel);

    NTSTATUS status;

    KAllocator& allocator = GetAllocator();
    ConcurrentDictionary<int, int>::SPtr dict;
    status = ConcurrentDictionary<int, int>::Create(allocator, dict);
    KInvariant(NT_SUCCESS(status));

    int N = 2 * threads * removesPerThread;
    for (int i = 0; i < N; ++i)
    {
        KInvariant(dict->TryAdd(i, -i));
    }

    KArray<KThreadPool::WorkItem*> itemArray(allocator);
    KFinally([&] {
        for (ULONG i = 0; i < itemArray.Count(); ++i)
        {
            _delete(itemArray[i]);
        }
    });

    volatile ULONG running = threads;
    KEvent event;
    for (int i = 0; i < threads; ++i)
    {
        int ii = i;
        auto work = [&, ii] {
            for (int j = 0; j < removesPerThread; ++j)
            {
                int value = 0;
                int key = 2 * (ii + j * threads);
                KInvariant(dict->TryRemove(key, value));
                KInvariant(value == -key);
            }
            if (InterlockedDecrement((volatile LONG*)&running) == 0)
            {
                event.SetEvent();
            }
        };
        WorkItem<decltype(work)> *item = _new(CONCURRENTDICTIONARY_TAG, allocator) WorkItem<decltype(work)>(work);
        KInvariant(item);

        GetThreadPool().QueueWorkItem(*item);
        itemArray.Append(item);
    }
    event.WaitUntilSet();

    typename IEnumerator<KeyValuePair<int, int>>::SPtr enumerator = dict->GetEnumerator();
    while (enumerator->MoveNext())
    {
        KeyValuePair<int, int> entry = enumerator->Current();
        KInvariant(entry.Key == -entry.Value);
    }
}

static int valueFactory(int const & key)
{
    return -key;
}

void ConcurrentDictionaryTest::RunDictionaryTest_GetOrAdd(
    int cLevel,
    int initSize,
    int threads,
    int addsPerThread)
{
    UNREFERENCED_PARAMETER(cLevel);
    UNREFERENCED_PARAMETER(initSize);

    NTSTATUS status;

    KAllocator& allocator = GetAllocator();
    ConcurrentDictionary<int, int>::SPtr dict;
    status = ConcurrentDictionary<int, int>::Create(allocator, dict);
    KInvariant(NT_SUCCESS(status));

    KArray<KThreadPool::WorkItem*> itemArray(allocator);
    KFinally([&] {
        for (ULONG i = 0; i < itemArray.Count(); ++i)
        {
            _delete(itemArray[i]);
        }
    });

    volatile ULONG count = threads;
    KDelegate<int(int const &)> func(valueFactory);
    KEvent event;
    for (int i = 0; i < threads; ++i)
    {
        int ii = i;
        auto work = [&, ii] {
            for (int j = 0; j < addsPerThread; ++j)
            {
                if (j + ii % 2 == 0)
                {
                    dict->GetOrAdd(j, -j);
                }
                else
                {
                    dict->GetOrAdd(j, func);
                }
            }
            if (InterlockedDecrement((volatile LONG*)&count) == 0)
            {
                event.SetEvent();
            }
        };
        WorkItem<decltype(work)> *item = _new(CONCURRENTDICTIONARY_TAG, allocator) WorkItem<decltype(work)>(work);
        KInvariant(item);

        GetThreadPool().QueueWorkItem(*item);
        itemArray.Append(item);
    }
    event.WaitUntilSet();

    typename IEnumerator<KeyValuePair<int, int>>::SPtr enumerator = dict->GetEnumerator();
    while (enumerator->MoveNext())
    {
        KeyValuePair<int, int> entry = enumerator->Current();
        KInvariant(entry.Key == -entry.Value);
    }
}

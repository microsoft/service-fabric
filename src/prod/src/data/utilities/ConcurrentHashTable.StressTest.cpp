
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#define ALLOC_TAG 'ttHC'

namespace UtilitiesTests
{
    using namespace Common;
    using namespace ktl;
    using namespace Data;
    using namespace Data::Utilities;

    class ConcurrentHashTableStressTest
    {
    public:
        typedef ConcurrentHashTable<LONG64, LONG64> LongLongHashTable;

        enum OperationType
        {
            Read,
            Update,
            Add,
            Remove
        };

        struct HashTableOperation
        {
            OperationType Type;
            LONG64 Key;

            // Value to set for Add/Update operations. Expected value for read/remove operations
            LONG64 Value;

            bool ShouldSucceed = true;

        };

        static LONG32 Compare(__in KeyValuePair<LONG64, LONG64> const & itemOne, __in KeyValuePair<LONG64, LONG64> const & itemTwo)
        {
            return itemOne.Key > itemTwo.Key ? 1 : (itemOne.Key < itemTwo.Key ? -1 : 0);
        }

        ConcurrentHashTableStressTest()
        {
            NTSTATUS status;
            status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);
        }

        ~ConcurrentHashTableStressTest()
        {
            ktlSystem_->Shutdown();
        }

        KAllocator& GetAllocator()
        {
            return ktlSystem_->NonPagedAllocator();
        }

        Random GetRandom()
        {
            auto seed = Common::Stopwatch::Now().Ticks;
            Random random(static_cast<int>(seed));
            cout << "Random seed (use this seed to reproduce test failures): " << seed << endl;
            return random;
        }

        LongLongHashTable::SPtr CreateEmptyHashTable(__in ULONG concurrencyLevel=16)
        {
            LongLongHashTable::SPtr result = nullptr;
            LongLongHashTable::Create(K_DefaultHashFunction, concurrencyLevel, GetAllocator(), result);
            return result;
        }

        Awaitable<void> ApplyOperationsAsync(
            __in LongLongHashTable & hashTable,
            __in KSharedArray<HashTableOperation> & items,
            __in ULONG offset,
            __in ULONG count,
            __in bool verifyContainsKey = true,
            __in bool verifyValue = true)
        {
            co_await CorHelper::ThreadPoolThread(GetAllocator().GetKtlSystem().DefaultThreadPool());

            for (ULONG i = offset; i < offset + count; i++)
            {
                bool successful = false;
                bool added = false;
                LONG64 outValue = -1;

                HashTableOperation item = items[i];
                switch (item.Type)
                {
                case OperationType::Add:
                    hashTable.AddOrUpdate(item.Key, item.Value, successful);
                    break;
                case OperationType::Update:
                    hashTable.AddOrUpdate(item.Key, item.Value, added);
                    successful = !added;
                    break;
                case OperationType::Remove:
                    successful = hashTable.TryRemove(item.Key, outValue);
                    break;
                case OperationType::Read:
                    successful = hashTable.TryGetValue(item.Key, outValue);
                    break;
                default:
                    CODING_ERROR_ASSERT(false);
                }

                if (verifyValue)
                {
                    CODING_ERROR_ASSERT(item.ShouldSucceed == successful);
                    if (item.ShouldSucceed && (item.Type == OperationType::Read || item.Type == OperationType::Remove))
                    {
                        CODING_ERROR_ASSERT(item.Value == outValue);
                    }
                }

                if (verifyContainsKey)
                {
                    CODING_ERROR_ASSERT(item.ShouldSucceed == successful);
                }
            }
        }

        Awaitable<void> ApplyOperationsAsync(
            __in LongLongHashTable & hashTable,
            __in KSharedArray<HashTableOperation> & items,
            __in ULONG numTasks,
            __in bool verifyContainsKey = true,
            __in bool verifyValue = true)
        {
            KArray<Awaitable<void>> tasks = KArray<Awaitable<void>>(GetAllocator(), numTasks);
            const ULONG keysPerTask = items.Count() / numTasks;

            for (ULONG32 i = 0; i < numTasks; i++)
            {
                tasks.Append(ApplyOperationsAsync(hashTable, items, i * keysPerTask, keysPerTask, verifyContainsKey, verifyValue));
            }

            co_await TaskUtilities<void>::WhenAll(tasks);
        }

    public:
        Awaitable<void> ConcurrentHashTable_AddReadUpdateReadDeleteRead_Sequential_Test()
        {
            const LONG64 numKeys = 10'000'000;
            const ULONG numTasks = 1; // Sequential test
            
            KSharedArray<HashTableOperation>::SPtr operationsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<HashTableOperation>();
            
            for (LONG64 key = 0; key < numKeys; key++)
            {
                HashTableOperation operation = { OperationType::Add, key, key };
                operationsSPtr->Append(operation);
            }

            for (LONG64 key = 0; key < numKeys; key++)
            {
                HashTableOperation operation = { OperationType::Read, key, key };
                operationsSPtr->Append(operation);
            }

            for (LONG64 key = 0; key < numKeys; key++)
            {
                HashTableOperation operation = { OperationType::Update, key, key + 10 };
                operationsSPtr->Append(operation);
            }

            for (LONG64 key = 0; key < numKeys; key++)
            {
                HashTableOperation operation = { OperationType::Read, key, key + 10 };
                operationsSPtr->Append(operation);
            }

            for (LONG64 key = 0; key < numKeys; key++)
            {
                HashTableOperation operation = { OperationType::Remove, key, key + 10 };
                operationsSPtr->Append(operation);
            }

            for (LONG64 key = 0; key < numKeys; key++)
            {
                HashTableOperation operation = { OperationType::Read, key, 0, false };
                operationsSPtr->Append(operation);
            }
            
            LongLongHashTable::SPtr hashTableSPtr = CreateEmptyHashTable();
            co_await ApplyOperationsAsync(*hashTableSPtr, *operationsSPtr, numTasks);
        }

        Awaitable<void> ConcurrentHashTable_ConcurrentAdd_Test()
        {
            const LONG64 numKeys = 10'000'000;

            const ULONG numAddTasks = 50;
            const ULONG numReadTasks = 1; // Verify sequentially

            KSharedArray<HashTableOperation>::SPtr addOperationsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<HashTableOperation>();
            for (LONG64 key = 0; key < numKeys; key++)
            {
                HashTableOperation operation = { OperationType::Add, key, key };
                addOperationsSPtr->Append(operation);
            }

            KSharedArray<HashTableOperation>::SPtr readOperationsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<HashTableOperation>();
            for (LONG64 key = 0; key < numKeys; key++)
            {
                HashTableOperation operation = { OperationType::Read, key, key };
                readOperationsSPtr->Append(operation);
            }

            LongLongHashTable::SPtr hashTableSPtr = CreateEmptyHashTable();

            co_await ApplyOperationsAsync(*hashTableSPtr, *addOperationsSPtr, numAddTasks);
            co_await ApplyOperationsAsync(*hashTableSPtr, *readOperationsSPtr, numReadTasks);
        }

        Awaitable<void> ConcurrentHashTable_ConcurrentUpdate_Test()
        {
            const LONG64 numKeys = 10'000'000;

            const ULONG numWriteTasks = 50;
            const ULONG numReadTasks = 1; // Verify sequentially

            KSharedArray<HashTableOperation>::SPtr addOperationsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<HashTableOperation>();
            for (LONG64 key = 0; key < numKeys; key++)
            {
                HashTableOperation operation = { OperationType::Add, key, key };
                addOperationsSPtr->Append(operation);
            }

            KSharedArray<HashTableOperation>::SPtr updateOperationsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<HashTableOperation>();
            for (LONG64 key = 0; key < numKeys; key++)
            {
                HashTableOperation operation = { OperationType::Update, key, key };
                updateOperationsSPtr->Append(operation);
            }

            KSharedArray<HashTableOperation>::SPtr readOperationsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<HashTableOperation>();
            for (LONG64 key = 0; key < numKeys; key++)
            {
                HashTableOperation operation = { OperationType::Read, key, key };
                readOperationsSPtr->Append(operation);
            }

            LongLongHashTable::SPtr hashTableSPtr = CreateEmptyHashTable();

            co_await ApplyOperationsAsync(*hashTableSPtr, *addOperationsSPtr, numWriteTasks);
            co_await ApplyOperationsAsync(*hashTableSPtr, *updateOperationsSPtr, numWriteTasks);
            co_await ApplyOperationsAsync(*hashTableSPtr, *readOperationsSPtr, numReadTasks);
        }

        Awaitable<void> ConcurrentHashTable_ConcurrentRead_Test()
        {
            const LONG64 numKeys = 10'000'000;

            const ULONG numTasks = 50;

            KSharedArray<HashTableOperation>::SPtr addOperationsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<HashTableOperation>();
            for (LONG64 key = 0; key < numKeys; key++)
            {
                HashTableOperation operation = { OperationType::Add, key, key };
                addOperationsSPtr->Append(operation);
            }

            KSharedArray<HashTableOperation>::SPtr readOperationsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<HashTableOperation>();
            for (LONG64 key = 0; key < numKeys; key++)
            {
                HashTableOperation operation = { OperationType::Read, key, key };
                readOperationsSPtr->Append(operation);
            }

            LongLongHashTable::SPtr hashTableSPtr = CreateEmptyHashTable();

            co_await ApplyOperationsAsync(*hashTableSPtr, *addOperationsSPtr, numTasks);
            co_await ApplyOperationsAsync(*hashTableSPtr, *readOperationsSPtr, numTasks);
        }

        Awaitable<void> ConcurrentHashTable_ConcurrentRemove_Test()
        {
            const LONG64 numKeys = 10'000'000;

            const ULONG numWriteTasks = 50;
            const ULONG numReadTasks = 1; //Verify sequentially

            KSharedArray<HashTableOperation>::SPtr addOperationsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<HashTableOperation>();
            for (LONG64 key = 0; key < numKeys; key++)
            {
                HashTableOperation operation = { OperationType::Add, key, key };
                addOperationsSPtr->Append(operation);
            }

            KSharedArray<HashTableOperation>::SPtr removeOperationsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<HashTableOperation>();
            for (LONG64 key = 0; key < numKeys; key++)
            {
                HashTableOperation operation = { OperationType::Remove, key, key };
                removeOperationsSPtr->Append(operation);
            }

            KSharedArray<HashTableOperation>::SPtr readOperationsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<HashTableOperation>();
            for (LONG64 key = 0; key < numKeys; key++)
            {
                HashTableOperation operation = { OperationType::Read, key, key, false };
                readOperationsSPtr->Append(operation);
            }

            LongLongHashTable::SPtr hashTableSPtr = CreateEmptyHashTable();

            co_await ApplyOperationsAsync(*hashTableSPtr, *addOperationsSPtr, numWriteTasks);
            co_await ApplyOperationsAsync(*hashTableSPtr, *removeOperationsSPtr, numWriteTasks);
            co_await ApplyOperationsAsync(*hashTableSPtr, *readOperationsSPtr, numReadTasks);
        }

        Awaitable<void> ConcurrentHashTable_ConcurrentRandomAdds_Test()
        {
            LongLongHashTable::SPtr hashTableSPtr = CreateEmptyHashTable();

            Random random = GetRandom();

            const ULONG numKeys = 10'000'000;
            const ULONG numTasks = 50;

            KAutoHashTable<LONG64, bool> keysSet(100, K_DefaultHashFunction, GetAllocator());

            KSharedArray<HashTableOperation>::SPtr writeOperationsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<HashTableOperation>();
            KSharedArray<HashTableOperation>::SPtr readOperationsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<HashTableOperation>();

            for (ULONG i = 0; i < numKeys; i++)
            {
                LONG64 key = random.Next(100'000'000);

                if (keysSet.ContainsKey(key))
                {
                    i--;
                    continue;
                }

                keysSet.Put(key, true);

                LONG64 value = random.Next(1'000'000);

                HashTableOperation writeOperation = { OperationType::Add, key, value };
                writeOperationsSPtr->Append(writeOperation);

                HashTableOperation readOperation = { OperationType::Read, key, value };
                readOperationsSPtr->Append(readOperation);
            }

            // Write in random order
            co_await ApplyOperationsAsync(*hashTableSPtr, *writeOperationsSPtr, numTasks);

            // Read in random order
            co_await ApplyOperationsAsync(*hashTableSPtr, *readOperationsSPtr, numTasks);
        }

        Awaitable<void> ConcurrentHashTable_ConcurrentRandomAddOrUpdateAndRead_Test()
        {
            LongLongHashTable::SPtr hashTableSPtr = CreateEmptyHashTable();

            Random random = GetRandom();

            const ULONG numKeys = 10'000'000;
            const ULONG numTasks = 50;
            const int maxKey = 500; // High contention

            KAutoHashTable<LONG64, bool> keysSet(100, K_DefaultHashFunction, GetAllocator());

            KSharedArray<HashTableOperation>::SPtr operationsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<HashTableOperation>();

            for (ULONG i = 0; i < numKeys; i++)
            {
                OperationType opType = (i % 2 == 0) ? OperationType::Add : OperationType::Read;

                LONG64 key = random.Next(maxKey);

                if (keysSet.ContainsKey(key))
                {
                    opType = OperationType::Update;
                }

                keysSet.Put(key, true);

                LONG64 value = random.Next(maxKey);

                HashTableOperation operation = { opType, key, value };
                operationsSPtr->Append(operation);
            }

            co_await ApplyOperationsAsync(*hashTableSPtr, *operationsSPtr, numTasks, false, false);
        }

        Awaitable<void> ConcurrentHashTable_Stress_Test()
        {
            LongLongHashTable::SPtr hashTableSPtr = CreateEmptyHashTable();

            Random random = GetRandom();

            const ULONG numOps = 50'000'000;
            const ULONG numTasks = 12;
            const int maxKey = 1'000'000;

            KSharedArray<HashTableOperation>::SPtr operationsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<HashTableOperation>();

            for (ULONG i = 0; i < numOps; i++)
            {
                double choice = random.NextDouble();

                OperationType opType;

                if (choice < 0.70)
                {
                    opType = OperationType::Update;
                }
                else if (choice < 0.95)
                {
                    opType = OperationType::Read;
                }
                else
                {
                    opType = OperationType::Remove;
                }

                LONG64 key = random.Next(maxKey);
                LONG64 value = random.Next(maxKey);

                HashTableOperation operation = { opType, key, value };
                operationsSPtr->Append(operation);
            }

            co_await ApplyOperationsAsync(*hashTableSPtr, *operationsSPtr, numTasks, false, false);
        }

        Awaitable<void> TestAdd(__in ULONG concurrencyLevel, __in ULONG numTasks, __in ULONG addsPerTask)
        {
            auto hashTable = CreateEmptyHashTable(concurrencyLevel);

            KArray<Awaitable<void>> tasks = KArray<Awaitable<void>>(GetAllocator(), numTasks);

            for (ULONG32 i = 0; i < numTasks; i++)
            {
                tasks.Append([&] (ULONG32 taskNum) -> Awaitable<void> {
                    co_await CorHelper::ThreadPoolThread(GetAllocator().GetKtlSystem().DefaultThreadPool());
                    for (LONG64 j = 0; j < addsPerTask; j++)
                    {
                        hashTable->Add(j + taskNum * addsPerTask, -(j + taskNum * addsPerTask));
                    }
                    co_return;
                }(i));
            }

            co_await TaskUtilities<void>::WhenAll(tasks);

            KSharedArray<KeyValuePair<LONG64, LONG64>>::SPtr items = _new(ALLOC_TAG, GetAllocator()) KSharedArray<KeyValuePair<LONG64, LONG64>>();

            auto enumerator = hashTable->GetEnumerator();
            while (enumerator->MoveNext())
            {
                auto current = enumerator->Current();
                CODING_ERROR_ASSERT(current.Key == -current.Value);
                items->Append(current);
            }

            ULONG32 expectedCount = numTasks * addsPerTask;
            CODING_ERROR_ASSERT(items->Count() == expectedCount);
            Sort<KeyValuePair<LONG64, LONG64>>::QuickSort(true, Compare, items);

            for (ULONG32 i = 0; i < expectedCount; i++)
            {
                CODING_ERROR_ASSERT((*items)[i].Key == static_cast<LONG64>(i));
            }

            CODING_ERROR_ASSERT(items->Count() == expectedCount);
        }

        Awaitable<void> TestUpdate(__in ULONG concurrencyLevel, __in ULONG numTasks, __in ULONG updatesPerTask)
        {
            auto hashTable = CreateEmptyHashTable(concurrencyLevel);

            for (ULONG32 i = 1; i <= updatesPerTask; i++)
            {
                hashTable->Add(i, i);
            }

            KArray<Awaitable<void>> tasks = KArray<Awaitable<void>>(GetAllocator(), numTasks);

            for (ULONG32 i = 0; i < numTasks; i++)
            {
                tasks.Append([&] (ULONG32 taskNum) -> Awaitable<void> {
                    co_await CorHelper::ThreadPoolThread(GetAllocator().GetKtlSystem().DefaultThreadPool());
                    for (LONG64 j = 1; j <= updatesPerTask; j++)
                    {
                        bool added = false;
                        hashTable->AddOrUpdate(j, (taskNum + 2) * j, added);
                        CODING_ERROR_ASSERT(added == false);
                    }
                    co_return;
                }(i));
            }

            co_await TaskUtilities<void>::WhenAll(tasks);

            KSharedArray<KeyValuePair<LONG64, LONG64>>::SPtr items = _new(ALLOC_TAG, GetAllocator()) KSharedArray<KeyValuePair<LONG64, LONG64>>();

            auto enumerator = hashTable->GetEnumerator();
            while (enumerator->MoveNext())
            {
                auto current = enumerator->Current();
                items->Append(current);

                auto div = current.Value / current.Key;
                auto rem = current.Value % current.Key;

                CODING_ERROR_ASSERT(0 == rem);
                CODING_ERROR_ASSERT(div > 1 && div <= numTasks + 1);
            }
            Sort<KeyValuePair<LONG64, LONG64>>::QuickSort(true, Compare, items);

            ULONG32 expectedCount = updatesPerTask;
            CODING_ERROR_ASSERT(items->Count() == expectedCount);

            for (ULONG32 i = 0; i < expectedCount; i++)
            {
                CODING_ERROR_ASSERT((*items)[i].Key == static_cast<LONG64>(i + 1));
            }

            CODING_ERROR_ASSERT(items->Count() == expectedCount);
        }

        Awaitable<void> TestRead(__in ULONG concurrencyLevel, __in ULONG numTasks, __in ULONG readsPerTask)
        {
            auto hashTable = CreateEmptyHashTable(concurrencyLevel);

            for (ULONG32 i = 0; i < readsPerTask; i += 2)
            {
                hashTable->Add(i, i);
            }

            KArray<Awaitable<void>> tasks = KArray<Awaitable<void>>(GetAllocator(), numTasks);

            for (ULONG32 i = 0; i < numTasks; i++)
            {
                tasks.Append([&] (ULONG32 taskNum) -> Awaitable<void> {
                    co_await CorHelper::ThreadPoolThread(GetAllocator().GetKtlSystem().DefaultThreadPool());
                    for (LONG64 j = 0; j < readsPerTask; j++)
                    {
                        LONG64 val = 0;
                        bool found = hashTable->TryGetValue(j, val);
                        if (found)
                        {
                            CODING_ERROR_ASSERT(j % 2 == 0);
                            CODING_ERROR_ASSERT(j == val);
                        }
                        else
                        {
                            CODING_ERROR_ASSERT(j % 2 == 1);
                        }
                    }
                    co_return;
                }(i));
            }

            co_await TaskUtilities<void>::WhenAll(tasks);
        }

        Awaitable<void> TestRemove(__in ULONG concurrencyLevel, __in ULONG numTasks, __in ULONG removesPerTask)
        {
            auto hashTable = CreateEmptyHashTable(concurrencyLevel);
            ULONG32 numKeys = 2 * numTasks * removesPerTask;

            for (LONG64 i = 0; i < numKeys; i++)
            {
                hashTable->Add(i, -i);
            }

            KArray<Awaitable<void>> tasks = KArray<Awaitable<void>>(GetAllocator(), numTasks);

            for (ULONG32 i = 0; i < numTasks; i++)
            {
                tasks.Append([&] (ULONG32 taskNum) -> Awaitable<void> {
                    co_await CorHelper::ThreadPoolThread(GetAllocator().GetKtlSystem().DefaultThreadPool());
                    for (LONG64 j = 0; j < removesPerTask; j++)
                    {
                        LONG64 val = 0;
                        // Only remove even keys
                        LONG64 key = 2 * (taskNum + j * numTasks);
                        bool removed = hashTable->TryRemove(key, val);
                        CODING_ERROR_ASSERT(removed);
                        CODING_ERROR_ASSERT(key == -val);
                    }
                    co_return;
                }(i));
            }

            co_await TaskUtilities<void>::WhenAll(tasks);

            KSharedArray<KeyValuePair<LONG64, LONG64>>::SPtr items = _new(ALLOC_TAG, GetAllocator()) KSharedArray<KeyValuePair<LONG64, LONG64>>();

            auto enumerator = hashTable->GetEnumerator();
            while (enumerator->MoveNext())
            {
                auto current = enumerator->Current();
                items->Append(current);

                CODING_ERROR_ASSERT(current.Key == -current.Value);

            }
            Sort<KeyValuePair<LONG64, LONG64>>::QuickSort(true, Compare, items);
            
            for (ULONG32 i = 0; i < (numTasks * removesPerTask); i++)
            {
                LONG64 expected = 2 * i + 1;
                CODING_ERROR_ASSERT(expected == (*items)[i].Key);
            }

            CODING_ERROR_ASSERT(numTasks * removesPerTask == hashTable->Count);
        }

    private:
      KtlSystem* ktlSystem_;
    };

    BOOST_FIXTURE_TEST_SUITE(ConcurrentHashTableStressTestSuite, ConcurrentHashTableStressTest)

    BOOST_AUTO_TEST_CASE(ConcurrentHashTable_AddReadUpdateReadDeleteRead_ShouldSucceed)
    {
        SyncAwait(ConcurrentHashTable_AddReadUpdateReadDeleteRead_Sequential_Test());
    }

    BOOST_AUTO_TEST_CASE(ConcurrentHashTable_ConcurrentAdd_ShouldSucceed)
    {
        SyncAwait(ConcurrentHashTable_ConcurrentAdd_Test());
    }

    BOOST_AUTO_TEST_CASE(ConcurrentHashTable_ConcurrentUpdate_ShouldSucceed)
    {
        SyncAwait(ConcurrentHashTable_ConcurrentUpdate_Test());
    }

    BOOST_AUTO_TEST_CASE(ConcurrentHashTable_ConcurrentRead_ShouldSucceed)
    {
        SyncAwait(ConcurrentHashTable_ConcurrentRead_Test());
    }

    BOOST_AUTO_TEST_CASE(ConcurrentHashTable_ConcurrentRemove_ShouldSucceed)
    {
        SyncAwait(ConcurrentHashTable_ConcurrentRemove_Test());
    }

    BOOST_AUTO_TEST_CASE(ConcurrentHashTable_ConcurrentRandomAdds_ShouldSucceed)
    {
        SyncAwait(ConcurrentHashTable_ConcurrentRandomAdds_Test());
    }

    BOOST_AUTO_TEST_CASE(ConcurrentHashTable_ConcurrentRandomAddOrUpdateAndRead_ShouldSucceed)
    {
        SyncAwait(ConcurrentHashTable_ConcurrentRandomAddOrUpdateAndRead_Test());
    }

    BOOST_AUTO_TEST_CASE(ConcurrentHashTable_Stress_ShouldSucceed)
    {
        SyncAwait(ConcurrentHashTable_Stress_Test());
    }

    BOOST_AUTO_TEST_CASE(ConcurrentHashTable_EnumerateSimple_ShouldSucceed)
    {
        ULONG numKeys = 1'000;
        LongLongHashTable::SPtr hashTableSPtr = CreateEmptyHashTable();

        KSharedArray<KeyValuePair<LONG64, LONG64>>::SPtr items = _new(ALLOC_TAG, GetAllocator()) KSharedArray<KeyValuePair<LONG64, LONG64>>();

        for (LONG64 i = 0; i < numKeys; i++)
        {
            hashTableSPtr->Add(i, i);
        }

        auto enumerator = hashTableSPtr->GetEnumerator();
        ULONG count = 0;
        while (enumerator->MoveNext())
        {
            items->Append(enumerator->Current());
            count++;
        }
        
        CODING_ERROR_ASSERT(count == numKeys);

        Sort<KeyValuePair<LONG64, LONG64>>::QuickSort(true, ConcurrentHashTableStressTest::Compare, items);

        for (LONG64 i = 0; i < items->Count(); i++)
        {
            auto item = (*items)[static_cast<ULONG>(i)];
            CODING_ERROR_ASSERT(item.Key == i);
            CODING_ERROR_ASSERT(item.Value == i);
        }
    }

    BOOST_AUTO_TEST_CASE(ConcurrentHashTable_Enumerate_ShouldSucceed)
    {
        LongLongHashTable::SPtr hashTableSPtr = CreateEmptyHashTable();
        auto enumerator = hashTableSPtr->GetEnumerator();
        ULONG count = 0;
        while (enumerator->MoveNext())
        {
            count++;
        }
        
        CODING_ERROR_ASSERT(count == 0);
    }

    BOOST_AUTO_TEST_CASE(ConcurrentHashTable_Enumerate_AfterEnd_ShouldSucceed)
    {
        ULONG numKeys = 1'000;
        LongLongHashTable::SPtr hashTableSPtr = CreateEmptyHashTable();

        for (LONG64 i = 0; i < numKeys; i++)
        {
            hashTableSPtr->Add(i, i);
        }

        auto enumerator = hashTableSPtr->GetEnumerator();
        ULONG count = 0;
        while (enumerator->MoveNext())
        {
            count++;
        }
        CODING_ERROR_ASSERT(count == numKeys);
        
        CODING_ERROR_ASSERT(enumerator->MoveNext() == false);
        CODING_ERROR_ASSERT(enumerator->MoveNext() == false);
    }

    BOOST_AUTO_TEST_CASE(ConcurrentHashTable_EnumerationAborted_LockShouldBeReleased)
    {
        ULONG numKeys = 1'000;
        LongLongHashTable::SPtr hashTableSPtr = CreateEmptyHashTable();

        for (LONG64 i = 0; i < numKeys; i++)
        {
            hashTableSPtr->Add(i, i);
        }

        auto enumerator = hashTableSPtr->GetEnumerator();
        enumerator->MoveNext(); // This will lock on bucket 0
        enumerator = nullptr;

        // This should not deadlock since the enumerator is disposed
        bool added = false;
        hashTableSPtr->AddOrUpdate(0, 1, added);
    }

    BOOST_AUTO_TEST_CASE(ConcurrentHashTable_ConcurrentEnumerations_ShouldSucceed)
    {
        ULONG numKeys = 1'000;
        LongLongHashTable::SPtr hashTableSPtr = CreateEmptyHashTable();

        for (LONG64 i = 0; i < numKeys; i++)
        {
            hashTableSPtr->Add(i, i);
        }

        auto enumerator1 = hashTableSPtr->GetEnumerator();
        auto enumerator2 = hashTableSPtr->GetEnumerator();
        
        ULONG count1 = 0;
        ULONG count2 = 0;
        
        while (true)
        {
            bool hasNext1 = enumerator1->MoveNext();
            bool hasNext2 = enumerator2->MoveNext();
            
            if (!hasNext1 && !hasNext2)
            {
                break;
            }
            
            count1 += hasNext1 ? 1 : 0;
            count2 += hasNext2 ? 1 : 0;
        }

        CODING_ERROR_ASSERT(count1 == numKeys);
        CODING_ERROR_ASSERT(count2 == numKeys);
    }

    BOOST_AUTO_TEST_CASE(TestAdd_ShouldSucceed)
    {
        SyncAwait(TestAdd( 1, 1, 10000));
        SyncAwait(TestAdd( 5, 1, 10000));
        SyncAwait(TestAdd( 1, 2,  5000));
        SyncAwait(TestAdd( 1, 5,  2000));
        SyncAwait(TestAdd( 4, 4,  2000));
        SyncAwait(TestAdd(16, 4,  2000));
        SyncAwait(TestAdd(64, 5,  5000));
        SyncAwait(TestAdd( 5, 5,  2500));
    }

    BOOST_AUTO_TEST_CASE(TestUpdate_ShouldSucceed)
    {
        SyncAwait(TestUpdate( 1, 1, 10000));
        SyncAwait(TestUpdate( 5, 1, 10000));
        SyncAwait(TestUpdate( 1, 2,  5000));
        SyncAwait(TestUpdate( 1, 5,  2001));
        SyncAwait(TestUpdate( 4, 4,  2001));
        SyncAwait(TestUpdate(15, 5,  2001));
        SyncAwait(TestUpdate(64, 5,  5000));
        SyncAwait(TestUpdate( 5, 5, 25000));
    }

    BOOST_AUTO_TEST_CASE(TestRead_ShouldSucceed)
    {
        SyncAwait(TestRead( 1, 1, 10000));
        SyncAwait(TestRead( 5, 1, 10000));
        SyncAwait(TestRead( 1, 2,  5000));
        SyncAwait(TestRead( 1, 5,  2001));
        SyncAwait(TestRead( 4, 4,  2001));
        SyncAwait(TestRead(15, 5,  2001));
        SyncAwait(TestRead(64, 5,  5000));
        SyncAwait(TestRead( 5, 5, 25000));
    }

    BOOST_AUTO_TEST_CASE(TestRemove_ShouldSucceed)
    {
        SyncAwait(TestRemove( 1, 1, 10000));
        SyncAwait(TestRemove( 5, 1, 10000));
        SyncAwait(TestRemove( 1, 2,  5000));
        SyncAwait(TestRemove( 1, 5,  2001));
        SyncAwait(TestRemove( 4, 4,  2001));
        SyncAwait(TestRemove(15, 5,  2001));
        SyncAwait(TestRemove(64, 5,  5000));
        SyncAwait(TestRemove( 5, 5, 25000));
    }

    BOOST_AUTO_TEST_SUITE_END()
}

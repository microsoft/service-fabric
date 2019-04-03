// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#define ALLOC_TAG 'tlSF'

namespace TStoreTests
{
    using namespace Common;
    using namespace ktl;
    using namespace Data::TStore;

    class FastSkipListTest
    {
    public:
        typedef ConcurrentSkipList<LONG64, LONG64> LongLongSkipList;

        enum OperationType
        {
           Read,
           Update,
           Add,
           Remove,
        };

        struct SkipListOperation
        {
            OperationType Type;
            LONG64 Key;
            // Value to set for Add/Update operationsSPtr. Expected value for read/remove operationsSPtr
            LONG64 Value; 

            bool ShouldSucceed = true; 
        };

        FastSkipListTest()
        {
            NTSTATUS status;
            status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            ktlSystem_->SetStrictAllocationChecks(TRUE);
        }

        ~FastSkipListTest()
        {
            ktlSystem_->Shutdown();
        }

        Common::Random GetRandom()
        {
            auto seed = Common::Stopwatch::Now().Ticks;
            Common::Random random(static_cast<int>(seed));
            cout << "Random seed (use this seed to reproduce test failures): " << seed << endl;
            return random;
        }

        KAllocator& GetAllocator()
        {
            return ktlSystem_->NonPagedAllocator();
        }

        LongLongSkipList::SPtr CreateEmptySkipList()
        {
            LongComparer::SPtr longComparer = nullptr;
            NTSTATUS status = LongComparer::Create(GetAllocator(), longComparer);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            IComparer<LONG64>::SPtr comparer = static_cast<IComparer<LONG64> *>(longComparer.RawPtr());

            LongLongSkipList::SPtr listSPtr = nullptr;
            status = LongLongSkipList::Create(comparer, GetAllocator(), listSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            return listSPtr;
        }

        ktl::Awaitable<void> ApplyOperationsAsync(
            __in LongLongSkipList & list,
            __in KSharedArray<SkipListOperation> & items,
            __in ULONG offset,
            __in ULONG count,
            __in bool verifyContainsKey=true,
            __in bool verifyValue=true)
        {
            co_await ktl::CorHelper::ThreadPoolThread(ktlSystem_->DefaultThreadPool());

            for (ULONG i = offset; i < offset + count; i++)
            {
                bool successful = false;
                LONG64 outValue = -1;

                SkipListOperation item = items[i];
                switch (item.Type)
                {
                case OperationType::Add:
                    successful = list.TryAdd(item.Key, item.Value);
                    break;
                case OperationType::Update:
                    list.Update(item.Key, item.Value);
                    successful = true;
                    break;
                case OperationType::Remove:
                    successful = list.TryRemove(item.Key, outValue);
                    break;
                case OperationType::Read:
                    successful = list.TryGetValue(item.Key, outValue);
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

        ktl::Awaitable<void> ApplyOperationsAsync(
           __in LongLongSkipList & list,
           __in KSharedArray<SkipListOperation> & items,
           __in ULONG numTasks,
           __in bool verifyContainsKey = true,
           __in bool verifyValue = true)
        {
           KSharedArray<ktl::Awaitable<void>>::SPtr tasks = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ktl::Awaitable<void>>();

           const ULONG keysPerTask = items.Count() / numTasks;

           for (ULONG i = 0; i < numTasks; i++)
           {
              tasks->Append(ApplyOperationsAsync(list, items, i * keysPerTask, keysPerTask, verifyContainsKey, verifyValue));
           }

           co_await StoreUtilities::WhenAll(*tasks, GetAllocator());
        }

        static LONG32 CompareOperations(__in SkipListOperation const & KeyOne, __in SkipListOperation const & KeyTwo)
        {
            if (KeyOne.Key < KeyTwo.Key)
            {
                return -1;
            }

            if (KeyOne.Key > KeyTwo.Key)
            {
                return 1;
            }
            
            return 0;
        }

    private:
        KtlSystem* ktlSystem_;

#pragma region test functions
    public:
        ktl::Awaitable<void> FastSkipList_AddReadUpdateReadDeleteRead_Sequential_ShouldSucceed_Test()
        {
            const LONG64 numKeys = 2'500;
            const ULONG numTasks = 1; // Sequential test

            KSharedArray<SkipListOperation>::SPtr operationsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<SkipListOperation>();
            for (LONG64 key = 0; key < numKeys; key++)
            {
                SkipListOperation operation = { OperationType::Add, key, key };
                operationsSPtr->Append(operation);
            }

            for (LONG64 key = 0; key < numKeys; key++)
            {
                SkipListOperation operation = { OperationType::Read, key, key };
                operationsSPtr->Append(operation);
            }

            for (LONG64 key = 0; key < numKeys; key++)
            {
                SkipListOperation operation = { OperationType::Update, key, key + 10 };
                operationsSPtr->Append(operation);
            }

            for (LONG64 key = 0; key < numKeys; key++)
            {
                SkipListOperation operation = { OperationType::Read, key, key + 10 };
                operationsSPtr->Append(operation);
            }

            for (LONG64 key = 0; key < numKeys; key++)
            {
                SkipListOperation operation = { OperationType::Remove, key, key + 10 };
                operationsSPtr->Append(operation);
            }

            for (LONG64 key = 0; key < numKeys; key++)
            {
                SkipListOperation operation = { OperationType::Read, key, 0, false };
                operationsSPtr->Append(operation);
            }

            LongLongSkipList::SPtr listSPtr = CreateEmptySkipList();
            co_await ApplyOperationsAsync(*listSPtr, *operationsSPtr, numTasks);
            co_return;
        }

        ktl::Awaitable<void> FastSkipList_ConcurrentAdd_ShouldSucceed_Test()
        {
            const LONG64 numKeys = 10'000;

            const ULONG numAddTasks = 50;
            const ULONG numReadTasks = 1; // Verify sequentially

            KSharedArray<SkipListOperation>::SPtr addOperationsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<SkipListOperation>();
            for (LONG64 key = 0; key < numKeys; key++)
            {
                SkipListOperation operation = { OperationType::Add, key, key };
                addOperationsSPtr->Append(operation);
            }

            KSharedArray<SkipListOperation>::SPtr readOperationsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<SkipListOperation>();
            for (LONG64 key = 0; key < numKeys; key++)
            {
                SkipListOperation operation = { OperationType::Read, key, key};
                readOperationsSPtr->Append(operation);
            }

            LongLongSkipList::SPtr listSPtr = CreateEmptySkipList();
            co_await ApplyOperationsAsync(*listSPtr, *addOperationsSPtr, numAddTasks);
            co_await ApplyOperationsAsync(*listSPtr, *readOperationsSPtr, numReadTasks);
            co_return;
        }

        ktl::Awaitable<void> FastSkipList_ConcurrentUpdate_ShouldSucceed_Test()
        {
            const LONG64 numKeys = 10'000;

            const ULONG numWriteTasks = 50;
            const ULONG numReadTasks = 1; // Verify sequentially

            KSharedArray<SkipListOperation>::SPtr addOperationsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<SkipListOperation>();
            for (LONG64 key = 0; key < numKeys; key++)
            {
                SkipListOperation operation = { OperationType::Add, key, key };
                addOperationsSPtr->Append(operation);
            }

            KSharedArray<SkipListOperation>::SPtr updateOperationsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<SkipListOperation>();
            for (LONG64 key = 0; key < numKeys; key++)
            {
                SkipListOperation operation = { OperationType::Update, key, key + 10 };
                updateOperationsSPtr->Append(operation);
            }

            KSharedArray<SkipListOperation>::SPtr readOperationsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<SkipListOperation>();
            for (LONG64 key = 0; key < numKeys; key++)
            {
                SkipListOperation operation = { OperationType::Read, key, key + 10};
                readOperationsSPtr->Append(operation);
            }

            LongLongSkipList::SPtr listSPtr = CreateEmptySkipList();
            co_await ApplyOperationsAsync(*listSPtr, *addOperationsSPtr, numWriteTasks);
            co_await ApplyOperationsAsync(*listSPtr, *updateOperationsSPtr, numWriteTasks);
            co_await ApplyOperationsAsync(*listSPtr, *readOperationsSPtr, numReadTasks);
            co_return;
        }

        ktl::Awaitable<void> FastSkipList_ConcurrentRead_ShouldSucceed_Test()
        {
            const LONG64 numKeys = 10'000;
            const ULONG numTasks = 50;

            KSharedArray<SkipListOperation>::SPtr addOperationsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<SkipListOperation>();
            for (LONG64 key = 0; key < numKeys; key++)
            {
                SkipListOperation operation = { OperationType::Add, key, key };
                addOperationsSPtr->Append(operation);
            }

            KSharedArray<SkipListOperation>::SPtr readOperationsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<SkipListOperation>();
            for (LONG64 key = 0; key < numKeys; key++)
            {
                SkipListOperation operation = { OperationType::Read, key, key};
                readOperationsSPtr->Append(operation);
            }

            LongLongSkipList::SPtr listSPtr = CreateEmptySkipList();
            co_await ApplyOperationsAsync(*listSPtr, *addOperationsSPtr, numTasks);
            co_await ApplyOperationsAsync(*listSPtr, *readOperationsSPtr, numTasks);
            co_return;
        }

        ktl::Awaitable<void> FastSkipList_ConcurrentRandomAdds_ShouldSucceed_Test()
        {
            LongLongSkipList::SPtr listSPtr = CreateEmptySkipList();

            Common::Random random = GetRandom();

            const ULONG numKeys = 10'000;
            const ULONG numTasks = 50;

            KAutoHashTable<LONG64, bool> keysSet(100, K_DefaultHashFunction, GetAllocator());

            KSharedArray<SkipListOperation>::SPtr writeOperationsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<SkipListOperation>();
            KSharedArray<SkipListOperation>::SPtr readOperationsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<SkipListOperation>();

            for (ULONG i = 0; i < numKeys; i++)
            {
                LONG64 key = random.Next(1'000'000);

                if (keysSet.ContainsKey(key))
                {
                    i--;
                    continue;
                }

                keysSet.Put(key, true);

                LONG64 value = random.Next(1'000'000);

                SkipListOperation writeOperation = { OperationType::Add, key, value };
                writeOperationsSPtr->Append(writeOperation);

                SkipListOperation readOperation = { OperationType::Read, key, value };
                readOperationsSPtr->Append(readOperation);
            }

            // Write in random order
            co_await ApplyOperationsAsync(*listSPtr, *writeOperationsSPtr, numTasks);

            // Read in random order
            co_await ApplyOperationsAsync(*listSPtr, *readOperationsSPtr, numTasks);

            // Sort read operations
            Sort<SkipListOperation>::QuickSort(true, FastSkipListTest::CompareOperations, readOperationsSPtr);

            // Read in sorted order
            co_await ApplyOperationsAsync(*listSPtr, *readOperationsSPtr, numTasks);
            co_return;
        }

        ktl::Awaitable<void> FastSkipList_ConcurrentRandomUpdateAndRead_ShouldSucceed_Test()
        {
            LongLongSkipList::SPtr listSPtr = CreateEmptySkipList();

            Common::Random random = GetRandom();

            const ULONG numKeys = 10'000;
            const ULONG numTasks = 50;
            const int maxKey = 100; // High contention

            KSharedArray<SkipListOperation>::SPtr operationsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<SkipListOperation>();

            for (LONG64 key = 0; key < numKeys; key++)
            {
               SkipListOperation operation = { OperationType::Add, key, key };
               operationsSPtr->Append(operation);
            }

            co_await ApplyOperationsAsync(*listSPtr, *operationsSPtr, numTasks);

            operationsSPtr->Clear();

            // Concurrent reads and updates.
            for (ULONG i = 0; i < numKeys; i++)
            {
                OperationType opType = static_cast<OperationType>(random.Next(2)); // Pick updates or reads.
                LONG64 key = random.Next(maxKey);
                LONG64 value = random.Next(maxKey);

                SkipListOperation operation = { opType, key, value };
                operationsSPtr->Append(operation);

            }

            // Update can override the value before read.
            bool shouldVerifyContainsKey = true;
            bool shouldVerifyValue = false;
            co_await ApplyOperationsAsync(*listSPtr, *operationsSPtr, numTasks, shouldVerifyContainsKey, shouldVerifyValue);
            co_return;
        }

        ktl::Awaitable<void> FastSkipList_ConcurrentRandomAddAndRead_ShouldSucceed_Test()
        {
           LongLongSkipList::SPtr listSPtr = CreateEmptySkipList();

           Common::Random random = GetRandom();

           const ULONG numKeys = 10'000;
           const ULONG numTasks = 50;
           const int maxKey = 100; // High contention

           KSharedArray<SkipListOperation>::SPtr operationsSPtr = _new(ALLOC_TAG, GetAllocator()) KSharedArray<SkipListOperation>();

           // Concurrent reads and updates.
           for (ULONG i = 0; i < numKeys; i++)
           {
              OperationType opType = OperationType::Add;
              if (i % 2 == 0)
              {
                 opType = OperationType::Read;
              }

              LONG64 key = random.Next(maxKey);
              LONG64 value = random.Next(maxKey);

              SkipListOperation operation = { opType, key, value };
              operationsSPtr->Append(operation);

           }

           // Update can override the value before read.
           bool shouldVerifyContainsKey = false;
           bool shouldVerifyValue = false;
           co_await ApplyOperationsAsync(*listSPtr, *operationsSPtr, numTasks, shouldVerifyContainsKey, shouldVerifyValue);
            co_return;
        }
    #pragma endregion
    };

    BOOST_FIXTURE_TEST_SUITE(FastSkipListTestSuite, FastSkipListTest)

    BOOST_AUTO_TEST_CASE(FastSkipList_AddReadUpdateReadDeleteRead_Sequential_ShouldSucceed)
    {
        SyncAwait(FastSkipList_AddReadUpdateReadDeleteRead_Sequential_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(FastSkipList_ConcurrentAdd_ShouldSucceed)
    {
        SyncAwait(FastSkipList_ConcurrentAdd_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(FastSkipList_ConcurrentUpdate_ShouldSucceed)
    {
        SyncAwait(FastSkipList_ConcurrentUpdate_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(FastSkipList_ConcurrentRead_ShouldSucceed)
    {
        SyncAwait(FastSkipList_ConcurrentRead_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(FastSkipList_ConcurrentRandomAdds_ShouldSucceed)
    {
        SyncAwait(FastSkipList_ConcurrentRandomAdds_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(FastSkipList_ConcurrentRandomUpdateAndRead_ShouldSucceed)
    {
        SyncAwait(FastSkipList_ConcurrentRandomUpdateAndRead_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(FastSkipList_ConcurrentRandomAddAndRead_ShouldSucceed)
    {
        SyncAwait(FastSkipList_ConcurrentRandomAddAndRead_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_SUITE_END()
}

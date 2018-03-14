// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'lsTP'

inline bool StringEquals(__in KString::SPtr & one, __in KString::SPtr & two)
{
    if (one == nullptr || two == nullptr)
    {
        return one == two;
    }

    return one->Compare(*two) == 0;
}

namespace TStoreTests
{
    using namespace ktl;
    using namespace Data::Utilities;

    class StoreStressTest : public TStoreTestBase<LONG64, KString::SPtr, LongComparer, TestStateSerializer<LONG64>, StringStateSerializer>
    {
    public:
        StoreStressTest()
        {
            Setup(3);
        }

        ~StoreStressTest()
        {
            Cleanup();
        }

        Common::Random GetRandom()
        {
            auto seed = Common::Stopwatch::Now().Ticks;
            Common::Random random(static_cast<int>(seed));
            return random;
        }

        KString::SPtr GenerateStringValue(__in const WCHAR* str)
        {
            KString::SPtr value;
            auto status = KString::Create(value, GetAllocator(), str);
            KInvariant(NT_SUCCESS(status));
            return value;
        }

        KString::SPtr GenerateStringValue(__in ULONG32 seed)
        {
            KString::SPtr value;
            wstring str = to_wstring(seed);
            auto status = KString::Create(value, GetAllocator(), str.c_str());
            KInvariant(NT_SUCCESS(status));
            return value;
        }

        ktl::Awaitable<void> AddKeysAsync(__in ULONG32 numOfIterations, __in ULONG32 startKey)
        {
            auto tx = CreateWriteTransaction();
            for (ULONG32 i = 0; i < numOfIterations; i++)
            {
                ULONG32 key = startKey + i;
                KString::SPtr value = GenerateStringValue(key);
                co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None);
            }

            co_await tx->CommitAsync();
            co_return;
        }

        ktl::Awaitable<void> UpdateKeysAsync(__in ULONG32 numOfIterations, __in ULONG32 startKey)
        {
            auto tx = CreateWriteTransaction();
            for (ULONG32 i = 0; i < numOfIterations; i++)
            {
                ULONG32 key = startKey + i;
                KString::SPtr value = GenerateStringValue(key * 11);
                co_await Store->ConditionalUpdateAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None);
            }

            co_await tx->CommitAsync();
            co_return;
        }

        ktl::Awaitable<void> RemoveKeysAsync(__in ULONG32 numOfIterations, __in ULONG32 startKey)
        {
            auto tx = CreateWriteTransaction();
            for (ULONG32 i = 0; i < numOfIterations; i++)
            {
                ULONG32 key = startKey + i;
                co_await Store->ConditionalRemoveAsync(*tx->StoreTransactionSPtr, key, DefaultTimeout, ktl::CancellationToken::None);
            }

            co_await tx->CommitAsync();
            co_return;
        }

        ktl::Awaitable<void> UpdateKeysOverlappingAsync(
            __in ULONG32 numIterations,
            __in ULONG32 keysUpdatedPerTransaction,
            __in LONG64 keyCount,
            __in LONG64 & iterationNumber,
            __in ConcurrentDictionary<LONG64, Dictionary<LONG64, KString::SPtr>::SPtr> & operations)
        {
            ConcurrentDictionary<LONG64, Dictionary<LONG64, KString::SPtr>::SPtr>::SPtr operationsSPtr = &operations;
            Common::Random random = GetRandom();

            KSharedArray<LONG64>::SPtr keys = _new(ALLOC_TAG, GetAllocator()) KSharedArray<LONG64>();

            for (ULONG32 i = 0; i < numIterations; i++)
            {
                for (ULONG32 k = 0; k < keysUpdatedPerTransaction; k++)
                {
                    keys->Append(random.Next(0, static_cast<int>(keyCount)));
                }

                // Sort keys, so we always take write locks in the same order to avoid deadlocks
                Sorter<LONG64>::QuickSort(true, *Store->KeyComparerSPtr, keys);

                // Update the keys
                {
                    auto txn = CreateWriteTransaction();

                    // Track the updates that are applied to each key
                    Dictionary<LONG64, KString::SPtr>::SPtr updatesSPtr = nullptr;
                    NTSTATUS status = Dictionary<LONG64, KString::SPtr>::Create(100, K_DefaultHashFunction, *Store->KeyComparerSPtr, GetAllocator(), updatesSPtr);
                    CODING_ERROR_ASSERT(NT_SUCCESS(status));

                    for (ULONG32 k = 0; k < keys->Count(); k++)
                    {
                        LONG64 key = (*keys)[k];
                        KString::SPtr value = GenerateStringValue(static_cast<ULONG32>(InterlockedIncrement64(&iterationNumber)));

                        co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None);
                        updatesSPtr->AddOrUpdate(key, value);
                    }

                    if (random.Next() % 10 > 0)
                    {
                        // Store the updates that were applied as part of this commit
                        LONG64 commitSequenceNumber = co_await txn->CommitAsync();
                        operationsSPtr->Add(commitSequenceNumber, updatesSPtr);
                    }
                    else
                    {
                        // Every so often, abort a transaction
                        co_await txn->AbortAsync();
                    }
                }
            }
        }

        Common::CommonConfig config; // load the config object as it's needed for the tracing to work
    };

    BOOST_FIXTURE_TEST_SUITE(StoreStressTestSuite, StoreStressTest)

    BOOST_AUTO_TEST_CASE(AddAsync_ConcurrentNonOverlapping_ShouldSucceed, *boost::unit_test_framework::disabled(/*Should be moved under perf and stress testsn*/))
    {
        const ULONG32 numOfTasks = 100;
        const ULONG32 numOfIterations = 1000;
        KAllocator& allocator = StoreStressTest::GetAllocator();
        KArray<ktl::Awaitable<void>> tasks(allocator, numOfTasks);

        // Concurrently add keys from multiple tasks, with no overlapping writes.
        for (ULONG32 t = 0; t < numOfTasks; t++)
        {
            ULONG32 startKey = (t * numOfIterations);
            tasks.Append(AddKeysAsync(numOfIterations, startKey));
        }

        // Wait for all concurrent tasks to complete
        SyncAwait(TaskUtilities<ktl::Awaitable<void>>::WhenAll(tasks));

        // Verify the concurrent AddAsync succeeded on all replicas
        for (ULONG32 j = 0; j < Stores->Count(); j++)
        {
            auto store = (*Stores)[j];
            VerifyCount(*store, numOfTasks * numOfIterations);
            auto tx = CreateWriteTransaction(*store);
            for (ULONG32 t = 0; t < numOfTasks; t++)
            {
                for (ULONG32 i = 0; i < numOfIterations; i++)
                {
                    ULONG32 key = (t * numOfIterations) + i;
                    KString::SPtr expectedValue = GenerateStringValue(key);
                    SyncAwait(VerifyKeyExistsAsync(*store, *tx->StoreTransactionSPtr, key, nullptr, expectedValue, StringEquals));
                }
            }
            
            SyncAwait(tx->AbortAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(RemoveAsync_ConcurrentNonOverlapping_ShouldSucceed)
    {
        const ULONG32 numOfTasks = 100;
        const ULONG32 numOfIterations = 1000;
        KAllocator& allocator = StoreStressTest::GetAllocator();
        KArray<ktl::Awaitable<void>> tasks(allocator, numOfTasks);

        // Concurrently add keys from multiple tasks, with no overlapping writes.
        for (ULONG32 t = 0; t < numOfTasks; t++)
        {
            for (ULONG32 i = 0; i < numOfIterations; i++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    LONG64 key = (t * numOfIterations) + i;
                    KString::SPtr value = GenerateStringValue(i);
                    SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None));
                    SyncAwait(txn->CommitAsync());
                }
            }
        }

        // Concurrently remove keys from multiple tasks, with no overlapping writes.
        for (ULONG32 t = 0; t < numOfTasks; t++)
        {
            ULONG32 startKey = (t * numOfIterations);
            tasks.Append(RemoveKeysAsync(numOfIterations, startKey));
        }

        // Wait for all concurrent tasks to complete
        SyncAwait(TaskUtilities<ktl::Awaitable<void>>::WhenAll(tasks));

        // Verify the concurrent CondtionalRemoveAsync succeeded on all replicas
        for (ULONG32 j = 0; j < Stores->Count(); j++)
        {
            auto store = (*Stores)[j];
            VerifyCount(*store, 0);
            auto tx = CreateWriteTransaction(*store);
            for (ULONG32 t = 0; t < numOfTasks; t++)
            {
                for (ULONG32 i = 0; i < numOfIterations; i++)
                {
                    ULONG32 key = (t * numOfIterations) + i;
                    SyncAwait(VerifyKeyDoesNotExistAsync(*store, *tx->StoreTransactionSPtr, key));
                }
            }
            
            SyncAwait(tx->AbortAsync());
        }

    }

    BOOST_AUTO_TEST_CASE(UpdateAsync_ConcurrentNonOverlapping_ShouldSucceed)
    {
        const ULONG32 numOfTasks = 100;
        const ULONG32 numOfIterations = 1000;
        KAllocator& allocator = StoreStressTest::GetAllocator();
        KArray<ktl::Awaitable<void>> tasks(allocator, numOfTasks);

        for (ULONG32 t = 0; t < numOfTasks; t++)
        {
            for (ULONG32 i = 0; i < numOfIterations; i++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    LONG64 key = (t * numOfIterations) + i;
                    KString::SPtr value = GenerateStringValue(i);
                    SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None));
                    SyncAwait(txn->CommitAsync());
                }
            }
        }

        // Concurrently remove keys from multiple tasks, with no overlapping writes.
        for (ULONG32 t = 0; t < numOfTasks; t++)
        {
            ULONG32 startKey = (t * numOfIterations);
            tasks.Append(UpdateKeysAsync(numOfIterations, startKey));
        }

        // Wait for all concurrent tasks to complete
        SyncAwait(TaskUtilities<ktl::Awaitable<void>>::WhenAll(tasks));

        // Verify the concurrent CondtionalRemoveAsync succeeded on all replicas
        for (ULONG32 j = 0; j < Stores->Count(); j++)
        {
            auto store = (*Stores)[j];
            VerifyCount(*store, numOfTasks * numOfIterations);
            auto tx = CreateWriteTransaction(*store);
            for (ULONG32 t = 0; t < numOfTasks; t++)
            {
                for (ULONG32 i = 0; i < numOfIterations; i++)
                {
                    ULONG32 key = (t * numOfIterations) + i;
                    KString::SPtr expectedValue = GenerateStringValue(key * 11);
                    SyncAwait(VerifyKeyExistsAsync(*store, *tx->StoreTransactionSPtr, key, nullptr, expectedValue, StringEquals));
                }
            }
            
            SyncAwait(tx->AbortAsync());
        }

    }

    BOOST_AUTO_TEST_CASE(UpdateAsync_ConcurrentOverlapping_ShouldSucceed)
    {
        const ULONG32 numTasks = 64;
        const LONG64 keyCount = 1000;
        const ULONG32 keysUpdatedPerTransaction = 5;
        const ULONG32 iterations = 100;
        
        // Sequence number to generate unique values for verification
        LONG64 iterationNumber = 0;

        Dictionary<LONG64, KString::SPtr>::SPtr expectedSPtr = nullptr;
        NTSTATUS status = Dictionary<LONG64, KString::SPtr>::Create(1000, K_DefaultHashFunction, *Store->KeyComparerSPtr, GetAllocator(), expectedSPtr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        // Add initial keys to operate on concurrently
        {
            auto txn = CreateWriteTransaction();
            for (LONG64 key = 0; key < keyCount; key++)
            {
                {
                    KString::SPtr value = GenerateStringValue(static_cast<ULONG32>(key));
                    SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None));
                    expectedSPtr->AddOrUpdate(key, value);
                }
            }
            SyncAwait(txn->CommitAsync());
        }

        // We will use the commit sequence numbers to observe the order the updates were applied to compute the expectedValue final state
        ConcurrentDictionary<LONG64, Dictionary<LONG64, KString::SPtr>::SPtr>::SPtr operationsSPtr = nullptr; // { LSN -> { key -> value }}
        status = ConcurrentDictionary<LONG64, Dictionary<LONG64, KString::SPtr>::SPtr>::Create(GetAllocator(), operationsSPtr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        // Concurrently update keys from multiple tasks, with overlapping writes
        KArray<ktl::Awaitable<void>> tasks(GetAllocator(), numTasks);
        for (ULONG32 t = 0; t < numTasks; t++)
        {
            tasks.Append(UpdateKeysOverlappingAsync(iterations, keysUpdatedPerTransaction, keyCount, iterationNumber, *operationsSPtr));
        }

        // Wait for all concurrent tasks to complete
        SyncAwait(TaskUtilities<ktl::Awaitable<void>>::WhenAll(tasks));

        // Compute the expectedValue final state. We sort on the commit sequence number and apply the updates
        // in order. This order represents the order the transactions were 'atomically' applied.
        KSharedArray<LONG64>::SPtr sequenceNumbers = _new(ALLOC_TAG, GetAllocator()) KSharedArray<LONG64>();

        auto operationsEnumeratorSPtr = operationsSPtr->GetEnumerator();
        while (operationsEnumeratorSPtr->MoveNext())
        {
            auto item = operationsEnumeratorSPtr->Current();
            sequenceNumbers->Append(item.Key);
        }
        Sorter<LONG64>::QuickSort(true, *Store->KeyComparerSPtr, sequenceNumbers); // Exploiting the fact that the store already has a LONG comparer
        
        for (ULONG32 i = 0; i < sequenceNumbers->Count(); i++)
        {
            LONG64 commitSequenceNumber = (*sequenceNumbers)[i];
            Dictionary<LONG64, KString::SPtr>::SPtr transactionOperationsSPtr = nullptr;
            bool found = operationsSPtr->TryGetValue(commitSequenceNumber, transactionOperationsSPtr);
            CODING_ERROR_ASSERT(found);
            
            auto transactionOperationsEnumeratorSPtr = transactionOperationsSPtr->GetEnumerator();
            while (transactionOperationsEnumeratorSPtr->MoveNext())
            {
                auto item = transactionOperationsEnumeratorSPtr->Current();
                LONG64 key = item.Key;
                KString::SPtr value = item.Value;
                expectedSPtr->AddOrUpdate(key, value);
            }
        }

        // Verify the TStore ended up in the expectedValue state on all replicas
        for (ULONG32 s = 0; s < Stores->Count(); s++)
        {
            auto store = (*Stores)[s];
            VerifyCount(*store, keyCount);

            {
                auto txn = CreateWriteTransaction();
                for (LONG64 key = 0; key < keyCount; key++)
                {
                    KString::SPtr expectedValue = nullptr;
                    bool found = expectedSPtr->TryGetValue(key, expectedValue);
                    CODING_ERROR_ASSERT(found);
                    SyncAwait(VerifyKeyExistsAsync(*store, key, nullptr, expectedValue, StringEquals));
                }

                SyncAwait(txn->AbortAsync());
            }
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}

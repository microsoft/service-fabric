// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'tsPC'

namespace TStoreTests
{
    using namespace ktl;
    using namespace Common;

    class EnumerationStressTest : public TStoreTestBase<LONG64, LONG64, LongComparer, TestStateSerializer<LONG64>, TestStateSerializer<LONG64>>
    {
    public:
        EnumerationStressTest()
        {
            Setup(1);
        }

        ~EnumerationStressTest()
        {
            Cleanup();
        }

        Task CheckpointTask(__in AwaitableCompletionSource<void> & completionSignal)
        {
            co_await Store->PerformCheckpointAsync(CancellationToken::None);
            co_await Store->CompleteCheckpointAsync(CancellationToken::None);
            completionSignal.Set();
        }

        AwaitableCompletionSource<void>::SPtr StartCheckpoint()
        {
            AwaitableCompletionSource<void>::SPtr completionSignal = nullptr;
            AwaitableCompletionSource<void>::Create(GetAllocator(), ALLOC_TAG, completionSignal);

            LONG64 checkpointLSN = Replicator->IncrementAndGetCommitSequenceNumber();
            Store->PrepareCheckpoint(checkpointLSN);
            CheckpointTask(*completionSignal);
            
            return completionSignal;
        }

        ktl::Awaitable<void> PopulateAsync(
            __in ULONG32 count, 
            __in RandomGenerator & random,
            __in FastSkipList<LONG64, LONG64> & resultsDict)
        {
            co_await CorHelper::ThreadPoolThread(GetAllocator().GetKtlSystem().DefaultThreadPool());

            for (ULONG32 i = 0; i < count; i++)
            {
                LONG64 key = static_cast<LONG64>(random.Next());
                {
                    auto txn = CreateWriteTransaction();

                    LONG64 value = static_cast<LONG64>(random.Next());

                    txn->StoreTransactionSPtr->LockingHints = LockingHints::Enum::UpdateLock;
                    bool exists = co_await Store->ContainsKeyAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None);

                    if (exists)
                    {
                        co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                        resultsDict.Update(key, value);
                    }
                    else
                    {
                        co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                        resultsDict.Add(key, value);
                    }

                    co_await txn->CommitAsync();
                }
            }
        }

        ktl::Awaitable<void> PopulateAsync(
            __in ULONG32 count,
            __in RandomGenerator & random,
            __in FastSkipList<LONG64, LONG64> & resultsDict,
            __in ULONG32 numTasks)
        {
            ULONG32 countPerTask = count / numTasks;

            KSharedArray<Awaitable<void>>::SPtr tasks = _new(ALLOC_TAG, GetAllocator()) KSharedArray<Awaitable<void>>;
            for (ULONG32 i = 0; i < numTasks; i++)
            {
                tasks->Append(PopulateAsync(countPerTask, random, resultsDict));
            }
            
            co_await StoreUtilities::WhenAll(*tasks, GetAllocator());
        }

        void Update(__in FastSkipList<LONG64, LONG64> & resultsDict)
        {
            auto enumerator = resultsDict.GetEnumerator();
            while (enumerator->MoveNext())
            {
                auto row = enumerator->Current();

                {
                    auto txn = CreateWriteTransaction();
                    SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, row.Key, row.Key + 1, DefaultTimeout, CancellationToken::None));
                    SyncAwait(txn->CommitAsync());
                }
            }
        }

        void Remove(__in FastSkipList<LONG64, LONG64> & resultsDict)
        {
            auto enumerator = resultsDict.GetEnumerator();
            while (enumerator->MoveNext())
            {
                auto row = enumerator->Current();

                {
                    auto txn = CreateWriteTransaction();
                    bool removed = SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, row.Key, DefaultTimeout, CancellationToken::None));
                    CODING_ERROR_ASSERT(removed);
                }
            }
        }
        
        ktl::Awaitable<void> VerifyEnumerationAsync(
            __in WriteTransaction<LONG64, LONG64> & txn,
            __in FastSkipList<LONG64, LONG64> & expectedState)
        {
            KArray<KeyValuePair<LONG64, LONG64>> expectedItems(GetAllocator());
            KArray<KeyValuePair<LONG64, LONG64>> actualItems(GetAllocator());

            auto expectedEnumerator = expectedState.GetEnumerator();
            while (expectedEnumerator->MoveNext())
            {
                expectedItems.Append(expectedEnumerator->Current());
            }

            auto enumerable = co_await Store->CreateEnumeratorAsync(*txn.StoreTransactionSPtr);
            while (co_await enumerable->MoveNextAsync(CancellationToken::None))
            {
                auto item = enumerable->GetCurrent();
                actualItems.Append(KeyValuePair<LONG64, LONG64>(item.Key, item.Value.Value));
            }

            CODING_ERROR_ASSERT(expectedItems.Count() == actualItems.Count());

            for (ULONG32 i = 0; i < expectedItems.Count(); i++)
            {
                auto expected = expectedItems[i];
                auto actual = actualItems[i];

                CODING_ERROR_ASSERT(expected.Key == actual.Key);
                CODING_ERROR_ASSERT(expected.Value == actual.Value);
            }
        }

    private:
        CommonConfig config; // load the config object as it's needed for the tracing to work
    };

    BOOST_FIXTURE_TEST_SUITE(EnumerationStressTestSuite, EnumerationStressTest)

    BOOST_AUTO_TEST_CASE(Enumeration_Stress_RandomItemsInEachComponent_VerifyAllExistInEnumeration)
    {
        ULONG32 NumberOfItemsPerComponent = 256'000;

        RandomGenerator random;

        FastSkipList<LONG64, LONG64>::SPtr expectedState = nullptr;
        FastSkipList<LONG64, LONG64>::Create(Store->KeyComparerSPtr, GetAllocator(), expectedState);

        // Populate consolidated state
        SyncAwait(PopulateAsync(NumberOfItemsPerComponent, random, *expectedState, 200));
        Checkpoint();
        SyncAwait(PopulateAsync(NumberOfItemsPerComponent, random, *expectedState, 200));
        Checkpoint();
        SyncAwait(PopulateAsync(NumberOfItemsPerComponent, random, *expectedState, 200));
        Checkpoint();

        // Setup Delta Differentials
        SyncAwait(PopulateAsync(NumberOfItemsPerComponent, random, *expectedState, 200));
        Checkpoint();

        // Deleta differential two
        SyncAwait(PopulateAsync(NumberOfItemsPerComponent, random, *expectedState, 200));
        Checkpoint();

        // Setup differential: Last Commited - 1
        SyncAwait(PopulateAsync(NumberOfItemsPerComponent, random, *expectedState, 200));

        cout << "Finished populating" << endl;

        // Setup snapshot and writeset
        {
            auto testTxn = CreateWriteTransaction();
            testTxn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            LONG64 vsn;
            SyncAwait(testTxn->TransactionSPtr->GetVisibilitySequenceNumberAsync(vsn));
            Update(*expectedState);
            Remove(*expectedState);

            try
            {
                LONG64 randomKey = static_cast<LONG64>(random.Next());

                SyncAwait(Store->AddAsync(*testTxn->StoreTransactionSPtr, randomKey, randomKey, DefaultTimeout, CancellationToken::None));
                expectedState->Add(randomKey, randomKey);
            }
            catch (ktl::Exception const &)
            {
                // Swallow exception
            }
            
            cout << "Verifying..." << endl;
            SyncAwait(VerifyEnumerationAsync(*testTxn, *expectedState));

            SyncAwait(testTxn->CommitAsync());
        }
    }

    BOOST_AUTO_TEST_CASE(Enumeration_Stress_AddRemove_ShouldSucceed)
    {
        LONG64 NumberOfItemsPerComponent = 1024;
        LONG64 NumberOfIterations = 4096;

        ULONG32 currentIteration = 0;
            
        try
        {
            FastSkipList<LONG64, LONG64>::SPtr expectedState = nullptr;
            FastSkipList<LONG64, LONG64>::Create(Store->KeyComparerSPtr, GetAllocator(), expectedState);

            for (LONG64 key = 0; key < NumberOfItemsPerComponent; key++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, key, DefaultTimeout, CancellationToken::None));
                    SyncAwait(txn->CommitAsync());

                    expectedState->Add(key, key);
                }
            }

            ktl::AwaitableCompletionSource<void>::SPtr checkpointCompletionSource = nullptr;
            for (ULONG32 i = 0; i < NumberOfIterations; i++)
            {
                currentIteration = i;

                if (currentIteration % 20 == 0)
                {
                    cout << currentIteration << " ";
                }
                if (currentIteration % 200 == 0)
                {
                    cout << endl;
                }

                for (LONG64 key = 0; key < NumberOfItemsPerComponent; key++)
                {
                    {
                        auto txn = CreateWriteTransaction();
                        SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));

                        if (key % 2 == 1)
                        {
                            SyncAwait(txn->CommitAsync());
                        }
                        else
                        {
                            SyncAwait(txn->AbortAsync());
                        }
                    }
                }

                if (checkpointCompletionSource == nullptr || checkpointCompletionSource->IsCompleted())
                {
                    checkpointCompletionSource = StartCheckpoint();
                }

                for (LONG64 key = 0; key < NumberOfItemsPerComponent; key++)
                {
                    auto txn = CreateWriteTransaction();
                    try
                    {
                        SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, key, DefaultTimeout, CancellationToken::None));

                        if (key % 2 == 1)
                        {
                            SyncAwait(txn->CommitAsync());
                        }
                        else
                        {
                            SyncAwait(txn->AbortAsync());
                        }
                    }
                    catch (Exception const & e)
                    {
                        SyncAwait(txn->AbortAsync());
                        // Swallow key already exists exceptions
                        if (e.GetStatus() != SF_STATUS_WRITE_CONFLICT)
                        {
                            throw;
                        }
                    }
                }

                // Enumerate
                {
                    auto txn = CreateWriteTransaction();
                    txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
                    SyncAwait(VerifyEnumerationAsync(*txn, *expectedState));
                    SyncAwait(txn->AbortAsync());
                }
            }

            SyncAwait(checkpointCompletionSource->GetAwaitable());
        }
        catch (Exception const & e)
        {
            ASSERT_IFNOT(false, "Failed on iteration {0} with status {1}", currentIteration, e.GetStatus());
        }
    }

    BOOST_AUTO_TEST_SUITE_END();
}

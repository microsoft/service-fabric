// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'sbTP'

inline bool StringEquals(KString::SPtr & one, KString::SPtr & two)
{
    if (one == nullptr || two == nullptr) 
    {
        return one == two;
    }

    return one->Compare(*two) == 0;
}

namespace TStoreTests
{
    class StoreCheckpointTests : public TStoreTestBase<LONG64, KString::SPtr, LongComparer, TestStateSerializer<LONG64>, StringStateSerializer>
    {
    public:
        StoreCheckpointTests()
        {
            Setup(3);
            SyncAwait(Store->RecoverCheckpointAsync(CancellationToken::None));
        }

        ~StoreCheckpointTests()
        {
            Cleanup();
        }

        KString::SPtr GetStringValue(__in LPCWSTR value)
        {
            KString::SPtr result;
            auto status = KString::Create(result, GetAllocator(), value);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            return result;
        }

        KString::SPtr GenerateString(__in LONG64 seed)
        {
            KString::SPtr result = GetStringValue(L"value");
            result->Concat(to_wstring(seed).c_str());
            return result;
        }

        void AddKey(__in LONG64 key, __in KString::SPtr value)
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        void UpdateKey(__in LONG64 key, __in KString::SPtr updateValue)
        {
            auto txn = CreateWriteTransaction();
            bool result = SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, updateValue, DefaultTimeout, CancellationToken::None));
            ASSERT_IFNOT(result, "Should be able to update key {0}", key);
            SyncAwait(txn->CommitAsync());
        }

        void RemoveKey(__in LONG64 key)
        {
            auto txn = CreateWriteTransaction();
            bool result = SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
            ASSERT_IFNOT(result, "Should be able to remove key {0}", key);
            SyncAwait(txn->CommitAsync());
        }

        void CheckpointAndRecover()
        {
            Checkpoint();
            CloseAndReOpenStore();
        }

        void VerifyKeyExists(__in LONG64 key, __in KString::SPtr expectedValue)
        {
            SyncAwait(VerifyKeyExistsAsync(*Store, key, nullptr, expectedValue, StringEquals));
        }

        ktl::Awaitable<void> VerifyKeysExistAsync(
            __in LONG64 startKey, 
            __in LONG64 count,
            __in ktl::AwaitableCompletionSource<bool> & signalStart,
            __in bool mustHaveKey)
        {
            auto txn = CreateWriteTransaction();
            co_await CorHelper::ThreadPoolThread(GetAllocator().GetKtlSystem().DefaultThreadPool()); // Move to background thread

            co_await signalStart.GetAwaitable();

            for (LONG64 key = startKey; key < startKey + count; key++)
            {
                KString::SPtr expectedValue = GenerateString(key);
                
                KeyValuePair<LONG64, KString::SPtr> actual(-1, nullptr);
                bool exists = co_await Store->ConditionalGetAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, actual, ktl::CancellationToken::None);
                CODING_ERROR_ASSERT(exists || mustHaveKey == exists);

                if (exists)
                {
                    KString::SPtr actualValue = actual.Value;
                    CODING_ERROR_ASSERT(StringEquals(actualValue, expectedValue));
                }
            }

            co_await txn->AbortAsync();
        }

        ktl::Awaitable<void> AddAndCheckpointAsync(
            __in LONG64 count,
            __in LONG64 startKey,
            __in LONG64 endKey,
            __in ktl::AwaitableCompletionSource<bool> & signalStart)
        {
            LONG64 numKeys = endKey - startKey;
            LONG64 keysPerCheckpoint = numKeys / count;

            co_await CorHelper::ThreadPoolThread(GetAllocator().GetKtlSystem().DefaultThreadPool()); // Move to background thread

            co_await signalStart.GetAwaitable();

            for (ULONG32 i = 0; i < count; i++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    for (LONG64 key = startKey + (i * keysPerCheckpoint); key < keysPerCheckpoint; key++)
                    {
                        co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, GenerateString(key), DefaultTimeout, ktl::CancellationToken::None);
                    }

                    co_await txn->CommitAsync();
                }

                LONG64 checkpointLSN = Replicator->IncrementAndGetCommitSequenceNumber();
                Store->PrepareCheckpoint(checkpointLSN);
                co_await Store->PerformCheckpointAsync(ktl::CancellationToken::None);
                co_await Store->CompleteCheckpointAsync(ktl::CancellationToken::None);
            }
        }

        void PopulateConsolidatedAndDifferential(__in LONG64 consolidatedCount, __in LONG64 differentialCount, bool isUpdate)
        {
            LONG64 highestConsolidatedValue = 2 * consolidatedCount;
            LONG64 highestDifferentialValue = 2 * differentialCount;

            for (LONG64 i = highestConsolidatedValue - 1; i >= 0; i -= 2) // Consolidated, all odd keys
            {
                AddKey(i, GenerateString(i));
                if (isUpdate)
                {
                    UpdateKey(i, GenerateString(i));
                }
            }

            Checkpoint();

            for (LONG64 i = 0; i < highestDifferentialValue; i += 2) // Differential, all even keys
            {
                AddKey(i, GenerateString(i));
                if (isUpdate)
                {
                    UpdateKey(i, GenerateString(i));
                }
            }

            CheckpointAndRecover();

            // TODO: Verify checkpoint file size
            for (LONG64 i = highestConsolidatedValue - 1; i >= 0; i -= 2)
            {
                SyncAwait(VerifyKeyExistsInStoresAsync(i, nullptr, GenerateString(i), StringEquals));
            }

            for (LONG64 i = 0; i < highestDifferentialValue; i += 2)
            {
                SyncAwait(VerifyKeyExistsInStoresAsync(i, nullptr, GenerateString(i), StringEquals));
            }
        }

        void DoOperation(__in LONG64 key, __in ULONG32 cyclePosition)
        {
            auto value = GenerateString(key);
            switch (cyclePosition % 3)
            {
            case 0: 
                AddKey(key, value); 
                SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, value, StringEquals));
                break;
            case 1: 
                UpdateKey(key, GenerateString(key));
                SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, value, StringEquals));
                break;
            case 2: 
                RemoveKey(key);
                SyncAwait(VerifyKeyDoesNotExistAsync(*Store, key));
            }
        }

        void TwoDeltaTest(__in ULONG32 numVersionsFirstDifferential, __in ULONG32 numVersionsSecondDifferential, __in ULONG32 numIterations = 9)
        {
            LONG64 checkpointLSN = 1;

            for (LONG64 i = 0; i < numIterations; i++)
            {
                ULONG32 numOperationsCompleted = 0;

                for (ULONG32 j = 0; j < numVersionsFirstDifferential; j++)
                {
                    DoOperation(i, numOperationsCompleted++);
                }
                
                checkpointLSN++;

                for (ULONG32 j = 0; j < Stores->Count(); j++)
                {
                    auto store = (*Stores)[j];
                    store->PrepareCheckpoint(checkpointLSN);
                }

                for (ULONG32 j = 0; j < numVersionsSecondDifferential; j++)
                {
                    DoOperation(i, numOperationsCompleted++);
                }

                CheckpointAndRecover();

                auto expectedNumOps = numVersionsFirstDifferential + numVersionsSecondDifferential;

                if (expectedNumOps % 3 == 0)
                {
                    SyncAwait(VerifyKeyDoesNotExistInStoresAsync(i));
                    VerifyCountInAllStores(0);
                }
                else
                {
                    SyncAwait(VerifyKeyExistsInStoresAsync(i, nullptr, GenerateString(i), StringEquals));
                    VerifyCountInAllStores(i + 1);
                }
            }
        }

        Common::CommonConfig config; // load the config object as its needed for the tracing to work
    };

    BOOST_FIXTURE_TEST_SUITE(StoreCheckpointTestSuite, StoreCheckpointTests)

#pragma region Recovery tests
    BOOST_AUTO_TEST_CASE(ReopenStore_ShouldSucceed)
    {
        CloseAndReOpenStore();
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_NoState_ShouldSucceed)
    {
        CheckpointAndRecover();

        // TODO: verify checkpoint file size
        VerifyCountInAllStores(0);
    }

    BOOST_AUTO_TEST_CASE(AddCheckpointRead_ShouldSucceed)
    {
        LONG64 key = 17;
        KString::SPtr value = GetStringValue(L"value");

        AddKey(key, value);
        CheckpointAndRecover();

        //TODO: Verify checkpoint file size
        VerifyCountInAllStores(1);
        SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, value, StringEquals));
    }

    BOOST_AUTO_TEST_CASE(AddCheckpointRemove_ShouldSucceed)
    {
        LONG64 key = 17;
        KString::SPtr value = GetStringValue(L"value");

        AddKey(key, value);
        CheckpointAndRecover();
        VerifyKeyExists(key, value);
        RemoveKey(key);
        SyncAwait(VerifyKeyDoesNotExistInStoresAsync(key));
    }

    BOOST_AUTO_TEST_CASE(AddCheckpointUpdateRead_ShouldSucceed)
    {
        LONG64 key = 17;
        KString::SPtr value = GetStringValue(L"value");
        KString::SPtr updatedValue = GetStringValue(L"update");

        AddKey(key, value);
        CheckpointAndRecover();
        UpdateKey(key, updatedValue);
        SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, updatedValue, StringEquals));
    }

    BOOST_AUTO_TEST_CASE(EmptyCheckpoint_AfterNotEmptyCheckpoint_ShouldSucceed)
    {
        LONG64 key = 17;
        KString::SPtr value = GetStringValue(L"value");

        AddKey(key, value);
        Checkpoint(); // Not empty
        CheckpointAndRecover(); // Empty

        // TODO: Verify checkpoint file size
        SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, value, StringEquals));
    }

    BOOST_AUTO_TEST_CASE(AddUpdateRemoveCheckpoint_MultipleTimes_ShouldSucceed)
    {
        LONG64 key = 17;
        KString::SPtr value = GetStringValue(L"value");

        AddKey(key, value);
        
        for (ULONG32 i = 0; i < 81; i++)
        {
            switch (i % 3)
            {
            case 0: UpdateKey(key, value); break;
            case 1: RemoveKey(key); break;
            case 2: AddKey(key, value); break;
            }
            Checkpoint();
        }

        CheckpointAndRecover();

        // TODO: verify checkpoint file size
        VerifyCountInAllStores(1);
        SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, value, StringEquals));
    }

    BOOST_AUTO_TEST_CASE(AddUpdateCheckpointRead_ShouldSucceed)
    {
        LONG64 key = 17;
        KString::SPtr value = GetStringValue(L"value");
        KString::SPtr updatedValue = GetStringValue(L"update");

        AddKey(key, value);
        UpdateKey(key, updatedValue);
        CheckpointAndRecover();

        // TODO: verify checkpoint file size
        VerifyCountInAllStores(1);
        SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, updatedValue, StringEquals));
    }

    BOOST_AUTO_TEST_CASE(AddRemoveCheckpoint_ShouldSucceed)
    {
        LONG64 key = 17;
        KString::SPtr value = GetStringValue(L"value");

        AddKey(key, value);
        RemoveKey(key);
        CheckpointAndRecover();

        // TODO: Verify checkpoint file size
        VerifyCountInAllStores(0);
        SyncAwait(VerifyKeyDoesNotExistInStoresAsync(key));
    }

    BOOST_AUTO_TEST_CASE(AddCheckpointRemoveCheckpoint_ShouldSucceed)
    {
        LONG64 key = 17;
        KString::SPtr value = GetStringValue(L"value");

        AddKey(key, value);
        CheckpointAndRecover();
        RemoveKey(key);
        CheckpointAndRecover();

        // TODO: Verify checkpoint file size
        VerifyCountInAllStores(0);
        SyncAwait(VerifyKeyDoesNotExistInStoresAsync(key));
    }

    BOOST_AUTO_TEST_CASE(AddCheckpointRemoveCheckpointAddCheckpoint_ShouldSucceed)
    {
        LONG64 key = 17;
        KString::SPtr value1 = GetStringValue(L"value 1");
        KString::SPtr value2 = GetStringValue(L"value 2");

        AddKey(key, value1);
        CheckpointAndRecover();
        RemoveKey(key);
        CheckpointAndRecover();
        AddKey(key, value2);
        CheckpointAndRecover();

        // TODO: Verify checkpoint file size
        VerifyCountInAllStores(1);
        SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, value2, StringEquals));
    }

    BOOST_AUTO_TEST_CASE(AddCheckpointReadCheckpointRead_ShouldSucceed)
    {
        LONG64 key = 17;
        KString::SPtr value = GetStringValue(L"value");

        AddKey(key, value);
        Checkpoint();
        
        VerifyCountInAllStores(1);
        SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, value, StringEquals));

        Checkpoint();

        VerifyCountInAllStores(1);
        SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, value, StringEquals));
    }

    BOOST_AUTO_TEST_CASE(AddCheckpointReadUpdateCheckpointRead_ShouldSucceed)
    {
        LONG64 key = 17;
        KString::SPtr value1 = GetStringValue(L"value1");
        KString::SPtr value2 = GetStringValue(L"value2");

        AddKey(key, value1);
        Checkpoint();

        VerifyCountInAllStores(1);
        SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, value1, StringEquals));

        UpdateKey(key, value2);
        Checkpoint();

        VerifyCountInAllStores(1);
        SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, value2, StringEquals));
    }

    BOOST_AUTO_TEST_CASE(AddCheckpointCloseOpenReadUpdateCheckpointCloseOpenRead_ShouldSucceed)
    {
        LONG64 key = 17;
        KString::SPtr value1 = GetStringValue(L"value1");
        KString::SPtr value2 = GetStringValue(L"value2");

        AddKey(key, value1);
        CheckpointAndRecover();

        VerifyCountInAllStores(1);
        SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, value1, StringEquals));

        UpdateKey(key, value2);
        CheckpointAndRecover();

        VerifyCountInAllStores(1);
        SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, value2, StringEquals));
    }
#pragma endregion

#pragma region Prepare Checkpoint tests
    BOOST_AUTO_TEST_CASE(Checkpoint_ZeroLSNPrepare_ShouldSucceed)
    {
        auto checkpointLSN = 0;
        for (ULONG32 i = 0; i < Stores->Count(); i++)
        {
            auto store = (*Stores)[i];
            store->PrepareCheckpoint(checkpointLSN);
        }

        for (ULONG32 i = 0; i < Stores->Count(); i++)
        {
            auto store = (*Stores)[i];
            SyncAwait(store->PerformCheckpointAsync(CancellationToken::None));
            SyncAwait(store->CompleteCheckpointAsync(CancellationToken::None));
        }

        // Recover original checkpoint
        CloseAndReOpenStore();

        VerifyCountInAllStores(0);
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_DuringApply_ShouldSucceed)
    {
        LONG64 key1 = 17;
        LONG64 key2 = 18;
        KString::SPtr value1 = GetStringValue(L"value 1");
        KString::SPtr value2 = GetStringValue(L"value 2");

        AddKey(key1, value1);

        // Start checkpoint
        auto checkpointLSN = 1;
        for (ULONG32 i = 0; i < Stores->Count(); i++)
        {
            auto store = (*Stores)[i];
            store->PrepareCheckpoint(checkpointLSN);
        }
        
        // Apply after checkpoint has started
        AddKey(key2, value2); 
        
        // Finish checkpoint
        for (ULONG32 i = 0; i < Stores->Count(); i++)
        {
            auto store = (*Stores)[i];
            SyncAwait(store->PerformCheckpointAsync(CancellationToken::None));
            SyncAwait(store->CompleteCheckpointAsync(CancellationToken::None));
        }

        // Recover original checkpoint
        CloseAndReOpenStore();

        // TODO: Verify checkpoint file size
        VerifyCountInAllStores(1);

        SyncAwait(VerifyKeyExistsInStoresAsync(key1, nullptr, value1, StringEquals));
    }

    BOOST_AUTO_TEST_CASE(AddPrepareRead_ShouldSucceed)
    {
        LONG64 key = 17;
        KString::SPtr value = GetStringValue(L"value");

        AddKey(key, value);

        auto checkpointLSN = 1;
        for (ULONG32 i = 0; i < Stores->Count(); i++)
        {
            auto store = (*Stores)[i];
            store->PrepareCheckpoint(checkpointLSN);
        }

        VerifyCountInAllStores(1);
        SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, value, StringEquals));
    }

    BOOST_AUTO_TEST_CASE(PreparePreparePerform_ShouldSucceed)
    {
        LONG64 key1 = 17;
        LONG64 key2 = 18;
        KString::SPtr value = GetStringValue(L"value");

        AddKey(key1, value);

        auto checkpointLSN = 1;
        for (ULONG32 i = 0; i < Stores->Count(); i++)
        {
            auto store = (*Stores)[i];
            store->PrepareCheckpoint(checkpointLSN);
        }

        AddKey(key2, value);

        checkpointLSN = 2;
        for (ULONG32 i = 0; i < Stores->Count(); i++)
        {
            auto store = (*Stores)[i];
            store->PrepareCheckpoint(checkpointLSN);
        }

        SyncAwait(VerifyKeyExistsInStoresAsync(key1, nullptr, value, StringEquals));
        SyncAwait(VerifyKeyExistsInStoresAsync(key2, nullptr, value, StringEquals));

        for (ULONG32 i = 0; i < Stores->Count(); i++)
        {
            auto store = (*Stores)[i];
            SyncAwait(store->PerformCheckpointAsync(CancellationToken::None));
            SyncAwait(store->CompleteCheckpointAsync(CancellationToken::None));
        }

        VerifyCountInAllStores(2);
        SyncAwait(VerifyKeyExistsInStoresAsync(key1, nullptr, value, StringEquals));
        SyncAwait(VerifyKeyExistsInStoresAsync(key2, nullptr, value, StringEquals));
    }

    BOOST_AUTO_TEST_CASE(AddUpdatePrepareRemoveCheckpointRecover_Twice_ShouldSucceed)
    {
        KString::SPtr value = GetStringValue(L"value");

        auto checkpointLSN = 1;
        for (ULONG32 i = 0; i < 2; i++)
        {
            auto key = i;
            AddKey(key, value);
            SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, value, StringEquals));

            UpdateKey(key, value);
            SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, value, StringEquals));

            checkpointLSN++;
            for (ULONG32 j = 0; j < Stores->Count(); j++)
            {
                auto store = (*Stores)[j];
                store->PrepareCheckpoint(checkpointLSN);
            }

            RemoveKey(key);
            SyncAwait(VerifyKeyDoesNotExistInStoresAsync(key));

            CheckpointAndRecover();

            VerifyCountInAllStores(0);
            SyncAwait(VerifyKeyDoesNotExistInStoresAsync(key));
        }
    }

    BOOST_AUTO_TEST_CASE(ConcurrentReadsDuringCheckpoint_MultipleDifferentials_ShouldSucceed)
    {
        Store->EnableBackgroundConsolidation = false;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 6;

        for (LONG64 startKey = 0; startKey < 1000; startKey += 100)
        {
            {
                auto txn = CreateWriteTransaction();

                for (LONG64 key = startKey; key < startKey + 100; key++)
                {
                    SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, GenerateString(key), DefaultTimeout, ktl::CancellationToken::None));
                }

                SyncAwait(txn->CommitAsync());
            }

            if (startKey < 900) // Don't checkpoint last keys added
            {
                Checkpoint();
            }
        }
        
        LONG64 checkpointLSN = Replicator->IncrementAndGetCommitSequenceNumber();
        Store->PrepareCheckpoint(checkpointLSN);

        ktl::AwaitableCompletionSource<bool>::SPtr signalSPtr = nullptr;
        NTSTATUS status = ktl::AwaitableCompletionSource<bool>::Create(GetAllocator(), ALLOC_TAG, signalSPtr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        KSharedArray<ktl::Awaitable<void>>::SPtr tasks = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ktl::Awaitable<void>>();
        for (ULONG32 t = 0; t < 20; t++)
        {
            tasks->Append(VerifyKeysExistAsync(t * 50, 50, *signalSPtr, true));
        }

        signalSPtr->SetResult(true); // Start the background readers

        SyncAwait(Store->PerformCheckpointAsync(ktl::CancellationToken::None));

        SyncAwait(StoreUtilities::WhenAll<void>(*tasks, GetAllocator()));

        SyncAwait(Store->CompleteCheckpointAsync(ktl::CancellationToken::None));
    }

    BOOST_AUTO_TEST_CASE(ConcurrentReadsAndsCheckpoints_MultipleDifferentials_ShouldSucceed)
    {
        Store->EnableBackgroundConsolidation = false;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 6;

        for (LONG64 startKey = 0; startKey < 1000; startKey += 100)
        {
            {
                auto txn = CreateWriteTransaction();

                for (LONG64 key = startKey; key < startKey + 100; key++)
                {
                    SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, GenerateString(key), DefaultTimeout, ktl::CancellationToken::None));
                }

                SyncAwait(txn->CommitAsync());
            }

            if (startKey < 900) // Don't checkpoint last keys added
            {
                Checkpoint();
            }
        }
        
        ktl::AwaitableCompletionSource<bool>::SPtr signalSPtr = nullptr;
        NTSTATUS status = ktl::AwaitableCompletionSource<bool>::Create(GetAllocator(), ALLOC_TAG, signalSPtr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        KSharedArray<ktl::Awaitable<void>>::SPtr tasks = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ktl::Awaitable<void>>();

        // Reads a total of 1200 items. 200 of which are from the checkpoint task and may not exist
        for (ULONG32 t = 0; t < 20; t++)
        {
            tasks->Append(VerifyKeysExistAsync(t * 60, 60, *signalSPtr, false));
        }

        auto checkpointsTask = AddAndCheckpointAsync(10, 1000, 1200, *signalSPtr);

        signalSPtr->SetResult(true); // Start the background readers and checkpoint task

        SyncAwait(StoreUtilities::WhenAll<void>(*tasks, GetAllocator()));
        SyncAwait(checkpointsTask);
    }

#pragma endregion

#pragma region Consolidated and Differential State Tests
    BOOST_AUTO_TEST_CASE(Checkpoint_EmptyDifferentialState_ShouldSucceed)
    {
        LONG64 key = 17;
        KString::SPtr value = GetStringValue(L"value");
        
        AddKey(key, value);
        CheckpointAndRecover();
        
        // TODO: Verify checkpoint file size
        VerifyCountInAllStores(1);
        SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, value, StringEquals));

        CheckpointAndRecover();

        // TODO: Verify checkpoint file size
        VerifyCountInAllStores(1);
        SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, value, StringEquals));
    }
       
    BOOST_AUTO_TEST_CASE(Checkpoint_DifferentialAndConsolidated_SameCount_WithIntersection_DifferentialAndConsolidatedPopulatedInReverseOrder_Add_Delete_ShouldSucceed)
    {
        LONG64 numItems = 100; // TODO: managed has this at 1024

        for (LONG64 i = numItems - 1; i >= 0; i--)
        {
            AddKey(i, GenerateString(i));
        }

        Checkpoint(); // Consolidate

        // Create a reverse differential state
        for (LONG64 i = 0; i < numItems; i++)
        {
            RemoveKey(i);
        }

        Checkpoint();

        VerifyCountInAllStores(0);
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_DifferentialAndConsolidated_SameCount_WithIntersection_DifferentialAndConsolidatedPopulatedInReverseOrder_Add_Update_UpdateRepeated_ShouldSucceed)
    {
        LONG64 numItems = 100; // TODO: Managed has this at 1024
        ULONG32 numUpdateIterations = 8;

        for (LONG64 i = numItems - 1; i >= 0; i--)
        {
            AddKey(i, GenerateString(i));
        }

        Checkpoint(); // Consolidate

        // Create a reverse differential state
        for (LONG64 i = 0; i < numItems; i++)
        {
            UpdateKey(i, GenerateString(i));
        }

        Checkpoint();

        for (ULONG32 i = 0; i < numUpdateIterations; i++)
        {
            for (LONG64 j = 0; j < numItems; j++)
            {
                UpdateKey(j, GenerateString(j));
            }

            Checkpoint();
        }

        VerifyCountInAllStores(numItems);
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_DifferentialAndConsolidated_SameCount_WithIntersection_DifferentialAndConsolidatedPopulatedInReverseOrder_AddPartialUpdate_ShouldSucceed)
    {
        LONG64 numItems = 100; // TODO: Managed has this at 1024
        ULONG32 numUpdateIterations = 8;

        for (LONG64 i = numItems - 1; i >= 0; i--)
        {
            AddKey(i, GenerateString(i));
        }

        Checkpoint(); // Consolidate

        // Create a reverse differential state
        for (LONG64 i = 0; i < numItems; i++)
        {
            UpdateKey(i, GenerateString(i));
        }

        Checkpoint();

        for (ULONG32 i = 0; i < numUpdateIterations; i++)
        {
            // Lower half
            for (LONG64 j = 0; j < numItems/2; j++)
            {
                UpdateKey(j, GenerateString(j));
            }

            Checkpoint();
            
            // Upper half
            for (LONG64 j = numItems/2; j < numItems; j++)
            {
                UpdateKey(j, GenerateString(j));
            }

            Checkpoint();
        }

        VerifyCountInAllStores(numItems);
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_DifferentialAndConsolidated_SameCount_NoIntersection_AddAdd_ShouldSucceed)
    {
        // TODO: Managed has this at 512, 512. Making smaller for now
        PopulateConsolidatedAndDifferential(50, 50, false);
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_DifferentialAndConsolidated_SameCount_NoIntersection_UpdateUpdate_ShouldSucceed)
    {
        // TODO: Managed has this at 512, 512. Making smaller for now
        PopulateConsolidatedAndDifferential(50, 50, true);
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_DifferentialAndConsolidated_BiggerDifferential_NoIntersection_AddAdd_ShouldSucceed)
    {
        // TODO: Managed has this at 512, 1024. Making smaller for now
        PopulateConsolidatedAndDifferential(50, 100, false);
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_DifferentialAndConsolidated_BiggerDifferential_NoIntersection_UpdateUpdate_ShouldSucceed)
    {
        // TODO: Managed has this at 512, 1024. Making smaller for now
        PopulateConsolidatedAndDifferential(50, 100, true);
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_DifferentialAndConsolidated_BiggerConsolidated_NoIntersection_AddAdd_ShouldSucceed)
    {
        // TODO: Managed has this at 1024, 512. Making smaller for now
        PopulateConsolidatedAndDifferential(100, 50, false);
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_DifferentialAndConsolidated_BiggerConsolidated_NoIntersection_UpdateUpdate_ShouldSucceed)
    {
        // TODO: Managed has this at 1024, 512. Making smaller for now
        PopulateConsolidatedAndDifferential(100, 50, true);
    }
#pragma endregion

#pragma region Checkpoint 2 Delta Tests

    BOOST_AUTO_TEST_CASE(Checkpoint_TwoDeltaDifferentials_ZeroOne_ShouldSucceed)
    {
        TwoDeltaTest(0, 1);
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_TwoDeltaDifferentials_OneZero_ShouldSucceed)
    {
        TwoDeltaTest(1, 0);
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_TwoDeltaDifferentials_OneOne_ShouldSucceed)
    {
        TwoDeltaTest(1, 1);
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_TwoDeltaDifferentials_TwoOne_ShouldSucceed)
    {
        TwoDeltaTest(2, 1);
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_TwoDeltaDifferentials_OneTwo_ShouldSucceed)
    {
        TwoDeltaTest(1, 2);
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_TwoDeltaDifferentials_TwoTwo_ShouldSucceed)
    {
        TwoDeltaTest(2, 2);
    }

#pragma endregion

BOOST_AUTO_TEST_CASE(Add100Checkpoint_WithValueRecovery_ShouldSucceed)
{
    Store->ShouldLoadValuesOnRecovery = true;
    for (LONG64 i = 0; i < 100; i++)
    {
        auto key = i;
        auto value = GenerateString(i);
        AddKey(key, value);
    }

    CheckpointAndRecover();

    VerifyCountInAllStores(100);
    for (LONG64 i = 0; i < 100; i++)
    {
        auto key = i;
        auto expectedValue = GenerateString(i);
        SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, expectedValue, StringEquals));
    }
}

BOOST_AUTO_TEST_CASE(Add100Checkpoint_WithValueRecovery_MultipleInflightTasks_ShouldSucceed)
{
    Store->ShouldLoadValuesOnRecovery = true;
    Store->MaxNumberOfInflightValueRecoveryTasks = 10;

    for (LONG64 i = 0; i < 100; i++)
    {
        auto key = i;
        auto value = GenerateString(i);
        AddKey(key, value);
    }

    CheckpointAndRecover();

    VerifyCountInAllStores(100);
    for (LONG64 i = 0; i < 100; i++)
    {
        auto key = i;
        auto expectedValue = GenerateString(i);
        SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, expectedValue, StringEquals));
    }
}

#pragma region Performance Tests
    BOOST_AUTO_TEST_CASE(Add1000Checkpoint_ShouldSucceed)
    {
        for (LONG64 i = 0; i < 1000; i++)
        {
            auto key = i;
            auto value = GenerateString(i);
            AddKey(key, value);
        }

        CheckpointAndRecover();

        //TODO: Verify checkpoint file size
        VerifyCountInAllStores(1000);
        for (LONG64 i = 0; i < 1000; i++)
        {
            auto key = i;
            auto expectedValue = GenerateString(i);
            SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, expectedValue, StringEquals));
        }
    }
#pragma endregion
    BOOST_AUTO_TEST_SUITE_END()
}

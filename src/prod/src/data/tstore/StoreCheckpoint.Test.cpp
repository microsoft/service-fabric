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

        ktl::Awaitable<void> AddKeyAsync(__in LONG64 key, __in KString::SPtr value)
        {
            auto txn = CreateWriteTransaction();
            co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
            co_await txn->CommitAsync();
            co_return;
        }

        ktl::Awaitable<void> UpdateKeyAsync(__in LONG64 key, __in KString::SPtr updateValue)
        {
            auto txn = CreateWriteTransaction();
            bool result = co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, updateValue, DefaultTimeout, CancellationToken::None);
            ASSERT_IFNOT(result, "Should be able to update key {0}", key);
            co_await txn->CommitAsync();
            co_return;
        }

        ktl::Awaitable<void> RemoveKeyAsync(__in LONG64 key)
        {
            auto txn = CreateWriteTransaction();
            bool result = co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None);
            ASSERT_IFNOT(result, "Should be able to remove key {0}", key);
            co_await txn->CommitAsync();
            co_return;
        }

        ktl::Awaitable<void> CheckpointAndRecoverAsync()
        {
            co_await CheckpointAsync();
            co_await CloseAndReOpenStoreAsync();
            co_return;
        }

        ktl::Awaitable<void> VerifyKeyExistsAsync(__in LONG64 key, __in KString::SPtr expectedValue)
        {
            co_await static_cast<TStoreTestBase *>(this)->VerifyKeyExistsAsync(
                *Store, key, nullptr, expectedValue, StringEquals);
            co_return;
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

        ktl::Awaitable<void> PopulateConsolidatedAndDifferentialAsync(__in LONG64 consolidatedCount, __in LONG64 differentialCount, bool isUpdate)
        {
            LONG64 highestConsolidatedValue = 2 * consolidatedCount;
            LONG64 highestDifferentialValue = 2 * differentialCount;

            for (LONG64 i = highestConsolidatedValue - 1; i >= 0; i -= 2) // Consolidated, all odd keys
            {
                co_await AddKeyAsync(i, GenerateString(i));
                if (isUpdate)
                {
                    co_await UpdateKeyAsync(i, GenerateString(i));
                }
            }

            co_await CheckpointAsync();

            for (LONG64 i = 0; i < highestDifferentialValue; i += 2) // Differential, all even keys
            {
                co_await AddKeyAsync(i, GenerateString(i));
                if (isUpdate)
                {
                    co_await UpdateKeyAsync(i, GenerateString(i));
                }
            }

            co_await CheckpointAndRecoverAsync();

            // TODO: Verify checkpoint file size
            for (LONG64 i = highestConsolidatedValue - 1; i >= 0; i -= 2)
            {
                co_await VerifyKeyExistsInStoresAsync(i, nullptr, GenerateString(i), StringEquals);
            }

            for (LONG64 i = 0; i < highestDifferentialValue; i += 2)
            {
                co_await VerifyKeyExistsInStoresAsync(i, nullptr, GenerateString(i), StringEquals);
            }

            co_return;
        }

        ktl::Awaitable<void> DoOperationAsync(__in LONG64 key, __in ULONG32 cyclePosition)
        {
            auto value = GenerateString(key);
            switch (cyclePosition % 3)
            {
            case 0: 
                co_await AddKeyAsync(key, value);
                co_await VerifyKeyExistsInStoresAsync(key, nullptr, value, StringEquals);
                break;
            case 1: 
                co_await UpdateKeyAsync(key, GenerateString(key));
                co_await VerifyKeyExistsInStoresAsync(key, nullptr, value, StringEquals);
                break;
            case 2: 
                co_await RemoveKeyAsync(key);
                co_await VerifyKeyDoesNotExistAsync(*Store, key);
            }
            co_return;
        }

        ktl::Awaitable<void> TwoDeltaTestAsync(__in ULONG32 numVersionsFirstDifferential, __in ULONG32 numVersionsSecondDifferential, __in ULONG32 numIterations = 9)
        {
            LONG64 checkpointLSN = 1;

            for (LONG64 i = 0; i < numIterations; i++)
            {
                ULONG32 numOperationsCompleted = 0;

                for (ULONG32 j = 0; j < numVersionsFirstDifferential; j++)
                {
                    co_await DoOperationAsync(i, numOperationsCompleted++);
                }
                
                checkpointLSN++;

                for (ULONG32 j = 0; j < Stores->Count(); j++)
                {
                    auto store = (*Stores)[j];
                    store->PrepareCheckpoint(checkpointLSN);
                }

                for (ULONG32 j = 0; j < numVersionsSecondDifferential; j++)
                {
                    co_await DoOperationAsync(i, numOperationsCompleted++);
                }

                co_await CheckpointAndRecoverAsync();

                auto expectedNumOps = numVersionsFirstDifferential + numVersionsSecondDifferential;

                if (expectedNumOps % 3 == 0)
                {
                    co_await VerifyKeyDoesNotExistInStoresAsync(i);
                    VerifyCountInAllStores(0);
                }
                else
                {
                    co_await VerifyKeyExistsInStoresAsync(i, nullptr, GenerateString(i), StringEquals);
                    VerifyCountInAllStores(i + 1);
                }
            }
            co_return;
        }

        Common::CommonConfig config; // load the config object as its needed for the tracing to work

#pragma region test functions
    public:
        ktl::Awaitable<void> ReopenStore_ShouldSucceed_Test()
        {
            co_await CloseAndReOpenStoreAsync();
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_NoState_ShouldSucceed_Test()
        {
            co_await CheckpointAndRecoverAsync();

            // TODO: verify checkpoint file size
            VerifyCountInAllStores(0);
            co_return;
        }

        ktl::Awaitable<void> AddCheckpointRead_ShouldSucceed_Test()
        {
            LONG64 key = 17;
            KString::SPtr value = GetStringValue(L"value");

            co_await AddKeyAsync(key, value);
            co_await CheckpointAndRecoverAsync();

            //TODO: Verify checkpoint file size
            VerifyCountInAllStores(1);
            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value, StringEquals);
            co_return;
        }

        ktl::Awaitable<void> AddCheckpointRemove_ShouldSucceed_Test()
        {
            LONG64 key = 17;
            KString::SPtr value = GetStringValue(L"value");

            co_await AddKeyAsync(key, value);
            co_await CheckpointAndRecoverAsync();
            co_await VerifyKeyExistsAsync(key, value);
            co_await RemoveKeyAsync(key);
            co_await VerifyKeyDoesNotExistInStoresAsync(key);
            co_return;
        }

        ktl::Awaitable<void> AddCheckpointUpdateRead_ShouldSucceed_Test()
        {
            LONG64 key = 17;
            KString::SPtr value = GetStringValue(L"value");
            KString::SPtr updatedValue = GetStringValue(L"update");

            co_await AddKeyAsync(key, value);
            co_await CheckpointAndRecoverAsync();
            co_await UpdateKeyAsync(key, updatedValue);
            co_await VerifyKeyExistsInStoresAsync(key, nullptr, updatedValue, StringEquals);
            co_return;
        }

        ktl::Awaitable<void> EmptyCheckpoint_AfterNotEmptyCheckpoint_ShouldSucceed_Test()
        {
            LONG64 key = 17;
            KString::SPtr value = GetStringValue(L"value");

            co_await AddKeyAsync(key, value);
            co_await CheckpointAsync(); // Not empty
            co_await CheckpointAndRecoverAsync(); // Empty

            // TODO: Verify checkpoint file size
            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value, StringEquals);
            co_return;
        }

        ktl::Awaitable<void> AddUpdateRemoveCheckpoint_MultipleTimes_ShouldSucceed_Test()
        {
            LONG64 key = 17;
            KString::SPtr value = GetStringValue(L"value");

            co_await AddKeyAsync(key, value);
        
            for (ULONG32 i = 0; i < 81; i++)
            {
                switch (i % 3)
                {
                case 0: co_await UpdateKeyAsync(key, value); break;
                case 1: co_await RemoveKeyAsync(key); break;
                case 2: co_await AddKeyAsync(key, value); break;
                }
                co_await CheckpointAsync();
            }

            co_await CheckpointAndRecoverAsync();

            // TODO: verify checkpoint file size
            VerifyCountInAllStores(1);
            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value, StringEquals);
            co_return;
        }

        ktl::Awaitable<void> AddUpdateCheckpointRead_ShouldSucceed_Test()
        {
            LONG64 key = 17;
            KString::SPtr value = GetStringValue(L"value");
            KString::SPtr updatedValue = GetStringValue(L"update");

            co_await AddKeyAsync(key, value);
            co_await UpdateKeyAsync(key, updatedValue);
            co_await CheckpointAndRecoverAsync();

            // TODO: verify checkpoint file size
            VerifyCountInAllStores(1);
            co_await VerifyKeyExistsInStoresAsync(key, nullptr, updatedValue, StringEquals);
            co_return;
        }

        ktl::Awaitable<void> AddRemoveCheckpoint_ShouldSucceed_Test()
        {
            LONG64 key = 17;
            KString::SPtr value = GetStringValue(L"value");

            co_await AddKeyAsync(key, value);
            co_await RemoveKeyAsync(key);
            co_await CheckpointAndRecoverAsync();

            // TODO: Verify checkpoint file size
            VerifyCountInAllStores(0);
            co_await VerifyKeyDoesNotExistInStoresAsync(key);
            co_return;
        }

        ktl::Awaitable<void> AddCheckpointRemoveCheckpoint_ShouldSucceed_Test()
        {
            LONG64 key = 17;
            KString::SPtr value = GetStringValue(L"value");

            co_await AddKeyAsync(key, value);
            co_await CheckpointAndRecoverAsync();
            co_await RemoveKeyAsync(key);
            co_await CheckpointAndRecoverAsync();

            // TODO: Verify checkpoint file size
            VerifyCountInAllStores(0);
            co_await VerifyKeyDoesNotExistInStoresAsync(key);
            co_return;
        }

        ktl::Awaitable<void> AddCheckpointRemoveCheckpointAddCheckpoint_ShouldSucceed_Test()
        {
            LONG64 key = 17;
            KString::SPtr value1 = GetStringValue(L"value 1");
            KString::SPtr value2 = GetStringValue(L"value 2");

            co_await AddKeyAsync(key, value1);
            co_await CheckpointAndRecoverAsync();
            co_await RemoveKeyAsync(key);
            co_await CheckpointAndRecoverAsync();
            co_await AddKeyAsync(key, value2);
            co_await CheckpointAndRecoverAsync();

            // TODO: Verify checkpoint file size
            VerifyCountInAllStores(1);
            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value2, StringEquals);
            co_return;
        }

        ktl::Awaitable<void> AddCheckpointReadCheckpointRead_ShouldSucceed_Test()
        {
            LONG64 key = 17;
            KString::SPtr value = GetStringValue(L"value");

            co_await AddKeyAsync(key, value);
            co_await CheckpointAsync();
        
            VerifyCountInAllStores(1);
            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value, StringEquals);

            co_await CheckpointAsync();

            VerifyCountInAllStores(1);
            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value, StringEquals);
            co_return;
        }

        ktl::Awaitable<void> AddCheckpointReadUpdateCheckpointRead_ShouldSucceed_Test()
        {
            LONG64 key = 17;
            KString::SPtr value1 = GetStringValue(L"value1");
            KString::SPtr value2 = GetStringValue(L"value2");

            co_await AddKeyAsync(key, value1);
            co_await CheckpointAsync();

            VerifyCountInAllStores(1);
            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value1, StringEquals);

            co_await UpdateKeyAsync(key, value2);
            co_await CheckpointAsync();

            VerifyCountInAllStores(1);
            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value2, StringEquals);
            co_return;
        }

        ktl::Awaitable<void> AddCheckpointCloseOpenReadUpdateCheckpointCloseOpenRead_ShouldSucceed_Test()
        {
            LONG64 key = 17;
            KString::SPtr value1 = GetStringValue(L"value1");
            KString::SPtr value2 = GetStringValue(L"value2");

            co_await AddKeyAsync(key, value1);
            co_await CheckpointAndRecoverAsync();

            VerifyCountInAllStores(1);
            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value1, StringEquals);

            co_await UpdateKeyAsync(key, value2);
            co_await CheckpointAndRecoverAsync();

            VerifyCountInAllStores(1);
            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value2, StringEquals);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_ZeroLSNPrepare_ShouldSucceed_Test()
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
                co_await store->PerformCheckpointAsync(CancellationToken::None);
                co_await store->CompleteCheckpointAsync(CancellationToken::None);
            }

            // Recover original checkpoint
            co_await CloseAndReOpenStoreAsync();

            VerifyCountInAllStores(0);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_DuringApply_ShouldSucceed_Test()
        {
            LONG64 key1 = 17;
            LONG64 key2 = 18;
            KString::SPtr value1 = GetStringValue(L"value 1");
            KString::SPtr value2 = GetStringValue(L"value 2");

            co_await AddKeyAsync(key1, value1);

            // Start checkpoint
            auto checkpointLSN = 1;
            for (ULONG32 i = 0; i < Stores->Count(); i++)
            {
                auto store = (*Stores)[i];
                store->PrepareCheckpoint(checkpointLSN);
            }
        
            // Apply after checkpoint has started
            co_await AddKeyAsync(key2, value2);
        
            // Finish checkpoint
            for (ULONG32 i = 0; i < Stores->Count(); i++)
            {
                auto store = (*Stores)[i];
                co_await store->PerformCheckpointAsync(CancellationToken::None);
                co_await store->CompleteCheckpointAsync(CancellationToken::None);
            }

            // Recover original checkpoint
            co_await CloseAndReOpenStoreAsync();

            // TODO: Verify checkpoint file size
            VerifyCountInAllStores(1);

            co_await VerifyKeyExistsInStoresAsync(key1, nullptr, value1, StringEquals);
            co_return;
        }

        ktl::Awaitable<void> AddPrepareRead_ShouldSucceed_Test()
        {
            LONG64 key = 17;
            KString::SPtr value = GetStringValue(L"value");

            co_await AddKeyAsync(key, value);

            auto checkpointLSN = 1;
            for (ULONG32 i = 0; i < Stores->Count(); i++)
            {
                auto store = (*Stores)[i];
                store->PrepareCheckpoint(checkpointLSN);
            }

            VerifyCountInAllStores(1);
            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value, StringEquals);
            co_return;
        }

        ktl::Awaitable<void> PreparePreparePerform_ShouldSucceed_Test()
        {
            LONG64 key1 = 17;
            LONG64 key2 = 18;
            KString::SPtr value = GetStringValue(L"value");

            co_await AddKeyAsync(key1, value);

            auto checkpointLSN = 1;
            for (ULONG32 i = 0; i < Stores->Count(); i++)
            {
                auto store = (*Stores)[i];
                store->PrepareCheckpoint(checkpointLSN);
            }

            co_await AddKeyAsync(key2, value);

            checkpointLSN = 2;
            for (ULONG32 i = 0; i < Stores->Count(); i++)
            {
                auto store = (*Stores)[i];
                store->PrepareCheckpoint(checkpointLSN);
            }

            co_await VerifyKeyExistsInStoresAsync(key1, nullptr, value, StringEquals);
            co_await VerifyKeyExistsInStoresAsync(key2, nullptr, value, StringEquals);

            for (ULONG32 i = 0; i < Stores->Count(); i++)
            {
                auto store = (*Stores)[i];
                co_await store->PerformCheckpointAsync(CancellationToken::None);
                co_await store->CompleteCheckpointAsync(CancellationToken::None);
            }

            VerifyCountInAllStores(2);
            co_await VerifyKeyExistsInStoresAsync(key1, nullptr, value, StringEquals);
            co_await VerifyKeyExistsInStoresAsync(key2, nullptr, value, StringEquals);
            co_return;
        }

        ktl::Awaitable<void> AddUpdatePrepareRemoveCheckpointRecover_Twice_ShouldSucceed_Test()
        {
            KString::SPtr value = GetStringValue(L"value");

            auto checkpointLSN = 1;
            for (ULONG32 i = 0; i < 2; i++)
            {
                auto key = i;
                co_await AddKeyAsync(key, value);
                co_await VerifyKeyExistsInStoresAsync(key, nullptr, value, StringEquals);

                co_await UpdateKeyAsync(key, value);
                co_await VerifyKeyExistsInStoresAsync(key, nullptr, value, StringEquals);

                checkpointLSN++;
                for (ULONG32 j = 0; j < Stores->Count(); j++)
                {
                    auto store = (*Stores)[j];
                    store->PrepareCheckpoint(checkpointLSN);
                }

                co_await RemoveKeyAsync(key);
                co_await VerifyKeyDoesNotExistInStoresAsync(key);

                co_await CheckpointAndRecoverAsync();

                VerifyCountInAllStores(0);
                co_await VerifyKeyDoesNotExistInStoresAsync(key);
            }
            co_return;
        }

        ktl::Awaitable<void> ConcurrentReadsDuringCheckpoint_MultipleDifferentials_ShouldSucceed_Test()
        {
            Store->EnableBackgroundConsolidation = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 6;

            for (LONG64 startKey = 0; startKey < 1000; startKey += 100)
            {
                {
                    auto txn = CreateWriteTransaction();

                    for (LONG64 key = startKey; key < startKey + 100; key++)
                    {
                        co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, GenerateString(key), DefaultTimeout, ktl::CancellationToken::None);
                    }

                    co_await txn->CommitAsync();
                }

                if (startKey < 900) // Don't checkpoint last keys added
                {
                    co_await CheckpointAsync();
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

            co_await Store->PerformCheckpointAsync(ktl::CancellationToken::None);

            co_await StoreUtilities::WhenAll<void>(*tasks, GetAllocator());

            co_await Store->CompleteCheckpointAsync(ktl::CancellationToken::None);
            co_return;
        }

        ktl::Awaitable<void> ConcurrentReadsAndsCheckpoints_MultipleDifferentials_ShouldSucceed_Test()
        {
            Store->EnableBackgroundConsolidation = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 6;

            for (LONG64 startKey = 0; startKey < 1000; startKey += 100)
            {
                {
                    auto txn = CreateWriteTransaction();

                    for (LONG64 key = startKey; key < startKey + 100; key++)
                    {
                        co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, GenerateString(key), DefaultTimeout, ktl::CancellationToken::None);
                    }

                    co_await txn->CommitAsync();
                }

                if (startKey < 900) // Don't checkpoint last keys added
                {
                    co_await CheckpointAsync();
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

            co_await StoreUtilities::WhenAll<void>(*tasks, GetAllocator());
            co_await checkpointsTask;
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_EmptyDifferentialState_ShouldSucceed_Test()
        {
            LONG64 key = 17;
            KString::SPtr value = GetStringValue(L"value");
        
            co_await AddKeyAsync(key, value);
            co_await CheckpointAndRecoverAsync();
        
            // TODO: Verify checkpoint file size
            VerifyCountInAllStores(1);
            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value, StringEquals);

            co_await CheckpointAndRecoverAsync();

            // TODO: Verify checkpoint file size
            VerifyCountInAllStores(1);
            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value, StringEquals);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_DifferentialAndConsolidated_SameCount_WithIntersection_DifferentialAndConsolidatedPopulatedInReverseOrder_Add_Delete_ShouldSucceed_Test()
        {
            LONG64 numItems = 100; // TODO: managed has this at 1024

            for (LONG64 i = numItems - 1; i >= 0; i--)
            {
                co_await AddKeyAsync(i, GenerateString(i));
            }

            co_await CheckpointAsync(); // Consolidate

            // Create a reverse differential state
            for (LONG64 i = 0; i < numItems; i++)
            {
                co_await RemoveKeyAsync(i);
            }

            co_await CheckpointAsync();

            VerifyCountInAllStores(0);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_DifferentialAndConsolidated_SameCount_WithIntersection_DifferentialAndConsolidatedPopulatedInReverseOrder_Add_Update_UpdateRepeated_ShouldSucceed_Test()
        {
            LONG64 numItems = 100; // TODO: Managed has this at 1024
            ULONG32 numUpdateIterations = 8;

            for (LONG64 i = numItems - 1; i >= 0; i--)
            {
                co_await AddKeyAsync(i, GenerateString(i));
            }

            co_await CheckpointAsync(); // Consolidate

            // Create a reverse differential state
            for (LONG64 i = 0; i < numItems; i++)
            {
                co_await UpdateKeyAsync(i, GenerateString(i));
            }

            co_await CheckpointAsync();

            for (ULONG32 i = 0; i < numUpdateIterations; i++)
            {
                for (LONG64 j = 0; j < numItems; j++)
                {
                    co_await UpdateKeyAsync(j, GenerateString(j));
                }

                co_await CheckpointAsync();
            }

            VerifyCountInAllStores(numItems);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_DifferentialAndConsolidated_SameCount_WithIntersection_DifferentialAndConsolidatedPopulatedInReverseOrder_AddPartialUpdate_ShouldSucceed_Test()
        {
            LONG64 numItems = 100; // TODO: Managed has this at 1024
            ULONG32 numUpdateIterations = 8;

            for (LONG64 i = numItems - 1; i >= 0; i--)
            {
                co_await AddKeyAsync(i, GenerateString(i));
            }

            co_await CheckpointAsync(); // Consolidate

            // Create a reverse differential state
            for (LONG64 i = 0; i < numItems; i++)
            {
                co_await UpdateKeyAsync(i, GenerateString(i));
            }

            co_await CheckpointAsync();

            for (ULONG32 i = 0; i < numUpdateIterations; i++)
            {
                // Lower half
                for (LONG64 j = 0; j < numItems/2; j++)
                {
                    co_await UpdateKeyAsync(j, GenerateString(j));
                }

                co_await CheckpointAsync();
            
                // Upper half
                for (LONG64 j = numItems/2; j < numItems; j++)
                {
                    co_await UpdateKeyAsync(j, GenerateString(j));
                }

                co_await CheckpointAsync();
            }

            VerifyCountInAllStores(numItems);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_DifferentialAndConsolidated_SameCount_NoIntersection_AddAdd_ShouldSucceed_Test()
        {
            // TODO: Managed has this at 512, 512. Making smaller for now
            co_await PopulateConsolidatedAndDifferentialAsync(50, 50, false);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_DifferentialAndConsolidated_SameCount_NoIntersection_UpdateUpdate_ShouldSucceed_Test()
        {
            // TODO: Managed has this at 512, 512. Making smaller for now
            co_await PopulateConsolidatedAndDifferentialAsync(50, 50, true);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_DifferentialAndConsolidated_BiggerDifferential_NoIntersection_AddAdd_ShouldSucceed_Test()
        {
            // TODO: Managed has this at 512, 1024. Making smaller for now
            co_await PopulateConsolidatedAndDifferentialAsync(50, 100, false);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_DifferentialAndConsolidated_BiggerDifferential_NoIntersection_UpdateUpdate_ShouldSucceed_Test()
        {
            // TODO: Managed has this at 512, 1024. Making smaller for now
            co_await PopulateConsolidatedAndDifferentialAsync(50, 100, true);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_DifferentialAndConsolidated_BiggerConsolidated_NoIntersection_AddAdd_ShouldSucceed_Test()
        {
            // TODO: Managed has this at 1024, 512. Making smaller for now
            co_await PopulateConsolidatedAndDifferentialAsync(100, 50, false);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_DifferentialAndConsolidated_BiggerConsolidated_NoIntersection_UpdateUpdate_ShouldSucceed_Test()
        {
            // TODO: Managed has this at 1024, 512. Making smaller for now
            co_await PopulateConsolidatedAndDifferentialAsync(100, 50, true);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_TwoDeltaDifferentials_ZeroOne_ShouldSucceed_Test()
        {
            co_await TwoDeltaTestAsync(0, 1);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_TwoDeltaDifferentials_OneZero_ShouldSucceed_Test()
        {
            co_await TwoDeltaTestAsync(1, 0);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_TwoDeltaDifferentials_OneOne_ShouldSucceed_Test()
        {
            co_await TwoDeltaTestAsync(1, 1);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_TwoDeltaDifferentials_TwoOne_ShouldSucceed_Test()
        {
            co_await TwoDeltaTestAsync(2, 1);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_TwoDeltaDifferentials_OneTwo_ShouldSucceed_Test()
        {
            co_await TwoDeltaTestAsync(1, 2);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_TwoDeltaDifferentials_TwoTwo_ShouldSucceed_Test()
        {
            co_await TwoDeltaTestAsync(2, 2);
            co_return;
        }

        ktl::Awaitable<void> Add100Checkpoint_WithValueRecovery_ShouldSucceed_Test()
    {
        Store->ShouldLoadValuesOnRecovery = true;
        for (LONG64 i = 0; i < 100; i++)
        {
            auto key = i;
            auto value = GenerateString(i);
            co_await AddKeyAsync(key, value);
        }

        co_await CheckpointAndRecoverAsync();

        VerifyCountInAllStores(100);
        for (LONG64 i = 0; i < 100; i++)
        {
            auto key = i;
            auto expectedValue = GenerateString(i);
            co_await VerifyKeyExistsInStoresAsync(key, nullptr, expectedValue, StringEquals);
        }
        co_return;
    }

        ktl::Awaitable<void> Add100Checkpoint_WithValueRecovery_MultipleInflightTasks_ShouldSucceed_Test()
    {
        Store->ShouldLoadValuesOnRecovery = true;
        Store->MaxNumberOfInflightValueRecoveryTasks = 10;

        for (LONG64 i = 0; i < 100; i++)
        {
            auto key = i;
            auto value = GenerateString(i);
            co_await AddKeyAsync(key, value);
        }

        co_await CheckpointAndRecoverAsync();

        VerifyCountInAllStores(100);
        for (LONG64 i = 0; i < 100; i++)
        {
            auto key = i;
            auto expectedValue = GenerateString(i);
            co_await VerifyKeyExistsInStoresAsync(key, nullptr, expectedValue, StringEquals);
        }
        co_return;
    }

        ktl::Awaitable<void> Add1000Checkpoint_ShouldSucceed_Test()
        {
            for (LONG64 i = 0; i < 1000; i++)
            {
                auto key = i;
                auto value = GenerateString(i);
                co_await AddKeyAsync(key, value);
            }

            co_await CheckpointAndRecoverAsync();

            //TODO: Verify checkpoint file size
            VerifyCountInAllStores(1000);
            for (LONG64 i = 0; i < 1000; i++)
            {
                auto key = i;
                auto expectedValue = GenerateString(i);
                co_await VerifyKeyExistsInStoresAsync(key, nullptr, expectedValue, StringEquals);
            }
            co_return;
        }
    #pragma endregion
    };

    BOOST_FIXTURE_TEST_SUITE(StoreCheckpointTestSuite, StoreCheckpointTests)

#pragma region Recovery tests
    BOOST_AUTO_TEST_CASE(ReopenStore_ShouldSucceed)
    {
        SyncAwait(ReopenStore_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_NoState_ShouldSucceed)
    {
        SyncAwait(Checkpoint_NoState_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddCheckpointRead_ShouldSucceed)
    {
        SyncAwait(AddCheckpointRead_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddCheckpointRemove_ShouldSucceed)
    {
        SyncAwait(AddCheckpointRemove_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddCheckpointUpdateRead_ShouldSucceed)
    {
        SyncAwait(AddCheckpointUpdateRead_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(EmptyCheckpoint_AfterNotEmptyCheckpoint_ShouldSucceed)
    {
        SyncAwait(EmptyCheckpoint_AfterNotEmptyCheckpoint_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddUpdateRemoveCheckpoint_MultipleTimes_ShouldSucceed)
    {
        SyncAwait(AddUpdateRemoveCheckpoint_MultipleTimes_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddUpdateCheckpointRead_ShouldSucceed)
    {
        SyncAwait(AddUpdateCheckpointRead_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddRemoveCheckpoint_ShouldSucceed)
    {
        SyncAwait(AddRemoveCheckpoint_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddCheckpointRemoveCheckpoint_ShouldSucceed)
    {
        SyncAwait(AddCheckpointRemoveCheckpoint_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddCheckpointRemoveCheckpointAddCheckpoint_ShouldSucceed)
    {
        SyncAwait(AddCheckpointRemoveCheckpointAddCheckpoint_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddCheckpointReadCheckpointRead_ShouldSucceed)
    {
        SyncAwait(AddCheckpointReadCheckpointRead_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddCheckpointReadUpdateCheckpointRead_ShouldSucceed)
    {
        SyncAwait(AddCheckpointReadUpdateCheckpointRead_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddCheckpointCloseOpenReadUpdateCheckpointCloseOpenRead_ShouldSucceed)
    {
        SyncAwait(AddCheckpointCloseOpenReadUpdateCheckpointCloseOpenRead_ShouldSucceed_Test());
    }
#pragma endregion

#pragma region Prepare Checkpoint tests
    BOOST_AUTO_TEST_CASE(Checkpoint_ZeroLSNPrepare_ShouldSucceed)
    {
        SyncAwait(Checkpoint_ZeroLSNPrepare_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_DuringApply_ShouldSucceed)
    {
        SyncAwait(Checkpoint_DuringApply_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddPrepareRead_ShouldSucceed)
    {
        SyncAwait(AddPrepareRead_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(PreparePreparePerform_ShouldSucceed)
    {
        SyncAwait(PreparePreparePerform_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddUpdatePrepareRemoveCheckpointRecover_Twice_ShouldSucceed)
    {
        SyncAwait(AddUpdatePrepareRemoveCheckpointRecover_Twice_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(ConcurrentReadsDuringCheckpoint_MultipleDifferentials_ShouldSucceed)
    {
        SyncAwait(ConcurrentReadsDuringCheckpoint_MultipleDifferentials_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(ConcurrentReadsAndsCheckpoints_MultipleDifferentials_ShouldSucceed)
    {
        SyncAwait(ConcurrentReadsAndsCheckpoints_MultipleDifferentials_ShouldSucceed_Test());
    }

#pragma endregion

#pragma region Consolidated and Differential State Tests
    BOOST_AUTO_TEST_CASE(Checkpoint_EmptyDifferentialState_ShouldSucceed)
    {
        SyncAwait(Checkpoint_EmptyDifferentialState_ShouldSucceed_Test());
    }
       
    BOOST_AUTO_TEST_CASE(Checkpoint_DifferentialAndConsolidated_SameCount_WithIntersection_DifferentialAndConsolidatedPopulatedInReverseOrder_Add_Delete_ShouldSucceed)
    {
        SyncAwait(Checkpoint_DifferentialAndConsolidated_SameCount_WithIntersection_DifferentialAndConsolidatedPopulatedInReverseOrder_Add_Delete_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_DifferentialAndConsolidated_SameCount_WithIntersection_DifferentialAndConsolidatedPopulatedInReverseOrder_Add_Update_UpdateRepeated_ShouldSucceed)
    {
        SyncAwait(Checkpoint_DifferentialAndConsolidated_SameCount_WithIntersection_DifferentialAndConsolidatedPopulatedInReverseOrder_Add_Update_UpdateRepeated_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_DifferentialAndConsolidated_SameCount_WithIntersection_DifferentialAndConsolidatedPopulatedInReverseOrder_AddPartialUpdate_ShouldSucceed)
    {
        SyncAwait(Checkpoint_DifferentialAndConsolidated_SameCount_WithIntersection_DifferentialAndConsolidatedPopulatedInReverseOrder_AddPartialUpdate_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_DifferentialAndConsolidated_SameCount_NoIntersection_AddAdd_ShouldSucceed)
    {
        SyncAwait(Checkpoint_DifferentialAndConsolidated_SameCount_NoIntersection_AddAdd_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_DifferentialAndConsolidated_SameCount_NoIntersection_UpdateUpdate_ShouldSucceed)
    {
        SyncAwait(Checkpoint_DifferentialAndConsolidated_SameCount_NoIntersection_UpdateUpdate_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_DifferentialAndConsolidated_BiggerDifferential_NoIntersection_AddAdd_ShouldSucceed)
    {
        SyncAwait(Checkpoint_DifferentialAndConsolidated_BiggerDifferential_NoIntersection_AddAdd_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_DifferentialAndConsolidated_BiggerDifferential_NoIntersection_UpdateUpdate_ShouldSucceed)
    {
        SyncAwait(Checkpoint_DifferentialAndConsolidated_BiggerDifferential_NoIntersection_UpdateUpdate_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_DifferentialAndConsolidated_BiggerConsolidated_NoIntersection_AddAdd_ShouldSucceed)
    {
        SyncAwait(Checkpoint_DifferentialAndConsolidated_BiggerConsolidated_NoIntersection_AddAdd_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_DifferentialAndConsolidated_BiggerConsolidated_NoIntersection_UpdateUpdate_ShouldSucceed)
    {
        SyncAwait(Checkpoint_DifferentialAndConsolidated_BiggerConsolidated_NoIntersection_UpdateUpdate_ShouldSucceed_Test());
    }
#pragma endregion

#pragma region Checkpoint 2 Delta Tests

    BOOST_AUTO_TEST_CASE(Checkpoint_TwoDeltaDifferentials_ZeroOne_ShouldSucceed)
    {
        SyncAwait(Checkpoint_TwoDeltaDifferentials_ZeroOne_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_TwoDeltaDifferentials_OneZero_ShouldSucceed)
    {
        SyncAwait(Checkpoint_TwoDeltaDifferentials_OneZero_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_TwoDeltaDifferentials_OneOne_ShouldSucceed)
    {
        SyncAwait(Checkpoint_TwoDeltaDifferentials_OneOne_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_TwoDeltaDifferentials_TwoOne_ShouldSucceed)
    {
        SyncAwait(Checkpoint_TwoDeltaDifferentials_TwoOne_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_TwoDeltaDifferentials_OneTwo_ShouldSucceed)
    {
        SyncAwait(Checkpoint_TwoDeltaDifferentials_OneTwo_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_TwoDeltaDifferentials_TwoTwo_ShouldSucceed)
    {
        SyncAwait(Checkpoint_TwoDeltaDifferentials_TwoTwo_ShouldSucceed_Test());
    }

#pragma endregion

BOOST_AUTO_TEST_CASE(Add100Checkpoint_WithValueRecovery_ShouldSucceed)
{
    SyncAwait(Add100Checkpoint_WithValueRecovery_ShouldSucceed_Test());
}

BOOST_AUTO_TEST_CASE(Add100Checkpoint_WithValueRecovery_MultipleInflightTasks_ShouldSucceed)
{
    SyncAwait(Add100Checkpoint_WithValueRecovery_MultipleInflightTasks_ShouldSucceed_Test());
}

#pragma region Performance Tests
    BOOST_AUTO_TEST_CASE(Add1000Checkpoint_ShouldSucceed)
    {
        SyncAwait(Add1000Checkpoint_ShouldSucceed_Test());
    }
#pragma endregion
    BOOST_AUTO_TEST_SUITE_END()
}

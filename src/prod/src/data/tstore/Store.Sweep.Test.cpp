// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'ssTP'

namespace TStoreTests
{
    using namespace ktl;

    class StoreSweepTest : public TStoreTestBase<LONG64, KString::SPtr, LongComparer, TestStateSerializer<LONG64>, StringStateSerializer>
    {
    public:
        StoreSweepTest()
        {
            Setup(1);
            Store->EnableSweep = false;  // Sweep will be manually called
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
        }

        ~StoreSweepTest()
        {
            Cleanup();
        }

        KString::SPtr CreateString(__in LPCWSTR value)
        {
            KString::SPtr result;
            auto status = KString::Create(result, GetAllocator(), value);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            return result;
        }

        KString::SPtr CreateString(__in ULONG seed)
        {
            KString::SPtr value;
            wstring str = wstring(L"test_value") + to_wstring(seed);
            auto status = KString::Create(value, GetAllocator(), str.c_str());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            return value;
        }

        void SweepConsolidatedState()
        {
           Store->ConsolidationManagerSPtr->SweepConsolidatedState(ktl::CancellationToken::None);
        }

        void Sweep()
        {
           Store->SweepManagerSPtr->Sweep();
        }

        ktl::Awaitable<void> CheckpointAndSweepAsync()
        {
            co_await CheckpointAsync();
            SweepConsolidatedState();
            co_return;
        }

        LONG64 GetSerializedSize(__in KString & value)
        {
           BinaryWriter writer(GetAllocator());
           KString::SPtr valueSPtr = &value;
           Store->ValueConverterSPtr->Write(valueSPtr, writer);
           return writer.GetBuffer(0)->QuerySize();
        }

        LONG64 GetKeysAndMetadataSize(
           __in LONG64 estimatedKeySize,
           __in ULONG32 numDifferential,
           __in ULONG32 numConsolidated)
        {
           return numDifferential * (2 * Constants::ValueMetadataSizeBytes + estimatedKeySize) + numConsolidated * (estimatedKeySize + Constants::ValueMetadataSizeBytes);
        }

        static bool EqualityFunction(KString::SPtr & one, KString::SPtr & two)
        {
            if (one == nullptr || two == nullptr)
            {
                return one == two;
            }

            return one->Compare(*two) == 0;
        }

        Common::CommonConfig config; // load the config object as its needed for the tracing to work

#pragma region test functions
    public:
        ktl::Awaitable<void> Sweep_ItemsInOldConsolidatedState_ShouldBeSwept_Test()
        {
            // Add one item to the store
            LONG64 key = 1;
            KString::SPtr value = CreateString(L"value");

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            // Perform checkpoint twice

            // 1. Sweep does not touch this item since it just got moved from differential
            co_await CheckpointAsync();
            SweepConsolidatedState();

            // No readers
            VersionedItem<KString::SPtr>::SPtr versionedItem = Store->ConsolidationManagerSPtr->Read(key);
            CODING_ERROR_ASSERT(versionedItem->GetValue() != nullptr);
            CODING_ERROR_ASSERT(versionedItem->GetInUse() == false);
        
            // 2. Sweep again, item should be swept out
            SweepConsolidatedState();

            // No readers, read from consolidation manager again
            versionedItem = Store->ConsolidationManagerSPtr->Read(key);
            CODING_ERROR_ASSERT(versionedItem->GetValue() == nullptr);
            CODING_ERROR_ASSERT(versionedItem->GetInUse() == false);

            LONG64 expectedSize = GetKeysAndMetadataSize(Store->GetEstimatedKeySize(), 0, 1);
            LONG64 actualSize = Store->Size;
            CODING_ERROR_ASSERT(actualSize == expectedSize);
            co_return;
        }

        ktl::Awaitable<void> Sweep_ItemsThatJustCameFromDifferential_ShouldNotBeSweptOnConsolidation_Test()
        {
            // Add one item to the store
            LONG64 key = 1;
            KString::SPtr value = CreateString(L"value");

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            // Perform checkpoint twice
            co_await CheckpointAsync();
            SweepConsolidatedState();

            // No readers
            VersionedItem<KString::SPtr>::SPtr versionedItem = Store->ConsolidationManagerSPtr->Read(key);

            // Should not have been swept
            CODING_ERROR_ASSERT(versionedItem != nullptr);
            CODING_ERROR_ASSERT(versionedItem->GetValue()->Compare(*value) == 0);

            // This should be false because only the top level read api from consolidated state sets the InUse flag to true
            CODING_ERROR_ASSERT(versionedItem->GetInUse() == false);

            LONG64 expectedSize = GetSerializedSize(*value) + GetKeysAndMetadataSize(Store->GetEstimatedKeySize(), 0, 1);
            LONG64 actualSize = Store->Size;

            CODING_ERROR_ASSERT(expectedSize == actualSize);
            co_return;
        }

        ktl::Awaitable<void> Sweep_WhenThereIsAReader_ItemShouldNotBeSwept_Test()
        {
            // Add one item in the store
            LONG64 key = 1;
            KString::SPtr value = CreateString(L"value");

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            // Perform checkpoint twice
            co_await CheckpointAsync();
            SweepConsolidatedState();

            // No readers
            VersionedItem<KString::SPtr>::SPtr versionedItem = Store->ConsolidationManagerSPtr->Read(key);
            CODING_ERROR_ASSERT(versionedItem->GetValue() != nullptr);
            CODING_ERROR_ASSERT(versionedItem->GetInUse() == false);
            CODING_ERROR_ASSERT(versionedItem->GetValue()->Compare(*value) == 0);

            // Read data
            co_await VerifyKeyExistsAsync(*Store, key, nullptr, value, StoreSweepTest::EqualityFunction);
            CODING_ERROR_ASSERT(versionedItem->GetInUse() == true);

            // Perform checkpoint again
            SweepConsolidatedState();

            // No readers
            versionedItem = Store->ConsolidationManagerSPtr->Read(key);

            CODING_ERROR_ASSERT(versionedItem->GetValue() != nullptr);
            CODING_ERROR_ASSERT(versionedItem->GetInUse() == false);
            CODING_ERROR_ASSERT(versionedItem->GetValue()->Compare(*value) == 0);

            LONG64 expectedSize = GetSerializedSize(*value) + GetKeysAndMetadataSize(Store->GetEstimatedKeySize(), 0, 1);
            LONG64 actualSize = Store->Size;

            CODING_ERROR_ASSERT(expectedSize == actualSize);
            co_return;
        }

        ktl::Awaitable<void> Sweep_CheckpointAfterRead_ItemShouldBeSwept_Test()
        {
            LONG64 key = 1;
            KString::SPtr value = CreateString(L"value");

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();

            // Does not simulate read
            VersionedItem<KString::SPtr>::SPtr versionedItem = Store->ConsolidationManagerSPtr->Read(key);
            CODING_ERROR_ASSERT(versionedItem->GetValue() != nullptr);
            CODING_ERROR_ASSERT(versionedItem->GetInUse() == true);

            // Sweep, the should reset the inuse flag
            SweepConsolidatedState();
            CODING_ERROR_ASSERT(versionedItem->GetValue() != nullptr);
            CODING_ERROR_ASSERT(versionedItem->GetInUse() == false);

            // Read data
            co_await VerifyKeyExistsAsync(*Store, key, nullptr, value, StoreSweepTest::EqualityFunction);
            CODING_ERROR_ASSERT(versionedItem->GetValue() != nullptr);
            CODING_ERROR_ASSERT(versionedItem->GetInUse() == true);

            // Sweep again and the item should not have been swept
            SweepConsolidatedState();
            CODING_ERROR_ASSERT(versionedItem->GetValue() != nullptr);
            CODING_ERROR_ASSERT(versionedItem->GetInUse() == false);

            // Sweep again
            SweepConsolidatedState();
            CODING_ERROR_ASSERT(versionedItem->GetValue() == nullptr);
            CODING_ERROR_ASSERT(versionedItem->GetInUse() == false);

            LONG64 expecetdSize = GetKeysAndMetadataSize(Store->GetEstimatedKeySize(), 0, 1);
            LONG64 actualSize = Store->Size;
            CODING_ERROR_ASSERT(actualSize == expecetdSize);
            co_return;
        }

        ktl::Awaitable<void> Sweep_WithMerge_ShouldSucceed_Test()
        {
            Store->EnableSweep = true;
            // TODO: FileCount policy doesn't work background consolidation
            Store->MergeHelperSPtr->CurrentMergePolicy = MergePolicy::InvalidEntries;
            Store->MergeHelperSPtr->NumberOfInvalidEntries = 1;
            Store->SetKeySize(8);

            LONG64 key1 = 1;
            LONG64 key2 = 2;

            KString::SPtr value = CreateString(L"value");

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key1, value, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key1, value, DefaultTimeout, ktl::CancellationToken::None);
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key2, value, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key1, value, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();
            SweepConsolidatedState();
            SweepConsolidatedState();

            LONG64 expectedSize = GetKeysAndMetadataSize(Store->GetEstimatedKeySize(), 0, 2);
            LONG64 actualSize = Store->Size;
            CODING_ERROR_ASSERT(actualSize == expectedSize);
        
            co_await VerifyKeyExistsInStoresAsync(key1, nullptr, value, StoreSweepTest::EqualityFunction);
            co_await VerifyKeyExistsInStoresAsync(key2, nullptr, value, StoreSweepTest::EqualityFunction);

            expectedSize = (GetSerializedSize(*value) * 2) + GetKeysAndMetadataSize(Store->GetEstimatedKeySize(), 0, 2);
            actualSize = Store->Size;

            CODING_ERROR_ASSERT(expectedSize == actualSize);
            co_return;
        }

        ktl::Awaitable<void> AddSweepUpdate_ShouldSucceed_Test()
        {
            LONG64 key = 17;
            KString::SPtr value = CreateString(L"value");
            KString::SPtr value2 = CreateString(L"update");

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();
            SweepConsolidatedState();
            SweepConsolidatedState();

            {
                auto txn = CreateWriteTransaction();
                bool updated = co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value2, DefaultTimeout, ktl::CancellationToken::None);
                CODING_ERROR_ASSERT(updated);
                co_await txn->CommitAsync();
            }

            LONG64 expectedSize = GetSerializedSize(*value2) + GetKeysAndMetadataSize(Store->GetEstimatedKeySize(), 1, 1);
            LONG64 actualSize = Store->Size;

            CODING_ERROR_ASSERT(expectedSize == actualSize)
            co_return;
        }

        ktl::Awaitable<void> Sweep_VerifyValueOnRead_ShouldSucceed_Test()
        {
            LONG64 key = 17;
            KString::SPtr value = CreateString(L"value");

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();
            SweepConsolidatedState();
            SweepConsolidatedState();

            VersionedItem<KString::SPtr>::SPtr versionedItem = Store->ConsolidationManagerSPtr->Read(key);
            CODING_ERROR_ASSERT(versionedItem->GetInUse() == false);
            CODING_ERROR_ASSERT(versionedItem->GetValue() == nullptr);

            LONG64 expectedSize = GetKeysAndMetadataSize(Store->GetEstimatedKeySize(), 0, 1);
            LONG64 actualSize = Store->Size;

            CODING_ERROR_ASSERT(expectedSize == actualSize)

            co_await VerifyKeyExistsAsync(*Store, key, nullptr, value, StoreSweepTest::EqualityFunction);

            expectedSize = GetSerializedSize(*value) + GetKeysAndMetadataSize(Store->GetEstimatedKeySize(), 0, 1);
            actualSize = Store->Size;

            CODING_ERROR_ASSERT(expectedSize == actualSize)
            co_return;
        }

        ktl::Awaitable<void> CheckpointRecoverSweepRead_ShouldSucceed_Test()
        {
            LONG64 key = 17;
            KString::SPtr value = CreateString(L"value");

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();
            co_await CloseAndReOpenStoreAsync();
            SweepConsolidatedState();
            SweepConsolidatedState();

            VersionedItem<KString::SPtr>::SPtr versionedItem = Store->ConsolidationManagerSPtr->Read(key);
            CODING_ERROR_ASSERT(versionedItem->GetInUse() == false);
            CODING_ERROR_ASSERT(versionedItem->GetValue() == nullptr);

            LONG64 expectedSize = GetKeysAndMetadataSize(Store->GetEstimatedKeySize(), 0, 1);
            LONG64 actualSize = Store->Size;

            CODING_ERROR_ASSERT(expectedSize == actualSize)

            co_await VerifyKeyExistsAsync(*Store, key, nullptr, value, StoreSweepTest::EqualityFunction);

            expectedSize = GetSerializedSize(*value) + GetKeysAndMetadataSize(Store->GetEstimatedKeySize(), 0, 1);
            actualSize = Store->Size;

            CODING_ERROR_ASSERT(expectedSize == actualSize)
            co_return;
        }

        ktl::Awaitable<void> CheckpointWithSweep_ItemsInOldConsolidatedState_ShouldBeSwept_Test()
        {
            Store->EnableSweep = true;
            Store->EnableBackgroundConsolidation = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            LONG64 key = 1;
            KString::SPtr value = CreateString(L"value");

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }
        
            // Sweep does not touch this item since it just got moved from differential
            co_await CheckpointAndSweepAsync();

            // Sweep again, item should be swept out
            co_await CheckpointAndSweepAsync();

            // Read from consolidated should succeed
            co_await VerifyKeyExistsAsync(*Store, key, nullptr, value, StoreSweepTest::EqualityFunction);
            co_return;
        }

        ktl::Awaitable<void> CheckpointWithSweep_ItemsThatJustCameFromDifferential_ShouldNotBeSweptOnConsolidation_Test()
        {
            Store->EnableSweep = true;
            Store->EnableBackgroundConsolidation = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            // Add one item to the store
            LONG64 key = 1;
            KString::SPtr value = CreateString(L"value");

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }
        
            co_await CheckpointAndSweepAsync();

            co_await VerifyKeyExistsAsync(*Store, key, nullptr, value, StoreSweepTest::EqualityFunction);
            co_return;
        }

        ktl::Awaitable<void> CheckpointWithSweep_WhenThereIsAReader_ItemShouldNotBeSwept_Test()
        {
            Store->EnableSweep = true;
            Store->EnableBackgroundConsolidation = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            // Add one item in the store
            LONG64 key = 1;
            KString::SPtr value = CreateString(L"value");

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAndSweepAsync();

            // Read data
            co_await VerifyKeyExistsAsync(*Store, key, nullptr, value, StoreSweepTest::EqualityFunction);

            // Perform checkpoint again
            co_await CheckpointAndSweepAsync();

            // No readers
            VersionedItem<KString::SPtr>::SPtr versionedItem = Store->ConsolidationManagerSPtr->Read(key);

            CODING_ERROR_ASSERT(versionedItem->GetValue() != nullptr);
            CODING_ERROR_ASSERT(versionedItem->GetInUse() == false);
            CODING_ERROR_ASSERT(versionedItem->GetValue()->Compare(*value) == 0);

            co_await VerifyKeyExistsAsync(*Store, key, nullptr, value, StoreSweepTest::EqualityFunction);
            co_return;
        }

        ktl::Awaitable<void> CheckpointWithSweep_CheckpointAfterRead_ItemShouldBeSwept_Test()
        {
            Store->EnableSweep = true;
            Store->EnableBackgroundConsolidation = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            LONG64 key = 1;
            KString::SPtr value = CreateString(L"value");

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }
        
            // Sweep and read
            co_await CheckpointAndSweepAsync();
            co_await VerifyKeyExistsAsync(*Store, key, nullptr, value, StoreSweepTest::EqualityFunction);

            // Sweep again and the item should not have been swept
            co_await CheckpointAndSweepAsync();
            co_await VerifyKeyExistsAsync(*Store, key, nullptr, value, StoreSweepTest::EqualityFunction);

            // Sweep again
            co_await CheckpointAndSweepAsync();
            co_await VerifyKeyExistsAsync(*Store, key, nullptr, value, StoreSweepTest::EqualityFunction);
            co_return;
        }

        ktl::Awaitable<void> CheckpointWithSweep_AddSweepUpdate_ShouldSucceed_Test()
        {
            Store->EnableSweep = true;
            Store->EnableBackgroundConsolidation = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            LONG64 key = 17;
            KString::SPtr value = CreateString(L"value");
            KString::SPtr value2 = CreateString(L"update");

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }
        
            co_await CheckpointAndSweepAsync();
            co_await CheckpointAndSweepAsync();
            co_await CheckpointAndSweepAsync();

            {
                auto txn = CreateWriteTransaction();
                bool updated = co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value2, DefaultTimeout, ktl::CancellationToken::None);
                CODING_ERROR_ASSERT(updated);
                co_await txn->CommitAsync();
            }
            co_return;
        }

        ktl::Awaitable<void> CheckpointRecoverCheckpointSweepRead_ShouldSucceed_Test()
        {
            Store->EnableSweep = true;
            Store->EnableBackgroundConsolidation = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            LONG64 key = 17;
            KString::SPtr value = CreateString(L"value");

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAndSweepAsync();
            co_await CloseAndReOpenStoreAsync();

            Store->EnableSweep = true;
            Store->EnableBackgroundConsolidation = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            co_await CheckpointAndSweepAsync();
            co_await CheckpointAndSweepAsync();
            co_await CheckpointAndSweepAsync();

            co_await VerifyKeyExistsAsync(*Store, key, nullptr, value, StoreSweepTest::EqualityFunction);
            co_return;
        }

        ktl::Awaitable<void> CheckpointWithSweep_AddUpdateRemoveMany_ShouldSucceed_Test()
        {
            Store->EnableSweep = true;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 3;

            LONG64 key = 17;
            KString::SPtr value = CreateString(L"value");

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None);
                co_await txn->CommitAsync();
            }
        
            for (ULONG32 i = 0; i < 81; i++)
            {
                switch (i % 3)
                {
                case 0: 
                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None);
                    co_await txn->CommitAsync();
                }
                break;
                case 1: 
                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, ktl::CancellationToken::None);
                    co_await txn->CommitAsync();
                }
                break;
                case 2: 
                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None);
                    co_await txn->CommitAsync();
                }
                break;
                }
                co_await CheckpointAsync();
            }
        
            co_await CheckpointAsync();
            co_await CloseAndReOpenStoreAsync();

            Store->EnableSweep = true;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 3;

            VerifyCountInAllStores(1);
            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value, StoreSweepTest::EqualityFunction);
            co_return;
        }

        ktl::Awaitable<void> Sweep_ManyItems_ShouldSucceed_Test()
        {
            Store->EnableSweep = true;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 3;

            for (ULONG32 startKey = 0; startKey < 100; startKey += 10)
            {
                for (ULONG32 key = startKey; key < startKey + 20; key++)
                {
                    {
                        auto txn = CreateWriteTransaction();
                    
                        KeyValuePair<LONG64, KString::SPtr> output;
                        bool found = co_await Store->ConditionalGetAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, output, ktl::CancellationToken::None);
                        if (!found)
                        {
                            co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, CreateString(key), DefaultTimeout, ktl::CancellationToken::None);
                        }
                        else
                        {
                            co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, CreateString(key), DefaultTimeout, ktl::CancellationToken::None);
                        }

                        co_await txn->CommitAsync();
                    }
                }
                co_await CheckpointAsync();
            }

            for (ULONG32 key = 0; key < 110; key++)
            {
                co_await VerifyKeyExistsInStoresAsync(key, nullptr, CreateString(key), StoreSweepTest::EqualityFunction);
            }

            co_await CloseAndReOpenStoreAsync();
            Store->EnableSweep = true;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 3;

            for (ULONG32 key = 0; key < 110; key++)
            {
                co_await VerifyKeyExistsInStoresAsync(key, nullptr, CreateString(key), StoreSweepTest::EqualityFunction);
            }
            co_return;
        }

        ktl::Awaitable<void> CompleteCheckpoint_WithConcurrentReads_ShouldSucceed_Test()
        {
            Store->EnableSweep = true;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            for (LONG64 i = 0; i < 1000; i++)
            {
                auto key = i;
                auto value = CreateString(static_cast<ULONG>(i));
                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None);
                    co_await txn->CommitAsync();
                }
            }

            // Checkpoint multiple times to sweep items
            co_await CheckpointAsync();
            co_await CheckpointAsync();
            co_await CheckpointAsync();

            LONG64 checkpointLSN = Replicator->IncrementAndGetCommitSequenceNumber();
            Store->PrepareCheckpoint(checkpointLSN);
            co_await Store->PerformCheckpointAsync(ktl::CancellationToken::None);

            KSharedArray<ktl::Awaitable<bool>>::SPtr getTasks = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ktl::Awaitable<bool>>();

            {
                auto txn = CreateWriteTransaction();
                for (LONG64 i = 0; i < 1000; i++)
                {
                    KeyValuePair<LONG64, KString::SPtr> result(-1, nullptr);
                    auto task = Store->ConditionalGetAsync(*txn->StoreTransactionSPtr, i, DefaultTimeout, result, ktl::CancellationToken::None);
                    getTasks->Append(Ktl::Move(task));
                }

                auto whenAllTask = StoreUtilities::WhenAll<bool>(*getTasks, GetAllocator());

                co_await Store->CompleteCheckpointAsync(ktl::CancellationToken::None);
                co_await whenAllTask;

                co_await txn->AbortAsync();
            }
            co_return;
        }
    #pragma endregion
    };

    BOOST_FIXTURE_TEST_SUITE(StoreSweepTestSuite, StoreSweepTest)

#pragma region Consolidation Manager Sweep Unit Tests
    BOOST_AUTO_TEST_CASE(Sweep_ItemsInOldConsolidatedState_ShouldBeSwept)
    {
        SyncAwait(Sweep_ItemsInOldConsolidatedState_ShouldBeSwept_Test());
    }

    BOOST_AUTO_TEST_CASE(Sweep_ItemsThatJustCameFromDifferential_ShouldNotBeSweptOnConsolidation)
    {
        SyncAwait(Sweep_ItemsThatJustCameFromDifferential_ShouldNotBeSweptOnConsolidation_Test());
    }

    BOOST_AUTO_TEST_CASE(Sweep_WhenThereIsAReader_ItemShouldNotBeSwept)
    {
        SyncAwait(Sweep_WhenThereIsAReader_ItemShouldNotBeSwept_Test());
    }

    BOOST_AUTO_TEST_CASE(Sweep_CheckpointAfterRead_ItemShouldBeSwept)
    {
        SyncAwait(Sweep_CheckpointAfterRead_ItemShouldBeSwept_Test());
    }

    BOOST_AUTO_TEST_CASE(Sweep_WithMerge_ShouldSucceed)
    {
        SyncAwait(Sweep_WithMerge_ShouldSucceed_Test());
    }

    //BOOST_AUTO_TEST_CASE(Sweep_WithNullValues_ShouldSucceed, * boost::unit_test_framework::disabled(/* StringStateSerializer doesn't support null values */))
    //{
    //    LONG64 key = 1;

    //    KString::SPtr value1 = nullptr;
    //    KString::SPtr value2 = CreateString(L"value");

    //    {
    //        auto txn = CreateWriteTransaction();
    //        SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value1, DefaultTimeout, ktl::CancellationToken::None));
    //        SyncAwait(txn->CommitAsync());
    //    }
    //    
    //    Checkpoint();

    //    VersionedItem<KString::SPtr>::SPtr versionedItem = Store->ConsolidationManagerSPtr->Read(key);
    //    CODING_ERROR_ASSERT(versionedItem->GetValue() == nullptr);
    //    CODING_ERROR_ASSERT(versionedItem->GetInUse() == true);

    //    TriggerSweep();

    //    CODING_ERROR_ASSERT(versionedItem->GetValue() == nullptr);
    //    CODING_ERROR_ASSERT(versionedItem->GetInUse() == true);

    //    TriggerSweep();

    //    CODING_ERROR_ASSERT(versionedItem->GetValue() == nullptr);
    //    CODING_ERROR_ASSERT(versionedItem->GetInUse() == true);

    //    CloseAndReOpenStore();

    //    Store->EnableSweep = false;
    //    Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

    //    CODING_ERROR_ASSERT(versionedItem->GetValue() == nullptr);
    //    CODING_ERROR_ASSERT(versionedItem->GetInUse() == true);

    //    SyncAwait(VerifyKeyExistsAsync(*Store, key, nullptr, value1, StoreSweepTest::EqualityFunction));

    //    CODING_ERROR_ASSERT(versionedItem->GetValue() == nullptr);
    //    CODING_ERROR_ASSERT(versionedItem->GetInUse() == true);

    //    {
    //        auto txn = CreateWriteTransaction();
    //        SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value2, DefaultTimeout, ktl::CancellationToken::None));
    //        SyncAwait(txn->CommitAsync());
    //    }

    //    versionedItem = Store->DifferentialState->Read(key);
    //    CODING_ERROR_ASSERT(versionedItem->GetValue() != nullptr);
    //    CODING_ERROR_ASSERT(versionedItem->GetInUse() == true);

    //    Checkpoint();

    //    versionedItem = Store->ConsolidationManagerSPtr->Read(key);
    //    CODING_ERROR_ASSERT(versionedItem->GetValue() != nullptr);
    //    CODING_ERROR_ASSERT(versionedItem->GetInUse() == true);

    //    TriggerSweep();

    //    CODING_ERROR_ASSERT(versionedItem->GetValue() != nullptr);
    //    CODING_ERROR_ASSERT(versionedItem->GetInUse() == false);

    //    TriggerSweep();

    //    CODING_ERROR_ASSERT(versionedItem->GetValue() == nullptr);
    //    CODING_ERROR_ASSERT(versionedItem->GetInUse() == false);
    //}

    BOOST_AUTO_TEST_CASE(AddSweepUpdate_ShouldSucceed)
    {
        SyncAwait(AddSweepUpdate_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Sweep_VerifyValueOnRead_ShouldSucceed)
    {
        SyncAwait(Sweep_VerifyValueOnRead_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(CheckpointRecoverSweepRead_ShouldSucceed)
    {
        SyncAwait(CheckpointRecoverSweepRead_ShouldSucceed_Test());
    }
#pragma endregion

#pragma region Store Sweep tests

    BOOST_AUTO_TEST_CASE(CheckpointWithSweep_ItemsInOldConsolidatedState_ShouldBeSwept)
    {
        SyncAwait(CheckpointWithSweep_ItemsInOldConsolidatedState_ShouldBeSwept_Test());
    }

    BOOST_AUTO_TEST_CASE(CheckpointWithSweep_ItemsThatJustCameFromDifferential_ShouldNotBeSweptOnConsolidation)
    {
        SyncAwait(CheckpointWithSweep_ItemsThatJustCameFromDifferential_ShouldNotBeSweptOnConsolidation_Test());
    }

    BOOST_AUTO_TEST_CASE(CheckpointWithSweep_WhenThereIsAReader_ItemShouldNotBeSwept)
    {
        SyncAwait(CheckpointWithSweep_WhenThereIsAReader_ItemShouldNotBeSwept_Test());
    }

    BOOST_AUTO_TEST_CASE(CheckpointWithSweep_CheckpointAfterRead_ItemShouldBeSwept)
    {
        SyncAwait(CheckpointWithSweep_CheckpointAfterRead_ItemShouldBeSwept_Test());
    }

    BOOST_AUTO_TEST_CASE(CheckpointWithSweep_AddSweepUpdate_ShouldSucceed)
    {
        SyncAwait(CheckpointWithSweep_AddSweepUpdate_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(CheckpointRecoverCheckpointSweepRead_ShouldSucceed)
    {
        SyncAwait(CheckpointRecoverCheckpointSweepRead_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(CheckpointWithSweep_AddUpdateRemoveMany_ShouldSucceed)
    {
        SyncAwait(CheckpointWithSweep_AddUpdateRemoveMany_ShouldSucceed_Test());
    }
    
    BOOST_AUTO_TEST_CASE(Sweep_ManyItems_ShouldSucceed)
    {
        SyncAwait(Sweep_ManyItems_ShouldSucceed_Test());
    }

#pragma endregion

    BOOST_AUTO_TEST_CASE(CompleteCheckpoint_WithConcurrentReads_ShouldSucceed)
    {
        SyncAwait(CompleteCheckpoint_WithConcurrentReads_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_SUITE_END()
}

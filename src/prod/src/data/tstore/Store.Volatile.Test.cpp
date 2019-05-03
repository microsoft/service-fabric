// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'stSV'

namespace TStoreTests
{
    using namespace ktl;

    class VolatileStoreTest : public TStoreTestBase<KBuffer::SPtr, KBuffer::SPtr, KBufferComparer, KBufferSerializer, KBufferSerializer>
    {
    public:
        VolatileStoreTest()
        {
            Setup(1, DefaultHash, nullptr, /*hasPersistedState: */false);
        }

        ~VolatileStoreTest()
        {
            Cleanup();
        }

        KBuffer::SPtr CreateBuffer(__in ULONG32 sizeInBytes, __in ULONG32 fillValue)
        {
            CODING_ERROR_ASSERT(sizeInBytes % sizeof(ULONG32) == 0);
            KBuffer::SPtr resultSPtr = nullptr;
            KBuffer::Create(sizeInBytes, resultSPtr, this->GetAllocator());

            ULONG32* buffer = static_cast<ULONG32 *>(resultSPtr->GetBuffer());

            auto numElements = sizeInBytes / sizeof(ULONG32);
            for (ULONG32 i = 0; i < numElements; i++)
            {
                buffer[i] = fillValue;
            }

            return resultSPtr;
        }

        OperationData::SPtr GetBytes(__in KBuffer::SPtr & value)
        {
            Utilities::BinaryWriter binaryWriter(GetAllocator());
            Store->KeyConverterSPtr->Write(value, binaryWriter);

            OperationData::SPtr operationDataSPtr = OperationData::Create(this->GetAllocator());
            operationDataSPtr->Append(*binaryWriter.GetBuffer(0));

            return operationDataSPtr;
        }

        ktl::Awaitable<void> SecondaryApplyAsync(
            __in Data::TStore::Store<KBuffer::SPtr, KBuffer::SPtr> & secondaryStore,
            __in Transaction & transaction,
            __in LONG64 commitSequenceNumber,
            __in StoreModificationType::Enum operationType,
            __in KBuffer::SPtr key,
            __in OperationData & keyBytes,
            __in KBuffer::SPtr value,
            __in OperationData::SPtr & valueBytes)
        {
            KAllocator& allocator = GetAllocator();

            transaction.CommitSequenceNumber = commitSequenceNumber;

            OperationData::CSPtr metadataCSPtr = nullptr;

            if (valueBytes != nullptr)
            {
                KSharedPtr<MetadataOperationDataKV<KBuffer::SPtr, KBuffer::SPtr> const> metadataKVCSPtr = nullptr;
                NTSTATUS status = MetadataOperationDataKV<KBuffer::SPtr, KBuffer::SPtr>::Create(
                    key,
                    value,
                    Constants::SerializedVersion,
                    operationType,
                    transaction.TransactionId,
                    &keyBytes,
                    allocator,
                    metadataKVCSPtr);
                Diagnostics::Validate(status);
                metadataCSPtr = static_cast<const OperationData* const>(metadataKVCSPtr.RawPtr());
            }
            else
            {
                KSharedPtr<MetadataOperationDataK<KBuffer::SPtr> const> metadataKCSPtr = nullptr;
                NTSTATUS status = MetadataOperationDataK<KBuffer::SPtr>::Create(
                    key,
                    Constants::SerializedVersion,
                    operationType,
                    transaction.TransactionId,
                    &keyBytes,
                    allocator,
                    metadataKCSPtr);
                Diagnostics::Validate(status);
                metadataCSPtr = static_cast<const OperationData* const>(metadataKCSPtr.RawPtr());
            }

            RedoUndoOperationData::SPtr redoDataSPtr = nullptr;
            NTSTATUS status = RedoUndoOperationData::Create(GetAllocator(), valueBytes, nullptr, redoDataSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status))

            auto operationContext = co_await secondaryStore.ApplyAsync(
                commitSequenceNumber,
                transaction,
                ApplyContext::SecondaryRedo,
                metadataCSPtr.RawPtr(),
                redoDataSPtr.RawPtr());

            if (operationContext != nullptr)
            {
                secondaryStore.Unlock(*operationContext);
            }

            co_return;
        }

        ktl::Awaitable<void> SecondaryAddAsync(
            __in Data::TStore::Store<KBuffer::SPtr, KBuffer::SPtr> & secondaryStore,
            __in Transaction & transaction,
            __in LONG64 commitSequenceNumber,
            __in KBuffer::SPtr & key, 
            __in KBuffer::SPtr & value)
        {
            auto keyBytesSPtr = GetBytes(key);
            auto valueBytesSPtr = GetBytes(value);
            co_await SecondaryApplyAsync(secondaryStore, transaction, commitSequenceNumber, StoreModificationType::Enum::Add, key, *keyBytesSPtr, value, valueBytesSPtr);
            co_return;
        }

        VersionedItem<KBuffer::SPtr>::SPtr CreateInsertedVersionedItem(
            __in KBuffer::SPtr const & valueBufferSPtr,
            __in ULONG32 valueSize,
            __in ULONG32 versionSequenceNumber = -1)
        {
            InsertedVersionedItem<KBuffer::SPtr>::SPtr itemSPtr;
            InsertedVersionedItem<KBuffer::SPtr>::Create(GetAllocator(), itemSPtr);

            itemSPtr->InitializeOnApply(versionSequenceNumber, valueBufferSPtr);
            itemSPtr->SetValueSize(valueSize);

            return itemSPtr.DownCast<VersionedItem<KBuffer::SPtr>>();
        }

        static bool EqualityFunction(KBuffer::SPtr & one, KBuffer::SPtr & two)
        {
            return *one == *two;
        }

        Awaitable<void> PopulateStoreAsync(__in ULONG32 count, __in ULONG32 checkpointFrequency = 0, __in ULONG32 startingKey = 0)
        {
            for (ULONG32 keySeed = startingKey; keySeed < startingKey + count; keySeed++)
            {
                auto key = CreateBuffer(8, keySeed);
                auto value = CreateBuffer(16, keySeed);

                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }

                if (checkpointFrequency != 0 && keySeed % checkpointFrequency == 0)
                {
                    co_await CheckpointAsync();
                }
            }

            co_return;
        }

        Awaitable<void> VerifyStateAsync(__in ULONG32 keyCount)
        {
            for (ULONG32 i = 0; i < keyCount; i++)
            {
                auto key = CreateBuffer(8, i);
                auto value = CreateBuffer(16, i);
                co_await VerifyKeyExistsInStoresAsync(key, nullptr, value, KBufferComparer::Equals);
            }

            co_return;
        }

        Awaitable<void> FullCopyTestAsync(__in ULONG32 numItems)
        {
            co_await PopulateStoreAsync(numItems);

            // Copy state to a new secondary
            auto secondaryStore = co_await CreateSecondaryAsync();
            co_await FullCopyToSecondaryAsync(*secondaryStore);

            // Verify all keys copied
            co_await VerifyStateAsync(numItems);
        }

        Awaitable<void> FullCopyToSecondariesAsync(__inout KSharedArray<Data::TStore::Store<KBuffer::SPtr, KBuffer::SPtr>::SPtr> & secondaries)
        {
            // Checkpoint the primary, to get all current state
            co_await CheckpointAsync(*Store);

            // Get the current state from the primary for each secondary
            KSharedArray<OperationDataStream::SPtr>::SPtr copyStates = _new(TEST_TAG, GetAllocator()) KSharedArray<OperationDataStream::SPtr>();
            for (ULONG32 i = 0; i < secondaries.Count(); i++)
            {
                copyStates->Append(Store->GetCurrentState());
            }

            // Start copying the state to the secondaries
            for (ULONG32 i = 0; i < secondaries.Count(); i++)
            {
                auto secondary = secondaries[i];
                co_await secondary->BeginSettingCurrentStateAsync();
            }

            // Copy all state to the secondary
            LONG64 stateRecordNumber = 0;
            while (secondaries.Count() > 0)
            {
                // Go backwards so we can remove secondary when it's compelte without interrupting the loop
                for (LONG32 i = secondaries.Count() - 1; i >= 0; i--)
                {
                    auto secondaryStore = secondaries[i];
                    auto copyState = (*copyStates)[i];
                    OperationData::CSPtr operationData;
                    co_await copyState->GetNextAsync(CancellationToken::None, operationData);
                    if (operationData == nullptr)
                    {
                        secondaries.Remove(i);
                        continue;
                    }

                    co_await secondaryStore->SetCurrentStateAsync(stateRecordNumber, *operationData, CancellationToken::None);
                }

                stateRecordNumber++;
            }
        }

        Common::CommonConfig config; // load the config object as its needed for the tracing to work

#pragma region test functions
    public:
        Awaitable<void> Checkpoint_ShouldNotWriteToDisk_Test()
        {
            {
                auto key = CreateBuffer(8, 'k');
                auto value = CreateBuffer(8, 'v');

                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }
            }

            co_await CheckpointAsync();

            auto files = Common::Directory::GetFiles(Store->WorkingDirectoryCSPtr->operator LPCWSTR());
            CODING_ERROR_ASSERT(files.size() == 0);
        }

        Awaitable<void> Copy_SingleKey_ShouldSucced_Test()
        {
            auto key = CreateBuffer(8, 'k');
            auto value = CreateBuffer(8, 'v');

            // Setup - add a single key
            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            // Copy state to a new secondary
            auto secondaryStore = co_await CreateSecondaryAsync();
            co_await FullCopyToSecondaryAsync(*secondaryStore);

            co_await VerifyKeyExistsInStoresAsync(key, nullptr, value, KBufferComparer::Equals);
        }

        Awaitable<void> Copy_NoState_ShouldSucceed_Test()
        {
            co_await FullCopyTestAsync(0);
        }

        Awaitable<void> Copy_ManyItems_ShouldSucceed_Test()
        {
            ULONG32 numItems = 64;
            co_await FullCopyTestAsync(numItems);
        }

        Awaitable<void> Copy_MultipleChunks_ShouldSucceed_Test()
        {
            // Keys + Values + Metadata = 30 + 20 = 50 bytes
            ULONG32 numItems = (600 * 1024) / 50;
            co_await FullCopyTestAsync(numItems);
        }
        
        Awaitable<void> Copy_100AddUpdate_ShouldSucceed_Test()
        {
            ULONG32 numItems = 100;
            
            for (ULONG32 i = 0; i < numItems; i++)
            {
                auto key = CreateBuffer(8, i);
                auto value = CreateBuffer(16, i);

                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }
            }

            for (ULONG32 i = 0; i < numItems; i++)
            {
                auto key = CreateBuffer(8, i);
                auto value = CreateBuffer(16, i);

                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }
            }

            auto secondaryStore = co_await CreateSecondaryAsync();
            co_await FullCopyToSecondaryAsync(*secondaryStore);

            // Verify all keys copied
            co_await VerifyStateAsync(numItems);

        }

        Awaitable<void> Copy_100Add50Remove_ShouldSucceed_Test()
        {
            ULONG32 numItems = 100;
            for (ULONG32 k = 0; k < numItems; k++)
            {
                auto key = CreateBuffer(8, k);
                auto value = CreateBuffer(16, k);

                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }

                // Delete all even keys
                if (k % 2 == 0)
                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }
            }

            auto secondaryStore = co_await CreateSecondaryAsync();
            co_await FullCopyToSecondaryAsync(*secondaryStore);

            // Verify all keys copied
            for (ULONG32 k = 0; k < numItems; k++)
            {
                auto key = CreateBuffer(8, k);
                auto expectedValue = CreateBuffer(16, k);

                if (k % 2 == 0)
                {
                    co_await VerifyKeyDoesNotExistInStoresAsync(key);
                }
                else
                {
                    co_await VerifyKeyExistsInStoresAsync(key, nullptr, expectedValue, KBufferComparer::Equals);
                }
            }
        }

        Awaitable<void> Copy_MultipleCheckpoints_ShouldSucceed_Test()
        {
            ULONG32 numItems = 100;
            ULONG32 checkpointFrequency = 10;

            // Setup - 100 items over ~10 checkpoints
            co_await PopulateStoreAsync(numItems, checkpointFrequency);

            auto secondaryStore = co_await CreateSecondaryAsync();
            co_await FullCopyToSecondaryAsync(*secondaryStore);

            co_await VerifyStateAsync(numItems);
        }

        Awaitable<void> Copy_MultipleSecondaries_Sequential_ShouldSucceed_Test()
        {
            ULONG32 numItems = 100;
            ULONG32 numSecondaries = 4;
            ULONG32 checkpointFrequency = 10;

            // Setup - 100 items over ~10 checkpoints
            co_await PopulateStoreAsync(numItems, checkpointFrequency);

            for (ULONG32 i = 0; i < numSecondaries; i++)
            {
                auto secondaryStore = co_await CreateSecondaryAsync();
                co_await FullCopyToSecondaryAsync(*secondaryStore);
            }

            // Verify all keys copied
            co_await VerifyStateAsync(numItems);
        }

        Awaitable<void> Copy_MultipleSecondaries_Concurrent_ShouldSucceed_Test()
        {
            ULONG32 numItems = 100;
            ULONG32 numSecondaries = 4;
            ULONG32 checkpointFrequency = 10;

            // Setup - 100 items over ~10 checkpoints
            co_await PopulateStoreAsync(numItems, checkpointFrequency);

            KSharedArray<Data::TStore::Store<KBuffer::SPtr, KBuffer::SPtr>::SPtr>::SPtr secondaries = _new(TEST_TAG, GetAllocator()) KSharedArray<Data::TStore::Store<KBuffer::SPtr, KBuffer::SPtr>::SPtr>();
            KSharedArray<Data::TStore::Store<KBuffer::SPtr, KBuffer::SPtr>::SPtr>::SPtr secondariesCopy = _new(TEST_TAG, GetAllocator()) KSharedArray<Data::TStore::Store<KBuffer::SPtr, KBuffer::SPtr>::SPtr>();

            for (ULONG32 i = 0; i < numSecondaries; i++)
            {
                auto secondaryStore = co_await CreateSecondaryAsync();
                secondaries->Append(secondaryStore);
                secondariesCopy->Append(secondaryStore);
            }

            // Start coying files to each
            co_await FullCopyToSecondariesAsync(*secondariesCopy);

            for (ULONG32 i = 0; i < numSecondaries; i++)
            {
                auto secondaryStore = (*secondaries)[i];
                co_await secondaryStore->EndSettingCurrentStateAsync(CancellationToken::None);
            }

            // Perform more modifications, which will be replicated and applied on the secondaries as well
            for (ULONG32 k = 90; k < numItems; k++)
            {
                auto key = CreateBuffer(8, k);
                {
                    auto txn = CreateWriteTransaction();
                    bool removed = co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None);
                    CODING_ERROR_ASSERT(removed == true);
                    co_await txn->CommitAsync();
                }
            }

            // Verify all keys copied
            {
                for (ULONG32 m = 0; m < Stores->Count(); m++)
                {
                    for (ULONG32 k = 0; k < 90; k++)
                    {
                        auto key = CreateBuffer(8, k);
                        auto value = CreateBuffer(16, k);
                        co_await VerifyKeyExistsInStoresAsync(key, nullptr, value, KBufferComparer::Equals);
                    }

                    for (ULONG32 k = 90; k < numItems; k++)
                    {
                        auto key = CreateBuffer(8, k);
                        co_await VerifyKeyDoesNotExistInStoresAsync(key);
                    }
                }
            }
        }

        // TODO: Fix this test to actually checkpoint primary during copy. 
        // Currently the MockReplicator will replicate operations regardless of whether secondary is finished copying.
        Awaitable<void> Copy_CheckpointPrimaryDuringCopy_ShouldSucceed_Test()
        {
            ULONG32 numItems = 100;
            ULONG32 checkpointFrequency = 10;

            // Setup - Add 100 items over ~10 checkpoints
            co_await PopulateStoreAsync(numItems, checkpointFrequency);

            // Checkpoint to ensure we have all state ready
            co_await CheckpointAsync(*Store);

            // Start the copy operation to a new secondary
            auto secondaryStore = co_await CreateSecondaryAsync();
            co_await CopyCheckpointToSecondaryAsync(*secondaryStore);

            // Perform more modifications, which will be replicated and applied on the secondary as well
            ULONG32 numAdditional = 10;
            checkpointFrequency = 0;
            co_await PopulateStoreAsync(numAdditional, checkpointFrequency, numItems);

            co_await CheckpointAsync(*Store);
            
            // Verify state on all replicas
            co_await VerifyStateAsync(numItems + numAdditional);
        }

        Awaitable<void> Copy_Prepare_EnsureStrictCheckpointingAsync(
            __in ULONG32 startingCheckpointCount,
            __in bool hasOperationsBeforePrepare,
            __in bool hasOperationsAfterPrepare,
            __in bool hasOperationsAsActiveSecondary)
        {
            const ULONG32 numItemsPerCheckpoint = 8;

            LONG64 checkpointLSN = 0;

            // ------------------------------------------
            // Create the specified number of checkpoints
            // ------------------------------------------
            for (ULONG32 i = 0; i < startingCheckpointCount; i++)
            {
                co_await PopulateStoreAsync(numItemsPerCheckpoint, 0, i * numItemsPerCheckpoint);
                checkpointLSN = co_await CheckpointAsync(*Store);
            }

            // -----------------------------------------------------------
            // Create a secondary, and copy checkpoints from primary to it
            // -----------------------------------------------------------
            auto secondaryStore = co_await CreateSecondaryAsync();
            co_await CopyCheckpointToSecondaryAsync(*secondaryStore);

            // ------------------------------------------
            // Optionally apply an operation to secondary
            // ------------------------------------------
            ULONG32 k = startingCheckpointCount * numItemsPerCheckpoint;
            auto currentLSN = checkpointLSN;
            if (hasOperationsBeforePrepare)
            {
                auto key = CreateBuffer(8, k);
                auto value = CreateBuffer(16, k);

                auto txn = CreateWriteTransaction();
                co_await SecondaryAddAsync(*secondaryStore, *txn->TransactionSPtr, ++currentLSN, key, value);
                k++;
                co_await txn->AbortAsync();
            }

            // -----------------------------------
            // Prepare checkpoint #1 for secondary
            // -----------------------------------
            checkpointLSN = (*SecondaryReplicators)[0]->CommitSequenceNumber + 1; // Safe here since no concurrent access
            (*SecondaryReplicators)[0]->UpdateLSN(checkpointLSN);
            secondaryStore->PrepareCheckpoint(checkpointLSN);
            
            // Optionally apply an operation to secondary
            if (hasOperationsAfterPrepare)
            {
                {
                    auto key = CreateBuffer(8, k);
                    auto value = CreateBuffer(16, k);

                    auto txn = CreateWriteTransaction();
                    co_await SecondaryAddAsync(*secondaryStore, *txn->TransactionSPtr, ++currentLSN, key, value);
                    k++;
                    co_await txn->AbortAsync();
                }
            }

            // --------------------------------------------
            // Perform and complete secondary checkpoint #1
            // --------------------------------------------
            Replicator->CommitSequenceNumber = ++currentLSN;
            co_await secondaryStore->PerformCheckpointAsync(CancellationToken::None);
            co_await secondaryStore->CompleteCheckpointAsync(CancellationToken::None);

            // ---------------------------------------------
            // Optionally add keys while secondary is active
            // ---------------------------------------------
            if (hasOperationsAsActiveSecondary)
            {
                co_await PopulateStoreAsync(numItemsPerCheckpoint, 0, (startingCheckpointCount + 1) * numItemsPerCheckpoint);
            }

            // ------------------------------------------------
            // Prepare/Perform/Complete secondary checkpoint #2
            // ------------------------------------------------
            checkpointLSN = (*SecondaryReplicators)[0]->CommitSequenceNumber + 1; // Safe here since no conurrent access
            (*SecondaryReplicators)[0]->UpdateLSN(checkpointLSN);
            secondaryStore->PrepareCheckpoint(checkpointLSN);
            co_await secondaryStore->PerformCheckpointAsync(CancellationToken::None);
            co_await secondaryStore->CompleteCheckpointAsync(CancellationToken::None);
        }
        
        Awaitable<void> Checkpoint_ShouldConsolidate_Test()
        {
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 3;
            Store->EnableBackgroundConsolidation = false;

            for (ULONG32 i = 0; i < 3; i++)
            {
                CODING_ERROR_ASSERT(Store->ConsolidationManagerSPtr->AggregatedStoreComponentSPtr->GetConsolidatedState()->Count() == 0);
                for (ULONG32 k = 0; k < 5; k++)
                {
                    auto key = CreateBuffer(4, i*5 + k);

                    {
                        auto txn = CreateWriteTransaction();
                        co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, key, DefaultTimeout, CancellationToken::None);
                        co_await txn->CommitAsync();
                    }
                }

                co_await CheckpointAsync();
            }

            CODING_ERROR_ASSERT(Store->ConsolidationManagerSPtr->AggregatedStoreComponentSPtr->GetConsolidatedState()->Count() > 0);
        }
    };

    BOOST_FIXTURE_TEST_SUITE(VolatileStoreTestSuite, VolatileStoreTest)

    BOOST_AUTO_TEST_CASE(KeyVersionedItemComparer_Test)
    {
        auto aKey = CreateBuffer(8, 'a');
        auto bKey = CreateBuffer(8, 'b');
        
        auto value1 = CreateBuffer(16, 'v');
        auto value2 = CreateBuffer(16, 'v');

        auto versionedItem1 = CreateInsertedVersionedItem(value1, 16, 1);
        auto versionedItem2 = CreateInsertedVersionedItem(value1, 16, 2);
        auto versionedItem3 = CreateInsertedVersionedItem(value2, 16, 2);
        
        typedef KeyValuePair<KBuffer::SPtr, VersionedItem<KBuffer::SPtr>::SPtr> BufferBufferPair;

        BufferBufferPair pair1(aKey, versionedItem1);
        BufferBufferPair pair2(aKey, versionedItem2);
        BufferBufferPair pair3(bKey, versionedItem3);

        KeyVersionedItemComparer<KBuffer::SPtr, KBuffer::SPtr>::SPtr comparerSPtr = nullptr;
        KeyVersionedItemComparer<KBuffer::SPtr, KBuffer::SPtr>::Create(*Store->KeyComparerSPtr, GetAllocator(), comparerSPtr);
    
        CODING_ERROR_ASSERT(comparerSPtr->Compare(pair1, pair1) == 0);

        CODING_ERROR_ASSERT(comparerSPtr->Compare(pair2, pair1) < 0);
        CODING_ERROR_ASSERT(comparerSPtr->Compare(pair1, pair2) > 0);

        CODING_ERROR_ASSERT(comparerSPtr->Compare(pair2, pair3) < 0);
        CODING_ERROR_ASSERT(comparerSPtr->Compare(pair3, pair2) > 0);
    }
    
    BOOST_AUTO_TEST_CASE(Checkpoint_ShouldNotWriteToDisk)
    {
        SyncAwait(Checkpoint_ShouldNotWriteToDisk_Test());
    }

    BOOST_AUTO_TEST_CASE(Copy_SingleKey_ShouldSucceed)
    {
        SyncAwait(Copy_SingleKey_ShouldSucced_Test());
    }

    BOOST_AUTO_TEST_CASE(Copy_NoState_ShouldSucceed)
    {
        SyncAwait(Copy_NoState_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Copy_ManyItems_ShouldSucceed)
    {
        SyncAwait(Copy_ManyItems_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Copy_MultipleChunks_ShouldSucceed)
    {
        SyncAwait(Copy_MultipleChunks_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Copy_100AddUpdate_ShouldSucceed)
    {
        SyncAwait(Copy_100AddUpdate_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Copy_100Add50Remove_ShouldSucceed)
    {
        SyncAwait(Copy_100Add50Remove_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Copy_MultipleCheckpoints_ShouldSucceed)
    {
        SyncAwait(Copy_MultipleCheckpoints_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Copy_MultipleSecondaries_Sequential_ShouldSucceed)
    {
        SyncAwait(Copy_MultipleSecondaries_Sequential_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Copy_MultipleSecondaries_Concurrent_ShouldSucceed)
    {
        SyncAwait(Copy_MultipleSecondaries_Concurrent_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Copy_CheckpointPrimaryDuringCopy_ShouldSucceed)
    {
        SyncAwait(Copy_CheckpointPrimaryDuringCopy_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_SingleCheckpoint_NoOpsBeforePrepare_NoOpsAfterPrepare_NoOpsAsActiveSecondary_ShouldSucceed)
    {
        SyncAwait(Copy_Prepare_EnsureStrictCheckpointingAsync(1, false, false, false));
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_SingleCheckpoint_NoOpsBeforePrepare_NoOpsAfterPrepare_OpsAsActiveSecondary_ShouldSucceed)
    {
        SyncAwait(Copy_Prepare_EnsureStrictCheckpointingAsync(1, false, false, true));
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_SingleCheckpoint_NoOpsBeforePrepare_OpsAfterPrepare_NoOpsAsActiveSecondary_ShouldSucceed)
    {
        SyncAwait(Copy_Prepare_EnsureStrictCheckpointingAsync(1, false, true, false));
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_SingleCheckpoint_NoOpsBeforePrepare_OpsAfterPrepare_OpsAsActiveSecondary_ShouldSucceed)
    {
        SyncAwait(Copy_Prepare_EnsureStrictCheckpointingAsync(1, false, true, true));
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_SingleCheckpoint_OpsBeforePrepare_NoOpsAfterPrepare_NoOpsAsActiveSecondary_ShouldSucceed)
    {
        SyncAwait(Copy_Prepare_EnsureStrictCheckpointingAsync(1, true, false, false));
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_SingleCheckpoint_OpsBeforePrepare_NoOpsAfterPrepare_OpsAsActiveSecondary_ShouldSucceed)
    {
        SyncAwait(Copy_Prepare_EnsureStrictCheckpointingAsync(1, true, false, true));
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_SingleCheckpoint_OpsBeforePrepare_OpsAfterPrepare_NoOpsAsActiveSecondary_ShouldSucceed)
    {
        SyncAwait(Copy_Prepare_EnsureStrictCheckpointingAsync(1, true, true, false));
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_SingleCheckpoint_OpsBeforePrepare_OpsAfterPrepare_OpsAsActiveSecondary_ShouldSucceed)
    {
        SyncAwait(Copy_Prepare_EnsureStrictCheckpointingAsync(1, true, true, true));
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_MultipleCheckpoints_NoOpsBeforePrepare_NoOpsAfterPrepare_NoOpsAsActiveSecondary_ShouldSucceed)
    {
        SyncAwait(Copy_Prepare_EnsureStrictCheckpointingAsync(1, false, false, false));
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_MultipleCheckpoints_NoOpsBeforePrepare_NoOpsAfterPrepare_OpsAsActiveSecondary_ShouldSucceed)
    {
        SyncAwait(Copy_Prepare_EnsureStrictCheckpointingAsync(8, false, false, true));
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_MultipleCheckpoints_NoOpsBeforePrepare_OpsAfterPrepare_NoOpsAsActiveSecondary_ShouldSucceed)
    {
        SyncAwait(Copy_Prepare_EnsureStrictCheckpointingAsync(8, false, true, false));
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_MultipleCheckpoints_NoOpsBeforePrepare_OpsAfterPrepare_OpsAsActiveSecondary_ShouldSucceed)
    {
        SyncAwait(Copy_Prepare_EnsureStrictCheckpointingAsync(8, false, true, true));
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_MultipleCheckpoints_OpsBeforePrepare_NoOpsAfterPrepare_NoOpsAsActiveSecondary_ShouldSucceed)
    {
        SyncAwait(Copy_Prepare_EnsureStrictCheckpointingAsync(8, true, false, false));
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_MultipleCheckpoints_OpsBeforePrepare_NoOpsAfterPrepare_OpsAsActiveSecondary_ShouldSucceed)
    {
        SyncAwait(Copy_Prepare_EnsureStrictCheckpointingAsync(8, true, false, true));
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_MultipleCheckpoints_OpsBeforePrepare_OpsAfterPrepare_NoOpsAsActiveSecondary_ShouldSucceed)
    {
        SyncAwait(Copy_Prepare_EnsureStrictCheckpointingAsync(8, true, true, false));
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_MultipleCheckpoints_OpsBeforePrepare_OpsAfterPrepare_OpsAsActiveSecondary_ShouldSucceed)
    {
        SyncAwait(Copy_Prepare_EnsureStrictCheckpointingAsync(8, true, true, true));
    }

    BOOST_AUTO_TEST_CASE(Sweep_ShouldNotStart)
    {
        Store->EnableSweep = true;
        Store->StartBackgroundSweep();

        // Completion source will not be null if sweep actually started
        CODING_ERROR_ASSERT(Store->SweepManagerSPtr->SweepCompletionSource == nullptr);
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_ShouldConsolidate)
    {
        SyncAwait(Checkpoint_ShouldConsolidate_Test());
    }

    BOOST_AUTO_TEST_SUITE_END()
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'scTP'

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
    class StoreCopyTest : public TStoreTestBase<LONG64, KString::SPtr, LongComparer, TestStateSerializer<LONG64>, StringStateSerializer>
    {
    public:
        void Test_Resurrection(
            __in int numberOfCheckpointsBeforeResurrection,
            __in int numberOfItemsBeforeResurrectionPerCheckpoint,
            __in int numberOfIdleAddsBeforeCheckpoint,
            __in int numberOfIdleAddsPerCheckpoint,
            __in int numberOfActiveAddsPerCheckpoint,
            __in int numberOfPrimaryAddsPerCheckpoint);

    public:
        StoreCopyTest()
        {
            Setup(1);
            SyncAwait(Store->RecoverCheckpointAsync(CancellationToken::None));
        }

        ~StoreCopyTest()
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

        KString::SPtr GetStringValue(__in LONG64 seed)
        {
            KString::SPtr key;
            wstring str = wstring(L"test_key") + to_wstring(seed);
            auto status = KString::Create(key, GetAllocator(), str.c_str());
            Diagnostics::Validate(status);
            return key;
        }

        StoreTraceComponent::SPtr CreateTraceComponent()
        {
            KGuid guid;
            guid.CreateNew();
            ::FABRIC_REPLICA_ID replicaId = 1;
            int stateProviderId = 1;
            
            StoreTraceComponent::SPtr traceComponent = nullptr;
            NTSTATUS status = StoreTraceComponent::Create(guid, replicaId, stateProviderId, GetAllocator(), traceComponent);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            return traceComponent;
        }

        void VerifyKeyExists(__in LONG64 key, __in KString::SPtr expectedValue)
        {
            SyncAwait(VerifyKeyExistsAsync(*Store, key, nullptr, expectedValue, StringEquals));
        }

        void VerifyKeyExists(__in StoreTransaction<LONG64, KString::SPtr> & transaction, __in LONG64 key, __in KString::SPtr expectedValue)
        {
            SyncAwait(VerifyKeyExistsAsync(*Store, transaction, key, nullptr, expectedValue, StringEquals));
        }

        void PopulateStore(__in ULONG32 count, __in LONG64 checkpointFrequency=0, __in LONG64 startingKey=0)
        {
            for (LONG64 key = startingKey; key < startingKey + count; key++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, GetStringValue(key), DefaultTimeout, CancellationToken::None));
                    SyncAwait(txn->CommitAsync());
                }

                if (checkpointFrequency != 0 && key % checkpointFrequency == 0)
                {
                    Checkpoint();
                }
            }
        }

        OperationData::SPtr GetBytes(__in LONG64& value)
        {
            Utilities::BinaryWriter binaryWriter(GetAllocator());
            Store->KeyConverterSPtr->Write(value, binaryWriter);

            OperationData::SPtr operationDataSPtr = OperationData::Create(this->GetAllocator());
            operationDataSPtr->Append(*binaryWriter.GetBuffer(0));

            return operationDataSPtr;
        }

        OperationData::SPtr GetBytes(__in KString::SPtr& value)
        {
            Utilities::BinaryWriter binaryWriter(GetAllocator());
            Store->ValueConverterSPtr->Write(value, binaryWriter);

            OperationData::SPtr operationDataSPtr = OperationData::Create(this->GetAllocator());
            operationDataSPtr->Append(*binaryWriter.GetBuffer(0));

            return operationDataSPtr;
        }

        void SecondaryApply(
            __in Data::TStore::Store<LONG64, KString::SPtr> & secondaryStore,
            __in Transaction & transaction,
            __in LONG64 commitSequenceNumber,
            __in StoreModificationType::Enum operationType,
            __in LONG64 key,
            __in OperationData & keyBytes,
            __in KString::SPtr value,
            __in OperationData::SPtr & valueBytes)
        {
            KAllocator& allocator = GetAllocator();

            transaction.CommitSequenceNumber = commitSequenceNumber;

            OperationData::CSPtr metadataCSPtr = nullptr;

            if (valueBytes != nullptr)
            {
                KSharedPtr<MetadataOperationDataKV<LONG64, KString::SPtr> const> metadataKVCSPtr = nullptr;
                NTSTATUS status = MetadataOperationDataKV<LONG64, KString::SPtr>::Create(
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
                KSharedPtr<MetadataOperationDataK<LONG64> const> metadataKCSPtr = nullptr;
                NTSTATUS status = MetadataOperationDataK<LONG64>::Create(
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

            auto operationContext = SyncAwait(secondaryStore.ApplyAsync(
                commitSequenceNumber,
                transaction,
                ApplyContext::SecondaryRedo,
                metadataCSPtr.RawPtr(),
                redoDataSPtr.RawPtr()));

            if (operationContext != nullptr)
            {
                secondaryStore.Unlock(*operationContext);
            }
        }

        void SecondaryAdd(
            __in Data::TStore::Store<LONG64, KString::SPtr> & secondaryStore,
            __in Transaction & transaction,
            __in LONG64 commitSequenceNumber,
            __in LONG64 key, 
            __in KString::SPtr value)
        {
            auto keyBytesSPtr = GetBytes(key);
            auto valueBytesSPtr = GetBytes(value);
            SecondaryApply(secondaryStore, transaction, commitSequenceNumber, StoreModificationType::Enum::Add, key, *keyBytesSPtr, value, valueBytesSPtr);
        }

        void FullCopyToSecondary(__in Data::TStore::Store<LONG64, KString::SPtr> & secondaryStore)
        {
            // Checkpoint the Primary, to get all current state
            Checkpoint(*Store);
            CopyCheckpointToSecondary(secondaryStore);
        }
        
        void FullCopyToSecondaries(__inout KSharedArray<Data::TStore::Store<LONG64, KString::SPtr>::SPtr> & secondaries)
        {
            // Checkpoint the Primary, to get all current state
            Checkpoint(*Store);
            
            // Get the current state from the Primary for each secondary
            KSharedArray<OperationDataStream::SPtr>::SPtr copyStates = _new(TEST_TAG, GetAllocator()) KSharedArray<OperationDataStream::SPtr>();
            for (ULONG32 i = 0; i < secondaries.Count(); i++)
            {
                copyStates->Append(Store->GetCurrentState());
            }

            // Start copying the state to the Secondary
            for (ULONG32 i = 0; i < secondaries.Count(); i++)
            {
                auto secondary = secondaries[i];
                secondary->BeginSettingCurrentState();
            }

            // Copy all state to the Secondary
            LONG64 stateRecordNumber = 0;
            while (secondaries.Count() > 0)
            {
                // Go backwards so we can remove a Secondary when it's complete without interrupting the loop
                for (LONG32 i = secondaries.Count() - 1; i >= 0; i--)
                {
                    auto secondaryStore = secondaries[i];
                    auto copyState = (*copyStates)[i];
                    OperationData::CSPtr operationData;
                    SyncAwait(copyState->GetNextAsync(CancellationToken::None, operationData));
                    if (operationData == nullptr)
                    {
                        // Done with Copy for this state provider
                        secondaries.Remove(i);
                        continue;
                    }

                    SyncAwait(secondaryStore->SetCurrentStateAsync(stateRecordNumber, *operationData, CancellationToken::None));
                }

                stateRecordNumber++;
            }
        }

        void FullCopyTestWithFileSize(__in ULONG32 checkpointFileSize)
        {
            // Each key takes 56 bytes in the key checkpoint file
            const ULONG32 keyCheckpointFileItemSize = 56;
            ULONG32 numItems = checkpointFileSize / keyCheckpointFileItemSize;
            FullCopyTest(numItems);
        }

        void FullCopyTest(__in ULONG32 numItems)
        {
            PopulateStore(numItems);

            // Copy state to a new secondary
            auto secondaryStore = CreateSecondary();
            FullCopyToSecondary(*secondaryStore);

            // Verify all keys copied
            VerifyState(*Stores, numItems);
        }

        KSharedArray<ULONG32>::SPtr SnapCheckpointIds(__in MetadataTable & table)
        {
            KSharedArray<ULONG32>::SPtr checkpointFileIdsToCopy = _new(ALLOC_TAG, GetAllocator()) KSharedArray<ULONG32>();
            auto enumeratorSPtr = table.Table->GetEnumerator();

            while (enumeratorSPtr->MoveNext())
            {
                auto item = enumeratorSPtr->Current();
                checkpointFileIdsToCopy->Append(item.Value->FileId);
            }
            return checkpointFileIdsToCopy;
        }

        void VerifyCheckpointIds(__in KSharedArray<ULONG32> & src, __in KSharedArray<ULONG32> & other, __in bool secondaryIsSuperSet)
        {
            if (secondaryIsSuperSet)
            {
                CODING_ERROR_ASSERT(other.Count() > src.Count());
            }
            else
            {
                CODING_ERROR_ASSERT(other.Count() == src.Count());
            }

            for (ULONG32 i = 0; i < src.Count(); i++)
            {
                bool found = false;
                for (ULONG32 j = 0; j < other.Count(); j++)
                {
                    if (src[i] == other[j])
                    {
                        found = true;
                        break;
                    }
                }
                CODING_ERROR_ASSERT(found);
            }
        }

        void VerifyCurrentMetadataTable(
            __in Data::TStore::Store<LONG64, KString::SPtr> & primary, 
            __in Data::TStore::Store<LONG64, KString::SPtr> & secondary, 
            bool secondaryIsSuperSet)
        {
            auto primaryCheckpointIds = SnapCheckpointIds(*primary.CurrentMetadataTableSPtr);
            auto secondaryCheckpointIds = SnapCheckpointIds(*secondary.CurrentMetadataTableSPtr);

            VerifyCheckpointIds(*primaryCheckpointIds, *secondaryCheckpointIds, secondaryIsSuperSet);
        }

        void VerifyCurrentDiskMatchesMemory(__in Data::TStore::Store<LONG64, KString::SPtr> & store)
        {
            if (store.NextMetadataTableSPtr != nullptr)
            {
                MetadataTable::SPtr diskTableSPtr = nullptr;
                NTSTATUS status = MetadataTable::Create(GetAllocator(), diskTableSPtr);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
                SyncAwait(MetadataManager::OpenAsync(*diskTableSPtr, *store.TmpDiskMetadataFilePath, GetAllocator(), *CreateTraceComponent()));

                auto diskMetadata = SnapCheckpointIds(*diskTableSPtr);
                auto memoryMetadata = SnapCheckpointIds(*store.NextMetadataTableSPtr);

                VerifyCheckpointIds(*diskMetadata, *memoryMetadata, false);
            }

            MetadataTable::SPtr currentDiskTableSPtr = nullptr;
            NTSTATUS status = MetadataTable::Create(GetAllocator(), currentDiskTableSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            SyncAwait(MetadataManager::OpenAsync(*currentDiskTableSPtr, *store.CurrentDiskMetadataFilePath, GetAllocator(), *CreateTraceComponent()));

            auto currentDiskMetadata = SnapCheckpointIds(*currentDiskTableSPtr);
            auto currentMemoryMetadata = SnapCheckpointIds(*store.CurrentMetadataTableSPtr);

            VerifyCheckpointIds(*currentDiskMetadata, *currentMemoryMetadata, false);
        }
        
        void VerifyState(__in KSharedArray<Data::TStore::Store<LONG64, KString::SPtr>::SPtr> & secondaries, ULONG32 keyCount)
        {
            for (ULONG32 m = 0; m < secondaries.Count(); m++)
            {
                VerifyState(*secondaries[m], keyCount);
            }
        }

        void VerifyState(__in Data::TStore::Store<LONG64, KString::SPtr> & secondary, ULONG32 keyCount)
        {
            auto txn = CreateWriteTransaction(secondary);
            for (LONG64 key = 0; key < keyCount; key++)
            {
                SyncAwait(VerifyKeyExistsAsync(secondary, *txn->StoreTransactionSPtr, key, nullptr, GetStringValue(key), StringEquals));
            }
            SyncAwait(txn->AbortAsync());
        }

        void Copy_Prepare_EnsureStrictCheckpointing(
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
                PopulateStore(numItemsPerCheckpoint, 0, i * numItemsPerCheckpoint);
                checkpointLSN = Checkpoint(*Store);
            }

            // TODO: Check store count is correct
            CODING_ERROR_ASSERT(startingCheckpointCount == Store->CurrentMetadataTableSPtr->Table->Count);
            
            // -----------------------------------------------------------
            // Create a secondary, and copy checkpoints from primary to it 
            // -----------------------------------------------------------
            auto secondaryStore = CreateSecondary();
            CopyCheckpointToSecondary(*secondaryStore);
            
            // ------------------------------------------
            // Optionally apply an operation to secondary
            // ------------------------------------------
            auto key = startingCheckpointCount * numItemsPerCheckpoint;
            auto currentLSN = checkpointLSN;
            if (hasOperationsBeforePrepare)
            {
                {
                    auto txn = CreateWriteTransaction();
                    SecondaryAdd(*secondaryStore, *txn->TransactionSPtr, ++currentLSN, key, GetStringValue(key));
                    key++;
                    CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->CheckpointLSN == secondaryStore->CurrentMetadataTableSPtr->CheckpointLSN);
                    SyncAwait(txn->AbortAsync());
                }
            }
            
            // -----------------------------------
            // Prepare checkpoint #1 for secondary 
            // -----------------------------------
            auto checkpointLsn = (*SecondaryReplicators)[0]->CommitSequenceNumber + 1; // Safe here since no concurrent access
            (*SecondaryReplicators)[0]->UpdateLSN(checkpointLsn);
            secondaryStore->PrepareCheckpoint(checkpointLsn);
            CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->CheckpointLSN == secondaryStore->CurrentMetadataTableSPtr->CheckpointLSN);
        
            auto expectedItemCount = startingCheckpointCount * numItemsPerCheckpoint;
            expectedItemCount += hasOperationsBeforePrepare ? 1 : 0;
            // TODO: Check item count is correct
            
            VerifyCurrentMetadataTable(*Store, *secondaryStore, false);
            
            // ------------------------------------------
            // Optionally apply an operation to secondary
            // -------------------------------------------
            if (hasOperationsAfterPrepare)
            {
                {
                    auto txn = CreateWriteTransaction();
                    SecondaryAdd(*secondaryStore, *txn->TransactionSPtr, ++currentLSN, key, GetStringValue(key));
                    key++;
                    CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->CheckpointLSN == secondaryStore->CurrentMetadataTableSPtr->CheckpointLSN);
                    SyncAwait(txn->AbortAsync());
                }
            }

            expectedItemCount += hasOperationsAfterPrepare ? 1 : 0;
            // TODO: Check item count is correct
            ASSERT_IFNOT(startingCheckpointCount == secondaryStore->CurrentMetadataTableSPtr->Table->Count, "Unexpected number of checkpoint files copied");
            
            // --------------------------------------------
            // Perform and complete secondary checkpoint #1
            // --------------------------------------------
            Replicator->CommitSequenceNumber = ++currentLSN;
            SyncAwait(secondaryStore->PerformCheckpointAsync(CancellationToken::None));
            // TODO: Check secondary's consolidation manager count is 0
            SyncAwait(secondaryStore->CompleteCheckpointAsync(CancellationToken::None));
            
            auto expectedCheckpointCount = hasOperationsBeforePrepare ? startingCheckpointCount + 1 : startingCheckpointCount;
            auto actualCount = secondaryStore->CurrentMetadataTableSPtr->Table->Count;
            ASSERT_IFNOT(expectedCheckpointCount == actualCount, "Expected {0} checkpoint files, but found {1} instead", expectedCheckpointCount, actualCount);
            bool isSuperSet = hasOperationsBeforePrepare;
            VerifyCurrentMetadataTable(*Store, *secondaryStore, isSuperSet);
            VerifyCurrentDiskMatchesMemory(*secondaryStore);
            
            // ---------------------------------------------
            // Optionally add keys while secondary is active
            // ---------------------------------------------
            if (hasOperationsAsActiveSecondary)
            {
                PopulateStore(numItemsPerCheckpoint, 0, (startingCheckpointCount + 1) * (numItemsPerCheckpoint));
            }

            // TODO: check item count
            
            // ------------------------------------------------
            // Prepare/Perform/Complete secondary checkpoint #2
            // ------------------------------------------------
            checkpointLsn = (*SecondaryReplicators)[0]->CommitSequenceNumber + 1; // Safe here since no concurrent access
            (*SecondaryReplicators)[0]->UpdateLSN(checkpointLsn);
            secondaryStore->PrepareCheckpoint(checkpointLsn);
            SyncAwait(secondaryStore->PerformCheckpointAsync(CancellationToken::None));
            expectedCheckpointCount += hasOperationsAfterPrepare || hasOperationsAsActiveSecondary ? 1 : 0;
            CODING_ERROR_ASSERT(secondaryStore->NextMetadataTableSPtr->Table->Count == expectedCheckpointCount);
            // TODO: check item count
            VerifyCurrentDiskMatchesMemory(*secondaryStore);

            SyncAwait(secondaryStore->CompleteCheckpointAsync(CancellationToken::None));
            CODING_ERROR_ASSERT(secondaryStore->CurrentMetadataTableSPtr->Table->Count == expectedCheckpointCount);
            // TODO: check item count
        }

        TestDictionaryChangeHandler<LONG64, KString::SPtr>::SPtr CreateTestChangeHandler()
        {
            LongComparer::SPtr longComparerSPtr = nullptr;
            auto status = LongComparer::Create(GetAllocator(), longComparerSPtr);

            KStringComparer::SPtr stringComparerSPtr = nullptr;
            status = KStringComparer::Create(GetAllocator(), stringComparerSPtr);

            IComparer<LONG64>::SPtr keyComparerSPtr = static_cast<IComparer<LONG64> *>(longComparerSPtr.RawPtr());
            IComparer<KString::SPtr>::SPtr valueComparerSPtr = static_cast<IComparer<KString::SPtr> *>(stringComparerSPtr.RawPtr());

            TestDictionaryChangeHandler<LONG64, KString::SPtr>::SPtr handlerSPtr = nullptr;
            status = TestDictionaryChangeHandler<LONG64, KString::SPtr>::Create(*keyComparerSPtr, *valueComparerSPtr, GetAllocator(), handlerSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            return handlerSPtr;
        }

        Common::CommonConfig config; // load the config object as its needed for the tracing to work
    };

    void StoreCopyTest::Test_Resurrection(
        __in int numberOfCheckpointPerScenario,
        __in int numberOfAddsBeforeResurrectionPerCheckpoint,
        __in int numberOfIdleAddsBeforeCheckpoint,
        __in int numberOfIdleAddsPerCheckpoint,
        __in int numberOfActiveAddsPerCheckpoint,
        __in int numberOfPrimaryAddsPerCheckpoint)
    {
        // Do not merge on primary
        Store->MergeHelperSPtr->CurrentMergePolicy = MergePolicy::None;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
        TestDictionaryChangeHandler<LONG64, KString::SPtr>::SPtr testHandlerSPtr = CreateTestChangeHandler();

        // Populate the replica to be resurrected.
        LONG64 highestSeenKey = 0;
        for (int iteration = 0; iteration < numberOfCheckpointPerScenario; iteration++)
        {
            for (LONG64 numberOfAdds = 0; numberOfAdds < numberOfAddsBeforeResurrectionPerCheckpoint; numberOfAdds++)
            {
                LONG64 key = highestSeenKey++;

                WriteTransaction<LONG64, KString::SPtr>::SPtr txn = CreateWriteTransaction();
                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, GetStringValue(key), DefaultTimeout, CancellationToken::None));
                FABRIC_SEQUENCE_NUMBER commitLSN = SyncAwait(txn->CommitAsync());

                testHandlerSPtr->AddToExpectedRebuild(key, GetStringValue(key), commitLSN);
            }

            Checkpoint(*Store);
        }

        // Verify that the store has been populated.
        VerifyState(*Store, numberOfCheckpointPerScenario * numberOfAddsBeforeResurrectionPerCheckpoint);

        // Restart the replica: Close, Open and Recover.
        // Note that below method also promotes the store to Primary. 
        // Preferably this should just open and recover.
        CloseAndReOpenStore(testHandlerSPtr.RawPtr());

        VERIFY_ARE_EQUAL2(testHandlerSPtr->RebuildCallCount(), 1);
        testHandlerSPtr->Validate();

        // Reset the notification.
        testHandlerSPtr = CreateTestChangeHandler();
        Store->DictionaryChangeHandlerSPtr = testHandlerSPtr.RawPtr();

        // Simulate resurrection with Change Role to IS followed by empty copy.
        SyncAwait(Store->ChangeRoleAsync(FABRIC_REPLICA_ROLE_IDLE_SECONDARY, CancellationToken::None));
        Store->BeginSettingCurrentState();
        SyncAwait(Store->EndSettingCurrentStateAsync(CancellationToken::None));

        // Verification. Verfies best effort count as well.
        VerifyState(*Store, 0);
        VERIFY_ARE_EQUAL2(Store->Count, 0);
        VERIFY_ARE_EQUAL2(testHandlerSPtr->RebuildCallCount(), 1);

        // Reset due to VerifyState requirements.
        highestSeenKey = 0;

        // BLOCK 1: Adds before copy checkpoint
        FABRIC_SEQUENCE_NUMBER lastLSN = 1;
        for (LONG64 numberOfAdds = 0; numberOfAdds < numberOfIdleAddsBeforeCheckpoint; numberOfAdds++)
        {
            LONG64 key = highestSeenKey++;
            WriteTransaction<LONG64, KString::SPtr>::SPtr txn = CreateWriteTransaction();
            SecondaryAdd(*Store, *txn->TransactionSPtr, ++lastLSN, key, GetStringValue(key));
            testHandlerSPtr->AddToExpected(StoreModificationType::Add, key, GetStringValue(key), lastLSN);
        }

        // Verify block 1.
        ULONG32 expectedCount = numberOfIdleAddsBeforeCheckpoint;
        VerifyState(*Store, expectedCount);
        testHandlerSPtr->Validate();

        // Copy checkpoint
        lastLSN = Checkpoint(*Store);

        // BLOCK 2: Adds after copy checkpoint as IDLE
        for (int iteration = 0; iteration < numberOfCheckpointPerScenario; iteration++)
        {
            for (LONG64 numberOfAdds = 0; numberOfAdds < numberOfIdleAddsPerCheckpoint; numberOfAdds++)
            {
                LONG64 key = highestSeenKey++;
                WriteTransaction<LONG64, KString::SPtr>::SPtr txn = CreateWriteTransaction();
                SecondaryAdd(*Store, *txn->TransactionSPtr, ++lastLSN, key, GetStringValue(key));
                testHandlerSPtr->AddToExpected(StoreModificationType::Add, key, GetStringValue(key), lastLSN);
            }

            lastLSN = Checkpoint(*Store);
        }

        // Verify block 2.
        expectedCount += numberOfCheckpointPerScenario * numberOfIdleAddsPerCheckpoint;
        VerifyState(*Store, expectedCount);
        testHandlerSPtr->Validate();

        // BLOCK 3: Adds after copy checkpoint as ACTIVE
        SyncAwait(Store->ChangeRoleAsync(FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, CancellationToken::None));
        for (int iteration = 0; iteration < numberOfCheckpointPerScenario; iteration++)
        {
            for (LONG64 numberOfAdds = 0; numberOfAdds < numberOfActiveAddsPerCheckpoint; numberOfAdds++)
            {
                LONG64 key = highestSeenKey++;
                WriteTransaction<LONG64, KString::SPtr>::SPtr txn = CreateWriteTransaction();
                SecondaryAdd(*Store, *txn->TransactionSPtr, ++lastLSN, key, GetStringValue(key));
                testHandlerSPtr->AddToExpected(StoreModificationType::Add, key, GetStringValue(key), lastLSN);
            }

            lastLSN = Checkpoint(*Store);
        }

        // Verify block 3.
        expectedCount += numberOfCheckpointPerScenario * numberOfActiveAddsPerCheckpoint;
        VerifyState(*Store, expectedCount);
        VERIFY_ARE_EQUAL2(Store->Count, expectedCount);
        testHandlerSPtr->Validate();

        // BLOCK 4: Adds after copy checkpoint as PRIMARY
        SyncAwait(Store->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None));

        for (int iteration = 0; iteration < numberOfCheckpointPerScenario; iteration++)
        {
            for (LONG64 numberOfAdds = 0; numberOfAdds < numberOfPrimaryAddsPerCheckpoint; numberOfAdds++)
            {
                LONG64 key = highestSeenKey++;

                WriteTransaction<LONG64, KString::SPtr>::SPtr txn = CreateWriteTransaction();
                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, GetStringValue(key), DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }

            lastLSN = Checkpoint(*Store);
        }

        // Verify that the store has been populated.
        expectedCount += numberOfCheckpointPerScenario * numberOfPrimaryAddsPerCheckpoint;
        VerifyState(*Store, expectedCount);
        VERIFY_ARE_EQUAL2(Store->Count, expectedCount);
    }

    BOOST_FIXTURE_TEST_SUITE(StoreCopyTestSuite, StoreCopyTest)

    BOOST_AUTO_TEST_CASE(Copy_SingleKey_ShouldSucceed)
    {
        LONG64 key = 8;
        KString::SPtr value = GetStringValue(L"value");
        
        // Setup - add a single key
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        // Copy state to a new secondary
        auto secondaryStore = CreateSecondary();
        FullCopyToSecondary(*secondaryStore);
        
        SyncAwait(VerifyKeyExistsInStoresAsync(key, nullptr, value, StringEquals));
    }

    BOOST_AUTO_TEST_CASE(Copy_NoState_ShouldSucceed)
    {
        FullCopyTest(0);
    }

    BOOST_AUTO_TEST_CASE(Copy_ManyKeys_ShouldSucceed)
    {
        ULONG32 numItems = 64;
        FullCopyTest(numItems);
    }

    BOOST_AUTO_TEST_CASE(Copy_ExactlyOneChunk_4KBChunks_ShouldSucceed)
    {
        StoreCopyStream::CopyChunkSize = 4192;
        ULONG32 checkpointFileSize = StoreCopyStream::CopyChunkSize;
        FullCopyTestWithFileSize(checkpointFileSize);
    }

    BOOST_AUTO_TEST_CASE(Copy_MoreThanOneChunk_4KBChunks_ShouldSucceed)
    {
        StoreCopyStream::CopyChunkSize = 4192;
        ULONG32 checkpointFileSize = StoreCopyStream::CopyChunkSize + 1024;
        FullCopyTestWithFileSize(checkpointFileSize);
    }

    BOOST_AUTO_TEST_CASE(Copy_TwoChunks_4KBChunks_ShouldSucceed)
    {
        StoreCopyStream::CopyChunkSize = 4192;
        ULONG32 checkpointFileSize = StoreCopyStream::CopyChunkSize * 2;
        FullCopyTestWithFileSize(checkpointFileSize);
    }

    BOOST_AUTO_TEST_CASE(Copy_MoreThanTwoChunks_4KBChunks_ShouldSucceed)
    {
        StoreCopyStream::CopyChunkSize = 4192;
        ULONG32 checkpointFileSize = StoreCopyStream::CopyChunkSize * 2 + 1024;
        FullCopyTestWithFileSize(checkpointFileSize);
    }

    BOOST_AUTO_TEST_CASE(Copy_ManyChunks_4KBChunks_ShouldSucceed)
    {
        StoreCopyStream::CopyChunkSize = 4192;
        ULONG32 checkpointFileSize = StoreCopyStream::CopyChunkSize * 5 + 1024;
        FullCopyTestWithFileSize(checkpointFileSize);
    }

    BOOST_AUTO_TEST_CASE(Copy_100AddUpdate_ShouldSucceed)
    {
        ULONG32 numItems = 100;

        // Setup - Add 100 items
        PopulateStore(numItems);

        auto secondaryStore = CreateSecondary();
        FullCopyToSecondary(*secondaryStore);

        // Verify all keys copied
        VerifyState(*Stores, numItems);
    }

    BOOST_AUTO_TEST_CASE(Copy_100Add50Remove_ShouldSucceed)
    {
        ULONG32 numItems = 100;
        for (LONG64 key = 0; key < numItems; key++)
        {
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, GetStringValue(L"value"), DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }
            
            // Delete all even keys
            if (key % 2 == 0)
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }
        }

        auto secondaryStore = CreateSecondary();
        FullCopyToSecondary(*secondaryStore);

        // Verify all keys copied
        {
            for (ULONG32 m = 0; m < Stores->Count(); m++)
            {
                auto txn = CreateWriteTransaction(*(*Stores)[m]);
                for (LONG64 key = 0; key < numItems; key++)
                {
                    if (key % 2 == 0)
                    {
                        SyncAwait(VerifyKeyDoesNotExistAsync(*(*Stores)[m], *txn->StoreTransactionSPtr, key));
                    }
                    else
                    {
                        SyncAwait(VerifyKeyExistsAsync(*(*Stores)[m], *txn->StoreTransactionSPtr, key, nullptr, GetStringValue(L"value"), StringEquals));
                    }
                }
                SyncAwait(txn->AbortAsync());
            }
        }
    }

    BOOST_AUTO_TEST_CASE(Copy_MultipleCheckpoints_ShouldSucceed)
    {
        ULONG32 numItems = 100;
        ULONG32 checkpointFrequency = 10;

        // Setup - 100 items over ~10 checkpoints
        PopulateStore(numItems, checkpointFrequency);

        auto secondaryStore = CreateSecondary();
        FullCopyToSecondary(*secondaryStore);
        
        VerifyState(*Stores, numItems);
    }

    BOOST_AUTO_TEST_CASE(Copy_MultipleSecondaries_Sequential_ShouldSucceed)
    {
        ULONG32 numItems = 100;
        ULONG32 numSecondaries = 4;
        ULONG32 checkpointFrequency = 10;

        // Setup - 100 items over ~10 checkpoints
        PopulateStore(numItems, checkpointFrequency);

        for (ULONG32 i = 0; i < numSecondaries; i++)
        {
            auto secondaryFolderPath = CreateTempDirectory();
            auto secondaryStore = CreateSecondary();
            FullCopyToSecondary(*secondaryStore);
        }

        // Verify all keys copied
        VerifyState(*Stores, numItems);
    }

    BOOST_AUTO_TEST_CASE(Copy_MultipleSecondaries_Concurrent_ShouldSucceed)
    {
        ULONG32 numItems = 100;
        ULONG32 numSecondaries = 4;
        ULONG32 checkpointFrequency = 10;
        
        // Setup - 100 items over ~10 checkpoints
        PopulateStore(numItems, checkpointFrequency);

        KSharedArray<Data::TStore::Store<LONG64, KString::SPtr>::SPtr>::SPtr secondaries = _new(TEST_TAG, GetAllocator()) KSharedArray<Data::TStore::Store<LONG64, KString::SPtr>::SPtr>();
        KSharedArray<Data::TStore::Store<LONG64, KString::SPtr>::SPtr>::SPtr secondariesCopy = _new(TEST_TAG, GetAllocator()) KSharedArray<Data::TStore::Store<LONG64, KString::SPtr>::SPtr>();

        for (ULONG32 i = 0; i < numSecondaries; i++)
        {
            auto secondaryStore = CreateSecondary();
            secondaries->Append(secondaryStore);
            secondariesCopy->Append(secondaryStore);
        }

        // Start copying files to each
        FullCopyToSecondaries(*secondariesCopy);

        // TODO: Can Apply come before EndSettingCurrentState?
        for (ULONG32 i = 0; i < numSecondaries; i++)
        {
            auto secondaryStore = (*secondaries)[i];
            SyncAwait(secondaryStore->EndSettingCurrentStateAsync(CancellationToken::None));
        }

        // Perform more modifications, which will be replicated and applied on the secondaries as well
        for (LONG64 key = 90; key < numItems; key++)
        {
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }
        }

        // Finish copying to each. Perform the checkpoint and recover the secondary
        for (ULONG32 i = 0; i < numSecondaries; i++)
        {
            auto secondaryStore = (*secondaries)[i];
            Checkpoint(*secondaryStore);
        }

        // Verify all keys copied
        {
            for (ULONG32 m = 0; m < Stores->Count(); m++)
            {
                auto txn = CreateWriteTransaction(*(*Stores)[m]);
                for (LONG64 key = 0; key < 90; key++)
                {
                    SyncAwait(VerifyKeyExistsAsync(*(*Stores)[m], *txn->StoreTransactionSPtr, key, nullptr, GetStringValue(key), StringEquals));
                }
                for (LONG64 key = 90; key < numItems; key++)
                {
                    SyncAwait(VerifyKeyDoesNotExistAsync(*(*Stores)[m], *txn->StoreTransactionSPtr, key));
                }
                SyncAwait(txn->AbortAsync());
            }
        }
    }
    
    BOOST_AUTO_TEST_CASE(Copy_CheckpointPrimaryDuringCopy_ShouldSucceed)
    {
        ULONG32 numItems = 100;
        ULONG32 checkpointFrequency = 10;

        // Setup - Add 100 items over ~10 checkpoints
        PopulateStore(numItems, checkpointFrequency);
        
        // Checkpoint to ensure we all have state written
        Checkpoint(*Store);

        // Start the copy operation to a new secondary
        auto secondaryStore = CreateSecondary();
        CopyCheckpointToSecondary(*secondaryStore);

        // Perform more modifications, which will be replicated and applied on the Secondary as well
        ULONG32 numAdditional = 10;
        checkpointFrequency = 0;
        PopulateStore(numAdditional, checkpointFrequency, numItems);

        // Checkpoint the Primary
        Checkpoint(*Store);

        // Finish the Copy operation
        Checkpoint(*secondaryStore);

        // Verify state on all replicas
        VerifyState(*Stores, numItems + numAdditional);
    }

    BOOST_AUTO_TEST_CASE(Copy_ApplyDuringCopy_BeforeCheckpoint_ShouldSucceed)
    {
        ULONG32 numItems = 100;
        ULONG32 checkpointFrequency = 10;
        // Setup - Add 100 items over ~10 checkpoints
        PopulateStore(numItems, checkpointFrequency);
        
        // Checkpoint to ensure we all have state written
        Checkpoint(*Store);

        // Start the copy operation to a new secondary
        auto secondaryStore = CreateSecondary();
        CopyCheckpointToSecondary(*secondaryStore);

        // Perform more modifications, which will be replicated and applied on the Secondary as well
        ULONG32 numAdditional = 10;
        checkpointFrequency = 0;
        PopulateStore(numAdditional, checkpointFrequency, numItems);

        // Finish the Copy operation
        Checkpoint(*secondaryStore);

        // Verify state on all replicas
        VerifyState(*Stores, numItems + numAdditional);
    }

    BOOST_AUTO_TEST_CASE(Copy_ApplyDuringCopy_DuringCheckpoint_ShouldSucceed)
    {
        ULONG32 numItems = 100;
        ULONG32 checkpointFrequency = 10;
        // Setup - Add 100 items over ~10 checkpoints
        PopulateStore(numItems, checkpointFrequency);

        // Checkpoint to ensure we all have state written
        Checkpoint(*Store);

        // Start the copy operation to a new secondary
        auto secondaryStore = CreateSecondary();
        CopyCheckpointToSecondary(*secondaryStore);

        // Perform more modifications, which will be replicated and applied on the Secondary as well
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, numItems, GetStringValue(numItems), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        // Finish the Copy operation
        auto checkpointLSN = Replicator->IncrementAndGetCommitSequenceNumber();
        secondaryStore->PrepareCheckpoint(checkpointLSN);

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, numItems + 1, GetStringValue(numItems + 1), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        SyncAwait(secondaryStore->PerformCheckpointAsync(CancellationToken::None));

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, numItems + 2, GetStringValue(numItems + 2), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        SyncAwait(secondaryStore->CompleteCheckpointAsync(CancellationToken::None));

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, numItems + 3, GetStringValue(numItems + 3), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        // Verify state on all replicas
        VerifyState(*Stores, numItems + 4);
    }

    BOOST_AUTO_TEST_CASE(Copy_CheckpointSecondaryAfterCopy_NoExtraOperations_ShouldSucceed)
    {
        ULONG32 numItems = 100;
        ULONG32 checkpointFrequency = 10;
        // Setup - Add 100 items over ~10 checkpoints
        PopulateStore(numItems, checkpointFrequency);
        
        // Start the copy operation to a new secondary
        auto secondaryStore = CreateSecondary();
        FullCopyToSecondary(*secondaryStore);

        // Perform no additional operations between the copy and additonal checkpoints
        
        // Checkpoint all the replicas 
        Checkpoint();

        // Verify state on all replicas
        VerifyState(*Stores, numItems);
    }

    BOOST_AUTO_TEST_CASE(Copy_CheckpointSecondaryAfterCopy_WithExtraOperations_ShouldSucceed)
    {
        ULONG32 numItems = 100;
        ULONG32 checkpointFrequency = 10;
        // Setup - Add 100 items over ~10 checkpoints
        PopulateStore(numItems, checkpointFrequency);

        // Copy state to a new secondary
        auto secondaryStore = CreateSecondary();
        FullCopyToSecondary(*secondaryStore);

        // Perform additional operations between the copy and addtional checkpoints
        checkpointFrequency = 0;
        PopulateStore(numItems, checkpointFrequency, numItems);

        // Checkpoint all the replicas 
        Checkpoint();

        // Verify state on all replicas
        VerifyState(*Stores, numItems * 2);
    }

    BOOST_AUTO_TEST_CASE(Copy_AfterRecovery_ShouldSucceed)
    {
        ULONG32 numItems = 10;
        ULONG32 checkpointFrequency = 0;
        // Setup - Add 100 items
        PopulateStore(numItems, checkpointFrequency);

        // Checkpoint, close store, and recover
        Checkpoint(*Store);
        CloseAndReOpenStore();

        // Copy state to a new secondary
        auto secondaryStore = CreateSecondary();
        CopyCheckpointToSecondary(*secondaryStore);
        
        // Finish the copy
        Checkpoint(*secondaryStore);
        
        // Add more items
        PopulateStore(numItems, checkpointFrequency, numItems);

        // Checkpoint primary
        Checkpoint(*Store);

        // Verify state on all replicas
        VerifyState(*Stores, numItems * 2);
    }
    
#pragma region Copy-then-Checkpoint Tests
    BOOST_AUTO_TEST_CASE(Copy_CheckpointWithZeroLSN_DuplicatesFromLogReplay_ShouldSucceed)
    {
        // Test for #10903530: Rebuild Failure: Overactive TStore Assert: PrepareCheckpointLSN can be 0.

        ULONG32 numItems = 10;
        ULONG32 checkpointFrequency = 0;
        // Setup - Add 10 items
        PopulateStore(numItems, checkpointFrequency);

        // Checkpoint, close store, and recover
        Checkpoint(*Store);
        CloseAndReOpenStore();

        // Copy state to a new secondary
        auto secondaryStore = CreateSecondary();
        CopyCheckpointToSecondary(*secondaryStore);
        
        // Checkpoint the secondary with 0 LSN
        secondaryStore->PrepareCheckpoint(0);
        SyncAwait(secondaryStore->PerformCheckpointAsync(CancellationToken::None));
        SyncAwait(secondaryStore->CompleteCheckpointAsync(CancellationToken::None));
        
        // Simulate duplicates from log replay
        for (LONG64 key = 0; key < 10; key++)
        {
            LONG64 lsn = 2 * key + 1;
            OperationContext::CSPtr unlockContext = SyncAwait(ApplyAddOperationAsync(*secondaryStore, key, GetStringValue(key), lsn, ApplyContext::SecondaryRedo));

            CODING_ERROR_ASSERT(unlockContext == nullptr); // Duplicate should not have been applied
        }
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_SingleCheckpoint_NoOpsBeforePrepare_NoOpsAfterPrepare_NoOpsAsActiveSecondary_ShouldSucceed)
    {
        Copy_Prepare_EnsureStrictCheckpointing(1, false, false, false);
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_SingleCheckpoint_NoOpsBeforePrepare_NoOpsAfterPrepare_OpsAsActiveSecondary_ShouldSucceed)
    {
        Copy_Prepare_EnsureStrictCheckpointing(1, false, false, true);
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_SingleCheckpoint_NoOpsBeforePrepare_OpsAfterPrepare_NoOpsAsActiveSecondary_ShouldSucceed)
    {
        Copy_Prepare_EnsureStrictCheckpointing(1, false, true, false);
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_SingleCheckpoint_NoOpsBeforePrepare_OpsAfterPrepare_OpsAsActiveSecondary_ShouldSucceed)
    {
        Copy_Prepare_EnsureStrictCheckpointing(1, false, true, true);
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_SingleCheckpoint_OpsBeforePrepare_NoOpsAfterPrepare_NoOpsAsActiveSecondary_ShouldSucceed)
    {
        Copy_Prepare_EnsureStrictCheckpointing(1, true, false, false);
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_SingleCheckpoint_OpsBeforePrepare_NoOpsAfterPrepare_OpsAsActiveSecondary_ShouldSucceed)
    {
        Copy_Prepare_EnsureStrictCheckpointing(1, true, false, true);
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_SingleCheckpoint_OpsBeforePrepare_OpsAfterPrepare_NoOpsAsActiveSecondary_ShouldSucceed)
    {
        Copy_Prepare_EnsureStrictCheckpointing(1, true, true, false);
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_SingleCheckpoint_OpsBeforePrepare_OpsAfterPrepare_OpsAsActiveSecondary_ShouldSucceed)
    {
        Copy_Prepare_EnsureStrictCheckpointing(1, true, true, true);
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_MultipleCheckpoints_NoOpsBeforePrepare_NoOpsAfterPrepare_NoOpsAsActiveSecondary_ShouldSucceed)
    {
        Copy_Prepare_EnsureStrictCheckpointing(8, false, false, false);
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_MultipleCheckpoints_NoOpsBeforePrepare_NoOpsAfterPrepare_OpsAsActiveSecondary_ShouldSucceed)
    {
        Copy_Prepare_EnsureStrictCheckpointing(8, false, false, true);
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_MultipleCheckpoints_NoOpsBeforePrepare_OpsAfterPrepare_NoOpsAsActiveSecondary_ShouldSucceed)
    {
        Copy_Prepare_EnsureStrictCheckpointing(8, false, true, false);
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_MultipleCheckpoints_NoOpsBeforePrepare_OpsAfterPrepare_OpsAsActiveSecondary_ShouldSucceed)
    {
        Copy_Prepare_EnsureStrictCheckpointing(8, false, true, true);
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_MultipleCheckpoints_OpsBeforePrepare_NoOpsAfterPrepare_NoOpsAsActiveSecondary_ShouldSucceed)
    {
        Copy_Prepare_EnsureStrictCheckpointing(8, true, false, false);
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_MultipleCheckpoints_OpsBeforePrepare_NoOpsAfterPrepare_OpsAsActiveSecondary_ShouldSucceed)
    {
        Copy_Prepare_EnsureStrictCheckpointing(8, true, false, true);
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_MultipleCheckpoints_OpsBeforePrepare_OpsAfterPrepare_NoOpsAsActiveSecondary_ShouldSucceed)
    {
        Copy_Prepare_EnsureStrictCheckpointing(8, true, true, false);
    }

    BOOST_AUTO_TEST_CASE(Copy_Prepare_MultipleCheckpoints_OpsBeforePrepare_OpsAfterPrepare_OpsAsActiveSecondary_ShouldSucceed)
    {
        Copy_Prepare_EnsureStrictCheckpointing(8, true, true, true);
    }

#pragma endregion Exhaustively test combinations of checkpointing, copying, and applying operations in different parts of checkpoint lifecycle
    
    BOOST_AUTO_TEST_CASE(Copy_FailSecondaryAfterCopy_VerifyOldStateRetained, * boost::unit_test_framework::disabled(/*Fails in the case where copy manager has built metadatatable but Store hasn't consumed it*/))
    {
        // Create several Secondaries ahead of time, so it has some state.
        // We will be failing one Secondary after Copy, one after EndSettingCurrentState, one after PrepareCheckpoint, and one after PerformCheckpoint.
        // They should all retain their old state on recovery, because CompleteCheckpoint was not called.

        KSharedArray<Data::TStore::Store<LONG64, KString::SPtr>::SPtr>::SPtr secondaries = _new(TEST_TAG, GetAllocator()) KSharedArray<Data::TStore::Store<LONG64, KString::SPtr>::SPtr>();
        KSharedArray<Data::TStore::Store<LONG64, KString::SPtr>::SPtr>::SPtr secondariesCopy = _new(TEST_TAG, GetAllocator()) KSharedArray<Data::TStore::Store<LONG64, KString::SPtr>::SPtr>();

        for (ULONG32 i = 0; i < 5; i++)
        {
            auto secondaryStore = CreateSecondary();
            secondaries->Append(secondaryStore);
            secondariesCopy->Append(secondaryStore);
        }

        // Setup state
        ULONG32 numItems = 100;
        ULONG32 checkpointFrequency = 0;
        PopulateStore(numItems, checkpointFrequency);
        
        // Checkpoint secondaries, to write their state to disk
        for (ULONG32 i = 0; i < secondaries->Count(); i++)
        {
            auto secondaryStore = (*secondaries)[i];
            Checkpoint(*secondaryStore);
        }

        // Verify state on the secondaries
        VerifyState(*secondaries, numItems);

        // Perform additional operations, so the Copy will attempt to overwrite the Secondaries' old state
        PopulateStore(numItems, checkpointFrequency, numItems);

        // Start Copy: copy state (SetCurrentState) to all Secondaries
        FullCopyToSecondaries(*secondariesCopy);

        // Simulate Copy failure on a Secondary by closing and re-open the Secondary after the last SetCurrentState completes. Verify old state is retained
        (*secondaries)[0] = CloseAndRecoverSecondary(*(*secondaries)[0], *(*SecondaryReplicators)[0]);
        VerifyState(*(*secondaries)[0], numItems);

        // Continue Copy: EndSettingCurrentState on remaining Secondaries
        for (ULONG32 i = 1; i < secondaries->Count(); i++)
        {
            auto secondaryStore = (*secondaries)[i];
            SyncAwait(secondaryStore->EndSettingCurrentStateAsync(CancellationToken::None));
        }

        // Simulate Copy failure on a Secondary after EndSettingCurrentState completes. Verify old state is retained
        (*secondaries)[1] = CloseAndRecoverSecondary(*(*secondaries)[1], *(*SecondaryReplicators)[1]);
        VerifyState(*(*secondaries)[1], numItems);

        // Continue Copy: PrepareCheckpointAsync on remaining Secondaries
        for (ULONG32 i = 2; i < secondaries->Count(); i++)
        {
            auto secondaryStore = (*secondaries)[i];
            auto commitLSN = Replicator->IncrementAndGetCommitSequenceNumber();
            secondaryStore->PrepareCheckpoint(commitLSN);
        }
        
        // Simulate Copy failure on a Secondary after PrepareCheckpointAsync completes. Verify old state is retained
        (*secondaries)[2] = CloseAndRecoverSecondary(*(*secondaries)[2], *(*SecondaryReplicators)[2]);
        VerifyState(*(*secondaries)[2], numItems);
        
        // Continue Copy: PrepareCheckpointAsync on remaining Secondaries
        for (ULONG32 i = 3; i < secondaries->Count(); i++)
        {
            auto secondaryStore = (*secondaries)[i];
            SyncAwait(secondaryStore->PerformCheckpointAsync(CancellationToken::None));
        }

        // Simulate Copy failure on a Secondary after PerformCheckpoint completes. Verify old state is retained
        (*secondaries)[3] = CloseAndRecoverSecondary(*(*secondaries)[3], *(*SecondaryReplicators)[3]);
        VerifyState(*(*secondaries)[3], numItems);

        // Complete Copy: CompleteCheckpointAsync on the last Secondary
        {
            auto secondaryStore = (*secondaries)[4];
            SyncAwait(secondaryStore->CompleteCheckpointAsync(CancellationToken::None));
        }

        // Simulate a failure on the secondary after CompleteCheckpointAsync completes. Verify the new state was copied correctly
        (*secondaries)[4] = CloseAndRecoverSecondary(*(*secondaries)[4], *(*SecondaryReplicators)[4]);
        VerifyState(*(*secondaries)[4], numItems * 2);
    }

    BOOST_AUTO_TEST_CASE(Copy_MultipleChunks_500KBChunks_ShouldSucceed, * boost::unit_test_framework::disabled(/* Very slow test */))
    {
        ULONG32 numItems = 4000;
        FullCopyTest(numItems);
    }

    BOOST_AUTO_TEST_CASE(Copy_VerifyIdempotencyOnOverlapBetweenCopyCheckpointAndLog_ShouldSucceed)
    {
        LONG64 key = 17;
        KString::SPtr value = GetStringValue(L"value");
        KString::SPtr value1 = GetStringValue(L"value1");
        LONG64 replayLSN = -1;

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
            replayLSN = txn->TransactionSPtr->CommitSequenceNumber;
        }

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value1, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        // Create idle secondary
        auto secondaryStore = CreateSecondary();
        (*SecondaryReplicators)[0]->SetReadable(false);
        
        // Copy state to a new secondary
        FullCopyToSecondary(*secondaryStore);

        // Replay key with lsn as part of apply
        auto transaction = CreateReplicatorTransaction();
        auto keyBytes = GetBytes(key);
        auto valueBytes = GetBytes(value);
        SecondaryApply(*secondaryStore, *transaction, replayLSN, StoreModificationType::Add, key, *keyBytes, value, valueBytes);
        
        // Assert that the key was not added to differential state
        auto version = secondaryStore->DifferentialState->Read(key);
        ASSERT_IFNOT(version == nullptr, "version should be null");

        // Change role to active secondary for reads
        (*SecondaryReplicators)[0]->Role = FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY;
        (*SecondaryReplicators)[0]->SetReadable(true);
        SyncAwait(VerifyKeyExistsAsync(*secondaryStore, key, nullptr, value1, StringEquals));
    }

    BOOST_AUTO_TEST_CASE(Copy_MultipleMergeableCheckpoints_Failover)
    {
        // Do not merge on primary
        Store->MergeHelperSPtr->CurrentMergePolicy = MergePolicy::None;
        Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

        // Add key
        // Setup state - 101 keys over 11 checkpoints
        for (LONG64 key = 0; key <= 100; key++)
        {
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, GetStringValue(key), DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }

            if (key % 10 == 0)
            {
                Checkpoint(*Store);
            }
        }

        // Update keys
        // 101 keys over 11 checkpoints
        for (LONG64 key = 0; key <= 100; key++)
        {
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, GetStringValue(key), DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());
            }

            if (key % 10 == 0)
            {
                Checkpoint(*Store);
            }
        }

        CODING_ERROR_ASSERT(22 == Store->CurrentMetadataTableSPtr->Table->Count);

        // Copy state to a new secondary
        auto secondaryStore = CreateSecondary();
        secondaryStore->MergeHelperSPtr->CurrentMergePolicy = MergePolicy::InvalidEntries;
        secondaryStore->MergeHelperSPtr->MergeFilesCountThreshold = 2;
        secondaryStore->MergeHelperSPtr->NumberOfInvalidEntries = 1;

        CopyCheckpointToSecondary(*secondaryStore);

        secondaryStore->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

        Checkpoint(*secondaryStore);

        CODING_ERROR_ASSERT(11 == secondaryStore->CurrentMetadataTableSPtr->Table->Count);

        VerifyState(*Stores, 101);

        // Failover the secondary
        secondaryStore = CloseAndRecoverSecondary(*secondaryStore, *(*SecondaryReplicators)[0]);
        
        VerifyState(*secondaryStore, 101);
    }

    BOOST_AUTO_TEST_CASE(Resurrection_EmptyStore_EmptyIdle_EmptyActive_EmptyPrimary_Success)
    {
        Test_Resurrection(
            0, // number of checkpoints per scenario
            0, // number of adds per checkpoint before resurrection
            0,  // number of adds before copy checkpoint
            0, // number of adds per checkpoint as idle after resurrection
            0, // number of adds per checkpoint as active secondary after resurrection
            0);// number of adds per checkpoint as primary after resurrection.
    }

    BOOST_AUTO_TEST_CASE(Resurrection_OneCheckpointStore_EmptyIdle_EmptyActive_EmptyPrimary_Success)
    {
        Test_Resurrection(
            1, // number of checkpoints per scenario
            1, // number of adds per checkpoint before resurrection
            0,  // number of adds before copy checkpoint
            0, // number of adds per checkpoint as idle after resurrection
            0, // number of adds per checkpoint as active secondary after resurrection
            0);// number of adds per checkpoint as primary after resurrection.
    }

    BOOST_AUTO_TEST_CASE(Resurrection_MutlipleCheckpointStore_EmptyIdle_EmptyActive_EmptyPrimary_Success)
    {
        Test_Resurrection(
            8, // number of checkpoints per scenario
            16, // number of adds per checkpoint before resurrection
            0,  // number of adds before copy checkpoint
            0, // number of adds per checkpoint as idle after resurrection
            0, // number of adds per checkpoint as active secondary after resurrection
            0);// number of adds per checkpoint as primary after resurrection.
    }

    BOOST_AUTO_TEST_CASE(Resurrection_MutlipleCheckpointStore_MutlipleIdle_MultipleActive_EmptyPrimary_Success)
    {
        Test_Resurrection(
            8, // number of checkpoints per scenario
            16, // number of adds per checkpoint before resurrection
            0,  // number of adds before copy checkpoint
            16, // number of adds per checkpoint as idle after resurrection
            16, // number of adds per checkpoint as active secondary after resurrection
            0);// number of adds per checkpoint as primary after resurrection.
    }

    BOOST_AUTO_TEST_CASE(Resurrection_MutlipleCheckpointStore_MultiplePreCheckpoint_MutlipleIdle_MultipleActive_EmptyPrimary_Success)
    {
        Test_Resurrection(
            8, // number of checkpoints per scenario
            16, // number of adds per checkpoint before resurrection
            16,  // number of adds before copy checkpoint
            16, // number of adds per checkpoint as idle after resurrection
            16, // number of adds per checkpoint as active secondary after resurrection
            0);// number of adds per checkpoint as primary after resurrection.
    }

    BOOST_AUTO_TEST_CASE(Resurrection_MutlipleCheckpointStore_EmptyIdle_EmptyActive_MultiplePrimary_Success)
    {
        Test_Resurrection(
            8, // number of checkpoints per scenario
            16, // number of adds per checkpoint before resurrection
            0,  // number of adds before copy checkpoint
            0, // number of adds per checkpoint as idle after resurrection
            0, // number of adds per checkpoint as active secondary after resurrection
            16);// number of adds per checkpoint as primary after resurrection.
    }

    BOOST_AUTO_TEST_CASE(AbortedCopy_ShouldSucceed)
    {
        // Populate store with 1000 items over ~10 checkpoints
        PopulateStore(1000, 100);
        auto secondaryStore = CreateSecondary();

        // Get the current state from the Primary
        auto copyState = Store->GetCurrentState();

        // Start copying the state to the Secondary
        secondaryStore->BeginSettingCurrentState();

        // Copy some state, but not all 
        // ~10 checkpoints = >20 operations, so only copying 8
        for (LONG64 stateRecordNumber = 0; stateRecordNumber < 8; stateRecordNumber++)
        {
            OperationData::CSPtr operationData;
            SyncAwait(copyState->GetNextAsync(ktl::CancellationToken::None, operationData));
            if (operationData == nullptr)
            {
                copyState->Dispose();
                CODING_ERROR_ASSERT(false); // Should not get here
            }

            SyncAwait(secondaryStore->SetCurrentStateAsync(stateRecordNumber, *operationData, ktl::CancellationToken::None));
        }

        // Prematurely call EndSettingCurrentState
        SyncAwait(secondaryStore->EndSettingCurrentStateAsync(ktl::CancellationToken::None));
        copyState->Dispose();
    }

    BOOST_AUTO_TEST_SUITE_END()
}

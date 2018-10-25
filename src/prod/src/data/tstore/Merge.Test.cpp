// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'mgTP'

namespace TStoreTests
{
    using namespace ktl;

    class MergeTest : public TStoreTestBase<KString::SPtr, KBuffer::SPtr, KStringComparer, StringStateSerializer, KBufferSerializer>
    {
    public:
        bool doNotDeleteStoreFilesOnCleanUp_ = false;

        MergeTest()
        {
            Setup(1);
            Store->MergeHelperSPtr->CurrentMergePolicy = MergePolicy::InvalidEntries;
        }

        ~MergeTest()
        {
            if (doNotDeleteStoreFilesOnCleanUp_)
            {
                wstring workDirectory = Store->WorkingDirectoryCSPtr->operator LPCWSTR();
                Cleanup(false);
                Common::Directory::Delete(workDirectory, true);
            }
            else
            {
                Cleanup();
            }
        }

        static bool BufferEquals(__in KBuffer::SPtr & one, __in KBuffer::SPtr & two)
        {
            if (one == nullptr || two == nullptr)
            {
                return one == two;
            }

            ULONG numElementsOne = one->QuerySize();
            ULONG numElementsTwo = two->QuerySize();
            if (numElementsOne != numElementsTwo)
            {
                return false;
            }

            auto oneBytes = (byte *)one->GetBuffer();
            auto twoBytes = (byte *)two->GetBuffer();

            return memcmp(oneBytes, twoBytes, numElementsOne) == 0;
        }

        KString::SPtr CreateString(__in LPCWSTR value)
        {
            KString::SPtr result;
            auto status = KString::Create(result, GetAllocator(), value);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            return result;
        }

        KString::SPtr CreateString(__in ULONG32 seed)
        {
            KString::SPtr key;
            wstring str = wstring(L"test_key") + to_wstring(seed);
            auto status = KString::Create(key, GetAllocator(), str.c_str());
            Diagnostics::Validate(status);
            return key;
        }

        KBuffer::SPtr CreateBuffer(__in byte fillValue, __in  ULONG32 size = 8)
        {
            KBuffer::SPtr bufferSptr;
            auto status = KBuffer::Create(size, bufferSptr, GetAllocator());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            auto buffer = (byte *)bufferSptr->GetBuffer();
            memset(buffer, fillValue, size);

            return bufferSptr;
        }

        ConcurrentDictionary<KString::SPtr, bool>::SPtr CreateStringHashSet()
        {
            ConcurrentDictionary<KString::SPtr, bool>::SPtr hashSetSPtr = nullptr;
            //auto status = Dictionary<KString::SPtr, bool>::Create(32, K_DefaultHashFunction, *Store->KeyComparerSPtr, GetAllocator(), hashSetSPtr);
            auto status = ConcurrentDictionary<KString::SPtr, bool>::Create(Store->KeyComparerSPtr, GetAllocator(), hashSetSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            return hashSetSPtr;
        }

        void AddFileNames(__in ConcurrentDictionary<KString::SPtr, bool> & fileNames)
        {
            auto enumeratorSPtr = Store->CurrentMetadataTableSPtr->Table->GetEnumerator();
            while (enumeratorSPtr->MoveNext())
            {
                auto item = enumeratorSPtr->Current();
                FileMetadata::SPtr value = item.Value;

                auto keyCheckpointFilePath = value->CheckpointFileSPtr->KeyCheckpointFileNameSPtr;
                auto valueCheckpointFilePath = value->CheckpointFileSPtr->ValueCheckpointFileNameSPtr;

                if (!fileNames.ContainsKey(keyCheckpointFilePath))
                {
                    fileNames.Add(keyCheckpointFilePath, true);
                }

                if (!fileNames.ContainsKey(valueCheckpointFilePath))
                {
                    fileNames.Add(valueCheckpointFilePath, true);
                }
            }
        }

        void RemoveFileNames(__in ConcurrentDictionary<KString::SPtr, bool> & fileNames)
        {
            auto enumeratorSPtr = Store->CurrentMetadataTableSPtr->Table->GetEnumerator();
            while (enumeratorSPtr->MoveNext())
            {
                auto item = enumeratorSPtr->Current();
                FileMetadata::SPtr value = item.Value;

                auto keyCheckpointFilePath = value->CheckpointFileSPtr->KeyCheckpointFileNameSPtr;
                auto valueCheckpointFilePath = value->CheckpointFileSPtr->ValueCheckpointFileNameSPtr;

                if (fileNames.ContainsKey(keyCheckpointFilePath))
                {
                    fileNames.Remove(keyCheckpointFilePath);
                }

                if (fileNames.ContainsKey(valueCheckpointFilePath))
                {
                    fileNames.Remove(valueCheckpointFilePath);
                }
            }
        }

        void VerifyInvalidFilesAreDeleted(__in ConcurrentDictionary<KString::SPtr, bool> & fileNames)
        {
            CODING_ERROR_ASSERT(fileNames.Count > 0);
            auto enumeratorSPtr = fileNames.GetEnumerator();
            while (enumeratorSPtr->MoveNext())
            {
                auto current = enumeratorSPtr->Current();
                KString::SPtr fileNameSPtr = current.Key;
                bool exists = Common::File::Exists(fileNameSPtr->operator LPCWSTR());
                CODING_ERROR_ASSERT(exists == false);
            }
        }

        ktl::Awaitable<void> VerifyKeyExistsAsync(__in KString::SPtr key, __in KBuffer::SPtr expectedValue)
        {
            co_await VerifyKeyExistsInStoresAsync(key, nullptr, expectedValue, MergeTest::BufferEquals);
            co_return;
        }

        void VerifyNumberOfCheckpointFiles(ULONG32 expectedNumberOfCheckpointFilesWithoutMerge)
        {
            vector<wstring> files = Common::Directory::GetFiles(Store->WorkingDirectoryCSPtr->operator LPCWSTR());
            ULONG32 actualNumOfCheckpointFiles = static_cast<ULONG32>(files.size());

            CODING_ERROR_ASSERT(actualNumOfCheckpointFiles < expectedNumberOfCheckpointFilesWithoutMerge);
        }

        auto GetExpectedFileCount(ULONG32 numExpectedCheckpoints)
        {
            ULONG32 const numMetadataFiles = 1;
            ULONG32 const numFilesPerCheckpoint = 2;

            return numMetadataFiles + (numExpectedCheckpoints * numFilesPerCheckpoint);
        }

        ktl::Awaitable<void> VerifyKeysOnDiskAndInMemoryAsync(
            __in ConcurrentDictionary<KString::SPtr, bool> & addedKeys,
            __in ConcurrentDictionary<KString::SPtr, bool> & deletedKeys)
        {
            auto metadataTable = Store->CurrentMetadataTableSPtr->Table;
            auto serializer = Store->KeyConverterSPtr;

            auto addedList = CreateStringHashSet();
            auto deletedList = CreateStringHashSet();

            // Populate the list of added and deleted keys from disk
            auto tableEnumerator = metadataTable->GetEnumerator();
            while (tableEnumerator->MoveNext())
            {
                auto item = tableEnumerator->Current();
                auto fileMetadata = item.Value;

                {
                    auto keyEnumerator = fileMetadata->CheckpointFileSPtr->GetAsyncEnumerator<KString::SPtr, KBuffer::SPtr>(*serializer);

                    while (co_await keyEnumerator->MoveNextAsync(ktl::CancellationToken::None))
                    {
                        auto key = keyEnumerator->GetCurrent();
                        if (key->Value->GetRecordKind() == RecordKind::InsertedVersion)
                        {
                            addedList->Add(key->Key, true);
                        }
                        else if (key->Value->GetRecordKind() == RecordKind::DeletedVersion)
                        {
                            deletedList->Add(key->Key, true);
                        }
                        else
                        {
                            CODING_ERROR_ASSERT(false);
                        }
                    }

                    co_await keyEnumerator->CloseAsync();
                }
            }

            // Assert that the number of keys in deletedList (disk) <= deletedKeys (memory)
            // Some of them might have been picked up by merge
            // If nothing is merged, then the number of deleted keys in memory and on disk will be same
            auto deletedListCount = deletedList->Count;
            auto deletedKeysCount = deletedKeys.Count;
            CODING_ERROR_ASSERT(deletedListCount <= deletedKeysCount);

            // Assert that the number of added keys on disk >= memory
            // Some keys might not be merged yet, so so the disk might have a key in both added and deleted list
            CODING_ERROR_ASSERT(addedList->Count >= addedKeys.Count);

            // For each key in the addedList (disk),
            // If the key exists in the addedKeys (memory), then it should not exist on disk or memory as a deleted key
            // Else it should exist in both on disk and in memory as a deleted key
            auto addedListEnumerator = addedList->GetEnumerator();
            while (addedListEnumerator->MoveNext())
            {
                auto key = addedListEnumerator->Current().Key;
                if (addedKeys.ContainsKey(key))
                {
                    CODING_ERROR_ASSERT(deletedKeys.ContainsKey(key) == false);
                    CODING_ERROR_ASSERT(deletedList->ContainsKey(key) == false);
                }
                else
                {
                    CODING_ERROR_ASSERT(deletedKeys.ContainsKey(key) == true);
                    CODING_ERROR_ASSERT(deletedList->ContainsKey(key) == true);
                }
            }

            co_return;
        }

        ktl::Awaitable<void> AddInitialSetOfKeysAsync(ULONG32 lastKey, KBuffer::SPtr value, ConcurrentDictionary<KString::SPtr, bool> & keys)
        {
            {
                auto txn = CreateWriteTransaction();
                for (ULONG32 i = 0; i < lastKey; i++)
                {
                    auto key = CreateString(i);
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None);
                    keys.Add(key, true);
                }

                co_await txn->CommitAsync();
            }

            co_return;
        }

        ktl::Awaitable<void> AddIncrementalKeysAsync(ULONG32 numKeysPerTransaction, ULONG32 numTransactions, ULONG32 startingKey, KBuffer::SPtr value, ConcurrentDictionary<KString::SPtr, bool> & keys)
        {
            ULONG32 currentKey = startingKey;
            for (ULONG32 i = 0; i < numTransactions; i++)
            {
                auto txn = CreateWriteTransaction();
                for (ULONG32 j = 0; j < numKeysPerTransaction; j++)
                {
                    auto key = CreateString(currentKey);
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None);
                    keys.Add(key, true);
                    currentKey++;
                }

                co_await txn->CommitAsync();
            }

            co_return;
        }

        ktl::Awaitable<void> AddRandomKeysAsync(ULONG32 numKeysPerTransaction, ULONG32 numTransactions, KBuffer::SPtr value, ConcurrentDictionary<KString::SPtr, bool> & keys)
        {
            for (ULONG32 i = 0; i < numTransactions; i++)
            {
                auto txn = CreateWriteTransaction();
                for (ULONG32 j = 0; j < numKeysPerTransaction; j++)
                {
                    co_await AddKeyWithRetryAsync(*txn, value, keys);
                }
                co_await txn->CommitAsync();
            }

            co_return;
        }

        ktl::Awaitable<void> AddKeyWithRetryAsync(
            __in WriteTransaction<KString::SPtr, KBuffer::SPtr> & txn,
            __in KBuffer::SPtr value,
            __in ConcurrentDictionary<KString::SPtr, bool> & keys)
        {
            auto seed = Common::Stopwatch::Now().Ticks;
            Common::Random random(static_cast<int>(seed));

            ULONG32 retryCount = 100;

            KString::SPtr key = CreateString(random.Next());

            while (retryCount > 0 && keys.ContainsKey(key))
            {
                key = CreateString(random.Next());
                retryCount--;
            }

            co_await Store->AddAsync(*txn.StoreTransactionSPtr, key, value, DefaultTimeout, ktl::CancellationToken::None);
            keys.Add(key, true);

            co_return;
        }

        ktl::Awaitable<void> DeleteKeysAsync(
            __in ULONG32 numKeysPerTransaction,
            __in ULONG32 numTransactions,
            __in ConcurrentDictionary<KString::SPtr, bool> & addedKeys,
            __in ConcurrentDictionary<KString::SPtr, bool> & deletedKeys)
        {
            auto addedKeysEnumerator = addedKeys.GetEnumerator();
            bool hasNext = addedKeysEnumerator->MoveNext();

            ULONG32 retryCount = 100;
            while (!hasNext && retryCount > 0)
            {
                addedKeysEnumerator = addedKeys.GetEnumerator();
                hasNext = addedKeysEnumerator->MoveNext();
                retryCount--;
                co_await KTimer::StartTimerAsync(GetAllocator(), ALLOC_TAG, 100, nullptr);
            }

            if (!hasNext)
            {
                co_return;
            }

            for (ULONG32 i = 0; i < numTransactions; i++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    bool isEmptyTxn = true;

                    for (ULONG32 j = 0; j < numKeysPerTransaction; j++)
                    {
                        auto key = addedKeysEnumerator->Current().Key;
                        co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, ktl::CancellationToken::None);
                        isEmptyTxn = false;

                        addedKeys.Remove(key);
                        deletedKeys.Add(key, true);

                        // Rudimentary way to make addedKeys queue-like
                        addedKeysEnumerator = addedKeys.GetEnumerator();
                        hasNext = addedKeysEnumerator->MoveNext();

                        retryCount = 100;
                        while (!hasNext && retryCount > 0)
                        {
                            addedKeysEnumerator = addedKeys.GetEnumerator();
                            hasNext = addedKeysEnumerator->MoveNext();
                            retryCount--;
                            co_await KTimer::StartTimerAsync(GetAllocator(), ALLOC_TAG, 100, nullptr);
                        }
                    }

                    if (!isEmptyTxn)
                    {
                        co_await txn->CommitAsync();
                    }
                    else
                    {
                        co_await txn->AbortAsync();
                    }

                    if (!hasNext)
                    {
                        break;
                    }
                }
            }

            co_return;
        }


        ktl::Awaitable<void> FileCountMerge_FilesAreAllValid_MergeWhenFileCountThresholdIsReachedAsync(MergePolicy policy)
        {
            FileCountMergeConfiguration::SPtr fileCountConfigSPtr = nullptr;
            auto status = FileCountMergeConfiguration::Create(3, GetAllocator(), fileCountConfigSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            Store->MergeHelperSPtr->FileCountMergeConfigurationSPtr = *fileCountConfigSPtr;
            Store->MergeHelperSPtr->CurrentMergePolicy = policy;

            auto verySmallBuffer = CreateBuffer(0xc2, FileCountMergeConfiguration::DefaultVerySmallFileSizeThreshold / 3);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), verySmallBuffer, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();
            CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->Table->Count == 1);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(2), verySmallBuffer, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();
            CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->Table->Count == 2);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(3), verySmallBuffer, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();

            ULONG32 expectedFileCount = IsMergePolicyEnabled(policy, MergePolicy::FileCount) ? 1 : 3;
            CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->Table->Count == expectedFileCount);

            co_return;
        }

        ktl::Awaitable<void> FileCountMerge_FilesAreAllValid_MergeOnlyWhenFileTypeCountHitsThresholdAsync(MergePolicy policy)
        {
            CODING_ERROR_ASSERT(IsMergePolicyEnabled(policy, MergePolicy::FileCount));

            FileCountMergeConfiguration::SPtr fileCountConfigSPtr = nullptr;
            auto status = FileCountMergeConfiguration::Create(3, GetAllocator(), fileCountConfigSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            auto verySmallBuffer = CreateBuffer(0xc2, FileCountMergeConfiguration::DefaultVerySmallFileSizeThreshold / 3);
            auto smallBuffer = CreateBuffer(0xb6, FileCountMergeConfiguration::DefaultSmallFileSizeThreshold / 3);

            // Setup
            Store->MergeHelperSPtr->CurrentMergePolicy = policy;
            Store->MergeHelperSPtr->FileCountMergeConfigurationSPtr = *fileCountConfigSPtr;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            // Create 1 VSmall: Total: 1 VSmall
            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), verySmallBuffer, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();

            CODING_ERROR_ASSERT(1 == Store->CurrentMetadataTableSPtr->Table->Count);

            // Create 1 VSmall: Total: 2 VSmall
            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(2), verySmallBuffer, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();

            CODING_ERROR_ASSERT(2 == Store->CurrentMetadataTableSPtr->Table->Count);

            // Create 1 Small: Total: 2 VSmall, 1 Small
            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(3), smallBuffer, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();

            CODING_ERROR_ASSERT(3 == Store->CurrentMetadataTableSPtr->Table->Count);

            // Create 1 Small: Total: 2 VSmall, 2 Small
            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), smallBuffer, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();

            CODING_ERROR_ASSERT(4 == Store->CurrentMetadataTableSPtr->Table->Count);

            // Create 1 VSmall: Total 3 VSmall, 2 Small -> 3 Small
            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(5), verySmallBuffer, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();

            CODING_ERROR_ASSERT(3 == Store->CurrentMetadataTableSPtr->Table->Count);

            // Create 1 VSmall: Total 1 VSmall, 3 Small -> 1 VSmall, 1 Medium
            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(6), verySmallBuffer, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();

            ULONG32 count = Store->CurrentMetadataTableSPtr->Table->Count;
            CODING_ERROR_ASSERT(count == 2);

            co_return;
        }

        ktl::Awaitable<void> FileCountMerge_NoOpCheckpoint_MergeStillRunsAsync(MergePolicy policy)
        {
            CODING_ERROR_ASSERT(IsMergePolicyEnabled(policy, MergePolicy::FileCount));

            FileCountMergeConfiguration::SPtr fileCountConfigSPtr = nullptr;
            auto status = FileCountMergeConfiguration::Create(3, GetAllocator(), fileCountConfigSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            auto verySmallBuffer = CreateBuffer(0xc2, FileCountMergeConfiguration::DefaultVerySmallFileSizeThreshold / 3);
            auto smallBuffer = CreateBuffer(0xb6, FileCountMergeConfiguration::DefaultSmallFileSizeThreshold / 3);

            // Setup
            Store->MergeHelperSPtr->CurrentMergePolicy = policy;
            Store->MergeHelperSPtr->FileCountMergeConfigurationSPtr = *fileCountConfigSPtr;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            // Create 1 VSmall: Total: 1 VSmall
            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(1), verySmallBuffer, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();
            CODING_ERROR_ASSERT(1 == Store->CurrentMetadataTableSPtr->Table->Count);

            // Create 1 VSmall: Total: 2 VSmall
            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(2), verySmallBuffer, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();
            CODING_ERROR_ASSERT(2 == Store->CurrentMetadataTableSPtr->Table->Count);

            // Create 1 Small: Total: 2 VSmall, 1 Small
            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(3), smallBuffer, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();
            CODING_ERROR_ASSERT(3 == Store->CurrentMetadataTableSPtr->Table->Count);

            // Create 1 Small: Total: 2 VSmall, 2 Small
            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(4), smallBuffer, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();
            CODING_ERROR_ASSERT(4 == Store->CurrentMetadataTableSPtr->Table->Count);

            // Create 1 VSmall: Total 3 VSmall, 2 Small -> 3 Small
            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(5), verySmallBuffer, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();
            CODING_ERROR_ASSERT(3 == Store->CurrentMetadataTableSPtr->Table->Count);

            // Merge: Total: 3 Small -> 1 Medium
            co_await CheckpointAsync();
            co_await CheckpointAsync();

            ULONG32 count = Store->CurrentMetadataTableSPtr->Table->Count;
            CODING_ERROR_ASSERT(count == 1);

            co_return;
        }

        ktl::Awaitable<void> FileCountMerge_Upgrade_MoreThanThresholdNumberOfFilesForOneFileType_MergeThreeAtATimeAsync(MergePolicy policy)
        {
            CODING_ERROR_ASSERT(IsMergePolicyEnabled(policy, MergePolicy::FileCount));

            auto verySmallBuffer = CreateBuffer(0xc2, FileCountMergeConfiguration::DefaultVerySmallFileSizeThreshold / 3);
            auto smallBuffer = CreateBuffer(0xb6, FileCountMergeConfiguration::DefaultSmallFileSizeThreshold / 3);

            // Setup
            Store->MergeHelperSPtr->CurrentMergePolicy = MergePolicy::None;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            for (ULONG32 i = 0; i < 10; i++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(i), verySmallBuffer, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }

                co_await CheckpointAsync();
                CODING_ERROR_ASSERT(i + 1 == Store->CurrentMetadataTableSPtr->Table->Count);
            }

            // Upgrade
            FileCountMergeConfiguration::SPtr fileCountConfigSPtr = nullptr;
            auto status = FileCountMergeConfiguration::Create(3, GetAllocator(), fileCountConfigSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            Store->MergeHelperSPtr->CurrentMergePolicy = policy;
            Store->MergeHelperSPtr->FileCountMergeConfigurationSPtr = *fileCountConfigSPtr;

            // Merge 10 VSmall: 7 VSmall, 1 Small
            co_await CheckpointAsync();
            CODING_ERROR_ASSERT(8 == Store->CurrentMetadataTableSPtr->Table->Count);

            // Merge 7 VSmall: 4 VSmall, 2 Small
            co_await CheckpointAsync();
            CODING_ERROR_ASSERT(6 == Store->CurrentMetadataTableSPtr->Table->Count);

            // Merge 4 VSmall: 1 VSmall, 3 Small
            co_await CheckpointAsync();
            CODING_ERROR_ASSERT(4 == Store->CurrentMetadataTableSPtr->Table->Count);

            // Merge 1 VSmall, 3 small: 1 VSmall, 1 medium
            co_await CheckpointAsync();
            CODING_ERROR_ASSERT(2 == Store->CurrentMetadataTableSPtr->Table->Count);

            co_return;
        }

        bool IsMergePolicyEnabled(MergePolicy input, MergePolicy expected)
        {
            auto flagValue = static_cast<ULONG32>(input) & static_cast<ULONG32>(expected);
            return flagValue == static_cast<ULONG32>(expected);
        }

        ktl::Task CancelCompletionSourceWithDelayAsync(__in AwaitableCompletionSource<bool> & completionSource, __in ULONG milliseconds)
        {
            AwaitableCompletionSource<bool>::SPtr completionSourceSPtr = &completionSource;
            co_await KTimer::StartTimerAsync(GetAllocator(), ALLOC_TAG, milliseconds, nullptr, nullptr);
            completionSourceSPtr->SetCanceled();
        }

        Common::CommonConfig config; // load the config object as its needed for the tracing to work

#pragma region test functions
    public:
        ktl::Awaitable<void> Merge_InvalidFiles_WithMergePolicyNone_ShouldNotMerge_Test()
        {
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
            auto key = CreateString(1);
            auto verySmallBuffer = CreateBuffer(0xc2, FileCountMergeConfiguration::DefaultVerySmallFileSizeThreshold / 3);

            Store->MergeHelperSPtr->CurrentMergePolicy = MergePolicy::None;

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, verySmallBuffer, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();
            CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->Table->Count == 1);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, verySmallBuffer, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();
            CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->Table->Count == 2);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, verySmallBuffer, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();
            CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->Table->Count == 3);
            co_return;
        }

        ktl::Awaitable<void> Merge3Files_NoNewFileNeeded_ShouldSucceed_Test()
        {
            auto fileNamesSPtr = CreateStringHashSet();

            // Set MergeFilesCountThreshold to 3
            Store->MergeHelperSPtr->MergeFilesCountThreshold = 3;
            Store->MergeHelperSPtr->NumberOfInvalidEntries = 1;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            for (ULONG32 i = 0; i < 3; i++)
            {
                auto value = CreateBuffer(8);

                // Repeatedly update the same key once with a new value, run consolidation (by triggering checkpoint), and validate the value exists
                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(i), value, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }

                // Start consolidation
                co_await CheckpointAsync(*Store);
                AddFileNames(*fileNamesSPtr);
            }

            for (ULONG32 i = 0; i < 3; i++)
            {
                auto value = CreateBuffer(88);

                // Repeatedly update the same key once with a new value, run consolidation (by triggering checkpoint), and validate the value exists
                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(i), value, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }
            }

            // Consolidate after the 3 updates
            co_await CheckpointAsync(*Store);
            RemoveFileNames(*fileNamesSPtr);

            // Assert that the number of files is 1 (just the new checkpoint file and no merged file)
            ULONG32 count = Store->CurrentMetadataTableSPtr->Table->Count;
            CODING_ERROR_ASSERT(count == 1);
            CODING_ERROR_ASSERT(6 == fileNamesSPtr->Count);

            // Assert invalid files are deleted.
            VerifyInvalidFilesAreDeleted(*fileNamesSPtr);

            for (ULONG32 i = 0; i < 3; i++)
            {
                co_await VerifyKeyExistsAsync(CreateString(i), CreateBuffer(88));
            }

            co_await CloseAndReOpenStoreAsync();
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            for (ULONG32 i = 0; i < 3; i++)
            {
                co_await VerifyKeyExistsAsync(CreateString(i), CreateBuffer(88));
            }
            co_return;
        }

        ktl::Awaitable<void> Merge3Files_ToNewFile_ShouldSucceed_Test()
        {
            auto fileNamesSPtr = CreateStringHashSet();

            // Set MergeFilesCountThreshold to 3
            Store->MergeHelperSPtr->MergeFilesCountThreshold = 3;
            Store->MergeHelperSPtr->NumberOfInvalidEntries = 1;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            for (ULONG32 i = 1; i <= 3; i++)
            {
                auto value = CreateBuffer(8);

                // Repeatedly update the same key once with a new value, run consolidation (by triggering checkpoint), and validate the value exists
                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(i), value, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }
            }

            // Start consolidation
            co_await CheckpointAsync(*Store);
            AddFileNames(*fileNamesSPtr);

            for (ULONG32 i = 1; i <= 3; i++)
            {
                ULONG32 key = 1;
                auto value = CreateBuffer(88);

                // Repeatedly update the same key once with a new value, run consolidation (by triggering checkpoint), and validate the value exists
                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(key), value, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }

                // Start consolidation
                co_await CheckpointAsync(*Store);
                AddFileNames(*fileNamesSPtr);
            }

            RemoveFileNames(*fileNamesSPtr);

            // Assert that the number of files is 2 (1 merged file containing keys 2 and 3 but files containing key 1 is ignored, 1 new checkpoint file)
            CODING_ERROR_ASSERT(2 == Store->CurrentMetadataTableSPtr->Table->Count);

            // Assert invalid files are deleted
            VerifyInvalidFilesAreDeleted(*fileNamesSPtr);

            co_await VerifyKeyExistsAsync(CreateString(1), CreateBuffer(88));
            co_await VerifyKeyExistsAsync(CreateString(2), CreateBuffer(8));
            co_await VerifyKeyExistsAsync(CreateString(3), CreateBuffer(8));

            co_await CloseAndReOpenStoreAsync();
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            co_await VerifyKeyExistsAsync(CreateString(1), CreateBuffer(88));
            co_await VerifyKeyExistsAsync(CreateString(2), CreateBuffer(8));
            co_await VerifyKeyExistsAsync(CreateString(3), CreateBuffer(8));
            co_return;
        }

        ktl::Awaitable<void> Merge3Files_ToNewFile_WithRepeatingEntries_ShouldSucceed_Test()
        {
            auto fileNamesSPtr = CreateStringHashSet();

            // Set MergeFilesCountThreshold to 3
            Store->MergeHelperSPtr->MergeFilesCountThreshold = 3;
            Store->MergeHelperSPtr->NumberOfInvalidEntries = 1;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            // Checkpoint 6 keys, 3 times
            for (ULONG32 i = 0; i < 6; i++)
            {
                auto value = CreateBuffer(8);

                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(i), value, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }
            }

            co_await CheckpointAsync(*Store);
            AddFileNames(*fileNamesSPtr);

            for (ULONG32 j = 0; j < 2; j++)
            {
                auto value = CreateBuffer(8);

                for (ULONG32 i = 0; i < 6; i++)
                {
                    {
                        auto txn = CreateWriteTransaction();
                        co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(i), value, DefaultTimeout, CancellationToken::None);
                        co_await txn->CommitAsync();
                    }
                }

                co_await CheckpointAsync(*Store);
                AddFileNames(*fileNamesSPtr);
            }

            auto expectedValue = CreateBuffer(8);
            for (ULONG32 i = 0; i < 6; i++)
            {
                co_await VerifyKeyExistsAsync(CreateString(i), expectedValue);
            }

            // Update only the first 3 keys
            for (ULONG32 i = 0; i < 3; i++)
            {
                auto updateValue = CreateBuffer(88);
                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(i), updateValue, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }
            }

            co_await CheckpointAsync(*Store);
            RemoveFileNames(*fileNamesSPtr);

            // Assert that the number of files is 2 (1 merged, 1 new checkpoint file).
            CODING_ERROR_ASSERT(2 == Store->CurrentMetadataTableSPtr->Table->Count);

            VerifyInvalidFilesAreDeleted(*fileNamesSPtr);

            expectedValue = CreateBuffer(88);
            for (ULONG32 i = 0; i < 3; i++)
            {
                co_await VerifyKeyExistsAsync(CreateString(i), expectedValue);
            }

            expectedValue = CreateBuffer(8);
            for (ULONG32 i = 3; i < 6; i++)
            {
                co_await VerifyKeyExistsAsync(CreateString(i), expectedValue);
            }

            co_await CloseAndReOpenStoreAsync();
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            expectedValue = CreateBuffer(88);
            for (ULONG32 i = 0; i < 3; i++)
            {
                co_await VerifyKeyExistsAsync(CreateString(i), expectedValue);
            }

            expectedValue = CreateBuffer(8);
            for (ULONG32 i = 3; i < 6; i++)
            {
                co_await VerifyKeyExistsAsync(CreateString(i), expectedValue);
            }
            co_return;
        }

        ktl::Awaitable<void> Merge3Files_ToNewFile_WithRepeatingEntries_FollowedByAnotherMerge_ShouldSucceed_Test()
        {
            auto fileNamesSPtr = CreateStringHashSet();

            // Set MergeFilesCountThreshold to 3
            Store->MergeHelperSPtr->MergeFilesCountThreshold = 3;
            Store->MergeHelperSPtr->NumberOfInvalidEntries = 1;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            // Checkpoint 6 keys, 3 times
            for (ULONG32 i = 0; i < 6; i++)
            {
                auto value = CreateBuffer(8);

                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(i), value, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }
            }

            co_await CheckpointAsync(*Store);
            AddFileNames(*fileNamesSPtr);

            for (ULONG32 j = 0; j < 2; j++)
            {
                auto value = CreateBuffer(8);

                for (ULONG32 i = 0; i < 6; i++)
                {
                    {
                        auto txn = CreateWriteTransaction();
                        co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(i), value, DefaultTimeout, CancellationToken::None);
                        co_await txn->CommitAsync();
                    }
                }

                co_await CheckpointAsync(*Store);
                AddFileNames(*fileNamesSPtr);
            }

            auto expectedValue = CreateBuffer(8);
            for (ULONG32 i = 0; i < 6; i++)
            {
                co_await VerifyKeyExistsAsync(CreateString(i), expectedValue);
            }

            // Update only the first 3 keys
            for (ULONG32 i = 0; i < 3; i++)
            {
                auto updateValue = CreateBuffer(88);
                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(i), updateValue, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }
            }

            co_await CheckpointAsync(*Store);
            RemoveFileNames(*fileNamesSPtr);

            // Assert that the number of files is 2 (1 merged, 1 new checkpoint file).
            CODING_ERROR_ASSERT(2 == Store->CurrentMetadataTableSPtr->Table->Count);

            VerifyInvalidFilesAreDeleted(*fileNamesSPtr);

            expectedValue = CreateBuffer(88);
            for (ULONG32 i = 0; i < 3; i++)
            {
                co_await VerifyKeyExistsAsync(CreateString(i), expectedValue);
            }

            expectedValue = CreateBuffer(8);
            for (ULONG32 i = 3; i < 6; i++)
            {
                co_await VerifyKeyExistsAsync(CreateString(i), expectedValue);
            }

            for (ULONG32 i = 0; i < 2; i++)
            {
                // Update key 1 and key 4 and checkpoint
                auto updateValue = CreateBuffer(18);

                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(1), updateValue, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }

                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(4), updateValue, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }

                co_await CheckpointAsync(*Store);
                AddFileNames(*fileNamesSPtr);
            }

            RemoveFileNames(*fileNamesSPtr);

            // Assert that the number of files is 2 (1 merged, 1 new checkpoint file)
            CODING_ERROR_ASSERT(2 == Store->CurrentMetadataTableSPtr->Table->Count);
            VerifyInvalidFilesAreDeleted(*fileNamesSPtr);

            co_await VerifyKeyExistsAsync(CreateString((ULONG32)(0)), CreateBuffer(88));
            co_await VerifyKeyExistsAsync(CreateString(1), CreateBuffer(18));
            co_await VerifyKeyExistsAsync(CreateString(2), CreateBuffer(88));
            co_await VerifyKeyExistsAsync(CreateString(3), CreateBuffer(8));
            co_await VerifyKeyExistsAsync(CreateString(4), CreateBuffer(18));
            co_await VerifyKeyExistsAsync(CreateString(5), CreateBuffer(8));

            co_await CloseAndReOpenStoreAsync();
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            co_await VerifyKeyExistsAsync(CreateString((ULONG32)(0)), CreateBuffer(88));
            co_await VerifyKeyExistsAsync(CreateString(1), CreateBuffer(18));
            co_await VerifyKeyExistsAsync(CreateString(2), CreateBuffer(88));
            co_await VerifyKeyExistsAsync(CreateString(3), CreateBuffer(8));
            co_await VerifyKeyExistsAsync(CreateString(4), CreateBuffer(18));
            co_await VerifyKeyExistsAsync(CreateString(5), CreateBuffer(8));
            co_return;
        }

        ktl::Awaitable<void> Merge_WithDeletedKey_ShouldSucceed_Test()
        {
            // Test targed to check InvalidEntries policy
            Store->MergeHelperSPtr->CurrentMergePolicy = MergePolicy::InvalidEntries;
            // Set MergeFilesCountThreshold to 2.
            Store->MergeHelperSPtr->MergeFilesCountThreshold = 2;
            Store->MergeHelperSPtr->NumberOfInvalidEntries = 1;
            Store->EnableBackgroundConsolidation = false;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            auto fileNamesSPtr = CreateStringHashSet();

            auto key1 = CreateString(1);
            auto key2 = CreateString(2);

            auto value = CreateBuffer(8);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key1, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            // Start consolidation
            co_await CheckpointAsync(*Store);
            AddFileNames(*fileNamesSPtr);
            CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->Table->Count == 1);

            // Delete key1 and add key2
            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key2, value, DefaultTimeout, CancellationToken::None);
                bool removed = co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key1, DefaultTimeout, CancellationToken::None);
                CODING_ERROR_ASSERT(removed);
                co_await txn->CommitAsync();
            }

            // Consolidate after delete
            co_await CheckpointAsync(*Store);
            CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->Table->Count == 2);
            AddFileNames(*fileNamesSPtr);

            // Update key 2
            {
                auto updatedValue = CreateBuffer(88);
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key2, updatedValue, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            // Consolidate after update of key2 to cause merge of deleted key.
            co_await CheckpointAsync(*Store);
            RemoveFileNames(*fileNamesSPtr);

            // Assert that the number of files is 1 (1 new checkpoint file)
            CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->Table->Count == 1);
            VerifyInvalidFilesAreDeleted(*fileNamesSPtr);

            FileMetadata::SPtr fileMetadataSPtr = nullptr;
            auto enumerator = Store->CurrentMetadataTableSPtr->Table->GetEnumerator();
            CODING_ERROR_ASSERT(enumerator->MoveNext());
            fileMetadataSPtr = enumerator->Current().Value;

            // Assert that the existing file contains only key2 and the deleted key1 is gone
            KArray<KString::SPtr> results(GetAllocator());
            auto keyEnumerator = fileMetadataSPtr->CheckpointFileSPtr->GetAsyncEnumerator<KString::SPtr, KBuffer::SPtr>(*Store->KeyConverterSPtr);
            while (co_await keyEnumerator->MoveNextAsync(CancellationToken::None))
            {
                results.Append(keyEnumerator->GetCurrent()->Key);
            }
            co_await keyEnumerator->CloseAsync();

            CODING_ERROR_ASSERT(results.Count() == 1);
            CODING_ERROR_ASSERT(Store->KeyComparerSPtr->Compare(results[0], key2) == 0);

            co_await VerifyKeyExistsAsync(key2, CreateBuffer(88));
            co_await VerifyKeyDoesNotExistInStoresAsync(key1);

            co_await CloseAndReOpenStoreAsync();
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            co_await VerifyKeyExistsAsync(key2, CreateBuffer(88));
            co_await VerifyKeyDoesNotExistInStoresAsync(key1);
            co_return;
        }

        ktl::Awaitable<void> Merge_WithDeletedKey_ShouldBeInMergedFile_ShouldSucceed_Test()
        {
            // Set MergeFilesCountThreshold to 2
            Store->MergeHelperSPtr->MergeFilesCountThreshold = 2;
            Store->MergeHelperSPtr->NumberOfInvalidEntries = 2;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            auto fileNamesSPtr = CreateStringHashSet();

            auto key1 = CreateString(1);
            auto key2 = CreateString(2);
            auto key3 = CreateString(3);
            auto key4 = CreateString(4);
            auto key5 = CreateString(5);

            auto value = CreateBuffer(0xad);
            auto updateValue = CreateBuffer(0x45);

            // Add key1
            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key1, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            // Consolidate
            co_await CheckpointAsync();
            AddFileNames(*fileNamesSPtr);

            // Delete key1 and add key2 and key3
            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key2, value, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key3, value, DefaultTimeout, CancellationToken::None);
                co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key1, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            // Consolidate after delete
            co_await CheckpointAsync();
            AddFileNames(*fileNamesSPtr);

            // Add key4, key5
            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key4, value, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key5, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            // Consolidate
            co_await CheckpointAsync();
            AddFileNames(*fileNamesSPtr);

            // Update key2, key3, key4, and key5
            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key2, updateValue, DefaultTimeout, CancellationToken::None);
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key3, updateValue, DefaultTimeout, CancellationToken::None);
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key4, updateValue, DefaultTimeout, CancellationToken::None);
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key5, updateValue, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            // Consolidate
            co_await CheckpointAsync();
            RemoveFileNames(*fileNamesSPtr);

            // Assert that the number of files is 3 (1 latest file, 1 merged file and the file containing key1)
            CODING_ERROR_ASSERT(3 == Store->CurrentMetadataTableSPtr->Table->Count);
            VerifyInvalidFilesAreDeleted(*fileNamesSPtr);

            co_await VerifyKeyDoesNotExistInStoresAsync(key1);
            co_await VerifyKeyExistsAsync(key2, updateValue);
            co_await VerifyKeyExistsAsync(key3, updateValue);
            co_await VerifyKeyExistsAsync(key4, updateValue);
            co_await VerifyKeyExistsAsync(key5, updateValue);

            co_await CloseAndReOpenStoreAsync();
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            co_await VerifyKeyDoesNotExistInStoresAsync(key1);
            co_await VerifyKeyExistsAsync(key2, updateValue);
            co_await VerifyKeyExistsAsync(key3, updateValue);
            co_await VerifyKeyExistsAsync(key4, updateValue);
            co_await VerifyKeyExistsAsync(key5, updateValue);
            co_return;
        }

        ktl::Awaitable<void> Merge_WithDuplicateDeletedKeys_ShouldSucceed_Test()
        {
            // Set MergeFileCountThreshold to 2
            Store->MergeHelperSPtr->MergeFilesCountThreshold = 2;
            Store->MergeHelperSPtr->NumberOfInvalidEntries = 1;

            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            auto key1 = CreateString(1);
            auto key2 = CreateString(2);

            auto value = CreateBuffer(0xc2);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key1, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key2, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key1, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            VersionedItem<KBuffer::SPtr>::SPtr versionedItem1SPtr = Store->DifferentialState->Read(key1);
            CODING_ERROR_ASSERT(versionedItem1SPtr->GetRecordKind() == RecordKind::DeletedVersion);

            // Start consolidation
            co_await CheckpointAsync(*Store);

            DeletedVersionedItem<KBuffer::SPtr>::SPtr deletedItem = nullptr;
            DeletedVersionedItem<KBuffer::SPtr>::Create(GetAllocator(), deletedItem);
            deletedItem->SetVersionSequenceNumber(versionedItem1SPtr->GetVersionSequenceNumber());
            VersionedItem<KBuffer::SPtr>::SPtr versionedItem2SPtr = static_cast<VersionedItem<KBuffer::SPtr> *>(deletedItem.RawPtr());

            // Add an item with the same lsn to the differential state
            Store->DifferentialState->Add(key1, *versionedItem2SPtr, *Store->ConsolidationManagerSPtr);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key2, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync(*Store);

            // Cause merge
            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key2, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync(*Store);
            co_return;
        }

        ktl::Awaitable<void> Merge_WithDuplicateDeletedKeys_MergeAgain_ShouldSucceed_Test()
        {
            Store->MergeHelperSPtr->MergeFilesCountThreshold = 2;
            Store->MergeHelperSPtr->NumberOfInvalidEntries = 1;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            auto key1 = CreateString(1);
            auto key2 = CreateString(2);
            auto key3 = CreateString(3);
            auto key4 = CreateString(4);

            auto value = CreateBuffer(0xbe);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key4, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            // Cause a checkpoint write and ensure deleted keys do not get removed by creating a checkpoint file with key that never gets updated
            // Since this will have a logical timestamp that is lower than the files qualified for merge, the deleted items cannot be deleted
            co_await CheckpointAsync(*Store);
            CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->Table->Count == 1);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key1, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key2, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key1, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key3, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            VersionedItem<KBuffer::SPtr>::SPtr versionedItem1SPtr = Store->DifferentialState->Read(key1);

            // Start consolidation
            co_await CheckpointAsync(*Store);
            CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->Table->Count == 2);

            DeletedVersionedItem<KBuffer::SPtr>::SPtr deletedItem = nullptr;
            DeletedVersionedItem<KBuffer::SPtr>::Create(GetAllocator(), deletedItem);
            deletedItem->SetVersionSequenceNumber(versionedItem1SPtr->GetVersionSequenceNumber());
            VersionedItem<KBuffer::SPtr>::SPtr versionedItem2SPtr = static_cast<VersionedItem<KBuffer::SPtr> *>(deletedItem.RawPtr());

            // Add an item with the same lsn to the differential state
            Store->DifferentialState->Add(key1, *versionedItem2SPtr, *Store->ConsolidationManagerSPtr);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key3, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key2, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync(*Store);
            CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->Table->Count == 3);

            // Cause first merge
            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key2, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync(*Store);
            CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->Table->Count == 3);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key3, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync(*Store);
            CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->Table->Count == 4);

            // Cause second merge
            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key3, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync(*Store);
            CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->Table->Count == 4);

            co_await CloseAndReOpenStoreAsync();
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            co_await VerifyKeyDoesNotExistAsync(*Store, key1);
            co_await VerifyKeyExistsAsync(key2, value);
            co_await VerifyKeyExistsAsync(key3, value);
            co_await VerifyKeyExistsAsync(key4, value);
            co_return;
        }

        ktl::Awaitable<void> FileCountMerge_FilesAreAllValid_MergeWhenFileCountThresholdIsReached_MergePolicyNone_ShouldSucceed_Test()
        {
            Store->EnableBackgroundConsolidation = false;
            co_await FileCountMerge_FilesAreAllValid_MergeWhenFileCountThresholdIsReachedAsync(MergePolicy::None);
            co_return;
        }

        ktl::Awaitable<void> FileCountMerge_FilesAreAllValid_MergeWhenFileCountThresholdIsReached_MergePolicyFileCount_ShouldSucceed_Test()
        {
            Store->EnableBackgroundConsolidation = false;
            co_await FileCountMerge_FilesAreAllValid_MergeWhenFileCountThresholdIsReachedAsync(MergePolicy::FileCount);
            co_return;
        }

        ktl::Awaitable<void> FileCountMerge_FilesAreAllValid_MergeWhenFileCountThresholdIsReached_MergePolicyAll_ShouldSucceed_Test()
        {
            Store->EnableBackgroundConsolidation = false;
            co_await FileCountMerge_FilesAreAllValid_MergeWhenFileCountThresholdIsReachedAsync(MergePolicy::All);
            co_return;
        }

        ktl::Awaitable<void> FileCountMerge_FilesAreAllValid_MergeOnlyWhenFileTypeCountHitsThreshold_MergePolicyFileCount_ShouldSucceed_Test()
        {
            Store->EnableBackgroundConsolidation = false;
            co_await FileCountMerge_FilesAreAllValid_MergeOnlyWhenFileTypeCountHitsThresholdAsync(MergePolicy::FileCount);
            co_return;
        }

        ktl::Awaitable<void> FileCountMerge_FilesAreAllValid_MergeOnlyWhenFileTypeCountHitsThreshold_MergePolicyAll_ShouldSucceed_Test()
        {
            Store->EnableBackgroundConsolidation = false;
            co_await FileCountMerge_FilesAreAllValid_MergeOnlyWhenFileTypeCountHitsThresholdAsync(MergePolicy::All);
            co_return;
        }

        ktl::Awaitable<void> FileCountMerge_NoOpCheckpoint_MergeStillRuns_MergePolicyFileCount_ShouldSucceed_Test()
        {
            Store->EnableBackgroundConsolidation = false;
            co_await FileCountMerge_NoOpCheckpoint_MergeStillRunsAsync(MergePolicy::FileCount);
            co_return;
        }

        ktl::Awaitable<void> FileCountMerge_NoOpCheckpoint_MergeStillRuns_MergePolicyAll_ShouldSucceed_Test()
        {
            Store->EnableBackgroundConsolidation = false;
            co_await FileCountMerge_NoOpCheckpoint_MergeStillRunsAsync(MergePolicy::All);
            co_return;
        }

        ktl::Awaitable<void> FileCountMerge_Upgrade_MoreThanThresholdNumberOfFilesForOneFileType_MergeThreeAtATime_MergePolicyFileCount_ShouldSucceed_Test()
        {
            Store->EnableBackgroundConsolidation = false;
            co_await FileCountMerge_Upgrade_MoreThanThresholdNumberOfFilesForOneFileType_MergeThreeAtATimeAsync(MergePolicy::FileCount);
            co_return;
        }

        ktl::Awaitable<void> FileCountMerge_Upgrade_MoreThanThresholdNumberOfFilesForOneFileType_MergeThreeAtATime_MergePolicyAll_ShouldSucceed_Test()
        {
            Store->EnableBackgroundConsolidation = false;
            co_await FileCountMerge_Upgrade_MoreThanThresholdNumberOfFilesForOneFileType_MergeThreeAtATimeAsync(MergePolicy::All);
            co_return;
        }

        ktl::Awaitable<void> Merge_WithBackgroundConsolidation_ShouldSucceed_Test()
        {
            auto filenamesSPtr = CreateStringHashSet();

            // Set MergeFilesCountThreshold to 3
            Store->MergeHelperSPtr->MergeFilesCountThreshold = 3;
            Store->MergeHelperSPtr->NumberOfInvalidEntries = 1;
            Store->EnableBackgroundConsolidation = true;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            for (ULONG32 i = 0; i < 3; i++)
            {
                auto value = CreateBuffer(0x3e);

                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, CreateString(i), value, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }
            }

            // Start consolidation
            co_await CheckpointAsync(*Store, false);
            AddFileNames(*filenamesSPtr);

            ktl::AwaitableCompletionSource<bool>::SPtr consolidationTcs = nullptr;
            ktl::AwaitableCompletionSource<bool>::Create(GetAllocator(), ALLOC_TAG, consolidationTcs);

            for (ULONG32 i = 0; i < 3; i++)
            {
                auto value = CreateBuffer(0xc8);
                // Repeatedly update the same key
                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, CreateString(1), value, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }

                if (i == 2)
                {
                    // Block consolidation here
                    Store->TestDelayOnConsolidationSPtr = consolidationTcs;
                }

                co_await CheckpointAsync(*Store, false);
                AddFileNames(*filenamesSPtr);
            }

            co_await VerifyKeyExistsAsync(CreateString(static_cast<ULONG32>(0)), CreateBuffer(0x3e));
            co_await VerifyKeyExistsAsync(CreateString(1), CreateBuffer(0xc8));
            co_await VerifyKeyExistsAsync(CreateString(2), CreateBuffer(0x3e));

            // TODO: Sweep 3x

            // Signal consolidation
            consolidationTcs->SetResult(true);

            // Read again
            co_await VerifyKeyExistsAsync(CreateString(static_cast<ULONG32>(0)), CreateBuffer(0x3e));
            co_await VerifyKeyExistsAsync(CreateString(1), CreateBuffer(0xc8));
            co_await VerifyKeyExistsAsync(CreateString(2), CreateBuffer(0x3e));

            co_await Store->ConsolidationTcs->GetAwaitable();

            CODING_ERROR_ASSERT(Store->MergeMetadataTableSPtr != nullptr);

            // Read again
            co_await VerifyKeyExistsAsync(CreateString(static_cast<ULONG32>(0)), CreateBuffer(0x3e));
            co_await VerifyKeyExistsAsync(CreateString(1), CreateBuffer(0xc8));
            co_await VerifyKeyExistsAsync(CreateString(2), CreateBuffer(0x3e));

            // Perform another checkpoint
            co_await CheckpointAsync(*Store, false);
            RemoveFileNames(*filenamesSPtr);

            CODING_ERROR_ASSERT(Store->MergeMetadataTableSPtr == nullptr);

            // Assert that number of files is 2 (1 merged file containing keys 2 and 3 but files containing key 1 is ignored, 1 new checkpoint file)
            CODING_ERROR_ASSERT(Store->CurrentMetadataTableSPtr->Table->Count == 2);

            VerifyInvalidFilesAreDeleted(*filenamesSPtr);
            co_return;
        }

        ktl::Awaitable<void> CheckpointRemove_WithMerge_SnapshotRead_ShouldSucceed_Test()
        {
            auto key1 = CreateString(1);
            auto value = CreateBuffer(0x64);
            auto updateValue = CreateBuffer(0xe3);

            Store->MergeHelperSPtr->NumberOfInvalidEntries = 1;
            Store->MergeHelperSPtr->MergeFilesCountThreshold = 1;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key1, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync(*Store);
            co_await CloseAndReOpenStoreAsync();
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            // Create the snapshot transaction to read from consolidated
            auto snapshotTxn = CreateWriteTransaction();
            snapshotTxn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            // Snapshot read from consolidated
            co_await static_cast<TStoreTestBase *>(this)->VerifyKeyExistsAsync(
                *Store, *snapshotTxn->StoreTransactionSPtr, key1, nullptr, value, MergeTest::BufferEquals);

            // Update
            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key1, updateValue, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();

            CODING_ERROR_ASSERT(Store->SnapshotContainerSPtr->GetCount() == 1);

            // Snapshot read from snapshot component
            co_await static_cast<TStoreTestBase *>(this)->VerifyKeyExistsAsync(
                *Store, *(snapshotTxn->StoreTransactionSPtr), key1, nullptr, value, MergeTest::BufferEquals);

            co_await snapshotTxn->AbortAsync();
            co_return;
        }

        ktl::Awaitable<void> Merge_RemoveState_ShouldSucceed_Test()
        {
            Store->MergeHelperSPtr->MergeFilesCountThreshold = 2;
            Store->MergeHelperSPtr->NumberOfInvalidEntries = 1;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            auto key = CreateString(1);
            auto value1 = CreateBuffer(0xba);
            auto value2 = CreateBuffer(0xcd);
            auto value3 = CreateBuffer(0xef);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value1, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync(*Store);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value2, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync(*Store);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value3, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            // Consolidate after the 3 updates, results in merge
            auto checkpointLSN = Replicator->IncrementAndGetCommitSequenceNumber();
            Store->PrepareCheckpoint(checkpointLSN);
            co_await Store->PerformCheckpointAsync(CancellationToken::None);

            // Close and re-open store again
            co_await CloseAndReOpenStoreAsync();
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_PreparePerformMergeCloseOpen_ShouldSucceed_Test()
        {
            Store->MergeHelperSPtr->MergeFilesCountThreshold = 2;
            Store->MergeHelperSPtr->NumberOfInvalidEntries = 1;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            auto key = CreateString(1);
            auto value1 = CreateBuffer(0xba);
            auto value2 = CreateBuffer(0xcd);
            auto value3 = CreateBuffer(0xef);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value1, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync(*Store);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value2, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync(*Store);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value3, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            // Consolidate after the 3 updates, results in merge
            co_await CheckpointAsync(*Store);

            co_await Store->RemoveStateAsync(CancellationToken::None);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_AddDeleteCheckpointRecoverAddCheckpoint_ShouldSucceed_Test()
        {
            Store->MergeHelperSPtr->MergeFilesCountThreshold = 1;
            Store->MergeHelperSPtr->NumberOfInvalidEntries = 1;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            auto key1 = CreateString(1);
            auto key2 = CreateString(2);
            auto value = CreateBuffer(0x24);

            // Add and checkpoint
            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key1, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync(*Store);

            // Delete and checkpoint
            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key1, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key2, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync(*Store);

            co_await CloseAndReOpenStoreAsync();

            Store->MergeHelperSPtr->MergeFilesCountThreshold = 1;
            Store->MergeHelperSPtr->NumberOfInvalidEntries = 1;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;

            // Update and checkpoint so that the file containing deleted entry is removed on merge
            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key2, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();

            // Add the same key again
            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key1, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync();

            co_await VerifyKeyExistsAsync(key1, value);
            co_return;
        }

        ktl::Awaitable<void> Merge_AddDelete_Checkpoint_MergePolicyAll_ShouldSucceed_Test()
        {
            Store->MergeHelperSPtr->CurrentMergePolicy = MergePolicy::All;

            auto addedKeys = CreateStringHashSet();
            auto deletedKeys = CreateStringHashSet();

            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
            Store->EnableBackgroundConsolidation = false;

            KBuffer::SPtr value = CreateBuffer(8);

            ULONG32 initialSetupCount = 5;
            co_await AddInitialSetOfKeysAsync(initialSetupCount, value, *addedKeys);
            co_await CheckpointAsync();

            ULONG32 numTxns = 10;
            ULONG32 numKeyPerTxn = 10;
            ULONG32 numIterations = 10;

            ULONG32 expectedNumOfCheckpointFilesWithoutMerge = 1;

            for (ULONG32 i = 0; i < numIterations; i++)
            {
                co_await AddIncrementalKeysAsync(numKeyPerTxn, numTxns, initialSetupCount + (i * numTxns * numKeyPerTxn), value, *addedKeys);
                co_await DeleteKeysAsync(numKeyPerTxn, numTxns, *addedKeys, *deletedKeys);
                co_await CheckpointAsync();
                expectedNumOfCheckpointFilesWithoutMerge += 2;
            }

            VerifyNumberOfCheckpointFiles(expectedNumOfCheckpointFilesWithoutMerge);
            co_await VerifyKeysOnDiskAndInMemoryAsync(*addedKeys, *deletedKeys);
            co_return;
        }

        ktl::Awaitable<void> Merge_AddDelete_Checkpoint_MergePolicyInvalidAndDeletedEntries_ShouldSucceed_Test()
        {
            Store->MergeHelperSPtr->CurrentMergePolicy = static_cast<MergePolicy>(MergePolicy::InvalidEntries | MergePolicy::DeletedEntries);

            auto addedKeys = CreateStringHashSet();
            auto deletedKeys = CreateStringHashSet();

            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
            Store->EnableBackgroundConsolidation = false;

            KBuffer::SPtr value = CreateBuffer(8);

            ULONG32 initialSetupCount = 5;
            co_await AddInitialSetOfKeysAsync(initialSetupCount, value, *addedKeys);
            co_await CheckpointAsync();

            ULONG32 numTxns = 10;
            ULONG32 numKeyPerTxn = 10;
            ULONG32 numIterations = 10;

            ULONG32 expectedNumOfCheckpointFilesWithoutMerge = 1;

            for (ULONG32 i = 0; i < numIterations; i++)
            {
                co_await AddIncrementalKeysAsync(numKeyPerTxn, numTxns, initialSetupCount + (i * numTxns * numKeyPerTxn), value, *addedKeys);
                co_await DeleteKeysAsync(numKeyPerTxn, numTxns, *addedKeys, *deletedKeys);
                co_await CheckpointAsync();
                expectedNumOfCheckpointFilesWithoutMerge += 2;
            }

            VerifyNumberOfCheckpointFiles(expectedNumOfCheckpointFilesWithoutMerge);
            co_await VerifyKeysOnDiskAndInMemoryAsync(*addedKeys, *deletedKeys);
            co_return;
        }

        ktl::Awaitable<void> Merge_AddDelete_Checkpoint_NoInitialState_MergePolicyInvalidAndDeletedEntries_ShouldSucceed_Test()
        {
            Store->MergeHelperSPtr->CurrentMergePolicy = MergePolicy::DeletedEntries;

            auto addedKeys = CreateStringHashSet();
            auto deletedKeys = CreateStringHashSet();

            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
            Store->EnableBackgroundConsolidation = false;

            KBuffer::SPtr value = CreateBuffer(8);

            ULONG32 initialSetupCount = 0;

            ULONG32 numTxns = 10;
            ULONG32 numKeyPerTxn = 10;
            ULONG32 numIterations = 10;

            ULONG32 expectedNumOfCheckpointFilesWithoutMerge = 1;

            for (ULONG32 i = 0; i < numIterations; i++)
            {
                co_await AddIncrementalKeysAsync(numKeyPerTxn, numTxns, initialSetupCount + (i * numTxns * numKeyPerTxn), value, *addedKeys);
                co_await DeleteKeysAsync(numKeyPerTxn, numTxns, *addedKeys, *deletedKeys);
                co_await CheckpointAsync();
                expectedNumOfCheckpointFilesWithoutMerge += 2;
            }

            VerifyNumberOfCheckpointFiles(expectedNumOfCheckpointFilesWithoutMerge);
            co_await VerifyKeysOnDiskAndInMemoryAsync(*addedKeys, *deletedKeys);
            co_return;
        }

        ktl::Awaitable<void> Merge_Add_Checkpoint_Delete_Checkpoint_MergePolicyAll_ShouldSucceed_Test()
        {
            Store->MergeHelperSPtr->CurrentMergePolicy = MergePolicy::All;

            auto addedKeys = CreateStringHashSet();
            auto deletedKeys = CreateStringHashSet();

            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
            Store->EnableBackgroundConsolidation = false;

            KBuffer::SPtr value = CreateBuffer(8);

            ULONG32 numTxns = 10;
            ULONG32 numKeyPerTxn = 10;
            ULONG32 numIterations = 10;

            ULONG32 expectedNumOfCheckpointFilesWithoutMerge = 1;

            for (ULONG32 i = 0; i < numIterations; i++)
            {
                co_await AddRandomKeysAsync(numKeyPerTxn, numTxns, value, *addedKeys);
                co_await CheckpointAsync();
                expectedNumOfCheckpointFilesWithoutMerge += 2;

                co_await DeleteKeysAsync(numKeyPerTxn, numTxns, *addedKeys, *deletedKeys);
                co_await CheckpointAsync();
                expectedNumOfCheckpointFilesWithoutMerge += 2;
            }

            VerifyNumberOfCheckpointFiles(expectedNumOfCheckpointFilesWithoutMerge);
            co_await VerifyKeysOnDiskAndInMemoryAsync(*addedKeys, *deletedKeys);
            co_return;
        }

        ktl::Awaitable<void> Merge_AddDeleteInParallel_Checkpoint_MergePolicyAll_Test()
        {
            Store->MergeHelperSPtr->CurrentMergePolicy = MergePolicy::All;

            auto addedKeys = CreateStringHashSet();
            auto deletedKeys = CreateStringHashSet();

            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
            Store->EnableBackgroundConsolidation = false;

            KBuffer::SPtr value = CreateBuffer(8);

            ULONG32 initialSetupCount = 10;
            co_await AddInitialSetOfKeysAsync(initialSetupCount, value, *addedKeys);

            ULONG32 numTxns = 10;
            ULONG32 numKeyPerTxn = 10;
            ULONG32 numIterations = 10;

            ULONG32 expectedNumOfCheckpointFilesWithoutMerge = 1;

            for (ULONG32 i = 0; i < numIterations; i++)
            {
                auto addTask = AddIncrementalKeysAsync(numKeyPerTxn, numTxns, initialSetupCount + (i * numTxns * numKeyPerTxn), value, *addedKeys);
                auto deleteTask = DeleteKeysAsync(numKeyPerTxn, numTxns, *addedKeys, *deletedKeys);

                co_await addTask;
                co_await deleteTask;
                co_await CheckpointAsync();

                expectedNumOfCheckpointFilesWithoutMerge += 2;
            }

            VerifyNumberOfCheckpointFiles(expectedNumOfCheckpointFilesWithoutMerge);
            co_await VerifyKeysOnDiskAndInMemoryAsync(*addedKeys, *deletedKeys);
            co_return;
        }

        ktl::Awaitable<void> Merge_AddDeleteInParallel_Checkpoint_MergePolicyInvalidAndDeletedEntries_Test()
        {
            Store->MergeHelperSPtr->CurrentMergePolicy = static_cast<MergePolicy>(MergePolicy::InvalidEntries | MergePolicy::DeletedEntries);

            auto addedKeys = CreateStringHashSet();
            auto deletedKeys = CreateStringHashSet();

            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
            Store->EnableBackgroundConsolidation = false;

            KBuffer::SPtr value = CreateBuffer(8);

            ULONG32 initialSetupCount = 10;
            co_await AddInitialSetOfKeysAsync(initialSetupCount, value, *addedKeys);

            ULONG32 numTxns = 10;
            ULONG32 numKeyPerTxn = 10;
            ULONG32 numIterations = 10;

            ULONG32 expectedNumOfCheckpointFilesWithoutMerge = 1;

            for (ULONG32 i = 0; i < numIterations; i++)
            {
                auto addTask = AddIncrementalKeysAsync(numKeyPerTxn, numTxns, initialSetupCount + (i * numTxns * numKeyPerTxn), value, *addedKeys);
                auto deleteTask = DeleteKeysAsync(numKeyPerTxn, numTxns, *addedKeys, *deletedKeys);

                co_await addTask;
                co_await deleteTask;
                co_await CheckpointAsync();

                expectedNumOfCheckpointFilesWithoutMerge += 2;
            }

            VerifyNumberOfCheckpointFiles(expectedNumOfCheckpointFilesWithoutMerge);
            co_await VerifyKeysOnDiskAndInMemoryAsync(*addedKeys, *deletedKeys);
            co_return;
        }

        ktl::Awaitable<void> Merge_AddDeleteInParallel_Checkpoint_MergePolicyDeletedEntries_Test()
        {
            Store->MergeHelperSPtr->CurrentMergePolicy = MergePolicy::DeletedEntries;

            auto addedKeys = CreateStringHashSet();
            auto deletedKeys = CreateStringHashSet();

            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
            Store->EnableBackgroundConsolidation = false;

            KBuffer::SPtr value = CreateBuffer(8);

            ULONG32 initialSetupCount = 10;
            co_await AddInitialSetOfKeysAsync(initialSetupCount, value, *addedKeys);

            ULONG32 numTxns = 10;
            ULONG32 numKeyPerTxn = 10;
            ULONG32 numIterations = 10;

            ULONG32 expectedNumOfCheckpointFilesWithoutMerge = 1;

            for (ULONG32 i = 0; i < numIterations; i++)
            {
                auto addTask = AddIncrementalKeysAsync(numKeyPerTxn, numTxns, initialSetupCount + (i * numTxns * numKeyPerTxn), value, *addedKeys);
                auto deleteTask = DeleteKeysAsync(numKeyPerTxn, numTxns, *addedKeys, *deletedKeys);

                co_await addTask;
                co_await deleteTask;
                co_await CheckpointAsync();

                expectedNumOfCheckpointFilesWithoutMerge += 2;
            }

            VerifyNumberOfCheckpointFiles(expectedNumOfCheckpointFilesWithoutMerge);
            co_await VerifyKeysOnDiskAndInMemoryAsync(*addedKeys, *deletedKeys);
            co_return;
        }

        ktl::Awaitable<void> Merge_Add_Checkpoint_Delete_Checkpoint_MergePolicyInvalidAndDeletedEntries_ShouldSucceed_Test()
        {
            Store->MergeHelperSPtr->CurrentMergePolicy = static_cast<MergePolicy>(MergePolicy::InvalidEntries | MergePolicy::DeletedEntries);

            auto addedKeys = CreateStringHashSet();
            auto deletedKeys = CreateStringHashSet();

            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
            Store->EnableBackgroundConsolidation = false;

            KBuffer::SPtr value = CreateBuffer(8);

            ULONG32 initialSetupCount = 10;
            co_await AddInitialSetOfKeysAsync(initialSetupCount, value, *addedKeys);

            ULONG32 numTxns = 10;
            ULONG32 numKeyPerTxn = 10;
            ULONG32 numIterations = 10;

            ULONG32 expectedNumOfCheckpointFilesWithoutMerge = 1;

            for (ULONG32 i = 0; i < numIterations; i++)
            {
                co_await AddRandomKeysAsync(numKeyPerTxn, numTxns, value, *addedKeys);
                co_await CheckpointAsync();
                expectedNumOfCheckpointFilesWithoutMerge += 2;

                co_await DeleteKeysAsync(numKeyPerTxn, numTxns, *addedKeys, *deletedKeys);
                co_await CheckpointAsync();
                expectedNumOfCheckpointFilesWithoutMerge += 2;
            }

            VerifyNumberOfCheckpointFiles(expectedNumOfCheckpointFilesWithoutMerge);
            co_await VerifyKeysOnDiskAndInMemoryAsync(*addedKeys, *deletedKeys);
            co_return;
        }

        ktl::Awaitable<void> Merge_Add_Checkpoint_Delete_Checkpoint_MergePolicyInvalidAndDeletedEntries_DefaultSettings_Test()
        {
            auto addedKeys = CreateStringHashSet();
            auto deletedKeys = CreateStringHashSet();

            KBuffer::SPtr value = CreateBuffer(8);

            ULONG32 numTxns = 10;
            ULONG32 numKeyPerTxn = 10;
            ULONG32 numIterations = 15;

            ULONG32 expectedNumOfCheckpointFilesWithoutMerge = 1;

            for (ULONG32 i = 0; i < numIterations; i++)
            {
                co_await AddIncrementalKeysAsync(numKeyPerTxn, numTxns, i * numKeyPerTxn * numTxns, value, *addedKeys);
                co_await CheckpointAsync();
                expectedNumOfCheckpointFilesWithoutMerge += 2;

                co_await VerifyKeysOnDiskAndInMemoryAsync(*addedKeys, *deletedKeys);

                co_await DeleteKeysAsync(numKeyPerTxn, numTxns, *addedKeys, *deletedKeys);
                co_await CheckpointAsync();
                expectedNumOfCheckpointFilesWithoutMerge += 2;

                co_await VerifyKeysOnDiskAndInMemoryAsync(*addedKeys, *deletedKeys);
            }

            VerifyNumberOfCheckpointFiles(expectedNumOfCheckpointFilesWithoutMerge);
            co_await CloseAndReOpenStoreAsync();
            co_await VerifyKeysOnDiskAndInMemoryAsync(*addedKeys, *deletedKeys);
            co_return;
        }

        ktl::Awaitable<void> Merge_SizeOnDiskPolicy_5Files_BiggerThanThreshold_WithDeletedEntry_ShouldMerge_Test()
        {
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
            Store->EnableBackgroundConsolidation = false;
            Store->MergeHelperSPtr->CurrentMergePolicy = MergePolicy::SizeOnDisk;
            Store->MergeHelperSPtr->SizeOnDiskThreshold = 1024 * 1024;

            ULONG32 const numCheckpoints = 4;
            ULONG32 const numKeysPerCheckpoint = 25;

            for (ULONG i = 0; i < numCheckpoints; i++)
            {
                auto value = CreateBuffer('s', 10000);
                for (ULONG32 keyOffset = 0; keyOffset < numKeysPerCheckpoint; keyOffset++)
                {
                    auto key = CreateString(i * numKeysPerCheckpoint + keyOffset);
                    auto txn = CreateWriteTransaction();
                    co_await Store->AddAsync(
                        *txn->StoreTransactionSPtr,
                        key,
                        value,
                        DefaultTimeout,
                        CancellationToken::None);
                    co_await txn->CommitAsync();
                }

                co_await CheckpointAsync();

                CODING_ERROR_ASSERT(i + 1 == Store->CurrentMetadataTableSPtr->Table->Count);
            }

            ULONG64 totalSize = 0;
            auto enumeratorSPtr = Store->CurrentMetadataTableSPtr->Table->GetEnumerator();
            while (enumeratorSPtr->MoveNext())
            {
                auto fileMetadata = enumeratorSPtr->Current().Value;
                totalSize += co_await fileMetadata->GetFileSizeAsync();
            }

            CODING_ERROR_ASSERT(totalSize > Store->MergeHelperSPtr->SizeOnDiskThreshold);

            // Delete a single key to trigger a merge
            {
                auto keyToDelete = CreateString(5);

                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalRemoveAsync(
                    *txn->StoreTransactionSPtr,
                    keyToDelete,
                    DefaultTimeout,
                    CancellationToken::None);
                co_await txn->CommitAsync();
            }

            auto checkpointLSN = Replicator->IncrementAndGetCommitSequenceNumber();

            // The current metadata table must have keys 1, 2, 3, 4
            auto currTable = Store->CurrentMetadataTableSPtr->Table;
            CODING_ERROR_ASSERT(currTable->Count == numCheckpoints);
            for (ULONG32 key = 1; key <= numCheckpoints; key++)
            {
                CODING_ERROR_ASSERT(currTable->ContainsKey(key));
            }

            Store->PrepareCheckpoint(checkpointLSN);
            co_await Store->PerformCheckpointAsync(CancellationToken::None);

            // The next metadata table must have just key 6 since everything merges into it
            auto nextTable = Store->NextMetadataTableSPtr->Table;
            CODING_ERROR_ASSERT(nextTable->Count == 1);
            CODING_ERROR_ASSERT(nextTable->ContainsKey(6));

            co_await Store->CompleteCheckpointAsync(CancellationToken::None);

            // There should only be one checkpoint
            ULONG32 const numExpectedCheckpoints = 1;
            auto files = Common::Directory::GetFiles(Store->WorkingDirectoryCSPtr->operator LPCWSTR());
            CODING_ERROR_ASSERT(GetExpectedFileCount(numExpectedCheckpoints) == files.size());
            co_return;
        }

        ktl::Awaitable<void> Merge_SizeOnDiskPolicy_5Files_BiggerThanThreshold_WithInvalidEntry_ShouldMerge_Test()
        {
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
            Store->EnableBackgroundConsolidation = false;
            Store->MergeHelperSPtr->CurrentMergePolicy = MergePolicy::SizeOnDisk;
            Store->MergeHelperSPtr->SizeOnDiskThreshold = 1024 * 1024;

            ULONG32 const numCheckpoints = 4;
            ULONG32 const numKeysPerCheckpoint = 25;

            for (ULONG i = 0; i < numCheckpoints; i++)
            {
                auto value = CreateBuffer('s', 10000);
                for (ULONG32 keyOffset = 0; keyOffset < numKeysPerCheckpoint; keyOffset++)
                {
                    auto key = CreateString(i * numKeysPerCheckpoint + keyOffset);
                    auto txn = CreateWriteTransaction();
                    co_await Store->AddAsync(
                        *txn->StoreTransactionSPtr,
                        key,
                        value,
                        DefaultTimeout,
                        CancellationToken::None);
                    co_await txn->CommitAsync();
                }

                co_await CheckpointAsync();

                CODING_ERROR_ASSERT(i + 1 == Store->CurrentMetadataTableSPtr->Table->Count);
            }

            ULONG64 totalSize = 0;
            auto enumeratorSPtr = Store->CurrentMetadataTableSPtr->Table->GetEnumerator();
            while (enumeratorSPtr->MoveNext())
            {
                auto fileMetadata = enumeratorSPtr->Current().Value;
                totalSize += co_await fileMetadata->GetFileSizeAsync();
            }

            CODING_ERROR_ASSERT(totalSize > Store->MergeHelperSPtr->SizeOnDiskThreshold);

            // Invalidate a single key to trigger a merge
            {
                auto keyToUpdate = CreateString(5);
                auto updateValue = CreateBuffer('t', 5);

                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(
                    *txn->StoreTransactionSPtr,
                    keyToUpdate,
                    updateValue,
                    DefaultTimeout,
                    CancellationToken::None);
                co_await txn->CommitAsync();
            }

            auto checkpointLSN = Replicator->IncrementAndGetCommitSequenceNumber();

            // The current metadata table must have keys 1, 2, 3, 4
            auto currTable = Store->CurrentMetadataTableSPtr->Table;
            CODING_ERROR_ASSERT(currTable->Count == numCheckpoints);
            for (ULONG32 key = 1; key <= numCheckpoints; key++)
            {
                CODING_ERROR_ASSERT(currTable->ContainsKey(key));
            }

            Store->PrepareCheckpoint(checkpointLSN);
            co_await Store->PerformCheckpointAsync(CancellationToken::None);

            // The next metadata table must have just key 6 since everything merges into it
            auto nextTable = Store->NextMetadataTableSPtr->Table;
            CODING_ERROR_ASSERT(nextTable->Count == 1);
            CODING_ERROR_ASSERT(nextTable->ContainsKey(6));

            co_await Store->CompleteCheckpointAsync(CancellationToken::None);

            // There should only be one checkpoint
            ULONG32 const numExpectedCheckpoints = 1;
            auto files = Common::Directory::GetFiles(Store->WorkingDirectoryCSPtr->operator LPCWSTR());
            CODING_ERROR_ASSERT(GetExpectedFileCount(numExpectedCheckpoints) == files.size());
            co_return;
        }

        ktl::Awaitable<void> Merge_SizeOnDiskPolicy_5Files_BiggerThanThreshold_WithInvalidAndDeletedEntry_ShouldMerge_Test()
        {
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
            Store->EnableBackgroundConsolidation = false;
            Store->MergeHelperSPtr->CurrentMergePolicy = MergePolicy::SizeOnDisk;
            Store->MergeHelperSPtr->SizeOnDiskThreshold = 1024 * 1024;

            ULONG32 const numCheckpoints = 4;
            ULONG32 const numKeysPerCheckpoint = 25;

            for (ULONG32 i = 0; i < numCheckpoints; i++)
            {
                auto value = CreateBuffer('s', 10000);
                for (ULONG32 keyOffset = 0; keyOffset < numKeysPerCheckpoint; keyOffset++)
                {
                    auto key = CreateString(i * numKeysPerCheckpoint + keyOffset);
                    auto txn = CreateWriteTransaction();
                    co_await Store->AddAsync(
                        *txn->StoreTransactionSPtr,
                        key,
                        value,
                        DefaultTimeout,
                        CancellationToken::None);
                    co_await txn->CommitAsync();
                }

                co_await CheckpointAsync();

                CODING_ERROR_ASSERT(i + 1 == Store->CurrentMetadataTableSPtr->Table->Count);
            }

            ULONG64 totalSize = 0;
            auto enumeratorSPtr = Store->CurrentMetadataTableSPtr->Table->GetEnumerator();
            while (enumeratorSPtr->MoveNext())
            {
                auto fileMetadata = enumeratorSPtr->Current().Value;
                totalSize += co_await fileMetadata->GetFileSizeAsync();
            }

            CODING_ERROR_ASSERT(totalSize > Store->MergeHelperSPtr->SizeOnDiskThreshold);

            // Invalidate a single key and delete another to trigger a merge
            {
                auto keyToUpdate = CreateString(5);
                auto updateValue = CreateBuffer('t', 5);

                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(
                    *txn->StoreTransactionSPtr,
                    keyToUpdate,
                    updateValue,
                    DefaultTimeout,
                    CancellationToken::None);
                co_await txn->CommitAsync();
            }
            {
                auto keyToDelete = CreateString(6);

                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalRemoveAsync(
                    *txn->StoreTransactionSPtr,
                    keyToDelete,
                    DefaultTimeout,
                    CancellationToken::None);
                co_await txn->CommitAsync();
            }

            auto checkpointLSN = Replicator->IncrementAndGetCommitSequenceNumber();

            // The current metadata table must have keys 1, 2, 3, 4
            auto currTable = Store->CurrentMetadataTableSPtr->Table;
            CODING_ERROR_ASSERT(currTable->Count == numCheckpoints);
            for (ULONG32 key = 1; key <= numCheckpoints; key++)
            {
                CODING_ERROR_ASSERT(currTable->ContainsKey(key));
            }

            Store->PrepareCheckpoint(checkpointLSN);
            co_await Store->PerformCheckpointAsync(CancellationToken::None);

            // The next metadata table must have just key 6 since everything merges into it
            auto nextTable = Store->NextMetadataTableSPtr->Table;
            CODING_ERROR_ASSERT(nextTable->Count == 1);
            CODING_ERROR_ASSERT(nextTable->ContainsKey(6));

            co_await Store->CompleteCheckpointAsync(CancellationToken::None);

            // There should only be one checkpoint
            ULONG32 const numExpectedCheckpoints = 1;
            auto files = Common::Directory::GetFiles(Store->WorkingDirectoryCSPtr->operator LPCWSTR());
            CODING_ERROR_ASSERT(GetExpectedFileCount(numExpectedCheckpoints) == files.size());
            co_return;
        }

        ktl::Awaitable<void> Merge_AbandonedInBackground_OnClose_ShouldDisposeAllFileHandles_Test()
        {
            Store->MergeHelperSPtr->MergeFilesCountThreshold = 2;
            Store->MergeHelperSPtr->NumberOfInvalidEntries = 1;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
            Store->EnableBackgroundConsolidation = true;

            ktl::AwaitableCompletionSource<bool>::SPtr consolidationTcsSPtr = nullptr;
            auto status = ktl::AwaitableCompletionSource<bool>::Create(GetAllocator(), ALLOC_TAG, consolidationTcsSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            // Block consolidation here
            Store->TestDelayOnConsolidationSPtr = consolidationTcsSPtr;

            auto key = CreateString(7);
            auto value = CreateBuffer(0x32);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync(*Store, false);

            for (ULONG32 i = 0; i < 2; i++)
            {
                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                    co_await txn->CommitAsync();
                }

                co_await CheckpointAsync(*Store, false);
            }

            consolidationTcsSPtr->SetCanceled();

            try
            {
                if (Store->ConsolidationTcs != nullptr)
                {
                    co_await Store->ConsolidationTcs->GetAwaitable();
                }
            }
            catch (ktl::Exception const &)
            {
                //Swallow exception
            }

            KArray<FileMetadata::SPtr> fileMetadataList(GetAllocator());
            auto currentEnumeratorSPtr = Store->CurrentMetadataTableSPtr->Table->GetEnumerator();
            while (currentEnumeratorSPtr->MoveNext())
            {
                auto item = currentEnumeratorSPtr->Current();
                fileMetadataList.Append(item.Value);
                co_await item.Value->ReleaseReferenceAsync();
            }

            CODING_ERROR_ASSERT(Store->FilesToBeDeletedSPtr->Count == 0);

            currentEnumeratorSPtr = Store->CurrentMetadataTableSPtr->Table->GetEnumerator();
            while (currentEnumeratorSPtr->MoveNext())
            {
                auto item = currentEnumeratorSPtr->Current();
                auto metadataSPtr = item.Value;
                Common::File::Delete(metadataSPtr->CheckpointFileSPtr->KeyCheckpointFileNameSPtr->operator LPCWSTR());
                Common::File::Delete(metadataSPtr->CheckpointFileSPtr->ValueCheckpointFileNameSPtr->operator LPCWSTR());

            }

            Store->CurrentMetadataTableSPtr->TestMarkAsClosed();
            doNotDeleteStoreFilesOnCleanUp_ = true;
            co_return;
        }

        ktl::Awaitable<void> RecoverStore_WithAbandonedMerge_ShouldSucceed_Test()
        {
            Store->MergeHelperSPtr->MergeFilesCountThreshold = 2;
            Store->MergeHelperSPtr->NumberOfInvalidEntries = 1;
            Store->ConsolidationManagerSPtr->NumberOfDeltasToBeConsolidated = 1;
            Store->EnableBackgroundConsolidation = true;

            auto key1 = CreateString(7); // This key is updated multiple times, triggering merge
            auto key2 = CreateString(6); // This key is added once and will be moved during the merge. Merge will be cancelled after this key has been written
            auto value = CreateBuffer(0x32);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key1, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key2, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync(*Store, false);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key1, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            co_await CheckpointAsync(*Store, false);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key1, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            ktl::AwaitableCompletionSource<bool>::SPtr consolidationTcsSPtr = nullptr;
            auto status = ktl::AwaitableCompletionSource<bool>::Create(GetAllocator(), ALLOC_TAG, consolidationTcsSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            // Block consolidation here
            Store->TestDelayOnConsolidationSPtr = consolidationTcsSPtr;

            co_await CheckpointAsync(*Store, false);

            // This Task should complete after Cleanup starts
            CancelCompletionSourceWithDelayAsync(*consolidationTcsSPtr, 1000);

            co_await CloseAndReOpenStoreAsync();

            co_await static_cast<TStoreTestBase *>(this)->VerifyKeyExistsAsync(
                *Store, key1, nullptr, value, BufferEquals);
            co_return;
        }
    #pragma endregion
    };
    BOOST_FIXTURE_TEST_SUITE(MergeTestSuite, MergeTest)

        BOOST_AUTO_TEST_CASE(Merge_InvalidFiles_WithMergePolicyNone_ShouldNotMerge)
    {
        SyncAwait(Merge_InvalidFiles_WithMergePolicyNone_ShouldNotMerge_Test());
    }

    BOOST_AUTO_TEST_CASE(Merge3Files_NoNewFileNeeded_ShouldSucceed)
    {
        SyncAwait(Merge3Files_NoNewFileNeeded_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Merge3Files_ToNewFile_ShouldSucceed)
    {
        SyncAwait(Merge3Files_ToNewFile_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Merge3Files_ToNewFile_WithRepeatingEntries_ShouldSucceed)
    {
        SyncAwait(Merge3Files_ToNewFile_WithRepeatingEntries_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Merge3Files_ToNewFile_WithRepeatingEntries_FollowedByAnotherMerge_ShouldSucceed)
    {
        SyncAwait(Merge3Files_ToNewFile_WithRepeatingEntries_FollowedByAnotherMerge_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Merge_WithDeletedKey_ShouldSucceed)
    {
        SyncAwait(Merge_WithDeletedKey_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Merge_WithDeletedKey_ShouldBeInMergedFile_ShouldSucceed)
    {
        SyncAwait(Merge_WithDeletedKey_ShouldBeInMergedFile_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Merge_WithDuplicateDeletedKeys_ShouldSucceed)
    {
        SyncAwait(Merge_WithDuplicateDeletedKeys_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Merge_WithDuplicateDeletedKeys_MergeAgain_ShouldSucceed)
    {
        SyncAwait(Merge_WithDuplicateDeletedKeys_MergeAgain_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(FileCountMerge_FilesAreAllValid_MergeWhenFileCountThresholdIsReached_MergePolicyNone_ShouldSucceed)
    {
        SyncAwait(FileCountMerge_FilesAreAllValid_MergeWhenFileCountThresholdIsReached_MergePolicyNone_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(FileCountMerge_FilesAreAllValid_MergeWhenFileCountThresholdIsReached_MergePolicyFileCount_ShouldSucceed)
    {
        SyncAwait(FileCountMerge_FilesAreAllValid_MergeWhenFileCountThresholdIsReached_MergePolicyFileCount_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(FileCountMerge_FilesAreAllValid_MergeWhenFileCountThresholdIsReached_MergePolicyAll_ShouldSucceed)
    {
        SyncAwait(FileCountMerge_FilesAreAllValid_MergeWhenFileCountThresholdIsReached_MergePolicyAll_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(FileCountMerge_FilesAreAllValid_MergeOnlyWhenFileTypeCountHitsThreshold_MergePolicyFileCount_ShouldSucceed)
    {
        SyncAwait(FileCountMerge_FilesAreAllValid_MergeOnlyWhenFileTypeCountHitsThreshold_MergePolicyFileCount_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(FileCountMerge_FilesAreAllValid_MergeOnlyWhenFileTypeCountHitsThreshold_MergePolicyAll_ShouldSucceed)
    {
        SyncAwait(FileCountMerge_FilesAreAllValid_MergeOnlyWhenFileTypeCountHitsThreshold_MergePolicyAll_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(FileCountMerge_NoOpCheckpoint_MergeStillRuns_MergePolicyFileCount_ShouldSucceed)
    {
        SyncAwait(FileCountMerge_NoOpCheckpoint_MergeStillRuns_MergePolicyFileCount_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(FileCountMerge_NoOpCheckpoint_MergeStillRuns_MergePolicyAll_ShouldSucceed)
    {
        SyncAwait(FileCountMerge_NoOpCheckpoint_MergeStillRuns_MergePolicyAll_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(FileCountMerge_Upgrade_MoreThanThresholdNumberOfFilesForOneFileType_MergeThreeAtATime_MergePolicyFileCount_ShouldSucceed)
    {
        SyncAwait(FileCountMerge_Upgrade_MoreThanThresholdNumberOfFilesForOneFileType_MergeThreeAtATime_MergePolicyFileCount_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(FileCountMerge_Upgrade_MoreThanThresholdNumberOfFilesForOneFileType_MergeThreeAtATime_MergePolicyAll_ShouldSucceed)
    {
        SyncAwait(FileCountMerge_Upgrade_MoreThanThresholdNumberOfFilesForOneFileType_MergeThreeAtATime_MergePolicyAll_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Merge_WithBackgroundConsolidation_ShouldSucceed)
    {
        SyncAwait(Merge_WithBackgroundConsolidation_ShouldSucceed_Test());
    }

#pragma region Checkpoint Tests
    BOOST_AUTO_TEST_CASE(CheckpointRemove_WithMerge_SnapshotRead_ShouldSucceed)
    {
        SyncAwait(CheckpointRemove_WithMerge_SnapshotRead_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Merge_RemoveState_ShouldSucceed)
    {
        SyncAwait(Merge_RemoveState_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_PreparePerformMergeCloseOpen_ShouldSucceed)
    {
        SyncAwait(Checkpoint_PreparePerformMergeCloseOpen_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_AddDeleteCheckpointRecoverAddCheckpoint_ShouldSucceed)
    {
        SyncAwait(Checkpoint_AddDeleteCheckpointRecoverAddCheckpoint_ShouldSucceed_Test());
    }

#pragma endregion

#pragma region Merge AddDelete Workloads
    BOOST_AUTO_TEST_CASE(Merge_AddDelete_Checkpoint_MergePolicyAll_ShouldSucceed)
    {
        SyncAwait(Merge_AddDelete_Checkpoint_MergePolicyAll_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Merge_AddDelete_Checkpoint_MergePolicyInvalidAndDeletedEntries_ShouldSucceed)
    {
        SyncAwait(Merge_AddDelete_Checkpoint_MergePolicyInvalidAndDeletedEntries_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Merge_AddDelete_Checkpoint_NoInitialState_MergePolicyInvalidAndDeletedEntries_ShouldSucceed)
    {
        SyncAwait(Merge_AddDelete_Checkpoint_NoInitialState_MergePolicyInvalidAndDeletedEntries_ShouldSucceed_Test());
    }


    BOOST_AUTO_TEST_CASE(Merge_Add_Checkpoint_Delete_Checkpoint_MergePolicyAll_ShouldSucceed)
    {
        SyncAwait(Merge_Add_Checkpoint_Delete_Checkpoint_MergePolicyAll_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Merge_AddDeleteInParallel_Checkpoint_MergePolicyAll)
    {
        SyncAwait(Merge_AddDeleteInParallel_Checkpoint_MergePolicyAll_Test());
    }

    BOOST_AUTO_TEST_CASE(Merge_AddDeleteInParallel_Checkpoint_MergePolicyInvalidAndDeletedEntries)
    {
        SyncAwait(Merge_AddDeleteInParallel_Checkpoint_MergePolicyInvalidAndDeletedEntries_Test());
    }

    BOOST_AUTO_TEST_CASE(Merge_AddDeleteInParallel_Checkpoint_MergePolicyDeletedEntries)
    {
        SyncAwait(Merge_AddDeleteInParallel_Checkpoint_MergePolicyDeletedEntries_Test());
    }

    BOOST_AUTO_TEST_CASE(Merge_Add_Checkpoint_Delete_Checkpoint_MergePolicyInvalidAndDeletedEntries_ShouldSucceed)
    {
        SyncAwait(Merge_Add_Checkpoint_Delete_Checkpoint_MergePolicyInvalidAndDeletedEntries_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Merge_Add_Checkpoint_Delete_Checkpoint_MergePolicyInvalidAndDeletedEntries_DefaultSettings)
    {
        SyncAwait(Merge_Add_Checkpoint_Delete_Checkpoint_MergePolicyInvalidAndDeletedEntries_DefaultSettings_Test());
    }
#pragma endregion

    BOOST_AUTO_TEST_CASE(Merge_SizeOnDiskPolicy_5Files_BiggerThanThreshold_WithDeletedEntry_ShouldMerge)
    {
        SyncAwait(Merge_SizeOnDiskPolicy_5Files_BiggerThanThreshold_WithDeletedEntry_ShouldMerge_Test());
    }

    BOOST_AUTO_TEST_CASE(Merge_SizeOnDiskPolicy_5Files_BiggerThanThreshold_WithInvalidEntry_ShouldMerge)
    {
        SyncAwait(Merge_SizeOnDiskPolicy_5Files_BiggerThanThreshold_WithInvalidEntry_ShouldMerge_Test());
    }

    BOOST_AUTO_TEST_CASE(Merge_SizeOnDiskPolicy_5Files_BiggerThanThreshold_WithInvalidAndDeletedEntry_ShouldMerge)
    {
        SyncAwait(Merge_SizeOnDiskPolicy_5Files_BiggerThanThreshold_WithInvalidAndDeletedEntry_ShouldMerge_Test());
    }

    BOOST_AUTO_TEST_CASE(Merge_AbandonedInBackground_OnClose_ShouldDisposeAllFileHandles)
    {
        SyncAwait(Merge_AbandonedInBackground_OnClose_ShouldDisposeAllFileHandles_Test());
    }

    BOOST_AUTO_TEST_CASE(RecoverStore_WithAbandonedMerge_ShouldSucceed)
    {
        SyncAwait(RecoverStore_WithAbandonedMerge_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_SUITE_END()
}

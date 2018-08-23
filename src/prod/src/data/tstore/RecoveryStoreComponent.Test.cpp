// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#define ALLOC_TAG 'sbTP'

namespace TStoreTests
{
    using namespace ktl;

    class RecoveryStoreComponentTest
    {
    public:
        RecoveryStoreComponentTest()
        {
            NTSTATUS status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);
        }

        ~RecoveryStoreComponentTest()
        {
            ktlSystem_->Shutdown();
        }

        KAllocator& GetAllocator()
        {
            return ktlSystem_->NonPagedAllocator();
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

        KString::SPtr GetWorkingDirectory()
        {
            KString::SPtr workingDirectorySPtr;

#if !defined(PLATFORM_UNIX)
            NTSTATUS status = KString::Create(workingDirectorySPtr, GetAllocator(), L"\\??\\");
#else
            NTSTATUS status = KString::Create(workingDirectorySPtr, GetAllocator(), L"");
#endif
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            WCHAR currentDirectoryPathCharArray[MAX_PATH];
            GetCurrentDirectory(MAX_PATH, currentDirectoryPathCharArray);

            BOOLEAN concatSuccess = workingDirectorySPtr->Concat(currentDirectoryPathCharArray);
            CODING_ERROR_ASSERT(concatSuccess == TRUE);

            return workingDirectorySPtr;
        }

        KString::SPtr CreateFileString(__in KStringView const & name)
        {
            KString::SPtr fileName = GetWorkingDirectory();

            BOOLEAN concatSuccess = fileName->Concat(Common::Path::GetPathSeparatorWstr().c_str());
            CODING_ERROR_ASSERT(concatSuccess == TRUE);

            concatSuccess = fileName->Concat(name);
            CODING_ERROR_ASSERT(concatSuccess == TRUE);

            return fileName.RawPtr();
        }

        KSharedPtr<IEnumerator<KeyValuePair<int, KSharedPtr<VersionedItem<ULONG32>>>>> GetEnumerator(
           __in KSharedArray<KeyValuePair<int, KSharedPtr<VersionedItem<ULONG32>>>> const & items)
        {
           KSharedPtr<ArrayKeyVersionedItemEnumerator<int, ULONG32>> arrayEnumeratorSPtr = nullptr;
           NTSTATUS status = ArrayKeyVersionedItemEnumerator<int, ULONG32>::Create(items, GetAllocator(), arrayEnumeratorSPtr);
           CODING_ERROR_ASSERT(NT_SUCCESS(status));

           KSharedPtr<IEnumerator<KeyValuePair<int, KSharedPtr<VersionedItem<ULONG32>>>>> enumeratorSPtr =
              static_cast<IEnumerator<KeyValuePair<int, KSharedPtr<VersionedItem<ULONG32>>>> *>(arrayEnumeratorSPtr.RawPtr());

           CODING_ERROR_ASSERT(enumeratorSPtr != nullptr);
           return enumeratorSPtr;
        }

        void CreateRecoveryStoreComponent(
            __in MetadataTable & metadataTable,
            __out RecoveryStoreComponent<int, ULONG32>::SPtr & result)
        {
            auto workingDirectorySPtr = GetWorkingDirectory();
            auto traceId = CreateTraceComponent();
            
            IntComparer::SPtr keyComparerSPtr;
            IntComparer::Create(GetAllocator(), keyComparerSPtr);

            TestStateSerializer<int>::SPtr keySerializerSPtr;
            TestStateSerializer<int>::Create(GetAllocator(), keySerializerSPtr);

            NTSTATUS status = RecoveryStoreComponent<int, ULONG32>::Create(
                metadataTable,
                *workingDirectorySPtr,
                *traceId,
                *(keyComparerSPtr.DownCast<IComparer<int>>()),
                *keySerializerSPtr,
                FALSE,
                GetAllocator(),
                result);
            Diagnostics::Validate(NT_SUCCESS(status));
        }

        KSharedPtr<KSharedArray<KeyValuePair<int, KSharedPtr<VersionedItem<ULONG32>>>>> GenerateCheckpointData(
            __in ULONG32 count,
            __in ULONG32 keySeedStart = 0,
            __in ULONG32 sequenceNumberStart = 0)
        {
            KSharedPtr<KSharedArray<KeyValuePair<int, KSharedPtr<VersionedItem<ULONG32>>>>> itemsArray =
                _new(TEST_TAG, GetAllocator()) KSharedArray<KeyValuePair<int, KSharedPtr<VersionedItem<ULONG32>>>>();

            for (ULONG32 i = 0; i < count; i++)
            {
                int key = i + keySeedStart;
                LONG64 sequenceNumber = i + sequenceNumberStart;

                InsertedVersionedItem<ULONG32>::SPtr valueSPtr = nullptr;
                NTSTATUS status = InsertedVersionedItem<ULONG32>::Create(GetAllocator(), valueSPtr);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));

                valueSPtr->SetValue(static_cast<ULONG32>(sequenceNumber));
                valueSPtr->SetValueSize(sizeof(ULONG32));
                valueSPtr->SetVersionSequenceNumber(sequenceNumber);
                KSharedPtr<VersionedItem<ULONG32>> item(&(*valueSPtr));

                KeyValuePair<int, KSharedPtr<VersionedItem<ULONG32>>> pair(key, item);
                itemsArray->Append(pair);
            }

            return itemsArray;
        }

        ktl::Awaitable<CheckpointFile::SPtr> CreateCheckpointFileAsync(
            __in KStringView & filename, 
            __in ULONG32 fileId,
            __in ULONG32 numItems, 
            __in ULONG32 keySeedStart = 0,
            __in ULONG32 sequenceNumberStart = 0)
        {
            KString::SPtr filepath = CreateFileString(filename);
            
            auto traceId = CreateTraceComponent();

            TestStateSerializer<int>::SPtr keySerializerSPtr;
            TestStateSerializer<int>::Create(GetAllocator(), keySerializerSPtr);

            TestStateSerializer<ULONG32>::SPtr valueSerializerSPtr;
            TestStateSerializer<ULONG32>::Create(GetAllocator(), valueSerializerSPtr);
            
            KSharedPtr<KSharedArray<KeyValuePair<int, KSharedPtr<VersionedItem<ULONG32>>>>> checkpointFileData = GenerateCheckpointData(numItems, keySeedStart, sequenceNumberStart);
            KSharedPtr<KSharedArray<KeyValuePair<int, KSharedPtr<VersionedItem<ULONG32>>>> const> items = checkpointFileData.RawPtr();

            StorePerformanceCountersSPtr perfCounters = nullptr;
            auto checkpointFileSPtr = co_await CheckpointFile::CreateAsync<int, ULONG32>(
                fileId, 
                *filepath, 
                *(GetEnumerator(*items)),
                *keySerializerSPtr,
                *valueSerializerSPtr,
                1,
                GetAllocator(),
                *traceId,
                perfCounters,
                true);
            co_return checkpointFileSPtr;
         }

    private:
        KtlSystem* ktlSystem_;
#pragma region test functions
    public:
        ktl::Awaitable<void> Recover_EmptyMetadataTable_ShouldSucceed_Test()
        {
            MetadataTable::SPtr metadataTableSPtr;
            MetadataTable::Create(GetAllocator(), metadataTableSPtr);
            RecoveryStoreComponent<int, ULONG32>::SPtr recoveryStoreComponentSPtr;
            CreateRecoveryStoreComponent(*metadataTableSPtr, recoveryStoreComponentSPtr);
            co_await recoveryStoreComponentSPtr->RecoverAsync(CancellationToken::None);
            co_await metadataTableSPtr->CloseAsync();
            co_return;
        }

        ktl::Awaitable<void> Recover_MetadataTableOneEntry_OneKey_ShouldSucceed_Test()
        {
            KString::SPtr checkpointFileName;
            KString::Create(checkpointFileName, GetAllocator(), L"Recover_MetadataTableOneEntry_OneKey_ShouldSucceed_Checkpoint.txt");

            // Create checkpoint file and immediately close since recovery will re-open it
            auto checkpointFileSPtr = co_await CreateCheckpointFileAsync(*checkpointFileName, 1, 1);
            co_await checkpointFileSPtr->CloseAsync();

            MetadataTable::SPtr metadataTableSPtr;
            MetadataTable::Create(GetAllocator(), metadataTableSPtr);

            FileMetadata::SPtr fileMetadataSPtr;
            FileMetadata::Create(1, *checkpointFileName, 1, 1, 1, 0, true, GetAllocator(), *CreateTraceComponent(), fileMetadataSPtr);

            MetadataManager::AddFile(*(metadataTableSPtr->Table), 1, *fileMetadataSPtr);

            RecoveryStoreComponent<int, ULONG32>::SPtr recoveryStoreComponentSPtr;
            CreateRecoveryStoreComponent(*metadataTableSPtr, recoveryStoreComponentSPtr);

            co_await recoveryStoreComponentSPtr->RecoverAsync(CancellationToken::None);

            co_await metadataTableSPtr->CloseAsync();
            co_return;
        }

        ktl::Awaitable<void> Recover_MetadataTableOneEntry_1000Keys_ShouldSucceed_Test()
        {
            KString::SPtr checkpointFileName;
            KString::Create(checkpointFileName, GetAllocator(), L"Recover_MetadataTableOneEntry_1000Keys_ShouldSucceed_Checkpoint.txt");

            // Create checkpoint file and immediately close since recovery will re-open it
            auto checkpointFileSPtr = co_await CreateCheckpointFileAsync(*checkpointFileName, 1, 1000);
            co_await checkpointFileSPtr->CloseAsync();

            MetadataTable::SPtr metadataTableSPtr;
            MetadataTable::Create(GetAllocator(), metadataTableSPtr);

            FileMetadata::SPtr fileMetadataSPtr;
            FileMetadata::Create(1, *checkpointFileName, 1000, 1000, 1, 0, true, GetAllocator(), *CreateTraceComponent(), fileMetadataSPtr);

            MetadataManager::AddFile(*(metadataTableSPtr->Table), 1, *fileMetadataSPtr);

            RecoveryStoreComponent<int, ULONG32>::SPtr recoveryStoreComponentSPtr;
            CreateRecoveryStoreComponent(*metadataTableSPtr, recoveryStoreComponentSPtr);

            co_await recoveryStoreComponentSPtr->RecoverAsync(CancellationToken::None);

            co_await metadataTableSPtr->CloseAsync();
            co_return;
        }

        ktl::Awaitable<void> Recover_MetadataTableTwoEntries_SecondEntryReplacesFirst_ShouldSucceed_Test()
        {
            KString::SPtr checkpoint1FileName;
            KString::Create(checkpoint1FileName, GetAllocator(), L"Recover_MetadataTableTwoEntries_SecondEntryReplacesFirst_ShouldSucceed_1");

            KString::SPtr checkpoint2FileName;
            KString::Create(checkpoint2FileName, GetAllocator(), L"Recover_MetadataTableTwoEntries_SecondEntryReplacesFirst_ShouldSucceed_2");

            // Create checkpoint file and immediately close since recovery will re-open it
            auto checkpointFile1SPtr = co_await CreateCheckpointFileAsync(*checkpoint1FileName, 1, 100, 0, 0);
            co_await checkpointFile1SPtr->CloseAsync();
            auto checkpointFile2SPtr = co_await CreateCheckpointFileAsync(*checkpoint2FileName, 2, 100, 0, 100); // Same keys, newer sequence numbers
            co_await checkpointFile2SPtr->CloseAsync();

            MetadataTable::SPtr metadataTableSPtr;
            MetadataTable::Create(GetAllocator(), metadataTableSPtr);

            FileMetadata::SPtr file1MetadataSPtr;
            FileMetadata::Create(1, *checkpoint1FileName, 100, 100, 1, 0, true, GetAllocator(), *CreateTraceComponent(), file1MetadataSPtr);

            FileMetadata::SPtr file2MetadataSPtr;
            FileMetadata::Create(2, *checkpoint2FileName, 100, 100, 2, 0, true, GetAllocator(), *CreateTraceComponent(), file2MetadataSPtr);

            MetadataManager::AddFile(*(metadataTableSPtr->Table), 1, *file1MetadataSPtr);
            MetadataManager::AddFile(*(metadataTableSPtr->Table), 2, *file2MetadataSPtr);

            RecoveryStoreComponent<int, ULONG32>::SPtr recoveryStoreComponentSPtr;
            CreateRecoveryStoreComponent(*metadataTableSPtr, recoveryStoreComponentSPtr);

            co_await recoveryStoreComponentSPtr->RecoverAsync(CancellationToken::None);

            co_await metadataTableSPtr->CloseAsync();
            co_return;
        }

        ktl::Awaitable<void> Recover_MetadataTableTwoEntries_SecondEntryDoesNotReplaceFirst_ShouldSucceed_Test()
        {
            KString::SPtr checkpoint1FileName;
            KString::Create(checkpoint1FileName, GetAllocator(), L"Recover_MetadataTableTwoEntries_SecondEntryDoesNotReplaceFirst_ShouldSucceed_1");

            KString::SPtr checkpoint2FileName;
            KString::Create(checkpoint2FileName, GetAllocator(), L"Recover_MetadataTableTwoEntries_SecondEntryDoesNotReplaceFirst_ShouldSucceed_2");

            // Create checkpoint file and immediately close since recovery will re-open it
            auto checkpointFile1SPtr = co_await CreateCheckpointFileAsync(*checkpoint1FileName, 1, 100, 0, 100);
            co_await checkpointFile1SPtr->CloseAsync();
            auto checkpointFile2SPtr = co_await CreateCheckpointFileAsync(*checkpoint2FileName, 2, 100, 0, 0); // Same keys, older sequence numbers
            co_await checkpointFile2SPtr->CloseAsync();

            MetadataTable::SPtr metadataTableSPtr;
            MetadataTable::Create(GetAllocator(), metadataTableSPtr);

            FileMetadata::SPtr file1MetadataSPtr;
            FileMetadata::Create(1, *checkpoint1FileName, 100, 100, 1, 0, true, GetAllocator(), *CreateTraceComponent(), file1MetadataSPtr);

            FileMetadata::SPtr file2MetadataSPtr;
            FileMetadata::Create(2, *checkpoint2FileName, 100, 100, 2, 0, true, GetAllocator(), *CreateTraceComponent(), file2MetadataSPtr);

            MetadataManager::AddFile(*(metadataTableSPtr->Table), 1, *file1MetadataSPtr);
            MetadataManager::AddFile(*(metadataTableSPtr->Table), 2, *file2MetadataSPtr);

            RecoveryStoreComponent<int, ULONG32>::SPtr recoveryStoreComponentSPtr;
            CreateRecoveryStoreComponent(*metadataTableSPtr, recoveryStoreComponentSPtr);

            co_await recoveryStoreComponentSPtr->RecoverAsync(CancellationToken::None);

            co_await metadataTableSPtr->CloseAsync();
            co_return;
        }

        ktl::Awaitable<void> Enumerate_MetadataTableSingleEntry_100Keys_ShouldSucceed_Test()
        {
            KString::SPtr checkpointFileName;
            KString::Create(checkpointFileName, GetAllocator(), L"Enumerate_MetadataTableSingleEntry_ShouldSucceed");

            // Create checkpoint file and immediately close since recovery will re-open it
            auto checkpointFileSPtr = co_await CreateCheckpointFileAsync(*checkpointFileName, 1, 100);
            co_await checkpointFileSPtr->CloseAsync();

            MetadataTable::SPtr metadataTableSPtr;
            MetadataTable::Create(GetAllocator(), metadataTableSPtr);

            FileMetadata::SPtr fileMetadataSPtr;
            FileMetadata::Create(1, *checkpointFileName, 1, 1, 1, 0, true, GetAllocator(), *CreateTraceComponent(), fileMetadataSPtr);

            MetadataManager::AddFile(*(metadataTableSPtr->Table), 1, *fileMetadataSPtr);

            RecoveryStoreComponent<int, ULONG32>::SPtr recoveryStoreComponentSPtr;
            CreateRecoveryStoreComponent(*metadataTableSPtr, recoveryStoreComponentSPtr);

            co_await recoveryStoreComponentSPtr->RecoverAsync(CancellationToken::None);

            auto enumeratorSPtr = recoveryStoreComponentSPtr->GetEnumerable();

            ULONG32 count = 0;
            while (enumeratorSPtr->MoveNext())
            {
                auto current = enumeratorSPtr->Current();
                count++;
            }
            CODING_ERROR_ASSERT(count == 100);

            co_await metadataTableSPtr->CloseAsync();
            co_return;
        }
    #pragma endregion
    };

    BOOST_FIXTURE_TEST_SUITE(RecoveryStoreComponentTestSuite, RecoveryStoreComponentTest)

    BOOST_AUTO_TEST_CASE(Recover_EmptyMetadataTable_ShouldSucceed)
    {
        SyncAwait(Recover_EmptyMetadataTable_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Recover_MetadataTableOneEntry_OneKey_ShouldSucceed)
    {
        SyncAwait(Recover_MetadataTableOneEntry_OneKey_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Recover_MetadataTableOneEntry_1000Keys_ShouldSucceed)
    {
        SyncAwait(Recover_MetadataTableOneEntry_1000Keys_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Recover_MetadataTableTwoEntries_SecondEntryReplacesFirst_ShouldSucceed)
    {
        SyncAwait(Recover_MetadataTableTwoEntries_SecondEntryReplacesFirst_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Recover_MetadataTableTwoEntries_SecondEntryDoesNotReplaceFirst_ShouldSucceed)
    {
        SyncAwait(Recover_MetadataTableTwoEntries_SecondEntryDoesNotReplaceFirst_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Enumerate_MetadataTableSingleEntry_100Keys_ShouldSucceed)
    {
        SyncAwait(Enumerate_MetadataTableSingleEntry_100Keys_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_SUITE_END()
}

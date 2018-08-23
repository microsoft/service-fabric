// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#define ALLOC_TAG 'scTP'

namespace TStoreTests
{
    using namespace ktl;
    using namespace Data::Utilities;

    class SnapshotTest
    {
    public:
        SnapshotTest()
        {
            NTSTATUS status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);

            IntComparer::SPtr intComparerSPtr = nullptr;
            status = IntComparer::Create(GetAllocator(), intComparerSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            keyComparerSPtr_ = intComparerSPtr.DownCast<IComparer<int>>();

            TestStateSerializer<int>::SPtr valueSerializer = nullptr;
            status = TestStateSerializer<int>::Create(GetAllocator(), valueSerializer);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            valueSerializerSPtr_ = valueSerializer.DownCast<Data::StateManager::IStateSerializer<int>>();
        }
        
        ~SnapshotTest()
        {
            keyComparerSPtr_ = nullptr;
            valueSerializerSPtr_ = nullptr;
            ktlSystem_->Shutdown();
        }

        KAllocator& GetAllocator()
        {
            return ktlSystem_->NonPagedAllocator();
        }

        VersionedItem<int>::SPtr CreateVersionedItem(__in LONG64 sequenceNumber, __in int value)
        {
            InsertedVersionedItem<int>::SPtr insertedItem = nullptr;
            auto status = InsertedVersionedItem<int>::Create(GetAllocator(), insertedItem);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            insertedItem->InitializeOnApply(sequenceNumber, value);

            return insertedItem.DownCast<VersionedItem<int>>();
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

        FileMetadata::SPtr CreateFileMetadata(ULONG32 fileId)
        {
            FileMetadata::SPtr resultSPtr;
            KString::SPtr filepathSPtr;
            KString::Create(filepathSPtr, GetAllocator(), L"test_path.txt"); // Not actually creating files, so it doesn't matter
            FileMetadata::Create(fileId, *filepathSPtr, 1, 1, 1, 0, true, GetAllocator(), *CreateTraceComponent(), resultSPtr);
            return resultSPtr;
        }

        SnapshotContainer<int, int>::SPtr CreateSnapshotContainer()
        {
            SnapshotContainer<int, int>::SPtr snapshotContainerSPtr = nullptr;
            SnapshotContainer<int, int>::Create(*valueSerializerSPtr_, *CreateTraceComponent(), GetAllocator(), snapshotContainerSPtr);
            return snapshotContainerSPtr;
        }

        SnapshotComponent<int, int>::SPtr CreateSnapshotComponent(__in SnapshotContainer<int, int>::SPtr snapshotContainerSPtr)
        {
            SnapshotComponent<int, int>::SPtr snapshotComponentSPtr = nullptr;
            auto status = SnapshotComponent<int, int>::Create(*snapshotContainerSPtr, false, *keyComparerSPtr_, *CreateTraceComponent(), GetAllocator(), snapshotComponentSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            return snapshotComponentSPtr;
        }

        ktl::Awaitable<void> DictionaryRemoveWithRandomPauses(__in Dictionary<LONG64, LONG64> & dict, __in LONG64 numItems)
        {
            KSharedPtr<Dictionary<LONG64, LONG64>> dictSPtr = &dict;

            auto seed = Common::Stopwatch::Now().Ticks;
            Common::Random random(static_cast<int>(seed));

            for (LONG64 i = random.Next(20); i < numItems; i += random.Next(1,20))
            {
                dictSPtr->Remove(i);
                int timeout = random.Next(0, 1000);
                co_await KTimer::StartTimerAsync(GetAllocator(), ALLOC_TAG, static_cast<ULONG>(timeout), nullptr);
            }
        }

        ktl::Awaitable<void> DictionaryEnumerateWithRandomPauses(__in Dictionary<LONG64, LONG64> & dict, __in LONG64 numItems)
        {
            KSharedPtr<Dictionary<LONG64, LONG64>> dictSPtr = &dict;

            auto seed = Common::Stopwatch::Now().Ticks;
            Common::Random random(static_cast<int>(seed));

            auto enumeratorSPtr = dict.GetEnumerator();

            while (enumeratorSPtr->MoveNext())
            {
                auto current = enumeratorSPtr->Current();
                CODING_ERROR_ASSERT(current.Key == current.Value);
                int timeout = random.Next(0, 120);
                co_await KTimer::StartTimerAsync(GetAllocator(), ALLOC_TAG, static_cast<ULONG>(timeout), nullptr);
            }
        }

    private:
        KtlSystem* ktlSystem_;
        IComparer<int>::SPtr keyComparerSPtr_;
        Data::StateManager::IStateSerializer<int>::SPtr valueSerializerSPtr_;

#pragma region test functions
    public:
        ktl::Awaitable<void> SnapshotComponent_AddSingleVersion_ShouldSucceed_Test()
        {
            auto snapshotContainerSPtr = CreateSnapshotContainer();
            auto snapshotComponentSPtr = CreateSnapshotComponent(snapshotContainerSPtr);

            int key = 3;
            int value = 5;

            VersionedItem<int>::SPtr item = CreateVersionedItem(1, value);

            snapshotComponentSPtr->Add(key, *item);

            StoreComponentReadResult<int>::SPtr result = co_await snapshotComponentSPtr->ReadAsync(key, 1, ReadMode::CacheResult);
            CODING_ERROR_ASSERT(result->Value == value);

            co_await snapshotContainerSPtr->CloseAsync();
            co_return;
        }

        ktl::Awaitable<void> SnapshotComponent_UpdateWithNewerVersion_ShouldSucceed_Test()
        {
            auto snapshotContainerSPtr = CreateSnapshotContainer();
            auto snapshotComponentSPtr = CreateSnapshotComponent(snapshotContainerSPtr);

            int key = 3;

            int oldValue = 5;
            int newValue = 6;

            int oldVersion = 1;
            int newVersion = 2;

            VersionedItem<int>::SPtr oldItem = CreateVersionedItem(oldVersion, oldValue);
            VersionedItem<int>::SPtr newItem = CreateVersionedItem(newVersion, newValue);

            snapshotComponentSPtr->Add(key, *oldItem);
            snapshotComponentSPtr->Add(key, *newItem);

            StoreComponentReadResult<int>::SPtr result = co_await snapshotComponentSPtr->ReadAsync(key, newVersion, ReadMode::CacheResult);
            CODING_ERROR_ASSERT(result->Value == newValue);

            co_await snapshotContainerSPtr->CloseAsync();
            co_return;
        }

        ktl::Awaitable<void> SnapshotComponent_UpdateWithOlderVersion_ShouldSucceed_Test()
        {
            auto snapshotContainerSPtr = CreateSnapshotContainer();
            auto snapshotComponentSPtr = CreateSnapshotComponent(snapshotContainerSPtr);

            int key = 3;

            int oldValue = 5;
            int newValue = 6;

            int oldVersion = 1;
            int newVersion = 2;

            VersionedItem<int>::SPtr oldItem = CreateVersionedItem(oldVersion, oldValue);
            VersionedItem<int>::SPtr newItem = CreateVersionedItem(newVersion, newValue);

            snapshotComponentSPtr->Add(key, *newItem);
            snapshotComponentSPtr->Add(key, *oldItem);

            StoreComponentReadResult<int>::SPtr result = co_await snapshotComponentSPtr->ReadAsync(key, newVersion, ReadMode::CacheResult);
            CODING_ERROR_ASSERT(result->Value == newValue);

            co_await snapshotContainerSPtr->CloseAsync();
            co_return;
        }

        ktl::Awaitable<void> SnapshotComponent_GetOlderVersion_ShouldSucceed_Test()
        {
            auto snapshotContainerSPtr = CreateSnapshotContainer();
            auto snapshotComponentSPtr = CreateSnapshotComponent(snapshotContainerSPtr);

            int key = 3;
            int newValue = 6;

            int oldVersion = 1;
            int newVersion = 2;

            VersionedItem<int>::SPtr newItem = CreateVersionedItem(newVersion, newValue);

            snapshotComponentSPtr->Add(key, *newItem);

            StoreComponentReadResult<int>::SPtr result = co_await snapshotComponentSPtr->ReadAsync(key, oldVersion, ReadMode::CacheResult);
            CODING_ERROR_ASSERT(result->VersionedItem == nullptr);

            co_await snapshotContainerSPtr->CloseAsync();
            co_return;
        }

        ktl::Awaitable<void> SnapshotComponent_GetEnumerable_ShouldSucceed_Test()
        {
            auto snapshotContainerSPtr = CreateSnapshotContainer();
            auto snapshotComponentSPtr = CreateSnapshotComponent(snapshotContainerSPtr);

            int key = 3;
            int value = 6;
            VersionedItem<int>::SPtr item = CreateVersionedItem(1, value);
        
            snapshotComponentSPtr->Add(key, *item);

            auto enumeratorSPtr = snapshotComponentSPtr->GetEnumerable();
            bool result = enumeratorSPtr->MoveNext();
            CODING_ERROR_ASSERT(result);

            int currentKey = enumeratorSPtr->Current();
            CODING_ERROR_ASSERT(currentKey == key);

            result = enumeratorSPtr->MoveNext();
            CODING_ERROR_ASSERT(!result);

            co_await snapshotContainerSPtr->CloseAsync();
            co_return;
        }

        ktl::Awaitable<void> SnapshotContainer_AddReadSingleComponent_ShouldSucceed_Test()
        {
            auto snapshotContainerSPtr = CreateSnapshotContainer();
            auto snapshotComponentSPtr = CreateSnapshotComponent(snapshotContainerSPtr);

            int key = 3;
            int value = 6;
            VersionedItem<int>::SPtr item = CreateVersionedItem(1, value);
            snapshotComponentSPtr->Add(key, *item);
        
            LONG64 lsn = 1;
            SnapshotComponent<int, int>::SPtr componentSPtr = snapshotContainerSPtr->GetOrAdd(lsn, *snapshotComponentSPtr);
            CODING_ERROR_ASSERT(componentSPtr != nullptr);

            auto readResult = co_await componentSPtr->ReadAsync(key, 1, ReadMode::CacheResult);
            CODING_ERROR_ASSERT(readResult->Value == 6);

            componentSPtr = snapshotContainerSPtr->Read(lsn);
            CODING_ERROR_ASSERT(componentSPtr != nullptr);

            readResult = co_await componentSPtr->ReadAsync(key, 1, ReadMode::CacheResult);
            CODING_ERROR_ASSERT(readResult->Value == 6);

            co_await snapshotContainerSPtr->CloseAsync();
            co_return;
        }

        ktl::Awaitable<void> SnapshotContainer_AddFileMetadata_MultipleTimes_ShouldSucceed_Test()
        {
            auto snapshotContainerSPtr = CreateSnapshotContainer();

            LONG64 lsn = 1;
        
            ULONG32 fileId = 1;
            FileMetadata::SPtr metadataSPtr = CreateFileMetadata(fileId);
        
            auto previousRefCount = metadataSPtr->ReferenceCount;
            auto result = snapshotContainerSPtr->TryAddFileMetadata(lsn, *metadataSPtr);
            CODING_ERROR_ASSERT(result == true);
            CODING_ERROR_ASSERT(metadataSPtr->ReferenceCount == previousRefCount + 1);
        
            // Adding it again should not change anything
            previousRefCount = metadataSPtr->ReferenceCount;
            result = snapshotContainerSPtr->TryAddFileMetadata(lsn, *metadataSPtr);
            CODING_ERROR_ASSERT(result == false);
            CODING_ERROR_ASSERT(metadataSPtr->ReferenceCount == previousRefCount);

            co_await snapshotContainerSPtr->CloseAsync();
            co_return;
        }

        ktl::Awaitable<void> SnapshotContainer_Remove_ShouldSucceed_Test()
        {
            LONG64 lsn1 = 1;
            LONG64 lsn2 = 2;

            auto snapshotContainerSPtr = CreateSnapshotContainer();

            int snapshotKey = 3;
            int snapshotValue = 6;

            auto snapshotComponent1SPtr = CreateSnapshotComponent(snapshotContainerSPtr);
            auto snapshotComponent2SPtr = CreateSnapshotComponent(snapshotContainerSPtr);

            VersionedItem<int>::SPtr item1 = CreateVersionedItem(1, snapshotValue);
            snapshotComponent1SPtr->Add(snapshotKey, *item1);

            VersionedItem<int>::SPtr item2 = CreateVersionedItem(2, snapshotValue);
            snapshotComponent2SPtr->Add(snapshotKey, *item2);

            snapshotContainerSPtr->GetOrAdd(lsn1, *snapshotComponent1SPtr);
            snapshotContainerSPtr->GetOrAdd(lsn2, *snapshotComponent2SPtr);

            // File metadata for LSN 1
            FileMetadata::SPtr fileMetadata1SPtr = CreateFileMetadata(1);
            snapshotContainerSPtr->TryAddFileMetadata(lsn1, *fileMetadata1SPtr);

            // File metadata for LSN 2
            FileMetadata::SPtr fileMetadata2SPtr = CreateFileMetadata(2);
            FileMetadata::SPtr fileMetadata3SPtr = CreateFileMetadata(3);
            snapshotContainerSPtr->TryAddFileMetadata(lsn2, *fileMetadata2SPtr);
            snapshotContainerSPtr->TryAddFileMetadata(lsn2, *fileMetadata3SPtr);

            CODING_ERROR_ASSERT(snapshotContainerSPtr->GetCount() == 2);

            auto previousRefCount1 = fileMetadata1SPtr->ReferenceCount;
            auto previousRefCount2 = fileMetadata2SPtr->ReferenceCount;
            auto previousRefCount3 = fileMetadata3SPtr->ReferenceCount;

            co_await snapshotContainerSPtr->RemoveAsync(lsn2);

            CODING_ERROR_ASSERT(snapshotContainerSPtr->GetCount() == 1);

            CODING_ERROR_ASSERT(fileMetadata1SPtr->ReferenceCount == previousRefCount1);
            CODING_ERROR_ASSERT(fileMetadata2SPtr->ReferenceCount == previousRefCount2 - 1);
            CODING_ERROR_ASSERT(fileMetadata3SPtr->ReferenceCount == previousRefCount3 - 1);

            CODING_ERROR_ASSERT(snapshotContainerSPtr->Read(lsn1) != nullptr);
            CODING_ERROR_ASSERT(snapshotContainerSPtr->Read(lsn2) == nullptr);
        
            co_await snapshotContainerSPtr->CloseAsync();
            co_return;
        }
    #pragma endregion
    };

    BOOST_FIXTURE_TEST_SUITE(SnapshotTestSuite, SnapshotTest)

    BOOST_AUTO_TEST_CASE(SnapshotComponent_AddSingleVersion_ShouldSucceed)
    {
        SyncAwait(SnapshotComponent_AddSingleVersion_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(SnapshotComponent_UpdateWithNewerVersion_ShouldSucceed)
    {
        SyncAwait(SnapshotComponent_UpdateWithNewerVersion_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(SnapshotComponent_UpdateWithOlderVersion_ShouldSucceed)
    {
        SyncAwait(SnapshotComponent_UpdateWithOlderVersion_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(SnapshotComponent_GetOlderVersion_ShouldSucceed)
    {
        SyncAwait(SnapshotComponent_GetOlderVersion_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(SnapshotComponent_GetEnumerable_ShouldSucceed)
    {
        SyncAwait(SnapshotComponent_GetEnumerable_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(SnapshotContainer_AddReadSingleComponent_ShouldSucceed)
    {
        SyncAwait(SnapshotContainer_AddReadSingleComponent_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(SnapshotContainer_AddFileMetadata_MultipleTimes_ShouldSucceed)
    {
        SyncAwait(SnapshotContainer_AddFileMetadata_MultipleTimes_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(SnapshotContainer_Remove_ShouldSucceed)
    {
        SyncAwait(SnapshotContainer_Remove_ShouldSucceed_Test());
    }

    // TODO: Remove this test
    //BOOST_AUTO_TEST_CASE(Dictionary_EnumerateWithConcurrentRemoves_ShouldSucceed)
    //{
    //    LongComparer::SPtr longComparerSPtr;
    //    LongComparer::Create(GetAllocator(), longComparerSPtr);

    //    Dictionary<LONG64, LONG64>::SPtr dictSPtr = nullptr;
    //    Dictionary<LONG64, LONG64>::Create(32, K_DefaultHashFunction, *longComparerSPtr, GetAllocator(), dictSPtr);

    //    LONG64 numItems = 1000;

    //    for (LONG64 key = 0; key < numItems; key++)
    //    {
    //        dictSPtr->Add(key, key);
    //    }

    //    auto removeTask = DictionaryRemoveWithRandomPauses(*dictSPtr, numItems);
    //    auto enumerateTask = DictionaryEnumerateWithRandomPauses(*dictSPtr, numItems);

    //    SyncAwait(removeTask);
    //    SyncAwait(enumerateTask);
    //}

    // TODO: Tests that require values to be loaded from disk

    BOOST_AUTO_TEST_SUITE_END()
}

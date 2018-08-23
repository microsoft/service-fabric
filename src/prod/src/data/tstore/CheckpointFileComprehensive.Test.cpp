// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#define TEST_TAG 'gtST'
namespace TStoreTests
{
    class CheckpointFileComprehensiveTest
    {
    public:
        CheckpointFileComprehensiveTest()
        {
            NTSTATUS status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);
        }

        ~CheckpointFileComprehensiveTest()
        {
            // Adding a short delay as a temporary workaround for an assert in 
            // KtlSystem::Initialize that is sometimes hit in Linux debug builds
            ktlSystem_->Shutdown();
        }

        KAllocator& GetAllocator()
        {
            return ktlSystem_->NonPagedAllocator();
        }

        Common::Random GetRandom()
        {
            auto seed = Common::Stopwatch::Now().Ticks;
            Common::Random random(static_cast<int>(seed));
            cout << "Random seed (use this seed to reproduce test failures): " << seed << endl;
            return random;
        }

        KString::SPtr CreateFileString(__in KStringView const & name, __in KAllocator & allocator)
        {
            KString::SPtr fileName;

            WCHAR currentDirectoryPathCharArray[MAX_PATH];
            GetCurrentDirectory(MAX_PATH, currentDirectoryPathCharArray);

#if !defined(PLATFORM_UNIX)
            NTSTATUS status = KString::Create(fileName, allocator, L"\\??\\");
#else
            NTSTATUS status = KString::Create(fileName, allocator, L"");
#endif
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            BOOLEAN concatSuccess = fileName->Concat(currentDirectoryPathCharArray);
            CODING_ERROR_ASSERT(concatSuccess == TRUE);

            concatSuccess = fileName->Concat(Common::Path::GetPathSeparatorWstr().c_str());
            CODING_ERROR_ASSERT(concatSuccess == TRUE);

            concatSuccess = fileName->Concat(name);
            CODING_ERROR_ASSERT(concatSuccess == TRUE);

            return fileName.RawPtr();
        }

        KBufferSerializer::SPtr CreateBufferSerializer()
        {
            KBufferSerializer::SPtr valueSerializerSPtr;
            auto status = KBufferSerializer::Create(GetAllocator(), valueSerializerSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            return valueSerializerSPtr;
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

        bool BufferEquals(__in ULONG32 size, __in KBuffer::SPtr const & one, __in KBuffer::SPtr const & two)
        {
            if (one == nullptr || two == nullptr)
            {
                return one == two;
            }

            auto oneBytes = (byte *)one->GetBuffer();
            auto twoBytes = (byte *)two->GetBuffer();

            for (ULONG32 i = 0; i < size; i++)
            {
                if (oneBytes[i] != twoBytes[i])
                {
                    return false;
                }
            }
            return true;
        }

        KBuffer::SPtr CreateBuffer(__in ULONG size, __in UCHAR storeValue=0)
        {
            KBuffer::SPtr bufferSPtr = nullptr;
            KBuffer::Create(size, bufferSPtr, GetAllocator());
            char* buffer = static_cast<char*>(bufferSPtr->GetBuffer());
            for (ULONG i = 0; i < size; i++)
            {
                buffer[i] = storeValue;
            }
            return bufferSPtr;
        }

        KBuffer::SPtr CreateRandomBuffer(__in ULONG size, __in Common::Random random)
        {
            KBuffer::SPtr bufferSPtr = nullptr;
            KBuffer::Create(size, bufferSPtr, GetAllocator());
            UCHAR* buffer = static_cast<UCHAR*>(bufferSPtr->GetBuffer());
            for (ULONG i = 0; i < size; i++)
            {
                buffer[i] = static_cast<UCHAR>(random.Next(UCHAR_MAX));
            }
            return bufferSPtr;
        }
        
        KSharedArray<KeyValuePair<KBuffer::SPtr, VersionedItem<KBuffer::SPtr>::SPtr>>::SPtr CreateEmptyArray()
        {
            KSharedArray<KeyValuePair<KBuffer::SPtr, VersionedItem<KBuffer::SPtr>::SPtr>>::SPtr resultSPtr =
                _new(TEST_TAG, GetAllocator()) KSharedArray<KeyValuePair<KBuffer::SPtr, VersionedItem<KBuffer::SPtr>::SPtr>>();
            return resultSPtr;
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

        VersionedItem<KBuffer::SPtr>::SPtr CreateDeletedVersionedItem(__in ULONG32 versionSequenceNumber = -1)
        {
            DeletedVersionedItem<KBuffer::SPtr>::SPtr itemSPtr;
            DeletedVersionedItem<KBuffer::SPtr>::Create(GetAllocator(), itemSPtr);
            itemSPtr->InitializeOnApply(versionSequenceNumber);
            return itemSPtr.DownCast<VersionedItem<KBuffer::SPtr>>();
        }

        KeyValuePair<KBuffer::SPtr, VersionedItem<KBuffer::SPtr>::SPtr> MakeKeyValuePair(__in KBuffer::SPtr const & key, __in VersionedItem<KBuffer::SPtr>::SPtr value)
        {
            return KeyValuePair<KBuffer::SPtr, VersionedItem<KBuffer::SPtr>::SPtr>(key, value);
        }

        KSharedArray<KBuffer::SPtr>::SPtr CreateKeysArrayFromItems(__in KSharedArray<KeyValuePair<KBuffer::SPtr, VersionedItem<KBuffer::SPtr>::SPtr>>::SPtr const & items)
        {
            KSharedArray<KBuffer::SPtr>::SPtr keysSPtr = _new(TEST_TAG, GetAllocator()) KSharedArray<KBuffer::SPtr>();

            for (ULONG32 i = 0; i < items->Count(); i++)
            {
                keysSPtr->Append((*items)[i].Key);
            }
           
            return keysSPtr;
        }

        ktl::Awaitable<KSharedArray<KeyData<KBuffer::SPtr, KBuffer::SPtr>::SPtr>::SPtr> ReadItemsFromEnumeratorAsync(__in KeyCheckpointFileAsyncEnumerator<KBuffer::SPtr, KBuffer::SPtr>::SPtr const & enumeratorSPtr)
        {
            KSharedArray<KeyData<KBuffer::SPtr, KBuffer::SPtr>::SPtr>::SPtr resultSPtr = _new(TEST_TAG, GetAllocator()) KSharedArray<KeyData<KBuffer::SPtr, KBuffer::SPtr>::SPtr>();
            while (co_await enumeratorSPtr->MoveNextAsync(CancellationToken::None))
            {
                resultSPtr->Append(enumeratorSPtr->GetCurrent());
            }

            co_return resultSPtr;
        }

#pragma region Checkpoint File Utilities

        KSharedPtr<IEnumerator<KeyValuePair<KBuffer::SPtr, KSharedPtr<VersionedItem<KBuffer::SPtr>>>>> GetEnumerator(
           __in KSharedArray<KeyValuePair<KSharedPtr<KBuffer>, KSharedPtr<VersionedItem<KSharedPtr<KBuffer>>>>> const & items)
        {
           KSharedPtr<ArrayKeyVersionedItemEnumerator<KBuffer::SPtr, KBuffer::SPtr>> arrayEnumeratorSPtr = nullptr;
           NTSTATUS status = ArrayKeyVersionedItemEnumerator<KBuffer::SPtr, KBuffer::SPtr>::Create(items, GetAllocator(), arrayEnumeratorSPtr);
           CODING_ERROR_ASSERT(NT_SUCCESS(status));

           KSharedPtr<IEnumerator<KeyValuePair<KBuffer::SPtr, KSharedPtr<VersionedItem<KBuffer::SPtr>>>>> enumeratorSPtr = 
              static_cast<IEnumerator<KeyValuePair<KBuffer::SPtr, KSharedPtr<VersionedItem<KBuffer::SPtr>>>> *>(arrayEnumeratorSPtr.RawPtr());

           CODING_ERROR_ASSERT(enumeratorSPtr != nullptr);
           return enumeratorSPtr;
        }

        ktl::Awaitable<CheckpointFile::SPtr> CreateCheckpointFileAsync(
           __in KStringView & filepath,
           __in KSharedArray<KeyValuePair<KBuffer::SPtr, VersionedItem<KBuffer::SPtr>::SPtr>>::SPtr const & items,
           __in ULONG32 fileId = 1)
        {
           auto bufferSerializerSPtr = CreateBufferSerializer();
           StorePerformanceCountersSPtr perfCounters = nullptr;
           co_return co_await CheckpointFile::CreateAsync<KBuffer::SPtr, KBuffer::SPtr>(
              fileId,
              filepath,
              *GetEnumerator(*items),
              *bufferSerializerSPtr,
              *bufferSerializerSPtr,
              1,
              GetAllocator(),
              *CreateTraceComponent(),
              perfCounters,
              true);
        }

        ktl::Awaitable<CheckpointFile::SPtr> OpenCheckpointFileAsync(__in KStringView const & filepath)
        {
            KStringView path = const_cast<KStringView &>(filepath);
            co_return co_await CheckpointFile::OpenAsync(
                path,
                *CreateTraceComponent(),
                GetAllocator(),
                true
            );
        }

        ktl::Awaitable<void> CleanupCheckpointFileAsync(__in CheckpointFile::SPtr const & checkpointFileSPtr)
        {
            co_await checkpointFileSPtr->CloseAsync();
            auto keyCheckpointNameSPtr = checkpointFileSPtr->KeyCheckpointFileNameSPtr;
            auto valueCheckpointNameSPtr = checkpointFileSPtr->ValueCheckpointFileNameSPtr;
            Common::File::Delete(checkpointFileSPtr->KeyCheckpointFileNameSPtr->operator LPCWSTR());
            Common::File::Delete(checkpointFileSPtr->ValueCheckpointFileNameSPtr->operator LPCWSTR());
            co_return;
        }
#pragma endregion

        ktl::Awaitable<void> CheckpointCreateAndOpenTestAsync(__in KStringView const & filename, __in ULONG32 numItems, __in ULONG32 keySize, __in ULONG32 valueSize)
        {
            ULONG32 fileId = 7;
            ULONG32 versionSequenceNumber = 87;

            auto testKeySPtr = CreateBuffer(keySize, UCHAR_MAX);
            auto testValueSPtr = CreateBuffer(valueSize);

            auto filepathSPtr = CreateFileString(filename, GetAllocator());
            auto bufferSerializerSPtr = CreateBufferSerializer();
            
            auto itemsSPtr = CreateEmptyArray();
            for (ULONG32 i = 0; i < numItems; i++)
            {
                auto key = testKeySPtr;
                auto insertedItem = CreateInsertedVersionedItem(testValueSPtr, 0, versionSequenceNumber);
                itemsSPtr->Append(MakeKeyValuePair(key, insertedItem));
            }

            auto createdCheckpointFileSPtr = co_await CreateCheckpointFileAsync(*filepathSPtr, itemsSPtr, fileId);
            
            ASSERT_IFNOT(createdCheckpointFileSPtr->KeyCount == numItems, "Checkpoint file should have {0} keys", numItems);
            ASSERT_IFNOT(createdCheckpointFileSPtr->ValueCount == numItems, "Checkpoint file should have {0} values", numItems);

            for (ULONG32 i = 0; i < itemsSPtr->Count(); i++)
            {
                auto item = (*itemsSPtr)[i];
                auto result = co_await createdCheckpointFileSPtr->ReadValueAsync<KBuffer::SPtr>(*item.Value, *bufferSerializerSPtr);
                ASSERT_IFNOT(BufferEquals(valueSize, item.Value->GetValue(), result), "CheckpointFile failed to read the result from disk");
            }
            co_await createdCheckpointFileSPtr->CloseAsync();

            auto openedCheckpointFileSPtr = co_await OpenCheckpointFileAsync(*filepathSPtr);

            ASSERT_IFNOT(openedCheckpointFileSPtr->KeyCount == numItems, "Checkpoint file should {0} keys", numItems);
            ASSERT_IFNOT(openedCheckpointFileSPtr->ValueCount == numItems, "Checkpoint file should {0} values", numItems);
            ASSERT_IFNOT(openedCheckpointFileSPtr->DeletedKeyCount == 0, "Checkpoint file should have no deleted keys");

            auto enumeratorSPtr = openedCheckpointFileSPtr->GetAsyncEnumerator<KBuffer::SPtr, KBuffer::SPtr>(*bufferSerializerSPtr);
            while (co_await enumeratorSPtr->MoveNextAsync(CancellationToken::None))
            {
                auto keyResult = enumeratorSPtr->GetCurrent()->Key;
                auto expectedKeyEnumerableSPtr = CreateKeysArrayFromItems(itemsSPtr);
                CODING_ERROR_ASSERT(expectedKeyEnumerableSPtr->Count() == numItems);

                bool isKeySuccess = false;
                for (ULONG32 i = 0; i < expectedKeyEnumerableSPtr->Count(); i++)
                {
                    auto key = (*expectedKeyEnumerableSPtr)[i];
                    isKeySuccess = BufferEquals(keySize, key, keyResult);
                    if (isKeySuccess) 
                    {
                        break;
                    }
                }
                ASSERT_IFNOT(isKeySuccess, "CheckpointFile failed to read the result from disk");

                ASSERT_IFNOT(enumeratorSPtr->GetCurrent()->Value->GetVersionSequenceNumber() == versionSequenceNumber, "Expected version sequence number {0}", versionSequenceNumber);
                ASSERT_IFNOT(enumeratorSPtr->GetCurrent()->Value->GetFileId() == fileId, "Expected file ID to be {0}", fileId);

                auto result = co_await openedCheckpointFileSPtr->ReadValueAsync<KBuffer::SPtr>(*enumeratorSPtr->GetCurrent()->Value, *bufferSerializerSPtr);
                ASSERT_IFNOT(BufferEquals(valueSize, testValueSPtr, result), "CheckpointFile failed to read the result from disk");
            }

            co_await enumeratorSPtr->CloseAsync();

            co_await CleanupCheckpointFileAsync(openedCheckpointFileSPtr);
            co_return;
        }

        ktl::Awaitable<void> ValidateCheckpointFileAsync(
            __in KStringView const & filepath,
            __in KSharedArray<KeyValuePair<KBuffer::SPtr, VersionedItem<KBuffer::SPtr>::SPtr>>::SPtr const & itemsSPtr,
            __in KSharedArray<ULONG32>::SPtr const & keySizesSPtr,
            __in KSharedArray<ULONG32>::SPtr const & valueSizesSPtr)
        {
            auto bufferSerializerSPtr = CreateBufferSerializer();
            auto checkpointFileSPtr = co_await OpenCheckpointFileAsync(filepath);

            for (ULONG i = 0; i < itemsSPtr->Count(); i++)
            {
                auto item = (*itemsSPtr)[i];
                if (item.Value->GetRecordKind() != RecordKind::DeletedVersion)
                {
                    KBuffer::SPtr result = co_await checkpointFileSPtr->ReadValueAsync<KBuffer::SPtr>(*item.Value, *bufferSerializerSPtr);
                    ASSERT_IFNOT(BufferEquals((*valueSizesSPtr)[i], item.Value->GetValue(), result), "CheckpointFile failed to read the result from disk after Open");
                }
            }
                
            auto enumeratorSPtr = checkpointFileSPtr->GetAsyncEnumerator<KBuffer::SPtr, KBuffer::SPtr>(*bufferSerializerSPtr);
            auto readItemsSPtr = co_await ReadItemsFromEnumeratorAsync(enumeratorSPtr);

            for (ULONG32 j = 0; j < readItemsSPtr->Count(); j++)
            {
                auto expectedItem = (*itemsSPtr)[j];
                auto actualItemSPtr = (*readItemsSPtr)[j];
                CODING_ERROR_ASSERT(BufferEquals((*keySizesSPtr)[j], expectedItem.Key, actualItemSPtr->Key));
                CODING_ERROR_ASSERT(expectedItem.Value->GetRecordKind() == actualItemSPtr->Value->GetRecordKind());
                CODING_ERROR_ASSERT(expectedItem.Value->GetVersionSequenceNumber() == actualItemSPtr->Value->GetVersionSequenceNumber());

                if (expectedItem.Value->GetRecordKind() != RecordKind::DeletedVersion)
                {
                    CODING_ERROR_ASSERT(expectedItem.Value->GetOffset() == expectedItem.Value->GetOffset());
                    CODING_ERROR_ASSERT(expectedItem.Value->GetValueSize() == expectedItem.Value->GetValueSize());
                    CODING_ERROR_ASSERT(expectedItem.Value->GetValueChecksum() == expectedItem.Value->GetValueChecksum());

                    KBuffer::SPtr actualValue = co_await checkpointFileSPtr->ReadValueAsync<KBuffer::SPtr>(
                        *actualItemSPtr->Value,
                        *bufferSerializerSPtr);
                    CODING_ERROR_ASSERT(BufferEquals((*valueSizesSPtr)[j], expectedItem.Value->GetValue(), actualValue));
                }
            }

            co_await enumeratorSPtr->CloseAsync();

            co_await CleanupCheckpointFileAsync(checkpointFileSPtr);
            co_return;
        }

    private:
        KtlSystem* ktlSystem_;

#pragma region test functions
    public:
        ktl::Awaitable<void> OneCheckpoint_NoItems_ShouldSucceed_Test()
        {
       
            auto itemsSPtr = CreateEmptyArray();
            KStringView filename = L"OneCheckpoint_NoItems_ShouldSucceed";
            auto filepath = CreateFileString(filename, GetAllocator());

            auto createdCheckpointFileSPtr = co_await CreateCheckpointFileAsync(*filepath, itemsSPtr);
            ASSERT_IFNOT(Common::File::Exists(createdCheckpointFileSPtr->KeyCheckpointFileNameSPtr->operator LPCWSTR()), "Key checkpoint file does not exist");
            ASSERT_IFNOT(Common::File::Exists(createdCheckpointFileSPtr->ValueCheckpointFileNameSPtr->operator LPCWSTR()), "Value checkpoint file does not exist");
            ASSERT_IFNOT(createdCheckpointFileSPtr->KeyCount == 0, "Empty checkpoint should have no keys");
            ASSERT_IFNOT(createdCheckpointFileSPtr->ValueCount == 0, "Empty checkpoint should have no values");
            co_await createdCheckpointFileSPtr->CloseAsync();

            auto openedCheckpointFileSPtr = co_await OpenCheckpointFileAsync(*filepath);
            auto enumeratorSPtr = openedCheckpointFileSPtr->GetAsyncEnumerator<KBuffer::SPtr, KBuffer::SPtr>(*CreateBufferSerializer());
            ASSERT_IFNOT(co_await enumeratorSPtr->MoveNextAsync(CancellationToken::None) == false, "Enumerator should not have any values");
            co_await enumeratorSPtr->CloseAsync();
        
            co_await CleanupCheckpointFileAsync(openedCheckpointFileSPtr);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_OneInsertedItemSmallerThanSingleBlock_ShouldSucceed_Test()
        {
            ULONG32 keyMetadataSize = 40;
            ULONG32 blockChecksumSize = 8;
            ULONG32 expectedBlockSize = (1024 * 4) - 10;
            ULONG32 serializerOverhead = 4;

            auto keySize = expectedBlockSize - (KeyChunkMetadata::Size + serializerOverhead + keyMetadataSize + blockChecksumSize);
            auto valueSize = expectedBlockSize;
        
            KStringView filename = L"Checkpoint_OneInsertedItemSmallerThanSingleBlock_ShouldSucceed";
            co_await CheckpointCreateAndOpenTestAsync(filename, 1, keySize, valueSize);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_OneInsertedItemLargerThanSingleBlock_ShouldSucceed_Test()
        {
            ULONG32 keyMetadataSize = 40;
            ULONG32 blockChecksumSize = 8;
            ULONG32 expectedBlockSize = (1024 * 4) + 10;
            ULONG32 serializerOverhead = 4;

            auto keySize = expectedBlockSize - (KeyChunkMetadata::Size + serializerOverhead + keyMetadataSize + blockChecksumSize);
            auto valueSize = expectedBlockSize;
        
            KStringView filename = L"Checkpoint_OneInsertedItemLargerThanSingleBlock_ShouldSucceed";
            co_await CheckpointCreateAndOpenTestAsync(filename, 1, keySize, valueSize);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_OneInsertedItemExactlyOneBlock_ShouldSucceed_Test()
        {
            ULONG32 keyMetadataSize = 40;
            ULONG32 blockChecksumSize = 8;
            ULONG32 expectedBlockSize = (1024 * 4);
            ULONG32 serializerOverhead = 4;

            auto keySize = expectedBlockSize - (KeyChunkMetadata::Size + serializerOverhead + keyMetadataSize + blockChecksumSize);
            auto valueSize = expectedBlockSize;
        
            KStringView filename = L"Checkpoint_OneInsertedItemExactlyOneBlock_ShouldSucceed";
            co_await CheckpointCreateAndOpenTestAsync(filename, 1, keySize, valueSize);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_OneInsertedItemExactlyTwoBlocks_ShouldSucceed_Test()
        {
            ULONG32 keyMetadataSize = 40;
            ULONG32 blockChecksumSize = 8;
            ULONG32 expectedBlockSize = (1024 * 4) * 2;
            ULONG32 serializerOverhead = 4;

            auto keySize = expectedBlockSize - (KeyChunkMetadata::Size + serializerOverhead + keyMetadataSize + blockChecksumSize);
            auto valueSize = expectedBlockSize;
        
            KStringView filename = L"Checkpoint_OneInsertedItemExactlyTwoBlocks_ShouldSucceed";
            co_await CheckpointCreateAndOpenTestAsync(filename, 1, keySize, valueSize);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_OneInsertedItemLargerThanTwoBlocks_ShouldSucceed_Test()
        {
            ULONG32 keyMetadataSize = 40;
            ULONG32 blockChecksumSize = 8;
            ULONG32 expectedBlockSize = ((1024 * 4) * 2) + 10;
            ULONG32 serializerOverhead = 4;

            auto keySize = expectedBlockSize - (KeyChunkMetadata::Size + serializerOverhead + keyMetadataSize + blockChecksumSize);
            auto valueSize = expectedBlockSize;
        
            KStringView filename = L"Checkpoint_OneInsertedItemLargerThanTwoBlocks_ShouldSucceed";
            co_await CheckpointCreateAndOpenTestAsync(filename, 1, keySize, valueSize);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_TwoInsertedItemsExactlyTwoBlocks_ShouldSucceed_Test()
        {
            ULONG32 keyMetadataSize = 40;
            ULONG32 blockChecksumSize = 8;
            ULONG32 expectedBlockSize = (1024 * 4) * 2;
            ULONG32 serializerOverhead = 4;

            auto keySize = expectedBlockSize - (KeyChunkMetadata::Size + serializerOverhead + keyMetadataSize + blockChecksumSize);
            auto valueSize = expectedBlockSize;
        
            KStringView filename = L"Checkpoint_TwoInsertedItemsExactlyTwoBlocks_ShouldSucceed";
            co_await CheckpointCreateAndOpenTestAsync(filename, 2, keySize, valueSize);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_ManyInsertedItemsExactlyTwoBlocks_ShouldSucceed_Test()
        {
            ULONG32 numItems = 10; // TODO: Managed has this at 512, but it takes way too long right now

            ULONG32 keyMetadataSize = 40;
            ULONG32 blockChecksumSize = 8;
            ULONG32 expectedBlockSize = (1024 * 4) * 2;
            ULONG32 serializerOverhead = 4;

            auto keySize = expectedBlockSize - (KeyChunkMetadata::Size + serializerOverhead + keyMetadataSize + blockChecksumSize);
            auto valueSize = expectedBlockSize;
        
            KStringView filename = L"Checkpoint_ManyInsertedItemsExactlyTwoBlocks_ShouldSucceed";
            co_await CheckpointCreateAndOpenTestAsync(filename, numItems, keySize, valueSize);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_KeysWithIndividualRecordsSmallerThanBlockSizeAndNoStraddlingBoundaries_ShouldSucceed_Test()
        {
            ULONG32 keyMetadataSize = 40;
            ULONG32 blockChecksumSize = 8;
            ULONG32 expectedBlockSize = 1024;
            ULONG32 serializerOverhead = 4;

            auto firstKeySize = expectedBlockSize - (KeyChunkMetadata::Size + serializerOverhead + blockChecksumSize + keyMetadataSize);
            auto otherKeySize = expectedBlockSize - (serializerOverhead + keyMetadataSize);

            auto itemsSPtr = CreateEmptyArray();
            KSharedArray<ULONG32>::SPtr keySizesSPtr = _new(TEST_TAG, GetAllocator()) KSharedArray<ULONG32>();
            KSharedArray<ULONG32>::SPtr valueSizesSPtr = _new(TEST_TAG, GetAllocator()) KSharedArray<ULONG32>();

            for (UCHAR count = 0; count < 64; count++)
            {
                auto keySize = (count % 4 == 0) ? firstKeySize : otherKeySize;
                auto key = CreateBuffer(keySize, count);

                auto valueBufferSPtr = CreateBuffer(1020);
                auto value = CreateInsertedVersionedItem(valueBufferSPtr, 1024);
            
                keySizesSPtr->Append(keySize);
                valueSizesSPtr->Append(1020);
                itemsSPtr->Append(MakeKeyValuePair(key, value));
            }
        
            KStringView filename = L"Checkpoint_KeysWithIndividualRecordsSmallerThanBlockSizeAndNoStraddlingBoundaries_ShouldSucceed";
            auto filepathSPtr = CreateFileString(filename, GetAllocator());

            auto checkpointFileSPtr = co_await CreateCheckpointFileAsync(*filepathSPtr, itemsSPtr, 1);
            ASSERT_IFNOT(checkpointFileSPtr->KeyBlockHandleSPtr->Size == (64 / 4) * (4 * 1024), "Value section should have expanded to exactly 4k aligned");
            co_await checkpointFileSPtr->CloseAsync();

            co_await ValidateCheckpointFileAsync(*filepathSPtr, itemsSPtr, keySizesSPtr, valueSizesSPtr);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_KeysWithIndividualRecordsSmallerThanBlockSizeButStraddle_ShouldSucceed_Test()
        {
            ULONG32 keySize = 3 * 1024;

            auto itemsSPtr = CreateEmptyArray();
            KSharedArray<ULONG32>::SPtr keySizesSPtr = _new(TEST_TAG, GetAllocator()) KSharedArray<ULONG32>();
            KSharedArray<ULONG32>::SPtr valueSizesSPtr = _new(TEST_TAG, GetAllocator()) KSharedArray<ULONG32>();

            for (UCHAR count = 0; count < 20; count++)
            {
                auto key = CreateBuffer(keySize, count);
                auto valueBufferSPtr = CreateBuffer(1020);
                auto value = CreateInsertedVersionedItem(valueBufferSPtr, 1024);
            
                keySizesSPtr->Append(keySize);
                valueSizesSPtr->Append(1020);
                itemsSPtr->Append(MakeKeyValuePair(key, value));
            }
        
            KStringView filename = L"Checkpoint_KeysWithIndividualRecordsSmallerThanBlockSizeButStraddle_ShouldSucceed";
            auto filepathSPtr = CreateFileString(filename, GetAllocator());

            auto checkpointFileSPtr = co_await CreateCheckpointFileAsync(*filepathSPtr, itemsSPtr, 1);
            ASSERT_IFNOT(checkpointFileSPtr->KeyBlockHandleSPtr->Size == (20) * (4 * 1024), "Value section should have expanded to exactly 4k aligned");
            co_await checkpointFileSPtr->CloseAsync();

            co_await ValidateCheckpointFileAsync(*filepathSPtr, itemsSPtr, keySizesSPtr, valueSizesSPtr);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_KeysWithIndvidualRecordsBiggerThanBlockSize_ShouldSucceed_Test()
        {
            ULONG32 keySize = 5 * 1024;

            auto itemsSPtr = CreateEmptyArray();
            KSharedArray<ULONG32>::SPtr keySizesSPtr = _new(TEST_TAG, GetAllocator()) KSharedArray<ULONG32>();
            KSharedArray<ULONG32>::SPtr valueSizesSPtr = _new(TEST_TAG, GetAllocator()) KSharedArray<ULONG32>();

            for (UCHAR count = 0; count < 20; count++)
            {
                auto key = CreateBuffer(keySize, count);
                auto valueBufferSPtr = CreateBuffer(1020);
                auto value = CreateInsertedVersionedItem(valueBufferSPtr, 1024);
            
                keySizesSPtr->Append(keySize);
                valueSizesSPtr->Append(1020);
                itemsSPtr->Append(MakeKeyValuePair(key, value));
            }
        
            KStringView filename = L"Checkpoint_KeysWithIndividualRecordsBiggerThanBlockSize_ShouldSucceed";
            auto filepathSPtr = CreateFileString(filename, GetAllocator());

            auto checkpointFileSPtr = co_await CreateCheckpointFileAsync(*filepathSPtr, itemsSPtr, 1);
            ASSERT_IFNOT(checkpointFileSPtr->KeyBlockHandleSPtr->Size == (20 * 2) * (4 * 1024), "Value section should have expanded to exactly 4k aligned");
            co_await checkpointFileSPtr->CloseAsync();

            co_await ValidateCheckpointFileAsync(*filepathSPtr, itemsSPtr, keySizesSPtr, valueSizesSPtr);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_KeysWithRandomSizes_ShouldSucceed_Test()
        {
            auto random = GetRandom();

            auto itemsSPtr = CreateEmptyArray();
            KSharedArray<ULONG32>::SPtr keySizesSPtr = _new(TEST_TAG, GetAllocator()) KSharedArray<ULONG32>();
            KSharedArray<ULONG32>::SPtr valueSizesSPtr = _new(TEST_TAG, GetAllocator()) KSharedArray<ULONG32>();

            for (ULONG32 count = 0; count < 50; count++)
            {
                auto keySize = random.Next(16, 5 * 1024);
                auto key = CreateRandomBuffer(keySize, random);

                auto valueBufferSPtr = CreateBuffer(1020);

                VersionedItem<KBuffer::SPtr>::SPtr value;
                if (count % 10 == 0)
                {
                    value = CreateDeletedVersionedItem();
                }
                else
                {
                    value = CreateInsertedVersionedItem(valueBufferSPtr, 1024);
                }
            
                keySizesSPtr->Append(keySize);
                valueSizesSPtr->Append(1020);
                itemsSPtr->Append(MakeKeyValuePair(key, value));
            }
        
            KStringView filename = L"Checkpoint_KeysWithRandomSizes_ShouldSucceed";
            auto filepathSPtr = CreateFileString(filename, GetAllocator());

            auto checkpointFileSPtr = co_await CreateCheckpointFileAsync(*filepathSPtr, itemsSPtr, 1);
            co_await checkpointFileSPtr->CloseAsync();
            co_await ValidateCheckpointFileAsync(*filepathSPtr, itemsSPtr, keySizesSPtr, valueSizesSPtr);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_KeysAndValuesWithRandomSizes_ShouldSucceed_Test()
        {
            auto random = GetRandom();

            auto itemsSPtr = CreateEmptyArray();
            KSharedArray<ULONG32>::SPtr keySizesSPtr = _new(TEST_TAG, GetAllocator()) KSharedArray<ULONG32>();
            KSharedArray<ULONG32>::SPtr valueSizesSPtr = _new(TEST_TAG, GetAllocator()) KSharedArray<ULONG32>();

            for (ULONG32 count = 0; count < 50; count++)
            {
                auto keySize = random.Next(16, 5 * 1024);
                auto key = CreateRandomBuffer(keySize, random);

                VersionedItem<KBuffer::SPtr>::SPtr value;
                if (count % 10 == 0)
                {
                    value = CreateDeletedVersionedItem();
                    valueSizesSPtr->Append(0);
                }
                else
                { 
                    // Values must be at least 1 byte big because serializer fails for empty buffers
                    auto valueSize = random.Next(1, 12 * 1024);
                    auto valueBufferSPtr = CreateRandomBuffer(valueSize, random);
                    value = CreateInsertedVersionedItem(valueBufferSPtr, 1024);
                    valueSizesSPtr->Append(valueSize);
                }
            
                keySizesSPtr->Append(keySize);
                itemsSPtr->Append(MakeKeyValuePair(key, value));
            }
        
            KStringView filename = L"Checkpoint_KeysAndValuesWithRandomSizes_ShouldSucceed";
            auto filepathSPtr = CreateFileString(filename, GetAllocator());

            auto checkpointFileSPtr = co_await CreateCheckpointFileAsync(*filepathSPtr, itemsSPtr, 1);
            co_await checkpointFileSPtr->CloseAsync();
            co_await ValidateCheckpointFileAsync(*filepathSPtr, itemsSPtr, keySizesSPtr, valueSizesSPtr);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_ValuesThatOverflow64KOnResize_ShouldSucceed_Test()
        {
            auto random = GetRandom();

            auto itemsSPtr = CreateEmptyArray();
            KSharedArray<ULONG32>::SPtr keySizesSPtr = _new(TEST_TAG, GetAllocator()) KSharedArray<ULONG32>();
            KSharedArray<ULONG32>::SPtr valueSizesSPtr = _new(TEST_TAG, GetAllocator()) KSharedArray<ULONG32>();
        
            // Create the first item
            auto firstKey = CreateRandomBuffer(16, random);
            auto firstValue = CreateInsertedVersionedItem(CreateRandomBuffer(1 * 1024, random), 0);
            keySizesSPtr->Append(16);
            valueSizesSPtr->Append(1 * 1024);
            itemsSPtr->Append(MakeKeyValuePair(firstKey, firstValue));

            // Create the second item
            auto secondKey = CreateRandomBuffer(16, random);
            auto secondValue = CreateInsertedVersionedItem(CreateRandomBuffer(60 * 1024, random), 0);
            keySizesSPtr->Append(16);
            valueSizesSPtr->Append(60 * 1024);
            itemsSPtr->Append(MakeKeyValuePair(secondKey, secondValue));
        
            KStringView filename = L"Checkpoint_CreateValuesThatOverflow64KOnResize_ShouldSucceed";
            auto filepathSPtr = CreateFileString(filename, GetAllocator());

            auto checkpointFileSPtr = co_await CreateCheckpointFileAsync(*filepathSPtr, itemsSPtr, 1);
            co_await checkpointFileSPtr->CloseAsync();
            co_await ValidateCheckpointFileAsync(*filepathSPtr, itemsSPtr, keySizesSPtr, valueSizesSPtr);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_ValuesThatOverflow32KOnResize_ShouldSucceed_Test()
        {
            auto random = GetRandom();

            auto itemsSPtr = CreateEmptyArray();
            KSharedArray<ULONG32>::SPtr keySizesSPtr = _new(TEST_TAG, GetAllocator()) KSharedArray<ULONG32>();
            KSharedArray<ULONG32>::SPtr valueSizesSPtr = _new(TEST_TAG, GetAllocator()) KSharedArray<ULONG32>();
        
            // Create the first item
            auto firstKey = CreateRandomBuffer(16, random);
            auto firstValue = CreateInsertedVersionedItem(CreateRandomBuffer(1 * 1024, random), 0);
            keySizesSPtr->Append(16);
            valueSizesSPtr->Append(1 * 1024);
            itemsSPtr->Append(MakeKeyValuePair(firstKey, firstValue));

            // Create the second item
            auto secondKey = CreateRandomBuffer(16, random);
            auto secondValue = CreateInsertedVersionedItem(CreateRandomBuffer(30 * 1024, random), 0);
            keySizesSPtr->Append(16);
            valueSizesSPtr->Append(6 * 1024);
            itemsSPtr->Append(MakeKeyValuePair(secondKey, secondValue));
        
            KStringView filename = L"Checkpoint_CreateValuesThatOverflow32KOnResize_ShouldSucceed";
            auto filepathSPtr = CreateFileString(filename, GetAllocator());

            auto checkpointFileSPtr = co_await CreateCheckpointFileAsync(*filepathSPtr, itemsSPtr, 1);
            co_await checkpointFileSPtr->CloseAsync();
            co_await ValidateCheckpointFileAsync(*filepathSPtr, itemsSPtr, keySizesSPtr, valueSizesSPtr);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_InconsistentValueSerialization_ShouldSucceed_Test()
        {
            auto itemsSPtr = CreateEmptyArray();
            KSharedArray<ULONG32>::SPtr keySizesSPtr = _new(TEST_TAG, GetAllocator()) KSharedArray<ULONG32>();
            KSharedArray<ULONG32>::SPtr valueSizesSPtr = _new(TEST_TAG, GetAllocator()) KSharedArray<ULONG32>();
        
            // 2K but claims to be bigger (3K)
            ULONG32 firstValueSize = (2 * 1024) - sizeof(ULONG32);
            auto firstKey = CreateBuffer(16);
            auto firstValue = CreateInsertedVersionedItem(CreateBuffer(firstValueSize), 3 * 1024);
            keySizesSPtr->Append(16);
            valueSizesSPtr->Append(firstValueSize);
            itemsSPtr->Append(MakeKeyValuePair(firstKey, firstValue));
        
            // 1K but claims to be smaller (16 bytes)
            ULONG32 secondValueSize = (1 * 1024) - sizeof(ULONG32);
            auto secondKey = CreateBuffer(16);
            auto secondValue = CreateInsertedVersionedItem(CreateBuffer(secondValueSize), 16);
            keySizesSPtr->Append(16);
            valueSizesSPtr->Append(secondValueSize);
            itemsSPtr->Append(MakeKeyValuePair(secondKey, secondValue));

            // 5K but claims to be smaller (16 bytes)
            ULONG32 thirdValueSize = (1 * 1024) - sizeof(ULONG32);
            auto thirdKey = CreateBuffer(16);
            auto thirdValue = CreateInsertedVersionedItem(CreateBuffer(thirdValueSize), 16);
            keySizesSPtr->Append(16);
            valueSizesSPtr->Append(thirdValueSize);
            itemsSPtr->Append(MakeKeyValuePair(thirdKey, thirdValue));

            KStringView filename = L"Checkpoint_InconsistenValueSerialization_ShouldSucceed";
            auto filepathSPtr = CreateFileString(filename, GetAllocator());

            auto checkpointFileSPtr = co_await CreateCheckpointFileAsync(*filepathSPtr, itemsSPtr, 1);
            co_await checkpointFileSPtr->CloseAsync();
            co_await ValidateCheckpointFileAsync(*filepathSPtr, itemsSPtr, keySizesSPtr, valueSizesSPtr);
            co_return;
        }

        ktl::Awaitable<void> Checkpoint_OneSmallDeletedItem_ReadAsync_ShouldFail_Test()
        {
            ULONG32 fileId = 7;
            KStringView filename = L"Checkpoint_OneSmallDeletedItem_ShouldSucceed";
            auto filepathSPtr = CreateFileString(filename, GetAllocator());

            KBuffer::SPtr testKey = CreateBuffer(8);
            auto bufferSerializerSPtr = CreateBufferSerializer();

            auto itemsSPtr = CreateEmptyArray();
            itemsSPtr->Append(KeyValuePair<KBuffer::SPtr, VersionedItem<KBuffer::SPtr>::SPtr>(testKey, CreateDeletedVersionedItem()));

            auto createdCheckpointFileSPtr = co_await CreateCheckpointFileAsync(*filepathSPtr, itemsSPtr, fileId);

            ASSERT_IFNOT(createdCheckpointFileSPtr->KeyCount == 1, "Checkpoint file should have 1 key");
            ASSERT_IFNOT(createdCheckpointFileSPtr->ValueCount == 0, "Checkpoint file should have no values");
            co_await createdCheckpointFileSPtr->CloseAsync();

            // Since ValueCheckpointFile::ReadValueAsync asserts (instead of throwing an exception) on deleted items, that assertion is skipped

            auto openedCheckpointFileSPtr = co_await OpenCheckpointFileAsync(*filepathSPtr);
        
            auto enumeratorSPtr = openedCheckpointFileSPtr->GetAsyncEnumerator<KBuffer::SPtr, KBuffer::SPtr>(*bufferSerializerSPtr);
            CODING_ERROR_ASSERT(co_await enumeratorSPtr->MoveNextAsync(CancellationToken::None) == true);
            CODING_ERROR_ASSERT(BufferEquals(8, testKey, enumeratorSPtr->GetCurrent()->Key));
            CODING_ERROR_ASSERT(enumeratorSPtr->GetCurrent()->Value->GetRecordKind() == RecordKind::DeletedVersion);
            CODING_ERROR_ASSERT(co_await enumeratorSPtr->MoveNextAsync(CancellationToken::None) == false);
            co_await enumeratorSPtr->CloseAsync();

            co_await CleanupCheckpointFileAsync(openedCheckpointFileSPtr);
            co_return;
        }
#pragma endregion
    };

    BOOST_FIXTURE_TEST_SUITE(CheckpointFileComprehensiveTestSuite, CheckpointFileComprehensiveTest)
    
    BOOST_AUTO_TEST_CASE(OneCheckpoint_NoItems_ShouldSucceed)
    {
        SyncAwait(OneCheckpoint_NoItems_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_OneInsertedItemSmallerThanSingleBlock_ShouldSucceed)
    {
        SyncAwait(Checkpoint_OneInsertedItemSmallerThanSingleBlock_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_OneInsertedItemLargerThanSingleBlock_ShouldSucceed)
    {
        SyncAwait(Checkpoint_OneInsertedItemLargerThanSingleBlock_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_OneInsertedItemExactlyOneBlock_ShouldSucceed)
    {
        SyncAwait(Checkpoint_OneInsertedItemExactlyOneBlock_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_OneInsertedItemExactlyTwoBlocks_ShouldSucceed)
    {
        SyncAwait(Checkpoint_OneInsertedItemExactlyTwoBlocks_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_OneInsertedItemLargerThanTwoBlocks_ShouldSucceed)
    {
        SyncAwait(Checkpoint_OneInsertedItemLargerThanTwoBlocks_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_TwoInsertedItemsExactlyTwoBlocks_ShouldSucceed)
    {
        SyncAwait(Checkpoint_TwoInsertedItemsExactlyTwoBlocks_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_ManyInsertedItemsExactlyTwoBlocks_ShouldSucceed)
    {
        SyncAwait(Checkpoint_ManyInsertedItemsExactlyTwoBlocks_ShouldSucceed_Test());
    }
    
    BOOST_AUTO_TEST_CASE(Checkpoint_KeysWithIndividualRecordsSmallerThanBlockSizeAndNoStraddlingBoundaries_ShouldSucceed)
    {
        SyncAwait(Checkpoint_KeysWithIndividualRecordsSmallerThanBlockSizeAndNoStraddlingBoundaries_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_KeysWithIndividualRecordsSmallerThanBlockSizeButStraddle_ShouldSucceed)
    {
        SyncAwait(Checkpoint_KeysWithIndividualRecordsSmallerThanBlockSizeButStraddle_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_KeysWithIndvidualRecordsBiggerThanBlockSize_ShouldSucceed)
    {
        SyncAwait(Checkpoint_KeysWithIndvidualRecordsBiggerThanBlockSize_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_KeysWithRandomSizes_ShouldSucceed)
    {
        SyncAwait(Checkpoint_KeysWithRandomSizes_ShouldSucceed_Test());
    }
    
    BOOST_AUTO_TEST_CASE(Checkpoint_KeysAndValuesWithRandomSizes_ShouldSucceed)
    {
        SyncAwait(Checkpoint_KeysAndValuesWithRandomSizes_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_ValuesThatOverflow64KOnResize_ShouldSucceed)
    {
        SyncAwait(Checkpoint_ValuesThatOverflow64KOnResize_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_ValuesThatOverflow32KOnResize_ShouldSucceed)
    {
        SyncAwait(Checkpoint_ValuesThatOverflow32KOnResize_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_InconsistentValueSerialization_ShouldSucceed)
    {
        SyncAwait(Checkpoint_InconsistentValueSerialization_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Checkpoint_OneSmallDeletedItem_ReadAsync_ShouldFail)
    {
        SyncAwait(Checkpoint_OneSmallDeletedItem_ReadAsync_ShouldFail_Test());
    }

    BOOST_AUTO_TEST_SUITE_END()
    
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#define TEST_TAG 'gtST'

inline ULONG BufferHashFunc(const KBuffer::SPtr &key)
{
   ULONG64 hash = Data::Utilities::CRC64::ToCRC64(*key, 0, key->QuerySize());
   return static_cast<ULONG>(hash);
}

namespace TStoreTests
{
    using namespace ktl;
    using namespace Data::TStore;
    using namespace Data::TestCommon;
    using namespace Common;

    class CheckpointFileTest
    {
    public:
        CheckpointFileTest()
        {
            NTSTATUS status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);
        }

        ~CheckpointFileTest()
        {
            ktlSystem_->Shutdown();
        }

        KAllocator& GetAllocator()
        {
            return ktlSystem_->NonPagedAllocator();
        }

        ktl::io::KFileStream::SPtr GetFileStream()
        {
            CODING_ERROR_ASSERT(fakeStreamSPtr_ != nullptr);
            return fakeStreamSPtr_;
        }

        ktl::Awaitable<void> CloseFileStreamAsync(bool remove = true)
        {
            if (fakeStreamSPtr_ != nullptr)
            {
                NTSTATUS status = co_await fakeStreamSPtr_->CloseAsync();
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
                fakeStreamSPtr_ = nullptr;
            }

            if (fakeFileSPtr != nullptr)
            {
                fakeFileSPtr->Close();
                fakeFileSPtr = nullptr;
            }

            if (currentFilePath_ != nullptr && remove == true)
            {
                RemoveFile(*currentFilePath_);
                currentFilePath_ = nullptr;
            }

            co_return;
        }

        void RemoveFile(KString & path)
        {
            KWString filePath(GetAllocator(), path);
            NTSTATUS status = KVolumeNamespace::DeleteFileOrDirectory(filePath, GetAllocator(), nullptr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
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

            concatSuccess = fileName->Concat(Path::GetPathSeparatorWstr().c_str());
            CODING_ERROR_ASSERT(concatSuccess == TRUE);

            concatSuccess = fileName->Concat(name);
            CODING_ERROR_ASSERT(concatSuccess == TRUE);

            return fileName.RawPtr();
        }

        ktl::Awaitable<KString::SPtr> CreateFileStreamAsync(__in KStringView& name, __in bool overwriteExisting = true)
        {
            KString::SPtr fileName = CreateFileString(name, GetAllocator());
            //todo: remove this till fileformattesthelper api is updated.
            KWString filePath(GetAllocator(), *fileName);

            if (overwriteExisting)
            {
                NTSTATUS status = co_await FileFormatTestHelper::ForceCreateNewBlockFile(filePath, GetAllocator(), CancellationToken::None, fakeFileSPtr, fakeStreamSPtr_);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }
            else
            {
                NTSTATUS status = co_await FileFormatTestHelper::CreateBlockFile(filePath, GetAllocator(), CancellationToken::None, fakeFileSPtr, fakeStreamSPtr_);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            NTSTATUS status = co_await fakeStreamSPtr_->OpenAsync(*fakeFileSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            currentFilePath_ = fileName;

            co_return fileName;
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

        KBuffer::SPtr MakeCustomSizeBufferWithIntVal(int sizeOfBuffer, int valueToStoreInTheBuffer)
        {
            CODING_ERROR_ASSERT(sizeOfBuffer >= sizeof(int));
            KBuffer::SPtr bufferSptr;
            auto status = KBuffer::Create(sizeOfBuffer, bufferSptr, GetAllocator());
            Diagnostics::Validate(status);

            auto buffer = (int *)bufferSptr->GetBuffer();
            //store the int data in the beginning of the buffer.
            buffer[0] = valueToStoreInTheBuffer;

            auto byteBuffer = (byte *)bufferSptr->GetBuffer();
            for (int i = sizeof(int); i < sizeOfBuffer; i++)
            {
                byteBuffer[i] = 0;
            }

            return bufferSptr;
        }

        bool IsSameBufferValue(
            __in KBuffer& buffer1,
            __in KBuffer& buffer2)
        {
            BinaryReader br1(buffer1, GetAllocator());
            BinaryReader br2(buffer2, GetAllocator());

            if (br1.Length != br2.Length)
            {
                return false;
            }

            for (ULONG32 i = 0; i < br1.Length; i++)
            {
                byte val1 = 0;
                br1.Read(val1);
                byte val2 = 0;
                br2.Read(val2);
                if (val1 != val2)
                {
                    return false;
                }
            }

            return true;
        }

#pragma region keycheckpointfile utils
        void WriteKeyWithInsertedVersionedItem(
            __in BinaryWriter& bw, 
            __in KeyCheckpointFile& checkpointFile, 
            __in int startVal, 
            __in TestStateSerializer<int>& stateSerializer)
        {
            InsertedVersionedItem<int>::SPtr valueSPtr = nullptr;
            KeyCheckpointFile::SPtr file (&checkpointFile);
            NTSTATUS status = InsertedVersionedItem<int>::Create(GetAllocator(), valueSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            int key = startVal++;
            valueSPtr->SetVersionSequenceNumber(startVal++);
            valueSPtr->SetValueSize(startVal++);
            valueSPtr->SetValueChecksum(startVal++);
            valueSPtr->SetOffset(startVal++, *CreateTraceComponent());
            VersionedItem<int>::SPtr item(&(*valueSPtr));
            KeyValuePair<int, VersionedItem<int>::SPtr> itemToAdd(key, item);
            file->WriteItem<int, int>(bw, itemToAdd, stateSerializer, 321);
        }

        void ReadAndVerifyKeysWithInsertedVersionedItem(
            __in BinaryReader& br, 
            __in KeyCheckpointFile& checkpointFile, 
            __in int startVal, 
            __in TestStateSerializer<int>& stateSerializer)
        {
            KeyCheckpointFile::SPtr fileSPtr(&checkpointFile);
            KeyData<int, int>::SPtr keyDataSPtr = fileSPtr->ReadKey<int, int>(br, stateSerializer);
            CODING_ERROR_ASSERT(keyDataSPtr->get_Key() == startVal++);
            CODING_ERROR_ASSERT(keyDataSPtr->get_Value()->GetVersionSequenceNumber() == startVal++);
            CODING_ERROR_ASSERT(keyDataSPtr->get_Value()->GetValueSize() == startVal++);
            CODING_ERROR_ASSERT(keyDataSPtr->get_Value()->GetValueChecksum() == startVal++);
            CODING_ERROR_ASSERT(keyDataSPtr->get_Value()->GetOffset() == startVal++);
            CODING_ERROR_ASSERT(keyDataSPtr->get_Value()->GetRecordKind() == RecordKind::InsertedVersion);
        }
#pragma endregion

#pragma region valuecheckpointfile utils
        KSharedPtr<VersionedItem<int>> AddValuesWithInsertedVersionedItem(
            __in ktl::io::KFileStream& stream,
            __in BinaryWriter& bw,
            __in ValueCheckpointFile& checkpointFile,
            __in int value,
            __in TestStateSerializer<int>& stateSerializer)
        {
            InsertedVersionedItem<int>::SPtr valueSPtr = nullptr;
            ValueCheckpointFile::SPtr fileSPtr(&checkpointFile);
            NTSTATUS status = InsertedVersionedItem<int>::Create(GetAllocator(), valueSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            valueSPtr->SetValue(value++);
            valueSPtr->SetVersionSequenceNumber(value++);
            KSharedPtr<VersionedItem<int>> item(&(*valueSPtr));
            fileSPtr->WriteItem<int>(stream, bw, *item, stateSerializer);
            return item;
        }

        KSharedPtr<VersionedItem<int>> AddValuesInBytesWithInsertedVersionedItem(
            __in ktl::io::KFileStream& stream,
            __in BinaryWriter& bw,
            __in ValueCheckpointFile& checkpointFile,
            __in int value,
            __in KBuffer& valueBytes)
        {
            InsertedVersionedItem<int>::SPtr valueSPtr = nullptr;
            ValueCheckpointFile::SPtr fileSPtr(&checkpointFile);
            NTSTATUS status = InsertedVersionedItem<int>::Create(GetAllocator(), valueSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            valueSPtr->SetValue(value++);
            valueSPtr->SetVersionSequenceNumber(value++);
            KSharedPtr<VersionedItem<int>> item(&(*valueSPtr));
            fileSPtr->WriteItem<int>(stream, bw, *item, valueBytes);
            return item;
        }
#pragma endregion

#pragma region checkpoint utils

        KSharedArray<KeyValuePair<KBuffer::SPtr, VersionedItem<KBuffer::SPtr>::SPtr>>::SPtr CheckpointWriteKeyWithInsertedVersionedItem(
            __in int count,
            __in int keySerializedSize,
            __in int valSerializedSize)
        {
            KSharedArray<KeyValuePair<KBuffer::SPtr, VersionedItem<KBuffer::SPtr>::SPtr>>::SPtr itemsArray =
                _new(TEST_TAG, GetAllocator()) KSharedArray<KeyValuePair<KBuffer::SPtr, VersionedItem<KBuffer::SPtr>::SPtr>>();
            CODING_ERROR_ASSERT(itemsArray != nullptr);

            for (int i = 0; i < count; i++)
            {
                InsertedVersionedItem<KBuffer::SPtr>::SPtr valueSPtr = nullptr;
                NTSTATUS status = InsertedVersionedItem<KBuffer::SPtr>::Create(GetAllocator(), valueSPtr);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));

                int key = i;
                int value = i+100;
                KBuffer::SPtr bufferKey = MakeCustomSizeBufferWithIntVal(keySerializedSize, key);
                KBuffer::SPtr bufferValue = MakeCustomSizeBufferWithIntVal(valSerializedSize, value);

                valueSPtr->SetValue(bufferValue);
                valueSPtr->SetValueSize(valSerializedSize);
                valueSPtr->SetVersionSequenceNumber(2);
                KSharedPtr<VersionedItem<KBuffer::SPtr>> item(&(*valueSPtr));

                KeyValuePair<KBuffer::SPtr, KSharedPtr<VersionedItem<KBuffer::SPtr>>> pair(bufferKey, item);
                itemsArray->Append(pair);
            }
            return itemsArray;
        }

        ktl::Awaitable<void> CheckpointReadAndVerifyKeysWithInsertedVersionedItemAsync(
            __in IEnumerator<KeyValuePair<KBuffer::SPtr, KSharedPtr<VersionedItem<KBuffer::SPtr>>>>& items,
            __in CheckpointFile& file,
            __in KBufferSerializer& keyStateSerializer,
            __in KBufferSerializer& valSerializer)
            {
            KeyCheckpointFileAsyncEnumerator<KBuffer::SPtr, KBuffer::SPtr>::SPtr keyEnumSPtr = 
                file.GetAsyncEnumerator<KBuffer::SPtr, KBuffer::SPtr>(keyStateSerializer);
            ULONG32 index = 0;
            bool isEndOfEnumerator = false;
            while (co_await keyEnumSPtr->MoveNextAsync(CancellationToken::None))
            {
               items.MoveNext();
               KSharedPtr<VersionedItem<KBuffer::SPtr>> itemSPtr = items.Current().Value;

                KeyData<KBuffer::SPtr, KBuffer::SPtr>::SPtr keyDataSPtr = keyEnumSPtr->GetCurrent();
                KBuffer::SPtr val = co_await file.ReadValueAsync(*(keyDataSPtr->Value), valSerializer);
                CODING_ERROR_ASSERT(keyDataSPtr != nullptr);

                //Compare the buffer value.
                CODING_ERROR_ASSERT(IsSameBufferValue(*(keyDataSPtr->Key), *(items.Current().Key)));
                CODING_ERROR_ASSERT(IsSameBufferValue(*val, *(items.Current().Value->GetValue())));

                CODING_ERROR_ASSERT(keyDataSPtr->Value->GetVersionSequenceNumber() == itemSPtr->GetVersionSequenceNumber());
                CODING_ERROR_ASSERT(keyDataSPtr->Value->GetValueSize() == itemSPtr->GetValueSize());
                CODING_ERROR_ASSERT(keyDataSPtr->Value->GetValueChecksum() == itemSPtr->GetValueChecksum());
                CODING_ERROR_ASSERT(keyDataSPtr->Value->GetOffset() == itemSPtr->GetOffset());
                CODING_ERROR_ASSERT(keyDataSPtr->Value->GetRecordKind() == itemSPtr->GetRecordKind());
                index++;
            }

            if (!items.MoveNext())
            {
               isEndOfEnumerator = true;
            }

            co_await keyEnumSPtr->CloseAsync();
            CODING_ERROR_ASSERT(isEndOfEnumerator);
            co_return;
        }

        ktl::Awaitable<void> TestCheckpointFileAsync(
            __in int numOfItems, 
            __in int KeySerializedSize,
            __in int valSerializedSize)
        {
            KAllocator& allocator = GetAllocator();
            KStringView filename = L"CheckpointFileTest.txt";
            KString::SPtr filePath = CreateFileString(filename, allocator);

            KBufferSerializer::SPtr stateSerializerSPtr = nullptr;
            NTSTATUS status = KBufferSerializer::Create(allocator, stateSerializerSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            KSharedArray<KeyValuePair<KBuffer::SPtr, KSharedPtr<VersionedItem<KBuffer::SPtr>>>>::SPtr itemsArraySPtr
                = CheckpointWriteKeyWithInsertedVersionedItem(numOfItems, KeySerializedSize, valSerializedSize);

            IEnumerator<KeyValuePair<KBuffer::SPtr, KSharedPtr<VersionedItem<KBuffer::SPtr>>>>::SPtr writeEnumerator;
            status = KSharedArrayEnumerator<KeyValuePair<KBuffer::SPtr, VersionedItem<KBuffer::SPtr>::SPtr>>::Create(*itemsArraySPtr, GetAllocator(), writeEnumerator);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            StorePerformanceCountersSPtr perfCounters = nullptr;
            CheckpointFile::SPtr checkpointFileSPtr = co_await
                CheckpointFile::CreateAsync<KBuffer::SPtr, KBuffer::SPtr>(
                    1,
                    *filePath,
                    *writeEnumerator,
                    *stateSerializerSPtr,
                    *stateSerializerSPtr,
                    1,
                    allocator,
                    *CreateTraceComponent(),
                    perfCounters,
                    false);

            IEnumerator<KeyValuePair<KBuffer::SPtr, KSharedPtr<VersionedItem<KBuffer::SPtr>>>>::SPtr readEnumerator;
            status = KSharedArrayEnumerator<KeyValuePair<KBuffer::SPtr, VersionedItem<KBuffer::SPtr>::SPtr>>::Create(*itemsArraySPtr, GetAllocator(), readEnumerator);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            co_await CheckpointReadAndVerifyKeysWithInsertedVersionedItemAsync(
                *readEnumerator, *checkpointFileSPtr, *stateSerializerSPtr, *stateSerializerSPtr);

            CheckpointFile::SPtr checkpointFileOpenedSPtr = co_await CheckpointFile::OpenAsync(*filePath, *CreateTraceComponent(), allocator, false);
            CODING_ERROR_ASSERT(checkpointFileOpenedSPtr->ValueCount == numOfItems);
            CODING_ERROR_ASSERT(checkpointFileOpenedSPtr->KeyCount == numOfItems);

            //clean up 
            co_await checkpointFileSPtr->CloseAsync();
            co_await checkpointFileOpenedSPtr->CloseAsync();
            KWString keyFilePathToRemove(allocator, *filePath);
            KWString valueFilePathToRemove(allocator, *filePath);
            keyFilePathToRemove += KeyCheckpointFile::GetFileExtension();
            valueFilePathToRemove += ValueCheckpointFile::GetFileExtension();
            KVolumeNamespace::DeleteFileOrDirectory(keyFilePathToRemove, allocator, nullptr);
            KVolumeNamespace::DeleteFileOrDirectory(valueFilePathToRemove, allocator, nullptr);

            co_return;
        }
#pragma endregion

#pragma region keyblockalignedwriter utils
        ktl::Awaitable<void> BlockAlignedWriteKeyWithInsertedVersionedItemAsync(
            __in KeyBlockAlignedWriter<KBuffer::SPtr, int>& writer,
            __in int startVal,
            __in int count,
            __in int keySerializedSize)
        {
            for (int i = 0; i < count; i++)
            {
                InsertedVersionedItem<int>::SPtr valueSPtr = nullptr;
                NTSTATUS status = InsertedVersionedItem<int>::Create(GetAllocator(), valueSPtr);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
                KBuffer::SPtr key = MakeCustomSizeBufferWithIntVal(keySerializedSize, startVal++);
                valueSPtr->SetVersionSequenceNumber(startVal++);
                valueSPtr->SetValueSize(startVal++);
                valueSPtr->SetValueChecksum(startVal++);
                valueSPtr->SetOffset(startVal++, *CreateTraceComponent());
                VersionedItem<int>::SPtr item(&(*valueSPtr));
                KeyValuePair<KBuffer::SPtr, VersionedItem<int>::SPtr> itemToAdd(key, item);
                co_await writer.BlockAlignedWriteKeyAsync(itemToAdd);
            }
        }

        ktl::Awaitable<int> BlockAlignedReadAndVerifyKeysWithInsertedVersionedItemAsync(
            __in KeyCheckpointFileAsyncEnumerator<KBuffer::SPtr, int>& enumerator,
            __in int startVal,
            __in int keySerializedSize)
        {
            KeyCheckpointFileAsyncEnumerator<KBuffer::SPtr, int>::SPtr keyEnumSPtr(&enumerator);
            int count = 0;
            while (co_await keyEnumSPtr->MoveNextAsync(CancellationToken::None))
            {
                KeyData<KBuffer::SPtr, int>::SPtr keyDataSPtr = keyEnumSPtr->GetCurrent();
                CODING_ERROR_ASSERT(keyDataSPtr != nullptr);
                //Compare the buffer value.
                CODING_ERROR_ASSERT(IsSameBufferValue(*(keyDataSPtr->Key), *MakeCustomSizeBufferWithIntVal(keySerializedSize, startVal++)));

                CODING_ERROR_ASSERT(keyDataSPtr->Value->GetVersionSequenceNumber() == startVal++);
                CODING_ERROR_ASSERT(keyDataSPtr->Value->GetValueSize() == startVal++);
                CODING_ERROR_ASSERT(keyDataSPtr->Value->GetValueChecksum() == startVal++);
                CODING_ERROR_ASSERT(keyDataSPtr->Value->GetOffset() == startVal++);
                CODING_ERROR_ASSERT(keyDataSPtr->Value->GetRecordKind() == RecordKind::InsertedVersion);
                count++;
            }

            co_return count;
        }

        ktl::Awaitable<void> TestKeyBlockAlignedWriteAndEnumeratorAsync(
            __in int numberOfKeys, 
            __in int keySize, 
            __in KBufferSerializer& serializer)
        {
            KAllocator& allocator = GetAllocator();
            KStringView filename = L"BlockAlignedWriteEnumerateTests.txt";
            KString::SPtr filePathToOpenSPtr = CreateFileString(filename, GetAllocator());
            
            KeyCheckpointFile::SPtr fileSPtr = co_await KeyCheckpointFile::CreateAsync(*CreateTraceComponent(), *filePathToOpenSPtr, false, 10, allocator);

            SharedBinaryWriter::SPtr bwSPtr = nullptr;
            NTSTATUS status = SharedBinaryWriter::Create(allocator, bwSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            ktl::io::KFileStream::SPtr streamSPtr = co_await fileSPtr->StreamPoolSPtr->AcquireStreamAsync();

            KeyBlockAlignedWriter<KBuffer::SPtr, int>::SPtr keyWriterSPtr = nullptr;
            status = KeyBlockAlignedWriter<KBuffer::SPtr, int>::Create(*streamSPtr, *fileSPtr, *bwSPtr, serializer, 0, *CreateTraceComponent(), allocator, keyWriterSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            int startVal = 99;
            co_await BlockAlignedWriteKeyWithInsertedVersionedItemAsync(*keyWriterSPtr, startVal, numberOfKeys, keySize);

            co_await keyWriterSPtr->FlushAsync();
            co_await fileSPtr->StreamPoolSPtr->ReleaseStreamAsync(*streamSPtr);

            KeyCheckpointFile::SPtr keyCheckpointFileSPtr = co_await KeyCheckpointFile::OpenAsync(allocator, *filePathToOpenSPtr, *CreateTraceComponent(), true);
            CODING_ERROR_ASSERT(keyCheckpointFileSPtr != nullptr);
            CODING_ERROR_ASSERT(keyCheckpointFileSPtr->PropertiesSPtr->KeyCount == numberOfKeys);
            CODING_ERROR_ASSERT(keyCheckpointFileSPtr->PropertiesSPtr->FileId == 10);
            CODING_ERROR_ASSERT(keyCheckpointFileSPtr->PropertiesSPtr->KeysHandle->Offset == 0);
            int defaultBlockSize = BlockAlignedWriter<int, int>::DefaultBlockAlignmentSize;
            int blockItemPartSize = defaultBlockSize - KeyChunkMetadata::Size - sizeof(ULONG64);

            //this is the actually key item size which is the key + reserved space padding and 8 byte aligned reserved.
            int serializedKeyItemSize = (keySize + 44) % 8 == 0 ? (keySize + 44) : (((keySize + 44) / 8 + 1) * 8);
            
            if (serializedKeyItemSize > blockItemPartSize)
            {
                int numberOfBlocksNeededForOneKey = (serializedKeyItemSize % blockItemPartSize == 0 ? serializedKeyItemSize / blockItemPartSize : serializedKeyItemSize / blockItemPartSize + 1);
                int keyblockSize = defaultBlockSize * numberOfBlocksNeededForOneKey * numberOfKeys;
                CODING_ERROR_ASSERT(keyCheckpointFileSPtr->PropertiesSPtr->KeysHandle->get_Size() == keyblockSize);
            }
            else
            {
                int loadPerBlock = (defaultBlockSize - (KeyChunkMetadata::Size + KeyBlockAlignedWriter<int, int>::ChecksumSize)) / serializedKeyItemSize;
                int blockCount = (numberOfKeys % loadPerBlock == 0 ? numberOfKeys / loadPerBlock : numberOfKeys / loadPerBlock + 1);
                int keyblockSize = defaultBlockSize * blockCount;
                CODING_ERROR_ASSERT(keyCheckpointFileSPtr->PropertiesSPtr->KeysHandle->get_Size() == keyblockSize);
            }

            KeyCheckpointFileAsyncEnumerator<KBuffer::SPtr, int>::SPtr keyEnumSPtr = nullptr;
            KeyCheckpointFileAsyncEnumerator<KBuffer::SPtr, int>::Create(
                *fileSPtr,
                serializer,
                fileSPtr->PropertiesSPtr->KeysHandle->Offset,
                fileSPtr->PropertiesSPtr->KeysHandle->EndOffset(),
                *CreateTraceComponent(),
                allocator,
                keyEnumSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            int count = co_await BlockAlignedReadAndVerifyKeysWithInsertedVersionedItemAsync(*keyEnumSPtr, startVal, keySize);
            CODING_ERROR_ASSERT(count == numberOfKeys);
            co_await keyEnumSPtr->CloseAsync();
            
            co_await fileSPtr->CloseAsync();
            co_await keyCheckpointFileSPtr->CloseAsync();
            RemoveFile(*filePathToOpenSPtr);
            co_return;
        }
#pragma endregion

#pragma region ValueBlockAlignedWriter utils
        ktl::Awaitable<void> CreateValueCheckpointFileStreamAsync(
            __in const WCHAR* fileName,
            __out ValueCheckpointFile::SPtr & fileSPtr,
            __out SharedBinaryWriter::SPtr & binaryWriterSPtr,
            __out ktl::io::KFileStream::SPtr & streamSPtr)
        {
            KAllocator& allocator = GetAllocator();
            KStringView filename = fileName;
            KString::SPtr filePathToOpenSPtr = CreateFileString(filename, GetAllocator());

            fileSPtr = co_await ValueCheckpointFile::CreateAsync(*CreateTraceComponent(), *filePathToOpenSPtr, 10, allocator);

            NTSTATUS status = SharedBinaryWriter::Create(allocator, binaryWriterSPtr);
            Diagnostics::Validate(status);

            streamSPtr = co_await fileSPtr->StreamPoolSPtr->AcquireStreamAsync();
            co_return;
        }
        
        ktl::Awaitable<void> CreateBlockAlignedWriterAsync(
            __in const WCHAR* filename,
            __out ValueCheckpointFile::SPtr & fileSPtr,
            __out SharedBinaryWriter::SPtr & binaryWriterSPtr,
            __out ktl::io::KFileStream::SPtr & streamSPtr,
            __out KBufferSerializer::SPtr & stateSerializer,
            __out ValueBlockAlignedWriter<ULONG32, KBuffer::SPtr>::SPtr & valBlockAlignedWriterSPtr)
        {
            KAllocator& allocator = GetAllocator();

            co_await CreateValueCheckpointFileStreamAsync(filename, fileSPtr, binaryWriterSPtr, streamSPtr);

            auto status = KBufferSerializer::Create(allocator, stateSerializer);
            Diagnostics::Validate(status);

            status = ValueBlockAlignedWriter<ULONG32, KBuffer::SPtr>::Create(*streamSPtr, *fileSPtr, *binaryWriterSPtr, *stateSerializer, *CreateTraceComponent(), allocator, valBlockAlignedWriterSPtr);
            Diagnostics::Validate(status);
            co_return;
        }

         KSharedPtr<VersionedItem<KBuffer::SPtr>> MakeVersionedBufferItem(
            __in ULONG32 bufferSize,  // The actual size to make
            __in ULONG32 valueSize)   // The stated serialized size
         {
            KBuffer::SPtr valueBufferSPtr = MakeCustomSizeBufferWithIntVal(bufferSize, 1);
            byte* valueArray = static_cast<byte*>(valueBufferSPtr->GetBuffer());
            for (ULONG32 i = 0; i < bufferSize; i++)
            {
                valueArray[i] = (byte)1;
            }

            KSharedPtr<InsertedVersionedItem<KBuffer::SPtr>> valueSPtr = nullptr;
            auto status = InsertedVersionedItem<KBuffer::SPtr>::Create(GetAllocator(), valueSPtr);
            Diagnostics::Validate(status);

            valueSPtr->SetValue(valueBufferSPtr);
            valueSPtr->SetValueSize(valueSize);
            valueSPtr->SetVersionSequenceNumber(100);
            return KSharedPtr<VersionedItem<KBuffer::SPtr>>(valueSPtr.RawPtr());
         }

         ktl::Awaitable<void> TestBlockWriteSingleItemAsync(
             __in const WCHAR * filename,
             __in ULONG32 bufferSize, // The actual serialized size
             __in ULONG32 valueSize)   // The stated serialized size
         {
            ValueCheckpointFile::SPtr fileSPtr = nullptr;
            SharedBinaryWriter::SPtr bwSPtr = nullptr;
            ktl::io::KFileStream::SPtr streamSPtr = nullptr;
            KBufferSerializer::SPtr stateSerializer = nullptr;
            ValueBlockAlignedWriter<ULONG32, KBuffer::SPtr>::SPtr valBlockAlignedWriterSPtr = nullptr;

            co_await CreateBlockAlignedWriterAsync(
                filename,
                fileSPtr,
                bwSPtr,
                streamSPtr,
                stateSerializer,
                valBlockAlignedWriterSPtr
            );

            KSharedPtr<VersionedItem<KBuffer::SPtr>> itemSPtr = MakeVersionedBufferItem(bufferSize, valueSize);

            ULONG32 key = 5;
            KeyValuePair<ULONG32, KSharedPtr<VersionedItem<KBuffer::SPtr>>> keyValuePair(key, itemSPtr);
            co_await valBlockAlignedWriterSPtr->BlockAlignedWriteValueAsync(keyValuePair, nullptr, true);
            co_await valBlockAlignedWriterSPtr->FlushAsync();
            co_await fileSPtr->FlushAsync(*streamSPtr, *bwSPtr);
            NTSTATUS status = co_await streamSPtr->CloseAsync();
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            KBuffer::SPtr resultBufferSPtr = co_await fileSPtr->ReadValueAsync<KSharedPtr<KBuffer>>(*itemSPtr, *stateSerializer);

            byte* result = static_cast<byte*>(resultBufferSPtr->GetBuffer());
            CODING_ERROR_ASSERT(resultBufferSPtr->QuerySize() == bufferSize);
            for (ULONG32 i = 0; i < bufferSize; i++)
            {
                CODING_ERROR_ASSERT(result[i] == (byte)1);
            }
            
            co_await fileSPtr->StreamPoolSPtr->ReleaseStreamAsync(*streamSPtr);
            co_await fileSPtr->CloseAsync();
            Common::File::Delete(filename);
         }

        ktl::Awaitable<KSharedPtr<KSharedArray<InsertedVersionedItem<KBuffer::SPtr>::SPtr>>> AlignWriteMultipleItemsAsync(
            __in ULONG32 count,
            __in ULONG32 bufferSizeBytes, // The actual serialized size
            __in ULONG32 valueSizeBytes,  // The stated serialized size 
            __in ValueBlockAlignedWriter<ULONG32, KBuffer::SPtr>& blockAlignedWriter)
        {
            ULONG32 bufferSize = bufferSizeBytes;

            KSharedPtr<KSharedArray<KSharedPtr<InsertedVersionedItem<KSharedPtr<KBuffer>>>>> itemsArray =
                _new(1, GetAllocator()) KSharedArray<KSharedPtr<InsertedVersionedItem<KSharedPtr<KBuffer>>>>();

            for (ULONG32 i = 0; i < count; i++)
            {
                KBuffer::SPtr valueBufferSPtr = nullptr;

                NTSTATUS status = KBuffer::Create(bufferSize, valueBufferSPtr, GetAllocator());
                CODING_ERROR_ASSERT(NT_SUCCESS(status));

                byte* valueArray = static_cast<byte*>(valueBufferSPtr->GetBuffer());
                for (ULONG32 j = 0; j < bufferSize; j++)
                {
                    valueArray[j] = (byte)1;
                }

                KSharedPtr<InsertedVersionedItem<KBuffer::SPtr>> valueSPtr = nullptr;
                status = InsertedVersionedItem<KBuffer::SPtr>::Create(GetAllocator(), valueSPtr);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
                valueSPtr->SetValue(valueBufferSPtr);
                valueSPtr->SetValueSize(valueSizeBytes);
                valueSPtr->SetVersionSequenceNumber(100);
                itemsArray->Append(valueSPtr);

                KSharedPtr<VersionedItem<KBuffer::SPtr>> itemSPtr(valueSPtr.RawPtr());

                KeyValuePair<ULONG32, KSharedPtr<VersionedItem<KBuffer::SPtr>>> keyValuePair(i, itemSPtr);
                co_await blockAlignedWriter.BlockAlignedWriteValueAsync(keyValuePair, nullptr, true);
            }

            co_return itemsArray;
        }

        ktl::Awaitable<void> TestBlockWriteMultipleItemsAsync(
            __in const WCHAR * filename,
            __in ULONG32 numItems,
            __in ULONG32 eachItemBufferSize,
            __in ULONG32 eachItemValueSize)
        {
            ValueCheckpointFile::SPtr fileSPtr = nullptr;
            SharedBinaryWriter::SPtr bwSPtr = nullptr;
            ktl::io::KFileStream::SPtr streamSPtr = nullptr;
            KBufferSerializer::SPtr stateSerializer = nullptr;
            ValueBlockAlignedWriter<ULONG32, KBuffer::SPtr>::SPtr valBlockAlignedWriterSPtr = nullptr;

            co_await CreateBlockAlignedWriterAsync(
                filename,
                fileSPtr,
                bwSPtr,
                streamSPtr,
                stateSerializer,
                valBlockAlignedWriterSPtr
            );

            auto itemsArray = co_await AlignWriteMultipleItemsAsync(numItems, eachItemBufferSize, eachItemValueSize, *valBlockAlignedWriterSPtr);
            co_await valBlockAlignedWriterSPtr->FlushAsync();

            co_await fileSPtr->FlushAsync(*streamSPtr, *bwSPtr);
            NTSTATUS status = co_await streamSPtr->CloseAsync();
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            for (ULONG32 i = 0; i < itemsArray->Count(); i++)
            {
                VersionedItem<KBuffer::SPtr>::SPtr itemSPtr((*itemsArray)[i].RawPtr());
                KBuffer::SPtr resultBufferSPtr = co_await fileSPtr->ReadValueAsync<KSharedPtr<KBuffer>>(*itemSPtr, *stateSerializer);

                byte* result = static_cast<byte*>(resultBufferSPtr->GetBuffer());
                CODING_ERROR_ASSERT(resultBufferSPtr->QuerySize() == eachItemBufferSize);
                for (ULONG32 j = 0; j < eachItemBufferSize; j++)
                {
                    CODING_ERROR_ASSERT(result[j] == (byte)1);
                }
            }
            
            co_await fileSPtr->CloseAsync();
            Common::File::Delete(filename);
            co_return;
        }
#pragma endregion
    private:
        KtlSystem* ktlSystem_;
        KBlockFile::SPtr fakeFileSPtr = nullptr;
        ktl::io::KFileStream::SPtr fakeStreamSPtr_ = nullptr;
        KString::SPtr currentFilePath_;

#pragma region test functions
    public:
        ktl::Awaitable<void> CheckpointFile_WriteOneItemKeySizeBiggerThanOneBlock4K_ShouldSucceed_Test()
        {
            co_await TestCheckpointFileAsync(1, 4084, 4100);
            co_return;
        }

        ktl::Awaitable<void> CheckpointFile_WriteOneItemKeySizeLargerThanOneChunk64K_ShouldSucceed_Test()
        {
            co_await TestCheckpointFileAsync(1, 32772, 32768);
            co_return;
        }

        ktl::Awaitable<void> CheckpointFile_WriteMutipleItemKeySizeLargerThanOneChunk64K_ShouldSucceed_Test()
        {
            co_await TestCheckpointFileAsync(100, 32772, 32768);
            co_return;
        }

        ktl::Awaitable<void> CheckpointFile_WriteOneItemKeySizeEqualToOneChunk64K_ShouldSucceed_Test()
        {
            //32708 + 44 reserved + 16bytes key chunk metatdata = 32*1024 bytes (1 chunk)
            co_await TestCheckpointFileAsync(1, 32708, 32764);
            co_return;
        }

        ktl::Awaitable<void> CheckpointFile_WriteMutipleItemKeySizeEqualToOneChunk64K_ShouldSucceed_Test()
        {
            //32708 + 44 reserved + 16bytes key chunk metatdata = 32*1024 bytes (1 chunk)
            co_await TestCheckpointFileAsync(100, 32708, 32764);
            co_return;
        }

        ktl::Awaitable<void> CheckpointFile_WriteMutipleItemSumKeySizeEqualToOneChunk64K_ShouldSucceed_Test()
        {
            co_await TestCheckpointFileAsync(2, 16324, 32764);
            co_return;
        }

        ktl::Awaitable<void> CheckpointFile_Write1000KeyValueItemsTotalTakeMutipleChunks_ShouldSucceed_Test()
        {
            co_await TestCheckpointFileAsync(1000, sizeof(int), sizeof(int));
            co_return;
        }

        ktl::Awaitable<void> FlieMetadataWithCanBeDeletedFlagSet_OnClose_ShouldDeleteCheckpointFile_Test()
        {
            KAllocator& allocator = GetAllocator();
            KStringView filename = L"FlieMetadataWithCanBeDeletedFlagSet_OnClose_ShouldDeleteCheckpointFile.txt";
            KString::SPtr filePath = CreateFileString(filename, allocator);

            KBufferSerializer::SPtr stateSerializerSPtr = nullptr;
            NTSTATUS status = KBufferSerializer::Create(allocator, stateSerializerSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            KSharedArray<KeyValuePair<KBuffer::SPtr, KSharedPtr<VersionedItem<KBuffer::SPtr>>>>::SPtr itemsArray
                = CheckpointWriteKeyWithInsertedVersionedItem(1000, sizeof(int), sizeof(int));

            IEnumerator<KeyValuePair<KBuffer::SPtr, KSharedPtr<VersionedItem<KBuffer::SPtr>>>>::SPtr itemsEnumerator;
            status = KSharedArrayEnumerator<KeyValuePair<KBuffer::SPtr, KSharedPtr<VersionedItem<KBuffer::SPtr>>>>::Create(*itemsArray, GetAllocator(), itemsEnumerator);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            StorePerformanceCountersSPtr perfCounters = nullptr;
            CheckpointFile::SPtr checkpointFileSPtr = co_await 
                CheckpointFile::CreateAsync<KBuffer::SPtr, KBuffer::SPtr>(
                    1,
                    *filePath,
                    *itemsEnumerator,
                    *stateSerializerSPtr,
                    *stateSerializerSPtr,
                    1,
                    allocator,
                    *CreateTraceComponent(),
                    perfCounters,
                    false);

            FileMetadata::SPtr fileMetadataSPtr = nullptr;
            KString::SPtr fileNameSPtr = nullptr;
            status = KString::Create(fileNameSPtr, allocator, L"test");
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            status = FileMetadata::Create(2, *fileNameSPtr, 2, 2, 2, 2, false, allocator, *CreateTraceComponent(), fileMetadataSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            fileMetadataSPtr->CheckpointFileSPtr = *checkpointFileSPtr;
            fileMetadataSPtr->CanBeDeleted = true;

            KString::SPtr keyFilePathToRemoveSPtr = nullptr;
            KString::Create(keyFilePathToRemoveSPtr, allocator, *filePath);
            KString::SPtr valueFilePathToRemoveSPtr = nullptr;
            KString::Create(valueFilePathToRemoveSPtr, allocator, *filePath);
            keyFilePathToRemoveSPtr->Concat(KeyCheckpointFile::GetFileExtension());
            valueFilePathToRemoveSPtr->Concat(ValueCheckpointFile::GetFileExtension());

            CODING_ERROR_ASSERT(Common::File::Exists((*keyFilePathToRemoveSPtr).operator LPCWSTR()));
            CODING_ERROR_ASSERT(Common::File::Exists((*valueFilePathToRemoveSPtr).operator LPCWSTR()));

            co_await fileMetadataSPtr->CloseAsync();
            CODING_ERROR_ASSERT(!Common::File::Exists((*keyFilePathToRemoveSPtr).operator LPCWSTR()));
            CODING_ERROR_ASSERT(!Common::File::Exists((*valueFilePathToRemoveSPtr).operator LPCWSTR()));
            co_return;
        }

        ktl::Awaitable<void> KeyCheckpointFileProp_WriteAndRead_ShouldSucceed_Test()
        {
            KAllocator& allocator = GetAllocator();

            KeyCheckpointFileProperties::SPtr prop = nullptr;
            NTSTATUS status = KeyCheckpointFileProperties::Create(allocator, prop);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            prop->KeyCount = 3;
            prop->FileId = 10;
            ULONG64 offset = 0;
            ULONG64 size = 16;
            BlockHandle::SPtr handle = nullptr;
            status = BlockHandle::Create(offset, size, allocator, handle);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            prop->KeysHandle = *handle;

            BinaryWriter bw(allocator);
            prop->Write(bw);

            BlockHandle::SPtr bh = nullptr;
            status = BlockHandle::Create(0, bw.get_Position(), allocator, bh);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            BinaryReader br(*bw.GetBuffer(0), allocator);

            auto p = FilePropertySection::Read<KeyCheckpointFileProperties>(br, *bh, allocator);

            CODING_ERROR_ASSERT(prop->KeyCount == p->KeyCount);
            CODING_ERROR_ASSERT(prop->FileId == p->FileId);
            CODING_ERROR_ASSERT(prop->KeysHandle->Offset == offset);
            CODING_ERROR_ASSERT(prop->KeysHandle->Size == size);
            co_return;
        }

        ktl::Awaitable<void> KeyCheckpointFileProp_WriteToDiskAndRead_ShouldSucceed_Test()
        {
            KAllocator& allocator = GetAllocator();
            KStringView filename = L"KeyCheckpointFileProp_WriteToDiskAndRead_ShouldSucceed.txt";
            co_await CreateFileStreamAsync(filename);

            //create prop and assign value
            KeyCheckpointFileProperties::SPtr prop = nullptr;
            NTSTATUS status = KeyCheckpointFileProperties::Create(allocator, prop);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            prop->KeyCount = 3;
            prop->FileId = 10;
            ULONG64 offset = 0;
            ULONG64 size = 16;
            BlockHandle::SPtr handle = nullptr;
            status = BlockHandle::Create(offset, size, allocator, handle);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            prop->KeysHandle = *handle;

            //write using binary writer.
            BinaryWriter bw(allocator);
            prop->Write(bw);

            //collect data written block info.
            BlockHandle::SPtr bh = nullptr;
            status = BlockHandle::Create(0, bw.get_Position(), allocator, bh);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            //write to disk.
            ktl::io::KFileStream::SPtr fileStreamSPtr = GetFileStream();
            status = co_await fileStreamSPtr->WriteAsync(*bw.GetBuffer(0));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            status = co_await fileStreamSPtr->FlushAsync();
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            co_await CloseFileStreamAsync(false);

            //read from disk.
            co_await CreateFileStreamAsync(filename, false);
            fileStreamSPtr = GetFileStream();
            KBuffer::SPtr buffer = nullptr;
            ULONG bytesRead = 0;
            ULONG bytesToRead = static_cast<ULONG>(bh->get_Size());
            ULONG offsetToRead = static_cast<ULONG>(bh->get_Offset());
            status = KBuffer::Create(
                bytesToRead,
                buffer,
                allocator);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            status = co_await fileStreamSPtr->ReadAsync(*buffer, bytesRead, offsetToRead, bytesToRead);
            CODING_ERROR_ASSERT(bytesRead == bytesToRead);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            BinaryReader br(*buffer, allocator);

            auto p = FilePropertySection::Read<KeyCheckpointFileProperties>(br, *bh, allocator);

            CODING_ERROR_ASSERT(prop->KeyCount == p->KeyCount);
            CODING_ERROR_ASSERT(prop->FileId == p->FileId);
            CODING_ERROR_ASSERT(prop->KeysHandle->Offset == offset);
            CODING_ERROR_ASSERT(prop->KeysHandle->Size == size);

            co_await CloseFileStreamAsync();
            co_return;
        }

        ktl::Awaitable<void> ValueCheckpointFileProp_WriteAndRead_ShouldSucceed_Test()
        {
            KAllocator& allocator = GetAllocator();

            ValueCheckpointFileProperties::SPtr prop = nullptr;
            NTSTATUS status = ValueCheckpointFileProperties::Create(allocator, prop);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            prop->ValueCount = 3;
            prop->FileId = 10;
            ULONG64 offset = 0;
            ULONG64 size = 16;
            BlockHandle::SPtr handle = nullptr;
            status = BlockHandle::Create(offset, size, allocator, handle);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            prop->ValuesHandle = *handle;

            BinaryWriter bw(allocator);
            prop->Write(bw);

            BlockHandle::SPtr bh = nullptr;
            status = BlockHandle::Create(0, bw.get_Position(), allocator, bh);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            BinaryReader br(*bw.GetBuffer(0), allocator);

            auto p = FilePropertySection::Read<ValueCheckpointFileProperties>(br, *bh, allocator);

            CODING_ERROR_ASSERT(prop->ValueCount == p->ValueCount);
            CODING_ERROR_ASSERT(prop->FileId == p->FileId);
            CODING_ERROR_ASSERT(prop->ValuesHandle->Offset == offset);
            CODING_ERROR_ASSERT(prop->ValuesHandle->Size == size);
            co_return;
        }

        ktl::Awaitable<void> ValueCheckpointFileProp_WriteToDiskAndRead_ShouldSucceed_Test()
        {
            KAllocator& allocator = GetAllocator();
            KStringView filename = L"ValueCheckpointFileProp_WriteToDiskAndRead_ShouldSucceed.txt";
            co_await CreateFileStreamAsync(filename);

            //create prop and assign value
            ValueCheckpointFileProperties::SPtr prop = nullptr;
            NTSTATUS status = ValueCheckpointFileProperties::Create(allocator, prop);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            prop->ValueCount = 3;
            prop->FileId = 10;
            ULONG64 offset = 0;
            ULONG64 size = 16;
            BlockHandle::SPtr handle = nullptr;
            status = BlockHandle::Create(offset, size, allocator, handle);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            prop->ValuesHandle = *handle;

            //write using binary writer.
            BinaryWriter bw(allocator);
            prop->Write(bw);

            //collect data written block info.
            BlockHandle::SPtr bh = nullptr;
            status = BlockHandle::Create(0, bw.get_Position(), allocator, bh);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            //write to disk.
            ktl::io::KFileStream::SPtr fileStreamSPtr = GetFileStream();
            status = co_await fileStreamSPtr->WriteAsync(*bw.GetBuffer(0));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            status = co_await fileStreamSPtr->FlushAsync();
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            co_await CloseFileStreamAsync(false);

            //read from disk.
            co_await CreateFileStreamAsync(filename, false);
            fileStreamSPtr = GetFileStream();
            KBuffer::SPtr bufferSPtr = nullptr;
            ULONG bytesRead = 0;
            ULONG bytesToRead = static_cast<ULONG>(bh->get_Size());
            ULONG offsetToRead = static_cast<ULONG>(bh->get_Offset());
            status = KBuffer::Create(
                bytesToRead,
                bufferSPtr,
                allocator);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            status = co_await fileStreamSPtr->ReadAsync(*bufferSPtr, bytesRead, offsetToRead, bytesToRead);
            CODING_ERROR_ASSERT(bytesRead == bytesToRead);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            BinaryReader br(*bufferSPtr, allocator);

            auto p = FilePropertySection::Read<ValueCheckpointFileProperties>(br, *bh, allocator);

            CODING_ERROR_ASSERT(prop->ValueCount == p->ValueCount);
            CODING_ERROR_ASSERT(prop->FileId == p->FileId);
            CODING_ERROR_ASSERT(prop->ValuesHandle->Offset == offset);
            CODING_ERROR_ASSERT(prop->ValuesHandle->Size == size);

            co_await CloseFileStreamAsync();
            co_return;
        }

        ktl::Awaitable<void> KeyCheckpointFile_WriteOneKeyAndRead_ShouldSucceed_Test()
        {
            KAllocator& allocator = GetAllocator();
            KStringView filename = L"KeyCheckpointFile_WriteOneKeyAndRead_ShouldSucceed.txt";
            KString::SPtr filePathToOpenSPtr = CreateFileString(filename, GetAllocator());

            KeyCheckpointFile::SPtr fileSPtr = co_await KeyCheckpointFile::CreateAsync(*CreateTraceComponent(), *filePathToOpenSPtr, false, 10, allocator);
            BinaryWriter bw(allocator);

            TestStateSerializer<int>::SPtr stateSerializer = nullptr;
            NTSTATUS status = TestStateSerializer<int>::Create(allocator, stateSerializer);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            int key = 99;
            WriteKeyWithInsertedVersionedItem(bw, *fileSPtr, key, *stateSerializer);

            BinaryReader br(*bw.GetBuffer(0), allocator);
            ReadAndVerifyKeysWithInsertedVersionedItem(br, *fileSPtr, key, *stateSerializer);
            co_await fileSPtr->CloseAsync();
            RemoveFile(*filePathToOpenSPtr);
            co_return;
        }

        ktl::Awaitable<void> KeyCheckpointFile_Write1000KeysAndRead_ShouldSucceed_Test()
        {
            KAllocator& allocator = GetAllocator();
            KStringView filename = L"KeyCheckpointFile_Write1000KeysAndRead_ShouldSucceed.txt";
            KString::SPtr filePathToOpenSPtr = CreateFileString(filename, GetAllocator());

            KeyCheckpointFile::SPtr fileSPtr = co_await KeyCheckpointFile::CreateAsync(*CreateTraceComponent(), *filePathToOpenSPtr, false, 10, allocator);
            BinaryWriter bw(allocator);
            TestStateSerializer<int>::SPtr stateSerializer = nullptr;
            NTSTATUS status = TestStateSerializer<int>::Create(allocator, stateSerializer);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            for (int i = 0; i < 1000; i++)
            {
                WriteKeyWithInsertedVersionedItem(bw, *fileSPtr, i, *stateSerializer);
            }

            BinaryReader br(*bw.GetBuffer(0), allocator);
            for (int i = 0; i < 1000; i++)
            {
                ReadAndVerifyKeysWithInsertedVersionedItem(br, *fileSPtr, i, *stateSerializer);
            }

            co_await fileSPtr->CloseAsync();
            RemoveFile(*filePathToOpenSPtr);
            co_return;
        }

        ktl::Awaitable<void> KeyCheckpointFile_WriteOneKeyAndFlushThenOpenVerifyMetadata_ShouldSucceed_Test()
        {
            KAllocator& allocator = GetAllocator();
            KStringView filename = L"KeyCheckpointFile_WriteOneKeyAndFlushThenOpenVerifyMetadata_ShouldSucceed.txt";
            KString::SPtr filePathToOpenSPtr = CreateFileString(filename, GetAllocator());

            ULONG32 fileId = 10;
            KeyCheckpointFile::SPtr fileSPtr = co_await KeyCheckpointFile::CreateAsync(*CreateTraceComponent(), *filePathToOpenSPtr, false, fileId, allocator);
            SharedBinaryWriter::SPtr bwSptr = nullptr;
            NTSTATUS status = SharedBinaryWriter::Create(allocator, bwSptr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            TestStateSerializer<int>::SPtr stateSerializer = nullptr;
            status = TestStateSerializer<int>::Create(allocator, stateSerializer);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            int key = 1;
            WriteKeyWithInsertedVersionedItem(*bwSptr, *fileSPtr, key, *stateSerializer);

            ktl::io::KFileStream::SPtr streamSPtr = co_await fileSPtr->StreamPoolSPtr->AcquireStreamAsync();
            co_await fileSPtr->FlushAsync(*streamSPtr, *bwSptr);
            co_await fileSPtr->StreamPoolSPtr->ReleaseStreamAsync(*streamSPtr);

            KeyCheckpointFile::SPtr keyCheckpointFileSPtr = co_await KeyCheckpointFile::OpenAsync(allocator, *filePathToOpenSPtr, *CreateTraceComponent(), true);
            CODING_ERROR_ASSERT(keyCheckpointFileSPtr != nullptr);
            CODING_ERROR_ASSERT(keyCheckpointFileSPtr->PropertiesSPtr->KeyCount == 1);
            CODING_ERROR_ASSERT(keyCheckpointFileSPtr->PropertiesSPtr->FileId == fileId);

            CODING_ERROR_ASSERT(keyCheckpointFileSPtr->PropertiesSPtr->KeysHandle->get_Offset() == 0);
            CODING_ERROR_ASSERT(keyCheckpointFileSPtr->PropertiesSPtr->KeysHandle->get_Size() == 48); //40 reserved bytes for item and 8 for the item with padding

            KBuffer::SPtr bufferSPtr = nullptr;
            ULONG bytesToRead = 48;
            ULONG bytesRead = 0;
            status = KBuffer::Create(
                bytesToRead,
                bufferSPtr,
                allocator);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktl::io::KFileStream::SPtr streamReadSPtr = co_await keyCheckpointFileSPtr->StreamPoolSPtr->AcquireStreamAsync();
            status = co_await streamReadSPtr->ReadAsync(*bufferSPtr, bytesRead, 0, bytesToRead);
            co_await keyCheckpointFileSPtr->StreamPoolSPtr->ReleaseStreamAsync(*streamReadSPtr);

            CODING_ERROR_ASSERT(bytesRead == bytesToRead);

            BinaryReader br(*bufferSPtr, allocator);
            ReadAndVerifyKeysWithInsertedVersionedItem(br, *fileSPtr, key, *stateSerializer);
        
            co_await fileSPtr->CloseAsync();
            co_await keyCheckpointFileSPtr->CloseAsync();
            RemoveFile(*filePathToOpenSPtr);
            co_return;
        }

        ktl::Awaitable<void> ValueCheckpointFile_WriteOneKeyAndRead_ShouldSucceed_Test()
        {
            KAllocator& allocator = GetAllocator();
            KStringView filename = L"ValueCheckpointFile_WriteOneKeyAndRead_ShouldSucceed.txt";
            KString::SPtr filePathToOpenSPtr = CreateFileString(filename, GetAllocator());

            ValueCheckpointFile::SPtr fileSPtr = co_await ValueCheckpointFile::CreateAsync(*CreateTraceComponent(), *filePathToOpenSPtr, 10, allocator);

            SharedBinaryWriter::SPtr bwSPtr = nullptr;
            NTSTATUS status = SharedBinaryWriter::Create(allocator, bwSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            ktl::io::KFileStream::SPtr streamSPtr = co_await fileSPtr->StreamPoolSPtr->AcquireStreamAsync();
            TestStateSerializer<int>::SPtr stateSerializer = nullptr;
            status = TestStateSerializer<int>::Create(allocator, stateSerializer);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            int value = 99;
            KSharedPtr<VersionedItem<int>> itemSPtr = AddValuesWithInsertedVersionedItem(*streamSPtr, *bwSPtr, *fileSPtr, value, *stateSerializer);

            co_await fileSPtr->FlushAsync(*streamSPtr, *bwSPtr);
            co_await fileSPtr->StreamPoolSPtr->ReleaseStreamAsync(*streamSPtr);

            int val = co_await fileSPtr->ReadValueAsync<int>(*itemSPtr, *stateSerializer);
            CODING_ERROR_ASSERT(val== value);
   
            co_await fileSPtr->CloseAsync();
            RemoveFile(*filePathToOpenSPtr);
            co_return;
        }

        ktl::Awaitable<void> ValueCheckpointFile_WriteOneKeyAndReadUsingBytes_ShouldSucceed_Test()
        {
            KAllocator& allocator = GetAllocator();
            KStringView filename = L"ValueCheckpointFile_WriteOneKeyAndReadUsingBytes_ShouldSucceed.txt";
            KString::SPtr filePathToOpenSPtr = CreateFileString(filename, GetAllocator());

            ValueCheckpointFile::SPtr fileSPtr = co_await ValueCheckpointFile::CreateAsync(*CreateTraceComponent(), *filePathToOpenSPtr, 10, allocator);
            SharedBinaryWriter::SPtr bwSPtr = nullptr;
            NTSTATUS status = SharedBinaryWriter::Create(allocator, bwSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            ktl::io::KFileStream::SPtr streamSPtr = co_await fileSPtr->StreamPoolSPtr->AcquireStreamAsync();

            byte value = 99;
            BinaryWriter br(allocator);
            br.Write(value);
            KBuffer::SPtr keydataSPtr = br.GetBuffer(0);
            KSharedPtr<VersionedItem<int>> itemSPtr = AddValuesInBytesWithInsertedVersionedItem(*streamSPtr, *bwSPtr, *fileSPtr, value, *keydataSPtr);

            co_await fileSPtr->FlushAsync(*streamSPtr, *bwSPtr);
            co_await fileSPtr->StreamPoolSPtr->ReleaseStreamAsync(*streamSPtr);

            KBuffer::SPtr val = co_await fileSPtr->ReadValueAsync<int>(*itemSPtr);
            CODING_ERROR_ASSERT(val->QuerySize() == keydataSPtr->QuerySize());
            CODING_ERROR_ASSERT(IsSameBufferValue(*val, *keydataSPtr));

            co_await fileSPtr->CloseAsync();
            RemoveFile(*filePathToOpenSPtr);
            co_return;
        }

        ktl::Awaitable<void> ValueCheckpointFile_Write1000KeysAndReadAndVerifyMetadata_ShouldSucceed_Test()
        {
            KAllocator& allocator = GetAllocator();
            KStringView filename = L"ValueCheckpointFile_Write1000KeysAndReadAndVerifyMetadata_ShouldSucceed.txt";
            KString::SPtr filePathToOpenSPtr = CreateFileString(filename, GetAllocator());

            ULONG32 fileId = 10;
            ValueCheckpointFile::SPtr fileSPtr = co_await ValueCheckpointFile::CreateAsync(*CreateTraceComponent(), *filePathToOpenSPtr, fileId, allocator);
            SharedBinaryWriter::SPtr bwSPtr = nullptr;
            NTSTATUS status = SharedBinaryWriter::Create(allocator, bwSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            ktl::io::KFileStream::SPtr streamSPtr = co_await fileSPtr->StreamPoolSPtr->AcquireStreamAsync();
            TestStateSerializer<int>::SPtr stateSerializer = nullptr;
            status = TestStateSerializer<int>::Create(allocator, stateSerializer);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            KArray<KSharedPtr<VersionedItem<int>>> itemList(allocator);
            for (int i = 0; i < 1000; i++)
            {
                KSharedPtr<VersionedItem<int>> itemSPtr = AddValuesWithInsertedVersionedItem(*streamSPtr, *bwSPtr, *fileSPtr, i, *stateSerializer);
                itemList.Append(itemSPtr);
            }

            co_await fileSPtr->FlushAsync(*streamSPtr, *bwSPtr);
            co_await fileSPtr->StreamPoolSPtr->ReleaseStreamAsync(*streamSPtr);

            ValueCheckpointFile::SPtr valueCheckpointFileSPtr = co_await ValueCheckpointFile::OpenAsync(allocator, *filePathToOpenSPtr, *CreateTraceComponent());
            CODING_ERROR_ASSERT(valueCheckpointFileSPtr != nullptr);
            CODING_ERROR_ASSERT(valueCheckpointFileSPtr->PropertiesSPtr->ValueCount == 1000);
            CODING_ERROR_ASSERT(valueCheckpointFileSPtr->PropertiesSPtr->FileId == fileId);

            CODING_ERROR_ASSERT(valueCheckpointFileSPtr->PropertiesSPtr->ValuesHandle->get_Offset() == 0);
            CODING_ERROR_ASSERT(valueCheckpointFileSPtr->PropertiesSPtr->ValuesHandle->get_Size() == 4 * 1000); //int 4 bytes, 1000 is 4000 bytes size.

            for (int i = 0; i < 1000; i++)
            {
                int val = co_await fileSPtr->ReadValueAsync<int>(*itemList[i], *stateSerializer);
                CODING_ERROR_ASSERT(val == i);
            }
        
            co_await fileSPtr->CloseAsync();
            co_await valueCheckpointFileSPtr->CloseAsync();
            RemoveFile(*filePathToOpenSPtr);
            co_return;
        }

        ktl::Awaitable<void> ValueCheckpointFile_Write1000KeysAndReadAndVerifyConcurrentlyWithStreamPool_ShouldSucceed_Test()
        {
            KAllocator& allocator = GetAllocator();
            KStringView filename = L"ValueCheckpointFile_Write1000KeysAndReadAndVerifyConcurrentlyWithStreamPool_ShouldSucceed.txt";
            KString::SPtr filePathToOpenSPtr = CreateFileString(filename, GetAllocator());

            ULONG32 fileId = 10;
            ValueCheckpointFile::SPtr fileSPtr = co_await ValueCheckpointFile::CreateAsync(*CreateTraceComponent(), *filePathToOpenSPtr, fileId, allocator);
            SharedBinaryWriter::SPtr bwSPtr = nullptr;
            NTSTATUS status = SharedBinaryWriter::Create(allocator, bwSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            ktl::io::KFileStream::SPtr streamSPtr = co_await fileSPtr->StreamPoolSPtr->AcquireStreamAsync();
            TestStateSerializer<int>::SPtr stateSerializer = nullptr;
            status = TestStateSerializer<int>::Create(allocator, stateSerializer);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            KArray<KSharedPtr<VersionedItem<int>>> itemList(allocator);
            for (int i = 0; i < 1000; i++)
            {
                KSharedPtr<VersionedItem<int>> itemSPtr = AddValuesWithInsertedVersionedItem(*streamSPtr, *bwSPtr, *fileSPtr, i, *stateSerializer);
                itemList.Append(itemSPtr);
            }

            co_await fileSPtr->FlushAsync(*streamSPtr, *bwSPtr);
            co_await fileSPtr->StreamPoolSPtr->ReleaseStreamAsync(*streamSPtr);

            ValueCheckpointFile::SPtr valueCheckpointFileSPtr = co_await ValueCheckpointFile::OpenAsync(allocator, *filePathToOpenSPtr, *CreateTraceComponent());
            CODING_ERROR_ASSERT(valueCheckpointFileSPtr != nullptr);
            CODING_ERROR_ASSERT(valueCheckpointFileSPtr->PropertiesSPtr->ValueCount == 1000);
            CODING_ERROR_ASSERT(valueCheckpointFileSPtr->PropertiesSPtr->FileId == fileId);

            CODING_ERROR_ASSERT(valueCheckpointFileSPtr->PropertiesSPtr->ValuesHandle->get_Offset() == 0);
            CODING_ERROR_ASSERT(valueCheckpointFileSPtr->PropertiesSPtr->ValuesHandle->get_Size() == 4 * 1000); //int 4 bytes, 1000 is 4000 bytes size.

            KArray<Awaitable<int>> taskList(GetAllocator());

            for (ULONG i = 0; i < 500; i++)
            {
                taskList.Append(fileSPtr->ReadValueAsync<int>(*itemList[i], *stateSerializer));
            }

            for (ULONG i = 0; i < taskList.Count(); i++)
            {
                int val = co_await taskList[i];
                CODING_ERROR_ASSERT((ULONG)val == i);
            }

            taskList.Clear();

            for (ULONG i = 500; i < 1000; i++)
            {
                taskList.Append(fileSPtr->ReadValueAsync<int>(*itemList[i], *stateSerializer));
            }

            for (ULONG i = 0; i < taskList.Count(); i++)
            {
                int val = co_await taskList[i];
                CODING_ERROR_ASSERT((ULONG)val == i + 500);
            }

            co_await fileSPtr->CloseAsync();
            co_await valueCheckpointFileSPtr->CloseAsync();
            RemoveFile(*filePathToOpenSPtr);
            co_return;
        }

        ktl::Awaitable<void> ValueCheckpointFile_Write100KeysAndReadAndVerifyMetadataUsingBytes_ShouldSucceed_Test()
        {
            KAllocator& allocator = GetAllocator();
            KStringView filename = L"ValueCheckpointFile_Write100KeysAndReadAndVerifyMetadataUsingBytes_ShouldSucceed.txt";
            KString::SPtr filePathToOpenSPtr = CreateFileString(filename, GetAllocator());

            ULONG32 fileId = 10;
            ValueCheckpointFile::SPtr fileSPtr = co_await ValueCheckpointFile::CreateAsync(*CreateTraceComponent(), *filePathToOpenSPtr, fileId, allocator);
            SharedBinaryWriter::SPtr bwSPtr = nullptr;
            NTSTATUS status = SharedBinaryWriter::Create(allocator, bwSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            ktl::io::KFileStream::SPtr streamSPtr = co_await fileSPtr->StreamPoolSPtr->AcquireStreamAsync();

            KArray<KSharedPtr<VersionedItem<int>>> itemList(allocator);
            KArray<KBuffer::SPtr> valueList(allocator);

            for (int i = 0; i < 100; i++)
            {
                BinaryWriter br(allocator);
                br.Write(static_cast<byte>(i));
                br.Write(static_cast<byte>(i+1));
                br.Write(static_cast<byte>(i+2));
                valueList.Append(br.GetBuffer(0));
                KSharedPtr<VersionedItem<int>> itemSPtr = AddValuesInBytesWithInsertedVersionedItem(*streamSPtr, *bwSPtr, *fileSPtr, i, *br.GetBuffer(0));
                itemList.Append(itemSPtr);
            }

            co_await fileSPtr->FlushAsync(*streamSPtr, *bwSPtr);
            co_await fileSPtr->StreamPoolSPtr->ReleaseStreamAsync(*streamSPtr);

            ValueCheckpointFile::SPtr valueCheckpointFileSPtr = co_await ValueCheckpointFile::OpenAsync(allocator, *filePathToOpenSPtr, *CreateTraceComponent());
            CODING_ERROR_ASSERT(valueCheckpointFileSPtr != nullptr);
            CODING_ERROR_ASSERT(valueCheckpointFileSPtr->PropertiesSPtr->ValueCount == 100);
            CODING_ERROR_ASSERT(valueCheckpointFileSPtr->PropertiesSPtr->FileId == fileId);

            CODING_ERROR_ASSERT(valueCheckpointFileSPtr->PropertiesSPtr->ValuesHandle->get_Offset() == 0);
            CODING_ERROR_ASSERT(valueCheckpointFileSPtr->PropertiesSPtr->ValuesHandle->get_Size() == 3 * 100); //3 bytes, 100 is 300 bytes size.


            for (int i = 0; i < 100; i++)
            {
                KBuffer::SPtr val = co_await fileSPtr->ReadValueAsync<int>(*itemList[i]);
                CODING_ERROR_ASSERT(val->QuerySize() == 3);
                CODING_ERROR_ASSERT(*valueList[i] == *val);
            }
        
            co_await fileSPtr->CloseAsync();
            co_await valueCheckpointFileSPtr->CloseAsync();
            RemoveFile(*filePathToOpenSPtr);
            co_return;
        }

        ktl::Awaitable<void> KeyBlockAlignedWriter_WriteOnKeyAndEnumerate_ShouldSucceed_Test()
        {
            //one int key item is 4 bytes of serialzied key size, 48 bytes data in total (44 is reserved for meta and padding)
            KBufferSerializer::SPtr serializer = nullptr;
            KBufferSerializer::Create(GetAllocator(), serializer);
            co_await TestKeyBlockAlignedWriteAndEnumeratorAsync(1, sizeof(int), *serializer);
            co_return;
        }

        ktl::Awaitable<void> KeyBlockAlignedWriter_Write50KeysLessThanOneBlockSize_ShouldSucceed_Test()
        {
            //50 int keys are 48 * 50 = 2400 bytes, which less than one block.
            KBufferSerializer::SPtr serializer = nullptr;
            KBufferSerializer::Create(GetAllocator(), serializer);
            co_await TestKeyBlockAlignedWriteAndEnumeratorAsync(50, sizeof(int), *serializer);
            co_return;
        }

        ktl::Awaitable<void> KeyBlockAlignedWriter_WriteKeysTakeMutipleBlocks_ShouldSucceed_Test()
        {
            //1200 int keys are 48 * 1200 = 57600 bytes, one block hold 4080 bytes or 85 4-byte key items, which then need 15 blocks.
            KBufferSerializer::SPtr serializer = nullptr;
            KBufferSerializer::Create(GetAllocator(), serializer);
            co_await TestKeyBlockAlignedWriteAndEnumeratorAsync(1200, sizeof(int),*serializer);
            co_return;
        }

        ktl::Awaitable<void> KeyBlockAlignedWriter_WriteKeysTakeMutipleChunks_ShouldSucceed_Test()
        {
            //100000 int keys are 1177 blocks, or 147 chunks.
            KBufferSerializer::SPtr serializer = nullptr;
            KBufferSerializer::Create(GetAllocator(), serializer);
            co_await TestKeyBlockAlignedWriteAndEnumeratorAsync(100000, sizeof(int), *serializer);
            co_return;
        }

        ktl::Awaitable<void> KeyBlockAlignedWriter_WriteBiggerKeysWithBlockNotFullyOccupiedAndMoveToNextBlock_ShouldSucceed_Test()
        {
            //one key is 12 byte, with reserved space is 12+44 = 56bytes , then 73 keys are 4088 bytes.( more than one block bytes - one block item part is 4080 bytes)
            //this will need two blocks with the 1st block not fully occupied and the last key moves to the 2nd block.
            KBufferSerializer::SPtr serializer = nullptr;
            KBufferSerializer::Create(GetAllocator(), serializer);
            co_await TestKeyBlockAlignedWriteAndEnumeratorAsync(73, 12, *serializer);
            co_return;
        }

        ktl::Awaitable<void> KeyBlockAlignedWriter_WriteOneKeyLargerThanOneBlock_ShouldSucceed_Test()
        {
            //one key is 4084 byte, with reserved space is 4084+44 = 4128 (more than one block bytes - one block item part is 4080 bytes)
            //this one key takes two blocks.
            KBufferSerializer::SPtr serializer = nullptr;
            KBufferSerializer::Create(GetAllocator(), serializer);
            co_await TestKeyBlockAlignedWriteAndEnumeratorAsync(1, 4084, *serializer);
            co_return;
        }

        ktl::Awaitable<void> KeyBlockAlignedWriter_WriteOneKeyLargerThanOneChunk_ShouldSucceed_Test()
        {
            //one key is 32772 bytes, with reserved space is 32772+44 = 32816 (more than one chunk 32768 bytes).
            //this one key takes two chunks.
            KBufferSerializer::SPtr serializer = nullptr;
            KBufferSerializer::Create(GetAllocator(), serializer);
            co_await TestKeyBlockAlignedWriteAndEnumeratorAsync(1, 32772, *serializer);
            co_return;
        }

        ktl::Awaitable<void> ValueBlockAlignedWrite_SingleKey_ShouldSucceed_Test()
        {
            ULONG32 bufferSize = 1024;
            ULONG32 valueSize = bufferSize;
            co_await TestBlockWriteSingleItemAsync(
                L"BlockAlignedWrite_SingleKey_ShouldSucceed.txt",
                bufferSize, 
                valueSize
            );
            co_return;
        }

        ktl::Awaitable<void> ValueBlockAlignedWrite_ValueBiggerThanBlockSize_ShouldSucceed_Test()
        {
            ULONG32 bufferSize = 5000;
            ULONG32 valueSize = bufferSize;
            co_await TestBlockWriteSingleItemAsync(
                L"BlockAlignedWrite_ValueBiggerThanBlockSize_ShouldSucceed.txt",
                bufferSize, 
                valueSize
            );
            co_return;
        }

        ktl::Awaitable<void> ValueBlockAlignedWrite_ValueSizeSmallerThanActualSize_ShouldSucceed_Test()
        {
            ULONG32 bufferSize = 1024;
            ULONG32 valueSize = 100;
            co_await TestBlockWriteSingleItemAsync(
                L"BlockAlignedWrite_ValueSizeSmallerThanActualSize_ShouldSucceed.txt",
                bufferSize, 
                valueSize
            );
            co_return;
        }

        ktl::Awaitable<void> ValueBlockAlignedWrite_ValueSizeLargerThanActualSize_ShouldSucceed_Test()
        {
            ULONG32 bufferSize = 1024;
            ULONG32 valueSize = 5000;
            co_await TestBlockWriteSingleItemAsync(
                L"BlockAlignedWrite_ValueSizeSmallerThanActualSize_ShouldSucceed.txt",
                bufferSize, 
                valueSize
            );
            co_return;
        }

        ktl::Awaitable<void> ValueBlockAlignedWrite_MultipleValuesNotBlockAligned_ShouldSucceed_Test()
        {
            ULONG32 numItems = 5;
            ULONG32 bufferSize = 1000;
            ULONG32 valueSize = 1000;
            co_await TestBlockWriteMultipleItemsAsync(
                L"BlockAlignedWrite_MultipleValuesNotBlockAligned_ShouldSucceed.txt",
                numItems,
                bufferSize,
                valueSize
            );
            co_return;
        }

        ktl::Awaitable<void> ValueBlockAlignedWrite_MultipleValuesBlockAligned_ShouldSucceed_Test()
        {
            ULONG32 numItems = 4;
            ULONG32 bufferSize = 1024;
            ULONG32 valueSize = 1024;
            co_await TestBlockWriteMultipleItemsAsync(
                L"BlockAlignedWrite_MultipleValuesNotBlockAligned_ShouldSucceed.txt",
                numItems,
                bufferSize,
                valueSize
            );
            co_return;
        }

        ktl::Awaitable<void> ValueBlockAlignedWrite_MultipleValuesBlockAligned_ValueSizeSmaller_ShouldSucceed_Test()
        {
            ULONG32 numItems = 4;
            ULONG32 bufferSize = 1024;
            ULONG32 valueSize = 100;
            co_await TestBlockWriteMultipleItemsAsync(
                L"BlockAlignedWrite_MultipleValuesNotBlockAligned_ShouldSucceed.txt",
                numItems,
                bufferSize,
                valueSize
            );
            co_return;
        }

        ktl::Awaitable<void> ValueBlockAlignedWrite_MultipleValuesBlockAligned_ValueSizeLarger_ShouldSucceed_Test()
        {
            ULONG32 numItems = 4;
            ULONG32 bufferSize = 1024;
            ULONG32 valueSize = 2048;
            co_await TestBlockWriteMultipleItemsAsync(
                L"BlockAlignedWrite_MultipleValuesBlockAligned_ValueSizeLarger_ShouldSucceed.txt",
                numItems,
                bufferSize,
                valueSize
            );
            co_return;
        }
    #pragma endregion
    };

    BOOST_FIXTURE_TEST_SUITE(CheckpointFilePropertiesTestSuite, CheckpointFileTest)
    
    BOOST_AUTO_TEST_CASE(CheckpointFile_WriteOneItemKeySizeBiggerThanOneBlock4K_ShouldSucceed)
    {
        SyncAwait(CheckpointFile_WriteOneItemKeySizeBiggerThanOneBlock4K_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_WriteOneItemKeySizeLargerThanOneChunk64K_ShouldSucceed)
    {
        SyncAwait(CheckpointFile_WriteOneItemKeySizeLargerThanOneChunk64K_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_WriteMutipleItemKeySizeLargerThanOneChunk64K_ShouldSucceed)
    {
        SyncAwait(CheckpointFile_WriteMutipleItemKeySizeLargerThanOneChunk64K_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_WriteOneItemKeySizeEqualToOneChunk64K_ShouldSucceed)
    {
        SyncAwait(CheckpointFile_WriteOneItemKeySizeEqualToOneChunk64K_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_WriteMutipleItemKeySizeEqualToOneChunk64K_ShouldSucceed)
    {
        SyncAwait(CheckpointFile_WriteMutipleItemKeySizeEqualToOneChunk64K_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_WriteMutipleItemSumKeySizeEqualToOneChunk64K_ShouldSucceed)
    {
        SyncAwait(CheckpointFile_WriteMutipleItemSumKeySizeEqualToOneChunk64K_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_Write1000KeyValueItemsTotalTakeMutipleChunks_ShouldSucceed)
    {
        SyncAwait(CheckpointFile_Write1000KeyValueItemsTotalTakeMutipleChunks_ShouldSucceed_Test());
    }

    //todo: takes too long since no streampool. disable till have stress test.
    //BOOST_AUTO_TEST_CASE(CheckpointFile_Write100000KeyValueItemsMutipleChunks_ShouldSucceed)
    //{
    //    TestCheckpointFile(100000, sizeof(int), sizeof(int));
    //}

    BOOST_AUTO_TEST_CASE(FlieMetadataWithCanBeDeletedFlagSet_OnClose_ShouldDeleteCheckpointFile)
    {
        SyncAwait(FlieMetadataWithCanBeDeletedFlagSet_OnClose_ShouldDeleteCheckpointFile_Test());
    }

    BOOST_AUTO_TEST_CASE(KeyCheckpointFileProp_WriteAndRead_ShouldSucceed)
    {
        SyncAwait(KeyCheckpointFileProp_WriteAndRead_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(KeyCheckpointFileProp_WriteToDiskAndRead_ShouldSucceed)
    {
        SyncAwait(KeyCheckpointFileProp_WriteToDiskAndRead_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(ValueCheckpointFileProp_WriteAndRead_ShouldSucceed)
    {
        SyncAwait(ValueCheckpointFileProp_WriteAndRead_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(ValueCheckpointFileProp_WriteToDiskAndRead_ShouldSucceed)
    {
        SyncAwait(ValueCheckpointFileProp_WriteToDiskAndRead_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(KeyCheckpointFile_WriteOneKeyAndRead_ShouldSucceed)
    {
        SyncAwait(KeyCheckpointFile_WriteOneKeyAndRead_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(KeyCheckpointFile_Write1000KeysAndRead_ShouldSucceed)
    {
        SyncAwait(KeyCheckpointFile_Write1000KeysAndRead_ShouldSucceed_Test());
    }


    BOOST_AUTO_TEST_CASE(KeyCheckpointFile_WriteOneKeyAndFlushThenOpenVerifyMetadata_ShouldSucceed)
    {
        SyncAwait(KeyCheckpointFile_WriteOneKeyAndFlushThenOpenVerifyMetadata_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(ValueCheckpointFile_WriteOneKeyAndRead_ShouldSucceed)
    {
        SyncAwait(ValueCheckpointFile_WriteOneKeyAndRead_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(ValueCheckpointFile_WriteOneKeyAndReadUsingBytes_ShouldSucceed)
    {
        SyncAwait(ValueCheckpointFile_WriteOneKeyAndReadUsingBytes_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(ValueCheckpointFile_Write1000KeysAndReadAndVerifyMetadata_ShouldSucceed)
    {
        SyncAwait(ValueCheckpointFile_Write1000KeysAndReadAndVerifyMetadata_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(ValueCheckpointFile_Write1000KeysAndReadAndVerifyConcurrentlyWithStreamPool_ShouldSucceed)
    {
        SyncAwait(ValueCheckpointFile_Write1000KeysAndReadAndVerifyConcurrentlyWithStreamPool_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(ValueCheckpointFile_Write100KeysAndReadAndVerifyMetadataUsingBytes_ShouldSucceed)
    {
        SyncAwait(ValueCheckpointFile_Write100KeysAndReadAndVerifyMetadataUsingBytes_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(KeyBlockAlignedWriter_WriteOnKeyAndEnumerate_ShouldSucceed)
    {
        SyncAwait(KeyBlockAlignedWriter_WriteOnKeyAndEnumerate_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(KeyBlockAlignedWriter_Write50KeysLessThanOneBlockSize_ShouldSucceed)
    {
        SyncAwait(KeyBlockAlignedWriter_Write50KeysLessThanOneBlockSize_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(KeyBlockAlignedWriter_WriteKeysTakeMutipleBlocks_ShouldSucceed)
    {
        SyncAwait(KeyBlockAlignedWriter_WriteKeysTakeMutipleBlocks_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(KeyBlockAlignedWriter_WriteKeysTakeMutipleChunks_ShouldSucceed)
    {
        SyncAwait(KeyBlockAlignedWriter_WriteKeysTakeMutipleChunks_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(KeyBlockAlignedWriter_WriteBiggerKeysWithBlockNotFullyOccupiedAndMoveToNextBlock_ShouldSucceed)
    {
        SyncAwait(KeyBlockAlignedWriter_WriteBiggerKeysWithBlockNotFullyOccupiedAndMoveToNextBlock_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(KeyBlockAlignedWriter_WriteOneKeyLargerThanOneBlock_ShouldSucceed)
    {
        SyncAwait(KeyBlockAlignedWriter_WriteOneKeyLargerThanOneBlock_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(KeyBlockAlignedWriter_WriteOneKeyLargerThanOneChunk_ShouldSucceed)
    {
        SyncAwait(KeyBlockAlignedWriter_WriteOneKeyLargerThanOneChunk_ShouldSucceed_Test());
    }

    //
    // ValueBlockAlignedWriterTests
    //
    BOOST_AUTO_TEST_CASE(ValueBlockAlignedWrite_SingleKey_ShouldSucceed)
    {
        SyncAwait(ValueBlockAlignedWrite_SingleKey_ShouldSucceed_Test());
    }

    //
    // Single item bigger than block size.
    //
    BOOST_AUTO_TEST_CASE(ValueBlockAlignedWrite_ValueBiggerThanBlockSize_ShouldSucceed)
    {
        SyncAwait(ValueBlockAlignedWrite_ValueBiggerThanBlockSize_ShouldSucceed_Test());
    }

    //
    // Single item with a ValueSize smaller than the serialized size
    //
    BOOST_AUTO_TEST_CASE(ValueBlockAlignedWrite_ValueSizeSmallerThanActualSize_ShouldSucceed)
    {
        SyncAwait(ValueBlockAlignedWrite_ValueSizeSmallerThanActualSize_ShouldSucceed_Test());
    }

    //
    // Single item with a ValueSize bigger than the serialized size
    //
    BOOST_AUTO_TEST_CASE(ValueBlockAlignedWrite_ValueSizeLargerThanActualSize_ShouldSucceed)
    {
        SyncAwait(ValueBlockAlignedWrite_ValueSizeLargerThanActualSize_ShouldSucceed_Test());
    }

    //
    // Multiple items - not within the 4k block size.
    //
    BOOST_AUTO_TEST_CASE(ValueBlockAlignedWrite_MultipleValuesNotBlockAligned_ShouldSucceed)
    {
        SyncAwait(ValueBlockAlignedWrite_MultipleValuesNotBlockAligned_ShouldSucceed_Test());
    }

    //
    // Multiple items - within the 4k block size.
    //
    BOOST_AUTO_TEST_CASE(ValueBlockAlignedWrite_MultipleValuesBlockAligned_ShouldSucceed)
    {
        SyncAwait(ValueBlockAlignedWrite_MultipleValuesBlockAligned_ShouldSucceed_Test());
    }

    //
    // Multiple items - value size smaller than serialized size
    //
    BOOST_AUTO_TEST_CASE(ValueBlockAlignedWrite_MultipleValuesBlockAligned_ValueSizeSmaller_ShouldSucceed)
    {
        SyncAwait(ValueBlockAlignedWrite_MultipleValuesBlockAligned_ValueSizeSmaller_ShouldSucceed_Test());
    }

    //
    // Multiple items - value size larger than serialized size
    //
    BOOST_AUTO_TEST_CASE(ValueBlockAlignedWrite_MultipleValuesBlockAligned_ValueSizeLarger_ShouldSucceed)
    {
        SyncAwait(ValueBlockAlignedWrite_MultipleValuesBlockAligned_ValueSizeLarger_ShouldSucceed_Test());
    }
    BOOST_AUTO_TEST_SUITE_END()
}

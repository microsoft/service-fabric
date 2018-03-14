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

        void CloseFileStream(bool remove = true)
        {
            if (fakeStreamSPtr_ != nullptr)
            {
                NTSTATUS status = SyncAwait(fakeStreamSPtr_->CloseAsync());
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

        KString::SPtr CreateFileStream(__in KStringView& name, __in bool overwriteExisting = true)
        {
            KString::SPtr fileName = CreateFileString(name, GetAllocator());
            //todo: remove this till fileformattesthelper api is updated.
            KWString filePath(GetAllocator(), *fileName);

            if (overwriteExisting)
            {
                NTSTATUS status = SyncAwait(FileFormatTestHelper::ForceCreateNewBlockFile(filePath, GetAllocator(), CancellationToken::None, fakeFileSPtr, fakeStreamSPtr_));
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }
            else
            {
                NTSTATUS status = SyncAwait(FileFormatTestHelper::CreateBlockFile(filePath, GetAllocator(), CancellationToken::None, fakeFileSPtr, fakeStreamSPtr_));
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            NTSTATUS status = SyncAwait(fakeStreamSPtr_->OpenAsync(*fakeFileSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            currentFilePath_ = fileName;

            return fileName;
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

        void CheckpointReadAndVerifyKeysWithInsertedVersionedItem(
            __in IEnumerator<KeyValuePair<KBuffer::SPtr, KSharedPtr<VersionedItem<KBuffer::SPtr>>>>& items,
            __in CheckpointFile& file,
            __in KBufferSerializer& keyStateSerializer,
            __in KBufferSerializer& valSerializer)
            {
            KeyCheckpointFileAsyncEnumerator<KBuffer::SPtr, KBuffer::SPtr>::SPtr keyEnumSPtr = 
                file.GetAsyncEnumerator<KBuffer::SPtr, KBuffer::SPtr>(keyStateSerializer);
            ULONG32 index = 0;
            bool isEndOfEnumerator = false;
            while (SyncAwait(keyEnumSPtr->MoveNextAsync(CancellationToken::None)))
            {
               items.MoveNext();
               KSharedPtr<VersionedItem<KBuffer::SPtr>> itemSPtr = items.Current().Value;

                KeyData<KBuffer::SPtr, KBuffer::SPtr>::SPtr keyDataSPtr = keyEnumSPtr->GetCurrent();
                KBuffer::SPtr val = SyncAwait(file.ReadValueAsync(*(keyDataSPtr->Value), valSerializer));
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

            SyncAwait(keyEnumSPtr->CloseAsync());
            CODING_ERROR_ASSERT(isEndOfEnumerator);
        }

        void TestCheckpointFile(
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

            CheckpointFile::SPtr checkpointFileSPtr = SyncAwait(
                CheckpointFile::CreateAsync<KBuffer::SPtr, KBuffer::SPtr>(
                    1,
                    *filePath,
                    *writeEnumerator,
                    *stateSerializerSPtr,
                    *stateSerializerSPtr,
                    1,
                    allocator,
                    *CreateTraceComponent(),
                    false));

            IEnumerator<KeyValuePair<KBuffer::SPtr, KSharedPtr<VersionedItem<KBuffer::SPtr>>>>::SPtr readEnumerator;
            status = KSharedArrayEnumerator<KeyValuePair<KBuffer::SPtr, VersionedItem<KBuffer::SPtr>::SPtr>>::Create(*itemsArraySPtr, GetAllocator(), readEnumerator);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            CheckpointReadAndVerifyKeysWithInsertedVersionedItem(*readEnumerator, *checkpointFileSPtr, *stateSerializerSPtr, *stateSerializerSPtr);

            CheckpointFile::SPtr checkpointFileOpenedSPtr = SyncAwait(CheckpointFile::OpenAsync(*filePath, *CreateTraceComponent(), allocator, false));
            CODING_ERROR_ASSERT(checkpointFileOpenedSPtr->ValueCount == numOfItems);
            CODING_ERROR_ASSERT(checkpointFileOpenedSPtr->KeyCount == numOfItems);

            //clean up 
            SyncAwait(checkpointFileSPtr->CloseAsync());
            SyncAwait(checkpointFileOpenedSPtr->CloseAsync());
            KWString keyFilePathToRemove(allocator, *filePath);
            KWString valueFilePathToRemove(allocator, *filePath);
            keyFilePathToRemove += KeyCheckpointFile::GetFileExtension();
            valueFilePathToRemove += ValueCheckpointFile::GetFileExtension();
            KVolumeNamespace::DeleteFileOrDirectory(keyFilePathToRemove, allocator, nullptr);
            KVolumeNamespace::DeleteFileOrDirectory(valueFilePathToRemove, allocator, nullptr);      
        }
#pragma endregion

#pragma region keyblockalignedwriter utils
        void BlockAlignedWriteKeyWithInsertedVersionedItem(
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
                SyncAwait(writer.BlockAlignedWriteKeyAsync(itemToAdd));
            }
        }

        int BlockAlignedReadAndVerifyKeysWithInsertedVersionedItem(
            __in KeyCheckpointFileAsyncEnumerator<KBuffer::SPtr, int>& enumerator,
            __in int startVal,
            __in int keySerializedSize)
        {
            KeyCheckpointFileAsyncEnumerator<KBuffer::SPtr, int>::SPtr keyEnumSPtr(&enumerator);
            int count = 0;
            while (SyncAwait(keyEnumSPtr->MoveNextAsync(CancellationToken::None)))
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

            return count;
        }

        void TestKeyBlockAlignedWriteAndEnumerator(
            __in int numberOfKeys, 
            __in int keySize, 
            __in KBufferSerializer& serializer)
        {
            KAllocator& allocator = GetAllocator();
            KStringView filename = L"BlockAlignedWriteEnumerateTests.txt";
            KString::SPtr filePathToOpenSPtr = CreateFileString(filename, GetAllocator());
            
            KeyCheckpointFile::SPtr fileSPtr = SyncAwait(KeyCheckpointFile::CreateAsync(*CreateTraceComponent(), *filePathToOpenSPtr, false, 10, allocator));

            SharedBinaryWriter::SPtr bwSPtr = nullptr;
            NTSTATUS status = SharedBinaryWriter::Create(allocator, bwSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            ktl::io::KFileStream::SPtr streamSPtr = SyncAwait(fileSPtr->StreamPoolSPtr->AcquireStreamAsync());

            KeyBlockAlignedWriter<KBuffer::SPtr, int>::SPtr keyWriterSPtr = nullptr;
            status = KeyBlockAlignedWriter<KBuffer::SPtr, int>::Create(*streamSPtr, *fileSPtr, *bwSPtr, serializer, 0, *CreateTraceComponent(), allocator, keyWriterSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            int startVal = 99;
            BlockAlignedWriteKeyWithInsertedVersionedItem(*keyWriterSPtr, startVal, numberOfKeys, keySize);

            SyncAwait(keyWriterSPtr->FlushAsync());
            SyncAwait(fileSPtr->StreamPoolSPtr->ReleaseStreamAsync(*streamSPtr));

            KeyCheckpointFile::SPtr keyCheckpointFileSPtr = SyncAwait(KeyCheckpointFile::OpenAsync(allocator, *filePathToOpenSPtr, *CreateTraceComponent(), true));
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

            int count = BlockAlignedReadAndVerifyKeysWithInsertedVersionedItem(*keyEnumSPtr, startVal, keySize);
            CODING_ERROR_ASSERT(count == numberOfKeys);
            SyncAwait(keyEnumSPtr->CloseAsync());
            
            SyncAwait(fileSPtr->CloseAsync());
            SyncAwait(keyCheckpointFileSPtr->CloseAsync());
            RemoveFile(*filePathToOpenSPtr);
        }
#pragma endregion

#pragma region ValueBlockAlignedWriter utils
        void CreateValueCheckpointFileStream(
            __in const WCHAR* fileName,
            __out ValueCheckpointFile::SPtr & fileSPtr,
            __out SharedBinaryWriter::SPtr & binaryWriterSPtr,
            __out ktl::io::KFileStream::SPtr & streamSPtr)
        {
            KAllocator& allocator = GetAllocator();
            KStringView filename = fileName;
            KString::SPtr filePathToOpenSPtr = CreateFileString(filename, GetAllocator());

            fileSPtr = SyncAwait(ValueCheckpointFile::CreateAsync(*CreateTraceComponent(), *filePathToOpenSPtr, 10, allocator));

            NTSTATUS status = SharedBinaryWriter::Create(allocator, binaryWriterSPtr);
            Diagnostics::Validate(status);

            streamSPtr = SyncAwait(fileSPtr->StreamPoolSPtr->AcquireStreamAsync());
        }
        
        void CreateBlockAlignedWriter(
            __in const WCHAR* filename,
            __out ValueCheckpointFile::SPtr & fileSPtr,
            __out SharedBinaryWriter::SPtr & binaryWriterSPtr,
            __out ktl::io::KFileStream::SPtr & streamSPtr,
            __out KBufferSerializer::SPtr & stateSerializer,
            __out ValueBlockAlignedWriter<ULONG32, KBuffer::SPtr>::SPtr & valBlockAlignedWriterSPtr)
        {
            KAllocator& allocator = GetAllocator();

            CreateValueCheckpointFileStream(filename, fileSPtr, binaryWriterSPtr, streamSPtr);

            auto status = KBufferSerializer::Create(allocator, stateSerializer);
            Diagnostics::Validate(status);

            status = ValueBlockAlignedWriter<ULONG32, KBuffer::SPtr>::Create(*streamSPtr, *fileSPtr, *binaryWriterSPtr, *stateSerializer, *CreateTraceComponent(), allocator, valBlockAlignedWriterSPtr);
            Diagnostics::Validate(status);
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

         void TestBlockWriteSingleItem(
             __in const WCHAR * filename,
             __in ULONG32 bufferSize, // The actual serialized size
             __in ULONG32 valueSize)   // The stated serialized size
         {
            ValueCheckpointFile::SPtr fileSPtr = nullptr;
            SharedBinaryWriter::SPtr bwSPtr = nullptr;
            ktl::io::KFileStream::SPtr streamSPtr = nullptr;
            KBufferSerializer::SPtr stateSerializer = nullptr;
            ValueBlockAlignedWriter<ULONG32, KBuffer::SPtr>::SPtr valBlockAlignedWriterSPtr = nullptr;

            CreateBlockAlignedWriter(
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
            SyncAwait(valBlockAlignedWriterSPtr->BlockAlignedWriteValueAsync(keyValuePair, nullptr, true));
            SyncAwait(valBlockAlignedWriterSPtr->FlushAsync());
            SyncAwait(fileSPtr->FlushAsync(*streamSPtr, *bwSPtr));
            NTSTATUS status = SyncAwait(streamSPtr->CloseAsync());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            KBuffer::SPtr resultBufferSPtr = SyncAwait(fileSPtr->ReadValueAsync<KSharedPtr<KBuffer>>(*itemSPtr, *stateSerializer));

            byte* result = static_cast<byte*>(resultBufferSPtr->GetBuffer());
            CODING_ERROR_ASSERT(resultBufferSPtr->QuerySize() == bufferSize);
            for (ULONG32 i = 0; i < bufferSize; i++)
            {
                CODING_ERROR_ASSERT(result[i] == (byte)1);
            }
            
            SyncAwait(fileSPtr->StreamPoolSPtr->ReleaseStreamAsync(*streamSPtr));
            SyncAwait(fileSPtr->CloseAsync());
            Common::File::Delete(filename);
         }

        KSharedPtr<KSharedArray<InsertedVersionedItem<KBuffer::SPtr>::SPtr>>AlignWriteMultipleItems(
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
                SyncAwait(blockAlignedWriter.BlockAlignedWriteValueAsync(keyValuePair, nullptr, true));
            }

            return itemsArray;
        }

        void TestBlockWriteMultipleItems(
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

            CreateBlockAlignedWriter(
                filename,
                fileSPtr,
                bwSPtr,
                streamSPtr,
                stateSerializer,
                valBlockAlignedWriterSPtr
            );

            auto itemsArray = AlignWriteMultipleItems(numItems, eachItemBufferSize, eachItemValueSize, *valBlockAlignedWriterSPtr);
            SyncAwait(valBlockAlignedWriterSPtr->FlushAsync());

            SyncAwait(fileSPtr->FlushAsync(*streamSPtr, *bwSPtr));
            NTSTATUS status = SyncAwait(streamSPtr->CloseAsync());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            for (ULONG32 i = 0; i < itemsArray->Count(); i++)
            {
                VersionedItem<KBuffer::SPtr>::SPtr itemSPtr((*itemsArray)[i].RawPtr());
                KBuffer::SPtr resultBufferSPtr = SyncAwait(fileSPtr->ReadValueAsync<KSharedPtr<KBuffer>>(*itemSPtr, *stateSerializer));

                byte* result = static_cast<byte*>(resultBufferSPtr->GetBuffer());
                CODING_ERROR_ASSERT(resultBufferSPtr->QuerySize() == eachItemBufferSize);
                for (ULONG32 j = 0; j < eachItemBufferSize; j++)
                {
                    CODING_ERROR_ASSERT(result[j] == (byte)1);
                }
            }
            
            SyncAwait(fileSPtr->CloseAsync());
            Common::File::Delete(filename);
        }
#pragma endregion
    private:
        KtlSystem* ktlSystem_;
        KBlockFile::SPtr fakeFileSPtr = nullptr;
        ktl::io::KFileStream::SPtr fakeStreamSPtr_ = nullptr;
        KString::SPtr currentFilePath_;
    };

    BOOST_FIXTURE_TEST_SUITE(CheckpointFilePropertiesTestSuite, CheckpointFileTest)
    
    BOOST_AUTO_TEST_CASE(CheckpointFile_WriteOneItemKeySizeBiggerThanOneBlock4K_ShouldSucceed)
    {
        TestCheckpointFile(1, 4084, 4100);
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_WriteOneItemKeySizeLargerThanOneChunk64K_ShouldSucceed)
    {
        TestCheckpointFile(1, 32772, 32768);
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_WriteMutipleItemKeySizeLargerThanOneChunk64K_ShouldSucceed)
    {
        TestCheckpointFile(100, 32772, 32768);
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_WriteOneItemKeySizeEqualToOneChunk64K_ShouldSucceed)
    {
        //32708 + 44 reserved + 16bytes key chunk metatdata = 32*1024 bytes (1 chunk)
        TestCheckpointFile(1, 32708, 32764);
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_WriteMutipleItemKeySizeEqualToOneChunk64K_ShouldSucceed)
    {
        //32708 + 44 reserved + 16bytes key chunk metatdata = 32*1024 bytes (1 chunk)
        TestCheckpointFile(100, 32708, 32764);
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_WriteMutipleItemSumKeySizeEqualToOneChunk64K_ShouldSucceed)
    {
        TestCheckpointFile(2, 16324, 32764);
    }

    BOOST_AUTO_TEST_CASE(CheckpointFile_Write1000KeyValueItemsTotalTakeMutipleChunks_ShouldSucceed)
    {
        TestCheckpointFile(1000, sizeof(int), sizeof(int));
    }

    //todo: takes too long since no streampool. disable till have stress test.
    //BOOST_AUTO_TEST_CASE(CheckpointFile_Write100000KeyValueItemsMutipleChunks_ShouldSucceed)
    //{
    //    TestCheckpointFile(100000, sizeof(int), sizeof(int));
    //}

    BOOST_AUTO_TEST_CASE(FlieMetadataWithCanBeDeletedFlagSet_OnClose_ShouldDeleteCheckpointFile)
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

        CheckpointFile::SPtr checkpointFileSPtr = SyncAwait(
            CheckpointFile::CreateAsync<KBuffer::SPtr, KBuffer::SPtr>(
                1,
                *filePath,
                *itemsEnumerator,
                *stateSerializerSPtr,
                *stateSerializerSPtr,
                1,
                allocator,
                *CreateTraceComponent(),
                false));

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

        SyncAwait(fileMetadataSPtr->CloseAsync());
        CODING_ERROR_ASSERT(!Common::File::Exists((*keyFilePathToRemoveSPtr).operator LPCWSTR()));
        CODING_ERROR_ASSERT(!Common::File::Exists((*valueFilePathToRemoveSPtr).operator LPCWSTR()));
    }

    BOOST_AUTO_TEST_CASE(KeyCheckpointFileProp_WriteAndRead_ShouldSucceed)
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
    }

    BOOST_AUTO_TEST_CASE(KeyCheckpointFileProp_WriteToDiskAndRead_ShouldSucceed)
    {
        KAllocator& allocator = GetAllocator();
        KStringView filename = L"KeyCheckpointFileProp_WriteToDiskAndRead_ShouldSucceed.txt";
        CreateFileStream(filename);

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

        //write to disk.
        ktl::io::KFileStream::SPtr fileStreamSPtr = GetFileStream();
        status = SyncAwait(fileStreamSPtr->WriteAsync(*bw.GetBuffer(0)));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        status = SyncAwait(fileStreamSPtr->FlushAsync());
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        CloseFileStream(false);

        //read from disk.
        CreateFileStream(filename, false);
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
        status = SyncAwait(fileStreamSPtr->ReadAsync(*buffer, bytesRead, offsetToRead, bytesToRead));
        CODING_ERROR_ASSERT(bytesRead == bytesToRead);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        BinaryReader br(*buffer, allocator);

        auto p = FilePropertySection::Read<KeyCheckpointFileProperties>(br, *bh, allocator);

        CODING_ERROR_ASSERT(prop->KeyCount == p->KeyCount);
        CODING_ERROR_ASSERT(prop->FileId == p->FileId);
        CODING_ERROR_ASSERT(prop->KeysHandle->Offset == offset);
        CODING_ERROR_ASSERT(prop->KeysHandle->Size == size);

        CloseFileStream();
    }

    BOOST_AUTO_TEST_CASE(ValueCheckpointFileProp_WriteAndRead_ShouldSucceed)
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
    }

    BOOST_AUTO_TEST_CASE(ValueCheckpointFileProp_WriteToDiskAndRead_ShouldSucceed)
    {
        KAllocator& allocator = GetAllocator();
        KStringView filename = L"ValueCheckpointFileProp_WriteToDiskAndRead_ShouldSucceed.txt";
        CreateFileStream(filename);

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

        //write to disk.
        ktl::io::KFileStream::SPtr fileStreamSPtr = GetFileStream();
        status = SyncAwait(fileStreamSPtr->WriteAsync(*bw.GetBuffer(0)));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        status = SyncAwait(fileStreamSPtr->FlushAsync());
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        CloseFileStream(false);

        //read from disk.
        CreateFileStream(filename, false);
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
        status = SyncAwait(fileStreamSPtr->ReadAsync(*bufferSPtr, bytesRead, offsetToRead, bytesToRead));
        CODING_ERROR_ASSERT(bytesRead == bytesToRead);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        BinaryReader br(*bufferSPtr, allocator);

        auto p = FilePropertySection::Read<ValueCheckpointFileProperties>(br, *bh, allocator);

        CODING_ERROR_ASSERT(prop->ValueCount == p->ValueCount);
        CODING_ERROR_ASSERT(prop->FileId == p->FileId);
        CODING_ERROR_ASSERT(prop->ValuesHandle->Offset == offset);
        CODING_ERROR_ASSERT(prop->ValuesHandle->Size == size);

        CloseFileStream();
    }

    BOOST_AUTO_TEST_CASE(KeyCheckpointFile_WriteOneKeyAndRead_ShouldSucceed)
    {
        KAllocator& allocator = GetAllocator();
        KStringView filename = L"KeyCheckpointFile_WriteOneKeyAndRead_ShouldSucceed.txt";
        KString::SPtr filePathToOpenSPtr = CreateFileString(filename, GetAllocator());

        KeyCheckpointFile::SPtr fileSPtr = SyncAwait(KeyCheckpointFile::CreateAsync(*CreateTraceComponent(), *filePathToOpenSPtr, false, 10, allocator));
        BinaryWriter bw(allocator);

        TestStateSerializer<int>::SPtr stateSerializer = nullptr;
        NTSTATUS status = TestStateSerializer<int>::Create(allocator, stateSerializer);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        int key = 99;
        WriteKeyWithInsertedVersionedItem(bw, *fileSPtr, key, *stateSerializer);

        BinaryReader br(*bw.GetBuffer(0), allocator);
        ReadAndVerifyKeysWithInsertedVersionedItem(br, *fileSPtr, key, *stateSerializer);
        SyncAwait(fileSPtr->CloseAsync());
        RemoveFile(*filePathToOpenSPtr);
    }

    BOOST_AUTO_TEST_CASE(KeyCheckpointFile_Write1000KeysAndRead_ShouldSucceed)
    {
        KAllocator& allocator = GetAllocator();
        KStringView filename = L"KeyCheckpointFile_Write1000KeysAndRead_ShouldSucceed.txt";
        KString::SPtr filePathToOpenSPtr = CreateFileString(filename, GetAllocator());

        KeyCheckpointFile::SPtr fileSPtr = SyncAwait(KeyCheckpointFile::CreateAsync(*CreateTraceComponent(), *filePathToOpenSPtr, false, 10, allocator));
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

        SyncAwait(fileSPtr->CloseAsync());
        RemoveFile(*filePathToOpenSPtr);
    }


    BOOST_AUTO_TEST_CASE(KeyCheckpointFile_WriteOneKeyAndFlushThenOpenVerifyMetadata_ShouldSucceed)
    {
        KAllocator& allocator = GetAllocator();
        KStringView filename = L"KeyCheckpointFile_WriteOneKeyAndFlushThenOpenVerifyMetadata_ShouldSucceed.txt";
        KString::SPtr filePathToOpenSPtr = CreateFileString(filename, GetAllocator());

        ULONG32 fileId = 10;
        KeyCheckpointFile::SPtr fileSPtr = SyncAwait(KeyCheckpointFile::CreateAsync(*CreateTraceComponent(), *filePathToOpenSPtr, false, fileId, allocator));
        SharedBinaryWriter::SPtr bwSptr = nullptr;
        NTSTATUS status = SharedBinaryWriter::Create(allocator, bwSptr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        TestStateSerializer<int>::SPtr stateSerializer = nullptr;
        status = TestStateSerializer<int>::Create(allocator, stateSerializer);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        int key = 1;
        WriteKeyWithInsertedVersionedItem(*bwSptr, *fileSPtr, key, *stateSerializer);

        ktl::io::KFileStream::SPtr streamSPtr = SyncAwait(fileSPtr->StreamPoolSPtr->AcquireStreamAsync());
        SyncAwait(fileSPtr->FlushAsync(*streamSPtr, *bwSptr));
        SyncAwait(fileSPtr->StreamPoolSPtr->ReleaseStreamAsync(*streamSPtr));

        KeyCheckpointFile::SPtr keyCheckpointFileSPtr = SyncAwait(KeyCheckpointFile::OpenAsync(allocator, *filePathToOpenSPtr, *CreateTraceComponent(), true));
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
        ktl::io::KFileStream::SPtr streamReadSPtr = SyncAwait(keyCheckpointFileSPtr->StreamPoolSPtr->AcquireStreamAsync());
        status = SyncAwait(streamReadSPtr->ReadAsync(*bufferSPtr, bytesRead, 0, bytesToRead));
        SyncAwait(keyCheckpointFileSPtr->StreamPoolSPtr->ReleaseStreamAsync(*streamReadSPtr));

        CODING_ERROR_ASSERT(bytesRead == bytesToRead);

        BinaryReader br(*bufferSPtr, allocator);
        ReadAndVerifyKeysWithInsertedVersionedItem(br, *fileSPtr, key, *stateSerializer);
        
        SyncAwait(fileSPtr->CloseAsync());
        SyncAwait(keyCheckpointFileSPtr->CloseAsync());
        RemoveFile(*filePathToOpenSPtr);
    }

    BOOST_AUTO_TEST_CASE(ValueCheckpointFile_WriteOneKeyAndRead_ShouldSucceed)
    {
        KAllocator& allocator = GetAllocator();
        KStringView filename = L"ValueCheckpointFile_WriteOneKeyAndRead_ShouldSucceed.txt";
        KString::SPtr filePathToOpenSPtr = CreateFileString(filename, GetAllocator());

        ValueCheckpointFile::SPtr fileSPtr = SyncAwait(ValueCheckpointFile::CreateAsync(*CreateTraceComponent(), *filePathToOpenSPtr, 10, allocator));

        SharedBinaryWriter::SPtr bwSPtr = nullptr;
        NTSTATUS status = SharedBinaryWriter::Create(allocator, bwSPtr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        ktl::io::KFileStream::SPtr streamSPtr = SyncAwait(fileSPtr->StreamPoolSPtr->AcquireStreamAsync());
        TestStateSerializer<int>::SPtr stateSerializer = nullptr;
        status = TestStateSerializer<int>::Create(allocator, stateSerializer);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        int value = 99;
        KSharedPtr<VersionedItem<int>> itemSPtr = AddValuesWithInsertedVersionedItem(*streamSPtr, *bwSPtr, *fileSPtr, value, *stateSerializer);

        SyncAwait(fileSPtr->FlushAsync(*streamSPtr, *bwSPtr));
        SyncAwait(fileSPtr->StreamPoolSPtr->ReleaseStreamAsync(*streamSPtr));

        int val = SyncAwait(fileSPtr->ReadValueAsync<int>(*itemSPtr, *stateSerializer));
        CODING_ERROR_ASSERT(val== value);
   
        SyncAwait(fileSPtr->CloseAsync());
        RemoveFile(*filePathToOpenSPtr);
    }

    BOOST_AUTO_TEST_CASE(ValueCheckpointFile_WriteOneKeyAndReadUsingBytes_ShouldSucceed)
    {
        KAllocator& allocator = GetAllocator();
        KStringView filename = L"ValueCheckpointFile_WriteOneKeyAndReadUsingBytes_ShouldSucceed.txt";
        KString::SPtr filePathToOpenSPtr = CreateFileString(filename, GetAllocator());

        ValueCheckpointFile::SPtr fileSPtr = SyncAwait(ValueCheckpointFile::CreateAsync(*CreateTraceComponent(), *filePathToOpenSPtr, 10, allocator));
        SharedBinaryWriter::SPtr bwSPtr = nullptr;
        NTSTATUS status = SharedBinaryWriter::Create(allocator, bwSPtr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        ktl::io::KFileStream::SPtr streamSPtr = SyncAwait(fileSPtr->StreamPoolSPtr->AcquireStreamAsync());

        byte value = 99;
        BinaryWriter br(allocator);
        br.Write(value);
        KBuffer::SPtr keydataSPtr = br.GetBuffer(0);
        KSharedPtr<VersionedItem<int>> itemSPtr = AddValuesInBytesWithInsertedVersionedItem(*streamSPtr, *bwSPtr, *fileSPtr, value, *keydataSPtr);

        SyncAwait(fileSPtr->FlushAsync(*streamSPtr, *bwSPtr));
        SyncAwait(fileSPtr->StreamPoolSPtr->ReleaseStreamAsync(*streamSPtr));

        KBuffer::SPtr val = SyncAwait(fileSPtr->ReadValueAsync<int>(*itemSPtr));
        CODING_ERROR_ASSERT(val->QuerySize() == keydataSPtr->QuerySize());
        CODING_ERROR_ASSERT(IsSameBufferValue(*val, *keydataSPtr));

        SyncAwait(fileSPtr->CloseAsync());
        RemoveFile(*filePathToOpenSPtr);
    }

    BOOST_AUTO_TEST_CASE(ValueCheckpointFile_Write1000KeysAndReadAndVerifyMetadata_ShouldSucceed)
    {
        KAllocator& allocator = GetAllocator();
        KStringView filename = L"ValueCheckpointFile_Write1000KeysAndReadAndVerifyMetadata_ShouldSucceed.txt";
        KString::SPtr filePathToOpenSPtr = CreateFileString(filename, GetAllocator());

        ULONG32 fileId = 10;
        ValueCheckpointFile::SPtr fileSPtr = SyncAwait(ValueCheckpointFile::CreateAsync(*CreateTraceComponent(), *filePathToOpenSPtr, fileId, allocator));
        SharedBinaryWriter::SPtr bwSPtr = nullptr;
        NTSTATUS status = SharedBinaryWriter::Create(allocator, bwSPtr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        ktl::io::KFileStream::SPtr streamSPtr = SyncAwait(fileSPtr->StreamPoolSPtr->AcquireStreamAsync());
        TestStateSerializer<int>::SPtr stateSerializer = nullptr;
        status = TestStateSerializer<int>::Create(allocator, stateSerializer);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        KArray<KSharedPtr<VersionedItem<int>>> itemList(allocator);
        for (int i = 0; i < 1000; i++)
        {
            KSharedPtr<VersionedItem<int>> itemSPtr = AddValuesWithInsertedVersionedItem(*streamSPtr, *bwSPtr, *fileSPtr, i, *stateSerializer);
            itemList.Append(itemSPtr);
        }

        SyncAwait(fileSPtr->FlushAsync(*streamSPtr, *bwSPtr));
        SyncAwait(fileSPtr->StreamPoolSPtr->ReleaseStreamAsync(*streamSPtr));

        ValueCheckpointFile::SPtr valueCheckpointFileSPtr = SyncAwait(ValueCheckpointFile::OpenAsync(allocator, *filePathToOpenSPtr, *CreateTraceComponent()));
        CODING_ERROR_ASSERT(valueCheckpointFileSPtr != nullptr);
        CODING_ERROR_ASSERT(valueCheckpointFileSPtr->PropertiesSPtr->ValueCount == 1000);
        CODING_ERROR_ASSERT(valueCheckpointFileSPtr->PropertiesSPtr->FileId == fileId);

        CODING_ERROR_ASSERT(valueCheckpointFileSPtr->PropertiesSPtr->ValuesHandle->get_Offset() == 0);
        CODING_ERROR_ASSERT(valueCheckpointFileSPtr->PropertiesSPtr->ValuesHandle->get_Size() == 4 * 1000); //int 4 bytes, 1000 is 4000 bytes size.

        for (int i = 0; i < 1000; i++)
        {
            int val = SyncAwait(fileSPtr->ReadValueAsync<int>(*itemList[i], *stateSerializer));
            CODING_ERROR_ASSERT(val == i);
        }
        
        SyncAwait(fileSPtr->CloseAsync());
        SyncAwait(valueCheckpointFileSPtr->CloseAsync());
        RemoveFile(*filePathToOpenSPtr);
    }

    BOOST_AUTO_TEST_CASE(ValueCheckpointFile_Write1000KeysAndReadAndVerifyConcurrentlyWithStreamPool_ShouldSucceed)
    {
        KAllocator& allocator = GetAllocator();
        KStringView filename = L"ValueCheckpointFile_Write1000KeysAndReadAndVerifyConcurrentlyWithStreamPool_ShouldSucceed.txt";
        KString::SPtr filePathToOpenSPtr = CreateFileString(filename, GetAllocator());

        ULONG32 fileId = 10;
        ValueCheckpointFile::SPtr fileSPtr = SyncAwait(ValueCheckpointFile::CreateAsync(*CreateTraceComponent(), *filePathToOpenSPtr, fileId, allocator));
        SharedBinaryWriter::SPtr bwSPtr = nullptr;
        NTSTATUS status = SharedBinaryWriter::Create(allocator, bwSPtr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        ktl::io::KFileStream::SPtr streamSPtr = SyncAwait(fileSPtr->StreamPoolSPtr->AcquireStreamAsync());
        TestStateSerializer<int>::SPtr stateSerializer = nullptr;
        status = TestStateSerializer<int>::Create(allocator, stateSerializer);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        KArray<KSharedPtr<VersionedItem<int>>> itemList(allocator);
        for (int i = 0; i < 1000; i++)
        {
            KSharedPtr<VersionedItem<int>> itemSPtr = AddValuesWithInsertedVersionedItem(*streamSPtr, *bwSPtr, *fileSPtr, i, *stateSerializer);
            itemList.Append(itemSPtr);
        }

        SyncAwait(fileSPtr->FlushAsync(*streamSPtr, *bwSPtr));
        SyncAwait(fileSPtr->StreamPoolSPtr->ReleaseStreamAsync(*streamSPtr));

        ValueCheckpointFile::SPtr valueCheckpointFileSPtr = SyncAwait(ValueCheckpointFile::OpenAsync(allocator, *filePathToOpenSPtr, *CreateTraceComponent()));
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
            int val = SyncAwait(taskList[i]);
            CODING_ERROR_ASSERT((ULONG)val == i);
        }

        taskList.Clear();

        for (ULONG i = 500; i < 1000; i++)
        {
            taskList.Append(fileSPtr->ReadValueAsync<int>(*itemList[i], *stateSerializer));
        }

        for (ULONG i = 0; i < taskList.Count(); i++)
        {
            int val = SyncAwait(taskList[i]);
            CODING_ERROR_ASSERT((ULONG)val == i + 500);
        }

        SyncAwait(fileSPtr->CloseAsync());
        SyncAwait(valueCheckpointFileSPtr->CloseAsync());
        RemoveFile(*filePathToOpenSPtr);
    }

    BOOST_AUTO_TEST_CASE(ValueCheckpointFile_Write100KeysAndReadAndVerifyMetadataUsingBytes_ShouldSucceed)
    {
        KAllocator& allocator = GetAllocator();
        KStringView filename = L"ValueCheckpointFile_Write100KeysAndReadAndVerifyMetadataUsingBytes_ShouldSucceed.txt";
        KString::SPtr filePathToOpenSPtr = CreateFileString(filename, GetAllocator());

        ULONG32 fileId = 10;
        ValueCheckpointFile::SPtr fileSPtr = SyncAwait(ValueCheckpointFile::CreateAsync(*CreateTraceComponent(), *filePathToOpenSPtr, fileId, allocator));
        SharedBinaryWriter::SPtr bwSPtr = nullptr;
        NTSTATUS status = SharedBinaryWriter::Create(allocator, bwSPtr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        ktl::io::KFileStream::SPtr streamSPtr = SyncAwait(fileSPtr->StreamPoolSPtr->AcquireStreamAsync());

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

        SyncAwait(fileSPtr->FlushAsync(*streamSPtr, *bwSPtr));
        SyncAwait(fileSPtr->StreamPoolSPtr->ReleaseStreamAsync(*streamSPtr));

        ValueCheckpointFile::SPtr valueCheckpointFileSPtr = SyncAwait(ValueCheckpointFile::OpenAsync(allocator, *filePathToOpenSPtr, *CreateTraceComponent()));
        CODING_ERROR_ASSERT(valueCheckpointFileSPtr != nullptr);
        CODING_ERROR_ASSERT(valueCheckpointFileSPtr->PropertiesSPtr->ValueCount == 100);
        CODING_ERROR_ASSERT(valueCheckpointFileSPtr->PropertiesSPtr->FileId == fileId);

        CODING_ERROR_ASSERT(valueCheckpointFileSPtr->PropertiesSPtr->ValuesHandle->get_Offset() == 0);
        CODING_ERROR_ASSERT(valueCheckpointFileSPtr->PropertiesSPtr->ValuesHandle->get_Size() == 3 * 100); //3 bytes, 100 is 300 bytes size.


        for (int i = 0; i < 100; i++)
        {
            KBuffer::SPtr val = SyncAwait(fileSPtr->ReadValueAsync<int>(*itemList[i]));
            CODING_ERROR_ASSERT(val->QuerySize() == 3);
            CODING_ERROR_ASSERT(*valueList[i] == *val);
        }
        
        SyncAwait(fileSPtr->CloseAsync());
        SyncAwait(valueCheckpointFileSPtr->CloseAsync());
        RemoveFile(*filePathToOpenSPtr);
    }

    BOOST_AUTO_TEST_CASE(KeyBlockAlignedWriter_WriteOnKeyAndEnumerate_ShouldSucceed)
    {
        //one int key item is 4 bytes of serialzied key size, 48 bytes data in total (44 is reserved for meta and padding)
        KBufferSerializer::SPtr serializer = nullptr;
        KBufferSerializer::Create(GetAllocator(), serializer);
        TestKeyBlockAlignedWriteAndEnumerator(1, sizeof(int), *serializer);
    }

    BOOST_AUTO_TEST_CASE(KeyBlockAlignedWriter_Write50KeysLessThanOneBlockSize_ShouldSucceed)
    {
        //50 int keys are 48 * 50 = 2400 bytes, which less than one block.
        KBufferSerializer::SPtr serializer = nullptr;
        KBufferSerializer::Create(GetAllocator(), serializer);
        TestKeyBlockAlignedWriteAndEnumerator(50, sizeof(int), *serializer);
    }

    BOOST_AUTO_TEST_CASE(KeyBlockAlignedWriter_WriteKeysTakeMutipleBlocks_ShouldSucceed)
    {
        //1200 int keys are 48 * 1200 = 57600 bytes, one block hold 4080 bytes or 85 4-byte key items, which then need 15 blocks.
        KBufferSerializer::SPtr serializer = nullptr;
        KBufferSerializer::Create(GetAllocator(), serializer);
        TestKeyBlockAlignedWriteAndEnumerator(1200, sizeof(int),*serializer);
    }

    BOOST_AUTO_TEST_CASE(KeyBlockAlignedWriter_WriteKeysTakeMutipleChunks_ShouldSucceed)
    {
        //100000 int keys are 1177 blocks, or 147 chunks.
        KBufferSerializer::SPtr serializer = nullptr;
        KBufferSerializer::Create(GetAllocator(), serializer);
        TestKeyBlockAlignedWriteAndEnumerator(100000, sizeof(int), *serializer);
    }

    BOOST_AUTO_TEST_CASE(KeyBlockAlignedWriter_WriteBiggerKeysWithBlockNotFullyOccupiedAndMoveToNextBlock_ShouldSucceed)
    {
        //one key is 12 byte, with reserved space is 12+44 = 56bytes , then 73 keys are 4088 bytes.( more than one block bytes - one block item part is 4080 bytes)
        //this will need two blocks with the 1st block not fully occupied and the last key moves to the 2nd block.
        KBufferSerializer::SPtr serializer = nullptr;
        KBufferSerializer::Create(GetAllocator(), serializer);
        TestKeyBlockAlignedWriteAndEnumerator(73, 12, *serializer);
    }

    BOOST_AUTO_TEST_CASE(KeyBlockAlignedWriter_WriteOneKeyLargerThanOneBlock_ShouldSucceed)
    {
        //one key is 4084 byte, with reserved space is 4084+44 = 4128 (more than one block bytes - one block item part is 4080 bytes)
        //this one key takes two blocks.
        KBufferSerializer::SPtr serializer = nullptr;
        KBufferSerializer::Create(GetAllocator(), serializer);
        TestKeyBlockAlignedWriteAndEnumerator(1, 4084, *serializer);
    }

    BOOST_AUTO_TEST_CASE(KeyBlockAlignedWriter_WriteOneKeyLargerThanOneChunk_ShouldSucceed)
    {
        //one key is 32772 bytes, with reserved space is 32772+44 = 32816 (more than one chunk 32768 bytes).
        //this one key takes two chunks.
        KBufferSerializer::SPtr serializer = nullptr;
        KBufferSerializer::Create(GetAllocator(), serializer);
        TestKeyBlockAlignedWriteAndEnumerator(1, 32772, *serializer);
    }

    //
    // ValueBlockAlignedWriterTests
    //
    BOOST_AUTO_TEST_CASE(ValueBlockAlignedWrite_SingleKey_ShouldSucceed)
    {
        ULONG32 bufferSize = 1024;
        ULONG32 valueSize = bufferSize;
        TestBlockWriteSingleItem(
            L"BlockAlignedWrite_SingleKey_ShouldSucceed.txt",
            bufferSize, 
            valueSize
        );
    }

    //
    // Single item bigger than block size.
    //
    BOOST_AUTO_TEST_CASE(ValueBlockAlignedWrite_ValueBiggerThanBlockSize_ShouldSucceed)
    {
        ULONG32 bufferSize = 5000;
        ULONG32 valueSize = bufferSize;
        TestBlockWriteSingleItem(
            L"BlockAlignedWrite_ValueBiggerThanBlockSize_ShouldSucceed.txt",
            bufferSize, 
            valueSize
        );
    }

    //
    // Single item with a ValueSize smaller than the serialized size
    //
    BOOST_AUTO_TEST_CASE(ValueBlockAlignedWrite_ValueSizeSmallerThanActualSize_ShouldSucceed)
    {
        ULONG32 bufferSize = 1024;
        ULONG32 valueSize = 100;
        TestBlockWriteSingleItem(
            L"BlockAlignedWrite_ValueSizeSmallerThanActualSize_ShouldSucceed.txt",
            bufferSize, 
            valueSize
        );
    }

    //
    // Single item with a ValueSize bigger than the serialized size
    //
    BOOST_AUTO_TEST_CASE(ValueBlockAlignedWrite_ValueSizeLargerThanActualSize_ShouldSucceed)
    {
        ULONG32 bufferSize = 1024;
        ULONG32 valueSize = 5000;
        TestBlockWriteSingleItem(
            L"BlockAlignedWrite_ValueSizeSmallerThanActualSize_ShouldSucceed.txt",
            bufferSize, 
            valueSize
        );
    }

    //
    // Multiple items - not within the 4k block size.
    //
    BOOST_AUTO_TEST_CASE(ValueBlockAlignedWrite_MultipleValuesNotBlockAligned_ShouldSucceed)
    {
        ULONG32 numItems = 5;
        ULONG32 bufferSize = 1000;
        ULONG32 valueSize = 1000;
        TestBlockWriteMultipleItems(
            L"BlockAlignedWrite_MultipleValuesNotBlockAligned_ShouldSucceed.txt",
            numItems,
            bufferSize,
            valueSize
        );
    }

    //
    // Multiple items - within the 4k block size.
    //
    BOOST_AUTO_TEST_CASE(ValueBlockAlignedWrite_MultipleValuesBlockAligned_ShouldSucceed)
    {
        ULONG32 numItems = 4;
        ULONG32 bufferSize = 1024;
        ULONG32 valueSize = 1024;
        TestBlockWriteMultipleItems(
            L"BlockAlignedWrite_MultipleValuesNotBlockAligned_ShouldSucceed.txt",
            numItems,
            bufferSize,
            valueSize
        );
    }

    //
    // Multiple items - value size smaller than serialized size
    //
    BOOST_AUTO_TEST_CASE(ValueBlockAlignedWrite_MultipleValuesBlockAligned_ValueSizeSmaller_ShouldSucceed)
    {
        ULONG32 numItems = 4;
        ULONG32 bufferSize = 1024;
        ULONG32 valueSize = 100;
        TestBlockWriteMultipleItems(
            L"BlockAlignedWrite_MultipleValuesNotBlockAligned_ShouldSucceed.txt",
            numItems,
            bufferSize,
            valueSize
        );
    }

    //
    // Multiple items - value size larger than serialized size
    //
    BOOST_AUTO_TEST_CASE(ValueBlockAlignedWrite_MultipleValuesBlockAligned_ValueSizeLarger_ShouldSucceed)
    {
        ULONG32 numItems = 4;
        ULONG32 bufferSize = 1024;
        ULONG32 valueSize = 2048;
        TestBlockWriteMultipleItems(
            L"BlockAlignedWrite_MultipleValuesBlockAligned_ValueSizeLarger_ShouldSucceed.txt",
            numItems,
            bufferSize,
            valueSize
        );
    }
    BOOST_AUTO_TEST_SUITE_END()
}

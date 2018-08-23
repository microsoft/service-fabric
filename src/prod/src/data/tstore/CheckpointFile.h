// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define CHECKPOINTFILE_TAG 'ftPC'


namespace Data
{
    namespace TStore
    {

        class CheckpointFile : 
            public KObject<CheckpointFile>,
            public KShared<CheckpointFile>
        {
            K_FORCE_SHARED(CheckpointFile)

        public:

            static NTSTATUS
                Create(
                    __in KStringView const& filename,
                    __in KeyCheckpointFile& keyCheckpointFile,
                    __in ValueCheckpointFile& valueCheckpointFile,
                    __in StoreTraceComponent & traceComponent,
                    __in KAllocator& allocator,
                    __out SPtr& result);

            __declspec(property(get = get_KeyCheckpointFileName)) KString::SPtr KeyCheckpointFileNameSPtr;
            KString::SPtr get_KeyCheckpointFileName() const
            {
                return keyFileNameSPtr_;
            }

            __declspec(property(get = get_ValueCheckpointFileName)) KString::SPtr ValueCheckpointFileNameSPtr;
            KString::SPtr get_ValueCheckpointFileName() const
            {
                return valueFileNameSPtr_;
            }

            __declspec(property(get = get_KeyCount)) ULONG64 KeyCount;
            ULONG64 get_KeyCount() const
            {
                return keyCheckpointFileSPtr_->KeyCount;
            }

            __declspec(property(get = get_ValueCount)) ULONG64 ValueCount;
            ULONG64 get_ValueCount() const
            {
                return valueCheckpointFileSPtr_->ValueCount;
            }

            __declspec(property(get = get_DeletedKeyCount)) ULONG64 DeletedKeyCount;
            ULONG64 get_DeletedKeyCount() const
            {
                return KeyCount - ValueCount;
            }

            __declspec(property(get = get_KeyBlockHandle)) KSharedPtr<BlockHandle> KeyBlockHandleSPtr;
            KSharedPtr<BlockHandle> get_KeyBlockHandle() const
            {
                return keyCheckpointFileSPtr_->PropertiesSPtr->KeysHandle;
            }

            __declspec(property(get = get_ValueBlockHandle)) KSharedPtr<BlockHandle> ValueBlockHandleSPtr;
            KSharedPtr<BlockHandle> get_ValueBlockHandle() const
            {
                return valueCheckpointFileSPtr_->PropertiesSPtr->ValuesHandle;
            }

            ktl::Awaitable<ULONG64> GetTotalFileSizeAsync(__in KAllocator& allocator);

            //
            // Opens a TStore checkpoint from the given file name.
            // file to open that contains an existing checkpoint.</param>
            //
            static ktl::Awaitable<KSharedPtr<CheckpointFile>> OpenAsync(
                __in KStringView& filename,
                __in StoreTraceComponent & traceComponent,
                __in KAllocator& allocator,
                __in bool isValueReferenceType);

            //
            // From the given file, serializing the given key-values into the file.
            //
            template<typename TKey, typename TValue>
            static ktl::Awaitable<KSharedPtr<CheckpointFile>> CreateAsync(
               __in ULONG32 fileId,
               __in KStringView const& filename,
               __in IEnumerator<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>>& sortedItemData,
               __in Data::StateManager::IStateSerializer<TKey>& keySerializer,
               __in Data::StateManager::IStateSerializer<TValue>& valueSerializer,
               __in ULONG64 logicalTimeStamp,
               __in KAllocator& allocator,
               __in StoreTraceComponent & traceComponent,
               __in StorePerformanceCountersSPtr & perfCounters,
               __in bool isValueAReferenceType)
            {
                SharedException::CSPtr exceptionSPtr = nullptr;
                KSharedPtr<IEnumerator<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>>> sortedItemDataSPtr(&sortedItemData);
                KSharedPtr<Data::StateManager::IStateSerializer<TKey>> keySerializerSPtr(&keySerializer);
                KSharedPtr<Data::StateManager::IStateSerializer<TValue>> valueSerializerSPtr(&valueSerializer);

                KSharedPtr<KString> keyFileNameSPtr = nullptr;
                NTSTATUS status = KString::Create(keyFileNameSPtr, allocator, filename);
                Diagnostics::Validate(status);
                keyFileNameSPtr->Concat(KeyCheckpointFile::GetFileExtension());

                KSharedPtr<KString> valueFileNameSPtr = nullptr;
                status = KString::Create(valueFileNameSPtr, allocator, filename);
                Diagnostics::Validate(status);
                valueFileNameSPtr->Concat(ValueCheckpointFile::GetFileExtension());

                KSharedPtr<KeyCheckpointFile> keyFileSPtr = co_await KeyCheckpointFile::CreateAsync(traceComponent, *keyFileNameSPtr, isValueAReferenceType, fileId, allocator);
                ValueCheckpointFile::SPtr valueFileSPtr = co_await ValueCheckpointFile::CreateAsync(traceComponent, *valueFileNameSPtr, fileId, allocator);

                KSharedPtr<CheckpointFile> checkpointFileSPtr = nullptr;
                status = CheckpointFile::Create(filename, *keyFileSPtr, *valueFileSPtr, traceComponent, allocator, checkpointFileSPtr);
                Diagnostics::Validate(status);

                // Create the key checkpoint file and memory buffer.
                ktl::io::KFileStream::SPtr keyFileStreamSPtr = co_await keyFileSPtr->StreamPoolSPtr->AcquireStreamAsync();
                ktl::io::KFileStream::SPtr valueFileStreamSPtr = co_await valueFileSPtr->StreamPoolSPtr->AcquireStreamAsync();

                try
                {
                    SharedBinaryWriter::SPtr keyMemoryBufferSPtr = nullptr;
                    status = SharedBinaryWriter::Create(allocator, keyMemoryBufferSPtr);
                    Diagnostics::Validate(status);

                    // Create the value checkpoint file and memory buffer.
                    SharedBinaryWriter::SPtr valueMemoryBufferSPtr = nullptr;
                    status = SharedBinaryWriter::Create(allocator, valueMemoryBufferSPtr);
                    Diagnostics::Validate(status);

                    KSharedPtr<BlockAlignedWriter<TKey, TValue>> blockWriterSPtr = nullptr;
                    status = BlockAlignedWriter<TKey, TValue>::Create(
                        *valueFileSPtr,
                        *keyFileSPtr,
                        *valueMemoryBufferSPtr,
                        *keyMemoryBufferSPtr,
                        *valueFileStreamSPtr,
                        *keyFileStreamSPtr,
                        logicalTimeStamp,
                        *keySerializerSPtr,
                        *valueSerializerSPtr,
                        traceComponent,
                        allocator,
                        blockWriterSPtr);
                    Diagnostics::Validate(status);

                    CheckpointPerformanceCounterWriter perfCounterWriter(perfCounters);
                    perfCounterWriter.StartMeasurement();

                    while (sortedItemDataSPtr->MoveNext())
                    {
                        co_await blockWriterSPtr->BlockAlignedWriteItemAsync(sortedItemDataSPtr->Current(), nullptr, true);
                    }

                    // Flush both key and value checkpoints to disk.
                    co_await blockWriterSPtr->FlushAsync();

                    perfCounterWriter.StopMeasurement();
                    ULONG64 writeBytes = co_await checkpointFileSPtr->GetTotalFileSizeAsync(allocator);
                    ULONG64 writeBytesPerSecond = perfCounterWriter.UpdatePerformanceCounter(writeBytes);
                    
                    StoreEventSource::Events->CheckpointFileWriteBytesPerSec(
                        traceComponent.PartitionId, traceComponent.TraceTag,
                        writeBytesPerSecond);
                }
                catch (ktl::Exception const& e)
                {
                    exceptionSPtr = SharedException::Create(e, allocator);
                }

                if (keyFileStreamSPtr != nullptr && keyFileStreamSPtr->IsOpen())
                {
                    co_await keyFileSPtr->StreamPoolSPtr->ReleaseStreamAsync(*keyFileStreamSPtr);
                }

                if (valueFileStreamSPtr != nullptr && valueFileStreamSPtr->IsOpen())
                {
                    co_await valueFileSPtr->StreamPoolSPtr->ReleaseStreamAsync(*valueFileStreamSPtr);
                }

                if (exceptionSPtr != nullptr)
                {
                    //clang compiler error, needs to assign before throw.
                    auto ex = exceptionSPtr->Info;
                    throw ex;
                }

                co_return checkpointFileSPtr;
            }

            //
            // Read the given value from disk.
            //
            template<typename TValue>
            ktl::Awaitable<TValue> ReadValueAsync(
                __in VersionedItem<TValue>& item,
                __in Data::StateManager::IStateSerializer<TValue>& valueSerializer)
            {
                return valueCheckpointFileSPtr_->ReadValueAsync(item, valueSerializer);
            }

            //
            // Read the given value from disk.
            //
            template<typename TValue>
            ktl::Awaitable<KBuffer::SPtr> ReadValueAsync(__in VersionedItem<TValue>& item)
            {
                return valueCheckpointFileSPtr_->ReadValueAsync(item);
            }

            template<typename TKey, typename TValue>
            KSharedPtr<KeyCheckpointFileAsyncEnumerator<TKey, TValue>> GetAsyncEnumerator(
                __in Data::StateManager::IStateSerializer<TKey>& keySerializer)
            {
                 KSharedPtr<KeyCheckpointFileAsyncEnumerator<TKey, TValue>> enumeratorSPtr = nullptr;
                 NTSTATUS status = KeyCheckpointFileAsyncEnumerator<TKey, TValue>::Create(
                     *keyCheckpointFileSPtr_,
                     keySerializer,
                     keyCheckpointFileSPtr_->PropertiesSPtr->KeysHandle->Offset,
                     keyCheckpointFileSPtr_->PropertiesSPtr->KeysHandle->EndOffset(),
                     *traceComponent_,
                     GetThisAllocator(),
                     enumeratorSPtr);
                 Diagnostics::Validate(status);
                 return enumeratorSPtr;
            }

            ktl::Awaitable<void> CloseAsync()
            {
                co_await keyCheckpointFileSPtr_->CloseAsync();
                co_await valueCheckpointFileSPtr_->CloseAsync();
            }

        private:

            ktl::Awaitable<ULONG64> GetFileSizeAsync(
                __in KString& filename,
                __in KAllocator& allocator);

            CheckpointFile(
                __in KStringView const& filename,
                __in KeyCheckpointFile& keyCheckpointFile,
                __in ValueCheckpointFile& valueCheckpointFile,
                __in StoreTraceComponent & traceComponent);

            KString::SPtr fileNameBaseSPtr_;
            KString::SPtr keyFileNameSPtr_;
            KString::SPtr valueFileNameSPtr_;
            KSharedPtr<KeyCheckpointFile> keyCheckpointFileSPtr_;
            KSharedPtr<ValueCheckpointFile> valueCheckpointFileSPtr_;
            ULONG64 cachedFileSize_;
            bool isFileSizeCached_;
            
            StoreTraceComponent::SPtr traceComponent_;
        };
    }
}

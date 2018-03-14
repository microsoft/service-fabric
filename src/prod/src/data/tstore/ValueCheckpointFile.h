// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#define VALUECHECKPOINTFILE_TAG 'fpCV'


namespace Data
{
    namespace TStore
    {
        //
        // Represents a TStore checkpoint file containing the serialized values and metadata.
        //
        class ValueCheckpointFile : 
            public KObject<ValueCheckpointFile>,
            public KShared<ValueCheckpointFile>
        {
            K_FORCE_SHARED(ValueCheckpointFile)

        public:

            static NTSTATUS
                Create(
                    __in StoreTraceComponent & traceComponent,
                    __in KString& filename,
                    __in KBlockFile& file,
                    __in KAllocator& allocator,
                    __out ValueCheckpointFile::SPtr& result);

            static NTSTATUS
                Create(
                    __in StoreTraceComponent & traceComponent,
                    __in KString& filename,
                    __in ULONG32 fileId,
                    __in KBlockFile& file,
                    __in KAllocator& allocator,
                    __out ValueCheckpointFile::SPtr& result);

            static ktl::Awaitable<ValueCheckpointFile::SPtr> CreateAsync(
                __in StoreTraceComponent & traceComponent,
                __in KString& filename,
                __in KAllocator& allocator);

            static ktl::Awaitable<ValueCheckpointFile::SPtr> CreateAsync(
                __in StoreTraceComponent & traceComponent,
                __in KString& filename,
                __in ULONG32 fileId,
                __in KAllocator& allocator);

            //
            // The currently supported value checkpoint file version.
            //
            static const int FileVersion = 1;

            //
            // Buffer in memory approximately 32 KB of data before flushing to disk.
            //
            static const int MemoryBufferFlushSize = 32 * 1024;

            //
            // The file extension for TStore checkpoint files that hold the serialized values.
            //
            static KStringView GetFileExtension()
            {
                KStringView extensionName = L".sfv";
                return extensionName;
            }

            //
            // Variable sized properties (metadata) about the checkpoint file.
            //
           
            __declspec(property(get = get_Properties, put = set_Properties)) KSharedPtr<ValueCheckpointFileProperties> PropertiesSPtr;
            KSharedPtr<ValueCheckpointFileProperties>  get_Properties() const
            {
                return propertiesSPtr_;
            }
            void set_Properties(__in ValueCheckpointFileProperties& properties)
            {
                propertiesSPtr_ = &properties;
            }


            __declspec(property(get = get_ValueCount)) ULONG64 ValueCount;
            ULONG64 get_ValueCount() const
            {
                return propertiesSPtr_->ValueCount;
            }

            //
            // Gets the file Id of the checkpoint file.
            //
            __declspec(property(get = get_FileId)) ULONG32 FileId;
            ULONG32 get_FileId() const
            {
                return propertiesSPtr_->FileId;
            }

            __declspec(property(get = get_FileName)) KString::CSPtr FileName;
            KString::CSPtr get_FileName() const
            {
                return filenameSPtr_.RawPtr();
            }

            __declspec(property(get = get_StreamPool)) StreamPool::SPtr StreamPoolSPtr;
            StreamPool::SPtr get_StreamPool() const
            {
                return streamPool_;
            }

            //
            // Opens a ValueCheckpointFile from the given file.
            // The file stream will be disposed when the checkpoint file is disposed.
            //
            static ktl::Awaitable<KSharedPtr<ValueCheckpointFile>> OpenAsync(
                __in KAllocator& allocator,
                __in KString& filename,
                __in StoreTraceComponent & traceComponent);
            

            //
            // Read the given value from disk.
            //
            template<typename TValue>
            ktl::Awaitable<TValue> ReadValueAsync(
                __in VersionedItem<TValue>& versionItem, 
                __in Data::StateManager::IStateSerializer<TValue>& valueSerializer)
            {
                KSharedPtr<VersionedItem<TValue>>item (&versionItem);
                // Validate the item has a value that can be read from disk.
                STORE_ASSERT(item != nullptr, "VersionedItem should not be null");
                STORE_ASSERT(item->GetRecordKind() != RecordKind::DeletedVersion, "VersionedItem should not be DeletedVersion");

                // Validate that the item's disk properties are valid.
                if (static_cast<ULONG64>(item->GetOffset()) < propertiesSPtr_->ValuesHandle->Offset)
                {
                    throw ktl::Exception(K_STATUS_OUT_OF_BOUNDS);
                }
                if (item->GetValueSize() < 0)
                {
                    throw ktl::Exception(K_STATUS_OUT_OF_BOUNDS);
                }

                if (static_cast<ULONG64>(item->GetOffset() + item->GetValueSize()) > propertiesSPtr_->ValuesHandle->EndOffset())
                {
                    throw ktl::Exception(K_STATUS_OUT_OF_BOUNDS);
                }

                ktl::io::KFileStream::SPtr fileStreamSPtr = nullptr;
                SharedException::CSPtr exception = nullptr;

                try
                {
                    fileStreamSPtr = co_await streamPool_->AcquireStreamAsync();

                    //read from disk.
                    KBuffer::SPtr bufferSPtr = nullptr;
                    ULONG bytesRead = 0;
                    ULONG size = static_cast<ULONG>(item->GetValueSize());
                    LONG64 offset = item->GetOffset();

                    NTSTATUS status = KBuffer::Create(
                        size,
                        bufferSPtr,
                        GetThisAllocator());
                    Diagnostics::Validate(status);

                    // Read the value bytes and the checksum into memory.
                    STORE_ASSERT(offset >= 0, "Offset={1} should be non-negative", offset);
                    fileStreamSPtr->SetPosition(offset);

                    status = co_await fileStreamSPtr->ReadAsync(*bufferSPtr, bytesRead, 0 , size);
                    STORE_ASSERT(NT_SUCCESS(status), "Failed to read from file. status={1}", status);
                    STORE_ASSERT(bytesRead == size, "Did not read correct number of bytes. bytesRead={1} expected={2}", bytesRead, size);

                    BinaryReader reader(*bufferSPtr, GetThisAllocator());

                    // Read the checksum from memory.
                    ULONG64 checksum = item->GetValueChecksum();

                    //Re-compute the checksum.
                    ULONG64 expectedChecksum = CRC64::ToCRC64(*bufferSPtr, 0, static_cast<ULONG32>(size));
                    if (checksum != expectedChecksum)
                    {
                        throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
                    }

                    // Deserialize the value into memory.
                    TValue value = valueSerializer.Read(reader);
                    co_await streamPool_->ReleaseStreamAsync(*fileStreamSPtr);
                    fileStreamSPtr = nullptr;
   
                    co_return value;
                }
                catch (ktl::Exception const& e)
                {
                    exception = SharedException::Create(e, GetThisAllocator());
                }

                if (fileStreamSPtr != nullptr && fileStreamSPtr->IsOpen())
                {
                    co_await streamPool_->ReleaseStreamAsync(*fileStreamSPtr);
                    fileStreamSPtr = nullptr;
                }

                if (exception != nullptr)
                {
                    //clang compiler error, needs to assign before throw.
                    auto ex = exception->Info;
                    throw ex;
                }
            }


            //
            // Read the given value from disk.
            //
            template<typename TValue>
            ktl::Awaitable<KBuffer::SPtr> ReadValueAsync(
                __in VersionedItem<TValue>& versionItem)
            {
                KSharedPtr<VersionedItem<TValue>>item(&versionItem);

                ktl::io::KFileStream::SPtr fileStreamSPtr = nullptr;
                SharedException::CSPtr exception = nullptr;

                try
                {
                    fileStreamSPtr = co_await streamPool_->AcquireStreamAsync();

                    //read from disk.
                    KBuffer::SPtr bufferSPtr = nullptr;
                    ULONG bytesRead = 0;
                    ULONG size = static_cast<ULONG>(item->GetValueSize());
                    LONG64 offset = item->GetOffset();

                    NTSTATUS status = KBuffer::Create(
                        size,
                        bufferSPtr,
                        GetThisAllocator());
                    Diagnostics::Validate(status);

                    // Read the value bytes and the checksum into memory.
                    STORE_ASSERT(offset >= 0, "Offset={1} should be non-negative", offset);
                    fileStreamSPtr->SetPosition(offset);

                    status = co_await fileStreamSPtr->ReadAsync(*bufferSPtr, bytesRead, 0, size);
                    STORE_ASSERT(NT_SUCCESS(status), "Failed to read from file. status={1}", status);
                    STORE_ASSERT(bytesRead == size, "Read incorrect number of bytes. bytesRead={1} size={2}", bytesRead, size);

                    // Read the checksum from memory.
                    ULONG64 checksum = item->GetValueChecksum();

                    //Re-compute the checksum.
                    ULONG64 expectedChecksum = CRC64::ToCRC64(*bufferSPtr, 0, static_cast<ULONG32>(size));
                    if (checksum != expectedChecksum)
                    {
                        throw ktl::Exception(STATUS_INTERNAL_DB_CORRUPTION);
                    }
                    
                    co_await streamPool_->ReleaseStreamAsync(*fileStreamSPtr);
                    fileStreamSPtr = nullptr;

                    co_return bufferSPtr;
                }
                catch (ktl::Exception const& e)
                {
                    exception = SharedException::Create(e, GetThisAllocator());
                }

                if (fileStreamSPtr != nullptr && fileStreamSPtr->IsOpen())
                {
                    co_await streamPool_->ReleaseStreamAsync(*fileStreamSPtr);
                    fileStreamSPtr = nullptr;
                }

                if (exception != nullptr)
                {
                    //clang compiler error, needs to assign before throw.
                    auto ex = exception->Info;
                    throw ex;
                }
            }

            //
            // Add a value to the given file stream, using the memory buffer to stage writes before issuing bulk disk IOs.
            //
            template<typename TValue>
            void WriteItem(
                __in ktl::io::KFileStream& fileStream,
                __in BinaryWriter& memoryBuffer,
                __in VersionedItem<TValue>& item,
                __in Data::StateManager::IStateSerializer<TValue>& valueSerializer)
            {
                // Write the value into the memory buffer.
                WriteValue(memoryBuffer, fileStream.GetPosition(), item, valueSerializer);
            }

            //
            // Add a value to the given file stream, using the memory buffer to stage writes before issuing bulk disk IOs.
            //
            template<typename TValue>
            void WriteItem(
                __in ktl::io::KFileStream& fileStream,
                __in BinaryWriter& memoryBuffer,
                __in VersionedItem<TValue>& item,
                __in KBuffer& value)
            {
                // Write the value into the memory buffer.
                WriteValue(memoryBuffer, fileStream.GetPosition(), item, value);
            }

            //
            // A Flush indicates that all values have been written to the checkpoint file (via AddItemAsync), so
            // the checkpoint file can finish flushing any remaining in-memory buffered data, write any extra
            // metadata (e.g. properties and footer), and flush to disk.
            //
            ktl::Awaitable<void> FlushAsync(
                __in ktl::io::KFileStream& stream,
                __in SharedBinaryWriter& memoryBuffer);                          

            ktl::Awaitable<void> FlushMemoryBufferAsync(
                __in ktl::io::KFileStream& fileStream,
                __in SharedBinaryWriter& memoryBuffer);

            ktl::Awaitable<void> CloseAsync();

        private:
            static ktl::Awaitable<KBlockFile::SPtr> CreateBlockFileAsync(
                __in KString& filename,
                __in KAllocator& allocator,
                __in KBlockFile::CreateDisposition createType);
            ktl::Awaitable<ktl::io::KFileStream::SPtr> CreateFileStreamAsync();

            template<typename TValue>
            void WriteValue(
                __in BinaryWriter& memoryBuffer,
                __in LONGLONG basePosition,
                __in VersionedItem<TValue>& item,
                __in Data::StateManager::IStateSerializer<TValue>& valueSerializer)
            {
                // Deleted items don't have values.  Only serialize valid items.
                if (item.GetRecordKind() != RecordKind::DeletedVersion)
                {
                    // WriteItemAsync valueSerializer followed by checksum.
                    // Serialize the value.
                    ULONG valueStartPosition = memoryBuffer.Position;
                    valueSerializer.Write(item.GetValue(), memoryBuffer);
                    ULONG valueEndPosition = memoryBuffer.Position;
                    STORE_ASSERT(valueEndPosition >= valueStartPosition, "valueEndPosition={1} >= valueStartPosition={2}", valueEndPosition, valueStartPosition);

                    // Write the checksum of just that value's bytes.
                    ULONG32 valueSize = static_cast<ULONG32>(valueEndPosition - valueStartPosition);
                    ULONG64 checksum = CRC64::ToCRC64(*memoryBuffer.GetBuffer(0), static_cast<ULONG32>(valueStartPosition), valueSize);

                    // Update the in-memory offset and size for this item.
                    item.SetOffset(static_cast<LONG64>(basePosition + valueStartPosition), *traceComponent_);
                    item.SetValueSize(static_cast<int>(valueSize));
                    item.SetValueChecksum(checksum);

                    // Update checkpoint file in-memory metadata.
                    propertiesSPtr_->ValueCount = propertiesSPtr_->ValueCount+1;
                }

                // Update the in-memory metadata about which file this key-value exists in on disk.
                item.SetFileId(FileId);
            }

            template<typename TValue>
            void WriteValue(
                __in BinaryWriter& memoryBuffer,
                __in LONGLONG basePosition,
                __in VersionedItem<TValue>& item,
                __in KBuffer& value)
            {
                // Deleted items don't have values.  Only serialize valid items.
                if (item.GetRecordKind() != RecordKind::DeletedVersion)
                {
                    // WriteItemAsync valueSerializer followed by checksum.
                    // Serialize the value.
                    ULONG valueStartPosition = memoryBuffer.Position;
                    memoryBuffer.Write(value);

                    ULONG valueEndPosition = memoryBuffer.Position;
                    STORE_ASSERT(valueEndPosition >= valueStartPosition, "valueEndPosition={1} >= valueStartPosition={2}", valueEndPosition, valueStartPosition);

                    // Write the checksum of just that value's bytes.
                    ULONG32 valueSize = static_cast<ULONG32>(valueEndPosition - valueStartPosition);
                    ULONG64 checksum = CRC64::ToCRC64(*memoryBuffer.GetBuffer(0), static_cast<ULONG32>(valueStartPosition), valueSize);

                    // Update the in-memory offset and size for this item.
                    item.SetOffset(static_cast<LONG64>(basePosition + valueStartPosition), *traceComponent_);
                    item.SetValueSize(static_cast<int>(valueSize));
                    item.SetValueChecksum(checksum);

                    // Update checkpoint file in-memory metadata.
                    propertiesSPtr_->ValueCount = propertiesSPtr_->ValueCount + 1;
                }

                // Update the in-memory metadata about which file this key-value exists in on disk.
                item.SetFileId(FileId);
            }

            //
            // Deserializes the metadata (footer, properties, etc.) for this checkpoint file.
            //
            ktl::Awaitable<void> ReadMetadataAsync();

            
            KBlockFile::SPtr fileSPtr_;

            KSharedPtr<ValueCheckpointFileProperties> propertiesSPtr_;

            KString::SPtr filenameSPtr_;

            FileFooter::SPtr footerSPtr_;

            StreamPool::SPtr streamPool_;

            StoreTraceComponent::SPtr traceComponent_;

            //
            // Create a new key checkpoint file with the given filename.
            //
            ValueCheckpointFile(
                __in KString& filename,
                __in ULONG32 fileId,
                __in KBlockFile & file,
                __in StoreTraceComponent & traceComponent);

            //
            // Reserved for openasync.
            // 
            ValueCheckpointFile(
                __in KString& filename,
                __in KBlockFile & file,
                __in StoreTraceComponent & traceComponent);
        };
    }
}

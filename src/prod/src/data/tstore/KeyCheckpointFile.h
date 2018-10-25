// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TStore
    {
        //
        // Represents a TStore checkpoint file containing the serialized keys and metadata.
        //
        class KeyCheckpointFile : public KObject<KeyCheckpointFile>,
            public KShared<KeyCheckpointFile>
        {
            K_FORCE_SHARED(KeyCheckpointFile)

        public:

            static NTSTATUS
                Create(
                    __in StoreTraceComponent & traceComponent,
                    __in KString& filename,
                    __in bool isValueAReferenceType,
                    __in KBlockFile & file,
                    __in KAllocator& allocator,
                    __out KeyCheckpointFile::SPtr& result);

            static NTSTATUS
                Create(
                    __in StoreTraceComponent & traceComponent,
                    __in KString& filename,
                    __in bool isValueAReferenceType,
                    __in ULONG32 fileId,
                    __in KBlockFile & file,
                    __in KAllocator& allocator,
                    __out KeyCheckpointFile::SPtr& result);
            
            static ktl::Awaitable<KeyCheckpointFile::SPtr> CreateAsync(
                __in StoreTraceComponent & traceComponent,
                __in KString& filename,
                __in bool isValueAReferenceType,
                __in KAllocator& allocator);

            static ktl::Awaitable<KeyCheckpointFile::SPtr> CreateAsync(
                __in StoreTraceComponent & traceComponent,
                __in KString& filename,
                __in bool isValueAReferenceType,
                __in ULONG32 fileId,
                __in KAllocator& allocator);

            // 
            // Buffer in memory approximately 32 KB of data before flushing to disk.
            // 
            static const int MemoryBufferFlushSize = 32 * 1024;

            // 
            // The file extension for TStore checkpoint files that hold the keys and metadata.
            //
            static KStringView GetFileExtension()
            {
                KStringView extensionName = L".sfk";
                return extensionName;
            }

            //
            // Variable sized properties (metadata) about the checkpoint file.
            //       
            __declspec(property(get = get_Properties, put = set_Properties)) KSharedPtr<KeyCheckpointFileProperties> PropertiesSPtr;
            KSharedPtr<KeyCheckpointFileProperties>  get_Properties() const
            {
                return propertiesSPtr_;
            }
            void set_Properties(__in KeyCheckpointFileProperties& properties)
            {
                propertiesSPtr_ = &properties;
            }

            //
            // Pool of reusable FileStreams for the checkpoint file.
            //
            //StreamPool ReaderPool;

            __declspec(property(get = get_KeyCount)) ULONG64 KeyCount;
            ULONG64 get_KeyCount() const
            {
                return propertiesSPtr_->KeyCount;
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
            // Opens a KeyCheckpointFile from the given file.
            // The file to open that contains an existing checkpoint file.</param>
            // Tracing information.
            //
            static ktl::Awaitable<KSharedPtr<KeyCheckpointFile>> OpenAsync(
                __in KAllocator& allocator,
                __in KString& filename,
                __in StoreTraceComponent & traceComponent,
                __in bool isValueReferenceType);

            // 
            // Add a key to the given file stream, using the memory buffer to stage writes before issuing bulk disk IOs.
            //
            template<typename TKey, typename TValue>         
            void WriteItem(
                __in BinaryWriter& memoryBuffer,
                __in KeyValuePair<TKey, typename VersionedItem<TValue>::SPtr>& item,
                __in Data::StateManager::IStateSerializer<TKey>& keySerializer,
                __in LONG64 logicalTimeStamp)
            {
                // Write the key into the memory buffer.
                WriteKey<TKey, TValue>(memoryBuffer, item, keySerializer, logicalTimeStamp);
            }

            // 
            // A Flush indicates that all keys have been written to the checkpoint file (via AddItemAsync), so
            // the checkpoint file can finish flushing any remaining in-memory buffered data, write any extra
            // metadata (e.g. properties and footer), and flush to disk.
            // 
            ktl::Awaitable<void> FlushAsync(
                __in ktl::io::KFileStream& fileStream,
                __in SharedBinaryWriter& writer);

            //
            // Read key from file for merge.
            // The data is written is 8 bytes aligned.
            // 
            // Name                    Type        Size
            // 
            // KeySize                 int         4
            // Kind                    byte        1
            // RESERVED                            3
            // VersionSequenceNumber   long        8
            // 
            // (DeletedVersion)
            // logicalTimeStamp        long        8
            // 
            // (Inserted || Updated)
            // Offset                  long        8
            // ValueChecksum           ulong       8
            // ValueSize               int         4
            // RESERVED                            4
            // 
            // Key                     TKey        N
            // PADDING                             (N % 8 ==0) ? 0 : 8 - (N % 8)
            //
            // RESERVED: Fixed padding that is usable to add fields in future.
            // PADDING:  Due to dynamic size, cannot be used for adding fields.
            // 
            // Note: Larges Key size supported is int.MaxValue in bytes.
            //
            template<typename TKey, typename TValue>
            KSharedPtr<KeyData<TKey, TValue>> ReadKey(
                __in BinaryReader& memoryBuffer,
                __in Data::StateManager::IStateSerializer<TKey>& keySerializer)
            {
                ByteAlignedReaderWriterHelper::ThrowIfNotAligned(memoryBuffer.Position);

                // This mirrors WriteKey().
                ULONG32 keySize = 0;
                memoryBuffer.Read(keySize);
                byte kindByte = 0;
                memoryBuffer.Read(kindByte);

                RecordKind kind = static_cast<RecordKind>(kindByte);
                ByteAlignedReaderWriterHelper::ReadPaddingUntilAligned(memoryBuffer);

                ULONG64 lsn = 0;
                memoryBuffer.Read(lsn);

                ULONG64 valueOffset = 0;
                ULONG32 valueSize = 0;
                ULONG64 valueChecksum = 0;
                ULONG64 logicalTimeStamp = 0;

                if (kind == RecordKind::DeletedVersion)
                {
                     memoryBuffer.Read(logicalTimeStamp);
                }
                else
                {
                    memoryBuffer.Read(valueOffset);
                    memoryBuffer.Read(valueChecksum);
                    memoryBuffer.Read(valueSize);
                    ByteAlignedReaderWriterHelper::ReadPaddingUntilAligned(memoryBuffer);
                }

                // Protection in case the user's key serializer doesn't leave the stream at the correct end point.
                ULONG32 keyPosition = memoryBuffer.Position;
                TKey key = keySerializer.Read(memoryBuffer);
                memoryBuffer.Position = keyPosition + keySize;

                ByteAlignedReaderWriterHelper::ReadPaddingUntilAligned(memoryBuffer);

                typename VersionedItem<TValue>::SPtr value = nullptr;
                typename DeletedVersionedItem<TValue>::SPtr deletedVersion = nullptr;
                typename InsertedVersionedItem<TValue>::SPtr insertedVersion = nullptr;
                typename UpdatedVersionedItem<TValue>::SPtr updatedVersion = nullptr;

                NTSTATUS status = STATUS_SUCCESS;
                switch (kind)
                {
                case RecordKind::DeletedVersion:
                    status = DeletedVersionedItem<TValue>::Create(GetThisAllocator(), deletedVersion);
                    Diagnostics::Validate(status);
                    value = &(*deletedVersion);
                    break;
                case RecordKind::InsertedVersion:
                    status = InsertedVersionedItem<TValue>::Create(GetThisAllocator(), insertedVersion);
                    Diagnostics::Validate(status);
                    insertedVersion->InitializeOnRecovery(lsn, FileId, valueOffset, valueSize, valueChecksum);

                    value = &(*insertedVersion);
                    break;
                case RecordKind::UpdatedVersion:
                    status = UpdatedVersionedItem<TValue>::Create(GetThisAllocator(), updatedVersion);
                    Diagnostics::Validate(status);
                    updatedVersion->InitializeOnRecovery(lsn, FileId, valueOffset, valueSize, valueChecksum);

                    value = &(*updatedVersion);
                    break;

                default:
                    throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
                }

                value->SetFileId(FileId);
                value->SetVersionSequenceNumber(lsn);

                KSharedPtr<KeyData<TKey, TValue>> keydata = nullptr;
                NTSTATUS keystatus = KeyData<TKey, TValue>::Create(key, *value, logicalTimeStamp, keySize, GetThisAllocator(), keydata);
                Diagnostics::Validate(keystatus);
                return keydata;
            }

            ktl::Awaitable<void> FlushMemoryBufferAsync(
                __in ktl::io::KFileStream& fileStream,
                __in SharedBinaryWriter& writer);

            ktl::Awaitable<void> CloseAsync();

        private:
            static ktl::Awaitable<KBlockFile::SPtr> CreateBlockFileAsync(
                __in KString& filename,
                __in KAllocator& allocator,
                __in KBlockFile::CreateDisposition createType);

            ktl::Awaitable<ktl::io::KFileStream::SPtr> CreateFileStreamAsync();

            //
            // The data is written is 8 bytes aligned.
            // 
            // Name                    Type        Size
            // 
            // KeySize                 int         4
            // Kind                    byte        1
            // RESERVED                            3
            // VersionSequenceNumber   long        8
            // 
            // (DeletedVersion)
            // logicalTimeStamp        long        8
            // 
            // (Inserted || Updated)
            // Offset                  long        8
            // ValueChecksum           ulong       8
            // ValueSize               int         4
            // RESERVED                            4
            // 
            // Key                     TKey        N
            // PADDING                             (N % 8 ==0) ? 0 : 8 - (N % 8)
            //
            // RESERVED: Fixed padding that is usable to add fields in future.
            // PADDING:  Due to dynamic size, cannot be used for adding fields.
            // 
            // Note: Larges Key size supported is int.MaxValue in bytes.
            //
            template<typename TKey, typename TValue>
            void WriteKey(
                __in BinaryWriter& memoryBuffer,
                __in KeyValuePair<TKey, typename VersionedItem<TValue>::SPtr>& item, 
                __in Data::StateManager::IStateSerializer<TKey>& keySerializer,
                __in LONG64 logicalTimeStamp)
            {
                STORE_ASSERT(ByteAlignedReaderWriterHelper::IsAligned(memoryBuffer.Position), "position={1} should be aligned", memoryBuffer.Position);
                ULONG recordPosition = memoryBuffer.Position;
                memoryBuffer.Position += sizeof(ULONG32);

                memoryBuffer.Write(static_cast<byte>(item.Value->GetRecordKind())); // RecordKind
                ByteAlignedReaderWriterHelper::WritePaddingUntilAligned(memoryBuffer); // RESERVED

                memoryBuffer.Write(item.Value->GetVersionSequenceNumber()); // LSN

                if (item.Value->GetRecordKind() == RecordKind::DeletedVersion)
                {
                    memoryBuffer.Write(logicalTimeStamp);
                }
                else
                {
                    // Deleted items don't have a value.  We only serialize value properties for non-deleted items.
                    memoryBuffer.Write(item.Value->GetOffset()); // value offset
                    memoryBuffer.Write(item.Value->GetValueChecksum()); // value checksum
                    memoryBuffer.Write(item.Value->GetValueSize()); // value size
                    ByteAlignedReaderWriterHelper::WritePaddingUntilAligned(memoryBuffer); // RESERVED
                }

                ULONG keyPosition = memoryBuffer.Position;
                keySerializer.Write(item.Key, memoryBuffer);
                ULONG keyEndPosition = memoryBuffer.Position;
                STORE_ASSERT(keyEndPosition >= keyPosition, "keyEndPosition={1} >= keyPosition={2}", keyEndPosition, keyPosition);

                memoryBuffer.Position = recordPosition;
                memoryBuffer.Write(static_cast<ULONG32>(keyEndPosition - keyPosition));
                memoryBuffer.Position = keyEndPosition;

                ByteAlignedReaderWriterHelper::WritePaddingUntilAligned(memoryBuffer); // RESERVED
                STORE_ASSERT(ByteAlignedReaderWriterHelper::IsAligned(memoryBuffer.Position), "position={1} should be aligned", memoryBuffer.Position);

                // Update in-memory metadata.
                propertiesSPtr_->KeyCount = propertiesSPtr_->KeyCount + 1;
            }

            //
            // Deserializes the metadata (footer, properties, etc.) for this checkpoint file.
            //
            ktl::Awaitable<void> ReadMetadataAsync();

            //
            // The currently supported key checkpoint file version.
            // 
            static const int FileVersion = 1;

            //
            // Fixed sized metadata about the checkpoint file.  E.g. location of the Properties section.
            //
            FileFooter::SPtr footerSPtr_;

            bool isValueAReferenceType_;

            KString::SPtr filenameSPtr_;

            KeyCheckpointFileProperties::SPtr propertiesSPtr_;

            KBlockFile::SPtr fileSPtr_;

            StreamPool::SPtr streamPool_;

            StoreTraceComponent::SPtr traceComponent_;

            //
            // Create a new key checkpoint file with the given filename.
            //
            KeyCheckpointFile(
                __in KString& filename,
                __in ULONG32 fileId,
                __in KBlockFile& file,
                __in bool isValueAReferenceType,
                __in StoreTraceComponent & traceComponent);

            //
            // Reserved for openasync.
            // 
            KeyCheckpointFile(
                __in KString& filename,
                __in KBlockFile& file,
                __in bool isValueAReferenceType,
                __in StoreTraceComponent & traceComponent);
        };
    }
}

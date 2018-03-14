// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TStore
    {
        class MetadataManager 
        {
        public:
           static void AddFile(
              __in IDictionary<ULONG32, FileMetadata::SPtr>& table,
              __in ULONG32 fileId,
              __in FileMetadata& metadata);

           static void RemoveFile(
              __in IDictionary<ULONG32, FileMetadata::SPtr>& table,
              __in ULONG32 fileId);

           static ktl::Awaitable<void> OpenAsync(
              __in MetadataTable& metadataTable,
              __in ktl::io::KStream& stream,
              __in ULONG64 length,
              __in KAllocator& allocator,
              __in StoreTraceComponent & traceComponent);

           static ktl::Awaitable<void> OpenAsync(
              __in MetadataTable& metdataTable,
              __in KString const & fileName,
              __in KAllocator& allocator,
              __in StoreTraceComponent & traceComponent);

           static ktl::Awaitable<void> PopulateMetadataAsync(
              __in MetadataTable& metadataTable,
              __in ktl::io::KStream& stream,
              __in ULONG64 length,
              __in KAllocator& allocator,
              __in StoreTraceComponent & traceComponent);

           static ktl::Awaitable<void> PopulateMetadataAsync(
              __in MetadataTable& metadataTable,
              __in KString const & fileName,
              __in KAllocator& allocator,
              __in StoreTraceComponent & traceComponent);

           static ktl::Awaitable<void> MetadataManager::WriteAsync(
              __in MetadataTable& metadataTable,
              __in ktl::io::KStream& stream,
              __in KAllocator& allocator);

           static ktl::Awaitable<void> MetadataManager::WriteAsync(
               __in MetadataTable& metadataTable,
               __in KString const & fileName,
               __in KAllocator& allocator);

           static ktl::Awaitable<void> MetadataManager::SafeFileReplaceAsync(
               __in KString const & checkpointFileName,
               __in KString const & temporaryCheckpointFileName,
               __in KString const & backupCheckpointFileName,
               __in bool isValueAReferenceType,
               __in StoreTraceComponent & traceComponent,
               __in KAllocator & allocator);

           static ktl::Awaitable<void> ValidateAsync(
               __in KString const& filename,
               __in KAllocator& allocator);

            template<typename TValue>
            static ktl::Awaitable<TValue> ReadValueAsync(
                __in FileMetadata& fileMetadata,
                __in VersionedItem<TValue>& item,
                __in Data::StateManager::IStateSerializer<TValue>& valueSerializer)
            {
                ASSERT_IF(fileMetadata.CheckpointFileSPtr == nullptr, "Checkpoint file with id '{0}' does not exist in memory.", item.GetFileId());
                
                return fileMetadata.CheckpointFileSPtr->ReadValueAsync<TValue>(item, valueSerializer);
            }

            template<typename TValue>
            static ktl::Awaitable<TValue> ReadValueAsync(
                __in MetadataTable& metadataTable,
                __in VersionedItem<TValue>& item,
                __in Data::StateManager::IStateSerializer<TValue>& valueSerializer)
            {
                auto fileId = item.GetFileId();
                FileMetadata::SPtr fileMetadataSPtr = nullptr;
                bool result = metadataTable.Table->TryGetValue(fileId, fileMetadataSPtr);
                if (!result)
                {
                    throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
                }

                ASSERT_IFNOT(fileMetadataSPtr->CheckpointFileSPtr != nullptr, "Checkpoint file with id '{0}' does not exist in memory.", fileId);

                return MetadataManager::ReadValueAsync<TValue>(*fileMetadataSPtr, item, valueSerializer);
            }

            template<typename TValue>
            static ktl::Awaitable<KBuffer::SPtr> ReadValueAsync(
                __in MetadataTable& metadataTable,
                __in VersionedItem<TValue>& item)
            {
                auto fileId = item.GetFileId();
                FileMetadata::SPtr fileMetadataSPtr = nullptr;
                bool result = metadataTable.Table->TryGetValue(fileId, fileMetadataSPtr);
                if (!result)
                {
                    throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
                }

                ASSERT_IFNOT(fileMetadataSPtr->CheckpointFileSPtr != nullptr, "Checkpoint file with id '{0}' does not exist in memory.", fileId);
                
                return fileMetadataSPtr->CheckpointFileSPtr->ReadValueAsync(item);
            }

        private:
           static const ULONG32 FileVersion = 1;
           static const ULONG32 MemoryBufferFlushSize = 32 * 1024;

           static ktl::Awaitable<ULONG32> ReadDiskMetadataAsync(
               __in IDictionary<ULONG32, FileMetadata::SPtr>& metadataTable,
               __in ktl::io::KStream& stream,
               __in MetadataManagerFileProperties& properties,
               __in KAllocator& allocator,
               __in StoreTraceComponent & traceComponent);

            static ktl::Awaitable<MetadataManagerFileProperties::SPtr> MetadataManager::WriteDiskMetadataAsync(
               __in IDictionary<ULONG32, FileMetadata::SPtr>& metadataTable,
               __in ktl::io::KStream& outputStream,
               __in KAllocator& allocator);

           static ktl::Awaitable<void>FlushMemoryBufferAsync(
              __in ktl::io::KStream& fileStream,
              __in SharedBinaryWriter& binaryWriter);

        };
    }
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#define METADATAMANAGER_TAG 'rgMM'

using namespace ktl;
using namespace Data::TStore;
using namespace Data::Utilities;

void MetadataManager::AddFile(
   __in Data::IDictionary<ULONG32, FileMetadata::SPtr>& table,
   __in ULONG32 fileId,
   __in FileMetadata& metadata)
{
   if (table.ContainsKey(fileId))
   {
      throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
   }

   FileMetadata::SPtr fileMetdataSPtr = &metadata;
   table.Add(fileId, fileMetdataSPtr);
}

 void MetadataManager::RemoveFile(
   __in Data::IDictionary<ULONG32, FileMetadata::SPtr>& table,
   __in ULONG32 fileId)
{
   KInvariant(table.ContainsKey(fileId));

   bool removed = table.Remove(fileId);
   ASSERT_IFNOT(removed, "Failed to remove file id {0}", fileId);
}

ktl::Awaitable<void> MetadataManager::OpenAsync(
   __in MetadataTable& metdataTable,
   __in ktl::io::KStream& stream,
   __in ULONG64 length,
   __in KAllocator& allocator,
   __in StoreTraceComponent & traceComponent)
{
   KInvariant(metdataTable.Table->Count == 0);
   return PopulateMetadataAsync(metdataTable, stream, length, allocator, traceComponent);
}

ktl::Awaitable<void> MetadataManager::OpenAsync(
    __in MetadataTable& metadataTable,
    __in KString const & filename,
    __in KAllocator& allocator,
    __in StoreTraceComponent & traceComponent)
{
    ASSERT_IFNOT(metadataTable.Table->Count == 0, "Count should be zero");
    // TODO: KWString should be replaced by KString once ktl apis are fixed 
    KWString filePath(allocator, filename);
    KBlockFile::SPtr blockFileSPtr = nullptr;
    SharedException::CSPtr exception = nullptr;

    auto status = co_await KBlockFile::CreateSparseFileAsync(
        filePath,
        TRUE,
        KBlockFile::CreateDisposition::eOpenExisting,
        KBlockFile::CreateOptions::eInheritFileSecurity,
        blockFileSPtr,
        nullptr,
        allocator,
        METADATAMANAGER_TAG
    );
    KFinally([&] { if (blockFileSPtr != nullptr) blockFileSPtr->Close(); });

    ASSERT_IFNOT(NT_SUCCESS(status), "Unable to open file {0}", (wchar_t *)(filename));

    ktl::io::KFileStream::SPtr fileStreamSPtr = nullptr;
        
    try
    {
        status = ktl::io::KFileStream::Create(fileStreamSPtr, allocator);
        Diagnostics::Validate(status);

        status = co_await fileStreamSPtr->OpenAsync(*blockFileSPtr);
        ASSERT_IFNOT(NT_SUCCESS(status), "Unable to open file stream for file {0}", (wchar_t *)(filename));
        co_await PopulateMetadataAsync(metadataTable, *fileStreamSPtr, fileStreamSPtr->GetLength(), allocator, traceComponent);
        metadataTable.MetadataFileSize = fileStreamSPtr->GetLength();
    }
    catch (ktl::Exception const& e)
    {
        exception = SharedException::Create(e, allocator);
    }

    if (fileStreamSPtr != nullptr && fileStreamSPtr->IsOpen())
    {
        status = co_await fileStreamSPtr->CloseAsync();
        Diagnostics::Validate(status);
    }

    if (exception != nullptr)
    {
        //clang compiler error, needs to assign before throw.
        auto ex = exception->Info;
        throw ex;
    }
}

ktl::Awaitable<void> MetadataManager::PopulateMetadataAsync(
   __in MetadataTable& metadataTable,
   __in ktl::io::KStream& stream,
   __in ULONG64 length,
   __in KAllocator& allocator,
   __in StoreTraceComponent & traceComponent)
{
   ktl::io::KStream::SPtr streamSPtr(&stream);

   BlockHandle::SPtr footerHandleSPtr;

   // From the bottom, checksum, footer and the block offset starts.
   ULONG64 offset = length - FileFooter::SerializedSize() - sizeof(ULONG64);
   NTSTATUS status = BlockHandle::Create(offset, FileFooter::SerializedSize(), allocator, footerHandleSPtr);
   Diagnostics::Validate(status);

   FileBlock<FileFooter::SPtr>::DeserializerFunc footerDeserializerFunc(&FileFooter::Read);
   auto footerSPtr = co_await FileBlock<FileFooter::SPtr>::ReadBlockAsync(*streamSPtr, *footerHandleSPtr, footerDeserializerFunc, allocator, ktl::CancellationToken::None);

   if (footerSPtr->Version != MetadataManager::FileVersion)
   {
      throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
   }

   // Read and validate property section
   auto propertiesHandle = footerSPtr->PropertiesHandle;

   FileBlock<MetadataManagerFileProperties::SPtr>::DeserializerFunc propertyDeserializerFunc(&MetadataManagerFileProperties::Read);
   auto properties = co_await FileBlock<MetadataManagerFileProperties::SPtr>::ReadBlockAsync(*streamSPtr, *propertiesHandle, propertyDeserializerFunc, allocator, ktl::CancellationToken::None);

   // Read disk metadata into memory.
   co_await ReadDiskMetadataAsync(*metadataTable.Table, *streamSPtr, *properties, allocator, traceComponent);
   metadataTable.CheckpointLSN = properties->CheckpointLSN;
   co_return;
}

ktl::Awaitable<void> MetadataManager::ValidateAsync(
    __in KString const& filename,
    __in KAllocator& allocator)
{
    KWString filePath(allocator, filename);
    KBlockFile::SPtr blockFileSPtr = nullptr;
    SharedException::CSPtr exception = nullptr;

    auto status = co_await KBlockFile::CreateSparseFileAsync(
        filePath,
        TRUE,
        KBlockFile::CreateDisposition::eOpenExisting,
        KBlockFile::CreateOptions::eInheritFileSecurity,
        blockFileSPtr,
        nullptr,
        allocator,
        METADATAMANAGER_TAG
    );

    KFinally([&] { if (blockFileSPtr != nullptr) blockFileSPtr->Close(); });

    ASSERT_IFNOT(NT_SUCCESS(status), "Unable to open file {0}", (wchar_t *)(filename));

    ktl::io::KFileStream::SPtr fileStreamSPtr = nullptr;
    status = ktl::io::KFileStream::Create(fileStreamSPtr, allocator);
    Diagnostics::Validate(status);

    status = co_await fileStreamSPtr->OpenAsync(*blockFileSPtr);
    ASSERT_IFNOT(NT_SUCCESS(status), "Unable to open file stream for file {0}", (wchar_t *)(filename));

    try
    {
        BlockHandle::SPtr footerHandleSPtr;

        // From the bottom, checksum, footer and the block offset starts.
        ULONG64 offset = fileStreamSPtr->GetLength() - FileFooter::SerializedSize() - sizeof(ULONG64);
        status = BlockHandle::Create(offset, FileFooter::SerializedSize(), allocator, footerHandleSPtr);
        Diagnostics::Validate(status);

        FileBlock<FileFooter::SPtr>::DeserializerFunc footerDeserializerFunc(&FileFooter::Read);
        auto footerSPtr = co_await FileBlock<FileFooter::SPtr>::ReadBlockAsync(*fileStreamSPtr, *footerHandleSPtr, footerDeserializerFunc, allocator, ktl::CancellationToken::None);

        if (footerSPtr->Version != MetadataManager::FileVersion)
        {
            //todo:add correct status
            throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
        }

        // Read and validate property section
        auto propertiesHandle = footerSPtr->PropertiesHandle;

        FileBlock<MetadataManagerFileProperties::SPtr>::DeserializerFunc propertyDeserializerFunc(&MetadataManagerFileProperties::Read);
        co_await FileBlock<MetadataManagerFileProperties::SPtr>::ReadBlockAsync(*fileStreamSPtr, *propertiesHandle, propertyDeserializerFunc, allocator, ktl::CancellationToken::None);
    }
    catch (ktl::Exception const& e)
    {
        exception = SharedException::Create(e, allocator);
    }

    if (fileStreamSPtr != nullptr && fileStreamSPtr->IsOpen())
    {
        status = co_await fileStreamSPtr->CloseAsync();
        Diagnostics::Validate(status);
    }

    if (exception != nullptr)
    {
        //clang compiler error, needs to assign before throw.
        auto ex = exception->Info;
        throw ex;
    }
}

ktl::Awaitable<void> MetadataManager::PopulateMetadataAsync(
    __in MetadataTable& metadataTable,
    __in KString const & filename,
    __in KAllocator& allocator,
    __in StoreTraceComponent & traceComponent)
{
    // TODO: KWString should be replaced by KString once ktl apis are fixed 
    KWString filePath(allocator, filename);
    KBlockFile::SPtr blockFileSPtr = nullptr;
    SharedException::CSPtr exception = nullptr;

    auto status = co_await KBlockFile::CreateSparseFileAsync(
        filePath,
        TRUE,
        KBlockFile::CreateDisposition::eOpenExisting,
        KBlockFile::CreateOptions::eInheritFileSecurity,
        blockFileSPtr,
        nullptr,
        allocator,
        METADATAMANAGER_TAG
    );

    KFinally([&] { if (blockFileSPtr != nullptr) blockFileSPtr->Close(); });

    ASSERT_IFNOT(NT_SUCCESS(status), "Unable to open file {0}", (wchar_t *)(filename));

    ktl::io::KFileStream::SPtr fileStreamSPtr = nullptr;
    ktl::io::KFileStream::Create(fileStreamSPtr, allocator);

    status = co_await fileStreamSPtr->OpenAsync(*blockFileSPtr);
    ASSERT_IFNOT(NT_SUCCESS(status), "Unable to open file stream for file {0}", (wchar_t *)(filename));
    try
    {
        co_await PopulateMetadataAsync(metadataTable, *fileStreamSPtr, fileStreamSPtr->GetLength(), allocator, traceComponent);
        metadataTable.MetadataFileSize = fileStreamSPtr->GetLength();
    }
    catch (ktl::Exception const& e)
    {
        exception = SharedException::Create(e, allocator);
    }

    if (fileStreamSPtr != nullptr && fileStreamSPtr->IsOpen())
    {
        status = co_await fileStreamSPtr->CloseAsync();
        Diagnostics::Validate(status);
    }

    if (exception != nullptr)
    {
        //clang compiler error, needs to assign before throw.
        auto ex = exception->Info;
        throw ex;
    }
}

ktl::Awaitable<ULONG32> MetadataManager::ReadDiskMetadataAsync(
   __in Data::IDictionary<ULONG32, FileMetadata::SPtr>& metadataTable,
   __in ktl::io::KStream& fileStream,
   __in MetadataManagerFileProperties& properties,
   __in KAllocator& allocator,
   __in StoreTraceComponent & traceComponent)
{
   ktl::io::KStream::SPtr  streamSPtr(&fileStream);
   auto startOffset = properties.MetadataHandleSPtr->Offset;
   auto endOffset = properties.MetadataHandleSPtr->EndOffset();
   ULONG32 metadataCount = 0;
   ULONG32 fileId = 0;

   if (startOffset + sizeof(LONG32) >= endOffset)
   {
      co_return fileId;
   }

   streamSPtr->SetPosition(startOffset);

   // Create a buffer
   KBuffer::SPtr metadataStream = nullptr;
   NTSTATUS status = KBuffer::Create((64 * 1024), metadataStream, allocator, METADATAMANAGER_TAG);
   Diagnostics::Validate(status);

   ULONG bytesRead = 0;
   status = co_await streamSPtr->ReadAsync(*metadataStream, bytesRead, 0, PropertyChunkMetadata::Size);
   Diagnostics::Validate(status);
   KInvariant(bytesRead == PropertyChunkMetadata::Size);

   BinaryReader reader(*metadataStream, allocator);
   auto propertyChunkMetadata = PropertyChunkMetadata::Read(reader);
   auto chunkSize = propertyChunkMetadata.BlockSize;
   streamSPtr->SetPosition(static_cast<LONGLONG>(streamSPtr->GetPosition() - PropertyChunkMetadata::Size));

   while (streamSPtr->GetPosition() + chunkSize + sizeof(ULONG64) <= endOffset)
   {
      // Read the entire chunk (plus the checksum and next chunk size) into memory.

      ULONG32 size = chunkSize + sizeof(ULONG64) + sizeof(ULONG32);

      bytesRead = 0;
      status = co_await streamSPtr->ReadAsync(*metadataStream, bytesRead, 0, size);
      Diagnostics::Validate(status);
      KInvariant(bytesRead == size);

      BinaryReader metadataReader(*metadataStream, allocator);

      // Read the checksum.
      metadataReader.Position = chunkSize;
      ULONG64 checksum = 0L;
      metadataReader.Read(checksum);

      // Re-compute the checksum.
      auto expectedChecksum = CRC64::ToCRC64(*metadataStream, 0, chunkSize);

      if (checksum != expectedChecksum)
      {
         // todo: InvalidDataException
         throw ktl::Exception(SF_STATUS_SERVICE_METADATA_MISMATCH);
      }

      // Deserialize the value into memory
      metadataReader.Position = sizeof(LONG32);
      ByteAlignedReaderWriterHelper::ReadPaddingUntilAligned(metadataReader);
      while (metadataReader.Position < chunkSize)
      {
         auto fileMetadata = FileMetadata::Read(metadataReader, allocator, traceComponent);
         if (metadataTable.ContainsKey(fileMetadata->FileId))
         {
            // todo: InvalidDataException
            throw ktl::Exception(SF_STATUS_SERVICE_METADATA_MISMATCH);
         }

         ULONG32 lFileId = fileMetadata->FileId;
         KInvariant(NT_SUCCESS(status));
         metadataTable.Add(lFileId, fileMetadata);
         metadataCount++;
      }

      // Read the next chunk size
      metadataReader.Position = chunkSize + sizeof(ULONG64);
      metadataReader.Read(chunkSize);
      streamSPtr->SetPosition(streamSPtr->GetPosition() - sizeof(ULONG32));
   }
   // Consistency checks
   if (streamSPtr->GetPosition() != static_cast<LONGLONG>(endOffset))
   {
      // todo: InvalidDataException
      throw ktl::Exception(SF_STATUS_SERVICE_METADATA_MISMATCH);
   }

   if (metadataCount != properties.FileCount)
   {
      // todo: InvalidDataException
      throw ktl::Exception(SF_STATUS_SERVICE_METADATA_MISMATCH);
   }

   co_return fileId;
}

ktl::Awaitable<void> MetadataManager::WriteAsync(
   __in MetadataTable& metadataTable,
   __in ktl::io::KStream& stream,
   __in KAllocator& allocator)
{
   ktl::io::KStream::SPtr streamSPtr(&stream);
   auto propertiesSPtr = co_await WriteDiskMetadataAsync(*metadataTable.Table, *streamSPtr, allocator);
   propertiesSPtr->CheckpointLSN = metadataTable.CheckpointLSN;

   // Write the properties
   FileBlock<MetadataManagerFileProperties::SPtr>::SerializerFunc propertiesSerializerFunction(propertiesSPtr.RawPtr(), &MetadataManagerFileProperties::Write);
   BlockHandle::SPtr propertiesHandleSPtr = nullptr;
   NTSTATUS status = co_await FileBlock<MetadataManagerFileProperties::SPtr>::WriteBlockAsync(*streamSPtr, propertiesSerializerFunction, allocator, ktl::CancellationToken::None, propertiesHandleSPtr);
   Diagnostics::Validate(status);

   // Write the footer
   FileFooter::SPtr footerSPtr = nullptr;
   FileFooter::Create(*propertiesHandleSPtr, MetadataManager::FileVersion, allocator, footerSPtr);

   FileBlock<FileFooter::SPtr>::SerializerFunc footerSerializerFunction(footerSPtr.RawPtr(), &FileFooter::Write);
   BlockHandle::SPtr footerHandleSPtr = nullptr;
   status = co_await FileBlock<FileFooter::SPtr>::WriteBlockAsync(*streamSPtr, footerSerializerFunction, allocator, ktl::CancellationToken::None, footerHandleSPtr);
   Diagnostics::Validate(status);

   // Finally, flush to disk
   status = co_await streamSPtr->FlushAsync();
   Diagnostics::Validate(status);
   co_return;
}

ktl::Awaitable<void> MetadataManager::WriteAsync(
    __in MetadataTable& metadataTable,
    __in KString const & filename,
    __in KAllocator& allocator)
{
    // TODO: KWString should be replaced by KString once ktl apis are fixed 
    KWString filePath(allocator, filename);
    KBlockFile::SPtr blockFileSPtr = nullptr;
    SharedException::CSPtr exception = nullptr;

    auto status = co_await KBlockFile::CreateSparseFileAsync(
        filePath,
        TRUE,
        KBlockFile::CreateDisposition::eCreateAlways,
        KBlockFile::eInheritFileSecurity,
        blockFileSPtr,
        nullptr,
        allocator,
        METADATAMANAGER_TAG
    );

    KFinally([&] { if (blockFileSPtr != nullptr) blockFileSPtr->Close(); });

    ASSERT_IFNOT(NT_SUCCESS(status), "Unable to create file {0}", (wchar_t *)(filename));

    ktl::io::KFileStream::SPtr fileStreamSPtr = nullptr;
    ktl::io::KFileStream::Create(fileStreamSPtr, allocator);

    status = co_await fileStreamSPtr->OpenAsync(*blockFileSPtr);
    ASSERT_IFNOT(NT_SUCCESS(status), "Unable to open file stream for file {0}", (wchar_t *)(filename));

    try
    {
        co_await WriteAsync(metadataTable, *fileStreamSPtr, allocator);
        metadataTable.MetadataFileSize = fileStreamSPtr->GetLength();
    }
    catch (ktl::Exception const& e)
    {
        exception = SharedException::Create(e, allocator);
    }

    if (fileStreamSPtr != nullptr && fileStreamSPtr->IsOpen())
    {
        status = co_await fileStreamSPtr->CloseAsync();
        Diagnostics::Validate(status);
    }

    if (exception != nullptr)
    {
        //clang compiler error, needs to assign before throw.
        auto ex = exception->Info;
        throw ex;
    }
}


 ktl::Awaitable<MetadataManagerFileProperties::SPtr> MetadataManager::WriteDiskMetadataAsync(
    __in Data::IDictionary<ULONG32, FileMetadata::SPtr>& metadataTable,
    __in ktl::io::KStream& outputStream,
    __in KAllocator& allocator)
{
   ktl::io::KStream::SPtr outputStreamSPtr(&outputStream);
   MetadataManagerFileProperties::SPtr propertiesSPtr= nullptr;
   NTSTATUS status = MetadataManagerFileProperties::Create(allocator, propertiesSPtr);
   Diagnostics::Validate(status);
   bool startOfChunk = true;

   // Set the underlying stream length- not a priority
   SharedBinaryWriter::SPtr metadataWriterSPtr = nullptr;
   status = SharedBinaryWriter::Create(allocator, metadataWriterSPtr);
   Diagnostics::Validate(status);

   KSharedPtr<IEnumerator<KeyValuePair<ULONG32, FileMetadata::SPtr>>> enumerator = metadataTable.GetEnumerator();
   
   while(enumerator->MoveNext())
   {
      KeyValuePair<ULONG32, FileMetadata::SPtr> entry = enumerator->Current();
      FileMetadata::SPtr metadataSPtr = entry.Value;

      if (startOfChunk)
      {
         // Leave space for an int 'size' at the front of the memory stream.
         // Leave space for size and RESERVED
         metadataWriterSPtr->Position += PropertyChunkMetadata::Size;
         startOfChunk = false;
      }

      metadataSPtr->Write(*metadataWriterSPtr);
      propertiesSPtr->FileCount++;

      // Flush the memory buffer periodically to disk.
      if (metadataWriterSPtr->Position >= MetadataManager::MemoryBufferFlushSize)
      {
         co_await FlushMemoryBufferAsync(*outputStreamSPtr, *metadataWriterSPtr);
         startOfChunk = true;
      }
   }

   // If there's any remaining buffered metadata in memory, flush them to disk.
   if (metadataWriterSPtr->Position > PropertyChunkMetadata::Size)
   {
      co_await FlushMemoryBufferAsync(*outputStreamSPtr, *metadataWriterSPtr);
   }

   // Update properties.
   BlockHandle::SPtr metadataHandleSPtr = nullptr;
   status = BlockHandle::Create(0, metadataWriterSPtr->Position, allocator, metadataHandleSPtr);
   Diagnostics::Validate(status);
   propertiesSPtr->MetadataHandleSPtr = *metadataHandleSPtr;

   co_return propertiesSPtr;
}

Awaitable<void> MetadataManager::SafeFileReplaceAsync(
    __in KString const & checkpointFileName,
    __in KString const & temporaryCheckpointFileName,
    __in KString const & backupCheckpointFileName,
    __in bool isValueAReferenceType,
    __in StoreTraceComponent & traceComponent,
    __in KAllocator & allocator)
{
    StoreEventSource::Events->MetadataManagerSafeFileReplace(traceComponent.PartitionId, traceComponent.TraceTag, L"starting");

    if (checkpointFileName.IsNull() || checkpointFileName.IsEmpty())
    {
        throw Exception(STATUS_INVALID_PARAMETER_1);
    }

    if (temporaryCheckpointFileName.IsNull() || temporaryCheckpointFileName.IsEmpty())
    {
        throw Exception(STATUS_INVALID_PARAMETER_2);
    }

    if (backupCheckpointFileName.IsNull() || backupCheckpointFileName.IsEmpty())
    {
        throw Exception(STATUS_INVALID_PARAMETER_3);
    }

    bool currentExists = Common::File::Exists(checkpointFileName.operator LPCWSTR());
    bool tempExists = Common::File::Exists(temporaryCheckpointFileName.operator LPCWSTR());
    bool backupExists = Common::File::Exists(backupCheckpointFileName.operator LPCWSTR());

    ASSERT_IFNOT(tempExists, "{0}: Expected temp checkpoint file {1} to exist", traceComponent.AssertTag, tempExists);

    // If the backup file exists, or the checkpoint file does not exist, then we need to validate the temp file before we delete the backup.
    if (backupExists || !currentExists)
    {
        try
        {
            StoreEventSource::Events->MetadataManagerSafeFileReplaceValidating(traceComponent.PartitionId, traceComponent.TraceTag, ToStringLiteral(checkpointFileName));
            co_await ValidateAsync(temporaryCheckpointFileName, allocator);
        }
        catch (ktl::Exception const & e)
        {
            KDynStringA stackString(allocator);
            Diagnostics::GetExceptionStackTrace(e, stackString);
            StoreEventSource::Events->StoreException(
                traceComponent.PartitionId, traceComponent.TraceTag,
                L"MetadaManager::SafeFileReplaceAsync",
                -1, 0, // TODO: Replace with constants in a way that won't break Linux build
                e.GetStatus(),
                Data::Utilities::ToStringLiteral(stackString));
            throw;
        }
    }

    // If a previous iteration failed, the backup file might exist.
    Common::ErrorCode errorCode;
    if (backupExists)
    {
       errorCode = Common::File::Delete2(backupCheckpointFileName.operator LPCWSTR());
       if (errorCode.IsSuccess() == false)
       {
          throw Exception(errorCode.ToHResult());
       }
    }

    // Previous replace could have failed in the middle before the next checkpoint file got renamed to current.
    if (!currentExists)
    {
        StoreEventSource::Events->MetadataManagerSafeFileReplaceMoving(traceComponent.PartitionId, traceComponent.TraceTag, ToStringLiteral(temporaryCheckpointFileName), ToStringLiteral(checkpointFileName));
        errorCode = Common::File::Move(temporaryCheckpointFileName.operator LPCWSTR(), checkpointFileName.operator LPCWSTR(), false);
        if (errorCode.IsSuccess() == false)
        {
            throw Exception(errorCode.ToHResult());
        }
    }
    else
    {
        // Current exists, so we must have gotten here only after writing a valid next checkpoint file.
        StoreEventSource::Events->MetadataManagerSafeFileReplaceReplacing(traceComponent.PartitionId, traceComponent.TraceTag, ToStringLiteral(temporaryCheckpointFileName), ToStringLiteral(checkpointFileName));

        errorCode = Common::File::Move(checkpointFileName.operator LPCWSTR(), backupCheckpointFileName.operator LPCWSTR(), false);
        if (errorCode.IsSuccess() == false)
        {
            throw Exception(errorCode.ToHResult());
        }

        errorCode = Common::File::Move(temporaryCheckpointFileName.operator LPCWSTR(), checkpointFileName.operator LPCWSTR(), false);
        if (errorCode.IsSuccess() == false)
        {
            throw Exception(errorCode.ToHResult());
        }

        // Since the move completed successfully, we need to delete the bkpFilePath. This is best-effort cleanup.
        StoreEventSource::Events->MetadataManagerSafeFileReplaceDeleting(traceComponent.PartitionId, traceComponent.TraceTag, ToStringLiteral(backupCheckpointFileName));
        errorCode = Common::File::Delete2(backupCheckpointFileName.operator LPCWSTR());
        if (errorCode.IsSuccess() == false)
        {
            throw Exception(errorCode.ToHResult());
        }
    }

    StoreEventSource::Events->MetadataManagerSafeFileReplace(traceComponent.PartitionId, traceComponent.TraceTag, L"completed");

    co_return;
}

 ktl::Awaitable<void>MetadataManager::FlushMemoryBufferAsync(
    __in ktl::io::KStream& stream,
    __in SharedBinaryWriter& binaryWriter)
 {
    SharedBinaryWriter::SPtr writerSPtr(&binaryWriter);
    ktl::io::KStream::SPtr outputStreamSPtr(&stream);
    auto chunkSize = writerSPtr->Position;
    writerSPtr->Position = 0;
    PropertyChunkMetadata propertyChunkMetadata(chunkSize);
    propertyChunkMetadata.Write(*writerSPtr);

    // Move to end.
    writerSPtr->Position = chunkSize;

    // Checksum the chunk
    ULONG64 checksum = CRC64::ToCRC64(*writerSPtr->GetBuffer(0), 0, writerSPtr->Position);
    writerSPtr->Write(checksum);

    // Write to disk
    NTSTATUS status = co_await outputStreamSPtr->WriteAsync(*writerSPtr->GetBuffer(0), 0, writerSPtr->Position);
    KInvariant(NT_SUCCESS(status));

    // Do not reset the buffer since the buffer is not re-used here.
    co_return;
 }

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data;
using namespace Data::StateManager;
using namespace Data::Utilities;

const LONGLONG CheckpointFileAsyncEnumerator::BlockSizeSectionSize = sizeof(LONG32);
const LONGLONG CheckpointFileAsyncEnumerator::CheckSumSectionSize = sizeof(ULONG64);
const ULONG32 CheckpointFileAsyncEnumerator::DesiredBlockSize = 32 * 1024;

NTSTATUS CheckpointFileAsyncEnumerator::Create(
    __in PartitionedReplicaId const & traceId,
    __in KWString const & fileName,
    __in KSharedArray<ULONG32> const & blockSize,
    __in CheckpointFileProperties const & properties,
    __in KAllocator & allocator,
    __out SPtr & result) noexcept
{
    result = _new(Checkpoint_FILE_ASYNC_ENUMERATOR_TAG, allocator) CheckpointFileAsyncEnumerator(
        traceId,
        fileName,
        blockSize,
        properties);

    if (result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(result->Status()))
    {
        return (SPtr(Ktl::Move(result)))->Status();
    }

    return STATUS_SUCCESS;
}

// Move the async enumerator to see if more SerializableMetadata exist.
// Algorithm:
// 1. Open the file and the stream if not already open.
// 2. Read from the cache if available.
// 3. Read the next block and populate the cache if available.
ktl::Awaitable<NTSTATUS> CheckpointFileAsyncEnumerator::GetNextAsync(
    __in CancellationToken const & cancellationToken,
    __out SerializableMetadata::CSPtr & result) noexcept
{
    UNREFERENCED_PARAMETER(cancellationToken);

    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // If the Enumerator has been disposed, the function should not be called.
    ASSERT_IFNOT(
        isDisposed_ == false,
        "{0}: MoveNextAsync: The enumerator is disposed.",
        TraceId);

    // Step 1: If not already done, open the file and the stream.
    // MCoskun: Uses default IOPriorityHint since this operation is called during Restore.
    if (fileStreamSPtr_ == nullptr)
    {
        KWString & tmpFileName = const_cast<KWString &>(fileName_);

        status = co_await KBlockFile::CreateSparseFileAsync(
            tmpFileName,
            FALSE, // IsWriteThrough.
            KBlockFile::eOpenExisting,
            KBlockFile::eSequentialAccess,
            fileSPtr_,
            nullptr,
            GetThisAllocator(),
            GetThisAllocationTag());
        if (NT_SUCCESS(status) == false)
        {
            StateManagerEventSource::Events->CheckpointFileError(
                TracePartitionId,
                ReplicaId,
                L"CheckpointFileAsyncEnumerator: Open KBlockFile failed.",
                status);

            result = nullptr;
            co_return status;
        }

        ktl::io::KFileStream::SPtr localFileStream = nullptr;
        status = ktl::io::KFileStream::Create(
            localFileStream,
            GetThisAllocator(),
            GetThisAllocationTag());
        if (NT_SUCCESS(status) == false)
        {
            StateManagerEventSource::Events->CheckpointFileError(
                TracePartitionId,
                ReplicaId,
                L"CheckpointFileAsyncEnumerator: Create KFileStream failed.",
                status);

            result = nullptr;
            co_return status;
        }

        status = co_await localFileStream->OpenAsync(*fileSPtr_);
        if (NT_SUCCESS(status) == false)
        {
            StateManagerEventSource::Events->CheckpointFileError(
                TracePartitionId,
                ReplicaId,
                L"CheckpointFileAsyncEnumerator: Open KFileStream failed.",
                status);

            result = nullptr;
            co_return status;
        }

        // Create the local FileStream and set the member only if open file stream is successful.
        // This is used to avoid CloseAsync call on fileStream if Create succeed but OpenAsync failed.
        fileStreamSPtr_ = Ktl::Move(localFileStream);
    }

    // Step 2: If there are items in the cache that have not been returned yet,
    // move the cursor and return true.
    LONG32 count = static_cast<LONG32>(currentSerializableMetadataCSPtr_->Count());
    if (currentIndex_ < count - 1)
    {
        ++currentIndex_;

        result = (*currentSerializableMetadataCSPtr_)[currentIndex_];
        co_return STATUS_SUCCESS;
    }

    // Step 3: Since there are no items in the cache, read the next block.
    // This call populates the cache (currentSerializableMetadataCSPtr_) if there is another block.
    status = co_await ReadBlockAsync();
    if (NT_SUCCESS(status))
    {
        result = (*currentSerializableMetadataCSPtr_)[currentIndex_];
    }

    co_return status;
}

// Reads the next block. If the block exists, updates the cache.
// Algorithm:
// 1. Set the file stream position if it is the first read
// 2. Check if another block exists. If not return.
// 3. Reset the cache
// 4. Read the block + checksum (Block includes the size)
// 5. Verify checksum
// 6. Populate the cache
ktl::Awaitable<NTSTATUS> CheckpointFileAsyncEnumerator::ReadBlockAsync() noexcept
{
    KShared$ApiEntry();
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // If it is the first time to read block, set the file stream position.
    if (blockIndex_ == 0)
    {
        fileStreamSPtr_->SetPosition(propertiesCSPtr_->MetadataHandle->Offset);
    }

    // Check if another block exists. If not return.
    if (blockIndex_ >= blockSize_->Count())
    {
        co_return STATUS_NOT_FOUND;
    }
    
    // Reset the cache
    currentSerializableMetadataCSPtr_->Clear();
    currentIndex_ = 0;

    ULONG64 endOffset = propertiesCSPtr_->MetadataHandle->EndOffset();
    if (static_cast<ULONG64>(fileStreamSPtr_->GetPosition()) + static_cast<ULONG64>((*blockSize_)[blockIndex_]) > endOffset)
    {
        co_return STATUS_INTERNAL_DB_CORRUPTION;
    }

    // Read blocks from the file.  Each state provider metadata is checksummed individually.
    KBuffer::SPtr itemStream = nullptr;
    status = KBuffer::Create((DesiredBlockSize * 2), itemStream, GetThisAllocator(), GetThisAllocationTag());
    if (NT_SUCCESS(status) == false)
    {
        StateManagerEventSource::Events->CheckpointFileError(
            TracePartitionId,
            ReplicaId,
            L"CheckpointFileAsyncEnumerator: Create KBuffer failed.",
            status);
        co_return status;
    }

    // Read the block into memory.
    itemStream->SetSize((*blockSize_)[blockIndex_]);
    ULONG bytesRead = 0;
    status = co_await fileStreamSPtr_->ReadAsync(*itemStream, bytesRead, 0, (*blockSize_)[blockIndex_]);
    if (NT_SUCCESS(status) == false)
    {
        StateManagerEventSource::Events->CheckpointFileError(
            TracePartitionId,
            ReplicaId,
            L"CheckpointFileAsyncEnumerator: Read FileStream failed.",
            status);
        co_return status;
    }

    BinaryReader itemReader(*itemStream, GetThisAllocator());
    if (NT_SUCCESS(itemReader.Status()) == false)
    {
        StateManagerEventSource::Events->CheckpointFileError(
            TracePartitionId,
            ReplicaId,
            L"CheckpointFileAsyncEnumerator: Create BinaryReader failed.",
            itemReader.Status());
        co_return itemReader.Status();
    }

    // Read to the end of the metadata section.
    ULONG64 endBlockOffset = (*blockSize_)[blockIndex_];

    while (itemReader.Position < endBlockOffset)
    {
        ULONG position = itemReader.Position;

        // Read the record size and validate it is not obviously corrupted.
        if (static_cast<ULONG64>(position) + BlockSizeSectionSize > endBlockOffset)
        {
            co_return STATUS_INTERNAL_DB_CORRUPTION;
        }

        ULONG32 recordSize;
        itemReader.Read(recordSize);
        ULONG32 recordSizeWithChecksum = recordSize + CheckSumSectionSize;

        if (static_cast<ULONG64>(position) + static_cast<ULONG64>(recordSize) > endBlockOffset)
        {
            co_return STATUS_INTERNAL_DB_CORRUPTION;
        }

        if (static_cast<ULONG64>(position) + static_cast<ULONG64>(recordSizeWithChecksum) > endBlockOffset)
        {
            co_return STATUS_INTERNAL_DB_CORRUPTION;
        }

        // Compute the checksum.
        ULONG64 computedChecksum = CRC64::ToCRC64(*itemStream, position, recordSize);

        // Read the checksum (checksum is after the record bytes).
        itemReader.Position = position + static_cast<ULONG>(recordSize);
        ULONG64 checksum;
        itemReader.Read(checksum);

        // Verify the checksum.
        if (checksum != computedChecksum)
        {
            co_return STATUS_INTERNAL_DB_CORRUPTION;
        }

        // Read and re-create the state provider metadata, now that the checksum is validated.
        itemReader.Position = position;
        SerializableMetadata::CSPtr metadataSPtr = nullptr;
        status = ReadMetadata(*PartitionedReplicaIdentifier, itemReader, GetThisAllocator(), metadataSPtr);
        CO_RETURN_ON_FAILURE(status);

        status = currentSerializableMetadataCSPtr_->Append(metadataSPtr);
        if (NT_SUCCESS(status) == false)
        {
            StateManagerEventSource::Events->CheckpointFileError(
                TracePartitionId,
                ReplicaId,
                L"CheckpointFileAsyncEnumerator: Append SerializableMetadata to KArray failed.",
                status);
            co_return status;
        }

        itemReader.Position = position + recordSizeWithChecksum;
    }

    // Block is cached then increase the block size array index.
    ++blockIndex_;
    co_return STATUS_SUCCESS;
}

// Read Metadata from the buffer
// Algorithm:
// 1. Read all properties
// 2. Check any data corruption 
// 3. Create the serializable metadata
NTSTATUS CheckpointFileAsyncEnumerator::ReadMetadata(
    __in Data::Utilities::PartitionedReplicaId const & traceId,
    __in Utilities::BinaryReader & reader,
    __in KAllocator & allocator,
    __out SerializableMetadata::CSPtr & result) noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    ULONG startPosition = reader.Position;

    LONG32 recordSize;
    reader.Read(recordSize);
    KUri::SPtr name;
    reader.Read(name);

    KString::SPtr type;
    reader.Read(type);

    FABRIC_SEQUENCE_NUMBER createLsn;
    FABRIC_SEQUENCE_NUMBER deleteLsn;
    reader.Read(createLsn);
    reader.Read(deleteLsn);

    // To read the OperationData, first read a LONG32 length. If length is -1
    // It means the OperationData is null. Otherwise, the length is the OperationData
    // buffers count. Then read each buffer. 
    OperationData::CSPtr initializationParameters = nullptr;
    status = CheckpointFileAsyncEnumerator::DeSerialize(reader, allocator, initializationParameters);
    if (NT_SUCCESS(status) == false)
    {
        StateManagerEventSource::Events->CheckpointFileError(
            traceId.TracePartitionId,
            traceId.ReplicaId,
            L"CheckpointFileAsyncEnumerator: DeSerialize failed.",
            status);
        result = nullptr;
        return status;
    }

    LONG32 metadataModeValue;
    reader.Read(metadataModeValue);
    MetadataMode::Enum metadataMode = static_cast<MetadataMode::Enum>(metadataModeValue);

    FABRIC_STATE_PROVIDER_ID stateProviderId;
    FABRIC_STATE_PROVIDER_ID parentStateProviderId;
    reader.Read(stateProviderId);
    reader.Read(parentStateProviderId);

    ULONG endPosition = reader.get_Position();
    LONG32 readRecordSize = static_cast<LONG32>(endPosition - startPosition);
    ASSERT_IFNOT(
        recordSize >= 0,
        "{0}: recordSize indicates size of the buffer, it must not be negative. recordSize: {1}.",
        traceId,
        recordSize);

    if (readRecordSize != recordSize)
    {
        result = nullptr;
        return STATUS_INTERNAL_DB_CORRUPTION;
    }

    SerializableMetadata::CSPtr serializableMetadataSPtr = nullptr;
    status = SerializableMetadata::Create(
        *name,
        *type,
        stateProviderId,
        parentStateProviderId,
        createLsn,
        deleteLsn,
        metadataMode,
        allocator,
        initializationParameters.RawPtr(),
        serializableMetadataSPtr);
    if (NT_SUCCESS(status) == false)
    {
        StateManagerEventSource::Events->CheckpointFileError(
            traceId.TracePartitionId,
            traceId.ReplicaId,
            L"CheckpointFileAsyncEnumerator: Create SerializableMetadataCSPtr failed.",
            status);
        result = nullptr;
        return status;
    }

    result = Ktl::Move(serializableMetadataSPtr);
    return STATUS_SUCCESS;
}

NTSTATUS CheckpointFileAsyncEnumerator::DeSerialize(
    __in Data::Utilities::BinaryReader & binaryReader,
    __in KAllocator & allocator,
    __out Data::Utilities::OperationData::CSPtr & result) noexcept
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    result = nullptr;

    LONG32 size = 0;
    binaryReader.Read(size);
    if (size == NULL_OPERATIONDATA)
    {
        return STATUS_SUCCESS;
    }

    KArray<KBuffer::CSPtr> items(allocator);
    RETURN_ON_FAILURE(items.Status());

    // If the size == 0, we return a empty operation data for both managed and native case.
    // If the size > 0, it is managed code, the size indicates the buffer size, and then we 
    //      return the operation data based on the single buffer we read from stream.
    // If the size < 0, it is native code, the size indicates the buffer count, then we read
    //      the corresponding buffers and create operation data.
    if (size > 0)
    {
        KBuffer::SPtr buffer = nullptr;
        status = KBuffer::Create(
            size,
            buffer,
            allocator,
            Checkpoint_FILE_ASYNC_ENUMERATOR_TAG);
        RETURN_ON_FAILURE(status);
        binaryReader.Read(size, buffer);
        status = items.Append(buffer.RawPtr());
        RETURN_ON_FAILURE(status);
    }
    else if (size < 0)
    {
        size = ~size;
        for (LONG32 i = 0; i < size; i++)
        {
            ULONG32 bufferSize;
            binaryReader.Read(bufferSize);
            KBuffer::SPtr buffer = nullptr;
            status = KBuffer::Create(
                bufferSize,
                buffer,
                allocator,
                Checkpoint_FILE_ASYNC_ENUMERATOR_TAG);
            RETURN_ON_FAILURE(status);
            binaryReader.Read(bufferSize, buffer);
            status = items.Append(buffer.RawPtr());
            RETURN_ON_FAILURE(status);
        }
    }
    
    result = OperationData::Create(items, allocator);
    return STATUS_SUCCESS;
}

NTSTATUS CheckpointFileAsyncEnumerator::Reset() noexcept
{
    return STATUS_NOT_IMPLEMENTED;
}

// Close FileStream and File
ktl::Awaitable<void> CheckpointFileAsyncEnumerator::CloseAsync()
{
    KShared$ApiEntry()

    if (isDisposed_ == true)
    {
        co_return;
    }

    // Note: Handle the case if KBlockFile throw exception, fileSPtr_ will be nullptr.
    if (fileStreamSPtr_ != nullptr)
    {
        NTSTATUS status = co_await fileStreamSPtr_->CloseAsync();
        ASSERT_IFNOT(
            NT_SUCCESS(status),
            "{0}: CheckpointFileAsyncEnumerator: CloseAsync: Close file stream failed. Status: {1} FileName: {2}",
            TraceId,
            status,
            static_cast<LPCWSTR>(fileName_));
    }

    // Note: Handle the case if KBlockFile throw exception, fileSPtr_ will be nullptr.
    if (fileSPtr_ != nullptr)
    {
        fileSPtr_->Close();
    }

    isDisposed_ = true;

    co_return;
}

void CheckpointFileAsyncEnumerator::Dispose()
{
    ASSERT_IFNOT(
        false,
        "{0}: CheckpointFileAsyncEnumerator: Dispose: Because filestream has to be closed asynchronously, CloseAsync should be used instead.",
        TraceId);
}

CheckpointFileAsyncEnumerator::CheckpointFileAsyncEnumerator(
    __in PartitionedReplicaId const & traceId,
    __in KWString const & fileName,
    __in KSharedArray<ULONG32> const & blockSize,
    __in CheckpointFileProperties const & properties) noexcept
    : KObject<CheckpointFileAsyncEnumerator>()
    , KShared<CheckpointFileAsyncEnumerator>()
    , PartitionedReplicaTraceComponent(traceId)
    , fileName_(fileName)
    , propertiesCSPtr_(&properties)
    , blockSize_(&blockSize)
{
    currentSerializableMetadataCSPtr_ = _new(GetThisAllocationTag(), GetThisAllocator())KSharedArray<SerializableMetadata::CSPtr>;
    if (currentSerializableMetadataCSPtr_ == nullptr)
    {
        SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    SetConstructorStatus(currentSerializableMetadataCSPtr_->Status());
}

CheckpointFileAsyncEnumerator::~CheckpointFileAsyncEnumerator()
{
    ASSERT_IFNOT(isDisposed_ == true, "CheckpointFileAsyncEnumerator must be disposed,");
}

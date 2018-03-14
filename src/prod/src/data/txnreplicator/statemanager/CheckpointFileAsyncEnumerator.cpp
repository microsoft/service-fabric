// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data;
using namespace Data::StateManager;
using namespace Data::Utilities;

CheckpointFileAsyncEnumerator::SPtr CheckpointFileAsyncEnumerator::Create(
    __in PartitionedReplicaId const & traceId,
    __in KWString const & fileName,
    __in KSharedArray<ULONG32> const & blockSize,
    __in CheckpointFileProperties const & properties,
    __in KAllocator & allocator)
{
    CheckpointFileAsyncEnumerator * pointer = _new(Checkpoint_FILE_ASYNC_ENUMERATOR_TAG, allocator) CheckpointFileAsyncEnumerator(
        traceId,
        fileName,
        blockSize,
        properties);
    THROW_ON_ALLOCATION_FAILURE(pointer);
    return Ktl::Move(SPtr(pointer));
}

// Get current SerializableMetadataCSPtr
SerializableMetadata::CSPtr CheckpointFileAsyncEnumerator::GetCurrent()
{
    // If the Enumberator has been disposed, the function should not be called.
    ASSERT_IFNOT(
        isDisposed_ == false, 
        "{0}: GetCurrent: The enumerator is disposed.", 
        TraceId);

    return (*currentSerializableMetadataCSPtr_)[currentIndex_];
}

// Move the async enumerator to see if more SerializableMetadata exist.
// Algorithm:
// 1. Open the file and the stream if not already open.
// 2. Read from the cache if avaiable.
// 3. Read the next block and populate the cache if available.
ktl::Awaitable<bool> CheckpointFileAsyncEnumerator::MoveNextAsync(
    __in CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // If the Enumberator has been disposed, the function should not be called.
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
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"CheckpointFileAsyncEnumerator: Open KBlockFile failed.",
            Helper::CheckpointFile);

        status = ktl::io::KFileStream::Create(
            fileStreamSPtr_,
            GetThisAllocator(),
            GetThisAllocationTag());
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"CheckpointFileAsyncEnumerator: Create KFileStream failed.",
            Helper::CheckpointFile);

        status = co_await fileStreamSPtr_->OpenAsync(*fileSPtr_);
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"CheckpointFileAsyncEnumerator: Open KFileStream failed.",
            Helper::CheckpointFile);
    }

    // Step 2: If there are items in the cache that have not been returned yet,
    // move the cursor and return true.
    LONG32 count = static_cast<LONG32>(currentSerializableMetadataCSPtr_->Count());
    if (currentIndex_ < count - 1)
    {
        ++currentIndex_;
        co_return true;
    }

    // Step 3: Since there are no items in the cache, read the next block.
    // This call populates the cache (currentSerializableMetadataCSPtr_) if there is another block.
    bool isDrained = co_await ReadBlockAsync();
    co_return isDrained;
}

// Reads the next block. If the block exists, updates the cache.
// Algorithm:
// 1. Set the file stream position if it is the fisrt read
// 2. Check if another block exists. If not return.
// 3. Reset the cache
// 4. Read the block + checksum (Block includes the size)
// 5. Verify checksum
// 6. Populate the cache
ktl::Awaitable<bool> CheckpointFileAsyncEnumerator::ReadBlockAsync()
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
        co_return false;
    }
    
    // Reset the cache
    currentSerializableMetadataCSPtr_->Clear();
    currentIndex_ = 0;

    ULONG64 endOffset = propertiesCSPtr_->MetadataHandle->EndOffset();
    if (static_cast<ULONG64>(fileStreamSPtr_->GetPosition()) + static_cast<ULONG64>((*blockSize_)[blockIndex_]) > endOffset)
    {
        throw Exception(STATUS_INTERNAL_DB_CORRUPTION);
    }

    // Read blocks from the file.  Each state provider metadata is checksummed individually.
    KBuffer::SPtr itemStream = nullptr;
    status = KBuffer::Create((DesiredBlockSize * 2), itemStream, GetThisAllocator(), CHECKPOINTFILE_TAG);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"CheckpointFileAsyncEnumerator: Create KBuffer failed.",
        Helper::CheckpointFile);

    // Read the block into memory.
    itemStream->SetSize((*blockSize_)[blockIndex_]);
    ULONG bytesRead = 0;
    status = co_await fileStreamSPtr_->ReadAsync(*itemStream, bytesRead, 0, (*blockSize_)[blockIndex_]);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"CheckpointFileAsyncEnumerator: Read FileStream failed.",
        Helper::CheckpointFile);

    BinaryReader itemReader(*itemStream, GetThisAllocator());

    // Read to the end of the metadata section.
    ULONG64 endBlockOffset = (*blockSize_)[blockIndex_];

    while (itemReader.Position < endBlockOffset)
    {
        ULONG position = itemReader.Position;

        // Read the record size and validate it is not obviously corrupted.
        if (static_cast<ULONG64>(position) + BlockSizeSectionSize > endBlockOffset)
        {
            throw Exception(STATUS_INTERNAL_DB_CORRUPTION);
        }

        ULONG32 recordSize;
        itemReader.Read(recordSize);
        ULONG32 recordSizeWithChecksum = recordSize + CheckSumSectionSize;

        if (static_cast<ULONG64>(position) + static_cast<ULONG64>(recordSize) > endBlockOffset)
        {
            throw Exception(STATUS_INTERNAL_DB_CORRUPTION);
        }

        if (static_cast<ULONG64>(position) + static_cast<ULONG64>(recordSizeWithChecksum) > endBlockOffset)
        {
            throw Exception(STATUS_INTERNAL_DB_CORRUPTION);
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
            throw Exception(STATUS_INTERNAL_DB_CORRUPTION);
        }

        // Read and re-create the state provider metadata, now that the checksum is validated.
        itemReader.Position = position;
        SerializableMetadata::CSPtr metadataSPtr = ReadMetadata(*PartitionedReplicaIdentifier, itemReader, GetThisAllocator());
        status = currentSerializableMetadataCSPtr_->Append(metadataSPtr);
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"CheckpointFileAsyncEnumerator: Append SerializableMetadata to KArray failed.",
            Helper::CheckpointFile);
        itemReader.Position = position + recordSizeWithChecksum;
    }

    // Block is cached then increase the block size array index.
    ++blockIndex_;
    co_return true;
}

// Read Metadata from the buffer
// Algorithm:
// 1. Read all properties
// 2. Check any data corruption 
// 3. Create the serializable metadata
SerializableMetadata::CSPtr CheckpointFileAsyncEnumerator::ReadMetadata(
    __in Data::Utilities::PartitionedReplicaId const & traceId,
    __in BinaryReader & reader,
    __in KAllocator & allocator)
{
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
    OperationData::CSPtr initializationParameters = CheckpointFileAsyncEnumerator::DeSerialize(reader, allocator);

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
        throw Exception(STATUS_INTERNAL_DB_CORRUPTION);
    }

    SerializableMetadata::CSPtr serializableMetadataSPtr = nullptr;
    NTSTATUS status = SerializableMetadata::Create(
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
    Helper::ThrowIfNecessary(
        status,
        traceId.TracePartitionId,
        traceId.ReplicaId,
        L"CheckpointFileAsyncEnumerator: Create SerializableMetadataCSPtr failed.",
        Helper::CheckpointFile);

    return Ktl::Move(serializableMetadataSPtr);
}

Data::Utilities::OperationData::CSPtr CheckpointFileAsyncEnumerator::DeSerialize(
    __in Data::Utilities::BinaryReader & binaryReader,
    __in KAllocator & allocator)
{
    NTSTATUS status;
    LONG32 size = 0;
    binaryReader.Read(size);
    if (size == NULL_OPERATIONDATA)
    {
        return nullptr;
    }
    KArray<KBuffer::CSPtr> items(allocator);
    THROW_ON_FAILURE(items.Status());
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
        THROW_ON_FAILURE(status);
        binaryReader.Read(size, buffer);
        status = items.Append(buffer.RawPtr());
        THROW_ON_FAILURE(status);
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
            THROW_ON_FAILURE(status);
            binaryReader.Read(bufferSize, buffer);
            status = items.Append(buffer.RawPtr());
            THROW_ON_FAILURE(status);
        }
    }
    return OperationData::Create(items, allocator);
}

void CheckpointFileAsyncEnumerator::Reset()
{
    throw ktl::Exception(STATUS_NOT_IMPLEMENTED);
}

// Close FileStream and File
ktl::Awaitable<void> CheckpointFileAsyncEnumerator::CloseAsync()
{
    KShared$ApiEntry()

    if (isDisposed_ == true)
    {
        co_return;
    }

    co_await fileStreamSPtr_->CloseAsync();
    fileSPtr_->Close();

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

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data;
using namespace Data::StateManager;
using namespace Data::Utilities;

const ULONG32 CheckpointFile::CopyProtocolVersion = 1;
const ULONG32 CheckpointFile::DesiredBlockSize = 32 * 1024;
const ULONG32 CheckpointFile::Version = 1;
const LONGLONG CheckpointFile::BlockSizeSectionSize = sizeof(LONG32);
const LONGLONG CheckpointFile::CheckSumSectionSize = sizeof(ULONG64);

NTSTATUS CheckpointFile::Create(
    __in Utilities::PartitionedReplicaId const & traceId,
    __in KWString const & filePath,
    __in KAllocator & allocator,
    __out SPtr & result) noexcept
{
    result = _new(CHECKPOINTFILE_TAG, allocator) CheckpointFile(traceId, filePath);
    if (result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(result->Status()))
    {
        // null Result while fetching failure status with no extra AddRefs or Releases; should opt very well
        return (SPtr(Ktl::Move(result)))->Status();
    }

    return STATUS_SUCCESS;
}

// Provides a safe file replace that replaces the current checkpoint file with the next checkpoint file.
// 
// Algorithm
// 1. Delete backup if exists.
// 2. Check if current exists. If not, move next to current (after verifying next), return
// 3. Move current to backup
// 4. Move next to current
// 5. Delete backup.
//
// <remarks>
// Code depends on only being called in cases where next (tmp) checkpoint exists.
// </remarks>
Awaitable<void> CheckpointFile::SafeFileReplaceAsync(
    __in PartitionedReplicaId const & partitionedReplicaId,
    __in KString const & checkpointFileName,
    __in KString const & nextCheckpointFileName,
    __in KString const & backupCheckpointFileName,
    __in CancellationToken const & cancellationToken,
    __in KAllocator & allocator)
{
    ASSERT_IF(checkpointFileName.IsNull() || checkpointFileName.IsEmpty(), "{0}: SafeFileReplaceAsync: Invalid current checkpoint name.", partitionedReplicaId.TraceId);
    ASSERT_IF(nextCheckpointFileName.IsNull() || nextCheckpointFileName.IsEmpty(), "{0}: SafeFileReplaceAsync: Invalid next checkpoint name.", partitionedReplicaId.TraceId);
    ASSERT_IF(backupCheckpointFileName.IsNull() || backupCheckpointFileName.IsEmpty(), "{0}: SafeFileReplaceAsync: Invalid backup checkpoint name.", partitionedReplicaId.TraceId);

    // 1. Delete backup if exists.
    Common::ErrorCode errorCode;
    if (Common::File::Exists(static_cast<LPCWSTR>(backupCheckpointFileName)))
    {
        errorCode = Common::File::Delete2(static_cast<LPCWSTR>(backupCheckpointFileName));
        Helper::ThrowIfNecessary(
            errorCode, 
            Common::Guid(partitionedReplicaId.PartitionId),
            partitionedReplicaId.ReplicaId, 
            L"SafeFileReplaceAsync: Deleting backup at start.", 
            Helper::CheckpointFile);
    }

    // 2. Check if current exists. If not, move next to current (after verifying next), return
    if (Common::File::Exists(static_cast<LPCWSTR>(checkpointFileName)) == false)
    {
        // Previous replace could have failed in the middle before the next checkpoint file got renamed to current.
        // Validate the next file is complete (this will throw InvalidDataException if it's not valid).      
        KWString filePath = KWString(allocator, nextCheckpointFileName.ToUNICODE_STRING());

        // For the SerializationMode, there is no difference between passing Native or Managed here. Since the next ReadAsync
        // call only read the file footer and properties to validate the file, which Native and Managed have the exactly format.
        CheckpointFile::SPtr checkpointFileSPtr = nullptr;
        NTSTATUS status = CheckpointFile::Create(
            partitionedReplicaId, 
            filePath, 
            allocator, 
            checkpointFileSPtr);
        Helper::ThrowIfNecessary(
            status,
            Common::Guid(partitionedReplicaId.PartitionId),
            partitionedReplicaId.ReplicaId,
            L"SafeFileReplaceAsync: Create CheckpointFile.",
            Helper::CheckpointFile);
        co_await checkpointFileSPtr->ReadAsync(cancellationToken);

        // Next checkpoint file is valid. Move it to be current.
        // MCoskun: Note that File::Move is MOVEFILE_WRITE_THROUGH by default.
        // Not using this flag can cause undetected dataloss.
        // Note: Set Common::File::Move throwIfFail to false to invoke returning error code, otherwise it will throw system_error.
        errorCode = Common::File::Move(static_cast<LPCWSTR>(nextCheckpointFileName), static_cast<LPCWSTR>(checkpointFileName), false);
        Helper::ThrowIfNecessary(
            errorCode, 
            Common::Guid(partitionedReplicaId.PartitionId),
            partitionedReplicaId.ReplicaId,
            L"SafeFileReplaceAsync: Moving next to current.",
            Helper::CheckpointFile);

        co_return;
    }

    // Current exists, so we must have gotten here only after writing a valid next checkpoint file.
    // MCoskun: Note that FabricFile.Move is MOVEFILE_WRITE_THROUGH by default.
    // NTFS guarantees that this operation will not be lost as long as MOVEFILE_WRITE_THROUGH is on.
    // If NTFS metadata log's tail is not flushed yet, it will be flushed before this operation is logged with FUA.
    // #9291020: Using ReplaceFile here instead of MoveFile with MOVEFILE_WRITE_THROUGH can cause data-loss
    // This is because ReplaceFile is not guaranteed to be persisted even after it returns.

    // 3. Move current to backup
    errorCode = Common::File::Move(static_cast<LPCWSTR>(checkpointFileName), static_cast<LPCWSTR>(backupCheckpointFileName), false);
    Helper::ThrowIfNecessary(
        errorCode, 
        Common::Guid(partitionedReplicaId.PartitionId),
        partitionedReplicaId.ReplicaId,
        L"SafeFileReplaceAsync: Moving current to backup.",
        Helper::CheckpointFile);

    // 4. Move next to current
    errorCode = Common::File::Move(static_cast<LPCWSTR>(nextCheckpointFileName), static_cast<LPCWSTR>(checkpointFileName), false);
    Helper::ThrowIfNecessary(
        errorCode,
        Common::Guid(partitionedReplicaId.PartitionId),
        partitionedReplicaId.ReplicaId,
        L"SafeFileReplaceAsync: Moving next to current.",
        Helper::CheckpointFile);

    // 5. Delete backup, now that we've safely replaced the file.
    // It is safe to ignore this failure since the next safe file replace async will redo it.
    errorCode = Common::File::Delete2(static_cast<LPCWSTR>(backupCheckpointFileName));
    Helper::ThrowIfNecessary(
        errorCode, 
        Common::Guid(partitionedReplicaId.PartitionId),
        partitionedReplicaId.ReplicaId,
        L"SafeFileReplaceAsync: Delete backup.",
        Helper::CheckpointFile);

    co_return;
}

// Gets the copy data from the in-memory state provider metadata array.
OperationDataEnumerator::SPtr CheckpointFile::GetCopyData(
    __in Data::Utilities::PartitionedReplicaId const & traceId,
    __in KArray<SerializableMetadata::CSPtr> const & stateArray,
    __in FABRIC_REPLICA_ID targetReplicaId,
    __in SerializationMode::Enum serailizationMode,
    __in KAllocator & allocator)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // metadataCount used for trace later
    ULONG32 metadataCount = 0;

    OperationDataEnumerator::SPtr operationDataEnumerator;
    status = OperationDataEnumerator::Create(allocator, operationDataEnumerator);
    Helper::ThrowIfNecessary(
        status,
        traceId.TracePartitionId,
        traceId.ReplicaId,
        L"GetCopyData: Create OperationDataEnumerator failed.",
        Helper::CheckpointFile);

    BinaryWriter itemWriter(allocator);
    Helper::ThrowIfNecessary(
        itemWriter.Status(),
        traceId.TracePartitionId,
        traceId.ReplicaId,
        L"GetCopyData: Create BinaryWriter failed.",
        Helper::CheckpointFile);

    // Send the copy operation protocol version number first.
    {
        itemWriter.Write(CopyProtocolVersion);
        itemWriter.Write(static_cast<byte>(StateManagerCopyOperation::Version));

        KBuffer::SPtr versionData = nullptr;
        status = KBuffer::CreateOrCopy(versionData, itemWriter.GetBuffer(0, itemWriter.Position).RawPtr(), allocator);
        Helper::ThrowIfNecessary(
            status,
            traceId.TracePartitionId,
            traceId.ReplicaId,
            L"GetCopyData: Copy Buffer failed.",
            Helper::CheckpointFile);
        ASSERT_IFNOT(
            versionData != nullptr,
            "{0}: GetCopyData: versionData is nullptr.",
            traceId.TraceId);

        status = operationDataEnumerator->Add(*versionData);
        Helper::ThrowIfNecessary(
            status,
            traceId.TracePartitionId,
            traceId.ReplicaId,
            L"GetCopyData: Add versionData to OperationDataEnumerator failed.",
            Helper::CheckpointFile);

        StateManagerEventSource::Events->CheckpointFile_GetCurrentStateStart(
            traceId.TracePartitionId,
            traceId.ReplicaId,
            targetReplicaId,
            serailizationMode,
            CheckpointFile::CopyProtocolVersion,
            itemWriter.Position);
    }

    // Send the metadata.
    int copyOperationDataIndex = 0;
    wstring traceMessage;
    Common::StringWriter copyTraceMessageWriter(traceMessage);

    // Send the state provider metadata in chunks.
    itemWriter.Position = 0;
    for (SerializableMetadata::CSPtr serializableMetadata : stateArray)
    {
        WriteMetadata(traceId, itemWriter, *serializableMetadata, serailizationMode);
        metadataCount++;

        copyTraceMessageWriter.Write("{0}:{1} ", MetadataMode::ToChar(serializableMetadata->MetadataMode), serializableMetadata->StateProviderId);

        if (itemWriter.Position >= DesiredBlockSize)
        {
            // Send a chunk of state provider metadata.
            itemWriter.Write(static_cast<byte>(StateManagerCopyOperation::StateProviderMetadata));

            KBuffer::SPtr metadataBuffer = nullptr;
            status = KBuffer::CreateOrCopy(metadataBuffer, itemWriter.GetBuffer(0, itemWriter.Position).RawPtr(), allocator);
            Helper::ThrowIfNecessary(
                status,
                traceId.TracePartitionId,
                traceId.ReplicaId,
                L"GetCopyData: Copy Metadata chunk failed.",
                Helper::CheckpointFile);
            ASSERT_IFNOT(
                metadataBuffer != nullptr,
                "{0}: GetCopyData: metadataBuffer is nullptr.",
                traceId.TraceId);

            status = operationDataEnumerator->Add(*metadataBuffer);
            Helper::ThrowIfNecessary(
                status,
                traceId.TracePartitionId,
                traceId.ReplicaId,
                L"GetCopyData: Add metadataBuffer to OperationDataEnumerator failed.",
                Helper::CheckpointFile);

            // Flush the writer before tracing.
            copyTraceMessageWriter.Flush();
            StateManagerEventSource::Events->CheckpointFile_GetCurrentState(
                traceId.TracePartitionId,
                traceId.ReplicaId,
                targetReplicaId,
                serailizationMode,
                copyOperationDataIndex,
                itemWriter.Position,
                traceMessage);

            // Reset tracing.
            traceMessage.clear();
            ++copyOperationDataIndex;

            // Reset the item memory stream for the next chunk.
            itemWriter.Position = 0;
        }
    }

    // Send the last chunk, if any exists.
    if (itemWriter.Position > 0)
    {
        itemWriter.Write(static_cast<byte>(StateManagerCopyOperation::StateProviderMetadata));

        KBuffer::SPtr metadataBuffer = nullptr;
        status = KBuffer::CreateOrCopy(metadataBuffer, itemWriter.GetBuffer(0, itemWriter.Position).RawPtr(), allocator);
        Helper::ThrowIfNecessary(
            status,
            traceId.TracePartitionId,
            traceId.ReplicaId,
            L"GetCopyData: Copy last Metadata chunk failed.",
            Helper::CheckpointFile);
        ASSERT_IFNOT(
            metadataBuffer != nullptr,
            "{0}: GetCopyData: last metadataBuffer is nullptr.",
            traceId.TraceId);

        status = operationDataEnumerator->Add(*metadataBuffer);
        Helper::ThrowIfNecessary(
            status,
            traceId.TracePartitionId,
            traceId.ReplicaId,
            L"GetCopyData: Add last metadataBuffer to OperationDataEnumerator failed.",
            Helper::CheckpointFile);

        copyTraceMessageWriter.Flush();
        StateManagerEventSource::Events->CheckpointFile_GetCurrentState(
            traceId.TracePartitionId,
            traceId.ReplicaId,
            targetReplicaId,
            serailizationMode,
            copyOperationDataIndex,
            itemWriter.Position,
            traceMessage);
    }

    return operationDataEnumerator;
}

// Reads the state provider metadata from the copy data.
KSharedPtr<SerializableMetadataEnumerator> CheckpointFile::ReadCopyData(
    __in Data::Utilities::PartitionedReplicaId const & traceId,
    __in_opt OperationData const * const operationData,
    __in KAllocator & allocator)
{
    ASSERT_IFNOT(
        operationData != nullptr,
        "{0}: ReadCopyData: Input OperationData is nullptr.",
        traceId.TraceId);
    ASSERT_IFNOT(
        operationData->BufferCount > 0,
        "{0}: ReadCopyData: Input OperationData is empty.",
        traceId.TraceId);

    // Get the data 
    KBuffer::CSPtr itemData = (*operationData)[0];

    // get length here, which is used for reading last byte operationType and indicating the end position
    ULONG length = itemData->QuerySize();
    ASSERT_IFNOT(
        length >= 1,
        "{0}: ReadCopyData: KBuffer size should be positive. Size: {1}.",
        traceId.TraceId,
        length);

    byte const * point = static_cast<byte const *>(itemData->GetBuffer());

    // Read the last byte of the buffer, which is the operationType
    byte operationType = point[length - 1];

    if (operationType == StateManagerCopyOperation::Version)
    {
        BinaryReader itemReader(*(itemData), allocator);
        Helper::ThrowIfNecessary(
            itemReader.Status(),
            traceId.TracePartitionId,
            traceId.ReplicaId,
            L"ReadCopyData: Create BinaryReader failed.",
            Helper::CheckpointFile);

        // Verify the copy protocol version is known.
        ULONG32 copyProtocolVersion;
        itemReader.Read(copyProtocolVersion);

        // Verify the data is correct 
        if (copyProtocolVersion != CopyProtocolVersion)
        {
            throw Exception(STATUS_INTERNAL_DB_CORRUPTION);
        }
    }
    else if (operationType == StateManagerCopyOperation::StateProviderMetadata)
    {
        // The end position is length - 1, because we dont need to read the last byte operationType anymore
        SerializableMetadataEnumerator::SPtr result = nullptr;
        NTSTATUS status = SerializableMetadataEnumerator::Create(traceId, *(itemData), allocator, result);
        Helper::ThrowIfNecessary(
            status,
            traceId.TracePartitionId,
            traceId.ReplicaId,
            L"ReadCopyData: Create SerializableMetadataEnumerator failed.",
            Helper::CheckpointFile);

        return result;
    }
    else
    {
        throw Exception(STATUS_INTERNAL_DB_CORRUPTION);
    }

    return nullptr;
}

// Write the serializable metadatas to the checkpoint file
// Algorithm:
// 1. Create the file and the file stream.
// 2. Write the state provider metadata.
// 3. Write properties.
// 4. Write the footer.
// 5. Close the file stream and the file.
Awaitable<void> CheckpointFile::WriteAsync(
    __in KSharedArray<SerializableMetadata::CSPtr> const & metadataArray,
    __in SerializationMode::Enum serailizationMode,
    __in bool allowPrepareCheckpointLSNToBeInvalid,
    __in FABRIC_SEQUENCE_NUMBER prepareCheckpointLSN,
    __in CancellationToken const & cancellationToken)
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // #12249219: Without the "allowPrepareCheckpointLSNToBeInvalid" being set to true in the backup code path, 
    // It is possible for all backups before the first checkpoint after the upgrade to fail.
    // If the code has the PrepareCheckpointLSN property, it must be larger than or equal to FABRIC_AUTO_SEQUENCE_NUMBER(0).
    ASSERT_IFNOT(
        allowPrepareCheckpointLSNToBeInvalid || (prepareCheckpointLSN >= FABRIC_AUTO_SEQUENCE_NUMBER),
        "{0}: WriteAsync: In the write path, prepareCheckpointLSN must be larger or equal to 0. PrepareCheckpointLSN: {1}.",
        TraceId,
        prepareCheckpointLSN);

    // Step 1: Create the file and the file stream.
    // Note: Sparse file is used to make disk space usage more efficient by only allocating hard disk drive 
    // space to a file in areas where it contains nonzero data, so in this way the disk space is saved.
    // Another benefit is even without sufficient free space, we can also create large files.
    KBlockFile::SPtr fileSPtr = nullptr;
    status = co_await KBlockFile::CreateSparseFileAsync(
        const_cast<KWString &>(filePath_),
        FALSE,
        KBlockFile::eCreateAlways,
        static_cast<KBlockFile::CreateOptions>(KBlockFile::eSequentialAccess + KBlockFile::eInheritFileSecurity),
        fileSPtr,
        nullptr,
        GetThisAllocator(),
        GetThisAllocationTag());
    Helper::ThrowIfNecessary(
        status, 
        TracePartitionId,
        ReplicaId, 
        L"WriteAsync: Create sparse file.",
        Helper::CheckpointFile);
    KFinally([&] {fileSPtr->Close(); });
    
    io::KFileStream::SPtr fileStreamSPtr = nullptr;
    status = io::KFileStream::Create(fileStreamSPtr, GetThisAllocator(), GetThisAllocationTag());
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"WriteAsync: Create file stream.",
        Helper::CheckpointFile);

    status = co_await fileStreamSPtr->OpenAsync(*fileSPtr);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"WriteAsync: Opening file stream.",
        Helper::CheckpointFile);

    SharedException::CSPtr exceptionSPtr = nullptr;
    try
    {
        // #9165663: KTL SetSystemIoPriorityHintAsync is required.
        // fileSPtr->SetSystemIoPriorityHint(IO_PRIORITY_HINT::IoPriorityLow);

        // Step 2: Write the state provider metadata.
        stateProviderMetadata_ = &metadataArray;
        co_await this->WriteMetadataAsync(
            serailizationMode, 
            allowPrepareCheckpointLSNToBeInvalid,
            prepareCheckpointLSN, 
            cancellationToken, 
            *fileStreamSPtr);

        // Step 3: Write properties.
        BlockHandle::SPtr propertiesHandleSPtr = co_await this->WritePropertiesAsync(cancellationToken, *fileStreamSPtr);

        // After we done with writing all metadatas and properties, flush the buffed writes to disk
        status = co_await fileStreamSPtr->FlushAsync();
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"WriteAsync: Flush file stream.",
            Helper::CheckpointFile);

        // Step 4: Write the footer.
        co_await this->WriteFooterAsync(cancellationToken, *fileStreamSPtr, *propertiesHandleSPtr);

        // Explicitly call FlushAsync before close to avoid the CloseAsync call failed
        status = co_await fileStreamSPtr->FlushAsync();
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"WriteAsync: Explicitly flush file stream before close.",
            Helper::CheckpointFile);

        fileSize_ = static_cast<ULONG64>(fileStreamSPtr->GetPosition());
    }
    catch (ktl::Exception & e)
    {
        exceptionSPtr = SharedException::Create(e, GetThisAllocator());
    }

    // Note: CloseAsync may fail in two situation, 1. OOM 2. Flush-on-close.
    // We always explicitly FlushAsync before close, so we can use assert (OOM) instead here instead of throwing
    status = co_await fileStreamSPtr->CloseAsync();
    ASSERT_IFNOT(
        NT_SUCCESS(status), 
        "{0}: CreateAsync: FileStream CloseAsync failed. Status: {1}", 
        TraceId,
        status);

    if (exceptionSPtr != nullptr)
    {
        StateManagerEventSource::Events->CheckpointFileError(
            TracePartitionId,
            ReplicaId,
            L"WriteAsync failed",
            exceptionSPtr->Status());

        throw exceptionSPtr->get_Info();
    }

    StateManagerEventSource::Events->CheckpointFileWriteComplete(
        TracePartitionId,
        ReplicaId,
        static_cast<int>(serailizationMode));

    co_return;
}

// Reads the backup log file.
// Algorithm:
// 1. Open the file and the file stream.
// 2. Read and validate the Footer section.  The footer is always at the end of the stream, minus space for the checksum.
// 3. Read and populate the properties.
// 4. Close the file stream and the file.
ktl::Awaitable<void> CheckpointFile::ReadAsync(
    __in CancellationToken const& cancellationToken)
{
    KShared$ApiEntry();
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    KBlockFile::SPtr fileSPtr = nullptr;
    status = co_await KBlockFile::CreateSparseFileAsync(
        const_cast<KWString &>(filePath_),
        FALSE, // IsWriteThrough.
        KBlockFile::eOpenExisting,
        KBlockFile::eRandomAccess,
        fileSPtr,
        nullptr,
        GetThisAllocator(),
        GetThisAllocationTag());
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"ReadAsync: Create sparse file.",
        Helper::CheckpointFile);
    KFinally([&] {fileSPtr->Close(); });

    io::KFileStream::SPtr fileStreamSPtr = nullptr;
    status = io::KFileStream::Create(
        fileStreamSPtr,
        GetThisAllocator(),
        GetThisAllocationTag());
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"ReadAsync: Create file stream.",
        Helper::CheckpointFile);

    status = co_await fileStreamSPtr->OpenAsync(*fileSPtr);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"ReadAsync: Open file stream.",
        Helper::CheckpointFile);

    SharedException::CSPtr exceptionSPtr = nullptr;
    try
    {
        // #9165663: KTL SetSystemIoPriorityHintAsync is required.
        // fileSPtr->SetSystemIoPriorityHint(IO_PRIORITY_HINT::IoPriorityLow);

        // Step 2: Read and validate the Footer section.  The footer is always at the end of the stream, minus space for the checksum.
        co_await this->ReadFooterAsync(*fileStreamSPtr, cancellationToken);

        // Step 3: Read and populate the properties.
        co_await this->ReadPropertiesAsync(*fileStreamSPtr, cancellationToken);

        // Also read the blocks size array here, and pass the array ref to the enumerator when we need to 
        // read the metadata.
        // Note: We don't read the metadata here because for some calls it's unnecessary to read metadata into memory
        // if we just want to check the file is completed or not, like the one in SafeFileReplaceAsync. Because we always
        // write properties and footer after metadata, so it is safe to check only these two section if we want to verify the 
        // file is completed
        blockSize_ = co_await this->ReadBlocksAsync(*fileStreamSPtr, cancellationToken);
    }
    catch (ktl::Exception & e)
    {
        exceptionSPtr = SharedException::Create(e, GetThisAllocator());
    }

    // Note: CloseAsync may fail in two situation, 1. OOM 2. Flush-on-close.
    // We always explicitly FlushAsync before close, so we can use assert (OOM) instead here instead of throwing
    status = co_await fileStreamSPtr->CloseAsync();
    ASSERT_IFNOT(
        NT_SUCCESS(status), 
        "{0}: CreateAsync: FileStream CloseAsync failed. Status: {1}", 
        TraceId,
        status);

    if (exceptionSPtr != nullptr)
    {
        StateManagerEventSource::Events->CheckpointFileError(
            TracePartitionId,
            ReplicaId,
            L"ReadAsync failed",
            exceptionSPtr->Status());

        throw exceptionSPtr->get_Info();
    }

    co_return;
}

// Create CheckpointFileAsyncEnumerator and return 
CheckpointFileAsyncEnumerator::SPtr CheckpointFile::GetAsyncEnumerator() const
{
    CheckpointFileAsyncEnumerator::SPtr enumerator = nullptr;
    NTSTATUS status = CheckpointFileAsyncEnumerator::Create(
        *PartitionedReplicaIdentifier,
        filePath_,
        *blockSize_,
        *propertiesSPtr_,
        GetThisAllocator(),
        enumerator);
    THROW_ON_FAILURE(status);

    return enumerator;
}

ULONG32 CheckpointFile::WriteMetadata(
    __in Utilities::PartitionedReplicaId const & traceId,
    __in BinaryWriter & writer, 
    __in SerializableMetadata const & metadata,
    __in SerializationMode::Enum serailizationMode)
{
    ULONG startPosition = writer.get_Position();

    // for the record size
    writer.Position += static_cast<ULONG>(sizeof(ULONG32));

    KUri const & name = *(metadata.Name);
    KString const & type = *(metadata.TypeString);

    writer.Write(name);
    writer.Write(type);
    writer.Write(metadata.CreateLSN);
    writer.Write(metadata.DeleteLSN);

    if (serailizationMode == SerializationMode::Managed)
    {
        WriteManagedInitializationParameter(traceId, writer, metadata.InitializationParameters.RawPtr());
    }
    else
    {
        WriteNativeInitializationParameter(traceId, writer, metadata.InitializationParameters.RawPtr());
    }

    writer.Write(static_cast<LONG32>(metadata.MetadataMode));
    writer.Write(metadata.StateProviderId);
    writer.Write(metadata.ParentStateProviderId);

    // Get the size of the record.
    ULONG endPosition = writer.Position;
    LONG32 recordSize = static_cast<LONG32>(endPosition - startPosition);

    // Assert if not larger then 44 bytes: Reserved Size(4), CreateLSN(8), DeleteLSN(8), InitializationParameter Size(4),
    // MetadataMode(4), SPID(8), PID(8), and Since Name can not be empty, size must large then 44.
    ASSERT_IFNOT(
        recordSize > 44,
        "{0}: recordSize indicates size of the buffer, it must not be less then 44. recordSize: {1}.",
        traceId,
        recordSize);

    // Write the record size at the start of the block.
    writer.Position = startPosition;
    writer.Write(recordSize);

    // Restore the end position.
    writer.Position = endPosition;

    return recordSize;
}

void CheckpointFile::WriteManagedInitializationParameter(
    __in Utilities::PartitionedReplicaId const & traceId,
    __in Utilities::BinaryWriter & writer,
    __in OperationData const * const initializationParameter)
{
    if (initializationParameter == nullptr)
    {
        writer.Write(NULL_OPERATIONDATA);
        return;
    }

    // #10376233: Document initialization parameter change for native, and the cases for different upgrade phase.
    ASSERT_IFNOT(
        initializationParameter->BufferCount < 2,
        "{0}: Managed should only have 0 or 1 buffer for initialization parameters and 0 indicates the byte array is empty. BufferSize: {1}.",
        traceId,
        initializationParameter->BufferCount);

    LONG32 bufferSize = initializationParameter->BufferCount == 0 ? 0 : (*initializationParameter)[0]->QuerySize();
    writer.Write(bufferSize);

    if (bufferSize != 0)
    {
        // This writer will just write the buffer instead of writing size and buffer.
        writer.Write((*initializationParameter)[0].RawPtr(), bufferSize);
    }
}

void CheckpointFile::WriteNativeInitializationParameter(
    __in Utilities::PartitionedReplicaId const & traceId,
    __in Utilities::BinaryWriter & writer,
    __in OperationData const * const initializationParameter)
{
    // To write the OperationData, first write a LONG32 length. If OperationData is null
    // write NULL_OPERATIONDATA(-1). Otherwise, write the length of OperationData
    // buffers count. Then write each buffer. 
    if (initializationParameter == nullptr)
    {
        writer.Write(NULL_OPERATIONDATA);
    }
    else
    {
        // In the managed code, if the initialization parameter is nullptr, write NULL_OPERATIONDATA(-1), otherwise, write a length("int")
        // which indicates the length of the buffer. In the native code, if the initialization parameter is nullptr, 
        // write NULL_OPERATIONDATA(-1), which is the same as the managed code. If the initialization parameter is not nullptr, write a 
        // length(taking a NOT operation of the length). We do this to distinguish the native and managed format.
        // Note: We only support Buffer size less then MAXLONG32. Otherwise it will overflow.
        ASSERT_IFNOT(
            initializationParameter->get_BufferCount() <= MAXLONG32,
            "{0}: BufferCout for initializationParameters can not be larger then MAXLONG32. BufferCount: {1}.",
            traceId,
            initializationParameter->get_BufferCount());
        LONG32 bufferCount = static_cast<LONG32>(initializationParameter->get_BufferCount());
        if (bufferCount == 0)
        {
            writer.Write(0);
        }
        else
        {
            writer.Write(~bufferCount);
        }

        for (LONG32 i = 0; i < bufferCount; i++)
        {
            writer.Write((*initializationParameter)[i].RawPtr());
        }
    }
}

// Write the metadata to the checkpoint file 
// Algorithm:
// 1. Write metadata block by block
// 2. Write the block section, which records all block's size
Awaitable<void> CheckpointFile::WriteMetadataAsync(
    __in SerializationMode::Enum serailizationMode,
    __in bool allowPrepareCheckpointLSNToBeInvalid,
    __in FABRIC_SEQUENCE_NUMBER prepareCheckpointLSN,
    __in CancellationToken const & cancellationToken,
    __inout ktl::io::KStream & fileStream)
{
    KShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    KSharedArray<ULONG32>::SPtr recordBlockSizes = _new(GetThisAllocationTag(), GetThisAllocator()) KSharedArray<ULONG32>();
    ULONG32 recordBlockSize = 0;

    BinaryWriter itemWriter(GetThisAllocator());

    for (SerializableMetadata::CSPtr serializableMetadata : *stateProviderMetadata_)
    {
        ULONG32 recordPosition = static_cast<ULONG32>(itemWriter.Position);
        ULONG32 recordSize = WriteMetadata(*(this->PartitionedReplicaIdentifier), itemWriter, *serializableMetadata, serailizationMode);

        // Checksum this metadata record (with the record size included).
        ULONG64 recordChecksum = CRC64::ToCRC64(*itemWriter.GetBuffer(0), recordPosition, recordSize);

        // Add the checksum at the end of the memory stream.
        itemWriter.Write(recordChecksum);

        recordBlockSize = static_cast<ULONG32>(itemWriter.Position);

        if (recordBlockSize >= DesiredBlockSize)
        {
            // Write the metadata records to the output stream.
            status = co_await fileStream.WriteAsync(*itemWriter.GetBuffer(0), 0, recordBlockSize);
            Helper::ThrowIfNecessary(
                status,
                TracePartitionId,
                ReplicaId,
                L"WriteMetadataAsync: Write metadata records to the stream.",
                Helper::CheckpointFile);

            status = recordBlockSizes->Append(recordBlockSize);
            Helper::ThrowIfNecessary(
                status,
                TracePartitionId,
                ReplicaId,
                L"WriteMetadataAsync: Append size to BlockSizeArray.",
                Helper::CheckpointFile);
            itemWriter.Position = 0;
        }

        // Update properties.
        stateProviderCount_++;

        if (serializableMetadata->ParentStateProviderId == Constants::EmptyStateProviderId)
        {
            rootStateProviderCount_++;
        }
    }

    // Write any remaining bytes to Buffer, and add the last record block size (if any).
    recordBlockSize = static_cast<ULONG32>(itemWriter.Position);
    if (recordBlockSize > 0)
    {
        status = co_await fileStream.WriteAsync(*itemWriter.GetBuffer(0), 0, recordBlockSize);
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"WriteMetadataAsync: Write the remaining items to the stream.",
            Helper::CheckpointFile);

        status = recordBlockSizes->Append(recordBlockSize);
        Helper::ThrowIfNecessary(
            status,
            TracePartitionId,
            ReplicaId,
            L"WriteMetadataAsync: Append size to BlockSizeArray for the remaining items.",
            Helper::CheckpointFile);
    }

    // Return the record block sizes.
    CheckpointFileBlocks::SPtr blocks = nullptr;
    status = CheckpointFileBlocks::Create(*recordBlockSizes, GetThisAllocator(), blocks);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"WriteMetadataAsync: Create CheckpointFileBlocks.",
        Helper::CheckpointFile);

    BlockHandle::SPtr metadataBlockHandle = nullptr;
    status = BlockHandle::Create(0, fileStream.GetPosition(), GetThisAllocator(), metadataBlockHandle);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"WriteMetadataAsync: Create BlockHandle.",
        Helper::CheckpointFile);

    cancellationToken.ThrowIfCancellationRequested();

    // Write the blocks and update the properties.
    BlockHandle::SPtr propertiesBlockHandle = nullptr;

    FileBlock<CheckpointFileBlocks::SPtr>::SerializerFunc blockSerializerFunction(blocks.RawPtr(), &CheckpointFileBlocks::Write);
    status = co_await FileBlock<BlockHandle::SPtr>::WriteBlockAsync(fileStream, blockSerializerFunction, GetThisAllocator(), cancellationToken, propertiesBlockHandle);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"WriteMetadataAsync: Write the blocks.",
        Helper::CheckpointFile);

    bool shouldNotWritePrepareCheckpointLSN = prepareCheckpointLSN == FABRIC_INVALID_SEQUENCE_NUMBER ? true : false;

    // #10348967: Update properties information.
    status = CheckpointFileProperties::Create(
        propertiesBlockHandle.RawPtr(),
        metadataBlockHandle.RawPtr(),
        PartitionId,
        ReplicaId,
        rootStateProviderCount_,
        stateProviderCount_,
        shouldNotWritePrepareCheckpointLSN,
        prepareCheckpointLSN,
        GetThisAllocator(),
        propertiesSPtr_);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"WriteMetadataAsync: Create the CheckpointFileProperties.",
        Helper::CheckpointFile);

    co_return;
}

// Write the properties to the checkpoint file 
ktl::Awaitable<BlockHandle::SPtr> CheckpointFile::WritePropertiesAsync(
    __in CancellationToken const & cancellationToken,
    __inout io::KFileStream & fileStream)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    KCoShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    BlockHandle::SPtr propertiesHandleSPtr = nullptr;

    FileBlock<CheckpointFileProperties::SPtr>::SerializerFunc propertiesSerializerFunction(propertiesSPtr_.RawPtr(), &CheckpointFileProperties::Write);
    status = co_await FileBlock<BlockHandle::SPtr>::WriteBlockAsync(fileStream, propertiesSerializerFunction, GetThisAllocator(), cancellationToken, propertiesHandleSPtr);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"WritePropertiesAsync: Write the properties.",
        Helper::CheckpointFile);

    co_return propertiesHandleSPtr;
}

// Write the footer to the checkpoint file 
ktl::Awaitable<void> CheckpointFile::WriteFooterAsync(
    __in CancellationToken const & cancellationToken,
    __in ktl::io::KFileStream & fileStream,
    __in Data::Utilities::BlockHandle & propertiesBlockHandle)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    KCoShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    status = FileFooter::Create(propertiesBlockHandle, Version, GetThisAllocator(), footerSPtr_);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"WriteFooterAsync: Create the footer.",
        Helper::CheckpointFile);

    BlockHandle::SPtr returnBlockHandle;
    FileBlock<FileFooter::SPtr>::SerializerFunc footerSerializerFunction(footerSPtr_.RawPtr(), &FileFooter::Write);
    status = co_await FileBlock<BlockHandle::SPtr>::WriteBlockAsync(fileStream, footerSerializerFunction, GetThisAllocator(), cancellationToken, returnBlockHandle);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"WriteFooterAsync: Write the footer.",
        Helper::CheckpointFile);

    co_return;
}

// Read properties section
ktl::Awaitable<void> CheckpointFile::ReadPropertiesAsync(
    __in io::KFileStream & fileStream,
    __in CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    KCoShared$ApiEntry();

    io::KFileStream::SPtr fileStreamSPtr(&fileStream);

    BlockHandle::SPtr propertiesHandle = footerSPtr_->PropertiesHandle;
    FileBlock<CheckpointFileProperties::SPtr>::DeserializerFunc propertiesDeserializerFunction(&CheckpointFileProperties::Read);

    propertiesSPtr_ = co_await FileBlock<CheckpointFileProperties::SPtr>::ReadBlockAsync(
        *fileStreamSPtr,
        *propertiesHandle,
        propertiesDeserializerFunction,
        GetThisAllocator(),
        cancellationToken);

    co_return;
}

// Read footer section
ktl::Awaitable<void> CheckpointFile::ReadFooterAsync(
    __in io::KFileStream & fileStream,
    __in CancellationToken const & cancellationToken)
{
    KCoShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    io::KFileStream::SPtr fileStreamSPtr(&fileStream);

    BlockHandle::SPtr footerHandle = nullptr;
    LONG64 tmpSize = fileStreamSPtr->GetLength();
    fileSize_ = tmpSize;

    ASSERT_IFNOT(tmpSize >= 0, "{0}: ReadFooterAsync: Get length from file stream, it must be position number", TraceId);

    ULONG64 fileSize = static_cast<ULONG64>(tmpSize);
    ULONG32 footerSize = FileFooter::SerializedSize();
    if (fileSize < (footerSize + static_cast<ULONG64>(CheckSumSectionSize)))
    {
        StateManagerEventSource::Events->CheckpointFileError(
            TracePartitionId,
            ReplicaId,
            L"ReadFooterAsync: File is too small.",
            status);

        // File is too small to contain even a footer.
        throw ktl::Exception(STATUS_INTERNAL_DB_CORRUPTION);
    }

    cancellationToken.ThrowIfCancellationRequested();

    // Create a block handle for the footer.
    // Note that size required is footer size + the checksum (sizeof(ULONG64))
    ULONG64 footerStartingOffset = fileSize - footerSize - CheckSumSectionSize;
    status = BlockHandle::Create(
        footerStartingOffset,
        footerSize,
        GetThisAllocator(),
        footerHandle);
    Helper::ThrowIfNecessary(
        status,
        TracePartitionId,
        ReplicaId,
        L"ReadFooterAsync: Create the footer BlockHandle.",
        Helper::CheckpointFile);

    FileBlock<FileFooter::SPtr>::DeserializerFunc deserializerFunction(&FileFooter::Read);
    footerSPtr_ = co_await FileBlock<FileFooter::SPtr>::ReadBlockAsync(
        *fileStreamSPtr,
        *footerHandle,
        deserializerFunction,
        GetThisAllocator(),
        cancellationToken);
    cancellationToken.ThrowIfCancellationRequested();

    // Verify we know how to deserialize this version of the state manager checkpoint file.
    if (footerSPtr_->Version != Version)
    {
        StateManagerEventSource::Events->CheckpointFileError(
            TracePartitionId,
            ReplicaId,
            L"ReadFooterAsync: Version is incorrect.",
            status);

        throw ktl::Exception(STATUS_INTERNAL_DB_CORRUPTION);
    }

    co_return;
}

// Read the block size array
Awaitable<KSharedArray<ULONG32>::SPtr> CheckpointFile::ReadBlocksAsync(
    __in io::KStream & stream,
    __in CancellationToken const & cancellationToken)
{
    KShared$ApiEntry();
    io::KStream::SPtr streamSPtr(&stream);

    // Read the blocks, to improve sequential reads.
    CheckpointFileBlocks::SPtr blocks;
    FileBlock<CheckpointFileBlocks::SPtr>::DeserializerFunc deserializerFunc(&CheckpointFileBlocks::Read);
    blocks = co_await FileBlock<CheckpointFileBlocks::SPtr>::ReadBlockAsync(stream, *propertiesSPtr_->BlocksHandle, deserializerFunc, GetThisAllocator(), cancellationToken);

    if (blocks == nullptr)
    {
        throw Exception(STATUS_INTERNAL_DB_CORRUPTION);
    }

    co_return blocks->RecordBlockSizes;
}

LONG64 CheckpointFile::get_FileSize() const { return fileSize_; }

CheckpointFile::CheckpointFile(
    __in Utilities::PartitionedReplicaId const & traceId,
    __in KWString const & filePath) noexcept
    : KObject()
    , KShared()
    , PartitionedReplicaTraceComponent(traceId)
    , stateProviderMetadata_(nullptr)
    , footerSPtr_(nullptr)
    , filePath_(filePath)
    , fileSize_(0)
    , rootStateProviderCount_(0)
    , stateProviderCount_(0)
    , blockSize_(nullptr)
    , propertiesSPtr_(nullptr)
{
}

CheckpointFile::~CheckpointFile()
{
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::LogRecordLib;
using namespace Data::LoggingReplicator;
using namespace Data::Utilities;
using namespace ktl;

Common::StringLiteral const TraceComponent("BackupLogFile");

BackupLogFile::SPtr BackupLogFile::Create(
    __in PartitionedReplicaId const& traceId,
    __in KWString const & filePath,
    __in KAllocator & allocator)
{
    BackupLogFile * pointer = _new(BACKUP_LOG_FILE_TAG, allocator) BackupLogFile(
        traceId,
        filePath);
    THROW_ON_ALLOCATION_FAILURE(pointer);
    return SPtr(pointer);
}

KWString const BackupLogFile::get_FileName() const { return filePath_; }

LONG64 BackupLogFile::get_Size() const { return size_; }

ULONG32 BackupLogFile::get_SizeInKB() const { return static_cast<ULONG32>(ceil(size_ / NumberOfBytesInKB)); }

ULONG32 BackupLogFile::get_Count() const { return propertiesSPtr_->Count; }

TxnReplicator::Epoch BackupLogFile::get_IndexingRecordEpoch() const { return propertiesSPtr_->IndexingRecordEpoch; }

FABRIC_SEQUENCE_NUMBER BackupLogFile::get_IndexingRecordLSN() const { return propertiesSPtr_->IndexingRecordLSN; }

TxnReplicator::Epoch BackupLogFile::get_LastBackedUpEpoch() const { return propertiesSPtr_->LastBackedUpEpoch; }

FABRIC_SEQUENCE_NUMBER BackupLogFile::get_LastBackedUpLSN() const { return propertiesSPtr_->LastBackedUpLSN; }

// This function writes a backup log file.
// Algorithm
// 1. Create file and file stream.
// 2. Write the log records.
// 3. Write properties.
// 4. Write footer.
ktl::Awaitable<void> BackupLogFile::WriteAsync(
    __in IAsyncEnumerator<LogRecord::SPtr>& logRecords,
    __in BackupLogRecord const & backupLogRecord,
    __in CancellationToken const& cancellationToken)
{
    KShared$ApiEntry();
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    Common::Stopwatch stopwatch;
    stopwatch.Start();

    // Step 1: Create the file and the file stream.
    KBlockFile::SPtr fileSPtr = nullptr;
    status = co_await KBlockFile::CreateSparseFileAsync(
        filePath_,
        FALSE,                      // Is Write Through
        KBlockFile::eCreateAlways,  // Create Disposition
        static_cast<KBlockFile::CreateOptions>(KBlockFile::eSequentialAccess + KBlockFile::eInheritFileSecurity),
        fileSPtr,
        nullptr,
        GetThisAllocator(),
        GetThisAllocationTag());

    THROW_ON_FAILURE(status);
    KFinally([&] {fileSPtr->Close(); });

    ktl::io::KFileStream::SPtr fileStreamSPtr = nullptr;
    status = ktl::io::KFileStream::Create(fileStreamSPtr, GetThisAllocator(), GetThisAllocationTag());
    THROW_ON_FAILURE(status);

    status = co_await fileStreamSPtr->OpenAsync(*fileSPtr);
    THROW_ON_FAILURE(status);

    SharedException::CSPtr exceptionSPtr = nullptr;
    try
    {
        // TODO: #9165663: KTL SetSystemIoPriorityHintAsync is requried.
        // fileSPtr->SetSystemIoPriorityHint(IO_PRIORITY_HINT::IoPriorityLow);

        // Step 2: Write the log records.
        IncrementalBackupLogRecordsAsyncEnumerator * incrementalLogRecordsPtr = dynamic_cast<IncrementalBackupLogRecordsAsyncEnumerator *>(
            &logRecords);

        if (incrementalLogRecordsPtr == nullptr)
        {
            co_await this->WriteLogRecordsAsync(logRecords, cancellationToken, *fileStreamSPtr);
        }
        else
        {
            co_await this->WriteLogRecordsAsync(*incrementalLogRecordsPtr, cancellationToken, *fileStreamSPtr);
            co_await incrementalLogRecordsPtr->VerifyDrainedAsync();

            ASSERT_IFNOT(
                Count == incrementalLogRecordsPtr->Count(),
                "{0}: Expected count: {1} Backed up count: {2}",
                TraceId,
                Count,
                incrementalLogRecordsPtr->Count());

            propertiesSPtr_->IndexingRecordEpoch = incrementalLogRecordsPtr->StartingEpoch;
            propertiesSPtr_->IndexingRecordLSN = incrementalLogRecordsPtr->StartingLSN;

            if (incrementalLogRecordsPtr->HighestBackedUpEpoch.IsInvalid())
            {
                propertiesSPtr_->LastBackedUpEpoch = backupLogRecord.HighestBackedupEpoch;
            }
            else
            {
                propertiesSPtr_->LastBackedUpEpoch = incrementalLogRecordsPtr->HighestBackedUpEpoch;
            }

            propertiesSPtr_->LastBackedUpLSN = incrementalLogRecordsPtr->HighestBackedUpLSN;
        }

        // Step 3: Write properties.
        BlockHandle::SPtr propertiesHandleSPtr = co_await this->WritePropertiesAsync(*fileStreamSPtr, cancellationToken);

        // We must flush after writing the data records and properties.
        // This is because we do not have a log to depend on during recovery which can
        // guarantee that if footer exists the rest must exist.
        status = co_await fileStreamSPtr->FlushAsync();
        THROW_ON_FAILURE(status);

        // Step 4: Write the footer.
        co_await this->WriteFooterAsync(*fileStreamSPtr, *propertiesHandleSPtr, cancellationToken);

        // After writing the footer, flush to ensure all buffers are persisted.
        co_await fileStreamSPtr->FlushAsync();

        // Store the size in bytes.
        size_ = fileStreamSPtr->GetPosition();
    }
    catch (ktl::Exception & e)
    {
        LR_TRACE_UNEXPECTEDEXCEPTION(
            L"BackupLogFile::WriteAsync failed",
            e.GetStatus());
    }

    // Note: CloseAsync may fail in two situation, 1. OOM 2. Flush-on-close.
    // We always explicitly FlushAsync before close, so we can use assert (OOM) instead here instead of throwing
    status = co_await fileStreamSPtr->CloseAsync();
    ASSERT_IFNOT(NT_SUCCESS(status), "{0}: CreateAsync: FileStream CloseAsync failed.", TraceId);

    if (exceptionSPtr != nullptr)
    {
        throw exceptionSPtr->get_Info();
    }

    stopwatch.Stop();
    writeTimeInMilliseconds_ = stopwatch.ElapsedMilliseconds;

    co_return;
}

// Reads the backup log file.
// Algorithm:
// 1. Open the file and the file stream.
// 2. Read the footer
// 3. Use the handle in footer to read properties.
// 4. Close the file stream and the file.
ktl::Awaitable<void> BackupLogFile::ReadAsync(
    __in CancellationToken const& cancellationToken)
{
    KShared$ApiEntry();
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    KBlockFile::SPtr fileSPtr = nullptr;
    status = co_await KBlockFile::CreateSparseFileAsync(
        filePath_,
        FALSE,                      // Is Write Through.
        KBlockFile::eOpenExisting,  // Create Disposition
        KBlockFile::eRandomAccess,
        fileSPtr,
        nullptr,
        GetThisAllocator(),
        GetThisAllocationTag());

    THROW_ON_FAILURE(status);
    KFinally([&] {fileSPtr->Close(); });

    ktl::io::KFileStream::SPtr fileStreamSPtr = nullptr;
    status = ktl::io::KFileStream::Create(
        fileStreamSPtr,
        GetThisAllocator(),
        GetThisAllocationTag());
    THROW_ON_FAILURE(status);

    status = co_await fileStreamSPtr->OpenAsync(*fileSPtr);
    THROW_ON_FAILURE(status);

    SharedException::CSPtr exceptionSPtr = nullptr;
    try
    {
        // TODO: #9165663: KTL SetSystemIoPriorityHintAsync is requried.
        // fileSPtr->SetSystemIoPriorityHint(IO_PRIORITY_HINT::IoPriorityLow);

        // Step 2: Read and validate the Footer section.  The footer is always at the end of the stream, minus space for the checksum.
        co_await ReadFooterAsync(*fileStreamSPtr, cancellationToken);

        // Step 3: Read and populate the properties.
        co_await ReadPropertiesAsync(*fileStreamSPtr, cancellationToken);
    }
    catch (ktl::Exception & e)
    {
        LR_TRACE_UNEXPECTEDEXCEPTION_STATUS(
            L"BackupLogFile::ReadAsync failed",
            e.GetStatus());

        exceptionSPtr = SharedException::Create(e, GetThisAllocator());
    }

    // Note: CloseAsync may fail in two situation, 1. OOM 2. Flush-on-close.
    // We always explicitly FlushAsync before close, so we can use assert (OOM) instead here instead of throwing
    status = co_await fileStreamSPtr->CloseAsync();
    ASSERT_IFNOT(NT_SUCCESS(status), "{0}: CreateAsync: FileStream CloseAsync failed.", TraceId);

    if (exceptionSPtr != nullptr)
    {
        throw exceptionSPtr->get_Info();
    }

    co_return;
}

BackupLogFileAsyncEnumerator::SPtr BackupLogFile::GetAsyncEnumerator() const
{
    return BackupLogFileAsyncEnumerator::Create(
        filePath_,
        propertiesSPtr_->Count,
        *propertiesSPtr_->RecordsHandle,
        GetThisAllocator());
}

ktl::Awaitable<void> BackupLogFile::VerifyAsync()
{
    KShared$ApiEntry();

    BackupLogFileAsyncEnumerator::SPtr enumerator = this->GetAsyncEnumerator();

    SharedException::CSPtr exceptionSPtr = nullptr;
    try
    {
        while (co_await enumerator->MoveNextAsync(CancellationToken::None) == true)
        {
            // Enumerator the log to check data integrity.
        }
    }
    catch (ktl::Exception & e)
    {
        exceptionSPtr = SharedException::Create(e, GetThisAllocator());
    }

    co_await enumerator->CloseAsync();

    if (exceptionSPtr != nullptr)
    {
        throw exceptionSPtr->get_Info();
    }

    co_return;
}

ktl::Awaitable<void> BackupLogFile::WriteLogRecordsAsync(
    __in IAsyncEnumerator<LogRecord::SPtr> & logRecords,
    __in CancellationToken const& cancellationToken,
    __inout ktl::io::KStream & outputStream)
{
    KShared$ApiEntry();

    bool firstIndexingRecordProcessed = false;

    // MCoskun: Difference with MANAGED
    // #9165772: Native avoid an unnecessary copy in to a secondary binary writer by buffering in a KArray.
    // Copy 1: Code first uses recordWriter and generate IOperationData
    // Copy 2: Writes in to filestream.
    // Copy 3: Filestream in to KBlockFile
    // TODO: #9165801 : Eliminate copy 2 by aligning the allocations.
    BinaryWriter recordWriter(GetThisAllocator());
    KArray<OperationData::CSPtr> bufferedOperationData(GetThisAllocator());

    // Reserve an 'LONG32' at the start to store the size of the block
    ULONG32 bufferedSize = BlockSizeSectionSize;

    while (co_await logRecords.MoveNextAsync(cancellationToken) == true)
    {
        LogRecord::SPtr logRecord = logRecords.GetCurrent();

        if (logRecord->RecordType == LogRecordType::Indexing)
        {
            IndexingLogRecord& indexingLogRecord = dynamic_cast<IndexingLogRecord &>(*logRecord);

            if (firstIndexingRecordProcessed == false)
            {
                propertiesSPtr_->IndexingRecordEpoch = indexingLogRecord.CurrentEpoch;
                propertiesSPtr_->IndexingRecordLSN = indexingLogRecord.Lsn;

                firstIndexingRecordProcessed = true;
            }
            
            propertiesSPtr_->LastBackedUpEpoch = indexingLogRecord.CurrentEpoch;
        }

        if (logRecord->RecordType == LogRecordType::UpdateEpoch)
        {
            UpdateEpochLogRecord& updateEpochLogRecord = dynamic_cast<UpdateEpochLogRecord &>(*logRecord);
            propertiesSPtr_->LastBackedUpEpoch = updateEpochLogRecord.EpochValue;
        }

        propertiesSPtr_->LastBackedUpLSN = logRecord->Lsn;

        OperationData::CSPtr operationDataSPtr = LogRecord::WriteRecord(
            *logRecord, 
            recordWriter,
            GetThisAllocator(), 
            false, 
            false);

        // Assert: We rely on LoggingReplicator enforcing a single log record size to be limited to 4 GB.
        LONG64 operationDataSize = OperationData::GetOperationSize(*operationDataSPtr);
        ASSERT_IFNOT(operationDataSize <= MAXULONG32, "{0}: WriteLogRecordsAsync: Log Record size {1} is greater than max log record size {2}.", TraceId, operationDataSize, MAXULONG32);

        // MCoskun: Block size is ULONG32, so it can at max hold 4 GB.
        // Max log record supported by LoggingReplicator is 4 GB so the block size is sufficient.
        // Problem is that if the buffered block contains >0 bytes before we try to add a large log record,
        // then the size of the block may surpass 4 GB.
        // To avoid this, we need flush the existing buffer first.
        LONG64 nextSize = operationDataSize + bufferedSize;
        if (nextSize > MAXULONG32)
        {
            ASSERT_IFNOT(bufferedSize > 0, "{0}: WriteLogRecordsAsync: Buffered size {1} must already be greater than 0.", TraceId, bufferedSize);

            co_await this->WriteLogRecordBlockAsync(cancellationToken, bufferedSize, recordWriter, bufferedOperationData, outputStream);
            bufferedOperationData.Clear();
            bufferedSize = BlockSizeSectionSize;
        }

        NTSTATUS status = bufferedOperationData.Append(operationDataSPtr);
        THROW_ON_FAILURE(status);

        bufferedSize += static_cast<ULONG32>(operationDataSize);

        propertiesSPtr_->Count++;

        // Flush the block after it's large enough.
        if (bufferedSize >= MinimumIntermediateFlushSize)
        {
            co_await this->WriteLogRecordBlockAsync(cancellationToken, bufferedSize, recordWriter, bufferedOperationData, outputStream);
            bufferedOperationData.Clear();
            bufferedSize = BlockSizeSectionSize;
        }

        cancellationToken.ThrowIfCancellationRequested();
    }

    // Flush the block, if there's any remaining data (ignoring the block size 'int' at the start).
    if (bufferedSize > BlockSizeSectionSize)
    {
        co_await this->WriteLogRecordBlockAsync(cancellationToken, bufferedSize, recordWriter, bufferedOperationData, outputStream);
        bufferedOperationData.Clear();
        bufferedSize = BlockSizeSectionSize;
    }

    // If no logical record is being backed up, lastBackedupLsn can be Invalid.
    if (LastBackedUpLSN == FABRIC_INVALID_SEQUENCE_NUMBER)
    {
        propertiesSPtr_->LastBackedUpLSN = IndexingRecordLSN;
    }

    ASSERT_IFNOT(firstIndexingRecordProcessed == true, "{0}: WriteLogRecordsAsync: Indexing log record must have been processed.", TraceId);

    ASSERT_IFNOT(IndexingRecordEpoch > TxnReplicator::Epoch::InvalidEpoch(), "{0}: WriteLogRecordsAsync: Indexing record epoch has not been set.", TraceId);
    ASSERT_IFNOT(IndexingRecordLSN > FABRIC_INVALID_SEQUENCE_NUMBER, "{0}: WriteLogRecordsAsync: Indexing record LSN has not been set.", TraceId);

    ASSERT_IFNOT(LastBackedUpEpoch > TxnReplicator::Epoch::InvalidEpoch(), "{0}: WriteLogRecordsAsync: Ending epoch has not been set.", TraceId);
    ASSERT_IFNOT(LastBackedUpLSN > FABRIC_INVALID_SEQUENCE_NUMBER, "{0}: WriteLogRecordsAsync: Ending LSN has not been set.", TraceId);

    // Note that we depend on flushing after properties are written.
    co_return;
}

ktl::Awaitable<void> BackupLogFile::WriteLogRecordsAsync(
    __in IncrementalBackupLogRecordsAsyncEnumerator & logRecords,
    __in CancellationToken const& cancellationToken,
    __inout ktl::io::KStream & outputStream)
{
    KShared$ApiEntry();

    BinaryWriter recordWriter(GetThisAllocator());
    KArray<OperationData::CSPtr> bufferedOperationData(GetThisAllocator());

    // Reserve an 'LONG32' at the start to store the size of the block
    ULONG32 bufferedSize = BlockSizeSectionSize;

    while (co_await logRecords.MoveNextAsync(cancellationToken) == true)
    {
        LogRecord::SPtr logRecord = logRecords.GetCurrent();

        OperationData::CSPtr operationDataSPtr = LogRecord::WriteRecord(
            *logRecord,
            recordWriter,
            GetThisAllocator(),
            false,
            false);

        // Assert: We rely on LoggingReplicator enforcing a single log record size to be limited to 4 GB.
        LONG64 operationDataSize = OperationData::GetOperationSize(*operationDataSPtr);
        ASSERT_IFNOT(operationDataSize <= MAXULONG32, "{0}: WriteLogRecordsAsync: Log Record size {1} is greater than max log record size {2}.", TraceId, operationDataSize, MAXULONG32);

        // MCoskun: Block size is ULONG32, so it can at max hold 4 GB.
        // Max log record supported by LoggingReplicator is 4 GB so the block size is sufficient.
        // Problem is that if the buffered block contains >0 bytes before we try to add a large log record,
        // then the size of the block may surpass 4 GB.
        // To avoid this, we need flush the existing buffer first.
        LONG64 nextSize = operationDataSize + bufferedSize;
        if (nextSize > MAXULONG32)
        {
            ASSERT_IFNOT(bufferedSize > 0, "{0}: WriteLogRecordsAsync: Buffered size {1} must already be greater than 0.", TraceId, bufferedSize);

            co_await this->WriteLogRecordBlockAsync(cancellationToken, bufferedSize, recordWriter, bufferedOperationData, outputStream);
            bufferedOperationData.Clear();
            bufferedSize = BlockSizeSectionSize;
        }

        NTSTATUS status = bufferedOperationData.Append(operationDataSPtr);
        THROW_ON_FAILURE(status);

        bufferedSize += static_cast<ULONG32>(operationDataSize);

        propertiesSPtr_->Count++;

        // Flush the block after it's large enough.
        if (bufferedSize >= MinimumIntermediateFlushSize)
        {
            co_await this->WriteLogRecordBlockAsync(cancellationToken, bufferedSize, recordWriter, bufferedOperationData, outputStream);
            bufferedOperationData.Clear();
            bufferedSize = BlockSizeSectionSize;
        }
    }

    // Flush the block, if there's any remaining data (ignoring the block size 'int' at the start).
    if (bufferedSize > BlockSizeSectionSize)
    {
        co_await this->WriteLogRecordBlockAsync(cancellationToken, bufferedSize, recordWriter, bufferedOperationData, outputStream);
        bufferedOperationData.Clear();
        bufferedSize = BlockSizeSectionSize;
    }

    // Note that we depend on flushing after properties are written.
    co_return;
}

ktl::Awaitable<void> BackupLogFile::WriteLogRecordBlockAsync(
    __in CancellationToken const& cancellationToken,
    __in ULONG32 blockSize,
    __inout BinaryWriter & binaryWriter,
    __inout KArray<OperationData::CSPtr> & operationDataArray,
    __inout ktl::io::KStream & outputStream)
{
    KCoShared$ApiEntry();

    // Serialize block size.
    binaryWriter.Position = 0;
    binaryWriter.Write(blockSize);
    KBuffer::SPtr blockSizeBuffer = binaryWriter.GetBuffer(0, sizeof(ULONG32));
    KArray<KBuffer::CSPtr> buffers(GetThisAllocator());
    NTSTATUS status = buffers.Append(blockSizeBuffer.RawPtr());
    THROW_ON_FAILURE(status);

    OperationData::CSPtr operationData = OperationData::Create(buffers, GetThisAllocator());
    status = operationDataArray.InsertAt(0, operationData);
    THROW_ON_FAILURE(status);

    for (OperationData::CSPtr opData : operationDataArray)
    {
        for (ULONG32 bufferIndex = 0; bufferIndex < opData->BufferCount; bufferIndex++)
        {
            KBuffer::CSPtr buffer((*opData)[bufferIndex]);
            co_await outputStream.WriteAsync(*buffer);
        }
    }

    // Checksum this block of data (with the block size included).
    ULONG64 blockChecksum = CRC64::ToCRC64(operationDataArray, 0, operationDataArray.Count());

    // Add the checksum at the end of the memory stream.
    binaryWriter.Position = 0;
    binaryWriter.Write(blockChecksum);

    // Write the records block to the output stream.
    KBuffer::SPtr checkSumBuffer = binaryWriter.GetBuffer(0, sizeof(ULONG64));
    co_await outputStream.WriteAsync(*checkSumBuffer);

    co_return;
}

ktl::Awaitable<void> BackupLogFile::WriteFooterAsync(
    __in ktl::io::KFileStream& fileStream, 
    __in BlockHandle& propertiesBlockHandle, 
    __in CancellationToken const& cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    KCoShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    ktl::io::KFileStream::SPtr fileStreamSPtr(&fileStream);

    FileFooter::SPtr fileFooterSPtr;
    status = FileFooter::Create(
        propertiesBlockHandle,
        Version,
        GetThisAllocator(),
        fileFooterSPtr);
    THROW_ON_FAILURE(status);

    BlockHandle::SPtr footerBlockHandleSPtr = nullptr;
    FileBlock<BackupLogFileProperties::SPtr>::SerializerFunc footerSerializationFunction(
        fileFooterSPtr.RawPtr(),
        &FileFooter::Write);
    status = co_await FileBlock<BackupLogFileProperties::SPtr>::WriteBlockAsync(
        *fileStreamSPtr,
        footerSerializationFunction,
        GetThisAllocator(),
        cancellationToken,
        footerBlockHandleSPtr);
    THROW_ON_FAILURE(status);

    co_return;
}

ktl::Awaitable<void> BackupLogFile::ReadFooterAsync(
    __in ktl::io::KFileStream & fileStream,
    __in CancellationToken const & cancellationToken)
{
    KCoShared$ApiEntry();

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    ktl::io::KFileStream::SPtr fileStreamSPtr(&fileStream);

    BlockHandle::SPtr footerHandle = nullptr;
    LONG64 tmpSize = fileStreamSPtr->GetLength();

    ASSERT_IFNOT(tmpSize >= 0, "{0}: The size read from fileStream must not be negative.", TraceId);

    ULONG64 fileSize = static_cast<ULONG64>(tmpSize);
    ULONG32 footerSize = FileFooter::SerializedSize();
    if (fileSize < (footerSize + static_cast<ULONG64>(CheckSumSectionSize)))
    {
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
    THROW_ON_FAILURE(status);

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
        throw ktl::Exception(STATUS_INTERNAL_DB_CORRUPTION);
    }

    co_return;
}

ktl::Awaitable<BlockHandle::SPtr> BackupLogFile::WritePropertiesAsync(
    __in ktl::io::KFileStream & fileStream,
    __in CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    KShared$ApiEntry();
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    BlockHandle::SPtr recordsHandleSPtr = nullptr;
    status = BlockHandle::Create(
        0,
        fileStream.GetPosition(),
        GetThisAllocator(),
        recordsHandleSPtr);
    THROW_ON_FAILURE(status);

    propertiesSPtr_->RecordsHandle = *recordsHandleSPtr;

    FileBlock<BackupLogFileProperties::SPtr>::SerializerFunc serializerFunction(
        propertiesSPtr_.RawPtr(),
        &BackupLogFileProperties::Write);

    BlockHandle::SPtr propertiesHandleSPtr = nullptr;
    status = co_await FileBlock<BackupLogFileProperties::SPtr>::WriteBlockAsync(
        fileStream,
        serializerFunction,
        GetThisAllocator(),
        cancellationToken,
        propertiesHandleSPtr);
    THROW_ON_FAILURE(status);

    co_return Ktl::Move(propertiesHandleSPtr);
}

ktl::Awaitable<void> BackupLogFile::ReadPropertiesAsync(
    __in ktl::io::KFileStream & fileStream,
    __in CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    KCoShared$ApiEntry();

    ktl::io::KFileStream::SPtr fileStreamSPtr(&fileStream);

    BlockHandle::SPtr propertiesHandle = footerSPtr_->PropertiesHandle;
    FileBlock<BackupLogFileProperties::SPtr>::DeserializerFunc propertiesDeserializerFunction(
        &BackupLogFileProperties::Read);

    // TODO: #9166098: Take in cancellation token.
    propertiesSPtr_ = co_await FileBlock<BackupLogFileProperties::SPtr>::ReadBlockAsync(
        *fileStreamSPtr,
        *propertiesHandle,
        propertiesDeserializerFunction,
        GetThisAllocator(),
        cancellationToken);

    co_return;
}

BackupLogFile::BackupLogFile(
    __in PartitionedReplicaId const& traceId,
    __in KWString const & filePath) noexcept
    : KObject()
    , KShared()
    , PartitionedReplicaTraceComponent(traceId)
    , filePath_(filePath)
{
    NTSTATUS status =  BackupLogFileProperties::Create(GetThisAllocator(), propertiesSPtr_);
    SetConstructorStatus(status);
}

BackupLogFile::~BackupLogFile()
{
}

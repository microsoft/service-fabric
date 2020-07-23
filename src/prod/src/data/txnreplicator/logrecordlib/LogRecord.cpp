// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::Log;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

// 2 * record Length + lsn + psn + recordType + previousphysicalrecordoffset
const ULONG LogRecord::DiskSpaceUsed = (2 * sizeof(ULONG)) + (2 * sizeof(LONG64)) + sizeof(LONG32) + sizeof(ULONG64);

LogRecord::LogRecord(
    __in LogRecordType::Enum recordType,
    __in PhysicalLogRecord & invalidPhysicalLogRecord)
    : KObject()
    , KShared()
    , recordType_(recordType)
    , lsn_(Constants::InvalidLsn)
    , psn_(Constants::InvalidLsn)
    , recordLength_(Constants::InvalidRecordLength)
    , recordPosition_(Constants::InvalidRecordPosition)
    , previousPhysicalRecordOffset_(Constants::InvalidPhysicalRecordOffset)
    , previousPhysicalRecord_(&invalidPhysicalLogRecord)
    , appliedTask_(CompletionTask::Create(GetThisAllocator()))
    , flushedTask_(CompletionTask::Create(GetThisAllocator()))
    , processedTask_(CompletionTask::Create(GetThisAllocator()))
    , approximateDiskSize_(0)
{
    ASSERT_IFNOT(
        invalidPhysicalLogRecord.RecordType == LogRecordType::Enum::Invalid,
        "Invalid PhysicalLogRecord: {0}", invalidPhysicalLogRecord.RecordType);

    UpdateApproximateDiskSize();
}

LogRecord::LogRecord(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord)
    : KObject()
    , KShared()
    , recordType_(recordType)
    , lsn_(lsn)
    , psn_(Constants::InvalidLsn)
    , recordLength_(Constants::InvalidRecordLength)
    , recordPosition_(recordPosition)
    , previousPhysicalRecordOffset_(Constants::InvalidPhysicalRecordOffset)
    , previousPhysicalRecord_(&invalidPhysicalLogRecord)
    , appliedTask_(CompletionTask::Create(GetThisAllocator()))
    , flushedTask_(CompletionTask::Create(GetThisAllocator()))
    , processedTask_(CompletionTask::Create(GetThisAllocator()))
    , approximateDiskSize_(0)
{
    ASSERT_IFNOT(
        invalidPhysicalLogRecord.RecordType == LogRecordType::Enum::Invalid,
        "Invalid PhysicalLogRecord: {0}", invalidPhysicalLogRecord.RecordType);
}

LogRecord::LogRecord()
    : KObject()
    , KShared()
    , recordType_(LogRecordType::Invalid)
    , lsn_(Constants::InvalidLsn)
    , psn_(Constants::InvalidLsn)
    , recordLength_(Constants::InvalidRecordLength)
    , recordPosition_(Constants::InvalidRecordPosition)
    , previousPhysicalRecordOffset_(Constants::InvalidPhysicalRecordOffset)
    , previousPhysicalRecord_(nullptr)
    , appliedTask_(CompletionTask::Create(GetThisAllocator()))
    , flushedTask_(CompletionTask::Create(GetThisAllocator()))
    , processedTask_(CompletionTask::Create(GetThisAllocator()))
    , approximateDiskSize_(0)
{
    ASSERT_IFNOT(
        RecordType == LogRecordType::Enum::Invalid,
        "Invalid RecordType: {0}", RecordType);
}

LogRecord::~LogRecord()
{
}

bool LogRecord::IsInvalid(__in_opt LogRecord const * const record)
{
    return (record != nullptr) && (record->RecordType == LogRecordType::Enum::Invalid);
}

bool LogRecord::IsPhysical(__in LogRecord const & record)
{
    LogRecordType::Enum type = record.RecordType;

    return (
        type == LogRecordType::Enum::BeginCheckpoint ||
        type == LogRecordType::Enum::CompleteCheckpoint ||
        type == LogRecordType::Enum::EndCheckpoint ||
        type == LogRecordType::Enum::Indexing ||
        type == LogRecordType::Enum::Information ||
        type == LogRecordType::Enum::TruncateHead ||
        type == LogRecordType::Enum::TruncateTail
        );
}

bool LogRecord::IsTransactionLogRecord(__in LogRecord const & logRecord)
{
    LogRecordType::Enum recordType = logRecord.RecordType;

    return
        recordType == LogRecordType::Enum::BeginTransaction ||
        recordType == LogRecordType::Enum::Operation ||
        recordType == LogRecordType::Enum::EndTransaction;
}

bool LogRecord::IsOperationLogRecord(__in LogRecord const & logRecord)
{
    LogRecordType::Enum recordType = logRecord.RecordType;

    return 
        recordType == LogRecordType::Enum::Operation;
}

bool LogRecord::IsCommitLogRecord(__in LogRecord const & logRecord)
{
    if (logRecord.RecordType == LogRecordType::BeginTransaction)
    {
        BeginTransactionOperationLogRecord::CSPtr beginTransactionOperationLogRecord = dynamic_cast<BeginTransactionOperationLogRecord const *>(&logRecord);
        return beginTransactionOperationLogRecord->IsSingleOperationTransaction;
    }

    if (logRecord.RecordType == LogRecordType::EndTransaction)
    {
        EndTransactionLogRecord::CSPtr endTransactionRecord = dynamic_cast<EndTransactionLogRecord const *>(&logRecord);
        return endTransactionRecord->IsCommitted;
    }

    if (logRecord.RecordType == LogRecordType::Operation)
    {
        OperationLogRecord::CSPtr operationLogRecord = dynamic_cast<OperationLogRecord const *>(&logRecord);
        return operationLogRecord->BaseTransaction.IsAtomicOperation || operationLogRecord->IsRedoOnly;
    }

    return false;
}

std::wstring LogRecord::ToString() const
{
    std::wstring logRecordString = Constants::StartingJSON + Constants::Quote;

    logRecordString += L"PSN" + Constants::DivisionJSON +
        std::to_wstring(get_Psn()) + Constants::EndlJSON;
    logRecordString += L"LSN" + Constants::DivisionJSON + 
        std::to_wstring(get_Lsn()) + Constants::EndlJSON;
    logRecordString += L"Record Type" + Constants::DivisionJSON +
        get_RecordTypeName() + Constants::EndlJSON;
    logRecordString += L"Record Position" + Constants::DivisionJSON +
        std::to_wstring(get_RecordPosition()) + Constants::EndlJSON;
    logRecordString += L"Record Length" + Constants::DivisionJSON +
        std::to_wstring(get_RecordLength()) + Constants::Quote;

    return logRecordString;
}


void LogRecord::UpdateApproximateDiskSize()
{
    ApproximateSizeOnDisk = ApproximateSizeOnDisk + DiskSpaceUsed;
}

Awaitable<LogRecord::SPtr> LogRecord::ReadNextRecordAsync(
    __in io::KStream & stream,
    __in InvalidLogRecords & invalidLogRecords,
    __in KAllocator & allocator,
    __in bool isPhysicalRead,
    __in bool useInvalidRecordPosition,
    __in bool setRecordLength)
{
    ULONG64 nextRecordPosition = 0;
    co_return co_await ReadNextRecordAsync(
        stream,
        invalidLogRecords,
        allocator,
        nextRecordPosition,
        isPhysicalRead,
        useInvalidRecordPosition,
        setRecordLength);
}

Awaitable<LogRecord::SPtr> LogRecord::ReadNextRecordAsync(
    __in io::KStream & stream,
    __in InvalidLogRecords & invalidLogRecords,
    __in KAllocator & allocator,
    __out ULONG64 & nextRecordPosition,
    __in bool isPhysicalRead,
    __in bool useInvalidRecordPosition,
    __in bool setRecordLength)
{
    NTSTATUS status = 0;
    ULONG32 recordLength = 0;
    ULONG64 recordPosition = (ULONG64) stream.GetPosition();
    ULONG bytesRead = 0;
    ULONG32 bytesToRead = 0;
    KBuffer::SPtr buffer = nullptr;

    // Read next record length
    bytesToRead = sizeof(ULONG32);
    status = KBuffer::Create(
        bytesToRead,
        buffer,
        allocator);
    THROW_ON_FAILURE(status);

    status = co_await stream.ReadAsync(
        *buffer,
        bytesRead,
        0,
        bytesToRead
        );

    ASSERT_IFNOT(
        status == STATUS_SUCCESS && bytesRead == bytesToRead,
        "Incorrect bytes read: {0} {1}", bytesRead, bytesToRead);

    BinaryReader recordSizereader(*buffer, allocator);
    recordSizereader.Read(recordLength);

    nextRecordPosition = 2 * sizeof(ULONG32) + recordPosition + recordLength;

    bytesToRead = sizeof(ULONG32) + recordLength;

    status = KBuffer::Create(
        bytesToRead,
        buffer,
        allocator);
    THROW_ON_FAILURE(status);

    status = co_await stream.ReadAsync(
        *buffer,
        bytesRead,
        0,
        bytesToRead);

    ASSERT_IFNOT(
        status == STATUS_SUCCESS && bytesRead == bytesToRead,
        "Incorrect bytes read: {0} {1}", bytesRead, bytesToRead);

    BinaryReader logReader(*buffer, allocator);
    if (useInvalidRecordPosition)
    {
        recordPosition = Constants::InvalidRecordPosition;
    }

    co_return ReadRecordWithHeaders(
        logReader,
        recordPosition,
        invalidLogRecords,
        allocator,
        isPhysicalRead,
        setRecordLength);
}

Awaitable<LogRecord::SPtr> LogRecord::ReadPreviousRecordAsync(
    __in ILogicalLogReadStream & stream,
    __in InvalidLogRecords & invalidLogRecords,
    __in KAllocator & allocator)
{
    NTSTATUS status = 0;
    ULONG64 recordPosition = 0;
    ULONG32 recordLength = 0;
    ULONG64 readPosition = (ULONG64)stream.GetPosition() - sizeof(ULONG32);
    ULONG bytesRead = 0;
    ULONG32 bytesToRead = 0;
    KBuffer::SPtr buffer = nullptr;

    stream.SetPosition((LONG64)readPosition);

    // Read next record length
    bytesToRead = sizeof(ULONG32);
    status = KBuffer::Create(
        bytesToRead,
        buffer,
        allocator);
    THROW_ON_FAILURE(status);

    status = co_await stream.ReadAsync(
        *buffer,
        bytesRead,
        0,
        bytesToRead);

    ASSERT_IFNOT(
        status == STATUS_SUCCESS && bytesRead == bytesToRead,
        "Incorrect bytes read: {0} {1}", bytesRead, bytesToRead);

    BinaryReader recordSizereader(*buffer, allocator);
    recordSizereader.Read(recordLength);

    readPosition = readPosition - recordLength;
    stream.SetPosition((LONG64)readPosition);

    bytesToRead = sizeof(ULONG32) + recordLength;

    status = KBuffer::Create(
        bytesToRead,
        buffer,
        allocator);
    THROW_ON_FAILURE(status);

    status = co_await stream.ReadAsync(
        *buffer,
        bytesRead,
        0,
        bytesToRead);

    ASSERT_IFNOT(
        status == STATUS_SUCCESS && bytesRead == bytesToRead,
        "Incorrect bytes read: {0} {1}", bytesRead, bytesToRead);

    recordPosition = readPosition - sizeof(ULONG32);

    BinaryReader logReader(*buffer, allocator);

    co_return ReadRecordWithHeaders(
        logReader,
        recordPosition,
        invalidLogRecords,
        allocator);
}

std::wstring LogRecord::get_RecordTypeName() const
{
    wstring name;

    switch (get_RecordType())
    {
    case LogRecordType::Enum::Invalid:
        name = L"Invalid";
        break;
    case LogRecordType::Enum::BeginTransaction:
        name = L"BeginTransaction";
        break;
    case LogRecordType::Enum::Operation:
        name = L"Operation";
        break;
    case LogRecordType::Enum::EndTransaction:
        name = L"EndTransaction";
        break;
    case LogRecordType::Enum::Barrier:
        name = L"Barrier";
        break;
    case LogRecordType::Enum::UpdateEpoch:
        name = L"UpdateEpoch";
        break;
    case LogRecordType::Enum::Backup:
        name = L"Backup";
        break;
    case LogRecordType::Enum::BeginCheckpoint:
        name = L"BeginCheckpoint";
        break;
    case LogRecordType::Enum::EndCheckpoint:
        name = L"EndCheckpoint";
        break;
    case LogRecordType::Enum::Indexing:
        name = L"Indexing";
        break;
    case LogRecordType::Enum::TruncateHead:
        name = L"TruncateHead";
        break;
    case LogRecordType::Enum::TruncateTail:
        name = L"TruncateTail";
        break;
    case LogRecordType::Enum::Information:
        name = L"Information";
        break;
    case LogRecordType::Enum::CompleteCheckpoint:
        name = L"CompleteCheckPoint";
        break;
    default:
        CODING_ASSERT("Unexpected log record type {0}", get_RecordType());
        break;
    }

    return name;
}

// MCoskun: When EnableSecondaryCommitApplyAcknowledgement is true, we need to consider each commit as barrier.
// This is because in-case primary fails before replicating the barrier operation for the in flight group commit.
// In which case, all secondary replicas will go through UpdateEpoch and GetLSN stage.
// During UpdateEpoch, V1 replicator will wait for all operations in the replication stream to be drained/acked.
// Since the barrier has not arrived (and is not coming), the dispatcher will not dispatch these operations.
// Hence these operations will never be acked causing the replica to get stuck.
bool LogRecord::IsBarrierRecord(
    __in LogRecord const & record,
    __in bool considerCommitAsBarrier)
{
    switch (record.RecordType)
    {
    case LogRecordType::Enum::Barrier:
    case LogRecordType::Enum::BeginCheckpoint:
    case LogRecordType::Enum::UpdateEpoch:
    case LogRecordType::Enum::Information:
        return true;

    default:
        if (considerCommitAsBarrier && IsCommitLogRecord(record))
        {
            return true;
        }

        return false;
    }
}

OperationData::CSPtr LogRecord::WriteRecord(
    __in LogRecord & record,
    __in BinaryWriter & binaryWriter,
    __in KAllocator & allocator,
    __in bool isPhysicalWrite,
    __in bool setRecordLength,
    __in bool forceRecomputeOffsets)
{
    // Note: The binary writer is not where the real data is written
    // It is only passed in to avoid each log record from creating its own writer

    // The real data of the log record is returned in the operation data
    // As a result, reset the position of the writer before starting to write

    binaryWriter.Position = 0;

    OperationData::SPtr operationData = OperationData::Create(allocator);

    record.Write(
        binaryWriter,
        *operationData,
        isPhysicalWrite,
        forceRecomputeOffsets);

    ULONG32 recordLength = 0;

    for (ULONG32 i = 0; i < operationData->BufferCount; i++)
    {
        recordLength += (*operationData)[i]->QuerySize();
    }

    if (setRecordLength)
    {
        record.RecordLength = recordLength;
    }

    ULONG32 startingPosition = binaryWriter.Position;
    binaryWriter.Write(recordLength);

    KBuffer::SPtr kBuffer = binaryWriter.GetBuffer(startingPosition);

    // Append and prepend record length in a fixed encoding of sizeof(ULONG32) bytes
    operationData->Append(*kBuffer);
    operationData->InsertAt0(*kBuffer);

    return OperationData::CSPtr(operationData.RawPtr());
}

KArray<LogRecord::SPtr> LogRecord::ReadFromOperationDataBatch(
    __in OperationData const & operationData,
    __in InvalidLogRecords & invalidLogRecords,
    __in KAllocator & allocator)
{
    KArray<LogRecord::SPtr> records(allocator);

    int index = -1;
    while (index < (int)operationData.BufferCount - 2) //-1 for last buffer (copy stage), another -1 for comparing index against count
    {
        LogRecord::SPtr record = LogRecord::InternalReadFromOperationData(
            operationData,
            invalidLogRecords,
            index,
            allocator);
        records.Append(record);
    }
    return records;
}

LogRecord::SPtr LogRecord::InternalReadFromOperationData(
    __in OperationData const & operationData,
    __in InvalidLogRecords & invalidLogRecords,
    __inout int & index,
    __in KAllocator & allocator)
{
    LogRecord::SPtr record;
    LONG64 lsn;
    ULONG64 recordPosition = Constants::InvalidRecordPosition;
    LogRecordType::Enum recordType;
    LONG32 recordTypeInt;

    {
        KBuffer::CSPtr buffer = OperationData::IncrementIndexAndGetBuffer(operationData, index);
        BinaryReader reader(*buffer, allocator);

        // Logical metadata section
        ULONG32 startingPosition = reader.Position;
        ULONG32 sizeOfRecord = 0;
        reader.Read(sizeOfRecord);

        ULONG32 endPosition = startingPosition + sizeOfRecord;

        reader.Read(recordTypeInt);
        recordType = static_cast<LogRecordType::Enum>(recordTypeInt);
        reader.Read(lsn);

        ASSERT_IFNOT(
            endPosition >= reader.Position,
            "Could not have read more than section size: {0} {1}", endPosition, reader.Position);

        reader.Position = endPosition;
    }

    switch (recordType)
    {
    case LogRecordType::Enum::BeginTransaction:
        record = BeginTransactionOperationLogRecord::Create(
            recordType,
            recordPosition,
            lsn,
            *invalidLogRecords.Inv_PhysicalLogRecord,
            *invalidLogRecords.Inv_TransactionLogRecord,
            allocator).RawPtr();
        break;
    case LogRecordType::Enum::Operation:
        record = OperationLogRecord::Create(
            recordType,
            recordPosition,
            lsn,
            *invalidLogRecords.Inv_PhysicalLogRecord,
            *invalidLogRecords.Inv_TransactionLogRecord,
            allocator).RawPtr();
        break;
    case LogRecordType::Enum::EndTransaction:
        record = EndTransactionLogRecord::Create(
            recordType,
            recordPosition,
            lsn,
            *invalidLogRecords.Inv_PhysicalLogRecord,
            *invalidLogRecords.Inv_TransactionLogRecord,
            allocator).RawPtr();
        break;
    case LogRecordType::Enum::Barrier:
        record = BarrierLogRecord::Create(
            recordType,
            recordPosition,
            lsn,
            *invalidLogRecords.Inv_PhysicalLogRecord,
            allocator).RawPtr();
        break;

    case LogRecordType::Enum::UpdateEpoch:
        record = UpdateEpochLogRecord::Create(
            recordType,
            recordPosition,
            lsn,
            *invalidLogRecords.Inv_PhysicalLogRecord,
            allocator).RawPtr();
        break;

    case LogRecordType::Enum::Backup:
        record = BackupLogRecord::Create(
            recordType,
            recordPosition,
            lsn,
            *invalidLogRecords.Inv_PhysicalLogRecord,
            allocator).RawPtr();
        break;
    default:
        CODING_ASSERT("Unexpected log record type {0}", recordType);
    }

    record->ReadLogical(operationData, index);
    return record;
}

LogRecord::SPtr LogRecord::ReadFromOperationData(
    __in OperationData const & operationData,
    __in InvalidLogRecords & invalidLogRecords,
    __in KAllocator & allocator)
{
    int index = -1;

    return LogRecord::InternalReadFromOperationData(
        operationData,
        invalidLogRecords,
        index,
        allocator);
}

LogRecord::SPtr LogRecord::ReadRecord(
    __in BinaryReader & binaryReader,
    __in ULONG64 recordPosition,
    __in InvalidLogRecords & invalidLogRecords,
    __in bool isPhysicalRead,
    __in KAllocator & allocator)
{
    LogRecord::SPtr record;
    LONG64 lsn = Constants::InvalidLsn;
    LogRecordType::Enum recordType;
    LONG32 recordTypeInt;

    // Metadata section
    ULONG32 startingPosition = binaryReader.Position;
    ULONG32 sizeOfSection = 0;
    binaryReader.Read(sizeOfSection);
    ULONG32 endingPosition = startingPosition + sizeOfSection;

    binaryReader.Read(recordTypeInt);
    recordType = static_cast<LogRecordType::Enum>(recordTypeInt);

    switch (recordType)
    {
    case LogRecordType::Enum::BeginTransaction:
        record = BeginTransactionOperationLogRecord::Create(
            recordType,
            recordPosition,
            lsn,
            *invalidLogRecords.Inv_PhysicalLogRecord,
            *invalidLogRecords.Inv_TransactionLogRecord,
            allocator).RawPtr();
        break;
    case LogRecordType::Enum::Operation:
        record = OperationLogRecord::Create(
            recordType,
            recordPosition,
            lsn,
            *invalidLogRecords.Inv_PhysicalLogRecord,
            *invalidLogRecords.Inv_TransactionLogRecord,
            allocator).RawPtr();
        break;
    case LogRecordType::Enum::EndTransaction:
        record = EndTransactionLogRecord::Create(
            recordType,
            recordPosition,
            lsn,
            *invalidLogRecords.Inv_PhysicalLogRecord,
            *invalidLogRecords.Inv_TransactionLogRecord,
            allocator).RawPtr();
        break;
    case LogRecordType::Enum::Barrier:
        record = BarrierLogRecord::Create(
            recordType,
            recordPosition,
            lsn,
            *invalidLogRecords.Inv_PhysicalLogRecord,
            allocator).RawPtr();
        break;

    case LogRecordType::Enum::UpdateEpoch:
        record = UpdateEpochLogRecord::Create(
            recordType,
            recordPosition,
            lsn,
            *invalidLogRecords.Inv_PhysicalLogRecord,
            allocator).RawPtr();
        break;

    case LogRecordType::Enum::Backup:
        record = BackupLogRecord::Create(
            recordType,
            recordPosition,
            lsn,
            *invalidLogRecords.Inv_PhysicalLogRecord,
            allocator).RawPtr();
        break;

    case LogRecordType::Enum::Indexing:
        record = IndexingLogRecord::Create(
            recordType,
            recordPosition,
            lsn,
            *invalidLogRecords.Inv_PhysicalLogRecord,
            allocator).RawPtr();
        break;

    case LogRecordType::Enum::Information:
        record = InformationLogRecord::Create(
            recordType,
            recordPosition,
            lsn,
            *invalidLogRecords.Inv_PhysicalLogRecord,
            allocator).RawPtr();
        break;

    case LogRecordType::Enum::BeginCheckpoint:
        record = BeginCheckpointLogRecord::Create(
            recordType,
            recordPosition,
            lsn,
            *invalidLogRecords.Inv_PhysicalLogRecord,
            *invalidLogRecords.Inv_BeginTransactionOperationLogRecord,
            allocator).RawPtr();
        break;

    case LogRecordType::Enum::EndCheckpoint:
        record = EndCheckpointLogRecord::Create(
            recordType,
            recordPosition,
            lsn,
            *invalidLogRecords.Inv_IndexingLogRecord,
            *invalidLogRecords.Inv_PhysicalLogRecord,
            *invalidLogRecords.Inv_BeginCheckpointLogRecord,
            allocator).RawPtr();
        break;

    case LogRecordType::Enum::CompleteCheckpoint:
        record = CompleteCheckPointLogRecord::Create(
            recordType,
            recordPosition,
            lsn,
            *invalidLogRecords.Inv_PhysicalLogRecord,
            *invalidLogRecords.Inv_IndexingLogRecord,
            allocator).RawPtr();
        break;

    case LogRecordType::Enum::TruncateHead:
        record = TruncateHeadLogRecord::Create(
            recordType,
            recordPosition,
            lsn,
            *invalidLogRecords.Inv_PhysicalLogRecord,
            *invalidLogRecords.Inv_IndexingLogRecord,
            allocator).RawPtr();
        break;

    case LogRecordType::Enum::TruncateTail:
        record = TruncateTailLogRecord::Create(
            recordType,
            recordPosition,
            lsn,
            *invalidLogRecords.Inv_PhysicalLogRecord,
            allocator).RawPtr();
        break;

    default:
        CODING_ASSERT("Unexpected log record type {0}", recordType);
    }

    binaryReader.Read(lsn);
    record->Lsn = lsn;

    ASSERT_IFNOT(
        endingPosition >= binaryReader.Position, 
        "Could not have read more than section size: {0} {1}", endingPosition, binaryReader.Position);
    
    binaryReader.Position = endingPosition;

    record->Read(binaryReader, isPhysicalRead);

    return record;
}

LogRecord::SPtr LogRecord::ReadRecordWithHeaders(
    __in BinaryReader & binaryReader,
    __in ULONG64 recordPosition,
    __in InvalidLogRecords & invalidLogRecords,
    __in KAllocator & allocator,
    __in bool isPhysicalRead,
    __in bool setRecordLength)
{
    LogRecord::SPtr record = ReadRecord(
        binaryReader,
        recordPosition,
        invalidLogRecords,
        isPhysicalRead,
        allocator);

    ULONG32 recordLength = 0;
    binaryReader.Read(recordLength);

    ASSERT_IFNOT(
        recordLength == (binaryReader.Length - sizeof(ULONG32)),
        "Incorrect record length: {0}", recordLength);

    if (setRecordLength)
    {
        record->RecordLength = recordLength;
    }

    return record;
}

PhysicalLogRecord::SPtr LogRecord::get_previousPhysicalRecord() const
{
    return previousPhysicalRecord_;
}

void LogRecord::set_previousPhysicalRecord(__in_opt PhysicalLogRecord * const value)
{
    ASSERT_IFNOT(
        LogRecord::IsInvalid(previousPhysicalRecord_.RawPtr()) || previousPhysicalRecord_.RawPtr() == value || LogRecord::IsInvalid(value),
        "Invlaid physical log record");

    previousPhysicalRecord_ = value;
}

void LogRecord::Read(
    __in BinaryReader & binaryReader,
    __in bool isPhysicalRead)
{
    if (isPhysicalRead)
    {
        // Metadata section
        ULONG32 startingPosition = binaryReader.Position;
        ULONG32 sizeOfSection = 0;
        binaryReader.Read(sizeOfSection);
        ULONG32 endPosition = startingPosition + sizeOfSection;
        
        // Read Physical metadata section
        binaryReader.Read(psn_);
        binaryReader.Read(previousPhysicalRecordOffset_);

        // Jump to the end of the section ignoring fields that are not understood
        ASSERT_IFNOT(
            endPosition >= binaryReader.Position,
            "Could not have read more than section size: {0} {1}", endPosition, binaryReader.Position);

        binaryReader.Position = endPosition;

        if (previousPhysicalRecordOffset_ == 0)
        {
            previousPhysicalRecord_ = nullptr;
        }
    }

    UpdateApproximateDiskSize();
}

void LogRecord::ReadLogical(
    __inout OperationData const & operationData,
    __inout INT & index)
{
    // The logical contents of the log record are read in the LogRecord::ReadFromOperationData() method
    UNREFERENCED_PARAMETER(operationData);
    UNREFERENCED_PARAMETER(index);
}

void LogRecord::Write(
    __in BinaryWriter & binaryWriter,
    __inout OperationData & operationData,
    __in bool isPhysicalWrite,
    __in bool forceRecomputeOffsets)
{
    UNREFERENCED_PARAMETER(forceRecomputeOffsets);

    // NOTE - LogicalData optimization is NOT done here unlike other log records because the LSN is not yet assigned at this point
    // Logical Metadata section

    ULONG32 logicalStartingPosition = binaryWriter.Position;
    binaryWriter.Position += sizeof(ULONG32);

    // Logical Metadata fields
    binaryWriter.Write(static_cast<LONG32>(recordType_));
    binaryWriter.Write(lsn_);

    // End Logical Metadata section
    ULONG32 logicalEndPosition = binaryWriter.Position;

    ULONG32 logicalSizeOfSection = logicalEndPosition - logicalStartingPosition;
    binaryWriter.Position = logicalStartingPosition;
    binaryWriter.Write(logicalSizeOfSection);
    binaryWriter.Position = logicalEndPosition;
    
    KBuffer::SPtr kBuffer = binaryWriter.GetBuffer(logicalStartingPosition);
    operationData.Append(*kBuffer);

    if (isPhysicalWrite)
    {
        // Physical Metadata section
        ULONG32 physicalStartPosition = binaryWriter.Position;
        binaryWriter.Position += sizeof(ULONG32);

        binaryWriter.Write(psn_);

        ASSERT_IFNOT(
            recordPosition_ != Constants::InvalidRecordPosition,
            "Invalid record position: {0}", recordPosition_);

        if (previousPhysicalRecordOffset_ == Constants::InvalidPhysicalRecordOffset)
        {
            ASSERT_IFNOT(
                !PhysicalLogRecord::IsInvalid(previousPhysicalRecord_.RawPtr()),
                "Invalid previous physical log record");

            if (previousPhysicalRecord_ == nullptr)
            {
                previousPhysicalRecordOffset_ = 0;
            }
            else
            {
                ASSERT_IFNOT(
                    previousPhysicalRecord_->RecordPosition != Constants::InvalidRecordPosition,
                    "Invalid previous physical log record position: {0}", previousPhysicalRecord_->RecordPosition);                
                ASSERT_IFNOT(
                    RecordPosition != Constants::InvalidRecordPosition,
                    "Invalid record position: {0}", RecordPosition);
                
                previousPhysicalRecordOffset_ = recordPosition_ - previousPhysicalRecord_->RecordPosition;
            }
        }
        else
        {
            // Record written twice
            CODING_ASSERT(
                "Record written twice. Record Type:{0} RecordPosition={1}",
                RecordType, RecordPosition);
        }

        binaryWriter.Write(previousPhysicalRecordOffset_);

        // End Physical Metadata section
        ULONG32 physicalEndPosition = binaryWriter.Position;
        ULONG32 physicalSizeOfSection = physicalEndPosition - physicalStartPosition;
        binaryWriter.Position = physicalStartPosition;
        binaryWriter.Write(physicalSizeOfSection);
        binaryWriter.Position = physicalEndPosition;

        KBuffer::SPtr lKBuffer = binaryWriter.GetBuffer(physicalStartPosition);
        operationData.Append(*lKBuffer);
    }
}

bool LogRecord::FreePreviousLinksLowerThanPsn(
    __in LONG64 newHeadPsn,
    __in InvalidLogRecords & invalidLogRecords)
{
    if (previousPhysicalRecord_ != nullptr &&
        previousPhysicalRecord_->Psn < newHeadPsn)
    {
        ASSERT_IF(
            previousPhysicalRecordOffset_ == Constants::InvalidPhysicalRecordOffset,
            "FreePreviousLinksLowerThanPsn: PreviousPhysicalRecordOffset is invalid. Record LSN is {0}", lsn_);

        previousPhysicalRecord_ = previousPhysicalRecordOffset_ == 0 ?
            nullptr :
            invalidLogRecords.Inv_PhysicalLogRecord;

        return true;
    }

    return false;
}

bool LogRecord::Test_Equals(__in LogRecord const & other) const
{
    return recordType_ == other.RecordType && lsn_ == other.Lsn && psn_ == other.Psn;
}

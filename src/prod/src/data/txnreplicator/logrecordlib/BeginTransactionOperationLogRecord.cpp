// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

const ULONG BeginTransactionOperationLogRecord::DiskSpaceUsed = sizeof(LONG32) + sizeof(bool);

BeginTransactionOperationLogRecord::BeginTransactionOperationLogRecord()
    : TransactionLogRecord()
    , isSingleOperationTransaction_(false)
    , metaData_(nullptr)
    , undo_(nullptr)
    , redo_(nullptr)
    , operationContext_(nullptr)
    , recordEpoch_(Epoch::InvalidEpoch())
    , replicatedData_(nullptr)
{
}

BeginTransactionOperationLogRecord::BeginTransactionOperationLogRecord(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in TransactionLogRecord & invalidTransactionLog)
    : TransactionLogRecord(recordType, recordPosition, lsn, invalidPhysicalLogRecord, invalidTransactionLog)
    , isSingleOperationTransaction_(false)
    , metaData_(nullptr)
    , undo_(nullptr)
    , redo_(nullptr)
    , operationContext_(nullptr)
    , recordEpoch_(Epoch::InvalidEpoch())
    , replicatedData_(nullptr)
{
    parentTransactionRecord_ = nullptr;

    ASSERT_IFNOT(recordType == LogRecordType::Enum::BeginTransaction, "Expected begin xact log record type, actual: {0}", recordType);
}

BeginTransactionOperationLogRecord::BeginTransactionOperationLogRecord(
    __in TransactionBase & transactionBase,
    __in bool isSingleOperationTransaction,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in_opt OperationContext const * const operationContext,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in TransactionLogRecord & invalidTransactionLog)
    : TransactionLogRecord(LogRecordType::Enum::BeginTransaction, transactionBase, invalidPhysicalLogRecord, invalidTransactionLog, nullptr)
    , isSingleOperationTransaction_(isSingleOperationTransaction)
    , metaData_(metaData)
    , undo_(undo)
    , redo_(redo)
    , operationContext_(operationContext)
    , recordEpoch_(Epoch::InvalidEpoch())
    , replicatedData_(nullptr)
{
    UpdateApproximateDiskSize();
}

BeginTransactionOperationLogRecord::~BeginTransactionOperationLogRecord()
{
}

BeginTransactionOperationLogRecord::SPtr BeginTransactionOperationLogRecord::Create(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in TransactionLogRecord & invalidTransactionLog,
    __in KAllocator & allocator)
{
    BeginTransactionOperationLogRecord* pointer = _new(BEGINTXLOGRECORD_TAG, allocator) BeginTransactionOperationLogRecord(
        recordType, 
        recordPosition, 
        lsn, 
        invalidPhysicalLogRecord, 
        invalidTransactionLog);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return BeginTransactionOperationLogRecord::SPtr(pointer);
}

BeginTransactionOperationLogRecord::SPtr BeginTransactionOperationLogRecord::Create(
    __in TransactionBase & transactionBase,
    __in bool isSingleOperationTransaction,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in_opt OperationContext const * const operationContext,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in TransactionLogRecord & invalidTransactionLog,
    __in KAllocator & allocator)
{
    BeginTransactionOperationLogRecord* pointer = _new(BEGINTXLOGRECORD_TAG, allocator) BeginTransactionOperationLogRecord(
        transactionBase,
        isSingleOperationTransaction,
        metaData,
        undo,
        redo,
        operationContext,
        invalidPhysicalLogRecord,
        invalidTransactionLog);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return BeginTransactionOperationLogRecord::SPtr(pointer);
}

std::wstring BeginTransactionOperationLogRecord::ToString() const
{
    wstring boolean = isSingleOperationTransaction_ ?  L"true" : L"false"; 

    std::wstring logRecordString = Constants::CompEndlJSON;

    logRecordString += L"Single Operation Transaction" + 
        Constants::DivisionBoolJSON + boolean;
    logRecordString += Constants::CloseJSON;

    return __super::ToString() + logRecordString;
}

void BeginTransactionOperationLogRecord::UpdateApproximateDiskSize()
{
    ApproximateSizeOnDisk = ApproximateSizeOnDisk + DiskSpaceUsed;
    ApproximateSizeOnDisk = ApproximateSizeOnDisk + __super::CalculateDiskWriteSize(metaData_.RawPtr());
    ApproximateSizeOnDisk = ApproximateSizeOnDisk + __super::CalculateDiskWriteSize(undo_.RawPtr());
    ApproximateSizeOnDisk = ApproximateSizeOnDisk + __super::CalculateDiskWriteSize(redo_.RawPtr());
}

OperationContext::CSPtr BeginTransactionOperationLogRecord::ResetOperationContext()
{
    OperationContext::CSPtr context = operationContext_;
    if (context != nullptr)
    {
        operationContext_ = nullptr;
    }

    return context;
}

void BeginTransactionOperationLogRecord::Read(
    __in BinaryReader & binaryReader,
    __in bool isPhysicalRead)
{
    UNREFERENCED_PARAMETER(isPhysicalRead);

    // IF THIS METHOD CHANGES, READLOGICAL MUST CHANGE ACCORDINGLY AS WE ARE ADDING MULTIPLE ARRAYSEGMENTS HERE
    __super::Read(binaryReader, isPhysicalRead);

    // Metadata section.
    ULONG32 startingPosition = binaryReader.Position;
    ULONG32 sizeOfSection = 0;
    binaryReader.Read(sizeOfSection);
    ULONG32 endPosition = startingPosition + sizeOfSection;

    // Read single operation optimization flag.
    binaryReader.Read(isSingleOperationTransaction_);

    // Jump to the end of the section ignoring fields that are not understood.
    ASSERT_IFNOT(
        endPosition >= binaryReader.Position,
        "Invalid begin xact op log record, end position: {0} reader position:{1}",
        endPosition, binaryReader.Position);

    binaryReader.Position = endPosition;

    metaData_ = OperationData::DeSerialize(binaryReader, GetThisAllocator());
    redo_ = OperationData::DeSerialize(binaryReader, GetThisAllocator());
    undo_ = OperationData::DeSerialize(binaryReader, GetThisAllocator());

    UpdateApproximateDiskSize();
}

void BeginTransactionOperationLogRecord::ReadLogical(
    __in OperationData const & operationData,
    __inout INT & index)
{
    __super::ReadLogical(operationData, index);

    KBuffer::CSPtr buffer = OperationData::IncrementIndexAndGetBuffer(operationData, index);
    BinaryReader reader(*buffer, GetThisAllocator());

    // Metadata section.
    ULONG32 startingPosition = reader.Position;
    ULONG32 sizeOfSection = 0;
    reader.Read(sizeOfSection);
    ULONG32 endPosition = startingPosition + sizeOfSection;

    // Read metadata fields
    reader.Read(isSingleOperationTransaction_);

    // Jump to the end of the section ignoring fields that are not understood.
    ASSERT_IFNOT(
        endPosition >= reader.Position, 
        "Invalid logical begin xact op log record, end position: {0} reader position:{1}",
        endPosition, reader.Position);

    reader.Position = endPosition;

    metaData_ = OperationData::DeSerialize(operationData, index, GetThisAllocator());
    redo_ = OperationData::DeSerialize(operationData, index, GetThisAllocator());
    undo_ = OperationData::DeSerialize(operationData, index, GetThisAllocator());

    UpdateApproximateDiskSize();
}

void BeginTransactionOperationLogRecord::Write(
    __in BinaryWriter & binaryWriter,
    __inout OperationData & operationData,
    __in bool isPhysicalWrite,
    __in bool forceRecomputeOffsets)
{
    __super::Write(binaryWriter, operationData, isPhysicalWrite, forceRecomputeOffsets);

    if(replicatedData_ == nullptr)
    {
        replicatedData_ = OperationData::Create(GetThisAllocator());
        ULONG32 startingPosition = binaryWriter.Position;
        
        binaryWriter.Position += sizeof(ULONG32);
        
        binaryWriter.Write(isSingleOperationTransaction_);
        
        ULONG32 endPosition = binaryWriter.Position;
        ULONG32 sizeOfSection = endPosition - startingPosition;

        binaryWriter.Position = startingPosition;
        binaryWriter.Write(sizeOfSection);
        binaryWriter.Position = endPosition;

        KBuffer::SPtr logMetadata = binaryWriter.GetBuffer(startingPosition);
        replicatedData_->Append(*logMetadata);

        // Write user metadata
        OperationData::Serialize(
            binaryWriter, 
            metaData_.RawPtr(),
            *replicatedData_);

        // Write user redo data
        OperationData::Serialize(
            binaryWriter,
            redo_.RawPtr(),
            *replicatedData_);

        // Write user undo data
        OperationData::Serialize(
            binaryWriter,
            undo_.RawPtr(),
            *replicatedData_);
    }
    
    operationData.Append(*replicatedData_);
}

ULONG BeginTransactionOperationLogRecord::GetSizeOnWire() const
{
    return
        __super::GetSizeOnWire() +
        sizeof(LONG32) +
        __super::CalculateWireSize(metaData_.RawPtr()) +
        __super::CalculateWireSize(redo_.RawPtr()) +
        __super::CalculateWireSize(undo_.RawPtr()) +
        sizeof(bool);
}

bool BeginTransactionOperationLogRecord::Test_Equals(__in LogRecord const & other) const
{
    BeginTransactionOperationLogRecord const & otherBeginTxLogRecord = dynamic_cast<BeginTransactionOperationLogRecord const &>(other);

    if (__super::Test_Equals(other))
    {
        return
            isSingleOperationTransaction_ == otherBeginTxLogRecord.isSingleOperationTransaction_ &&
            metaData_ != nullptr ?
                metaData_->Test_Equals(*otherBeginTxLogRecord.metaData_) :
                (metaData_ == otherBeginTxLogRecord.metaData_) != 0 &&
            redo_ != nullptr ?
                redo_->Test_Equals(*otherBeginTxLogRecord.redo_) :
                (redo_ == otherBeginTxLogRecord.redo_) != 0 &&
            undo_ != nullptr ?
                undo_->Test_Equals(*otherBeginTxLogRecord.undo_) :
                (undo_ == otherBeginTxLogRecord.undo_) != 0;
    }

    return false;
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

const ULONG OperationLogRecord::DiskSpaceUsed = sizeof(LONG32) + sizeof(bool);

OperationLogRecord::OperationLogRecord(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in TransactionLogRecord & invalidTransactionLog)
    : TransactionLogRecord(recordType, recordPosition, lsn, invalidPhysicalLogRecord, invalidTransactionLog)
    , isRedoOnly_(false)
    , metaData_(nullptr)
    , undo_(nullptr)
    , redo_(nullptr)
    , operationContext_(nullptr)
    , replicatedData_(nullptr)
{
    ASSERT_IFNOT(recordType == LogRecordType::Enum::Operation, "Expected operation log record type, actual: {0}", recordType);
}

OperationLogRecord::OperationLogRecord(
    __in TransactionBase & transaction,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in_opt OperationContext const * const operationContext,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in TransactionLogRecord & invalidTransactionLog)
    : TransactionLogRecord(LogRecordType::Enum::Operation, transaction, invalidPhysicalLogRecord, invalidTransactionLog, &invalidTransactionLog)
    , isRedoOnly_(false)
    , metaData_(metaData)
    , undo_(undo)
    , redo_(redo)
    , operationContext_(operationContext)
    , replicatedData_(nullptr)
{
    UpdateApproximateDiskSize();
}

OperationLogRecord::OperationLogRecord(
    __in TransactionBase & transaction,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const redo,
    __in_opt OperationContext const * const operationContext,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in TransactionLogRecord & invalidTransactionLog)
    : TransactionLogRecord(LogRecordType::Enum::Operation, transaction, invalidPhysicalLogRecord, invalidTransactionLog, &invalidTransactionLog)
    , isRedoOnly_(true)
    , metaData_(metaData)
    , undo_(nullptr)
    , redo_(redo)
    , operationContext_(operationContext)
    , replicatedData_(nullptr)
{
    UpdateApproximateDiskSize();
}

OperationLogRecord::~OperationLogRecord()
{
}

OperationLogRecord::SPtr OperationLogRecord::Create(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in TransactionLogRecord & invalidTransactionLog,
    __in KAllocator & allocator)
{
    OperationLogRecord* pointer = _new(OPERATIONLOGRECORD_TAG, allocator) OperationLogRecord(
        recordType, 
        recordPosition, 
        lsn, 
        invalidPhysicalLogRecord, 
        invalidTransactionLog);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return OperationLogRecord::SPtr(pointer);
}

OperationLogRecord::SPtr OperationLogRecord::Create(
    __in TransactionBase & transaction,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in_opt OperationContext const * const operationContext,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in TransactionLogRecord & invalidTransactionLog,
    __in KAllocator & allocator)
{
    OperationLogRecord* pointer = _new(OPERATIONLOGRECORD_TAG, allocator) OperationLogRecord(
        transaction,
        metaData,
        undo,
        redo,
        operationContext,
        invalidPhysicalLogRecord,
        invalidTransactionLog);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return OperationLogRecord::SPtr(pointer);
}

OperationLogRecord::SPtr OperationLogRecord::Create(
    __in TransactionBase & transaction,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const redo,
    __in_opt OperationContext const * const operationContext,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in TransactionLogRecord & invalidTransactionLog,
    __in KAllocator & allocator)
{
    OperationLogRecord* pointer = _new(OPERATIONLOGRECORD_TAG, allocator) OperationLogRecord(
        transaction,
        metaData,
        redo,
        operationContext,
        invalidPhysicalLogRecord,
        invalidTransactionLog);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return OperationLogRecord::SPtr(pointer);
}

std::wstring OperationLogRecord::ToString() const
{
    wstring boolean = isRedoOnly_ ? L"true" : L"false"; 

    std::wstring logRecordString = Constants::CompEndlJSON;

    logRecordString += L"Redo Only" + Constants::DivisionBoolJSON + boolean;
    logRecordString += Constants::CloseJSON;;

    return __super::ToString() + logRecordString;
}


void OperationLogRecord::UpdateApproximateDiskSize()
{
    ApproximateSizeOnDisk = ApproximateSizeOnDisk + DiskSpaceUsed;
    ApproximateSizeOnDisk = ApproximateSizeOnDisk + __super::CalculateDiskWriteSize(metaData_.RawPtr());
    ApproximateSizeOnDisk = ApproximateSizeOnDisk + __super::CalculateDiskWriteSize(redo_.RawPtr());

    if (!isRedoOnly_)
    {
        ApproximateSizeOnDisk = ApproximateSizeOnDisk + __super::CalculateDiskWriteSize(undo_.RawPtr());
    }
}

OperationContext::CSPtr OperationLogRecord::ResetOperationContext()
{
    OperationContext::CSPtr context = operationContext_;
    if (context != nullptr)
    {
        operationContext_ = nullptr;
    }

    return context;
}

void OperationLogRecord::Read(
    __in BinaryReader & binaryReader,
    __in bool isPhysicalRead)
{
    // IF THIS METHOD CHANGES, READLOGICAL MUST CHANGE ACCORDINGLY AS WE ARE ADDING MULTIPLE ARRAYSEGMENTS HERE
    // THIS IS TRUE FOR ANY REDO+UNDO TYPE RECORDS
    __super::Read(binaryReader, isPhysicalRead);

    // Read Metadata size
    ULONG32 logicalStartingPosition = binaryReader.Position;
    ULONG32 logicalSizeOfSection = 0;
    binaryReader.Read(logicalSizeOfSection);
    ULONG32 logicalEndPosition = logicalStartingPosition + logicalSizeOfSection;

    binaryReader.Read(isRedoOnly_);

    // Jump to the end of the section ignoring fields that are not understood.
    ASSERT_IFNOT(
        logicalEndPosition >= binaryReader.Position,
        "Invalid operation log record, end position: {0} reader position:{1}",
        logicalEndPosition, binaryReader.Position);

    binaryReader.Position = logicalEndPosition;

    if (isRedoOnly_)
    {
        transactionBase_ = AtomicRedoOperation::CreateAtomicRedoOperation(
            transactionBase_->TransactionId,
            false,
            GetThisAllocator()).RawPtr();
    }

    metaData_ = OperationData::DeSerialize(binaryReader, GetThisAllocator());
    redo_ = OperationData::DeSerialize(binaryReader, GetThisAllocator());

    if (!isRedoOnly_)
    {
        undo_ = OperationData::DeSerialize(binaryReader, GetThisAllocator());
    }

    UpdateApproximateDiskSize();
}

void OperationLogRecord::ReadLogical(
    __in OperationData const & operationData,
    __inout INT & index)
{
    __super::ReadLogical(operationData, index);
    
    KBuffer::CSPtr buffer = OperationData::IncrementIndexAndGetBuffer(operationData, index); 
    BinaryReader reader(*buffer, GetThisAllocator());

    // Read Metadata size
    ULONG32 logicalStartingPosition = reader.Position;
    ULONG32 logicalSizeOfSection = 0;
    reader.Read(logicalSizeOfSection);
    ULONG32 logicalEndPosition = logicalStartingPosition + logicalSizeOfSection;

    reader.Read(isRedoOnly_);

    // Jump to the end of the section ignoring fields that are not understood.
    ASSERT_IFNOT(
        logicalEndPosition >= reader.Position,
        "Invalid operation log record, logical end position: {0} reader position:{1}",
        logicalEndPosition, reader.Position);

    reader.Position = logicalEndPosition;

    if (isRedoOnly_)
    {
        transactionBase_ = AtomicRedoOperation::CreateAtomicRedoOperation(
            transactionBase_->TransactionId,
            false,
            GetThisAllocator()).RawPtr();
    }

    metaData_ = OperationData::DeSerialize(operationData, index, GetThisAllocator());
    redo_ = OperationData::DeSerialize(operationData, index, GetThisAllocator());
   
    if (!isRedoOnly_)
    {
        undo_ = OperationData::DeSerialize(operationData, index, GetThisAllocator());
    }

    UpdateApproximateDiskSize();
}

void OperationLogRecord::Write(
    __in BinaryWriter & binaryWriter,
    __inout OperationData & operationData,
    __in bool isPhysicalWrite,
    __in bool forceRecomputeOffsets)
{
    __super::Write(binaryWriter, operationData, isPhysicalWrite, forceRecomputeOffsets);

    if (replicatedData_ == nullptr)
    {
        OperationData::SPtr localReplicatedData = OperationData::Create(GetThisAllocator());
        ULONG32 startingPosition = binaryWriter.Position;

        // Metadata Size
        binaryWriter.Position += sizeof(ULONG32);

        binaryWriter.Write(isRedoOnly_);

        // End of metadata.
        ULONG32 endPosition = binaryWriter.Position;
        ULONG32 sizeOfSection = endPosition - startingPosition;

        binaryWriter.Position = startingPosition;
        binaryWriter.Write(sizeOfSection);
        binaryWriter.Position = endPosition;

        KBuffer::SPtr logMetaData = binaryWriter.GetBuffer(startingPosition);
        localReplicatedData->Append(*logMetaData);

        // Write user metadata
        OperationData::Serialize(
            binaryWriter,
            metaData_.RawPtr(),
            *localReplicatedData);

        // Write user redo data
        OperationData::Serialize(
            binaryWriter,
            redo_.RawPtr(),
            *localReplicatedData);

        if (!isRedoOnly_)
        {
            // Write user undo data
            OperationData::Serialize(
                binaryWriter,
                undo_.RawPtr(),
                *localReplicatedData);
        }

        replicatedData_ = localReplicatedData.RawPtr();
    }

    operationData.Append(*replicatedData_);
}

ULONG OperationLogRecord::GetSizeOnWire() const
{
    ULONG size = 
        __super::GetSizeOnWire() +
        sizeof(LONG32) +
        __super::CalculateWireSize(metaData_.RawPtr()) +
        __super::CalculateWireSize(redo_.RawPtr()) +
        sizeof(bool);

    if (!isRedoOnly_)
    {
        size += __super::CalculateWireSize(undo_.RawPtr());
    }

    return size;
}

bool OperationLogRecord::Test_Equals(__in LogRecord const & other) const
{
    OperationLogRecord const & otherOperationLogRecord = dynamic_cast<OperationLogRecord const &>(other);

    if (__super::Test_Equals(other))
    {
        return
            isRedoOnly_ == otherOperationLogRecord.isRedoOnly_ &&
            metaData_ != nullptr ?
                metaData_->Test_Equals(*otherOperationLogRecord.metaData_) :
                (metaData_ == otherOperationLogRecord.metaData_) != 0 &&
            redo_ != nullptr ?
                redo_->Test_Equals(*otherOperationLogRecord.redo_) :
                (redo_ == otherOperationLogRecord.redo_) != 0 &&
            undo_ != nullptr ?
                undo_->Test_Equals(*otherOperationLogRecord.undo_) :
                (undo_ == otherOperationLogRecord.undo_) != 0;
    }

    return false;
}

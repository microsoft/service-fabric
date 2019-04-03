// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

const ULONG EndTransactionLogRecord::DiskSpaceUsed = sizeof(bool) + sizeof(LONG32);
const ULONG EndTransactionLogRecord::SizeOnWireIncrement = sizeof(LONG32) + sizeof(bool);

EndTransactionLogRecord::EndTransactionLogRecord(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in TransactionLogRecord & invalidTransactionLog)
    : TransactionLogRecord(recordType, recordPosition, lsn, invalidPhysicalLogRecord, invalidTransactionLog)
    , isCommitted_(false)
    , isThisReplicaTransaction_(false)
{
    ASSERT_IFNOT(recordType == LogRecordType::Enum::EndTransaction, "Expected end xact record type, actual: {0}", recordType);
}

EndTransactionLogRecord::EndTransactionLogRecord(
    __in TransactionBase & transaction,
    __in bool isCommitted,
    __in bool isThisReplicaTransaction,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in TransactionLogRecord & invalidTransactionLog)
    : TransactionLogRecord(LogRecordType::EndTransaction, transaction, invalidPhysicalLogRecord, invalidTransactionLog, &invalidTransactionLog)
    , isCommitted_(isCommitted)
    , isThisReplicaTransaction_(isThisReplicaTransaction)
{
    UpdateApproximateDiskSize();
}

EndTransactionLogRecord::~EndTransactionLogRecord()
{
}

EndTransactionLogRecord::SPtr EndTransactionLogRecord::Create(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in TransactionLogRecord & invalidTransactionLog,
    __in KAllocator & allocator)
{
    EndTransactionLogRecord* pointer = _new(ENDTXLOGRECORD_TAG, allocator) EndTransactionLogRecord(
        recordType, 
        recordPosition, 
        lsn, 
        invalidPhysicalLogRecord, 
        invalidTransactionLog);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return EndTransactionLogRecord::SPtr(pointer);
}

EndTransactionLogRecord::SPtr EndTransactionLogRecord::Create(
    __in TransactionBase & transaction,
    __in bool isCommitted,
    __in bool isThisReplicaTransaction,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in TransactionLogRecord & invalidTransactionLog,
    __in KAllocator & allocator)
{
    EndTransactionLogRecord* pointer = _new(ENDTXLOGRECORD_TAG, allocator) EndTransactionLogRecord(
        transaction,
        isCommitted,
        isThisReplicaTransaction,
        invalidPhysicalLogRecord,
        invalidTransactionLog);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return EndTransactionLogRecord::SPtr(pointer);;
}

std::wstring EndTransactionLogRecord::ToString() const
{
    wstring boolean = isCommitted_ ? L"true" : L"false"; 

    std::wstring logRecordString = Constants::CompEndlJSON;

    logRecordString += L"Committed" + Constants::DivisionBoolJSON + boolean;
    logRecordString += Constants::CloseJSON;

    std::wstring logRecordWstring(logRecordString.begin(), logRecordString.end());

    return __super::ToString() + logRecordString;
}

void EndTransactionLogRecord::UpdateApproximateDiskSize()
{
    ApproximateSizeOnDisk = ApproximateSizeOnDisk + DiskSpaceUsed;
}

void EndTransactionLogRecord::Read(
    __in BinaryReader & binaryReader,
    __in bool isPhysicalRead)
{
    __super::Read(binaryReader, isPhysicalRead);
    ReadPrivate(binaryReader, isPhysicalRead);
}

void EndTransactionLogRecord::ReadLogical(
    __in OperationData const & operationData,
    __inout INT & index)
{
    __super::ReadLogical(operationData, index);
    KBuffer::CSPtr buffer = OperationData::IncrementIndexAndGetBuffer(operationData, index);
    BinaryReader reader(*buffer, GetThisAllocator());
    ReadPrivate(reader, false);
}

void EndTransactionLogRecord::ReadPrivate(
    __in BinaryReader & binaryReader,
    __in bool isPhysicalRead)
{
    UNREFERENCED_PARAMETER(isPhysicalRead);

    // Read Metadata size
    ULONG32 startingPosition = binaryReader.Position;
    ULONG32 sizeOfSection = 0;
    binaryReader.Read(sizeOfSection);
    ULONG32 endPosition = startingPosition + sizeOfSection;

    binaryReader.Read(isCommitted_);

    ASSERT_IFNOT(
        endPosition >= binaryReader.Position,
        "Invalid end xact op log record, end position: {0} reader position:{1}",
        endPosition, binaryReader.Position);

    binaryReader.Position = endPosition;

    UpdateApproximateDiskSize();
}

void EndTransactionLogRecord::Write(
    __in BinaryWriter & binaryWriter,
    __inout OperationData & operationData,
    __in bool isPhysicalWrite,
    __in bool forceRecomputeOffsets)
{
    __super::Write(binaryWriter, operationData, isPhysicalWrite, forceRecomputeOffsets);

    if (replicatedData_ == nullptr)
    {        
        ULONG32 startingPosition = binaryWriter.Position;
        
        binaryWriter.Position += sizeof(ULONG32);
        
        binaryWriter.Write(isCommitted_);

        ULONG32 endPosition = binaryWriter.Position;
        ULONG32 sizeOfSection = endPosition - startingPosition;

        binaryWriter.Position = startingPosition;
        binaryWriter.Write(sizeOfSection);
        binaryWriter.Position = endPosition;

        replicatedData_ = binaryWriter.GetBuffer(startingPosition);
    }

    operationData.Append(*replicatedData_);
}

ULONG EndTransactionLogRecord::GetSizeOnWire() const
{
    return __super::GetSizeOnWire() + SizeOnWireIncrement;
}

bool EndTransactionLogRecord::Test_Equals(__in LogRecord const & other) const
{
    EndTransactionLogRecord const & otherEndTxLogRecord = dynamic_cast<EndTransactionLogRecord const &>(other);

    if (__super::Test_Equals(other))
    {
        return
            isCommitted_ == otherEndTxLogRecord.isCommitted_;
    }

    return false;
}

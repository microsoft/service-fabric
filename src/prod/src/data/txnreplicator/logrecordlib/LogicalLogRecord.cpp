// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

const ULONG LogicalLogRecord::DiskSpaceUsed = sizeof(LONG32);
const ULONG LogicalLogRecord::SizeOnWire = sizeof(LONG32) + sizeof(LONG32) + sizeof(ULONG32) + sizeof(LONG64);

LogicalLogRecord::LogicalLogRecord()
    : LogRecord()
    , replicationTask_()
    , replicatedData_()
    , replicatedDataBinaryWriter_(GetThisAllocator())
{
}

LogicalLogRecord::LogicalLogRecord(
    __in LogRecordType::Enum recordType,
    __in PhysicalLogRecord & invalidPhysicalLogRecord)
    : LogRecord(recordType, invalidPhysicalLogRecord)
    , replicationTask_()
    , replicatedData_(nullptr)
    , replicatedDataBinaryWriter_(GetThisAllocator())
{
    UpdateApproximateDiskSize(); 
}

LogicalLogRecord::LogicalLogRecord(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord)
    : LogRecord(recordType, recordPosition, lsn, invalidPhysicalLogRecord)
    , replicationTask_()
    , replicatedData_(nullptr)
    , replicatedDataBinaryWriter_(GetThisAllocator())
{
}

LogicalLogRecord::~LogicalLogRecord()
{
    if (replicationTask_)
    {
        ASSERT_IFNOT(
            replicationTask_->IsCompleted,
            "Replication task for LSN {0} is not complete during destruction",
            Lsn);
    }
}

bool LogicalLogRecord::IsAllowedToThrottle(__in LogicalLogRecord const & logicalRecord)
{
    LogRecordType::Enum recordType = logicalRecord.RecordType;

    return
        recordType == LogRecordType::Enum::BeginTransaction ||
        recordType == LogRecordType::Enum::Operation;
}

void LogicalLogRecord::UpdateApproximateDiskSize()
{
    ApproximateSizeOnDisk = ApproximateSizeOnDisk + DiskSpaceUsed;
}

std::wstring LogicalLogRecord::ToString() const
{
    return __super::ToString();
}

OperationData::CSPtr LogicalLogRecord::SerializeLogicalData()
{
    OperationData::SPtr operationData = OperationData::Create(GetThisAllocator());
    Write(replicatedDataBinaryWriter_, *operationData, false, false);

    OperationData::CSPtr returnData(operationData.RawPtr());

    return returnData;
}

void LogicalLogRecord::ReleaseSerializedLogicalDataBuffers()
{
    replicatedDataBinaryWriter_.Reset();
}

Awaitable<NTSTATUS> LogicalLogRecord::AwaitReplication()
{
    ASSERT_IF(Lsn <= 0, "Invalid Replication Lsn in AwaitReplication")
    NTSTATUS status = co_await replicationTask_->AwaitCompletion();

    co_return status;
}

void LogicalLogRecord::Read(
    __in BinaryReader & binaryReader,
    __in bool isPhysicalRead)
{
    __super::Read(binaryReader, isPhysicalRead);
    ReadPrivate(binaryReader, isPhysicalRead);
}

void LogicalLogRecord::ReadLogical(
    __in OperationData const & operationData,
    __inout INT & index)
{
    __super::ReadLogical(operationData, index);
    
    KBuffer::CSPtr buffer = OperationData::IncrementIndexAndGetBuffer(operationData, index);
    BinaryReader reader(*buffer, GetThisAllocator());
    
    ReadPrivate(reader, false);
}

void LogicalLogRecord::Write(
    __in BinaryWriter & binaryWriter,
    __inout OperationData & operationData,
    __in bool isPhysicalWrite,
    __in bool forceRecomputeOffsets)
{
    __super::Write(binaryWriter, operationData, isPhysicalWrite, forceRecomputeOffsets);

    if (replicatedData_ == nullptr)
    {
        ULONG32 startingPosition = binaryWriter.Position;

        // Metadata Size
        binaryWriter.Position += sizeof(ULONG32);

        // Future fields

        // End of metadata.
        ULONG32 endPosition = binaryWriter.Position;
        ULONG32 sizeOfSection = endPosition - startingPosition;

        binaryWriter.Position = startingPosition;
        binaryWriter.Write(sizeOfSection);
        binaryWriter.Position = endPosition;

        replicatedData_ = binaryWriter.GetBuffer(startingPosition);
    }

    operationData.Append(*replicatedData_);
}

ULONG LogicalLogRecord::GetSizeOnWire() const
{
    return SizeOnWire;
}

void LogicalLogRecord::ReadPrivate(
    __in BinaryReader & binaryReader,
    __in bool isPhysicalRead)
{
    UNREFERENCED_PARAMETER(isPhysicalRead);

    // Read Metadata size
    ULONG32 logicalStartingPosition = binaryReader.Position;
    ULONG32 logicalSizeOfSection = 0;
    binaryReader.Read(logicalSizeOfSection);
    ULONG32 logicalEndPosition = logicalStartingPosition + logicalSizeOfSection;

    // Read Metadata fields (future)

    // Jump to the end of the section ignoring fields that are not understood.
    ASSERT_IFNOT(
        logicalEndPosition >= binaryReader.Position,
        "Invalid logical log record, end position: {0} reader position:{1}",
        logicalEndPosition, binaryReader.Position);

    binaryReader.Position = logicalEndPosition;

    UpdateApproximateDiskSize();
}

bool LogicalLogRecord::Test_Equals(__in LogRecord const & other) const
{
    return __super::Test_Equals(other);
}

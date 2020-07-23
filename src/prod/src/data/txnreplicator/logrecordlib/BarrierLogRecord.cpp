// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

const ULONG BarrierLogRecord::DiskSpaceUsed = sizeof(LONG32) + sizeof(LONG64);
const ULONG BarrierLogRecord::SizeOnWireIncrement = sizeof(LONG32) + sizeof(LONG64);

BarrierLogRecord::BarrierLogRecord(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord)
    : LogicalLogRecord(recordType, recordPosition, lsn, invalidPhysicalLogRecord)
    , lastStableLsn_(0)
    , replicatedData_(nullptr)
{
    ASSERT_IFNOT(recordType == LogRecordType::Enum::Barrier, "Expected barrier log record type, actual: {0}", recordType);
}

BarrierLogRecord::BarrierLogRecord(
    __in LONG64 lastStableLsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord)
    : LogicalLogRecord(LogRecordType::Enum::Barrier, invalidPhysicalLogRecord)
    , lastStableLsn_(lastStableLsn)
    , replicatedData_(nullptr)
{
    ASSERT_IFNOT(
        lastStableLsn_ != Constants::InvalidLsn,
        "Invalid last stable lsn: {0}",
        lastStableLsn_);

    UpdateApproximateDiskSize();
}

BarrierLogRecord::BarrierLogRecord(
    __in LONG64 lsn,
    __in LONG64 lastStableLsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord)
    : LogicalLogRecord(LogRecordType::Enum::Barrier, invalidPhysicalLogRecord)
    , lastStableLsn_(lastStableLsn)
    , replicatedData_(nullptr)
{
    Lsn = lsn;
    UpdateApproximateDiskSize();
}

BarrierLogRecord::~BarrierLogRecord()
{
}

BarrierLogRecord::SPtr BarrierLogRecord::Create(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in KAllocator & allocator)
{
    BarrierLogRecord* pointer = _new(BARRIERLOGRECORD_TAG, allocator) BarrierLogRecord(
        recordType, 
        recordPosition, 
        lsn, 
        invalidPhysicalLogRecord);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return BarrierLogRecord::SPtr(pointer);
}

BarrierLogRecord::SPtr BarrierLogRecord::Create(
    __in LONG64 lastStableLsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in KAllocator & allocator)
{
    BarrierLogRecord* pointer = _new(BARRIERLOGRECORD_TAG, allocator) BarrierLogRecord(
        lastStableLsn, 
        invalidPhysicalLogRecord);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return BarrierLogRecord::SPtr(pointer);
}

BarrierLogRecord::SPtr BarrierLogRecord::CreateOneBarrierRecord(
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in KAllocator & allocator)
{
    BarrierLogRecord* pointer = _new(BARRIERLOGRECORD_TAG, allocator) BarrierLogRecord(
        Constants::OneLsn, 
        Constants::ZeroLsn, 
        invalidPhysicalLogRecord);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return BarrierLogRecord::SPtr(pointer);
}

void BarrierLogRecord::UpdateApproximateDiskSize()
{
    ApproximateSizeOnDisk = ApproximateSizeOnDisk + DiskSpaceUsed;
}

void BarrierLogRecord::Read(
    __in BinaryReader & binaryReader,
    __in bool isPhysicalRead)
{
    __super::Read(binaryReader, isPhysicalRead);
    ReadPrivate(binaryReader, isPhysicalRead);
}

void BarrierLogRecord::ReadLogical(
    __in OperationData const & operationData,
    __inout INT & index)
{
    __super::ReadLogical(operationData, index);
    
    KBuffer::CSPtr buffer = OperationData::IncrementIndexAndGetBuffer(operationData, index);
    BinaryReader reader(*buffer, GetThisAllocator());
    
    ReadPrivate(reader, false);
}

void BarrierLogRecord::Write(
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

        binaryWriter.Write(lastStableLsn_);

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

void BarrierLogRecord::ReadPrivate(
    __in BinaryReader & binaryReader,
    __in bool isPhysicalRead)
{
    UNREFERENCED_PARAMETER(isPhysicalRead);

    // Read Metadata size
    ULONG32 logicalStartingPosition = binaryReader.Position;
    ULONG32 logicalSizeOfSection = 0;
    binaryReader.Read(logicalSizeOfSection);
    ULONG32 logicalEndPosition = logicalStartingPosition + logicalSizeOfSection;

    binaryReader.Read(lastStableLsn_);

    // Jump to the end of the section ignoring fields that are not understood.
    ASSERT_IFNOT(
        logicalEndPosition >= binaryReader.Position,
        "Invalid barrier log record, end position: {0} reader position:{1}",
        logicalEndPosition, binaryReader.Position);

    binaryReader.Position = logicalEndPosition;

    UpdateApproximateDiskSize();
}

ULONG BarrierLogRecord::GetSizeOnWire() const
{
    return __super::GetSizeOnWire() + SizeOnWireIncrement;
}

std::wstring BarrierLogRecord::ToString() const
{
    std::wstring logRecordString = Constants::CompEndlJSON;

    logRecordString += L"Last Stable LSN" + Constants::DivisionJSON +
        std::to_wstring(lastStableLsn_) + Constants::Quote;
    logRecordString += Constants::CloseJSON;

    return __super::ToString() + logRecordString;
}

bool BarrierLogRecord::Test_Equals(__in LogRecord const & other) const
{
    BarrierLogRecord const & otherBarrierLogRecord = dynamic_cast<BarrierLogRecord const &>(other);

    if (__super::Test_Equals(other))
    {
        return lastStableLsn_ == otherBarrierLogRecord.lastStableLsn_;
    }

    return false;
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

const ULONG TruncateTailLogRecord::DiskSpaceUsed = sizeof(LONG32);

TruncateTailLogRecord::TruncateTailLogRecord(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord)
    : PhysicalLogRecord(recordType, recordPosition, lsn, invalidPhysicalLogRecord)
{
    ASSERT_IFNOT(
        recordType == LogRecordType::Enum::TruncateTail,
        "Invalid truncate tail log record in constructor: {0}", recordType);
}

TruncateTailLogRecord::TruncateTailLogRecord(
    __in LONG64 lsn,
    __in PhysicalLogRecord & lastLinkedPhysicalRecord,
    __in PhysicalLogRecord & invalidPhysicalLogRecord)
    : PhysicalLogRecord(LogRecordType::Enum::TruncateTail, lsn, &lastLinkedPhysicalRecord, invalidPhysicalLogRecord)
{
    UpdateApproximateDiskSize();
}

TruncateTailLogRecord::~TruncateTailLogRecord()
{
}

TruncateTailLogRecord::SPtr TruncateTailLogRecord::Create(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in KAllocator & allocator)
{
    TruncateTailLogRecord* pointer = _new(TRUNCATETAILLOGRECORD_TAG, allocator) TruncateTailLogRecord(
        recordType, 
        recordPosition, 
        lsn, 
        invalidPhysicalLogRecord);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return TruncateTailLogRecord::SPtr(pointer);
}

TruncateTailLogRecord::SPtr TruncateTailLogRecord::Create(
    __in LONG64 lsn,
    __in PhysicalLogRecord & linkedPhysicalRecord,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in KAllocator & allocator)
{
    TruncateTailLogRecord* pointer = _new(TRUNCATETAILLOGRECORD_TAG, allocator) TruncateTailLogRecord(
        lsn, 
        linkedPhysicalRecord, 
        invalidPhysicalLogRecord);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return TruncateTailLogRecord::SPtr(pointer);
}

void TruncateTailLogRecord::UpdateApproximateDiskSize()
{
    ApproximateSizeOnDisk = ApproximateSizeOnDisk + DiskSpaceUsed;
}

void TruncateTailLogRecord::Read(
    __in BinaryReader & binaryReader,
    __in bool isPhysicalRead)
{
    __super::Read(binaryReader, isPhysicalRead);

    // Read Metadata size
    ULONG32 logicalStartingPosition = binaryReader.Position;
    ULONG32 logicalSizeOfSection = 0;
    binaryReader.Read(logicalSizeOfSection);
    ULONG32 logicalEndPosition = logicalStartingPosition + logicalSizeOfSection;

    ASSERT_IFNOT(
        logicalEndPosition >= binaryReader.Position,
        "Could not have read more than section size: {0} {1}", logicalEndPosition, binaryReader.Position);

    binaryReader.Position = logicalEndPosition;

    UpdateApproximateDiskSize();
}

void TruncateTailLogRecord::Write(
    __in BinaryWriter & binaryWriter,
    __inout OperationData & operationData,
    __in bool isPhysicalWrite,
    __in bool forceRecomputeOffsets)
{
    __super::Write(binaryWriter, operationData, isPhysicalWrite, forceRecomputeOffsets);
    ULONG32 logicalStartingPosition = binaryWriter.Position;
    
    binaryWriter.Position += sizeof(ULONG32);
    
    ULONG32 logicalEndPosition = binaryWriter.Position;
    ULONG32 logicalSizeOfSection = logicalEndPosition - logicalStartingPosition;

    binaryWriter.Position = logicalStartingPosition;
    binaryWriter.Write(logicalSizeOfSection);
    binaryWriter.Position = logicalEndPosition;

    KBuffer::SPtr kBuffer = binaryWriter.GetBuffer(logicalStartingPosition);
    operationData.Append(*kBuffer);
}

bool TruncateTailLogRecord::Test_Equals(__in LogRecord const & other) const
{
    return __super::Test_Equals(other);
}

std::wstring TruncateTailLogRecord::ToString() const
{
    std::wstring logRecordString = Constants::CloseJSON;

    return __super::ToString() + logRecordString;
}

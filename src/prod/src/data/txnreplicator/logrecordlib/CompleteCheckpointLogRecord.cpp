// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

const ULONG CompleteCheckPointLogRecord::DiskSpaceUsed = sizeof(LONG32);

CompleteCheckPointLogRecord::CompleteCheckPointLogRecord(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in IndexingLogRecord & invalidIndexingLogRecord)
    : LogHeadRecord(recordType, recordPosition, lsn, invalidPhysicalLogRecord, invalidIndexingLogRecord)
{
}

CompleteCheckPointLogRecord::CompleteCheckPointLogRecord(
    __in LONG64 lsn,
    __in IndexingLogRecord & logHeadRecord,
    __in_opt PhysicalLogRecord * const lastLinkedPhysicalRecord,
    __in PhysicalLogRecord & invalidPhysicalLogRecord)
    : LogHeadRecord(LogRecordType::Enum::CompleteCheckpoint, logHeadRecord, lsn, lastLinkedPhysicalRecord, invalidPhysicalLogRecord)
{
    UpdateApproximateDiskSize();
}

CompleteCheckPointLogRecord::~CompleteCheckPointLogRecord()
{
}

CompleteCheckPointLogRecord::SPtr CompleteCheckPointLogRecord::Create(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in IndexingLogRecord & invalidIndexingLogRecord,
    __in KAllocator & allocator)
{
    CompleteCheckPointLogRecord* pointer = _new(COMPLETECHECKPOINTLOGRECORD_TAG, allocator) CompleteCheckPointLogRecord(
        recordType, 
        recordPosition, 
        lsn, 
        invalidPhysicalLogRecord, 
        invalidIndexingLogRecord);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return CompleteCheckPointLogRecord::SPtr(pointer);
}

CompleteCheckPointLogRecord::SPtr CompleteCheckPointLogRecord::Create(
    __in LONG64 lsn,
    __in IndexingLogRecord & logHeadRecord,
    __in_opt PhysicalLogRecord * const lastLinkedPhysicalRecord,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in KAllocator & allocator)
{
    CompleteCheckPointLogRecord* pointer = _new(COMPLETECHECKPOINTLOGRECORD_TAG, allocator) CompleteCheckPointLogRecord(
        lsn, 
        logHeadRecord, 
        lastLinkedPhysicalRecord, 
        invalidPhysicalLogRecord);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return CompleteCheckPointLogRecord::SPtr(pointer);
}

std::wstring CompleteCheckPointLogRecord::ToString() const
{
    std::wstring logRecordString = Constants::CloseJSON;

    return __super::ToString() + logRecordString;
}

void CompleteCheckPointLogRecord::UpdateApproximateDiskSize()
{
    ApproximateSizeOnDisk = ApproximateSizeOnDisk + DiskSpaceUsed;
}

void CompleteCheckPointLogRecord::Read(
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
        "Invalid complete checkpoint log record, logical end position: {0} reader position:{1}", 
        logicalEndPosition, binaryReader.Position);

    binaryReader.Position = logicalEndPosition;

    UpdateApproximateDiskSize();
}

void CompleteCheckPointLogRecord::Write(
    __in BinaryWriter & binaryWriter,
    __inout OperationData & operationData,
    __in bool isPhysicalWrite,
    __in bool forceRecomputeOffsets)
{
    __super::Write(binaryWriter, operationData, isPhysicalWrite, forceRecomputeOffsets);
    ULONG32 startingPosition = binaryWriter.Position;

    // Metadata size
    binaryWriter.Position += sizeof(ULONG32);

    // Future fields

    // End of Metadata
    ULONG32 endPosition = binaryWriter.Position;
    ULONG32 sizeOfSection = endPosition - startingPosition;

    binaryWriter.Position = startingPosition;
    binaryWriter.Write(sizeOfSection);
    binaryWriter.Position = endPosition;

    KBuffer::SPtr kBuffer = binaryWriter.GetBuffer(startingPosition);
    operationData.Append(*kBuffer);
}

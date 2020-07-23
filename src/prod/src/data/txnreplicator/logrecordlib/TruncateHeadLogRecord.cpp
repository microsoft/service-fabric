// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

const ULONG TruncateHeadLogRecord::DiskSpaceUsed = sizeof(LONG32) + sizeof(bool);

TruncateHeadLogRecord::TruncateHeadLogRecord(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in IndexingLogRecord & invalidIndexingLogRecord)
    : LogHeadRecord(recordType, recordPosition, lsn, invalidPhysicalLogRecord, invalidIndexingLogRecord)
    , isStable_(false)
    , truncationState_(TruncationState::Invalid)
    , periodicTruncationTimeTicks_(0)
{
    ASSERT_IFNOT(
        recordType == LogRecordType::Enum::TruncateHead, 
        "Invalid truncate head log record in constructor: {0}", recordType);
}

TruncateHeadLogRecord::TruncateHeadLogRecord(
    __in IndexingLogRecord & logHeadRecord,
    __in LONG64 lsn,
    __in_opt PhysicalLogRecord * const lastLinkedPhysicalRecord,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in bool isStable,
    __in LONG64 periodicTruncationTimeStamp)
    : LogHeadRecord(LogRecordType::Enum::TruncateHead, logHeadRecord, lsn, lastLinkedPhysicalRecord, invalidPhysicalLogRecord)
    , isStable_(isStable)
    , truncationState_(TruncationState::Invalid)
    , periodicTruncationTimeTicks_(periodicTruncationTimeStamp)
{
    UpdateApproximateDiskSize();
}

TruncateHeadLogRecord::~TruncateHeadLogRecord()
{
}

TruncateHeadLogRecord::SPtr TruncateHeadLogRecord::Create(
    __in IndexingLogRecord & logHeadRecord,
    __in LONG64 lsn,
    __in_opt PhysicalLogRecord * const lastLinkedPhysicalRecord,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in bool isStable,
    __in LONG64 periodicTruncationTimeStamp,
    __in KAllocator & allocator)
{
    TruncateHeadLogRecord* pointer = _new(TRUNCATEHEADLOGRECORD_TAG, allocator) TruncateHeadLogRecord(
        logHeadRecord, 
        lsn, 
        lastLinkedPhysicalRecord, 
        invalidPhysicalLogRecord, 
        isStable,
        periodicTruncationTimeStamp);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return TruncateHeadLogRecord::SPtr(pointer);
}

TruncateHeadLogRecord::SPtr TruncateHeadLogRecord::Create(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in IndexingLogRecord & invalidIndexingLogRecord,
    __in KAllocator & allocator)
{
    TruncateHeadLogRecord* pointer = _new(TRUNCATEHEADLOGRECORD_TAG, allocator) TruncateHeadLogRecord(
        recordType, 
        recordPosition, 
        lsn, 
        invalidPhysicalLogRecord, 
        invalidIndexingLogRecord);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return TruncateHeadLogRecord::SPtr(pointer);
}

std::wstring TruncateHeadLogRecord::ToString() const
{
    std::wstring logRecordString = Constants::CloseJSON;

    return __super::ToString() + logRecordString;
}

void TruncateHeadLogRecord::UpdateApproximateDiskSize()
{
    ApproximateSizeOnDisk = ApproximateSizeOnDisk + DiskSpaceUsed;
}

void TruncateHeadLogRecord::Read(
    __in BinaryReader & binaryReader,
    __in bool isPhysicalRead)
{
    __super::Read(binaryReader, isPhysicalRead);

    ULONG32 startingPosition = binaryReader.Position;
    ULONG32 sizeOfSection = 0;
    binaryReader.Read(sizeOfSection);
    ULONG32 endPosition = startingPosition + sizeOfSection;

    binaryReader.Read(isStable_);

    // Conditionally read periodicTruncationTimestamp_
    // Ensures compatibility with versions prior to addition of timestamp field
    if (binaryReader.Position < endPosition)
    {
        binaryReader.Read(periodicTruncationTimeTicks_);
    }

    ASSERT_IFNOT(
        endPosition >= binaryReader.Position, 
        "Could not have read more than section size: {0} {1}", endPosition, binaryReader.Position);

    binaryReader.Position = endPosition;

    UpdateApproximateDiskSize();
}

void TruncateHeadLogRecord::Write(
    __in BinaryWriter & binaryWriter,
    __inout OperationData & operationData,
    __in bool isPhysicalWrite,
    __in bool forceRecomputeOffsets)
{
    __super::Write(binaryWriter, operationData, isPhysicalWrite, forceRecomputeOffsets);
    ULONG32 startingPosition = binaryWriter.Position;
    
    binaryWriter.Position += sizeof(ULONG32);
    binaryWriter.Write(isStable_);
    binaryWriter.Write(periodicTruncationTimeTicks_);

    ULONG32 endPosition = binaryWriter.Position;
    ULONG32 sizeOfSection = endPosition - startingPosition;

    binaryWriter.Position = startingPosition;
    binaryWriter.Write(sizeOfSection);
    binaryWriter.Position = endPosition;

    KBuffer::SPtr kBuffer = binaryWriter.GetBuffer(startingPosition);
    operationData.Append(*kBuffer);
}

bool TruncateHeadLogRecord::Test_Equals(__in LogRecord const & other) const
{
    TruncateHeadLogRecord const & otherTruncateHeadLogRecord = dynamic_cast<TruncateHeadLogRecord const &>(other);

    if (__super::Test_Equals(other))
    {
        return isStable_ == otherTruncateHeadLogRecord.isStable_;
    }

    return false;
}

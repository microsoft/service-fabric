// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

const ULONG IndexingLogRecord::DiskSpaceUsed = sizeof(LONG32) + (2 * sizeof(LONG64));

IndexingLogRecord::IndexingLogRecord(
    __in Epoch const & currentEpoch,
    __in LONG64 lsn,
    __in_opt PhysicalLogRecord * const linkedPhysicalRecord,
    __in PhysicalLogRecord & invalidPhysicalLogRecord)
    : PhysicalLogRecord(LogRecordType::Enum::Indexing, lsn, linkedPhysicalRecord, invalidPhysicalLogRecord)
    , recordEpoch_(currentEpoch)
{
    UpdateApproximateDiskSize();
}

IndexingLogRecord::IndexingLogRecord(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord)
    : PhysicalLogRecord(recordType, lsn, recordPosition, invalidPhysicalLogRecord)
    , recordEpoch_(Epoch::InvalidEpoch())
{
}

IndexingLogRecord::IndexingLogRecord()
    : PhysicalLogRecord()
    , recordEpoch_(Epoch::InvalidEpoch())
{
}

IndexingLogRecord::~IndexingLogRecord()
{
}

IndexingLogRecord::SPtr IndexingLogRecord::Create(
    __in Epoch const & currentEpoch,
    __in LONG64 lsn,
    __in_opt PhysicalLogRecord * const linkedPhysicalRecord,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in KAllocator & allocator)
{
    IndexingLogRecord* pointer = _new(INDEXINGLOGRECORD_TAG, allocator) IndexingLogRecord(
        currentEpoch, 
        lsn, 
        linkedPhysicalRecord, 
        invalidPhysicalLogRecord);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return IndexingLogRecord::SPtr(pointer);
}

IndexingLogRecord::SPtr IndexingLogRecord::Create(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in KAllocator & allocator)
{
    IndexingLogRecord* pointer = _new(INDEXINGLOGRECORD_TAG, allocator) IndexingLogRecord(
        recordType, 
        lsn, 
        recordPosition, 
        invalidPhysicalLogRecord);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return IndexingLogRecord::SPtr(pointer);
}

IndexingLogRecord::SPtr IndexingLogRecord::CreateZeroIndexingLogRecord(
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in KAllocator & allocator)
{
    IndexingLogRecord* pointer = _new(INDEXINGLOGRECORD_TAG, allocator) IndexingLogRecord(
        Epoch::ZeroEpoch(), 
        Constants::ZeroLsn, 
        nullptr, 
        invalidPhysicalLogRecord);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return IndexingLogRecord::SPtr(pointer);
}

void IndexingLogRecord::UpdateApproximateDiskSize()
{
    ApproximateSizeOnDisk = ApproximateSizeOnDisk + DiskSpaceUsed;
}

void IndexingLogRecord::Read(
    __in BinaryReader & binaryReader,
    __in bool isPhysicalRead)
{
    __super::Read(binaryReader, isPhysicalRead);

    ULONG32 startingPosition = binaryReader.Position;
    ULONG32 sizeOfSection = 0;
    binaryReader.Read(sizeOfSection);
    ULONG32 endPosition = startingPosition + sizeOfSection;

    LONG64 dataLossVersion;
    LONG64 configurationVersion;
    binaryReader.Read(dataLossVersion);
    binaryReader.Read(configurationVersion);

    recordEpoch_ = Epoch(dataLossVersion, configurationVersion);

    ASSERT_IFNOT(
        endPosition >= binaryReader.Position,
        "Invalid indexing log record, end position: {0} reader position:{1}",
        endPosition, binaryReader.Position);

    binaryReader.Position = endPosition;

    UpdateApproximateDiskSize();
}

void IndexingLogRecord::Write(
    __in BinaryWriter & binaryWriter,
    __inout OperationData & operationData,
    __in bool isPhysicalWrite,
    __in bool forceRecomputeOffsets)
{
    __super::Write(binaryWriter, operationData, isPhysicalWrite, forceRecomputeOffsets);

    ULONG32 startingPosition = binaryWriter.Position;
    binaryWriter.Position += sizeof(ULONG32);

    binaryWriter.Write(recordEpoch_.DataLossVersion);
    binaryWriter.Write(recordEpoch_.ConfigurationVersion);

    ULONG32 endPosition = binaryWriter.Position;
    ULONG32 sizeOfSection = endPosition - startingPosition;
    binaryWriter.Position = startingPosition;
    binaryWriter.Write(sizeOfSection);
    binaryWriter.Position = endPosition;

    KBuffer::SPtr kBuffer = binaryWriter.GetBuffer(startingPosition);
    operationData.Append(*kBuffer);
}

void IndexingLogRecord::OnTruncateHead(
    __in InvalidLogRecords & invalidLogRecords,
    __out ULONG32 & freeLinkCallCount,
    __out ULONG32 & freeLinkCallTrueCount)
{
    freeLinkCallCount = 0;
    freeLinkCallTrueCount = 0;

    // start from the current record (new head) all the way to the latest physical record free-ing previous links if they point to earlier than the new head
    // Without doing so, we slowly start leaking physical links
    // Refer to unit test in Integration.Test.cpp for repro scenario

    PhysicalLogRecord::SPtr currentPhysicalRecord = this;

    do
    {
        freeLinkCallCount += 1;
        if (currentPhysicalRecord->FreePreviousLinksLowerThanPsn(Psn, invalidLogRecords))
        {
            freeLinkCallTrueCount += 1;
        }

        currentPhysicalRecord = currentPhysicalRecord->NextPhysicalRecord;
    
        // We reached the end of the chain
        if (currentPhysicalRecord->Psn == Constants::InvalidPsn)
        {
            break;
        }

    } while (true);
}

bool IndexingLogRecord::Test_Equals(__in LogRecord const & other) const
{
    IndexingLogRecord const & otherIndexingLogRecord = dynamic_cast<IndexingLogRecord const &>(other);    

    if (__super::Test_Equals(other))
    {
        return recordEpoch_ == otherIndexingLogRecord.recordEpoch_;
    }

    return false;
}

std::wstring IndexingLogRecord::ToString() const
{
    std::wstring logRecordString = Constants::CloseJSON;

    return __super::ToString() + logRecordString;
}
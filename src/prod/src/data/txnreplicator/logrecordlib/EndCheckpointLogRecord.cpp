// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

const ULONG EndCheckpointLogRecord::DiskSpaceUsed = sizeof(LONG32) + sizeof(LONG64) + sizeof(ULONG64);

EndCheckpointLogRecord::EndCheckpointLogRecord(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in IndexingLogRecord & invalidIndexLogRecord,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in BeginCheckpointLogRecord & invalidBeginCheckpointLogRecord)
    : LogHeadRecord(recordType, recordPosition, lsn, invalidPhysicalLogRecord, invalidIndexLogRecord)
    , lastStableLsn_(Constants::InvalidLsn)
    , lastCompletedBeginCheckpointRecordOffset_(Constants::InvalidPhysicalRecordOffset)
    , lastCompletedBeginCheckpointRecord_(&invalidBeginCheckpointLogRecord)
{
    ASSERT_IFNOT(
        recordType == LogRecordType::Enum::EndCheckpoint, 
        "Expected end checkpoint record type, actual: {0}", 
        recordType);
}

EndCheckpointLogRecord::EndCheckpointLogRecord(
    __in BeginCheckpointLogRecord & lastCompletedBeginCheckpointRecord,
    __in IndexingLogRecord & logHeadRecord,
    __in LONG64 lsn,
    __in_opt PhysicalLogRecord * const linkedPhysicalRecord,
    __in PhysicalLogRecord & invalidPhysicalRecord)
    : LogHeadRecord(LogRecordType::Enum::EndCheckpoint, logHeadRecord, lsn, linkedPhysicalRecord, invalidPhysicalRecord)
    , lastStableLsn_(lastCompletedBeginCheckpointRecord.LastStableLsn)
    , lastCompletedBeginCheckpointRecordOffset_(Constants::InvalidPhysicalRecordOffset)
    , lastCompletedBeginCheckpointRecord_(&lastCompletedBeginCheckpointRecord)
{
    ASSERT_IFNOT(
        lastStableLsn_ >= lastCompletedBeginCheckpointRecord_->LastStableLsn,
        "Invalid last stable lsn: {0}, should be >= last completed begin checkpoint record: {1}", 
        lastStableLsn_, 
        lastCompletedBeginCheckpointRecord_->LastStableLsn);

    UpdateApproximateDiskSize();
}

EndCheckpointLogRecord::EndCheckpointLogRecord(
    __in BeginCheckpointLogRecord & lastCompletedBeginCheckpointRecord,
    __in IndexingLogRecord & logHeadRecord,
    __in PhysicalLogRecord & invalidPhysicalLogRecord)
    : LogHeadRecord(LogRecordType::Enum::EndCheckpoint, logHeadRecord, Constants::OneLsn, nullptr, invalidPhysicalLogRecord)
    , lastStableLsn_(lastCompletedBeginCheckpointRecord.LastStableLsn)
    , lastCompletedBeginCheckpointRecordOffset_(Constants::InvalidPhysicalRecordOffset)
    , lastCompletedBeginCheckpointRecord_(&lastCompletedBeginCheckpointRecord)
{
    UpdateApproximateDiskSize();
}

EndCheckpointLogRecord::~EndCheckpointLogRecord()
{
}

EndCheckpointLogRecord::SPtr EndCheckpointLogRecord::Create(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in IndexingLogRecord & invalidIndexLogRecord,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in BeginCheckpointLogRecord & invalidBeginCheckpointLogRecord,
    __in KAllocator & allocator)
{
    EndCheckpointLogRecord* pointer = _new(ENDCHECKPOINTLOGRECORD_TAG, allocator) EndCheckpointLogRecord(
        recordType, 
        recordPosition, 
        lsn, 
        invalidIndexLogRecord, 
        invalidPhysicalLogRecord, 
        invalidBeginCheckpointLogRecord);
    
    THROW_ON_ALLOCATION_FAILURE(pointer);
    return EndCheckpointLogRecord::SPtr(pointer);
}

EndCheckpointLogRecord::SPtr EndCheckpointLogRecord::Create(
    __in BeginCheckpointLogRecord & lastCompletedBeginCheckpointRecord,
    __in IndexingLogRecord & logHeadRecord,
    __in LONG64 lsn,
    __in_opt PhysicalLogRecord * const linkedPhysicalRecord,
    __in PhysicalLogRecord & invalidPhysicalRecord,
    __in KAllocator & allocator)
{
    EndCheckpointLogRecord* pointer = _new(ENDCHECKPOINTLOGRECORD_TAG, allocator) EndCheckpointLogRecord(
        lastCompletedBeginCheckpointRecord, 
        logHeadRecord, 
        lsn, 
        linkedPhysicalRecord, 
        invalidPhysicalRecord);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return EndCheckpointLogRecord::SPtr(pointer);
}

EndCheckpointLogRecord::SPtr EndCheckpointLogRecord::CreateOneEndCheckpointLogRecord(
    __in BeginCheckpointLogRecord & lastCompletedBeginCheckpointRecord,
    __in IndexingLogRecord & logHeadRecord,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in KAllocator & allocator)
{
    EndCheckpointLogRecord* pointer = _new(ENDCHECKPOINTLOGRECORD_TAG, allocator) EndCheckpointLogRecord(
        lastCompletedBeginCheckpointRecord, 
        logHeadRecord, 
        invalidPhysicalLogRecord);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return EndCheckpointLogRecord::SPtr(pointer);
}

std::wstring EndCheckpointLogRecord::ToString() const
{
    std::wstring logRecordString = Constants::CloseJSON;

    return __super::ToString() + logRecordString;
}

void EndCheckpointLogRecord::UpdateApproximateDiskSize()
{
    ApproximateSizeOnDisk = ApproximateSizeOnDisk + DiskSpaceUsed;
}

void EndCheckpointLogRecord::Read(
    __in BinaryReader & binaryReader,
    __in bool isPhysicalRead)
{
    __super::Read(binaryReader, isPhysicalRead);
    ULONG32 startingPosition = binaryReader.Position;
    ULONG32 sizeOfSection = 0; 
    binaryReader.Read(sizeOfSection);

    ULONG32 endPosition = startingPosition + sizeOfSection;
    binaryReader.Read(lastStableLsn_);

    if(isPhysicalRead)
    {
        binaryReader.Read(lastCompletedBeginCheckpointRecordOffset_);
    }

    // Jump to the end of the section ignoring fields that are not understood.
    ASSERT_IFNOT(
        endPosition >= binaryReader.Position,
        "Invalid end checkpoint log record, end position: {0} reader position:{1}",
        endPosition, 
        binaryReader.Position);

    binaryReader.Position = endPosition;

    UpdateApproximateDiskSize();
}

void EndCheckpointLogRecord::Write(
    __in BinaryWriter & binaryWriter,
    __inout OperationData & operationData,
    __in bool isPhysicalWrite,
    __in bool forceRecomputeOffsets)
{
    __super::Write(binaryWriter, operationData, isPhysicalWrite, forceRecomputeOffsets);
    ULONG32 startingPosition = binaryWriter.Position;
    binaryWriter.Position += sizeof(ULONG32);
    binaryWriter.Write(lastStableLsn_);
    
    if(isPhysicalWrite)
    {
        if(lastCompletedBeginCheckpointRecordOffset_ == Constants::InvalidPhysicalRecordOffset || forceRecomputeOffsets == true)
        {
            ASSERT_IFNOT(
                lastCompletedBeginCheckpointRecord_ != nullptr || forceRecomputeOffsets == true,
                "EndCheckpointLogRecord::Write : lastCompletedBeginCheckpointRecord_ must not be null. (lastCompletedBeginCheckpointRecord_ != nullptr)={0}, forceRecomputeOffsets={1}",
                lastCompletedBeginCheckpointRecord_ != nullptr,
                forceRecomputeOffsets);

            if (lastCompletedBeginCheckpointRecord_ != nullptr)
            {
                if (lastCompletedBeginCheckpointRecord_->RecordPosition != Constants::InvalidRecordPosition)
                {
                    ASSERT_IFNOT(
                        RecordPosition != Constants::InvalidRecordPosition,
                        "EndCheckpointLogRecord::Write : RecordPosition must be valid");

                    lastCompletedBeginCheckpointRecordOffset_ = RecordPosition - lastCompletedBeginCheckpointRecord_->RecordPosition;
                }
                else
                {
                    lastCompletedBeginCheckpointRecordOffset_ = Constants::InvalidPhysicalRecordOffset;
                }
            }
        }

        binaryWriter.Write(lastCompletedBeginCheckpointRecordOffset_);
    }

    ULONG32 endPosition = binaryWriter.Position;
    ULONG32 sizeOfSection = endPosition - startingPosition;
    binaryWriter.Position = startingPosition;
    binaryWriter.Write(sizeOfSection);
    binaryWriter.Position = endPosition;

    KBuffer::SPtr kBuffer = binaryWriter.GetBuffer(startingPosition);
    operationData.Append(*kBuffer);
}

bool EndCheckpointLogRecord::FreePreviousLinksLowerThanPsn(
    __in LONG64 newHeadPsn,
    __in InvalidLogRecords & invalidLogRecords)
{
    bool ret = __super::FreePreviousLinksLowerThanPsn(
        newHeadPsn,
        invalidLogRecords);

    if (lastCompletedBeginCheckpointRecord_ != nullptr &&
        lastCompletedBeginCheckpointRecord_->Psn < newHeadPsn)
    {
        ASSERT_IF(
            lastCompletedBeginCheckpointRecordOffset_ == Constants::InvalidPhysicalRecordOffset,
            "EndCheckpointLogRecord::FreePreviousLinksLowerThanPsn : LastCompletedBeginCheckpointRecordOffset must be valid");

        lastCompletedBeginCheckpointRecord_ =
            lastCompletedBeginCheckpointRecordOffset_ == 0 ?
            nullptr :
            invalidLogRecords.Inv_BeginCheckpointLogRecord;

        return true;
    }

    return ret;
}

bool EndCheckpointLogRecord::Test_Equals(__in LogRecord const & other) const
{
    EndCheckpointLogRecord const & otherEndCheckpointRecord = dynamic_cast<EndCheckpointLogRecord const &>(other);

    if (__super::Test_Equals(other))
    {
        return 
            lastCompletedBeginCheckpointRecordOffset_ == otherEndCheckpointRecord.lastCompletedBeginCheckpointRecordOffset_ && 
            lastStableLsn_ == otherEndCheckpointRecord.lastStableLsn_;
    }

    return false;
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

const ULONG LogHeadRecord::DiskSpaceUsed = sizeof(LONG32) + sizeof(ULONG64) + (4 * sizeof(LONG64));

LogHeadRecord::LogHeadRecord(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in IndexingLogRecord & invalidIndexingLogRecord)
    : PhysicalLogRecord(recordType, recordPosition, lsn, invalidPhysicalLogRecord)
    , logHeadLsn_(Constants::InvalidLsn)
    , logHeadPsn_(Constants::InvalidPsn)
    , logHeadRecordOffset_(Constants::InvalidPhysicalRecordOffset)
    , logHeadEpoch_(Epoch::InvalidEpoch())
    , logHeadRecord_(&invalidIndexingLogRecord)
{
}

LogHeadRecord::LogHeadRecord(
    __in LogRecordType::Enum recordType,
    __in IndexingLogRecord & logHeadRecord,
    __in LONG64 lsn,
    __in_opt PhysicalLogRecord * const lastLinkedPhysicalRecord,
    __in PhysicalLogRecord & invalidPhysicalRecord)
    : PhysicalLogRecord(recordType, lsn, lastLinkedPhysicalRecord, invalidPhysicalRecord)
    , logHeadLsn_(logHeadRecord.Lsn)
    , logHeadPsn_(logHeadRecord.Psn)
    , logHeadRecordOffset_(Constants::InvalidPhysicalRecordOffset)
    , logHeadEpoch_(logHeadRecord.CurrentEpoch)
    , logHeadRecord_(&logHeadRecord)
{
    UpdateApproximateDiskSize();
}

LogHeadRecord::~LogHeadRecord()
{
}

void LogHeadRecord::UpdateApproximateDiskSize()
{
    ApproximateSizeOnDisk = ApproximateSizeOnDisk + DiskSpaceUsed;
}

void LogHeadRecord::Read(
    __in BinaryReader & binaryReader,
    __in bool isPhysicalRead)
{
    __super::Read(binaryReader, isPhysicalRead);

    ULONG32 startingPosition = binaryReader.Position;
    ULONG32 sizeOfSection = 0;
    binaryReader.Read(sizeOfSection);

    ULONG32 endPosition = startingPosition + sizeOfSection;

    // Read signed LONGLONG - Int64 in Managed
    LONG64 dataLossNumber;
    LONG64 configurationNumber;
    binaryReader.Read(dataLossNumber);
    binaryReader.Read(configurationNumber);

    logHeadEpoch_ = Epoch(dataLossNumber, configurationNumber);

    binaryReader.Read(logHeadLsn_);
    binaryReader.Read(logHeadPsn_);

    if(isPhysicalRead)
    {
        binaryReader.Read(logHeadRecordOffset_);
    }

    ASSERT_IFNOT(
        endPosition >= binaryReader.Position,
        "Invalid log head record, end position: {0} reader position:{1}",
        endPosition, 
        binaryReader.Position);

    binaryReader.Position = endPosition;

    UpdateApproximateDiskSize();
}

void LogHeadRecord::Write(
    __in BinaryWriter & binaryWriter,
    __inout OperationData & operationData,
    __in bool isPhysicalWrite,
    __in bool forceRecomputeOffsets)
{
    __super::Write(binaryWriter, operationData, isPhysicalWrite, forceRecomputeOffsets);
    ULONG32 startingPosition = binaryWriter.Position;

    binaryWriter.Position += sizeof(int);

    binaryWriter.Write(logHeadEpoch_.DataLossVersion);
    binaryWriter.Write(logHeadEpoch_.ConfigurationVersion);
    binaryWriter.Write(logHeadLsn_);
    binaryWriter.Write(logHeadPsn_);

    if(isPhysicalWrite)
    {
        if(logHeadRecordOffset_ == Constants::InvalidPhysicalRecordOffset || forceRecomputeOffsets == true)
        {
            ASSERT_IFNOT(
                logHeadRecord_ != nullptr,
                "logHeadRecord_ must not be null");

            ASSERT_IFNOT(
                logHeadRecord_->RecordPosition != Constants::InvalidRecordPosition,
                "logHeadRecord_->RecordPosition must be valid");

            ASSERT_IFNOT(
                RecordPosition != Constants::InvalidRecordPosition,
                "RecordPosition must be valid");

            logHeadRecordOffset_ = RecordPosition - logHeadRecord_->RecordPosition;
        }

        binaryWriter.Write(logHeadRecordOffset_);
    }

    ULONG32 endPosition = binaryWriter.Position;
    ULONG32 sizeOfSection = endPosition - startingPosition;

    binaryWriter.Position = startingPosition;
    binaryWriter.Write(sizeOfSection);
    binaryWriter.Position = endPosition;

    KBuffer::SPtr kBuffer = binaryWriter.GetBuffer(startingPosition);
    operationData.Append(*kBuffer);
}

bool LogHeadRecord::FreePreviousLinksLowerThanPsn(
    __in LONG64 newHeadPsn,
    __in InvalidLogRecords & invalidLogRecords)
{
    bool ret = __super::FreePreviousLinksLowerThanPsn(
        newHeadPsn,
        invalidLogRecords);

    if (logHeadRecord_ != nullptr &&
        logHeadRecord_->Psn < newHeadPsn)
    {
        ASSERT_IF(
            logHeadRecordOffset_ == Constants::InvalidPhysicalRecordOffset,
            "logHeadRecordOffset_ must be valid");

        logHeadRecord_ = 
            logHeadRecordOffset_ == 0 ?
            nullptr :
            invalidLogRecords.Inv_IndexingLogRecord;
        return true;
    }

    return ret;
}

bool LogHeadRecord::Test_Equals(__in LogRecord const & other) const
{
    LogHeadRecord const & otherLogHeadRecord = dynamic_cast<LogHeadRecord const &>(other);

    if (__super::Test_Equals(other))
    {
        return
            logHeadLsn_ == otherLogHeadRecord.logHeadLsn_ &&
            logHeadPsn_ == otherLogHeadRecord.logHeadPsn_ &&
            logHeadRecordOffset_ == otherLogHeadRecord.logHeadRecordOffset_ &&
            logHeadEpoch_ == otherLogHeadRecord.logHeadEpoch_;
    }

    return false;
}

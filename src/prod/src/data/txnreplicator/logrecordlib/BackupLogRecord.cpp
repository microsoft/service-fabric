// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

const ULONG BackupLogRecord::DiskSpaceUsed = sizeof(LONG32) + sizeof(KGuid) + (3 * sizeof(LONG64)) + (2 * sizeof(ULONG32));
const ULONG BackupLogRecord::SizeOnWireIncrement = sizeof(LONG32) + sizeof(KGuid) + (3 * sizeof(LONG64)) + (2 * sizeof(ULONG32));

BackupLogRecord::BackupLogRecord(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord)
    : LogicalLogRecord(recordType, recordPosition, lsn, invalidPhysicalLogRecord)
    , backupId_()
    , highestBackedUpEpoch_(Epoch::InvalidEpoch())
    , highestBackedupLsn_(0)
    , backupLogRecordCount_(0)
    , backupLogSize_(0)
    , replicatedData_(nullptr)
{
    ASSERT_IFNOT(recordType == LogRecordType::Enum::Backup, "Expected backup log record type, actual: {0}", recordType);
}

BackupLogRecord::BackupLogRecord(
    __in KGuid const & backupId,
    __in Epoch const & highestBackedupEpoch,
    __in LONG64 highestBackedupLsn,
    __in UINT backupLogRecordCount,
    __in UINT backupLogSize,
    __in PhysicalLogRecord & invalidPhysicalLogRecord)
    : LogicalLogRecord(LogRecordType::Enum::Backup, invalidPhysicalLogRecord)
    , backupId_(backupId)
    , highestBackedUpEpoch_(highestBackedupEpoch)
    , highestBackedupLsn_(highestBackedupLsn)
    , backupLogRecordCount_(backupLogRecordCount)
    , backupLogSize_(backupLogSize)
    , replicatedData_(nullptr)
{
    UpdateApproximateDiskSize();
}

BackupLogRecord::~BackupLogRecord()
{
}

BackupLogRecord::SPtr BackupLogRecord::Create(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in KAllocator & allocator)
{
    BackupLogRecord* pointer = _new(BACKUPLOGRECORD_TAG, allocator) BackupLogRecord(
        recordType, 
        recordPosition, 
        lsn, 
        invalidPhysicalLogRecord);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return BackupLogRecord::SPtr(pointer);;
}

BackupLogRecord::SPtr BackupLogRecord::Create(
    __in KGuid const & backupId,
    __in Epoch const & highestBackedupEpoch,
    __in LONG64 highestBackedupLsn,
    __in UINT backupLogRecordCount,
    __in UINT backupLogSize,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in KAllocator & allocator)
{
    BackupLogRecord* pointer = _new(BACKUPLOGRECORD_TAG, allocator) BackupLogRecord(
        backupId, 
        highestBackedupEpoch, 
        highestBackedupLsn, 
        backupLogRecordCount, 
        backupLogSize, 
        invalidPhysicalLogRecord);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return BackupLogRecord::SPtr(pointer);
}

BackupLogRecord::SPtr BackupLogRecord::CreateZeroBackupLogRecord(
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in KAllocator& allocator)
{
    KGuid emptyGuid(TRUE);

    BackupLogRecord* pointer = _new(BACKUPLOGRECORD_TAG, allocator) BackupLogRecord(
        emptyGuid,
        TxnReplicator::Epoch::ZeroEpoch(),
        Constants::ZeroLsn,
        0,
        0,
        invalidPhysicalLogRecord);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return BackupLogRecord::SPtr(pointer);
}

std::wstring BackupLogRecord::ToString() const
{
    std::wstring logRecordString = Constants::CompEndlJSON;

    logRecordString += L"Highest Backedup LSN" + Constants::DivisionJSON +
        std::to_wstring(highestBackedupLsn_) + Constants::EndlJSON;
    logRecordString += L"Backup count" + Constants::DivisionJSON +
        std::to_wstring(backupLogRecordCount_) + Constants::EndlJSON;
    logRecordString += L"Backup size" + Constants::DivisionJSON +
        std::to_wstring(backupLogSize_) + Constants::Quote;
    logRecordString += Constants::CloseJSON;
    
    return __super::ToString() + logRecordString;
}

bool BackupLogRecord::IsZeroBackupLogRecord() const
{
    KGuid emptyGuid(TRUE);

    if (backupId_ != emptyGuid)
    {
        return false;
    }

    ASSERT_IFNOT(
        highestBackedUpEpoch_ == Epoch::InvalidEpoch(),
        "If ZeroBackupLogRecord, epoch must be ZeroEpoch. {0}",
        highestBackedUpEpoch_);

    ASSERT_IFNOT(
        highestBackedupLsn_ == 0,
        "If ZeroBackupLogRecord, highest backed up LSN must be ZeroLsn. {0}",
        highestBackedupLsn_);

    ASSERT_IFNOT(
        backupLogRecordCount_ == 0,
        "If ZeroBackupLogRecord, backupLogRecordCount must be ZeroLsn. {0}",
        backupLogRecordCount_);

    ASSERT_IFNOT(
        backupLogSize_ == 0,
        "If ZeroBackupLogRecord, backupLogSizemust be ZeroLsn. {0}",
        backupLogSize_);

    return true;
}

void BackupLogRecord::UpdateApproximateDiskSize()
{
    ApproximateSizeOnDisk = ApproximateSizeOnDisk + DiskSpaceUsed;
}

void BackupLogRecord::Read(
    __in BinaryReader & binaryReader,
    __in bool isPhysicalRead)
{
    __super::Read(binaryReader, isPhysicalRead);
    ReadPrivate(binaryReader, isPhysicalRead);
}

void BackupLogRecord::ReadLogical(
    __in OperationData const & operationData,
    __inout INT & index)
{
    __super::ReadLogical(operationData, index);
    KBuffer::CSPtr buffer = OperationData::IncrementIndexAndGetBuffer(operationData, index);
    BinaryReader reader(*buffer, GetThisAllocator());
    ReadPrivate(reader, false);
}


void BackupLogRecord::ReadPrivate(
    __in BinaryReader & binaryReader,
    __in bool isPhysicalRead)
{
    UNREFERENCED_PARAMETER(isPhysicalRead);

    // Read Metadata size
    ULONG32 startingPosition = binaryReader.Position;
    ULONG32 sizeOfSection = 0;
    binaryReader.Read(sizeOfSection);
    ULONG32 endPosition = startingPosition + sizeOfSection;

    binaryReader.Read(backupId_);

    LONG64 backupDataLossNumber;
    LONG64 backupConfigurationNumber;
    binaryReader.Read(backupDataLossNumber);
    binaryReader.Read(backupConfigurationNumber);

    highestBackedUpEpoch_ = Epoch(backupDataLossNumber, backupConfigurationNumber);

    binaryReader.Read(highestBackedupLsn_);
    binaryReader.Read(backupLogRecordCount_);
    binaryReader.Read(backupLogSize_);

    ASSERT_IFNOT(
        endPosition >= binaryReader.Position,
        "Endposition: {0} should be greater than binary reader position: {1} reading backup log record",
        endPosition, binaryReader.Position);

    binaryReader.Position = endPosition;

    UpdateApproximateDiskSize();
}

void BackupLogRecord::Write(
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

        // Note that if you change this part, begin checkpoint log record also must be changed.
        binaryWriter.Write(backupId_);
        binaryWriter.Write(highestBackedUpEpoch_.DataLossVersion);
        binaryWriter.Write(highestBackedUpEpoch_.ConfigurationVersion);
        binaryWriter.Write(highestBackedupLsn_);

        binaryWriter.Write(backupLogRecordCount_);
        binaryWriter.Write(backupLogSize_);

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

ULONG BackupLogRecord::GetSizeOnWire() const
{
    return __super::GetSizeOnWire() + SizeOnWireIncrement;
}

bool BackupLogRecord::Test_Equals(__in LogRecord const & other) const
{
    BackupLogRecord const & otherBackupLogRecord = dynamic_cast<BackupLogRecord const &>(other);

    if (__super::Test_Equals(other))
    {
        return
            backupId_ == otherBackupLogRecord.backupId_ &&
            highestBackedUpEpoch_ == otherBackupLogRecord.highestBackedUpEpoch_ &&
            highestBackedupLsn_ == otherBackupLogRecord.highestBackedupLsn_ &&
            backupLogRecordCount_ == otherBackupLogRecord.backupLogRecordCount_ &&
            backupLogSize_ == otherBackupLogRecord.backupLogSize_;
    }

    return false;
}

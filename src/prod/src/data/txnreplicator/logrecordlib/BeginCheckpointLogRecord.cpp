// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

// Not including size of section here because it comes from BackupLogRecord::DiskSpaceUsed
const ULONG BeginCheckpointLogRecord::DiskSpaceUsed = (2 * sizeof(LONG64)) + sizeof(ULONG64) + BackupLogRecord::DiskSpaceUsed;

BeginCheckpointLogRecord::BeginCheckpointLogRecord(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in BeginTransactionOperationLogRecord & invalidBeginTx)
    : PhysicalLogRecord(recordType, recordPosition, lsn, invalidPhysicalLogRecord)
    , backupLogRecordCount_(UINT_MAX)
    , backupLogSize_(UINT_MAX)
    , checkpointState_(CheckpointState::Invalid)
    , earliestPendingTransaction_(&invalidBeginTx)
    , earliestPendingTransactionOffset_(Constants::InvalidLogicalRecordOffset)
    , highestBackedUpEpoch_(Epoch::InvalidEpoch())
    , highestBackedUpLsn_(Constants::InvalidLsn)
    , lastStableLsn_(Constants::InvalidLsn)
    , progressVector_()
    , isFirstCheckpointOnFullCopy_(false)
    , firstCheckpointPhase1CompletionTcs_(nullptr)
    , epoch_(Epoch::InvalidEpoch())
    , earliestPendingTransactionInvalidated_(false)
    , periodicCheckpointTimeTicks_(0)
    , periodicTruncationTimeTicks_(0)
{
}

BeginCheckpointLogRecord::BeginCheckpointLogRecord(
  __in bool isFirstCheckpointOnFullCopy,
    __in ProgressVector & progressVector,
    __in_opt BeginTransactionOperationLogRecord * const earliestPendingTransaction,
    __in Epoch const & headEpoch,
    __in Epoch const & epoch,
    __in LONG64 lsn,
    __in_opt PhysicalLogRecord * const lastLinkedPhysicalRecord,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in BackupLogRecord const & lastCompletedBackupLogRecord,
    __in ULONG progressVectorMaxEntries,
    __in LONG64 periodicCheckpointTimeTicks,
    __in LONG64 periodicTruncationTimeTicks)
    : PhysicalLogRecord(LogRecordType::Enum::BeginCheckpoint, lsn, lastLinkedPhysicalRecord, invalidPhysicalLogRecord)
    , backupLogRecordCount_(lastCompletedBackupLogRecord.BackupLogRecordCount) 
    , backupLogSize_(lastCompletedBackupLogRecord.BackupLogSize)
    , checkpointState_(CheckpointState::Invalid)
    , epoch_(Epoch::InvalidEpoch())
    , earliestPendingTransaction_(earliestPendingTransaction)
    , earliestPendingTransactionOffset_(Constants::InvalidLogicalRecordOffset)
    , highestBackedUpEpoch_(lastCompletedBackupLogRecord.HighestBackedupEpoch) 
    , highestBackedUpLsn_(lastCompletedBackupLogRecord.HighestBackedupLsn)
    , lastStableLsn_(Constants::InvalidLsn)
    , progressVector_(ProgressVector::Clone(progressVector, progressVectorMaxEntries, highestBackedUpEpoch_, headEpoch, GetThisAllocator()))
    , isFirstCheckpointOnFullCopy_(isFirstCheckpointOnFullCopy)
    , firstCheckpointPhase1CompletionTcs_(CompletionTask::CreateAwaitableCompletionSource<NTSTATUS>(BEGINCHECKPOINTLOGRECORD_TAG, GetThisAllocator()).RawPtr())
    , earliestPendingTransactionInvalidated_(false)
    , periodicCheckpointTimeTicks_(periodicCheckpointTimeTicks)
    , periodicTruncationTimeTicks_(periodicTruncationTimeTicks)
{
    epoch_ = (earliestPendingTransaction_ != nullptr) ? earliestPendingTransaction_->RecordEpoch : epoch;
    UpdateApproximateDiskSize();
}

BeginCheckpointLogRecord::BeginCheckpointLogRecord()
    : PhysicalLogRecord()
    , backupLogRecordCount_(UINT_MAX) // backupLogRecordCount_ and backupLogSize_ == 0 indicates that the current backup stream has zero logs hence 0kb size
    , backupLogSize_(UINT_MAX)
    , checkpointState_(CheckpointState::Invalid)
    , epoch_(Epoch::InvalidEpoch())
    , earliestPendingTransaction_(nullptr)
    , earliestPendingTransactionOffset_(Constants::InvalidLogicalRecordOffset)
    , highestBackedUpEpoch_(Epoch::InvalidEpoch()) // highestBackedUpEpoch_ = InvalidEpoch and highestBackedUpLsn_ = Constants::InvalidLsn Indicates a full backup has not been taken yet
    , highestBackedUpLsn_(Constants::InvalidLsn)
    , lastStableLsn_(Constants::InvalidLsn)
    , progressVector_()
    , isFirstCheckpointOnFullCopy_(false)
    , firstCheckpointPhase1CompletionTcs_(nullptr)
    , earliestPendingTransactionInvalidated_(false)
    , periodicCheckpointTimeTicks_(0)
    , periodicTruncationTimeTicks_(0)
{
}

BeginCheckpointLogRecord::BeginCheckpointLogRecord(
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in BeginTransactionOperationLogRecord & invalidBeginTx,
    __in bool dummy)
    : PhysicalLogRecord(LogRecordType::Enum::BeginCheckpoint, Constants::ZeroLsn, nullptr, invalidPhysicalLogRecord)
    , backupLogRecordCount_(0)
    , backupLogSize_(0)
    , checkpointState_(CheckpointState::Completed)
    , epoch_(Epoch::ZeroEpoch())
    , earliestPendingTransaction_(&invalidBeginTx)
    , earliestPendingTransactionOffset_(0)
    , highestBackedUpEpoch_(Epoch::ZeroEpoch())
    , highestBackedUpLsn_(Constants::ZeroLsn)
    , lastStableLsn_(Constants::ZeroLsn)
    , progressVector_(ProgressVector::Clone(*ProgressVector::CreateZeroProgressVector(GetThisAllocator()), 0, Epoch::ZeroEpoch(), Epoch::ZeroEpoch(), GetThisAllocator()))
    , isFirstCheckpointOnFullCopy_(false)
    , firstCheckpointPhase1CompletionTcs_(nullptr)
    , earliestPendingTransactionInvalidated_(false)
    , periodicCheckpointTimeTicks_(Common::DateTime::Now().Ticks)
    , periodicTruncationTimeTicks_(Common::DateTime::Now().Ticks)
{
    // periodicCheckpointTimeTicks_ persists current time during creation of ZeroBeginCheckpointLogRecord.
    // Ensures that replica close prior to first checkpoint recovers initial time
    UNREFERENCED_PARAMETER(dummy);
}

BeginCheckpointLogRecord::~BeginCheckpointLogRecord()
{
}

BeginCheckpointLogRecord::SPtr BeginCheckpointLogRecord::Create(
    __in LogRecordType::Enum recordType,
    __in ULONG64 recordPosition,
    __in LONG64 lsn,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in BeginTransactionOperationLogRecord & invalidBeginTx,
    __in KAllocator & allocator)
{
    BeginCheckpointLogRecord* pointer = _new(BEGINCHECKPOINTLOGRECORD_TAG, allocator) BeginCheckpointLogRecord(
    recordType, 
        recordPosition, 
        lsn, 
        invalidPhysicalLogRecord, 
        invalidBeginTx);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return BeginCheckpointLogRecord::SPtr(pointer);
}

BeginCheckpointLogRecord::SPtr BeginCheckpointLogRecord::CreateZeroBeginCheckpointLogRecord(
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in BeginTransactionOperationLogRecord & invalidBeginTx,
    __in KAllocator & allocator)
{
    BeginCheckpointLogRecord* pointer = _new(BEGINCHECKPOINTLOGRECORD_TAG, allocator) BeginCheckpointLogRecord(
        invalidPhysicalLogRecord, 
        invalidBeginTx, 
        true);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return BeginCheckpointLogRecord::SPtr(pointer);
}

BeginCheckpointLogRecord::SPtr BeginCheckpointLogRecord::Create(
  __in bool isFirstCheckpointOnFullCopy,
    __in ProgressVector & progressVector,
    __in_opt BeginTransactionOperationLogRecord * const earliestPendingTransaction,
    __in Epoch const & headEpoch,
    __in Epoch const & epoch,
    __in LONG64 lsn,
    __in_opt PhysicalLogRecord * const lastLinkedPhysicalRecord,
    __in PhysicalLogRecord & invalidPhysicalLogRecord,
    __in BackupLogRecord const & lastCompletedBackupLogRecord,
    __in ULONG progressVectorMaxEntries,
    __in LONG64 periodicCheckpointTimeTicks,
    __in LONG64 periodicTruncationTimeTicks,
    __in KAllocator & allocator)
{
    BeginCheckpointLogRecord* pointer = _new(BEGINCHECKPOINTLOGRECORD_TAG, allocator) BeginCheckpointLogRecord(
    isFirstCheckpointOnFullCopy,
        progressVector, 
        earliestPendingTransaction,
        headEpoch,
        epoch, 
        lsn, 
        lastLinkedPhysicalRecord, 
        invalidPhysicalLogRecord, 
        lastCompletedBackupLogRecord,
        progressVectorMaxEntries,
        periodicCheckpointTimeTicks,
        periodicTruncationTimeTicks);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return BeginCheckpointLogRecord::SPtr(pointer);
}

void BeginCheckpointLogRecord::UpdateApproximateDiskSize()
{
    ApproximateSizeOnDisk = ApproximateSizeOnDisk + DiskSpaceUsed + (progressVector_->Length * sizeof(LONG64) * 6);
}

void BeginCheckpointLogRecord::Read(
    __in BinaryReader & binaryReader,
    __in bool isPhysicalRead)
{
    __super::Read(binaryReader, isPhysicalRead);

    ULONG32 startingPosition = binaryReader.Position;
    ULONG32 sizeOfSection = 0;
    binaryReader.Read(sizeOfSection);
    ULONG32 endPosition = startingPosition + sizeOfSection;
    
    progressVector_ = ProgressVector::Create(GetThisAllocator());
    progressVector_->Read(binaryReader, isPhysicalRead);
    binaryReader.Read(earliestPendingTransactionOffset_);

    LONG64 copyDataLossVersion;
    LONG64 copyConfigurationVersion;
    binaryReader.Read(copyDataLossVersion);
    binaryReader.Read(copyConfigurationVersion);
    epoch_ = Epoch(copyDataLossVersion, copyConfigurationVersion);

    binaryReader.Read(backupId_);

    LONG64 backupDataLossVersion;
    LONG64 backupConfigurationVersion;
    binaryReader.Read(backupDataLossVersion);
    binaryReader.Read(backupConfigurationVersion);
    highestBackedUpEpoch_ = Epoch(backupDataLossVersion, backupConfigurationVersion);

    binaryReader.Read(highestBackedUpLsn_);
    binaryReader.Read(backupLogRecordCount_);
    binaryReader.Read(backupLogSize_);

    // Conditionally read periodicCheckpointTimeTicks_
    // Ensures compatibility with versions prior to addition of timestamp field
    if (binaryReader.Position < endPosition)
    {
        binaryReader.Read(periodicCheckpointTimeTicks_);
        binaryReader.Read(periodicTruncationTimeTicks_);
    }

    ASSERT_IFNOT(
        endPosition >= binaryReader.Position,
        "Invalid begin checkpoint log record, end position: {0} reader position:{1}",
        endPosition, binaryReader.Position);

    binaryReader.Position = endPosition;

    UpdateApproximateDiskSize();
}

void BeginCheckpointLogRecord::Write(
    __in BinaryWriter & binaryWriter,
    __inout OperationData & operationData,
    __in bool isPhysicalWrite,
    __in bool forceRecomputeOffsets)
{
    __super::Write(binaryWriter, operationData, isPhysicalWrite, forceRecomputeOffsets);
    ULONG32 startingPosition = binaryWriter.Position;
    binaryWriter.Position += sizeof(ULONG32);

    // By now, all the log records before this begin checkpoint must have been serialized
    if (earliestPendingTransactionOffset_ == Constants::InvalidLogicalRecordOffset || forceRecomputeOffsets == true)
    {
        ASSERT_IFNOT(
            earliestPendingTransaction_ == nullptr ||
            earliestPendingTransaction_->RecordType != LogRecordType::Enum::Invalid ||
            forceRecomputeOffsets == true,
            "Earliest pending transaction must be null or RecordType Invalid. forceRecomputeOffsets={0}",
            forceRecomputeOffsets);

        if (earliestPendingTransaction_ == nullptr)
        {
            earliestPendingTransactionOffset_ = 0;
        }
        else
        {
            earliestPendingTransactionOffset_ = RecordPosition - earliestPendingTransaction_->RecordPosition;
        }
    }

    ASSERT_IFNOT(
        progressVector_ != nullptr,
        "BeginCheckpointLogRecord progressVector must exist");

    progressVector_->Write(binaryWriter);
    binaryWriter.Write(earliestPendingTransactionOffset_);

    ASSERT_IFNOT(
        epoch_ != Epoch::InvalidEpoch(),
        "BeginCheckpointLogRecord epoch must be valid");

    binaryWriter.Write(epoch_.DataLossVersion);
    binaryWriter.Write(epoch_.ConfigurationVersion);

    ASSERT_IFNOT(
        highestBackedUpEpoch_.DataLossVersion != Constants::InvalidLsn,
        "BeginCheckpointLogRecord highest backed up epoch DataLossVersion must be valid");

    ASSERT_IFNOT(
        highestBackedUpEpoch_.ConfigurationVersion != Constants::InvalidLsn,
        "BeginCheckpointLogRecord highest backed up epoch ConfigurationVersion must be valid");

    ASSERT_IFNOT(
        highestBackedUpLsn_ != Constants::InvalidLsn,
        "BeginCheckpointLogRecord highest backed up LSN must be valid");

    ASSERT_IFNOT(
        backupLogRecordCount_ != UINT_MAX,
        "BeginCheckpointLogRecord backupLogRecordCount_ must be < UINT_MAX");

    ASSERT_IFNOT(
        backupLogSize_ != UINT_MAX,
        "BeginCheckpointLogRecord backupLogSize_ must be < UINT_MAX");

    binaryWriter.Write(backupId_);
    binaryWriter.Write(highestBackedUpEpoch_.DataLossVersion);
    binaryWriter.Write(highestBackedUpEpoch_.ConfigurationVersion);
    binaryWriter.Write(highestBackedUpLsn_);

    binaryWriter.Write(backupLogRecordCount_);
    binaryWriter.Write(backupLogSize_);

    binaryWriter.Write(periodicCheckpointTimeTicks_);
    binaryWriter.Write(periodicTruncationTimeTicks_);

    ULONG32 endPosition = binaryWriter.Position;
    ULONG32 sizeOfSection = endPosition - startingPosition;
    binaryWriter.Position = startingPosition;
    binaryWriter.Write(sizeOfSection);
    binaryWriter.Position = endPosition;

    KBuffer::SPtr kBuffer = binaryWriter.GetBuffer(startingPosition);
    operationData.Append(*kBuffer);
}

bool BeginCheckpointLogRecord::Test_Equals(__in LogRecord const & other) const
{
    BeginCheckpointLogRecord const & otherBeginCheckpointRecord = dynamic_cast<BeginCheckpointLogRecord const &>(other);

    if (__super::Test_Equals(other))
    {
        return
            progressVector_->Equals(*otherBeginCheckpointRecord.progressVector_) &&
            epoch_ == otherBeginCheckpointRecord.epoch_ &&
            highestBackedUpEpoch_ == otherBeginCheckpointRecord.highestBackedUpEpoch_ &&
            backupId_ == otherBeginCheckpointRecord.backupId_ &&
            highestBackedUpLsn_ == otherBeginCheckpointRecord.highestBackedUpLsn_ &&
            backupLogRecordCount_ == otherBeginCheckpointRecord.backupLogRecordCount_ &&
            backupLogSize_ == otherBeginCheckpointRecord.backupLogSize_;
    }

    return false;
}

Awaitable<NTSTATUS> BeginCheckpointLogRecord::AwaitCompletionPhase1FirstCheckpointOnFullCopy()
{
    ASSERT_IFNOT(
      isFirstCheckpointOnFullCopy_ == true,
      "Cannot invoke AwaitCompletionOfPerformCheckpointOnFirstCheckpoint if this is not the first checkpoint on full copy");

    NTSTATUS status = co_await firstCheckpointPhase1CompletionTcs_.Get()->GetAwaitable();

    co_return status;
}

void BeginCheckpointLogRecord::SignalExceptionForPhase1OfFirstCheckpointOnFullCopy(__in NTSTATUS error)
{
    ASSERT_IFNOT(
        isFirstCheckpointOnFullCopy_ == true,
        "Cannot invoke AwaitCompletionOfPerformCheckpointOnFirstCheckpoint if this is not the first checkpoint on full copy or if exception is null");

    firstCheckpointPhase1CompletionTcs_.Get()->SetResult(error);
}

void BeginCheckpointLogRecord::SignalCompletionPhase1FirstCheckpointOnFullCopy()
{
    ASSERT_IFNOT(
        isFirstCheckpointOnFullCopy_ == true,
        "Cannot invoke AwaitCompletionOfPerformCheckpointOnFirstCheckpoint if this is not the first checkpoint on full copy");

    firstCheckpointPhase1CompletionTcs_.Get()->SetResult(STATUS_SUCCESS);
}

std::wstring BeginCheckpointLogRecord::ToString() const
{
    std::wstring logRecordString = Constants::CompEndlJSON;

    logRecordString += L"Earliest Pending Transaction Offset" + Constants::DivisionJSON +
        std::to_wstring(earliestPendingTransactionOffset_) + Constants::Quote;
    logRecordString += Constants::CloseJSON;

    return __super::ToString() + logRecordString;
}

bool BeginCheckpointLogRecord::FreePreviousLinksLowerThanPsn(
    __in LONG64 newHeadPsn,
    __in InvalidLogRecords & invalidLogRecords)
{
    bool ret = __super::FreePreviousLinksLowerThanPsn(
        newHeadPsn,
        invalidLogRecords);

    if (earliestPendingTransaction_ != nullptr &&
        earliestPendingTransaction_->Psn < newHeadPsn)
    {
        ASSERT_IF(
            earliestPendingTransactionOffset_ == Constants::InvalidLogicalRecordOffset,
            "FreePreviousLinksLowerThanPsn: Earliest pending transaction offset cannot be invalid for checkpoint record lsn: {0}, psn: {1}",
            Lsn,
            Psn);

        earliestPendingTransaction_ = invalidLogRecords.Inv_BeginTransactionOperationLogRecord;

        earliestPendingTransactionInvalidated_.store(true);
        return true;
    }

    return ret;
}

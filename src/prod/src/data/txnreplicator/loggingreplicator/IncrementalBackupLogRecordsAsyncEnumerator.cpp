// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::LoggingReplicator;
using namespace Data::LogRecordLib;
using namespace ktl;

IncrementalBackupLogRecordsAsyncEnumerator::SPtr IncrementalBackupLogRecordsAsyncEnumerator::Create(
    __in Data::Utilities::IAsyncEnumerator<LogRecord::SPtr>& source, 
    __in BackupLogRecord const & backupLogRecord,
    __in IReplicatedLogManager const & loggingReplicator,
    __in KAllocator & allocator)
{
    IncrementalBackupLogRecordsAsyncEnumerator * pointer = _new(INCREMENTAL_BACKUP_LOG_RECORDS_ASYNC_ENUMERATOR, allocator) IncrementalBackupLogRecordsAsyncEnumerator(
        source,
        backupLogRecord,
        loggingReplicator);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    THROW_ON_FAILURE(pointer->Status());

    return SPtr(pointer);
}

TxnReplicator::Epoch IncrementalBackupLogRecordsAsyncEnumerator::get_StartingEpoch() const
{
    return startingEpoch_;
}

FABRIC_SEQUENCE_NUMBER IncrementalBackupLogRecordsAsyncEnumerator::get_StartingLSN() const
{
    return startingLSN_;
}

TxnReplicator::Epoch IncrementalBackupLogRecordsAsyncEnumerator::get_HighestBackedUpEpoch() const
{
    return lastEpoch_;
}

FABRIC_SEQUENCE_NUMBER IncrementalBackupLogRecordsAsyncEnumerator::get_HighestBackedUpLSN() const
{
    return lastLSN_;
}

LogRecord::SPtr IncrementalBackupLogRecordsAsyncEnumerator::GetCurrent()
{
    ASSERT_IFNOT(isDisposed_ == false, "GetCurrent called after object was disposed.");
    ASSERT_IFNOT(sourceSPtr_ != nullptr, "Source is nullptr.");

    return sourceSPtr_->GetCurrent();
}

ktl::Awaitable<bool> IncrementalBackupLogRecordsAsyncEnumerator::MoveNextAsync(
    __in CancellationToken const& cancellationToken)
{
    KShared$ApiEntry();

    while(true)
    {
        bool isNotEmpty = co_await sourceSPtr_->MoveNextAsync(cancellationToken);
        if (isNotEmpty == false)
        {
            co_return false;
        }

        LogRecord::SPtr logRecord = sourceSPtr_->GetCurrent();
        ASSERT_IFNOT(logRecord != nullptr, "Log record cannot be nullptr");

        // If not a logical log record, skip.
        if (logRecord->AsLogicalLogRecord() == nullptr)
        {
            continue;
        }

        // If lower than what we backed up on the last backup, skip.
        if (logRecord->Lsn < backupLogRecordCSPtr_->HighestBackedupLsn)
        {
            continue;
        }

        if (logRecord->RecordType == LogRecordType::UpdateEpoch)
        {
            UpdateEpochLogRecord& updateEpochLogRecord = dynamic_cast<UpdateEpochLogRecord &>(*logRecord);
            if (updateEpochLogRecord.EpochValue == backupLogRecordCSPtr_->get_HighestBackedupEpoch())
            {
                // Skip over the Update Epoch if it was previously backed up.
                // Note that Update Epoch does not have a unique LSN.
                continue;
            }

            lastEpoch_ = updateEpochLogRecord.EpochValue;
        }

        count_++;
        lastLSN_ = logRecord->Lsn;

        if (startingLSN_ == FABRIC_INVALID_SEQUENCE_NUMBER)
        {
            startingLSN_ = logRecord->Lsn;
            startingEpoch_ = loggingReplicatorCSPtr_->GetEpoch(startingLSN_);
        }

        co_return true;
    }
}

void IncrementalBackupLogRecordsAsyncEnumerator::Reset()
{
    ASSERT_IFNOT(isDisposed_ == false, "Reset called after object was disposed.");
    ASSERT_IFNOT(sourceSPtr_ != nullptr, "Source is nullptr.");

    sourceSPtr_->Reset();
}

void IncrementalBackupLogRecordsAsyncEnumerator::Dispose()
{
    if (isDisposed_)
    {
        ASSERT_IFNOT(sourceSPtr_ == nullptr, "IncrementalBackupLogRecordsAsyncEnumerator disposed but not the source.");
        return;
    }

    sourceSPtr_->Dispose();
    sourceSPtr_.Reset();

    isDisposed_ = true;
}

ULONG32 IncrementalBackupLogRecordsAsyncEnumerator::Count()
{
    return count_;
}

ktl::Awaitable<void> IncrementalBackupLogRecordsAsyncEnumerator::VerifyDrainedAsync()
{
    KShared$ApiEntry();

    ASSERT_IFNOT(sourceSPtr_ != nullptr, "Source is nullptr.");
    bool isNotEmpty = co_await sourceSPtr_->MoveNextAsync(CancellationToken::None);
    ASSERT_IFNOT(isNotEmpty == false, "enumerator must be drained");

    // Difference with managed.
    // To support incremental backup from difference sources, we must take into account that UpdateEpoch may or may not been backed up last time.
    // If not, we have to put it which has same LSN as HighestBackedupLsn.
    ASSERT_IFNOT(
        startingLSN_ >= backupLogRecordCSPtr_->HighestBackedupLsn, 
        "Must have at least backedup one logical record. StartingLSN: {0} HighestBackedUpLSN: {1}",
        startingLSN_, 
        backupLogRecordCSPtr_->HighestBackedupLsn);
    ASSERT_IFNOT(count_ > 0, "Must have at least backedup one logical record.");
}

IncrementalBackupLogRecordsAsyncEnumerator::IncrementalBackupLogRecordsAsyncEnumerator(
    __in Data::Utilities::IAsyncEnumerator<LogRecord::SPtr> & source,
    __in BackupLogRecord const & backupLogRecord,
    __in IReplicatedLogManager const & loggingReplicator) noexcept
    : sourceSPtr_(&source)
    , backupLogRecordCSPtr_(&backupLogRecord)
    , loggingReplicatorCSPtr_(&loggingReplicator)
    , isDisposed_(false)
    , startingEpoch_(TxnReplicator::Epoch::InvalidEpoch())
    , startingLSN_(FABRIC_INVALID_SEQUENCE_NUMBER)
    , lastEpoch_(TxnReplicator::Epoch::InvalidEpoch())
    , lastLSN_(FABRIC_INVALID_SEQUENCE_NUMBER)
    , count_(0)
{
}

IncrementalBackupLogRecordsAsyncEnumerator::~IncrementalBackupLogRecordsAsyncEnumerator()
{
    ASSERT_IFNOT(isDisposed_ == true, "Log records object is not disposed on destructor");
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

RecoveryPhysicalLogReader::RecoveryPhysicalLogReader(__in ILogManagerReadOnly & logManager)
    : PhysicalLogReader(logManager)
    , readStream_(logManager.CreateReaderStream())
    , startingRecord_(logManager.InvalidLogRecords->Inv_LogRecord)
    , endingRecord_(logManager.InvalidLogRecords->Inv_LogRecord)
    , invalidLogRecords_(logManager.InvalidLogRecords)
{
}

RecoveryPhysicalLogReader::~RecoveryPhysicalLogReader()
{
}
 
RecoveryPhysicalLogReader::SPtr RecoveryPhysicalLogReader::Create(
    __in ILogManagerReadOnly & logManager,
    __in KAllocator & allocator)
{
    RecoveryPhysicalLogReader * pointer = _new(PHYSICALLOGREADER_TAG, allocator)RecoveryPhysicalLogReader(logManager);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return RecoveryPhysicalLogReader::SPtr(pointer);
}

void RecoveryPhysicalLogReader::Dispose()
{
    if (IsDisposed)
    {
        return;
    }

    if (readStream_ != nullptr)
    {
        DisposeReadStream();
    }

    PhysicalLogReader::Dispose();
}

Task RecoveryPhysicalLogReader::DisposeReadStream()
{
    KCoShared$ApiEntry()
    co_await readStream_->CloseAsync();
    co_return;
}

ktl::Awaitable<LogRecord::SPtr> RecoveryPhysicalLogReader::GetLastCompletedBeginCheckpointRecord(LogRecord & record)
{
    PhysicalLogRecord::SPtr previousPhysicalLogRecord = co_await GetPreviousPhysicalRecord(record);
    PhysicalLogRecord::SPtr possibleEndCheckpoint = co_await GetLinkedPhysicalRecord(*previousPhysicalLogRecord);

    while (possibleEndCheckpoint->get_RecordType() != LogRecordType::Enum::EndCheckpoint)
    {
        possibleEndCheckpoint = co_await GetLinkedPhysicalRecord(*possibleEndCheckpoint);
    }

    LogRecord::SPtr nextLogRecord = co_await GetNextLogRecord(possibleEndCheckpoint->RecordPosition);
    EndCheckpointLogRecord::SPtr lastEndCheckpointLogRecord = dynamic_cast<EndCheckpointLogRecord *>(nextLogRecord.RawPtr());
    ASSERT_IF(lastEndCheckpointLogRecord == nullptr, "Null last end checkpoint record found");

    BeginCheckpointLogRecord::SPtr lastCompletedBeginCheckpointRecord = co_await GetLastCompletedBeginCheckpointRecord(*lastEndCheckpointLogRecord);
    LogRecord::SPtr logRecord = dynamic_cast< BeginCheckpointLogRecord *>(lastCompletedBeginCheckpointRecord.RawPtr());

    co_return logRecord;
}

Awaitable<BeginCheckpointLogRecord::SPtr> RecoveryPhysicalLogReader::GetLastCompletedBeginCheckpointRecord(__inout EndCheckpointLogRecord & record)
{
    EndCheckpointLogRecord::SPtr recordSPtr = &record;
    BeginCheckpointLogRecord::SPtr lastCompletedBeginCheckpointRecord = recordSPtr->LastCompletedBeginCheckpointRecord;

    if (!LogRecord::IsInvalid(lastCompletedBeginCheckpointRecord.RawPtr()))
    {
        co_await suspend_never{}; 
        co_return lastCompletedBeginCheckpointRecord;
    }

    ULONG64 lastCompletedBeginCheckpointRecordOffset = recordSPtr->LastCompletedBeginCheckpointRecordOffset;

    ASSERT_IFNOT(lastCompletedBeginCheckpointRecordOffset > 0, "Invalid last completed begin checkpoint record offset: {0}", lastCompletedBeginCheckpointRecordOffset);

    ULONG64 recordPosition = recordSPtr->RecordPosition;
    ASSERT_IFNOT(recordPosition != Constants::InvalidRecordPosition, "Invalid record position in getting last completed begin checkpoint record");

    ULONG64 lastCompletedBeginCheckpointRecordPosition = recordPosition - lastCompletedBeginCheckpointRecordOffset;

    ASSERT_IFNOT(
        lastCompletedBeginCheckpointRecordPosition >= startingRecordPosition_, 
        "Starting record position is greater than last completed begin checkpoint position: {0} {1}",
        startingRecordPosition_, lastCompletedBeginCheckpointRecordPosition);

    LogRecord::SPtr nextLogRecord = co_await GetNextLogRecord(lastCompletedBeginCheckpointRecordPosition);
    lastCompletedBeginCheckpointRecord = dynamic_cast<BeginCheckpointLogRecord *>(nextLogRecord.RawPtr());
    ASSERT_IF(lastCompletedBeginCheckpointRecord == nullptr, "Null last completed begin checkpoint record found");

    recordSPtr->LastCompletedBeginCheckpointRecord = *lastCompletedBeginCheckpointRecord;

    co_return lastCompletedBeginCheckpointRecord;
}

Awaitable<PhysicalLogRecord::SPtr> RecoveryPhysicalLogReader::GetLinkedPhysicalRecord(__in PhysicalLogRecord & record)
{
    PhysicalLogRecord::SPtr recordSPtr = &record;
    PhysicalLogRecord::SPtr linkedPhysicalRecord = recordSPtr->LinkedPhysicalRecord;

    if (LogRecord::IsInvalid(linkedPhysicalRecord.RawPtr()))
    {
        ULONG64 linkedPhysicalRecordOffset = recordSPtr->LinkedPhysicalRecordOffset;

        ASSERT_IFNOT(linkedPhysicalRecordOffset != Constants::InvalidPhysicalRecordOffset, "Invalid linked physical record offset");

        if (linkedPhysicalRecordOffset == 0)
        {
            recordSPtr->LinkedPhysicalRecord = nullptr;
            co_await suspend_never{}; 
            co_return nullptr;
        }

        // Read desired linked record

        ULONG64 recordPosition = recordSPtr->RecordPosition;

        ASSERT_IFNOT(recordPosition != Constants::InvalidRecordPosition, "Invalid record position getting linked physical record");

        ULONG64 linkedPhysicalRecordPosition = recordPosition - linkedPhysicalRecordOffset;

        ASSERT_IFNOT(
            linkedPhysicalRecordPosition >= startingRecordPosition_,
            "Starting record position is greater than the linked physical log record position: {0} {1}",
            startingRecordPosition_, linkedPhysicalRecordPosition);

        LogRecord::SPtr logRecord = co_await GetNextLogRecord(linkedPhysicalRecordPosition);
        linkedPhysicalRecord = dynamic_cast<PhysicalLogRecord *>(logRecord.RawPtr());
        ASSERT_IF(linkedPhysicalRecord == nullptr, "Null linked physical log record");
    }

    ASSERT_IFNOT(
        (linkedPhysicalRecord == nullptr) == (recordSPtr->LinkedPhysicalRecordOffset == 0),
        "Invalid linked physical record offset");

    co_return linkedPhysicalRecord;
}

ULONG64 RecoveryPhysicalLogReader::GetNextLogRecordPosition(__in ULONG64 recordPosition)
{   
    readStream_->SetPosition((LONG64)recordPosition);
    ULONG64 nextRecordPosition = 0;

    LogRecord::SPtr logRecord = SyncAwait(LogRecord::ReadNextRecordAsync(
        *readStream_,
        *invalidLogRecords_,
        GetThisAllocator(),
        nextRecordPosition));

    return nextRecordPosition;
}

Awaitable<LogRecord::SPtr> RecoveryPhysicalLogReader::GetNextLogRecord(__in ULONG64 recordPosition)
{
    if (recordPosition >= (ULONG64)readStream_->GetLength())
    {
        co_await suspend_never{};
        co_return nullptr;
    }

    readStream_->SetPosition((LONG64)recordPosition);

    LogRecord::SPtr logRecord = co_await LogRecord::ReadNextRecordAsync(
        *readStream_,
        *invalidLogRecords_,
        GetThisAllocator());

    co_return logRecord.RawPtr();
}

Awaitable<LogRecord::SPtr> RecoveryPhysicalLogReader::GetPreviousLogRecord(__in ULONG64 recordPosition)
{
    if (recordPosition == 0)
    {
        co_await suspend_never{};
        co_return nullptr;
    }

    readStream_->SetPosition((LONG64)recordPosition);

    LogRecord::SPtr logRecord = co_await LogRecord::ReadPreviousRecordAsync(
        *readStream_,
        *invalidLogRecords_,
        GetThisAllocator());

    co_return logRecord.RawPtr();
}

Awaitable<void> RecoveryPhysicalLogReader::IndexPhysicalRecords(__in PhysicalLogRecord::SPtr & physicalRecord)
{
    while (physicalRecord->RecordPosition > startingRecordPosition_)
    {
        physicalRecord = co_await GetPreviousPhysicalRecord(*physicalRecord);
    }

    ASSERT_IFNOT(!LogRecord::IsInvalid(startingRecord_.RawPtr()), "Invalid starting record while indexing physical records");
}

Awaitable<PhysicalLogRecord::SPtr> RecoveryPhysicalLogReader::GetPreviousPhysicalRecord(__in LogRecord & record)
{
    LogRecord::SPtr recordSPtr = &record;
    PhysicalLogRecord::SPtr previousPhysicalRecord = recordSPtr->PreviousPhysicalRecord;

    if (LogRecord::IsInvalid(previousPhysicalRecord.RawPtr()))
    {
        ULONG64 previousPhysicalRecordOffset = recordSPtr->PreviousPhysicalRecordOffset;

        ASSERT_IFNOT(previousPhysicalRecordOffset != Constants::InvalidPhysicalRecordOffset, "Invalid previous physical record offset");

        if (previousPhysicalRecordOffset == 0)
        {
            recordSPtr->PreviousPhysicalRecord = nullptr;
            co_await suspend_never{}; 
            co_return nullptr;
        }

        // Read previous physical record
        ULONG64 recordPosition = recordSPtr->RecordPosition;
        ASSERT_IFNOT(recordPosition != Constants::InvalidRecordPosition, "Invalid record position in getting previous physical record");

        ULONG64 previousPhysicalRecordPosition = recordPosition - previousPhysicalRecordOffset;

        ASSERT_IFNOT(
            previousPhysicalRecordPosition >= startingRecordPosition_,
            "Starting record position is greater than the previous physical log record position: {0} {1}",
            startingRecordPosition_, previousPhysicalRecordPosition);

        LogRecord::SPtr logRecord = co_await GetNextLogRecord(previousPhysicalRecordPosition);
        previousPhysicalRecord = dynamic_cast<PhysicalLogRecord *>(logRecord.RawPtr());
        ASSERT_IF(previousPhysicalRecord == nullptr, "Unexpected dynamic cast failure");

        PhysicalLogRecord::SPtr nextPhysicalLogRecord = dynamic_cast<PhysicalLogRecord *>(recordSPtr.RawPtr());
        if (nextPhysicalLogRecord != nullptr)
        {
            previousPhysicalRecord->NextPhysicalRecord = nextPhysicalLogRecord;
        }

        recordSPtr->PreviousPhysicalRecord = previousPhysicalRecord.RawPtr();

        if (previousPhysicalRecordPosition == startingRecordPosition_)
        {
            startingRecord_ = logRecord;
        }
    }

    ASSERT_IFNOT(
        (previousPhysicalRecord == nullptr) == (recordSPtr->PreviousPhysicalRecordOffset == 0),
        "Invalid previous physical record offset");

    co_return previousPhysicalRecord;
}

void RecoveryPhysicalLogReader::MoveStartingRecordPosition(
    __in LONG64 startingLsn,
    __in ULONG64 newStartingRecordPosition)
{
    if (isValid_)
    {
        bool isValid = logManager_->AddLogReader(
            startingLsn,
            newStartingRecordPosition,
            endingRecordPosition_,
            L"RecoveryLogReader",
            LogReaderType::Enum::Recovery);

        ASSERT_IFNOT(
            isValid, 
            "Could not move starting record position, starting lsn: {0}, new starting position: {1}, end starting position: {2}",
            startingLsn, newStartingRecordPosition, endingRecordPosition_);

        logManager_->RemoveLogReader(startingRecordPosition_);
    }

    startingRecordPosition_ = newStartingRecordPosition;
    startLsn_ = startingLsn;
    startingRecord_ = invalidLogRecords_->Inv_LogRecord;
}

Awaitable<LogRecord::SPtr> RecoveryPhysicalLogReader::SeekToLastRecord()
{
    ASSERT_IFNOT(endingRecordPosition_ == MAXLONG64, "Expected invalid ending record position, current: {0}", endingRecordPosition_);

    readStream_->SetPosition(readStream_->GetLength());

    LogRecord::SPtr logRecord = co_await LogRecord::ReadPreviousRecordAsync(
        *readStream_, 
        *invalidLogRecords_,
        GetThisAllocator());

    endingRecord_ = logRecord.RawPtr();

    co_return endingRecord_;
}

Awaitable<LogRecord::SPtr> RecoveryPhysicalLogReader::SeekToFirstRecord(__in ULONG64 headTruncationPosition)
{
	readStream_->SetPosition(headTruncationPosition);

	LogRecord::SPtr logRecord = co_await LogRecord::ReadNextRecordAsync(
		*readStream_,
		*invalidLogRecords_,
		GetThisAllocator());

	startingRecord_ = logRecord.RawPtr();

	co_return startingRecord_;
}

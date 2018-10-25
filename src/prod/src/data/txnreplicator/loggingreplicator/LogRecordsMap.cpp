// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LoggingReplicator;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

Common::StringLiteral const TraceComponent("LogRecordsMap");

LogRecordsMap::LogRecordsMap(
    __in PartitionedReplicaId const & traceId,
    __in IndexingLogRecord & indexingLogRecord,
    __in InvalidLogRecords & invalidLogRecords)
    : KObject()
    , KShared()
    , PartitionedReplicaTraceComponent(traceId)
    , firstIndexingLogRecord_(&indexingLogRecord)
    , mode_(Mode::Restore)
    , invalidLogRecords_(&invalidLogRecords)
    , recoveredLastCompletedBeginCheckpointRecord_(nullptr)
    , recoveredLastEndCheckpointRecord_(nullptr)
    , lastCompletedEndCheckpointRecord_(nullptr)
    , lastInProgressCheckpointRecord_(nullptr)
    , lastLinkedPhysicalRecord_(nullptr)
    , lastPhysicalRecord_(invalidLogRecords.Inv_PhysicalLogRecord)
    , lastLogicalSequenceNumber_(indexingLogRecord.Lsn)
    , lastRecoveredAtomicRedoOperationLsn_(MINLONG64)
    , lastStableLsn_(Constants::InvalidLsn)
    , currentLogTailEpoch_(indexingLogRecord.CurrentEpoch)
    , progressVector_(ProgressVector::CreateZeroProgressVector(GetThisAllocator()))
    , transactionMap_(TransactionMap::Create(traceId, GetThisAllocator()))
    , lastPeriodicTruncationTimeTicks_(0)
{
    EventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        L"LogRecordsMap",
        reinterpret_cast<uintptr_t>(this));
}

LogRecordsMap::LogRecordsMap(
    __in PartitionedReplicaId const & traceId,
    __in LONG64 startingLogicalSequenceNumber,
    __in TransactionMap & transactionMap,
    __in Epoch const & currentLogTailEpoch,
    __in LONG64 lastStableLsn,
    __in ProgressVector & progressVector,
    __in BeginCheckpointLogRecord & recoveredLastCompletedBeginCheckpointRecord,
    __in EndCheckpointLogRecord & recoveredLastEndCheckpointRecord,
    __in InvalidLogRecords & invalidLogRecords)
    : KObject()
    , KShared()
    , PartitionedReplicaTraceComponent(traceId)
    , firstIndexingLogRecord_(nullptr)
    , mode_(Mode::Recovery)
    , invalidLogRecords_(&invalidLogRecords)
    , recoveredLastCompletedBeginCheckpointRecord_(&recoveredLastCompletedBeginCheckpointRecord)
    , recoveredLastEndCheckpointRecord_(&recoveredLastEndCheckpointRecord)
    , lastCompletedEndCheckpointRecord_(nullptr)
    , lastInProgressCheckpointRecord_(nullptr)
    , lastLinkedPhysicalRecord_(nullptr)
    , lastPhysicalRecord_(invalidLogRecords.Inv_PhysicalLogRecord)
    , lastLogicalSequenceNumber_(startingLogicalSequenceNumber)
    , lastRecoveredAtomicRedoOperationLsn_(MINLONG64)
    , lastStableLsn_(lastStableLsn)
    , currentLogTailEpoch_(currentLogTailEpoch)
    , progressVector_(&progressVector)
    , transactionMap_(&transactionMap)
    , lastPeriodicTruncationTimeTicks_(0)
{
    EventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        L"LogRecordsMap",
        reinterpret_cast<uintptr_t>(this));
}

LogRecordsMap::~LogRecordsMap()
{
    EventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        L"LogRecordsMap",
        reinterpret_cast<uintptr_t>(this));
}

LogRecordsMap::SPtr LogRecordsMap::Create(
    __in PartitionedReplicaId const & traceId,
    __in IndexingLogRecord & indexingLogRecord,
    __in InvalidLogRecords & invalidLogRecords,
    __in KAllocator & allocator)
{
    LogRecordsMap * pointer = _new(LOGRECORDSMAP_TAG, allocator) LogRecordsMap(
        traceId, 
        indexingLogRecord, 
        invalidLogRecords);

    THROW_ON_ALLOCATION_FAILURE(pointer);

    return LogRecordsMap::SPtr(pointer);
}

LogRecordsMap::SPtr LogRecordsMap::Create(
    __in PartitionedReplicaId const & traceId,
    __in LONG64 startingLogicalSequenceNumber,
    __in TransactionMap & transactionMap,
    __in Epoch const & currentLogTailEpoch,
    __in LONG64 lastStableLsn,
    __in ProgressVector & progressVector,
    __in BeginCheckpointLogRecord & recoveredLastCompletedBeginCheckpointRecord,
    __in EndCheckpointLogRecord & recoveredLastEndCheckpointRecord,
    __in InvalidLogRecords & invalidLogRecords,
    __in KAllocator & allocator)
{
    LogRecordsMap * pointer = _new(LOGRECORDSMAP_TAG, allocator) LogRecordsMap(
        traceId, 
        startingLogicalSequenceNumber, 
        transactionMap,
        currentLogTailEpoch,
        lastStableLsn,
        progressVector,
        recoveredLastCompletedBeginCheckpointRecord,
        recoveredLastEndCheckpointRecord,
        invalidLogRecords);

    THROW_ON_ALLOCATION_FAILURE(pointer);

    return LogRecordsMap::SPtr(pointer);
}

void LogRecordsMap::ProcessLogRecord(__in LogRecord & logRecord)
{
    bool isRecoverableRecord = true;

    ProcessLogRecord(logRecord, isRecoverableRecord);
}

void LogRecordsMap::ProcessLogRecord(
    __in LogRecord & logRecord,
    __out bool & isRecoverableRecord)
{
    isRecoverableRecord = true;
    LogRecord::SPtr logRecordSPtr = &logRecord;

    if (LogRecord::IsPhysical(logRecord))
    {
        PhysicalLogRecord::SPtr physicalLogRecord = dynamic_cast<PhysicalLogRecord *>(logRecordSPtr.RawPtr());
        ProcessLogRecord(*physicalLogRecord);
    }

    switch (logRecordSPtr->RecordType)
    {
    // transaction related log records
    case LogRecordType::Enum::BeginTransaction:
    {
        BeginTransactionOperationLogRecord::SPtr beginTransactionRecord = dynamic_cast<BeginTransactionOperationLogRecord *>(logRecordSPtr.RawPtr());
        ASSERT_IF(beginTransactionRecord == nullptr, "Unexpected dynamic cast failure");
        ProcessLogRecord(*beginTransactionRecord, isRecoverableRecord);
        break;
    }
    case LogRecordType::Enum::Operation:
    {
        OperationLogRecord::SPtr operationLogRecord = dynamic_cast<OperationLogRecord *>(logRecordSPtr.RawPtr());
        ASSERT_IF(operationLogRecord == nullptr, "Unexpected dynamic cast failure");
        ProcessLogRecord(*operationLogRecord, isRecoverableRecord);
        break;
    }
    case LogRecordType::Enum::EndTransaction:
    {
        EndTransactionLogRecord::SPtr endTransactionRecord = dynamic_cast<EndTransactionLogRecord *>(logRecordSPtr.RawPtr());
        ASSERT_IF(endTransactionRecord == nullptr, "Unexpected dynamic cast failure");
        ProcessLogRecord(*endTransactionRecord, isRecoverableRecord);
        break;
    }

    // Other logical records
    case LogRecordType::Enum::Barrier:
    {
        BarrierLogRecord::SPtr barrierLogRecord = dynamic_cast<BarrierLogRecord *>(logRecordSPtr.RawPtr());
        ASSERT_IF(barrierLogRecord == nullptr, "Unexpected dynamic cast failure");
        ProcessLogRecord(*barrierLogRecord, isRecoverableRecord);
        break;
    }
    case LogRecordType::Enum::Backup:
    {
        BackupLogRecord::SPtr backupLogRecord = dynamic_cast<BackupLogRecord *>(logRecordSPtr.RawPtr());
        ASSERT_IF(backupLogRecord == nullptr, "Unexpected dynamic cast failure");
        ProcessLogRecord(*backupLogRecord, isRecoverableRecord);
        break;
    }

    // not replicated logical log
    case LogRecordType::Enum::UpdateEpoch:
    {
        UpdateEpochLogRecord::SPtr updateEpochLogRecord = dynamic_cast<UpdateEpochLogRecord *>(logRecordSPtr.RawPtr());
        ASSERT_IF(updateEpochLogRecord == nullptr, "Unexpected dynamic cast failure");
        ProcessLogRecord(*updateEpochLogRecord, isRecoverableRecord);
        break;
    }

    // Physical records

    // Checkpoint related
    case LogRecordType::Enum::BeginCheckpoint:
    {
        BeginCheckpointLogRecord::SPtr beginCheckpointLogRecord = dynamic_cast<BeginCheckpointLogRecord *>(logRecordSPtr.RawPtr());
        ASSERT_IF(beginCheckpointLogRecord == nullptr, "Unexpected dynamic cast failure");
        ProcessLogRecord(*beginCheckpointLogRecord, isRecoverableRecord);
        break;
    }
    case LogRecordType::Enum::EndCheckpoint:
    {
        EndCheckpointLogRecord::SPtr endCheckpointLogRecord = dynamic_cast<EndCheckpointLogRecord *>(logRecordSPtr.RawPtr());
        ASSERT_IF(endCheckpointLogRecord == nullptr, "Unexpected dynamic cast failure");
        ProcessLogRecord(*endCheckpointLogRecord, isRecoverableRecord);
        break;
    }
    case LogRecordType::Enum::CompleteCheckpoint:
    {
        CompleteCheckPointLogRecord::SPtr completeCheckpointLogRecord = dynamic_cast<CompleteCheckPointLogRecord *>(logRecordSPtr.RawPtr());
        ASSERT_IF(completeCheckpointLogRecord == nullptr, "Unexpected dynamic cast failure");
        ProcessLogRecord(*completeCheckpointLogRecord, isRecoverableRecord);
        break;
    }

    // truncation related
    case LogRecordType::Enum::TruncateHead:
    {
        TruncateHeadLogRecord::SPtr truncateHeadCheckpointLogRecord = dynamic_cast<TruncateHeadLogRecord *>(logRecordSPtr.RawPtr());
        ASSERT_IF(truncateHeadCheckpointLogRecord == nullptr, "Unexpected dynamic cast failure");
        ProcessLogRecord(*truncateHeadCheckpointLogRecord, isRecoverableRecord);
        break;
    }
    case LogRecordType::Enum::Indexing:
    {
        IndexingLogRecord::SPtr indexingLogRecord = dynamic_cast<IndexingLogRecord *>(logRecordSPtr.RawPtr());
        ASSERT_IF(indexingLogRecord == nullptr, "Unexpected dynamic cast failure");
        ProcessLogRecord(*indexingLogRecord, isRecoverableRecord);
        break;
    }

    case LogRecordType::Enum::Information:
    {
        InformationLogRecord::SPtr informationLogRecord = dynamic_cast<InformationLogRecord *>(logRecordSPtr.RawPtr());
        ASSERT_IF(informationLogRecord == nullptr, "Unexpected dynamic cast failure");
        ProcessLogRecord(*informationLogRecord, isRecoverableRecord);
        break;
    }
    case LogRecordType::Enum::TruncateTail:
    {
        TruncateTailLogRecord::SPtr truncateTailLogRecord = dynamic_cast<TruncateTailLogRecord *>(logRecordSPtr.RawPtr());
        ASSERT_IF(truncateTailLogRecord == nullptr, "Unexpected dynamic cast failure");
        ProcessLogRecord(*truncateTailLogRecord, isRecoverableRecord);
        break;
    }

    default:
        CODING_ASSERT("Unexpected record type");
        break;
    }
}

// Links to the last "Linked Physical Log Record".
void LogRecordsMap::ProcessLogRecord(__in PhysicalLogRecord & physicalLogRecord)
{
    lastPhysicalRecord_ = &physicalLogRecord;
    lastPhysicalRecord_->LinkedPhysicalRecord = lastLinkedPhysicalRecord_;
}

void LogRecordsMap::ProcessLogRecord(
    __in BeginTransactionOperationLogRecord & beginTransactionRecord,
    __out bool & isRecoverableRecord)
{
    isRecoverableRecord = true;

    lastLogicalSequenceNumber_++;

    ASSERT_IFNOT(
        beginTransactionRecord.Lsn == lastLogicalSequenceNumber_,
        "{0} begin transaction record lsn:{1} must be equal to last logical lsn {2}",
        TraceId,
        beginTransactionRecord.Lsn,
        lastLogicalSequenceNumber_);
    
    beginTransactionRecord.RecordEpoch = currentLogTailEpoch_;
    transactionMap_->CreateTransaction(beginTransactionRecord);
}

void LogRecordsMap::ProcessLogRecord(
    __in OperationLogRecord & operationLogRecord,
    __out bool & isRecoverableRecord)
{
    lastLogicalSequenceNumber_++;

    ASSERT_IFNOT(
        operationLogRecord.Lsn == lastLogicalSequenceNumber_,
        "{0} operation record lsn:{1} must be equal to last logical lsn {2}",
        TraceId,
        operationLogRecord.Lsn,
        lastLogicalSequenceNumber_);

    transactionMap_->AddOperation(operationLogRecord);

    if (mode_ == Mode::Recovery)
    {
        ASSERT_IFNOT(
            operationLogRecord.IsEnlistedTransaction ||
            operationLogRecord.RecordPosition < recoveredLastCompletedBeginCheckpointRecord_->RecordPosition,
            "{0}: operation log record isenlistedtransaction: {1} record position: {2} recoveredlastcompletedbegincheckpointrecord position: {3}",
            TraceId,
            operationLogRecord.IsEnlistedTransaction,
            operationLogRecord.RecordPosition,
            recoveredLastCompletedBeginCheckpointRecord_->RecordPosition);
    }
    else
    {
        ASSERT_IFNOT(
            operationLogRecord.RecordPosition == Constants::InvalidRecordPosition,
            "{0}: operationLogRecord.RecordPosition {1} is not invalid",
            TraceId,
            operationLogRecord.RecordPosition);
    }

    isRecoverableRecord = operationLogRecord.IsEnlistedTransaction;

    if (operationLogRecord.IsRedoOnly)
    {
        lastRecoveredAtomicRedoOperationLsn_ = operationLogRecord.Lsn;
    }
}

void LogRecordsMap::ProcessLogRecord(
    __in EndTransactionLogRecord & endTransactionLogRecord,
    __out bool & isRecoverableRecord)
{
    lastLogicalSequenceNumber_++;

    ASSERT_IFNOT(
        endTransactionLogRecord.Lsn == lastLogicalSequenceNumber_,
        "{0} endTransactionLogRecord record lsn:{1} must be equal to last logical lsn {2}",
        TraceId,
        endTransactionLogRecord.Lsn,
        lastLogicalSequenceNumber_);

    transactionMap_->CompleteTransaction(endTransactionLogRecord);

    if (mode_ == Mode::Recovery)
    {
        ASSERT_IFNOT(
            endTransactionLogRecord.IsEnlistedTransaction ||
            endTransactionLogRecord.RecordPosition < recoveredLastCompletedBeginCheckpointRecord_->RecordPosition,
            "{0}: endTransactionLogRecord log record isenlistedtransaction: {1} record position: {2} recoveredlastcompletedbegincheckpointrecord position: {3}",
            TraceId,
            endTransactionLogRecord.IsEnlistedTransaction,
            endTransactionLogRecord.RecordPosition,
            recoveredLastCompletedBeginCheckpointRecord_->RecordPosition);
    }
    else
    {
        ASSERT_IFNOT(
            endTransactionLogRecord.RecordPosition == Constants::InvalidRecordPosition,
            "{0}: endTransactionLogRecord.RecordPosition {1} is not invalid",
            TraceId,
            endTransactionLogRecord.RecordPosition);
    }

    isRecoverableRecord = endTransactionLogRecord.IsEnlistedTransaction;
}

void LogRecordsMap::ProcessLogRecord(
    __in BarrierLogRecord & barrierLogRecord,
    __out bool & isRecoverableRecord)
{
    isRecoverableRecord = true;
    
    lastLogicalSequenceNumber_++;

    ASSERT_IFNOT(
        barrierLogRecord.Lsn == lastLogicalSequenceNumber_,
        "{0} barrierLogRecord record lsn:{1} must be equal to last logical lsn {2}",
        TraceId,
        barrierLogRecord.Lsn,
        lastLogicalSequenceNumber_);

    lastStableLsn_ = barrierLogRecord.LastStableLsn;

    transactionMap_->RemoveStableTransactions(lastStableLsn_);
}

void LogRecordsMap::ProcessLogRecord(
    __in BackupLogRecord & backupLogRecord,
    __out bool & isRecoverableRecord)
{
    // For incremental backup chaining, we need to recover the backup log records.
    isRecoverableRecord = true;
    
    lastLogicalSequenceNumber_++;

    ASSERT_IFNOT(
        backupLogRecord.Lsn == lastLogicalSequenceNumber_,
        "{0} backup log record lsn:{1} must be equal to last logical lsn {2}",
        TraceId,
        backupLogRecord.Lsn,
        lastLogicalSequenceNumber_);
}

void LogRecordsMap::ProcessLogRecord(
    __in BeginCheckpointLogRecord & beginCheckpointLogRecord,
    __out bool & isRecoverableRecord)
{
    isRecoverableRecord = true;

    if (mode_ == Mode::Recovery)
    {
        ASSERT_IFNOT(
            beginCheckpointLogRecord.RecordPosition <= recoveredLastCompletedBeginCheckpointRecord_->RecordPosition ||
            lastCompletedEndCheckpointRecord_ != nullptr,
            "{0}: beginCheckpointLogRecord record position: {1} must be <= recoveredlastcompletedbegincheckpointrecord position: {2} or lastCompletedendCheckpointrecord != nullptr",
            TraceId,
            beginCheckpointLogRecord.RecordPosition,
            recoveredLastCompletedBeginCheckpointRecord_->RecordPosition);
    }
    else
    {
        ASSERT_IFNOT(
            beginCheckpointLogRecord.RecordPosition == Constants::InvalidRecordPosition,
            "{0}: beginCheckpointLogRecord.RecordPosition {1} is not invalid",
            TraceId,
            beginCheckpointLogRecord.RecordPosition);
    }

    bool failedBarrierCheck = true;
    beginCheckpointLogRecord.EarliestPendingTransaction = transactionMap_->GetEarliestPendingTransaction(beginCheckpointLogRecord.Lsn, failedBarrierCheck).RawPtr();

    ASSERT_IFNOT(
        !failedBarrierCheck, 
        "{0}: Failed barrier check must be false. LSN: {1} LastStableLsn: {2} EarliestPendingTransactionLsn: {3} HighestBackedUpLsn: {4}", 
        TraceId, 
        beginCheckpointLogRecord.Lsn, 
        beginCheckpointLogRecord.LastStableLsn,
        beginCheckpointLogRecord.EarliestPendingTransactionLsn,
        beginCheckpointLogRecord.HighestBackedUpLsn);

    if (mode_ == Mode::Recovery)
    {
        if (beginCheckpointLogRecord.RecordPosition == recoveredLastCompletedBeginCheckpointRecord_->RecordPosition)
        {
            ASSERT_IFNOT(
                currentLogTailEpoch_ == progressVector_->LastProgressVectorEntry.CurrentEpoch,
                "{0}: currentLogTailEpoch: <{1}:{2}> is not equal to progressvector's last epoch <{3}:{4}>",
                TraceId,
                currentLogTailEpoch_.DataLossVersion,
                currentLogTailEpoch_.ConfigurationVersion,
                progressVector_->LastProgressVectorEntry.CurrentEpoch.DataLossVersion,
                progressVector_->LastProgressVectorEntry.CurrentEpoch.ConfigurationVersion);

            lastInProgressCheckpointRecord_ = &beginCheckpointLogRecord;
        }
    }
    else
    {
        lastInProgressCheckpointRecord_ = &beginCheckpointLogRecord;
    }
}

void LogRecordsMap::ProcessLogRecord(
    __in EndCheckpointLogRecord & endCheckpointLogRecord,
    __out bool & isRecoverableRecord)
{
    isRecoverableRecord = false;

    if (mode_ == Mode::Restore)
    {
        endCheckpointLogRecord.HeadRecord = *firstIndexingLogRecord_;
    }

    lastStableLsn_ = endCheckpointLogRecord.LastStableLsn;
    lastLinkedPhysicalRecord_ = &endCheckpointLogRecord;

    if (mode_ == Mode::Recovery)
    {
        if (endCheckpointLogRecord.RecordPosition == recoveredLastEndCheckpointRecord_->RecordPosition)
        {
            ASSERT_IFNOT(
                lastStableLsn_ >= recoveredLastCompletedBeginCheckpointRecord_->Lsn,
                "{0}: lastStableLsn_ : {1} >= recoveredLastCompletedBeginCheckpointRecord_->Lsn {2}",
                TraceId,
                lastStableLsn_,
                recoveredLastCompletedBeginCheckpointRecord_->Lsn);
        }
    }

    lastCompletedEndCheckpointRecord_ = &endCheckpointLogRecord;

    if (lastInProgressCheckpointRecord_ != nullptr)
    {
        lastInProgressCheckpointRecord_->LastStableLsn = lastStableLsn_;
        lastCompletedEndCheckpointRecord_->LastCompletedBeginCheckpointRecord = *lastInProgressCheckpointRecord_;
        lastInProgressCheckpointRecord_ = nullptr;
    }
}

void LogRecordsMap::ProcessLogRecord(
    __in CompleteCheckPointLogRecord & completeCheckpointLogRecord,
    __out bool & isRecoverableRecord)
{
    isRecoverableRecord = false;

    if (mode_ == Mode::Restore)
    {
        completeCheckpointLogRecord.HeadRecord = *firstIndexingLogRecord_;
    }

    lastLinkedPhysicalRecord_ = &completeCheckpointLogRecord;
}

void LogRecordsMap::ProcessLogRecord(
    __in TruncateHeadLogRecord & truncateHeadLogRecord,
    __out bool & isRecoverableRecord)
{
    isRecoverableRecord = false;
    
    if (mode_ == Mode::Restore)
    {
        truncateHeadLogRecord.HeadRecord = *firstIndexingLogRecord_;
    }

    lastLinkedPhysicalRecord_ = &truncateHeadLogRecord;
    lastPeriodicTruncationTimeTicks_ = truncateHeadLogRecord.PeriodicTruncationTimeStampTicks;
}

void LogRecordsMap::ProcessLogRecord(
    __in IndexingLogRecord & indexingLogRecord,
    __out bool & isRecoverableRecord)
{
    isRecoverableRecord = false;
    
    ASSERT_IFNOT(
        indexingLogRecord.CurrentEpoch == currentLogTailEpoch_,
        "{0}: indexingLogRecord.CurrentEpoch <{1}:{2}> == currentLogTailEpoch_ <{3}:{4}>",
        TraceId,
        indexingLogRecord.CurrentEpoch.DataLossVersion,
        indexingLogRecord.CurrentEpoch.ConfigurationVersion,
        currentLogTailEpoch_.DataLossVersion,
        currentLogTailEpoch_.ConfigurationVersion);
}

void LogRecordsMap::ProcessLogRecord(
    __in InformationLogRecord & informationLogRecord,
    __out bool & isRecoverableRecord)
{
    UNREFERENCED_PARAMETER(informationLogRecord);

    isRecoverableRecord = true;
}

void LogRecordsMap::ProcessLogRecord(
    __in TruncateTailLogRecord & truncateTailLogRecord,
    __out bool & isRecoverableRecord)
{
    isRecoverableRecord = false;
    UNREFERENCED_PARAMETER(truncateTailLogRecord);
}

void LogRecordsMap::ProcessLogRecord(
    __in UpdateEpochLogRecord & updateEpochLogRecord,
    __out bool & isRecoverableRecord)
{
    isRecoverableRecord = true;

    ASSERT_IFNOT(
        updateEpochLogRecord.Lsn == lastLogicalSequenceNumber_,
        "{0} updateEpochLogRecord lsn:{1} must be equal to last logical lsn {2}",
        TraceId,
        updateEpochLogRecord.Lsn,
        lastLogicalSequenceNumber_);

    Epoch const & recordEpoch = updateEpochLogRecord.EpochValue;

    if (currentLogTailEpoch_ < recordEpoch)
    {
        currentLogTailEpoch_ = recordEpoch;
    }

    if (mode_ == Mode::Recovery)
    {
        if (updateEpochLogRecord.RecordPosition > recoveredLastCompletedBeginCheckpointRecord_->RecordPosition)
        {
            ProgressVectorEntry progressVectorEntry(updateEpochLogRecord);

            if (progressVector_->LastProgressVectorEntry.CurrentEpoch < recordEpoch)
            {
                progressVector_->Append(progressVectorEntry);
            }
            else
            {
                bool isInserted = progressVector_->Insert(progressVectorEntry);

                ASSERT_IFNOT(
                    isInserted,
                    "{0}: Could not insert progress vector entry <{1}:{2}>,{3}",
                    TraceId,
                    progressVectorEntry.CurrentEpoch.DataLossVersion,
                    progressVectorEntry.CurrentEpoch.ConfigurationVersion,
                    progressVectorEntry.Lsn);
            }
        }
        else
        {
            ProgressVectorEntry progressVectorEntry = progressVector_->Find(recordEpoch);
            ASSERT_IFNOT(
                progressVectorEntry.Lsn == lastLogicalSequenceNumber_,
                "{0}: progressVectorEntry.Lsn {1} == lastLogicalSequenceNumber_ {2}",
                TraceId,
                progressVectorEntry.Lsn,
                lastLogicalSequenceNumber_);
        }
    }
}

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
using namespace Common;

StringLiteral const TraceComponent("LogTruncationManager");

ULONG const LogTruncationManager::ThrottleAfterPendingCheckpointCount = 4;
ULONG const LogTruncationManager::AbortOldTxFactor = 2;
ULONG const LogTruncationManager::MBtoBytesMultiplier = 1024 * 1024;
ULONG const LogTruncationManager::MinTruncationFactor = 2;
ULONG const LogTruncationManager::NumberOfIndexRecordsPerCheckpoint = 50;
TimeSpan const LogTruncationManager::ConfigUpdateIntervalInSeconds = TimeSpan::FromSeconds(30);

LogTruncationManager::LogTruncationManager(
    __in PartitionedReplicaId const & traceId,
    __in ReplicatedLogManager const & replicatedLogManager,
    __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig)
    : KShared()
    , KObject()
    , PartitionedReplicaTraceComponent(traceId)
    , transactionalReplicatorConfig_(transactionalReplicatorConfig)
    , configUpdateStopWatch_(Stopwatch())
    , replicatedLogManager_(&replicatedLogManager)
    , forceCheckpoint_(false)
    , periodicCheckpointTruncationState_(PeriodicCheckpointTruncationState::NotStarted)
{
    // Load the configuration values
    RefreshConfigurationValues(true);

    ASSERT_IFNOT(minTruncationAmountInBytes_ > 0, "{0} Min truncation amount in bytes cannot be less than 1", TraceId);

    ASSERT_IFNOT(indexIntervalBytes_ != 0, "{0} Index interval in bytes cannot be 0", TraceId);
    ASSERT_IFNOT(checkpointIntervalBytes_ != 0, "{0} Checkpoint interval cannot be 0", TraceId);
    ASSERT_IFNOT(minLogSizeInBytes_ != 0, "{0} Min log size in bytes cannot be 0", TraceId);

    ASSERT_IFNOT(
        truncationThresholdInBytes_ >= minLogSizeInBytes_,
        "{0} truncationThresholdInBytes {1} must be larger than minLogSizeInBytes {2}",
        TraceId,
        truncationThresholdInBytes_,
        minLogSizeInBytes_);

    ASSERT_IFNOT(
        throttleAtLogUsageBytes_ > truncationThresholdInBytes_,
        "{0} throttleAtLogUsageBytes {1} must be larger than truncationThresholdInBytes {2}",
        TraceId,
        throttleAtLogUsageBytes_,
        truncationThresholdInBytes_);

    ASSERT_IFNOT(
        throttleAtLogUsageBytes_ > checkpointIntervalBytes_,
        "{0} throttleAtLogUsageBytes {1} must be larger than checkpointIntervalBytes {2}",
        TraceId,
        throttleAtLogUsageBytes_,
        checkpointIntervalBytes_);

    EventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        wformatString("LogTruncationManager \r\n IndexInterval={0} \r\n CheckpointInterval={1} \r\n MinLogSize={2} \r\n TruncationThreshold={3} \r\n ThrottleAt={4} \r\n MinTruncation={5}",
            indexIntervalBytes_,
            checkpointIntervalBytes_,
            minLogSizeInBytes_,
            truncationThresholdInBytes_,
            throttleAtLogUsageBytes_,
            minTruncationAmountInBytes_),
        reinterpret_cast<uintptr_t>(this));

    configUpdateStopWatch_.Start();
}

LogTruncationManager::~LogTruncationManager()
{
    EventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        L"LogTruncationManager",
        reinterpret_cast<uintptr_t>(this));
}

LogTruncationManager::SPtr LogTruncationManager::Create(
    __in PartitionedReplicaId const & traceId,
    __in ReplicatedLogManager const & replicatedLogManager,
    __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in KAllocator & allocator)
{
    LogTruncationManager * pointer = _new(LOGTRUNCATIONMANAGER_TAG, allocator)LogTruncationManager(
        traceId, 
        replicatedLogManager,
        transactionalReplicatorConfig);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return LogTruncationManager::SPtr(pointer);
}

Awaitable<void> LogTruncationManager::BlockSecondaryPumpIfNeeded(__in LONG64 lastStableLsn)
{
    RefreshConfigurationValues();

    // If no checkpoints have happened on the secondary, it implies the copy is in progress and we cannot block as
    // we need to pump more operations to be able to issue the first checkpoint
    if(replicatedLogManager_->LastCompletedBeginCheckpointRecord == nullptr)
    {
        co_return;
    }

    ULONG64 byteUsedFromCurrentLogHead = GetBytesUsed(*replicatedLogManager_->CurrentLogHeadRecord);
    LogRecord::SPtr pendingOperationRecord = replicatedLogManager_->LastInProgressCheckpointRecord.RawPtr();

    if(pendingOperationRecord == nullptr)
    {
        pendingOperationRecord = replicatedLogManager_->LastInProgressTruncateHeadRecord.RawPtr();
    }

    if(pendingOperationRecord == nullptr)
    {
        co_return;
    }

    // If there is pending checkpoint/truncation and the current barrier stable LSN is greater than the checkpoint/truncatehead LSN,
    // it implies that the checkpoint/truncation is in a ready state and it can be applied
    // The fact that it is still pending implies the actual perform checkpoint operation/truncation operation is taking a long time
    // and we hence block here
    // If the stable lsn is smaller, we need to accept more so that more group commits lead to more progress in stable lsn
    // V1 Repl will take care of throttling if we are not reaching stable (Queue full errors)
    if(pendingOperationRecord->Lsn > lastStableLsn)
    {
        co_return;
    }

    if(byteUsedFromCurrentLogHead > throttleAtLogUsageBytes_)
    {
        LONG64 logUsage = replicatedLogManager_->InnerLogManager->Length;
        
        EventSource::Events->DrainBlocked(
            TracePartitionId,
            ReplicaId,
            pendingOperationRecord->RecordType,
            pendingOperationRecord->Lsn,
            byteUsedFromCurrentLogHead,
            logUsage);

        co_await pendingOperationRecord->AwaitProcessing();

        logUsage = replicatedLogManager_->InnerLogManager->Length;

        EventSource::Events->DrainContinue(
            TracePartitionId,
            ReplicaId,
            byteUsedFromCurrentLogHead,
            logUsage);
    }

    co_return;
}

KArray<BeginTransactionOperationLogRecord::SPtr> LogTruncationManager::GetOldTransactions(__in TransactionMap & transactionMap)
{
    RefreshConfigurationValues();
    ULONG64 tail = GetCurrentTailPosition();

    // Allocate an instance of KArray to be returned
    KArray<BeginTransactionOperationLogRecord::SPtr> oldTransactions(GetThisAllocator());
    THROW_ON_CONSTRUCTOR_FAILURE(oldTransactions);

    // The tail is smaller than the truncate interval
    // Return empty list of transactions
    if(tail <= txnAbortThresholdInBytes_)
    {
        return oldTransactions;
    }

    ULONG64 oldTxOffset = GetCurrentTailPosition() - txnAbortThresholdInBytes_ - 1;

    // Get all 'bad' transactions preventing us from checkpointing enough state
    transactionMap.GetPendingTransactionsOlderThanPosition(oldTxOffset, oldTransactions);

    return oldTransactions;
}

bool LogTruncationManager::IsGoodLogHeadCandidate(__in IndexingLogRecord & proposedLogHead)
{
    RefreshConfigurationValues();

    if (periodicCheckpointTruncationState_ == PeriodicCheckpointTruncationState::TruncationStarted)
    {
        // Configured truncation has been initiated, override minTruncationAmount and minLogSize requirements
        return true;
    }

    // This is a very recent indexing record and not a good candidate
    // We can say that because it has not yet been flushed to the disk
    if (proposedLogHead.RecordPosition == Constants::InvalidRecordPosition)
    {
        return false;
    }

    // Is it worth truncating?
    // We do not want to truncate a couple of bytes since we would have to repeat the process soon.
    // This threshold is overridden when periodic head truncation is enabled via config or explicitly requested by user
    if ((proposedLogHead.RecordPosition - replicatedLogManager_->CurrentLogHeadRecord->RecordPosition < minTruncationAmountInBytes_))
    {
        return false;
    }

    // Would it truncate too much?
    // We do not want to truncate if it would cause the log to shrink below minTruncationAmountInBytes.
    // This is because small logs can cause unnecessary full copies and full backups.
    ULONG64 resultingSize = GetCurrentTailPosition() - proposedLogHead.RecordPosition;
    if (resultingSize < minLogSizeInBytes_)
    {
        return false;
    }

    return true;
}

ReplicatedLogManager::IsGoodLogHeadCandidateCalculator LogTruncationManager::GetGoodLogHeadCandidateCalculator()
{
    return ReplicatedLogManager::IsGoodLogHeadCandidateCalculator(this, &LogTruncationManager::IsGoodLogHeadCandidate);
}

bool LogTruncationManager::ShouldBlockOperationsOnPrimary()
{
    RefreshConfigurationValues();

    // We block incoming operations on the primary if the log usage > throttle limit
    return GetBytesUsed(*replicatedLogManager_->CurrentLogHeadRecord) > throttleAtLogUsageBytes_;
}

bool LogTruncationManager::ShouldCheckpointOnPrimary(
    __in TransactionMap & transactionMap,
    __out KArray<BeginTransactionOperationLogRecord::SPtr> & abortTxList)
{
    return ShouldCheckpoint(transactionMap, abortTxList);
}

bool LogTruncationManager::ShouldCheckpointOnSecondary(
    __in TransactionMap & transactionMap)
{
    // The first checkpoint is always initiated by copy
    if (replicatedLogManager_->LastCompletedBeginCheckpointRecord == nullptr)
    {
        return false;
    }

    KArray<BeginTransactionOperationLogRecord::SPtr> tempList(GetThisAllocator());
    THROW_ON_CONSTRUCTOR_FAILURE(tempList);
    return ShouldCheckpoint(transactionMap, tempList);
}

bool LogTruncationManager::ShouldIndex()
{
    RefreshConfigurationValues();

    IndexingLogRecord::SPtr lastIndexRecord = replicatedLogManager_->LastIndexRecord;

    // If there was never an indexing record, return true
    if(lastIndexRecord == nullptr)
    {
        return true;
    }

    // If there is a pending indexing record that is not yet flushed, do not index again
    if(lastIndexRecord->RecordPosition == Constants::InvalidRecordPosition)
    {
        return false;
    }

    // If the last indexing record position is older than the configured limit, return true
    if(GetBytesUsed(*lastIndexRecord) >= indexIntervalBytes_)
    {
        return true;
    }

    return false;
}

bool LogTruncationManager::ShouldTruncateHead()
{
    RefreshConfigurationValues();

    // If the last in progress truncate head record is non-null, return false
    if(replicatedLogManager_->LastInProgressTruncateHeadRecord != nullptr)
    {
        return false;
    }

    // If no checkpoint has been completed, return false
    if(replicatedLogManager_->LastCompletedEndCheckpointRecord == nullptr)
    {
        return false;
    }

    if (ShouldInitiatePeriodicTruncation())
    {
        return true;
    }

    // If the truncation threshold has not been reached, return false
    ULONG64 bytesUsedFromCurrentLogHead = GetBytesUsed(*replicatedLogManager_->CurrentLogHeadRecord);
    if(bytesUsedFromCurrentLogHead < truncationThresholdInBytes_)
    {
        return false;
    }

    return true;
}

ULONG64 LogTruncationManager::GetBytesUsed(__in LogRecord const & startingRecord)
{
    ASSERT_IFNOT(&startingRecord != nullptr, "{0} GetBytesUsedFromRecord: Record Cannot be null", TraceId);
    ASSERT_IF(
        startingRecord.RecordPosition == Constants::InvalidRecordPosition,
        "{0} GetBytesUsedFromRecord: Record Cannot have invalid position", TraceId);

    LONG64 bufferedBytes = replicatedLogManager_->InnerLogManager->BufferedBytes;
    LONG64 pendingFlushBytes = replicatedLogManager_->InnerLogManager->PendingFlushBytes;
    ULONG64 tail = replicatedLogManager_->InnerLogManager->CurrentLogTailPosition;
    ULONG tailRecordSize = replicatedLogManager_->InnerLogManager->CurrentLogTailRecord->ApproximateSizeOnDisk;

    ASSERT_IFNOT(
        bufferedBytes >= 0 && pendingFlushBytes >= 0,
        "{0} BufferedBytes = {1} and PendingFlushBytes = {2} cannot be negative",
        TraceId,
        bufferedBytes,
        pendingFlushBytes);

    ULONG64 bytesUsed = 0;

    // Every log record is assigned its record position before flushing and before updating the tail and other statistics
    // As a result, this value is an approximate and can sometimes end up being negative.

    // To prevent from overflow errors, we only return a positive value of the bytes used or else just return 0 to indicate that the
    // tail is not very far away from the 'startingRecord'
    if (tail + (ULONG64)tailRecordSize + (ULONG64)bufferedBytes + (ULONG64)pendingFlushBytes > startingRecord.RecordSize)
    {
        bytesUsed = tail + (ULONG64)tailRecordSize + (ULONG64)bufferedBytes + (ULONG64)pendingFlushBytes - startingRecord.RecordPosition;
    }

    return bytesUsed;
}

ULONG64 LogTruncationManager::GetCurrentTailPosition()
{
    LONG64 bufferedBytes = replicatedLogManager_->InnerLogManager->BufferedBytes;
    LONG64 pendingFlushBytes = replicatedLogManager_->InnerLogManager->PendingFlushBytes;
    LogRecord::SPtr logTail = replicatedLogManager_->InnerLogManager->CurrentLogTailRecord;

    ASSERT_IFNOT(
        bufferedBytes >= 0 && pendingFlushBytes >= 0,
        "{0} BufferedBytes = {1} and PendingFlushBytes = {2} cannot be negative",
        TraceId,
        bufferedBytes,
        pendingFlushBytes);

    return logTail->RecordPosition + logTail->RecordSize + (ULONG64)bufferedBytes + (ULONG64)pendingFlushBytes;
}

ULONG64 LogTruncationManager::GetMinTruncationAmountInBytes(__in ULONG64 checkpointThresholdInBytes)
{
    return checkpointThresholdInBytes / MinTruncationFactor;
}

ULONG64 LogTruncationManager::GetThrottleThresholdInBytes(
    __in int64 throttleThresholdFactor,
    __in ULONG64 checkpointThresholdInBytes,
    __in ULONG64 minLogSizeInBytes)
{
    ULONG64 throttleThresholdInMBFromCheckpointInMB = checkpointThresholdInBytes * throttleThresholdFactor;
    ULONG64 throttleThresholdInMBFromMinLogSizeInMB = minLogSizeInBytes * throttleThresholdFactor;

    return max(throttleThresholdInMBFromCheckpointInMB, throttleThresholdInMBFromMinLogSizeInMB);
}

bool LogTruncationManager::ShouldCheckpoint(
    __in TransactionMap & transactionMap,
    __out KArray<BeginTransactionOperationLogRecord::SPtr> & abortTxList)
{
    RefreshConfigurationValues();

    // There is a checkpoint in progress. So return false
    if(replicatedLogManager_->LastInProgressCheckpointRecord != nullptr)
    {
        return false;
    }

    if (ShouldInitiatePeriodicCheckpoint())
    {
        return true;
    }

    ULONG64 bytesUsedFromLastCheckpoint = GetBytesUsed(*replicatedLogManager_->LastCompletedBeginCheckpointRecord);

    // If there is enough data to checkpoint, we should try to look for 'bad' transactions that are preventing
    // enough data from being checkpointed
    if (bytesUsedFromLastCheckpoint <= checkpointIntervalBytes_ &&
        forceCheckpoint_.load() == false)
    {
        return false;
    }

    BeginTransactionOperationLogRecord::SPtr earliestPendingTx = transactionMap.GetEarliestPendingTransaction();

    // If there is no pending transaction, we should checkpoint now
    if(earliestPendingTx == nullptr)
    {
        return true;
    }

    // The earliest pending tx is very new and it has not yet been flushed. So it is fine to checkpoint
    // This could ALSO imply that we have a really old Tx that has not been flushed and has been lying around in the buffers
    // However, the latter case is taken care of by throttling the incoming operations based on "pending bytes" that are unflushed
    if(earliestPendingTx->RecordPosition == Constants::InvalidRecordPosition)
    {
        return true;
    }

    ULONG64 oldTxOffset = GetCurrentTailPosition() - txnAbortThresholdInBytes_ - 1;

    // The transaction is new enough. We can checkpoint
    if(earliestPendingTx->RecordPosition > oldTxOffset)
    {
        return true;
    }

    // Get all 'bad' transactions that are preventing us from checkpointing enough state
    // We will return 'false' now and the next group commit should successfully checkpoint since the earliest pending should be newer
    transactionMap.GetPendingTransactionsOlderThanPosition(oldTxOffset, abortTxList);
    return false;
}

void LogTruncationManager::RefreshConfigurationValues(bool forceRefresh)
{
    if (configUpdateStopWatch_.Elapsed <= ConfigUpdateIntervalInSeconds && forceRefresh == false)
    {
        // If the minimum duration for the config update has not passed, return
        return;
    }

    checkpointIntervalBytes_ = transactionalReplicatorConfig_->CheckpointThresholdInMB * MBtoBytesMultiplier;
    minLogSizeInBytes_ = transactionalReplicatorConfig_->MinLogSizeInMB * MBtoBytesMultiplier;
    truncationThresholdInBytes_ = transactionalReplicatorConfig_->TruncationThresholdFactor * minLogSizeInBytes_;
    throttleAtLogUsageBytes_ = GetThrottleThresholdInBytes(transactionalReplicatorConfig_->ThrottlingThresholdFactor, checkpointIntervalBytes_, minLogSizeInBytes_);
    minTruncationAmountInBytes_ = GetMinTruncationAmountInBytes(checkpointIntervalBytes_);
    indexIntervalBytes_ = checkpointIntervalBytes_ / NumberOfIndexRecordsPerCheckpoint;
    txnAbortThresholdInBytes_ = checkpointIntervalBytes_ / AbortOldTxFactor;

    // Reset the config update stopwatch
    configUpdateStopWatch_.Restart();
}

void LogTruncationManager::ForceCheckpoint()
{
    forceCheckpoint_.store(true);
}

bool LogTruncationManager::ShouldInitiatePeriodicCheckpoint()
{
    bool periodicCheckpointRequested = periodicCheckpointTruncationState_ == PeriodicCheckpointTruncationState::Ready;
    
    if (periodicCheckpointRequested)
    {
        periodicCheckpointTruncationState_ = PeriodicCheckpointTruncationState::CheckpointStarted;
    }

    return periodicCheckpointRequested;
}

bool LogTruncationManager::ShouldInitiatePeriodicTruncation()
{
    bool periodicCheckpointCompleted = periodicCheckpointTruncationState_ == PeriodicCheckpointTruncationState::CheckpointCompleted;

    if (periodicCheckpointCompleted)
    {
        periodicCheckpointTruncationState_ = PeriodicCheckpointTruncationState::TruncationStarted;
    }
    
    return periodicCheckpointCompleted;
}

void LogTruncationManager::OnCheckpointCompleted(
    __in NTSTATUS status,
    __in Data::LogRecordLib::CheckpointState::Enum checkpointState,
    __in bool isRecoveredCheckpoint)
{
    if (!isRecoveredCheckpoint && periodicCheckpointTruncationState_ != PeriodicCheckpointTruncationState::CheckpointStarted)
    {
        // Checkpoint not initiated by timer config
        // This indicates a regular checkpoint
        return;
    }

    if (!NT_SUCCESS(status) || checkpointState != CheckpointState::Completed)
    {
        // Checkpoint failed to complete successfully, reset periodic process to 'Ready'
        periodicCheckpointTruncationState_ = PeriodicCheckpointTruncationState::Ready;
        return;
    }

    periodicCheckpointTruncationState_ = PeriodicCheckpointTruncationState::CheckpointCompleted;
}

void LogTruncationManager::OnTruncationCompleted()
{
    // Reset the periodic process
    periodicCheckpointTruncationState_ = PeriodicCheckpointTruncationState::NotStarted;
}

void LogTruncationManager::InitiatePeriodicCheckpoint()
{
    if (periodicCheckpointTruncationState_ == PeriodicCheckpointTruncationState::NotStarted)
    {
        periodicCheckpointTruncationState_ = PeriodicCheckpointTruncationState::Ready;
    }
}

/*
    Recovery Scenario 1:
        Replica opens, chkptTime == truncTime == DateTime::Now()   [T1, time1]

        Interval Timer expires

        Checkpoint initiated
            chkptTime updated value persisted                      [T2]
            Checkpoint completes

        Replica closes before truncation started
            chkptTime [T2] persisted in BeginCPLR
            truncTime [T1] not yet persisted (TruncateHeadLogRecord not created)

        Replica re-opens
             chkptTime == truncTime == DateTime::Now() [T3]
        Replica recovers
            chkptTime = [T2]
            truncTime = [T3]

        - T3 < T2, so stage = CheckpointCompleted
            - Happy path would have updated 
        - Next call to 'ShouldTruncateHead' will return true
        - Timer is expected to wait the entire duration


    Recovery Scenario 2:
        Replica opens, chkptTime == truncTime == DateTime::Now()   [T1]
            ZeroBeginCheckpointLogRecord flushed to disk with DateTime::Now()

        Replica closes before interval timer has expired

        Replica re-opens
             chkptTime == truncTime == DateTime::Now()              [T2]

        Replica recovers
            chkptTime = [T1]
            truncTime = 0

        However, the 1st interval checkpoint has NOT ACTUALLY COMPLETED since the timer has not fired
        In this scenario, truncTime should be set to chkptTime and timer started for remaining duration
        
        So, truncation is incomplete IF
            truncTime < chkptTime
*/

void LogTruncationManager::Recover(
    __in LONG64 recoveredCheckpointTime,
    __in LONG64 recoveredTruncationTime)
{
    bool truncationIncomplete = recoveredTruncationTime < recoveredCheckpointTime;
    if (truncationIncomplete)
    {
        OnCheckpointCompleted(
            STATUS_SUCCESS,
            CheckpointState::Completed,
            true);
    }
}
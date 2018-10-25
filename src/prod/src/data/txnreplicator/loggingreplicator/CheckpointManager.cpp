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

CheckpointManager::CheckpointManager(
    __in PartitionedReplicaId const & traceId,
    __in ILogTruncationManager & logtruncationManager,
    __in RecoveredOrCopiedCheckpointState & recoveredOrCopiedCheckpointState,
    __in ReplicatedLogManager & replicatedLogManager,
    __in TransactionMap & transactionMap,
    __in IStateProviderManager & stateManager,
    __in IBackupManager & backupManager,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in TRPerformanceCountersSPtr const & perfCounters,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
    __in InvalidLogRecords & invalidLogRecords)
    : ICheckpointManager()
    , KObject()
    , KShared()
    , KWeakRefType<CheckpointManager>()
    , PartitionedReplicaTraceComponent(traceId)
    , transactionMap_(&transactionMap)
    , logTruncationManager_(&logtruncationManager)
    , replicatedLogManager_(&replicatedLogManager)
    , backupManager_(&backupManager)
    , recoveredOrCopiedCheckpointState_(&recoveredOrCopiedCheckpointState)
    , transactionalReplicatorConfig_(transactionalReplicatorConfig)
    , invalidLogRecords_(&invalidLogRecords)
    , perfCounters_(perfCounters)
    , healthClient_(healthClient)
    , stateManager_()
    , backupAndCopyConsistencyLock_()
    , headTruncationLock_()
    , thisLock_()
    , goodLogHeadCalculator_()
    , stateManagerApiLock_()
    , groupCommitDelay_(0)
    , groupCommitNeeded_(false)
    , groupCommitTask_()
    , lastStableLsn_()
    , lastPeriodicCheckpointTime_(DateTime::Now())
    , lastPeriodicTruncationTime_(DateTime::Now())
{
    EventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        L"CheckpointManager",
        reinterpret_cast<uintptr_t>(this));

    ASSERT_IFNOT(
        invalidLogRecords_ != nullptr,
        "{0}: CheckpointManager: invalidLogRecords_ cannot be null",
        TraceId);

    ASSERT_IFNOT(
        logTruncationManager_ != nullptr,
        "{0}: CheckpointManager: logTruncationManager_ cannot be null",
        TraceId);

    ASSERT_IFNOT(
        recoveredOrCopiedCheckpointState_ != nullptr,
        "{0}: CheckpointManager: recoveredOrCopiedCheckpointState_ cannot be null",
        TraceId);

    ASSERT_IFNOT(
        replicatedLogManager_ != nullptr,
        "{0}: CheckpointManager: replicatedLogManager_ cannot be null",
        TraceId);

    NTSTATUS status = stateManager.GetWeakIfRef(stateManager_);
    THROW_ON_FAILURE(status);

    status = ReaderWriterAsyncLock::Create(
        GetThisAllocator(),
        CHECKPOINTMANAGER_TAG,
        backupAndCopyConsistencyLock_);
    THROW_ON_FAILURE(status);

    status = ReaderWriterAsyncLock::Create(
        GetThisAllocator(),
        CHECKPOINTMANAGER_TAG,
        stateManagerApiLock_);
    THROW_ON_FAILURE(status);

    goodLogHeadCalculator_ = logTruncationManager_->GetGoodLogHeadCandidateCalculator();

    Reuse();
}

CheckpointManager::~CheckpointManager()
{
    EventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        L"CheckpointManager",
        reinterpret_cast<uintptr_t>(this));
}

CheckpointManager::SPtr CheckpointManager::Create(
    __in PartitionedReplicaId const & traceId,
    __in ILogTruncationManager & logtruncationManager,
    __in RecoveredOrCopiedCheckpointState & recoveredOrCopiedCheckpointState,
    __in ReplicatedLogManager & replicatedLogManager,
    __in TransactionMap & transactionMap,
    __in IStateProviderManager & stateManager,
    __in IBackupManager & backupManager,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in InvalidLogRecords & invalidLogRecords,
    __in TRPerformanceCountersSPtr const & perfCounters,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
    __in KAllocator & allocator)
{
    CheckpointManager * pointer = _new(CHECKPOINTMANAGER_TAG, allocator) CheckpointManager(
        traceId,
        logtruncationManager,
        recoveredOrCopiedCheckpointState,
        replicatedLogManager,
        transactionMap,
        stateManager,
        backupManager,
        transactionalReplicatorConfig,
        perfCounters,
        healthClient,
        invalidLogRecords);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return CheckpointManager::SPtr(pointer);
}

void CheckpointManager::AbortPendingCheckpoint()
{
    CheckpointState::Enum checkpointState = CheckpointState::Enum::Invalid;
    BeginCheckpointLogRecord::SPtr checkpointRecord = nullptr;

    // Can race with ApplyCheckpointIfPermitted. after checkpoint state is set to either aborted or applied, we are guaranteed single threaded access. so no synchronisation is needed
    K_LOCK_BLOCK(thisLock_)
    {
        checkpointRecord = replicatedLogManager_->LastInProgressCheckpointRecord;

        if (checkpointRecord != nullptr)
        {
            checkpointState = checkpointRecord->CheckpointState;
            if (static_cast<LONG32>(checkpointState) <= static_cast<LONG32>(CheckpointState::Enum::Ready))
            {
                checkpointRecord->CheckpointState = CheckpointState::Enum::Aborted;
            }
        }
    }

    if (checkpointState == CheckpointState::Enum::Ready)
    {
        CheckpointApply(STATUS_SUCCESS);
    }
}

void CheckpointManager::AbortPendingLogHeadTruncation()
{
    TruncationState::Enum truncationState = TruncationState::Enum::Invalid;
    TruncateHeadLogRecord::SPtr truncateHeadRecord = nullptr;

    // can race with ApplyLogHeadTruncationIfPermitted. after truncation state is set to either aborted or applied, we are guaranteed only one thread access so no lock is needed
    K_LOCK_BLOCK(thisLock_)
    {
        truncateHeadRecord = replicatedLogManager_->LastInProgressTruncateHeadRecord;

        if (truncateHeadRecord != nullptr)
        {
            truncationState = truncateHeadRecord->TruncationState;
            if (static_cast<LONG32>(truncationState) <= static_cast<LONG32>(TruncationState::Enum::Ready))
            {
                truncateHeadRecord->TruncationState = TruncationState::Enum::Aborted;
            }
        }
    }

    if (truncationState == CheckpointState::Enum::Ready)
    {
        LogHeadTruncationApply(*truncateHeadRecord, STATUS_SUCCESS);
    }
}

Awaitable<bool> CheckpointManager::AcquireBackupAndCopyConsistencyLockAsync(
    __in KStringView const locker,
    __in KDuration const & timeout,
    __in CancellationToken const & cancellationToken)
{
    Stopwatch stopWatch;
    stopWatch.Start();

    UNREFERENCED_PARAMETER(cancellationToken);

    bool isAcquired = co_await backupAndCopyConsistencyLock_->AcquireWriteLockAsync(static_cast<int>(timeout.Milliseconds()));

    stopWatch.Stop();

    EventSource::Events->CheckpointManagerAcquireLock(
        TracePartitionId,
        ReplicaId,
        L"BAC",
        stopWatch.Elapsed);

    co_return isAcquired;
}

Awaitable<bool> CheckpointManager::AcquireBackupAndCopyConsistencyLockAsync(__in KStringView const locker)
{
    bool acquired = co_await AcquireBackupAndCopyConsistencyLockAsync(
        locker,
        KDuration(MAXLONGLONG),
        CancellationToken());

    co_return acquired;
}

Awaitable<bool> CheckpointManager::AcquireStateManagerApiLockAsync(
    __in KStringView const locker,
    __in KDuration const & timeout,
    __in CancellationToken const & cancellationToken)
{
    Stopwatch stopWatch;
    stopWatch.Start();

    UNREFERENCED_PARAMETER(cancellationToken); 

    bool isAcquired = co_await stateManagerApiLock_->AcquireWriteLockAsync(static_cast<int>(timeout.Milliseconds()));

    stopWatch.Stop();

    EventSource::Events->CheckpointManagerAcquireLock(
        TracePartitionId,
        ReplicaId,
        L"SMApi",
        stopWatch.Elapsed);

    co_return isAcquired;
}

// Cancels the checkpoint by not doing a perform checkpoint log and completes the processing
// It needs to wait for phase1 to finish however
Awaitable<void> CheckpointManager::CancelFirstCheckpointOnIdleDueToIncompleteCopy(
    __in BeginCheckpointLogRecord & checkpointRecord,
    __in LONG64 copiedUptoLsn)
{
    BeginCheckpointLogRecord::SPtr checkpointRecordSPtr = &checkpointRecord;

    ASSERT_IFNOT(
        checkpointRecordSPtr->IsFirstCheckpointOnFullCopy == true,
        "{0}: CancelFirstCheckpointOnIdleDueToIncompleteCopy must only be called on FirstCheckpoint",
        TraceId);

    // the only time this may not happen is when replica is closing prematurely, at which point log is older than copieduptolsn
    ASSERT_IFNOT(
        replicatedLogManager_->CurrentLogTailLsn < copiedUptoLsn,
        "{0}: CancelFirstCheckpointOnIdleDueToIncompleteCopy: Log tail is {1} and copiedUptoLsn is {2}. Former must be lesser than latter",
        TraceId,
        replicatedLogManager_->CurrentLogTailLsn,
        copiedUptoLsn);

    EventSource::Events->Checkpoint(
        TracePartitionId,
        ReplicaId,
        L"CancelFirstCheckpointOnIdleDueToIncompleteCopy",
        checkpointRecordSPtr->CheckpointState,
        checkpointRecordSPtr->Lsn,
        checkpointRecordSPtr->Psn,
        checkpointRecordSPtr->RecordPosition,
        checkpointRecordSPtr->EarliestPendingTransactionLsn);

    // this awaiting is needed to ensure that if a performcheckpointasync call is currently in progress, we dont complete the drain copy process until it completes
    co_await checkpointRecordSPtr->AwaitCompletionPhase1FirstCheckpointOnFullCopy();
    
    // This call is to ensure that we report fault. The replica must already be closing (since that is the only way copy pump gets interrupted and we will be closing)
    ProcessError(
        *checkpointRecordSPtr,
        L"CancelFirstCheckpointOnIdleDueToIncompleteCopy",
        SF_STATUS_INVALID_OPERATION);
        
    InvokePhysicalRecordProcessedCallback(*checkpointRecordSPtr);

    OnCompletePendingCheckpoint(SF_STATUS_INVALID_OPERATION, checkpointRecordSPtr->CheckpointState);
}

// Completes the checkpoint and renames the log
Awaitable<void> CheckpointManager::CompleteFirstCheckpointOnIdleAndRenameLog(
    __in BeginCheckpointLogRecord & checkpointRecord,
    __in LONG64 copiedUptoLsn)
{
    BeginCheckpointLogRecord::SPtr checkpointRecordSPtr = &checkpointRecord;
    NTSTATUS status = STATUS_SUCCESS;

    ASSERT_IFNOT(
        checkpointRecordSPtr->IsFirstCheckpointOnFullCopy == true,
        "{0}: CompleteCheckpointAndRename must only be called on FirstCheckpoint",
        TraceId);

    // we must rename the log at EXACTLY the copiedUptoLsn
    // the only time this may not happen is when replica is closing prematurely, at which point log is older than copieduptolsn
    ASSERT_IFNOT(
        replicatedLogManager_->CurrentLogTailLsn == copiedUptoLsn,
        "{0}: CompletecheckpointAndRename: Log tail is {1} and copiedUptoLsn is {2}. Both must be equal",
        TraceId,
        replicatedLogManager_->CurrentLogTailLsn,
        copiedUptoLsn);

    EventSource::Events->Checkpoint(
        TracePartitionId,
        ReplicaId,
        L"CompleteFirstCheckpointOnIdleAndRenameLog",
        checkpointRecordSPtr->CheckpointState,
        checkpointRecordSPtr->Lsn,
        checkpointRecordSPtr->Psn,
        checkpointRecordSPtr->RecordPosition,
        checkpointRecordSPtr->EarliestPendingTransactionLsn);

    // If successfully copied all records that were to be copied, rename the log here

    // Flush to ensure that if no more records are drained, the checkpoint record gets written to disk
    co_await replicatedLogManager_->InnerLogManager->FlushAsync(L"CompleteFirstCheckpointOnIdleAndRenameLog");

    status = co_await checkpointRecordSPtr->AwaitCompletionPhase1FirstCheckpointOnFullCopy();

    if (!NT_SUCCESS(status))
    {
        ProcessError(
            *checkpointRecordSPtr,
            L"CompleteFirstCheckpointOnIdleAndRenameLog_AwaitCompletionPhase1",
            status);

        InvokePhysicalRecordProcessedCallback(*checkpointRecordSPtr);
        OnCompletePendingCheckpoint(status, checkpointRecordSPtr->CheckpointState);
        co_return;
    }

    status = co_await CompleteCheckpointAndRenameIfNeeded(*checkpointRecordSPtr, true);

    if (NT_SUCCESS(status))
    {
        EventSource::Events->CopyOrBuildReplica(
            TracePartitionId,
            ReplicaId);
    }

    InvokePhysicalRecordProcessedCallback(*checkpointRecordSPtr);
    OnCompletePendingCheckpoint(status, checkpointRecordSPtr->CheckpointState);

    co_return;
}

void CheckpointManager::AppendBarrierOnSecondary(__in BarrierLogRecord & barrierRecord)
{
    BarrierLogRecord::SPtr barrierRecordSPtr = &barrierRecord;

    replicatedLogManager_->AppendBarrier(*barrierRecordSPtr, false);

    ProcessBarrierRecordOnSecondaryAsync(*barrierRecordSPtr);
}

Awaitable<void> CheckpointManager::ApplyCheckpointIfPermittedAsync(
    __in LogRecord & record,
    __in NTSTATUS flushError)
{
    ASSERT_IFNOT(
        replicatedLogManager_->LastInProgressCheckpointRecord.RawPtr() == &record,
        "{0}: Invalid input log record in apply checkpoint",
        TraceId);

    LogRecord::SPtr recordSPtr = &record;
    NTSTATUS status = flushError;

    BeginCheckpointLogRecord::SPtr checkpointRecord = replicatedLogManager_->LastInProgressCheckpointRecord;
    ASSERT_IFNOT(
        checkpointRecord != nullptr, 
        "{0}: Null checkpoint record in apply checkpoint",
        TraceId);

    if (flushError == STATUS_SUCCESS)
    {
        co_await AcquireStateManagerApiLockAsync(L"PrepareCheckpoint");

        IStateProviderManager::SPtr stateManager = stateManager_->TryGetTarget();
        ASSERT_IFNOT(
            stateManager != nullptr, 
            "{0}: Null state manager in apply checkpoint",
            TraceId);

        status = stateManager->PrepareCheckpoint(checkpointRecord->Lsn);

        ReleaseStateManagerApiLock(L"PrepareCheckpoint");
    }

    CheckpointState::Enum checkpointState =
        (status == STATUS_SUCCESS) ?
        CheckpointState::Enum::Ready :
        CheckpointState::Enum::Faulted;

    K_LOCK_BLOCK(thisLock_)
    {
        if (checkpointRecord->Lsn <= lastStableLsn_)
        {
            checkpointRecord->LastStableLsn = lastStableLsn_;

            if (status == STATUS_SUCCESS)
            {
                checkpointState = CheckpointState::Enum::Applied;
            }
        }

        if (checkpointRecord->CheckpointState == CheckpointState::Enum::Invalid)
        {
            checkpointRecord->CheckpointState = checkpointState;
        }
        else
        {
            checkpointState = checkpointRecord->CheckpointState;

            ASSERT_IFNOT(
                checkpointState == CheckpointState::Enum::Aborted,
                "{0}: Checkpoint state should be aborted in apply, state: {1}", 
                TraceId,
                checkpointState);
        }
    }

    EventSource::Events->Checkpoint(
        TracePartitionId,
        ReplicaId,
        L"Prepare",
        checkpointRecord->CheckpointState,
        checkpointRecord->Lsn,
        checkpointRecord->Psn,
        checkpointRecord->RecordPosition,
        checkpointRecord->EarliestPendingTransactionLsn);

    if (checkpointState == CheckpointState::Enum::Applied ||
        checkpointState == CheckpointState::Enum::Faulted ||
        checkpointState == CheckpointState::Enum::Aborted)
    {
        CheckpointApply(status);;
    }

    co_return;
}

void CheckpointManager::ApplyLogHeadTruncationIfPermitted(
    __in LogRecord & record,
    __in NTSTATUS flushError)
{
    ASSERT_IFNOT(
        replicatedLogManager_->LastInProgressTruncateHeadRecord.RawPtr() == &record,
        "{0}: Invalid input log record in apply log head truncation",
        TraceId);

    TruncationState::Enum truncationState =
        (flushError == STATUS_SUCCESS) ?
        TruncationState::Enum::Ready :
        TruncationState::Enum::Faulted;

    TruncateHeadLogRecord::SPtr truncateHeadRecord = nullptr;

    // This protects any racing call to AbortPendingTruncation
    K_LOCK_BLOCK(thisLock_)
    {
        truncateHeadRecord = replicatedLogManager_->LastInProgressTruncateHeadRecord;

        // During idle build, truncatehead record is already stable
        if (truncateHeadRecord->IsStable || truncateHeadRecord->Lsn <= lastStableLsn_)
        {
            if (flushError == STATUS_SUCCESS)
            {
                truncationState = TruncationState::Enum::Applied;
            }
        }

        if (truncateHeadRecord->TruncationState == TruncationState::Enum::Invalid)
        {
            truncateHeadRecord->TruncationState = truncationState;
        }
        else
        {
            truncationState = truncateHeadRecord->TruncationState;

            ASSERT_IFNOT(
                truncationState == TruncationState::Enum::Aborted,
                "{0}: Truncation state should be aborted, state: {1}", 
                TraceId,
                truncationState);
        }
    }

    if (truncationState == TruncationState::Enum::Applied ||
        truncationState == TruncationState::Enum::Faulted ||
        truncationState == TruncationState::Enum::Aborted)
    {
        LogHeadTruncationApply(*truncateHeadRecord, flushError);
    }
}

Awaitable<void> CheckpointManager::BlockSecondaryPumpIfNeeded()
{
    LONG64 lastStableLsn = -1;
    K_LOCK_BLOCK(thisLock_)
    {
        lastStableLsn = lastStableLsn_;
    }

    // 10637219: Secondary pump must only block on pending operations up to the current stable LSN
    co_await logTruncationManager_->BlockSecondaryPumpIfNeeded(lastStableLsn);
    co_return;
}

void CheckpointManager::FirstBeginCheckpointOnIdleSecondary()
{
    BeginCheckpointLogRecord::SPtr record;
    InitiateCheckpoint(
        false,  // isPrimary
        true,   // isFirstCheckpointOnFullCopy
        record);

    ASSERT_IFNOT(
        record != nullptr, 
        "{0}: Null checkpoint log record in begin checkpoint",
        TraceId);

    replicatedLogManager_->InsertBeginCheckpoint(*record);

    // This will ensure begin checkpoint record gets flushed to disk - including the first checkpoint on the idle replica
    // RDBug 9513833 - [Replicator] Fix missed flush while appending begin checkpoint
    Awaitable<void> flushAsync = replicatedLogManager_->InnerLogManager->FlushAsync(L"FirstBeginCheckpointOnIdleSecondary");

    // ToTask is invoked as we do not await on this call
    ToTask(flushAsync);
}

CopyModeFlag::Enum CheckpointManager::GetLogRecordsToCopy(
    __in ProgressVector & targetProgressVector,
    __in Epoch const & targetLogHeadEpoch,
    __in LONG64 targetLogHeadLsn,
    __in LONG64 targetLogTailLsn,
    __in LONG64 lastRecoveredAtomicRedoOperationLsn,
    __in LONG64 targetReplicaId,
    __out LONG64 & sourceStartingLsn,
    __out LONG64 & targetStartingLsn,
    __out IAsyncEnumerator<LogRecord::SPtr>::SPtr & logRecordsToCopy,
    __out BeginCheckpointLogRecord::SPtr & completedBeginCheckpointRecord)
{
    IPhysicalLogReader::SPtr physicalLogReader = nullptr;
    logRecordsToCopy = nullptr;
    completedBeginCheckpointRecord = nullptr;
    LogReaderType::Enum logReaderType = LogReaderType::Enum::Default;
    KString::SPtr readerName;
    CopyModeFlag::Enum copyModeFlag = CopyModeFlag::Invalid;

    K_LOCK_BLOCK(headTruncationLock_)
    {
        IndexingLogRecord::SPtr sourceLogHeadRecord = replicatedLogManager_->CurrentLogHeadRecord;

        CopyContextParameters sourceCopyContextParameters(
            *replicatedLogManager_->ProgressVectorValue,
            sourceLogHeadRecord->CurrentEpoch,
            sourceLogHeadRecord->Lsn,
            replicatedLogManager_->CurrentLogTailLsn);

        CopyContextParameters targetCopyContextParameters(
            targetProgressVector,
            targetLogHeadEpoch,
            targetLogHeadLsn,
            targetLogTailLsn);

        CopyModeResult copyModeResult = ProgressVector::FindCopyMode(
            sourceCopyContextParameters,
            targetCopyContextParameters,
            lastRecoveredAtomicRedoOperationLsn);

        copyModeFlag = static_cast<CopyModeFlag::Enum>(copyModeResult.CopyModeEnum);
        sourceStartingLsn = copyModeResult.SourceStartingLsn;
        targetStartingLsn = copyModeResult.TargetStartingLsn;

        ULONG64 startingRecordPosition = 0;

        if (copyModeResult.CopyModeEnum == CopyModeFlag::Enum::None)
        {
            return CopyModeFlag::Enum::None;
        }

        if (copyModeResult.CopyModeEnum == CopyModeFlag::Enum::Full)
        {
            EventSource::Events->PrimaryFullCopy(
                TracePartitionId,
                ReplicaId,
                copyModeResult.FullCopyReasonEnum,
                targetReplicaId);

            if (copyModeResult.FullCopyReasonEnum == FullCopyReason::Enum::ValidationFailed)
            {
                EventSource::Events->ProgressVectorValidationTelemetry(
                    TracePartitionId,
                    ReplicaId,
                    StringUtility::ToWString(copyModeResult.SharedProgressVector.FailedValidationMsg),
                    sourceCopyContextParameters.ProgressVectorData->ToString(
                        copyModeResult.SharedProgressVector.SourceIndex,
                        Constants::ProgressVectorMaxStringSizeInKb / 2),
                    copyModeResult.SharedProgressVector.SourceIndex,
                    copyModeResult.SharedProgressVector.SourceProgressVectorEntry.ToString(),
                    targetProgressVector.ToString(
                        copyModeResult.SharedProgressVector.TargetIndex,
                        Constants::ProgressVectorMaxStringSizeInKb / 2),
                    copyModeResult.SharedProgressVector.TargetIndex,
                    copyModeResult.SharedProgressVector.TargetProgressVectorEntry.ToString());
            }

            completedBeginCheckpointRecord = replicatedLogManager_->LastCompletedEndCheckpointRecord->LastCompletedBeginCheckpointRecord;
            BeginTransactionOperationLogRecord::SPtr earliestPendingTransaction = completedBeginCheckpointRecord->EarliestPendingTransaction;

            sourceStartingLsn =
                (earliestPendingTransaction != nullptr) ?
                earliestPendingTransaction->Lsn - 1 :
                completedBeginCheckpointRecord->Lsn;

            startingRecordPosition = completedBeginCheckpointRecord->RecordPosition - completedBeginCheckpointRecord->EarliestPendingTransactionOffest;

            logReaderType = LogReaderType::Enum::FullCopy;
        }
        else
        {
            ASSERT_IFNOT(
                (copyModeResult.CopyModeEnum & CopyModeFlag::Enum::Partial) != 0,
                "{0}: Expected partial copy, mode:{1}",
                TraceId,
                copyModeResult.CopyModeEnum);

            LONG64 startingLsn =
                (sourceStartingLsn < targetStartingLsn) ?
                sourceStartingLsn :
                targetStartingLsn;

            PhysicalLogRecord::SPtr startingRecord = replicatedLogManager_->InnerLogManager->CurrentLogTailRecord->PreviousPhysicalRecord;
            do
            {
                PhysicalLogRecord::SPtr previousStartingRecord = startingRecord->PreviousPhysicalRecord;

                if (previousStartingRecord == nullptr || LogRecord::IsInvalid(previousStartingRecord.RawPtr()))
                {
                    break;
                }

                startingRecord = previousStartingRecord;

                // Copied log records should contain every logical record after the
                // record bearing starting LSN. For example, currently UpdateEpoch
                // records are not assigned an LSN and can immediately follow the record
                // bearing starting LSN

                if (startingRecord->Lsn < startingLsn)
                {
                    break;
                }
            } while (true);

            startingRecordPosition = startingRecord->RecordPosition;
            logReaderType = LogReaderType::Enum::PartialCopy;
        }

        NTSTATUS status = KString::Create(readerName, GetThisAllocator(), L"Build for replica: ");
        THROW_ON_FAILURE(status);

        KLocalString<KStringView::Max64BitNumString> replicaId;
        BOOLEAN result = replicaId.FromLONGLONG(targetReplicaId);
        ASSERT_IFNOT(
            result == TRUE,
            "{0}: Failed to convert {1} to KLocalString<KStringView::Max64BitNumString>",
            TraceId,
            targetReplicaId);

        result = readerName->Concat(replicaId);

        ASSERT_IFNOT(
            result == TRUE,
            "{0}: Failed to concatenate replicaId:{1} to readerName:{2} with status {3:x}",
            TraceId,
            targetReplicaId,
            ToStringLiteral(*readerName),
            result);

        physicalLogReader = replicatedLogManager_->InnerLogManager->GetPhysicalLogReader(
            startingRecordPosition,
            replicatedLogManager_->InnerLogManager->CurrentLogTailRecord->RecordPosition,
            replicatedLogManager_->InnerLogManager->CurrentLogTailRecord->Lsn,
            *readerName,
            logReaderType);
    }

    ASSERT_IFNOT(
        physicalLogReader->IsValid, 
        "{0}: Invalid physical log reader during copy",
        TraceId);

    logRecordsToCopy = physicalLogReader->GetLogRecords(
        *PartitionedReplicaIdentifier,
        *readerName,
        logReaderType,
        transactionalReplicatorConfig_->ReadAheadCacheSizeInKb * 1024,
        healthClient_,
        transactionalReplicatorConfig_);

    EventSource::Events->PrimaryCopy(
        TracePartitionId,
        ReplicaId,
        targetReplicaId,
        physicalLogReader->StartingRecordPosition,
        physicalLogReader->EndingRecordPosition);

    physicalLogReader->Dispose();

    return copyModeFlag;
}

void CheckpointManager::InsertPhysicalRecordsIfNecessary()
{
    bool insertedIndex;
    bool insertedTruncateHead;
    InsertPhysicalRecordsIfNecessary(insertedIndex, insertedTruncateHead);
}

void CheckpointManager::InsertPhysicalRecordsIfNecessary(
    __out bool & addedIndex,
    __out bool & addedTruncateHead)
{
    addedIndex = IndexIfNecessary();
    TruncateHeadIfNecessary(addedTruncateHead);
}

void CheckpointManager::InsertPhysicalRecordsIfNecessaryOnSecondary(
    __in LONG64 copiedUptoLsn,
    __in DrainingStream::Enum drainingStream,
    __in LogicalLogRecord & record)
{
    // Do not insert indexing record right next to UpdateEpoch log record
    // Or else, it can lead to situations where the log is truncated to this point and we can end up losing the UpdateEpoch log record
    if (record.RecordType == LogRecordType::Enum::UpdateEpoch)
    {
        return;
    }

    if (replicatedLogManager_->CurrentLogTailLsn < recoveredOrCopiedCheckpointState_->Lsn)
    {
        ASSERT_IFNOT(
            drainingStream == DrainingStream::Enum::Copy || (copiedUptoLsn <= replicatedLogManager_->CurrentLogTailLsn),
            "{0}: Expected copy draining stream type, actual:{1} or copied upto LSN is greater than current log tail LSN: {2} {3}",
            TraceId,
            drainingStream, 
            copiedUptoLsn, 
            replicatedLogManager_->CurrentLogTailLsn);

        IndexIfNecessary();
    }
    else if (replicatedLogManager_->CurrentLogTailLsn == recoveredOrCopiedCheckpointState_->Lsn)
    {
        ASSERT_IFNOT(
            drainingStream == DrainingStream::Enum::Copy || (copiedUptoLsn <= replicatedLogManager_->CurrentLogTailLsn),
            "{0}: Expected copy draining stream type, actual:{1} or copied upto LSN is greater than current log tail LSN: {2} {3}",
            TraceId,
            drainingStream, 
            copiedUptoLsn, 
            replicatedLogManager_->CurrentLogTailLsn);

        if (replicatedLogManager_->CurrentLogTailEpoch < replicatedLogManager_->ProgressVectorValue->LastProgressVectorEntry.CurrentEpoch)
        {
            return;
        }

        if (replicatedLogManager_->LastInProgressCheckpointRecord != nullptr ||
            replicatedLogManager_->LastCompletedEndCheckpointRecord != nullptr)
        {
            ASSERT_IFNOT(
                record.RecordType == LogRecordType::Enum::UpdateEpoch,
                "{0}: Excepted update epoch log record not found, current:{1}",
                TraceId,
                record.RecordType);

            return;
        }

        FirstBeginCheckpointOnIdleSecondary();
        return;
    }

    InsertPhysicalRecordsIfNecessary();
}

void CheckpointManager::ReleaseBackupAndCopyConsistencyLock(__in KStringView const & lockReleaser)
{
    UNREFERENCED_PARAMETER(lockReleaser);
    backupAndCopyConsistencyLock_->ReleaseWriteLock();
}

void CheckpointManager::ReleaseStateManagerApiLock(__in KStringView const & lockReleaser)
{
    UNREFERENCED_PARAMETER(lockReleaser);
    stateManagerApiLock_->ReleaseWriteLock();
}

NTSTATUS CheckpointManager::ReplicateBarrier(__out BarrierLogRecord::SPtr & result)
{
    BarrierLogRecord::SPtr record = nullptr;

    K_LOCK_BLOCK(thisLock_)
    {
        record = BarrierLogRecord::Create(
            lastStableLsn_,
            *invalidLogRecords_->Inv_PhysicalLogRecord,
            GetThisAllocator());
    }

    NTSTATUS status = ReplicateBarrier(*record);

    RETURN_ON_FAILURE(status);

    ScheduleProcessingOnPrimary(*record);
    result = record;
    return status;
}

void CheckpointManager::RequestGroupCommit()
{
    K_LOCK_BLOCK(thisLock_)
    {
        groupCommitNeeded_ = true;
        if (groupCommitTask_ == nullptr)
        {
            ArmGroupCommitCallerHoldsLock();
        }
    }
}

void CheckpointManager::ResetStableLsn(LONG64 lsn)
{
    K_LOCK_BLOCK(thisLock_)
    {
        lastStableLsn_ = lsn;
    }
}

void CheckpointManager::Reuse()
{
    K_LOCK_BLOCK(thisLock_)
    {
        lastStableLsn_ = Constants::ZeroLsn;
        groupCommitDelay_ = 2;
        groupCommitTask_ = nullptr;
        groupCommitNeeded_ = false;
    }
}

Task CheckpointManager::ScheduleProcessingOnPrimary(__in BarrierLogRecord & record)
{
    KCoShared$ApiEntry();
    BarrierLogRecord::SPtr recordSPtr = &record;

    NTSTATUS status1 = co_await recordSPtr->AwaitApply();
    NTSTATUS status2 = co_await replicatedLogManager_->AwaitReplication(*recordSPtr);
    
    NTSTATUS status = FIRST_NTSTATUS_ERROR(status1, status2);

    ProcessBarrierRecord(
        *recordSPtr,
        status,
        true);

    co_return;
}

NTSTATUS CheckpointManager::ErrorIfThrottled(__in LogicalLogRecord const & logicalRecord) noexcept
{
    if (!LogicalLogRecord::IsAllowedToThrottle(logicalRecord))
    {
        return STATUS_SUCCESS;
    }

    bool shouldThrottle = false;

    if (replicatedLogManager_->InnerLogManager->ShouldThrottleWrites)
    {
        shouldThrottle = true;
    }
    
    if (logTruncationManager_->ShouldBlockOperationsOnPrimary())
    {
        if (replicatedLogManager_->LastInProgressCheckpointRecord != nullptr)
        {
            // Implies a pending checkpoint is using log space
            shouldThrottle = true;
        }
        else if (replicatedLogManager_->LastInProgressTruncateHeadRecord != nullptr)
        {
            // Implies a pending truncation is using log space
            shouldThrottle = true;
        }
        else
        {
            KArray<BeginTransactionOperationLogRecord::SPtr> abortTxList = logTruncationManager_->GetOldTransactions(*transactionMap_);

            // Implies no tx's have been committed, leading to this condition where group commits are not happening
            if (abortTxList.Count() > 0)
            {
                AbortOldTransactions(Ktl::Move(abortTxList), replicatedLogManager_->CurrentLogTailLsn);
            }
        }
    }

    if(shouldThrottle)
    {
        perfCounters_->ThrottledOpsPerSecond.Increment();
        return SF_STATUS_SERVICE_TOO_BUSY;
    }

    return STATUS_SUCCESS;
}

void CheckpointManager::AbortOldTransactions(
    __in KArray<BeginTransactionOperationLogRecord::SPtr> && abortTxList,
    __in LONG64 checkpointPreventedAtLsn)
{
    ASSERT_IFNOT(
        abortTxList.Count() > 0, 
        "{0}: Abort xact list is empty",
        TraceId);

    EventSource::Events->AbortOldTransactions(
        TracePartitionId,
        ReplicaId,
        abortTxList.Count(),
        abortTxList[0]->Lsn,
        abortTxList[abortTxList.Count() - 1]->Lsn,
        checkpointPreventedAtLsn);

    AbortOldTransactionsWorkItem::SPtr pointer = _new(CHECKPOINTMANAGER_TAG, GetThisAllocator())AbortOldTransactionsWorkItem(*this, Ktl::Move(abortTxList), checkpointPreventedAtLsn);
    THROW_ON_ALLOCATION_FAILURE(pointer);
    pointer->AddRef(); // Ensure work item does not get destroyed after this thread returns
    // The work item releases itself after execute via KFinally block

    KThreadPool & threadPool = GetThisAllocator().GetKtlSystem().DefaultSystemThreadPool();

    // Add ref on itself to ensure object does not go away while executing the background work item
    threadPool.QueueWorkItem(*pointer);
}

Awaitable<bool> CheckpointManager::AcquireStateManagerApiLockAsync(__in KStringView const locker)
{
    bool isAcquired = co_await AcquireStateManagerApiLockAsync(locker, MAXINT, CancellationToken::None);
    co_return isAcquired;
}

void CheckpointManager::ApplyCheckpointOrLogHeadTruncationIfNecessaryCallerHoldsLock()
{
    BeginCheckpointLogRecord::SPtr inprogressCheckpoint = replicatedLogManager_->LastInProgressCheckpointRecord;
    TruncateHeadLogRecord::SPtr inprogressTruncateHead = replicatedLogManager_->LastInProgressTruncateHeadRecord;

    if (inprogressCheckpoint != nullptr &&
        inprogressCheckpoint->CheckpointState == CheckpointState::Enum::Ready &&
        inprogressCheckpoint->Lsn <= lastStableLsn_)
    {
        inprogressCheckpoint->LastStableLsn = lastStableLsn_;
        inprogressCheckpoint->CheckpointState = CheckpointState::Enum::Applied;
        CheckpointApply(STATUS_SUCCESS);
    }

    if (inprogressTruncateHead != nullptr &&
        inprogressTruncateHead->TruncationState == TruncationState::Enum::Ready &&
        inprogressTruncateHead->Lsn <= lastStableLsn_)
    {
        inprogressTruncateHead->TruncationState = TruncationState::Enum::Applied;
        LogHeadTruncationApply(*inprogressTruncateHead, STATUS_SUCCESS);
    }
}

Task CheckpointManager::ArmGroupCommitCallerHoldsLock()
{
    KCoShared$ApiEntry();

    groupCommitTask_ = replicatedLogManager_->InnerLogManager->GetFlushCompletionTask();
    AwaitableCompletionSource<void>::SPtr localTask = groupCommitTask_;

    co_await localTask->GetAwaitable(); // await on the local task the member can be reset

    // Use thread pool to invoke GroupCommit() method instead of directly calling it because it could end up running synchronously
    // and that causes problems as we try to acquire thisLock_ again on the same thread

    GroupCommitWorkItem::SPtr pointer = _new(CHECKPOINTMANAGER_TAG, GetThisAllocator())GroupCommitWorkItem(*this);
    THROW_ON_ALLOCATION_FAILURE(pointer);

    // Ensure work item does not get destroyed after this thread returns
    // The work item releases itself after execute via KFinally block
    pointer->AddRef();

    KThreadPool & threadPool = GetThisAllocator().GetKtlSystem().DefaultSystemThreadPool();

    // Add ref on itself to ensure object does not go away while executing the background work item
    threadPool.QueueWorkItem(*pointer);

    co_return;
}

Task CheckpointManager::ArmGroupCommitTimerCallerHoldsLock(__in ULONG delayInMilliseconds)
{
    KCoShared$ApiEntry();

    groupCommitTask_ = CompletionTask::CreateAwaitableCompletionSource<void>(CHECKPOINTMANAGER_TAG, GetThisAllocator());
    NTSTATUS status = co_await KTimer::StartTimerAsync(GetThisAllocator(), CHECKPOINTMANAGER_TAG, delayInMilliseconds, nullptr);
    THROW_ON_FAILURE(status);

    // Use thread pool to invoke GroupCommit() method instead of directly calling it because it could end up running synchronously
    // and that causes problems as we try to acquire thisLock_ again on the same thread

    GroupCommitWorkItem::SPtr pointer = _new(CHECKPOINTMANAGER_TAG, GetThisAllocator())GroupCommitWorkItem(*this);
    THROW_ON_ALLOCATION_FAILURE(pointer);
    pointer->AddRef(); // Ensure work item does not get destroyed after this thread returns
    // The work item releases itself after execute via KFinally block

    KThreadPool & threadPool = GetThisAllocator().GetKtlSystem().DefaultSystemThreadPool();

    // Add ref on itself to ensure object does not go away while executing the background work item
    threadPool.QueueWorkItem(*pointer);

    co_return;
}

void CheckpointManager::CheckpointApply(__in NTSTATUS errorCode)
{
    BeginCheckpointLogRecord::SPtr checkpointRecord = replicatedLogManager_->LastInProgressCheckpointRecord;
    ASSERT_IFNOT(
        checkpointRecord != nullptr, 
        "{0}: Null checkpoint record during checkpoint apply",
        TraceId);

    ASSERT_IFNOT(
        checkpointRecord->CheckpointState == CheckpointState::Enum::Applied ||
        checkpointRecord->CheckpointState == CheckpointState::Enum::Faulted ||
        checkpointRecord->CheckpointState == CheckpointState::Enum::Aborted,
        "{0}: Invalid checkpoint state during checkpoint apply: {1}",
        TraceId,
        checkpointRecord->CheckpointState);

    KStringView state =
        errorCode == STATUS_SUCCESS ?
        L"Apply" :
        L"ApplyFailed";

    EventSource::Events->Checkpoint(
        TracePartitionId,
        ReplicaId,
        ToStringLiteral(state),
        checkpointRecord->CheckpointState,
        checkpointRecord->Lsn,
        checkpointRecord->Psn,
        checkpointRecord->RecordPosition,
        checkpointRecord->EarliestPendingTransactionLsn);

    checkpointRecord->CompletedApply(errorCode);
}

BeginCheckpointLogRecord::SPtr CheckpointManager::CheckpointIfNecessary(__in bool isPrimary)
{
    KArray<BeginTransactionOperationLogRecord::SPtr> abortTxList(GetThisAllocator());

    if ((isPrimary && logTruncationManager_->ShouldCheckpointOnPrimary(*transactionMap_, abortTxList)) ||
        logTruncationManager_->ShouldCheckpointOnSecondary(*transactionMap_))
    {
        ASSERT_IFNOT(
            replicatedLogManager_->LastInProgressCheckpointRecord == nullptr,
            "{0}: Unexpected valid last inprogress checkpoint record found",
            TraceId);

        BeginCheckpointLogRecord::SPtr result;
        InitiateCheckpoint(
            isPrimary,
            false, // isFirstCheckpointOnFullCopy
            result);

        return result;
    }

    if (abortTxList.Count() > 0)
    {
        AbortOldTransactions(Ktl::Move(abortTxList), replicatedLogManager_->CurrentLogTailLsn);
    }

    return nullptr;
}

Awaitable<NTSTATUS> CheckpointManager::CompleteStateProviderCheckpointAsync(__in BeginCheckpointLogRecord & record)
{
    NTSTATUS status = STATUS_SUCCESS;
    BeginCheckpointLogRecord::SPtr checkpointRecord = &record;
    IStateProviderManager::SPtr stateManager = stateManager_->TryGetTarget();

    ASSERT_IFNOT(
        stateManager != nullptr,
        "{0}: Null state manager during complete state provider checkpoint",
        TraceId);

    co_await AcquireStateManagerApiLockAsync(L"CompleteCheckpoint");

    status = co_await stateManager->CompleteCheckpointAsync(CancellationToken::None);

    ReleaseStateManagerApiLock(L"CompleteCheckpoint");

    if (!NT_SUCCESS(status))
    {
        ProcessError(
            *checkpointRecord,
            L"CompleteCheckpointAsync",
            status);
    }
    else
    {
        EventSource::Events->Checkpoint(
            TracePartitionId,
            ReplicaId,
            L"Complete",
            checkpointRecord->CheckpointState,
            checkpointRecord->Lsn,
            checkpointRecord->Psn,
            checkpointRecord->RecordPosition,
            checkpointRecord->EarliestPendingTransactionLsn);

        replicatedLogManager_->CompleteCheckpoint();

        co_await replicatedLogManager_->InnerLogManager->FlushAsync(L"CompleteCheckpoint");
    }

    co_return status;
}

Task CheckpointManager::GroupCommit()
{
    KCoShared$ApiEntry();

    BarrierLogRecord::SPtr record = nullptr;

    K_LOCK_BLOCK(thisLock_)
    {
        // Mark that group commit has started by setting the flag to false
        groupCommitNeeded_ = false;
        record = BarrierLogRecord::Create(
            lastStableLsn_,
            *invalidLogRecords_->Inv_PhysicalLogRecord,
            GetThisAllocator());
    }

    NTSTATUS status = ReplicateBarrier(*record);

    K_LOCK_BLOCK(thisLock_)
    {
        groupCommitTask_ = nullptr;

        if (!NT_SUCCESS(status))
        {
            if (replicatedLogManager_->RoleContextDrainStateValue->ReplicaRole == FABRIC_REPLICA_ROLE_PRIMARY &&
                !replicatedLogManager_->RoleContextDrainStateValue->IsClosing)
            {
                groupCommitDelay_ *= 2;
                if (groupCommitDelay_ > 5000)
                {
                    groupCommitDelay_ = 5000;
                }

                ArmGroupCommitTimerCallerHoldsLock(groupCommitDelay_);

                co_return;
            }

            co_return;
        }
    }

    NTSTATUS status1 = co_await record->AwaitApply();
    NTSTATUS status2 = co_await replicatedLogManager_->AwaitReplication(*record);
    
    status = FIRST_NTSTATUS_ERROR(status1, status2);

    K_LOCK_BLOCK(thisLock_)
    {
        // Process barrier record
        groupCommitDelay_ = 2;

        bool isConsecutiveBarrierRecord = false;
        if (status == STATUS_SUCCESS)
        {
            if (lastStableLsn_ < record->Lsn)
            {
                isConsecutiveBarrierRecord = (lastStableLsn_ == (record->Lsn - 1));
                ProcessStableLsnCallerHoldsLock(record->Lsn, *record);
                ApplyCheckpointOrLogHeadTruncationIfNecessaryCallerHoldsLock();
            }
        }

        // If there were any latency sensitive records that require to be applied, issue a group commit
        if (groupCommitTask_ == nullptr && groupCommitNeeded_)
        {
            ArmGroupCommitCallerHoldsLock();
        }
        else if (groupCommitTask_ == nullptr &&
            replicatedLogManager_->RoleContextDrainStateValue->ReplicaRole == FABRIC_REPLICA_ROLE_PRIMARY &&
            !replicatedLogManager_->RoleContextDrainStateValue->IsClosing &&
            !isConsecutiveBarrierRecord)
        {
            // Ensure that idle primary will replicate an additional barrier record
            // to convey that all replicated operations are stable
            ArmGroupCommitCallerHoldsLock();
        }
    }

    if (status != STATUS_SUCCESS)
    {
        LR_TRACE_EXCEPTIONINFORECORD(
            L"GroupCommitWarning",
            (*record),
            status);
    }

    // Ensure this is the last line before returning as it could end up releasing all the objects
    InvokeLogicalRecordProcessedCallback(*record);

    co_return;
}

bool CheckpointManager::IndexIfNecessary()
{
    K_LOCK_BLOCK(thisLock_)
    {
        if (logTruncationManager_->ShouldIndex())
        {
            replicatedLogManager_->Index();
            return true;
        }
    }

    return false;
}

Task CheckpointManager::InitiateCheckpoint(
    __in bool isPrimary,
    __in bool isFirstCheckpointOnFullCopy,
    __out BeginCheckpointLogRecord::SPtr & beginCheckpoint)
{
    KCoShared$ApiEntry();

    bool failedBarrierCheck = false;
    BeginTransactionOperationLogRecord::SPtr earliestpendingTransaction =
        transactionMap_->GetEarliestPendingTransaction(
            replicatedLogManager_->CurrentLogTailLsn,
            failedBarrierCheck);

    ASSERT_IFNOT(
        isPrimary == false || isFirstCheckpointOnFullCopy == false,
        "{0}: isPrimary == false || isFirstCheckpointOnFullCopy == false. IsPrimary is {1}. IsFirstCheckpointOnFullCopy is {2}",
        TraceId,
        isPrimary,
        isFirstCheckpointOnFullCopy);

    if (failedBarrierCheck)
    {
        ASSERT_IFNOT(
            isPrimary, 
            "{0}: Invalid non Primary role during checkpoint initiation",
            TraceId);
        co_return;
    }

    BackupLogRecord::CSPtr lastCompletedBackup = backupManager_->GetLastCompletedBackupLogRecord();

    if (logTruncationManager_->PeriodicCheckpointTruncationState == PeriodicCheckpointTruncationState::CheckpointStarted)
    {
        // Update cached value for last periodic checkpoint time
        lastPeriodicCheckpointTime_ = DateTime::Now();

        EventSource::Events->PeriodicCheckpointAndTruncation(
            TracePartitionId,
            ReplicaId,
            logTruncationManager_->PeriodicCheckpointTruncationState,
            lastPeriodicCheckpointTime_,
            lastPeriodicTruncationTime_);
    }

    BeginCheckpointLogRecord::SPtr checkpointRecord = BeginCheckpointLogRecord::Create(
        isFirstCheckpointOnFullCopy,
        *replicatedLogManager_->ProgressVectorValue,
        earliestpendingTransaction.RawPtr(),
        replicatedLogManager_->CurrentLogHeadRecord->CurrentEpoch,
        replicatedLogManager_->CurrentLogTailEpoch,
        replicatedLogManager_->CurrentLogTailLsn,
        replicatedLogManager_->LastLinkedPhysicalRecord.RawPtr(),
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        *lastCompletedBackup,
        (ULONG)transactionalReplicatorConfig_->ProgressVectorMaxEntries,
        lastPeriodicCheckpointTime_.Ticks,
        lastPeriodicTruncationTime_.Ticks,
        GetThisAllocator());

    beginCheckpoint = checkpointRecord;

    EventSource::Events->Checkpoint(
        TracePartitionId,
        ReplicaId,
        L"Initiate",
        checkpointRecord->CheckpointState,
        checkpointRecord->Lsn,
        checkpointRecord->Psn,
        checkpointRecord->RecordPosition,
        checkpointRecord->EarliestPendingTransactionLsn);

    SharedException::CSPtr processingException;
    NTSTATUS status = co_await checkpointRecord->AwaitApply();
    
    PerformCheckpointAsync(status);

    co_return;
}

void CheckpointManager::LogHeadTruncationApply(
    __in TruncateHeadLogRecord & truncateHeadRecord,
    __in NTSTATUS flushError)
{
    ASSERT_IFNOT(
        truncateHeadRecord.TruncationState == TruncationState::Enum::Applied ||
        truncateHeadRecord.TruncationState == TruncationState::Enum::Faulted ||
        truncateHeadRecord.TruncationState == TruncationState::Enum::Aborted,
        "{0}: Invalid truncate head record state: {1}",
        TraceId,
        truncateHeadRecord.TruncationState);

    EventSource::Events->TruncateHead(
        TracePartitionId,
        ReplicaId,
        L"Apply",
        truncateHeadRecord.TruncationState,
        truncateHeadRecord.Lsn,
        truncateHeadRecord.Psn,
        truncateHeadRecord.RecordPosition,
        truncateHeadRecord.LogHeadLsn,
        truncateHeadRecord.LogHeadPsn,
        truncateHeadRecord.LogHeadRecordPosition);

    truncateHeadRecord.CompletedApply(flushError);
}

Awaitable<void> CheckpointManager::LogHeadTruncationProcessAsync()
{
    TruncateHeadLogRecord::SPtr truncateHeadRecord = replicatedLogManager_->LastInProgressTruncateHeadRecord;
    ASSERT_IFNOT(
        truncateHeadRecord != nullptr, 
        "{0}: Null truncate head record during truncation process",
        TraceId);

    bool isTruncationPerformed = false;

    if (truncateHeadRecord->TruncationState == TruncationState::Enum::Applied)
    {
        co_await replicatedLogManager_->InnerLogManager->ProcessLogHeadTruncationAsync(*truncateHeadRecord)->GetAwaitable();
        truncateHeadRecord->TruncationState = TruncationState::Enum::Completed;
        isTruncationPerformed = true;
    }

    OnCompletePendingLogHeadTruncation(truncateHeadRecord->TruncationState);

    EventSource::Events->TruncateHead(
        TracePartitionId,
        ReplicaId,
        L"LogHeadTruncationProcessAsync",
        truncateHeadRecord->TruncationState,
        truncateHeadRecord->Lsn,
        truncateHeadRecord->Psn,
        truncateHeadRecord->RecordPosition,
        truncateHeadRecord->LogHeadLsn,
        truncateHeadRecord->LogHeadPsn,
        truncateHeadRecord->LogHeadRecordPosition);

    // Ensure this is the last line before returning as it could end up releasing all the objects
    InvokePhysicalRecordProcessedCallback(*truncateHeadRecord);
    co_return;
}

Task CheckpointManager::PerformCheckpointAsync(__in NTSTATUS processingError)
{
    KCoShared$ApiEntry()

    BeginCheckpointLogRecord::SPtr checkpointRecord = replicatedLogManager_->LastInProgressCheckpointRecord;

    if (checkpointRecord->CheckpointState != CheckpointState::Enum::Applied)
    {
        if (processingError != STATUS_SUCCESS)
        {
            ProcessError(
                *checkpointRecord,
                L"PerformCheckpoint",
                processingError);
        }

        if (checkpointRecord->IsFirstCheckpointOnFullCopy)
        {
            checkpointRecord->SignalExceptionForPhase1OfFirstCheckpointOnFullCopy(processingError);

            // Do not complete the pending checkpoint here. The drain code will complete it
            co_return;
        }

        OnCompletePendingCheckpoint(processingError, checkpointRecord->CheckpointState);
        // Ensure this is the last line before returning as it could end up releasing all the objects
        InvokePhysicalRecordProcessedCallback(*checkpointRecord);
        co_return;
    }

    NTSTATUS status = co_await PerformStateProviderCheckpointAsync(*checkpointRecord);

    if (!NT_SUCCESS(status))
    {
        if (checkpointRecord->IsFirstCheckpointOnFullCopy)
        {
            checkpointRecord->SignalExceptionForPhase1OfFirstCheckpointOnFullCopy(status);
        }
        else
        {
            OnCompletePendingCheckpoint(status, checkpointRecord->CheckpointState);
            InvokePhysicalRecordProcessedCallback(*checkpointRecord);
        }

        co_return;
    }

    if (checkpointRecord->IsFirstCheckpointOnFullCopy)
    {
        EventSource::Events->Checkpoint(
            TracePartitionId,
            ReplicaId,
            L"DelayCompleteUntilDrainCopyEnds",
            checkpointRecord->CheckpointState,
            checkpointRecord->Lsn,
            checkpointRecord->Psn,
            checkpointRecord->RecordPosition,
            checkpointRecord->EarliestPendingTransactionLsn);

        checkpointRecord->SignalCompletionPhase1FirstCheckpointOnFullCopy();
        co_return;
    }

    status = co_await CompleteCheckpointAndRenameIfNeeded(*checkpointRecord, false);

    // Nothing can be done if complete checkpoint failed, other than going down
    
    OnCompletePendingCheckpoint(status, checkpointRecord->CheckpointState);
    InvokePhysicalRecordProcessedCallback(*checkpointRecord);

    co_return;
}

Awaitable<NTSTATUS> CheckpointManager::CompleteCheckpointAndRenameIfNeeded(
    __in BeginCheckpointLogRecord & checkpointRecord,
    __in bool renameCopyLog)
{
    BeginCheckpointLogRecord::SPtr checkpointRecordSptr = &checkpointRecord;
    NTSTATUS status = STATUS_SUCCESS;

    co_await AcquireBackupAndCopyConsistencyLockAsync(L"CompleteCheckpointAndRenameIfNeeded");
    co_await replicatedLogManager_->FlushEndCheckpointAsync();

    if (renameCopyLog)
    {
        ASSERT_IFNOT(
            recoveredOrCopiedCheckpointState_->Lsn == checkpointRecordSptr->Lsn,
            "{0}: CompleteCheckpointAndRenameIfNeeded: Renaming copy log must be true only when recoveredorcopiedcheckpointlsn {1} is checkpointrecord.lsn {2}",
            TraceId,
            recoveredOrCopiedCheckpointState_->Lsn,
            checkpointRecordSptr->Lsn);

        // Ensure the copy log is fully flushed to ensure that current tail record
        // does not change underneath the rename.
        co_await replicatedLogManager_->InnerLogManager->FlushAsync(L"CompleteCheckpointAndRenameIfNeeded-RenameCopyLogAtomically");

        status = co_await replicatedLogManager_->InnerLogManager->RenameCopyLogAtomicallyAsync();
    }

    if (!NT_SUCCESS(status))
    {
        ReleaseBackupAndCopyConsistencyLock(L"CompleteCheckpointAndRenameIfNeeded");

        ProcessError(
            *checkpointRecordSptr,
            L"RenameCopyLogAtomicallyAsync",
            status);

        co_return status;
    }

    status = co_await CompleteStateProviderCheckpointAsync(*checkpointRecordSptr);
    ReleaseBackupAndCopyConsistencyLock(L"CompleteCheckpointAndRenameIfNeeded");

    co_return status;
}

Awaitable<NTSTATUS> CheckpointManager::PerformStateProviderCheckpointAsync(__in BeginCheckpointLogRecord & record)
{
    BeginCheckpointLogRecord::SPtr checkpointRecord = &record;
    co_await AcquireStateManagerApiLockAsync(L"PerformCheckpoint");
    NTSTATUS status = STATUS_SUCCESS;

    IStateProviderManager::SPtr stateManager = stateManager_->TryGetTarget();
    ASSERT_IFNOT(
        stateManager != nullptr, 
        "{0}: Null state manager object in perform checkpoint",
        TraceId);

    status = co_await stateManager->PerformCheckpointAsync(CancellationToken::None);
    ReleaseStateManagerApiLock(L"PerformCheckpoint");

    if (!NT_SUCCESS(status))
    {
        checkpointRecord->CheckpointState = CheckpointState::Enum::Faulted; 

        ProcessError(
            *checkpointRecord,
            L"PerformCheckpointAsync",
            status);
    }

    CO_RETURN_ON_FAILURE(status);

    perfCounters_->CheckpointCount.Increment();

    EventSource::Events->Checkpoint(
        TracePartitionId,
        ReplicaId,
        L"Perform",
        checkpointRecord->CheckpointState,
        checkpointRecord->Lsn,
        checkpointRecord->Psn,
        checkpointRecord->RecordPosition,
        checkpointRecord->EarliestPendingTransactionLsn);

    checkpointRecord->CheckpointState = CheckpointState::Enum::Completed;

    co_return status;
}

void CheckpointManager::ProcessBarrierRecord(
    __in BarrierLogRecord & barrierRecord,
    __in NTSTATUS errorCode,
    __in bool isPrimary)
{
    LONG64 newStableLsn =
        isPrimary ?
        barrierRecord.Lsn :
        barrierRecord.LastStableLsn;

    if (errorCode == STATUS_SUCCESS)
    {
        // Optimistic check to ensure the lock is not contended on unnecessarily when there are lots of barriers about to be processed
        if (lastStableLsn_ < newStableLsn)
        {
            K_LOCK_BLOCK(thisLock_)
            {
                if (lastStableLsn_ < newStableLsn)
                {
                    ProcessStableLsnCallerHoldsLock(newStableLsn, barrierRecord);
                    ApplyCheckpointOrLogHeadTruncationIfNecessaryCallerHoldsLock();
                }
            }
        }
    }

    // Ensure this is the last line before returning as it could end up releasing all the objects
    InvokeLogicalRecordProcessedCallback(barrierRecord);
}

Task CheckpointManager::ProcessBarrierRecordOnSecondaryAsync(__in BarrierLogRecord & record)
{
    KCoShared$ApiEntry();

    BarrierLogRecord::SPtr barrierLogRecord = &record;

    NTSTATUS status = co_await barrierLogRecord->AwaitApply();
    ProcessBarrierRecord(*barrierLogRecord, status, false);

    co_return;
}

void CheckpointManager::ProcessError(
    __in LogRecord & record,
    __in KStringView const & messagePrefix,
    __in NTSTATUS errorCode)
{
    LR_TRACE_EXCEPTIONRECORD(
        ToStringLiteral(messagePrefix), 
        record, 
        errorCode);

    replicatedLogManager_->RoleContextDrainStateValue->ReportFault();
}

void CheckpointManager::ProcessStableLsnCallerHoldsLock(
    __in LONG64 stableLsn,
    __in BarrierLogRecord & barrierRecord)
{
    ASSERT_IFNOT(
        lastStableLsn_ < stableLsn,
        "{0}: Input stable LSN is should be greater than current: LastStableLSN: {1} StableLsn: {2}",
        TraceId,
        lastStableLsn_, 
        stableLsn);

    lastStableLsn_ = stableLsn;
    transactionMap_->RemoveStableTransactions(lastStableLsn_);

    EventSource::Events->ProcessBarrierRecord(
        TracePartitionId,
        ReplicaId,
        replicatedLogManager_->RoleContextDrainStateValue->DrainStream,
        barrierRecord.Psn,
        barrierRecord.LastStableLsn,
        lastStableLsn_);
}

NTSTATUS CheckpointManager::ReplicateBarrier(__in BarrierLogRecord & record)
{
    LONG64 bufferedBytes = 0;

    return replicatedLogManager_->ReplicateAndLog(
        record, 
        bufferedBytes, 
        nullptr);
}

Task CheckpointManager::TruncateHeadIfNecessary(__out bool addedTruncateHead)
{
    addedTruncateHead = false;
    KCoShared$ApiEntry();
    TruncateHeadLogRecord::SPtr truncateHeadRecord = nullptr;

    // Ensures only 1 truncate head record is inserted
    K_LOCK_BLOCK(thisLock_)
    {
        // Ensures head truncation does not happen due to log readers because of copy/builds
        K_LOCK_BLOCK(headTruncationLock_)
        {
            if (!logTruncationManager_->ShouldTruncateHead())
            {
                co_return;
            }

            bool isPeriodicTruncation = logTruncationManager_->PeriodicCheckpointTruncationState == PeriodicCheckpointTruncationState::TruncationStarted;

            truncateHeadRecord = replicatedLogManager_->TruncateHead(
                false,                      // isStable
                isPeriodicTruncation ?      // Conditionally update the last periodic truncation time
                    DateTime::Now().Ticks :
                    lastPeriodicTruncationTime_.Ticks, 
                goodLogHeadCalculator_);

            if (truncateHeadRecord != nullptr)
            {
                EventSource::Events->TruncateHead(
                    TracePartitionId,
                    ReplicaId,
                    L"Initialize",
                    truncateHeadRecord->TruncationState,
                    truncateHeadRecord->Lsn,
                    truncateHeadRecord->Psn,
                    truncateHeadRecord->RecordPosition,
                    truncateHeadRecord->LogHeadLsn,
                    truncateHeadRecord->LogHeadPsn,
                    truncateHeadRecord->LogHeadRecordPosition);

                if (isPeriodicTruncation)
                {
                    lastPeriodicTruncationTime_ = DateTime(truncateHeadRecord->PeriodicTruncationTimeStampTicks);
                    EventSource::Events->PeriodicCheckpointAndTruncation(
                        TracePartitionId,
                        ReplicaId,
                        logTruncationManager_->PeriodicCheckpointTruncationState,
                        lastPeriodicCheckpointTime_,
                        lastPeriodicTruncationTime_);
                }

                addedTruncateHead = true;
            }
        }
    }

    if (!addedTruncateHead)
    {
        co_return;
    }

    co_await truncateHeadRecord->AwaitApply();
    co_await LogHeadTruncationProcessAsync();
    co_return;
}

void CheckpointManager::InvokeLogicalRecordProcessedCallback(__in LogicalLogRecord & logicalRecord)
{
    ICompletedRecordsProcessor::SPtr processor = CompletedRecordsProcessor;

    if (processor)
    {
        processor->ProcessedLogicalRecord(logicalRecord);
    }
}

void CheckpointManager::InvokePhysicalRecordProcessedCallback(__in PhysicalLogRecord & physicalRecord)
{
    ICompletedRecordsProcessor::SPtr processor = CompletedRecordsProcessor;

    if (processor)
    {
        processor->ProcessedPhysicalRecord(physicalRecord);
    }
}

CheckpointManager::GroupCommitWorkItem::GroupCommitWorkItem(__in CheckpointManager & parent)
    : parent_(&parent)
{
}

CheckpointManager::GroupCommitWorkItem::~GroupCommitWorkItem()
{
}

void CheckpointManager::GroupCommitWorkItem::Execute()
{
    parent_->GroupCommit();
    this->Release();
}

CheckpointManager::AbortOldTransactionsWorkItem::AbortOldTransactionsWorkItem(
    __in CheckpointManager & parent,
    __in KArray<BeginTransactionOperationLogRecord::SPtr> && abortTxList,
    __in LONG64 checkpointPreventedAtLsn)
    : PartitionedReplicaTraceComponent(parent)
    , abortTxList_(Ktl::Move(abortTxList))
    , checkpointPreventedAtLsn_(checkpointPreventedAtLsn)
{
    THROW_ON_CONSTRUCTOR_FAILURE(abortTxList_);

    ASSERT_IFNOT(
        abortTxList_.Count() > 0, 
        "{0}: Empty abort xact list during aborting old xact work item",
        TraceId);
}

CheckpointManager::AbortOldTransactionsWorkItem::~AbortOldTransactionsWorkItem()
{
}

void CheckpointManager::AbortOldTransactionsWorkItem::Execute()
{
    // Release the reference taken before starting the work item
    KFinally([&] { this->Release(); });

    for (ULONG i = 0; i < abortTxList_.Count(); i++)
    {
        TransactionBase::SPtr badTransactionBase = &abortTxList_[i]->BaseTransaction;
        Transaction::SPtr badTransaction = dynamic_cast<Transaction *>(badTransactionBase.RawPtr());

        ASSERT_IF(
            badTransaction == nullptr, 
            "{0}: We can only abort transactions. Not atomic operations or redo operations. badTransaction is nullptr here",
            TraceId);

        badTransaction->AbortBadTransaction();
    }

    EventSource::Events->AbortOldTransactionsDone(
        TracePartitionId,
        ReplicaId,
        abortTxList_.Count(),
        abortTxList_[0]->Lsn,
        abortTxList_[abortTxList_.Count() - 1]->Lsn,
        checkpointPreventedAtLsn_);
}

void CheckpointManager::OnCompletePendingLogHeadTruncation(__in Data::LogRecordLib::TruncationState::Enum truncationState)
{
    replicatedLogManager_->OnCompletePendingLogHeadTruncation();

    if (truncationState == TruncationState::Completed)
    {
        bool tracePeriodicState = logTruncationManager_->PeriodicCheckpointTruncationState == PeriodicCheckpointTruncationState::TruncationStarted;
        logTruncationManager_->OnTruncationCompleted();

        if (tracePeriodicState)
        {
            EventSource::Events->PeriodicCheckpointAndTruncation(
                TracePartitionId,
                ReplicaId,
                logTruncationManager_->PeriodicCheckpointTruncationState,
                lastPeriodicCheckpointTime_,
                lastPeriodicTruncationTime_);
        }
    }
}

void CheckpointManager::OnCompletePendingCheckpoint(
    __in NTSTATUS status,
    __in Data::LogRecordLib::CheckpointState::Enum checkpointState)
{
    bool tracePeriodicState = logTruncationManager_->PeriodicCheckpointTruncationState == PeriodicCheckpointTruncationState::CheckpointStarted;
 
    logTruncationManager_->OnCheckpointCompleted(
        status,
        checkpointState,
        false);

    if (tracePeriodicState)
    {
        EventSource::Events->PeriodicCheckpointAndTruncation(
            TracePartitionId,
            ReplicaId,
            logTruncationManager_->PeriodicCheckpointTruncationState,
            lastPeriodicCheckpointTime_,
            lastPeriodicTruncationTime_);
    }

    replicatedLogManager_->OnCompletePendingCheckpoint();
}

void CheckpointManager::TimerCallback()
{
    if (!replicatedLogManager_->RoleContextDrainStateValue->IsClosing)
    {
        logTruncationManager_->InitiatePeriodicCheckpoint();

        // Only request group commit on primary replicas
        if (replicatedLogManager_->RoleContextDrainStateValue->ReplicaRole == FABRIC_REPLICA_ROLE_PRIMARY)
        {
            RequestGroupCommit();
        }

        EventSource::Events->PeriodicCheckpointAndTruncation(
            TracePartitionId,
            ReplicaId,
            logTruncationManager_->PeriodicCheckpointTruncationState,
            lastPeriodicCheckpointTime_,
            lastPeriodicTruncationTime_);

        StartPeriodicCheckpointTimer();
    }
}

ULONG CheckpointManager::CalculateTruncationTimerDurationMs(
    __in DateTime lastPeriodicCheckpointTime,
    __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in PeriodicCheckpointTruncationState::Enum const & state)
{
    DateTime currentTime = DateTime::Now();
    TimeSpan elapsedFromCheckpoint = currentTime - lastPeriodicCheckpointTime;
    TimeSpan remainingTime = transactionalReplicatorConfig->TruncationInterval - elapsedFromCheckpoint;

    if (remainingTime.Ticks <= 0)
    {
        // Return 0 if periodic checkpoint/truncation not started and interval has passed, immediately initiating process
        if (state == PeriodicCheckpointTruncationState::NotStarted)
        {
            return 0;
        }

        // If the process has started and interval has passed, fire timer after 1 full interval
        return static_cast<ULONG>(transactionalReplicatorConfig->TruncationInterval.TotalMilliseconds());
    }

    return static_cast<ULONG>(remainingTime.TotalMilliseconds());
}

void CheckpointManager::StartPeriodicCheckpointTimer()
{
    // Calculate appropriate wait duration
    ULONG waitMs = CalculateTruncationTimerDurationMs(
        lastPeriodicCheckpointTime_,
        transactionalReplicatorConfig_,
        logTruncationManager_->PeriodicCheckpointTruncationState);

    // Start timer with adjusted duration
    Common::TimeSpan waitDuration = TimeSpan::FromMilliseconds(waitMs);
    checkpointTimerSPtr_->Change(waitDuration);
}

void CheckpointManager::CancelPeriodicCheckpointTimer()
{
    // Ensure no further timer events are fired
    if (checkpointTimerSPtr_ != nullptr)
    {
        checkpointTimerSPtr_->Cancel();
    }
}

void CheckpointManager::Recover(
    __in Data::LogRecordLib::BeginCheckpointLogRecord::SPtr & lastCompletedBeginCheckpointRecord,
    __in LONG64 recoveredTruncationTime)
{
    LONG64 recoveredCheckpointTime = lastCompletedBeginCheckpointRecord->PeriodicCheckpointTimeTicks;
    lastPeriodicCheckpointTime_ = DateTime(recoveredCheckpointTime);

    if (lastCompletedBeginCheckpointRecord->PeriodicTruncationTimeStampTicks > recoveredTruncationTime)
    {
        recoveredTruncationTime = lastCompletedBeginCheckpointRecord->PeriodicTruncationTimeStampTicks;
    }

    lastPeriodicTruncationTime_ = DateTime(recoveredTruncationTime);

    logTruncationManager_->Recover(
        recoveredCheckpointTime,
        recoveredTruncationTime);

    // Disable
    if (transactionalReplicatorConfig_->TruncationInterval.TotalMilliseconds() == 0)
    {
        return;
    }

    CheckpointManager::SPtr thisSPtr(this);

    thisSPtr->checkpointTimerSPtr_ = Timer::Create(
        TimerTagDefault,
        [thisSPtr](TimerSPtr const &)
        {
            thisSPtr->TimerCallback();
        },
        false);

    // Start timer
    StartPeriodicCheckpointTimer();
}
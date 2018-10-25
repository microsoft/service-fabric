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

Common::StringLiteral const TraceComponent("RecoveryManager");
ULONG PARALLEL_DISPATCH_BATCHSIZE = 1024;

RecoveryManager::RecoveryManager(
    __in PartitionedReplicaId const & traceId,
    __in LogManager & logManager,
    __in PhysicalLogWriterCallbackManager & callbackManager,
    __in InvalidLogRecords & invalidLogRecords)
    : KObject()
    , KShared()
    , PartitionedReplicaTraceComponent(traceId)
    , logManager_(&logManager)
    , invalidLogRecords_(&invalidLogRecords)
    , recoveryError_(STATUS_SUCCESS)
    , logCompleteCheckpointAfterRecovery_(false)
    , callbackManager_(&callbackManager)
    , recoveredLsn_(Constants::ZeroLsn)
    , isRemovingStateAfterOpen_(false)
    , lastRecoveredAtomicRedoOperationLsn_(Constants::ZeroLsn)
    , tailRecordAtStart_(nullptr)
    , recoveredLastEndCheckpointRecord_(nullptr)
    , recoveryLogReader_(nullptr)
{
    EventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        L"RecoveryManager",
        reinterpret_cast<uintptr_t>(this));

    Reuse();
}

RecoveryManager::~RecoveryManager()
{
    EventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        L"RecoveryManager",
        reinterpret_cast<uintptr_t>(this));
}

RecoveryManager::SPtr RecoveryManager::Create(
    __in PartitionedReplicaId const & traceId,
    __in LogManager & logManager,
    __in PhysicalLogWriterCallbackManager & callbackManager,
    __in InvalidLogRecords & invalidLogRecords,
    __in KAllocator & allocator)
{
    RecoveryManager * pointer = _new(RECOVERYMANAGER_TAG, allocator) RecoveryManager(
        traceId,
        logManager,
        callbackManager,
        invalidLogRecords);

    THROW_ON_ALLOCATION_FAILURE(pointer);

    return RecoveryManager::SPtr(pointer);
}

Awaitable<RecoveryInformation> RecoveryManager::OpenAsync(
    __in bool shouldLocalStateBeRemoved,
    __in RecoveryPhysicalLogReader & recoveryReader,
    __in bool isRestoring)
{
    recoveryLogReader_ = &recoveryReader;

    PhysicalLogRecord::SPtr recoveredLinkedPhysicalRecord = nullptr;
    TruncateHeadLogRecord::SPtr recoveredLastTruncateHeadLogRecord = nullptr;
    CompleteCheckPointLogRecord::SPtr recoveredCompleteCheckpointRecord = nullptr;

    logCompleteCheckpointAfterRecovery_ = true;

    tailRecordAtStart_ = co_await recoveryLogReader_->SeekToLastRecord();

    EventSource::Events->RMOpenAsyncLogUsage(
        TracePartitionId,
        ReplicaId,
        logManager_->Length,
        tailRecordAtStart_->RecordType,
        tailRecordAtStart_->Lsn,
        tailRecordAtStart_->Psn,
        tailRecordAtStart_->RecordPosition);

    PhysicalLogRecord::SPtr lastPhysicalRecord = nullptr;

    if (LogRecord::IsPhysical(*tailRecordAtStart_))
    {
        lastPhysicalRecord = dynamic_cast<PhysicalLogRecord *>(tailRecordAtStart_.RawPtr());
        ASSERT_IF(lastPhysicalRecord == nullptr, "Unexpected dynamic cast failure");
    }
    else
    {
        lastPhysicalRecord = co_await recoveryLogReader_->GetPreviousPhysicalRecord(*tailRecordAtStart_);
    }

    EventSource::Events->RMOpenAsyncLastPhysicalRecord(
        TracePartitionId,
        ReplicaId,
        lastPhysicalRecord->RecordType,
        lastPhysicalRecord->Lsn,
        lastPhysicalRecord->Psn,
        lastPhysicalRecord->RecordPosition);

    if (lastPhysicalRecord->RecordType == LogRecordType::Enum::Information)
    {
        InformationLogRecord::SPtr infoRecord = dynamic_cast<InformationLogRecord *>(lastPhysicalRecord.RawPtr());
        ASSERT_IF(infoRecord == nullptr, "Unexpected dynamic cast failure");

        if (infoRecord->InformationEvent == InformationEvent::Enum::RemovingState)
        {
            ASSERT_IFNOT(
                lastPhysicalRecord == tailRecordAtStart_,
                "{0}: Last physical record {1} should be same as tail record at start {2}",
                TraceId,
                lastPhysicalRecord->Psn,
                tailRecordAtStart_->Psn);

            EventSource::Events->RMOpenAsyncSkippingRecovery(
                TracePartitionId,
                ReplicaId);

            isRemovingStateAfterOpen_ = true;

            co_return RecoveryInformation(shouldLocalStateBeRemoved);
        }
    }

    if (lastPhysicalRecord->RecordType == LogRecordType::Enum::TruncateHead)
    {
        recoveredLastTruncateHeadLogRecord = dynamic_cast<TruncateHeadLogRecord *>(lastPhysicalRecord.RawPtr());
        ASSERT_IF(recoveredLastTruncateHeadLogRecord == nullptr, "Unexpected dynamic cast failure");
    }
    else if (lastPhysicalRecord->RecordType == LogRecordType::Enum::EndCheckpoint)
    {
        recoveredLastEndCheckpointRecord_ = dynamic_cast<EndCheckpointLogRecord *>(lastPhysicalRecord.RawPtr());
        ASSERT_IF(recoveredLastEndCheckpointRecord_ == nullptr, "Unexpected dynamic cast failure");
    }
    else if (lastPhysicalRecord->RecordType == LogRecordType::Enum::CompleteCheckpoint)
    {
        recoveredCompleteCheckpointRecord = dynamic_cast<CompleteCheckPointLogRecord *>(lastPhysicalRecord.RawPtr());
        ASSERT_IF(recoveredCompleteCheckpointRecord == nullptr, "Unexpected dynamic cast failure");
    }
    else
    {
        recoveredLinkedPhysicalRecord = co_await recoveryLogReader_->GetLinkedPhysicalRecord(*lastPhysicalRecord);

        ASSERT_IFNOT(
            recoveredLinkedPhysicalRecord != nullptr,
            "{0} : Recovered linked physical record should not be null",
            TraceId);

        bool isValidLinkedRecord =
            recoveredLinkedPhysicalRecord->RecordType == LogRecordType::Enum::EndCheckpoint ||
            recoveredLinkedPhysicalRecord->RecordType == LogRecordType::Enum::CompleteCheckpoint ||
            recoveredLinkedPhysicalRecord->RecordType == LogRecordType::Enum::TruncateHead;

        ASSERT_IFNOT(
            isValidLinkedRecord,
            "{0}: Unexpected linked record type during recovery openasync. {1}",
            TraceId,
            recoveredLinkedPhysicalRecord->RecordType);

        EventSource::Events->RMOpenAsyncRecoveredLinkedPhysicalRecord(
            TracePartitionId,
            ReplicaId,
            recoveredLinkedPhysicalRecord->RecordType,
            recoveredLinkedPhysicalRecord->Lsn,
            recoveredLinkedPhysicalRecord->Psn,
            recoveredLinkedPhysicalRecord->RecordPosition);

        if (recoveredLinkedPhysicalRecord->RecordType == LogRecordType::Enum::TruncateHead)
        {
            recoveredLastTruncateHeadLogRecord = dynamic_cast<TruncateHeadLogRecord *>(recoveredLinkedPhysicalRecord.RawPtr());
            ASSERT_IF(recoveredLastTruncateHeadLogRecord == nullptr, "Unexpected dynamic cast failure");
        }
        else if (recoveredLinkedPhysicalRecord->RecordType == LogRecordType::Enum::EndCheckpoint)
        {
            recoveredLastEndCheckpointRecord_ = dynamic_cast<EndCheckpointLogRecord *>(recoveredLinkedPhysicalRecord.RawPtr());
            ASSERT_IF(recoveredLastEndCheckpointRecord_ == nullptr, "Unexpected dynamic cast failure");
        }
        else if (recoveredLinkedPhysicalRecord->RecordType == LogRecordType::Enum::CompleteCheckpoint)
        {
            recoveredCompleteCheckpointRecord = dynamic_cast<CompleteCheckPointLogRecord *>(recoveredLinkedPhysicalRecord.RawPtr());
            ASSERT_IF(recoveredCompleteCheckpointRecord == nullptr, "Unexpected dynamic cast failure");
        }
    }

    ULONG64 logHeadPosition = 0;
    LONG64 logHeadLsn = 0;

    if (recoveredLastTruncateHeadLogRecord != nullptr)
    {
        logHeadPosition = recoveredLastTruncateHeadLogRecord->LogHeadRecordPosition;
        logHeadLsn = recoveredLastTruncateHeadLogRecord->LogHeadLsn;

        EventSource::Events->RMOpenAsyncRecoveredTruncateHeadRecord(
            TracePartitionId,
            ReplicaId,
            logHeadLsn,
            logHeadPosition);

        recoveredLinkedPhysicalRecord = recoveredLastTruncateHeadLogRecord.RawPtr();

        do
        {
            recoveredLinkedPhysicalRecord = co_await recoveryLogReader_->GetLinkedPhysicalRecord(*recoveredLinkedPhysicalRecord);
            
            ASSERT_IFNOT(
                recoveredLinkedPhysicalRecord != nullptr,
                "{0} : Recovered linked physical record should not be null",
                TraceId);

            bool isValidLinkedRecord =
                recoveredLinkedPhysicalRecord->RecordType == LogRecordType::Enum::EndCheckpoint ||
                recoveredLinkedPhysicalRecord->RecordType == LogRecordType::Enum::CompleteCheckpoint ||
                recoveredLinkedPhysicalRecord->RecordType == LogRecordType::Enum::TruncateHead;

            ASSERT_IFNOT(
                isValidLinkedRecord,
                "{0}: Unexpected linked record type during recovery openasync. {1}",
                TraceId,
                recoveredLinkedPhysicalRecord->RecordType);

            if (recoveredLinkedPhysicalRecord->RecordType == LogRecordType::Enum::EndCheckpoint)
            {
                recoveredLastEndCheckpointRecord_ = dynamic_cast<EndCheckpointLogRecord *>(recoveredLinkedPhysicalRecord.RawPtr());
                ASSERT_IF(recoveredLastEndCheckpointRecord_ == nullptr, "Unexpected dynamic cast failure");
            }
            else if (recoveredLinkedPhysicalRecord->RecordType == LogRecordType::Enum::CompleteCheckpoint)
            {
                recoveredCompleteCheckpointRecord = dynamic_cast<CompleteCheckPointLogRecord *>(recoveredLinkedPhysicalRecord.RawPtr());
                ASSERT_IF(recoveredCompleteCheckpointRecord == nullptr, "Unexpected dynamic cast failure");
            }

        } while (recoveredLastEndCheckpointRecord_ == nullptr);
    }
    else if (recoveredCompleteCheckpointRecord != nullptr)
    {
        logHeadPosition = recoveredCompleteCheckpointRecord->LogHeadRecordPosition;
        logHeadLsn = recoveredCompleteCheckpointRecord->LogHeadLsn;

        EventSource::Events->RMOpenAsyncRecoveredLastCCRecord(
            TracePartitionId,
            ReplicaId,
            logHeadLsn,
            logHeadPosition);

        recoveredLinkedPhysicalRecord = recoveredCompleteCheckpointRecord.RawPtr();

        do
        {
            recoveredLinkedPhysicalRecord = co_await recoveryLogReader_->GetLinkedPhysicalRecord(*recoveredLinkedPhysicalRecord);
            
            ASSERT_IFNOT(
                recoveredLinkedPhysicalRecord != nullptr,
                "{0} : Recovered linked physical record should not be null",
                TraceId);

            bool isValidLinkedRecord =
                recoveredLinkedPhysicalRecord->RecordType == LogRecordType::Enum::EndCheckpoint ||
                recoveredLinkedPhysicalRecord->RecordType == LogRecordType::Enum::CompleteCheckpoint ||
                recoveredLinkedPhysicalRecord->RecordType == LogRecordType::Enum::TruncateHead;

            ASSERT_IFNOT(
                isValidLinkedRecord,
                "{0}: Unexpected linked record type during recovery openasync. {1}",
                TraceId,
                recoveredLinkedPhysicalRecord->RecordType);

            if (recoveredLinkedPhysicalRecord->RecordType == LogRecordType::Enum::EndCheckpoint)
            {
                recoveredLastEndCheckpointRecord_ = dynamic_cast<EndCheckpointLogRecord *>(recoveredLinkedPhysicalRecord.RawPtr());
                ASSERT_IF(recoveredLastEndCheckpointRecord_ == nullptr, "Unexpected dynamic cast failure");
            }

        } while (recoveredLastEndCheckpointRecord_ == nullptr);
    }
    else
    {
        logHeadPosition = recoveredLastEndCheckpointRecord_->LogHeadRecordPosition;
        logHeadLsn = recoveredLastEndCheckpointRecord_->LogHeadLsn;
    }

    EventSource::Events->RMOpenAsyncRecoveredLastECRecord(
        TracePartitionId,
        ReplicaId,
        recoveredLastEndCheckpointRecord_->Lsn,
        recoveredLastEndCheckpointRecord_->Psn,
        recoveredLastEndCheckpointRecord_->RecordPosition,
        recoveredLastEndCheckpointRecord_->LogHeadLsn,
        recoveredLastEndCheckpointRecord_->LogHeadRecordPosition);

    if (recoveredCompleteCheckpointRecord != nullptr)
    {
        logCompleteCheckpointAfterRecovery_ = false;

        EventSource::Events->RMOpenAsyncRecoveredCCRecordInfo(
            TracePartitionId,
            ReplicaId,
            recoveredCompleteCheckpointRecord->Lsn,
            recoveredCompleteCheckpointRecord->Psn,
            recoveredCompleteCheckpointRecord->RecordPosition,
            recoveredCompleteCheckpointRecord->LogHeadLsn,
            recoveredCompleteCheckpointRecord->LogHeadRecordPosition);
    }
    else
    {
        logCompleteCheckpointAfterRecovery_ = true;

        EventSource::Events->RMOpenAsyncCCRecordNotFound(
            TracePartitionId,
            ReplicaId);
    }

    // For restore, log head position must be 0
    if (isRestoring)
    {
        ASSERT_IFNOT(
            logHeadPosition == 0,
            "{0}: Full backup: Loghead Position {1} must be 0",
            TraceId,
            logHeadPosition);
    }

    recoveryLogReader_->MoveStartingRecordPosition(
        logHeadLsn,
        logHeadPosition);

    logManager_->SetLogHeadRecordPosition(logHeadPosition);

    co_return RecoveryInformation(
        logCompleteCheckpointAfterRecovery_,
        shouldLocalStateBeRemoved);
}

Awaitable<NTSTATUS> RecoveryManager::PerformRecoveryAsync(
    __in RecoveredOrCopiedCheckpointState & recoveredOrCopiedCheckpointState,
    __in OperationProcessor & operationProcessor,
    __in CheckpointManager & checkpointManager,
    __in TransactionManager & transactionManager,
    __in LogRecordsDispatcher & logRecordsDispatcher,
    __in ReplicatedLogManager & replicatedLogManager,
    __in LONG64 readAheadSize,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
    __in TRInternalSettingsSPtr const & config)
{
    OperationProcessor::SPtr operationProcessorSPtr = &operationProcessor;
    CheckpointManager::SPtr checkpointManagerSPtr = &checkpointManager;
    TransactionManager::SPtr transactionManagerSPtr = &transactionManager;
    LogRecordsDispatcher::SPtr logRecordsDispatcherSPtr = &logRecordsDispatcher;
    ReplicatedLogManager::SPtr replicatedLogManagerSPtr = &replicatedLogManager;
    NTSTATUS status = STATUS_SUCCESS;

    PhysicalLogRecord::SPtr lastPhysicalRecord = invalidLogRecords_->Inv_PhysicalLogRecord;
    LONG64 lsn = 0;

    KArray<LogRecord::SPtr> recoveredRecords(GetThisAllocator());
    
    CO_RETURN_ON_FAILURE(recoveredRecords.Status());

    BeginCheckpointLogRecord::SPtr recoveredLastCompletedBeginCheckpointRecord = co_await recoveryLogReader_->GetLastCompletedBeginCheckpointRecord(*recoveredLastEndCheckpointRecord_);
    ULONG64 recoveredLastCompletedBeginCheckpointRecordPosition = recoveredLastCompletedBeginCheckpointRecord->RecordPosition;
    ULONG64 recoveryStartingPosition = recoveredLastCompletedBeginCheckpointRecordPosition - recoveredLastCompletedBeginCheckpointRecord->EarliestPendingTransactionOffest;

    // Set the recovered lsn before performing recovery as it impacts unlock callback logic during recovery
    recoveredOrCopiedCheckpointState.Update(recoveredLastCompletedBeginCheckpointRecord->Lsn);

    EventSource::Events->RMPerformRecoveryAsync(
        TracePartitionId,
        ReplicaId,
        recoveredLastCompletedBeginCheckpointRecord->Lsn,
        recoveredLastCompletedBeginCheckpointRecord->Psn,
        recoveredLastCompletedBeginCheckpointRecord->RecordPosition,
        recoveryStartingPosition,
        recoveredLastCompletedBeginCheckpointRecord->EpochValue.DataLossVersion,
        recoveredLastCompletedBeginCheckpointRecord->EpochValue.ConfigurationVersion,
        logCompleteCheckpointAfterRecovery_,
        recoveredLastCompletedBeginCheckpointRecord->CurrentProgressVector->ToString());

    LogRecord::SPtr lastRecoverableRecord = nullptr;

    replicatedLogManagerSPtr->InnerLogManager->SetSequentialAccessReadSize(
        *recoveryLogReader_->ReadStream,
        readAheadSize);

    // Updated to the recovered value in LogRecordsMap during successful recovery
    LONG64 lastPeriodicTruncationTimeTicks = 0;

    {
        LogRecords::SPtr records = LogRecords::Create(
            *PartitionedReplicaIdentifier,
            *recoveryLogReader_->ReadStream,
            *invalidLogRecords_,
            recoveryStartingPosition,
            tailRecordAtStart_->RecordPosition,
            healthClient,
            config,
            GetThisAllocator());

        KFinally([records] { records->Dispose(); });

        bool hasMoved = co_await records->MoveNextAsync(CancellationToken::None);

        ASSERT_IFNOT(
            hasMoved,
            "{0}: Move failed during recovery enumeration",
            TraceId);

        lsn = records->GetCurrent()->Lsn;

        if (recoveredLastCompletedBeginCheckpointRecord->EarliestPendingTransactionOffest != 0)
        {
            lsn = lsn - 1;
        }

        LogRecordsMap::SPtr logRecordsMap = LogRecordsMap::Create(
            *PartitionedReplicaIdentifier,
            lsn,
            *transactionManagerSPtr->TransactionMapObject,
            recoveredLastCompletedBeginCheckpointRecord->EpochValue,
            checkpointManagerSPtr->LastStableLsn,
            *recoveredLastCompletedBeginCheckpointRecord->CurrentProgressVector,
            *recoveredLastCompletedBeginCheckpointRecord,
            *recoveredLastEndCheckpointRecord_,
            *invalidLogRecords_,
            GetThisAllocator());

        do
        {
            bool isRecoverableRecord = true;
            LogRecord::SPtr record = records->GetCurrent();
            record->CompletedFlush(STATUS_SUCCESS);

            ASSERT_IFNOT(
                record->PreviousPhysicalRecord == logRecordsMap->LastPhysicalRecord,
                "{0}: record->PreviousPhysicalRecord == logRecordsMap->LastPhysicalRecord",
                TraceId);

            logRecordsMap->ProcessLogRecord(*record, isRecoverableRecord);

            if (isRecoverableRecord)
            {
                status = recoveredRecords.Append(record);

                ASSERT_IFNOT(
                    NT_SUCCESS(status),
                    "{0}: recoveredRecords.Append failed with {1:x}",
                    TraceId,
                    status);

                lastRecoverableRecord = record;
                if (LogRecord::IsBarrierRecord(*lastRecoverableRecord, false) &&
                    // This condition was needed for LINUX perf as dispatching too often led to slow applies
                    recoveredRecords.Count() >= PARALLEL_DISPATCH_BATCHSIZE) 
                {
                    LoggedRecords::SPtr dispatchRecords = LoggedRecords::Create(
                        recoveredRecords,
                        GetThisAllocator());

                    logRecordsDispatcherSPtr->DispatchLoggedRecords(*dispatchRecords);
                    recoveredRecords.Clear();

                    recoveryError_ = operationProcessorSPtr->ServiceError;

                    if (NT_SUCCESS(recoveryError_))
                    {
                        recoveryError_ = operationProcessorSPtr->LogError;
                    }

                    // If there was an apply or unlock failure, report fault does not help during recovery because the replica is not opened yet.
                    // The only solution here is to fail OpenAsync
                    if (!NT_SUCCESS(recoveryError_))
                    {
                        EventSource::Events->RMRecoveryFailed(
                            TracePartitionId,
                            ReplicaId,
                            operationProcessorSPtr->ServiceError,
                            operationProcessorSPtr->LogError,
                            lastRecoverableRecord->RecordType,
                            lastRecoverableRecord->Lsn,
                            lastRecoverableRecord->Psn);

                        ASSERT_IFNOT(
                            LogRecord::IsBarrierRecord(*lastRecoverableRecord, false),
                            "{0}: Last Recoverable Record must be a barrier. Else recovery will get stuck here",
                            TraceId);

                        co_await lastRecoverableRecord->AwaitProcessing();

                        co_return recoveryError_;
                    }
                }
            }
            else
            {
                // Even when "EnableSecondaryCommitApplyAcknowledgement" is enabled, we use the default IsBarrierLogRecord.
                // Main reason why is that if a commit is not enlisted (e.g. its begin transaction is truncated) it cannot be recovered.
                ASSERT_IFNOT(
                    !LogRecord::IsBarrierRecord(*record, false),
                    "{0}: Barrier record should always be recoverable. Record type is {1}",
                    TraceId,
                    record->RecordType);

                record->CompletedApply(STATUS_SUCCESS);
                record->CompletedProcessing();
            }

            hasMoved = co_await records->MoveNextAsync(CancellationToken::None);
        } 
        while (hasMoved);

        lastRecoveredAtomicRedoOperationLsn_ = logRecordsMap->LastRecoveredAtomicRedoOperationLsn;
        checkpointManagerSPtr->ResetStableLsn(logRecordsMap->LastStableLsn);
        lastPhysicalRecord = logRecordsMap->LastPhysicalRecord;
        lsn = logRecordsMap->LastLogicalSequenceNumber;

        if (recoveredRecords.Count() > 0)
        {
            LoggedRecords::SPtr dispatchRecords = LoggedRecords::Create(
                recoveredRecords,
                GetThisAllocator());

            logRecordsDispatcherSPtr->DispatchLoggedRecords(*dispatchRecords);
        }

        LogRecord::SPtr tailRecord = records->GetCurrent();

        EventSource::Events->RMPerformRecoveryAsyncTailRecordType(
            TracePartitionId,
            ReplicaId,
            tailRecord->RecordType,
            tailRecord->Lsn,
            tailRecord->Psn,
            tailRecord->RecordPosition);

        ASSERT_IFNOT(
            lastPhysicalRecord == records->LastPhysicalRecord,
            "{0}: PerformRecoveryAsync: lastPhysicalRecord == records->LastPhysicalRecord",
            TraceId);
        
        ASSERT_IFNOT(
            (lastPhysicalRecord == tailRecord) || (tailRecord->PreviousPhysicalRecord == lastPhysicalRecord),
            "{0}: PerformRecoveryAsync: (lastPhysicalRecord == tailRecord) || (tailRecord->PreviousPhysicalRecord == lastPhysicalRecord)",
            TraceId);

        ASSERT_IFNOT(
            logRecordsMap->LastCompletedEndCheckpointRecord != nullptr,
            "{0}: PerformRecoveryAsync: logRecordsMap->LastCompletedEndCheckpointRecord != nullptr",
            TraceId);

        ASSERT_IFNOT(
            logRecordsMap->LastInProgressCheckpointRecord == nullptr,
            "{0}: PerformRecoveryAsync: logRecordsMap->LastInProgressCheckpointRecord == nullptr",
            TraceId);

        ASSERT_IFNOT(
            logRecordsMap->LastLinkedPhysicalRecord != nullptr,
            "{0}: PerformRecoveryAsync: logRecordsMap->LastLinkedPhysicalRecord != nullptr",
            TraceId);

        ASSERT_IFNOT(
            lsn == tailRecord->Lsn,
            "{0}: PerformRecoveryAsync: TailRecord Lsn: {1} must be equal to lsn: {2}",
            TraceId,
            tailRecord->Lsn,
            lsn);

        // Disable read ahead as indexing physical records will read the log backwards
        replicatedLogManagerSPtr->InnerLogManager->SetSequentialAccessReadSize(
            *recoveryLogReader_->ReadStream,
            0);

        co_await recoveryLogReader_->IndexPhysicalRecords(lastPhysicalRecord);

        callbackManager_->FlushedPsn = tailRecord->Psn + 1;

        replicatedLogManagerSPtr->InnerLogManager->PrepareToLog(
            *tailRecord,
            *callbackManager_);

        ASSERT_IFNOT(
            recoveryLogReader_->StartingRecord->RecordType == LogRecordType::Enum::Indexing,
            "Expected indexing log record in perform recover, actual: {0}", 
            recoveryLogReader_->StartingRecord->RecordType);

        IndexingLogRecord::SPtr startingRecord = dynamic_cast<IndexingLogRecord *>(recoveryLogReader_->StartingRecord.RawPtr());
        ASSERT_IF(startingRecord == nullptr, "Unexpected dynamic cast failure");

        replicatedLogManagerSPtr->Reuse(
            *recoveredLastCompletedBeginCheckpointRecord->CurrentProgressVector,
            logRecordsMap->LastCompletedEndCheckpointRecord.RawPtr(),
            logRecordsMap->LastLinkedPhysicalRecord.RawPtr(),
            replicatedLogManagerSPtr->LastInformationRecord.RawPtr(),
            *startingRecord,
            logRecordsMap->CurrentLogTailEpoch,
            lsn);

        recoveredLsn_ = lsn;

        if (logCompleteCheckpointAfterRecovery_)
        {
            replicatedLogManagerSPtr->CompleteCheckpoint();
        }

        replicatedLogManagerSPtr->Information(InformationEvent::Enum::Recovered);

        co_await replicatedLogManagerSPtr->InnerLogManager->FlushAsync(L"PerformRecovery");
        co_await lastRecoverableRecord->AwaitProcessing();
        co_await replicatedLogManagerSPtr->LastInformationRecord->AwaitProcessing();
        co_await operationProcessorSPtr->WaitForAllRecordsProcessingAsync();

        lastPeriodicTruncationTimeTicks = logRecordsMap->LastPeriodicTruncationTimeTicks;
    }

    recoveryError_ = operationProcessorSPtr->ServiceError;

    if (NT_SUCCESS(recoveryError_))
    {
        recoveryError_ = operationProcessorSPtr->LogError;
    }

    // If there was an apply or unlock failure, report fault does not help during recovery because the replica is not opened yet.
    // The only solution here is to throw during OpenAsync
    if (!NT_SUCCESS(recoveryError_))
    {
        EventSource::Events->RMRecoveryFailed(
            TracePartitionId,
            ReplicaId,
            operationProcessorSPtr->ServiceError,
            operationProcessorSPtr->LogError,
            lastRecoverableRecord->RecordType,
            lastRecoverableRecord->Lsn,
            lastRecoverableRecord->Psn);

        co_return recoveryError_;
    }

    // Update the last periodic checkpoint timestamp
    // Initiates a periodic timer which is only valid if recovery succeeded
    checkpointManagerSPtr->Recover(
        recoveredLastCompletedBeginCheckpointRecord,
        lastPeriodicTruncationTimeTicks);

    co_return STATUS_SUCCESS;
}

void RecoveryManager::OnTailTruncation(__in LONG64 newTailLsn)
{
    recoveredLsn_ = newTailLsn;
}

void RecoveryManager::Reuse()
{
    logCompleteCheckpointAfterRecovery_ = true;
    isRemovingStateAfterOpen_ = false;
    recoveredLsn_ = Constants::InvalidLsn;
    recoveryError_ = STATUS_SUCCESS;
    recoveredLastEndCheckpointRecord_ = nullptr;
    tailRecordAtStart_ = nullptr;
    lastRecoveredAtomicRedoOperationLsn_ = Constants::ZeroLsn;
}

void RecoveryManager::DisposeRecoveryReadStreamIfNeeded()
{
    RecoveryPhysicalLogReader::SPtr reader = recoveryLogReader_;

    if (reader != nullptr)
    {
        reader->Dispose();
        recoveryLogReader_ = nullptr;
    }
}

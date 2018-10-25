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

Common::StringLiteral const TraceComponent("OperationProcessor");

OperationProcessor::OperationProcessor(
    __in PartitionedReplicaId const & traceId,
    __in RecoveredOrCopiedCheckpointState & recoveredOrCopiedCheckpointState,
    __in RoleContextDrainState & roleContextDrainState,
    __in ILoggingReplicatorToVersionManager & versionManager,
    __in ICheckpointManager & checkpointManager,
    __in IStateProviderManager & stateManager,
    __in IBackupManager & backupManager,
    __in InvalidLogRecords & invalidLogRecords,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in ITransactionalReplicator & transactionalReplicator)
    : IOperationProcessor()
    , KObject()
    , KShared()
    , KWeakRefType<OperationProcessor>()
    , PartitionedReplicaTraceComponent(traceId)
    , invalidLogRecords_(&invalidLogRecords)
    , checkpointManager_(&checkpointManager)
    , versionManager_(&versionManager)
    , roleContextDrainState_(&roleContextDrainState)
    , recoveredOrCopiedCheckpointState_( &recoveredOrCopiedCheckpointState)
    , stateManager_()
    , backupManager_(&backupManager)
    , transactionalReplicatorConfig_(transactionalReplicatorConfig)
    , enableSecondaryCommitApplyAcknowledgement_(transactionalReplicatorConfig->EnableSecondaryCommitApplyAcknowledgement)
    , serviceError_(STATUS_SUCCESS)
    , logError_(STATUS_SUCCESS)
    , lastAppliedBarrierRecord_(nullptr)
    , numberOfLogicalRecordsBeingProcessed_(1)
    , numberOfPhysicalRecordsBeingProcessed_(1)
    , physicalRecordsProcessingTcs_()
    , logicalRecordsProcessingTcs_()
    , changeHandlerCache_(nullptr)
    , transactionalReplicator_(&transactionalReplicator)
{
    EventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        L"OperationProcessor",
        reinterpret_cast<uintptr_t>(this));

    ASSERT_IFNOT(
        checkpointManager_ != nullptr,
        "{0} OperationProcessor::Ctor | Checkpoint manager must not be null",
        TraceId);

    ASSERT_IFNOT(
        versionManager_ != nullptr,
        "{0} OperationProcessor::Ctor | Version manager must not be null",
        TraceId);

    ASSERT_IFNOT(
        roleContextDrainState_ != nullptr,
        "{0} OperationProcessor::Ctor | RoleContextDrainState must not be null",
        TraceId);

    ASSERT_IFNOT(
        recoveredOrCopiedCheckpointState_ != nullptr,
        "{0} OperationProcessor::Ctor | RecoveredOrCopiedCheckpointState must not be null",
        TraceId);

    NTSTATUS status = stateManager.GetWeakIfRef(stateManager_);
    ASSERT_IFNOT(
        status == STATUS_SUCCESS,
        "{0} OperationProcessor::Ctor | Failed to retrieve strong reference to state manager",
        TraceId);

    Reuse();
}

OperationProcessor::~OperationProcessor()
{
    EventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        L"OperationProcessor",
        reinterpret_cast<uintptr_t>(this));
}

OperationProcessor::SPtr OperationProcessor::Create(
    __in PartitionedReplicaId const & traceId,
    __in RecoveredOrCopiedCheckpointState & recoveredOrCopiedCheckpointState,
    __in RoleContextDrainState & roleContextDrainState,
    __in ILoggingReplicatorToVersionManager & versionManager,
    __in ICheckpointManager & checkpointManager,
    __in IStateProviderManager & stateManager,
    __in IBackupManager & backupManager,
    __in InvalidLogRecords & invalidLogRecords,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in ITransactionalReplicator & transactionalReplicator,
    __in KAllocator & allocator)
{
    OperationProcessor * pointer = _new(OPERATIONPROCESSOR_TAG, allocator)OperationProcessor(
        traceId,
        recoveredOrCopiedCheckpointState,
        roleContextDrainState,
        versionManager,
        checkpointManager,
        stateManager,
        backupManager,
        invalidLogRecords,
        transactionalReplicatorConfig,
        transactionalReplicator);

    THROW_ON_ALLOCATION_FAILURE(pointer);

    return OperationProcessor::SPtr(pointer);
}

void OperationProcessor::Reuse()
{
    serviceError_.store(0);
    logError_.store(0);

    LogRecord::SPtr record = BarrierLogRecord::CreateOneBarrierRecord(*invalidLogRecords_->Inv_PhysicalLogRecord, GetThisAllocator()).RawPtr();
    lastAppliedBarrierRecord_.Put(record.RawPtr());

    numberOfLogicalRecordsBeingProcessed_.store(1);
    numberOfPhysicalRecordsBeingProcessed_.store(1);

    physicalRecordsProcessingTcs_ = CompletionTask::CreateAwaitableCompletionSource<PhysicalLogRecord::SPtr>(OPERATIONPROCESSOR_TAG, GetThisAllocator());

    ASSERT_IFNOT(
        physicalRecordsProcessingTcs_ != nullptr,
        "{0} OperationProcessor::Reuse | physicalRecordsProcessingTcs_ must not be null",
        TraceId);

    logicalRecordsProcessingTcs_ = CompletionTask::CreateAwaitableCompletionSource<LogicalLogRecord::SPtr>(OPERATIONPROCESSOR_TAG, GetThisAllocator());

    ASSERT_IFNOT(
        logicalRecordsProcessingTcs_ != nullptr,
        "{0} OperationProcessor::Reuse : logicalRecordsProcessingTcs_ must not be null",
        TraceId);
}

void OperationProcessor::TruncateTail(__in LogRecord & currentRecord)
{
    lastAppliedBarrierRecord_.Put(&currentRecord);
}

RecordProcessingMode::Enum OperationProcessor::IdentifyProcessingModeForRecord(__in LogRecord const & record)
{
    DrainingStream::Enum stream = roleContextDrainState_->DrainStream;

    ASSERT_IFNOT(
        stream != DrainingStream::Enum::Invalid ||
        record.RecordType == LogRecordType::Enum::UpdateEpoch ||
        LogRecord::IsPhysical(record),
        "{0}:Draining stream should not be invalid or record should be of type epoch or physical log record. DrainingStream: {1} RecordType: {2}",
        TraceId,
        stream,
        record.RecordType);

    if (LogRecord::IsBarrierRecord(record, enableSecondaryCommitApplyAcknowledgement_))
    {
        lastAppliedBarrierRecord_.Put(&record);
    }

    RecordProcessingMode::Enum processingMode;

    switch (record.RecordType)
    {
    case LogRecordType::Enum::EndCheckpoint:
    case LogRecordType::Enum::TruncateTail:
    case LogRecordType::Enum::Indexing:
    case LogRecordType::Enum::UpdateEpoch:
    case LogRecordType::Enum::Information:
    case LogRecordType::Enum::CompleteCheckpoint:
        processingMode = RecordProcessingMode::Enum::ProcessImmediately;
        break;

    case LogRecordType::Enum::Barrier:
        processingMode = (roleContextDrainState_->ReplicaRole == FABRIC_REPLICA_ROLE_UNKNOWN)
            ? RecordProcessingMode::Enum::ProcessImmediately
            : RecordProcessingMode::Enum::ApplyImmediately;
        break;

    case LogRecordType::Enum::TruncateHead:
        processingMode = RecordProcessingMode::Enum::ApplyImmediately;
        break;

    case LogRecordType::Enum::BeginCheckpoint:

        ASSERT_IFNOT(
            record.Lsn >= recoveredOrCopiedCheckpointState_->Lsn || 
                roleContextDrainState_->ReplicaRole == FABRIC_REPLICA_ROLE_UNKNOWN,
            "{0}:Record lsn: {1} is less than recovered or copied checkpoint state lsn: {2} or unexpected unknown replica role",
            TraceId,
            record.Lsn,
            recoveredOrCopiedCheckpointState_->Lsn);

        processingMode = (roleContextDrainState_->ReplicaRole == FABRIC_REPLICA_ROLE_UNKNOWN)
            ? RecordProcessingMode::Enum::ProcessImmediately
            : RecordProcessingMode::Enum::ApplyImmediately;
        break;

    case LogRecordType::Enum::BeginTransaction:
    case LogRecordType::Enum::Operation:
    case LogRecordType::Enum::EndTransaction:
    case LogRecordType::Enum::Backup:
        processingMode = RecordProcessingMode::Enum::Normal;
        break;

    default:
        ASSERT_IFNOT(
            false,
            "{0}:IdentifyProcessingModeForRecord : Invalid record type",
            TraceId);

        break;
    }

    return processingMode;
}

Awaitable<void> OperationProcessor::ImmediatelyProcessRecordAsync(
    __in LogRecord & logRecord,
    __in NTSTATUS flushErrorCode,
    __in RecordProcessingMode::Enum processingMode)
{
    LogRecord::SPtr logRecordSPtr = &logRecord;

    ASSERT_IFNOT(
        processingMode > RecordProcessingMode::Enum::Normal,
        "{0}:ImmediatelyProcessRecordAsync | Processing mode is unexpectedly normal",
        TraceId);

    ASSERT_IFNOT(
        logRecordSPtr->RecordType != LogRecordType::Enum::Backup,
        "{0}:ImmediatelyProcessRecordAsync | Invalid backup record type found",
        TraceId);

    NTSTATUS errorCode = STATUS_SUCCESS;

    if (!NT_SUCCESS(flushErrorCode))
    {
        ASSERT_IF(
            logError_.load() == STATUS_SUCCESS,
            "{0}:ImmediatelyProcessRecordAsync | logException_ must be null",
            TraceId);

        errorCode = flushErrorCode;
    }

    // In case there was an apply failure, we should fault any further applies of any records.
    // Without this, we could end up assuming that all applies have succeeded and as a result, issue a checkpoint call

    if (NT_SUCCESS(errorCode))
    {
        errorCode = serviceError_.load();
    }

    switch (logRecordSPtr->RecordType)
    {
    case LogRecordType::Enum::TruncateHead:
        checkpointManager_->ApplyLogHeadTruncationIfPermitted(*logRecordSPtr, errorCode);
        break;

    case LogRecordType::Enum::BeginCheckpoint:
        if (processingMode == RecordProcessingMode::Enum::ProcessImmediately)
        {
            goto explicit_default;
        }
        else
        {
            co_await checkpointManager_->ApplyCheckpointIfPermittedAsync(*logRecordSPtr, errorCode);
        }
        break;

    default:
    explicit_default:
        logRecordSPtr->CompletedApply(errorCode);
        break;
    }

    if (processingMode == RecordProcessingMode::Enum::ProcessImmediately)
    {
        if (logRecordSPtr->RecordType == LogRecordType::Enum::Information)
        {
            InformationLogRecord::SPtr informationLogRecord = dynamic_cast<InformationLogRecord *>(logRecordSPtr.RawPtr());
            ASSERT_IF(
                informationLogRecord == nullptr,
                "{0}:ImmediatelyProcessRecordAsync | Unexpected dynamic cast failure",
                TraceId);
            
            EventSource::Events->OPRecordProcessedImmediatelyInfo(
                TracePartitionId,
                ReplicaId,
                roleContextDrainState_->DrainStream,
                logRecordSPtr->Psn,
                informationLogRecord->InformationEvent);
        }
        else
        { 
            EventSource::Events->OPRecordProcessedImmediately(
                TracePartitionId,
                ReplicaId,
                roleContextDrainState_->DrainStream,
                logRecordSPtr->Psn);
        }

        logRecordSPtr->CompletedProcessing();
    }

    co_return;
}

void OperationProcessor::PrepareToProcessLogicalRecord()
{
    numberOfLogicalRecordsBeingProcessed_++;
}

void OperationProcessor::PrepareToProcessPhysicalRecord()
{
    numberOfPhysicalRecordsBeingProcessed_++;
}

Awaitable<void> OperationProcessor::ProcessLoggedRecordAsync(__in LogRecord & logRecord)
{
    LogRecord::SPtr logRecordSPtr = &logRecord;

    ASSERT_IFNOT(
        logRecordSPtr->IsFlushed,
        "{0}:ProcessLoggedRecordAsync | logRecord must be flushed",
        TraceId);

    LogicalLogRecord * logicalLogRecord = logRecordSPtr->AsLogicalLogRecord();

    ASSERT_IFNOT(
        logicalLogRecord != nullptr,
        "{0}:ProcessLoggedRecordAsync | logicalLogRecord must not be null",
        TraceId);

    FABRIC_REPLICA_ROLE currentRole = roleContextDrainState_->ReplicaRole;

    co_await ApplyCallback(*logRecordSPtr);

    if (currentRole != FABRIC_REPLICA_ROLE_PRIMARY)
    {
        Unlock(*logicalLogRecord);
    }

    co_return;
}
        
void OperationProcessor::UpdateDispatchingBarrierTask(__in CompletionTask & barrierTask)
{
    versionManager_->UpdateDispatchingBarrierTask(barrierTask);
}
            
void OperationProcessor::Unlock(__in LogicalLogRecord & record)
{
    ASSERT_IFNOT(
        record.IsApplied,
        "{0}:Unlock | Record is not applied during unlock, type:{1} lsn:{2}, psn:{3}",
        TraceId,
        record.RecordType,
        record.Lsn,
        record.Psn);

    ASSERT_IFNOT(
        recoveredOrCopiedCheckpointState_->Lsn != Constants::InvalidLsn,
        "{0}:Unlock | Recovered or copied checkpoint state lsn is invalid during unlock",
        TraceId);

    IStateProviderManager::SPtr stateManager = stateManager_->TryGetTarget();
    ASSERT_IFNOT(
        stateManager != nullptr,
        "{0}:Unlock | State manager must not be null",
        TraceId);

    // If flush fails, the records are processed immediately, after which the processing can complete (before waiting for unlocks)
    if (record.FlushError == STATUS_SUCCESS)
    {
        ASSERT_IFNOT(
            roleContextDrainState_->DrainStream != DrainingStream::Enum::Invalid,
            "{0}:Unlock | Role context drain state is invalid during unlock",
            TraceId);
    }

    LogicalLogRecord::SPtr recordSPtr = &record;
    BeginTransactionOperationLogRecord::SPtr beginTransactionRecord = nullptr;
    NTSTATUS status = STATUS_SUCCESS;

    OperationLogRecord::SPtr operationRecord = nullptr;

    switch (record.RecordType)
    {
    case LogRecordType::Enum::BeginTransaction:
        beginTransactionRecord = dynamic_cast<BeginTransactionOperationLogRecord *>(recordSPtr.RawPtr());
        ASSERT_IF(
            beginTransactionRecord == nullptr,
            "{0}:Unlock | Unexpected dynamic cast failure",
            TraceId);

        if (beginTransactionRecord->IsSingleOperationTransaction)
        {
            if (beginTransactionRecord->Lsn > recoveredOrCopiedCheckpointState_->Lsn)
            {
                OperationContext::CSPtr operationContext = beginTransactionRecord->ResetOperationContext();
                if (operationContext != nullptr)
                {
                    status = stateManager->Unlock(*operationContext);

                    if (!NT_SUCCESS(status))
                    {
                        break;
                    }
                }
            }

            FireCommitNotification(beginTransactionRecord->BaseTransaction);
        }

        break;

    case LogRecordType::Enum::Operation:
        operationRecord = dynamic_cast<OperationLogRecord *>(recordSPtr.RawPtr());
        ASSERT_IF(
            operationRecord == nullptr,
            "{0}:Unlock | Unexpected dynamic cast failure",
            TraceId);

        if (operationRecord->BaseTransaction.IsAtomicOperation)
        {
            OperationContext::CSPtr operationContext = operationRecord->ResetOperationContext();
            if (operationContext != nullptr)
            {
                status = stateManager->Unlock(*operationContext);

                if (!NT_SUCCESS(status))
                {
                    break;
                }
            }
        }

        break;

    case LogRecordType::Enum::EndTransaction:
    {
        EndTransactionLogRecord::SPtr endTransactionRecord = dynamic_cast<EndTransactionLogRecord *>(recordSPtr.RawPtr());
        ASSERT_IF(
            endTransactionRecord == nullptr,
            "{0}:Unlock | Unexpected dynamic cast failure",
            TraceId);

        if (endTransactionRecord->Lsn > recoveredOrCopiedCheckpointState_->Lsn)
        {
            TransactionLogRecord::SPtr transactionRecord = endTransactionRecord.RawPtr();

            do
            {
                transactionRecord = transactionRecord->ParentTransactionRecord;
                ASSERT_IFNOT(
                    transactionRecord != nullptr,
                    "{0}:Unlock | Null parent xact record found during unlock",
                    TraceId);
            } while (transactionRecord->RecordType != LogRecordType::Enum::BeginTransaction);

            ASSERT_IFNOT(
                !LogRecord::IsInvalid(transactionRecord.RawPtr()) &&
                transactionRecord->RecordType == LogRecordType::Enum::BeginTransaction,
                "{0}:Unlock | Expected begin xact log record, actual: {1}",
                TraceId,
                transactionRecord->RecordType);

            beginTransactionRecord = dynamic_cast<BeginTransactionOperationLogRecord *>(transactionRecord.RawPtr());
            ASSERT_IF(
                beginTransactionRecord == nullptr,
                "{0}:Unlock | Unexpected dynamic cast failure",
                TraceId);

            OperationContext::CSPtr operationContext = beginTransactionRecord->ResetOperationContext();
            if (operationContext != nullptr)
            {
                status = stateManager->Unlock(*operationContext);
                
                if (!NT_SUCCESS(status))
                {
                    break;
                }
            }

            transactionRecord = beginTransactionRecord.RawPtr();
            do
            {
                transactionRecord = transactionRecord->ChildTransactionRecord;
                ASSERT_IFNOT(
                    transactionRecord != nullptr,
                    "{0}:Unlock | Null child xact record found",
                    TraceId);

                if (transactionRecord.RawPtr() == endTransactionRecord.RawPtr())
                {
                    break;
                }

                operationRecord = dynamic_cast<OperationLogRecord *>(transactionRecord.RawPtr());
                ASSERT_IF(
                    operationRecord == nullptr,
                    "{0}:Unlock | Unexpected dynamic cast failure");

                operationContext = operationRecord->ResetOperationContext();

                if (operationContext != nullptr)
                {
                    status = stateManager->Unlock(*operationContext);

                    if (!NT_SUCCESS(status))
                    {
                        break;
                    }
                }
            } while (true);
        }
        else
        {
            TransactionLogRecord::SPtr transactionRecord = endTransactionRecord.RawPtr();

            do
            {
                TransactionLogRecord::SPtr parentRecord = transactionRecord->ParentTransactionRecord;
                if (parentRecord == nullptr)
                {
                    break;
                }

                transactionRecord = parentRecord;
            } while (true);

            ASSERT_IFNOT(
                !endTransactionRecord->IsEnlistedTransaction ||
                transactionRecord->RecordType == LogRecordType::Enum::BeginTransaction,
                "{0}:Unlock | Expected begin xact log record, actual: {1} or end xact record is enlisted",
                TraceId,
                transactionRecord->RecordType);

            if (transactionRecord.RawPtr() != endTransactionRecord.RawPtr())
            {
                do
                {
                    transactionRecord = transactionRecord->ChildTransactionRecord;

                    ASSERT_IFNOT(
                        transactionRecord != nullptr && !LogRecord::IsInvalid(transactionRecord.RawPtr()),
                        "{0}:Unlock | Invalid child xact record encountered during unlock",
                        TraceId);

                    if (transactionRecord.RawPtr() == endTransactionRecord.RawPtr())
                    {
                        break;
                    }
                } while (true);
            }
        }

        if (!NT_SUCCESS(status))
        {
            break;
        }

        if (endTransactionRecord->IsCommitted)
        {
            FireCommitNotification(endTransactionRecord->BaseTransaction);
        }

        break;
    }

    case LogRecordType::Enum::Backup:
    {
        BackupLogRecord::SPtr backupLogRecord = dynamic_cast<BackupLogRecord *>(recordSPtr.RawPtr());
        ASSERT_IF(
            backupLogRecord == nullptr,
            "{0}:Unlock | Failed to cast backup log record in OperationProcessor",
            TraceId);

        backupManager_->SetLastCompletedBackupLogRecord(*backupLogRecord);
        break;
    }

    default:
        // Unexpected record type
        ASSERT_IFNOT(
            false,
            "{0}:Unlock | Unexpected record type:{1}",
            TraceId,
            record.RecordType);
    }

    if (!NT_SUCCESS(status))
    {
        ProcessServiceError(
            L"Unlock",
            record,
            status);
    }

    ProcessedLogicalRecord(record);
}

void OperationProcessor::FireCommitNotification(__in TransactionBase const & transaction)
{
    ITransactionChangeHandler::SPtr eventHandler = changeHandlerCache_.Get();

    // If there is no registered handler, return
    if (eventHandler == nullptr)
    {
        return;
    }

    ITransaction const * txnPtr = dynamic_cast<ITransaction const *>(&transaction);

    ASSERT_IFNOT(txnPtr != nullptr, "{0}: TxnBase must be an ITransaction.", TraceId);

    if (transactionalReplicator_ != nullptr)
    {
        eventHandler->OnTransactionCommitted(*transactionalReplicator_, *txnPtr);
    }
}

Awaitable<void> OperationProcessor::ApplyCallback(__in LogRecord & record) noexcept
{
    LogRecord::SPtr recordSPtr = &record;

    ASSERT_IFNOT(
        roleContextDrainState_->DrainStream != DrainingStream::Enum::Invalid &&
            roleContextDrainState_->DrainStream != DrainingStream::Enum::State,
        "{0}: ApplyCallback | Invalid role context drain state encountered: {1} for log record lsn: {2} and psn: {3}",
        TraceId,
        roleContextDrainState_->DrainStream,
        record.Lsn,
        record.Psn);

    IStateProviderManager::SPtr stateManager = stateManager_->TryGetTarget();
    ASSERT_IFNOT(
        stateManager != nullptr,
        "{0}: ApplyCallback | State manager must not be null",
        TraceId);

    NTSTATUS serviceError = serviceError_.load();

    if (!NT_SUCCESS(serviceError))
    {
        recordSPtr->CompletedApply(serviceError);
        co_return;
    }

    OperationLogRecord::SPtr operationRecord = nullptr;
    NTSTATUS status = STATUS_SUCCESS;
    ApplyContext::Enum applyRedoContext = roleContextDrainState_->ApplyRedoContext;

    switch (record.RecordType)
    {
    case LogRecordType::Enum::BeginTransaction:
    {
        BeginTransactionOperationLogRecord::SPtr beginTransactionRecord = dynamic_cast<BeginTransactionOperationLogRecord *>(recordSPtr.RawPtr());
        ASSERT_IF(
            beginTransactionRecord == nullptr,
            "{0}: ApplyCallback | Unexpected dynamic cast failure",
            TraceId);

        if (beginTransactionRecord->IsSingleOperationTransaction &&
            beginTransactionRecord->Lsn > recoveredOrCopiedCheckpointState_->Lsn)
        {
            beginTransactionRecord->BaseTransaction.CommitSequenceNumber = beginTransactionRecord->Lsn;

            OperationContext::CSPtr operationContext = nullptr;

            status = co_await stateManager->ApplyAsync(
                beginTransactionRecord->Lsn,
                beginTransactionRecord->BaseTransaction,
                applyRedoContext,
                beginTransactionRecord->Metadata.RawPtr(),
                beginTransactionRecord->Redo.RawPtr(),
                operationContext);

            if (!NT_SUCCESS(status))
            {
                break;
            }

            if (operationContext != nullptr)
            {
                beginTransactionRecord->OperationContextValue = *operationContext;
            }
        }

        EventSource::Events->OPApplyCallbackTransactionRecord(
            TracePartitionId,
            ReplicaId,
            roleContextDrainState_->DrainStream,
            beginTransactionRecord->RecordType,
            beginTransactionRecord->Lsn,
            beginTransactionRecord->Psn,
            beginTransactionRecord->RecordPosition,
            beginTransactionRecord->BaseTransaction.TransactionId,
            beginTransactionRecord->IsSingleOperationTransaction);

        break;
    }

    case LogRecordType::Enum::Operation:
        operationRecord = dynamic_cast<OperationLogRecord *>(recordSPtr.RawPtr());
        ASSERT_IF(
            operationRecord == nullptr,
            "{0}: ApplyCallback | Unexpected dynamic cast failure",
            TraceId);

        if (operationRecord->BaseTransaction.IsAtomicOperation)
        {
            if (operationRecord->Lsn > recoveredOrCopiedCheckpointState_->Lsn)
            {
                // For atomic operations create lsn equals commit lsn
                operationRecord->BaseTransaction.CommitSequenceNumber = operationRecord->Lsn;

                OperationContext::CSPtr operationContext = nullptr;

                status = co_await stateManager->ApplyAsync(
                    operationRecord->Lsn,
                    operationRecord->BaseTransaction,
                    applyRedoContext,
                    operationRecord->Metadata.RawPtr(),
                    operationRecord->Redo.RawPtr(),
                    operationContext);

                if (!NT_SUCCESS(status))
                {
                    break;
                }

                if (operationContext != nullptr)
                {
                    operationRecord->OperationContextValue = *operationContext;
                }
            }
        }

        EventSource::Events->OPApplyCallbackTransactionRecord(
            TracePartitionId,
            ReplicaId,
            roleContextDrainState_->DrainStream,
            operationRecord->RecordType,
            operationRecord->Lsn,
            operationRecord->Psn,
            operationRecord->RecordPosition,
            operationRecord->BaseTransaction.TransactionId,
            operationRecord->BaseTransaction.IsAtomicOperation);

        break;

    case LogRecordType::Enum::EndTransaction:
    {
        EndTransactionLogRecord::SPtr endTransactionRecord = dynamic_cast<EndTransactionLogRecord *>(recordSPtr.RawPtr());
        ASSERT_IF(
            endTransactionRecord == nullptr,
            "{0}: ApplyCallback | Unexpected dynamic cast failure",
            TraceId);

        if (endTransactionRecord->IsCommitted &&
            endTransactionRecord->Lsn > recoveredOrCopiedCheckpointState_->Lsn)
        {
            // I am currently adopting the approach of only applying updates that do not
            // belong to any transaction in the update switch above and
            // applying the updates that belong to a transaction only when if it commits.
            // The other approach is to immediately apply updates of all
            // transactions in the update switch case above and then undoing updates of
            // aborted transactions here.
            // Both approaches have their pros and cons and we may want to look into
            // making this a replicator option that the service developer can choose.

            // If on Primary, Transaction object is shared.

            endTransactionRecord->BaseTransaction.CommitSequenceNumber = endTransactionRecord->Lsn;

            BeginTransactionOperationLogRecord::SPtr beginTransactionRecord = nullptr;
            TransactionLogRecord::SPtr transactionRecord = endTransactionRecord.RawPtr();

            do
            {
                transactionRecord = transactionRecord->ParentTransactionRecord;
                ASSERT_IFNOT(
                    transactionRecord != nullptr,
                    "{0}: ApplyCallback | Null parent xact record found",
                    TraceId);
            }
            while (transactionRecord->RecordType != LogRecordType::Enum::BeginTransaction);

            ASSERT_IFNOT(
                !LogRecord::IsInvalid(transactionRecord.RawPtr()) && 
                    transactionRecord->RecordType == LogRecordType::Enum::BeginTransaction,
                "{0}: Expected begin xact log record, actual: {1}",
                TraceId,
                transactionRecord->RecordType);

            beginTransactionRecord = dynamic_cast<BeginTransactionOperationLogRecord *>(transactionRecord.RawPtr());
            ASSERT_IF(
                beginTransactionRecord == nullptr,
                "{0}: ApplyCallback | Unexpected dynamic cast failure",
                TraceId);

            ASSERT_IFNOT(
                !beginTransactionRecord->IsSingleOperationTransaction,
                "{0}: ApplyCallback | Unexpected single op xact found during end xact processing",
                TraceId);

            // If not on primary, Transaction object is shared
            if ((applyRedoContext & ApplyContext::PRIMARY) == 0)
            {
                beginTransactionRecord->BaseTransaction.CommitSequenceNumber = endTransactionRecord->Lsn;
            }
            else
            {
                // TODO: Temporary assert should be removed later
                ASSERT_IFNOT(
                    beginTransactionRecord->BaseTransaction.CommitSequenceNumber == endTransactionRecord->Lsn,
                    "{0}: ApplyCallback | beginTransactionRecord->BaseTransaction.CommitSequenceNumber == endTransactionRecord->Lsn. BaseTransaction.CommitSequenceNumber={1}, endTransactionRecord->Lsn={2}",
                    TraceId,
                    beginTransactionRecord->BaseTransaction.CommitSequenceNumber,
                    endTransactionRecord->Lsn);
            }

            OperationContext::CSPtr operationContext = nullptr;
            
            status = co_await stateManager->ApplyAsync(
                beginTransactionRecord->Lsn,
                beginTransactionRecord->BaseTransaction,
                applyRedoContext,
                beginTransactionRecord->Metadata.RawPtr(),
                beginTransactionRecord->Redo.RawPtr(),
                operationContext);

            if (!NT_SUCCESS(status))
            {
                break;
            }

            if (operationContext != nullptr)
            {
                beginTransactionRecord->OperationContextValue = *operationContext;
            }

            do
            {
                transactionRecord = transactionRecord->ChildTransactionRecord;

                ASSERT_IFNOT(
                    transactionRecord != nullptr && !LogRecord::IsInvalid(transactionRecord.RawPtr()),
                    "{0}: ApplyCallback | Invalid child xact record encountered",
                    TraceId);

                if (transactionRecord.RawPtr() == endTransactionRecord.RawPtr())
                {
                    break;
                }

                operationRecord = dynamic_cast<OperationLogRecord *>(transactionRecord.RawPtr());
                ASSERT_IF(
                    operationRecord == nullptr,
                    "{0}: ApplyCallback | Unexpected dynamic cast failure",
                    TraceId);

                // If not on primary, Transaction object is shared
                if ((applyRedoContext & ApplyContext::PRIMARY) == 0)
                {
                    operationRecord->BaseTransaction.CommitSequenceNumber = endTransactionRecord->Lsn;
                }
                else
                {
                    // TODO: Temporary assert should be removed later
                    ASSERT_IFNOT(
                        beginTransactionRecord->BaseTransaction.CommitSequenceNumber == endTransactionRecord->Lsn,
                        "{0}: ApplyCallback | beginTransactionRecord->BaseTransaction.CommitSequenceNumber == endTransactionRecord->Lsn. BaseTransaction.CommitSequenceNumber={1}, endTransactionRecord->Lsn={2}",
                        TraceId,
                        beginTransactionRecord->BaseTransaction.CommitSequenceNumber,
                        endTransactionRecord->Lsn);
                }

                OperationContext::CSPtr opContext = nullptr;
                
                status = co_await stateManager->ApplyAsync(
                    operationRecord->Lsn,
                    operationRecord->BaseTransaction,
                    applyRedoContext,
                    operationRecord->Metadata.RawPtr(),
                    operationRecord->Redo.RawPtr(),
                    opContext);

                if (!NT_SUCCESS(status))
                {
                    break;
                }

                if (opContext != nullptr)
                {
                    operationRecord->OperationContextValue = *opContext;
                }
            }
            while (true);

            if (!NT_SUCCESS(status))
            {
                break;
            }

            EventSource::Events->OPApplyCallbackTransactionRecord(
                TracePartitionId,
                ReplicaId,
                roleContextDrainState_->DrainStream,
                endTransactionRecord->RecordType,
                endTransactionRecord->Lsn,
                endTransactionRecord->Psn,
                endTransactionRecord->RecordPosition,
                endTransactionRecord->BaseTransaction.TransactionId,
                false);
        }

        break;
    }

    case LogRecordType::Enum::Backup:

        break;

    default:
        ASSERT_IFNOT(
            false,
            "{0}: ApplyCallback | Invalid record type {1}",
            TraceId,
            record.RecordType);
        break;
    }

    if (!NT_SUCCESS(status))
    {
        serviceError = ProcessServiceError(
            L"OnApply",
            record,
            status);

        ASSERT_IF(
            NT_SUCCESS(serviceError),
            "{0}:{1} Service Exception should not be null after apply failure",
            TracePartitionId,
            ReplicaId);
    }

    record.CompletedApply(serviceError);

    co_return;
}

NTSTATUS OperationProcessor::ProcessServiceError(
    __in KStringView const & component,
    __in LogRecord const & record,
    __in NTSTATUS errorCode)
{
    ProcessError(
        component, 
        record, 
        errorCode);

    if (roleContextDrainState_->DrainStream != DrainingStream::Enum::Recovery)
    {
        roleContextDrainState_->ReportFault();
    }

    if (serviceError_.load() == STATUS_SUCCESS)
    {
        serviceError_.store(errorCode);
    }

    return serviceError_.load();
}

void OperationProcessor::ProcessLogError(
    __in KStringView const & component,
    __in LogRecord const & record,
    __in NTSTATUS errorCode)
{
    bool isClosedException = ProcessError(
        component,
        record,
        errorCode);

    if (logError_.load() == STATUS_SUCCESS)
    {
        logError_.store(errorCode);
    }

    if (!isClosedException)
    {
        if (roleContextDrainState_->DrainStream != DrainingStream::Enum::Recovery)
        {
            roleContextDrainState_->ReportFault();
        }
    }
}

bool OperationProcessor::ProcessError(
    __in KStringView const & component,
    __in LogRecord const & record,
    __in NTSTATUS errorCode)
{
    LR_TRACE_EXCEPTIONRECORD(
        ToStringLiteral(component),
        record,
        errorCode);

    return (errorCode == SF_STATUS_OBJECT_CLOSED);
}

void OperationProcessor::ProcessedLogicalRecord(__in LogicalLogRecord & logicalRecord)
{
    if (!NT_SUCCESS(logicalRecord.FlushError))
    {
        return;
    }

    logicalRecord.CompletedProcessing();

    LONG outstandingNumberOfRecordsBeingProcessed = --numberOfLogicalRecordsBeingProcessed_;
    ASSERT_IFNOT(
        outstandingNumberOfRecordsBeingProcessed >= 0,
        "{0}:ProcessedLogicalRecord outstandingNumberOfRecordsBeingProcessed >= 0",
        TraceId);

    if (outstandingNumberOfRecordsBeingProcessed == 0)
    {
        AwaitableCompletionSource<LogicalLogRecord::SPtr>::SPtr snapLogicalRecordsProcessingTcs = logicalRecordsProcessingTcs_;
        bool success = snapLogicalRecordsProcessingTcs->TrySetResult(&logicalRecord);
        ASSERT_IFNOT(
            success,
            "{0}: Failed to set logicalRecordsProcessingTcs result",
            TraceId);
    }

    // Do not access anything more here as the replicator could possibly destruct after the trysetresult above
}

void OperationProcessor::ProcessedPhysicalRecord(__in PhysicalLogRecord & physicalRecord)
{
    if (!NT_SUCCESS(physicalRecord.FlushError))
    {
        return;
    }

    physicalRecord.CompletedProcessing();

    LONG outstandingNumberOfRecordsBeingProcessed = --numberOfPhysicalRecordsBeingProcessed_;
    ASSERT_IFNOT(
        outstandingNumberOfRecordsBeingProcessed >= 0,
        "{0}:ProcessedPhysicalRecord outstandingNumberOfRecordsBeingProcessed >= 0",
        TraceId);

    if (outstandingNumberOfRecordsBeingProcessed == 0)
    {
        AwaitableCompletionSource<PhysicalLogRecord::SPtr>::SPtr snapPhysicalRecordsProcessingTcs = physicalRecordsProcessingTcs_;
        bool success = snapPhysicalRecordsProcessingTcs->TrySetResult(&physicalRecord);
        ASSERT_IFNOT(
            success,
            "{0}: Failed to set physicalRecordsProcessingTcs_ result",
            TraceId);
    }

    // Do not access anything more here as the replicator could possibly destruct after the trysetresult above
}
 
Awaitable<void> OperationProcessor::WaitForLogicalRecordsProcessingAsync()
{
    LONG outstandingNumberOfRecordsBeingProcessed = --numberOfLogicalRecordsBeingProcessed_;

    if (outstandingNumberOfRecordsBeingProcessed > 0)
    {
        EventSource::Events->OPWaitForRecordsProcessing(
            TracePartitionId,
            ReplicaId,
            "Logical",
            outstandingNumberOfRecordsBeingProcessed);

        Awaitable<LogicalLogRecord::SPtr> recordsProcessingTask = logicalRecordsProcessingTcs_->GetAwaitable();
        LogicalLogRecord::SPtr lastProcessedRecord = co_await recordsProcessingTask;

        logicalRecordsProcessingTcs_ = CompletionTask::CreateAwaitableCompletionSource<LogicalLogRecord::SPtr>(OPERATIONPROCESSOR_TAG, GetThisAllocator());
        ASSERT_IFNOT(
            logicalRecordsProcessingTcs_ != nullptr,
            "{0}: logicalRecordsProcessingTcs_ must not be null",
            TraceId);

        EventSource::Events->OPWaitForRecordsProcessingDone(
            TracePartitionId,
            ReplicaId,
            "Logical",
            lastProcessedRecord->RecordType,
            lastProcessedRecord->Lsn,
            lastProcessedRecord->Psn,
            lastProcessedRecord->RecordPosition);
    }
    else
    { 
        EventSource::Events->OPWaitForRecordsProcessing(
            TracePartitionId,
            ReplicaId,
            "LogicalDone",
            outstandingNumberOfRecordsBeingProcessed);
    }

    outstandingNumberOfRecordsBeingProcessed = numberOfLogicalRecordsBeingProcessed_.load();

    ASSERT_IFNOT(
        outstandingNumberOfRecordsBeingProcessed == 0,
        "{0}: WaitForLogicalRecordsProcessingAsync: Outstanding number of records must be 0. It is {1}",
        TraceId,
        outstandingNumberOfRecordsBeingProcessed);

    numberOfLogicalRecordsBeingProcessed_.store(1);
    co_return;
}

Awaitable<void> OperationProcessor::WaitForPhysicalRecordsProcessingAsync()
{
    LONG outstandingNumberOfRecordsBeingProcessed = --numberOfPhysicalRecordsBeingProcessed_;

    if (outstandingNumberOfRecordsBeingProcessed > 0)
    {
        EventSource::Events->OPWaitForRecordsProcessing(
            TracePartitionId,
            ReplicaId,
            "Physical",
            outstandingNumberOfRecordsBeingProcessed);

        Awaitable<PhysicalLogRecord::SPtr> recordsProcessingTask = physicalRecordsProcessingTcs_->GetAwaitable();
        PhysicalLogRecord::SPtr lastProcessedRecord = co_await recordsProcessingTask;

        physicalRecordsProcessingTcs_ = CompletionTask::CreateAwaitableCompletionSource<PhysicalLogRecord::SPtr>(OPERATIONPROCESSOR_TAG, GetThisAllocator());
        ASSERT_IFNOT(
            physicalRecordsProcessingTcs_ != nullptr,
            "{0} physicalRecordsProcessingTcs_ must not be null",
            TraceId);

        EventSource::Events->OPWaitForRecordsProcessingDone(
            TracePartitionId,
            ReplicaId,
            "Physical",
            lastProcessedRecord->RecordType,
            lastProcessedRecord->Lsn,
            lastProcessedRecord->Psn,
            lastProcessedRecord->RecordPosition);
    }
    else
    {
        EventSource::Events->OPWaitForRecordsProcessing(
            TracePartitionId,
            ReplicaId,
            "PhysicalDone",
            outstandingNumberOfRecordsBeingProcessed);
    }

    outstandingNumberOfRecordsBeingProcessed = numberOfPhysicalRecordsBeingProcessed_.load();

    ASSERT_IFNOT(
        outstandingNumberOfRecordsBeingProcessed == 0,
        "{0}: WaitForPhysicalRecordsProcessingAsync: Outstanding number of records must be 0. It is {1}",
        TraceId,
        outstandingNumberOfRecordsBeingProcessed);

    numberOfPhysicalRecordsBeingProcessed_.store(1);
    co_return;
}

Awaitable<void> OperationProcessor::WaitForAllRecordsProcessingAsync()
{
    co_await WaitForLogicalRecordsProcessingAsync();
    co_await WaitForPhysicalRecordsProcessingAsync();
    co_return;
}


NTSTATUS OperationProcessor::RegisterTransactionChangeHandler(
    __in ITransactionChangeHandler & transactionChangeHandler) noexcept
{
    changeHandlerCache_.Put(&transactionChangeHandler);
    return STATUS_SUCCESS;
}

NTSTATUS OperationProcessor::UnRegisterTransactionChangeHandler() noexcept
{
    changeHandlerCache_.Put(nullptr);
    return STATUS_SUCCESS;
}

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

Common::StringLiteral const TraceComponent("TransactionManager");
Common::TimeSpan const TransactionManager::ConfigUpdateIntervalInSeconds = Common::TimeSpan::FromSeconds(30);

TransactionManager::TransactionManager(
    __in PartitionedReplicaId const & traceId,
    __in RecoveredOrCopiedCheckpointState const & recoveredOrCopiedCheckpointLsn,
    __in TransactionMap & transactionMap,
    __in CheckpointManager & checkpointManager,
    __in ReplicatedLogManager & replicatedLogManager,
    __in OperationProcessor & operationProcessor,
    __in InvalidLogRecords & invalidLogRecords,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in TRPerformanceCountersSPtr const & perfCounters)
    : KObject()
    , KShared()
    , PartitionedReplicaTraceComponent(traceId)
    , transactionalReplicatorConfig_(transactionalReplicatorConfig)
    , configUpdateStopWatch_(Common::Stopwatch())
    , recoveredOrCopiedCheckpointState_(&recoveredOrCopiedCheckpointLsn)
    , transactionMap_(&transactionMap)
    , checkpointManager_(&checkpointManager)
    , replicatedLogManager_(&replicatedLogManager)
    , operationProcessor_(&operationProcessor)
    , invalidLogRecords_(&invalidLogRecords)
    , perfCounters_(perfCounters)
{
    EventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        L"TransactionManager",
        reinterpret_cast<uintptr_t>(this));

    configUpdateStopWatch_.Start();
}

TransactionManager::~TransactionManager()
{
    EventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        L"TransactionManager",
        reinterpret_cast<uintptr_t>(this));
}

TransactionManager::SPtr TransactionManager::Create(
    __in PartitionedReplicaId const & traceId,
    __in RecoveredOrCopiedCheckpointState const & recoveredOrCopiedCheckpointLsn,
    __in TransactionMap & transactionMap,
    __in CheckpointManager & checkpointManager,
    __in ReplicatedLogManager & replicatedLogManager,
    __in OperationProcessor & operationProcessor,
    __in InvalidLogRecords & invalidLogRecords,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig,
    __in TRPerformanceCountersSPtr const & perfCounters,
    __in KAllocator & allocator)
{
    TransactionManager * pointer = _new(TRANSACTIONMANAGER_TAG, allocator) TransactionManager(
        traceId,
        recoveredOrCopiedCheckpointLsn,
        transactionMap,
        checkpointManager,
        replicatedLogManager,
        operationProcessor,
        invalidLogRecords,
        transactionalReplicatorConfig,
        perfCounters);

    THROW_ON_ALLOCATION_FAILURE(pointer);

    return TransactionManager::SPtr(pointer);
}

void TransactionManager::CreateTransactionState(
    __in Epoch const & tailEpoch,
    __in TransactionLogRecord & record) noexcept
{
    TransactionLogRecord::SPtr recordSPtr = &record;

    switch(recordSPtr->RecordType)
    {
    case LogRecordType::Enum::BeginTransaction:
    {
        BeginTransactionOperationLogRecord::SPtr beginTxRecord = dynamic_cast<BeginTransactionOperationLogRecord *>(recordSPtr.RawPtr());
        ASSERT_IF(
            beginTxRecord == nullptr,
            "{0}: Unexpected dynamic cast failure",
            TraceId);

        perfCounters_->BeginTransactionOperationRate.Increment();

        if (beginTxRecord->IsSingleOperationTransaction)
        {
            perfCounters_->CommitTransactionRate.Increment();

            EventSource::Events->AcceptSingleOperationTransaction(
                TracePartitionId,
                ReplicaId,
                beginTxRecord->BaseTransaction.TransactionId,
                recordSPtr->Lsn);
        }
        else
        {
            EventSource::Events->AcceptBeginTransaction(
                TracePartitionId,
                ReplicaId,
                beginTxRecord->BaseTransaction.TransactionId,
                recordSPtr->Lsn);
        }

        beginTxRecord->RecordEpoch = tailEpoch;
        transactionMap_->CreateTransaction(*beginTxRecord);

        break;
    }

    case LogRecordType::Enum::Operation: 
    {
        OperationLogRecord::SPtr dataRecord = dynamic_cast<OperationLogRecord *>(recordSPtr.RawPtr());
        ASSERT_IF(
            dataRecord == nullptr,
            "{0}: Unexpected dynamic cast failure",
            TraceId);

        if (dataRecord->BaseTransaction.IsAtomicOperation || dataRecord->IsRedoOnly)
        {
            perfCounters_->AtomicOperationRate.Increment();

            EventSource::Events->AcceptAtomicOperation(
                TracePartitionId,
                ReplicaId,
                dataRecord->BaseTransaction.TransactionId,
                recordSPtr->Lsn,
                dataRecord->IsRedoOnly);
        }
        else
        {
            perfCounters_->AddOperationRate.Increment();

            EventSource::Events->AcceptOperation(
                TracePartitionId,
                ReplicaId,
                dataRecord->BaseTransaction.TransactionId,
                recordSPtr->Lsn);
        }

        transactionMap_->AddOperation(*dataRecord);

        ASSERT_IFNOT(
            dataRecord->IsEnlistedTransaction ||
            dataRecord->Lsn <= recoveredOrCopiedCheckpointState_->Lsn,
            "{0}: Invalid op xact log record - lsn: {1}, recoverd or copied checkpoint lsn: {2}, enlisted: {3}",
            TraceId,
            dataRecord->Lsn,
            recoveredOrCopiedCheckpointState_->Lsn,
            dataRecord->IsEnlistedTransaction);

        break;
    }
    case LogRecordType::Enum::EndTransaction:
    {
        EndTransactionLogRecord::SPtr endTxRecord = dynamic_cast<EndTransactionLogRecord *>(recordSPtr.RawPtr());
        ASSERT_IF(
            endTxRecord == nullptr,
            "{0}: Unexpected dynamic cast failure",
            TraceId);

        if (endTxRecord->IsCommitted)
        {
            perfCounters_->CommitTransactionRate.Increment();
        }
        else 
        {
            perfCounters_->AbortTransactionRate.Increment();
        }

        EventSource::Events->AcceptEndTransaction(
            TracePartitionId,
            ReplicaId,
            endTxRecord->BaseTransaction.TransactionId,
            recordSPtr->Lsn,
            endTxRecord->IsCommitted);

        transactionMap_->CompleteTransaction(*endTxRecord);

        ASSERT_IFNOT(
            endTxRecord->IsEnlistedTransaction ||
            endTxRecord->Lsn <= recoveredOrCopiedCheckpointState_->Lsn,
            "{0}: Invalid end xact log record - lsn: {1}, recoverd or copied checkpoint lsn: {2}, enlisted: {3}",
            TraceId,
            endTxRecord->Lsn,
            recoveredOrCopiedCheckpointState_->Lsn,
            endTxRecord->IsEnlistedTransaction);

        break;
    }
    default: 
        
        ASSERT_IFNOT(
            false,
            "{0}: CreateTransactionState | Invalid record type {0}",
            TraceId,
            record.RecordType);
        break;
    }
}

Awaitable<NTSTATUS> TransactionManager::AbortTransactionAsync(
    __in Transaction & transaction,
    __out LONG64 & abortLsn) noexcept
{
    abortLsn = Constants::InvalidLsn;

    EndTransactionLogRecord::SPtr result;
    NTSTATUS status = co_await EndTransactionAsync(transaction, false, true, result);

    CO_RETURN_ON_FAILURE(status);

    abortLsn = result->Lsn;

    co_return status;
}

NTSTATUS TransactionManager::AddOperation(
    __in Transaction & transaction,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in_opt OperationContext const * const operationContext) noexcept
{
    OperationLogRecord::SPtr record = OperationLogRecord::Create(
        transaction,
        metaData,
        undo,
        redo,
        operationContext,
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        *invalidLogRecords_->Inv_TransactionLogRecord,
        GetThisAllocator());
    
    NTSTATUS status = ReplicateAndLogLogicalRecord(*record);

    RETURN_ON_FAILURE(status);

    ProcessLogicalRecordOnPrimary(*record);

    return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> TransactionManager::AddOperationAsync(
    __in AtomicOperation & atomicOperation,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in_opt OperationContext const * const operationContext,
    __out LONG64 & commitLsn) noexcept
{
    commitLsn = Constants::InvalidLsn;

    OperationLogRecord::SPtr record = OperationLogRecord::Create(
        atomicOperation,
        metaData,
        undo,
        redo,
        operationContext,
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        *invalidLogRecords_->Inv_TransactionLogRecord,
        GetThisAllocator());

    NTSTATUS status = ReplicateAndLogLogicalRecord(*record);

    CO_RETURN_ON_FAILURE(status);

    status = co_await ProcessLogicalRecordOnPrimaryAsync(*record);

    if (NT_SUCCESS(status))
    {
        commitLsn = record->Lsn;
    }

    co_return status;
}

Awaitable<NTSTATUS> TransactionManager::AddOperationAsync(
    __in AtomicRedoOperation & atomicRedoOperation,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const redo,
    __in_opt OperationContext const * const operationContext,
    __out LONG64 & commitLsn) noexcept
{
    commitLsn = Constants::InvalidLsn;

    OperationLogRecord::SPtr record = OperationLogRecord::Create(
        atomicRedoOperation,
        metaData,
        redo,
        operationContext,
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        *invalidLogRecords_->Inv_TransactionLogRecord,
        GetThisAllocator());

    NTSTATUS status = ReplicateAndLogLogicalRecord(*record);
    
    CO_RETURN_ON_FAILURE(status);

    status = co_await ProcessLogicalRecordOnPrimaryAsync(*record);

    if (NT_SUCCESS(status))
    {
        commitLsn = record->Lsn;
    }

    co_return status;
}

NTSTATUS TransactionManager::BeginTransaction(
    __in Transaction & transaction,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in_opt OperationContext const * const operationContext) noexcept
{
    BeginTransactionOperationLogRecord::SPtr record = BeginTransactionOperationLogRecord::Create(
        transaction,
        false, // isSingleOperationTransaction
        metaData,
        undo,
        redo,
        operationContext,
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        *invalidLogRecords_->Inv_TransactionLogRecord,
        GetThisAllocator());

    NTSTATUS status = ReplicateAndLogLogicalRecord(*record);

    RETURN_ON_FAILURE(status);

    ProcessLogicalRecordOnPrimary(*record);

    return STATUS_SUCCESS;
}

Awaitable<NTSTATUS> TransactionManager::BeginTransactionAsync(
    __in Transaction & transaction,
    __in_opt OperationData const * const metaData,
    __in_opt OperationData const * const undo,
    __in_opt OperationData const * const redo,
    __in_opt OperationContext const * const operationContext,
    __out LONG64 & commitLsn) noexcept
{
    commitLsn = Constants::InvalidLsn;

    BeginTransactionOperationLogRecord::SPtr record = BeginTransactionOperationLogRecord::Create(
        transaction,
        true, // isSingleOperationTransaction
        metaData,
        undo,
        redo,
        operationContext,
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        *invalidLogRecords_->Inv_TransactionLogRecord,
        GetThisAllocator());

    NTSTATUS status = ReplicateAndLogLogicalRecord(*record);

    CO_RETURN_ON_FAILURE(status);

    status = co_await ProcessLogicalRecordOnPrimaryAsync(*record);

    if (NT_SUCCESS(status))
    {
        commitLsn = record->Lsn;
    }

    co_return status;
}

Awaitable<NTSTATUS> TransactionManager::CommitTransactionAsync(
    __in Transaction & transaction,
    __out LONG64 & commitLsn) noexcept
{
    commitLsn = Constants::InvalidLsn;

    EndTransactionLogRecord::SPtr record = nullptr;

    NTSTATUS status = co_await EndTransactionAsync(
        transaction, 
        true, 
        true, 
        record);
    
    CO_RETURN_ON_FAILURE(status);

    commitLsn = record->Lsn;
    co_return status;
}

NTSTATUS TransactionManager::InitiateLogicalRecordProcessingOnPrimary(__in LogicalLogRecord & record) noexcept
{
    NTSTATUS status = checkpointManager_->ErrorIfThrottled(record);

    RETURN_ON_FAILURE(status);

    LogicalLogRecord::SPtr recordSPtr = &record;
    LONG64 bufferedRecordsSizebytes = 0;

    switch (recordSPtr->RecordType)
    {
    case LogRecordType::Enum::BeginTransaction:
    case LogRecordType::Enum::Operation:
    case LogRecordType::Enum::EndTransaction:
        status = replicatedLogManager_->ReplicateAndLog(*recordSPtr, bufferedRecordsSizebytes, this);
        break;
    default:
        ASSERT_IFNOT(
            false,
            "{0}: InitiateLogicalRecordProcessingOnPrimary | Invalid record type {1}",
            TraceId,
            record.RecordType);
        break;
    }

    RETURN_ON_FAILURE(status);

    bool addedIndexingRecord = false;
    bool addedTruncateHeadRecord = false;

    checkpointManager_->InsertPhysicalRecordsIfNecessary(
        addedIndexingRecord,
        addedTruncateHeadRecord);

    if (recordSPtr->IsLatencySensitiveRecord || addedTruncateHeadRecord)
    {
        checkpointManager_->RequestGroupCommit();
    }
    else if (bufferedRecordsSizebytes > (transactionalReplicatorConfig_->MaxRecordSizeInKB * 1024))
    {
        Awaitable<void> flushTask = replicatedLogManager_->InnerLogManager->FlushAsync(L"InitiateLogicalRecordProcessingOnPrimary");
        ToTask(flushTask);
    }

    return status;
}

Awaitable<NTSTATUS> TransactionManager::EndTransactionAsync(
    __in TransactionBase & transaction,
    __in bool isCommitted,
    __in bool isThisReplicaTransaction,
    __out EndTransactionLogRecord::SPtr & result) noexcept
{
    result = EndTransactionLogRecord::Create(
        transaction,
        isCommitted,
        isThisReplicaTransaction,
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        *invalidLogRecords_->Inv_TransactionLogRecord,
        GetThisAllocator());

    NTSTATUS status = ReplicateAndLogLogicalRecord(*result);

    CO_RETURN_ON_FAILURE(status);

    status = co_await ProcessLogicalRecordOnPrimaryAsync(*result);

    co_return status;
}

// NOTE - In managed code, this method had the replicate call while in native we do not have it here
// This is because we cannot throw out of this function if beginreplicate fails as Task's dont expect an exception to be thrown
Task TransactionManager::ProcessLogicalRecordOnPrimary(__in TransactionLogRecord & record)
{
    KCoShared$ApiEntry();
    TransactionLogRecord::SPtr recordSPtr = &record;

    NTSTATUS status1 = co_await recordSPtr->AwaitApply();
    NTSTATUS status2 = co_await replicatedLogManager_->AwaitReplication(*recordSPtr);

    // Keeping these variables in case we need for debugging
    UNREFERENCED_PARAMETER(status1);
    UNREFERENCED_PARAMETER(status2);

    InvokeUnlockCallback(*recordSPtr);
    co_return;
}

Awaitable<NTSTATUS> TransactionManager::ProcessLogicalRecordOnPrimaryAsync(__in TransactionLogRecord & record) noexcept
{
    TransactionLogRecord::SPtr recordSPtr = &record;

    NTSTATUS status1 = co_await recordSPtr->AwaitApply();
    NTSTATUS status2 = co_await replicatedLogManager_->AwaitReplication(*recordSPtr);

    NTSTATUS status = FIRST_NTSTATUS_ERROR(status1, status2);

    // Unlock to ensure any pending operations are released
    InvokeUnlockCallback(*recordSPtr);

    if (!NT_SUCCESS(status))
    {
        // Use a SF_STATUS_TRANSACTION_ABORTED error code to maintain the exception contract where user can expect a specific set of errors
        co_return SF_STATUS_TRANSACTION_ABORTED;
    }

    co_return STATUS_SUCCESS;
}

NTSTATUS TransactionManager::ReplicateAndLogLogicalRecord(__in TransactionLogRecord & record) noexcept
{
    return InitiateLogicalRecordProcessingOnPrimary(record);
}

void TransactionManager::InvokeUnlockCallback(__in TransactionLogRecord & record) noexcept
{
    operationProcessor_->Unlock(record);
}

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

Common::StringLiteral const TraceComponent("TruncateTailManager");

TruncateTailManager::TruncateTailManager(
    __in PartitionedReplicaId const & traceId,
    __in ReplicatedLogManager & replicatedLogManager,
    __in TransactionMap & transactionsMap,
    __in IStateProviderManager & stateManager,
    __in IBackupManager & backupManager,
    __in LONG64 tailLsn,
    __in ApplyContext::Enum falseProgressApplyContext,
    __in RecoveryPhysicalLogReader & recoveryLogsReader,
    __in InvalidLogRecords & invalidLogRecords)
    : KShared()
    , KObject()
    , PartitionedReplicaTraceComponent(traceId)
    , replicatedLogManager_(&replicatedLogManager)
    , transactionsMap_(&transactionsMap)
    , stateManager_(&stateManager)
    , backupManager_(&backupManager)
    , tailLsn_(tailLsn)
    , falseProgressApplyContext_(falseProgressApplyContext)
    , recoveryLogsReader_(&recoveryLogsReader)
    , invalidRecords_(&invalidLogRecords)
{
    EventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        L"TruncateTailManager",
        reinterpret_cast<uintptr_t>(this));
}

TruncateTailManager::~TruncateTailManager()
{
    EventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        L"TruncateTailManager",
        reinterpret_cast<uintptr_t>(this));
}

TruncateTailManager::SPtr TruncateTailManager::Create(
    __in PartitionedReplicaId const & traceId,
    __in ReplicatedLogManager & replicatedLogManager,
    __in TransactionMap & transactionsMap,
    __in IStateProviderManager & stateManager,
    __in IBackupManager & backupManager,
    __in LONG64 tailLsn,
    __in ApplyContext::Enum falseProgressApplyContext,
    __in RecoveryPhysicalLogReader & recoveryLogsReader,
    __in InvalidLogRecords & invalidLogRecords,
    __in KAllocator & allocator)
{
    TruncateTailManager * pointer = _new(TRUNCATETAILMANAGER_TAG, allocator) TruncateTailManager(
        traceId,
        replicatedLogManager,
        transactionsMap,
        stateManager,
        backupManager,
        tailLsn,
        falseProgressApplyContext,
        recoveryLogsReader,
        invalidLogRecords);

    THROW_ON_ALLOCATION_FAILURE(pointer);

    return TruncateTailManager::SPtr(pointer);
}

Awaitable<NTSTATUS> TruncateTailManager::TruncateTailAsync(__out LogRecord::SPtr & newTail) noexcept
{
    ASSERT_IFNOT(
        tailLsn_ < replicatedLogManager_->CurrentLogTailLsn,
        "{0} tailLsn < this.currentLogTailLsn. Current log tail lsn: {1}",
        TraceId,
        replicatedLogManager_->CurrentLogTailLsn);

    ASSERT_IFNOT(
        tailLsn_ >= replicatedLogManager_->LastCompletedBeginCheckpointRecord->Lsn,
        "{0} tailLsn >= this.LastCompletedBeginCheckpointRecord.LastLogicalSequenceNumber. LastCompletedBeginCheckpointLogRecord: {1}",
        TraceId,
        replicatedLogManager_->LastCompletedBeginCheckpointRecord->Lsn);

    ASSERT_IFNOT(
        replicatedLogManager_->LastInProgressTruncateHeadRecord == nullptr,
        "{0} this.lastInProgressTruncateHeadRecord == null",
        TraceId);

    LogRecord::SPtr currentRecord = replicatedLogManager_->InnerLogManager->CurrentLogTailRecord;
    LONG64 currentLsn = currentRecord->Lsn;

    NTSTATUS status = STATUS_SUCCESS;
    bool isUpdateRecordAtTail = true;
    LONG64 recoveredLsn = currentLsn;
    EndCheckpointLogRecord::SPtr endCheckpointLogRecord = nullptr;
    CompleteCheckPointLogRecord::SPtr completeCheckpointLogRecord = nullptr;
    PhysicalLogRecord::SPtr lastPhysicalRecord = replicatedLogManager_->InnerLogManager->CurrentLastPhysicalRecord;

    // This loop traverses the log backwards until the target tail lsn is reached while
    // safely removing records after the target tail lsn
    //
    // Removal of each record updates the transactions map. If the record represents an atomic operation 
    // or single operation transaction, ApplyAsync is called on the StateProviderManager with the provided 
    // falseProgressApplyContext (performing 'undo' on operation). StateProviderManager.Unlock(..) is then 
    // called with the OperationContext object returned from ApplyAsync.
    //
    // When undoing a committed EndTransactionLogRecord, all records associated with the transaction are undone 
    // and unlocked. They are not removed from the transaction map until the individual record is 
    // encountered as part of the backwards log traversal. 
    do
    {
        ASSERT_IF(
            LogRecord::IsInvalid(currentRecord.RawPtr()),
            "{0} LogRecord.IsInvalidRecord(currentRecord.RawPtr()) is invalid",
            TraceId);

        if(isUpdateRecordAtTail)
        {
            isUpdateRecordAtTail = currentRecord->Lsn == recoveredLsn;
        }

        OperationData::CSPtr metaData = nullptr;

        switch(currentRecord->RecordType)
        {
        case LogRecordType::Enum::BeginTransaction:
        {
            BeginTransactionOperationLogRecord::SPtr beginTransactionRecord = dynamic_cast<BeginTransactionOperationLogRecord *>(currentRecord.RawPtr());
            ASSERT_IF(beginTransactionRecord == nullptr, "Unexpected dynamic cast failure");

            // Cache the latest metadata just read from disk
            metaData = beginTransactionRecord->Metadata;

            // Remove this transaction from the transactions map
            beginTransactionRecord = transactionsMap_->DeleteTransaction(*beginTransactionRecord);

            // Reset the metadata of the transaction as it may have been modified during redo pass
            beginTransactionRecord->Metadata = *metaData;

            // If the begin transaction record represents a single operation transaction, it is undone here.
            // Transactions with more than one operation are undone in the EndTransaction case.
            if(beginTransactionRecord->IsSingleOperationTransaction)
            {
                ASSERT_IFNOT(
                    beginTransactionRecord->Lsn != Constants::InvalidLsn,
                    "{0} begin transaction record lsn must not be invalid",
                    TraceId);

                // Set the commit sequence number of the associated transaction to the current record LSN
                beginTransactionRecord->BaseTransaction.CommitSequenceNumber = beginTransactionRecord->Lsn;

                // Apply the updated begin transaction record with the associated false progress apply context
                OperationContext::CSPtr operationContext = nullptr;
                
                status = co_await stateManager_->ApplyAsync(
                    beginTransactionRecord->Lsn,
                    beginTransactionRecord->BaseTransaction,
                    falseProgressApplyContext_, 
                    beginTransactionRecord->Metadata.RawPtr(),
                    beginTransactionRecord->Undo.RawPtr(),
                    operationContext);
                
                CO_RETURN_ON_FAILURE(status);

                if(operationContext != nullptr)
                {
                    status = stateManager_->Unlock(*operationContext);

                    CO_RETURN_ON_FAILURE(status);
                }

                EventSource::Events->TruncateTailSingleOperationTransactionRecord(
                    TracePartitionId,
                    ReplicaId,
                    "Deleted",
                    beginTransactionRecord->Lsn,
                    beginTransactionRecord->Psn,
                    beginTransactionRecord->RecordPosition,
                    beginTransactionRecord->BaseTransaction.TransactionId);
            }
            else
            {
                EventSource::Events->TruncateTailTransactionRecord(
                    TracePartitionId,
                    ReplicaId,
                    "Deleted",
                    beginTransactionRecord->Lsn,
                    beginTransactionRecord->Psn,
                    beginTransactionRecord->RecordPosition,
                    beginTransactionRecord->BaseTransaction.TransactionId);
            }
            
            // Needed for keeping the transaction chain from not disappearing during destruction
            beginTransactionRecord->ParentTransactionRecord = invalidRecords_->Inv_TransactionLogRecord.RawPtr();
            break;
        }
        case LogRecordType::Enum::Operation:
        {
            OperationLogRecord::SPtr operationRecord = dynamic_cast<OperationLogRecord *>(currentRecord.RawPtr());
            ASSERT_IF(operationRecord == nullptr, "Unexpected dynamic cast failure");

            // Cache the latest metadata just read from disk
            metaData = operationRecord->Metadata;

            // Remove this operation from the transactions map
            // Updates the latest record associated with the transaction to the parent record
            operationRecord = transactionsMap_->RedactOperation(*operationRecord, *invalidRecords_->Inv_TransactionLogRecord);

            // Reset the metadata of the operation as it may have been modified during redo pass
            operationRecord->Metadata = *metaData;

            // If this operation log record represents an atomic operation, it is undone here
            // Operation log records associated with a transaction are undone in the EndTransaction case
            if(operationRecord->BaseTransaction.IsAtomicOperation)
            {
                ASSERT_IF(
                    operationRecord->IsRedoOnly,
                    "{0} TruncateTail - RedoOnly operation cannot be undone",
                    TraceId);

                ASSERT_IF(
                    operationRecord->Lsn == Constants::InvalidLsn,
                    "{0} Operation must have valid LSN",
                    TraceId);

                // Set the commit sequence number of the associated transaction to the current record LSN
                operationRecord->BaseTransaction.CommitSequenceNumber = operationRecord->Lsn;

                // Apply the updated begin transaction record with the associated false progress apply context
                OperationContext::CSPtr operationContext = nullptr;
                
                status = co_await stateManager_->ApplyAsync(
                    operationRecord->Lsn,
                    operationRecord->BaseTransaction,
                    falseProgressApplyContext_,
                    operationRecord->Metadata.RawPtr(),
                    operationRecord->Undo.RawPtr(),
                    operationContext);

                CO_RETURN_ON_FAILURE(status);

                if(operationContext != nullptr)
                {
                    status = stateManager_->Unlock(*operationContext);

                    CO_RETURN_ON_FAILURE(status);
                }

                EventSource::Events->TruncateTailAtomicOperation(
                    TracePartitionId,
                    ReplicaId,
                    operationRecord->Lsn,
                    operationRecord->Psn,
                    operationRecord->RecordPosition,
                    operationRecord->BaseTransaction.TransactionId);
            }
            else
            {
                EventSource::Events->TruncateTailOperationRecord(
                    TracePartitionId,
                    ReplicaId,
                    "Deleted",
                    operationRecord->Lsn,
                    operationRecord->Psn,
                    operationRecord->RecordPosition,
                    operationRecord->BaseTransaction.TransactionId);
            }

            // Needed for keeping the transaction chain from not disappearing during destruction
            operationRecord->ParentTransactionRecord = invalidRecords_->Inv_TransactionLogRecord.RawPtr();
            break;
        }
        case LogRecordType::Enum::EndTransaction:
        {
            EndTransactionLogRecord::SPtr endTransactionRecord = dynamic_cast<EndTransactionLogRecord *>(currentRecord.RawPtr());
            ASSERT_IF(
                endTransactionRecord == nullptr,
                "{0}: Unexpected dynamic cast failure",
                TraceId);

            endTransactionRecord = transactionsMap_->ReifyTransaction(*endTransactionRecord, *invalidRecords_->Inv_TransactionLogRecord);

            OperationLogRecord::SPtr operationRecord;

            ASSERT_IF(
                endTransactionRecord->Lsn == Constants::InvalidLsn,
                "{0} End transaction record cannot have an invalid lsn.",
                TraceId);

            // Undo all operations in this transaction (call ApplyAsync with undo).
            if(endTransactionRecord->IsCommitted)
            {
                TransactionLogRecord::SPtr transactionRecord = endTransactionRecord.RawPtr();

                OperationContext::CSPtr operationContext = nullptr;

                do
                {
                    // During recovery operations that may be undone are kept in memory.
                    // Since Truncate tail uses the in-memory links, Transaction have already been created and their commit sequence numbers
                    // have been set during recovery redo.
                    ASSERT_IFNOT(
                        transactionRecord->RecordType == LogRecordType::EndTransaction || 
                        transactionRecord->BaseTransaction.CommitSequenceNumber != Constants::InvalidLsn, 
                        "{0} For an operation to be undone, it must have already been done.", 
                        TraceId);

                    // Move to the parent record and continue traversing this transaction
                    transactionRecord = transactionRecord->ParentTransactionRecord;

                    ASSERT_IF(
                        transactionRecord == nullptr,
                        "{0} transactionRecord != nullptr",
                        TraceId)

                    // Process the begin transaction record associated with endTransactionRecord
                    if(transactionRecord->RecordType == LogRecordType::BeginTransaction)    
                    {
                        // Cache the metadata read from disk
                        LogRecord::SPtr justReadTransactionRecord = co_await recoveryLogsReader_->GetNextLogRecord(transactionRecord->RecordPosition);

                        ASSERT_IF(
                            justReadTransactionRecord->RecordType != LogRecordType::BeginTransaction,
                            "{0} Just read transaction during false progress is not begintransaction. It is {1}",
                            TraceId,
                            justReadTransactionRecord->RecordType);

                        BeginTransactionOperationLogRecord::SPtr justReadBeginTxRecord = dynamic_cast<BeginTransactionOperationLogRecord *>(justReadTransactionRecord.RawPtr());
                        ASSERT_IF(
                            justReadBeginTxRecord == nullptr,
                            "{0}: Unexpected dynamic cast failure",
                            TraceId);

                        BeginTransactionOperationLogRecord::SPtr beginTxRecord = dynamic_cast<BeginTransactionOperationLogRecord *>(transactionRecord.RawPtr());
                        ASSERT_IF(
                            beginTxRecord == nullptr,
                            "{0}: Unexpected dynamic cast failure",
                            TraceId);

                        beginTxRecord->Metadata = *justReadBeginTxRecord->Metadata;

                        ASSERT_IF(
                            beginTxRecord->IsSingleOperationTransaction,
                            "{0} beginTx.IsSingleOperationTransaction must be false when endTxRecord is being processed",
                            TraceId);

                        status = co_await stateManager_->ApplyAsync(
                            beginTxRecord->Lsn,
                            beginTxRecord->BaseTransaction,
                            falseProgressApplyContext_,
                            beginTxRecord->Metadata.RawPtr(),
                            beginTxRecord->Undo.RawPtr(),
                            operationContext);

                        CO_RETURN_ON_FAILURE(status);

                        if(operationContext != nullptr)
                        {
                            beginTxRecord->OperationContextValue = *operationContext;
                        }

                        // The BeginTransactionOperationLogRecord associated with endTransactionRecord has been processed
                        break;
                    }

                    // Process an operation log record associated with endTransactionRecord
                    // Cache the metadata read from disk
                    LogRecord::SPtr justReadOperationLogecord = co_await recoveryLogsReader_->GetNextLogRecord(transactionRecord->RecordPosition);

                    ASSERT_IF(
                        justReadOperationLogecord->RecordType != LogRecordType::Operation,
                        "{0} Just read operation during false progress is not of the right type. It is {1}",
                        TraceId,
                        justReadOperationLogecord->RecordType);

                    OperationLogRecord::SPtr justReadOperationRecord = dynamic_cast<OperationLogRecord *>(justReadOperationLogecord.RawPtr());
                    ASSERT_IF(
                        justReadOperationRecord == nullptr,
                        "{0}: Unexpected dynamic cast failure",
                        TraceId);
                            
                    operationRecord = dynamic_cast<OperationLogRecord *>(transactionRecord.RawPtr());
                    ASSERT_IF(
                        operationRecord == nullptr,
                        "{0}: Unexpected dynamic cast failure",
                        TraceId);

                    operationRecord->Metadata = *justReadOperationRecord->Metadata;

                    // Undo the operation
                    status = co_await stateManager_->ApplyAsync(
                        operationRecord->Lsn,
                        operationRecord->BaseTransaction,
                        falseProgressApplyContext_,
                        operationRecord->Metadata.RawPtr(),
                        operationRecord->Undo.RawPtr(),
                        operationContext);

                    CO_RETURN_ON_FAILURE(status);

                    if(operationContext != nullptr)
                    {
                        operationRecord->OperationContextValue = *operationContext;
                    }

                } while (true);

                // Call unlock for OperationLogRecords and the BeginTransactionLogRecord associated with endTransactionRecord
                transactionRecord = endTransactionRecord.RawPtr();
                do
                {
                    OperationContext::CSPtr opContext = nullptr;
                    transactionRecord = transactionRecord->ParentTransactionRecord;

                    ASSERT_IF(
                        transactionRecord == nullptr,
                        "{0} transactionRecord != nullptr",
                        TraceId)

                    if(transactionRecord->RecordType == LogRecordType::BeginTransaction)
                    {
                        BeginTransactionOperationLogRecord::SPtr beginTxRecord = dynamic_cast<BeginTransactionOperationLogRecord *>(transactionRecord.RawPtr());
                        ASSERT_IF(
                            beginTxRecord == nullptr,
                            "{0}: Unexpected dynamic cast failure",
                            TraceId);

                        // Retrieves the current OperationContext associated with beginTxRecord,and resets its value
                        opContext = beginTxRecord->ResetOperationContext();

                        if (opContext != nullptr)
                        {
                            stateManager_->Unlock(*opContext);
                        }

                        EventSource::Events->TruncateTailOperationRecord(
                            TracePartitionId,
                            ReplicaId,
                            "Undone",
                            beginTxRecord->Lsn,
                            beginTxRecord->Psn,
                            beginTxRecord->RecordPosition,
                            beginTxRecord->BaseTransaction.TransactionId);

                        break;
                    }

                    operationRecord = dynamic_cast<OperationLogRecord *>(transactionRecord.RawPtr());
                    ASSERT_IF(
                        operationRecord == nullptr,
                        "{0}: Unexpected dynamic cast failure",
                        TraceId);

                    // Retrieves the current OperationContext associated with operationRecord,and resets its value
                    opContext = operationRecord->ResetOperationContext();

                    if(opContext != nullptr)
                    {
                        stateManager_->Unlock(*opContext);
                    }

                    EventSource::Events->TruncateTailOperationRecord(
                        TracePartitionId,
                        ReplicaId,
                        "Undone",
                        operationRecord->Lsn,
                        operationRecord->Psn,
                        operationRecord->RecordPosition,
                        operationRecord->BaseTransaction.TransactionId);

                } while (true);
            }

            EventSource::Events->TruncateTailTransactionRecord(
                TracePartitionId,
                ReplicaId,
                "Reified",
                endTransactionRecord->Lsn,
                endTransactionRecord->Psn,
                endTransactionRecord->RecordPosition,
                endTransactionRecord->BaseTransaction.TransactionId);

            // Needed for keeping the transaction chain from not disappearing during destruction
            endTransactionRecord->ParentTransactionRecord = invalidRecords_->Inv_TransactionLogRecord.RawPtr();
            break;
        }
        case LogRecordType::Enum::Barrier:
        {
            BarrierLogRecord::SPtr barrierRecord = dynamic_cast<BarrierLogRecord *>(currentRecord.RawPtr());
            ASSERT_IF(
                barrierRecord == nullptr,
                "{0}: Unexpected dynamic cast failure",
                TraceId);

            EventSource::Events->TruncateTailBarrier(
                TracePartitionId,
                ReplicaId,
                barrierRecord->Lsn,
                barrierRecord->Psn,
                barrierRecord->RecordPosition);

            break;
        }
        case LogRecordType::Enum::Backup:
        {
            // Inform the backup manager that the last backup log record has been undone.
            backupManager_->UndoLastCompletedBackupLogRecord();

            // Trace that the backup log record has been false progressed.
            BackupLogRecord::SPtr backupRecord = dynamic_cast<BackupLogRecord *>(currentRecord.RawPtr());
            ASSERT_IF(
                backupRecord == nullptr,
                "{0}: Unexpected dynamic cast failure",
                TraceId);

            EventSource::Events->TruncateTailBackup(
                TracePartitionId,
                ReplicaId,
                backupRecord->Lsn,
                backupRecord->Psn,
                backupRecord->RecordPosition);

            break;
        }
        case LogRecordType::Enum::UpdateEpoch:
        {
            // These records can only occur at the tail
            ASSERT_IFNOT(
                isUpdateRecordAtTail, 
                "{0} isUpdateRecordAtTail is false", 
                TraceId);

            UpdateEpochLogRecord::CSPtr updateEpochRecord = dynamic_cast<UpdateEpochLogRecord *>(currentRecord.RawPtr());
            ASSERT_IF(
                updateEpochRecord == nullptr,
                "{0}: Unexpected dynamic cast failure",
                TraceId);

            ProgressVectorEntry lastVector(*updateEpochRecord);

            // Truncate the progress vector in the replicated log manager
            // Ensures outdated progress vector entries are not maintained on the secondary replica
            replicatedLogManager_->ProgressVectorValue->TruncateTail(lastVector);

            EventSource::Events->TruncateTailUpdateEpoch(
                TracePartitionId,
                ReplicaId,
                updateEpochRecord->EpochValue.DataLossVersion,
                updateEpochRecord->EpochValue.ConfigurationVersion,
                updateEpochRecord->Lsn,
                updateEpochRecord->Psn,
                updateEpochRecord->RecordPosition);

            break;
        }
        case LogRecordType::Enum::EndCheckpoint:
        {
            ASSERT_IF(
                currentRecord->Psn != replicatedLogManager_->LastCompletedEndCheckpointRecord->Psn,
                "{0} currentRecord.Psn({1}) != this.lastCompletedEndCheckpointRecord.Psn({2})",
                TraceId,
                currentRecord->Psn,
                replicatedLogManager_->LastCompletedEndCheckpointRecord->Psn);

            ASSERT_IF(
                currentRecord->Psn != replicatedLogManager_->LastLinkedPhysicalRecord->Psn,
                "{0} currentRecord.Psn({1}) != this.lastLinkedPhysicalRecord.Psn({2})",
                TraceId,
                currentRecord->Psn,
                replicatedLogManager_->LastLinkedPhysicalRecord->Psn);

            endCheckpointLogRecord = replicatedLogManager_->LastCompletedEndCheckpointRecord;

            // Update the last linked physical record
            replicatedLogManager_->OnTruncateTailOfLastLinkedPhysicalRecord();
            goto LastPhysicalRecordCase;
        }
        case LogRecordType::Enum::TruncateHead:
        {
            ASSERT_IF(
                currentRecord->Psn != replicatedLogManager_->LastLinkedPhysicalRecord->Psn,
                "{0} currentRecord.Psn({1}) != this.lastLinkedPhysicalRecord.Psn({2})",
                TraceId,
                currentRecord->Psn,
                replicatedLogManager_->LastLinkedPhysicalRecord->Psn);

            TruncateHeadLogRecord::SPtr truncateHeadRecord = dynamic_cast<TruncateHeadLogRecord *>(currentRecord.RawPtr());
            ASSERT_IF(truncateHeadRecord == nullptr, "Unexpected dynamic cast failure");

            ASSERT_IF(
                truncateHeadRecord->IsStable,
                "{0} Stable truncateHeadRecord cannot be undone due to false progress",
                TraceId);

            // Update the last linked physical record
            replicatedLogManager_->OnTruncateTailOfLastLinkedPhysicalRecord();
            goto LastPhysicalRecordCase;
        }
        case LogRecordType::Enum::CompleteCheckpoint:
        {
            completeCheckpointLogRecord = dynamic_cast<CompleteCheckPointLogRecord *>(currentRecord.RawPtr());
            ASSERT_IF(
                completeCheckpointLogRecord == nullptr,
                "{0}: Unexpected dynamic cast failure",
                TraceId);

            // Update the last linked physical record
            replicatedLogManager_->OnTruncateTailOfLastLinkedPhysicalRecord();
            goto LastPhysicalRecordCase;
        }
        case LogRecordType::Enum::Indexing:
        case LogRecordType::Enum::BeginCheckpoint:
        case LogRecordType::Enum::TruncateTail:
        case LogRecordType::Enum::Information:
        LastPhysicalRecordCase:
        {
            // The current record must be equal to the last physical record
            ASSERT_IF(
                currentRecord->Psn != lastPhysicalRecord->Psn,
                "{0} currentRecord.Psn({1}) != lastPhysicalRecord.Psn({2})",
                TraceId,
                currentRecord->Psn,
                lastPhysicalRecord->Psn);
            
            PhysicalLogRecord::SPtr current = lastPhysicalRecord;
            // Move up the last physical record to the previous physical record
            lastPhysicalRecord = lastPhysicalRecord->PreviousPhysicalRecord;

            // Needed for keeping the physical chain from not disappearing during destruction of the physical record
            current->PreviousPhysicalRecord = invalidRecords_->Inv_PhysicalLogRecord.RawPtr();
            break;
        }

        case LogRecordType::Enum::Invalid:
        default:
            {
                Common::Assert::CodingError(
                    "{0} Unexpected record type {1}",
                    TraceId,
                    currentRecord->RecordType);
            }
        }

        currentRecord = co_await recoveryLogsReader_->GetPreviousLogRecord(currentRecord->RecordPosition);
        currentLsn = currentRecord->Lsn;

    } while (currentLsn > tailLsn_);

    ASSERT_IF(
        currentLsn != tailLsn_,
        "{0} V1 replicator ensures that lsns are continuous. currentLsn {1} == tailLsn {2}",
        TraceId,
        currentLsn,
        tailLsn_);

    if(currentRecord->AsLogicalLogRecord() != nullptr)
    {
        switch(currentRecord->RecordType)
        {
        case LogRecordType::Enum::BeginTransaction:
        {
            BeginTransactionOperationLogRecord::SPtr beginTransactionRecord = dynamic_cast<BeginTransactionOperationLogRecord *>(currentRecord.RawPtr());
            ASSERT_IF(
                beginTransactionRecord == nullptr,
                "{0}: Unexpected dynamic cast failure",
                TraceId);

            currentRecord = transactionsMap_->FindTransaction(*beginTransactionRecord).RawPtr();

            // Single operation transactions are not stored in the tx map and hence are not returned above. 
            // As a result, they dont have a valid indexing of the previous physical record
            if(beginTransactionRecord->IsSingleOperationTransaction)
            {
                currentRecord->PreviousPhysicalRecord = lastPhysicalRecord.RawPtr();
            }

            break;
        }
        case LogRecordType::Enum::Operation:
        {
            OperationLogRecord::SPtr operationLogRecord = dynamic_cast<OperationLogRecord *>(currentRecord.RawPtr());
            ASSERT_IF(
                operationLogRecord == nullptr,
                "{0}: Unexpected dynamic cast failure",
                TraceId);

            currentRecord = transactionsMap_->FindOperation(*operationLogRecord).RawPtr();
            
            // Atomic operations are not stored in the tx map and hence are not returned above. 
            // As a result, they dont have a valid indexing of the previous physical record
            if(operationLogRecord->BaseTransaction.IsAtomicOperation)
            {
                currentRecord->PreviousPhysicalRecord = lastPhysicalRecord.RawPtr();
            }

            break;
        }
        case LogRecordType::Enum::EndTransaction:
        {
            EndTransactionLogRecord::SPtr endTransactionRecord = dynamic_cast<EndTransactionLogRecord *>(currentRecord.RawPtr());
            ASSERT_IF(
                endTransactionRecord == nullptr,
                "{0}: Unexpected dynamic cast failure",
                TraceId);

            currentRecord = transactionsMap_->FindUnstableTransaction(*endTransactionRecord).RawPtr();
            break;
        }
        case LogRecordType::Enum::Backup:
        case LogRecordType::Enum::Barrier:
        case LogRecordType::Enum::UpdateEpoch:
        {
            currentRecord->PreviousPhysicalRecord = lastPhysicalRecord.RawPtr();
            break;
        }
        default:
        {
            Common::Assert::CodingError(
                "{0} Unexpected record type {1}",
                TraceId,
                currentRecord->RecordType);
        }
        }

        ASSERT_IF(
            currentRecord->PreviousPhysicalRecord != lastPhysicalRecord,
            "{0} currentRecord.PreviousPhysicalRecord != lastPhysicalRecord",
            TraceId);
    }
    else
    {
        ASSERT_IF(
            lastPhysicalRecord->Psn != currentRecord->Psn,
            "{0} lastPhysicalRecord.Psn({1}) != currentRecord.Psn({2})",
            TraceId,
            lastPhysicalRecord->Psn,
            currentRecord->Psn);

        currentRecord = lastPhysicalRecord.RawPtr();
    }

    ASSERT_IFNOT(
        (replicatedLogManager_->LastLinkedPhysicalRecord == lastPhysicalRecord) ||
        (replicatedLogManager_->LastLinkedPhysicalRecord == lastPhysicalRecord->LinkedPhysicalRecord),
        "{0} (this.lastLinkedPhysicalRecord == lastPhysicalRecord) || (this.lastLinkedPhysicalRecord == lastPhysicalRecord.LinkedPhysicalRecord)",
        TraceId);

    // Perform the log truncation
    status = co_await replicatedLogManager_->InnerLogManager->PerformLogTailTruncationAsync(*currentRecord);
    
    CO_RETURN_ON_FAILURE(status);

    // Update the current tail lsn
    replicatedLogManager_->SetTailLsn(tailLsn_);

    // If end checkpoint was truncated, complete checkpoint must also be truncated
    if(endCheckpointLogRecord != nullptr)
    {
        ASSERT_IF(
            completeCheckpointLogRecord == nullptr,
            "{0} TruncateTailAsync: EndCheckpoint was truncated but CompleteCheckpoint was not",
            TraceId);

        replicatedLogManager_->EndCheckpoint(*endCheckpointLogRecord->LastCompletedBeginCheckpointRecord);
    }

    if(completeCheckpointLogRecord != nullptr)
    {
        replicatedLogManager_->CompleteCheckpoint();
    }

    // Insert and flush the truncate tail log record after successful truncation
    replicatedLogManager_->TruncateTail(tailLsn_);
    
    co_await replicatedLogManager_->InnerLogManager->FlushAsync(L"TruncateTailAsync");

    EventSource::Events->TruncateTailDone(
        TracePartitionId,
        ReplicaId,
        currentRecord->RecordType,
        currentRecord->Lsn,
        currentRecord->Psn,
        currentRecord->RecordPosition);

    newTail = currentRecord;

    co_return status;
}

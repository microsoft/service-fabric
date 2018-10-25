// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System.Fabric;
    using System.Threading.Tasks;
    using System.Fabric.Common.Tracing;
    using Backup;

    internal class TruncateTailManager
    {
        private readonly ReplicatedLogManager replicatedLogManager;
        private readonly TransactionMap transactionsMap;
        private readonly IStateManager stateManager;
        private readonly IBackupManager backupManager;
        private readonly ITracer tracer;
        private readonly LogicalSequenceNumber tailLsn;
        private readonly ApplyContext falseProgressApplyContext;
        private readonly PhysicalLogReader recoveryLogsReader;

        public TruncateTailManager(
            ReplicatedLogManager replicatedLogManager,
            TransactionMap transactionsMap,
            IStateManager stateManager,
            IBackupManager backupManager,
            LogicalSequenceNumber tailLsn,
            ApplyContext falseProgressApplyContext,
            PhysicalLogReader recoveryLogsReader,
            ITracer tracer)
        {
            Utility.Assert(tracer != null, "{0} TruncateTailManager: Input tracer cannot be null");
            Utility.Assert(backupManager != null, "{0} TruncateTailManager: Input backupManager cannot be null", tracer.Type);

            this.replicatedLogManager = replicatedLogManager;
            this.recoveryLogsReader = recoveryLogsReader;
            this.transactionsMap = transactionsMap;
            this.stateManager = stateManager;
            this.backupManager = backupManager;
            this.tracer = tracer;
            this.tailLsn = tailLsn;
            this.falseProgressApplyContext = falseProgressApplyContext;
        }

        public async Task<LogRecord> TruncateTailAsync()
        {
            Utility.Assert(
                tailLsn < this.replicatedLogManager.CurrentLogTailLsn,
                "tailLsn < this.currentLogTailLsn. Current log tail lsn: {0}",
                this.replicatedLogManager.CurrentLogTailLsn.LSN);

            Utility.Assert(
                tailLsn >= this.replicatedLogManager.LastCompletedBeginCheckpointRecord.Lsn,
                "tailLsn >= this.LastCompletedBeginCheckpointRecord.LastLogicalSequenceNumber. LastCompletedBeginCheckpointLogRecord: {0}",
                this.replicatedLogManager.LastCompletedBeginCheckpointRecord.Lsn);

            Utility.Assert(
                this.replicatedLogManager.LastInProgressTruncateHeadRecord == null,
                "this.lastInProgressTruncateHeadRecord == null");

            var currentRecord = this.replicatedLogManager.LogManager.CurrentLogTailRecord;
            var currentLsn = currentRecord.Lsn;

            var isUpdateRecordAtTail = true;
            var recoveredLsn = currentLsn;
            EndCheckpointLogRecord endCheckpointLogRecord = null;
            CompleteCheckpointLogRecord completeCheckpointLogRecord = null;
            var lastPhysicalRecord = this.replicatedLogManager.LogManager.CurrentLastPhysicalRecord;
            do
            {
                Utility.Assert(
                    LogRecord.IsInvalidRecord(currentRecord) == false,
                    "LogRecord.IsInvalidRecord(currentRecord ({0})) == false",
                    currentRecord);

                if (isUpdateRecordAtTail == true)
                {
                    isUpdateRecordAtTail = currentRecord.Lsn == recoveredLsn;
                }

                OperationData metaData = null;

                switch (currentRecord.RecordType)
                {
                    case LogRecordType.BeginTransaction:
                        var beginTransactionRecord = (BeginTransactionOperationLogRecord) currentRecord;

                        // Cache the latest metadata just read from disk
                        metaData = beginTransactionRecord.MetaData;

                        beginTransactionRecord = this.transactionsMap.DeleteTransaction(beginTransactionRecord);

                        // Reset the metadata of the transaction as it may have been modified during redo pass
                        beginTransactionRecord.MetaData = metaData;

                        if (beginTransactionRecord.IsSingleOperationTransaction)
                        {
                            Utility.Assert(
                                beginTransactionRecord.Lsn != LogicalSequenceNumber.InvalidLsn,
                                "begin transaction record lsn must not be invalid.");

                            beginTransactionRecord.Transaction.CommitSequenceNumber = beginTransactionRecord.Lsn.LSN;

                            var operationContext =
                                await
                                    this.stateManager.OnApplyAsync(
                                        beginTransactionRecord.Lsn.LSN,
                                        beginTransactionRecord.Transaction,
                                        beginTransactionRecord.MetaData,
                                        beginTransactionRecord.Undo,
                                        falseProgressApplyContext).ConfigureAwait(false);

                            if (operationContext != null)
                            {
                                this.stateManager.Unlock(operationContext);
                            }

                            FabricEvents.Events.TruncateTailSingleOperationTransactionRecord(
                                this.tracer.Type,
                                "Deleted",
                                beginTransactionRecord.Lsn.LSN,
                                beginTransactionRecord.Psn.PSN,
                                beginTransactionRecord.RecordPosition,
                                beginTransactionRecord.Transaction.Id);
                        }
                        else
                        {
                            FabricEvents.Events.TruncateTailTransactionRecord(
                                this.tracer.Type,
                                "Deleted",
                                beginTransactionRecord.Lsn.LSN,
                                beginTransactionRecord.Psn.PSN,
                                beginTransactionRecord.RecordPosition,
                                beginTransactionRecord.Transaction.Id);
                        }

                        break;

                    case LogRecordType.Operation:
                        var operationRecord = (OperationLogRecord) currentRecord;

                        // Cache the latest metadata just read from disk
                        metaData = operationRecord.MetaData;

                        operationRecord = this.transactionsMap.RedactOperation(operationRecord);

                        // Reset the metadata of the operation as it may have been modified during redo pass
                        operationRecord.MetaData = metaData;

                        if (operationRecord.Transaction.IsAtomicOperation == true)
                        {
                            Utility.Assert(
                                operationRecord.IsRedoOnly == false,
                                "TruncateTail- RedoOnly operation cannot be undone");
                            Utility.Assert(
                                operationRecord.Lsn != LogicalSequenceNumber.InvalidLsn,
                                "Operation's lsn must not be invalid.");

                            operationRecord.Transaction.CommitSequenceNumber = operationRecord.Lsn.LSN;

                            var operationContext =
                                await
                                    this.stateManager.OnApplyAsync(
                                        operationRecord.Lsn.LSN,
                                        operationRecord.Transaction,
                                        operationRecord.MetaData,
                                        operationRecord.Undo,
                                        falseProgressApplyContext).ConfigureAwait(false);

                            if (operationContext != null)
                            {
                                this.stateManager.Unlock(operationContext);
                            }

                            FabricEvents.Events.TruncateTailAtomicOperation(
                                this.tracer.Type,
                                operationRecord.Lsn.LSN,
                                operationRecord.Psn.PSN,
                                operationRecord.RecordPosition,
                                operationRecord.Transaction.Id);
                        }
                        else
                        {
                            FabricEvents.Events.TruncateTailOperationRecord(
                                this.tracer.Type,
                                "Deleted",
                                operationRecord.Lsn.LSN,
                                operationRecord.Psn.PSN,
                                operationRecord.RecordPosition,
                                operationRecord.Transaction.Id);
                        }

                        break;

                    case LogRecordType.EndTransaction:
                        var endTransactionRecord = (EndTransactionLogRecord) currentRecord;
                        endTransactionRecord = this.transactionsMap.ReifyTransaction(endTransactionRecord);

                        Utility.Assert(
                            endTransactionRecord.Lsn != LogicalSequenceNumber.InvalidLsn,
                            "end transaction record cannot have an invalid lsn.");

                        // Undo all operations (Call apply with undo).
                        if (endTransactionRecord.IsCommitted == true)
                        {
                            TransactionLogRecord transactionRecord = endTransactionRecord;
                            do
                            {
                                // During recovery operations that may be undo are kept in memory.
                                // Since Truncate tail uses the in-memory links, Transaction have already been created and their commit sequence numbers
                                // have been set during recovery redo.
                                Utility.Assert(
                                    transactionRecord.RecordType == LogRecordType.EndTransaction
                                    || transactionRecord.Transaction.CommitSequenceNumber
                                    != LogicalSequenceNumber.InvalidLsn.LSN,
                                    "For an operation to be undone, it must already have been done.");

                                object operationContext = null;
                                transactionRecord = transactionRecord.ParentTransactionRecord;
                                Utility.Assert(transactionRecord != null, "transactionRecord != null");
                                if (transactionRecord is BeginTransactionOperationLogRecord)
                                {
                                    // Cache the metadata read from disk
                                    var justReadTransactionRecord =
                                        await this.recoveryLogsReader.GetNextLogRecord(transactionRecord.RecordPosition).ConfigureAwait(false);
                                    Utility.Assert(
                                        justReadTransactionRecord.RecordType == LogRecordType.BeginTransaction,
                                        "Just read transaction during false progress is not begintransaction. It is {0}",
                                        justReadTransactionRecord.RecordType);
                                    var justReadBeginTransactionRecord =
                                        (BeginTransactionOperationLogRecord) justReadTransactionRecord;

                                    var beginTx = (BeginTransactionOperationLogRecord) transactionRecord;
                                    beginTx.MetaData = justReadBeginTransactionRecord.MetaData;

                                    Utility.Assert(
                                        beginTx.IsSingleOperationTransaction == false,
                                        "beginTx.IsSingleOperationTransaction must be false when endTxRecord is being processed");

                                    operationContext =
                                        await
                                            this.stateManager.OnApplyAsync(
                                                beginTx.Lsn.LSN,
                                                beginTx.Transaction,
                                                beginTx.MetaData,
                                                beginTx.Undo,
                                                falseProgressApplyContext).ConfigureAwait(false);

                                    if (operationContext != null)
                                    {
                                        beginTx.OperationContext = operationContext;
                                    }

                                    break;
                                }

                                // Cache the metadata read from disk
                                var justReadOperationLogRecord =
                                    await this.recoveryLogsReader.GetNextLogRecord(transactionRecord.RecordPosition).ConfigureAwait(false);
                                Utility.Assert(
                                    justReadOperationLogRecord.RecordType == LogRecordType.Operation,
                                    "Just read operation during false progress is not of the right type. It is {0}",
                                    justReadOperationLogRecord.RecordType);
                                var justReadOperationRecord = (OperationLogRecord) justReadOperationLogRecord;

                                operationRecord = (OperationLogRecord) transactionRecord;
                                operationRecord.MetaData = justReadOperationRecord.MetaData;

                                operationContext =
                                    await
                                        this.stateManager.OnApplyAsync(
                                            operationRecord.Lsn.LSN,
                                            operationRecord.Transaction,
                                            operationRecord.MetaData,
                                            operationRecord.Undo,
                                            falseProgressApplyContext).ConfigureAwait(false);

                                if (operationContext != null)
                                {
                                    operationRecord.OperationContext = operationContext;
                                }
                            } while (true);

                            // call unlock
                            transactionRecord = endTransactionRecord;
                            do
                            {
                                object operationContext = null;
                                transactionRecord = transactionRecord.ParentTransactionRecord;
                                Utility.Assert(transactionRecord != null, "transactionRecord != null");
                                if (transactionRecord is BeginTransactionOperationLogRecord)
                                {
                                    var beginTx = (BeginTransactionOperationLogRecord) transactionRecord;
                                    operationContext = beginTx.ResetOperationContext();

                                    if (operationContext != null)
                                    {
                                        this.stateManager.Unlock(operationContext);
                                    }

                                    FabricEvents.Events.TruncateTailOperationRecord(
                                        this.tracer.Type,
                                        "Undone",
                                        beginTx.Lsn.LSN,
                                        beginTx.Psn.PSN,
                                        beginTx.RecordPosition,
                                        beginTx.Transaction.Id);

                                    break;
                                }

                                operationRecord = (OperationLogRecord) transactionRecord;
                                operationContext = operationRecord.ResetOperationContext();
                                if (operationContext != null)
                                {
                                    this.stateManager.Unlock(operationContext);
                                }

                                FabricEvents.Events.TruncateTailOperationRecord(
                                    this.tracer.Type,
                                    "Undone",
                                    operationRecord.Lsn.LSN,
                                    operationRecord.Psn.PSN,
                                    operationRecord.RecordPosition,
                                    operationRecord.Transaction.Id);
                            } while (true);
                        }

                        FabricEvents.Events.TruncateTailTransactionRecord(
                            this.tracer.Type,
                            "Reified",
                            endTransactionRecord.Lsn.LSN,
                            endTransactionRecord.Psn.PSN,
                            endTransactionRecord.RecordPosition,
                            endTransactionRecord.Transaction.Id);
                        break;

                    case LogRecordType.Barrier:
                        var barrierRecord = (BarrierLogRecord) currentRecord;

                        FabricEvents.Events.TruncateTailBarrier(
                            this.tracer.Type,
                            barrierRecord.Lsn.LSN,
                            barrierRecord.Psn.PSN,
                            barrierRecord.RecordPosition);
                        break;

                    case LogRecordType.Backup:
                        // Inform the backup manager that the last backup log record has been undone.
                        this.backupManager.UndoLastCompletedBackupLogRecord();

                        // Trace that the backup log record has been false progressed.
                        var backupRecord = (BackupLogRecord)currentRecord;
#if !DotNetCoreClr
                        // These are new events defined in System.Fabric, existing CoreCLR apps would break 
                        // if these events are refernced as it wont be found. As CoreCLR apps carry System.Fabric
                        // along with application
                        // This is just a mitigation for now. Actual fix being tracked via bug# 11614507

                        FabricEvents.Events.TruncateTailBackup(
                            this.tracer.Type,
                            backupRecord.Lsn.LSN,
                            backupRecord.Psn.PSN,
                            backupRecord.RecordPosition);
#endif
                        break;

                    case LogRecordType.UpdateEpoch:

                        // These records can only occur at the tail
                        Utility.Assert(isUpdateRecordAtTail == true, "isUpdateRecordAtTail == true");

                        var updateEpochRecord = (UpdateEpochLogRecord) currentRecord;
                        var lastVector = new ProgressVectorEntry(updateEpochRecord);
                        this.replicatedLogManager.ProgressVector.TruncateTail(lastVector);

                        FabricEvents.Events.TruncateTailUpdateEpoch(
                            this.tracer.Type,
                            lastVector.Epoch.DataLossNumber,
                            lastVector.Epoch.ConfigurationNumber,
                            updateEpochRecord.Lsn.LSN,
                            updateEpochRecord.Psn.PSN,
                            updateEpochRecord.RecordPosition);
                        break;

                    case LogRecordType.EndCheckpoint:
                        Utility.Assert(
                            currentRecord.Psn == this.replicatedLogManager.LastCompletedEndCheckpointRecord.Psn,
                            "currentRecord.Psn == this.lastCompletedEndCheckpointRecord.Psn");
                        Utility.Assert(
                            currentRecord.Psn == this.replicatedLogManager.LastLinkedPhysicalRecord.Psn,
                            "currentRecord.Psn == this.lastLinkedPhysicalRecord.Psn");
                        endCheckpointLogRecord = this.replicatedLogManager.LastCompletedEndCheckpointRecord;
                        this.replicatedLogManager.OnTruncateTailOfLastLinkedPhysicalRecord();
                        goto case LogRecordType.Indexing;

                    case LogRecordType.TruncateHead:
                        Utility.Assert(
                            currentRecord.Psn == this.replicatedLogManager.LastLinkedPhysicalRecord.Psn,
                            "currentRecord.Psn == this.lastLinkedPhysicalRecord.Psn");
                        var truncateHeadRecord = (TruncateHeadLogRecord) currentRecord;
                        Utility.Assert(
                            truncateHeadRecord.IsStable == false,
                            "Stable truncateHeadRecord cannot be undone due to false progress");
                        this.replicatedLogManager.OnTruncateTailOfLastLinkedPhysicalRecord();
                        goto case LogRecordType.Indexing;

                    case LogRecordType.CompleteCheckpoint:
                        completeCheckpointLogRecord = currentRecord as CompleteCheckpointLogRecord;
                        this.replicatedLogManager.OnTruncateTailOfLastLinkedPhysicalRecord();
                        goto case LogRecordType.Indexing;

                    case LogRecordType.Indexing:
                    case LogRecordType.TruncateTail:
                    case LogRecordType.BeginCheckpoint:
                    case LogRecordType.Information:
                        Utility.Assert(
                            currentRecord.Psn == lastPhysicalRecord.Psn,
                            "currentRecord.Psn == lastPhysicalRecord.Psn");
                        lastPhysicalRecord = lastPhysicalRecord.PreviousPhysicalRecord;
                        break;

                    default:
                        Utility.CodingError("Unexpected record type {0}", currentRecord.RecordType);
                        break;
                }

                currentRecord = await this.recoveryLogsReader.GetPreviousLogRecord(currentRecord.RecordPosition).ConfigureAwait(false);
                currentLsn = currentRecord.Lsn;
            } while (currentLsn > tailLsn);

            Utility.Assert(currentLsn == tailLsn, "V1 replicator ensures that lsns are continuous. currentLsn {0} == tailLsn {1}", currentLsn, tailLsn);

            if (currentRecord is LogicalLogRecord)
            {
                switch (currentRecord.RecordType)
                {
                    case LogRecordType.BeginTransaction:
                        var beginTransactionRecord =
                            (BeginTransactionOperationLogRecord) currentRecord;
                        currentRecord = this.transactionsMap.FindTransaction(beginTransactionRecord);

                        // Single operation transactions are not stored in the tx map and hence are not returned above. As a result, they dont have a valid indexing of the previous physical record
                        if (beginTransactionRecord.IsSingleOperationTransaction)
                        {
                            currentRecord.PreviousPhysicalRecord = lastPhysicalRecord;
                        }

                        break;

                    case LogRecordType.Operation:
                        var operationRecord = (OperationLogRecord) currentRecord;
                        currentRecord = this.transactionsMap.FindOperation(operationRecord);

                        // Atomic operations are not stored in the tx map and hence are not returned above. As a result, they dont have a valid indexing of the previous physical record
                        if (operationRecord.Transaction.IsAtomicOperation)
                        {
                            currentRecord.PreviousPhysicalRecord = lastPhysicalRecord;
                        }

                        break;

                    case LogRecordType.EndTransaction:
                        var endTransactionRecord = (EndTransactionLogRecord) currentRecord;
                        currentRecord = this.transactionsMap.FindUnstableTransaction(endTransactionRecord);
                        break;

                    case LogRecordType.Backup:
                    case LogRecordType.Barrier:
                        currentRecord.PreviousPhysicalRecord = lastPhysicalRecord;
                        break;

                    case LogRecordType.UpdateEpoch:
                        currentRecord.PreviousPhysicalRecord = lastPhysicalRecord;
                        break;

                    default:
                        Utility.CodingError("Unexpected record type {0}", currentRecord.RecordType);
                        break;
                }

                Utility.Assert(
                    currentRecord.PreviousPhysicalRecord == lastPhysicalRecord,
                    "currentRecord.PreviousPhysicalRecord == lastPhysicalRecord");
            }
            else
            {
                Utility.Assert(
                    lastPhysicalRecord.Psn == currentRecord.Psn,
                    "lastPhysicalRecord.Psn == currentRecord.Psn");
                currentRecord = lastPhysicalRecord;
            }

            Utility.Assert(
                (this.replicatedLogManager.LastLinkedPhysicalRecord == lastPhysicalRecord)
                || (this.replicatedLogManager.LastLinkedPhysicalRecord == lastPhysicalRecord.LinkedPhysicalRecord),
                "(this.lastLinkedPhysicalRecord == lastPhysicalRecord) || (this.lastLinkedPhysicalRecord == lastPhysicalRecord.LinkedPhysicalRecord)");
            await this.replicatedLogManager.LogManager.PerformLogTailTruncationAsync(currentRecord).ConfigureAwait(false);
            this.replicatedLogManager.SetTailLsn(tailLsn);

            // If endchkpoint was truncated, even complete checkpoint must be truncated 
            if (endCheckpointLogRecord != null)
            {
                Utility.Assert(
                    completeCheckpointLogRecord != null,
                    "TruncateTailAsync: EndCheckpoint was truncated but CompleteCheckpoint was not");

                this.replicatedLogManager.EndCheckpoint(endCheckpointLogRecord.LastCompletedBeginCheckpointRecord);
            }

            if (completeCheckpointLogRecord != null)
            {
                this.replicatedLogManager.CompleteCheckpoint();
            }

            this.replicatedLogManager.TruncateTail(tailLsn);

            await this.replicatedLogManager.LogManager.FlushAsync("TruncateTailAsync").ConfigureAwait(false);

            FabricEvents.Events.TruncateTailDone(
                this.tracer.Type,
                currentRecord.RecordType.ToString(),
                currentRecord.Lsn.LSN,
                currentRecord.Psn.PSN,
                currentRecord.RecordPosition);

            return currentRecord;
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using Backup;
    using Data;
    using Data.Notifications;
    using System;
    using System.Fabric;
    using System.Fabric.Common.Tracing;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;

    internal class OperationProcessor : IOperationProcessor
    {
        private IStateManager stateManager;
        private IBackupManager backupManager;
        private IInternalVersionManager versionManager;
        private CheckpointManager checkpointManager;
        private ITracer tracer;
        private Exception serviceException;
        private Exception logException;
        private RoleContextDrainState roleContextDrainState;
        private long numberOfLogicalRecordsBeingProcessed;
        private long numberOfPhysicalRecordsBeingProcessed;
        private TaskCompletionSource<PhysicalLogRecord> physicalRecordsProcessingTcs;
        private TaskCompletionSource<LogicalLogRecord> logicalRecordsProcessingTcs;
        private RecoveredOrCopiedCheckpointLsn recoveredOrCopiedCheckpointLsn;

        /// <summary>
        /// EventHandler for publishing changes in transactions.
        /// For example, notifying that the transaction committed.
        /// </summary>
        public event EventHandler<NotifyTransactionChangedEventArgs> TransactionChanged;

        public OperationProcessor()
        {
            Reuse();
        }

        public LogRecord LastAppliedBarrierRecord
        {
            get; private set;
        }

        public Exception ServiceException
        {
            get
            {
                return this.serviceException;
            }
        }

        public Exception LogException 
        {
            get
            {
                return this.logException;
            }
        }

        public void Open(
            RecoveredOrCopiedCheckpointLsn recoveredOrCopiedCheckpointLsn,
            ITracer tracer,
            RoleContextDrainState roleContextDrainState,
            IBackupManager backupManager,
            IInternalVersionManager versionManager,
            CheckpointManager checkpointManager,
            IStateManager stateManager)
        {
            this.recoveredOrCopiedCheckpointLsn = recoveredOrCopiedCheckpointLsn;
            this.roleContextDrainState = roleContextDrainState;
            this.tracer = tracer;
            this.backupManager = backupManager;
            this.versionManager = versionManager;
            this.checkpointManager = checkpointManager;
            this.stateManager = stateManager;
        }

        public void Reuse()
        {
            this.numberOfLogicalRecordsBeingProcessed = 1;
            this.numberOfPhysicalRecordsBeingProcessed = 1;
            this.physicalRecordsProcessingTcs = new TaskCompletionSource<PhysicalLogRecord>();
            this.logicalRecordsProcessingTcs = new TaskCompletionSource<LogicalLogRecord>();
            this.LastAppliedBarrierRecord = BarrierLogRecord.CreateOneBarrierLogRecord();
            this.serviceException = null;
            this.logException = null;
        }

        public void TruncateTail(LogRecord currentRecord)
        {
            this.LastAppliedBarrierRecord = currentRecord;
        }

        public RecordProcessingMode IdentifyProcessingModeForRecord(LogRecord record)
        {
            var stream = this.roleContextDrainState.DrainingStream;
            Utility.Assert(
                stream != DrainingStream.Invalid || record is UpdateEpochLogRecord || record is PhysicalLogRecord,
                @"Draining stream should not be invalid or record should be of type epoch or physical log record.
                            DrainingStream: {0} RecordType: {1}",
                stream, record.RecordType);

            if (LoggingReplicator.IsBarrierRecord(record))
            {
                this.LastAppliedBarrierRecord = record;
            }

            RecordProcessingMode processingMode;
            switch (record.RecordType)
            {
                case LogRecordType.EndCheckpoint:
                case LogRecordType.TruncateTail:
                case LogRecordType.Indexing:
                case LogRecordType.UpdateEpoch:
                case LogRecordType.Information:
                case LogRecordType.CompleteCheckpoint:
                    processingMode = RecordProcessingMode.ProcessImmediately;
                    break;

                case LogRecordType.Barrier:
                    processingMode = (this.roleContextDrainState.ReplicaRole == ReplicaRole.Unknown)
                        ? RecordProcessingMode.ProcessImmediately
                        : RecordProcessingMode.ApplyImmediately;
                    break;

                case LogRecordType.TruncateHead:
                    processingMode = RecordProcessingMode.ApplyImmediately;
                    break;

                case LogRecordType.BeginCheckpoint:
                    Utility.Assert(
                        (record.Lsn >= this.recoveredOrCopiedCheckpointLsn.Value)
                        || (this.roleContextDrainState.ReplicaRole == ReplicaRole.Unknown),
                        "record.LastLogicalSequenceNumber >= this.recoveredOrCopiedCheckpointLsn || "
                        + "(this.replicaRole == ReplicaRole.Unknown)");
                    processingMode = (this.roleContextDrainState.ReplicaRole == ReplicaRole.Unknown)
                        ? RecordProcessingMode.ProcessImmediately
                        : RecordProcessingMode.ApplyImmediately;
                    break;

                case LogRecordType.BeginTransaction:
                case LogRecordType.Operation:
                case LogRecordType.EndTransaction:
                case LogRecordType.Backup:
                    processingMode = RecordProcessingMode.Normal;
                    break;

                default:
                    Utility.CodingError("Unexpected record type {0}", record.RecordType);
                    processingMode = RecordProcessingMode.Normal;
                    break;
            }

            return processingMode;
        }

        public async Task ImmediatelyProcessRecord(
            LogRecord record,
            Exception flushException,
            RecordProcessingMode processingMode)
        {
            Utility.Assert(
                processingMode > RecordProcessingMode.Normal,
                "processingMode ({0}) > RecordProcessingMode.Normal",
                processingMode);

            // TODO: Temporary double check.
            Utility.Assert(record.RecordType != LogRecordType.Backup, "record.RecordType != LogRecordType.Backup");

            Exception exception = null;

            if (flushException != null)
            {
                Utility.Assert(
                    this.logException != null,
                    "FlushException is {0} and this.logException is null",
                    flushException);
                exception = flushException;
            }

            // In case there was an apply failure, we should fault any further applies of any records.
            // Without this, we could end up assuming that all applies have succeeded and as a result, issue a checkpoint call
            if (exception == null)
            {
                exception = this.serviceException;
            }

            FabricEvents.Events.RecordProcessedImmediatelyNoise(
                this.tracer.Type,
                (int)this.roleContextDrainState.DrainingStream,
                record.Psn.PSN);

            var information = string.Empty;
            switch (record.RecordType)
            {
                case LogRecordType.TruncateHead:
                    this.checkpointManager.ApplyLogHeadTruncationIfPermitted(exception, record);
                    break;

                case LogRecordType.BeginCheckpoint:
                    if (processingMode == RecordProcessingMode.ProcessImmediately)
                    {
                        goto default;
                    }

                    await this.checkpointManager.ApplyCheckpointIfPermitted(exception, record).ConfigureAwait(false);
                    break;

                case LogRecordType.Information:
                    information = '-' + ((InformationLogRecord)record).InformationEvent.ToString();
                    goto case LogRecordType.EndCheckpoint;

                case LogRecordType.EndCheckpoint:
                case LogRecordType.TruncateTail:
                case LogRecordType.Indexing:
                case LogRecordType.UpdateEpoch:
                    goto default;

                default:
                    record.CompletedApply(exception);
                    break;
            }

            if (processingMode == RecordProcessingMode.ProcessImmediately)
            {
                FabricEvents.Events.RecordProcessedImmediately(
                    this.tracer.Type,
                    (int)this.roleContextDrainState.DrainingStream,
                    information,
                    record.Psn.PSN);

                record.CompletedProcessing(null);
            }

            return;
        }

        public void PrepareToProcessLogicalRecord()
        {
            Interlocked.Increment(ref this.numberOfLogicalRecordsBeingProcessed);
        }

        public void PrepareToProcessPhysicalRecord()
        {
            Interlocked.Increment(ref this.numberOfPhysicalRecordsBeingProcessed);
        }

        /// <summary>
        /// Apply and unlock an operation on Primary if the operation is flushed.
        /// </summary>
        /// <param name="record">Record to process.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public async Task ProcessLoggedRecordAsync(LogRecord record)
        {
            Utility.Assert(record.IsFlushed == true, "record.IsFlushed == true");
            var logicalRecord = record as LogicalLogRecord;
            Utility.Assert(logicalRecord != null, "logicalRecord != null");

            // GopalK: The order of the following instructions is significant.
            // Replica role might change after the record gets applied
            var currentRole = this.roleContextDrainState.ReplicaRole;
            await this.ApplyCallback(record).ConfigureAwait(false);
            if (currentRole != ReplicaRole.Primary)
            {
                Unlock(logicalRecord);
            }

            return;
        }

        public void UpdateDispatchingBarrierTask(CompletionTask barrierTask)
        {
            this.versionManager.UpdateDispatchingBarrierTask(barrierTask);
        }

        public void Unlock(LogicalLogRecord record)
        {
            Utility.Assert(record.IsApplied == true, "record.IsApplied == true");
            Utility.Assert(this.recoveredOrCopiedCheckpointLsn.Value != LogicalSequenceNumber.InvalidLsn, "recoveredOrCopiedCheckpointLsn must not be -1");

            // If flush fails, the records are processed immediately, after which the processing can complete (before waiting for unlocks)
            if (record.FlushException == null)
            {
                Utility.Assert(
                    roleContextDrainState.DrainingStream != DrainingStream.Invalid,
                    "this.DrainingStream != DrainingStream.Invalid during unlock for record lsn:{0} psn:{1}",
                    record.Lsn, record.Psn);
            }

            BeginTransactionOperationLogRecord beginTransactionRecord;
            try
            {
                OperationLogRecord operationRecord;
                switch (record.RecordType)
                {
                    case LogRecordType.BeginTransaction:
                        beginTransactionRecord = (BeginTransactionOperationLogRecord)record;
                        if (beginTransactionRecord.IsSingleOperationTransaction)
                        {
                            if (beginTransactionRecord.Lsn > this.recoveredOrCopiedCheckpointLsn.Value)
                            {
                                var operationContext = beginTransactionRecord.ResetOperationContext();
                                if (operationContext != null)
                                {
                                    this.stateManager.Unlock(operationContext);
                                }
                            }


                            this.FireCommitNotification((ITransaction)beginTransactionRecord.Transaction);
                        }

                        break;

                    case LogRecordType.Operation:
                        operationRecord = (OperationLogRecord)record;
                        if (operationRecord.Transaction.IsAtomicOperation == true)
                        {
                            if (operationRecord.Lsn > this.recoveredOrCopiedCheckpointLsn.Value)
                            {
                                var operationContext = operationRecord.ResetOperationContext();
                                if (operationContext != null)
                                {
                                    this.stateManager.Unlock(operationContext);
                                }
                            }
                        }

                        break;

                    case LogRecordType.EndTransaction:
                        var endTransactionRecord = (EndTransactionLogRecord)record;
                        if (endTransactionRecord.Lsn > this.recoveredOrCopiedCheckpointLsn.Value)
                        {
                            TransactionLogRecord transactionRecord = endTransactionRecord;
                            do
                            {
                                transactionRecord = transactionRecord.ParentTransactionRecord;
                                beginTransactionRecord = transactionRecord as BeginTransactionOperationLogRecord;
                            } while (beginTransactionRecord == null);

                            Utility.Assert(
                                (beginTransactionRecord != null)
                                && (LogRecord.IsInvalidRecord(beginTransactionRecord) == false),
                                "(beginTransactionRecord != null) && (TransactionLogRecord.IsInvalidRecord(beginTransactionRecord) == false)");

                            var operationContext = beginTransactionRecord.ResetOperationContext();
                            if (operationContext != null)
                            {
                                this.stateManager.Unlock(operationContext);
                            }

                            transactionRecord = beginTransactionRecord;
                            do
                            {
                                transactionRecord = transactionRecord.ChildTransactionRecord;
                                Utility.Assert(
                                    (transactionRecord != null)
                                    && (LogRecord.IsInvalidRecord(transactionRecord) == false),
                                    "(transactionRecord != null) && (TransactionLogRecord.IsInvalidRecord(transactionRecord) == false)");
                                if (transactionRecord == endTransactionRecord)
                                {
                                    break;
                                }

                                operationRecord = (OperationLogRecord)transactionRecord;
                                operationContext = operationRecord.ResetOperationContext();
                                if (operationContext != null)
                                {
                                    this.stateManager.Unlock(operationContext);
                                }
                            } while (true);
                        }
                        else
                        {
                            TransactionLogRecord transactionRecord = endTransactionRecord;
                            do
                            {
                                var parentRecord = transactionRecord.ParentTransactionRecord;
                                if (parentRecord == null)
                                {
                                    break;
                                }

                                transactionRecord = parentRecord;
                            } while (true);

                            Utility.Assert(
                                (endTransactionRecord.IsEnlistedTransaction == false)
                                || (transactionRecord is BeginTransactionOperationLogRecord),
                                "(endTransactionRecord.IsEnlistedTransaction {0} == false) || "
                                + "(transactionRecord is BeginTransactionLogRecord {1})",
                                endTransactionRecord.IsEnlistedTransaction, transactionRecord.RecordType);

                            if (transactionRecord != endTransactionRecord)
                            {
                                do
                                {
                                    transactionRecord = transactionRecord.ChildTransactionRecord;
                                    Utility.Assert(
                                        (transactionRecord != null)
                                        && (LogRecord.IsInvalidRecord(transactionRecord) == false),
                                        "(transactionRecord != null) && (TransactionLogRecord.IsInvalidRecord(transactionRecord) == false)");
                                    if (transactionRecord == endTransactionRecord)
                                    {
                                        break;
                                    }
                                } while (true);
                            }
                        }

                        if (endTransactionRecord.IsCommitted == true)
                        {
                            this.FireCommitNotification((ITransaction)endTransactionRecord.Transaction);
                        }

                        break;

                    case LogRecordType.Backup:
                        var backupLogRecord = (BackupLogRecord)record;
                        Utility.Assert(backupManager != null, "Backupmanager must not be null in UnlockCallbackManager");
                        backupManager.LastCompletedBackupLogRecord = backupLogRecord;
                        break;

                    default:
                        Utility.CodingError("Unexpected record type {0}", record.RecordType);
                        break;
                }
            }
            catch (Exception e)
            {
                this.ProcessServiceException("Unlockcallback", record, e);
            }

            this.ProcessedLogicalRecord(record);

            return;
        }

        private void FireCommitNotification(ITransaction transaction)
        {
            var transactionEventHandler = this.TransactionChanged;

            if (transactionEventHandler == null)
            {
                return;
            }

            try
            {
                transactionEventHandler(
                    this,
                    new NotifyTransactionChangedEventArgs(transaction, NotifyTransactionChangedAction.Commit));
            }
            catch (Exception e)
            {
                FabricEvents.Events.Warning(this.tracer.Type, e.ToString());
            }
        }

        private async Task ApplyCallback(LogRecord record)
        {
            Utility.Assert(
                this.roleContextDrainState.DrainingStream != DrainingStream.Invalid && this.roleContextDrainState.DrainingStream != DrainingStream.StateStream,
                "ApplyCallback: this.roleContextDrainState.DrainingStream != DrainingStream.Invalid && this.roleContextDrainState.DrainingStream != DrainingStream.StateStream. It is {0} for log record lsn: {1} and psn: {2}",
                this.roleContextDrainState.DrainingStream, record.Lsn, record.Psn);

            var serviceException = this.serviceException;

            if (serviceException != null)
            {
                record.CompletedApply(serviceException);
                return;
            }

            try
            {
                OperationLogRecord operationRecord;
                var callbackRedoContext = this.roleContextDrainState.ApplyRedoContext;
                switch (record.RecordType)
                {
                    case LogRecordType.BeginTransaction:
                        var beginTransactionRecordExtra =
                            (BeginTransactionOperationLogRecord)record;

                        FabricEvents.Events.ApplyCallbackTransactionRecordNoise(
                            this.tracer.Type,
                            (int)this.roleContextDrainState.DrainingStream,
                            (uint)beginTransactionRecordExtra.RecordType,
                            beginTransactionRecordExtra.Lsn.LSN,
                            beginTransactionRecordExtra.Psn.PSN,
                            beginTransactionRecordExtra.RecordPosition,
                            beginTransactionRecordExtra.Transaction.Id);

                        if (beginTransactionRecordExtra.IsSingleOperationTransaction)
                        {
                            if (beginTransactionRecordExtra.Lsn > this.recoveredOrCopiedCheckpointLsn.Value)
                            {
                                beginTransactionRecordExtra.Transaction.CommitSequenceNumber =
                                    beginTransactionRecordExtra.Lsn.LSN;

                                var operationContext =
                                    await
                                        this.stateManager.OnApplyAsync(
                                            beginTransactionRecordExtra.Lsn.LSN,
                                            beginTransactionRecordExtra.Transaction,
                                            beginTransactionRecordExtra.MetaData,
                                            beginTransactionRecordExtra.Redo,
                                            callbackRedoContext).ConfigureAwait(false);

                                if (operationContext != null)
                                {
                                    beginTransactionRecordExtra.OperationContext = operationContext;
                                }

                                FabricEvents.Events.ApplyCallbackSingleOperationTransaction(
                                    this.tracer.Type,
                                    (int)this.roleContextDrainState.DrainingStream,
                                    beginTransactionRecordExtra.Lsn.LSN,
                                    beginTransactionRecordExtra.Psn.PSN,
                                    beginTransactionRecordExtra.RecordPosition,
                                    beginTransactionRecordExtra.Transaction.Id);
                            }
                        }

                        break;

                    case LogRecordType.Operation:
                        operationRecord = (OperationLogRecord)record;
                        if (operationRecord.Transaction.IsAtomicOperation == true)
                        {
                            if (operationRecord.Lsn > this.recoveredOrCopiedCheckpointLsn.Value)
                            {
                                // For atomic operations create lsn equals commit lsn.
                                operationRecord.Transaction.CommitSequenceNumber = operationRecord.Lsn.LSN;

                                var operationContext =
                                    await
                                        this.stateManager.OnApplyAsync(
                                            operationRecord.Lsn.LSN,
                                            operationRecord.Transaction,
                                            operationRecord.MetaData,
                                            operationRecord.Redo,
                                            callbackRedoContext).ConfigureAwait(false);

                                if (operationContext != null)
                                {
                                    operationRecord.OperationContext = operationContext;
                                }

                                FabricEvents.Events.ApplyCallbackAtomicOperationRecord(
                                    this.tracer.Type,
                                    (int)this.roleContextDrainState.DrainingStream,
                                    operationRecord.Lsn.LSN,
                                    operationRecord.Psn.PSN,
                                    operationRecord.RecordPosition,
                                    operationRecord.Transaction.Id,
                                    operationRecord.IsRedoOnly);
                            }
                        }
                        else
                        {
                            FabricEvents.Events.ApplyCallbackTransactionRecordNoise(
                                this.tracer.Type,
                                (int)this.roleContextDrainState.DrainingStream,
                                (uint)operationRecord.RecordType,
                                operationRecord.Lsn.LSN,
                                operationRecord.Psn.PSN,
                                operationRecord.RecordPosition,
                                operationRecord.Transaction.Id);
                        }

                        break;

                    case LogRecordType.EndTransaction:
                        var endTransactionRecord = (EndTransactionLogRecord)record;
                        if ((endTransactionRecord.IsCommitted == true)
                            && (endTransactionRecord.Lsn > this.recoveredOrCopiedCheckpointLsn.Value))
                        {
                            // GopalK: I am currently adopting the approach of only applying updates that do not
                            // belong to any transaction in the update switch above and
                            // applying the updates that belong to a transaction only when if it commits.
                            // The other approach is to immediately apply updates of all
                            // transactions in the update switch case above and then undoing updates of
                            // aborted transactions here.
                            // Both approaches have their pros and cons and we may want to look into
                            // making this a replicator option that the service developer can choose.

                            // If on Primary, Transaction object is shared.
                            endTransactionRecord.Transaction.CommitSequenceNumber = endTransactionRecord.Lsn.LSN;

                            BeginTransactionOperationLogRecord beginTransactionRecord = null;
                            TransactionLogRecord transactionRecord = endTransactionRecord;
                            do
                            {
                                transactionRecord = transactionRecord.ParentTransactionRecord;
                                beginTransactionRecord = transactionRecord as BeginTransactionOperationLogRecord;
                            } while (beginTransactionRecord == null);
                            
                            {
                                Utility.Assert(
                                    LogRecord.IsInvalidRecord(beginTransactionRecord) == false,
                                    "TransactionLogRecord.IsInvalidRecord(beginTransactionRecord) == false");

                                Utility.Assert(
                                    beginTransactionRecord.IsSingleOperationTransaction == false,
                                    "beginTransactionRecord.IsSingleOperationTransaction must be false when endTxRecord is being processed");

                                // If on not Primary, Transaction object is shared.
                                if (callbackRedoContext.HasFlag(ApplyContext.PRIMARY) == false)
                                {
                                    beginTransactionRecord.Transaction.CommitSequenceNumber =
                                        endTransactionRecord.Lsn.LSN;
                                }
                                else
                                {
                                    // TODO: Temporary assert. Should be removed later.
                                    Utility.Assert(
                                        beginTransactionRecord.Transaction.CommitSequenceNumber
                                        == endTransactionRecord.Lsn.LSN,
                                        "Transaction's commit sequence number must have already been set. Expected: {0} Actual: {1}",
                                        transactionRecord.Transaction.CommitSequenceNumber,
                                        endTransactionRecord.Lsn.LSN);
                                }

                                var operationContext =
                                    await
                                        this.stateManager.OnApplyAsync(
                                            beginTransactionRecord.Lsn.LSN,
                                            beginTransactionRecord.Transaction,
                                            beginTransactionRecord.MetaData,
                                            beginTransactionRecord.Redo,
                                            callbackRedoContext).ConfigureAwait(false);

                                if (operationContext != null)
                                {
                                    beginTransactionRecord.OperationContext = operationContext;
                                }

                                FabricEvents.Events.ApplyCallbackTransactionRecordNoise(
                                    this.tracer.Type,
                                    (int)this.roleContextDrainState.DrainingStream,
                                    (uint)beginTransactionRecord.RecordType,
                                    beginTransactionRecord.Lsn.LSN,
                                    beginTransactionRecord.Psn.PSN,
                                    beginTransactionRecord.RecordPosition,
                                    beginTransactionRecord.Transaction.Id);
                            }

                            do
                            {
                                transactionRecord = transactionRecord.ChildTransactionRecord;
                                Utility.Assert(
                                    (transactionRecord != null)
                                    && (LogRecord.IsInvalidRecord(transactionRecord) == false),
                                    "(transactionRecord != null) && (TransactionLogRecord.IsInvalidRecord(transactionRecord) == false");

                                if (transactionRecord == endTransactionRecord)
                                {
                                    break;
                                }

                                operationRecord = (OperationLogRecord)transactionRecord;

                                // If on Primary, Transaction object is shared.
                                if (callbackRedoContext.HasFlag(ApplyContext.PRIMARY) == false)
                                {
                                    operationRecord.Transaction.CommitSequenceNumber = endTransactionRecord.Lsn.LSN;
                                }
                                else
                                {
                                    // TODO: Temporary assert. Should be removed later.
                                    Utility.Assert(
                                        operationRecord.Transaction.CommitSequenceNumber == endTransactionRecord.Lsn.LSN,
                                        "Transaction's commit sequence number must have already been set. Expected: {0} Actual: {1}",
                                        transactionRecord.Transaction.CommitSequenceNumber,
                                        endTransactionRecord.Lsn.LSN);
                                }

                                var operationContext =
                                    await
                                        this.stateManager.OnApplyAsync(
                                            operationRecord.Lsn.LSN,
                                            operationRecord.Transaction,
                                            operationRecord.MetaData,
                                            operationRecord.Redo,
                                            callbackRedoContext).ConfigureAwait(false);

                                if (operationContext != null)
                                {
                                    operationRecord.OperationContext = operationContext;
                                }

                                FabricEvents.Events.ApplyCallbackTransactionRecordNoise(
                                    this.tracer.Type,
                                    (int)this.roleContextDrainState.DrainingStream,
                                    (uint)operationRecord.RecordType,
                                    operationRecord.Lsn.LSN,
                                    operationRecord.Psn.PSN,
                                    operationRecord.RecordPosition,
                                    operationRecord.Transaction.Id);
                            } while (true);

                            FabricEvents.Events.ApplyCallbackTransactionRecord(
                                this.tracer.Type,
                                (int)this.roleContextDrainState.DrainingStream,
                                endTransactionRecord.Psn.PSN,
                                endTransactionRecord.Transaction.Id);
                        }

                        break;

                    case LogRecordType.Barrier:
                        this.LastAppliedBarrierRecord = (BarrierLogRecord)record;

                        FabricEvents.Events.ApplyCallbackBarrierRecord(
                            this.tracer.Type,
                            (int)this.roleContextDrainState.DrainingStream,
                            record.Lsn.LSN,
                            record.Psn.PSN,
                            record.RecordPosition);

                        break;

                    case LogRecordType.Backup:

                        // TODO: Trace.
                        break;

                    default:
                        Utility.CodingError("Unexpected record type {0}", record.RecordType);
                        break;
                }
            }
            catch (Exception e)
            {
                this.ProcessServiceException("OnApply", record, e);
                serviceException = e;
            }

            record.CompletedApply(serviceException);
            return;
        }

        private void ProcessServiceException(string component, LogRecord record, Exception e)
        {
            ProcessException(this.tracer.Type, component, record, "Unexpected service exception.", e);
            Interlocked.CompareExchange<Exception>(ref this.serviceException, e, null);
            if (this.roleContextDrainState.DrainingStream != DrainingStream.RecoveryStream)
            {
                this.roleContextDrainState.ReportPartitionFault();
            }
        }

        public void ProcessLogException(string component, LogRecord record, Exception e)
        {
            var isClosedException = ProcessException(this.tracer.Type, component, record, "Unexpected logging exception.", e);
            Interlocked.CompareExchange<Exception>(ref this.logException, e, null);
            if (isClosedException == false)
            {
                if (this.roleContextDrainState.DrainingStream != DrainingStream.RecoveryStream)
                {
                    this.roleContextDrainState.ReportPartitionFault();
                }
            }
        }

        [SuppressMessage(FxCop.Category.Globalization, FxCop.Rule.DoNotPassLiteralsAsLocalizedParameters, Justification = "message is not supposed to be localized.")]
        private static bool ProcessException(string type, string component, LogRecord record, string messagePrefix, Exception e)
        {
            int innerHResult = 0;
            var flattenedException = Utility.FlattenException(e, out innerHResult);

            var message =
                string.Format(
                    CultureInfo.InvariantCulture,
                    component + Environment.NewLine + "      " + messagePrefix
                    + " Type: {0} Message: {1} HResult: 0x{2:X8}" + Environment.NewLine
                    + "      Log record. Type: {3} LSN: {4}" + Environment.NewLine + "{5}",
                    flattenedException.GetType().ToString(),
                    flattenedException.Message,
                    flattenedException.HResult != 0 ? flattenedException.HResult : innerHResult,
                    record.RecordType,
                    record.Lsn.LSN,
                    flattenedException.StackTrace);

            FabricEvents.Events.Error(type, message);

            return flattenedException is FabricObjectClosedException;
        }

        public void ProcessedLogicalRecord(LogicalLogRecord record)
        {
            if (record.FlushException != null)
            {
                FabricEvents.Events.ProcessedLogicalRecordSkip(
                    this.tracer.Type,
                    (int)this.roleContextDrainState.DrainingStream,
                    record.Psn.PSN);

                return;
            }

            var outstandingNumberOfRecordsBeingProcessed =
                Interlocked.Decrement(ref this.numberOfLogicalRecordsBeingProcessed);
            Utility.Assert(
                outstandingNumberOfRecordsBeingProcessed >= 0,
                "outstandingNumberOfRecordsBeingProcessed {0} >= 0",
                outstandingNumberOfRecordsBeingProcessed);

            if (outstandingNumberOfRecordsBeingProcessed == 0)
            {
                this.logicalRecordsProcessingTcs.SetResult(record);
            }

            FabricEvents.Events.ProcessedLogicalRecord(this.tracer.Type, (int)this.roleContextDrainState.DrainingStream, record.Psn.PSN);
            record.CompletedProcessing(null);

            return;
        }

        public void ProcessedPhysicalRecord(PhysicalLogRecord record)
        {
            if (record.FlushException != null)
            {
                FabricEvents.Events.ProcessedPhysicalRecordSkip(
                    this.tracer.Type,
                    (int)this.roleContextDrainState.DrainingStream,
                    record.Psn.PSN);

                return;
            }

            var outstandingNumberOfRecordsBeingProcessed =
                Interlocked.Decrement(ref this.numberOfPhysicalRecordsBeingProcessed);
            Utility.Assert(
                outstandingNumberOfRecordsBeingProcessed >= 0,
                "outstandingNumberOfRecordsBeingProcessed {0} >= 0",
                outstandingNumberOfRecordsBeingProcessed);

            if (outstandingNumberOfRecordsBeingProcessed == 0)
            {
                this.physicalRecordsProcessingTcs.SetResult(record);
            }

            FabricEvents.Events.ProcessedPhysicalRecord(
                this.tracer.Type,
                (int)this.roleContextDrainState.DrainingStream,
                record.RecordType.ToString(),
                record.Lsn.LSN,
                record.Psn.PSN,
                record.RecordPosition);

            record.CompletedProcessing(null);

            return;
        }

        public async Task WaitForLogicalRecordsProcessingAsync()
        {
            var outstandingNumberOfLogicalRecordsBeingProcessed =
                Interlocked.Decrement(ref this.numberOfLogicalRecordsBeingProcessed);
            if (outstandingNumberOfLogicalRecordsBeingProcessed > 0)
            {
                FabricEvents.Events.WaitForProcessing(
                    this.tracer.Type,
                    "WaitForLogicalRecordsProcessingAsync",
                    outstandingNumberOfLogicalRecordsBeingProcessed);

                var recordsProcessingTask = this.logicalRecordsProcessingTcs.Task;
                var lastProcessedRecord = await recordsProcessingTask.ConfigureAwait(false);
                this.logicalRecordsProcessingTcs = new TaskCompletionSource<LogicalLogRecord>();

                FabricEvents.Events.WaitForProcessingDone(
                    this.tracer.Type,
                    "WaitForLogicalRecordsProcessingAsync",
                    (uint) lastProcessedRecord.RecordType,
                    lastProcessedRecord.Lsn.LSN,
                    lastProcessedRecord.Psn.PSN,
                    lastProcessedRecord.RecordPosition);
            }
            else
            {
                FabricEvents.Events.WaitForProcessingDone(
                    this.tracer.Type,
                    "WaitForLogicalRecordsProcessingAsync_Empty",
                    (uint) LogRecordType.Invalid,
                    0,
                    0,
                    0);
            }

            // GopalK: The order of the following statements is significant
            Utility.Assert(
                this.numberOfLogicalRecordsBeingProcessed == 0,
                "this.numberOfLogicalRecordsBeingProcessed ({0}) == 0",
                this.numberOfLogicalRecordsBeingProcessed);

            this.numberOfLogicalRecordsBeingProcessed = 1;
        }

        public async Task WaitForPhysicalRecordsProcessingAsync()
        {
            var outstandingNumberOfPhysicalRecordsBeingProcessed =
                Interlocked.Decrement(ref this.numberOfPhysicalRecordsBeingProcessed);
            if (outstandingNumberOfPhysicalRecordsBeingProcessed > 0)
            {
                FabricEvents.Events.WaitForProcessing(
                    this.tracer.Type,
                    "WaitForPhysicalRecordsProcessingAsync",
                    outstandingNumberOfPhysicalRecordsBeingProcessed);

                var recordsProcessingTask = this.physicalRecordsProcessingTcs.Task;
                var lastProcessedRecord = await recordsProcessingTask.ConfigureAwait(false);
                this.physicalRecordsProcessingTcs = new TaskCompletionSource<PhysicalLogRecord>();

                FabricEvents.Events.WaitForProcessingDone(
                    this.tracer.Type,
                    "WaitForPhysicalRecordsProcessingAsync",
                    (uint) lastProcessedRecord.RecordType,
                    lastProcessedRecord.Lsn.LSN,
                    lastProcessedRecord.Psn.PSN,
                    lastProcessedRecord.RecordPosition);
            }
            else
            {
                FabricEvents.Events.WaitForProcessingDone(
                    this.tracer.Type,
                    "WaitForPhysicalRecordsProcessingAsync_Empty",
                    (uint) LogRecordType.Invalid,
                    0,
                    0,
                    0);
            }

            Utility.Assert(
                this.numberOfPhysicalRecordsBeingProcessed == 0,
                "this.numberOfPhysicalRecordsBeingProcessed ({0}) == 0",
                this.numberOfPhysicalRecordsBeingProcessed);

            this.numberOfPhysicalRecordsBeingProcessed = 1;
        }

        public async Task WaitForAllRecordsProcessingAsync()
        {
            await this.WaitForLogicalRecordsProcessingAsync().ConfigureAwait(false);
            await this.WaitForPhysicalRecordsProcessingAsync().ConfigureAwait(false);
        }
    }
}

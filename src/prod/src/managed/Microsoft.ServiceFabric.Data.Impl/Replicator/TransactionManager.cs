// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using Diagnostics;
    using System;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Globalization;
    using System.Linq;
    using System.Threading.Tasks;

    internal class TransactionManager
    {
        private readonly BeginTransactionOperationRatePerformanceCounterWriter beginTransactionOperationRatePerformanceCounterWriter;
        private readonly AddOperationRatePerformanceCounterWriter addOperationRatePerformanceCounterWriter;
        private readonly AddAtomicOperationRatePerformanceCounterWriter addAtomicOperationRatePerformanceCounterWriter;
        private readonly TransactionCommitRatePerformanceCounterWriter transactionCommitRatePerformanceCounterWriter;
        private readonly TransactionAbortRatePerformanceCounterWriter transactionAbortRatePerformanceCounterWriter;

        private readonly CheckpointManager checkpointManager;
        private readonly RecoveredOrCopiedCheckpointLsn recoveredOrCopiedCheckpointLsn;
        private readonly ReplicatedLogManager replicatedLogManager;
        private readonly TransactionMap transactionMap;
        private readonly OperationProcessor unlockCallbackManager;
        private readonly long flushAtBufferedBytes;
        private readonly ITracer tracer;

        public TransactionManager(
            RecoveredOrCopiedCheckpointLsn recoveredOrCopiedCheckpointLsn,
            TransactionMap transactionMap,
            FabricPerformanceCounterSetInstance instance,
            CheckpointManager checkpointManager,
            ReplicatedLogManager replicatedLogManager,
            OperationProcessor unlockCallbackManager,
            ITracer tracer,
            long flushAtBufferedBytes)
        {
            this.recoveredOrCopiedCheckpointLsn = recoveredOrCopiedCheckpointLsn;
            this.transactionMap = transactionMap;
            this.checkpointManager = checkpointManager;
            this.replicatedLogManager = replicatedLogManager;
            this.unlockCallbackManager = unlockCallbackManager;
            this.tracer = tracer;
            this.flushAtBufferedBytes = flushAtBufferedBytes;

            this.beginTransactionOperationRatePerformanceCounterWriter =
                new BeginTransactionOperationRatePerformanceCounterWriter(instance);
            this.addOperationRatePerformanceCounterWriter =
                new AddOperationRatePerformanceCounterWriter(instance);
            this.transactionCommitRatePerformanceCounterWriter =
                new TransactionCommitRatePerformanceCounterWriter(instance);
            this.transactionAbortRatePerformanceCounterWriter =
                new TransactionAbortRatePerformanceCounterWriter(instance);
            this.addAtomicOperationRatePerformanceCounterWriter =
                new AddAtomicOperationRatePerformanceCounterWriter(instance);
        }

        public TransactionMap TransactionsMap
        {
            get { return this.transactionMap; }
        }

        public void CreateTransactionState(
            Epoch logTailEpoch,
            TransactionLogRecord record)
        {
            switch (record.RecordType)
            {
                case LogRecordType.BeginTransaction:
                    var beginTxRecord = record as BeginTransactionOperationLogRecord;

                    this.beginTransactionOperationRatePerformanceCounterWriter.Increment();

                    if (beginTxRecord.IsSingleOperationTransaction)
                    {
                        this.transactionCommitRatePerformanceCounterWriter.Increment();

                        FabricEvents.Events.AcceptSingleOperationTransaction(
                            tracer.Type,
                            beginTxRecord.Transaction.Id,
                            record.Lsn.LSN);

                        record.Transaction.CommitSequenceNumber = record.Lsn.LSN;
                    }
                    else
                    {
                        FabricEvents.Events.AcceptBeginTransaction(
                            tracer.Type,
                            beginTxRecord.Transaction.Id,
                            record.Lsn.LSN);
                    }

                    beginTxRecord.RecordEpoch = logTailEpoch;
                    this.transactionMap.CreateTransaction(beginTxRecord);
                    break;

                case LogRecordType.Operation:
                    var dataRecord = record as OperationLogRecord;

                    if (dataRecord.Transaction.IsAtomicOperation || dataRecord.IsRedoOnly)
                    {
                        this.addAtomicOperationRatePerformanceCounterWriter.Increment();
                        dataRecord.Transaction.CommitSequenceNumber = record.Lsn.LSN;
                    }
                    else
                    {
                        this.addOperationRatePerformanceCounterWriter.Increment();
                    }

                    FabricEvents.Events.AcceptOperation(tracer.Type, dataRecord.Transaction.Id, record.Lsn.LSN);

                    this.transactionMap.AddOperation(dataRecord);
                    Utility.Assert(
                        (dataRecord.IsEnlistedTransaction == true)
                        || (dataRecord.Lsn <= this.recoveredOrCopiedCheckpointLsn.Value),
                        "(record.IsEnlistedTransaction == true) || (record.LastLogicalSequenceNumber <= this.recoveredOrCopiedCheckpointLsn). record LSN :{0} and recoverd or copied checkpoint lsn:{1}. record.InEnlistedTx :{2}",
                        dataRecord.Lsn, recoveredOrCopiedCheckpointLsn,
                        dataRecord.IsEnlistedTransaction);
                    break;

                case LogRecordType.EndTransaction:
                    var endTxRecord = record as EndTransactionLogRecord;

                    if (endTxRecord.IsCommitted)
                    {
                        this.transactionCommitRatePerformanceCounterWriter.Increment();
                        endTxRecord.Transaction.CommitSequenceNumber = record.Lsn.LSN;
                    }
                    else
                    {
                        this.transactionAbortRatePerformanceCounterWriter.Increment();
                    }

                    FabricEvents.Events.AcceptEndTransaction(
                        tracer.Type,
                        endTxRecord.Transaction.Id,
                        record.Lsn.LSN,
                        endTxRecord.IsCommitted);

                    this.transactionMap.CompleteTransaction(endTxRecord);
                    Utility.Assert(
                        (endTxRecord.IsEnlistedTransaction == true)
                        || (record.Lsn <= this.recoveredOrCopiedCheckpointLsn.Value),
                        "(record.IsEnlistedTransaction == true) || (record.LastLogicalSequenceNumber <= this.recoveredOrCopiedCheckpointLsn). record LSN :{0} and recoverd or copied checkpoint lsn:{1}. record.InEnlistedTx :{2}",
                        record.Lsn,
                        this.recoveredOrCopiedCheckpointLsn.Value.LSN,
                        endTxRecord.IsEnlistedTransaction);
                    break;

                default:
                    Utility.CodingError("Unexpected record type in TransactionsManager CreateTransactionState {0}", record.RecordType);
                    break;
            }
        }

        public async Task<long> AbortTransactionAsync(Transaction transaction)
        {
            LogRecord endTransactionRecord = await this.EndTransactionAsync(transaction, false, true).ConfigureAwait(false);
            return endTransactionRecord.Lsn.LSN;
        }

        public void AddOperation(
            Transaction transaction,
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext)
        {
            var record = new OperationLogRecord(transaction, metaData, undo, redo, operationContext);
            this.ProcessLogicalRecordOnPrimary(record);
        }

        public async Task<long> AddOperationAsync(
            AtomicOperation atomicOperation,
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext)
        {
            var record = new OperationLogRecord(atomicOperation, metaData, undo, redo, operationContext);
            await this.ProcessLogicalRecordOnPrimaryAsync(record).ConfigureAwait(false);

            return record.Lsn.LSN;
        }

        public async Task<long> AddOperationAsync(
            AtomicRedoOperation atomicRedoOperation,
            OperationData metaData,
            OperationData redo,
            object operationContext)
        {
            var record = new OperationLogRecord(atomicRedoOperation, metaData, redo, operationContext);
            await this.ProcessLogicalRecordOnPrimaryAsync(record).ConfigureAwait(false);

            return record.Lsn.LSN;
        }

        public void BeginTransaction(Transaction transaction, OperationData metaData, OperationData undo, OperationData redo, object operationContext)
        {
            var record = new BeginTransactionOperationLogRecord(
                transaction,
                metaData,
                undo,
                redo,
                operationContext,
                isSingleOperationTransaction: false);
            this.ProcessLogicalRecordOnPrimary(record);
        }

        public async Task<long> BeginTransactionAsync(Transaction transaction, OperationData metaData, OperationData undo, OperationData redo, object operationContext)
        {
            var record = new BeginTransactionOperationLogRecord(
                transaction,
                metaData,
                undo,
                redo,
                operationContext,
                isSingleOperationTransaction: true);
            await this.ProcessLogicalRecordOnPrimaryAsync(record).ConfigureAwait(false);
            return record.Lsn.LSN;
        }

        public async Task<long> CommitTransactionAsync(Transaction transaction)
        {
            LogRecord endTransactionRecord = await this.EndTransactionAsync(transaction, true, true).ConfigureAwait(false);
            return endTransactionRecord.Lsn.LSN;
        }

        private void InitiateLogicalRecordProcessingOnPrimary(LogicalLogRecord record)
        {
            this.checkpointManager.ThrowIfThrottled(record);

            long bufferedRecordsSizeBytes = 0;

            switch (record.RecordType)
            {
                case LogRecordType.BeginTransaction:
                case LogRecordType.Operation:
                case LogRecordType.EndTransaction:
                    bufferedRecordsSizeBytes = this.replicatedLogManager.ReplicateAndLog(record, this);
                    break;

                default:
                    Utility.CodingError("Unexpected record type {0} in InitiateLogicalRecordProcessingOnPrimary", record.RecordType);
                    break;
            }

            var addedIndexingRecord = false;
            var addedTruncateHeadRecord = false;

            this.checkpointManager.InsertPhysicalRecordsIfNecessary(out addedIndexingRecord, out addedTruncateHeadRecord);

            if (record.IsLatencySensitiveRecord == true || addedTruncateHeadRecord)
            {
                this.checkpointManager.RequestGroupCommit();
            }
            else if (bufferedRecordsSizeBytes > this.flushAtBufferedBytes)
            {
                {
                    // If there is lot of buffered data, issue a flush
                    var t =
                        this.replicatedLogManager.LogManager.FlushAsync(
                            "InitiateLogicalLogRecordProcessing BufferedRecordsSize: " + bufferedRecordsSizeBytes);
                }
            }

            return;
        }

        internal async Task<EndTransactionLogRecord> EndTransactionAsync(
            TransactionBase transaction,
            bool isCommitted,
            bool isThisReplicaTransaction)
        {
            var record = new EndTransactionLogRecord(
                transaction,
                isCommitted,
                isThisReplicaTransaction);
            await this.ProcessLogicalRecordOnPrimaryAsync(record).ConfigureAwait(false);

            return record;
        }

        private void ProcessLogicalRecordOnPrimary(TransactionLogRecord record)
        {
            this.ReplicateAndLogLogicalRecord(record);

            Task.WhenAll(record.AwaitApply(), LoggingReplicator.AwaitReplication(record, this.replicatedLogManager)).IgnoreException().ContinueWith(
                task =>
                {
                    // Simply retrieve task exception to mark it as handled
                    if (task.Exception != null)
                    {
                        Utility.Assert(task.IsFaulted == true, "task.IsFaulted == true");
                    }

                    this.InvokeUnlockCallback(record);
                }).IgnoreExceptionVoid();
        }

        /// <summary>
        /// Primary: Process the logical log record.
        /// </summary>
        /// <param name="record"></param>
        /// <returns></returns>
        /// <remarks>
        /// Wait for the logical record to be flushed and replicated.
        /// If it is a barrier, increase stable lsn.
        /// If it is a operation record, unlock it.
        /// </remarks>
        private async Task ProcessLogicalRecordOnPrimaryAsync(TransactionLogRecord record)
        {
            Exception exception = null;
            this.ReplicateAndLogLogicalRecord(record);

            try
            {
                // Wait for the record to be flushed and replicated
                await Task.WhenAll(record.AwaitApply(), LoggingReplicator.AwaitReplication(record, this.replicatedLogManager)).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                exception = e;
            }
            finally
            {
                this.InvokeUnlockCallback(record);
            }

            // Throw TransactionFaultedException if there has been any failure during replication and apply awaits.
            if (exception != null)
            {
                throw new TransactionFaultedException(exception.Message);
            }

            return;
        }

        private void ReplicateAndLogLogicalRecord(TransactionLogRecord record)
        {
            this.InitiateLogicalRecordProcessingOnPrimary(record);
            return;
        }

        private void InvokeUnlockCallback(TransactionLogRecord record)
        {
            this.unlockCallbackManager.Unlock(record);
        }
    }
}
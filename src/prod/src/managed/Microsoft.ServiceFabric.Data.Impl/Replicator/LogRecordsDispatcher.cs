// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Common.Tracing;

    internal class LogRecordsDispatcher
    {
        private readonly object dispatchLock = new object();

        private List<LogRecord> concurrentRecords;

        private List<LoggedRecords> flushedRecords;

        private int numberOfSpawnedTransactions;

        private int processingIndex;

        private List<LoggedRecords> processingRecords;

        private IOperationProcessor processor;

        private ITracer tracer;

        public LogRecordsDispatcher(IOperationProcessor processor)
        {
            this.flushedRecords = null;
            this.processingRecords = null;
            this.processingIndex = 0;
            this.concurrentRecords = new List<LogRecord>();
            this.numberOfSpawnedTransactions = 0;
            this.processor = processor;
        }

        public ITracer Tracer
        {
            get { return this.tracer; }

            set { this.tracer = value; }
        }

        public void DispatchLoggedRecords(LoggedRecords loggedRecords)
        {
            lock (this.dispatchLock)
            {
                if (this.processingRecords != null)
                {
                    if (this.flushedRecords == null)
                    {
                        this.flushedRecords = new List<LoggedRecords>();
                    }

                    this.flushedRecords.Add(loggedRecords);
                    return;
                }

                Utility.Assert(this.flushedRecords == null, "this.flushedRecords == null");
                Utility.Assert(this.processingIndex == 0, "this.processingIndex == 0");
                this.processingRecords = new List<LoggedRecords>();
                this.processingRecords.Add(loggedRecords);
            }

            this.StartProcessingLoggedRecords().IgnoreExceptionVoid();
            return;
        }

        private bool IsProcessingCompleted()
        {
            if (this.processingRecords.Count == 0)
            {
                lock (this.dispatchLock)
                {
                    this.processingRecords = this.flushedRecords;
                    if (this.processingRecords == null)
                    {
                        return true;
                    }

                    this.flushedRecords = null;
                }
            }

            return false;
        }

        private void PrepareToProcessRecord(LogRecord record, RecordProcessingMode processingMode)
        {
            Utility.Assert(
                processingMode < RecordProcessingMode.ProcessImmediately,
                "processingMode < RecordProcessingMode.ProcessImmediately");

            var logicalRecord = record as LogicalLogRecord;
            if (logicalRecord != null)
            {
                this.processor.PrepareToProcessLogicalRecord();
            }
            else 
            {
                this.processor.PrepareToProcessPhysicalRecord();
            }
        }

        private async Task ProcessAtomicOperations(List<LogRecord> atomicOperations)
        {
            var parallelTasks = new Task[atomicOperations.Count];
            for (var i = 0; i < atomicOperations.Count; i++)
            {
                parallelTasks[i] = this.processor.ProcessLoggedRecordAsync(atomicOperations[i]);
            }

            await Task.WhenAll(parallelTasks).ConfigureAwait(false);

            return;
        }

        private void ProcessConcurrentTransactions(Dictionary<long, List<TransactionLogRecord>> concurrentTransactions)
        {
            foreach (var transaction in concurrentTransactions)
            {
                Task.Factory.StartNew<Task>(this.ProcessSpawnedTransaction, transaction.Value).IgnoreExceptionVoid();
            }

            return;
        }

        private void ProcessedSpawnedTransaction()
        {
            var remainingTransactions = Interlocked.Decrement(ref this.numberOfSpawnedTransactions);
            Utility.Assert(remainingTransactions >= 0, "remainingTransactions >= 0");
            if (remainingTransactions == 0)
            {
                this.concurrentRecords.Clear();
                var isProcessingCompleted = this.IsProcessingCompleted();
                if (isProcessingCompleted == false)
                {
                    this.StartProcessingLoggedRecords().IgnoreExceptionVoid();
                }
            }

            return;
        }

        private async Task<bool> ProcessLoggedRecords()
        {
            while (this.processingRecords.Count > 0)
            {
                var isBarrierRecord = false;
                do
                {
                    var exception = this.processingRecords[0].Exception;
                    var records = this.processingRecords[0].Records;
                    if (exception == null)
                    {
                        // Copy from processingRecords to concurrentRecords until barrierRecord
                        while (this.processingIndex < records.Count)
                        {
                            var record = records[this.processingIndex];

                            // TODO: GopalK, currently all barrier operations are full barriers. 
                            // Add support for weaker forms of barriers as well.
                            isBarrierRecord = LoggingReplicator.IsBarrierRecord(record);

                            // If barrier is hit, process records we currently have before barrier
                            if (isBarrierRecord == true)
                            {
                                if (this.concurrentRecords.Count > 0)
                                {
                                    this.processor.UpdateDispatchingBarrierTask(record.AppliedTask);

                                    break;
                                }
                            }

                            ++this.processingIndex;
                            this.concurrentRecords.Add(record);
                            if (isBarrierRecord == true)
                            {
                                break;
                            }
                        }
                    }
                    else if (this.concurrentRecords.Count > 0)
                    {
                        // If an exception happened, process what we have so far
                        isBarrierRecord = true;
                    }
                    else
                    {
                        // If first record is an exception, process everything with exception
                        while (this.processingIndex < records.Count)
                        {
                            var record = records[this.processingIndex];
                            ++this.processingIndex;
                            await
                                this.processor.ImmediatelyProcessRecord(
                                    record,
                                    exception,
                                    RecordProcessingMode.ProcessImmediately).ConfigureAwait(false);
                        }
                    }

                    if (this.processingIndex == records.Count)
                    {
                        this.processingRecords.RemoveAt(0);
                        this.processingIndex = 0;
                    }

                    if (isBarrierRecord == true)
                    {
                        break;
                    }
                } while (this.processingRecords.Count > 0);

                // If we do have records to process, but didn't hit barrier, we do not process yet
                if (isBarrierRecord == false)
                {
                    if (this.concurrentRecords.Count > 0)
                    {
                        var lastIndex = this.concurrentRecords.Count - 1;

                        FabricEvents.Events.ProcessLoggedRecords(
                            this.tracer.Type,
                            this.concurrentRecords.Count,
                            this.concurrentRecords[0].Lsn.LSN,
                            this.concurrentRecords[lastIndex].Lsn.LSN,
                            this.concurrentRecords[0].Psn.PSN,
                            this.concurrentRecords[lastIndex].Psn.PSN);
                    }

                    break;
                }

                for (var i = 0; i < this.concurrentRecords.Count; i++)
                {
                    var record = this.concurrentRecords[i];
                    var processingMode = this.processor.IdentifyProcessingModeForRecord(record);

                    // processingMode == Normal, ApplyImmediately
                    if (processingMode < RecordProcessingMode.ProcessImmediately)
                    {
                        this.PrepareToProcessRecord(record, processingMode);
                    }

                    // processingMode == ApplyImmediately, ProcessImmediately
                    if (processingMode > RecordProcessingMode.Normal)
                    {
                        await this.processor.ImmediatelyProcessRecord(record, null, processingMode).ConfigureAwait(false);
                        this.concurrentRecords.RemoveAt(i);
                        i--;
                    }
                }

                if (this.concurrentRecords.Count == 0)
                {
                    continue;
                }
                else if (this.concurrentRecords.Count == 1)
                {
                    await this.processor.ProcessLoggedRecordAsync(this.concurrentRecords[0]).ConfigureAwait(false);
                }
                else
                {
                    var numberOfTransactions = await this.SeparateTransactions().ConfigureAwait(false);
                    if (numberOfTransactions > 1)
                    {
                        return false;
                    }
                }

                this.concurrentRecords.Clear();
            }

            return true;
        }

        private async Task ProcessSpawnedAtomicOperations(object state)
        {
            try
            {
                var atomicOperations = (List<LogRecord>) state;
                await this.ProcessAtomicOperations(atomicOperations).ConfigureAwait(false);
                this.ProcessedSpawnedTransaction();
            }
            catch (Exception e)
            {
                Utility.ProcessUnexpectedException(
                    "ProcessSpawnedAtomicOperations",
                    this.tracer,
                    "processing spawned atomic operations",
                    e);
            }

            return;
        }

        private async Task ProcessSpawnedTransaction(object state)
        {
            try
            {
                var transaction = (List<TransactionLogRecord>) state;
                await this.ProcessTransaction(transaction).ConfigureAwait(false);
                this.ProcessedSpawnedTransaction();
            }
            catch (Exception e)
            {
                Utility.ProcessUnexpectedException(
                    "ProcessSpawnedTransaction",
                    this.tracer,
                    "processing spawned transaction",
                    e);
            }

            return;
        }

        private async Task ProcessTransaction(List<TransactionLogRecord> transaction)
        {
            foreach (LogRecord record in transaction)
            {
                await this.processor.ProcessLoggedRecordAsync(record).ConfigureAwait(false);
            }

            return;
        }

        private async Task<int> SeparateTransactions()
        {
            Utility.Assert(this.concurrentRecords.Count > 0, "this.concurrentRecords.Count > 0");

            var numberOfTransactions = 0;
            List<LogRecord> atomicOperations = null;
            Dictionary<long, List<TransactionLogRecord>> concurrentTransactions = new Dictionary<long, List<TransactionLogRecord>>();

            for (var i = 0; i < this.concurrentRecords.Count; i++)
            {
                var record = this.concurrentRecords[i];
                var operationLogRecord = record as OperationLogRecord;
                var transactionRecord = record as TransactionLogRecord;
                if (transactionRecord == null
                    || (operationLogRecord != null && operationLogRecord.Transaction.IsAtomicOperation))
                {
                    if (atomicOperations == null)
                    {
                        atomicOperations = new List<LogRecord>();
                        ++numberOfTransactions;
                    }

                    atomicOperations.Add(record);
                }
                else
                {
                    List<TransactionLogRecord> transaction = null;
                    bool foundTx = concurrentTransactions.TryGetValue(transactionRecord.Transaction.Id, out transaction);

                    if (foundTx)
                    {
                        Utility.Assert(transaction != null, "transaction is null");
                        transaction.Add(transactionRecord);
                    }
                    else
                    {
                        transaction = new List<TransactionLogRecord>();
                        transaction.Add(transactionRecord);
                        concurrentTransactions[transactionRecord.Transaction.Id] = transaction;
                        ++numberOfTransactions;
                    }
                }
            }

            Utility.Assert(numberOfTransactions > 0, "numberOfTransactions > 0");
            if (numberOfTransactions == 1)
            {
                if (atomicOperations != null)
                {
                    await this.ProcessAtomicOperations(atomicOperations).ConfigureAwait(false);
                }
                else
                {
                    var transactionRecord = (TransactionLogRecord)this.concurrentRecords[0]; // this has to be a transaction
                    await this.ProcessTransaction(concurrentTransactions[transactionRecord.Transaction.Id]).ConfigureAwait(false);
                }
            }
            else
            {
                this.numberOfSpawnedTransactions = numberOfTransactions;

                if (atomicOperations != null)
                {
                    Task.Factory.StartNew<Task>(this.ProcessSpawnedAtomicOperations, atomicOperations)
                        .IgnoreExceptionVoid();
                }

                if (concurrentTransactions != null)
                {
                    this.ProcessConcurrentTransactions(concurrentTransactions);
                }
            }

            return numberOfTransactions;
        }

        private async Task StartProcessingLoggedRecords()
        {
            try
            {
                while (true)
                {
                    Utility.Assert(this.processingRecords.Count > 0, "this.processingRecords.Count > 0");
                    Utility.Assert(this.numberOfSpawnedTransactions == 0, "this.numberOfSpawnedTransactions == 0");
                    var continueProcessing = await this.ProcessLoggedRecords().ConfigureAwait(false);
                    if (continueProcessing == false)
                    {
                        return;
                    }

                    Utility.Assert(this.processingRecords.Count == 0, "this.processingRecords.Count == 0");
                    var isProcessingCompleted = this.IsProcessingCompleted();
                    if (isProcessingCompleted == true)
                    {
                        break;
                    }
                }
            }
            catch (Exception e)
            {
                Utility.ProcessUnexpectedException(
                    "StartProcessingLoggedRecords",
                    this.tracer,
                    "processing logged records",
                    e);
            }

            return;
        }
    }
}
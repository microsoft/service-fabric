// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System.Fabric;
    using System.Globalization;
    using System.Fabric.Common.Tracing;

    /// <summary>
    /// Class used for linking log records.
    /// </summary>
    /// <remarks>Does not do physical log linking.</remarks>
    internal class LogRecordsMap
    {
        /// <summary>
        /// Begin checkpoint processing assert message.
        /// TODO: Structured tracing.
        /// </summary>
        public const string BeginCheckpointProcessingAssertMessage =
            "(record.RecordPosition <= recoveredLastCompletedBeginCheckpointRecordPosition) || (this.lastCompletedEndCheckpointRecord != null), RecordPosition: {0} RecoveredLastCompletedBeginCheckpointRecordPosition: {1}";

        /// <summary>
        /// End transaction processing assert message
        /// TODO: Structured tracing.
        /// </summary>
        public const string EndTransactionProcessingAssertMessage =
            "(endTransactionRecord.IsEnlistedTransaction == true) || (record.RecordPosition < recoveredLastCompletedBeginCheckpointRecordPosition) RecordPosition: {0} RecoveredLastCompletedBeginCheckpointRecordPosition: {1} ";

        // Privates only used internally.

        /// <summary>
        /// The last indexing log record.
        /// </summary>
        /// <remarks>Restore only.</remarks>
        private readonly IndexingLogRecord firstIndexingLogRecord;

        /// <summary>
        /// Mode for the log record mapping
        /// </summary>
        private Mode mode;

        /// <summary>
        /// Begin Checkpoint Log Record for the last completed checkpoint that was recovered.
        /// </summary>
        private BeginCheckpointLogRecord recoveredLastCompletedBeginCheckpointRecord;

        /// <summary>
        /// End Checkpoint Log Record for the last completed checkpoint that was recovered.
        /// </summary>
        private EndCheckpointLogRecord recoveredLastEndCheckpointRecord;

        /// <summary>
        /// The tracer.
        /// </summary>
        private ITracer tracer;

        /// <summary>
        /// Initializes a new instance of the LogRecordsMap class for restore.
        /// </summary>
        /// <param name="indexingLogRecord">The string indexing log record.</param>
        /// <param name="tracer"></param>
        public LogRecordsMap(
            IndexingLogRecord indexingLogRecord,
            ITracer tracer)
        {
            this.mode = Mode.Restore;

            this.LastLogicalSequenceNumber = indexingLogRecord.Lsn;
            this.TransactionsMap = new TransactionMap();
            this.CurrentLogTailEpoch = indexingLogRecord.CurrentEpoch;
            this.LastRecoveredAtomicRedoOperationLsn = long.MinValue;
            this.LastStableLsn = LogicalSequenceNumber.InvalidLsn;
            this.LastPhysicalRecord = PhysicalLogRecord.InvalidPhysicalLogRecord;

            this.ProgressVector = ProgressVector.ZeroProgressVector;

            this.firstIndexingLogRecord = indexingLogRecord;

            this.tracer = tracer;
        }
        
        /// <summary>
        /// Initializes a new instance of the LogRecordsMap class for recovery.
        /// </summary>
        /// <param name="startingLogicalSequenceNumber"></param>
        /// <param name="transactionsMap"></param>
        /// <param name="currentLogTailEpoch"></param>
        /// <param name="lastStableLsn"></param>
        /// <param name="progressVector"></param>
        /// <param name="recoveredLastCompletedBeginCheckpointRecord"></param>
        /// <param name="tracer"></param>
        /// <param name="recoveredLastEndCheckpointRecord"></param>
        public LogRecordsMap(
            LogicalSequenceNumber startingLogicalSequenceNumber,
            TransactionMap transactionsMap,
            Epoch currentLogTailEpoch,
            LogicalSequenceNumber lastStableLsn,
            ProgressVector progressVector,
            BeginCheckpointLogRecord recoveredLastCompletedBeginCheckpointRecord,
            ITracer tracer,
            EndCheckpointLogRecord recoveredLastEndCheckpointRecord)
        {
            this.mode = Mode.Recovery;

            this.LastLogicalSequenceNumber = startingLogicalSequenceNumber;
            this.TransactionsMap = transactionsMap;
            this.CurrentLogTailEpoch = currentLogTailEpoch;
            this.LastStableLsn = lastStableLsn;
            this.LastPhysicalRecord = PhysicalLogRecord.InvalidPhysicalLogRecord;

            this.ProgressVector = progressVector;
            this.recoveredLastCompletedBeginCheckpointRecord = recoveredLastCompletedBeginCheckpointRecord;

            this.tracer = tracer;
            this.recoveredLastEndCheckpointRecord = recoveredLastEndCheckpointRecord;
        }

        /// <summary>
        /// Mode for the log records mapping.
        /// </summary>
        public enum Mode : int
        {
            /// <summary>
            /// Recovery mode.
            /// </summary>
            Recovery = 0,

            /// <summary>
            /// Restore mode.
            /// </summary>
            Restore = 1
        }

        /// <summary>
        /// Gets the current epoch.
        /// </summary>
        public Epoch CurrentLogTailEpoch { get; private set; }

        /// <summary>
        /// Gets the end checkpoint log record for the last completed checkpoint.
        /// </summary>
        public EndCheckpointLogRecord LastCompletedEndCheckpointRecord { get; private set; }

        /// <summary>
        /// Gets the begin checkpoint log record of the last in progress checkpoint.
        /// </summary>
        public BeginCheckpointLogRecord LastInProgressCheckpointRecord { get; private set; }

        /// <summary>
        /// Gets the last seen linked physical log record
        /// </summary>
        /// <remarks>Linked physical log records (End checkpoint or Truncate head) are records that point at either another linked physical log record or indexing log record.</remarks>
        public PhysicalLogRecord LastLinkedPhysicalRecord { get; private set; }

        /// <summary>
        /// Gets the logical sequence number.
        /// </summary>
        public LogicalSequenceNumber LastLogicalSequenceNumber { get; private set; }

        /// <summary>
        /// Gets the last seen physical log record.
        /// </summary>
        public PhysicalLogRecord LastPhysicalRecord { get; private set; }

        /// <summary>
        /// Gets the last atomic redo operation's logical sequence number.
        /// </summary>
        public long LastRecoveredAtomicRedoOperationLsn { get; private set; }

        /// <summary>
        /// Gets the last logical sequence number.
        /// </summary>
        public LogicalSequenceNumber LastStableLsn { get; private set; }

        /// <summary>
        /// Gets the progress ProgressVectorEntry.
        /// </summary>
        public ProgressVector ProgressVector { get; private set; }

        /// <summary>
        /// Gets the transaction map.
        /// </summary>
        public TransactionMap TransactionsMap { get; private set; }

        /// <summary>
        /// Gets the recovered truncation timestamp in ticks
        /// </summary>
        public long LastPeriodicTruncationTimeTicks { get; private set; }

        /// <summary>
        /// Processes the log record.
        /// </summary>
        /// <param name="logRecord">The log record to be processed.</param>
        public void ProcessLogRecord(LogRecord logRecord)
        {
            var isRecoverableRecord = true;

            this.ProcessLogRecord(logRecord, out isRecoverableRecord);
        }

        /// <summary>
        /// Processes the log record.
        /// </summary>
        /// <param name="logRecord">The log record to be processed.</param>
        /// <param name="isRecoverableRecord">Is this a recoverable record.</param>
        public void ProcessLogRecord(LogRecord logRecord, out bool isRecoverableRecord)
        {
            isRecoverableRecord = true;

            var physicalLogRecord = logRecord as PhysicalLogRecord;
            if (physicalLogRecord != null)
            {
                this.ProcessLogRecord(physicalLogRecord);
            }

            switch (logRecord.RecordType)
            {
                // Transaction related log records
                case LogRecordType.BeginTransaction:
                    var beginTransactionRecord =
                        (BeginTransactionOperationLogRecord) logRecord;
                    this.ProcessLogRecord(beginTransactionRecord, out isRecoverableRecord);
                    break;
                case LogRecordType.Operation:
                    var operationLogRecord = (OperationLogRecord) logRecord;
                    this.ProcessLogRecord(operationLogRecord, out isRecoverableRecord);
                    break;
                case LogRecordType.EndTransaction:
                    var endTransactionRecord = (EndTransactionLogRecord) logRecord;
                    this.ProcessLogRecord(endTransactionRecord, out isRecoverableRecord);
                    break;
                case LogRecordType.Barrier:
                    var barrierRecord = (BarrierLogRecord) logRecord;
                    this.ProcessLogRecord(barrierRecord, out isRecoverableRecord);
                    break;

                // Replicated logical record.
                case LogRecordType.Backup:
                    var backupLogRecord = (BackupLogRecord) logRecord;
                    this.ProcessLogRecord(backupLogRecord, out isRecoverableRecord);
                    break;

                // Not replicated logical log record.
                case LogRecordType.UpdateEpoch:
                    var updateEpochLogRecord = (UpdateEpochLogRecord) logRecord;
                    this.ProcessLogRecord(updateEpochLogRecord, out isRecoverableRecord);
                    break;

                // Checkpoint cases
                case LogRecordType.BeginCheckpoint:
                    var beginCheckpointRecord = (BeginCheckpointLogRecord) logRecord;
                    this.ProcessLogRecord(beginCheckpointRecord, out isRecoverableRecord);
                    break;
                case LogRecordType.EndCheckpoint:
                    var endCheckpointLogRecord = (EndCheckpointLogRecord) logRecord;
                    this.ProcessLogRecord(endCheckpointLogRecord, out isRecoverableRecord);
                    break;
                case LogRecordType.CompleteCheckpoint:
                    var completeCheckpointLogRecord = (CompleteCheckpointLogRecord) logRecord;
                    this.ProcessLogRecord(completeCheckpointLogRecord, out isRecoverableRecord);
                    break;

                case LogRecordType.TruncateHead:
                    var truncateHeadLogRecord = (TruncateHeadLogRecord) logRecord;
                    this.ProcessLogRecord(truncateHeadLogRecord, out isRecoverableRecord);
                    break;
                case LogRecordType.Indexing:
                    var indexingLogRecord = (IndexingLogRecord) logRecord;
                    this.ProcessLogRecord(indexingLogRecord, out isRecoverableRecord);
                    break;

                case LogRecordType.Information:
                    var informationRecord = (InformationLogRecord) logRecord;
                    this.ProcessLogRecord(informationRecord, out isRecoverableRecord);
                    break;
                case LogRecordType.TruncateTail:
                    var truncateTailLogRecord = (TruncateTailLogRecord) logRecord;
                    this.ProcessLogRecord(truncateTailLogRecord, out isRecoverableRecord);
                    break;
                default:
                    Utility.CodingError("Unexpected record type {0}", logRecord.RecordType);
                    break;
            }
        }

        /// <summary>
        /// Links to the last "Linked Physical Log Record".
        /// </summary>
        /// <param name="physicalLogRecord">The physical log record.</param>
        private void ProcessLogRecord(PhysicalLogRecord physicalLogRecord)
        {
            this.LastPhysicalRecord = physicalLogRecord;
            this.LastPhysicalRecord.LinkedPhysicalRecord = this.LastLinkedPhysicalRecord;
        }

        /// <summary>
        /// Process the begin transaction log record.
        /// </summary>
        /// <param name="beginTransactionRecord">The begin transaction record to be processed.</param>
        /// <param name="isRecoverableRecord">Is this a recoverable record.</param>
        private void ProcessLogRecord(
            BeginTransactionOperationLogRecord beginTransactionRecord,
            out bool isRecoverableRecord)
        {
            isRecoverableRecord = true;

            this.LastLogicalSequenceNumber++;
            Utility.Assert(
                beginTransactionRecord.Lsn == this.LastLogicalSequenceNumber,
                "beginTransactionRecord.LastLogicalSequenceNumber == lsn");
            beginTransactionRecord.RecordEpoch = this.CurrentLogTailEpoch;
            this.TransactionsMap.CreateTransaction(beginTransactionRecord);
        }

        /// <summary>
        /// Process the operation transaction log record.
        /// </summary>
        /// <param name="operationLogRecord">The operation transaction record to be processed.</param>
        /// <param name="isRecoverableRecord">Is this a recoverable record.</param>
        private void ProcessLogRecord(OperationLogRecord operationLogRecord, out bool isRecoverableRecord)
        {
            isRecoverableRecord = true;

            this.LastLogicalSequenceNumber++;
            Utility.Assert(
                operationLogRecord.Lsn == this.LastLogicalSequenceNumber,
                "operationRecord.LastLogicalSequenceNumber == lsn");
            this.TransactionsMap.AddOperation(operationLogRecord);

            if (Mode.Recovery == this.mode)
            {
                Utility.Assert(
                    (operationLogRecord.IsEnlistedTransaction == true)
                    || (operationLogRecord.RecordPosition
                        < this.recoveredLastCompletedBeginCheckpointRecord.RecordPosition),
                    "(operationLogRecord.IsEnlistedTransaction == true) || (operationLogRecord.RecordPosition < this.recoveredLastCompletedBeginCheckpointRecord.RecordPosition). " +
                    "OperationLogRecord: {0}, this.recoveredLastCompletedBeginCheckpointRecord: {1}",
                    operationLogRecord, this.recoveredLastCompletedBeginCheckpointRecord);
            }
            else
            {
                Utility.Assert(
                    LogRecord.InvalidRecordPosition == operationLogRecord.RecordPosition,
                    "LogRecord.INVALID_RECORD_POSITION == operationLogRecord.RecordPosition. OperationLogRecord: {0}",
                    operationLogRecord);
            }

            isRecoverableRecord = operationLogRecord.IsEnlistedTransaction == true;

            if (operationLogRecord.IsRedoOnly)
            {
                this.LastRecoveredAtomicRedoOperationLsn = operationLogRecord.Lsn.LSN;

                var message = string.Format(
                    CultureInfo.InvariantCulture,
                    "RedoOnly Operation Epoch: <{0}, {1}>",
                    this.ProgressVector.LastProgressVectorEntry.Epoch.DataLossNumber,
                    this.ProgressVector.LastProgressVectorEntry.Epoch.ConfigurationNumber);

                FabricEvents.Events.LogRecordsMap(
                    this.tracer.Type,
                    operationLogRecord.RecordType.ToString(),
                    this.LastRecoveredAtomicRedoOperationLsn,
                    message);
            }
        }

        /// <summary>
        /// Process the end transaction log record.
        /// </summary>
        /// <param name="endTransactionLogRecord">The end transaction record to be processed.</param>
        /// <param name="isRecoverableRecord">Is this a recoverable record.</param>
        private void ProcessLogRecord(EndTransactionLogRecord endTransactionLogRecord, out bool isRecoverableRecord)
        {
            this.LastLogicalSequenceNumber++;

            Utility.Assert(
                endTransactionLogRecord.Lsn == this.LastLogicalSequenceNumber,
                "endTransactionRecord.LastLogicalSequenceNumber == lsn");
            this.TransactionsMap.CompleteTransaction(endTransactionLogRecord);

            if (Mode.Recovery == this.mode)
            {
                Utility.Assert(
                    endTransactionLogRecord.IsEnlistedTransaction == true
                    || endTransactionLogRecord.RecordPosition
                    < this.recoveredLastCompletedBeginCheckpointRecord.RecordPosition,
                    EndTransactionProcessingAssertMessage,
                    endTransactionLogRecord.RecordPosition, this.recoveredLastCompletedBeginCheckpointRecord.RecordPosition);
            }
            else
            {
                Utility.Assert(
                    LogRecord.InvalidRecordPosition == endTransactionLogRecord.RecordPosition,
                    "LogRecord.INVALID_RECORD_POSITION == endTransactionLogRecord.RecordPosition ({0})",
                    endTransactionLogRecord);
            }

            isRecoverableRecord = endTransactionLogRecord.IsEnlistedTransaction == true;
        }

        /// <summary>
        /// Process the barrier log record.
        /// </summary>
        /// <param name="barrierLogRecord">The barrier record to be processed.</param>
        /// <param name="isRecoverableRecord">Is this a recoverable record.</param>
        private void ProcessLogRecord(BarrierLogRecord barrierLogRecord, out bool isRecoverableRecord)
        {
            isRecoverableRecord = true;

            this.LastLogicalSequenceNumber++;

            Utility.Assert(
                barrierLogRecord.Lsn == this.LastLogicalSequenceNumber,
                "barrierRecord.LastLogicalSequenceNumber == lsn");
            this.LastStableLsn = barrierLogRecord.LastStableLsn;

            this.TransactionsMap.RemoveStableTransactions(this.LastStableLsn);
        }

        /// <summary>
        /// Process the barrier log record.
        /// </summary>
        /// <param name="backupLogRecord">The backupLogRecord record to be processed.</param>
        /// <param name="isRecoverableRecord">Is this a recoverable record.</param>
        private void ProcessLogRecord(BackupLogRecord backupLogRecord, out bool isRecoverableRecord)
        {
            // For incremental backup chaining, we need to recover the backup log records.
            isRecoverableRecord = true;

            this.LastLogicalSequenceNumber++;

            Utility.Assert(
                backupLogRecord.Lsn == this.LastLogicalSequenceNumber,
                "backupLogRecord.LastLogicalSequenceNumber == lsn");
        }

        /// <summary>
        /// Process the begin checkpoint log record.
        /// </summary>
        /// <param name="beginCheckpointLogRecord">The begin checkpoint record to be processed.</param>
        /// <param name="isRecoverableRecord">Is this a recoverable record.</param>
        private void ProcessLogRecord(BeginCheckpointLogRecord beginCheckpointLogRecord, out bool isRecoverableRecord)
        {
            isRecoverableRecord = true;

            if (Mode.Recovery == this.mode)
            {
                Utility.Assert(
                    beginCheckpointLogRecord.RecordPosition <= this.recoveredLastCompletedBeginCheckpointRecord.RecordPosition ||
                    this.LastCompletedEndCheckpointRecord != null,
                    BeginCheckpointProcessingAssertMessage,
                    beginCheckpointLogRecord.RecordPosition, this.recoveredLastCompletedBeginCheckpointRecord.RecordPosition);
            }
            else
            {
                Utility.Assert(
                    LogRecord.InvalidRecordPosition == beginCheckpointLogRecord.RecordPosition,
                    "LogRecord.INVALID_RECORD_POSITION == beginCheckpointLogRecord.RecordPosition ({0})",
                    beginCheckpointLogRecord);
            }

            var failedBarrierCheck = true;
            beginCheckpointLogRecord.EarliestPendingTransaction =
                this.TransactionsMap.GetEarliestPendingTransaction(
                    beginCheckpointLogRecord.Lsn.LSN,
                    out failedBarrierCheck);
            Utility.Assert(failedBarrierCheck == false, "failedBarrierCheck == false during recovery");

            if (Mode.Recovery == this.mode)
            {
                if (beginCheckpointLogRecord.RecordPosition
                    == this.recoveredLastCompletedBeginCheckpointRecord.RecordPosition)
                {
                    Utility.Assert(
                        this.CurrentLogTailEpoch == this.ProgressVector.LastProgressVectorEntry.Epoch,
                        "this.currentLogTailEpoch == this.progressVector.LastProgressVectorEntry.Epoch");

                    Utility.Assert(
                        this.CurrentLogTailEpoch == beginCheckpointLogRecord.ProgressVector.LastProgressVectorEntry.Epoch,
                        "this.currentLogTailEpoch == beginCheckpointRecord.ProgressVector.LastProgressVectorEntry.Epoch");

                    this.LastInProgressCheckpointRecord = beginCheckpointLogRecord;
                }
            }
            else
            {
                this.LastInProgressCheckpointRecord = beginCheckpointLogRecord;
            }
        }

        /// <summary>
        /// Process the end checkpoint log record.
        /// </summary>
        /// <param name="endCheckpointLogRecord">The end checkpoint record to be processed.</param>
        /// <param name="isRecoverableRecord">Is this a recoverable record.</param>
        private void ProcessLogRecord(EndCheckpointLogRecord endCheckpointLogRecord, out bool isRecoverableRecord)
        {
            if (this.mode == Mode.Restore)
            {
                endCheckpointLogRecord.HeadRecord = this.firstIndexingLogRecord;
            }

            this.LastStableLsn = endCheckpointLogRecord.LastStableLsn;
            this.LastLinkedPhysicalRecord = endCheckpointLogRecord;

            if (Mode.Recovery == this.mode)
            {
                if (endCheckpointLogRecord.RecordPosition == this.recoveredLastEndCheckpointRecord.RecordPosition)
                {
                    Utility.Assert(
                        this.LastStableLsn >= this.recoveredLastCompletedBeginCheckpointRecord.Lsn,
                        "this.lastStableLsn ({0}) >= recoveredLastCompletedBeginCheckpointRecord.LastLogicalSequenceNumber ({1}).",
                        this.LastStableLsn, this.recoveredLastCompletedBeginCheckpointRecord.Lsn);
                }
            }

            this.LastCompletedEndCheckpointRecord = endCheckpointLogRecord;

            if (this.LastInProgressCheckpointRecord != null)
            {
                this.LastInProgressCheckpointRecord.LastStableLsn = this.LastStableLsn;
                this.LastCompletedEndCheckpointRecord.LastCompletedBeginCheckpointRecord =
                    this.LastInProgressCheckpointRecord;
                this.LastInProgressCheckpointRecord = null;
            }

            isRecoverableRecord = false;
        }

        /// <summary>
        /// Process the complete checkpoint log record.
        /// </summary>
        /// <param name="completeCheckpointLogRecord">The complete checkpoint record to be processed.</param>
        /// <param name="isRecoverableRecord">Is this a recoverable record.</param>
        private void ProcessLogRecord(
            CompleteCheckpointLogRecord completeCheckpointLogRecord,
            out bool isRecoverableRecord)
        {
            if (this.mode == Mode.Restore)
            {
                completeCheckpointLogRecord.HeadRecord = this.firstIndexingLogRecord;
            }

            isRecoverableRecord = false;
            this.LastLinkedPhysicalRecord = completeCheckpointLogRecord;
        }

        /// <summary>
        /// Process the truncate head log record.
        /// </summary>
        /// <param name="truncateHeadLogRecord">The truncate head record to be processed.</param>
        /// <param name="isRecoverableRecord">Is this a recoverable record.</param>
        private void ProcessLogRecord(TruncateHeadLogRecord truncateHeadLogRecord, out bool isRecoverableRecord)
        {
            if (this.mode == Mode.Restore)
            {
                truncateHeadLogRecord.HeadRecord = this.firstIndexingLogRecord;
            }

            this.LastLinkedPhysicalRecord = truncateHeadLogRecord;

            isRecoverableRecord = false;

            this.LastPeriodicTruncationTimeTicks = truncateHeadLogRecord.PeriodicTruncationTimestampTicks;
        }

        /// <summary>
        /// Process the indexing log record.
        /// </summary>
        /// <param name="indexingLogRecord">The indexing log record to be processed.</param>
        /// <param name="isRecoverableRecord">Is this a recoverable record.</param>
        private void ProcessLogRecord(IndexingLogRecord indexingLogRecord, out bool isRecoverableRecord)
        {
            isRecoverableRecord = false;

            Utility.Assert(
                indexingLogRecord.CurrentEpoch == this.CurrentLogTailEpoch,
                "indexingRecord.CurrentEpoch == this.currentLogTailEpoch");
        }

        /// <summary>
        /// Process the information log record.
        /// </summary>
        /// <param name="informationLogRecord">The information log record to be processed.</param>
        /// <param name="isRecoverableRecord">Is this a recoverable record.</param>
        private void ProcessLogRecord(InformationLogRecord informationLogRecord, out bool isRecoverableRecord)
        {
            isRecoverableRecord = informationLogRecord.IsBarrierRecord;
        }

        /// <summary>
        /// Process the truncate tail log record.
        /// </summary>
        /// <param name="truncateTailLogRecord">The truncate tail log record to be processed.</param>
        /// <param name="isRecoverableRecord">Is this a recoverable record.</param>
        private void ProcessLogRecord(TruncateTailLogRecord truncateTailLogRecord, out bool isRecoverableRecord)
        {
            isRecoverableRecord = false;
        }

        /// <summary>
        /// Process the update epoch log record.
        /// </summary>
        /// <param name="updateEpochLogRecord">The truncate tail log record to be processed.</param>
        /// <param name="isRecoverableRecord">Is this a recoverable record.</param>
        private void ProcessLogRecord(UpdateEpochLogRecord updateEpochLogRecord, out bool isRecoverableRecord)
        {
            isRecoverableRecord = true;

            Utility.Assert(
                this.LastLogicalSequenceNumber == updateEpochLogRecord.Lsn,
                "{0} this.LastLogicalSequenceNumber ({1}) == updateEpochLogRecord.Lsn ({2})",
                this.tracer.Type,
                this.LastLogicalSequenceNumber, 
                updateEpochLogRecord.Lsn);

            var updateEpochRecordEpoch = updateEpochLogRecord.Epoch;

            if (this.CurrentLogTailEpoch < updateEpochRecordEpoch)
            {
                this.CurrentLogTailEpoch = updateEpochRecordEpoch;
            }

            if (Mode.Recovery == this.mode)
            {
                ProgressVectorEntry progressVectorEntry;

                if (updateEpochLogRecord.RecordPosition
                    > this.recoveredLastCompletedBeginCheckpointRecord.RecordPosition)
                {
                    progressVectorEntry = new ProgressVectorEntry(updateEpochLogRecord);
                    if (this.ProgressVector.LastProgressVectorEntry.Epoch < updateEpochRecordEpoch)
                    {
                        this.ProgressVector.Add(progressVectorEntry);
                    }
                    else
                    {
                        var isInserted = this.ProgressVector.Insert(progressVectorEntry);
                        Utility.Assert(
                            isInserted == true, 
                            "{0} isInserted ({1}) == true. Incoming: {2} CurrentTail: {3}", 
                            this.tracer.Type, 
                            isInserted, 
                            progressVectorEntry.ToString(),
                            this.ProgressVector.LastProgressVectorEntry.ToString());
                    }
                }
                else
                {
                    progressVectorEntry = this.ProgressVector.Find(updateEpochRecordEpoch);
                    Utility.Assert(
                        (progressVectorEntry != null) && (progressVectorEntry.Lsn == this.LastLogicalSequenceNumber),
                        "{0} (ProgressVectorEntry != null) && (ProgressVectorEntry.LastLogicalSequenceNumber {1} == LSN {2})",
                        this.tracer.Type,
                        this.LastLogicalSequenceNumber,
                        progressVectorEntry.Lsn);
                }
            }
        }
    }
}
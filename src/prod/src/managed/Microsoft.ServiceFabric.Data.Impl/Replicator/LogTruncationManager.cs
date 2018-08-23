// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;
    using System.Globalization;
    using System.Runtime.CompilerServices;
    using System.Threading.Tasks;

    internal sealed class LogTruncationManager
    {
        internal const int ThrottleAfterPendingCheckpointCount = 4;
        /// <summary>
        /// If an old transaction blocks checkpoint, it gets aborted.
        /// The transaction record offset must be older by this factor of the current log usage
        /// </summary>
        private const uint AbortOldTxFactor = 2;

        private const int MBtoBytesMultiplier = 1024 * 1024;
        /// <summary>
        /// Factor used for deciding whether it is worth truncating.
        /// </summary>
        private const uint MinTruncationFactor = 2;

        private const uint NumberOfIndexRecordsPerCheckpoint = 50;

        /// <summary>
        /// We try to checkpoint after this size of non-checkpointed data
        /// </summary>
        private ulong checkpointIntervalBytes;

        /// <summary>
        /// We try to insert an indexing log record after this size of log since the last index log record.
        /// </summary>
        private ulong indexIntervalBytes;

        /// <summary>
        /// Minimum size of the log we would like to keep.
        /// </summary>
        private ulong minLogSizeInBytes;

        /// <summary>
        /// Minimum size of the log we would like to keep.
        /// </summary>
        private ulong minTruncationAmountInBytes;

        private ReplicatedLogManager logManager;

        /// <summary>
        /// Size of the log at which throttling will be inititated.
        /// </summary>
        private ulong throttleAtLogUsageBytes;

        private ITracer tracer;

        /// <summary>
        /// We try to truncate the log when it reaches this usage
        /// </summary>
        private ulong truncationThresholdInBytes;

        /// <summary>
        /// Threshold above which in-flight transactions will start getting aborted.
        /// </summary>
        private ulong txnAbortThresholdInBytes;

        private PeriodicCheckpointTruncationState periodicCheckpointTruncationState = PeriodicCheckpointTruncationState.NotStarted;

        public LogTruncationManager()
        {
            this.tracer = null;
        }

        /// <summary>
        /// We try to checkpoint after this size of non-checkpointed data
        /// </summary>
        internal ulong CheckpointIntervalBytes
        {
            get
            {
                return this.checkpointIntervalBytes;
            }
        }

        /// <summary>
        /// Minimum size of the log we would like to keep.
        /// </summary>
        internal ulong MinLogSizeInBytes
        {
            get
            {
                return this.minLogSizeInBytes;
            }
        }

        /// <summary>
        /// Minimum size of the log we would like to keep.
        /// </summary>
        internal ulong MinTruncationAmountInBytes
        {
            get
            {
                return this.minTruncationAmountInBytes;
            }
        }

        /// <summary>
        /// Size of the log at which throttling will be inititated.
        /// </summary>
        internal ulong ThrottleAtLogUsageBytes
        {
            get
            {
                return this.throttleAtLogUsageBytes;
            }
        }

        /// <summary>
        /// We try to truncate the log when it reaches this usage
        /// </summary>
        internal ulong TruncationThresholdInBytes
        {
            get
            {
                return this.truncationThresholdInBytes;
            }
        }

        internal PeriodicCheckpointTruncationState PeriodicCheckpointTruncationState
        {
            get
            {
                return periodicCheckpointTruncationState;
            }
        }

        public async Task BlockSecondaryPumpIfNeeded(LogicalSequenceNumber lastStableLsn)
        {
            // If no checkpoints have happened on the secondary, it implies the copy is in progress and we cannot block as
            // we need to pump more operations to be able to issue the first checkpoint
            if (this.logManager.LastCompletedBeginCheckpointRecord == null)
            {
                return;
            }

            var bytesUsedFromCurrentLogHead = this.GetBytesUsed(this.logManager.CurrentLogHeadRecord);
            LogRecord pendingOperationRecord = this.logManager.LastInProgressCheckpointRecord;

            if (pendingOperationRecord == null)
            {
                pendingOperationRecord = this.logManager.LastInProgressTruncateHeadRecord;
            }

            if (pendingOperationRecord == null)
            {
                return;
            }

            // If there is pending checkpoint/truncation and the current barrier stable LSN is greater than the checkpoint/truncatehead LSN,
            // it implies that the checkpoint/truncation is in a ready state and it can be applied
            // The fact that it is still pending implies the actual perform checkpoint operation/truncation operation is taking a long time
            // and we hence block here
            // If the stable lsn is smaller, we need to accept more so that more group commits lead to more progress in stable lsn
            // V1 Repl will take care of throttling if we are not reaching stable (Queue full errors)
            if (pendingOperationRecord.Lsn > lastStableLsn)
            {
                return;
            }

            if (bytesUsedFromCurrentLogHead > this.throttleAtLogUsageBytes)
            {
                var logUsage = this.logManager.LogManager.Length;

                FabricEvents.Events.DrainReplicationBlocked(
                    this.tracer.Type,
                    pendingOperationRecord.RecordType.ToString(),
                    pendingOperationRecord.Lsn.LSN,
                    bytesUsedFromCurrentLogHead,
                    logUsage);

                await pendingOperationRecord.AwaitProcessing().ConfigureAwait(false);
                logUsage = this.logManager.LogManager.Length;

                FabricEvents.Events.DrainReplicationContinue(this.tracer.Type, bytesUsedFromCurrentLogHead, logUsage);
            }
        }

        public List<BeginTransactionOperationLogRecord> GetOldTransactions(TransactionMap txManager)
        {
            var tail = this.GetCurrentTailPosition();

            // The tail is smaller than truncate interval
            if (tail <= this.txnAbortThresholdInBytes)
            {
                return null;
            }

            var oldTxOffset = this.GetCurrentTailPosition() - this.txnAbortThresholdInBytes - 1;

            // Get all 'bad' transactions that are preventing us from checkpointing enough state
            return txManager.GetPendingTransactionsOlderThanPosition(oldTxOffset);
        }

        public bool IsGoodLogHeadCandidiate(IndexingLogRecord proposedLogHead)
        {
            // Configured truncation has been initiated, override minTruncationAmount and minLogSize requirements
            if (periodicCheckpointTruncationState == PeriodicCheckpointTruncationState.TruncationStarted)
            {
                return true;
            }

            // This is a very recent indexing record and not a good candidate
            // We can say that because it has not yet been flushed to the disk
            if (proposedLogHead.RecordPosition == LogRecord.InvalidRecordPosition)
            {
                return false;
            }

            // Is it worth truncating?
            // We do not want to truncate a couple of bytes since we would have to repeat the process soon.
            if (proposedLogHead.RecordPosition - this.logManager.CurrentLogHeadRecord.RecordPosition < this.minTruncationAmountInBytes)
            {
                return false;
            }

            // Would it truncate too much?
            // We do not want to truncate if it would cause the log to shrink below minTruncationAmountInBytes.
            // This is because small logs can cause unnecessary full copies and full backups.
            ulong resultingLogSize = this.GetCurrentTailPosition() - proposedLogHead.RecordPosition;
            if (resultingLogSize < this.minLogSizeInBytes)
            {
                return false;
            }

            return true;
        }

        public void Open(ReplicatedLogManager logManager, ITracer tracer, ulong checkpointSizeMB, ulong minLogSizeInMB, uint truncationThresholdFactor, uint throttlingThresholdFactor)
        {
            this.tracer = tracer;
            this.logManager = logManager;

            this.checkpointIntervalBytes = checkpointSizeMB * MBtoBytesMultiplier;
            this.minLogSizeInBytes = minLogSizeInMB * MBtoBytesMultiplier;
            this.truncationThresholdInBytes = truncationThresholdFactor * this.minLogSizeInBytes;
            this.throttleAtLogUsageBytes = this.GetThrottleThresholdInBytes(throttlingThresholdFactor, this.checkpointIntervalBytes, this.minLogSizeInBytes);
            this.minTruncationAmountInBytes = this.GetMinTruncationAmountInBytes(this.checkpointIntervalBytes);
            this.indexIntervalBytes = this.checkpointIntervalBytes / NumberOfIndexRecordsPerCheckpoint;
            this.txnAbortThresholdInBytes = this.checkpointIntervalBytes / AbortOldTxFactor;

            Utility.Assert(this.minTruncationAmountInBytes > 0, "Min truncation amount in bytes cannot be less than 1");
            Utility.Assert(this.indexIntervalBytes != 0, "Index interval in bytes cannot be 0");
            Utility.Assert(this.checkpointIntervalBytes != 0, "Checkpoint Interval cannot be 0");
            Utility.Assert(this.minLogSizeInBytes != 0, "Min Log Size in bytes cannot be 0");

            Utility.Assert(this.truncationThresholdInBytes >= this.minLogSizeInBytes, "truncationThresholdInBytes {0} must be larger than minLogSizeInBytes {1}", this.truncationThresholdInBytes, this.minLogSizeInBytes);
            Utility.Assert(this.throttleAtLogUsageBytes > this.truncationThresholdInBytes, "throttleAtLogUsageBytes {0} must be larger than truncationThresholdInBytes {1}", this.throttleAtLogUsageBytes, this.truncationThresholdInBytes);
            Utility.Assert(this.throttleAtLogUsageBytes > this.checkpointIntervalBytes, "throttleAtLogUsageBytes {0} must be larger than checkpointIntervalBytes {1}", this.throttleAtLogUsageBytes, this.checkpointIntervalBytes);

            var trace = string.Format(
                CultureInfo.InvariantCulture,
                "IndexIntervalBytes = {0}, CheckpointIntervalBytes = {1}, MinLogSizeInMB = {2}, TruncateIntervalBytes = {3}, ThrottleAtLogUsageBytes = {4}, MinTruncationThresholdInBytes = {5}",
                this.indexIntervalBytes,
                this.checkpointIntervalBytes,
                this.minLogSizeInBytes,
                this.truncationThresholdInBytes,
                this.throttleAtLogUsageBytes,
                this.minTruncationAmountInBytes);

            LoggingReplicator.ReplicatorTraceOnApi(tracer, trace);
        }
        public bool ShouldBlockOperationsOnPrimary()
        {
            // We block incoming operations on the primary if the log usage > throttle limit
            return this.GetBytesUsed(this.logManager.CurrentLogHeadRecord) > this.throttleAtLogUsageBytes;
        }

        public bool ShouldCheckpointOnPrimary(
            TransactionMap txMap,
            out List<BeginTransactionOperationLogRecord> abortTxList)
        {
            return this.ShouldCheckpoint(txMap, out abortTxList);
        }

        public bool ShouldCheckpointOnSecondary(TransactionMap txMap)
        {
            // The first checkpoint is always initiated by copy
            if (this.logManager.LastCompletedBeginCheckpointRecord == null)
            {
                return false;
            }

            List<BeginTransactionOperationLogRecord> tempList;
            return this.ShouldCheckpoint(txMap, out tempList);
        }

        public bool ShouldIndex()
        {
            var lastIndexRecord = this.logManager.LastIndexRecord;

            // If there was never an indexing record, return true
            if (lastIndexRecord == null)
            {
                return true;
            }

            // If there is a pending indexing record that is not yet flushed, do not index again
            if (lastIndexRecord.RecordPosition == LogRecord.InvalidRecordPosition)
            {
                return false;
            }

            // If the last indexing record position is older than the configured limit, return true
            if (this.GetBytesUsed(this.logManager.LastIndexRecord) >= this.indexIntervalBytes)
            {
                return true;
            }

            return false;
        }

        public bool ShouldTruncateHead()
        {
            if (this.logManager.LastInProgressTruncateHeadRecord != null)
            {
                return false;
            }

            if (this.logManager.LastCompletedEndCheckpointRecord == null)
            {
                return false;
            }

            if (ShouldInitiatePeriodicTruncation())
            {
                return true;
            }

            var bytesUsedFromCurrentLogHead = this.GetBytesUsed(this.logManager.CurrentLogHeadRecord);
            if (bytesUsedFromCurrentLogHead < this.truncationThresholdInBytes)
            {
                return false;
            }

            return true;
        }

        public void InitiatePeriodicCheckpoint()
        {
            if (periodicCheckpointTruncationState == PeriodicCheckpointTruncationState.NotStarted)
            {
                periodicCheckpointTruncationState = PeriodicCheckpointTruncationState.Ready;
            }
        }

        public void OnCheckpointCompleted(Exception err, CheckpointState state, bool isRecoveredCheckpoint)
        {
            if (!isRecoveredCheckpoint && periodicCheckpointTruncationState != PeriodicCheckpointTruncationState.CheckpointStarted)
            {
                // Checkpoint not initiated by config
                // Indicates regular checkpoint
                return;
            }

            if (err != null || state != CheckpointState.Completed)
            {
                // Checkpoint failed to complete successfully, reset periodic process to 'Ready'
                return;
            }

            periodicCheckpointTruncationState = PeriodicCheckpointTruncationState.CheckpointCompleted;
        }

        public void OnTruncationCompleted()
        {
            // Reset the periodic process
            periodicCheckpointTruncationState = PeriodicCheckpointTruncationState.NotStarted;
        }

        public void Recover(
            long recoveredCheckpointTime,
            long recoveredTruncationTime)
        {
            bool truncationIncomplete = recoveredTruncationTime < recoveredCheckpointTime;
            if (truncationIncomplete)
            {
                this.OnCheckpointCompleted(
                    null,
                    CheckpointState.Completed,
                    true);
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private ulong GetBytesUsed(LogRecord startingRecord)
        {
            Utility.Assert(startingRecord != null, "GetBytesUsedFromRecord: Record Cannot be null");
            Utility.Assert(
                startingRecord.RecordPosition != LogRecord.InvalidRecordPosition,
                "GetBytesUsedFromRecord: Record Cannot have invalid position");

            var bufferedBytes = this.logManager.LogManager.BufferedBytes;
            var pendingFlushBytes = this.logManager.LogManager.PendingFlushBytes;
            var tail = this.logManager.LogManager.CurrentLogTailPosition;
            ulong tailRecordSize = this.logManager.LogManager.CurrentLogTailRecord.ApproximateSizeOnDisk;

            Utility.Assert(
                bufferedBytes >= 0 && pendingFlushBytes >= 0,
                "BufferedBytes = {0} and PendingFlushBytes = {1} cannot be negative",
                bufferedBytes, pendingFlushBytes);

            ulong bytesUsed = 0;

            // Every log record is assigned its record position before flushing and before updating the tail and other statistics
            // As a result, this value is an approximate and can sometimes end up being negative.

            // To prevent from overflow errors, we only return a positive value of the bytes used or else just return 0 to indicate that the
            // tail is not very far away from the 'startingRecord'
            if (tail + tailRecordSize + (ulong)bufferedBytes + (ulong)pendingFlushBytes > startingRecord.RecordPosition)
            {
                bytesUsed = tail + tailRecordSize + (ulong)bufferedBytes + (ulong)pendingFlushBytes
                            - startingRecord.RecordPosition;
            }

            return bytesUsed;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private ulong GetCurrentTailPosition()
        {
            var bufferedBytes = this.logManager.LogManager.BufferedBytes;
            var pendingFlushBytes = this.logManager.LogManager.PendingFlushBytes;
            var logTail = this.logManager.LogManager.CurrentLogTailRecord;

            Utility.Assert(
                bufferedBytes >= 0 && pendingFlushBytes >= 0,
                "BufferedBytes = {0} and PendingFlushBytes = {1} cannot be negative",
                bufferedBytes, pendingFlushBytes);

            return logTail.RecordPosition + logTail.RecordSize + (ulong)bufferedBytes + (ulong)pendingFlushBytes;
        }

        /// <summary>
        /// Gets the min truncation amount threshold in bytes.
        /// </summary>
        /// <param name="checkpointThresholdInBytes"></param>
        /// <returns></returns>
        private ulong GetMinTruncationAmountInBytes(ulong checkpointThresholdInBytes)
        {
            return checkpointThresholdInBytes / MinTruncationFactor;
        }

        /// <summary>
        /// Gets the throttle threshold in bytes.
        /// 0 MinLogSizeInMB indicates that CheckpointThresholdInMB / DefaultMinLogDivider needs to be used.
        /// </summary>
        /// <param name="throttleThresholdFactor">Throttle threshold factor.</param>
        /// <param name="checkpointThresholdInBytes">Checkpoint Threshold In MB</param>
        /// <param name="minLogSizeInBytes">Minimum log size.</param>
        /// <returns></returns>
        private ulong GetThrottleThresholdInBytes(uint throttleThresholdFactor, ulong checkpointThresholdInBytes, ulong minLogSizeInBytes)
        {
            ulong throttleThresholdInMBFromCheckpointInMB = checkpointThresholdInBytes * throttleThresholdFactor;
            ulong throttleThresholdInMBFromMinLogSizeInMB = minLogSizeInBytes * throttleThresholdFactor;

            return Math.Max(throttleThresholdInMBFromCheckpointInMB, throttleThresholdInMBFromMinLogSizeInMB);
        }

        private bool ShouldCheckpoint(TransactionMap txManager, out List<BeginTransactionOperationLogRecord> abortTxList)
        {
            abortTxList = null;

            // There is a checkpoint in progres. So return false
            if (this.logManager.LastInProgressCheckpointRecord != null)
            {
                return false;
            }

            if (ShouldInitiatePeriodicCheckpoint())
            {
                return true;
            }

            var bytesUsedFromLastCheckpoint = this.GetBytesUsed(this.logManager.LastCompletedBeginCheckpointRecord);

            // If there is enough data to checkpoint, we should try to look for 'bad' transactions that are preventing
            // enough data from being checkpointed
            if (bytesUsedFromLastCheckpoint <= this.checkpointIntervalBytes)
            {
                return false;
            }

            var earliestPendingTx = txManager.GetEarliestPendingTransaction();

            // If there is no pending tx, we should checkpoint now
            if (earliestPendingTx == null)
            {
                return true;
            }

            // The earliest pending tx is very new and it has not yet been flushed. So it is fine to checkpoint
            // This could ALSO imply that we have a really old Tx that has not been flushed and has been lying around in the buffers
            // However, the latter case is taken care of by throttling the incoming operations based on "pending bytes" that are unflushed
            if (earliestPendingTx.RecordPosition == LogRecord.InvalidRecordPosition)
            {
                return true;
            }

            var oldTxOffset = this.GetCurrentTailPosition() - this.txnAbortThresholdInBytes - 1;

            // The transaction is new enough. We can checkpoint
            if (earliestPendingTx.RecordPosition > oldTxOffset)
            {
                return true;
            }

            // Get all 'bad' transactions that are preventing us from checkpointing enough state
            // We will return 'false' now and the next group commit should successfully checkpoint since the earliest pending should be newer
            abortTxList = txManager.GetPendingTransactionsOlderThanPosition(oldTxOffset);
            return false;
        }

        private bool ShouldInitiatePeriodicCheckpoint()
        {
            bool periodicCheckpointRequested = periodicCheckpointTruncationState == PeriodicCheckpointTruncationState.Ready;

            if (periodicCheckpointRequested)
            {
                periodicCheckpointTruncationState = PeriodicCheckpointTruncationState.CheckpointStarted;
            }

            return periodicCheckpointRequested;
        }

        private bool ShouldInitiatePeriodicTruncation()
        {
            bool periodicCheckpointCompleted = periodicCheckpointTruncationState == PeriodicCheckpointTruncationState.CheckpointCompleted;

            if (periodicCheckpointCompleted)
            {
                periodicCheckpointTruncationState = PeriodicCheckpointTruncationState.TruncationStarted;
            }

            return periodicCheckpointCompleted;
        }
    }
}
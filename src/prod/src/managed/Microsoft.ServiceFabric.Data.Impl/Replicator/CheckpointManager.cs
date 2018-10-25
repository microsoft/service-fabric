// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using Diagnostics;
    using Microsoft.ServiceFabric.Data;
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    internal sealed class CheckpointManager : ICheckpointManager
    {
        /// <summary>
        /// Ensures that the last End Checkpoint and Complete Checkpoint records in the full backup log
        /// correspond to State Provider checkpoints that were backed up.
        /// </summary>
        private readonly SemaphoreSlim backupAndCopyConsistencyLock = new SemaphoreSlim(1);
        private readonly Backup.IBackupManager backupManager;
        private readonly object headTruncationLock = new object();
        private readonly object thisLock = new object();
        private readonly Action<LogicalLogRecord> logicalRecordProcessedCallback;
        private readonly LogTruncationManager logTruncationManager;
        private readonly Action<PhysicalLogRecord> physicalRecordProcessedCallback;
        private readonly ReplicatedLogManager replicatedLogManager;
        private readonly IStateManager stateManager;
        private readonly RecoveredOrCopiedCheckpointLsn recoveredOrCopiedCheckpointLsn;

        /// <summary>
        /// Protects Backup, PrepareCheckpoint and PerformCheckpoint.
        /// </summary>
        private readonly SemaphoreSlim stateManagerApiLock = new SemaphoreSlim(1);


        private readonly TransactionMap transactionMap;
        private readonly CheckpointCountCounterWriter checkpointPerfCounter;
        private readonly ThrottleRateCounterWriter throttleRatePerfCounter;
        private int groupCommitDelay;
        private bool groupCommitNeeded;
        private Task groupCommitTask;
        private LogicalSequenceNumber lastStableLsn;
        private long lastTxIdAbortedDueToCheckpointPrevention = 0;
        private readonly uint progressVectorMaxEntries;
        private ITracer tracer;

        private DateTime lastPeriodicCheckpointTime;
        private DateTime lastPeriodicTruncationTime;
        private System.Threading.Timer checkpointTimer;

        private TimeSpan truncationInterval;

        public CheckpointManager(
            CheckpointCountCounterWriter checkpointPerfCounter,
            ThrottleRateCounterWriter throttleRatePerfCounter,
            RecoveredOrCopiedCheckpointLsn recoveredOrCopiedCheckpointLsn,
            Backup.IBackupManager backupManager,
            ReplicatedLogManager replicatedLogManager,
            TransactionMap transactionsMap,
            IStateManager stateManager,
            Action<PhysicalLogRecord> physicalRecordProcessedCallback,
            Action<LogicalLogRecord> logicalRecordProcessedCallback,
            uint progressVectorMaxEntries,
            int logTruncationIntervalSeconds)
        {
            this.checkpointPerfCounter = checkpointPerfCounter;
            this.throttleRatePerfCounter = throttleRatePerfCounter;
            this.logTruncationManager = new LogTruncationManager();
            this.recoveredOrCopiedCheckpointLsn = recoveredOrCopiedCheckpointLsn;
            this.backupManager = backupManager;
            this.physicalRecordProcessedCallback = physicalRecordProcessedCallback;
            this.logicalRecordProcessedCallback = logicalRecordProcessedCallback;
            this.stateManager = stateManager;
            this.replicatedLogManager = replicatedLogManager;
            this.transactionMap = transactionsMap;
            this.progressVectorMaxEntries = progressVectorMaxEntries;
            this.checkpointTimer = new Timer(this.TimerCallback, null, Timeout.Infinite, Timeout.Infinite);
            this.lastPeriodicCheckpointTime = DateTime.Now;
            this.lastPeriodicTruncationTime = DateTime.Now;
            this.truncationInterval = TimeSpan.FromSeconds(logTruncationIntervalSeconds);
        }

        public static TimeSpan CalculateTruncationTimerDuration(
            DateTime lastPeriodicCheckpointTime,
            TimeSpan truncationinterval,
            PeriodicCheckpointTruncationState state)
        {
            DateTime currentTime = DateTime.Now;
            TimeSpan elapsedFromLastCheckpoint = currentTime - lastPeriodicCheckpointTime;
            TimeSpan remainingTime = truncationinterval - elapsedFromLastCheckpoint;

            if (remainingTime.Ticks <= 0)
            {
                // Return 0 if periodic checkpoint/truncation not started and interval has passed, immediately initiating process
                if (state == PeriodicCheckpointTruncationState.NotStarted)
                {
                    return TimeSpan.Zero;
                }

                // If the process has started and interval has passed, fire timer after 1 full interval
                return truncationinterval;
            }

            return remainingTime;
        }

        public LogicalSequenceNumber LastStableLsn
        {
            get
            {
                return this.lastStableLsn;
            }
        }

        public DateTime LastPeriodicCheckpointTime
        {
            get
            {
                return this.lastPeriodicCheckpointTime;
            }
        }

        public DateTime LastPeriodicTruncationTime
        {
            get
            {
                return this.lastPeriodicTruncationTime;
            }
        }

        public PeriodicCheckpointTruncationState PeriodicCheckpointTruncationState
        {
            get
            {
                return this.logTruncationManager.PeriodicCheckpointTruncationState;
            }
        }

        public void AbortPendingCheckpoint()
        {
            var checkpointState = CheckpointState.Invalid;
            BeginCheckpointLogRecord checkpointRecord = null;

            // Can race with ApplyCheckpointIfPermitted. after checkpoint state is set to either aborted or applied, we are guaranteed single threaded access. so no synchronisation is needed
            lock (thisLock)
            {
                checkpointRecord = this.replicatedLogManager.LastInProgressCheckpointRecord;

                if (checkpointRecord != null)
                {
                    checkpointState = checkpointRecord.CheckpointState;
                    if (checkpointState <= CheckpointState.Ready)
                    {
                        checkpointRecord.CheckpointState = CheckpointState.Aborted;
                    }
                }
            }

            if (checkpointState == CheckpointState.Ready)
            {
                this.CheckpointApply(null);
            }
        }

        public void AbortPendingLogHeadTruncation()
        {
            var truncationState = TruncationState.Invalid;
            TruncateHeadLogRecord truncateHeadRecord = null;

            // can race with ApplyLogHeadTruncationIfPermitted. after truncation state is set to either aborted or applied, we are guaranteed only one thread access so no lock is needed
            lock (thisLock)
            {
                truncateHeadRecord = this.replicatedLogManager.LastInProgressTruncateHeadRecord;

                if (truncateHeadRecord != null)
                {
                    truncationState = truncateHeadRecord.TruncationState;
                    if (truncationState <= TruncationState.Ready)
                    {
                        truncateHeadRecord.TruncationState = TruncationState.Aborted;
                    }
                }
            }

            if (truncationState == TruncationState.Ready)
            {
                LogHeadTruncationApply(this.tracer, truncateHeadRecord, null);
            }
        }

        /// <summary>
        /// Acquire Backup Consistency Lock.
        /// </summary>
        /// <param name="lockTaker">Name of the lock taker.</param>
        /// <param name="timeout">The timeout.</param>
        /// <param name="cancellationToken">The cancellation token.</param>
        /// <returns>Asynchronous task that represents the operation.</returns>
        /// <remarks>
        /// MCoskun
        /// Backup consistency locks around logging EndCheckpoint, calling CompleteCheckpoint and logging CompleteCheckpoint.
        /// Locking around endcheckpoint allows efficient way of making sure the low water mark does not get truncated without blocking all truncates.
        /// Locking around completeCheckpoint ensures that all checkpoint files being backed up belong to the same checkpoint.
        /// </remarks>
        public async Task<bool> AcquireBackupAndCopyConsistencyLockAsync(
            string lockTaker,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            var stopWatch = new Stopwatch();
            stopWatch.Start();

            var isAcquired = await this.backupAndCopyConsistencyLock.WaitAsync(timeout, cancellationToken).ConfigureAwait(false);

            stopWatch.Stop();

            var latency = stopWatch.ElapsedMilliseconds;

            if (isAcquired)
            {
                FabricEvents.Events.Lock(this.tracer.Type, "BackupAndCopyConsistency", lockTaker, "Acquired in " + latency);
            }
            else
            {
                FabricEvents.Events.Lock(this.tracer.Type, "BackupAndCopyConsistency", lockTaker, "Failed to acquire in " + latency);
            }

            return isAcquired;
        }

        /// <summary>
        /// Acquire Backup Consistency Lock.
        /// </summary>
        /// <param name="lockTaker">Name of the lock taker.</param>
        /// <returns>Asynchronous task that represents the operation.</returns>
        /// <remarks>
        /// MCoskun
        /// Backup consistency locks around logging EndCheckpoint, calling CompleteCheckpoint and logging CompleteCheckpoint.
        /// Locking around endcheckpoint allows efficient way of making sure the low water mark does not get truncated without blocking all truncates.
        /// Locking around completeCheckpoint ensures that all checkpoint files being backed up belong to the same checkpoint.
        /// </remarks>
        public async Task AcquireBackupAndCopyConsistencyLockAsync(string lockTaker)
        {
            var stopWatch = new Stopwatch();
            stopWatch.Start();

            await this.backupAndCopyConsistencyLock.WaitAsync().ConfigureAwait(false);

            stopWatch.Stop();

            var latency = stopWatch.ElapsedMilliseconds;

            FabricEvents.Events.Lock(this.tracer.Type, "BackupAndCopyConsistency", lockTaker, "Acquired in " + latency);
        }

        /// <summary>
        /// Acquire state manager API  Lock.
        /// </summary>
        /// <param name="lockTaker">Name of the lock taker.</param>
        /// <param name="timeout">The timeout.</param>
        /// <param name="cancellationToken">The cancellation token.</param>
        /// <returns>Asynchronous task that represents the operation.</returns>
        public async Task<bool> AcquireStateManagerApiLockAsync(
            string lockTaker,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            var stopWatch = new Stopwatch();
            stopWatch.Start();

            var isAcquired = await this.stateManagerApiLock.WaitAsync(timeout, cancellationToken).ConfigureAwait(false);

            stopWatch.Stop();

            var latency = stopWatch.ElapsedMilliseconds;

            if (isAcquired)
            {
                FabricEvents.Events.Lock(this.tracer.Type, "StateManagerApi", lockTaker, "Acquired in " + latency);
            }
            else
            {
                FabricEvents.Events.Lock(
                    this.tracer.Type,
                    "StateManagerApi",
                    lockTaker,
                    "Failed to acquire in " + latency);
            }

            return isAcquired;
        }

        /// <summary>
        /// Cancels the checkpoint by not doing a perform checkpoint log and completes the processing
        /// It needs to wait for phase1 to finish however
        /// </summary>
        /// <param name="checkpointRecord"></param>
        /// <param name="copiedUptoLsn"></param>
        public async Task CancelFirstCheckpointOnIdleDueToIncompleteCopy(
            BeginCheckpointLogRecord checkpointRecord,
            long copiedUptoLsn)
        {
            Utility.Assert(checkpointRecord.IsFirstCheckpointOnFullCopy == true, "CancelFirstCheckpointOnIdleDueToIncompleteCopy must only be called on FirstCheckpoint");

            // the only time this may not happen is when replica is closing prematurely, at which point log is older than copieduptolsn
            Utility.Assert(
                replicatedLogManager.CurrentLogTailLsn.LSN < copiedUptoLsn,
                "CancelFirstCheckpointOnIdleDueToIncompleteCopy: Log tail is {0} and copiedUptoLsn is {1}. Former must be lesser than latter",
                replicatedLogManager.CurrentLogTailLsn.LSN, copiedUptoLsn);

            // Since we couldn't copy everything, ensure we fault (if not already faulted) and trace before that
            FabricEvents.Events.Checkpoint(
                this.tracer.Type,
                "CancelFirstCheckpointOnIdleDueToIncompleteCopy",
                (int)checkpointRecord.CheckpointState,
                checkpointRecord.Lsn.LSN,
                checkpointRecord.Psn.PSN,
                checkpointRecord.RecordPosition,
                checkpointRecord.EarliestPendingTransactionLsn.LSN);

            try
            {
                // this awaiting is needed to ensure that if a performcheckpointasync call is currently in progress, we dont complete the drain copy process until it completes
                await checkpointRecord.AwaitCompletionPhase1FirstCheckpointOnFullCopy(this.tracer.Type);
            }
            finally
            {
                this.ProcessException(
                    "CheckpointManager",
                    checkpointRecord,
                    "CancelFirstCheckpointOnIdleDueToIncompleteCopy",
                    new Exception("First Checkpoint cancelled due to copy log not pumping upto end of stream"));

                this.physicalRecordProcessedCallback(checkpointRecord);

                this.OnCompletePendingCheckpoint(null, checkpointRecord.CheckpointState);
            }
        }

        /// <summary>
        /// Completes the checkpoint and renames the log
        /// </summary>
        /// <param name="checkpointRecord"></param>
        /// <param name="copiedUptoLsn"></param>
        /// <returns></returns>
        public async Task CompleteFirstCheckpointOnIdleAndRenameLog(
            BeginCheckpointLogRecord checkpointRecord,
            long copiedUptoLsn)
        {
            Utility.Assert(checkpointRecord.IsFirstCheckpointOnFullCopy == true, "CompleteCheckpointAndRename must only be called on FirstCheckpoint");

            // we must rename the log at EXACTLY the copiedUptoLsn
            // the only time this may not happen is when replica is closing prematurely, at which point log is older than copieduptolsn
            Utility.Assert(
                replicatedLogManager.CurrentLogTailLsn.LSN == copiedUptoLsn,
                "CompletecheckpointAndRename: Log tail is {0} and copiedUptoLsn is {1}. Both must be equal",
                replicatedLogManager.CurrentLogTailLsn.LSN, copiedUptoLsn);

            try
            {
                // If successfully copied all records that were to be copied, rename the log here
                FabricEvents.Events.Checkpoint(
                    this.tracer.Type,
                    "CompleteFirstCheckpointOnIdleAndRenameLog",
                    (int)checkpointRecord.CheckpointState,
                    checkpointRecord.Lsn.LSN,
                    checkpointRecord.Psn.PSN,
                    checkpointRecord.RecordPosition,
                    checkpointRecord.EarliestPendingTransactionLsn.LSN);

                // Flush to ensure that if no more records are drained, the checkpoint record gets written to disk
                await replicatedLogManager.LogManager.FlushAsync("CompleteFirstCheckpointOnIdleAndRenameLog");

                await checkpointRecord.AwaitCompletionPhase1FirstCheckpointOnFullCopy(this.tracer.Type);

                await CompleteCheckpointAndRenameIfNeeded(checkpointRecord, true);

                FabricEvents.Events.CopyOrBuildReplica(
                    this.tracer.Type,
                    "Copied replica is now consistent");
            }
            catch (Exception e)
            {
                this.ProcessException("CheckpointManager", checkpointRecord, "CompleteFirstCheckpointOnIdleAndRenameLog", e);
            }

            this.physicalRecordProcessedCallback(checkpointRecord);

            this.OnCompletePendingCheckpoint(null, checkpointRecord.CheckpointState);
        }

        public void AppendBarrierOnSecondary(BarrierLogRecord barrierRecord)
        {
            this.replicatedLogManager.AppendBarrier(barrierRecord, false);
            this.ProcessBarrierRecordOnSecondaryAsync(barrierRecord).IgnoreExceptionVoid();
        }

        /// <summary>
        /// Applies the in flight checkpoint if it is permitted.
        /// </summary>
        /// <param name="flushException"></param>
        /// <param name="record"></param>
        /// <returns></returns>
        /// <remarks>
        /// Calls prepare, waits for the lsn to become stable and marks the log record as applied if
        /// record was successfully flushed and prepared.
        /// </remarks>
        public async Task ApplyCheckpointIfPermitted(Exception flushException, LogRecord record)
        {
            Utility.Assert(
                this.replicatedLogManager.LastInProgressCheckpointRecord == record,
                "this.lastInProgressCheckpointRecord ({0}) == record ({1})",
                this.replicatedLogManager.LastInProgressCheckpointRecord, record);

            BeginCheckpointLogRecord checkpointRecord = null;

            checkpointRecord = this.replicatedLogManager.LastInProgressCheckpointRecord;
            Utility.Assert(checkpointRecord != null, "Last in progress checkpoint should not be null in ApplyCheckpointIfPermitted");

            var exception = flushException;

            if (exception == null)
            {
                await this.AcquireStateManagerApiLockAsync("ApplyCheckpointIfPermitted").ConfigureAwait(false);
                try
                {
                    await this.stateManager.PrepareCheckpointAsync(checkpointRecord.Lsn.LSN).ConfigureAwait(false);
                }
                catch (Exception e)
                {
                    exception = e;
                }
                finally
                {
                    this.ReleaseStateManagerApiLock("ApplyCheckpointIfPermitted");
                }
            }

            var checkpointState = (exception == null) ? CheckpointState.Ready : CheckpointState.Faulted;

            lock (thisLock)
            {
                if (checkpointRecord.Lsn <= this.LastStableLsn)
                {
                    checkpointRecord.LastStableLsn = this.LastStableLsn;
                    if (exception == null)
                    {
                        checkpointState = CheckpointState.Applied;
                    }
                }

                if (checkpointRecord.CheckpointState == CheckpointState.Invalid)
                {
                    checkpointRecord.CheckpointState = checkpointState;
                }
                else
                {
                    checkpointState = checkpointRecord.CheckpointState;
                    Utility.Assert(
                        checkpointState == CheckpointState.Aborted,
                        "checkpointState should be aborted. state :{0}",
                        checkpointState);
                }
            }

            FabricEvents.Events.Checkpoint(
                this.tracer.Type,
                "Prepare",
                (int)checkpointRecord.CheckpointState,
                checkpointRecord.Lsn.LSN,
                checkpointRecord.Psn.PSN,
                checkpointRecord.RecordPosition,
                checkpointRecord.EarliestPendingTransactionLsn.LSN);

            if ((checkpointState == CheckpointState.Applied) || (checkpointState == CheckpointState.Faulted)
                || (checkpointState == CheckpointState.Aborted))
            {
                this.CheckpointApply(exception);
            }
        }

        public void ApplyLogHeadTruncationIfPermitted(Exception flushException, LogRecord record)
        {
            Utility.Assert(
                this.replicatedLogManager.LastInProgressTruncateHeadRecord == record,
                "this.lastInProgressTruncateHeadRecord ({0}) == record ({1})",
                this.replicatedLogManager.LastInProgressTruncateHeadRecord, record);

            var truncationState = (flushException == null) ? TruncationState.Ready : TruncationState.Faulted;
            TruncateHeadLogRecord truncateHeadRecord = null;

            // This protects any racing call to AbortPendingTruncation
            lock (thisLock)
            {
                truncateHeadRecord = this.replicatedLogManager.LastInProgressTruncateHeadRecord;

                // During idle build, truncatehead record is already stable
                if (truncateHeadRecord.IsStable || truncateHeadRecord.Lsn <= this.LastStableLsn)
                {
                    if (flushException == null)
                    {
                        truncationState = TruncationState.Applied;
                    }
                }

                if (truncateHeadRecord.TruncationState == TruncationState.Invalid)
                {
                    truncateHeadRecord.TruncationState = truncationState;
                }
                else
                {
                    truncationState = truncateHeadRecord.TruncationState;
                    Utility.Assert(
                        truncationState == TruncationState.Aborted,
                        "truncationState == TruncationState.Aborted");
                }
            }

            if ((truncationState == TruncationState.Applied) || (truncationState == TruncationState.Faulted)
                || (truncationState == TruncationState.Aborted))
            {
                LogHeadTruncationApply(this.tracer, truncateHeadRecord, flushException);
            }
        }

        public Task BlockSecondaryPumpIfNeeded(LogicalSequenceNumber lastStableLsn)
        {
            return logTruncationManager.BlockSecondaryPumpIfNeeded(lastStableLsn);
        }

        /// <summary>
        /// Append begin checkpoint record on idle secondary
        /// </summary>
        public void FirstBeginCheckpointOnIdleSecondary()
        {
            var record = InitiateCheckpoint(
                isPrimary: false,
                isFirstCheckpointOnFullCopy: true);

            replicatedLogManager.Insert_BeginCheckpoint(record);

            // This will ensure begin checkpoint record gets flushed to disk - including the first checkpoint on the idle replica
            // RDBug 9513833 - [Replicator] Fix missed flush while appending begin checkpoint
            this.replicatedLogManager.LogManager.FlushAsync("FirstBeginCheckpointOnIdleSecondary").IgnoreExceptionVoid();
        }

        [SuppressMessage(FxCop.Category.Globalization, FxCop.Rule.DoNotPassLiteralsAsLocalizedParameters, Justification = "message is not supposed to be localized.")]
        public CopyMode GetLogRecordsToCopy(
            ProgressVector targetProgressVector,
            Epoch targetLogHeadEpoch,
            LogicalSequenceNumber targetLogHeadLsn,
            LogicalSequenceNumber currentTargetLsn,
            long latestrecoveredAtomicRedoOperationLsn,
            long targetReplicaId,
            out LogicalSequenceNumber sourceStartingLsn,
            out LogicalSequenceNumber targetStartingLsn,
            out IAsyncEnumerator<LogRecord> logRecordsToCopy,
            out BeginCheckpointLogRecord completedBeginCheckpointRecord)
        {
            PhysicalLogReader physicalLogReader;
            CopyModeResult copyModeResult;
            logRecordsToCopy = null;
            completedBeginCheckpointRecord = null;

            LogManager.LogReaderType logReaderType;
            lock (headTruncationLock)
            {
                var sourceLogHeadRecord = this.replicatedLogManager.CurrentLogHeadRecord;
                copyModeResult = ProgressVector.FindCopyMode(
                    new CopyContextParameters(replicatedLogManager.ProgressVector, sourceLogHeadRecord.CurrentEpoch, sourceLogHeadRecord.Lsn, this.replicatedLogManager.CurrentLogTailLsn),
                    new CopyContextParameters(targetProgressVector, targetLogHeadEpoch, targetLogHeadLsn, currentTargetLsn),
                    latestrecoveredAtomicRedoOperationLsn);

                sourceStartingLsn = copyModeResult.SourceStartingLsn;
                targetStartingLsn = copyModeResult.TargetStartingLsn;

                ulong startingRecordPosition;

                if (copyModeResult.CopyMode == CopyMode.None)
                {
                    return CopyMode.None;
                }

                if (copyModeResult.CopyMode == CopyMode.Full)
                {
                    if (copyModeResult.FullCopyReason == FullCopyReason.ValidationFailed)
                    {
#if !DotNetCoreClr
                        // These are new events defined in System.Fabric, existing CoreCLR apps would break 
                        // if these events are refernced as it wont be found. As CoreCLR apps carry System.Fabric
                        // along with application
                        // This is just a mitigation for now. Actual fix being tracked via bug# 11614507

                        FabricEvents.Events.ProgressVectorValidationTelemetry(
                            this.tracer.Type,
                            copyModeResult.SharedProgressVectorEntry.FailedValidationMessage,
                            replicatedLogManager.ProgressVector.ToString(
                                    copyModeResult.SharedProgressVectorEntry.SourceIndex,
                                    Constants.ProgressVectorMaxStringSizeInKb / 2),
                            copyModeResult.SharedProgressVectorEntry.SourceIndex,
                            copyModeResult.SharedProgressVectorEntry.SourceProgressVectorEntry.ToString(),
                            targetProgressVector.ToString(
                                    copyModeResult.SharedProgressVectorEntry.TargetIndex,
                                    Constants.ProgressVectorMaxStringSizeInKb / 2),
                            copyModeResult.SharedProgressVectorEntry.TargetIndex,
                            copyModeResult.SharedProgressVectorEntry.TargetProgressVectorEntry.ToString());
#endif
                    }
                    else
                    {
                        FabricEvents.Events.PrimaryFullCopyInitiated(
                            this.tracer.Type,
                            "Reason: " + copyModeResult.FullCopyReason + ". Target Replica: " + targetReplicaId);
                    }

                    completedBeginCheckpointRecord =
                        this.replicatedLogManager.LastCompletedEndCheckpointRecord.LastCompletedBeginCheckpointRecord;
                    var earliestPendingTransaction =
                        completedBeginCheckpointRecord.EarliestPendingTransaction;
                    sourceStartingLsn = (earliestPendingTransaction != null)
                        ? (earliestPendingTransaction.Lsn - 1)
                        : completedBeginCheckpointRecord.Lsn;
                    startingRecordPosition = completedBeginCheckpointRecord.RecordPosition
                                             - completedBeginCheckpointRecord.EarliestPendingTransactionOffset;

                    logReaderType = LogManager.LogReaderType.FullCopy;
                }
                else
                {
                    Utility.Assert(copyModeResult.CopyMode.HasFlag(CopyMode.Partial), "(copyMode & CopyMode.Partial) != 0");

                    var startingLsn = (sourceStartingLsn < targetStartingLsn)
                        ? sourceStartingLsn
                        : targetStartingLsn;

                    completedBeginCheckpointRecord = null;
                    var startingRecord = this.replicatedLogManager.LogManager.CurrentLogTailRecord.PreviousPhysicalRecord;
                    do
                    {
                        var previousStartingRecord = startingRecord.PreviousPhysicalRecord;
                        if (previousStartingRecord == null || LogRecord.IsInvalidRecord(previousStartingRecord) == true)
                        {
                            break;
                        }

                        startingRecord = previousStartingRecord;

                        // Copied log records should contain every logical record after the
                        // record bearing starting LSN. For example, currently UpdateEpoch
                        // records are not assigned an LSN and can immediately follow the record
                        // bearing starting LSN
                        if (startingRecord.Lsn < startingLsn)
                        {
                            break;
                        }
                    } while (true);

                    startingRecordPosition = startingRecord.RecordPosition;
                    logReaderType = LogManager.LogReaderType.PartialCopy;
                }

                physicalLogReader = this.replicatedLogManager.LogManager.GetPhysicalLogReader(
                    startingRecordPosition,
                    this.replicatedLogManager.LogManager.CurrentLogTailRecord.RecordPosition,
                    this.replicatedLogManager.LogManager.CurrentLogTailRecord.Lsn.LSN,
                    "Build for replica " + targetReplicaId,
                    logReaderType);
            }

            using (physicalLogReader)
            {
                Utility.Assert(physicalLogReader.IsValid == true, "physicalLogReader.IsValid == true");
                logRecordsToCopy = physicalLogReader.GetLogRecords(
                    "Build for replica " + targetReplicaId,
                    logReaderType);

                FabricEvents.Events.GetLogRecordsToCopy(
                    this.tracer.Type,
                    physicalLogReader.StartingRecordPosition,
                    physicalLogReader.EndingRecordPosition);
            }

            return copyModeResult.CopyMode;
        }

        public void InsertPhysicalRecordsIfNecessary()
        {
            bool insertedIndex;
            bool insertedTruncateHead;
            this.InsertPhysicalRecordsIfNecessary(out insertedIndex, out insertedTruncateHead);
        }

        public void InsertPhysicalRecordsIfNecessary(out bool addedIndex, out bool addedTruncateHead)
        {
            addedIndex = this.IndexIfNecessary();
            addedTruncateHead = this.TruncateHeadIfNecessary();
        }

        public void InsertPhysicalRecordsIfNecessaryOnSecondary(
            LogicalSequenceNumber copiedUptoLsn,
            DrainingStream drainingStream,
            LogicalLogRecord record)
        {
            // Do not insert indexing record right next to UpdateEpoch log record
            // Or else, it can lead to situations where the log is truncated to this point and we can end up losing the UpdateEpoch log record
            if (record is UpdateEpochLogRecord)
            {
                return;
            }

            if (this.replicatedLogManager.CurrentLogTailLsn < this.recoveredOrCopiedCheckpointLsn.Value)
            {
                Utility.Assert(
                    (drainingStream == DrainingStream.CopyStream) || (copiedUptoLsn <= this.replicatedLogManager.CurrentLogTailLsn),
                    "(this.DrainingStream == DrainingStream.CopyStream) || (this.copiedUptoLsn <= this.currentLogTailLsn)");

                this.IndexIfNecessary();
            }
            else if (this.replicatedLogManager.CurrentLogTailLsn == this.recoveredOrCopiedCheckpointLsn.Value)
            {
                Utility.Assert(
                    (drainingStream == DrainingStream.CopyStream) || (copiedUptoLsn <= this.replicatedLogManager.CurrentLogTailLsn),
                    "(this.DrainingStream == DrainingStream.CopyStream) || (this.copiedUptoLsn <= this.currentLogTailLsn)");

                if (this.replicatedLogManager.CurrentLogTailEpoch < this.replicatedLogManager.ProgressVector.LastProgressVectorEntry.Epoch)
                {
                    return;
                }

                if ((this.replicatedLogManager.LastInProgressCheckpointRecord != null)
                    || (this.replicatedLogManager.LastCompletedEndCheckpointRecord != null))
                {
                    Utility.Assert(record is UpdateEpochLogRecord, "record is UpdateEpochLogRecord");
                    return;
                }

                this.FirstBeginCheckpointOnIdleSecondary();
            }

            this.InsertPhysicalRecordsIfNecessary();
            return;
        }

        public void Open(
            ITracer tracer,
            ulong checkpointSizeMB,
            ulong minLogSizeInMB,
            uint truncationThresholdFactor,
            uint throttlingThresholdFactor)
        {
            this.tracer = tracer;

            logTruncationManager.Open(
                replicatedLogManager,
                tracer,
                checkpointSizeMB,
                minLogSizeInMB,
                truncationThresholdFactor,
                throttlingThresholdFactor);
        }

        /// <summary>
        /// Release the backup consistency lock.
        /// </summary>
        /// <param name="lockReleaser">Name of the releaser.</param>
        [SuppressMessage(FxCop.Category.Globalization, FxCop.Rule.DoNotPassLiteralsAsLocalizedParameters, Justification = "message is not supposed to be localized.")]
        public void ReleaseBackupAndCopyConsistencyLock(string lockReleaser)
        {
            var stopWatch = new Stopwatch();
            stopWatch.Start();

            this.backupAndCopyConsistencyLock.Release();

            stopWatch.Stop();

            var latency = stopWatch.ElapsedMilliseconds;

            FabricEvents.Events.Lock(this.tracer.Type, "BackupConsistency", lockReleaser, "Released in " + latency);
        }

        /// <summary>
        /// Release the state manager API lock.
        /// </summary>
        /// <param name="lockReleaser">Name of the releaser.</param>
        [SuppressMessage(FxCop.Category.Globalization, FxCop.Rule.DoNotPassLiteralsAsLocalizedParameters, Justification = "message is not supposed to be localized.")]
        public void ReleaseStateManagerApiLock(string lockReleaser)
        {
            var stopWatch = new Stopwatch();
            stopWatch.Start();

            this.stateManagerApiLock.Release();

            stopWatch.Stop();

            var latency = stopWatch.ElapsedMilliseconds;

            FabricEvents.Events.Lock(this.tracer.Type, "StateManagerApi", lockReleaser, "Released in " + latency);
        }

        public BarrierLogRecord ReplicateBarrier()
        {
            BarrierLogRecord record = null;

            lock (thisLock)
            {
                record = new BarrierLogRecord(this.LastStableLsn);
            }

            ReplicateBarrier(record);
            this.ScheduleProcessingOnPrimary(record);
            return record;
        }

        public void RequestGroupCommit()
        {
            lock (thisLock)
            {
                this.groupCommitNeeded = true;
                if (this.groupCommitTask == null)
                {
                    this.ArmGroupCommitCallerHoldsLock();
                }
            }
        }

        public void ResetStableLsn(LogicalSequenceNumber newStableLsn)
        {
            lock (thisLock)
            {
                this.lastStableLsn = newStableLsn;
            }
        }

        public void Reuse()
        {
            this.lastStableLsn = LogicalSequenceNumber.ZeroLsn;
            this.groupCommitDelay = ReplicatorDynamicConstants.GroupCommitDelay;
            this.groupCommitTask = null;
            this.groupCommitNeeded = true;
        }
        public void ScheduleProcessingOnPrimary(BarrierLogRecord record)
        {
            Task.WhenAll(record.AwaitApply(), LoggingReplicator.AwaitReplication(record, this.replicatedLogManager)).IgnoreException().ContinueWith(
                task =>
                {
                    // Simply retrieve task exception to mark it as handled
                    if (task.Exception != null)
                    {
                        Utility.Assert(task.IsFaulted == true, "task.IsFaulted == true");
                    }

                    this.ProcessBarrierRecord(record, task.Exception, true);
                }).IgnoreExceptionVoid();
        }

        [SuppressMessage(FxCop.Category.Globalization, FxCop.Rule.DoNotPassLiteralsAsLocalizedParameters, Justification = "message is not supposed to be localized.")]
        public void ThrowIfThrottled(LogicalLogRecord record)
        {
            if (LoggingReplicator.CanThrottleWritesOnPrimary(record))
            {
                string message = null;
                if (this.replicatedLogManager.LogManager.ShouldThrottleWrites)
                {
                    message = string.Format(CultureInfo.InvariantCulture, "Higher incoming load than the disk throughput");
                }
                else if (this.logTruncationManager.ShouldBlockOperationsOnPrimary())
                {
                    if (this.replicatedLogManager.LastInProgressCheckpointRecord != null)
                    {
                        // Implies a pending checkpoint is using log space
                        message = string.Format(
                            CultureInfo.InvariantCulture,
                            "Throttling writes due to pending checkpoint");
                    }
                    else if (this.replicatedLogManager.LastInProgressTruncateHeadRecord != null)
                    {
                        // Implies a pending truncation is using log space. Could be due to backup or build
                        LogManager.LogReaderType readerType;
                        var earliestReader = this.replicatedLogManager.LogManager.GetEarliestLogReaderName(out readerType);
                        message = string.Format(
                            CultureInfo.InvariantCulture,
                            "Throttling writes due to pending log truncation that is blocked due to " + earliestReader
                            + " of type: " + readerType);
                    }
                    else
                    {
                        // Implies no tx's have been committed, leading to this condition where group commits are not happening
                        var abortTxList = this.logTruncationManager.GetOldTransactions(this.transactionMap);

                        if (abortTxList != null)
                        {
                            Task.Factory.StartNew(() => AbortOldTransactions(abortTxList, record.Lsn)).IgnoreException();
                        }
                    }
                }

                if (message != null)
                {
                    this.throttleRatePerfCounter.Increment();
                    FabricEvents.Events.Warning(this.tracer.Type, message);
                    throw new FabricTransientException(SR.Warning_V2ReplThrottle);
                }
            }
        }

        public void CancelPeriodicCheckpointTimer()
        {
            checkpointTimer.Change(Timeout.Infinite, Timeout.Infinite);
        }

        public void Recover(
            BeginCheckpointLogRecord lastCompletedBeginCheckpointRecord,
            long logRecordsMapTruncationTime)
        {
            long recoveredCheckpointTime = lastCompletedBeginCheckpointRecord.PeriodicCheckpointTimeTicks;
            this.lastPeriodicCheckpointTime = new DateTime(recoveredCheckpointTime);

            long recoveredTruncationTime = logRecordsMapTruncationTime;

            if (lastCompletedBeginCheckpointRecord.PeriodicTruncationTimeTicks > recoveredTruncationTime)
            {
                recoveredTruncationTime = lastCompletedBeginCheckpointRecord.PeriodicTruncationTimeTicks;
            }

            this.lastPeriodicTruncationTime = new DateTime(recoveredTruncationTime);

            this.logTruncationManager.Recover(
                recoveredCheckpointTime,
                recoveredTruncationTime);

            // Disable
            if (this.truncationInterval.TotalMilliseconds == 0)
            {
                return;
            }

            // Start Timer
            this.StartPeriodicCheckpointTimer();
        }

        private void AbortOldTransactions(
            List<BeginTransactionOperationLogRecord> abortTxList,
            LogicalSequenceNumber checkpointPreventedAtLsn)
        {
            lock (thisLock)
            {
                Utility.Assert(abortTxList != null, "AbortOldTransactionsCallerHoldsLock null abort list");

                if (abortTxList[abortTxList.Count - 1].Transaction.Id <= this.lastTxIdAbortedDueToCheckpointPrevention)
                {
                    return;
                }

                var abortedTxList = new List<BeginTransactionOperationLogRecord>();

                foreach (var beginTx in abortTxList)
                {
                    // Only abort transactions that have already not started aborting
                    if (beginTx.Transaction.Id > this.lastTxIdAbortedDueToCheckpointPrevention)
                    {
                        abortedTxList.Add(beginTx);

                        var badTx = beginTx.Transaction as Transaction;
                        Utility.Assert(badTx != null, "badTx != null");

                        // This can run into "Concurrency Exception" due to race between user and system.
                        // It is acceptable for this operation since it will be retried in the new throttled API call.
                        Task.Run(() => badTx.AbortBadTransaction()).IgnoreExceptionVoid();
                    }
                }

                FabricEvents.Events.CheckpointAbortOldTx(
                    this.tracer.Type,
                    abortedTxList.Count,
                    abortedTxList[0].Lsn.LSN,
                    abortedTxList[abortedTxList.Count - 1].Lsn.LSN,
                    checkpointPreventedAtLsn.LSN);

                this.lastTxIdAbortedDueToCheckpointPrevention = abortedTxList[abortedTxList.Count - 1].Transaction.Id;
            }
        }

        /// <summary>
        /// Acquire state manager API Lock.
        /// </summary>
        /// <param name="lockTaker">Name of the lock taker.</param>
        /// <returns>Asynchronous task that represents the operation.</returns>
        /// <remarks>
        /// MCoskun
        /// Used to ensure BackupAsync on the SM cannot be called while PrepareCheckpoint or PerformCheckpoint is inflight.
        /// </remarks>
        private async Task AcquireStateManagerApiLockAsync(string lockTaker)
        {
            var stopWatch = new Stopwatch();
            stopWatch.Start();

            await this.stateManagerApiLock.WaitAsync().ConfigureAwait(false);

            stopWatch.Stop();

            var latency = stopWatch.ElapsedMilliseconds;

            FabricEvents.Events.Lock(this.tracer.Type, "StateManagerApi", lockTaker, "Acquired in " + latency);
        }
        /// <summary>
        /// Apply checkpoint if necessary.
        /// </summary>
        /// <remarks>
        /// Will apply a begin checkpoint log record if all of the below are true
        /// 1) There is an inflight checkpoint
        /// 2) Prepare has been called.
        /// 3) LastLogicalSequenceNumber of the record is quorum acked.
        /// 
        /// 
        /// Multiple barriers can call this method. Hence the lock around the method
        /// This can also race with ApplyCheckpointIfPermitted call and hence the lock is at this level instead of the caller's level
        /// </remarks>
        private void ApplyCheckpointOrLogHeadTruncationIfNecessary()
        {
            var inProgressCheckpoint = this.replicatedLogManager.LastInProgressCheckpointRecord;
            var inProgressTruncateHead = this.replicatedLogManager.LastInProgressTruncateHeadRecord;

            if (inProgressCheckpoint != null &&
                inProgressCheckpoint.CheckpointState == CheckpointState.Ready &&
                inProgressCheckpoint.Lsn <= this.LastStableLsn)
            {
                inProgressCheckpoint.LastStableLsn = this.LastStableLsn;
                inProgressCheckpoint.CheckpointState = CheckpointState.Applied;
                this.CheckpointApply(null);
            }

            if (inProgressTruncateHead != null &&
                inProgressTruncateHead.TruncationState == TruncationState.Ready &&
                inProgressTruncateHead.Lsn <= this.LastStableLsn)
            {
                inProgressTruncateHead.TruncationState = TruncationState.Applied;
                LogHeadTruncationApply(this.tracer, inProgressTruncateHead, null);
            }
        }

        private void ArmGroupCommitCallerHoldsLock()
        {
            this.groupCommitTask = this.replicatedLogManager.LogManager.GetFlushCompletionTask();
            this.groupCommitTask.IgnoreException().ContinueWith(t => this.GroupCommit()).IgnoreExceptionVoid();

            return;
        }

        private void ArmGroupCommitTimerCallerHoldsLock(int delayInMilliseconds)
        {
            this.groupCommitTask = Task.Delay(delayInMilliseconds);
            this.groupCommitTask.ContinueWith(t => this.GroupCommit()).IgnoreExceptionVoid();
        }

        /// <summary>
        /// If Apply was successful, marks the record as applied.
        /// </summary>
        /// <param name="exception">Exception from apply.</param>
        private void CheckpointApply(Exception exception)
        {
            var checkpointRecord = this.replicatedLogManager.LastInProgressCheckpointRecord;
            Utility.Assert(
                (checkpointRecord.CheckpointState == CheckpointState.Applied)
                || (checkpointRecord.CheckpointState == CheckpointState.Faulted)
                || (checkpointRecord.CheckpointState == CheckpointState.Aborted),
                "(CheckpointState should be applied or faulted or aborted. State:{0})",
                checkpointRecord.CheckpointState);

            var state = exception == null ? "Apply" : "ApplyFailed";

            FabricEvents.Events.Checkpoint(
                this.tracer.Type,
                state,
                (int)checkpointRecord.CheckpointState,
                checkpointRecord.Lsn.LSN,
                checkpointRecord.Psn.PSN,
                checkpointRecord.RecordPosition,
                checkpointRecord.EarliestPendingTransactionLsn.LSN);

            checkpointRecord.CompletedApply(exception);
            return;
        }

        private async Task CompleteStateProviderCheckpointAsync(BeginCheckpointLogRecord checkpointRecord)
        {
            // todo: add memory log check
            await this.stateManager.CompleteCheckpointAsync().ConfigureAwait(false);

            FabricEvents.Events.Checkpoint(
                this.tracer.Type,
                "Complete",
                (int)checkpointRecord.CheckpointState,
                checkpointRecord.Lsn.LSN,
                checkpointRecord.Psn.PSN,
                checkpointRecord.RecordPosition,
                checkpointRecord.EarliestPendingTransactionLsn.LSN);

            this.replicatedLogManager.CompleteCheckpoint();

            await this.replicatedLogManager.LogManager.FlushAsync("CompleteCheckpoint").ConfigureAwait(false);
        }

        private async Task GroupCommit()
        {
            BarrierLogRecord record;

            lock (thisLock)
            {
                // Mark that group commit has started by setting the flag to false
                this.groupCommitNeeded = false;
                record = new BarrierLogRecord(this.LastStableLsn);
            }

            Exception replicateException = null;
            try
            {
                this.ReplicateBarrier(record);
            }
            catch (Exception ex)
            {
                replicateException = ex;
            }

            lock (thisLock)
            {
                this.groupCommitTask = null;

                if (replicateException != null)
                {
                    if (this.replicatedLogManager.RoleContextDrainState.ReplicaRole == ReplicaRole.Primary &&
                        this.replicatedLogManager.RoleContextDrainState.IsClosing == false)
                    {
                        this.groupCommitDelay *= ReplicatorDynamicConstants.GroupCommitBackoffFactor;
                        if (this.groupCommitDelay > ReplicatorDynamicConstants.MaxGroupCommitDelay)
                        {
                            this.groupCommitDelay = ReplicatorDynamicConstants.MaxGroupCommitDelay;
                        }

                        this.ArmGroupCommitTimerCallerHoldsLock(this.groupCommitDelay);

                        return;
                    }

                    return;
                }
            }

            // Wait for barrier record to get flushed and replicated
            Exception barrierException = null;
            try
            {
                await Task.WhenAll(record.AwaitApply(), LoggingReplicator.AwaitReplication(record, this.replicatedLogManager)).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                barrierException = e;
            }

            lock (thisLock)
            {
                // Process barrier record
                this.groupCommitDelay = ReplicatorDynamicConstants.GroupCommitDelay;

                // GopalK: The order of the following instructions is significant
                var isConsecutiveBarrierRecord = false;
                if (barrierException == null)
                {
                    if (this.lastStableLsn < record.Lsn)
                    {
                        isConsecutiveBarrierRecord = this.lastStableLsn == (record.Lsn - 1);
                        this.ProcessStableLsnCallerHoldsLock(record.Lsn, record);
                        this.ApplyCheckpointOrLogHeadTruncationIfNecessary();
                    }
                }

                // If there were any latency sensitive records that require to be applied, issue a group commit
                if ((this.groupCommitTask == null) && (this.groupCommitNeeded == true))
                {
                    this.ArmGroupCommitCallerHoldsLock();
                }
                else if ((this.groupCommitTask == null) && (this.replicatedLogManager.RoleContextDrainState.ReplicaRole == ReplicaRole.Primary)
                         && (isConsecutiveBarrierRecord == false))
                {
                    // Ensure that idle primary will replicate an additional barrier record
                    // to convey that all replicated operations are stable
                    this.ArmGroupCommitCallerHoldsLock();
                }
            }

            if (barrierException != null)
            {
                FabricEvents.Events.GroupCommitWarning(
                    this.tracer.Type,
                    barrierException.GetType().ToString() + Environment.NewLine + " Message: "
                    + barrierException.Message,
                    barrierException.HResult,
                    barrierException.StackTrace,
                    record.RecordType.ToString(),
                    record.Lsn.LSN,
                    record.Psn.PSN,
                    record.LastStableLsn.ToString(),
                    this.lastStableLsn.ToString());
            }

            this.logicalRecordProcessedCallback(record);

            return;
        }

        // Insert an index at random to the log records
        private bool IndexIfNecessary()
        {
            lock (thisLock)
            {
                if (this.logTruncationManager.ShouldIndex())
                {
                    this.replicatedLogManager.Index();
                    return true;
                }

                return false;
            }
        }

        // Start truncating log
        public BeginCheckpointLogRecord CheckpointIfNecessary(bool isPrimary)
        {
            List<BeginTransactionOperationLogRecord> abortTxList = null;

            if ((isPrimary && this.logTruncationManager.ShouldCheckpointOnPrimary(this.transactionMap, out abortTxList)) ||
                this.logTruncationManager.ShouldCheckpointOnSecondary(this.transactionMap))
            {
                Utility.Assert(
                    this.replicatedLogManager.LastInProgressCheckpointRecord == null,
                    "this.lastInProgressCheckpointRecord == null");

                return InitiateCheckpoint(isPrimary, false);
            }

            // Abort old transactions if needed
            if (abortTxList != null)
            {
                Task.Factory.StartNew(() => AbortOldTransactions(abortTxList, replicatedLogManager.CurrentLogTailLsn)).IgnoreException();
            }

            return null;
        }

        private BeginCheckpointLogRecord InitiateCheckpoint(
            bool isPrimary,
            bool isFirstCheckpointOnFullCopy)
        {
            var failedBarrierCheck = false;
            var earliestPendingTx = this.transactionMap.GetEarliestPendingTransaction(this.replicatedLogManager.CurrentLogTailLsn.LSN, out failedBarrierCheck);

            Utility.Assert(isPrimary == false || isFirstCheckpointOnFullCopy == false, "isPrimary == false || isFirstCheckpointOnFullCopy == false");

            if (failedBarrierCheck)
            {
                Utility.Assert(isPrimary == true, "Checkpoint failed barrier check on a non primary replica");
                return null;
            }
            else
            {
                var lastCompletedBackupLogRecord = this.backupManager.LastCompletedBackupLogRecord;

                if (this.logTruncationManager.PeriodicCheckpointTruncationState == PeriodicCheckpointTruncationState.CheckpointStarted)
                {
                    this.lastPeriodicCheckpointTime = DateTime.Now;

                    this.TracePeriodicState();
                }

                var record = new BeginCheckpointLogRecord(
                    isFirstCheckpointOnFullCopy,
                    this.replicatedLogManager.ProgressVector,
                    earliestPendingTx,
                    this.replicatedLogManager.CurrentLogHeadRecord.CurrentEpoch,
                    this.replicatedLogManager.CurrentLogTailEpoch,
                    this.replicatedLogManager.CurrentLogTailLsn,
                    this.replicatedLogManager.LastLinkedPhysicalRecord,
                    lastCompletedBackupLogRecord,
                    this.progressVectorMaxEntries,
                    this.lastPeriodicCheckpointTime.Ticks,
                    this.lastPeriodicTruncationTime.Ticks);

                FabricEvents.Events.Checkpoint(
                    this.tracer.Type,
                    "Initiate",
                    (int)record.CheckpointState,
                    record.Lsn.LSN,
                    record.Psn.PSN,
                    record.RecordPosition,
                    record.EarliestPendingTransactionLsn.LSN);

                record.AwaitApply()
                    .IgnoreException()
                    .ContinueWith(async task => { await this.PerformCheckpoint(task.Exception).ConfigureAwait(false); })
                    .IgnoreExceptionVoid();

                return record;
            }
        }

        private static void LogHeadTruncationApply(
            ITracer tracer,
            TruncateHeadLogRecord truncateHeadRecord,
            Exception flushException)
        {
            Utility.Assert(truncateHeadRecord != null, "Truncate head is null in LogHeadTruncationApply");

            Utility.Assert(
                (truncateHeadRecord.TruncationState == TruncationState.Applied)
                || (truncateHeadRecord.TruncationState == TruncationState.Faulted)
                || (truncateHeadRecord.TruncationState == TruncationState.Aborted),
                "(TruncationState should be applied, faulted or aborted. Truncation state :{0})",
                truncateHeadRecord.TruncationState);

            FabricEvents.Events.TruncateHeadApply(
                tracer.Type,
                truncateHeadRecord.TruncationState.ToString(),
                truncateHeadRecord.Lsn.LSN,
                truncateHeadRecord.Psn.PSN,
                truncateHeadRecord.RecordPosition,
                truncateHeadRecord.LogHeadLsn.LSN,
                truncateHeadRecord.LogHeadPsn.PSN,
                truncateHeadRecord.LogHeadRecordPosition);

            truncateHeadRecord.CompletedApply(flushException);
            return;
        }

        private async Task LogHeadTruncationAsyncProcess(Exception processingException)
        {
            TruncateHeadLogRecord truncateHeadRecord = null;

            truncateHeadRecord = this.replicatedLogManager.LastInProgressTruncateHeadRecord;
            Utility.Assert(truncateHeadRecord != null, "Last in progress truncate head must not be null in LogHeadTruncationAsyncProcess");

            var isTruncationPerformed = false;

            if (truncateHeadRecord.TruncationState == TruncationState.Applied)
            {
                await this.replicatedLogManager.LogManager.ProcessLogHeadTruncationAsync(truncateHeadRecord);
                truncateHeadRecord.TruncationState = TruncationState.Completed;
                isTruncationPerformed = true;
            }

            this.OnCompletePendingLogHeadTruncation(truncateHeadRecord.TruncationState);
            this.physicalRecordProcessedCallback(truncateHeadRecord);

            var logUsage = this.replicatedLogManager.LogManager.Length;

            FabricEvents.Events.TruncateHeadProcess(
                this.tracer.Type,
                truncateHeadRecord.Lsn.LSN,
                truncateHeadRecord.Psn.PSN,
                truncateHeadRecord.RecordPosition,
                truncateHeadRecord.LogHeadLsn.LSN,
                truncateHeadRecord.LogHeadPsn.PSN,
                truncateHeadRecord.LogHeadRecordPosition,
                isTruncationPerformed,
                truncateHeadRecord.TruncationState + " LogUsage: " + logUsage);

            return;
        }

        /// <summary>
        /// Completes the checkpoint by Performing, logging EndCheckpoint, renaming if necessary and completing
        /// </summary>
        /// <param name="processingException">Expected that was thrown from Apply.</param>
        /// <returns></returns>
        /// <remarks>
        /// Once a BeginCheckpoint record is applied
        /// (BeginCheckpoint logical sequence number has been quorum committed and PrepareCheckpont has been called),
        /// this method is called to finish the checkpoint.
        /// </remarks>
        private async Task PerformCheckpoint(Exception processingException)
        {
            var checkpointRecord = this.replicatedLogManager.LastInProgressCheckpointRecord;

            if (checkpointRecord.CheckpointState != CheckpointState.Applied)
            {
                if (processingException != null)
                {
                    this.ProcessException("CheckpointManager", checkpointRecord, "PerformCheckpoint", processingException);
                }

                // This will ensure exception is thrown in draining code path
                if (checkpointRecord.IsFirstCheckpointOnFullCopy)
                {
                    checkpointRecord.SignalExceptionForPhase1OfFirstCheckpointOnFullCopy(processingException, this.tracer.Type);

                    // Do not complete the pending checkpoint here. The drain code will complete it
                    return;
                }

                this.physicalRecordProcessedCallback(checkpointRecord);

                this.OnCompletePendingCheckpoint(processingException, checkpointRecord.CheckpointState);

                return;
            }

            // Processing the applied checkpoints.
            try
            {
                await this.PerformStateProviderCheckpointAsync(checkpointRecord).ConfigureAwait(false);

                if (checkpointRecord.IsFirstCheckpointOnFullCopy)
                {
                    FabricEvents.Events.Checkpoint(
                        this.tracer.Type,
                        "DelayCompleteUntilDrainCopyEnds",
                        (int)checkpointRecord.CheckpointState,
                        checkpointRecord.Lsn.LSN,
                        checkpointRecord.Psn.PSN,
                        checkpointRecord.RecordPosition,
                        checkpointRecord.EarliestPendingTransactionLsn.LSN);

                    checkpointRecord.SignalCompletionPhase1FirstCheckpointOnFullCopy(this.tracer.Type);
                    return;
                }

                await CompleteCheckpointAndRenameIfNeeded(checkpointRecord, false);
            }
            catch (Exception e)
            {
                this.ProcessException("CheckpointManager", checkpointRecord, "PerformCheckpoint", e);

                if (checkpointRecord.IsFirstCheckpointOnFullCopy)
                {
                    checkpointRecord.SignalExceptionForPhase1OfFirstCheckpointOnFullCopy(e, this.tracer.Type);

                    // Do not complete the pending checkpoint here. The drain code will complete it
                    return;
                }
            }

            this.physicalRecordProcessedCallback(checkpointRecord);

            this.OnCompletePendingCheckpoint(processingException, checkpointRecord.CheckpointState);

            return;
        }

        private async Task CompleteCheckpointAndRenameIfNeeded(
            BeginCheckpointLogRecord checkpointRecord,
            bool renameCopyLog)
        {
            await this.AcquireBackupAndCopyConsistencyLockAsync("CompleteCheckpointAndRenameIfNeeded").ConfigureAwait(false);

            try
            {
                await this.replicatedLogManager.FlushEndCheckpointAsync().ConfigureAwait(false);

                if (renameCopyLog)
                {
                    Utility.Assert(
                        recoveredOrCopiedCheckpointLsn.Value == checkpointRecord.Lsn,
                        "CompleteCheckpointAndRenameIfNeeded: Renaming copy log must be true only when recoveredorcopiedcheckpointlsn {0} is checkpointrecord.lsn {1}",
                        recoveredOrCopiedCheckpointLsn.Value, checkpointRecord.Lsn);

                    await this.replicatedLogManager.LogManager.FlushAsync("CompleteCheckpointAndRenameIfNeeded-RenameCopyLogAtomically").ConfigureAwait(false);

                    Utility.Assert(
                        this.replicatedLogManager.LogManager.IsCompletelyFlushed == true,
                        "this.replicatedLogManager.LogManager.IsCompleteFlushed == true when renaming copy log atomically");

                    // Ensure the copy log is fully flushed to ensure that current tail record
                    // does not change underneath the rename.
                    await this.replicatedLogManager.LogManager.RenameCopyLogAtomicallyAsync().ConfigureAwait(false);

                    Utility.Assert(
                        this.replicatedLogManager.LogManager.IsCompletelyFlushed == true,
                        "post this.replicatedLogManager.LogManager.IsCompleteFlushed == true when renaming copy log atomically");
                }

                await this.CompleteStateProviderCheckpointAsync(checkpointRecord).ConfigureAwait(false);
            }
            finally
            {
                this.ReleaseBackupAndCopyConsistencyLock("CompleteCheckpointAndRenameIfNeeded");
            }
        }

        private async Task PerformStateProviderCheckpointAsync(BeginCheckpointLogRecord checkpointRecord)
        {
            await this.AcquireStateManagerApiLockAsync("PerformCheckpoint").ConfigureAwait(false);
            try
            {
                //todo: add memory log check
                PerformCheckpointMode performMode = this.logTruncationManager.PeriodicCheckpointTruncationState == PeriodicCheckpointTruncationState.CheckpointStarted ?
                    PerformCheckpointMode.Periodic :
                    PerformCheckpointMode.Default;

                await this.stateManager.PerformCheckpointAsync(performMode).ConfigureAwait(false);
            }
            finally
            {
                this.ReleaseStateManagerApiLock("PerformCheckpoint");
            }

            this.checkpointPerfCounter.Increment();

            FabricEvents.Events.Checkpoint(
                this.tracer.Type,
                "Perform",
                (int)checkpointRecord.CheckpointState,
                checkpointRecord.Lsn.LSN,
                checkpointRecord.Psn.PSN,
                checkpointRecord.RecordPosition,
                checkpointRecord.EarliestPendingTransactionLsn.LSN);

            checkpointRecord.CheckpointState = CheckpointState.Completed;
        }

        private void ProcessBarrierRecord(
            BarrierLogRecord barrierRecord,
            Exception exception,
            bool isPrimary)
        {
            try
            {
                var newStableLsn = isPrimary ?
                    barrierRecord.Lsn :
                    barrierRecord.LastStableLsn;

                if (exception == null)
                {
                    // Optimistic check to ensure the lock is not contended on unnecessarily when there are lots of barriers about to be processed
                    if (this.lastStableLsn < newStableLsn)
                    {
                        lock (thisLock)
                        {
                            if (this.lastStableLsn < newStableLsn)
                            {
                                this.ProcessStableLsnCallerHoldsLock(newStableLsn, barrierRecord);
                                this.ApplyCheckpointOrLogHeadTruncationIfNecessary();
                            }
                        }
                    }
                }
                else
                {
                    FabricEvents.Events.ProcessBarrierRecordException(
                        this.tracer.Type,
                        "ProcessBarrierRecordOnPrimary: Encountered exception. Type: " + exception.GetType() + " Message: "
                        + exception.Message + " HResult: 0x" + exception.HResult + " Stack: " + exception.StackTrace,
                        (int)this.replicatedLogManager.RoleContextDrainState.DrainingStream,
                        barrierRecord.Psn.PSN,
                        barrierRecord.LastStableLsn.LSN,
                        this.lastStableLsn.LSN);
                }
            }
            catch (Exception e)
            {
                Utility.ProcessUnexpectedException("BarrierManager", tracer, "Exception during process log record", e);
            }

            this.logicalRecordProcessedCallback(barrierRecord);

            return;
        }

        private async Task ProcessBarrierRecordOnSecondaryAsync(BarrierLogRecord barrierRecord)
        {
            Exception barrierException = null;
            try
            {
                await barrierRecord.AwaitApply().ConfigureAwait(false);
            }
            catch (Exception e)
            {
                barrierException = e;
            }

            this.ProcessBarrierRecord(barrierRecord, barrierException, false);
        }

        [SuppressMessage(FxCop.Category.Globalization, FxCop.Rule.DoNotPassLiteralsAsLocalizedParameters, Justification = "message is not supposed to be localized.")]
        private void ProcessException(string component, LogRecord record, string messagePrefix, Exception e)
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

            FabricEvents.Events.Error(this.tracer.Type, message);

            this.replicatedLogManager.RoleContextDrainState.ReportPartitionFault();
        }

        private void ProcessStableLsnCallerHoldsLock(
            LogicalSequenceNumber stableLsn,
            BarrierLogRecord barrierRecord)
        {
            Utility.Assert(this.lastStableLsn < stableLsn, "this.lastStableLsn < stableLsn");
            this.lastStableLsn = stableLsn;
            this.transactionMap.RemoveStableTransactions(this.lastStableLsn);

            FabricEvents.Events.ProcessBarrierRecord(
                this.tracer.Type,
                (int)this.replicatedLogManager.RoleContextDrainState.DrainingStream,
                barrierRecord.Psn.PSN,
                barrierRecord.LastStableLsn.LSN,
                this.lastStableLsn.LSN);
        }

        private void ReplicateBarrier(BarrierLogRecord record)
        {
            this.replicatedLogManager.ReplicateAndLog(record, null);
        }

        // Determine if truncating is needed
        private bool TruncateHeadIfNecessary()
        {
            // Ensures only 1 truncate head record is inserted
            lock (thisLock)
            {
                // Ensures head truncation does not happen due to log readers because of copy/builds
                lock (headTruncationLock)
                {
                    if (!this.logTruncationManager.ShouldTruncateHead())
                    {
                        return false;
                    }

                    bool isPeriodicTruncation = this.logTruncationManager.PeriodicCheckpointTruncationState == PeriodicCheckpointTruncationState.TruncationStarted;

                    var record = this.replicatedLogManager.TruncateHead(
                        false,
                        isPeriodicTruncation ? 
                            DateTime.Now.Ticks : 
                            this.lastPeriodicTruncationTime.Ticks,
                        logTruncationManager.IsGoodLogHeadCandidiate);

                    if (record != null)
                    {
                        FabricEvents.Events.TruncateHeadInitiate(
                            this.tracer.Type,
                            record.Lsn.LSN,
                            record.Psn.PSN,
                            record.RecordPosition,
                            record.LogHeadLsn.LSN,
                            record.LogHeadPsn.PSN,
                            record.LogHeadRecordPosition,
                            record.IsStable);

                        if (isPeriodicTruncation)
                        {
                            this.TracePeriodicState();
                        }

                        record.AwaitApply()
                            .IgnoreException()
                            .ContinueWith(
                                async task => 
                                    {
                                        if (isPeriodicTruncation)
                                        {
                                            lastPeriodicTruncationTime = new DateTime(record.PeriodicTruncationTimestampTicks);
                                        }

                                        await this.LogHeadTruncationAsyncProcess(task.Exception).ConfigureAwait(false);
                                    })
                            .IgnoreExceptionVoid();

                        return true;
                    }
                }

                return false;
            }
        }

        private void StartPeriodicCheckpointTimer()
        {
            TimeSpan waitDuration = CalculateTruncationTimerDuration(
                lastPeriodicCheckpointTime,
                truncationInterval,
                logTruncationManager.PeriodicCheckpointTruncationState);

            this.checkpointTimer.Change(waitDuration, waitDuration);
        }

        private void TimerCallback(object sender)
        {
            if (!this.replicatedLogManager.RoleContextDrainState.IsClosing)
            {
                this.logTruncationManager.InitiatePeriodicCheckpoint();

                // Only request group commit on primary replicas
                if (this.replicatedLogManager.RoleContextDrainState.ReplicaRole == ReplicaRole.Primary)
                {
                    RequestGroupCommit();
                }

                this.TracePeriodicState();

                StartPeriodicCheckpointTimer();
            }
        }

        private void OnCompletePendingCheckpoint(Exception ex, CheckpointState state)
        {
            bool tracePeriodicState = this.logTruncationManager.PeriodicCheckpointTruncationState == PeriodicCheckpointTruncationState.CheckpointStarted;
            this.logTruncationManager.OnCheckpointCompleted(ex, state, false);

            if (tracePeriodicState)
            {
                this.TracePeriodicState();
            }

            this.replicatedLogManager.OnCompletedPendingCheckpoint();
        }

        private void OnCompletePendingLogHeadTruncation(TruncationState state)
        {
            this.replicatedLogManager.OnCompletedPendingLogHeadTruncation();

            if (state == TruncationState.Completed)
            {
                bool tracePeriodicState = this.logTruncationManager.PeriodicCheckpointTruncationState == PeriodicCheckpointTruncationState.TruncationStarted;

                this.logTruncationManager.OnTruncationCompleted();

                if (tracePeriodicState)
                {
                    this.TracePeriodicState();
                }
            }
        }

        private void TracePeriodicState()
        {
#if !DotNetCoreClr
            FabricEvents.Events.PeriodicCheckpointTruncation(
                this.tracer.Type,
                this.logTruncationManager.PeriodicCheckpointTruncationState.ToString(),
                this.lastPeriodicCheckpointTime.ToString(),
                this.lastPeriodicTruncationTime.ToString());
#endif
        }
    }
}
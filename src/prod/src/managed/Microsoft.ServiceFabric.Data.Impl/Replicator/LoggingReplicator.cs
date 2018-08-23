// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Data.Notifications;
    using Microsoft.ServiceFabric.Replicator.Backup;
    using Microsoft.ServiceFabric.Replicator.Diagnostics;
    using Microsoft.ServiceFabric.Replicator.Utils;
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Runtime.CompilerServices;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Implementation of logging replicator
    /// </summary>
    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1118:ParameterMustNotSpanMultipleLines", Justification = "related classes")]
    internal class LoggingReplicator : ILoggingReplicator,
        ITransactionManager,
        IBackupRestoreProvider,
        IVersionProvider,
        IDisposable
    {
        /// <summary>
        /// Enumeration for primary status.
        /// </summary>
        public enum PrimaryElectionStatus
        {
            /// <summary>
            /// Replica that is not a primary.
            /// </summary>
            None = 0,

            /// <summary>
            /// Primary that is being demoted in favor of promoting an active secondary in configuration set.
            /// </summary>
            SwappingOut = 1,

            /// <summary>
            /// Replica that retained the primary status across a failover.
            /// Will only start executing when Reliability introduces reconfiguration free partition wide restarts.
            /// </summary>
            Retained = 2,

            /// <summary>
            /// Replica that has been elected to be the new Primary (common case).
            /// </summary>
            Elected = 3,

            /// <summary>
            /// Replica that has established it self as the new Primary
            /// This means that it has started to replicate and log a Barrier log record.
            /// </summary>
            Established = 4,
        }

        internal static readonly Epoch InvalidEpoch = new Epoch(-1, -1);

        /// <summary>
        /// Performance counters.
        /// </summary>
        internal static readonly FabricPerformanceCounterSet ReplicatorCounterSet = InitializePerformanceCounterDefinition();

        private readonly object backupDrainLock = new object();

        private readonly object flushWaitLock = new object();

        private readonly object primaryStatusLock = new object();

        private readonly IStateManager stateManager;

        private int abortDelay;

        /// <summary>
        /// ReplicatorApiMonitor is used to monitor and report health events
        /// </summary>
        private ReplicatorApiMonitor apiMonitor;

        private AvgTransactionCommitLatencyCounterWriter avgTransactionCommitLatencyCounterWriter;

        /// <summary>
        /// Backup manager in charge of backup and restore operations.
        /// </summary>
        private IBackupManager backupManager;

        private AvgBytesPerFlushCounterWriter bytesPerFlushCounterWriter;

        private CheckpointCountCounterWriter checkpointCountCounterWriter;

        private CheckpointManager checkpointManager;

        private TaskCompletionSource<object> drainBackupCallBackTcs;

        private IStateReplicator fabricReplicator;

        private IncomingBytesRateCounterWriter incomingBytesRateCounterWriter;

        private InflightVisibilitySequenceNumberCountCounterWriter inflightVisibilitySequeneceNumberCountCounterWriter;

        // Fabric provided settings
        private StatefulServiceContext initializationParameters;

        private bool isBackupCallbackInflight = false;

        // TODO: Temporarily internal for testing purposes. Should be made private.
        private bool isBackupDrainablePhaseAllowed = false;

        private bool isDisposed;

        private string logDirectory;

        private LogFlushBytesRateCounterWriter logFlushBytesRateCounterWriter;

        private AvgFlushLatencyCounterWriter avgFlushLatencyCounterWriter;

        private AvgSerializationLatencyCounterWriter avgSerializationLatencyCounterWriter;

        /// <summary>
        /// Defines the engine for the logger.
        /// </summary>
        private LoggerEngine loggerEngineMode;

        /// <summary>
        /// Log Manager in charge of the log.
        /// </summary>
        private LogManager logManager;

        private LogRecordsDispatcher logRecordsDispatcher;

        private LogRecoveryPerfomanceCounterWriter logRecoveryPerformanceCounterWriter;

        /// <summary>
        /// Partition object.
        /// </summary>
        private IStatefulServicePartition partition;

        private PrimaryElectionStatus primaryStatus;
        private List<BeginTransactionOperationLogRecord> primaryTransactionsAbortingInstance;
        private Task primaryTransactionsAbortOrGroupCommitTask;

        /// <summary>
        /// Cancellation token source used for canceling aborting of pending transactions.
        /// </summary>
        private CancellationTokenSource primaryTransactionsAbortOrGroupCommitTaskCompletionSource;

        private readonly RecoveredOrCopiedCheckpointLsn recoveredOrCopiedCheckpointLsn = new RecoveredOrCopiedCheckpointLsn();
        private ReplicatedLogManager replicatedLogManager;
        private FabricPerformanceCounterSetInstance replicatorCounterSetInstance;
        private TransactionalReplicatorSettings replicatorSettings;

        private StateManagerRecoveryPerfomanceCounterWriter stateManagerRecoveryPerformanceCounterWriter;

        /// <summary>
        /// Test flag that causes restore to throw.
        /// </summary>
        private bool testFlagThrowExceptionAtRestoreData = false;

        private ThrottleRateCounterWriter throttleRateCounterWriter;

        private ITracer tracer;

        private readonly RoleContextDrainState roleContextDrainState = new RoleContextDrainState();
        private readonly SecondaryDrainManager secondaryDrainManager = new SecondaryDrainManager();
        private readonly OperationProcessor recordsProcessor = new OperationProcessor();
        private TransactionManager transactionManager;
        private RecoveryManager recoveryManager;

        private readonly TimeSpan BackgroundTelemetryPeriod = TimeSpan.FromMinutes(3);
        private CancellationTokenSource backgroundTelemetryCts = null;
        private Task backgroundTelemetryTask = null;

        /// <summary>
        /// Version manager in charge of snapshot visibility.
        /// </summary>
        private IInternalVersionManager versionManager;

        // Waiting copy streams
        private List<KeyValuePair<TaskCompletionSource<object>, LogicalSequenceNumber>> waitingStreams;

        public event EventHandler<NotifyTransactionChangedEventArgs> TransactionChanged
        {
            add
            {
                Utility.Assert(this.recordsProcessor != null, "TransactionChanged event needs unlock callback manager to be non null");
                this.recordsProcessor.TransactionChanged += value;
            }

            remove
            {
                Utility.Assert(this.recordsProcessor != null, "TransactionChanged event needs unlock callback manager to be non null");
                this.recordsProcessor.TransactionChanged -= value;
            }
        }

        /// <summary>
        /// This constructor is for testing purpose, not intended for production use.
        /// </summary>
        [Obsolete("This constructor is for testing purpose, not intended for production use.", false)]
        internal LoggingReplicator()
        {
        }

        internal LoggingReplicator(IStateManager stateManager, string logDirectory = null)
        {
            stateManager.ThrowIfNull("stateManager");

            this.stateManager = stateManager;
            this.logDirectory = logDirectory;
        }

        ~LoggingReplicator()
        {
            this.Dispose(false);
        }

        /// <summary>
        /// ReplicatorApiMonitor is used to monitor and report health events
        /// </summary>
        public ReplicatorApiMonitor ApiMonitor
        {
            get { return this.apiMonitor; }
        }

        public Epoch CurrentLogTailEpoch
        {
            get { return this.replicatedLogManager.CurrentLogTailEpoch; }
        }

        internal RecoveryManager RecoveryManager
        {
            get
            {
                return this.recoveryManager;
            }
        }

        public LogicalSequenceNumber CurrentLogTailLsn
        {
            get { return this.replicatedLogManager.CurrentLogTailLsn; }
        }

        public IStateReplicator FabricReplicator
        {
            get { return this.fabricReplicator; }

            set { this.fabricReplicator = value; }
        }

        public StatefulServiceContext InitializationParameters
        {
            get { return this.initializationParameters; }
        }

        /// <summary>
        /// Gets a value indicating whether the replicator is snapshot readable.
        /// </summary>
        /// <remarks>
        /// For the replicator to support snapshot, all operation upto and including the highest lsn in copy or recovery lsn must have benn applied.
        /// </remarks>
        public bool IsReadable
        {
            get
            {
                if (this.roleContextDrainState.ReplicaRole == ReplicaRole.Primary)
                {
                    return this.StatefulPartition.ReadStatus == PartitionAccessStatus.Granted;
                }

                return this.recordsProcessor.LastAppliedBarrierRecord.Lsn >= this.secondaryDrainManager.ReadConsistentAfterLsn;
            }
        }

        public LogicalSequenceNumber LastStableLsn
        {
            get { return checkpointManager.LastStableLsn; }
        }

        public virtual TransactionalReplicatorSettings ReplicatorSettings
        {
            get { return this.replicatorSettings; }
        }

        public ReplicaRole Role
        {
            get { return this.roleContextDrainState.ReplicaRole; }
        }

        public IStatefulServicePartition StatefulPartition
        {
            get { return this.partition; }
        }

        public ITracer Tracer
        {
            get { return this.tracer; }

            set
            {
                this.tracer = value;
                this.logRecordsDispatcher.Tracer = value;
            }
        }

        /// <summary>
        /// Gets the BackupManager.
        /// Test only property.
        /// </summary>
        internal IBackupManager BackupManager
        {
            get { return this.backupManager; }
        }
        /// <summary>
        /// Test only property.
        /// </summary>        
        internal BeginCheckpointLogRecord LastCompletedBeginCheckpointLogRecord
        {
            get { return this.replicatedLogManager.LastCompletedEndCheckpointRecord.LastCompletedBeginCheckpointRecord; }
        }
        /// <summary>
        /// Test only property.
        /// </summary>        
        internal LogManager LogManager
        {
            get { return this.logManager; }

            set { this.logManager = value; }
        }

        /// <summary>
        /// Test only property
        /// </summary>
        internal CheckpointManager CheckpointManager
        {
            get { return this.checkpointManager; }
        }

        /// <summary>
        /// Test only property
        /// </summary>
        internal ReplicatedLogManager ReplicatedLogManager
        {
            get { return this.replicatedLogManager; }
        }

        internal bool TestFlagNoOpDeleteRestoreToken
        {
            set { this.backupManager.TestFlagNoOpDeleteRestoreToken = value; }
        }

        internal bool TestFlagThrowExceptionAtRestoreData
        {
            get { return this.testFlagThrowExceptionAtRestoreData; }

            set
            {
                FabricEvents.Events.TestSettingModified(
                    this.tracer.Type,
                    "TestFlagThrowExceptionAtRestoreData",
                    "true");
                this.testFlagThrowExceptionAtRestoreData = value;
            }
        }

        public async static Task AwaitReplication(
            LogicalLogRecord record,
            IReplicatedLogManager replicatedLogManager)
        {
            try
            {
                await record.AwaitReplication().ConfigureAwait(false);
            }
            catch (Exception e)
            {
                replicatedLogManager.ThrowReplicationException(record, e);
            }

            return;
        }

        /// <summary>
        /// Determines if we can throttle additional writes on the primary
        ///
        /// Replicator throttles writes only on add operations and being transactions and atomic/redo-only operations
        /// Commits and Barriers are light weight records and hence we allow them
        /// </summary>
        /// <param name="logicalRecord"></param>
        /// <returns></returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool CanThrottleWritesOnPrimary(LogicalLogRecord logicalRecord)
        {
            switch (logicalRecord.RecordType)
            {
                case LogRecordType.BeginTransaction:
                case LogRecordType.Operation:
                    return true;

                default:
                    return false;
            }
        }

        /// <summary>
        /// API trace (Information)
        /// </summary>
        /// <param name="tracer">The tracer.</param>
        /// <param name="message">Message to be traced.</param>
        public static void ReplicatorTraceOnApi(ITracer tracer, string message)
        {
            FabricEvents.Events.Api(tracer.Type, message);
        }

        public async Task<long> AbortTransactionAsync(Transaction transaction)
        {
            this.ReplicateBarrierIfPrimaryStatusIsBelowEstablished();
            var lsn = await this.transactionManager.AbortTransactionAsync(transaction);
            return lsn;
        }

        public Task<bool> AcquireBackupAndCopyConsistencyLockAsync(string lockTaker, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.checkpointManager.AcquireBackupAndCopyConsistencyLockAsync(lockTaker, timeout, cancellationToken);
        }

        public Task<bool> AcquireStateManagerApiLockAsync(string lockTaker, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.checkpointManager.AcquireStateManagerApiLockAsync(lockTaker, timeout, cancellationToken);
        }

        public void AddOperation(
            Transaction transaction,
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext)
        {
            this.ReplicateBarrierIfPrimaryStatusIsBelowEstablished();
            this.transactionManager.AddOperation(
                transaction,
                metaData,
                undo,
                redo,
                operationContext);
        }

        public Task<long> AddOperationAsync(
            AtomicOperation atomicOperation,
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext)
        {
            this.ReplicateBarrierIfPrimaryStatusIsBelowEstablished();
            return this.transactionManager.AddOperationAsync(
                atomicOperation,
                metaData,
                undo,
                redo,
                operationContext);
        }

        public Task<long> AddOperationAsync(
            AtomicRedoOperation atomicRedoOperation,
            OperationData metaData,
            OperationData redo,
            object operationContext)
        {
            this.ReplicateBarrierIfPrimaryStatusIsBelowEstablished();
            return this.transactionManager.AddOperationAsync(
                atomicRedoOperation,
                metaData,
                redo,
                operationContext);
        }

        public Task<BackupInfo> BackupAsync(Func<BackupInfo, CancellationToken, Task<bool>> backupCallback)
        {
            return this.backupManager.BackupAsync(this.replicatedLogManager, this.checkpointManager, backupCallback);
        }

        public Task<BackupInfo> BackupAsync(
            Func<BackupInfo, CancellationToken, Task<bool>> backupCallback,
            BackupOption backupOption,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.backupManager.BackupAsync(this.replicatedLogManager, this.checkpointManager, backupCallback, backupOption, timeout, cancellationToken);
        }

        public async Task BarrierAsync()
        {
            var record = this.checkpointManager.ReplicateBarrier();
            await record.AwaitProcessing().ConfigureAwait(false);
        }

        public async Task BecomeActiveSecondaryAsync()
        {
            ReplicatorTraceOnApi(this.tracer, "BecomeActiveSecondaryAsync");

            Utility.Assert(
                this.recoveryManager.IsRemovingStateAfterOpen == false,
                "Cannot transition to active secondary because remove state is pending");
            Utility.Assert(
                this.recoveryManager.RecoveryException == null,
                "Cannot transition to active secondary because recovery failed with exception {0}",
                this.recoveryManager.RecoveryException);

            // GopalK: The order of the following statements is significant
            var previousRole = await this.PrepareForReplicaRoleTransitionAsync(ReplicaRole.ActiveSecondary).ConfigureAwait(false);
            if (previousRole != ReplicaRole.IdleSecondary)
            {
                this.secondaryDrainManager.StartBuildingSecondary();
            }
        }

        public async Task BecomeIdleSecondaryAsync()
        {
            ReplicatorTraceOnApi(this.tracer, "BecomeIdleSecondaryAsync");

            Utility.Assert(
                this.recoveryManager.IsRemovingStateAfterOpen == false,
                "Cannot transition to idle because remove state is pending");
            Utility.Assert(
                this.recoveryManager.RecoveryException == null,
                "Cannot transition to idle secondary because recovery failed with exception {0}",
                this.recoveryManager.RecoveryException);

            // GopalK: The order of the following statements is significant
            var previousRole = await this.PrepareForReplicaRoleTransitionAsync(ReplicaRole.IdleSecondary).ConfigureAwait(false);
            if (previousRole != ReplicaRole.ActiveSecondary)
            {
                this.secondaryDrainManager.StartBuildingSecondary();
            }
        }

        public async Task BecomeNoneAsync()
        {
            ReplicatorTraceOnApi(
                this.tracer,
                "BecomeNoneAsync: this.isRemovingStateAfterOpen = " + this.recoveryManager.IsRemovingStateAfterOpen);

            await this.secondaryDrainManager.WaitForCopyAndReplicationDrainToCompleteAsync().ConfigureAwait(false);
            this.checkpointManager.AbortPendingCheckpoint();
            this.checkpointManager.AbortPendingLogHeadTruncation();

            await this.PrepareForReplicaRoleTransitionAsync(ReplicaRole.None).ConfigureAwait(false);

            if (this.recoveryManager.IsRemovingStateAfterOpen == false)
            {
                await this.logManager.FlushAsync("BecomeNoneAsync").ConfigureAwait(false);
                await this.replicatedLogManager.LastInformationRecord.AwaitProcessing().ConfigureAwait(false);
                await this.recordsProcessor.WaitForPhysicalRecordsProcessingAsync().ConfigureAwait(false);
            }

            this.backupManager.DeletePersistedState();
        }

        /// <summary>
        /// Transitions the role of the replicator to Primary.
        /// Called during ChangeRole to Primary by StateManager.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task BecomePrimaryAsync()
        {
            return this.BecomePrimaryAsync(false);
        }

        /// <summary>
        /// Transitions the role of the replicator to Primary.
        /// </summary>
        /// <param name="isForRestore">Is this a Backup Manager initiated Change Role.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public async Task BecomePrimaryAsync(bool isForRestore)
        {
            ReplicatorTraceOnApi(this.tracer, "BecomePrimaryAsync");

            Utility.Assert(
                this.recoveryManager.IsRemovingStateAfterOpen == false,
                "Cannot transition to primary because remove state is pending");
            Utility.Assert(
                this.recoveryManager.RecoveryException == null,
                "Cannot transition to Primary because recovery failed with exception {0}",
                this.recoveryManager.RecoveryException);

            // Dispose recovery stream as we no longer need it
            this.recoveryManager.DisposeRecoveryReadStreamIfNeeded();
            await this.secondaryDrainManager.WaitForCopyAndReplicationDrainToCompleteAsync().ConfigureAwait(false);
            await this.QuiesceReplicaActivityAsync().ConfigureAwait(false);

            var abortingTransactions =
                new List<BeginTransactionOperationLogRecord>();
            foreach (var record in this.transactionManager.TransactionsMap.GetPendingTransactions())
            {
                FabricEvents.Events.AbortTx(
                    this.tracer.Type,
                    "BecomePrimaryAsync: Attempting to Abort Transaction",
                    record.RecordType.ToString(),
                    record.Lsn.LSN,
                    record.Psn.PSN,
                    record.RecordPosition,
                    record.Transaction.Id);

                abortingTransactions.Add(record);
            }

            // MCoskun:
            // Check if this replica is retaining its Primary Status: ChangeRole U->P, UpdateEpoch has not been called and Restore is not in flight.
            // Note that we know UpdateEpoch has not been called yet if the replica has progress higher than the
            // lsn in the last progress ProgressVectorEntry which contains the latest Epoch and highest lsn seen in the previous Epoch.
            // If Update Epoch was called, recovered lsn and highest lsn seen in the previous Epoch would match.
            // Note that during Restore, BecomePrimaryAsync is initiated by BackupManager and not by Reliability.
            if ((this.Role == ReplicaRole.Unknown) && (this.recoveryManager.RecoveredLsn > this.replicatedLogManager.ProgressVector.LastProgressVectorEntry.Lsn)
                && isForRestore == false)
            {
                // MCoskun:
                // There is a future Reliability improvement to not do a reconfiguration when an entire partition restarts.
                // Instead the previous Primary will come up and continue where it left of.
                // Since there is no reconfiguration, that means that UpdateEpoch would not be called.
                // In the absence of UpdateEpoch (which is a barrier), the replica that is retaining its primary status needs to inject its own.
                // PrimaryStatus.Retained is a primary status reserved for a replica becoming primary after failing as a primary without a reconfiguration.
                // Since this feature in reliability is not implemented yet, this code path should never execute.
                Utility.Assert(
                    false,
                    "TEST: Retained: Recovered Lsn: {0} LastProgressVectorEntry: <{1}, {2}> {3}",
                    this.recoveryManager.RecoveredLsn,
                    this.replicatedLogManager.ProgressVector.LastProgressVectorEntry.Epoch.DataLossNumber,
                    this.replicatedLogManager.ProgressVector.LastProgressVectorEntry.Epoch.ConfigurationNumber,
                    this.replicatedLogManager.ProgressVector.LastProgressVectorEntry.Lsn);

                this.primaryStatus = PrimaryElectionStatus.Retained;
            }

            this.primaryStatus = PrimaryElectionStatus.Elected;
            this.roleContextDrainState.ChangeRole(ReplicaRole.Primary);

            this.primaryTransactionsAbortingInstance = (abortingTransactions.Count == 0) ? null : abortingTransactions;
            this.primaryTransactionsAbortOrGroupCommitTaskCompletionSource = new CancellationTokenSource();
            this.primaryTransactionsAbortOrGroupCommitTask =
                this.AbortPendingTransactionsOrAddBarrierOnBecomingPrimary(
                    abortingTransactions,
                    this.primaryTransactionsAbortOrGroupCommitTaskCompletionSource.Token);

            this.backgroundTelemetryCts = new CancellationTokenSource();
            this.backgroundTelemetryTask = this.StartTelemetryTaskAsync(backgroundTelemetryCts.Token);

            lock (this.backupDrainLock)
            {
                this.isBackupDrainablePhaseAllowed = true;
                Utility.Assert(this.isBackupCallbackInflight == false, "false == this.isBackupCallbackInflight");
                Utility.Assert(this.drainBackupCallBackTcs == null, "null == this.drainBackupCallBackTcs");
            }
        }

        public void BeginTransaction(
            Transaction transaction,
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext)
        {
            this.ReplicateBarrierIfPrimaryStatusIsBelowEstablished();
            this.transactionManager.BeginTransaction(
                transaction,
                metaData,
                undo,
                redo,
                operationContext);
        }

        public async Task<long> BeginTransactionAsync(
            Transaction transaction,
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext)
        {
            this.ReplicateBarrierIfPrimaryStatusIsBelowEstablished();
            var lsn = await this.transactionManager.BeginTransactionAsync(
                transaction,
                metaData,
                undo,
                redo,
                operationContext).ConfigureAwait(false);

            this.avgTransactionCommitLatencyCounterWriter.IncrementBy(transaction.CommitLatencyWatch);
            return lsn;
        }

        public async Task CancelTheAbortingOfTransactionsIfNecessaryAsync()
        {
            if (this.primaryTransactionsAbortOrGroupCommitTaskCompletionSource != null)
            {
                this.primaryTransactionsAbortOrGroupCommitTaskCompletionSource.Cancel();
                await this.primaryTransactionsAbortOrGroupCommitTask.ConfigureAwait(false);
                this.primaryTransactionsAbortOrGroupCommitTask = null;
                this.primaryTransactionsAbortOrGroupCommitTaskCompletionSource = null;
            }
            else
            {
                Utility.Assert(
                    this.primaryTransactionsAbortOrGroupCommitTask == null,
                    "If transactionAbortTaskCts is null, transactionAbortTask must be null.");
            }
        }

        public async Task<long> CommitTransactionAsync(Transaction transaction)
        {
            this.ReplicateBarrierIfPrimaryStatusIsBelowEstablished();
            var lsn = await transactionManager.CommitTransactionAsync(transaction);
            this.avgTransactionCommitLatencyCounterWriter.IncrementBy(transaction.CommitLatencyWatch);
            return lsn;
        }

        /// <summary>
        /// Complete a drain-able operation.
        /// </summary>
        public void CompleteDrainableBackupPhase()
        {
            lock (this.backupDrainLock)
            {
                Utility.Assert(
                    this.isBackupCallbackInflight == true,
                    "Backup Callback must be in-flight if this is called.");

                this.isBackupCallbackInflight = false;

                if (this.drainBackupCallBackTcs != null)
                {
                    Utility.Assert(
                        this.isBackupDrainablePhaseAllowed == false,
                        "Backup must not be allowed if draining is initiated.");

                    this.drainBackupCallBackTcs.SetResult(null);
                    this.drainBackupCallBackTcs = null;
                }
            }
        }

        /// <summary>
        /// Delete the log.
        /// </summary>
        public async Task DeleteLogAsync()
        {
            ReplicatorTraceOnApi(this.tracer, "DeleteLog");

            await this.logManager.DeleteLogAsync().ConfigureAwait(false);
        }

        public void Dispose()
        {
            this.Dispose(true);
        }

        public async Task FinishRestoreAsync(Epoch dataLossEpoch)
        {
            // Add the data loss Update Epoch log record into the log and flush it
            await this.PerformUpdateEpochAsync(dataLossEpoch, this.replicatedLogManager.CurrentLogTailLsn.LSN).ConfigureAwait(false);

            // Record that restore from backup completed and reset the replicator LSN
            this.replicatedLogManager.Information(InformationEvent.RestoredFromBackup);
            await this.logManager.FlushAsync("FinishRestoreAsync").ConfigureAwait(false);
            await this.replicatedLogManager.LastInformationRecord.AwaitProcessing().ConfigureAwait(false);
            await this.recordsProcessor.WaitForAllRecordsProcessingAsync().ConfigureAwait(false);
        }

        public IOperationDataStream GetCopyContext()
        {
            ReplicatorTraceOnApi(this.tracer, "GetCopyContext");

            var copyContext =
                new LoggingReplicatorCopyContext(
                    this.initializationParameters.ReplicaId,
                    this.replicatedLogManager.ProgressVector,
                    this.replicatedLogManager.CurrentLogHeadRecord,
                    this.replicatedLogManager.CurrentLogTailLsn,
                    this.recoveryManager.LastRecoveredAtomicRedoOperationLsn);

            return copyContext;
        }

        public IOperationDataStream GetCopyState(long uptoSequenceNumber, IOperationDataStream copyContext)
        {
            ReplicatorTraceOnApi(this.tracer, "GetCopyState UptoLSN: " + uptoSequenceNumber);

            if (copyContext == null)
            {
                var errorMessage = string.Format(
                    CultureInfo.InvariantCulture,
                    SR.Error_NullCopyContext,
                    this.GetType().FullName);

                FabricEvents.Events.Error(this.tracer.Type, errorMessage);

                // This exception need not be localized as we are throwing it to the V1 replicator and not the user
                throw new ArgumentNullException(errorMessage);
            }

            return new LoggingReplicatorCopyStream(this.replicatedLogManager, this.stateManager, this.checkpointManager, this.WaitForLogFlushUptoLsn, this.initializationParameters.ReplicaId, new LogicalSequenceNumber(uptoSequenceNumber), copyContext, this.Tracer);
        }

        public long GetCurrentDataLossNumber()
        {
            return this.replicatedLogManager.CurrentLogTailEpoch.DataLossNumber;
        }

        public Epoch GetEpoch(LogicalSequenceNumber lsn)
        {
            return this.replicatedLogManager.ProgressVector.FindEpoch(lsn);
        }

        public long GetLastCommittedSequenceNumber()
        {
            var lastCommittedLsn = this.replicatedLogManager.CurrentLogTailLsn;

            ReplicatorTraceOnApi(this.tracer, "GetLastCommittedSequenceNumber LSN: " + lastCommittedLsn.LSN);

            return lastCommittedLsn.LSN;
        }

        public long GetLastStableSequenceNumber()
        {
            var lsn = this.LastStableLsn;

            ReplicatorTraceOnApi(this.tracer, "GetLastStableSequenceNumber LSN: " + lsn);

            return lsn.LSN;
        }

        /// <summary>
        /// Gets the physical restoreLogStream for incremental backup.
        /// Note that caller does not hold the lock.
        /// Enumerator returned from the function will have Current equal to the first record that has LSN = previouSequenceNumber.
        /// </summary>
        /// <returns>Physical log restoreLogStream.</returns>
        /// <remarks>
        /// Must move all records since the newest Indexing Log Records that is older than the last completed begin checkpoint and
        /// earliest begin transaction record.
        /// Note that caller does not hold lock.
        /// So the stream can be truncated until creation of physical log reader (and isValid).
        /// </remarks>
        /// <devnote>
        /// Algortihm:
        /// 1. Find the first physical log record that has LSN less than previouSequenceNumber.
        /// 2. Create phsyical log reader.
        /// 3. Get the LogRecords
        /// 4. Move the log records enumerator until Current equal to the first record that has LSN = previouSequenceNumber. 
        /// </devnote>
        public async Task<IAsyncEnumerator<LogRecord>> GetLogRecordsAsync(
            LogicalSequenceNumber previousSequenceNumber,
            string readerName,
            LogManager.LogReaderType logReaderType)
        {
            // Stage 0: Precondition validation.
            LogRecord tailLogRecord = this.replicatedLogManager.LogManager.CurrentLogTailRecord;
            Utility.Assert(LogRecord.IsInvalidRecord(tailLogRecord) == false, "{0} TailRecord is always expected to be valid.", this.tracer.Type);
            Utility.Assert(
                tailLogRecord.Lsn > previousSequenceNumber, 
                "{0} TailRecord must always have higher LSN than the previous backed up LSN. Tail: {1} Last Backed Up LSN: {2}", 
                this.tracer.Type,
                tailLogRecord.Lsn,
                previousSequenceNumber);

            // Stage 1: Find the first physical log record that has LSN < previouSequenceNumber.
            // Highest priority here is to avoid unnecessary IO.
            // So we use the backward physical log record chain (always in-memory) to find the record.

            // Get the starting point for the previous physical log record chain search.
            // Minor optimization: If the last Completed Checkpoint Log Record LSN > previouSequenceNumber, 
            // we can start walking back physical log record chain from the last Completed Checkpoint Log.
            // This is a minor optimization since it only avoids part of the in-memory link list search.
            LogRecord currentLogRecord = this.replicatedLogManager.LastCompletedEndCheckpointRecord;

            // Note that LastCompletedEndCheckpointRecord should never be invalid today but there is no reason to assert this in the incremental backup code.
            if (LogRecord.IsInvalidRecord(currentLogRecord) || currentLogRecord.Lsn < previousSequenceNumber)
            {
                currentLogRecord = tailLogRecord;
            }

            // Use the PreviousPhysicalRecord links to find the first Physical Log Record LSN > previouSequenceNumber.
            // Note that we go back even if they are equal because we do not want to miss the logical or physical log record with the same LSN.
            // TODO: Conisdier optimization:
            // We could potentially switch to ReadPreviousRecordAsync here to go back one at a time 
            // when previousPhysicalRecord.Lsn = or little more than previous sequence number
            while (currentLogRecord.Lsn >= previousSequenceNumber)
            {
                currentLogRecord = currentLogRecord.PreviousPhysicalRecord;
                if (LogRecord.IsInvalidRecord(currentLogRecord))
                {
                    throw new FabricMissingFullBackupException(SR.Error_FullBackupRequired);
                }
            } 

            // Stage 2: Create Physical log reader.
            // Usability: We get all the logs to make sure every log that was created before backup was called is in the backup.
            // Note that we use CurrentLogTailRecord.RecordPosition as the end position which is guaranteed to be flushed.
            // Note that this code depends on this method setting read ahead for performance.
            var physicalLogReader = this.logManager.GetPhysicalLogReader(
                currentLogRecord.RecordPosition,
                tailLogRecord.RecordPosition,
                tailLogRecord.Lsn.LSN,
                readerName,
                LogManager.LogReaderType.Backup);
            if (physicalLogReader.IsValid == false)
            {
                throw new FabricMissingFullBackupException(SR.Error_FullBackupRequired);
            }

            // Stage 3: Get the LogRecords
            IAsyncEnumerator<LogRecord> logRecords = null;
            using (physicalLogReader)
            {
                // Dispose log reader as soon as we have the log reader who will instead hold the truncation.
                logRecords = physicalLogReader.GetLogRecords(readerName, logReaderType);
            }

            // Stage 4: Move the log records enumerator until Current equal to the first record that has LSN = previouSequenceNumber. 
            while (await logRecords.MoveNextAsync(CancellationToken.None).ConfigureAwait(false) == true)
            {
                if (logRecords.Current.Lsn == previousSequenceNumber)
                {
                    return logRecords;
                }

                Utility.Assert(
                    logRecords.Current.Lsn < previousSequenceNumber,
                    "{0} logRecords.Current.Lsn {1} > previouSequenceNumber {2}",
                    this.tracer.Type,
                    logRecords.Current.Lsn,
                    previousSequenceNumber);
            }

            Utility.Assert(false, "{0}: Code cannot come here.", this.tracer.Type);
            return null;
        }

        /// <summary>
        /// Gets the physical restoreLogStream for backup
        /// </summary>
        /// <returns>Physical log restoreLogStream.</returns>
        /// <remarks>
        /// Must move all records since the newest Indexing Log Records that is older than the last completed begin checkpoint and
        /// earliest begin transaction record.
        /// </remarks>
        public IAsyncEnumerator<LogRecord> GetLogRecordsCallerHoldsLock(
            IndexingLogRecord previousIndexingRecord,
            string readerName,
            LogManager.LogReaderType logReaderType)
        {
            Utility.Assert(
                previousIndexingRecord != null,
                "Previous Indexing Record to start the backup from must not be null");

            var startingRecordPosition = previousIndexingRecord.RecordPosition;

            // Tail log record must be snapped to avoid races since it is not under a lock.
            var tailLogRecord = this.logManager.CurrentLogTailRecord;

            // Usability: We get all the logs to make sure every log that was created before backup was called is in the backup.
            // Note that we use CurrentLogTailRecord.RecordPosition as the end position which is guaranteed to be flushed.
            var physicalLogReader = this.logManager.GetPhysicalLogReader(
                startingRecordPosition,
                tailLogRecord.RecordPosition,
                tailLogRecord.Lsn.LSN,
                readerName,
                LogManager.LogReaderType.Backup);

            Utility.Assert(
                physicalLogReader.IsValid == true,
                "Due to caller lock, startingRecordPosition {0} could not have been truncated.",
                startingRecordPosition);

            using (physicalLogReader)
            {
                return physicalLogReader.GetLogRecords(readerName, logReaderType);
            }
        }

        public long GetSafeLsnToRemoveStateProvider()
        {
            long returnLsn = 0;

            var earliestLogRecord = this.replicatedLogManager.GetEarliestRecord();
            returnLsn = (earliestLogRecord == null) ? 0 : earliestLogRecord.Lsn.LSN;

            var earliestFullCopyLogStartingLsn = this.logManager.GetEarliestFullCopyStartingLsn();

            // If there was a full copy, return the LSN of the copy if it is smaller than the earliest record lsn
            if (earliestFullCopyLogStartingLsn < returnLsn)
            {
                returnLsn = earliestFullCopyLogStartingLsn;
            }

            return returnLsn;
        }

        public void Initialize(StatefulServiceContext parameters, IStatefulServicePartition partition)
        {
            parameters.ThrowIfNull("parameters");
            partition.ThrowIfNull("partition");

            this.initializationParameters = parameters;
            this.partition = partition;

            this.fabricReplicator = null;
            this.replicatorSettings = null;
            this.tracer = null;

            // Do not move the below members into Reuse
            // Reuse is invoked during dataloss processing when the replicator is internally closed and re-opened.
            // The below members are still to be kept valid, without which extremely rare race conditions end up in an unexpected exception and
            // it is not worth the effort in fixing them

            // Keeping the below fields un-touched during the 'Reuse' is not harmful since they all are accessed by threads that are completing
            // their processing of log records that have already marked the signal 'FinishedProcessing'.
            // However, the master loop (while loop in ProccessLoggedRecords) still accesses the below members before terminating the loop
            this.logRecordsDispatcher = new LogRecordsDispatcher(this.recordsProcessor);
            this.apiMonitor = new ReplicatorApiMonitor();
            this.backupManager = new BackupManager(this.initializationParameters, this, this.stateManager, this.apiMonitor);
            this.recoveryManager = new RecoveryManager(FlushedRecordsCallback);

            this.Reuse();
        }

        long IVersionProvider.GetVersion()
        {
            // Snap the last applied barrier's lsn.
            var version = this.recordsProcessor.LastAppliedBarrierRecord.Lsn.LSN;

            Utility.Assert(version > 0, "Must be a valid version");

            return version;
        }

        /// <summary>
        /// Called at the end of on data loss.
        /// </summary>
        /// <param name="cancellationToken">Used to signal cancellation.</param>
        /// <param name="restoredFromBackup">Has the replica restored from backup.</param>
        /// <returns></returns>
        public Task<bool> OnDataLossAsync(CancellationToken cancellationToken, bool restoredFromBackup)
        {
            var trace = "OnDataLossAsync: Completed Recovery. Backup-Restored: " + restoredFromBackup
                        + "DataLossNumber " + this.replicatedLogManager.CurrentLogTailEpoch.DataLossNumber + "ConfigurationNumber "
                        + this.replicatedLogManager.CurrentLogTailEpoch.ConfigurationNumber + "Lsn " + this.replicatedLogManager.CurrentLogTailLsn;

            ReplicatorTraceOnApi(this.tracer, trace);
            return Task.FromResult(restoredFromBackup);
        }

        /// <summary>
        /// Open for the replicator.
        /// </summary>
        /// <param name="openMode">The open mode.</param>
        /// <param name="inputReplicatorSettings">Replicator settings.</param>
        /// <returns>Recovery information.</returns>
        public async Task<RecoveryInformation> OpenAsync(
            ReplicaOpenMode openMode,
            TransactionalReplicatorSettings inputReplicatorSettings)
        {
            this.InitializePerformanceCounters();
            return await this.OpenAsync(openMode, inputReplicatorSettings, null).ConfigureAwait(false);
        }

        /// <summary>
        /// Opens the replication with initial set of logs.
        /// </summary>
        /// <param name="openMode">The open mode.</param>
        /// <param name="inputReplicatorSettings">Replicator settings.</param>
        /// <param name="restoreLogPathList">Replicator logs to be restored. Must be in backup order.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public async Task<RecoveryInformation> OpenAsync(
            ReplicaOpenMode openMode,
            TransactionalReplicatorSettings inputReplicatorSettings,
            IList<string> restoreLogPathList)
        {
            this.logRecoveryPerformanceCounterWriter.StartMeasurement();
            this.replicatorSettings = inputReplicatorSettings;

            var shouldLocalStateBeRemoved = this.backupManager.ShouldCleanState();

            Utility.Assert(
                restoreLogPathList == null || shouldLocalStateBeRemoved == false,
                "If restoreStream {0} is not currently ongoing, backupManager should initiate state removal {1}.",
                restoreLogPathList, shouldLocalStateBeRemoved);

            var trace = "OpenAsync: Open Mode: " + openMode + " ShouldLocalStateBeRemoved: " + shouldLocalStateBeRemoved
                        + " RestoreLogExists: " + (restoreLogPathList != null) + " " + Environment.NewLine + "Settings:"
                        + inputReplicatorSettings;

            ReplicatorTraceOnApi(this.tracer, trace);

            ReliableStateManagerReplicatorSettingsUtil.ValidateSettings(inputReplicatorSettings);
            this.replicatorSettings = inputReplicatorSettings;

            try
            {
                Guid.Parse(this.replicatorSettings.PublicSettings.SharedLogId);
            }
            catch (FormatException)
            {
                throw new InvalidOperationException(
                    string.Format(CultureInfo.CurrentCulture, SR.Error_InvalidV2ReplSetting_1Arg, "SharedLogId"));
            }

            LogManagerInitialization init = new LogManagerInitialization(
                this.logDirectory ?? ServiceReplicaUtils.GetDedicatedLogPath(this.InitializationParameters),
                this.initializationParameters.CodePackageActivationContext.WorkDirectory,
                this.initializationParameters.PartitionId,
                this.initializationParameters.ReplicaId);

            this.CreateAllManagers(this.replicatorSettings);

            await this.logManager.InitializeAsync(
                this.tracer,
                init,
                this.replicatorSettings).ConfigureAwait(false);

            // Delete the log and re-initialize the logging replicator if the restore token exists.
            if (shouldLocalStateBeRemoved == true)
            {
                ReplicatorTraceOnApi(this.tracer, "OpenAsync: Executing incomplete restore clean up.");

                await this.logManager.DeleteLogAsync().ConfigureAwait(false);
                this.Reuse();
                this.CreateAllManagers(this.replicatorSettings);
                await this.logManager.InitializeAsync(
                    this.tracer,
                    init,
                    this.replicatorSettings).ConfigureAwait(false);
            }

            this.apiMonitor.Initialize(this.initializationParameters, this.partition, this.replicatorSettings);

            PhysicalLogReader recoveryLogsReader = null;
            if (restoreLogPathList == null)
            {
                recoveryLogsReader = await logManager.OpenAsync(openMode).ConfigureAwait(false);
            }
            else
            {
                recoveryLogsReader = await logManager.OpenWithRestoreFileAsync(
                    openMode,
                    this.replicatorCounterSetInstance,
                    restoreLogPathList,
                    (int)replicatorSettings.PublicSettings.MaxRecordSizeInKB).ConfigureAwait(false);
            }

            var info = await this.recoveryManager.OpenAsync(
                openMode,
                this.replicatorSettings,
                this.tracer,
                this.logManager,
                shouldLocalStateBeRemoved,
                recoveryLogsReader,
                restoreLogPathList != null);

            this.logRecoveryPerformanceCounterWriter.PauseMeasurement();
            // Start measuring state provider recovery
            this.stateManagerRecoveryPerformanceCounterWriter.StartMeasurement();

            return info;
        }

        /// <summary>
        /// Performs recovery.
        /// </summary>
        public async Task PerformRecoveryAsync(RecoveryInformation recoveryInformation)
        {
            ReplicatorTraceOnApi(this.tracer, "PerformRecoveryAsync");
            // Finish measuring state provider recovery
            this.stateManagerRecoveryPerformanceCounterWriter.StopMeasurement();

            this.roleContextDrainState.OnRecovery();

            if (recoveryInformation.ShouldRemoveLocalStateDueToIncompleteRestore == true)
            {
                try
                {
                    this.backupManager.StateCleaningComplete();
                }
                catch (Exception e)
                {
                    Utility.Assert(false, "Unexpected Exception: " + e.ToString());
                }
            }

            // Start measuring recovery time
            this.logRecoveryPerformanceCounterWriter.ContinueMeasurement();

            await recoveryManager.PerformRecoveryAsync(
                this.recoveredOrCopiedCheckpointLsn,
                this.recordsProcessor,
                this.checkpointManager,
                this.transactionManager,
                this.logRecordsDispatcher,
                this.replicatedLogManager);

            // Finish measuring recovery time
            this.logRecoveryPerformanceCounterWriter.StopMeasurement();

            this.secondaryDrainManager.OnSuccessfulRecovery(this.recoveryManager.RecoveredLsn);
            await this.stateManager.OnRecoveryCompletedAsync().ConfigureAwait(false);
            this.roleContextDrainState.OnRecoveryCompleted();

            Utility.Assert(
                this.recoveryManager.RecoveryException == null,
                "Recovery exception must be null. It is {0}",
                this.recoveryManager.RecoveryException);

            return;
        }

        public async Task PrepareForRestore(Guid restoreId)
        {
            await this.logManager.DeleteLogAsync().ConfigureAwait(false);

            // To ensure Replicator LSN is reset to 0
            this.Reuse();
        }

        public async Task PrepareToCloseAsync()
        {
            var traceMessage = string.Format(
                "PrepareToCloseAsync: isRemovingStateAfterOpen: {0} isLogUnavailableDueToRestore: {1}",
                this.recoveryManager.IsRemovingStateAfterOpen,
                this.backupManager.IsLogUnavailableDueToRestore);
            ReplicatorTraceOnApi(this.tracer, traceMessage);

            // Do not log any more after changing role to none as the log file could have been deleted
            if (this.Role != ReplicaRole.None && this.recoveryManager.IsRemovingStateAfterOpen == false && this.backupManager.IsLogUnavailableDueToRestore == false)
            {
                await this.secondaryDrainManager.WaitForCopyAndReplicationDrainToCompleteAsync().ConfigureAwait(false);

                this.checkpointManager.AbortPendingCheckpoint();
                this.checkpointManager.AbortPendingLogHeadTruncation();

                await this.PrepareForReplicaRoleTransitionAsync(ReplicaRole.Unknown).ConfigureAwait(false);
                await this.logManager.FlushAsync("PrepareToCloseAsync").ConfigureAwait(false);
                await this.replicatedLogManager.LastInformationRecord.AwaitProcessing().ConfigureAwait(false);
                await this.recordsProcessor.WaitForPhysicalRecordsProcessingAsync().ConfigureAwait(false);

                // Log can only be closed all incoming operations have been stopped (PrepareToClose) & all of the current logs have been processed.
                await this.logManager.CloseLogAsync(CancellationToken.None).ConfigureAwait(false);
            }
        }

        public async Task RecoverRestoredDataAsync(Guid restoreId, IList<string> backupLogPathList)
        {
            FabricEvents.Events.RestoreAsync(
                this.tracer.Type,
                restoreId,
                this.GetRestoreBackupLogListTrace(backupLogPathList));

            // Re-create new logs and perform recovery
            var recoveryData = await this.OpenAsync(ReplicaOpenMode.Existing, this.replicatorSettings, backupLogPathList).ConfigureAwait(false);

            if (recoveryData.ShouldSkipRecoveryDueToIncompleteChangeRoleNone)
            {
                FabricEvents.Events.Error(
                    this.tracer.Type,
                    "Replicator Log cannot be restored as it was in the process of removing state on the state providers -> Changing Role to None");

                // TODO: Consider Assert.
                throw new InvalidDataException(SR.Error_V2ReplIncompleteChangeRoleToNone);
            }

            if (this.TestFlagThrowExceptionAtRestoreData == true)
            {
                throw new ArgumentException("Test Exception. Production code should never throw this exception.");
            }

            await this.PerformRecoveryAsync(recoveryData).ConfigureAwait(false);
        }

        public async Task<long> RegisterAsync()
        {
            this.inflightVisibilitySequeneceNumberCountCounterWriter.Increment();
            long visibilitySequenceNumber = await this.versionManager.RegisterAsync().ConfigureAwait(false);

            // 6450429: If the read stuck false progressed Secondary becomes Primary and the versioning values are not corrected as part of the truncate,
            // This assert will fail the process instead of giving inconsistent data.
            Utility.Assert(
                visibilitySequenceNumber <= this.CurrentLogTailLsn.LSN,
                "Visibility Sequence Number {0} must be less than or equal to current tail lsn {1}.",
                visibilitySequenceNumber, this.CurrentLogTailLsn.LSN);

            return visibilitySequenceNumber;
        }

        public void ReleaseBackupAndCopyConsistencyLock(string lockReleaser)
        {
            this.checkpointManager.ReleaseBackupAndCopyConsistencyLock(lockReleaser);
        }

        public void ReleaseStateManagerApiLock(string lockReleaser)
        {
            this.checkpointManager.ReleaseStateManagerApiLock(lockReleaser);
        }

        public async Task ReplicateAndAwaitBarrierLogRecordAsync(Guid backupId)
        {
            var barrierRecord = this.checkpointManager.ReplicateBarrier();
            await this.logManager.FlushAsync(backupId.ToString()).ConfigureAwait(false);

            await barrierRecord.AwaitProcessing().ConfigureAwait(false);
        }

        public async Task ReplicateBackupLogRecordAsync(Guid backupId, ReplicatorBackup replicatorBackup)
        {
            BackupLogRecord backupLogRecord;

            this.ThrowIfNoWriteStatus();

            backupLogRecord = new BackupLogRecord(
                backupId,
                replicatorBackup.EpochOfHighestBackedUpLogRecord,
                replicatorBackup.LsnOfHighestBackedUpLogRecord,
                replicatorBackup.LogCount,
                replicatorBackup.AccumulatedLogSizeInKB);

            // TODO: Can throw FabricTransientException.
            // MCoskun: Order of the following operations is important.
            // First, we need to make sure backupLogRecord is before the barrier log record that is inserted below
            // Second, we need to insert a barrier log record, flush and await replication.
            // Third, we need to await the replication and flush of backuplogRecord (can complete out of order) and call unlock.
            this.replicatedLogManager.ReplicateAndLog(backupLogRecord, this.transactionManager);

            Exception exception = null;

            // unlock must be called even if barrier replication failed. Otherwise close can get stuck.
            try
            {
                await this.ReplicateAndAwaitBarrierLogRecordAsync(backupId).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                exception = e;
            }

            await this.AwaitApplyReplicateAndCallUnlock(backupLogRecord).ConfigureAwait(false);

            if (exception != null)
            {
                throw exception;
            }

            Utility.Assert(backupLogRecord.IsReplicated, "BackupLogRecord must have been replicated");
        }

        /// <summary>
        /// Restores the transactional replicator from the backup folder.
        /// The default restorePolicy is RestorePolicy.Safe.
        /// The default cancellationToken is None.
        /// </summary>
        /// <param name="backupFolder">
        /// The directory where the replica is to be restored from.
        /// This parameter cannot be null, empty or contain just whitespace.
        /// UNC paths may also be provided.
        /// </param>
        /// <returns>Task that represents the asynchronous restore.</returns>
        public Task RestoreAsync(string backupFolder)
        {
            return this.backupManager.RestoreAsync(backupFolder);
        }

        /// <summary>
        /// Restores the transactional replicator from the backup folder.
        /// </summary>
        /// <param name="backupDirectory">
        /// The directory where the replica is to be restored from.
        /// This parameter cannot be null, empty or contain just whitespace.
        /// UNC paths may also be provided.
        /// </param>
        /// <param name="restorePolicy">The restore policy.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represents the asynchronous restore.</returns>
        public Task RestoreAsync(
            string backupDirectory,
            RestorePolicy restorePolicy,
            CancellationToken cancellationToken)
        {
            return this.backupManager.RestoreAsync(backupDirectory, restorePolicy, cancellationToken);
        }

        /// <summary>
        /// Start a drain-able operation.
        /// </summary>
        public void StartDrainableBackupPhase()
        {
            lock (this.backupDrainLock)
            {
                if (this.isBackupDrainablePhaseAllowed == false)
                {
                    throw new FabricNotPrimaryException();
                }

                Utility.Assert(this.isBackupCallbackInflight == false, "false == this.isBackupCallbackInflight");

                // TODO: rename to backup inflight
                this.isBackupCallbackInflight = true;
            }
        }

        public void ThrowIfNoWriteStatus()
        {
            if (this.roleContextDrainState.IsClosing == true)
            {
                throw new FabricNotPrimaryException();
            }

            if (this.Role != ReplicaRole.Primary || this.primaryStatus < PrimaryElectionStatus.Retained)
            {
                throw new FabricNotPrimaryException(FabricErrorCode.NotPrimary);
            }
        }

        public Task TryRemoveCheckpointAsync(long checkpointLsnToBeRemoved, long nextcheckpointLsn)
        {
            return ((IVersionManager)this.versionManager).TryRemoveCheckpointAsync(
                checkpointLsnToBeRemoved,
                nextcheckpointLsn);
        }

        public TryRemoveVersionResult TryRemoveVersion(
            long stateProviderId,
            long commitSequenceNumber,
            long nextCommitSequenceNumber)
        {
            return ((IVersionManager)this.versionManager).TryRemoveVersion(
                stateProviderId,
                commitSequenceNumber,
                nextCommitSequenceNumber);
        }

        public void UnRegister(long visibilitySequenceNumber)
        {
            this.inflightVisibilitySequeneceNumberCountCounterWriter.Decrement();
            this.versionManager.UnRegister(visibilitySequenceNumber);
        }

        public Task UpdateEpochAsync(
            Epoch epoch,
            long previousEpochLastSequenceNumber,
            CancellationToken cancellationToken)
        {
            return this.PerformUpdateEpochAsync(epoch, previousEpochLastSequenceNumber);
        }

        public void UpdateReplicatorSettings(ReliableStateManagerReplicatorSettings updatedSettings)
        {
            if (updatedSettings == null)
            {
                throw new ArgumentNullException(
                    string.Format(CultureInfo.CurrentCulture, SR.Error_NullArgument_Formatted, "updatedSettings"));
            }

            if (this.replicatorSettings.PublicSettings.Equals(updatedSettings))
            {
                this.FabricReplicator.UpdateReplicatorSettings(ReliableStateManagerReplicatorSettingsUtil.ToReplicatorSettings(updatedSettings));
            }
            else
            {
                FabricEvents.Events.UpdatedReplicatorSettings(
                    this.tracer.Type,
                    this.replicatorSettings.ToString(),
                    updatedSettings.ToString());

                throw new ArgumentException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        SR.Error_ArgumentInvalid_Formatted_arg1,
                        "updatedSettings"));
            }
        }

        internal static bool IsBarrierRecord(LogRecord record)
        {
            bool isBarrier;

            switch (record.RecordType)
            {
                case LogRecordType.Barrier:
                case LogRecordType.BeginCheckpoint:
                case LogRecordType.UpdateEpoch:
                    isBarrier = true;
                    break;

                case LogRecordType.Information:
                    var informationRecord = (InformationLogRecord)record;
                    isBarrier = informationRecord.IsBarrierRecord;
                    break;

                default:
                    isBarrier = false;
                    break;
            }

            return isBarrier;
        }

        internal Task AcquireBackupAndCopyConsistencyLockAsync(string lockTaker)
        {
            return this.checkpointManager.AcquireBackupAndCopyConsistencyLockAsync(lockTaker);
        }

        internal CopyMode GetLogRecordsToCopy(
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
            return this.checkpointManager.GetLogRecordsToCopy(
                targetProgressVector,
                targetLogHeadEpoch,
                targetLogHeadLsn,
                currentTargetLsn,
                latestrecoveredAtomicRedoOperationLsn,
                targetReplicaId,
                out sourceStartingLsn,
                out targetStartingLsn,
                out logRecordsToCopy,
                out completedBeginCheckpointRecord);
        }

        internal void PrepareToTransitionOutOfPrimary(bool isSwapping)
        {
            // Release locks held by pending transactions as we are becoming a secondary
            // Pending transaction will be aborted by the new primary
            ReplicatorTraceOnApi(this.tracer, "PerpareToTransitionOutOfPrimary");

            foreach (var pendingRecord in this.transactionManager.TransactionsMap.GetPendingRecords())
            {
                var record = pendingRecord;
                do
                {
                    var operationRecord = record as OperationLogRecord;
                    if ((operationRecord != null) && (operationRecord.IsOperationContextPresent == true))
                    {
                        var operationContext = operationRecord.ResetOperationContext();
                        if (operationContext != null)
                        {
                            var trace = "PrepareToTransitionOutOfPrimary: Unlocking operation" + this.roleContextDrainState.DrainingStream
                                        + " Unlocking operation: Type: " + operationRecord.RecordType + " LSN: "
                                        + operationRecord.Lsn.LSN + " PSN: " + operationRecord.Psn.PSN + " Position: "
                                        + operationRecord.RecordPosition + " Tx: " + operationRecord.Transaction.Id;

                            ReplicatorTraceOnApi(this.tracer, trace);
                            this.stateManager.Unlock(operationContext);
                        }
                    }

                    record = record.ParentTransactionRecord;
                } while (record != null);
            }

            this.primaryStatus = isSwapping ?
                PrimaryElectionStatus.SwappingOut :
                PrimaryElectionStatus.None;
        }

        internal async Task WaitForLogFlushUptoLsn(LogicalSequenceNumber uptoLsn)
        {
            var isWaiting = uptoLsn > this.logManager.CurrentLogTailRecord.Lsn;
            TaskCompletionSource<object> tcs = null;

            if (isWaiting == true)
            {
                lock (this.flushWaitLock)
                {
                    isWaiting = uptoLsn > this.logManager.CurrentLogTailRecord.Lsn;
                    if (isWaiting == true)
                    {
                        int i;
                        for (i = 0; i < this.waitingStreams.Count; i++)
                        {
                            if (uptoLsn < this.waitingStreams[i].Value)
                            {
                                break;
                            }
                        }

                        tcs = new TaskCompletionSource<object>();
                        this.waitingStreams.Insert(i, new KeyValuePair<TaskCompletionSource<object>, LogicalSequenceNumber>(tcs, uptoLsn));
                    }
                }

                if (isWaiting == true)
                {
                    FabricEvents.Events.WaitForLogFlushUptoLsn(
                        this.tracer.Type,
                        "Awaiting flush",
                        uptoLsn.LSN,
                        this.logManager.CurrentLogTailRecord.Lsn.LSN);

                    await this.logManager.FlushAsync("WaitForLogFlushUptoLsn").ConfigureAwait(false);
                    await tcs.Task.ConfigureAwait(false);

                    FabricEvents.Events.WaitForLogFlushUptoLsn(
                        this.tracer.Type,
                        "Awaited flush",
                        uptoLsn.LSN,
                        this.logManager.CurrentLogTailRecord.Lsn.LSN);
                }
            }
        }

        private static FabricPerformanceCounterSet InitializePerformanceCounterDefinition()
        {
            var performanceCounters = new TransactionalReplicatorPerformanceCounters();
            var counterSets = performanceCounters.GetCounterSets();
            Utility.Assert(counterSets.Keys.Count == 1, "More than 1 category of perf counters found in replicator");
            return new FabricPerformanceCounterSet(counterSets.Keys.First(), counterSets[counterSets.Keys.First()]);
        }

        /// <summary>
        /// Abort the given pending transactions
        /// </summary>
        /// <param name="abortingTransactions">Transactions to be aborted.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        private async Task AbortPendingTransactionsOrAddBarrierOnBecomingPrimary(
            IReadOnlyList<BeginTransactionOperationLogRecord> abortingTransactions,
            CancellationToken cancellationToken)
        {
            Utility.Assert(abortingTransactions != null, "Aborting Transactions object should never be null");

            if (abortingTransactions.Count > 0)
            {
                this.abortDelay = ReplicatorDynamicConstants.AbortDelay;

                // Abort pending transactions
                if (this.primaryTransactionsAbortingInstance == abortingTransactions)
                {
                    await Task.Delay(this.abortDelay).ConfigureAwait(false);

                    // Also number of aborted transactions
                    var index = 0;

                    while (index < abortingTransactions.Count)
                    {
                        var beginRecord = abortingTransactions[index];

                        try
                        {
                            var endRecord = await this.transactionManager.EndTransactionAsync(beginRecord.Transaction, false, false).ConfigureAwait(false);

                            Utility.Assert(
                                endRecord.IsEnlistedTransaction == true,
                                "endRecord.IsEnlistedTransaction == true");

                            FabricEvents.Events.AbortTx(
                                this.tracer.Type,
                                "AbortPendingTransactionsOrAddBarrierOnBecomingPrimary: Transaction aborted",
                                endRecord.RecordType.ToString(),
                                endRecord.Lsn.LSN,
                                endRecord.Psn.PSN,
                                endRecord.RecordPosition,
                                endRecord.Transaction.Id);

                            // transaction aborted successfully.
                            index++;

                            // go to the next transaction.
                            continue;
                        }
                        catch (Exception)
                        {
                            // Do nothing here and silently retry
                        }

                        // If still Primary, exponentially back off.
                        if (this.Role == ReplicaRole.Primary && this.roleContextDrainState.IsClosing == false
                            && cancellationToken.IsCancellationRequested == false)
                        {
                            this.abortDelay *= ReplicatorDynamicConstants.AbortBackoffFactor;

                            if (this.abortDelay > ReplicatorDynamicConstants.MaxAbortDelay)
                            {
                                this.abortDelay = ReplicatorDynamicConstants.MaxAbortDelay;
                            }

                            await Task.Delay(this.abortDelay).ConfigureAwait(false);
                        }
                        else
                        {
                            // If this replica is no longer primary or is closing or we are being asked to cancel, stop retrying
                            for (var i = index; i < abortingTransactions.Count; i++)
                            {
                                var failedToAbort = abortingTransactions[i];

                                FabricEvents.Events.AbortTx(
                                    this.tracer.Type,
                                    "AbortPendingTransactionsOrAddBarrierOnBecomingPrimary: Failed to Abort Transaction",
                                    failedToAbort.RecordType.ToString(),
                                    failedToAbort.Lsn.LSN,
                                    failedToAbort.Psn.PSN,
                                    failedToAbort.RecordPosition,
                                    failedToAbort.Transaction.Id);
                            }

                            // break out of the while loop
                            break;
                        }
                    }
                }
            }
            else
            {
                // Try inserting a barrier
                while (true)
                {
                    try
                    {
                        var barrierRecord = ReplicateBarrierIfPrimaryStatusIsBelowEstablished();

                        if (barrierRecord != null)
                        {
                            await barrierRecord.AwaitProcessing().ConfigureAwait(false);
                        }
                        else
                        {
                            // Nothing to do as a racing thread was able to replicate the barrier
                        }

                        break;
                    }
                    catch (Exception)
                    {
                        // silently ignore and retry
                    }

                    // If still Primary and not requested cancellation, exponentially back off.
                    if (this.Role == ReplicaRole.Primary && this.roleContextDrainState.IsClosing == false
                        && cancellationToken.IsCancellationRequested == false)
                    {
                        this.abortDelay *= ReplicatorDynamicConstants.AbortBackoffFactor;

                        if (this.abortDelay > ReplicatorDynamicConstants.MaxAbortDelay)
                        {
                            this.abortDelay = ReplicatorDynamicConstants.MaxAbortDelay;
                        }

                        await Task.Delay(this.abortDelay).ConfigureAwait(false);
                    }
                    else
                    {
                        FabricEvents.Events.Warning(
                            this.tracer.Type,
                            "AbortPendingTransactionsOrAddBarrierOnBecomingPrimary: Failed to insert barrier");

                        // break out of the while loop
                        break;
                    }
                }
            }
        }

        private async Task AwaitApplyReplicateAndCallUnlock(LogicalLogRecord record)
        {
            try
            {
                // Wait for the record to be flushed and replicated
                await Task.WhenAll(record.AwaitApply(), AwaitReplication(record, this.replicatedLogManager)).ConfigureAwait(false);
            }
            catch (Exception)
            {
                // Do nothing as we have already accepted the replication. Only option is to fault replica and go down (If replica is not already closing)
            }
            finally
            {
                this.recordsProcessor.Unlock(record);
            }
        }

        /// <summary>
        /// Drain the backup callback if necessary.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        private async Task AwaitBackupCallbackOnPrimaryAsync()
        {
            var role = this.Role;
            Utility.Assert(role == ReplicaRole.Primary, "this.recordsProcessor.ReplicaRole {0} == ReplicaRole.Primary", role);

            Task drainBackupCallbackTask = null;

            lock (this.backupDrainLock)
            {
                this.isBackupDrainablePhaseAllowed = false;
                if (this.isBackupCallbackInflight == true)
                {
                    Utility.Assert(this.drainBackupCallBackTcs == null, "this.drainBackupCallBackTcs must be null");
                    this.drainBackupCallBackTcs = new TaskCompletionSource<object>();
                    drainBackupCallbackTask = this.drainBackupCallBackTcs.Task;
                }
            }

            if (drainBackupCallbackTask != null)
            {
                await drainBackupCallbackTask.ConfigureAwait(false);
            }
        }

        private async Task StartTelemetryTaskAsync(CancellationToken cancellationToken)
        {
            long lastCommitCount = 0;

            // MCoskun: Do not dispose. If the replica becomes Primary again, it will be used.
            FabricPerformanceCounter counter;
            try
            {
                counter = this.replicatorCounterSetInstance.GetPerformanceCounter(
                    TransactionalReplicatorPerformanceCounters.CommitTransactionRateCounterName);
            }
            catch (ArgumentException e)
            {
                // Indicates that the counter has not been initialized.
                // Though this is not expected, we want to avoid failing the replica/process because of this.
                FabricEvents.Events.Warning(
                    this.tracer.Type,
                    "Telemetry not started due to ArgumentException.\n " + e);
                return;
            }

            try
            {
                lastCommitCount = counter.GetValue();

                while (cancellationToken.IsCancellationRequested == false)
                {
                    await Task.Delay(this.BackgroundTelemetryPeriod, cancellationToken);

                    // Sample the counter.
                    long sampleCommitCount = counter.GetValue();
                    double average = CalculateAverageRate(lastCommitCount, sampleCommitCount, this.BackgroundTelemetryPeriod.TotalMilliseconds);

                    FabricEvents.Events.Telemetry(
                        this.initializationParameters.PartitionId,
                        this.initializationParameters.ReplicaId,
                        average,
                        sampleCommitCount);

                    lastCommitCount = sampleCommitCount;
                }
            }
            catch (ArgumentOutOfRangeException e)
            {
                Utility.Assert(
                    false,
                    "{0}: Telemetry threw argument exception.\n {1}",
                    this.tracer.Type, e);
            }
            catch (TaskCanceledException e)
            {
                // Indicates that it has been cancelled.
                FabricEvents.Events.Warning(
                    this.tracer.Type,
                    "Telemetry stopped due to cancellation.\n " + e);
                return;
            }
            catch (Exception e)
            {
                // Indicates that there is a bug in the telemetry code path.
                FabricEvents.Events.Warning(
                    this.tracer.Type,
                    "Telemetry stopped due to unexpected exception.\n " + e);
                throw;
            }
        }

        private double CalculateAverageRate(long previousValue, long currentValue, double intervalInMs)
        {
            Utility.Assert(
                currentValue >= 0 && currentValue >= previousValue,
                "{0}: Commit count must be >= 0 && >= previous count. Previous: {1} Current: {2}",
                this.tracer.Type, currentValue, previousValue);

            long difference = checked(currentValue - previousValue);
            Utility.Assert(
                difference >= 0,
                "{0}: Commit count difference since last sample must >= 0. Previous: {1} Current: {2} Difference: {3}",
                this.tracer.Type, currentValue, previousValue, difference);

            // Avoid divide by zero.
            double average = 0;
            if (difference != 0)
            {
                average = (double)difference / this.BackgroundTelemetryPeriod.TotalSeconds;
            }

            return average;
        }

        private async Task CancelAndDrainTelemtryTaskAsync()
        {
            this.backgroundTelemetryCts.Cancel();

            try
            {
                await this.backgroundTelemetryTask.ConfigureAwait(false);
            }
            catch (Exception e)
            {
                // No exception is expected here since the telemetry handles the exceptions.
                // However, we still do not want to bring the process down due to a telemetry issue.
                // So, trace the issue and ignore the exception.
                FabricEvents.Events.Warning(
                    this.tracer.Type,
                    "Telemetry threw unexpected exception .\n" + e);
            }

            this.backgroundTelemetryTask = null;
            this.backgroundTelemetryCts = null;
        }

        private void CreateAllManagers(TransactionalReplicatorSettings inputReplicatorSettings)
        {
            var baseFileName = string.Format(CultureInfo.InvariantCulture, "Log_{0}_{1}", this.initializationParameters.PartitionId, this.initializationParameters.ReplicaId);

            if (!string.IsNullOrEmpty(inputReplicatorSettings.Test_LoggingEngine))
            {
                Utility.Assert(Enum.TryParse(inputReplicatorSettings.Test_LoggingEngine, true, out this.loggerEngineMode),
                    "Unknown logger engine {0}", inputReplicatorSettings.Test_LoggingEngine);
            }
            else
            {
                this.loggerEngineMode = LoggerEngine.Ktl;
            }

            switch (this.loggerEngineMode)
            {
                case LoggerEngine.File:
                    this.logManager = new FileLogManager(
                        baseFileName,
                        this.incomingBytesRateCounterWriter,
                        this.logFlushBytesRateCounterWriter,
                        this.bytesPerFlushCounterWriter,
                        this.avgFlushLatencyCounterWriter,
                        this.avgSerializationLatencyCounterWriter,
                        inputReplicatorSettings.LogWriteFaultInjectionParameters);
                    break;

                case LoggerEngine.Ktl:
                    this.logManager = new KtlLogManager(
                        baseFileName,
                        this.incomingBytesRateCounterWriter,
                        this.logFlushBytesRateCounterWriter,
                        this.bytesPerFlushCounterWriter,
                        this.avgFlushLatencyCounterWriter,
                        this.avgSerializationLatencyCounterWriter,
                        inputReplicatorSettings.LogWriteFaultInjectionParameters);
                    break;

                case LoggerEngine.Memory:
                    this.logManager = new MemoryLogManager(
                        baseFileName,
                        this.incomingBytesRateCounterWriter,
                        this.logFlushBytesRateCounterWriter,
                        this.bytesPerFlushCounterWriter,
                        this.avgFlushLatencyCounterWriter,
                        this.avgSerializationLatencyCounterWriter);
                    break;

                default:
                    Utility.Assert(false, "Unknown logger engine {0}", this.loggerEngineMode);
                    break;
            }

            var txMap = new TransactionMap();

            this.replicatedLogManager = new ReplicatedLogManager(this.logManager, this.recoveredOrCopiedCheckpointLsn);

            int logTruncationIntervalSeconds = 0;

#if !DotNetCoreClr
            // 12529905 - Disable new configuration for LogTruncationIntervalSeconds in CoreCLR
            logTruncationIntervalSeconds = inputReplicatorSettings.PublicSettings.LogTruncationIntervalSeconds.Value;
#endif

            this.checkpointManager = new CheckpointManager(
                this.checkpointCountCounterWriter,
                this.throttleRateCounterWriter,
                this.recoveredOrCopiedCheckpointLsn, 
                this.backupManager, 
                this.replicatedLogManager, 
                txMap, 
                this.stateManager, 
                this.recordsProcessor.ProcessedPhysicalRecord, 
                this.recordsProcessor.ProcessedLogicalRecord, 
                inputReplicatorSettings.ProgressVectorMaxEntries,
                logTruncationIntervalSeconds);

            this.recordsProcessor.Open(this.recoveredOrCopiedCheckpointLsn, this.tracer, this.roleContextDrainState, this.backupManager, this.versionManager, this.checkpointManager, this.stateManager);

            this.checkpointManager.Open(
                this.tracer,
                checked((ulong)this.replicatorSettings.PublicSettings.CheckpointThresholdInMB.Value),
                checked((ulong)this.replicatorSettings.PublicSettings.MinLogSizeInMB.Value),
                checked((uint)this.replicatorSettings.PublicSettings.TruncationThresholdFactor.Value),
                checked((uint)this.replicatorSettings.PublicSettings.ThrottlingThresholdFactor.Value));

            this.replicatedLogManager.Open(
                this.tracer,
                this.checkpointManager.CheckpointIfNecessary,
                null /*initialize the head to null*/,
                this.roleContextDrainState,
                this.fabricReplicator);

            // Create TxManager before log manager
            this.transactionManager = new TransactionManager(
                this.recoveredOrCopiedCheckpointLsn,
                txMap,
                this.replicatorCounterSetInstance,
                this.checkpointManager,
                this.replicatedLogManager,
                this.recordsProcessor,
                this.tracer,
                checked((long)this.replicatorSettings.PublicSettings.MaxRecordSizeInKB * 1024));

            this.secondaryDrainManager.Open(
                this.recoveredOrCopiedCheckpointLsn,
                this.roleContextDrainState,
                this.recordsProcessor,
                this.fabricReplicator,
                this.backupManager,
                this.checkpointManager,
                this.transactionManager,
                this.replicatedLogManager,
                this.stateManager,
                this.replicatorSettings,
                this.recoveryManager,
                this.tracer);
        }

        private void Dispose(bool isDisposing)
        {
            if (isDisposing == true)
            {
                if (this.isDisposed == false)
                {
                    this.recoveryManager.DisposeRecoveryReadStreamIfNeeded();
                    // GopalK, TODO: Preetha, should not we be disposing statemanager here?

                    if (this.logManager != null)
                    {
                        this.logManager.Dispose();
                    }

                    this.DisposePerformanceCounters();

                    this.isDisposed = true;
                    GC.SuppressFinalize(this);
                }
            }
        }

        /// <summary>
        /// Disposes all performance counters.
        /// </summary>
        /// <remarks>TODO: Consider adding a Performance Counter Manager.</remarks>
        private void DisposePerformanceCounters()
        {
            if (this.replicatorCounterSetInstance != null)
            {
                this.replicatorCounterSetInstance.Dispose();
            }
        }

        private void FlushedRecordsCallback(LoggedRecords loggedRecords)
        {
            var records = loggedRecords.Records;
            var exception = loggedRecords.Exception;
            foreach (var record in records)
            {
                if (exception == null)
                {
                    FabricEvents.Events.FlushedRecordsCallback(
                        this.tracer.Type,
                        (uint)record.RecordType,
                        record.Lsn.LSN,
                        record.Psn.PSN,
                        record.RecordPosition);
                }
                else
                {
                    FabricEvents.Events.FlushedRecordsCallbackFailed(
                        this.tracer.Type,
                        (uint)record.RecordType,
                        record.Lsn.LSN,
                        record.Psn.PSN,
                        record.RecordPosition);

                    this.recordsProcessor.ProcessLogException("FlushedRecordsCallback", record, exception);
                }

                var logicalRecord = record as LogicalLogRecord;
                if (logicalRecord != null)
                    logicalRecord.InvalidateReplicatedDataBuffers(this.replicatedLogManager.ReplicationSerializationBinaryWritersPool);

                record.CompletedFlush(exception);
            }

            this.WakeupWaitingStreams(records[records.Count - 1], exception);
            this.logRecordsDispatcher.DispatchLoggedRecords(loggedRecords);

            return;
        }

        private string GetRestoreBackupLogListTrace(IList<string> backupLogPathList)
        {
            var stringBuilder = new StringBuilder();
            var index = 0;
            foreach (var backupLogPath in backupLogPathList)
            {
                stringBuilder.AppendFormat("Index: {0} Path: {1}\r\n", index, backupLogPath);
                index++;
            }

            return stringBuilder.ToString();
        }

        /// <summary>
        /// Constructs all performance counters.
        /// </summary>
        private void InitializePerformanceCounters()
        {
            if (this.replicatorCounterSetInstance == null)
            {
                var performanceCounterInstanceName = string.Format(
                    CultureInfo.InvariantCulture,
                    "{0}:{1}",
                    this.initializationParameters.PartitionId,
                    this.initializationParameters.ReplicaId);

                this.replicatorCounterSetInstance = ReplicatorCounterSet.CreateCounterSetInstance(performanceCounterInstanceName);
            }

            this.inflightVisibilitySequeneceNumberCountCounterWriter =
                new InflightVisibilitySequenceNumberCountCounterWriter(this.replicatorCounterSetInstance);
            this.checkpointCountCounterWriter = new CheckpointCountCounterWriter(this.replicatorCounterSetInstance);
            this.logRecoveryPerformanceCounterWriter =
                new LogRecoveryPerfomanceCounterWriter(this.replicatorCounterSetInstance);
            this.stateManagerRecoveryPerformanceCounterWriter =
                new StateManagerRecoveryPerfomanceCounterWriter(this.replicatorCounterSetInstance);
            this.incomingBytesRateCounterWriter = new IncomingBytesRateCounterWriter(this.replicatorCounterSetInstance);
            this.logFlushBytesRateCounterWriter = new LogFlushBytesRateCounterWriter(this.replicatorCounterSetInstance);
            this.throttleRateCounterWriter = new ThrottleRateCounterWriter(this.replicatorCounterSetInstance);
            this.bytesPerFlushCounterWriter = new AvgBytesPerFlushCounterWriter(this.replicatorCounterSetInstance);
            this.avgTransactionCommitLatencyCounterWriter = new AvgTransactionCommitLatencyCounterWriter(this.replicatorCounterSetInstance);
            this.avgFlushLatencyCounterWriter = new AvgFlushLatencyCounterWriter(this.replicatorCounterSetInstance);
            this.avgSerializationLatencyCounterWriter = new AvgSerializationLatencyCounterWriter(this.replicatorCounterSetInstance);
        }

        /// <summary>
        /// Append epoch update record. Update epochs are barrier records
        /// </summary>
        /// <remarks>
        /// TODO:
        /// Preconditions:
        /// Assert that UpdateEpoch is called either because the current
        /// Primary failed or the new primary failed before old Primary
        /// could be made an ActiveSecondary during swap primary or
        /// an idle secondary that is past copy stage (meaning InBuild) or
        /// this is the very first replica for the service
        /// </remarks>
        private async Task PerformUpdateEpochAsync(Epoch epoch, long previousEpochLastSequenceNumber)
        {
            string message = string.Format(
                CultureInfo.InvariantCulture,
                "UpdateEpoch: <{0}, {1:x}> LSN: {2} ReplicaRole: {3} DrainingStream: {4}",
                epoch.DataLossNumber,
                epoch.ConfigurationNumber,
                previousEpochLastSequenceNumber,
                this.Role,
                this.roleContextDrainState.DrainingStream);
            ReplicatorTraceOnApi(this.tracer, message);

            // This will ensure any pending replicates that haven't been logged will first be logged before
            // inserting the updateepoch log record
            if (this.Role == ReplicaRole.Primary)
            {
                await this.replicatedLogManager.AwaitLsnOrderingTaskOnPrimaryAsync().ConfigureAwait(false);
            }

            var record = new UpdateEpochLogRecord(epoch, this.initializationParameters.ReplicaId);
            var lastVector = this.replicatedLogManager.ProgressVector.LastProgressVectorEntry;
            Utility.Assert(
                (lastVector.Epoch <= epoch) && (lastVector.Lsn.LSN <= previousEpochLastSequenceNumber),
                "Incoming epoch ({0},{1:x}) is less than the last known epoch ({2},{3:x})",
                epoch.DataLossNumber,
                epoch.ConfigurationNumber,
                lastVector.Epoch.DataLossNumber,
                lastVector.Epoch.ConfigurationNumber);

            if (epoch == lastVector.Epoch)
            {
                return;
            }

            // During copy, it is possible we end up copying MORE than what V1 replicator asks to copy.( Copied upto LSN 100 instead of 90)
            // V1 replicator does not know this. As a result the REPL queue is initialized to 90.
            // Now, if the primary goes down and this replica gets picked as Primary (I->P) transition, it is possible we get
            // UE call with e,90 which we cannot handle because we have already loggd upto 100.
            if (this.replicatedLogManager.CurrentLogTailLsn.LSN != previousEpochLastSequenceNumber)
            {
                Utility.Assert(
                    this.roleContextDrainState.ReplicaRole == ReplicaRole.IdleSecondary,
                    "Got UE with {0}:{1:x} for lsn {2} while the tail is {3} and role is not idle (4)",
                    epoch.DataLossNumber, epoch.ConfigurationNumber, previousEpochLastSequenceNumber, this.roleContextDrainState.ReplicaRole);

                throw new FabricException("Cannot handle UE call as log progress is higher than the lsn passed in during UE");
            }

            record.Lsn = new LogicalSequenceNumber(previousEpochLastSequenceNumber);
            this.replicatedLogManager.UpdateEpochRecord(record);

            // Epoch update is a barrier
            await this.logManager.FlushAsync("UpdateEpoch").ConfigureAwait(false);
            Utility.Assert(
                this.replicatedLogManager.CurrentLogTailLsn.LSN == previousEpochLastSequenceNumber,
                "this.currentLogTailLsn.LSN == previousEpochLastSequenceNumber)");

            await record.AwaitProcessing().ConfigureAwait(false);

            return;
        }

        private async Task<ReplicaRole> PrepareForReplicaRoleTransitionAsync(ReplicaRole newRole)
        {
            // GopalK: The order of the following statements is significant
            var currentRole = this.Role;
            if ((currentRole == ReplicaRole.IdleSecondary) && (newRole == ReplicaRole.ActiveSecondary))
            {
                // If copy was CopyStateNone, this is already completed.
                // Ensures that the copystream is drained before allowing the change role.
                await this.secondaryDrainManager.WaitForCopyDrainToCompleteAsync().ConfigureAwait(false);
                this.roleContextDrainState.ChangeRole(ReplicaRole.ActiveSecondary);
            }
            else if ((currentRole == ReplicaRole.ActiveSecondary) && (newRole == ReplicaRole.IdleSecondary))
            {
                Utility.Assert(false, "S->I not valid");
            }
            else
            {
                await this.secondaryDrainManager.WaitForCopyAndReplicationDrainToCompleteAsync().ConfigureAwait(false);

                if (currentRole == ReplicaRole.Primary)
                {
                    await replicatedLogManager.AwaitLsnOrderingTaskOnPrimaryAsync().ConfigureAwait(false);
                    await this.AwaitBackupCallbackOnPrimaryAsync().ConfigureAwait(false);
                    await this.CancelAndDrainTelemtryTaskAsync().ConfigureAwait(false);

                    // Drain the aborting of pending transactions if any.
                    if (this.primaryTransactionsAbortOrGroupCommitTaskCompletionSource != null)
                    {
                        this.primaryTransactionsAbortOrGroupCommitTaskCompletionSource.Cancel();
                        await this.primaryTransactionsAbortOrGroupCommitTask.ConfigureAwait(false);
                        this.primaryTransactionsAbortOrGroupCommitTask = null;
                        this.primaryTransactionsAbortOrGroupCommitTaskCompletionSource = null;
                    }
                    else
                    {
                        Utility.Assert(
                            this.primaryTransactionsAbortOrGroupCommitTask == null,
                            "If transactionAbortTaskCts is null, transactionAbortTask must be null.");
                    }
                }

                if ((newRole == ReplicaRole.Unknown) || (newRole == ReplicaRole.None))
                {
                    this.primaryTransactionsAbortingInstance = null;
                    this.roleContextDrainState.OnClosing();

                    // Do not log additional record if 'RemovingState' is already logged earlier
                    if (this.recoveryManager.IsRemovingStateAfterOpen == false)
                    {
                        var informationEvent = (newRole == ReplicaRole.None)
                            ? InformationEvent.RemovingState
                            : InformationEvent.Closed;

                        await this.replicatedLogManager.FlushInformationRecordAsync(
                            informationEvent,
                            closeLog: true,
                            flushInitiator: "Closing/Removing State in prepare for replica role transition").ConfigureAwait(false);
                    }
                }
                else if (currentRole == ReplicaRole.Primary)
                {
                    this.primaryTransactionsAbortingInstance = null;

                    await this.replicatedLogManager.FlushInformationRecordAsync(
                        InformationEvent.PrimarySwap,
                        closeLog: false,
                        flushInitiator: "Swapping out from primary").ConfigureAwait(false);
                }

                await this.QuiesceReplicaActivityAsync().ConfigureAwait(false);
                this.roleContextDrainState.ChangeRole(newRole);

                if (currentRole == ReplicaRole.Primary)
                {
                    this.PrepareToTransitionOutOfPrimary(newRole == ReplicaRole.ActiveSecondary);
                }
            }

            Utility.Assert(this.Role == newRole, "this.recordsProcessor.ReplicaRole == newRole");

            return currentRole;
        }

        private async Task QuiesceReplicaActivityAsync()
        {
            if (LogRecord.IsInvalidRecord(this.replicatedLogManager.LastInformationRecord) == false)
            {
                PhysicalLogRecord quiescedRecord = this.replicatedLogManager.LastInformationRecord;
                await this.logManager.FlushAsync("QuiesceReplicaActivity").ConfigureAwait(false);

                FabricEvents.Events.QuiesceReplicaActivityAsync(
                    this.tracer.Type,
                    this.Role.ToString(),
                    "Quiescing Record Type: " + quiescedRecord.RecordType + " LSN: " + quiescedRecord.Lsn.LSN + " PSN: "
                    + quiescedRecord.Psn.PSN + " Position: " + quiescedRecord.RecordPosition);

                await quiescedRecord.AwaitProcessing().ConfigureAwait(false);

                // NOTE - It is possible for the last information record to be overwritten to "ReplicationFinished" since the same drain task is used during copy and replication pump
                Utility.Assert(
                    quiescedRecord == this.replicatedLogManager.LastInformationRecord,
                    "quiescedRecord type is {0} not equal to last information record type {1}",
                    quiescedRecord.RecordType, this.replicatedLogManager.LastInformationRecord.RecordType);

                FabricEvents.Events.QuiesceReplicaActivityAsync(
                    this.tracer.Type,
                    this.Role.ToString(),
                    "Quiescing Record Done Type: " + quiescedRecord.RecordType + " LSN: " + quiescedRecord.Lsn.LSN
                    + " PSN: " + quiescedRecord.Psn.PSN + " Position: " + quiescedRecord.RecordPosition);
            }
            else
            {
                FabricEvents.Events.QuiesceReplicaActivityAsync(
                    this.tracer.Type,
                    this.Role.ToString(),
                    "No record to quiesce.");
            }

            await this.recordsProcessor.WaitForLogicalRecordsProcessingAsync().ConfigureAwait(false);
        }

        /// <summary>
        /// This method ensures that the first ever replicated record on a new primary is a barrier
        /// </summary>
        private BarrierLogRecord ReplicateBarrierIfPrimaryStatusIsBelowEstablished()
        {
            BarrierLogRecord record = null;

            if (this.primaryStatus <= PrimaryElectionStatus.Elected)
            {
                // Check outside the lock since the condition is false most of the times

                lock (this.primaryStatusLock)
                {
                    if (this.primaryStatus <= PrimaryElectionStatus.Elected)
                    {
                        record = this.checkpointManager.ReplicateBarrier();
                        this.primaryStatus = PrimaryElectionStatus.Established;
                    }
                }
            }

            return record;
        }

        private void Reuse()
        {
            this.isDisposed = false;

            this.recoveredOrCopiedCheckpointLsn.Update(LogicalSequenceNumber.InvalidLsn);
            this.roleContextDrainState.Reuse(this.partition);
            this.recoveryManager.Reuse();
            this.recordsProcessor.Reuse();

            lock (this.flushWaitLock)
            {
                this.waitingStreams = new List<KeyValuePair<TaskCompletionSource<object>, LogicalSequenceNumber>>();
            }

            if (this.checkpointManager != null)
            {
                this.checkpointManager.Reuse();
            }

            if (this.replicatedLogManager != null)
            {
                this.replicatedLogManager.Reuse();
            }

            this.primaryStatus = PrimaryElectionStatus.None;
            this.abortDelay = ReplicatorDynamicConstants.AbortDelay;

            this.drainBackupCallBackTcs = null;
            this.primaryTransactionsAbortingInstance = null;
            this.primaryTransactionsAbortOrGroupCommitTask = null;
            this.primaryTransactionsAbortOrGroupCommitTaskCompletionSource = null;
            this.versionManager = new VersionManager(this);
            this.secondaryDrainManager.Reuse();
        }

        private void WakeupWaitingStreams(LogRecord mostRecentFlushedRecord, Exception flushException)
        {
            var mostRecentFlushedLsn = mostRecentFlushedRecord.Lsn;
            List<KeyValuePair<TaskCompletionSource<object>, LogicalSequenceNumber>> streams = null;
            lock (this.flushWaitLock)
            {
                int i;
                for (i = 0; i < this.waitingStreams.Count; i++)
                {
                    var stream = this.waitingStreams[i];
                    if (stream.Value <= mostRecentFlushedLsn)
                    {
                        if (streams == null)
                        {
                            streams = new List<KeyValuePair<TaskCompletionSource<object>, LogicalSequenceNumber>>();
                        }

                        streams.Add(stream);
                        continue;
                    }

                    break;
                }

                if (streams != null)
                {
                    this.waitingStreams.RemoveRange(0, i);
                }
            }

            if (streams != null)
            {
                FabricEvents.Events.WakeupWaitingStreams(this.tracer.Type, mostRecentFlushedLsn.LSN);

                foreach (var stream in streams)
                {
                    if (flushException == null)
                    {
                        stream.Key.SetResult(null);
                    }
                    else
                    {
                        stream.Key.SetException(flushException);
                    }
                }
            }

            return;
        }
    }
}
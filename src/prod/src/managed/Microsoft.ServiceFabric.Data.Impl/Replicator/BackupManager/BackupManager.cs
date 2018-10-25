// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator.Backup
{
    using System;
    using System.Diagnostics;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Common.Monitoring;
    using System.Fabric.Common.Tracing;
    using System.Globalization;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;

    using Microsoft.ServiceFabric.Data;

    internal class BackupManager : IBackupManager
    {
        /// <summary>
        /// Class name for tracing.
        /// </summary>
        private const string TraceClassName = "TBackupManager";

        /// <summary>
        /// True string.
        /// </summary>
        private const string TrueString = "true";

        /// <summary>
        /// Starting backoff interval.
        /// </summary>
        private const int StartingBackoffInMilliSeconds = 500;

        /// <summary>
        /// Max retry count.
        /// </summary>
        private const int MaxRetryCount = 7;

        /// <summary>
        /// Period of trace during re-try.
        /// </summary>
        private const int periodOfTrace = 2;

        /// <summary>
        /// Initialization parameters.
        /// </summary>
        private readonly StatefulServiceContext initializationParameters;

        /// <summary>
        /// Logging replicator.
        /// </summary>
        private readonly IBackupRestoreProvider loggingReplicator;

        /// <summary>
        /// State manager.
        /// </summary>
        private readonly IStateManager stateManager;

        /// <summary>
        /// Trace type.
        /// </summary>
        private readonly string traceType;

        /// <summary>
        /// Flag used to make sure there is only one backup going on at any point of time.
        /// 0: false and 1: true.
        /// </summary>
        private int isBackupInProgress = 0;

        /// <summary>
        /// Flag used to make sure there is only one restore going on at any point of time.
        /// 0: false and 1: true.
        /// </summary>
        private int isRestoreInProgress = 0;

        /// <summary>
        /// The last completed backup log record.
        /// </summary>
        private BackupLogRecord lastCompletedBackupLogRecord;

        /// <summary>
        /// Test variable that causes the DeleteRestoreToken call to no-op instead of deleting the token. 
        /// </summary>
        private bool testFlagNoOpDeleteRestoreToken = false;

        /// <summary>
        /// Component used to monitor BackupCallbackAsync duration
        /// </summary>
        private ReplicatorApiMonitor apiMonitor;

        public BackupManager(
            StatefulServiceContext initializationParameters,
            IBackupRestoreProvider loggingReplicator,
            IStateManager stateManager,
            ReplicatorApiMonitor apiMonitor)
        {
            this.initializationParameters = initializationParameters;
            this.loggingReplicator = loggingReplicator;
            this.stateManager = stateManager;
            this.apiMonitor = apiMonitor;

            this.traceType = TraceClassName + "@" + initializationParameters.PartitionId + '.' + initializationParameters.ReplicaId;

            this.LastCompletedBackupLogRecord = BackupLogRecord.ZeroBackupLogRecord;
            this.IsLogUnavailableDueToRestore = false;
        }

        /// <summary>
        /// Indicates whether the log is unavailable due to restore.
        /// </summary>
        public bool IsLogUnavailableDueToRestore { get; private set; }

        /// <summary>
        /// Gets or sets the last completed backup log record.
        /// </summary>
        public BackupLogRecord LastCompletedBackupLogRecord
        {
            get { return this.lastCompletedBackupLogRecord; }

            set
            {
                FabricEvents.Events.SetBackupLogRecord(
                    this.traceType,
                    value.Lsn.LSN,
                    value.Psn.PSN,
                    value.RecordPosition,
                    value.HighestBackedUpEpoch.DataLossNumber,
                    value.HighestBackedUpEpoch.ConfigurationNumber,
                    value.HighestBackedUpLsn.LSN);

                this.lastCompletedBackupLogRecord = value;
            }
        }

        /// <summary>
        /// Undo the last compelted backup log record.
        /// </summary>
        public void UndoLastCompletedBackupLogRecord()
        {
            Utility.Assert(
                this.lastCompletedBackupLogRecord != null && this.lastCompletedBackupLogRecord.IsZeroBackupLogRecord() == false,
                "{0}: If there has been no completed backup log record, it cannot be undone.",
                this.traceType);

#if !DotNetCoreClr
            // These are new events defined in System.Fabric, existing CoreCLR apps would break 
            // if these events are referenced as it wont be found. As CoreCLR apps carry System.Fabric
            // along with application
            // This is just a mitigation for now. Actual fix being tracked via bug# 11614507

            FabricEvents.Events.UndoBackupLogRecord(
                this.traceType,
                this.lastCompletedBackupLogRecord.Lsn.LSN,
                this.lastCompletedBackupLogRecord.Psn.PSN,
                this.lastCompletedBackupLogRecord.RecordPosition,
                this.lastCompletedBackupLogRecord.HighestBackedUpEpoch.DataLossNumber,
                this.lastCompletedBackupLogRecord.HighestBackedUpEpoch.ConfigurationNumber,
                this.lastCompletedBackupLogRecord.HighestBackedUpLsn.LSN);
#endif

            // Note that given truncate tail can only undo one backup log record.
            // In the current implementation, if a replica goes through truncate tail and the primary does not replicate any other backup log records
            // this replica will be required to take a full backup when it becomes primary.
            // The probability can be reduced by keeping track of the last the completed backup log records.
            // In this way, if there is a LCB -1, LCB can become LCB -1 and backup in the last completed begin checkpoint otherwise.
            this.LastCompletedBackupLogRecord = BackupLogRecord.ZeroBackupLogRecord;
        }

        /// <summary>
        /// Test only flag.
        /// This flag can be used during testing to not delete the restore file marker at the end of restore.
        /// This is useful for simulating failures within restore code path.
        /// </summary>
        public bool TestFlagNoOpDeleteRestoreToken
        {
            get { return this.testFlagNoOpDeleteRestoreToken; }

            set
            {
                FabricEvents.Events.TestSettingModified(this.traceType, "TestFlagNoOpDeleteRestoreToken", TrueString);
                this.testFlagNoOpDeleteRestoreToken = value;
            }
        }

        /// <summary>
        /// Component used to monitor BackupCallbackAsync duration
        /// </summary>
        public ReplicatorApiMonitor ApiMonitor
        {
            get { return this.apiMonitor; }
            set { this.apiMonitor = value; }
        }

        /// <summary>
        /// Deletes system maintained local backup folder if it already exists.
        /// </summary>
        /// <devnote>
        /// #13227470 and #13227511
        /// </devnote>
        public void DeletePersistedState()
        {
            Exception exceptionTmp = null;

            try
            {
                // #13227470: This only delete the replica specfic trbackup token folder.
                var backupFolderPath = this.GetLocalBackupFolderPath();
                this.DeleteFolderIfExists(backupFolderPath);
            }
            catch (Exception e)
            {
                // If deleting the local backup folder fails, we still want to try to delete the restore folder.
                exceptionTmp = e;
            }

            // #13227470: This only delete the replica specfic restore token folder.
            var restoreTokenFolderPath = this.GetRestoreTokenFolderPath();
            this.DeleteFolderIfExists(restoreTokenFolderPath);

            if (exceptionTmp != null)
            {
                throw exceptionTmp;
            }
        }

        /// <summary>
        /// Acquire backup lock.
        /// </summary>
        /// <param name="backupId">Name of the lock taker.</param>
        /// <remarks>
        /// Used to ensure there is at max one backup at any given point of time.
        /// </remarks>
        public void AcquireBackupLock(Guid backupId)
        {
            var isBackupCurrentlyInProgress = Interlocked.CompareExchange(
                location1: ref this.isBackupInProgress,
                value: 1,
                comparand: 0);

            // If currently a backup is in progress throw.
            if (isBackupCurrentlyInProgress == 1)
            {
                FabricEvents.Events.Lock(
                    this.traceType,
                    "backup",
                    backupId.ToString(),
                    "Failed to acquire because backup is in progress.");

                throw new FabricBackupInProgressException(SR.Error_BackupInProgress);
            }
        }

        /// <summary>
        /// Creates a backup of the transactional replicator and all registered state providers.
        /// The default backupOption is FULL.
        /// The default timeout is 1 hour.
        /// The default cancellationToken is None.
        /// </summary>
        /// <param name="replicatedLogManager">The replicated log manager.</param>
        /// <param name="checkpointManager">The checkpoint manager.</param>
        /// <param name="backupCallback">Method that will be called once the local backup folder has been created.</param>
        /// <exception cref="FabricBackupInProgressException">Backup is in progress.</exception>
        /// <exception cref="FabricNotPrimaryException">Replicator role is not primary.</exception>
        /// <exception cref="ArgumentException">backup folder path is null.</exception>
        /// <exception cref="ArgumentNullException">backupCallbackAsync delegate is null.</exception>
        /// <exception cref="OperationCanceledException">Operation has been canceled.</exception>
        /// <exception cref="FabricTransientException">Retry-able exception.</exception>
        /// <exception cref="TimeoutException">Operation has timed out.</exception>
        /// <returns>Task that represents the asynchronous backup.</returns>
        public Task<BackupInfo> BackupAsync(
            IReplicatedLogManager replicatedLogManager,
            ICheckpointManager checkpointManager,
            Func<BackupInfo, CancellationToken, 
            Task<bool>> backupCallback)
        {
            return this.BackupAsync(replicatedLogManager, checkpointManager, backupCallback, BackupOption.Full, Timeout.InfiniteTimeSpan, CancellationToken.None);
        }

        /// <summary>
        /// Creates a backup of the transactional replicator and all registered state providers.
        /// </summary>
        /// <param name="replicatedLogManager">The replicated log manager.</param>
        /// <param name="checkpointManager">The checkpoint manager.</param>
        /// <param name="backupCallback">Method that will be called once the local backup folder has been created.</param>
        /// <param name="backupOption">The backup option.</param>
        /// <param name="timeout">The timeout.</param>
        /// <param name="cancellationToken">The cancellation token to signal cancellation requests.</param>
        /// <returns>Task that represents the asynchronous backup.</returns>
        /// <exception cref="FabricMissingFullBackupException">
        /// Indicates that a full backup needs to be taken before an incremental backup can be taken.
        /// Possible example cases are
        /// - Incremental backup is called on an epoch that has never been fully backed up.
        /// </exception>
        /// <remarks>
        /// Invariant: FabricMissingFullBackupException can only be thrown if backupOption is BackupOption.Incremental.
        /// </remarks>
        /// 
        /// <remarks>
        /// Locks used in backup
        /// 1. BackupLock: Used to ensure there is at most one backup in flight at any given point of time. (CExchange lock)
        /// 2. BackupConsistencyLock: Lock protects logging of EndCheckpoint to after logging of CompleteCheckpoint.
        ///     This guarantees that checkpoints copied as part of backup are all from the same prepare checkpoint 
        ///     and relevant log sections are not truncated before physical log reader is generated (which takes the logical truncation range lock).
        /// 3. BackupDrainLock: Used for the ManualResetEventAsync like functionality to drain backup during ChangeRole.
        /// 4. StateManagerApiLock: Ensures that Backup and (Prepare | Perform Checkpoint) are not call concurrently.
        /// </remarks>
        public async Task<BackupInfo> BackupAsync(
            IReplicatedLogManager replicatedLogManager,
            ICheckpointManager checkpointManager,
            Func<BackupInfo, CancellationToken, Task<bool>> backupCallback, 
            BackupOption backupOption, 
            TimeSpan timeout, 
            CancellationToken cancellationToken)
        {
            // Argument checks
            Requires.Argument("backupCallbackAsync", backupCallback).NotNull();

            this.loggingReplicator.ThrowIfNoWriteStatus();

            // Ensure that the timeout is in range.
            // Can throw ArgumentOutOfRangeException
            timeout = this.ParseTimeout(timeout);

            // Used to measure elapsed time to appropriately throw timeout
            var backupStopwatch = new Stopwatch();

            backupStopwatch.Start();

            var backupId = this.GenerateBackupId(backupOption);

            // TODO: Hosting to provide us with a backup folder.
            var backupFolder = this.GetLocalBackupFolderPath();

            // Construct the backup paths.
            var replicatorBackupFolderPath = this.GetLocalBackupReplicatorFolderPath();
            var stateManagerBackupFolderPath = Path.Combine(backupFolder, Replicator.Constants.StateManagerBackupFolderName);

            // Get the replicator settings from the replicator.
            // We could have also have the replicator set this setting when necessary (e.g. during open)
            // However, that option is more error prone since new code may forget to set it.
            TransactionalReplicatorSettings settings = this.loggingReplicator.ReplicatorSettings;

            // Can throw FabricBackupInProgressException
            // Used to ensure there is at max one backup at any given point of time.
            this.AcquireBackupLock(backupId);

            try
            {
                this.ThrowIfFullBackupIsMissing(settings, replicatedLogManager.ProgressVector, backupOption, backupId);

                // Indicates that Backup has been accepted.
                FabricEvents.Events.AcceptBackup(this.traceType, backupId, (int) backupOption, backupFolder);

                // Ensure that the replicator backup folder has been created.
                await this.PrepareBackupFolderAsync(backupOption, backupFolder, cancellationToken).ConfigureAwait(false);

                IAsyncEnumerator<LogRecord> logRecords = null;
                if (backupOption == BackupOption.Full)
                {
                    // This call will be drained in case of change role or close.
                    logRecords = await this.BackupStateManagerAndGetReplicatorLogsToBeBackedUpAsync(
                        replicatedLogManager,
                        backupId,
                        stateManagerBackupFolderPath,
                        backupOption,
                        timeout,
                        cancellationToken).ConfigureAwait(false);
                }
                else
                {
                    // This call will be drained in case of change role or close.
                    logRecords = await this.GetIncrementalLogRecordsAsync("incremental backup " + backupId).ConfigureAwait(false);
                }

                ReplicatorBackup replicatorBackupInfo;
                using (logRecords)
                {
                    // Must do this inside the using to avoid not disposing the logrecords.
                    this.ThrowIfBackupTimeoutExceeded(backupStopwatch.Elapsed, timeout);

                    replicatorBackupInfo = await this.BackupReplicatorAsync(
                                backupId,
                                backupOption,
                                replicatorBackupFolderPath,
                                logRecords,
                                cancellationToken).ConfigureAwait(false);
                }

                // This is the first time when we have accurate information of how much space the log records took.
                // Common reason for this is that records are serialized as logical and incremental filters records.
                this.ThrowIfAccumulatedIncrementalBackupLogsIsTooLarge(settings, backupOption, backupId, replicatorBackupInfo);

                var backupMetadataFilePath = this.GenerateBackupMetadataName(backupFolder, backupOption);

                // Can throw FabricNotPrimaryException.
                // This call will be drained in case of change role or close.
                var parentBackupId = (backupOption == BackupOption.Full)
                    ? backupId
                    : this.lastCompletedBackupLogRecord.BackupId;

                var backupMetadataFile = await this.WriteBackupMetadataFile(
                            backupMetadataFilePath,
                            backupOption,
                            parentBackupId,
                            backupId,
                            this.initializationParameters.PartitionId,
                            this.initializationParameters.ReplicaId,
                            replicatorBackupInfo.EpochOfFirstLogicalLogRecord,
                            replicatorBackupInfo.LsnOfFirstLogicalLogRecord.LSN,
                            replicatorBackupInfo.EpochOfHighestBackedUpLogRecord,
                            replicatorBackupInfo.LsnOfHighestBackedUpLogRecord.LSN,
                            cancellationToken).ConfigureAwait(false);

                FabricEvents.Events.BackupAsync(this.traceType, backupId, backupMetadataFile.ToString());

                // Wait for the highest backed up LSN to be replicated to ensure no false progress.
                // This is only required when we have incremental backup.
                await EnsureBackedUpStateIsStableAsync(
                    replicatedLogManager, 
                    checkpointManager, 
                    replicatorBackupInfo, 
                    backupId, 
                    backupStopwatch, 
                    timeout).ConfigureAwait(false);

                // Measure the elapsed time after the backup is generated, throw if specified timeout is exceeded
                this.ThrowIfBackupTimeoutExceeded(backupStopwatch.Elapsed, timeout);

                // TODO: This could be a provisional trace.
                FabricEvents.Events.BackupAsync(this.traceType, backupId, "Backed up state is now stable: " + replicatorBackupInfo.LsnOfHighestBackedUpLogRecord.LSN);

                BackupInfo.BackupVersion backupVersion = new BackupInfo.BackupVersion(replicatorBackupInfo.EpochOfHighestBackedUpLogRecord, replicatorBackupInfo.LsnOfHighestBackedUpLogRecord.LSN);
                BackupInfo.BackupVersion startBackupVersion = new BackupInfo.BackupVersion(replicatorBackupInfo.EpochOfFirstLogicalLogRecord, replicatorBackupInfo.LsnOfFirstLogicalLogRecord.LSN);

                BackupInfo backupInfo = new BackupInfo(backupFolder, backupOption, backupVersion, startBackupVersion, backupId, parentBackupId);
                bool callbackResult = await this.CallBackupCallbackAsync(backupId, backupInfo, backupCallback, cancellationToken).ConfigureAwait(false);

                // Measure elapsed time after the user-defined callback. Throw if specified timeout is exceeded
                this.ThrowIfBackupTimeoutExceeded(backupStopwatch.Elapsed, timeout);

                if (callbackResult == false)
                {
                    throw new InvalidOperationException(SR.Error_BackupCallbackFailed);
                }

                await this.ReplicateBackupLogRecordAsync(
                    backupId,
                    replicatorBackupInfo,
                    timeout,
                    backupStopwatch).ConfigureAwait(false);

                // TODO: This should include duration of each stage.
                FabricEvents.Events.BackupAsyncCompleted(
                    this.traceType,
                    backupId,
                    backupOption.ToString(),
                    backupFolder,
                    replicatorBackupInfo.EpochOfHighestBackedUpLogRecord.DataLossNumber,
                    replicatorBackupInfo.EpochOfHighestBackedUpLogRecord.ConfigurationNumber,
                    replicatorBackupInfo.LsnOfHighestBackedUpLogRecord.LSN);

                return backupInfo;
            }
            catch (ArgumentOutOfRangeException e)
            {
                throw new FabricMissingFullBackupException(SR.Error_FullBackupRequired, e);
            }
            catch (Exception e)
            {
                FabricEvents.Events.BackupException(
                    this.traceType,
                    backupId,
                    e.GetType().ToString(),
                    e.Message,
                    e.StackTrace);

                throw;
            }
            finally
            {
                this.ReleaseBackupLock(backupId);
            }
        }

        // Note: Replicate BackupLogRecord is the next step of user upload backup, it may throw transient 
        // exception if reconfiguration is going on. If the user retry the BackupAsync call, it will cause
        // the problem that two same backups get uploaded to the same folder. Here, we retry the replicate 
        // call in certain times to reduce the chance of the prblem.
        internal async Task ReplicateBackupLogRecordAsync(
            Guid backupId,
            ReplicatorBackup replicatorBackup,
            TimeSpan timeout,
            Stopwatch stopwatch)
        {
            int retryCount = 0;
            while (true)
            {
                try
                {
                    await this.loggingReplicator.ReplicateBackupLogRecordAsync(backupId, replicatorBackup).ConfigureAwait(false);
                    break;
                }
                catch (FabricTransientException e)
                {
                    // Trace out FabricTransientException in certain period.
                    if (retryCount % periodOfTrace == 0)
                    {
                        FabricEvents.Events.BackupException(
                            this.traceType,
                            backupId,
                            "FabricTransientException in BackupAsync BackupLogRecord replication.",
                            e.Message,
                            e.ToString());
                    }
                }

                // Retry the replication call if it throws FabricTransientException until timed out.
                if (!timeout.Equals(Timeout.InfiniteTimeSpan) && stopwatch.Elapsed >= timeout)
                {
                    string message = string.Format(
                        CultureInfo.CurrentCulture,
                        SR.Error_BackupTimeout_ReplicateFailed,
                        this.traceType,
                        backupId,
                        replicatorBackup.EpochOfHighestBackedUpLogRecord.DataLossNumber,
                        replicatorBackup.EpochOfHighestBackedUpLogRecord.ConfigurationNumber,
                        replicatorBackup.LsnOfHighestBackedUpLogRecord);
                    throw new TimeoutException(message);
                }

                // MCoskun: Because the starting back off time is high, use linear back off instead of exponential.
                await Task.Delay(StartingBackoffInMilliSeconds * (retryCount++)).ConfigureAwait(false);
            }
        }

        private async Task EnsureBackedUpStateIsStableAsync(
            IReplicatedLogManager replicatedLogManager, 
            ICheckpointManager checkpointManager, 
            ReplicatorBackup replicatorBackupInfo, 
            Guid backupId,
            Stopwatch stopWatch,
            TimeSpan timeout)
        {
            // If it is already stable, return.
            if (replicatorBackupInfo.LsnOfHighestBackedUpLogRecord.LSN <= this.loggingReplicator.LastStableLsn.LSN)
            {
                return;
            }

            BarrierLogRecord barrierLogRecord = null;
            int retryCount = 0;
            while (true)
            {
                try
                {
                    barrierLogRecord = checkpointManager.ReplicateBarrier();
                    break;
                }
                catch (FabricTransientException e)
                {
                    // Retry.
                    if (retryCount % periodOfTrace == 0)
                    {
                        FabricEvents.Events.BackupException(
                            this.traceType, 
                            backupId, 
                            "FabricTransientException in BackupAsync Barrier replication.", 
                            e.Message,
                            e.ToString());
                    }
                }

                this.ThrowIfBackupTimeoutExceeded(stopWatch.Elapsed, timeout);

                // MCoskun: Because the starting back off time is high, use linear back off instead of exponential.
                await Task.Delay(StartingBackoffInMilliSeconds * (retryCount++)).ConfigureAwait(false);

                // If during the wait, the LSN became stable, complete the call.
                if (replicatorBackupInfo.LsnOfHighestBackedUpLogRecord.LSN <= this.loggingReplicator.LastStableLsn.LSN)
                {
                    return;
                }
            }

            Utility.Assert(barrierLogRecord != null, "{0} {1} Barrier Record from CheckpointManager::ReplicateBarrier cannot be null.", this.traceType, backupId);

            // May throw FabricObjectClosedException or FabricNotPrimaryException
            await LoggingReplicator.AwaitReplication(barrierLogRecord, replicatedLogManager).ConfigureAwait(false);

            // Check if it is already flushed.
            Task barrierFlushTask = barrierLogRecord.AwaitFlush();
            if (barrierFlushTask.IsCompleted == false)
            {
                // If not, issue a flush in case no-one is issuing the flush.
                await replicatedLogManager.FlushAsync(backupId.ToString()).ConfigureAwait(false);
            }

            try
            {
                await barrierFlushTask.ConfigureAwait(false);
            }
            catch (Exception e)
            {
                // Even though this is not a replication operation, below Exception handling simply turns it into one of the below.
                // May throw FabricObjectClosedException or FabricNotPrimaryException
                replicatedLogManager.ThrowReplicationException(barrierLogRecord, e);
            }

            // This operation does not throw.
            await barrierLogRecord.AwaitProcessing().ConfigureAwait(false);
            return;
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
            return this.RestoreAsync(backupFolder, RestorePolicy.Safe, CancellationToken.None);
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
        /// <exception cref="InvalidOperationException">RestoreAsync has been called at an invalid state. E.g. it was not called during OnDataLossAsync.</exception>
        /// <exception cref="FabricMissingFullBackupException">A valid full backup folder to be the head of the backup chain could not be found.</exception>
        /// <exception cref="ArgumentException">The input folder is not a valid backup folder.</exception>
        /// <exception cref="InvalidDataException">There is a corruption in the folder.</exception>
        public async Task RestoreAsync(
            string backupDirectory,
            RestorePolicy restorePolicy,
            CancellationToken cancellationToken)
        {
            // Check for null argument.
            Requires.Argument("backupDirectory", backupDirectory).NotNullOrWhiteSpace();

            if (false == this.stateManager.IsServiceOnDataLossInFlight)
            {
                throw new InvalidOperationException(SR.Error_RestoreMustBeCalledDuringDataLoss);
            }

            var backupFolderInfo = new BackupFolderInfo(this.traceType, backupDirectory);
            await backupFolderInfo.AnalyzeAsync(cancellationToken).ConfigureAwait(false);

            // Find the backup folder for the replicator.
            var stateManagerBackupFolderPath = backupFolderInfo.StateManagerBackupFolderPath;

            var restoreId = Guid.NewGuid();
            this.ThrowIfInvalidState(restoreId);
            this.ThrowIfNotSafe(restorePolicy, backupFolderInfo, restoreId);

            this.AcquireRestoreLock(restoreId);
            try
            {
                FabricEvents.Events.AcceptRestore(
                    this.traceType,
                    restoreId,
                    Utility.ToString(backupFolderInfo.StartingEpoch),
                    backupFolderInfo.StartingLsn,
                    Utility.ToString(backupFolderInfo.HighestBackedupEpoch),
                    backupFolderInfo.HighestBackedupLsn,
                    backupFolderInfo.ReplicatorBackupFilePathList.Count,
                    backupDirectory);

                var stopwatch = new Stopwatch();
                stopwatch.Start();

                await this.loggingReplicator.CancelTheAbortingOfTransactionsIfNecessaryAsync().ConfigureAwait(false);

                this.CreateRestoreToken();

                await this.stateManager.RestoreAsync(stateManagerBackupFolderPath, cancellationToken).ConfigureAwait(false);
                var stateManagerRestoreInMilliseconds = stopwatch.ElapsedMilliseconds;
                stopwatch.Restart();

                var dataLossEpoch = this.loggingReplicator.CurrentLogTailEpoch;

                // Close the replicator, delete log.
                await this.loggingReplicator.PrepareToCloseAsync().ConfigureAwait(false);

                // To ensure any failures from this point on do not try to log new records in to the deleted / to be deleted log.
                Utility.Assert(
                    this.IsLogUnavailableDueToRestore == false,
                    "this.isLogUnavailableDueToRestore must be false");
                this.IsLogUnavailableDueToRestore = true;

                await this.loggingReplicator.PrepareForRestore(restoreId).ConfigureAwait(false);

                await this.loggingReplicator.RecoverRestoredDataAsync(
                        restoreId,
                        backupFolderInfo.ReplicatorBackupFilePathList).ConfigureAwait(false);
                var replicatorRestoreInMilliseconds = stopwatch.ElapsedMilliseconds;
                stopwatch.Restart();

                this.IsLogUnavailableDueToRestore = false;

                // Reset the replica role and primary state. These would have been reset during recovery
                await this.loggingReplicator.BecomePrimaryAsync(true).ConfigureAwait(false);

                await this.loggingReplicator.FinishRestoreAsync(dataLossEpoch).ConfigureAwait(false);

                Utility.Assert(true == this.CheckIfRestoreTokenExists(), "false == this.CheckIfRestoreTokenExists()");

                this.DeleteRestoreToken();

                FabricEvents.Events.RestoreCompleted(
                    this.traceType,
                    restoreId,
                    backupDirectory,
                    dataLossEpoch.DataLossNumber,
                    dataLossEpoch.ConfigurationNumber,
                    this.loggingReplicator.CurrentLogTailLsn.LSN,
                    stateManagerRestoreInMilliseconds,
                    replicatorRestoreInMilliseconds,
                    stateManagerRestoreInMilliseconds + replicatorRestoreInMilliseconds + stopwatch.ElapsedMilliseconds);
            }
            catch (Exception e)
            {
                FabricEvents.Events.RestoreException(
                    this.traceType,
                    restoreId,
                    e.GetType().ToString(),
                    e.Message,
                    e.StackTrace);

                throw;
            }
            finally
            {
                this.ReleaseRestoreLock(restoreId);
            }
        }

        public bool ShouldCleanState()
        {
            // Int reads are atomic.
            if (1 == this.isRestoreInProgress)
            {
                return false;
            }

            // If currently no restore is ongoing and restore token exists (replica is being opened) 
            // then logging replicator should initiate cleaning of the state since last restore was incomplete.
            return FabricFile.Exists(this.GetRestoreTokenFilePath());
        }

        public void StateCleaningComplete()
        {
            this.DeleteRestoreToken();
        }

        /// <summary>
        /// Acquire restore lock.
        /// </summary>
        /// <param name="restoreId">Restore id.</param>
        internal void AcquireRestoreLock(Guid restoreId)
        {
            var isBackupCurrentlyInProgress = Interlocked.CompareExchange(
                location1: ref this.isRestoreInProgress,
                value: 1,
                comparand: 0);

            // If currently a backup is in progress throw.
            if (isBackupCurrentlyInProgress == 1)
            {
                FabricEvents.Events.Lock(
                    this.traceType,
                    "restore",
                    restoreId.ToString(),
                    "Failed to acquire because restore is in progress.");

                throw new InvalidOperationException(SR.Error_RestoreInProgress);
            }
        }

        /// <summary>
        /// Backup the replicator.
        /// Should contain the logs and the progress vector.
        /// This will dispose the physical log restoreLogStream.
        /// </summary>
        /// <param name="backupId"></param>
        /// <param name="backupOption"></param>
        /// <param name="replicatorBackupFolder">Directory to upload replicator back up.</param>
        /// <param name="logRecordsAsyncEnumerator"></param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Backs up the replicator logs.</returns>
        internal async Task<ReplicatorBackup> BackupReplicatorAsync(
            Guid backupId,
            BackupOption backupOption,
            string replicatorBackupFolder,
            IAsyncEnumerator<LogRecord> logRecordsAsyncEnumerator,
            CancellationToken cancellationToken)
        {
            Utility.Assert(logRecordsAsyncEnumerator != null, "physicalLogReader must not be null.");

            BackupLogFile backupLogFile = null;

            var stopwatch = new Stopwatch();

            this.loggingReplicator.StartDrainableBackupPhase();
            try
            {
                var replicatorLogBackupFileName = Path.Combine(
                    replicatorBackupFolder,
                    Replicator.Constants.ReplicatorBackupLogName);

                // Sets SetSequentialAccessReadSize.
                if (backupOption == BackupOption.Full)
                {
                    backupLogFile = await BackupLogFile.CreateAsync(
                                replicatorLogBackupFileName,
                                logRecordsAsyncEnumerator,
                                this.lastCompletedBackupLogRecord,
                                cancellationToken).ConfigureAwait(false);
                }
                else if (backupOption == BackupOption.Incremental)
                {
                    var incrementalBackupEnumerator = new IncrementalBackupLogRecordAsyncEnumerator(
                        this.traceType,
                        backupId,
                        logRecordsAsyncEnumerator,
                        this.lastCompletedBackupLogRecord,
                        this.loggingReplicator);

                    backupLogFile = await BackupLogFile.CreateAsync(
                        replicatorLogBackupFileName,
                        incrementalBackupEnumerator,
                        this.lastCompletedBackupLogRecord,
                        cancellationToken).ConfigureAwait(false);
                }

                FabricEvents.Events.BackupReplicatorCompleted(
                    this.traceType,
                    backupId,
                    backupLogFile.IndexingRecordLsn.LSN,
                    backupLogFile.LastBackedUpEpoch.DataLossNumber,
                    backupLogFile.LastBackedUpEpoch.ConfigurationNumber,
                    backupLogFile.LastBackedUpLsn.LSN,
                    backupLogFile.Count,
                    backupLogFile.SizeInKB,
                    stopwatch.ElapsedMilliseconds);
            }
            finally
            {
                this.loggingReplicator.CompleteDrainableBackupPhase();
            }

            // #7528220: Make sure that accumulatedSize is reset if this is a Full Backup.
            long accumulatedSize = backupOption == BackupOption.Full ? 
                backupLogFile.Size : 
                (this.lastCompletedBackupLogRecord.BackupLogSizeInKB * Replicator.Constants.NumberOfBytesInKBytes) + backupLogFile.Size;

            return new ReplicatorBackup(
                backupLogFile.IndexingRecordEpoch,
                backupLogFile.IndexingRecordLsn,
                backupLogFile.LastBackedUpEpoch,
                backupLogFile.LastBackedUpLsn,
                backupLogFile.Count,
                backupLogFile.Size,
                accumulatedSize);
        }

        /// <summary>
        /// Backup the state manager and get the physical log restoreLogStream under the backupConsistencyLock.
        /// </summary>
        /// <param name="replicatedLogManager">Replicated log manager.</param>
        /// <param name="backupId">BackupId.</param>
        /// <param name="stateManagerBackupFolderPath">State Manager Backup Folder Path</param>
        /// <param name="backupOption">The backup option.</param>
        /// <param name="timeout">Timeout for the locks.</param>
        /// <param name="cancellationToken">The cancellation token.</param>
        /// <returns>Restore physical log reader that has the logs to be backed up.</returns>
        internal async Task<IAsyncEnumerator<LogRecord>> BackupStateManagerAndGetReplicatorLogsToBeBackedUpAsync(
            IReplicatedLogManager replicatedLogManager,
            Guid backupId,
            string stateManagerBackupFolderPath,
            BackupOption backupOption,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            var lockTakerName = backupId.ToString();
            var stopwatch = new Stopwatch();

            IAsyncEnumerator<LogRecord> physicalLogReader = null;

            var backupConsistencyLockIsAcquired = await this.loggingReplicator.AcquireBackupAndCopyConsistencyLockAsync(
                lockTakerName, 
                timeout, 
                cancellationToken).ConfigureAwait(false);
            if (false == backupConsistencyLockIsAcquired)
            {
                throw new TimeoutException(SR.Error_Timeout);
            }

            try
            {
                // Note that we must throw within try to avoid leak of backupAndCopyLock.
                timeout = this.ThrowIfBackupTimeoutExceeded(stopwatch.Elapsed, timeout);

                // Can throw FabricNotPrimaryException
                this.loggingReplicator.StartDrainableBackupPhase();

                try
                {
                    // Mark the low water mark to copy logs from.
                    var indexingLogRecord = this.GetIndexingLogRecordForFullBackup(replicatedLogManager);

                    var stateManagerApiLockIsAcquired = await this.loggingReplicator.AcquireStateManagerApiLockAsync(
                                lockTakerName,
                                timeout,
                                cancellationToken).ConfigureAwait(false);
                    if (false == stateManagerApiLockIsAcquired)
                    {
                        throw new TimeoutException(SR.Error_Lock);
                    }

                    try
                    {
                        stopwatch.Start();

                        // Backup the state manager first. 
                        // This is important as otherwise we will not have the logs to make the state provider consistent.
                        await this.stateManager.BackupAsync(
                            stateManagerBackupFolderPath, 
                            backupOption, 
                            cancellationToken).ConfigureAwait(false);

                        stopwatch.Stop();
                        FabricEvents.Events.BackupAsync(
                            this.traceType,
                            backupId,
                            "Checkpoints backed up in (ms) " + stopwatch.ElapsedMilliseconds);
                    }
                    finally
                    {
                        this.loggingReplicator.ReleaseStateManagerApiLock(lockTakerName);
                    }

                    physicalLogReader = this.loggingReplicator.GetLogRecordsCallerHoldsLock(
                        indexingLogRecord,
                        lockTakerName,
                        LogManager.LogReaderType.Backup);
                }
                finally
                {
                    this.loggingReplicator.CompleteDrainableBackupPhase();
                }
            }
            catch (Exception e)
            {
                var failedCallbackTraceMessage =
                    string.Format(
                        CultureInfo.InvariantCulture,
                        "BackupAsync {0}: StateManager.BackupAsync failed with {1}. {2}",
                        backupId,
                        e.GetType().ToString(),
                        e.ToString());

                FabricEvents.Events.BackupAsync(this.traceType, backupId, failedCallbackTraceMessage);

                throw;
            }
            finally
            {
                this.loggingReplicator.ReleaseBackupAndCopyConsistencyLock(lockTakerName);
            }

            return physicalLogReader;
        }

        /// <summary>
        /// Call the backup callback.
        /// </summary>
        /// <param name="backupId">Backup id.</param>
        /// <param name="backupInfo">Backup info.</param>
        /// <param name="backupCallbackAsync">The callback.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        internal async Task<bool> CallBackupCallbackAsync(
            Guid backupId, 
            BackupInfo backupInfo, 
            Func<BackupInfo, CancellationToken, Task<bool>> backupCallbackAsync,
            CancellationToken cancellationToken)
        {
            var result = false;
            FabricApiCallDescription apiCallDescription = null;

            this.loggingReplicator.StartDrainableBackupPhase();

            try
            {
                apiCallDescription = this.ApiMonitor.StartBackupCallbackAsyncMonitoring();

                result = await backupCallbackAsync.Invoke(backupInfo, cancellationToken).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                FabricEvents.Events.BackupException(
                    this.traceType,
                    backupId,
                    e.GetType().ToString(),
                    e.Message,
                    e.StackTrace);

                throw;
            }
            finally
            {
                this.ApiMonitor.StopBackupMonitoring(apiCallDescription);

                this.loggingReplicator.CompleteDrainableBackupPhase();
            }

            return result;
        }

        internal bool CheckIfRestoreTokenExists()
        {
            return FabricFile.Exists(this.GetRestoreTokenFilePath());
        }

        internal void CreateRestoreToken()
        {
            if (false == FabricDirectory.Exists(this.GetRestoreTokenFolderPath()))
            {
                FabricDirectory.CreateDirectory(this.GetRestoreTokenFolderPath());
            }

            // FabricFile.Open returns a stream that needs to be disposed.
            using (FabricFile.Open(this.GetRestoreTokenFilePath(), FileMode.CreateNew, FileAccess.Write))
            {
                // Using to dispose the stream.
            }

            // TODO: Get own structured trace.
            FabricEvents.Events.Lock(this.traceType, "RestoreToken", "Create", this.GetRestoreTokenFilePath());
        }

        internal void DeleteRestoreToken()
        {
            // TODO: Get own structured trace.
            FabricEvents.Events.Lock(
                this.traceType,
                "RestoreToken",
                "Delete",
                this.GetRestoreTokenFilePath() + " " + this.testFlagNoOpDeleteRestoreToken);

            if (this.testFlagNoOpDeleteRestoreToken == false)
            {
                FabricFile.Delete(this.GetRestoreTokenFilePath());
            }
        }

        internal string GetLocalBackupFolderPath()
        {
            return Path.Combine(
                this.initializationParameters.CodePackageActivationContext.WorkDirectory,
                this.initializationParameters.PartitionId.ToString(),
                this.initializationParameters.ReplicaId.ToString(),
                Replicator.Constants.LocalBackupFolderName);
        }

        internal string GetLocalBackupReplicatorFilePath()
        {
            return Path.Combine(this.GetLocalBackupReplicatorFolderPath(), Replicator.Constants.ReplicatorBackupLogName);
        }

        internal string GetLocalBackupReplicatorFolderPath()
        {
            return Path.Combine(this.GetLocalBackupFolderPath(), Replicator.Constants.ReplicatorBackupFolderName);
        }

        internal string GetLocalBackupStateManagerFolderPath()
        {
            return Path.Combine(this.GetLocalBackupFolderPath(), Replicator.Constants.StateManagerBackupFolderName);
        }

        internal virtual string GetRestoreTokenFilePath()
        {
            return Path.Combine(this.GetRestoreTokenFolderPath(), Replicator.Constants.RestoreTokenName);
        }

        internal virtual string GetRestoreTokenFolderPath()
        {
            return Path.Combine(
                this.initializationParameters.CodePackageActivationContext.WorkDirectory,
                Replicator.Constants.RestoreFolderName,
                this.initializationParameters.PartitionId.ToString(),
                this.initializationParameters.ReplicaId.ToString());
        }

        internal TimeSpan ParseTimeout(TimeSpan timeout)
        {
            if (timeout.TotalMilliseconds > int.MaxValue)
            {
                throw new ArgumentOutOfRangeException(SR.Error_Timeout);
            }
            else if (timeout.Equals(Timeout.InfiniteTimeSpan))
            {
                return timeout;
            }
            else if (timeout.TotalMilliseconds < 0)
            {
                throw new ArgumentOutOfRangeException(SR.Error_Timeout);
            }

            return timeout;
        }

        /// <summary>
        /// Release the backup lock.
        /// </summary>
        /// <param name="backupId">Backup id.</param>
        internal void ReleaseBackupLock(Guid backupId)
        {
            // Allow new backup calls to come in.
            // Note assignment would be safe here.
            var previousValue = Interlocked.CompareExchange(ref this.isBackupInProgress, value: 0, comparand: 1);

            FabricEvents.Events.Lock(this.traceType, "backup", backupId.ToString(), "Released");

            Utility.Assert(previousValue == 1, "Lock must have been taken.");
        }

        /// <summary>
        /// Release the restore lock.
        /// </summary>
        /// <param name="restoreId">Restore id.</param>
        internal void ReleaseRestoreLock(Guid restoreId)
        {
            // Allow new backup calls to come in.
            // Note assignment would be safe here.
            var previousValue = Interlocked.CompareExchange(ref this.isRestoreInProgress, value: 0, comparand: 1);

            FabricEvents.Events.Lock(this.traceType, "restore", restoreId.ToString(), "Released");

            Utility.Assert(previousValue == 1, "Lock must have been taken.");
        }

        /// <summary>
        /// Write the backup metadata file.
        /// </summary>
        /// <param name="backupMetadataFilePath">File to write the backup metadata file to.</param>
        /// <param name="backupOption">The backup option.</param>
        /// <param name="parentBackupId"></param>
        /// <param name="backupId">The backup id.</param>
        /// <param name="partitionId">The partition id.</param>
        /// <param name="replicaId">The replica id.</param>
        /// <param name="startingEpoch"></param>
        /// <param name="startingLsn"></param>
        /// <param name="backupEpoch">The backup Epoch.</param>
        /// <param name="backupLsn">The backup logical sequence number.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        internal async Task<BackupMetadataFile> WriteBackupMetadataFile(
            string backupMetadataFilePath,
            BackupOption backupOption,
            Guid parentBackupId,
            Guid backupId,
            Guid partitionId,
            long replicaId,
            Epoch startingEpoch,
            long startingLsn,
            Epoch backupEpoch,
            long backupLsn,
            CancellationToken cancellationToken)
        {
            this.loggingReplicator.StartDrainableBackupPhase();

            try
            {
                return await BackupMetadataFile.CreateAsync(
                            backupMetadataFilePath,
                            backupOption,
                            parentBackupId,
                            backupId,
                            partitionId,
                            replicaId,
                            startingEpoch,
                            startingLsn,
                            backupEpoch,
                            backupLsn,
                            cancellationToken).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                var failedCallbackTraceMessage =
                    string.Format(
                        CultureInfo.InvariantCulture,
                        "BackupAsync {0}: BackupMetadataFile.CreateAsync failed with {1}. {2}",
                        backupId,
                        e.GetType().ToString(),
                        e.ToString());

                FabricEvents.Events.BackupAsync(this.traceType, backupId, failedCallbackTraceMessage);

                throw;
            }
            finally
            {
                this.loggingReplicator.CompleteDrainableBackupPhase();
            }
        }

        private Guid GenerateBackupId(BackupOption backupOption)
        {
            while (true)
            {
                var result = Guid.NewGuid();

                if (result.Equals(BackupLogRecord.InvalidBackupId))
                {
                    continue;
                }

                return result;
            }
        }

        private void ThrowIfNotSafe(RestorePolicy restorePolicy, BackupFolderInfo backupFolderInfo, Guid restoreId)
        {
            if (RestorePolicy.Safe != restorePolicy)
            {
                return;
            }

            this.AcquireRestoreLock(restoreId);
            try
            {
                var currentLogTailLsn = this.loggingReplicator.CurrentLogTailLsn;

                var epochOfTheLastLogicalLogRecord = this.loggingReplicator.GetEpoch(currentLogTailLsn);

                if (epochOfTheLastLogicalLogRecord > backupFolderInfo.HighestBackedupEpoch ||
                    currentLogTailLsn.LSN >= backupFolderInfo.HighestBackedupLsn)
                {
                    var message = string.Format(
                        CultureInfo.InvariantCulture,
                        "Current {0} {1}. BackupFolder: {2} {3}",
                        Utility.ToString(epochOfTheLastLogicalLogRecord),
                        currentLogTailLsn,
                        Utility.ToString(backupFolderInfo.HighestBackedupEpoch),
                        backupFolderInfo.HighestBackedupLsn);

                    FabricEvents.Events.RestoreException(
                        this.traceType,
                        Guid.Empty,
                        "ArgumentException",
                        SR.Error_RestoreUnSafe,
                        message);
                    throw new ArgumentException(SR.Error_RestoreUnSafe);
                }
            }
            catch (Exception e)
            {
                FabricEvents.Events.RestoreException(
                    this.traceType,
                    restoreId,
                    e.GetType().ToString(),
                    e.Message,
                    e.StackTrace);

                throw;
            }
            finally
            {
                this.ReleaseRestoreLock(restoreId);
            }
        }

        private void ThrowIfInvalidState(Guid restoreId)
        {
            this.AcquireRestoreLock(restoreId);
            try
            {
                bool isTokenExists = this.CheckIfRestoreTokenExists();
                if (isTokenExists)
                {
                    // One consideration was to ReportFault here.
                    // However, as long as customer does not return from OnDataLossAsync, Reporting fault will not help.
                    throw new InvalidOperationException("A non-retryable RestoreAsync exception has been retried.");
                }
            }
            catch (Exception e)
            {
                FabricEvents.Events.RestoreException(
                    this.traceType,
                    restoreId,
                    e.GetType().ToString(),
                    e.Message,
                    e.StackTrace);

                throw;
            }
            finally
            {
                this.ReleaseRestoreLock(restoreId);
            }
        }

        private string GenerateBackupMetadataName(string backupFolder, BackupOption backupOption)
        {
            if (backupOption == BackupOption.Incremental)
            {
                return Path.Combine(backupFolder, Replicator.Constants.ReplicatorIncrementalBackupMetadataFileName);
            }

            return Path.Combine(backupFolder, Replicator.Constants.ReplicatorBackupMetadataFileName);
        }

        private async Task<IAsyncEnumerator<LogRecord>> GetIncrementalLogRecordsAsync(string lockTakerName)
        {
            // Can throw FabricNotPrimaryException
            // #10635451: Not making this part drainable causes null reference since
            // log manager may be deleted while this function is running.
            this.loggingReplicator.StartDrainableBackupPhase();

            try
            {
                var lastBackupLsn = this.lastCompletedBackupLogRecord.HighestBackedUpLsn;

                IAsyncEnumerator<LogRecord> logRecords = await this.loggingReplicator.GetLogRecordsAsync(
                    lastBackupLsn,
                    lockTakerName,
                    LogManager.LogReaderType.Backup).ConfigureAwait(false);
                Utility.Assert(
                    logRecords != null && logRecords.Current.Lsn == lastBackupLsn, 
                    "{0} Invalid log records returned from GetLogRecordsAsync. lastBackupLsn: {1}", 
                    this.traceType,
                    lastBackupLsn);

                return logRecords;
            }
            finally
            {
                this.loggingReplicator.CompleteDrainableBackupPhase();
            }
        }

        private async Task PrepareBackupFolderAsync(BackupOption option, string backupFolder, CancellationToken cancellationToken)
        {
            try
            {
                FabricDirectory.Delete(backupFolder, true);
            }
            catch (DirectoryNotFoundException)
            {
                // Already empty
            }

            int retryCount = 0;
            while (true)
            {
                cancellationToken.ThrowIfCancellationRequested();

                try
                {
                    if (option == BackupOption.Full)
                    {
                        var stateManagerBackupFolderPath = this.GetLocalBackupStateManagerFolderPath();
                        if (FabricDirectory.Exists(stateManagerBackupFolderPath) == false)
                        {
                            FabricDirectory.CreateDirectory(stateManagerBackupFolderPath);
                        }
                    }

                    var replicatorBackupFolderPath = this.GetLocalBackupReplicatorFolderPath();
                    if (FabricDirectory.Exists(replicatorBackupFolderPath) == false)
                    {
                        FabricDirectory.CreateDirectory(replicatorBackupFolderPath);
                    }

                    return;
                }
                catch (UnauthorizedAccessException)
                {
                    if (retryCount == MaxRetryCount)
                    {
                        throw;
                    }

                    retryCount++;
                }

                // MCoskun: Because the starting backoff time is high, use linear backoff instead of exponential.
                await Task.Delay(StartingBackoffInMilliSeconds * retryCount).ConfigureAwait(false);
            }
        }

        private void ThrowIfAccumulatedIncrementalBackupLogsIsTooLarge(
            TransactionalReplicatorSettings settings,
            BackupOption backupOption,
            Guid backupId,
            ReplicatorBackup replicatorBackupInfo)
        {
            if (backupOption != BackupOption.Incremental)
            {
                return;
            }

            int? snapOfMaxStreamSize = settings.PublicSettings.MaxStreamSizeInMB.Value;
            Utility.Assert(
                snapOfMaxStreamSize != null,
                "At this point ReplicatorSettings.CheckpointThresholdInMB cannot be null");

            if (replicatorBackupInfo.AccumulatedLogSizeInMB >= snapOfMaxStreamSize)
            {
                var exceptionMessage = string.Format(
                    CultureInfo.CurrentCulture,
                    SR.Error_AccumulatedBackupLogTooLarge,
                    backupId,
                    replicatorBackupInfo.AccumulatedLogSizeInMB,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MaxStreamSizeInMB,
                    snapOfMaxStreamSize);
                throw new FabricMissingFullBackupException(exceptionMessage);
            }

            var checkpointThresholdInMb = settings.PublicSettings.CheckpointThresholdInMB;
            Utility.Assert(
                checkpointThresholdInMb != null,
                "At this point ReplicatorSettings.CheckpointThresholdInMB cannot be null");

            var maxAccumulatedBackupLogSizeInMB = settings.PublicSettings.MaxAccumulatedBackupLogSizeInMB;
            Utility.Assert(
                maxAccumulatedBackupLogSizeInMB != null,
                "At this point ReplicatorSettings.MaxAccumulatedBackupLogSizeInMB cannot be null");

            if (replicatorBackupInfo.AccumulatedLogSizeInMB >= maxAccumulatedBackupLogSizeInMB)
            {
                var exceptionMessage = string.Format(
                    CultureInfo.CurrentCulture,
                    SR.Error_AccumulatedBackupLogTooLarge,
                    backupId,
                    replicatorBackupInfo.AccumulatedLogSizeInMB,
                    ReliableStateManagerReplicatorSettingsConfigurationNames.MaxAccumulatedBackupLogSizeInMB,
                    maxAccumulatedBackupLogSizeInMB);
                throw new FabricMissingFullBackupException(exceptionMessage);
            }
        }

        // Throws if timeout has been exceeded during backup async
        // If time is remaining, returns decremented timeout value
        private TimeSpan ThrowIfBackupTimeoutExceeded(TimeSpan elapsedSpan, TimeSpan timeout)
        {
            if (timeout.Equals(Timeout.InfiniteTimeSpan))
            {
                return timeout;
            }

            TimeSpan remainingTimeout = timeout - elapsedSpan;

            if (elapsedSpan >= timeout)
            {
                throw new TimeoutException(SR.Error_BackupTimeoutExceeded);
            }

            return remainingTimeout;
        }

        /// <summary>
        /// Method that detects whether incremental backup can be taken according to chaining rules.
        /// </summary>
        /// <param name="settings">Transactional replicator settings.</param>
        /// <param name="progressVector">The progress vector.</param>
        /// <param name="backupOption">The backup option.</param>
        /// <param name="backupId">The backup id.</param>
        /// <devnote>
        /// Caller holds backup lock.
        /// </devnote>
        private void ThrowIfFullBackupIsMissing(
            TransactionalReplicatorSettings settings, 
            ProgressVector progressVector, 
            BackupOption backupOption, 
            Guid backupId)
        {
            // No reason to check if the backup request is full.
            if (backupOption == BackupOption.Full)
            {
                return;
            }

            Epoch snapOfCurrentEpoch = this.loggingReplicator.CurrentLogTailEpoch;
            BackupLogRecord snapLastCompeltedBackupLogRecord = this.lastCompletedBackupLogRecord;

            // If the partition has not take a full backup yet, then incremental backup cannot be allowed.
            if (snapLastCompeltedBackupLogRecord.IsZeroBackupLogRecord())
            {
                string message = string.Format(
                    CultureInfo.CurrentCulture,
                    SR.Error_FullBackupRequiredInThisEpoch,
                    backupId,
                    snapOfCurrentEpoch);
                throw new FabricMissingFullBackupException(message);
            }

            // If the incremental backup request has been made in the same epoch as the previous backup,
            // Then allow the incremental backup.
            if (snapOfCurrentEpoch.Equals(snapLastCompeltedBackupLogRecord.HighestBackedUpEpoch))
            {
                return;
            }

            if (settings.EnableIncrementalBackupsAcrossReplicas == true)
            {
                // If the incremental backup request has been made in different data loss number as the previous backup, 
                // do not allow the incremental backup.
                // Allowing this case increases complexities in terms of potentially possible restore folders. 
                // E.g.
                // 1->2->3->4
                //    |->5->6
                // Note Epoch at incremental backup 4 could be ahead of 6 even though 6 was taken at a later time.
                // This is because cluster which 2 is restored back to before starting the incremental backup 5 only verifies that new data loss number
                // is greater than 2 not 4.
                // Note that progress vector may have been trimmed. If trimmed, than we cannot do the incremental backup.
                ProgressVectorEntry vectorEntry = progressVector.Find(snapLastCompeltedBackupLogRecord.HighestBackedUpEpoch);
                if (vectorEntry != null && snapOfCurrentEpoch.DataLossNumber == snapLastCompeltedBackupLogRecord.HighestBackedUpEpoch.DataLossNumber)
                {
                    return;
                }
            }

            string exceptionMessage = string.Format(
                CultureInfo.CurrentCulture,
                SR.Error_FullBackupRequiredInThisEpoch,
                backupId,
                snapOfCurrentEpoch);
            throw new FabricMissingFullBackupException(exceptionMessage);
        }

        /// <summary>
        /// Gets the indexing log record which will be starting record for the full backup.
        /// </summary>
        /// <returns>
        /// The first indexing log record before the last pending being transaction of the last completed checkpoint.
        /// </returns>
        /// <remarks>
        /// Caller holds backup and copy consistency lock.
        /// </remarks>
        /// <devnote>
        /// Algorithm:
        /// 1. Find the last completed begin checkpoint and the relevant earliest pending transaction.
        /// 2. Use the backwards physical log record link to find the first indexing log record less than earliest pending transaction.
        /// </devnote>
        private IndexingLogRecord GetIndexingLogRecordForFullBackup(IReplicatedLogManager replicatedLogManager)
        {
            // Stage 1: Find the last completed begin checkpoint and the relevant earliest pending transaction.
            EndCheckpointLogRecord lastCompeltedEndCheckpointLogRecord = replicatedLogManager.LastCompletedEndCheckpointRecord;
            Utility.Assert(
                lastCompeltedEndCheckpointLogRecord != null && LogRecord.IsInvalidRecord(lastCompeltedEndCheckpointLogRecord) == false, 
                "{0} EndCheckpointLogRecord is always expected to be valid.", 
                this.traceType);

            BeginCheckpointLogRecord completedBeginCheckpointRecord = lastCompeltedEndCheckpointLogRecord.LastCompletedBeginCheckpointRecord;
            Utility.Assert(
                completedBeginCheckpointRecord != null && LogRecord.IsInvalidRecord(completedBeginCheckpointRecord) == false,
                "{0} BeginCheckpointLogRecord is always expected to be valid.",
                this.traceType);

            IndexingLogRecord previousIndexingRecord = null;
            PhysicalLogRecord previousPhysicalRecord = completedBeginCheckpointRecord;
            BeginTransactionOperationLogRecord beginTransactionLogRecord = completedBeginCheckpointRecord.EarliestPendingTransaction;

            // Stage 2: Use the backwards physical log record link to find the first indexing log record less than earliest pending transaction.
            do
            {
                // Search back for the last indexing log record.
                do
                {
                    PhysicalLogRecord tmp = previousPhysicalRecord;
                    previousPhysicalRecord = previousPhysicalRecord.PreviousPhysicalRecord;

                    // This method must be called under the backup and copy consistency lock.
                    // Because of this a racing truncate cannot happen between stage 1 and stage 2.
                    // In this light, below code asserts that there is an indexing record with LSN < EarliestPendingTransaction
                    // below the LastCompletedBeginCheckpointRecord.
                    Utility.Assert(
                        LogRecord.IsInvalidRecord(previousPhysicalRecord) == false,
                        "{0} Physical record before PSN {1} Position {2} must not be invalid.",
                        this.traceType,
                        tmp.Psn,
                        tmp.RecordPosition);
                } while (!(previousPhysicalRecord is IndexingLogRecord));

                previousIndexingRecord = previousPhysicalRecord as IndexingLogRecord;

                // Check to make sure that the indexing record is older that the earliest pending begin transaction record.
                // If there is no pending begin transaction record, we are done since we started scanning back from last completed begin checkpoint.
                if (beginTransactionLogRecord == null)
                {
                    break;
                }

                // If indexing record is not old enough, keep scanning back.
                if (previousIndexingRecord.RecordPosition >= beginTransactionLogRecord.RecordPosition)
                {
                    continue;
                }

                break;
            } while (true);

            Utility.Assert(
                previousIndexingRecord != null,
                "{0} Previous Indexing Record to start the backup from must not be null", 
                this.traceType);

            return previousIndexingRecord;
        }

        private void DeleteFolderIfExists(string folder)
        {
            if (FabricDirectory.Exists(folder) == false)
            {
                return;
            }

            try
            {
                FabricDirectory.Delete(folder, true, true);
            }
            catch (Exception e)
            {
                FabricEvents.Events.BackupException(
                    this.traceType,
                    Guid.Empty,
                    folder,
                    e.Message,
                    e.StackTrace);

                // Failing this call is not fatal but it would cause the folder to leak.
                throw;
            }
        }
    }
}
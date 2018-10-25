// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator.Backup
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;

    internal interface IBackupManager
    {
        /// <summary>
        /// Indicates whether the log is unavailable due to restore.
        /// </summary>
        bool IsLogUnavailableDueToRestore { get; }

        /// <summary>
        /// Gets and sets the last completed log record.
        /// </summary>
        BackupLogRecord LastCompletedBackupLogRecord { get; set; }

        /// <summary>
        /// Notification that the last completed backup log record has been false progressed.
        /// </summary>
        void UndoLastCompletedBackupLogRecord();

        /// <summary>
        /// Sets the variable indicating that the delete restore token should be no-op.
        /// This ensures that during the next open, replica thinks the previous restore was not complete.
        /// </summary>
        bool TestFlagNoOpDeleteRestoreToken { set; }

        /// <summary>
        /// Deletes system maintained local backup folder if it already exists.
        /// </summary>
        void DeletePersistedState();

        /// <summary>
        /// Creates a backup of the transactional replicator and all registered state providers.
        /// The default backupOption is FULL.
        /// The default timeout is 1 hour.
        /// The default cancellationToken is None.
        /// </summary>
        /// <param name="replicatedLogManager">Replicated log manager.</param>
        /// <param name="checkpointManager">Checkpoint Manager.</param>
        /// <param name="backupCallback">Method that will be called once the local backup folder has been created.</param>
        /// <returns>Task that represents the asynchronous backup.</returns>
        Task<BackupInfo> BackupAsync(
            IReplicatedLogManager replicatedLogManager,
            ICheckpointManager checkpointManager,
            Func<BackupInfo, CancellationToken, 
            Task<bool>> backupCallback);

        /// <summary>
        /// Creates a backup of the transactional replicator and all registered state providers.
        /// </summary>
        /// <param name="replicatedLogManager">Replicated Log Manager</param>
        /// <param name="checkpointManager">Checkpoint Manager.</param>
        /// <param name="backupCallback">Method that will be called once the local backup folder has been created.</param>
        /// <param name="backupOption">The backup option. </param>
        /// <param name="timeout">The timeout for the backup.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests. </param>
        /// <returns>Task that represents the asynchronous backup.</returns>
        Task<BackupInfo> BackupAsync(
            IReplicatedLogManager replicatedLogManager,
            ICheckpointManager checkpointManager,
            Func<BackupInfo, CancellationToken, Task<bool>> backupCallback, 
            BackupOption backupOption, 
            TimeSpan timeout, 
            CancellationToken cancellationToken);

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
        Task RestoreAsync(string backupFolder);

        /// <summary>
        /// Restores the transactional replicator from the backup folder.
        /// </summary>
        /// <param name="backupFolder">
        /// The directory where the replica is to be restored from.
        /// This parameter cannot be null, empty or contain just whitespace. 
        /// UNC paths may also be provided.
        /// </param>
        /// <param name="restorePolicy">The restore policy.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represents the asynchronous restore.</returns>
        Task RestoreAsync(string backupFolder, RestorePolicy restorePolicy, CancellationToken cancellationToken);

        /// <summary>
        /// Return boolean indicating whether LoggingReplicator should delete its persisted state.
        /// </summary>
        /// <returns>Boolean indicating whether LoggingReplicator should delete its persisted state.</returns>
        bool ShouldCleanState();

        /// <summary>
        /// Informs backup that cleaning has been completed.
        /// </summary>
        void StateCleaningComplete();
    }
}
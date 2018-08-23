// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Fabric;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Data.Notifications;
    using ReplicatorTransaction = Microsoft.ServiceFabric.Replicator.Transaction;

    /// <summary>
    /// Interface for replicator.
    /// </summary>
    internal interface ILoggingReplicator : ITransactionVersionManager, IVersionManager
    {
        /// <summary>
        /// Gets o sets Fabric replicator
        /// </summary>
        IStateReplicator FabricReplicator { get; set; }

        /// <summary>
        /// Gets a value indicating whether the replicator is snapshot readable.
        /// </summary>
        bool IsReadable { get; }

        /// <summary>
        /// Gets or sets replica role.
        /// </summary>
        ReplicaRole Role { get; }

        /// <summary>
        /// Gets or sets tracer.
        /// </summary>
        ITracer Tracer { get; set; }

        /// <summary>
        /// Gets the ApiMonitor
        /// </summary>
        ReplicatorApiMonitor ApiMonitor { get; }

        /// <summary>
        /// Creates a backup of the transactional replicator and all registered state providers.
        /// The default backupOption is FULL.
        /// The default timeout is 1 hour.
        /// The default cancellationToken is None.
        /// </summary>
        /// <param name="backupCallback">Method that will be called once the local backup folder has been created.</param>
        /// <returns>Task that represents the asynchronous backup.</returns>
        Task<BackupInfo> BackupAsync(Func<BackupInfo, CancellationToken, Task<bool>> backupCallback);

        /// <summary>
        /// Creates a backup of the transactional replicator and all registered state providers.
        /// </summary>
        /// <param name="backupCallback">Method that will be called once the local backup folder has been created.</param>
        /// <param name="backupOption">The backup option. </param>
        /// <param name="timeout">The timeout for the backup.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests. </param>
        /// <returns>Task that represents the asynchronous backup.</returns>
        Task<BackupInfo> BackupAsync(
            Func<BackupInfo, CancellationToken, Task<bool>> backupCallback, 
            BackupOption backupOption, 
            TimeSpan timeout, 
            CancellationToken cancellationToken);

        /// <summary>
        /// Creates a barrier between operations.
        /// </summary>
        Task BarrierAsync();

        /// <summary>
        /// Change role to active secondary.
        /// </summary>
        Task BecomeActiveSecondaryAsync();

        /// <summary>
        /// Change role to idle secondary.
        /// </summary>
        Task BecomeIdleSecondaryAsync();

        /// <summary>
        /// Change role to none.
        /// </summary>
        Task BecomeNoneAsync();

        /// <summary>
        /// Change role to primary.
        /// </summary>
        Task BecomePrimaryAsync();

        /// <summary>
        /// Delete log.
        /// </summary>
        Task DeleteLogAsync();

        /// <summary>
        /// Dispose all managed resources.
        /// </summary>
        void Dispose();

        /// <summary>
        /// Gets copy context.
        /// </summary>
        IOperationDataStream GetCopyContext();

        /// <summary>
        /// Gets copy state.
        /// </summary>
        IOperationDataStream GetCopyState(long uptoSequenceNumber, IOperationDataStream copyContext);

        /// <summary>
        /// Gets data loss number
        /// </summary>
        long GetCurrentDataLossNumber();

        /// <summary>
        /// Gets last committed sequence number.
        /// </summary>
        long GetLastCommittedSequenceNumber();

        /// <summary>
        /// Returns the State provider "Remove LSN"
        /// Remove any deleted state provider from the state if the delete LSN is lesser than this returned lsn 
        /// </summary>
        long GetSafeLsnToRemoveStateProvider();

        /// <summary>
        /// Gets last stable logical sequence number.
        /// </summary>
        /// <returns>Last stable logical sequence number.</returns>
        long GetLastStableSequenceNumber();

        /// <summary>
        /// Initializes the logging replicator
        /// </summary>
        /// <param name="context"></param>
        /// <param name="partition"></param>
        /// <returns></returns>
        void Initialize(StatefulServiceContext context, IStatefulServicePartition partition);

        /// <summary>
        /// Called when there data loss in any of the state providers.
        /// </summary>
        Task<bool> OnDataLossAsync(CancellationToken cancellationToken, bool restoredFromBackup);

        /// <summary>
        /// Opens replicator.
        /// </summary>
        Task<RecoveryInformation> OpenAsync(
            ReplicaOpenMode openMode,
            TransactionalReplicatorSettings inputReplicatorSettings);

        /// <summary>
        /// Perform recovery.
        /// </summary>
        Task PerformRecoveryAsync(RecoveryInformation recoveryInformation);

        /// <summary>
        /// Perform clean-up and quiesce operations.
        /// </summary>
        Task PrepareToCloseAsync();

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
        /// Called on epoch change.
        /// </summary>
        Task UpdateEpochAsync(Epoch epoch, long previousEpochLastSequenceNumber, CancellationToken cancellationToken);

        /// <summary>
        /// Updates replicator settings.
        /// </summary>
        void UpdateReplicatorSettings(ReliableStateManagerReplicatorSettings updatedSettings);
    }
}
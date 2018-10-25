// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System;
    using System.Fabric;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Defines methods a reliable state provider replica must implement for Service Fabric to interact with it.
    /// </summary>
    public interface IStateProviderReplica
    {
        /// <summary>
        /// Function called during suspected data-loss.
        /// </summary>
        /// <value>
        /// Function called as part of suspected data loss processing.
        /// Function takes in CancellationToken and need to return a Task that represents the asynchronous processing of the event.
        /// Returning true, indicates that the replica's state has been restored.
        /// False indicates that the replica's state has not been changed.
        /// </value>
        Func<CancellationToken, Task<bool>> OnDataLossAsync { set; }

        /// <summary>
        /// Initialize the state provider replica using the service initialization information.
        /// </summary>
        /// <remarks>
        /// No complex processing should be done during Initialize. Expensive or long-running initialization should be done in OpenAsync.
        /// </remarks>
        /// <param name="initializationParameters">Service initialization information such as service name, partition id, replica id, and code package information.</param>
        void Initialize(StatefulServiceInitializationParameters initializationParameters);

        /// <summary>
        /// Open the state provider replica for use.
        /// </summary>
        /// <remarks>
        /// Extended state provider initialization tasks can be started at this time.
        /// </remarks>
        /// <param name="openMode">Indicates whether this is a new or existing replica.</param>
        /// <param name="partition">The partition this replica belongs to.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// Task that represents the asynchronous open operation. The result contains the replicator
        /// responsible for replicating state between other state provider replicas in the partition.
        /// </returns>
        Task<IReplicator> OpenAsync(ReplicaOpenMode openMode, IStatefulServicePartition partition,
            CancellationToken cancellationToken);

        /// <summary>
        /// Notify the state provider replica that its role is changing, for example to Primary or Secondary.
        /// </summary>
        /// <param name="newRole">The new replica role, such as primary or secondary.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represents the asynchronous change role operation.</returns>
        Task ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken);

        /// <summary>
        /// Gracefully close the state provider replica.
        /// </summary>
        /// <remarks>
        /// This generally occurs when the replica's code is being upgrade, the replica is being moved
        /// due to load balancing, or a transient fault is detected.
        /// </remarks>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represents the asynchronous close operation.</returns>
        Task CloseAsync(CancellationToken cancellationToken);

        /// <summary>
        /// Forcefully abort the state provider replica.
        /// </summary>
        /// <remarks>
        /// This generally occurs when a permanent fault is detected on the node, or when
        /// Service Fabric cannot reliably manage the replica's life-cycle due to internal failures.
        /// </remarks>
        void Abort();

        /// <summary>
        /// Performs a full backup of all reliable state managed by this <see cref="IReliableStateManager"/>.
        /// </summary>
        /// <param name="backupCallback">Callback to be called when the backup folder has been created locally and is ready to be moved out of the node.</param>
        /// <returns>Task that represents the asynchronous backup operation.</returns>
        /// <remarks>
        /// A FULL backup will be performed with a one-hour timeout.
        /// Boolean returned by the backupCallback indicate whether the service was able to successfully move the backup folder to an external location.
        /// If false is returned, BackupAsync throws InvalidOperationException with the relevant message indicating backupCallback returned false.
        /// Also, backup will be marked as unsuccessful.
        /// </remarks>
        Task BackupAsync(Func<BackupInfo, CancellationToken, Task<bool>> backupCallback);

        /// <summary>
        /// Performs a backup of all reliable state managed by this <see cref="IReliableStateManager"/>.
        /// </summary>
        /// <param name="option">The type of backup to perform.</param>
        /// <param name="timeout">The timeout for this operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <param name="backupCallback">Callback to be called when the backup folder has been created locally and is ready to be moved out of the node.</param>
        /// <returns>Task that represents the asynchronous backup operation.</returns>
        /// <remarks>
        /// Boolean returned by the backupCallback indicate whether the service was able to successfully move the backup folder to an external location.
        /// If false is returned, BackupAsync throws InvalidOperationException with the relevant message indicating backupCallback returned false.
        /// Also, backup will be marked as unsuccessful.
        /// </remarks>
        Task BackupAsync(
            BackupOption option,
            TimeSpan timeout,
            CancellationToken cancellationToken,
            Func<BackupInfo, CancellationToken, Task<bool>> backupCallback);

        /// <summary>
        /// Restore a backup taken by <see cref="BackupAsync(Func{BackupInfo, CancellationToken, Task{bool}})"/> or 
        /// <see cref="BackupAsync(BackupOption, TimeSpan, CancellationToken, Func{BackupInfo, CancellationToken, Task{bool}})"/>.
        /// </summary>
        /// <param name="backupFolderPath">
        /// The directory where the replica is to be restored from.
        /// This parameter cannot be null, empty or contain just whitespace. 
        /// UNC paths may also be provided.
        /// </param>
        /// <remarks>
        /// A safe restore will be performed, meaning the restore will only be completed if the data to restore is ahead of state of the current replica.
        /// </remarks>
        /// <returns>Task that represents the asynchronous restore operation.</returns>
        Task RestoreAsync(string backupFolderPath);

        /// <summary>
        /// Restore a backup taken by <see cref="BackupAsync(Func{BackupInfo, CancellationToken, Task{bool}})"/> or 
        /// <see cref="BackupAsync(BackupOption, TimeSpan, CancellationToken, Func{BackupInfo, CancellationToken, Task{bool}})"/>.
        /// </summary>
        /// <param name="restorePolicy">The restore policy.</param>
        /// <param name="backupFolderPath">
        /// The directory where the replica is to be restored from.
        /// This parameter cannot be null, empty or contain just whitespace. 
        /// UNC paths may also be provided.
        /// </param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represents the asynchronous restore operation.</returns>
        Task RestoreAsync(
            string backupFolderPath,
            RestorePolicy restorePolicy,
            CancellationToken cancellationToken);
    }
}
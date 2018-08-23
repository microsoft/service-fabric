// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Threading;
using System.Threading.Tasks;

namespace System.Fabric.BackupRestore
{
    /// <summary>
    /// Interface for backup restore manager which handles automatic backup and restore
    /// </summary>
    internal interface IBackupRestoreManager
    {
        /// <summary>
        /// Initializes the backup restore manager using the service initialization parameters
        /// </summary>
        /// <param name="initializationParameters">Service initialization information such as service name, partition id, replica id, code package information.</param>
        void Initialize(StatefulServiceInitializationParameters initializationParameters);

        /// <summary>
        /// Opens the backup restore manager for use.
        /// </summary>
        /// <param name="openMode">Indicates this is new or existing replica</param>
        /// <param name="partition">The partition this replica belongs to</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represent the asynhronous open operation.</returns>
        Task OpenAsync(ReplicaOpenMode openMode,
            IStatefulServicePartition partition,
            CancellationToken cancellationToken);

        /// <summary>
        /// Notify the backup restore manager that the replica role is changing, for example Primary or Secondary
        /// </summary>
        /// <param name="newRole">The new replica role, such as primary or secondary</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represents asynchronous change role operation</returns>
        Task ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken);

        /// <summary>
        /// Gracefully close the backup restore manager
        /// </summary>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represents the asynchronous close operation.</returns>
        Task CloseAsync(CancellationToken cancellationToken);

        /// <summary>
        /// Forcefully abort the backup restore manager
        /// </summary>
        void Abort();

        /// <summary>
        /// Method invoked when replica suffers a suspected data loss
        /// </summary>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represents the asynchronous on data loss operation</returns>
        Task<bool> OnDataLossAsync(CancellationToken cancellationToken);
    }
}
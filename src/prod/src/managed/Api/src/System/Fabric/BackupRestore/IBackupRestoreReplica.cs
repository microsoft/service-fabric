// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Threading;
using System.Threading.Tasks;

namespace System.Fabric.BackupRestore
{
    /// <summary>
    /// Interface which must be implemented by any stateful replica which supports automatic backup and restore
    /// </summary>
    internal interface IBackupRestoreReplica
    {
        /// <summary>
        /// Method invoked on replica to backup state
        /// </summary>
        /// <param name="backupOption">Whether to take full or incremental backup</param>
        /// <param name="backupCallback">The callback which must be invoked in case backup completes</param>
        /// <param name="cancellationToken">For handling cancellation</param>
        /// <returns>An asynchronous task</returns>
        Task BackupAsync(BackupOption  backupOption, Func<BackupInfo, CancellationToken, Task<bool>> backupCallback, CancellationToken cancellationToken);

        /// <summary>
        /// Method invoked on replica to trigger restore
        /// </summary>
        /// <param name="backupFolderPath">Path of the folder from which to restore state</param>
        /// <param name="forceRestore">Force restore state even if the backup folder contains a state which is older than or equal to the state contained in replica</param>
        /// <param name="cancellationToken">For handling cancellation</param>
        /// <returns>An asynchronous task</returns>
        Task RestoreAsync(string backupFolderPath, bool forceRestore, CancellationToken cancellationToken);

        /// <summary>
        /// Get's the backup metadata which was previously stored using <see cref="SaveBackupMetadataAsync"/> method.
        /// </summary>
        /// <param name="timeout">Timeout for the operation</param>
        /// <param name="cancellationToken">For handling cancellation</param>
        /// <returns>Returns the array of bytes representing the backup metadata asynchronously</returns>
        Task<byte[]> GetBackupMetadataAsync(TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Save the backup metadata persistently
        /// </summary>
        /// <param name="metadata">Backup metadata</param>
        /// <param name="timeout">Timeout for this operation</param>
        /// <param name="cancellationToken">For handling cancellation</param>
        /// <returns>An asynchronous task</returns>
        Task SaveBackupMetadataAsync(byte[] metadata, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Clear / delete the previously saved backup metadata
        /// </summary>
        /// <param name="timeout">Timeout for this operation</param>
        /// <returns>An asynchronous task</returns>
        Task ClearBackupMetadataAsync(TimeSpan timeout);
    }
}
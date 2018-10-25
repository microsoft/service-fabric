// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    /// <summary>
    /// A RestoreDescription contains all of the information necessary to restore a stateful service replica. 
    /// </summary>
    public struct RestoreDescription
    {
        private readonly string backupFolderPath;
        private readonly RestorePolicy restorePolicy;

        /// <summary>
        /// Initializes a new instance of the <cref name="RestoreDescription"/> structure
        /// </summary>
        /// <param name="backupFolderPath">
        /// The directory where the replica is to be restored from.
        /// This parameter cannot be null, empty, or consist only of whitespace. 
        /// UNC paths may also be provided.
        /// </param>
        public RestoreDescription(string backupFolderPath)
        {
            this.backupFolderPath = backupFolderPath;
            this.restorePolicy = RestorePolicy.Safe;
        }

        /// <summary>
        /// Initializes a new instance of the RestoreDescription structure.
        /// </summary>
        /// <param name="backupFolderPath">
        /// The directory where the replica is to be restored from.
        /// This parameter cannot be null, empty, or consist only of whitespace. 
        /// UNC paths may also be provided.
        /// </param>
        /// <param name="restorePolicy">The restore policy.</param>
        public RestoreDescription(string backupFolderPath, RestorePolicy restorePolicy)
        {
            this.backupFolderPath = backupFolderPath;
            this.restorePolicy = restorePolicy;
        }

        /// <summary>
        /// Gets the directory which will be used to restore the replica's state.
        /// This parameter cannot be null, empty, or consist only of whitespace. 
        /// UNC paths may also be provided.
        /// </summary>
        /// <remarks>
        /// Folder must at least contain one full backup.
        /// In addition, it could include one or more incremental backups.
        /// </remarks>
        /// <value>
        /// The directory which will be used to restore the replica's state.
        /// </value>
        public string BackupFolderPath
        {
            get
            {
                return this.backupFolderPath;
            }
        }

        /// <summary>
        /// Gets the restore policy.
        /// </summary>
        /// <value>
        /// Policy to be used for the restore.
        /// </value>
        public RestorePolicy Policy
        {
            get
            {
                return this.restorePolicy;
            }
        }
    }
}
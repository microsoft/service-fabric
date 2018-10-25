// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// A BackupDescription contains all of the information necessary to backup a stateful service replica. 
    /// </summary>
    public struct BackupDescription
    {
        private readonly BackupOption option;
        private readonly Func<BackupInfo, CancellationToken, Task<bool>> backupCallback;

        /// <summary>
        /// Initializes a new instance of the <cref name="BackupDescription"/> structure.
        /// </summary>
        /// <param name="backupCallback">
        /// Callback to be called when the backup folder has been created and populated locally by the system. 
        /// This folder is now ready to be moved out of the node.
        /// </param>
        public BackupDescription(Func<BackupInfo, CancellationToken, Task<bool>> backupCallback)
        {
            this.option = BackupOption.Full;
            this.backupCallback = backupCallback;
        }

        /// <summary>
        /// Initializes a new instance of the <cref name="BackupDescription"/> structure.
        /// </summary>
        /// <param name="option">
        /// The <cref name="BackupOption"/> for the backup.
        /// </param>
        /// <param name="backupCallback">
        /// Callback to be called when the backup folder has been created locally and is ready to be moved out of the node.
        /// </param>
        public BackupDescription(BackupOption option, Func<BackupInfo, CancellationToken, Task<bool>> backupCallback)
        {
            this.option = option;
            this.backupCallback = backupCallback;
        }

        /// <summary>
        /// The type of backup to perform.
        /// </summary>
        /// <value>
        /// The type of the backup.
        /// </value>
        public BackupOption Option
        {
            get
            {
                return this.option;
            }
        }

        /// <summary>
        /// Gets the callback to be called when the backup folder has been created locally and is ready to be moved out of the node.
        /// </summary>
        /// <value>
        /// The backup callback function commonly used to copy the backup folder to an external location.
        /// </value>
        /// <remarks>
        /// Backup callback function takes in BackupInfo and Cancellation token and returns a Task that represents the processing of the backup folder.
        /// Boolean returned by the backupCallback indicate whether the service was able to successfully move the backup folder to an external location.
        /// If false is returned, BackupAsync throws InvalidOperationException with the relevant message indicating backupCallback returned false.
        /// Also, backup will be marked as unsuccessful.
        /// </remarks>
        public Func<BackupInfo, CancellationToken, Task<bool>> BackupCallback
        {
            get
            {
                return this.backupCallback;
            }
        }
    }
}
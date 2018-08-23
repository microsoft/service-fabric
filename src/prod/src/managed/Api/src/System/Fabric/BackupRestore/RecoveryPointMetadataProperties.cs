// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore
{
    /// <summary>
    /// Class to hold the properties for recovery point.
    /// </summary>
    internal class RecoveryPointMetadataProperties
    {
        /// <summary>
        /// Gets the unique recovery point ID
        /// </summary>
        public Guid BackupId { get; set; }

        /// <summary>
        /// Gets the backup location
        /// </summary>
        public string BackupLocation { get; set; }

        /// <summary>
        /// Gets the backup time
        /// </summary>
        public DateTime BackupTime { get; set; }

        /// <summary>
        /// Gets the Epoch for the last backup log record
        /// </summary>
        public Epoch EpochOfLastBackupRecord { get; set; }

        /// <summary>
        /// Gets the LSN for the last backup log record
        /// </summary>
        public long LsnOfLastBackupRecord { get; set; }
    }
}
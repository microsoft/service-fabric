// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Diagnostics.CodeAnalysis;

namespace System.Fabric.BackupRestore
{
    /// <summary>
    /// Represent information about the backup
    /// </summary>
    internal class BackupInfo
    {
        /// <summary>
        /// Gets or sets the local folder/ directory where backup was taken
        /// </summary>
        public string Directory { get; set; }

        /// <summary>
        /// Gets or sets the backup option type
        /// </summary>
        public BackupOption Option { get; set; }

        /// <summary>
        /// Gets or sets the unique backup ID
        /// </summary>
        public Guid BackupId { get; set; }

        /// <summary>
        /// Backup ID of the parent backup
        /// </summary>
        public Guid ParentBackupId { get; set; }

        /// <summary>
        /// Gets or sets the Epoch and LSN of the first backed up log record
        /// </summary>
        public BackupVersion IndexBackupVersion { get; set; }

        /// <summary>
        /// Gets or sets the Epoch and LSN of the last backed up log record
        /// </summary>
        public BackupVersion LastBackupVersion { get; set; }

        /// <summary>
        /// Gets the ID of backup-chain to which this backup belongs. A backup chain contains
        /// one full backup and zero or more continuous incremental backup(s) and starts at full backup. 
        /// </summary>
        public Guid BackupChainId { get; set; }

        /// <summary>
        /// Gets the index of this backup in the backup-chain to which this backup belongs.
        /// A backup chain contains one full backup and zero or more continuous incremental backup(s).
        /// and starts at full backup. Since full backup starts the chain, its backup index is always zero.
        /// </summary>
        public long BackupIndex { get; set; }
    }

    /// <summary>
    /// Represents the version of the backup.
    /// </summary>
    [SuppressMessage("Microsoft.Performance", "CA1815")]
    internal struct BackupVersion : IComparable<BackupVersion>, IEquatable<BackupVersion>
    {
        /// <summary>
        /// Invalid Backup Version.
        /// </summary>
        public static readonly BackupVersion InvalidBackupVersion = new BackupVersion(new Epoch(-1, -1), -1);

        private Epoch _epoch;
        private long _lsn;

        /// <summary>
        /// Initializes a new instance of the <cref name="BackupVersion"/>
        /// </summary>
        /// <param name="epoch">The <cref name="Epoch"/> at which the backup was taken.</param>
        /// <param name="lsn">The last committed logical sequence number included in the backup.</param>
        public BackupVersion(Epoch epoch, long lsn)
        {
            this._epoch = epoch;
            this._lsn = lsn;
        }

        /// <summary>
        /// Gets the <cref name="Epoch"/> at which the backup was taken.
        /// </summary>
        /// <value>The <cref name="Epoch"/> at which the backup was taken.</value>
        public Epoch Epoch { get { return this._epoch; } private set { this._epoch = value; } }


        /// <summary>
        /// Gets the last committed logical sequence number included in the backup.
        /// </summary>
        /// <returns>The last committed logical sequence number included in the backup.</returns>
        public long Lsn { get { return this._lsn; } private set { this._lsn = value; } }


        /// <summary>
        /// Compares this instance with a specified <cref name="BackupVersion"/> object and indicates whether this instance precedes, 
        /// follows, or appears in the same position in the sort order as the specified <cref name="BackupVersion"/>. 
        /// </summary>
        /// <param name="other">An object that evaluates to a <cref name="BackupVersion"/>.</param>
        /// <returns>
        /// A value that indicates the relative order of the objects being compared.
        /// Less than zero indicates that this instance precedes other in the sort order.
        /// Zero indicates that this instance occurs in the same position in the sort order as other. 
        /// Greater than zero indicates that this instance follows other in the sort order.
        /// </returns>
        public int CompareTo(BackupVersion other)
        {
            var compareEpochResult = this.Epoch.CompareTo(other.Epoch);

            if (compareEpochResult != 0)
            {
                return compareEpochResult;
            }

            return this.Lsn.CompareTo(other.Lsn);
        }

        /// <summary>
        /// Determines whether this instance and another specified <cref name="BackupVersion"/> object have the same value.
        /// </summary>
        /// <param name="other">The <cref name="BackupVersion"/> to compare to this instance. </param>
        /// <returns>
        /// true if the value of the value parameter is the same as the value of this instance; otherwise, false. 
        /// </returns>
        public bool Equals(BackupVersion other)
        {
            if (this.CompareTo(other) == 0)
            {
                return true;
            }

            return false;
        }

        /// <summary>
        /// Determines whether this instance and another specified <cref name="BackupVersion"/> object have the same value.
        /// </summary>
        /// <param name="obj">The <cref name="BackupVersion"/> to compare to this instance. </param>
        /// <returns>
        /// true if the value of the value parameter is the same as the value of this instance; otherwise, false. 
        /// </returns>
        public override bool Equals(object obj)
        {
            if (obj is BackupVersion == false)
            {
                return false;
            }

            return this.Equals((BackupVersion)obj);
        }

        /// <summary>
        /// Returns the hash code for this <cref name="BackupVersion"/>.
        /// </summary>
        /// <returns>
        /// A 32-bit signed integer hash code.
        /// </returns>
        public override int GetHashCode()
        {
            return this.Epoch.DataLossNumber.GetHashCode() ^ this.Epoch.ConfigurationNumber.GetHashCode() ^ this.Lsn.GetHashCode();
        }
    }
}
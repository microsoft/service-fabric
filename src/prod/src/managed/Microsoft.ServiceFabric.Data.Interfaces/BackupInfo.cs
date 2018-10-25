// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric;
    using System.Globalization;

    /// <summary>
    /// Provides information about the backup.
    /// </summary>
    public class BackupInfo
    {
        /// <summary>
        /// Initializes a new instance of the <cref name="BackupInfo"/> class.
        /// </summary>
        /// <param name="directory">Folder path that contains the backup.</param>
        /// <param name="option"><cref name="BackupOption"/> that was used to take the backup.</param>
        /// <param name="version"><cref name="BackupVersion"/> of the backup.</param>
        public BackupInfo(string directory, BackupOption option, BackupVersion version)
        {
            this.Directory = directory;
            this.Option = option;
            this.Version = version;
            this.StartBackupVersion = BackupVersion.InvalidBackupVersion;
            this.BackupId = Guid.Empty;
            this.ParentBackupId = Guid.Empty;
        }

        /// <summary>
        /// Initializes a new instance of the <cref name="BackupInfo"/> class.
        /// </summary>
        /// <param name="directory">Folder path that contains the backup.</param>
        /// <param name="option"><cref name="BackupOption"/> that was used to take the backup.</param>
        /// <param name="version"><cref name="BackupVersion"/> of the backup.</param>
        /// <param name="startBackupVersion"><cref name="BackupVersion"/> of first logical log record in the backup.</param>
        /// <param name="backupId">Id of the backup.</param>
        /// <param name="parentBackupId">Id of the corresponding full backup in case of incremental backup, Guid.Empty in case this is full backup.</param>
        public BackupInfo(string directory, BackupOption option, BackupVersion version, BackupVersion startBackupVersion, Guid backupId, Guid parentBackupId)
        {
            this.Directory = directory;
            this.Option = option;
            this.Version = version;
            this.StartBackupVersion = startBackupVersion;
            this.BackupId = backupId;
            this.ParentBackupId = parentBackupId;
        }

        /// <summary>
        /// Gets the directory where the backup was created. 
        /// </summary>
        /// <returns>The directory containing the backup.</returns>
        public string Directory { get; private set; }

        /// <summary>
        /// Gets the backup option used.
        /// </summary>
        /// <returns>The <cref name="BackupOption"/> that was used to take the backup.</returns>
        public BackupOption Option { get; private set; }

        /// <summary>
        /// Gets the latest epoch and LSN included in the backup.
        /// </summary>
        /// <returns><cref name="BackupVersion"/> of the backup.</returns>
        public BackupVersion Version { get; private set; }

        /// <summary>
        /// Gets the Backup Id
        /// </summary>
        public Guid BackupId { get; private set; }

        /// <summary>
        /// Gets the first epoch and LSN in the backup
        /// </summary>
        public BackupVersion StartBackupVersion { get; private set; }

        /// <summary>
        /// Gets the parent backup ID
        /// </summary>
        public Guid ParentBackupId { get; private set; }

        /// <summary>
        /// Returns a string that represents this instance.
        /// </summary>
        /// <returns>A string that represents this instance.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.CurrentCulture, "Backup folder: {0}, backup option: {1}", this.Directory, this.Option);
        }

        /// <summary>
        /// Represents the version of the backup.
        /// </summary>
        [SuppressMessage("Microsoft.Performance", "CA1815")]
        public struct BackupVersion : IComparable<BackupVersion>, IEquatable<BackupVersion>
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
                // Note that Epoch.GetHashCode uses "+". Since it cannot be changed without breaking, I do not use this method.
                return this.Epoch.DataLossNumber.GetHashCode() ^ this.Epoch.ConfigurationNumber.GetHashCode() ^ this.Lsn.GetHashCode();
            }
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Diagnostics.CodeAnalysis;
    using Interop;
    using Globalization;
    using Common;

    /// <summary>
    /// <para>Provides information about the backup created by calling <see cref="System.Fabric.KeyValueStoreReplica.BackupAsync(string, StoreBackupOption, Func{StoreBackupInfo, System.Threading.Tasks.Task{bool}})"/>.</para>
    /// </summary>
    public sealed class StoreBackupInfo
    {
        /// <summary>
        /// <para>Gets the folder where the backup was created.</para>
        /// </summary>
        /// <value>
        /// <para>The backup folder</para>
        /// </value>
        public string BackupFolder { get; internal set; }

        /// <summary>
        /// <para>Gets the backup option used</para>
        /// </summary>
        /// <value>
        /// <para>The backup option used</para>
        /// </value>
        public StoreBackupOption BackupOption { get; internal set; }

        /// <summary>
        /// <para>
        /// Gets the ID of backup-chain to which this backup belongs. A backup chain contains
        /// one full backup and zero or more continuous incremental backup(s) and starts at full backup. 
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The ID of backup chain.</para>
        /// </value>
        public Guid BackupChainId { get; private set; }

        /// <summary>
        /// <para>
        /// Gets the index of this backup in the backup-chain to which this backup belongs.
        /// A backup chain contains one full backup and zero or more continuous incremental backup(s).
        /// and starts at full backup. Since full backup starts the chain, its backup index is always zero.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The backup index of current backup.</para>
        /// </value>
        public long BackupIndex { get; private set; }

        internal static unsafe StoreBackupInfo FromNative(NativeTypes.FABRIC_STORE_BACKUP_INFO nativeBackupInfo)
        {
            var backupInfo = new StoreBackupInfo
            {
                BackupFolder = NativeTypes.FromNativeString(nativeBackupInfo.BackupFolder),
                BackupOption = (StoreBackupOption)nativeBackupInfo.BackupOption
            };

            if (nativeBackupInfo.Reserved != IntPtr.Zero)
            {
                var ex1 = (NativeTypes.FABRIC_STORE_BACKUP_INFO_EX1*)nativeBackupInfo.Reserved;
                backupInfo.BackupChainId = ex1->BackupChainId;
                backupInfo.BackupIndex = ex1->BackupIndex;
            }

            return backupInfo;
        }
        
        /// <summary>
        /// <para>Returns a <see cref="System.String" /> that represents this instance</para>
        /// </summary>
        /// <returns>
        /// <para>A <see cref="System.String" /> that represents this instance</para>
        /// </returns>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.CurrentCulture, 
                "BackupFolder: {0}, BackupOption: {1}, BackupChainId: {2}, BackupIndex: {3}", 
                this.BackupFolder,
                this.BackupOption,
                this.BackupChainId,
                this.BackupIndex);
        }
    }
}
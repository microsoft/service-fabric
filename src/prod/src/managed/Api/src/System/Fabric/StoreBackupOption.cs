// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using Interop;

    /// <summary>
    /// <para>
    /// The backup option for the key-value store.
    /// </para>
    /// </summary>
    public enum StoreBackupOption
    {
        /// <summary>
        /// <para>
        /// A full backup of the key-value store.
        /// </para>
        /// </summary>
        Full = NativeTypes.FABRIC_STORE_BACKUP_OPTION.FABRIC_STORE_BACKUP_OPTION_FULL,

        /// <summary>
        /// <para>
        /// Incremental backup of the key-value store. i.e. only the log files created since the last 
        /// full or incremental backup will be backed up.
        /// </para>
        /// </summary>
        Incremental = NativeTypes.FABRIC_STORE_BACKUP_OPTION.FABRIC_STORE_BACKUP_OPTION_INCREMENTAL,

        /// <summary>
        /// <para>
        /// Option to truncate the transaction logs of the key-value store without creating any backup files.        
        /// </para>
        /// </summary>
        /// <remarks>
        /// This is useful in avoiding creation of backup files in secondary replicas when incremental backup is turned on.
        /// If incremental backup is turned on, then disk space fills up with transaction logs of the key-value store.
        /// To prevent this, frequent backups have to be created. However, creating backups on secondary replicas may not
        /// be useful for some key-value store services. These services also have to deal with discarding the backup
        /// directory. If this option is used, it cleans up the transaction logs without creating backup files.
        /// </remarks>
        TruncateLogsOnly = NativeTypes.FABRIC_STORE_BACKUP_OPTION.FABRIC_STORE_BACKUP_OPTION_TRUNCATE_LOGS_ONLY
    }
}
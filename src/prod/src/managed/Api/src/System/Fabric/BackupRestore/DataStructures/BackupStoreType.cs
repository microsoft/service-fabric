// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.BackupRestore.Interop;

namespace System.Fabric.BackupRestore.DataStructures
{
    /// <summary>
    /// Backup store type
    /// </summary>
    internal enum BackupStoreType
    {
        /// <summary>
        /// Invalid store type specified
        /// </summary>
        Invalid = NativeBackupRestoreTypes.FABRIC_BACKUP_STORE_TYPE.FABRIC_BACKUP_STORE_TYPE_INVALID,

        /// <summary>
        /// File share as the backup store type
        /// </summary>
        FileShare = NativeBackupRestoreTypes.FABRIC_BACKUP_STORE_TYPE.FABRIC_BACKUP_STORE_TYPE_FILE_SHARE,

        /// <summary>
        /// Azure storage as the backup store type
        /// </summary>
        AzureStorage = NativeBackupRestoreTypes.FABRIC_BACKUP_STORE_TYPE.FABRIC_BACKUP_STORE_TYPE_AZURE_STORE,

        /// <summary>
        /// dSMS managed Azure storage as the backup store type
        /// </summary>
        DsmsAzureStorage = NativeBackupRestoreTypes.FABRIC_BACKUP_STORE_TYPE.FABRIC_BACKUP_STORE_TYPE_DSMS_AZURE_STORE,
    }
}
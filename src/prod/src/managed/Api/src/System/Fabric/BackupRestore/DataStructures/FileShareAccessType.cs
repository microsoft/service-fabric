// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.BackupRestore.Interop;

namespace System.Fabric.BackupRestore.DataStructures
{
    /// <summary>
    /// Indicates access type for file share
    /// </summary>
    internal enum FileShareAccessType
    {
        /// <summary>
        /// No specific credentials to access the store
        /// </summary>
        None = NativeBackupRestoreTypes.FABRIC_BACKUP_STORE_FILE_SHARE_ACCESS_TYPE.FABRIC_BACKUP_STORE_FILE_SHARE_ACCESS_TYPE_NONE,

        /// <summary>
        /// Domain user credentials to access the store
        /// </summary>
        DomainUser = NativeBackupRestoreTypes.FABRIC_BACKUP_STORE_FILE_SHARE_ACCESS_TYPE.FABRIC_BACKUP_STORE_FILE_SHARE_ACCESS_TYPE_DOMAIN_USER,
    }
}
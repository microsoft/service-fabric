// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.BackupRestore.Interop;

namespace System.Fabric.BackupRestore.DataStructures
{
    /// <summary>
    /// Indicates type of backup scheduling policy
    /// </summary>
    internal enum BackupPolicyType
    {
        /// <summary>
        /// Invalid polocy type
        /// </summary>
        Invalid = NativeBackupRestoreTypes.FABRIC_BACKUP_POLICY_TYPE.FABRIC_BACKUP_POLICY_TYPE_INVALID,

        /// <summary>
        /// Frequency based backup scheduling policy
        /// </summary>
        FrequencyBased = NativeBackupRestoreTypes.FABRIC_BACKUP_POLICY_TYPE.FABRIC_BACKUP_POLICY_TYPE_FREQUENCY_BASED,

        /// <summary>
        /// Schedule based backup scheduling policy
        /// </summary>
        ScheduleBased = NativeBackupRestoreTypes.FABRIC_BACKUP_POLICY_TYPE.FABRIC_BACKUP_POLICY_TYPE_SCHEDULE_BASED,
    }
}
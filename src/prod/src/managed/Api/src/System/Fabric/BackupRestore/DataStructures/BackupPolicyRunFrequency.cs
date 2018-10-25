// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.BackupRestore.Interop;

namespace System.Fabric.BackupRestore.DataStructures
{
    /// <summary>
    /// Enum indicating backup scheduling policy run frequency
    /// </summary>
    internal enum BackupPolicyRunFrequency
    {
        /// <summary>
        /// Indicates invalid run frequency
        /// </summary>
        Invalid = NativeBackupRestoreTypes.FABRIC_BACKUP_POLICY_RUN_FREQUENCY_MODE.FABRIC_BACKUP_POLICY_RUN_FREQUENCY_MODE_INVALID,

        /// <summary>
        /// Indicates backup is taken every few minutes
        /// </summary>
        Minutes = NativeBackupRestoreTypes.FABRIC_BACKUP_POLICY_RUN_FREQUENCY_MODE.FABRIC_BACKUP_POLICY_RUN_FREQUENCY_MODE_MINUTES,

        /// <summary>
        /// Indicates backup is taken every few hours
        /// </summary>
        Hours = NativeBackupRestoreTypes.FABRIC_BACKUP_POLICY_RUN_FREQUENCY_MODE.FABRIC_BACKUP_POLICY_RUN_FREQUENCY_MODE_HOURS,
    }
}
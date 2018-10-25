// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.BackupRestore.Interop;

namespace System.Fabric.BackupRestore.DataStructures
{
    /// <summary>
    /// Enum indicates the backup run schedule 
    /// </summary>
    internal enum BackupPolicyRunSchedule
    {
        /// <summary>
        /// Indicates invalid run schedule
        /// </summary>
        Invalid = NativeBackupRestoreTypes.FABRIC_BACKUP_POLICY_RUN_SCHEDULE_MODE.FABRIC_BACKUP_POLICY_RUN_SCHEDULE_MODE_INVALID,

        /// <summary>
        /// Indicates daily backup schedule
        /// </summary>
        Daily = NativeBackupRestoreTypes.FABRIC_BACKUP_POLICY_RUN_SCHEDULE_MODE.FABRIC_BACKUP_POLICY_RUN_SCHEDULE_MODE_DAILY,

        /// <summary>
        /// Indicates weekly backup schedule
        /// </summary>
        Weekly = NativeBackupRestoreTypes.FABRIC_BACKUP_POLICY_RUN_SCHEDULE_MODE.FABRIC_BACKUP_POLICY_RUN_SCHEDULE_MODE_WEEKLY,
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common.Model
{
    using System.Fabric.BackupRestore.Enums;
    using System.Runtime.Serialization;
    using BackupRestoreView = System.Fabric.BackupRestore.BackupRestoreTypes;

    [DataContract]
    [KnownType(typeof(TimeBasedBackupSchedule))]
    [KnownType(typeof(FrequencyBasedBackupSchedule))]

    internal abstract class BackupSchedule
    {
        internal BackupSchedule(BackupScheduleType backupScheduleType)
        {
            this.BackupScheduleType = backupScheduleType;
        }

        [DataMember]
        internal BackupScheduleType BackupScheduleType { get; private set; }

        protected  internal static BackupSchedule FromBackupScheduleView(BackupRestoreView.BackupSchedule backupScheduleView)
        {
            BackupSchedule backupSchedule = null;
            if (backupScheduleView.ScheduleKind == Enums.BackupScheduleType.FrequencyBased )
            {
                backupSchedule =
                    FrequencyBasedBackupSchedule.FromFrequencyBasedBackupScheduleView(
                        (BackupRestoreView.FrequencyBasedBackupSchedule) backupScheduleView);
            }else if (backupScheduleView.ScheduleKind == BackupScheduleType.TimeBased)
            {
                backupSchedule =
                    TimeBasedBackupSchedule.FromTimeBasedBackupScheduleView(
                        (BackupRestoreView.TimeBasedBackupSchedule) backupScheduleView);
            }
            return backupSchedule;
        }

        protected internal BackupRestoreView.BackupSchedule ToBackupScheduleView()
        {
            BackupRestoreView.BackupSchedule backupScheduleView = null;
            if (this.BackupScheduleType == BackupScheduleType.FrequencyBased)
            {
                backupScheduleView = ((FrequencyBasedBackupSchedule)this).ToFrequencyBasedBackupScheduleView();
            }else if (this.BackupScheduleType == BackupScheduleType.TimeBased)
            {
                backupScheduleView = ((TimeBasedBackupSchedule)this).ToTimeBasedBackupScheduleView();
            }
            return backupScheduleView;
        }
    }
}
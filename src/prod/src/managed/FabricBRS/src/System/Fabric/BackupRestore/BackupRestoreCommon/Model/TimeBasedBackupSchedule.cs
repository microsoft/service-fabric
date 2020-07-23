// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common.Model
{
    using System.Linq;
    using System.Fabric.BackupRestore.Enums;
    using System.Runtime.Serialization;
    using BackupRestoreView = BackupRestoreTypes;
    using System.Text;
    using System.Collections.Generic;

    [DataContract]
    internal class TimeBasedBackupSchedule : BackupSchedule
    {
        internal TimeBasedBackupSchedule() : base(BackupScheduleType.TimeBased)
        {
        }

        internal TimeBasedBackupSchedule(TimeBasedBackupSchedule other) : this()
        {
            this.runDays = new List<DayOfWeek>(other.runDays);
            this.runTimes = new List<TimeSpan>(other.runTimes);
            this.ScheduleFrequencyType = other.ScheduleFrequencyType;
        }

        [DataMember]
        internal BackupScheduleFrequency ScheduleFrequencyType { get; private set; }

        [DataMember (Name = "RunDays")]
        private List<DayOfWeek> runDays = new List<DayOfWeek>();

        internal IReadOnlyList<DayOfWeek> RunDays 
        {
            get { return runDays; }
        }

        [DataMember(Name = "RunTimes")]
        private List<TimeSpan> runTimes = new List<TimeSpan>();

        internal IReadOnlyList<TimeSpan> RunTimes
        {
            get { return runTimes; }
        }
        
        internal static TimeBasedBackupSchedule FromTimeBasedBackupScheduleView(
            BackupRestoreView.TimeBasedBackupSchedule timeBasedBackupScheduleView)
        {
            var timeBasedBackupSchedule = new TimeBasedBackupSchedule
            {
                runDays = timeBasedBackupScheduleView.RunDays == null ? new List<DayOfWeek>() :  timeBasedBackupScheduleView.RunDays?.Distinct().ToList(),
                ScheduleFrequencyType = timeBasedBackupScheduleView.ScheduleFrequencyType,
                runTimes = timeBasedBackupScheduleView.RunTimes?.Distinct().ToList()
            };
            return timeBasedBackupSchedule;
        }

        internal BackupRestoreView.TimeBasedBackupSchedule ToTimeBasedBackupScheduleView()
        {
            var timeBasedBackupScheduleView = new BackupRestoreView.TimeBasedBackupSchedule
            {
                RunDays = this.runDays,
                ScheduleFrequencyType = this.ScheduleFrequencyType,
                RunTimes = this.runTimes
            };
            return timeBasedBackupScheduleView;
        }

        public override string ToString()
        {
            var stringBuilder = new StringBuilder(base.ToString());
            stringBuilder.AppendFormat("Run Days {0}", this.runDays == null ? "null" :  String.Join(", ", this.runDays.ToArray())).AppendLine();
            stringBuilder.AppendFormat("Run Times {0}", String.Join(", ", this.runTimes.ToArray())).AppendLine();
            stringBuilder.AppendFormat("Schedule Run Frequency {0}", this.ScheduleFrequencyType).AppendLine();
            return stringBuilder.ToString();
        }
    }
}
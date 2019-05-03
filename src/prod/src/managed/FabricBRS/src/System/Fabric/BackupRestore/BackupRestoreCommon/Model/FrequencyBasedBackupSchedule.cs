// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common.Model
{
    using System.Fabric.BackupRestore.Enums;
    using System.Runtime.Serialization;
    using BackupRestoreView = BackupRestoreTypes;
    using System.Text;


    [DataContract]
    internal class FrequencyBasedBackupSchedule : BackupSchedule
    {
        internal FrequencyBasedBackupSchedule() : base(BackupScheduleType.FrequencyBased)
        {
        }

        public FrequencyBasedBackupSchedule(FrequencyBasedBackupSchedule other)
            : this()
        {
            this.Interval = other.Interval;
            this.IntervalType = other.IntervalType;
        }

        [DataMember]
        internal BackupScheduleInterval IntervalType { get; private set; }

        [DataMember]
        internal int Interval { get; private set; }

        private const double maximumTimeIntervalInDays = 15;

        internal static FrequencyBasedBackupSchedule FromFrequencyBasedBackupScheduleView(
           BackupRestoreView.FrequencyBasedBackupSchedule frequencyBasedBackupScheduleView)
        {
            //from TimeSpan to BackupScheduleInterval.
            TimeSpan timespan = frequencyBasedBackupScheduleView.Interval;
            BackupScheduleInterval intervalType;
            int interval;

            if(timespan > TimeSpan.FromDays(maximumTimeIntervalInDays))
            {
                throw new ArgumentException(StringResources.InvalidBackupInterval);
            }

            if (timespan.Minutes != 0)
            {
                intervalType = BackupScheduleInterval.Minutes;
                interval = (int)timespan.TotalMinutes;
            }
            else if(timespan.TotalHours != 0)
            {
                intervalType = BackupScheduleInterval.Hours;
                interval = (int)timespan.TotalHours;
            }
            else
            {
                throw new ArgumentException(StringResources.InvalidInterval);
            }

            var frequencyBasedBackupSchedule = new FrequencyBasedBackupSchedule
            {
                IntervalType = intervalType,
                Interval = interval
            };

            return frequencyBasedBackupSchedule;
        }

        internal BackupRestoreView.FrequencyBasedBackupSchedule ToFrequencyBasedBackupScheduleView()
        {
            // from BackupScheduleInterval to Timespan
            TimeSpan timespan;
            if(this.IntervalType == BackupScheduleInterval.Minutes)
            {
                timespan = TimeSpan.FromMinutes(this.Interval);
            }
            else
            {
                timespan = TimeSpan.FromHours(this.Interval);
            }

            var frequencyBasedSchedulePolicyView =
                new BackupRestoreView.FrequencyBasedBackupSchedule
                {
                    Interval = timespan,
                };
            return frequencyBasedSchedulePolicyView;
        }

        public override string ToString()
        {
            var stringBuilder = new StringBuilder(base.ToString());
            stringBuilder.AppendFormat("Backup Frequency Type {0}", this.IntervalType).AppendLine();
            stringBuilder.AppendFormat("Run Interval {0}", this.Interval).AppendLine();
            return stringBuilder.ToString();
        }
    }
}
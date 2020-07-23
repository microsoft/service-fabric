// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.BackupRestore.Enums;
    using System.Runtime.Serialization;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;
    using System.ComponentModel.DataAnnotations;
    using System.Text;
    using System.Fabric.BackupRestore.View.JsonConverter;

    /// <summary>
    /// Describing the Schedule Type of Scheduling a Backup
    /// </summary>
    [DataContract]
    public class TimeBasedBackupSchedule : BackupSchedule
    {
        /// <summary>
        /// Default Constructor
        /// </summary>
        public TimeBasedBackupSchedule() : base(BackupScheduleType.TimeBased)
        {
            
        }

        /// <summary>
        /// Describing the Schedule Type of Scheduling a Backup
        /// </summary>
        [JsonConverter(typeof(StringEnumConverter))]
        [DataMember]
        [Required]
        public BackupScheduleFrequency ScheduleFrequencyType { set; get; }

        /// <summary>
        /// Describe the days to run the Backup
        /// </summary>
        [JsonProperty(ItemConverterType = typeof(StringEnumConverter))]
        [DataMember]
        [RunDaysValidation]
        public List<DayOfWeek> RunDays { set; get; }

        /// <summary>
        /// Time interval to run the scheduled backup 
        /// </summary>
        [JsonProperty(ItemConverterType = typeof(RunTimesConverter))]
        [DataMember]
        [Required]
        [RunTimesValidation(ErrorMessage = "TimeSpans Should be less than 24 hrs format")]
        public List<TimeSpan> RunTimes { set; get; }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder(base.ToString());
            stringBuilder.AppendFormat("ScheduleFrequencyType {0}", this.ScheduleFrequencyType).AppendLine();
            stringBuilder.AppendFormat("Run Days List {0}",this.RunDays != null ? string.Join(",",this.RunDays.ToArray()) :
                null).AppendLine();
            stringBuilder.AppendFormat("Run Times List {0}",this.RunTimes != null ? string.Join(",",this.RunTimes.ToArray()) : 
                null).AppendLine();
            return stringBuilder.ToString();
        }
    }
}
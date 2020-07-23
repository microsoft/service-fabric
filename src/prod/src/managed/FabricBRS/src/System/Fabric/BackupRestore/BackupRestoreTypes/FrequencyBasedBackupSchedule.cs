// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System.Fabric.BackupRestore.Enums;
    using System.Runtime.Serialization;
    using Newtonsoft.Json;
    using System.Fabric.BackupRestore.View.JsonConverter;
    using System.ComponentModel.DataAnnotations;
    using System.Text;

    /// <summary>
    /// Describing the Frequency Type of Scheduling a Backup
    /// </summary>
    [DataContract]
    public class FrequencyBasedBackupSchedule : BackupSchedule
    {
        /// <summary>
        /// Default Constructor
        /// </summary>
        public FrequencyBasedBackupSchedule() : base(BackupScheduleType.FrequencyBased)
        {
            
        }
        /// <summary>{
        /// Represents the time interval to take the backup. It is not supported to give time interval in seconds.
        /// </summary>
        [JsonConverter(typeof(TimeSpanConverter))]
        [DataMember]
        [Required]
        public TimeSpan Interval { set; get; } 

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder(base.ToString());
            stringBuilder.AppendFormat("Interval : {0}", this.Interval).AppendLine();
            return stringBuilder.ToString();
        }
    }

}
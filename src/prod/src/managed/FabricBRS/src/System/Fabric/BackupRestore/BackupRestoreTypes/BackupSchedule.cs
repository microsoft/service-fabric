// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System.Fabric.BackupRestore.Enums;
    using System.Runtime.Serialization;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;
    using System.ComponentModel.DataAnnotations;
    using System.Text;

    /// <summary>
    ///  Describes Backup Schedule Details.
    /// </summary>
    [DataContract]
    [KnownType(typeof(TimeBasedBackupSchedule))]
    [KnownType(typeof(FrequencyBasedBackupSchedule))]

    public class BackupSchedule
    {
        /// <summary>
        /// Default Constructor
        /// </summary>
        protected internal BackupSchedule(BackupScheduleType backupScheduleType)
        {
            this.ScheduleKind = backupScheduleType;
        }
        /// <summary>
        /// Defines the schedule policy type [ TimeBased , FrequencyBased ]
        /// </summary>
        [JsonConverter(typeof(StringEnumConverter))]
        [JsonProperty(Order = -2)]
        [DataMember]
        [Required]
        public BackupScheduleType ScheduleKind { set; get; }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("Schedule Kind {0}",this.ScheduleKind).AppendLine();
            return stringBuilder.ToString();
        }
    }
    
}
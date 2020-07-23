// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System.Runtime.Serialization;
    using Newtonsoft.Json;
    using System.Fabric.BackupRestore.BackupRestoreTypes.JsonConverter;
    using System.ComponentModel.DataAnnotations;
    using System.Text;

    /// <summary>
    ///  Backup Policy details for enabling protection.
    /// </summary>
    [DataContract]
    public class BackupPolicy
    {
        /// <summary>
        /// The name of backup policy needed to enable and query.
        /// </summary>
        [DataMember]
        [Required]
        public string Name { set; get; }

        /// <summary>
        /// To Trigger Restore as soon as Data Loss is  initiated.
        /// </summary>
        [DataMember]
        [Required]
        public bool AutoRestoreOnDataLoss;

        /// <summary>
        /// Defines the number of Maximum incremental backups can be taken for this Schedule Policy
        /// </summary>
        [DataMember]
        [Required]
        [Range(0, 255)]
        public int MaxIncrementalBackups { set; get; }


        /// <summary>
        /// Details for Scheduling of Backup on the Clusters Applications
        /// </summary>
        [JsonConverter(typeof(BackupScheduleJsonConverter))]
        [DataMember]
        [Required]
        public BackupSchedule Schedule { set; get; }

        /// <summary>
        /// Location Details for storing Backups
        /// </summary>
        [JsonConverter(typeof(BackupStorageConverter))]
        [DataMember]
        [Required]
        public BackupStorage Storage { set; get; }

        /// <summary>
        /// Retention Policy specified to delete old backups
        /// </summary>
        [JsonConverter(typeof(RetentionPolicyJsonConverter))]
        [DataMember(EmitDefaultValue = false)]
        public RetentionPolicy RetentionPolicy { set; get; } = null;

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("Name : {0}", this.Name).AppendLine();
            stringBuilder.AppendFormat("Auto-Restore On DataLoss : {0}", this.AutoRestoreOnDataLoss).AppendLine();
            stringBuilder.AppendFormat("MaxIncrementalBackups : {0}", this.MaxIncrementalBackups).AppendLine();
            stringBuilder.AppendFormat("Schedule : {0}", this.Schedule).AppendLine();
            stringBuilder.AppendFormat("BackupStorage : {0}", this.Storage).AppendLine();
            stringBuilder.AppendFormat("RetentionPolicy : {0}", this.RetentionPolicy).AppendLine();
            return stringBuilder.ToString();
        }

    }
}
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
    ///  Backup Policy details for updating the Backup Policy protection.
    /// </summary>
    [DataContract]
    public class UpdateBackupPolicy
    {
        /// <summary>
        /// To Trigger Restore as soon as Data Loss is  initiated.
        /// </summary>
        [DataMember]
        [Required]
        public bool AutoRestore;
        /// <summary>
        /// Details for Scheduling of Backup on the Clusters Applications
        /// </summary>
        [JsonConverter(typeof(SchedulePolicyJsonConverter))]
        [DataMember]
        [Required]
        public SchedulePolicy SchedulePolicy { set; get; }
        /// <summary>
        /// Retention Details of Backups in the storage
        /// </summary>
        [JsonConverter(typeof(RetentionPolicyJsonConverter))]
        [DataMember]
        [Required]
        public RetentionPoicy RetentionPoicy { set; get; }
        /// <summary>
        /// Location for storing Backups
        /// </summary>
        [JsonConverter(typeof(BackupStorageConverter))]
        [DataMember]
        public BackupStorage BackupStorage { set; get; }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("Auto-Restore : {0}", this.AutoRestore).AppendLine();
            stringBuilder.AppendFormat("Schedule Policy : {0}", this.SchedulePolicy).AppendLine();
            stringBuilder.AppendFormat("Retention Policy : {0}", this.RetentionPoicy).AppendLine();
            stringBuilder.AppendFormat("Backup Storage : {0}", this.BackupStorage).AppendLine();
            return stringBuilder.ToString();
        }

    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Text;

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System.Fabric.BackupRestore.Enums;
    using System.Runtime.Serialization;
    using Newtonsoft.Json;
    using System.ComponentModel.DataAnnotations;
    using System.Fabric.BackupRestore.View.JsonConverter;

    /// <summary>
    /// Describes the Short Retention Policy of the Backups
    /// </summary>
    [DataContract]
    public class BasicRetentionPolicy : RetentionPolicy
    {
        /// <summary>
        /// Default Constructor
        /// </summary>
        public BasicRetentionPolicy() : base(RetentionPolicyType.Basic)
        {
            
        }

        public BasicRetentionPolicy(TimeSpan retentionDuration) : base(RetentionPolicyType.Basic)
        {
            this.RetentionDuration = retentionDuration;
        }

        /// <summary>
        /// Defines the length of Duration for keeping the Backup
        /// </summary>
        [JsonConverter(typeof(TimeSpanConverter))]
        [DataMember]
        [Required]
        public TimeSpan RetentionDuration { set; get; }

        /// <summary>
        /// Defines the minimum number of backups which will be retained even after the backups move out of retention duration window.
        /// </summary>
        [DataMember]
        public int MinimumNumberOfBackups { set; get; } = 0;

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("Retention Duration : {0}",this.RetentionDuration).AppendLine();
            stringBuilder.AppendFormat("Minimum number of Backups {0}", this.MinimumNumberOfBackups).AppendLine();
            return stringBuilder.ToString();
        }
    }

    
}
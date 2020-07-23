// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System.Text;
    using System.Fabric.BackupRestore.Enums;
    using System.Runtime.Serialization;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;
    using System.ComponentModel.DataAnnotations;

    /// <summary>
    ///  Describes Retention Policy details for Backup.
    /// </summary>
    [DataContract]
    [KnownType(typeof(BasicRetentionPolicy))]
    public class RetentionPolicy
    {
        /// <summary>
        /// Default Constructor
        /// </summary>
        /// <param name="retentionPolicyType"></param>
        public RetentionPolicy(RetentionPolicyType retentionPolicyType)
        {
            this.RetentionPolicyType = retentionPolicyType;
        }

        /// <summary>
        /// Defines the retention policy type [ BasicRetentionType ] 
        /// </summary>
        [JsonProperty(Order = -2)]
        [JsonConverter(typeof(StringEnumConverter))]
        [DataMember]
        [Required]
        public RetentionPolicyType RetentionPolicyType { set; get; }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("Retention policy Type : {0}",this.RetentionPolicyType).AppendLine();
            return stringBuilder.ToString();
        }
    }
}
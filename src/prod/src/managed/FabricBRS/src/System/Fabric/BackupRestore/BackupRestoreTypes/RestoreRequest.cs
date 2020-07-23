// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Text;

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System.ComponentModel.DataAnnotations;
    using System.Fabric.BackupRestore.BackupRestoreTypes.JsonConverter;
    using System.Runtime.Serialization;
    using Newtonsoft.Json;


    /// <summary>
    ///  Represents a restore point from which backup will be restored
    /// </summary>
    [DataContract]
    public class RestoreRequest 
    {
        /// <summary>
        /// Unique restore point ID 
        /// </summary>
        [DataMember]
        [Required]
        public Guid BackupId { get; set; }

        /// <summary>
        /// Restore point Location from where it needs to be restored 
        /// </summary>
        [DataMember]
        [Required]
        public string BackupLocation { get; set; }

        /// <summary>
        /// Location from where the partition will be restored
        /// </summary>
        [JsonConverter(typeof(NullableBackupStorageConverter))]
        [DataMember]
        public BackupStorage BackupStorage { set; get; }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("BackupId : {0}", this.BackupId).AppendLine();
            stringBuilder.AppendFormat("BackupLocation : {0}", this.BackupLocation).AppendLine();
            stringBuilder.AppendFormat("BackupStorage :{0}", this.BackupStorage).AppendLine();
            return stringBuilder.ToString();
        }
    }
}
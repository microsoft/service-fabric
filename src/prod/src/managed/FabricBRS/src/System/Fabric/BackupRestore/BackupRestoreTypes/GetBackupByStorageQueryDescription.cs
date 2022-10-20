// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.BackupRestore.BackupRestoreTypes.JsonConverter;

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System.Fabric.BackupRestore.Enums;
    using System.Runtime.Serialization;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;
    using System.ComponentModel.DataAnnotations;
    using System.Fabric.BackupRestore.Common;
    using System.Collections.Generic;

    /// <summary>
    /// Describes the Meta Data of Backup Conifguration to Application/Service/Partition ID
    /// </summary>
    [DataContract]
    public class BackupByStorageQueryDescription
    {
        public BackupByStorageQueryDescription()
        {
            this.StartDateTimeFilter = DateTime.MinValue;
            this.EndDateTimeFilter = DateTime.MaxValue;
            this.Latest = false;
        }
        /// <summary>
        /// Filter to set Start  Date Time 
        /// </summary>
        [DataMember]
        public DateTime StartDateTimeFilter { set; get; }

        /// <summary>
        /// Filter to set End Date Time 
        /// </summary>
        [DataMember]
        public DateTime EndDateTimeFilter { set; get; }

        /// <summary>
        /// Filter to set End Date Time 
        /// </summary>
        [DataMember]
        public bool Latest { set; get; }

        /// <summary>
        /// Location Details for storing Backups
        /// </summary>
        [JsonConverter(typeof(BackupStorageConverter))]
        [DataMember]
        [Required]
        public BackupStorage Storage { set; get; }

        /// <summary>
        /// Requested Backup Points fabric uri
        /// </summary>
        [JsonConverter(typeof(BackupEntityConverter))]
        [DataMember]
        [Required]
        public BackupEntity BackupEntity { set; get; }
    }

    
}
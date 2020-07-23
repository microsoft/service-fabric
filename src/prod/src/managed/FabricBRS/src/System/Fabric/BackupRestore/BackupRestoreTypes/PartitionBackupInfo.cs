// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System.Runtime.Serialization;
    using System.Collections.Generic;
    using System.Fabric.BackupRestore.Enums;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;

    /// <summary>
    /// Describes the Meta Data of Getting the Applied Backup Policies to Application/Service/Partition ID and List of Suspended Partitions
    /// </summary>
    [DataContract]
    public class PartitionBackupInfo : IBackupMappingInfo
    {
        internal PartitionBackupInfo()
        {
            this.IsBackupSuspended = false;
        }

        /// <summary>
        /// Details of all the existing Backup Protections for requested Application/Service/Partition
        /// </summary>
        [DataMember]
        public string  BackupPolicyName{ set; get; }

        /// <summary>
        /// Partition Id details of all the suspended backups
        /// </summary>
        [DataMember]
        [JsonConverter(typeof(StringEnumConverter))]
        public BackupPolicyScopeType Scope { set; get; }

        [DataMember]
        public bool IsBackupSuspended { set; get; }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System.Runtime.Serialization;
    using System.Collections.Generic;

    /// <summary>
    /// Describes the Meta Data of Getting the Applied Backup Policies to Application/Service/Partition ID and List of Suspended Partitions
    /// </summary>
    [DataContract]
    public class BackupMappingInfo : IBackupMappingInfo
    {
        internal BackupMappingInfo()
        {
            this.BackupMappingInfos = new List<BackupMapping>();
            this.SuspendedPartitons = new List<BackupSuspendedPartitionInfo>();
        }

        /// <summary>
        /// Details of all the exisiting Backup Protections for requested Application/Service/Partition
        /// </summary>
        [DataMember]
        public List<BackupMapping> BackupMappingInfos{ set; get; }

        /// <summary>
        /// Partition Id details of all the suspended backups
        /// </summary>
        [DataMember]
        public List<BackupSuspendedPartitionInfo> SuspendedPartitons { set; get; }
    }

    public abstract class IBackupMappingInfo
    {
    }
}
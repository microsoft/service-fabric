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
    using System.Fabric.BackupRestore.Common;
    using System.Fabric.BackupRestore.BackupRestoreTypes.JsonConverter;

    /// <summary>
    /// Describes the Meta Data of Applying Backup Policies to Application/Service/Partition ID
    /// </summary>
    [JsonConverter(typeof(BackupEntityConverter))]
    [DataContract]
    public abstract class BackupEntity
    {
        protected internal BackupEntity(BackupEntityKind entityKind)
        {
            this.EntityKind = entityKind;
        }

        /// <summary>
        /// Entity Kind on which Policy is Applied
        /// </summary>
        [DataMember]
        [JsonConverter(typeof(StringEnumConverter))]
        [JsonProperty(Order = -2)]
        [Required]
        public BackupEntityKind EntityKind { set; get; }

        internal static BackupEntity GetBackupEntityFromFabricKey(string fabricKey)
        {
            BackupEntity backupEntity = null;
            string applicationName = null;
            string serviceName = null;
            string partitionId = null;
            FabricBackupResourceType fabricBackupResourceType =
                UtilityHelper.GetApplicationAndServicePartitionUri(fabricKey, out applicationName, out serviceName,
                    out partitionId);
            switch (fabricBackupResourceType)
            {
                    case FabricBackupResourceType.ApplicationUri:
                        backupEntity  = new ApplicationBackupEntity(applicationName);
                    break;
                    case FabricBackupResourceType.ServiceUri:
                        backupEntity = new ServiceBackupEntity(serviceName);
                    break;
                    case FabricBackupResourceType.PartitionUri:
                        backupEntity = new PartitionBackupEntity(serviceName,partitionId);
                    break;
                default:
                    throw new ArgumentException();

            }
            return backupEntity;
        }
    }

    public class ApplicationBackupEntity : BackupEntity
    {
        public ApplicationBackupEntity(string applicationName) : base(BackupEntityKind.Application)
        {
            this.ApplicationName = UtilityHelper.GetUriFromCustomUri(applicationName);
        }

        /// <summary>
        /// Create  Application Backup Entity
        /// </summary>
        public ApplicationBackupEntity() : base(BackupEntityKind.Application)
        {
            
        }

        /// <summary>
        /// Application Name on which Policy is Applied
        /// </summary>
        [DataMember]
        [Required]
        public string ApplicationName { get; set; }
    }

    public class ServiceBackupEntity : BackupEntity
    {
        public ServiceBackupEntity(string serviceName) : base(BackupEntityKind.Service)
        {
            this.ServiceName = UtilityHelper.GetUriFromCustomUri(serviceName);
        }

        /// <summary>
        /// Create  Service Backup Entity
        /// </summary>
        public ServiceBackupEntity( ) : base(BackupEntityKind.Service)
        {
        }

        /// <summary>
        /// Service Name on which Policy is Applied
        /// </summary>
        [DataMember]
        [Required]
        public string ServiceName { get; set; }
    }

    public class PartitionBackupEntity : BackupEntity
    {
        public PartitionBackupEntity(string serviceName , string partitionId) : base(BackupEntityKind.Partition)
        {
            this.ServiceName = UtilityHelper.GetUriFromCustomUri(serviceName);
            this.PartitionId = partitionId;
        }

        /// <summary>
        /// Create  Partition Backup Entity
        /// </summary>
        internal PartitionBackupEntity() : base(BackupEntityKind.Partition)
        {
        }

        /// <summary>
        /// Service Name on which Policy is Applied
        /// </summary>
        [DataMember]
        [Required]
        public string ServiceName { get; set; }

        /// <summary>
        /// Partition Id on which Policy is Applied
        /// </summary>
        [DataMember]
        [Required]
        public string PartitionId { get; set; }
    }



}
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
    using System.Collections.Generic;

    /// <summary>
    /// Describes the Data of Applying Backup Configuration to Application/Service/Partition ID
    /// </summary>
    [DataContract]
    public abstract class BackupConfigurationInfo
    {
        protected internal BackupConfigurationInfo()
        {
        }
    }


        /// <summary>
        /// Describes the Data of Applying Backup Configuration to Application/Service/Partition ID
        /// </summary>
        [DataContract]
    public abstract  class  BackupEntityConfigurationInfo : BackupConfigurationInfo
    {

        protected internal BackupEntityConfigurationInfo(BackupEntityKind kind , string policyName , BackupEntityKind policyInheritedFrom,
            BackupSuspensionInfo suspensionInfo)
        {
            this.Kind = kind;
            this.PolicyInheritedFrom = policyInheritedFrom;
            this.PolicyName = policyName;
            this.SuspensionInfo = suspensionInfo;
        }
        /// <summary>
        /// Backup configuration Kind
        /// </summary>
        [JsonProperty(Order = -2)]
        [DataMember]
        [JsonConverter(typeof(StringEnumConverter))]
        public BackupEntityKind Kind { set; get; }

        /// <summary>
        /// Backup Policy name for the Configuration
        /// </summary>
        [DataMember]
        public string PolicyName { set; get; }

        /// <summary>
        /// Backup policy from where it is inherited from
        /// </summary>
        [DataMember]
        [JsonConverter(typeof(StringEnumConverter))]
        public BackupEntityKind PolicyInheritedFrom { set; get; }

        /// <summary>
        /// Information about Suspension
        /// </summary>
        [DataMember]
        public BackupSuspensionInfo SuspensionInfo { set; get; }

    }

    /// <summary>
    /// Describes the Data of Applying Backup Configuration to Application/Service/Partition ID
    /// </summary>
    
    [DataContract]
    public class ApplicationBackupConfigurationInfo : BackupEntityConfigurationInfo
    {
        /// <summary>
        /// Describes the application name
        /// </summary>
        [DataMember]
        public string ApplicationName { set; get; }

        public ApplicationBackupConfigurationInfo(string applicationName , string policyName, BackupEntityKind policyInheritedFrom,
            BackupSuspensionInfo suspensionInfo) : base(BackupEntityKind.Application, policyName, policyInheritedFrom,
            suspensionInfo)
        {
            this.ApplicationName = UtilityHelper.GetUriFromCustomUri(applicationName);
        }

    }

    /// <summary>
    /// Describes the Data of Applying Backup Configuration to Service
    /// </summary>

    [DataContract]
    public class ServiceBackupConfigurationInfo : BackupEntityConfigurationInfo
    {
        /// <summary>
        /// Describes the Service name
        /// </summary>
        [DataMember]
        public string ServiceName { set; get; }

        public ServiceBackupConfigurationInfo(string serviceName, string policyName, BackupEntityKind policyInheritedFrom,
            BackupSuspensionInfo suspensionInfo) : base(BackupEntityKind.Service, policyName, policyInheritedFrom,
            suspensionInfo)
        {
            this.ServiceName = UtilityHelper.GetUriFromCustomUri(serviceName);
        }

    }

    /// <summary>
    /// Describes the Data of Applying Backup Configuration to Partition
    /// </summary>

    [DataContract]
    public class PartitionBackupConfigurationInfo : BackupEntityConfigurationInfo
    {
        /// <summary>
        /// Describes the Service name
        /// </summary>
        [DataMember]
        public string ServiceName { set; get; }

        /// <summary>
        /// Describes the Partition id
        /// </summary>
        [DataMember]
        public string PartitionId { set; get; }

        public PartitionBackupConfigurationInfo(string serviceName,string partitionId, string policyName, BackupEntityKind policyInheritedFrom,
            BackupSuspensionInfo suspensionInfo) : base(BackupEntityKind.Partition, policyName, policyInheritedFrom,
            suspensionInfo)
        {
            this.ServiceName = UtilityHelper.GetUriFromCustomUri(serviceName);
            this.PartitionId = partitionId;
        }
    }

    [DataContract]
    public class PagedBackupConfigurationInfo : BackupConfigurationInfo
    {
        /// <summary>
        /// 
        /// </summary>
        public PagedBackupConfigurationInfo()
        {
            this.Items = new List<BackupEntityConfigurationInfo>();
        }

        /// <summary>
        /// Continuation Token for getting paged Backup configuration list
        /// </summary>
        [DataMember]
        public string ContinuationToken { set; get; }

        /// <summary>
        /// The list of backup configuration information
        /// </summary>
        [DataMember]
        public List<BackupEntityConfigurationInfo> Items { set; get; }

    }
}
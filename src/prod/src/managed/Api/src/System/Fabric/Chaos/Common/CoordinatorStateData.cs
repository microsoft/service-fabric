// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.Common
{
    using System;
    using System.Runtime.Serialization;

    // State classes for deserializing JSON data coming back from InfrastructureService (IS).
    //
    internal class InfrastructureStatus
    {
        public CoordinatorStateData CurrentState { get; set; }

        public JobCollectionData Jobs { get; set; }
    }

    /// <summary>
    /// Common base class for the 2 different types of coordinators that IS supports.
    /// </summary>
    /// <remarks>
    /// There is little in common between the 2 coordinators of IS. Using this base class more for
    /// minimizing change in other modules in this code base than for genuinely sharing common stuff.
    /// </remarks>
    [DataContract]
    internal abstract class CoordinatorStateData
    {
        [DataMember]
        public string Mode { get; set; }

        [DataMember]
        public string JobBlockingPolicy { get; set; }
    }

    /// <summary>
    /// Modes of operation of the IS.
    /// </summary>
    internal static class InfrastructureServiceMode
    {
        /// <summary>
        /// IS can process only one Azure infrastructure job at a time.
        /// </summary>
        public const string Serial = "Serial";

        /// <summary>
        /// IS can process multiple Azure infrastructure jobs at a time.
        /// </summary>
        public const string Parallel = "Parallel";
    }

    /// <summary>
    /// 
    /// </summary>
    [DataContract]
    internal class CoordinatorStateDataSerial : CoordinatorStateData
    {
        [DataMember]
        public string InfrastructureTaskId { get; set; }
        [DataMember]
        public string LastKnownJobState { get; set; }
        [DataMember]
        public InfrastructureTaskStateData LastKnownTask { get; set; }
        [DataMember]
        public ManagementNotificationData ManagementNotification { get; set; }
        [DataMember]
        public bool IsManagementNotificationAvailable { get; set; }
    }

    /// <summary>
    /// 
    /// </summary>
    [DataContract]
    internal class CoordinatorStateDataParallel : CoordinatorStateData
    {
        [DataMember]
        public JobSummary[] Jobs { get; set; }

        [DataMember]
        public string JobDocumentIncarnation { get; set; }
    }

    /// <summary>
    /// 
    /// </summary>
    [DataContract]
    internal class JobSummary
    {
        [DataMember]
        public string Id { get; set; }

        [DataMember]
        public string ContextId { get; set; }

        [DataMember]
        public string JobStatus { get; set; }

        [DataMember]
        public string ImpactAction { get; set; }

        [DataMember]
        public string[] RoleInstancesToBeImpacted { get; set; }

        [DataMember]
        public JobStepSummary JobStep { get; set; }
    }

    /// <summary>
    /// 
    /// </summary>
    [DataContract]
    internal class JobStepSummary
    {
        [DataMember]
        public string AcknowledgementStatus { get; set; }

        [DataMember]
        public string ActionStatus { get; set; }

        [DataMember]
        public string ImpactStep { get; set; }

        [DataMember]
        public ImpactedRoleInstance[] CurrentlyImpactedRoleInstances { get; set; }
    }

    /// <summary>
    /// 
    /// </summary>
    [DataContract]
    internal class ImpactedRoleInstance
    {
        [DataMember]
        public string Name { get; set; }

        [DataMember]
        public string UD { get; set; }

        [DataMember]
        public string[] ImpactTypes { get; set; }
    }

    /// <summary>
    /// 
    /// </summary>
    [DataContract]
    internal class InfrastructureTaskStateData
    {
        [DataMember]
        public long TaskInstanceId { get; set; }
        [DataMember]
        public NodeTaskDescriptionData[] TaskDescription { get; set; }
    }

    /// <summary>
    /// 
    /// </summary>
    [DataContract]
    internal class NodeTaskDescriptionData
    {
        [DataMember]
        public string NodeName { get; set; }
        [DataMember]
        public string TaskType { get; set; }
    }

    /// <summary>
    /// 
    /// </summary>
    [DataContract]
    internal class ManagementNotificationData
    {
        [DataMember]
        public string NotificationId { get; set; }
        [DataMember]
        public string NotificationStatus { get; set; }
        [DataMember]
        public string NotificationType { get; set; }
        [DataMember]
        public DateTime NotificationDeadline { get; set; }
        [DataMember]
        public string ActiveJobStepId { get; set; }
        [DataMember]
        public string ActiveJobId { get; set; }
        [DataMember]
        public string ActiveJobType { get; set; }
        [DataMember]
        public bool ActiveJobIncludesTopologyChange { get; set; }
        [DataMember]
        public int ActiveJobStepTargetUD { get; set; }
        [DataMember]
        public string JobStepImpact { get; set; }
        [DataMember]
        public ulong Incarnation { get; set; }
        [DataMember]
        public string ActiveJobDetailedStatus { get; set; }
        [DataMember]
        public string ActiveJobContextId { get; set; }
        [DataMember]
        public string ActiveJobStepStatus { get; set; }
    }

    /// <summary>
    /// 
    /// </summary>
    [DataContract]
    internal class JobData
    {
        [DataMember]
        public string Id { get; set; }

        [DataMember]
        public string Status { get; set; }

        [DataMember]
        public string Type { get; set; }
        [DataMember]
        public string ContextId { get; set; }

        [DataMember]
        public string DetailedStatus { get; set; }
    }

    /// <summary>
    /// 
    /// </summary>
    [DataContract]
    internal class JobCollectionData
    {
        [DataMember]
        public JobData[] Jobs { get; set; }
    }
}
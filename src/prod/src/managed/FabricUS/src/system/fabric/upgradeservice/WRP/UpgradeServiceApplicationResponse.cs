// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.WRP.Model
{
    using System.Collections.Generic;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    using System.Fabric.Common;
    using System.Reflection;

    public class UpgradeServicePollRequest
    {
        public UpgradeServicePollRequest()
        {
            this.ApplicationTypeOperationStatuses = new List<ApplicationTypeVersionOperationStatus>();
            this.ApplicationOperationStatuses = new List<ApplicationOperationStatus>();
            this.ServiceOperationStatuses = new List<ServiceOperationStatus>();
        }

        public ClusterOperationStatus ClusterOperationStatus { get; set;}
        public List<ApplicationTypeVersionOperationStatus> ApplicationTypeOperationStatuses { get; set; }
        public List<ApplicationOperationStatus> ApplicationOperationStatuses { get; set; }
        public List<ServiceOperationStatus> ServiceOperationStatuses { get; set; }
    }

    public class UpgradeServicePollResponse
    {
        public UpgradeServicePollResponse()
        {
            this.ApplicationTypeOperationDescriptions = new List<ApplicationTypeVersionOperationDescription>();
            this.ApplicationOperationDescriptions = new List<ApplicationOperationDescription>();
            this.ServiceOperationDescriptions = new List<ServiceOperationDescription>();
        }

        public ClusterOperationDescription ClusterOperationDescription { get; set; }
        public List<ApplicationTypeVersionOperationDescription> ApplicationTypeOperationDescriptions { get; set; }
        public List<ApplicationOperationDescription> ApplicationOperationDescriptions { get; set; }

        [JsonConverter(typeof(ServiceOperationDescriptionConverter))]
        public List<ServiceOperationDescription> ServiceOperationDescriptions { get; set; }
    }

    public class UpgradeServiceApplicationPollRequest
    {
        public UpgradeServiceApplicationPollRequest()
        {
            this.ApplicationTypeOperationStatuses = new List<ApplicationTypeVersionOperationStatus>();
            this.ApplicationOperationStatuses = new List<ApplicationOperationStatus>();
            this.ServiceOperationStatuses = new List<ServiceOperationStatus>();
        }

        public List<ApplicationTypeVersionOperationStatus> ApplicationTypeOperationStatuses { get; set; }
        public List<ApplicationOperationStatus> ApplicationOperationStatuses { get; set; }
        public List<ServiceOperationStatus> ServiceOperationStatuses { get; set; }
    }

    public class UpgradeServiceApplicationPollResponse
    {
        public UpgradeServiceApplicationPollResponse()
        {
            this.ApplicationTypeOperationDescriptions = new List<ApplicationTypeVersionOperationDescription>();
            this.ApplicationOperationDescriptions = new List<ApplicationOperationDescription>();
            this.ServiceOperationDescriptions = new List<ServiceOperationDescription>();
        }

        public List<ApplicationTypeVersionOperationDescription> ApplicationTypeOperationDescriptions { get; set; }
        public List<ApplicationOperationDescription> ApplicationOperationDescriptions { get; set; }
        [JsonConverter(typeof(ServiceOperationDescriptionConverter))]
        public List<ServiceOperationDescription> ServiceOperationDescriptions { get; set; }
    }

    public class ApplicationTypeVersionOperationStatus : IOperationStatus
    {
        public ApplicationTypeVersionOperationStatus()
        {
        }

        public ApplicationTypeVersionOperationStatus(ApplicationTypeVersionOperationDescription description)
        {
            description.ThrowIfNull("description");

            this.ResourceId = description.ResourceId;
            this.OperationType = description.OperationType;
            this.OperationSequenceNumber = description.OperationSequenceNumber;
            this.DefaultParameterList = new Dictionary<string, string>();
        }

        public string ResourceId { get; set; }
        public OperationType OperationType { get; set; }
        public long OperationSequenceNumber { get; set; }
        public ResultStatus Status { get; set; }
        public JObject Progress { get; set; }
        public JObject ErrorDetails { get; set; }
        public Dictionary<string, string> DefaultParameterList { get; set; }
    }

    public class ApplicationTypeVersionOperationDescription : OperationDescription
    {
        public ApplicationTypeVersionOperationDescription()
        {
        }

        public ApplicationTypeVersionOperationDescription(string resourceId)
            : base(resourceId, ResourceType.ApplicationType)
        {
        }

        public string AppPackageUrl { get; set; }
        public string TypeName { get; set; }
        public string TypeVersion { get; set; }
    }

    public class ApplicationOperationDescription : OperationDescription
    {
        public ApplicationOperationDescription()
        {
        }

        public ApplicationOperationDescription(string resourceId)
            : base(resourceId, ResourceType.Application)
        {
        }

        public string TypeName { get; set; }
        public string TypeVersion { get; set; }
        public string ApplicationName { get; set; }
        public Uri ApplicationUri { get; set; }
        public Dictionary<string, string> Parameters { get; set; }
        public ArmApplicationUpgradePolicy UpgradePolicy { get; set; }
        public long? MaximumNodes { get; set; }
        public long? MinimumNodes { get; set; }
        public bool RemoveApplicationCapacity { get; set; }
        public List<ArmApplicationMetricDescription> Metrics { get; set; }
    }

    public class ArmApplicationUpgradePolicy
    {
        public ArmApplicationUpgradePolicy()
        {
        }

        public ArmApplicationUpgradePolicy(ArmApplicationUpgradePolicy other)
        {
            other.ThrowIfNull("other");

            if (other.ApplicationHealthPolicy != null)
            {
                this.ApplicationHealthPolicy = new ArmApplicationHealthPolicy(other.ApplicationHealthPolicy);
            }

            if (other.RollingUpgradeMonitoringPolicy != null)
            {
                this.RollingUpgradeMonitoringPolicy = new ArmRollingUpgradeMonitoringPolicy(other.RollingUpgradeMonitoringPolicy);
            }

            this.UpgradeReplicaSetCheckTimeout = other.UpgradeReplicaSetCheckTimeout;
            this.ForceRestart = other.ForceRestart;
        }

        public ArmApplicationHealthPolicy ApplicationHealthPolicy { get; set; }
        public ArmRollingUpgradeMonitoringPolicy RollingUpgradeMonitoringPolicy { get; set; }
        public TimeSpan? UpgradeReplicaSetCheckTimeout { get; set; }
        public bool? ForceRestart { get; set; }
    }

    public class ArmApplicationMetricDescription
    {
        public ArmApplicationMetricDescription()
        {
        }

        public ArmApplicationMetricDescription(ArmApplicationMetricDescription other)
        {
            other.ThrowIfNull("other");

            this.Name = other.Name;
            this.ReservationCapacity = other.ReservationCapacity;
            this.MaximumCapacity = other.MaximumCapacity;
            this.TotalApplicationCapacity = other.TotalApplicationCapacity;
        }

        public string Name { get; set; }
        public long ReservationCapacity { get; set; }
        public long MaximumCapacity { get; set; }
        public long TotalApplicationCapacity { get; set; }
    }

    public class ArmApplicationHealthPolicy
    {
        public ArmApplicationHealthPolicy()
        {
        }

        public ArmApplicationHealthPolicy(ArmApplicationHealthPolicy other)
        {
            other.ThrowIfNull("other");

            this.ConsiderWarningAsError = other.ConsiderWarningAsError;
            this.MaxPercentUnhealthyDeployedApplications = other.MaxPercentUnhealthyDeployedApplications;

            if (other.DefaultServiceTypeHealthPolicy != null)
            {
                this.DefaultServiceTypeHealthPolicy = new ArmServiceTypeHealthPolicy(other.DefaultServiceTypeHealthPolicy);
            }
        }

        public bool ConsiderWarningAsError { get; set; }
        public byte MaxPercentUnhealthyDeployedApplications { get; set; }
        public ArmServiceTypeHealthPolicy DefaultServiceTypeHealthPolicy { get; set; }
    }

    public class ArmServiceTypeHealthPolicy
    {
        public ArmServiceTypeHealthPolicy()
        {
        }

        public ArmServiceTypeHealthPolicy(ArmServiceTypeHealthPolicy other)
        {
            other.ThrowIfNull("other");

            this.MaxPercentUnhealthyServices = other.MaxPercentUnhealthyServices;
            this.MaxPercentUnhealthyPartitionsPerService = other.MaxPercentUnhealthyPartitionsPerService;
            this.MaxPercentUnhealthyReplicasPerPartition = other.MaxPercentUnhealthyReplicasPerPartition;
        }

        public byte MaxPercentUnhealthyServices { get; set; }
        public byte MaxPercentUnhealthyPartitionsPerService { get; set; }
        public byte MaxPercentUnhealthyReplicasPerPartition { get; set; }
    }

    public enum ArmUpgradeFailureAction
    {
        Invalid,
        Rollback,
        Manual
    }

    public class ArmRollingUpgradeMonitoringPolicy
    {
        public ArmRollingUpgradeMonitoringPolicy()
        {
            this.FailureAction = ArmUpgradeFailureAction.Rollback;
        }

        public ArmRollingUpgradeMonitoringPolicy(ArmRollingUpgradeMonitoringPolicy other)
        {
            other.ThrowIfNull("other");

            this.FailureAction = other.FailureAction;
            this.HealthCheckRetryTimeout = other.HealthCheckRetryTimeout;
            this.HealthCheckWaitDuration = other.HealthCheckWaitDuration;
            this.HealthCheckStableDuration = other.HealthCheckStableDuration;
            this.UpgradeDomainTimeout = other.UpgradeDomainTimeout;
            this.UpgradeTimeout = other.UpgradeTimeout;
        }

        public ArmUpgradeFailureAction FailureAction { get; set; }
        public TimeSpan HealthCheckRetryTimeout { get; set; }
        public TimeSpan HealthCheckWaitDuration { get; set; }
        public TimeSpan HealthCheckStableDuration { get; set; }
        public TimeSpan UpgradeDomainTimeout { get; set; }
        public TimeSpan UpgradeTimeout { get; set; }
    }

    public class ApplicationOperationStatus : IOperationStatus
    {
        public ApplicationOperationStatus()
        {
        }

        public ApplicationOperationStatus(ApplicationOperationDescription description)
        {
            description.ThrowIfNull("description");

            this.ResourceId = description.ResourceId;
            this.OperationType = description.OperationType;
            this.OperationSequenceNumber = description.OperationSequenceNumber;
        }

        public string ResourceId { get; set; }
        public OperationType OperationType { get; set; }
        public long OperationSequenceNumber { get; set; }
        public ResultStatus Status { get; set; }
        public JObject Progress { get; set; }
        public JObject ErrorDetails { get; set; }
    }

    public class ServiceOperationDescription : OperationDescription
    {
        protected ServiceOperationDescription()
        { }

        protected ServiceOperationDescription(string resourceId, ArmServiceKind kind)
            : base(resourceId, ResourceType.Service)
        {
            this.ServiceKind = kind;
        }

        #region service common properties
        public ArmServiceKind ServiceKind { get; set; }
        public string PlacementConstraints { get; set; }
        public string ServiceTypeName { get; set; }
        public Uri ApplicationName { get; set; }
        public Uri ServiceName { get; set; }
        [JsonConverter(typeof(PartitionDescriptionConverter))]
        public ArmPartitionDescription PartitionDescription { get; set; }
        public List<ArmServiceLoadMetrics> ServiceLoadMetrics { get; set; }
        public List<ArmServiceCorrelationDescription> CorrelationScheme { get; set; }
        public List<ArmServicePlacementPolicyDescription> ServicePlacementPolicies { get; set; }
        public ArmMoveCost? DefaultMoveCost { get; set; }
        #endregion
    }

    public class ServiceOperationDescriptionConverter : JsonConverter
    {
        public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer)
        {
            serializer.Serialize(writer, value);
        }

        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer)
        {
            var serviceDescriptions = new List<ServiceOperationDescription>();
            var items = JArray.Load(reader);
            foreach (JToken item in items)
            {
                string serviceKind = item["serviceKind"].Value<string>();

                ArmServiceKind kind;
                if (!Enum.TryParse(serviceKind, true, out kind))
                {
                    throw new JsonSerializationException($"ServiceOperationDescription.ServiceKind {serviceKind} could not be parsed as {typeof(ArmServiceKind)}.");
                }

                if (kind == ArmServiceKind.Stateful)
                {
                    serviceDescriptions.Add(item.ToObject<StatefulServiceOperationDescription>());
                    continue;
                }

                if (kind == ArmServiceKind.Stateless)
                {
                    serviceDescriptions.Add(item.ToObject<StatelessServiceOperationDescription>());
                    continue;
                }

                throw new JsonSerializationException($"ServiceOperationDescription.ServiceKind {serviceKind} is not supported.");
            }

            return serviceDescriptions;
        }

        public override bool CanConvert(Type objectType)
        {
            // type.IsAssignableFrom is not supported in CoreCLR
            // return typeof(List<ServiceOperationDescription>).IsAssignableFrom(objectType);
            return typeof(List<ServiceOperationDescription>).GetTypeInfo().IsAssignableFrom(objectType);
        }
    }

    public enum ArmServiceKind
    {
        Invalid,
        Stateless,
        Stateful
    }

    /// <summary>
    /// Public representation for System.Fabric.Description.PartitionScheme
    /// </summary>
    public enum ArmPartitionScheme
    {
        Invalid,
        Singleton,
        UniformInt64Range,
        Named
    }

    /// <summary>
    /// Public representation for System.Fabric.Description.PartitionSchemeDescription
    /// </summary>
    public abstract class ArmPartitionDescription
    {
        protected ArmPartitionDescription(ArmPartitionScheme scheme)
        {
            this.PartitionScheme = scheme;
        }

        public ArmPartitionScheme PartitionScheme { get; set; }

        public abstract ArmPartitionDescription Clone();
    }

    public class PartitionDescriptionConverter : JsonConverter
    {
        public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer)
        {
            serializer.Serialize(writer, value);
        }

        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer)
        {
            JObject item = JObject.Load(reader);
            string partitionScheme = item.GetValue("PartitionScheme", StringComparison.OrdinalIgnoreCase).Value<string>();

            ArmPartitionScheme scheme;
            if (!Enum.TryParse(partitionScheme, true, out scheme))
            {
                throw new JsonSerializationException($"PartitionDescription.PartitionScheme {partitionScheme} could not be parsed as {typeof(ArmPartitionScheme)}.");
            }

            if (scheme == ArmPartitionScheme.Named)
            {
                return item.ToObject<ArmNamedPartitionDescription>();
            }
            if (scheme == ArmPartitionScheme.Singleton)
            {
                return item.ToObject<ArmSingletonPartitionDescription>();
            }
            if (scheme == ArmPartitionScheme.UniformInt64Range)
            {
                return item.ToObject<ArmUniformInt64PartitionDescription>();
            }

            throw new JsonSerializationException($"ArmPartitionDescription.PartitionScheme {partitionScheme} is not supported.");
        }

        public override bool CanConvert(Type objectType)
        {
            // type.IsAssignableFrom is not supported in CoreCLR
            // return typeof(ArmPartitionDescription).IsAssignableFrom(objectType);
            return typeof(ArmPartitionDescription).GetTypeInfo().IsAssignableFrom(objectType);
        }
    }

    /// <summary>
    /// Public representation for System.Fabric.Description.SingletonPartitionSchemeDescription
    /// </summary>
    public class ArmSingletonPartitionDescription : ArmPartitionDescription
    {
        public ArmSingletonPartitionDescription()
            : base(ArmPartitionScheme.Singleton)
        {
        }

        public override ArmPartitionDescription Clone()
        {
            return new ArmSingletonPartitionDescription();
        }

        public override bool Equals(object obj)
        {
            ArmSingletonPartitionDescription other = obj as ArmSingletonPartitionDescription;
            return this.PartitionScheme == other?.PartitionScheme;
        }

        public override int GetHashCode()
        {
            return this.PartitionScheme.GetHashCode();
        }
    }

    /// <summary>
    /// Public representation for System.Fabric.Description.UniformInt64RangePartitionSchemeDescription
    /// </summary>
    public class ArmUniformInt64PartitionDescription : ArmPartitionDescription
    {
        public ArmUniformInt64PartitionDescription()
            : base(ArmPartitionScheme.UniformInt64Range)
        { }

        public int Count { get; set; }
        public long LowKey { get; set; }
        public long HighKey { get; set; }

        public override ArmPartitionDescription Clone()
        {
            var copy = (ArmUniformInt64PartitionDescription)base.MemberwiseClone();
            copy.Count = this.Count;
            copy.LowKey = this.LowKey;
            copy.HighKey = this.HighKey;
            return copy;
        }

        public override bool Equals(object obj)
        {
            ArmUniformInt64PartitionDescription other = obj as ArmUniformInt64PartitionDescription;
            return
                this.PartitionScheme == other?.PartitionScheme &&
                this.Count == other.Count &&
                this.LowKey == other.LowKey &&
                this.HighKey == other.HighKey;
        }

        public override int GetHashCode()
        {
            var hash = 27;
            hash = (13 * hash) + this.PartitionScheme.GetHashCode();
            hash = (13 * hash) + this.Count.GetHashCode();
            hash = (13 * hash) + this.LowKey.GetHashCode();
            hash = (13 * hash) + this.HighKey.GetHashCode();

            return hash;
        }
    }

    /// <summary>
    /// Public representation for System.Fabric.Description.NamedPartitionSchemeDescription
    /// </summary>
    public class ArmNamedPartitionDescription : ArmPartitionDescription
    {
        public ArmNamedPartitionDescription()
            : base(ArmPartitionScheme.Named)
        {
            this.Names = new List<string>();
        }

        public List<string> Names { get; set; }
        public int Count { get; set; }

        public override ArmPartitionDescription Clone()
        {
            var copy = (ArmNamedPartitionDescription)base.MemberwiseClone();
            copy.Names = this.Names != null ? new List<string>(this.Names) : null;
            copy.Count = this.Count;
            return copy;
        }

        public override bool Equals(object obj)
        {
            ArmNamedPartitionDescription other = obj as ArmNamedPartitionDescription;
            if (other == null)
            {
                return false;
            }
            if (this.Count != other.Count)
            {
                return false;
            }

            if (this.Names == null && other.Names == null)
            {
                return true;
            }

            if ((this.Names != null && other.Names == null) ||
                (this.Names == null && other.Names != null))
            {
                return false;
            }

            if (this.Names != null && other.Names != null)
            {
                if (this.Names.Count != other.Names.Count)
                {
                    return false;
                }

                // Duplicate names are ignored
                var hash = new HashSet<string>(this.Names);
                return hash.SetEquals(other.Names);
            }

            return true;
        }

        public override int GetHashCode()
        {
            var hash = 27;
            hash = (13 * hash) + this.PartitionScheme.GetHashCode();
            hash = (13 * hash) + this.Names.GetHashCode();
            hash = (13 * hash) + this.Count.GetHashCode();

            return hash;
        }
    }

    public enum ArmWeight
    {
        Zero,
        Low,
        Medium,
        High
    }

    public class ArmServiceLoadMetrics
    {
        public string Name { get; set; }

        public ArmWeight Weight { get; set; }
    }

    public enum ArmServiceCorrelationScheme
    {
        Invalid,
        Affinity,
        AlignedAffinity,
        NonAlignedAffinity
    }

    public class ArmServiceCorrelationDescription
    {
        public Uri ServiceName { get; set; }
        public ArmServiceCorrelationScheme Scheme { get; set; }
    }

    [JsonConverter(typeof(ArmServicePlacementPolicyDescriptionConverter))]
    public class ArmServicePlacementPolicyDescription
    {
        protected ArmServicePlacementPolicyDescription(ArmServicePlacementPolicyType type)
        {
            this.Type = type;
        }

        public ArmServicePlacementPolicyType Type { get; set; }
    }

    public enum ArmServicePlacementPolicyType
    {
        Invalid,
        InvalidDomain,
        RequiredDomain,
        PreferredPrimaryDomain,
        RequiredDomainDistribution,
        NonPartiallyPlaceService
    }


    public class ArmServicePlacementPolicyDescriptionConverter : JsonConverter
    {
        public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer)
        {
            serializer.Serialize(writer, value);
        }

        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer)
        {
            JObject item = JObject.Load(reader);
            string policyType = item.GetValue("Type", StringComparison.OrdinalIgnoreCase).Value<string>();

            ArmServicePlacementPolicyType type;
            if (!Enum.TryParse(policyType, true, out type))
            {
                throw new JsonSerializationException($"ArmServicePlacementPolicyDescription.Type {policyType} could not be parsed as {typeof(ArmServicePlacementPolicyType)}.");
            }

            if (type == ArmServicePlacementPolicyType.InvalidDomain)
            {
                return item.ToObject<ArmServicePlacementInvalidDomainPolicyDescription>();
            }
            if (type == ArmServicePlacementPolicyType.RequiredDomain)
            {
                return item.ToObject<ArmServicePlacementRequiredDomainPolicyDescription>();
            }
            if (type == ArmServicePlacementPolicyType.PreferredPrimaryDomain)
            {
                return item.ToObject<ArmServicePlacementPreferPrimaryDomainPolicyDescription>();
            }
            if (type == ArmServicePlacementPolicyType.RequiredDomainDistribution)
            {
                return item.ToObject<ArmServicePlacementRequireDomainDistributionPolicyDescription>();
            }
            if (type == ArmServicePlacementPolicyType.NonPartiallyPlaceService)
            {
                return item.ToObject<ArmServicePlacementNonPartiallyPlaceServicePolicyDescription>();
            }

            throw new JsonSerializationException($"ArmServicePlacementPolicyDescription.Type {policyType} is not supported.");
        }

        public override bool CanConvert(Type objectType)
        {
            // type.IsAssignableFrom is not supported in CoreCLR
            // return typeof(IOperationDescription).IsAssignableFrom(objectType);
            return typeof(IOperationDescription).GetTypeInfo().IsAssignableFrom(objectType);
        }
    }

    public class ArmServicePlacementInvalidDomainPolicyDescription : ArmServicePlacementPolicyDescription
    {
        public ArmServicePlacementInvalidDomainPolicyDescription()
            : base(ArmServicePlacementPolicyType.InvalidDomain)
        {
        }

        public string DomainName { get; set; }
    }

    public class ArmServicePlacementRequiredDomainPolicyDescription : ArmServicePlacementPolicyDescription
    {
        public ArmServicePlacementRequiredDomainPolicyDescription()
            : base(ArmServicePlacementPolicyType.RequiredDomain)
        {
        }

        public string DomainName { get; set; }
    }

    public class ArmServicePlacementPreferPrimaryDomainPolicyDescription : ArmServicePlacementPolicyDescription
    {
        public ArmServicePlacementPreferPrimaryDomainPolicyDescription()
            : base(ArmServicePlacementPolicyType.PreferredPrimaryDomain)
        {
        }

        public string DomainName { get; set; }
    }

    public class ArmServicePlacementRequireDomainDistributionPolicyDescription : ArmServicePlacementPolicyDescription
    {
        public ArmServicePlacementRequireDomainDistributionPolicyDescription()
            : base(ArmServicePlacementPolicyType.RequiredDomainDistribution)
        {
        }
    }

    public class ArmServicePlacementNonPartiallyPlaceServicePolicyDescription : ArmServicePlacementPolicyDescription
    {
        public ArmServicePlacementNonPartiallyPlaceServicePolicyDescription()
            : base(ArmServicePlacementPolicyType.NonPartiallyPlaceService)
        {
        }
    }

    public enum ArmMoveCost
    {
        Zero,
        Low,
        Medium,
        High
    }

    public class StatefulServiceOperationDescription : ServiceOperationDescription
    {
        public StatefulServiceOperationDescription()
        { }

        public StatefulServiceOperationDescription(string resourceId)
            : base(resourceId, ArmServiceKind.Stateful)
        { }

        #region Properties
        public bool HasPersistedState { get; set; }
        public int TargetReplicaSetSize { get; set; }
        public int MinReplicaSetSize { get; set; }
        public TimeSpan? ReplicaRestartWaitDuration { get; set; }
        public TimeSpan? QuorumLossWaitDuration { get; set; }
        public TimeSpan? StandByReplicaKeepDuration { get; set; }
        #endregion
    }

    public class StatelessServiceOperationDescription : ServiceOperationDescription
    {
        public StatelessServiceOperationDescription()
        { }

        public StatelessServiceOperationDescription(string resourceId)
            : base(resourceId, ArmServiceKind.Stateless)
        { }

        #region Properties
        public int InstanceCount { get; set; }
        #endregion
    }

    public class ServiceOperationStatus : IOperationStatus
    {
        public ServiceOperationStatus()
        { }

        public ServiceOperationStatus(ServiceOperationDescription description)
        {
            description.ThrowIfNull("description");

            this.ResourceId = description.ResourceId;
            this.OperationType = description.OperationType;
            this.OperationSequenceNumber = description.OperationSequenceNumber;
        }

        public string ResourceId { get; set; }
        public OperationType OperationType { get; set; }
        public long OperationSequenceNumber { get; set; }
        public ResultStatus Status { get; set; }
        public JObject Progress { get; set; }
        public JObject ErrorDetails { get; set; }
    }

    public enum OperationType
    {
        CreateOrUpdate,
        Delete
    }

    public enum ResourceType
    {
        Invalid,
        Cluster,
        ApplicationType,
        Application,
        Service
    }

    public class OperationDescription : IOperationDescription
    {
        protected OperationDescription()
        {
        }

        protected OperationDescription(string resourceId, ResourceType resourceType)
        {
            this.ResourceId = resourceId;
            this.ResourceType = resourceType;
        }

        #region Properties
        public string ResourceId { get; set; }
        public ResourceType ResourceType { get; set; }
        public OperationType OperationType { get; set; }
        public long OperationSequenceNumber { get; set; }
        #endregion
    }

    public interface IOperationDescription
    {
        string ResourceId { get; }
        ResourceType ResourceType { get; }
        OperationType OperationType { get; }
        long OperationSequenceNumber { get; }
    }

    public enum ResultStatus
    {
        InProgress,
        Succeeded,
        Failed
    }

    public interface IOperationStatus
    {
        string ResourceId { get; }
        OperationType OperationType { get; }
        long OperationSequenceNumber { get; }
        ResultStatus Status { get; }
        JObject Progress { get; }
        JObject ErrorDetails { get; }
    }

}
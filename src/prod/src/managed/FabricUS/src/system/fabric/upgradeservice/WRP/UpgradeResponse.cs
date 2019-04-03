// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.WRP.Model
{
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Query;    

    public class ClusterOperationError
    {
        public ClusterOperation Operation { get; set; }
        public bool Transient { get; set; }
        public JObject ErrorDetails { get; set; }
    }

    /// <summary>
    /// Cluster errror details sent in operation status
    /// </summary>
    public class ClusterErrorDetails
    {
        public ClusterErrorDetails()
        {
            this.Errors = new List<ClusterOperationError>();
        }

        public List<ClusterOperationError> Errors { get; set; }
    }


    public class ClusterOperationStatus : IOperationStatus
    {
        public ClusterOperationStatus()
        {         
        }

        /// <summary>
        /// To be used by Fabric US to reply to SFRP.
        /// </summary>
        /// <param name="response">SFRP response to the poll request</param>
        public ClusterOperationStatus(ClusterOperationDescription response)
        {
            if (response == null)
            {
                // Poll reqeuest response can be null in cluster reached Ready state in SFRP side.
                // We want to always be able to push a status for nodes.
                return;
            }

            this.ResourceId = response.ResourceId;
            this.OperationType = response.OperationType;
            this.OperationSequenceNumber = response.OperationSequenceNumber;            
        }

        #region IOperationStatus
        public string ResourceId { get; set; }
        public OperationType OperationType { get; set; }
        public long OperationSequenceNumber { get; set; }
        public ResultStatus Status { get; set; }
                
        [JsonProperty("UpgradeProgress")]
        public JObject Progress { get; set; }

        /// <summary>
        /// <para>Gets or sets the error details for the operation status.</para>
        /// 
        /// <para>This is filed with an instance of ClusterErrorDetails class.</para>
        /// </summary>
        public JObject ErrorDetails { get; set; }
        #endregion        

        public JObject ClusterHealth { get; set; }

        public List<PaasNodeStatusInfo> DisabledNodes { get; set; }

        public List<PaasNodeStatusInfo> EnabledNodes { get; set; }

        public List<PaasNodeStatusInfo> NodesStatus { get; set; }

        public Dictionary<string, ServiceRuntimeDescription> SystemServiceDescriptions { get; set; }
    }

    public class ClusterOperationDescription : OperationDescription
    {
        public ClusterOperationDescription(string resourceId)
                    :
                    base(resourceId, ResourceType.Cluster)
        {
        }

        public PaasClusterUpgradeDescription UpgradeDescription { get; set; }

        public List<PaasNodeStatusInfo> NodesToEnabled { get; set; }

        public List<PaasNodeStatusInfo> NodesToDisabled { get; set; }

        public List<PaasNodeStatusInfo> ProcessedNodesStatus { get; set; }

        public Dictionary<string, ServiceRuntimeDescription> SystemServiceDescriptionsToSet { get; set; }

        public List<string> PrimaryNodeTypes { get; set; }
    }

    public class ServiceRuntimeDescription
    {
        public int TargetReplicaSetSize { get; set; }

        public int MinReplicaSetSize { get; set; }

        public string PlacementConstraints { get; set; } 
    }

    public class PaasNodeStatusInfo
    {
        public PaasNodeStatusInfo()
        {
        }

        public PaasNodeStatusInfo(PaasNodeStatusInfo other)
        {
            if (other != null)
            {
                this.NodeName = other.NodeName;
                this.NodeType = other.NodeType;
                this.NodeState = other.NodeState;
                this.IntentionInstance = other.IntentionInstance;
            }
        }

        public PaasNodeStatusInfo(Node queryNode)
        {
            this.NodeName = queryNode.NodeName;
            this.NodeType = queryNode.NodeType;
            this.NodeState = queryNode.ToNodeState();
            this.NodeDeactivationIntent = (queryNode.NodeDeactivationInfo != null) ? queryNode.NodeDeactivationInfo.EffectiveIntent : NodeDeactivationIntent.Invalid;
        }

        public string NodeName { get; set; }

        public string NodeType { get; set; }

        public NodeState NodeState { get; set; }

        public NodeDeactivationIntent NodeDeactivationIntent { get; set; }

        public long IntentionInstance { get; set; }

        public override bool Equals(object obj)
        {
            var other = obj as PaasNodeStatusInfo;

            if (other == null ||
                this.NodeName != other.NodeName ||
                this.NodeType != other.NodeType ||
                this.NodeState != other.NodeState ||
                this.NodeDeactivationIntent != other.NodeDeactivationIntent)
            {
                return false;
            }

            return true;
        }

        public override int GetHashCode()
        {
            var hash = 27;
            hash = (13 * hash) + this.NodeName.GetHashCode();
            hash = (13 * hash) + this.NodeType.GetHashCode();
            hash = (13 * hash) + this.NodeState.GetHashCode();
            hash = (13 * hash) + this.NodeDeactivationIntent.GetHashCode();
            return hash;
        }

        public override string ToString()
        {
            return string.Format(
                "[NodeName={0}, NodeType={1}, NodeState={2}, NodeDeactivationIntent={3}, Instance={4}]", 
                this.NodeName, 
                this.NodeType, 
                this.NodeState, 
                this.NodeDeactivationIntent, 
                this.IntentionInstance);
        }
    }

    public enum NodeState
    {
        Enabling,
        Enabled,
        Disabling,
        Disabled,
        Removed,
        Unknown
    }

    public static class NodeExtension
    {
        public static NodeState ToNodeState(this Node node)
        {
            switch (node.NodeStatus)
            {                
                case NodeStatus.Enabling: return NodeState.Enabling;
                case NodeStatus.Disabling: return NodeState.Disabling;
                case NodeStatus.Disabled: return NodeState.Disabled;
                case NodeStatus.Removed: return NodeState.Removed;
                case NodeStatus.Up:
                case NodeStatus.Down: 
                    return (node.NodeDeactivationInfo == null || node.NodeDeactivationInfo.EffectiveIntent == NodeDeactivationIntent.Invalid) ? NodeState.Enabled : NodeState.Disabled;
                case NodeStatus.Unknown: return NodeState.Unknown;
                case NodeStatus.Invalid: return NodeState.Unknown;
                    
                default:
                    ReleaseAssert.Failfast("Mapping of {0} to NodeState is not available", node.NodeStatus);
                    break;
            }

            return NodeState.Unknown;
        }
    }

    // The response is returned by WRP to the PaaS coordinator service
    // when it receives the UpgradeRequest
    public class PaasClusterUpgradeDescription
    {
        public string TargetClusterManifestBase64 { get; set; }

        public Uri TargetMsiUri { get; set; }

        public string TargetMsiVersion { get; set; }

        // The upgrade policy to use
        public PaasClusterUpgradePolicy UpgradePolicy { get; set; }

        public override string ToString()
        {
            return string.Format("MsiVersion: {0}, IsClusterManifestSpecfied: {1}", this.TargetMsiVersion, !string.IsNullOrEmpty(this.TargetClusterManifestBase64));
        }
    }

    public class PaasClusterUpgradePolicy
    {
        public bool ForceRestart { get; set; }

        public TimeSpan UpgradeReplicaSetCheckTimeout { get; set; }

        public TimeSpan HealthCheckWaitDuration { get; set; }

        public TimeSpan HealthCheckStableDuration { get; set; }

        public TimeSpan HealthCheckRetryTimeout { get; set; }

        public TimeSpan UpgradeTimeout { get; set; }

        public TimeSpan UpgradeDomainTimeout { get; set; }

        public PaasClusterUpgradeHealthPolicy HealthPolicy { get; set; }

        public PaasClusterUpgradeDeltaHealthPolicy DeltaHealthPolicy { get; set; }
    }

    public class PaasClusterUpgradeHealthPolicy
    {
        public byte MaxPercentUnhealthyNodes { get; set; }

        public byte MaxPercentUnhealthyApplications { get; set; }

        public Dictionary<string, PaasApplicationHealthPolicy> ApplicationHealthPolicies { get; set; }
    }

    public class PaasClusterUpgradeDeltaHealthPolicy
    {
        public byte MaxPercentDeltaUnhealthyNodes { get; set; }

        public byte MaxPercentUpgradeDomainDeltaUnhealthyNodes { get; set; }

        // Application health
        public byte MaxPercentDeltaUnhealthyApplications { get; set; }

        public Dictionary<string, PaasApplicationDeltaHealthPolicy> ApplicationDeltaHealthPolicies { get; set; }
    }

    public class PaasApplicationHealthPolicy
    {
        public PaasServiceTypeHealthPolicy DefaultServiceTypeHealthPolicy { get; set; }

        public Dictionary<string, PaasServiceTypeHealthPolicy> SerivceTypeHealthPolicies { get; set; }
    }

    public class PaasServiceTypeHealthPolicy
    {
        public byte MaxPercentUnhealthyServices { get; set; }
    }

    public class PaasApplicationDeltaHealthPolicy
    {
        public PaasServiceTypeDeltaHealthPolicy DefaultServiceTypeDeltaHealthPolicy { get; set; }

        public Dictionary<string, PaasServiceTypeDeltaHealthPolicy> SerivceTypeDeltaHealthPolicies { get; set; }
    }

    public class PaasServiceTypeDeltaHealthPolicy
    {
        public byte MaxPercentDeltaUnhealthyServices { get; set; }
    }
}
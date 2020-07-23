// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Management.ServiceModel;
    using System.Linq;
    using Newtonsoft.Json;

    [JsonObject]
    internal abstract class ScaleClusterUpgradeBase : ClusterUpgradeStateBase
    {
        private static readonly TraceType TraceType = new TraceType("ScaleClusterUpgradeBase");

        [JsonConstructor]
        protected ScaleClusterUpgradeBase(
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            ClusterNodeConfig targetNodeConfig,
            ICluster clusterResource,
            Dictionary<string, int> nodeDiffCountList,
            ITraceLogger traceLogger)
        : base(targetCsmConfig, targetWrpConfig, targetNodeConfig, clusterResource, traceLogger)
        {
            this.NodeDiffCountList = nodeDiffCountList;
        }

        internal Dictionary<string, ReplicaSetSize> TargetSystemServicesSize { get; set; }

        [JsonProperty]
        internal Dictionary<string, int> NodeDiffCountList { get; set; }

        public override bool CanInterruptUpgrade()
        {
            return false;
        }

        //// public override PaasClusterUpgradePolicy GetUpgradePolicy(RolloutGoalState goalState)
        //// {
        ////    return ForceUpgradePolicy;
        //// }

        internal override IEnumerable<NodeStatus> GetNodesToDisable(List<NodeStatus> disabledNodes)
        {
            List<NodeStatus> nodesToBeDisabled = new List<NodeStatus>();

            if (disabledNodes != null)
            {
                var reportedDisabledNodes = disabledNodes.Where(n => n.NodeState == NodeState.Disabled).ToList();

                if (reportedDisabledNodes.Count > 0)
                {
                    foreach (var reportedNode in reportedDisabledNodes)
                    {
                        SortedList<string, NodeStatus> nodeList;
                        if (this.ClusterResource.NodeTypeNodeStatusList.TryGetValue(reportedNode.NodeType, out nodeList))
                        {
                            NodeStatus node;
                            if (nodeList.TryGetValue(reportedNode.NodeName, out node))
                            {
                                this.TraceLogger.WriteInfo(
                                    TraceType,
                                    "GetNodesToDisable:\r\nNodeStatus: {0}\r\nReportedNodeStatus: {1}",
                                    node,
                                    reportedNode);

                                if (reportedNode.InstanceId == node.InstanceId)
                                {
                                    node.NodeState = reportedNode.NodeState;
                                }
                            }
                        }
                    }
                }
            }

            foreach (var nodeTypeNodeList in this.ClusterResource.NodeTypeNodeStatusList)
            {
                var toBeDisabledNodeList = nodeTypeNodeList.Value.Where(
                                               n => n.Value.NodeState == NodeState.Disabling).Select(
                                               p => new NodeStatus()
                {
                    NodeName = p.Key,
                    NodeType = p.Value.NodeType,
                    NodeState = NodeState.Disabling,
                    InstanceId = p.Value.InstanceId
                }).ToList();

                nodesToBeDisabled.AddRange(toBeDisabledNodeList);
            }

            return nodesToBeDisabled.Count > 0 ? nodesToBeDisabled : null;
        }

        internal override IEnumerable<NodeStatus> GetNodesToEnable(List<NodeStatus> enabledNodes)
        {
            var nodesToBeEnabled = new List<NodeStatus>();

            if (enabledNodes != null)
            {
                var reportedEnabledNodes = enabledNodes.Where(n => n.NodeState == NodeState.Enabled).ToList();
                if (reportedEnabledNodes.Count > 0)
                {
                    foreach (var reportedNode in reportedEnabledNodes)
                    {
                        SortedList<string, NodeStatus> nodeList;
                        if (this.ClusterResource.NodeTypeNodeStatusList.TryGetValue(reportedNode.NodeType, out nodeList))
                        {
                            NodeStatus node;
                            if (nodeList.TryGetValue(reportedNode.NodeName, out node))
                            {
                                this.TraceLogger.WriteInfo(
                                    TraceType,
                                    "GetNodesToEnable:\r\nNodeStatus: {0}\r\nReportedNodeStatus: {1}",
                                    node,
                                    reportedNode);

                                if (reportedNode.InstanceId == node.InstanceId)
                                {
                                    node.NodeState = reportedNode.NodeState;
                                }
                            }
                        }
                    }
                }
            }

            foreach (var nodeTypeNodeList in this.ClusterResource.NodeTypeNodeStatusList)
            {
                var toBeDisabledNodeList = nodeTypeNodeList.Value.Where(
                                               n => n.Value.NodeState == NodeState.Enabling).Select(
                                               p => new NodeStatus()
                {
                    NodeName = p.Key,
                    NodeType = p.Value.NodeType,
                    NodeState = NodeState.Enabling,
                    InstanceId = p.Value.InstanceId
                }).ToList();

                nodesToBeEnabled.AddRange(toBeDisabledNodeList);
            }

            return nodesToBeEnabled.Count > 0 ? nodesToBeEnabled : null;
        }

        internal override IDictionary<string, ReplicaSetSize> GetSystemServiceReplicaSetSize(IDictionary<string, ReplicaSetSize> currentSystemServicesSize)
        {
            if (this.TargetSystemServicesSize == null)
            {
                return null;
            }

            if (currentSystemServicesSize == null)
            {
                return this.TargetSystemServicesSize;
            }

            Dictionary<string, ReplicaSetSize> updatedTargetSystemServicesSize = new Dictionary<string, ReplicaSetSize>(StringComparer.OrdinalIgnoreCase);
            foreach (var currentSystemServiceSize in currentSystemServicesSize)
            {
                var systemServiceName = currentSystemServiceSize.Key;
                var currentReplicaSetSize = currentSystemServiceSize.Value;

                ReplicaSetSize targetReplicaSetSize;
                if (this.TargetSystemServicesSize.TryGetValue(systemServiceName, out targetReplicaSetSize))
                {
                    if (!targetReplicaSetSize.Equals(currentReplicaSetSize))
                    {
                        // system service has not reached target. Send the target size in response
                        updatedTargetSystemServicesSize.Add(currentSystemServiceSize.Key, targetReplicaSetSize);
                    }
                    else
                    {
                        this.TraceLogger.WriteInfo(
                            TraceType,
                            "SystemService {0} has reached target size {1}",
                            systemServiceName,
                            targetReplicaSetSize);
                    }
                }
            }

            // The update target is the new target.
            // This also takes care of removing system services in TargetSystemServicesSize
            // but not present in the cluster. They should no longer be in target.
            this.TargetSystemServicesSize = updatedTargetSystemServicesSize;

            return updatedTargetSystemServicesSize.Any() ? updatedTargetSystemServicesSize : null;
        }

        protected void SetTargetSystemServicesReplicaSetSize()
        {
            if (this.TargetSystemServicesSize != null)
            {
                // The target is already set
                return;
            }

            Dictionary<string, ReplicaSetSize> systemServicesReplicaSetSize = new Dictionary<string, ReplicaSetSize>(StringComparer.OrdinalIgnoreCase);

            var systemServicesSupportedForScale = this.ClusterResource.ClusterManifestGeneratorSettings.SystemServicesForScale;
            ReplicaSetSize replicaSetSizeForReliability = this.TargetCsmConfig.ReliabilityLevel.GetReplicaSetSize();

            foreach (var systemServiceSuppotedForScale in systemServicesSupportedForScale)
            {
                systemServicesReplicaSetSize.Add(systemServiceSuppotedForScale.Value, replicaSetSizeForReliability);
            }

            // InfrastructureService name is not well known like other system services. Hence it is handled seaprately
            foreach (var nodeType in this.TargetCsmConfig.NodeTypes)
            {
                if (nodeType.DurabilityLevel != DurabilityLevel.Bronze)
                {
                    // IS is enabled. Hence adjust its replica set size
                    var infraSvcName = string.Format("fabric:/System/{0}/{1}", StringConstants.SectionName.InfrastructureServicePrefix, nodeType.Name);
                    systemServicesReplicaSetSize.Add(infraSvcName, replicaSetSizeForReliability);
                }
            }

            this.TargetSystemServicesSize = systemServicesReplicaSetSize;

            this.TraceLogger.WriteInfo(
                TraceType,
                "Initiating adjustment of system service replica set size.");
        }

        protected bool VerifyTargetSystemServicesReplicaSetSize()
        {
            if (this.TargetSystemServicesSize == null || this.TargetSystemServicesSize.Any())
            {
                return false;
            }
            else
            {
                return true;
            }
        }

        protected bool UpdateClusterResourceNodesStatus()
        {
            var upgradeInProgress = false;

            foreach (var nodeDiffCount in this.NodeDiffCountList)
            {
                if (nodeDiffCount.Value == 0)
                {
                    continue;
                }

                if (nodeDiffCount.Value > 0)
                {
                    upgradeInProgress |= this.UpdateTopologyWithNodesToBeDisabled(nodeDiffCount.Key);
                }
                else
                {
                    upgradeInProgress |= this.UpdateTopologyWithNodesToBeEnabled(nodeDiffCount.Key);
                }
            }

            return upgradeInProgress;
        }

        protected bool UpdateTopologyWithNodesToBeDisabled(string nodeTypeName)
        {
            var nodesToBeDisabled = this.GetNodesToBeDisabled(nodeTypeName, this.NodeDiffCountList[nodeTypeName]);

            if (nodesToBeDisabled.All(n => n.NodeState == NodeState.Disabled))
            {
                this.TraceLogger.WriteInfo(
                    TraceType,
                    "UpdateTopologyWithNodesToBeDisabled: No update required");

                return false;
            }

            var nodesStillToBeDisabled =
                nodesToBeDisabled.Where(n => n.NodeState == NodeState.Enabled || n.NodeState == NodeState.Enabling).Select(x => x.NodeName).ToList();

            this.ChangeNodesStatus(nodeTypeName, nodesStillToBeDisabled, NodeState.Disabling);

            return true;
        }

        protected bool UpdateTopologyWithNodesToBeEnabled(string nodeTypeRef)
        {
            var nodesToBeEnabled = this.GetNodesToBeEnabled(nodeTypeRef);

            if (nodesToBeEnabled.All(n => n.NodeState == NodeState.Enabled))
            {
                this.TraceLogger.WriteInfo(
                    TraceType,
                    "UpdateTopologyWithNodesToBeEnabled: No update required");

                return false;
            }

            var nodesStillToBeEnabled =
                nodesToBeEnabled.Where(n => n.NodeState == NodeState.Disabled || n.NodeState == NodeState.Disabling).Select(x => x.NodeName).ToList();

            this.ChangeNodesStatus(nodeTypeRef, nodesStillToBeEnabled, NodeState.Enabling);

            return true;
        }

        protected List<NodeStatus> GetNodesToBeEnabled(string nodeTypeName)
        {
            SortedList<string, NodeStatus> nodeTypeNodeStatusList;
            List<NodeStatus> nodesToBeEnabled = null;

            if (this.ClusterResource.NodeTypeNodeStatusList.TryGetValue(nodeTypeName, out nodeTypeNodeStatusList))
            {
                var targetNodeType =
                    this.TargetCsmConfig.NodeTypes.FirstOrDefault(p => p.Name == nodeTypeName);

                if (targetNodeType != null)
                {
                    var count = (targetNodeType.VMInstanceCount > nodeTypeNodeStatusList.Count)
                                ? nodeTypeNodeStatusList.Count
                                : targetNodeType.VMInstanceCount;

                    nodesToBeEnabled = nodeTypeNodeStatusList.Values.ToList().GetRange(0, count);
                }
            }

            return nodesToBeEnabled;
        }

        protected List<NodeStatus> GetNodesToBeDisabled(string nodeTypeName, int toBeDisabledNodeCount)
        {
            List<NodeStatus> nodesToBeDisabled = new List<NodeStatus>();
            SortedList<string, NodeStatus> nodeTypeNodeStatusList = this.ClusterResource.NodeTypeNodeStatusList[nodeTypeName];

            ReleaseAssert.AssertIfNull(nodeTypeNodeStatusList, "nodeTypeNodeStatusList");

            var seedNodes =
                new HashSet<string>(((ClusterManifestTypeInfrastructurePaaS)this.ExternalState.ClusterManifest.Infrastructure.Item).Votes.Select(vote => vote.NodeName));

            foreach (var nodeStatusKeyValue in nodeTypeNodeStatusList.Reverse())
            {
                if (toBeDisabledNodeCount <= 0)
                {
                    break;
                }

                var nodeStatus = nodeStatusKeyValue.Value;
                if (!seedNodes.Contains(nodeStatus.NodeName))
                {
                    nodesToBeDisabled.Add(nodeStatus);
                    toBeDisabledNodeCount--;
                }
            }

            if (toBeDisabledNodeCount > 0)
            {
                this.TraceLogger.WriteError(
                    TraceType,
                    "{0} nodes in NodeType {1} could be disabled since the are no more nodes left which are not seed nodes. Total current nodes in node type: {2}",
                    toBeDisabledNodeCount,
                    nodeTypeName,
                    nodeTypeNodeStatusList.Count);
            }

            return nodesToBeDisabled;
        }

        protected bool VerifyExpectedTopologyStateReached()
        {
            foreach (var nodeDiffCount in this.NodeDiffCountList)
            {
                if (nodeDiffCount.Value > 0
                    && this.GetNodesToBeDisabled(nodeDiffCount.Key, nodeDiffCount.Value).Any(n => n.NodeState != NodeState.Disabled))
                {
                    return false;
                }
                else if (nodeDiffCount.Value < 0
                         && this.GetNodesToBeEnabled(nodeDiffCount.Key).Any(n => n.NodeState != NodeState.Enabled))
                {
                    return false;
                }
            }

            return true;
        }

        private void ChangeNodesStatus(string nodeTypeRef, List<string> nodeList, NodeState nodeState)
        {
            SortedList<string, NodeStatus> nodeTypeNodeStatusList;
            this.ClusterResource.NodeTypeNodeStatusList.TryGetValue(nodeTypeRef, out nodeTypeNodeStatusList);

            if (nodeTypeNodeStatusList != null)
            {
                nodeList.ForEach(
                    (node) =>
                    {
                        var matchingNodeStatus = nodeTypeNodeStatusList[node];
                        if (matchingNodeStatus != null)
                        {
                            this.TraceLogger.WriteInfo(
                            TraceType,
                            "Before ChangeNodesStatus: {0}",
                            matchingNodeStatus);

                            matchingNodeStatus.NodeState = nodeState;
                            matchingNodeStatus.InstanceId += 1;

                            this.TraceLogger.WriteInfo(
                            TraceType,
                            "After ChangeNodesStatus: {0}",
                            matchingNodeStatus);
                        }
                    });
            }
        }
    }
}
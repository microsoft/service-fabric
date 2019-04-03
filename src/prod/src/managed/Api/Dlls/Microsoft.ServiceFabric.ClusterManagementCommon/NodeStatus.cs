// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using Newtonsoft.Json;

    public enum NodeState
    {
        Enabling,
        Enabled,
        Disabling,
        Disabled,
        Removed,
        Invalid
    }

    public enum WrpNodeDeactivationIntent
    {
        Invalid,
        Pause,
        Restart,
        RemoveData,
        RemoveNode
    }

    public class NodeStatus
    {
        public NodeStatus()
        {
        }

        [JsonConstructor]
        public NodeStatus(string nodeName, string nodeType, NodeState nodeState, long instanceId)
        {
            nodeName.MustNotBeNull("nodeName");
            nodeType.MustNotBeNull("nodeType");

            this.NodeName = nodeName;
            this.NodeType = nodeType;
            this.InstanceId = instanceId;
            this.NodeState = nodeState;
            this.NodeDeactivationIntent = WrpNodeDeactivationIntent.Invalid;
        }

        public NodeStatus(NodeStatus nodeStatus)
        {
            this.NodeName = nodeStatus.NodeName;
            this.NodeType = nodeStatus.NodeType;
            this.NodeState = nodeStatus.NodeState;
            this.NodeDeactivationIntent = nodeStatus.NodeDeactivationIntent;
            this.InstanceId = nodeStatus.InstanceId;
        }

        public string NodeName { get; set; }

        public string NodeType { get; set; }

        public NodeState NodeState { get; set; }

        public WrpNodeDeactivationIntent NodeDeactivationIntent { get; set; }

        public long InstanceId { get; set; }

        public override string ToString()
        {
            return string.Format(
                       "[NodeName: {0}, NodeType: {1}, NodeState: {2}, InstanceId: {3}]",
                       this.NodeName,
                       this.NodeType,
                       this.NodeState,
                       this.InstanceId);
        }
    }
}
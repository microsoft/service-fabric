// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Health;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Query;
    using System.Numerics;

    internal class NodeInfo : IEquatable<NodeInfo>
    {
        public const string ClientConnectionEndpoint = "ClientConnectionEndpoint";
        public const string HttpGatewayEndpoint = "HttpGatewayEndpoint";

        private NodeInfo(string ipAddessOrFQDN, string nodeName, NodeId nodeId, BigInteger instanceId, bool isSeedNode, HealthState healthState)
        {
            this.NodeName = nodeName;
            this.NodeId = nodeId;
            this.NodeInstanceId = instanceId;
            this.IsSeedNode = isSeedNode;
            this.IPAddressOrFQDN = ipAddessOrFQDN;
            this.HealthState = healthState;
        }

        public string IPAddressOrFQDN
        {
            get;
            private set;
        }

        public string NodeName
        {
            get;
            private set;
        }

        public NodeId NodeId
        {
            get;
            private set;
        }

        public BigInteger NodeInstanceId
        {
            get;
            private set;
        }

        public bool IsSeedNode
        {
            get;
            set;
        }

        public string UpgradeDomain
        {
            get;
            set;
        }

        public Uri FaultDomain
        {
            get;
            set;
        }

        public bool IsNodeUp
        {
            get;
            set;
        }

        public HealthState HealthState
        {
            get;
            set;
        }

        public string NodeType
        {
            get;
            set;
        }

        internal static NodeInfo CreateNodeInfo(Node nodeResult)
        {
            ThrowIf.Null(nodeResult, "nodeResult");
            NodeInfo nodeInfo = new NodeInfo(nodeResult.IpAddressOrFQDN, nodeResult.NodeName, nodeResult.NodeId, nodeResult.NodeInstanceId, nodeResult.IsSeedNode, nodeResult.HealthState)
            {
                UpgradeDomain = nodeResult.UpgradeDomain,
                FaultDomain = nodeResult.FaultDomain
            };

            nodeInfo.IsNodeUp = nodeResult.IsNodeUp();
            nodeInfo.NodeType = nodeResult.NodeType;

            return nodeInfo;
        }

        public override string ToString()
        {
            return StringHelper.Format(
                "NodeId: {0} NodeName: {1} NetworkName: {2} IsSeedNode: {3} UpgradeDomain: {4} FaultDomain: {5} IsNodeUp: {6}, HealthState: {7}, NodeType: {8}",
                this.NodeId,
                this.NodeName,
                this.IPAddressOrFQDN,
                this.IsSeedNode,
                this.UpgradeDomain,
                this.FaultDomain,
                this.IsNodeUp,
                this.HealthState,
                this.NodeType);
        }

        public NodeInfo Clone()
        {
            NodeInfo nodeInfo = new NodeInfo(this.IPAddressOrFQDN, this.NodeName, this.NodeId, this.NodeInstanceId, this.IsSeedNode, this.HealthState)
            {
                UpgradeDomain = this.UpgradeDomain,
                FaultDomain = this.FaultDomain
            };

            nodeInfo.IsNodeUp = this.IsNodeUp;
            nodeInfo.NodeType = this.NodeType;

            return nodeInfo;
        }

        public bool Equals(NodeInfo other)
        {
            if (other == null)
            {
                return false;
            }

            // NodeName maps to NodeId, so two nodes in a cluster cannot have the same name
            return string.Equals(this.NodeName, other.NodeName, StringComparison.Ordinal);
        }

        public override int GetHashCode()
        {
            return this.NodeName.GetHashCode();
        }
    }
}
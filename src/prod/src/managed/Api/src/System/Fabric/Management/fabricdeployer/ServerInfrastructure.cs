// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.FabricDeployer
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.FabricDeployer;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Management.WindowsFabricValidator;
    using System.Globalization;
    using System.Net;

    internal class ServerInfrastructure : Infrastructure
    {
        #region Public Constructors
        public ServerInfrastructure(ClusterManifestTypeInfrastructure cmInfrastructure, InfrastructureNodeType[] infrastructureNodes, Dictionary<string, ClusterManifestTypeNodeType> nameToNodeTypeMap)
        {
            this.votes = new List<SettingsTypeSectionParameter>();
            this.seedNodeClientConnectionAddresses = new List<SettingsTypeSectionParameter>();
            this.nodes = new List<InfrastructureNodeType>();
#if DotNetCoreClrLinux
            var infrastructure = cmInfrastructure.Item as ClusterManifestTypeInfrastructureLinux;
#else
            var infrastructure = cmInfrastructure.Item as ClusterManifestTypeInfrastructureWindowsServer;
#endif

            foreach (var nodeInfo in infrastructure.NodeList)
            {
                if (!nameToNodeTypeMap.ContainsKey(nodeInfo.NodeTypeRef))
                { 
                    throw new ArgumentException(
                        string.Format(CultureInfo.InvariantCulture, "Node {0} has NodeType {1}, which is not present in node types definition", nodeInfo.NodeName, nodeInfo.NodeTypeRef));
                }

                var nodeType = nameToNodeTypeMap[nodeInfo.NodeTypeRef];
                var node = (new InfrastructureNodeType()
                {
                    Certificates = nodeType.Certificates,
                    Endpoints = nodeType.Endpoints,
                    FaultDomain = nodeInfo.FaultDomain,
                    IPAddressOrFQDN = Helpers.isIPV6AddressAndNoBracket(nodeInfo.IPAddressOrFQDN) ?
                        Helpers.AddBracketsAroundIPV6(nodeInfo.IPAddressOrFQDN) : nodeInfo.IPAddressOrFQDN,
                    IsSeedNode = nodeInfo.IsSeedNode,
                    NodeName = nodeInfo.NodeName,
                    NodeTypeRef = nodeInfo.NodeTypeRef,
                    RoleOrTierName = nodeInfo.NodeTypeRef,
                    UpgradeDomain = nodeInfo.UpgradeDomain
                });
                this.nodes.Add(node);
                if (node.IsSeedNode)
                {
                    string key = node.NodeName;
                    string connectionAddress = string.Format(CultureInfo.InvariantCulture, "{0}:{1}", node.IPAddressOrFQDN, node.Endpoints.ClusterConnectionEndpoint.Port);
                    string value = string.Format(CultureInfo.InvariantCulture, "{0},{1}", "SeedNode", connectionAddress);
                    this.votes.Add(new SettingsTypeSectionParameter() { Name = key, Value = value, MustOverride = false });
                    FabricValidator.TraceSource.WriteNoise(FabricValidatorUtility.TraceTag, "Vote {0}-{1} added", key, value);

                    string clientConnectionAddress = string.Format(CultureInfo.InvariantCulture, "{0}:{1}", node.IPAddressOrFQDN, node.Endpoints.ClientConnectionEndpoint.Port);
                    this.seedNodeClientConnectionAddresses.Add(new SettingsTypeSectionParameter() { Name = key, Value = clientConnectionAddress, MustOverride = false });
                    FabricValidator.TraceSource.WriteNoise(FabricValidatorUtility.TraceTag, "Seed Nodes {0}-{1} added", key, clientConnectionAddress);
                }
            }

            if (infrastructureNodes != null)
            {
                foreach (var infraNode in infrastructureNodes)
                {
                    /* If the current machine node name is present in the infrastructure manifest but not present in the cluster manifest, then
                       it should be added back to this.nodes */
                    if (this.nodes.Find(node => infraNode.NodeName == node.NodeName) == null
                        && NetworkApiHelper.IsNodeForThisMachine(infraNode))
                    {
                        this.nodes.Add(infraNode);
                    }
                }
            }

            this.isScaleMin = infrastructure.IsScaleMin;
        }
        #endregion

        public override bool IsDefaultSystem
        {
            get
            {
                return false;
            }
        }

        public override bool IsScaleMin
        {
            get
            {
                return isScaleMin;
            }
        }

        public override List<SettingsTypeSectionParameter> InfrastructureVotes
        {
            get
            {
                return this.votes;
            }
        }

        public override Dictionary<string, string> VoteMappings
        {
            get
            {
                return null;
            }
        }

        public override List<SettingsTypeSectionParameter> SeedNodeClientConnectionAddresses
        {
            get
            {
                return this.seedNodeClientConnectionAddresses;
            }
        }

        private bool isScaleMin;
        private List<SettingsTypeSectionParameter> votes;
        private readonly List<SettingsTypeSectionParameter> seedNodeClientConnectionAddresses;
    }
}
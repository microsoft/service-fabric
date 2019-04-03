// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.FabricDeployer
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Management.WindowsFabricValidator;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Linq;

    internal class AzureInfrastructure : Infrastructure
    {
        [Flags]
        internal enum DynamicTopologyKind
        {
            None = 0x00,

            // Nodes defined in static topology can optionally rely on infrastructure wrappers
            // to provide values for IPAddressOrFQDN, FD, UD etc.
            NodeTopologyInfoUpdate = 0x01,
        }

        #region Public Constructors
        public AzureInfrastructure(ClusterManifestTypeInfrastructure cmInfrastructure, InfrastructureNodeType[] inputInfrastructureNodes, Dictionary<string, ClusterManifestTypeNodeType> nameToNodeTypeMap, int dynamicTopologyKindIntValue = 0)
        {
            ReleaseAssert.AssertIfNull(cmInfrastructure, "cmInfrastructure");
            ReleaseAssert.AssertIfNull(nameToNodeTypeMap, "nameToNodeTypeMap");
            
            this.votes = new List<SettingsTypeSectionParameter>();
            this.seedNodeClientConnectionAddresses = new List<SettingsTypeSectionParameter>();
            this.voteMappings = new Dictionary<string, string>();
            this.nodes = new List<InfrastructureNodeType>();

            TopologyProvider.TopologyProviderType topologyProviderType = TopologyProvider.GetTopologyProviderTypeFromClusterManifestTypeInfrastructure(cmInfrastructure);

            if (topologyProviderType == TopologyProvider.TopologyProviderType.ServiceRuntimeTopologyProvider)
            {
                this.InitializeFromServiceRuntimeTopologyProvider(cmInfrastructure.Item as ClusterManifestTypeInfrastructureWindowsAzure, inputInfrastructureNodes, nameToNodeTypeMap);
            }
            else if (topologyProviderType == TopologyProvider.TopologyProviderType.StaticTopologyProvider)
            {
                this.InitializeFromStaticTopologyProvider(cmInfrastructure.Item as ClusterManifestTypeInfrastructureWindowsAzureStaticTopology, inputInfrastructureNodes, nameToNodeTypeMap, (DynamicTopologyKind)dynamicTopologyKindIntValue);
            }
        }
        #endregion

        public override bool IsDefaultSystem
        {
            get
            {
                return true;
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

        public override List<SettingsTypeSectionParameter> SeedNodeClientConnectionAddresses
        {
            get
            {
                return this.seedNodeClientConnectionAddresses;
            }
        }

        public override Dictionary<string, string> VoteMappings
        {
            get
            {
                return this.voteMappings;
            }
        }

        private const string AutomaticTopologyInfoValue = "*";
        private bool isScaleMin;
        private List<SettingsTypeSectionParameter> votes;
        private Dictionary<string, string> voteMappings;
        private readonly List<SettingsTypeSectionParameter> seedNodeClientConnectionAddresses;

        private void InitializeFromServiceRuntimeTopologyProvider(ClusterManifestTypeInfrastructureWindowsAzure azureInfrastructure, InfrastructureNodeType[] inputInfrastructureNodes, Dictionary<string, ClusterManifestTypeNodeType> nameToNodeTypeMap)
        {
            // For service runtime topology provider, source of cluster topology is infrastructure manifest provided by azure wrapper service.
            // Seed nodes are determined as the first SeedNodeCount nodes of the seed node role. 
            ReleaseAssert.AssertIfNull(inputInfrastructureNodes, "inputInfrastructureNodes");            

            Dictionary<string, int> seedInfo = new Dictionary<string, int>();
            Dictionary<string, string> roleToTypeMap = new Dictionary<string, string>();

            foreach (var role in azureInfrastructure.Roles)
            {
                try
                {
                    seedInfo.Add(role.RoleName, role.SeedNodeCount);
                    roleToTypeMap.Add(role.RoleName, role.NodeTypeRef);
                }
                catch (ArgumentException)
                {
                    throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "Role name {0} is duplicate", role.RoleName));
                }
            }

            foreach (var nodeInfo in inputInfrastructureNodes)
            {
                string nodeIndex = nodeInfo.NodeName.Split(".".ToCharArray()).Last();
                int index = int.Parse(nodeIndex);
                ClusterManifestTypeNodeType nodeType = null;
                if (roleToTypeMap.ContainsKey(nodeInfo.RoleOrTierName))
                {
                    var nodeTypeName = roleToTypeMap[nodeInfo.RoleOrTierName];
                    if (nameToNodeTypeMap.ContainsKey(nodeTypeName))
                    {
                        nodeType = nameToNodeTypeMap[nodeTypeName];
                    }
                    else
                    {
                        throw new ArgumentException(
                            string.Format(
                            CultureInfo.InvariantCulture, 
                            "Node type {0} specified for role {1} in node {2} not found", 
                            nodeTypeName, 
                            nodeInfo.RoleOrTierName, 
                            nodeInfo.NodeName));
                    }
                }
                else
                {
                    throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "Role name {0} specified in node {1} not found", nodeInfo.NodeName, nodeInfo.RoleOrTierName));
                }

                var node = new InfrastructureNodeType()
                {
                    FaultDomain = nodeInfo.FaultDomain,
                    IPAddressOrFQDN = nodeInfo.IPAddressOrFQDN,
                    IsSeedNode = index < seedInfo[nodeInfo.RoleOrTierName],
                    NodeName = nodeInfo.NodeName,
                    NodeTypeRef = roleToTypeMap[nodeInfo.RoleOrTierName],
                    UpgradeDomain = nodeInfo.UpgradeDomain,
                    RoleOrTierName = nodeInfo.RoleOrTierName,
                    Endpoints = nodeInfo.Endpoints,
                    Certificates = nodeType.Certificates
                };

                this.nodes.Add(node);
                
                if (node.IsSeedNode)
                {
                    // Add vote mappings
                    string connectionAddress = string.Format(CultureInfo.InvariantCulture, "{0}:{1}", node.IPAddressOrFQDN, node.Endpoints.ClusterConnectionEndpoint.Port);
                    this.voteMappings.Add(node.NodeName, connectionAddress);
                    FabricValidator.TraceSource.WriteNoise(FabricValidatorUtility.TraceTag, "Mapping {0}-{1} added", node.NodeName, connectionAddress);

                    // Add votes.
                    string key = node.NodeName;
                    string value = string.Format(CultureInfo.InvariantCulture, "{0},{1}", "WindowsAzure", node.NodeName);
                    this.votes.Add(new SettingsTypeSectionParameter() { Name = key, Value = value, MustOverride = false });
                    FabricValidator.TraceSource.WriteNoise(FabricValidatorUtility.TraceTag, "Vote {0}-{1} added", key, value);

                    string clientConnectionAddress = string.Format(CultureInfo.InvariantCulture, "{0}:{1}", node.IPAddressOrFQDN, node.Endpoints.ClientConnectionEndpoint.Port);
                    this.seedNodeClientConnectionAddresses.Add(new SettingsTypeSectionParameter() { Name = key, Value = clientConnectionAddress, MustOverride = false });
                    FabricValidator.TraceSource.WriteNoise(FabricValidatorUtility.TraceTag, "Seed Nodes {0}-{1} added", key, clientConnectionAddress);

                }
            }

            this.isScaleMin = FabricValidatorUtility.IsAddressLoopback(nodes[0].IPAddressOrFQDN);
        }

        private void InitializeFromStaticTopologyProvider(ClusterManifestTypeInfrastructureWindowsAzureStaticTopology azureStaticTopologyInfrastructure, InfrastructureNodeType[] inputInfrastructureNodes, Dictionary<string, ClusterManifestTypeNodeType> nameToNodeTypeMap, DynamicTopologyKind dynamicTopologyKind)
        {
            FabricValidator.TraceSource.WriteInfo(FabricValidatorUtility.TraceTag, "DynamicTopologyKind = {0}", dynamicTopologyKind);

            Dictionary<string, InfrastructureNodeType> infrastructureManifestNodes = null;
            if (dynamicTopologyKind != DynamicTopologyKind.None)
            {
                infrastructureManifestNodes = new Dictionary<string, InfrastructureNodeType>(StringComparer.Ordinal);
                if (inputInfrastructureNodes != null)
                {
                    foreach (var inputInfrastructureNode in inputInfrastructureNodes)
                    {
                        if (!string.IsNullOrWhiteSpace(inputInfrastructureNode.NodeName))
                        {
                            infrastructureManifestNodes[inputInfrastructureNode.NodeName] = inputInfrastructureNode;
                        }
                    }
                }
            }

            // For static topology provider, effective cluster topology is based on the cluster manifest static topology section + infrastructure manifest.
            // When dynamicTopologyKind == DynamicTopologyKind.None, effective cluster topology is solely based on cluster manifest static topology section while infrastructure manifest is not used.
            // When dynamicTopologyKind <> DynamicTopologyKind.None, the information in infrastructure manifest would be used to supplement cluster manifest in determining effective clsuter topology.
            // Seed nodes are determined by IsSeedNode attribute for each node element. 
            foreach (var nodeInfo in azureStaticTopologyInfrastructure.NodeList)
            {
                var nodeType = nameToNodeTypeMap[nodeInfo.NodeTypeRef];
                var node = (new InfrastructureNodeType()
                {
                    Certificates = nodeType.Certificates,
                    Endpoints = nodeType.Endpoints,
                    FaultDomain = nodeInfo.FaultDomain,
                    IPAddressOrFQDN = nodeInfo.IPAddressOrFQDN,
                    IsSeedNode = nodeInfo.IsSeedNode,
                    NodeName = nodeInfo.NodeName,
                    NodeTypeRef = nodeInfo.NodeTypeRef,
                    RoleOrTierName = nodeInfo.NodeTypeRef,
                    UpgradeDomain = nodeInfo.UpgradeDomain
                });

                // Nodes defined in static topology can optionally rely on infrastructure wrappers to provide values for IPAddressOrFQDN, FD, UD etc.
                // Specifically:
                // 1) When a concrete value is defined for IPAddressOrFQDN/FD/UD of a node, the concrete value would always be used.
                // 2) When "*" is defined for IPAddressOrFQDN/FD/UD of a node, the value provided by infrastructure wrappers would be used.
                if ((dynamicTopologyKind & DynamicTopologyKind.NodeTopologyInfoUpdate) == DynamicTopologyKind.NodeTopologyInfoUpdate)
                {
                    if (infrastructureManifestNodes.ContainsKey(node.NodeName))
                    {
                        bool nodeTopologyInfoUpdated = false;
                        var infrastructureManifestNode = infrastructureManifestNodes[node.NodeName];

                        if (string.Compare(node.IPAddressOrFQDN, AutomaticTopologyInfoValue, StringComparison.OrdinalIgnoreCase) == 0)
                        {
                            node.IPAddressOrFQDN = infrastructureManifestNode.IPAddressOrFQDN;
                            nodeTopologyInfoUpdated = true;
                        }

                        if (string.Compare(node.UpgradeDomain, AutomaticTopologyInfoValue, StringComparison.OrdinalIgnoreCase) == 0)
                        {
                            node.UpgradeDomain = infrastructureManifestNode.UpgradeDomain;
                            nodeTopologyInfoUpdated = true;
                        }

                        if (string.Compare(node.FaultDomain, AutomaticTopologyInfoValue, StringComparison.OrdinalIgnoreCase) == 0)
                        {
                            node.FaultDomain = infrastructureManifestNode.FaultDomain;
                            nodeTopologyInfoUpdated = true;
                        }

                        infrastructureManifestNodes.Remove(node.NodeName);

                        if (nodeTopologyInfoUpdated)
                        {
                            FabricValidator.TraceSource.WriteNoise(FabricValidatorUtility.TraceTag, "Topology info for node {0} in cluster manifest is updated based infrastructure manifest", node.NodeName);
                        }
                    }
                }

                this.nodes.Add(node);

                if (node.IsSeedNode)
                {
                    // Add vote mappings
                    string connectionAddress = string.Format(CultureInfo.InvariantCulture, "{0}:{1}", node.IPAddressOrFQDN, node.Endpoints.ClusterConnectionEndpoint.Port);
                    this.voteMappings.Add(node.NodeName, connectionAddress);
                    FabricValidator.TraceSource.WriteNoise(FabricValidatorUtility.TraceTag, "Mapping {0}-{1} added", node.NodeName, connectionAddress);

                    // Add votes.
                    string key = node.NodeName;
                    string value = string.Format(CultureInfo.InvariantCulture, "{0},{1}", "WindowsAzure", node.NodeName);
                    this.votes.Add(new SettingsTypeSectionParameter() { Name = key, Value = value, MustOverride = false });
                    FabricValidator.TraceSource.WriteNoise(FabricValidatorUtility.TraceTag, "Vote {0}-{1} added", key, value);
                }
            }

            this.isScaleMin = FabricValidatorUtility.IsAddressLoopback(nodes[0].IPAddressOrFQDN);
        }
    }
}
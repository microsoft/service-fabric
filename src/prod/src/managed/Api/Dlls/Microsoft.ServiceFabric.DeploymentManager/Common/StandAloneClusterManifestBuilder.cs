// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Common
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.Linq;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Model;

    internal class StandAloneClusterManifestBuilder : ClusterManifestBuilder
    {
        public StandAloneClusterManifestBuilder(
            IClusterTopology topology,
            SeedNodeSelector seedNodeSelector,
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            ClusterNodeConfig targetNodeConfig,
            IManifestVersionGenerator versionGenerator,
            IFabricSettingsActivator fabricSettingsActivator,
            ClusterManifestGeneratorSettings clusterManifestGeneratorSettings,
            ITraceLogger traceLogger)
        : base(topology, seedNodeSelector, targetCsmConfig, targetWrpConfig, targetNodeConfig, versionGenerator, fabricSettingsActivator, clusterManifestGeneratorSettings, traceLogger)
        {
        }

        public override ClusterManifestType[] UpdateSeedNodesInClusterManifest(
            ClusterManifestType existingClusterManifest,
            FabricSettingsMetadata currentFabricSettingsMetadata,
            List<string> disabledNodes,
            List<string> removedNodes)
        {
            var primaryNodeTypes = this.TargetCsmConfig.NodeTypes.Where(nodeType => nodeType.IsPrimary);
            var existingSeedNodes = ((ClusterManifestTypeInfrastructureWindowsServer)existingClusterManifest.Infrastructure.Item).NodeList.Where(item => item.IsSeedNode == true).ToList();

            List<NodeDescription> seedNodesAdded;
            List<NodeDescription> seedNodesRemoved;
            if (!this.TryUpdateSeedNodes(
                    primaryNodeTypes,
                    existingSeedNodes,
                    disabledNodes,
                    removedNodes,
                    out seedNodesAdded,
                    out seedNodesRemoved))
            {
                // returns null if enough nodes are not available
                return null;
            }

            var nodeTypes = this.GetNodeTypes(this.TargetCsmConfig.NodeTypes, this.TargetCsmConfig.Security);

            List<ClusterManifestType> clusterManifests = new List<ClusterManifestType>();
            if (seedNodesAdded.Count() > 0)
            {
                List<NodeDescription> nonSeedNodesAdded;

                // We allow the scenario to add seednode and non-seednode at the same time. If without reliability level change, it will go through SimpleClusterUpgrade
                // which will handle the addition of both. If there is reliability level change, we need to take the addition of non seed node here. To add them only in the 
                // generated clusterManifest[0].
                // Find the added non seed node. If the node in topology is neither in existing infrastructure not in seedNodesAdded, it's a non seed node to add.
                nonSeedNodesAdded = this.Topology.Nodes.Where(node => !((ClusterManifestTypeInfrastructureWindowsServer)existingClusterManifest.Infrastructure.Item).NodeList.Select(n => n.NodeName).Contains(node.Key)
                && !seedNodesAdded.Select(s => s.NodeName).Contains(node.Key)).Select(kvp => kvp.Value).ToList();
                foreach (var seedNodeToAdd in seedNodesAdded)
                {
                    existingSeedNodes.Add(seedNodeToAdd.ToFabricNodeType());
                    var clusterManifest = this.UpdateClusterManifest(
                                              this.TargetCsmConfig.Security,
                                              clusterManifests.Count == 0 ? existingClusterManifest : clusterManifests[clusterManifests.Count - 1],
                                              existingSeedNodes,
                                              seedNodeToAdd,
                                              nonSeedNodesAdded,
                                              true,
                                              nodeTypes,
                                              currentFabricSettingsMetadata,
                                              this.VersionGenerator.GetNextClusterManifestVersion());
                    nonSeedNodesAdded.Clear();
                    clusterManifests.Add(clusterManifest);
                }
            }

            if (seedNodesRemoved.Count() > 0)
            {
                List<NodeDescription> nonSeedNodesRemoved;

                // Find the removed non seed node. If the node in existing infrastructure is neither in topology nor in seedNodesRemoved, it's a non seed node to remove.
                var nonSeedNodesRemovedNodeType = ((ClusterManifestTypeInfrastructureWindowsServer)existingClusterManifest.Infrastructure.Item).NodeList.Where(node => !this.Topology.Nodes.Select(n => n.Key).Contains(node.NodeName)
                && !seedNodesRemoved.Select(s => s.NodeName).Contains(node.NodeName)).ToList();
                nonSeedNodesRemoved = new List<NodeDescription>();

                if (nonSeedNodesRemovedNodeType.Count() > 0)
                {
                    foreach (var n in nonSeedNodesRemovedNodeType)
                    {
                        var node = new NodeDescription()
                        {
                            FaultDomain = n.FaultDomain,
                            IPAddress = n.IPAddressOrFQDN,
                            NodeName = n.NodeName,
                            NodeTypeRef = n.NodeTypeRef,
                            UpgradeDomain = n.UpgradeDomain
                        };

                        nonSeedNodesRemoved.Add(node);
                    }
                }

                foreach (var seedNodeToRemove in seedNodesRemoved)
                {
                    // Remove the seedNode from existingSeedNodes
                    existingSeedNodes = new List<FabricNodeType>(
                        existingSeedNodes.Where(existingSeedNode => existingSeedNode.NodeName != seedNodeToRemove.NodeName));

                    var clusterManifest = this.UpdateClusterManifest(
                                              this.TargetCsmConfig.Security,
                                              clusterManifests.Count == 0 ? existingClusterManifest : clusterManifests[clusterManifests.Count - 1],
                                              existingSeedNodes,
                                              seedNodeToRemove,
                                              nonSeedNodesRemoved,
                                              false,
                                              nodeTypes,
                                              currentFabricSettingsMetadata,
                                              this.VersionGenerator.GetNextClusterManifestVersion());

                    nonSeedNodesRemoved.Clear();
                    clusterManifests.Add(clusterManifest);
                }
            }

            // returns empty array if no upgrade is required
            return clusterManifests.ToArray();
        }

        protected ClusterManifestType UpdateClusterManifest(
            Security security,
            ClusterManifestType existingClusterManifest,
            List<FabricNodeType> seedNodes,
            NodeDescription seedNodeChanged,
            List<NodeDescription> nonSeedNodeChanged,
            bool isSeedNodeToAdd,
            IEnumerable<ClusterManifestTypeNodeType> nodeTypes,
            FabricSettingsMetadata currentFabricSettingsMetadata,
            string clusterManifestVersion)
        {
            FabricSettingsGeneratorBase fabricSettingsGenerator = this.CreateFabricSettingsGenerator(
                        this.TargetCsmConfig,
                        this.TargetWrpConfig,
                        this.ClusterManifestGeneratorSettings,
                        currentFabricSettingsMetadata,
                        existingClusterManifest);

            var originalNodeList = ((ClusterManifestTypeInfrastructureWindowsServer)existingClusterManifest.Infrastructure.Item).NodeList;
            List<FabricNodeType> nodelist = new List<FabricNodeType>();
            foreach (var n in originalNodeList)
            {       
                nodelist.Add(new FabricNodeType
                {
                    FaultDomain = n.FaultDomain,
                    IPAddressOrFQDN = n.IPAddressOrFQDN,
                    IsSeedNode = n.IsSeedNode,
                    NodeName = n.NodeName,
                    NodeTypeRef = n.NodeTypeRef,
                    UpgradeDomain = n.UpgradeDomain
                });
            }

            var fabricSettings = fabricSettingsGenerator.GenerateFabricSettings(security, nodelist.Count());

            if (isSeedNodeToAdd)
            {
                var hasNodeListAlreadyContainedThisSeedNode = originalNodeList.Select(node => node.NodeName).Contains(seedNodeChanged.NodeName);
                if (!hasNodeListAlreadyContainedThisSeedNode)
                {
                    nodelist.Add(seedNodeChanged.ToFabricNodeType());
                }

                if (nonSeedNodeChanged.Count > 0)
                {
                    foreach (var nonSeedNodeAdded in nonSeedNodeChanged)
                    {
                        nodelist.Add(nonSeedNodeAdded.ToFabricNodeType());
                    }
                }
            }
            else
            {
                nodelist.Where(n => n.NodeName.Equals(seedNodeChanged.NodeName)).FirstOrDefault().IsSeedNode = false;
                if (nonSeedNodeChanged.Count > 0)
                {
                    foreach (var nonSeedNodeRemoved in nonSeedNodeChanged)
                    {
                        nodelist = nodelist.Where(n => !nonSeedNodeRemoved.NodeName.Equals(n.NodeName)).ToList();
                    }
                }
            }

            // Set ExpectedNodeDeactivationDuration section in FailoverManager to 2 days. Deactivating node in seed node removal
            // could take a lot of intermediate seed node changing upgrades. This is to prevent deactivate node timeout.
            // TODO Chrissie: When the seed node removal causing the intermediate seed node shift bug is fixed, remove this parameter from add node scenario.
            var failoverManagerSettings = fabricSettings.FirstOrDefault(setting => setting.Name.Equals(StringConstants.SectionName.FailoverManager, StringComparison.OrdinalIgnoreCase));
            TimeSpan totalSeconds = TimeSpan.FromSeconds(120 * 60 * 24);
            double defaultNodeDeactivationDuration = totalSeconds.TotalSeconds;

            var expectedNodeDeactivationDurationParameter = failoverManagerSettings.Parameter.FirstOrDefault(
                param => param.Name.Equals(StringConstants.ParameterName.ExpectedNodeDeactivationDuration, StringComparison.OrdinalIgnoreCase));

            if (expectedNodeDeactivationDurationParameter != null)
            {
                expectedNodeDeactivationDurationParameter.Value = defaultNodeDeactivationDuration.ToString();
            }
            else
            {
                int originalParamCount = failoverManagerSettings.Parameter.Count();
                SettingsOverridesTypeSectionParameter[] newFailoverManagerSection = new SettingsOverridesTypeSectionParameter[originalParamCount + 1];
                Array.Copy(failoverManagerSettings.Parameter, newFailoverManagerSection, originalParamCount);
                newFailoverManagerSection[originalParamCount] = new SettingsOverridesTypeSectionParameter()
                {
                    Name = StringConstants.ParameterName.ExpectedNodeDeactivationDuration,
                    Value = defaultNodeDeactivationDuration.ToString()
                };

                failoverManagerSettings.Parameter = newFailoverManagerSection;
            }

            var seedNodeNames = seedNodes.Select(item => item.NodeName);
            foreach (var node in nodelist)
            {
                node.IsSeedNode = seedNodeNames.Contains(node.NodeName);
            }

            var updatedCluterManifest = new ClusterManifestType
            {
                FabricSettings = fabricSettings,
                Infrastructure = new ClusterManifestTypeInfrastructure()
                {
                    Item = new ClusterManifestTypeInfrastructureWindowsServer()
                    {
                        NodeList = nodelist.ToArray()
                    }
                },
                NodeTypes = nodeTypes.ToArray(),
                Name = StringConstants.ClusterManifestName,
                Version = clusterManifestVersion,
                Description = GeneratedCluterManifestDescription
            };

            this.SortClusterManifest(updatedCluterManifest);
            return updatedCluterManifest;
        }

        protected bool TryUpdateSeedNodes(
            IEnumerable<NodeTypeDescription> primaryNodeTypes,
            List<FabricNodeType> existingSeedNodes,
            IEnumerable<string> disabledNodes,
            IEnumerable<string> removedNodes,
            out List<NodeDescription> seedNodesToAdd,
            out List<NodeDescription> seedNodesToRemove)
        {
            // From inputTopology get the list of all enabled nodes
            IEnumerable<NodeDescription> enabledNodes = this.Topology.Nodes.Values
                    .Where(node => primaryNodeTypes.Any(primaryNodeType => primaryNodeType.Name == node.NodeTypeRef)
                           && (disabledNodes == null || !disabledNodes.Contains(node.NodeName))
                           && (removedNodes == null || !removedNodes.Contains(node.NodeName)));

            // From existing seed nodes, get list of all seed nodes which are not disabled
            List<NodeDescription> existingEnabledSeedNodes = new List<NodeDescription>();
            List<NodeDescription> existingSeedNodesRemoved = new List<NodeDescription>();
            foreach (var existingSeedNode in existingSeedNodes)
            {
                if (disabledNodes == null || !disabledNodes.Contains(existingSeedNode.NodeName))
                {
                    existingEnabledSeedNodes.Add(this.Topology.Nodes[existingSeedNode.NodeName]);
                }
                else
                {
                    existingSeedNodesRemoved.Add(this.Topology.Nodes[existingSeedNode.NodeName]);
                }
            }

            if (!this.SeedNodeSelector.TryUpdate(
                    this.TargetCsmConfig.ReliabilityLevel,
                    existingEnabledSeedNodes,
                    enabledNodes,
                    this.ClusterManifestGeneratorSettings.FDCount,
                    this.ClusterManifestGeneratorSettings.UDCount,
                    out seedNodesToAdd,
                    out seedNodesToRemove))
            {
                return false;
            }

            if (existingSeedNodesRemoved.Any())
            {
                // Add existing seed nodes which were removed because they are disabled/disabling
                seedNodesToRemove.AddRange(existingSeedNodesRemoved);
            }

            return true;
        }

        protected override ClusterManifestTypeInfrastructure OnGetInfrastructure(IClusterTopology topology, IEnumerable<NodeDescription> seedNodes, IEnumerable<ClusterManifestTypeNodeType> nodeTypes)
        {
            var serverInfra = new ClusterManifestTypeInfrastructureWindowsServer { NodeList = new FabricNodeType[topology.Nodes.Count] };
            int index = 0;
            var distinctIpAddreses = new HashSet<string>();
            foreach (var node in topology.Nodes.Values)
            {
                serverInfra.NodeList[index] = new FabricNodeType()
                {
                    NodeName = node.NodeName,
                    FaultDomain = node.FaultDomain,
                    UpgradeDomain = node.UpgradeDomain,
                    IPAddressOrFQDN = node.IPAddress,
                    IsSeedNode = seedNodes.Any(seedNode => string.Equals(seedNode.NodeName, node.NodeName, StringComparison.OrdinalIgnoreCase)),
                    NodeTypeRef = node.NodeTypeRef
                };
                distinctIpAddreses.Add(node.IPAddress);
                index++;
            }

            if (distinctIpAddreses.Count < topology.Nodes.Count)
            {
                serverInfra.IsScaleMin = true;
            }

            return new ClusterManifestTypeInfrastructure()
            {
                Item = serverInfra
            };
        }

        protected override List<NodeDescription> OnGetExistingSeedNodes(ClusterManifestType existingClusterManifest)
        {
            var seedNodes = ((ClusterManifestTypeInfrastructureWindowsServer)existingClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode);
            var nodeDesc = new List<NodeDescription>();
            foreach (var seedNode in seedNodes)
            {
                nodeDesc.Add(this.ToNodeDescription(seedNode));
            }

            return nodeDesc;
        }

        protected override ClusterManifestTypeInfrastructure OnSortInfrastructure(ClusterManifestTypeInfrastructure infrastructure)
        {
            var infra = (ClusterManifestTypeInfrastructureWindowsServer)infrastructure.Item;
            infra.NodeList = infra.NodeList.OrderBy(node => node.NodeName).ToArray();
            return infrastructure;
        }

        protected override void UpdateEndpointPortsForScaleMin(ClusterManifestTypeInfrastructure infrastructure, ClusterManifestTypeNodeType[] nodeTypes, List<NodeTypeDescription> targetNodeTypes)
        {
            var infra = (ClusterManifestTypeInfrastructureWindowsServer)infrastructure.Item;

            if (!infra.IsScaleMin)
            {
                return;
            }

            if (targetNodeTypes[0].ClusterConnectionEndpointPort != 0)
            {
                return;
            }

            var clusterConnectionEndpointPort = int.Parse(nodeTypes[0].Endpoints.ClusterConnectionEndpoint.Port);
            var serviceConnectionEndpointPort = int.Parse(nodeTypes[0].Endpoints.ServiceConnectionEndpoint.Port);
            var leaseDriverEndpointPort = int.Parse(nodeTypes[0].Endpoints.ServiceConnectionEndpoint.Port);

            for (int i = 1; i < nodeTypes.Length; i++)
            {
                clusterConnectionEndpointPort = clusterConnectionEndpointPort + 2;
                serviceConnectionEndpointPort = serviceConnectionEndpointPort + 2;
                leaseDriverEndpointPort = leaseDriverEndpointPort + 2;

                nodeTypes[i].Endpoints.ClusterConnectionEndpoint.Port = clusterConnectionEndpointPort.ToString(CultureInfo.InvariantCulture);
                nodeTypes[i].Endpoints.LeaseDriverEndpoint.Port = leaseDriverEndpointPort.ToString(CultureInfo.InvariantCulture);
                nodeTypes[i].Endpoints.ServiceConnectionEndpoint.Port = serviceConnectionEndpointPort.ToString(CultureInfo.InvariantCulture);
            }
        }

        private NodeDescription ToNodeDescription(FabricNodeType node)
        {
            return new NodeDescription()
            {
                NodeName = node.NodeName,
                NodeTypeRef = node.NodeTypeRef,
                IPAddress = node.IPAddressOrFQDN,
                FaultDomain = node.FaultDomain,
                UpgradeDomain = node.UpgradeDomain
            };
        }
    }
}
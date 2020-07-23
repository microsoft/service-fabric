// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.Linq;

    internal abstract class ClusterManifestBuilder
    {
        protected const string GeneratedCluterManifestDescription = "This is a generated file. Do not modify.";

        protected ClusterManifestBuilder(
            IClusterTopology topology,
            SeedNodeSelector seedNodeSelector,
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            ClusterNodeConfig targetNodeConfig,
            IManifestVersionGenerator versionGenerator,
            IFabricSettingsActivator fabricSettingsActivator,
            ClusterManifestGeneratorSettings clusterManifestGeneratorSettings,
            ITraceLogger traceLogger)
        {
            this.Topology = topology;
            this.SeedNodeSelector = seedNodeSelector;
            this.TargetCsmConfig = targetCsmConfig;
            this.TargetWrpConfig = targetWrpConfig;
            this.TargetNodeConfig = targetNodeConfig;
            this.VersionGenerator = versionGenerator;
            this.FabricSettingsActivator = fabricSettingsActivator;
            this.ClusterManifestGeneratorSettings = clusterManifestGeneratorSettings;
            this.TraceLogger = traceLogger;
        }

        protected IClusterTopology Topology { get; set; }

        protected SeedNodeSelector SeedNodeSelector { get; set; }

        protected IUserConfig TargetCsmConfig { get; set; }

        protected IAdminConfig TargetWrpConfig { get; set; }

        protected ClusterNodeConfig TargetNodeConfig { get; set; }

        protected ClusterManifestGeneratorSettings ClusterManifestGeneratorSettings { get; set; }

        protected IFabricSettingsActivator FabricSettingsActivator { get; set; }

        protected IManifestVersionGenerator VersionGenerator { get; set; }

        protected ITraceLogger TraceLogger { get; set; }

        public ClusterManifestType GenerateClusterManifest()
        {
            var primaryNodesTypes = this.TargetCsmConfig.NodeTypes.Where(nodeType => nodeType.IsPrimary);
            var seedNodes = this.GetSeedNodes(
                                this.TargetCsmConfig.ReliabilityLevel,
                                this.TargetCsmConfig.TotalPrimaryNodeTypeNodeCount,
                                primaryNodesTypes,
                                this.TargetCsmConfig.IsVMSS);
            if (seedNodes == null)
            {
                return null;
            }

            FabricSettingsGeneratorBase fabricSettingsGenerator = this.CreateFabricSettingsGenerator(
                        this.TargetCsmConfig,
                        this.TargetWrpConfig,
                        this.ClusterManifestGeneratorSettings,
                        null /*currentFabricSettingsMetadata*/,
                        null /*existingClusterManifest*/);
            var fabricSettings = fabricSettingsGenerator.GenerateFabricSettings(this.TargetCsmConfig.Security);
            var nodeTypes = this.GetNodeTypes(this.TargetCsmConfig.NodeTypes, this.TargetCsmConfig.Security).ToArray();
            var infrastructure = this.OnGetInfrastructure(this.Topology, seedNodes, nodeTypes);
            this.UpdateEndpointPortsForScaleMin(infrastructure, nodeTypes, this.TargetCsmConfig.NodeTypes);
            var clusterManifest = new ClusterManifestType
            {
                FabricSettings = fabricSettings,
                Infrastructure = infrastructure,
                NodeTypes = nodeTypes,
                Name = this.TargetCsmConfig.ClusterName,
                Version = this.VersionGenerator.GetNextClusterManifestVersion(),
                Description = GeneratedCluterManifestDescription
            };

            this.SortClusterManifest(clusterManifest);

            return clusterManifest;
        }

        public ClusterManifestType[] UpdateCertificateInClusterManifest(
            ClusterManifestType existingClusterManifest,
            Security currentSecurity,
            FabricSettingsMetadata currentFabricSettingsMetadata)
        {
            List<NodeDescription> existingSeedNodes = this.OnGetExistingSeedNodes(existingClusterManifest);

            List<CertificateClusterUpgradeStep> upgradeSteps = CertificateClusterUpgradeFlow.GetUpgradeFlow(
                currentSecurity.CertificateInformation,
                this.TargetCsmConfig.Security.CertificateInformation);

            ClusterManifestType[] result = new ClusterManifestType[upgradeSteps.Count];
            for (int i = 0; i < upgradeSteps.Count; i++)
            {
                result[i] = this.UpdateClusterManifest(
                    i == 0 ? existingClusterManifest : result[i - 1],
                    currentFabricSettingsMetadata,
                    this.VersionGenerator.GetNextClusterManifestVersion(),
                    upgradeSteps[i],
                    existingSeedNodes);
            }

            return result;
        }

        public ClusterManifestType UpdateClusterManifest(ClusterManifestType existingClusterManifest, FabricSettingsMetadata currentFabricSettingsMetadata)
        {
            List<NodeDescription> existingSeedNodes = this.OnGetExistingSeedNodes(existingClusterManifest);

            Security security = this.TargetCsmConfig.Security;
            CertificateDescription thumbprintClusterCert;
            List<string> thumbprintClusterCertList;
            ServerCertificateCommonNames commonNameClusterCert;
            Dictionary<string, string> commonNameClusterCertList;

            this.GetClusterCertConfig(
                security,
                out thumbprintClusterCert,
                out thumbprintClusterCertList,
                out commonNameClusterCert,
                out commonNameClusterCertList);

            return this.UpdateClusterManifest(
                security,
                existingClusterManifest,
                this.GetNodeTypes(this.TargetCsmConfig.NodeTypes, security),
                currentFabricSettingsMetadata,
                this.VersionGenerator.GetNextClusterManifestVersion(),
                thumbprintClusterCert,
                thumbprintClusterCertList,
                commonNameClusterCert,
                commonNameClusterCertList,
                existingSeedNodes);
        }

        // This method returns null if enough nodes are not available
        // This method returns empty array if no upgrade is required
        public virtual ClusterManifestType[] UpdateSeedNodesInClusterManifest(
            ClusterManifestType existingClusterManifest,
            FabricSettingsMetadata currentFabricSettingsMetadata,
            List<string> disabledNodes,
            List<string> removedNodes)
        {
            var primaryNodeTypes = this.TargetCsmConfig.NodeTypes.Where(nodeType => nodeType.IsPrimary);
            var existingSeedNodes = ((ClusterManifestTypeInfrastructurePaaS)existingClusterManifest.Infrastructure.Item).Votes.ToList();

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
            var roles = this.TargetCsmConfig.NodeTypes.Select(nodeType => nodeType.ToPaaSRoleType());

            List<ClusterManifestType> clusterManifests = new List<ClusterManifestType>();
            if (seedNodesAdded != null)
            {
                foreach (var seedNodeToAdd in seedNodesAdded)
                {
                    existingSeedNodes.Add(this.ToPaaSVoteType(seedNodeToAdd, nodeTypes));
                    var clusterManifest = this.UpdateClusterManifest(
                                              this.TargetCsmConfig.Security,
                                              existingClusterManifest,
                                              existingSeedNodes,
                                              nodeTypes,
                                              roles,
                                              currentFabricSettingsMetadata,
                                              this.VersionGenerator.GetNextClusterManifestVersion());

                    clusterManifests.Add(clusterManifest);
                }
            }

            if (seedNodesRemoved != null)
            {
                foreach (var seedNodeToRemove in seedNodesRemoved)
                {
                    // Remove the seedNode from existingSeedNodes
                    existingSeedNodes = new List<PaaSVoteType>(
                        existingSeedNodes.Where(existingSeedNode => existingSeedNode.NodeName != seedNodeToRemove.NodeName));

                    var clusterManifest = this.UpdateClusterManifest(
                                              this.TargetCsmConfig.Security,
                                              existingClusterManifest,
                                              existingSeedNodes,
                                              nodeTypes,
                                              roles,
                                              currentFabricSettingsMetadata,
                                              this.VersionGenerator.GetNextClusterManifestVersion());

                    clusterManifests.Add(clusterManifest);
                }
            }

            // returns empty array if no upgrade is required
            return clusterManifests.ToArray();
        }

        protected virtual ClusterManifestTypeInfrastructure OnGetInfrastructure(IClusterTopology inputTopology, IEnumerable<NodeDescription> seedNodes, IEnumerable<ClusterManifestTypeNodeType> nodeTypes)
        {
            IEnumerable<PaaSRoleType> roles = this.TargetCsmConfig.NodeTypes.Select(nodeType => nodeType.ToPaaSRoleType());
            IEnumerable<PaaSVoteType> votes = seedNodes.Select(seedNode => this.ToPaaSVoteType(seedNode, nodeTypes));

            var infra = new ClusterManifestTypeInfrastructurePaaS()
            {
                Roles = roles.ToArray(),
                Votes = votes.ToArray()
            };

            return new ClusterManifestTypeInfrastructure()
            {
                Item = infra
            };
        }

        protected virtual List<NodeDescription> OnGetExistingSeedNodes(ClusterManifestType existingClusterManifest)
        { 
            var votes = ((ClusterManifestTypeInfrastructurePaaS)existingClusterManifest.Infrastructure.Item).Votes.ToList();
            var nodeDesc = new List<NodeDescription>();
            foreach (var vote in votes)
            {
                nodeDesc.Add(this.ToNodeDescription(vote));
            }

            return nodeDesc;
        }

        protected abstract ClusterManifestTypeInfrastructure OnSortInfrastructure(ClusterManifestTypeInfrastructure infrastructure);

        protected virtual void UpdateEndpointPortsForScaleMin(ClusterManifestTypeInfrastructure infrastructure, ClusterManifestTypeNodeType[] nodeTypes, List<NodeTypeDescription> targetNodeType)
        {
        }

        protected bool TryUpdateSeedNodes(
            IEnumerable<NodeTypeDescription> primaryNodeTypes,
            List<PaaSVoteType> existingSeedNodes,
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

        protected ClusterManifestType UpdateClusterManifest(
            Security security,
            ClusterManifestType existingClusterManifest,
            List<PaaSVoteType> seedNodes,
            IEnumerable<ClusterManifestTypeNodeType> nodeTypes,
            IEnumerable<PaaSRoleType> roles,
            FabricSettingsMetadata currentFabricSettingsMetadata,
            string clusterManifestVersion)
        {
            FabricSettingsGeneratorBase fabricSettingsGenerator = this.CreateFabricSettingsGenerator(
                        this.TargetCsmConfig,
                        this.TargetWrpConfig,
                        this.ClusterManifestGeneratorSettings,
                        currentFabricSettingsMetadata,
                        existingClusterManifest);
            var fabricSettings = fabricSettingsGenerator.GenerateFabricSettings(security);

            var updatedCluterManifest = new ClusterManifestType
            {
                FabricSettings = fabricSettings,
                Infrastructure = new ClusterManifestTypeInfrastructure()
                {
                    Item = new ClusterManifestTypeInfrastructurePaaS()
                    {
                        Roles = roles.ToArray(),
                        Votes = seedNodes.ToArray()
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

        protected IEnumerable<ClusterManifestTypeNodeType> GetNodeTypes(
            IEnumerable<NodeTypeDescription> nodeTypeDescriptions,
            Security security,
            ClusterManifestType existingClusterManifestType = null)
        {
            //// TODO: Implement smarter port selection to avoid conflicts with ports specified by the user

            var result = new List<ClusterManifestTypeNodeType>();
            foreach (var nodeTypeDescription in nodeTypeDescriptions)
            {
                ClusterManifestTypeNodeType matchigExistingNodeType = null;
                if (existingClusterManifestType != null)
                {
                    matchigExistingNodeType =
                        existingClusterManifestType.NodeTypes.FirstOrDefault(
                            existingNodeType => existingNodeType.Name.Equals(nodeTypeDescription.Name));
                }

                var clusterConnectionEndpointPort = matchigExistingNodeType == null
                                                    ? this.ClusterManifestGeneratorSettings.ClusterConnectionEndpointPort.ToString()
                                                    : matchigExistingNodeType.Endpoints.ClusterConnectionEndpoint.Port;
                var leaseDriverEndpointPort = matchigExistingNodeType == null
                                              ? this.ClusterManifestGeneratorSettings.LeaseDriverEndpointPort.ToString()
                                              : matchigExistingNodeType.Endpoints.LeaseDriverEndpoint.Port;
                var serviceConnectionEndpointPort = matchigExistingNodeType == null
                                                    ? this.ClusterManifestGeneratorSettings.ServiceConnectionEndpointPort.ToString()
                                                    : matchigExistingNodeType.Endpoints.ServiceConnectionEndpoint.Port;

                var nodeType = nodeTypeDescription.ToClusterManifestTypeNodeType();

                var defaultPlacementProperty = new KeyValuePairType()
                {
                    Name = StringConstants.DefaultPlacementConstraintsKey,
                    Value = nodeType.Name
                };

                if (nodeType.PlacementProperties == null)
                {
                    nodeType.PlacementProperties = new[] { defaultPlacementProperty };
                }
                else
                {
                    var existingPlacementProperties = nodeType.PlacementProperties.ToList();
                    existingPlacementProperties.Add(defaultPlacementProperty);

                    nodeType.PlacementProperties = existingPlacementProperties.ToArray();
                }

                nodeType.Endpoints.ClusterConnectionEndpoint = new InternalEndpointType()
                {
                    Port = nodeTypeDescription.ClusterConnectionEndpointPort == 0 ? clusterConnectionEndpointPort : nodeTypeDescription.ClusterConnectionEndpointPort.ToString(),
                    Protocol = InternalEndpointTypeProtocol.tcp
                };

                nodeType.Endpoints.LeaseDriverEndpoint = new InternalEndpointType()
                {
                    Port = nodeTypeDescription.LeaseDriverEndpointPort == 0 ? leaseDriverEndpointPort : nodeTypeDescription.LeaseDriverEndpointPort.ToString(),
                    Protocol = InternalEndpointTypeProtocol.tcp
                };

                nodeType.Endpoints.ServiceConnectionEndpoint = new InternalEndpointType()
                {
                    Port = nodeTypeDescription.ServiceConnectionEndpointPort == 0 ? serviceConnectionEndpointPort : nodeTypeDescription.ServiceConnectionEndpointPort.ToString(),
                    Protocol = InternalEndpointTypeProtocol.tcp
                };

                nodeType.Endpoints.ApplicationEndpoints = new FabricEndpointsTypeApplicationEndpoints()
                {
                    StartPort = nodeTypeDescription.ApplicationPorts.StartPort,
                    EndPort = nodeTypeDescription.ApplicationPorts.EndPort
                };

                if (nodeTypeDescription.EphemeralPorts != null)
                {
                    nodeType.Endpoints.EphemeralEndpoints = new FabricEndpointsTypeEphemeralEndpoints()
                    {
                        StartPort = nodeTypeDescription.EphemeralPorts.StartPort,
                        EndPort = nodeTypeDescription.EphemeralPorts.EndPort
                    };
                }

                if (nodeTypeDescription.KtlLogger != null)
                {
                    nodeType.KtlLoggerSettings = nodeTypeDescription.KtlLogger.ToFabricKtlLoggerSettingsType();
                }

                if (nodeTypeDescription.LogicalDirectories != null && nodeTypeDescription.LogicalDirectories.Length > 0)
                {
                    LogicalDirectoryType[] logicalDirectories = new LogicalDirectoryType[nodeTypeDescription.LogicalDirectories.Count()];
                    var i = 0;
                    foreach (var dir in nodeTypeDescription.LogicalDirectories)
                    {
                        logicalDirectories[i++] = dir.ToLogicalDirectoryType();
                    }

                    nodeType.LogicalDirectories = logicalDirectories;
                }

                if (security != null && security.CertificateInformation != null
                    && (security.CertificateInformation.ClusterCertificate != null || security.CertificateInformation.ClusterCertificateCommonNames != null
                    || security.CertificateInformation.ServerCertificate != null || security.CertificateInformation.ServerCertificateCommonNames != null))
                {
                    nodeType.Endpoints.HttpGatewayEndpoint.Protocol = InputEndpointTypeProtocol.https;

                    FabricCertificateType clusterCertificate = null;
                    FabricCertificateType serverCertificate = null;

                    if (security.CertificateInformation.ClusterCertificate != null)
                    {
                        clusterCertificate = security.CertificateInformation.ClusterCertificate.ToFabricCertificateType();
                    }
                    else if (security.CertificateInformation.ClusterCertificateCommonNames != null)
                    {
                        clusterCertificate = security.CertificateInformation.ClusterCertificateCommonNames.ToFabricCertificateType();
                    }

                    if (security.CertificateInformation.ServerCertificate != null)
                    {
                        serverCertificate = security.CertificateInformation.ServerCertificate.ToFabricCertificateType();
                    }
                    else if (security.CertificateInformation.ServerCertificateCommonNames != null)
                    {
                        serverCertificate = security.CertificateInformation.ServerCertificateCommonNames.ToFabricCertificateType();
                    }

                    nodeType.Certificates = new CertificatesType()
                    {
                        ClientCertificate = clusterCertificate != null ? clusterCertificate : serverCertificate,
                        ClusterCertificate = clusterCertificate,
                        ServerCertificate = serverCertificate
                    };
                }

                if (nodeType.Endpoints.HttpApplicationGatewayEndpoint != null &&
                    this.TargetCsmConfig.Security != null &&
                    this.TargetCsmConfig.Security.CertificateInformation != null &&
                    (this.TargetCsmConfig.Security.CertificateInformation.ReverseProxyCertificate != null
                    || (this.TargetCsmConfig.Security.CertificateInformation.ReverseProxyCertificateCommonNames != null
                        && this.TargetCsmConfig.Security.CertificateInformation.ReverseProxyCertificateCommonNames.Any())))
                {
                    nodeType.Endpoints.HttpApplicationGatewayEndpoint.Protocol = InputEndpointTypeProtocol.https;
                }

                result.Add(nodeType);
            }

            return result;
        }

        protected FabricSettingsGeneratorBase CreateFabricSettingsGenerator(
            IUserConfig inputTargetUserConfig,
            IAdminConfig inputTargetAdminConfig,
            ClusterManifestGeneratorSettings inputClusterManifestGeneratorSettings,
            FabricSettingsMetadata currentFabricSettingsMetadata,
            ClusterManifestType existingClusterManifest)
        {
            if (this.TargetCsmConfig.IsLinuxVM)
            {
                return this.FabricSettingsActivator.CreateLinuxSettingsGenerator(inputTargetUserConfig, inputTargetAdminConfig, currentFabricSettingsMetadata, existingClusterManifest, inputClusterManifestGeneratorSettings, this.TraceLogger);
            }
            else
            {
                return this.FabricSettingsActivator.CreateWindowsSettingsGenerator(inputTargetUserConfig, inputTargetAdminConfig, currentFabricSettingsMetadata, existingClusterManifest, inputClusterManifestGeneratorSettings, this.TraceLogger);
            }
        }

        protected void SortClusterManifest(ClusterManifestType clusterManifest)
        {
            if (clusterManifest == null)
            {
                return;
            }

            clusterManifest.NodeTypes = clusterManifest.NodeTypes.OrderBy(nodeType => nodeType.Name).ToArray();
            foreach (var nodeType in clusterManifest.NodeTypes)
            {
                nodeType.Capacities = nodeType.Capacities == null ? null : nodeType.Capacities.OrderBy(capacity => capacity.Name).ToArray();
                nodeType.PlacementProperties = nodeType.PlacementProperties == null
                                               ? null
                                               : nodeType.PlacementProperties.OrderBy(placementProperty => placementProperty.Name).ToArray();
            }

            // var paasInfrastructureItem = ((ClusterManifestTypeInfrastructurePaaS)clusterManifest.Infrastructure.Item);
            // paasInfrastructureItem.Roles = paasInfrastructureItem.Roles.OrderBy(role => role.RoleName).ToArray();
            // paasInfrastructureItem.Votes = paasInfrastructureItem.Votes.OrderBy(vote => vote.NodeName).ToArray();
            clusterManifest.Infrastructure = this.OnSortInfrastructure(clusterManifest.Infrastructure);

            clusterManifest.FabricSettings = clusterManifest.FabricSettings.OrderBy(section => section.Name).ToArray();
            foreach (var section in clusterManifest.FabricSettings)
            {
                section.Parameter = section.Parameter == null ? null : section.Parameter.OrderBy(param => param.Name).ToArray();
            }
        }

        /// <summary>
        /// Overload this method to allow different seed node types.
        /// TODO: refactor to combine the two methods
        /// </summary>
        /// <param name="security"></param>
        /// <param name="existingClusterManifest"></param>
        /// <param name="nodeTypes"></param>
        /// <param name="currentFabricSettingsMetadata"></param>
        /// <param name="clusterManifestVersion"></param>
        /// <returns></returns>
        private ClusterManifestType UpdateClusterManifest(
            Security security,
            ClusterManifestType existingClusterManifest,
            IEnumerable<ClusterManifestTypeNodeType> nodeTypes,
            FabricSettingsMetadata currentFabricSettingsMetadata,
            string clusterManifestVersion,
            List<NodeDescription> existingSeedNodes)
        {
            CertificateDescription thumbprintClusterCert;
            List<string> thumbprintClusterCertList;
            ServerCertificateCommonNames commonNameClusterCert;
            Dictionary<string, string> commonNameClusterCertList;

            this.GetClusterCertConfig(
                security,
                out thumbprintClusterCert,
                out thumbprintClusterCertList,
                out commonNameClusterCert,
                out commonNameClusterCertList);

            return this.UpdateClusterManifest(
                security,
                existingClusterManifest,
                nodeTypes,
                currentFabricSettingsMetadata,
                clusterManifestVersion,
                thumbprintClusterCert,
                thumbprintClusterCertList,
                commonNameClusterCert,
                commonNameClusterCertList,
                existingSeedNodes);
        }

        private ClusterManifestType UpdateClusterManifest(
            ClusterManifestType existingClusterManifest,
            FabricSettingsMetadata currentFabricSettingsMetadata,
            string clusterManifestVersion,
            CertificateClusterUpgradeStep step,
            List<NodeDescription> existingSeedNodes)
        {
            Security nodeTypeSecurity = this.TargetCsmConfig.Security.Clone();
            nodeTypeSecurity.CertificateInformation.ClusterCertificate = step.ThumbprintLoadList;
            nodeTypeSecurity.CertificateInformation.ClusterCertificateCommonNames = step.CommonNameLoadList;

            return this.UpdateClusterManifest(
                this.TargetCsmConfig.Security,
                existingClusterManifest,
                this.GetNodeTypes(this.TargetCsmConfig.NodeTypes, nodeTypeSecurity),
                currentFabricSettingsMetadata,
                clusterManifestVersion,
                step.ThumbprintFileStoreSvcList,
                step.ThumbprintWhiteList,
                step.CommonNameFileStoreSvcList,
                step.CommonNameWhiteList,
                existingSeedNodes);
        }

        /// <summary>
        /// Overload this method to allow different seed node types.
        /// TODO: refactor to combine the two methods
        /// </summary>
        /// <param name="security"></param>
        /// <param name="existingClusterManifest"></param>
        /// <param name="nodeTypes"></param>
        /// <param name="currentFabricSettingsMetadata"></param>
        /// <param name="clusterManifestVersion"></param>
        /// <returns></returns>
        private ClusterManifestType UpdateClusterManifest(
            Security security,
            ClusterManifestType existingClusterManifest,
            IEnumerable<ClusterManifestTypeNodeType> nodeTypes,
            FabricSettingsMetadata currentFabricSettingsMetadata,
            string clusterManifestVersion,
            CertificateDescription thumbprintFileStoreCert,
            List<string> thumbprintWhiteList,
            ServerCertificateCommonNames commonNameFileStoreCert,
            Dictionary<string, string> commonNameWhiteList,
            List<NodeDescription> existingSeedNodes)
        {
            FabricSettingsGeneratorBase fabricSettingsGenerator = this.CreateFabricSettingsGenerator(
                        this.TargetCsmConfig,
                        this.TargetWrpConfig,
                        this.ClusterManifestGeneratorSettings,
                        currentFabricSettingsMetadata,
                        existingClusterManifest);
            var fabricSettings = fabricSettingsGenerator.GenerateFabricSettings(
                security,
                thumbprintFileStoreCert,
                thumbprintWhiteList,
                commonNameFileStoreCert,
                commonNameWhiteList);

            var primaryNodesTypes = this.TargetCsmConfig.NodeTypes.Where(nodeType => nodeType.IsPrimary);

            var updatedCluterManifest = new ClusterManifestType
            {
                FabricSettings = fabricSettings,
                Infrastructure = this.OnGetInfrastructure(this.Topology, existingSeedNodes, nodeTypes),
                NodeTypes = nodeTypes.ToArray(),
                Name = StringConstants.ClusterManifestName,
                Version = clusterManifestVersion,
                Description = GeneratedCluterManifestDescription
            };

            this.SortClusterManifest(updatedCluterManifest);

            return updatedCluterManifest;
        }

        private void GetClusterCertConfig(
            Security security,
            out CertificateDescription thumbprintClusterCert,
            out List<string> thumbprintClusterCertList,
            out ServerCertificateCommonNames commonNameClusterCert,
            out Dictionary<string, string> commonNameClusterCertList)
        {
            thumbprintClusterCert = null;
            thumbprintClusterCertList = new List<string>();
            commonNameClusterCert = null;
            commonNameClusterCertList = new Dictionary<string, string>();

            if (security != null && security.CertificateInformation != null)
            {
                if (security.CertificateInformation.ClusterCertificate != null)
                {
                    thumbprintClusterCert = security.CertificateInformation.ClusterCertificate;
                    thumbprintClusterCertList = thumbprintClusterCert.ToThumbprintList();
                }

                if (security.CertificateInformation.ClusterCertificateCommonNames != null
                    && security.CertificateInformation.ClusterCertificateCommonNames.Any())
                {
                    commonNameClusterCert = security.CertificateInformation.ClusterCertificateCommonNames;
                    commonNameClusterCertList = commonNameClusterCert.CommonNames.ToDictionary(p => p.CertificateCommonName, p => p.CertificateIssuerThumbprint);
                }
            }
        }

        private IEnumerable<NodeDescription> GetSeedNodes(ReliabilityLevel reliabilityLevel, int primaryNodeCount, IEnumerable<NodeTypeDescription> primaryNodeTypes, bool isVmss)
        {
            // In add non-primary node scenario, topology.nodes is set to the new node count; targetNodeConfig.NodeStatus still contains the old node count. This will cause exceptions
            // in var matchingNodeStatus = this.TargetNodeConfig.NodesStatus.First(nodeStatus => nodeStatus.NodeName.Equals(node.NodeName));
            // So here originalTopology contains the original nodes which we use to calculate the seednode based on.
            Dictionary<string, NodeDescription> originalTopology = new Dictionary<string, NodeDescription>();
            foreach (var node in this.Topology.Nodes)
            {
                if (this.TargetNodeConfig.NodesStatus.Select(n => n.NodeName).Distinct().ToList().Contains(node.Key))
                {
                    originalTopology.Add(node.Key, node.Value);
                }
            }

            IEnumerable<NodeDescription> primaryNodeTypeNodes = originalTopology.Values
                    .Where(node => primaryNodeTypes.Any(primaryNodeType => primaryNodeType.Name == node.NodeTypeRef))
                    .Where(node =>
                    {
                        // skip nodes which are either in removed state or disabled with intent to RemoveNode
                        var matchingNodeStatus = this.TargetNodeConfig.NodesStatus.FirstOrDefault(nodeStatus => nodeStatus.NodeName.Equals(node.NodeName));
                        return matchingNodeStatus != null ? (matchingNodeStatus.NodeState != NodeState.Removed) && (matchingNodeStatus.NodeDeactivationIntent != WrpNodeDeactivationIntent.RemoveNode) : false;
                    });

            IEnumerable<NodeDescription> seedNodes = this.SeedNodeSelector.Select(
                        reliabilityLevel,
                        primaryNodeCount,
                        primaryNodeTypeNodes,
                        this.ClusterManifestGeneratorSettings.FDCount,
                        this.ClusterManifestGeneratorSettings.UDCount,
                        isVmss);

            return seedNodes;
        }

        private PaaSVoteType ToPaaSVoteType(NodeDescription seedNode, IEnumerable<ClusterManifestTypeNodeType> nodeTypes)
        {
            var matchingNodeType =
                nodeTypes.FirstOrDefault(nodeType => string.Equals(nodeType.Name, seedNode.NodeTypeRef));
            if (matchingNodeType == null)
            {
                throw new ClusterManagementException(
                    0,
                    string.Format("SeedNode Name={0} has NodeTypeRef={1}. The NodeType is not found.", seedNode.NodeName, seedNode.NodeTypeRef));
            }

            return new PaaSVoteType()
            {
                IPAddressOrFQDN = seedNode.IPAddress,
                Port = int.Parse(matchingNodeType.Endpoints.ClusterConnectionEndpoint.Port),
                NodeName = seedNode.NodeName
            };
        }

        private NodeDescription ToNodeDescription(PaaSVoteType vote)
        {
            var nodeType = this.TargetNodeConfig.NodesStatus.First(node => node.NodeName == vote.NodeName).NodeType;

            return new NodeDescription()
            {
                NodeName = vote.NodeName,
                IPAddress = vote.IPAddressOrFQDN,
                NodeTypeRef = nodeType
            };
        }
    }
}
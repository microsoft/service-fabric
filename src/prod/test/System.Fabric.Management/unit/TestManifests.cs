// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.Linq;
    using System.Globalization;
    using System.IO;
    using Management.Test;

    enum InfrastructureType
    {
        WindowsServer,
        WindowsAzure,
        VMM,
        Blackbird,
        PaaS
    }

    internal class TestManifests
    {

        public TestManifests(bool isHttpGatewayEnabled,
            bool isUpgrade,
            bool isLogicalDirectories,
            int nNodeTypes,
            bool isScaleMin,
            InfrastructureType infrastructureType,
            int[] nNodes,
            int[] nSeedNodes,
            int[] nFaultDomains,
            int[] nUpgradeDomains,
            string testName,
            string version,
            bool isConfigure,
            bool disallowDnsSetup)
        {
            this.InitializeFabricSettings(isHttpGatewayEnabled, isUpgrade, isConfigure, disallowDnsSetup, testName);
            this.InitializeInfrastructureSettings(
                nNodeTypes,
                isScaleMin,
                isLogicalDirectories,
                nNodes,
                nSeedNodes,
                nFaultDomains,
                nUpgradeDomains,
                infrastructureType);
            InitializeInfrastructureSettings(infrastructureType, isScaleMin, nNodeTypes, nSeedNodes);
            this.ClusterManifest = new ClusterManifestType()
            {
                Certificates = null,
                Description = testName,
                FabricSettings = this.sections,
                Infrastructure = this.infrastructure,
                Name = testName,
                NodeTypes = this.nodeTypes,
                Version = version
            };
            this.InfrastructureManifest = new InfrastructureInformationType() { NodeList = infraNodes };
            InitializeClusterSettings();
        }

        public ClusterManifestType ClusterManifest { get; private set; }

        public InfrastructureInformationType InfrastructureManifest { get; private set; }

        public SettingsTypeSection[] ClusterSettings { get; private set; }

        private void InitializeClusterSettings()
        {
            this.ClusterSettings = new SettingsTypeSection[this.sections.Length + 1];
            for (int i = 0; i < this.sections.Length; i++)
            {
                this.ClusterSettings[i] = new SettingsTypeSection() { Name = this.sections[i].Name };
                if (this.sections[i].Parameter == null)
                {
                    continue;
                }
                this.ClusterSettings[i].Parameter = new SettingsTypeSectionParameter[this.sections[i].Parameter.Length];
                for (int j = 0; j < this.sections[i].Parameter.Length; j++)
                {
                    this.ClusterSettings[i].Parameter[j] = new SettingsTypeSectionParameter()
                    {
                        Name = this.sections[i].Parameter[j].Name,
                        Value = this.sections[i].Parameter[j].Value
                    };
                }
            }
            List<SettingsTypeSectionParameter> parameter = new List<SettingsTypeSectionParameter>();
            foreach (var node in this.infraNodes)
            {
                if (node.IsSeedNode)
                {
                    var nodeAddress = node.Endpoints.ClusterConnectionEndpoint.Port;
                    parameter.Add(new SettingsTypeSectionParameter()
                    {
                        Name = node.NodeName,
                        Value = string.Format("{0},{1}",
                            this.ClusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsAzure ? "WindowsAzure" : "SeedNode",
                            this.ClusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsAzure ? node.NodeName : string.Format("{0}:{1}", node.IPAddressOrFQDN, nodeAddress))
                    });
                }
            }
            this.ClusterSettings[this.sections.Length] = new SettingsTypeSection() { Name = "Votes", Parameter = parameter.ToArray() };
        }

        private void InitializeInfrastructureSettings(InfrastructureType infrastructureType, bool isScaleMin, int nNodeTypes, int[] nSeedNodes)
        {
            switch (infrastructureType)
            {
                case InfrastructureType.Blackbird:
                    return;
                case InfrastructureType.VMM:
                    return;
                case InfrastructureType.PaaS:
                    var paasInfra = new ClusterManifestTypeInfrastructurePaaS();
                    paasInfra.Roles = new PaaSRoleType[nNodeTypes];
                    for (int i = 0; i < nNodeTypes; i++)
                    {
                        paasInfra.Roles[i] = new PaaSRoleType();
                        paasInfra.Roles[i].RoleName = GetRoleOrTiername(i);
                        paasInfra.Roles[i].NodeTypeRef = GetRoleOrTiername(i);
                        paasInfra.Roles[i].RoleNodeCount = nSeedNodes[i];
                    }
                    var paasVotes = new List<PaaSVoteType>();
                    foreach (var node in this.infraNodes)
                    {
                        if (node.IsSeedNode)
                        {
                            paasVotes.Add(new PaaSVoteType()
                            {
                                NodeName = node.NodeName,
                                IPAddressOrFQDN = node.IPAddressOrFQDN,
                                Port = Convert.ToInt32(node.Endpoints.ClusterConnectionEndpoint.Port)
                            });
                        }
                    }
                    paasInfra.Votes = paasVotes.ToArray();
                    this.infrastructure = new ClusterManifestTypeInfrastructure();
                    this.infrastructure.Item = paasInfra;
                    return;
                case InfrastructureType.WindowsAzure:
                    var azureInfra = new ClusterManifestTypeInfrastructureWindowsAzure();
                    azureInfra.Roles = new AzureRoleType[nNodeTypes];
                    for (int i = 0; i < nNodeTypes; i++)
                    {
                        azureInfra.Roles[i] = new AzureRoleType();
                        azureInfra.Roles[i].RoleName = GetRoleOrTiername(i);
                        azureInfra.Roles[i].NodeTypeRef = GetRoleOrTiername(i);
                        azureInfra.Roles[i].SeedNodeCount = nSeedNodes[i];
                    }
                    this.infrastructure = new ClusterManifestTypeInfrastructure();
                    this.infrastructure.Item = azureInfra;
                    return;
                case InfrastructureType.WindowsServer:
                    var infra = new ClusterManifestTypeInfrastructureWindowsServer();
                    infra.IsScaleMin = isScaleMin;
                    infra.NodeList = this.nodes;
                    this.infrastructure = new ClusterManifestTypeInfrastructure();
                    this.infrastructure.Item = infra;
                    return;
            }
        }

        private void InitializeInfrastructureSettings(
            int nNodeTypes,
            bool isScaleMin,
            bool isLogicalDirectories,
            int[] nNodes,
            int[] nSeedNodes,
            int[] nFaultDomains,
            int[] nUpgradDomains,
            InfrastructureType infrastructureType)
        {
            this.nodeTypes = new ClusterManifestTypeNodeType[nNodeTypes];
            int totalNodes = 0;
            for (int i = 0; i < nNodeTypes; i++)
            {
                totalNodes += nNodes[i];
            }
            this.nodes = new FabricNodeType[totalNodes];
            this.infraNodes = new InfrastructureNodeType[totalNodes];
            int nodeIndex = 0;
            for (int i = 0; i < nNodeTypes; i++)
            {
                int clientConnectionPort = 19000 + i;
                int leaseDriverPort = 19000 + nNodeTypes + i;
                int clusterConnectionPort = 19000 + 2 * nNodeTypes + i;
                int httpGatewayPort = 19000 + 3 * nNodeTypes + i;
                int serviceConnectionPort = 19000 + 4 * nNodeTypes + i;
                int startAppPort = 30000 + 1000 * i + 1;
                int endAppPort = 30000 + 1000 * (i + 1);
                string host = isScaleMin ? "localhost" : System.Net.Dns.GetHostName();
                string roleOrTierName = GetRoleOrTiername(i);
                string faultDomain = string.Format(CultureInfo.InvariantCulture, "fd:/Rack{0}", i % nFaultDomains[i]);
                string upgradeDomain = string.Format(CultureInfo.InvariantCulture, "MYUD{0}", i % nUpgradDomains[i]);
                FabricEndpointsType endpoints = new FabricEndpointsType()
                {
                    ApplicationEndpoints = new FabricEndpointsTypeApplicationEndpoints() { StartPort = startAppPort, EndPort = endAppPort },
                    LeaseDriverEndpoint = new InternalEndpointType() { Port = leaseDriverPort.ToString(CultureInfo.InvariantCulture) },
                    ClusterConnectionEndpoint = new InternalEndpointType() { Port = clusterConnectionPort.ToString(CultureInfo.InvariantCulture) },
                    ClientConnectionEndpoint = new InputEndpointType() { Port = clientConnectionPort.ToString(CultureInfo.InvariantCulture) },
                    HttpGatewayEndpoint = new InputEndpointType() { Port = httpGatewayPort.ToString(CultureInfo.InvariantCulture) },
                    ServiceConnectionEndpoint = new InternalEndpointType() { Port = serviceConnectionPort.ToString(CultureInfo.InvariantCulture) }
                };

                for (int j = 0; j < nNodes[i]; j++)
                {
                    bool isSeedNode = j < nSeedNodes[i];
                    string nodeName = string.Format(CultureInfo.InvariantCulture, "{0}.Node.{1}", roleOrTierName, j);
                    FabricNodeType node = new FabricNodeType() { NodeName = nodeName, FaultDomain = faultDomain, IPAddressOrFQDN = host, IsSeedNode = isSeedNode, NodeTypeRef = roleOrTierName, UpgradeDomain = upgradeDomain };
                    InfrastructureNodeType infraNode = new InfrastructureNodeType() { NodeName = nodeName, FaultDomain = faultDomain, IPAddressOrFQDN = host, IsSeedNode = isSeedNode, NodeTypeRef = roleOrTierName, UpgradeDomain = upgradeDomain, RoleOrTierName = roleOrTierName, Certificates = null, Endpoints = endpoints };
                    this.nodes[nodeIndex] = node;
                    this.infraNodes[nodeIndex] = infraNode;
                    nodeIndex++;
                }
                nodeTypes[i] = new ClusterManifestTypeNodeType();
                nodeTypes[i].Certificates = null;
                if (infrastructureType == InfrastructureType.WindowsServer || infrastructureType == InfrastructureType.VMM || infrastructureType == InfrastructureType.PaaS)
                {
                    nodeTypes[i].Endpoints = endpoints;
                }

                nodeTypes[i].Name = roleOrTierName;
            }

            if (isLogicalDirectories)
            {
                nodeTypes[0].LogicalDirectories = new LogicalDirectoryType[5];
                nodeTypes[0].LogicalDirectories[0] = new LogicalDirectoryType
                {
                    LogicalDirectoryName = "0",
                    MappedTo = TestUtility.LogicalDirectoriesApplicationCheckPointFilesDir,
                    Context = LogicalDirectoryTypeContext.node
                };

                nodeTypes[0].LogicalDirectories[1] = new LogicalDirectoryType
                {
                    LogicalDirectoryName = "1",
                    MappedTo = TestUtility.LogicalDirectoriesLogDir,
                    Context = LogicalDirectoryTypeContext.node
                };
                nodeTypes[0].LogicalDirectories[2] = new LogicalDirectoryType
                {
                    LogicalDirectoryName = "2",
                    MappedTo = TestUtility.LogicalDirectoriesBackupDir,
                    Context = LogicalDirectoryTypeContext.application
                };
                nodeTypes[0].LogicalDirectories[3] = new LogicalDirectoryType
                {
                    LogicalDirectoryName = "3",
                    MappedTo = TestUtility.LogicalDirectoriesUserDefined1Dir
                };
                nodeTypes[0].LogicalDirectories[4] = new LogicalDirectoryType
                {
                    LogicalDirectoryName = "4",
                    MappedTo = TestUtility.LogicalDirectoriesUserDefined2Dir,
                    Context = LogicalDirectoryTypeContext.node
                };

                nodeTypes[1].LogicalDirectories = new LogicalDirectoryType[5];
                nodeTypes[1].LogicalDirectories[0] = new LogicalDirectoryType
                {
                    LogicalDirectoryName = "0",
                    MappedTo = TestUtility.LogicalDirectoriesApplicationCheckPointFilesDir2,
                    Context = LogicalDirectoryTypeContext.application
                };

                nodeTypes[1].LogicalDirectories[1] = new LogicalDirectoryType
                {
                    LogicalDirectoryName = "1",
                    MappedTo = TestUtility.LogicalDirectoriesLogDir2,
                    Context = LogicalDirectoryTypeContext.application
                };
                nodeTypes[1].LogicalDirectories[2] = new LogicalDirectoryType
                {
                    LogicalDirectoryName = "2",
                    MappedTo = TestUtility.LogicalDirectoriesBackupDir2,
                    Context = LogicalDirectoryTypeContext.node
                };
                nodeTypes[1].LogicalDirectories[3] = new LogicalDirectoryType
                {
                    LogicalDirectoryName = "3",
                    MappedTo = TestUtility.LogicalDirectoriesUserDefined1Dir2,
                    Context = LogicalDirectoryTypeContext.application
                };
                nodeTypes[1].LogicalDirectories[4] = new LogicalDirectoryType
                {
                    LogicalDirectoryName = "4",
                    MappedTo = TestUtility.LogicalDirectoriesUserDefined2Dir2,
                    Context = LogicalDirectoryTypeContext.application
                };
            }
        }

        private static string GetRoleOrTiername(int i)
        {
            string roleOrTierName = string.Format(CultureInfo.InvariantCulture, "NodeType{0}", i);
            return roleOrTierName;
        }

        private void InitializeFabricSettings(bool isHttpGatewayEnabled, bool isUpgrade, bool isConfigure, bool disAllowDnsSetup, string testName)
        {
            int sectionsCount = isUpgrade ? isConfigure ? 15 : 14 : isConfigure ? 14 : 13;
            this.sections = new SettingsOverridesTypeSection[sectionsCount];
            this.sections[0] = new SettingsOverridesTypeSection() { Name = "Security" };
            this.sections[0].Parameter = new SettingsOverridesTypeSectionParameter[2];
            this.sections[0].Parameter[0] = new SettingsOverridesTypeSectionParameter() { Name = "ClusterCredentialType", Value = "None" };
            this.sections[0].Parameter[1] = new SettingsOverridesTypeSectionParameter() { Name = "ServerAuthCredentialType", Value = "None" };
            this.sections[1] = new SettingsOverridesTypeSection() { Name = "NamingService" };
            this.sections[1].Parameter = new SettingsOverridesTypeSectionParameter[1];
            this.sections[1].Parameter[0] = new SettingsOverridesTypeSectionParameter() { Name = "PartitionCount", Value = "1" };
            this.sections[2] = new SettingsOverridesTypeSection() { Name = "FailoverManager" };
            this.sections[2].Parameter = new SettingsOverridesTypeSectionParameter[7];
            this.sections[2].Parameter[0] = new SettingsOverridesTypeSectionParameter() { Name = "ExpectedClusterSize", Value = "1" };
            this.sections[2].Parameter[1] = new SettingsOverridesTypeSectionParameter() { Name = "ClusterStableWaitDuration", Value = "0" };
            this.sections[2].Parameter[2] = new SettingsOverridesTypeSectionParameter() { Name = "MinRebuildRetryInterval", Value = "1" };
            this.sections[2].Parameter[3] = new SettingsOverridesTypeSectionParameter() { Name = "MaxRebuildRetryInterval", Value = "1" };
            this.sections[2].Parameter[4] = new SettingsOverridesTypeSectionParameter() { Name = "PeriodicStateScanInterval", Value = "1" };
            this.sections[2].Parameter[5] = new SettingsOverridesTypeSectionParameter() { Name = "MinActionRetryIntervalPerReplica", Value = "1" };
            this.sections[2].Parameter[6] = new SettingsOverridesTypeSectionParameter() { Name = "StandByReplicaKeepDuration", Value = "60" };
            this.sections[3] = new SettingsOverridesTypeSection() { Name = "ClusterManager" };
            this.sections[4] = new SettingsOverridesTypeSection() { Name = "Management" };
            this.sections[4].Parameter = new SettingsOverridesTypeSectionParameter[1];
            this.sections[4].Parameter[0] = new SettingsOverridesTypeSectionParameter() { Name = "ImageStoreConnectionString", Value = "file:RandomTestLocation" };
            this.sections[5] = new SettingsOverridesTypeSection() { Name = "Hosting" };
            this.sections[5].Parameter = new SettingsOverridesTypeSectionParameter[1];
            this.sections[5].Parameter[0] = new SettingsOverridesTypeSectionParameter() { Name = "EndpointProviderEnabled", Value = "true" };
            this.sections[6] = new SettingsOverridesTypeSection() { Name = "Trace/File" };
            this.sections[6].Parameter = new SettingsOverridesTypeSectionParameter[3];
            this.sections[6].Parameter[0] = new SettingsOverridesTypeSectionParameter() { Name = "Level", Value = "4" };
            this.sections[6].Parameter[1] = new SettingsOverridesTypeSectionParameter() { Name = "Path", Value = @"..\log\Fabric.trace" };
            this.sections[6].Parameter[2] = new SettingsOverridesTypeSectionParameter() { Name = "Option", Value = "m" };
            this.sections[7] = new SettingsOverridesTypeSection
            {
                Name = "Diagnostics",
                Parameter = new[] { new SettingsOverridesTypeSectionParameter { Name = "ProducerInstances", Value = "ServiceFabricEtlFile, ServiceFabricEtlFileQueryable, ServiceFabricEtlFileOperational, ServiceFabricCrashDump, ServiceFabricPerfCounter" } }
            };
            this.sections[8] = new SettingsOverridesTypeSection
            {
                Name = "ServiceFabricEtlFile",
                Parameter = new[] {
                    new SettingsOverridesTypeSectionParameter { Name = "ProducerType", Value = "EtlFileProducer" },
                    new SettingsOverridesTypeSectionParameter { Name = "IsEnabled", Value = "true" }
                }
            };
            this.sections[9] = new SettingsOverridesTypeSection
            {
                Name = "ServiceFabricEtlFileQueryable",
                Parameter = new[] {
                    new SettingsOverridesTypeSectionParameter { Name = "ProducerType", Value = "EtlFileProducer" },
                    new SettingsOverridesTypeSectionParameter { Name = "ServiceFabricEtlType", Value = "QueryEtl" },
                    new SettingsOverridesTypeSectionParameter { Name = "IsEnabled", Value = "true" }
                }
            };
            this.sections[10] = new SettingsOverridesTypeSection
            {
                Name = "ServiceFabricEtlFileOperational",
                Parameter = new[] {
                    new SettingsOverridesTypeSectionParameter { Name = "ProducerType", Value = "EtlFileProducer" },
                    new SettingsOverridesTypeSectionParameter { Name = "ServiceFabricEtlType", Value = "OperationalEtl" },
                    new SettingsOverridesTypeSectionParameter { Name = "IsEnabled", Value = "true" }
                }
            };
            this.sections[11] = new SettingsOverridesTypeSection
            {
                Name = "ServiceFabricCrashDump",
                Parameter = new[] {
                    new SettingsOverridesTypeSectionParameter { Name = "ProducerType", Value = "FolderProducer" },
                    new SettingsOverridesTypeSectionParameter { Name = "FolderType", Value = "ServiceFabricCrashDumps" },
                    new SettingsOverridesTypeSectionParameter { Name = "IsEnabled", Value = "true" }
                }
            };
            this.sections[12] = new SettingsOverridesTypeSection
            {
                Name = "ServiceFabricPerfCounter",
                Parameter = new[] {
                    new SettingsOverridesTypeSectionParameter { Name = "ProducerType", Value = "FolderProducer" },
                    new SettingsOverridesTypeSectionParameter { Name = "FolderType", Value = "ServiceFabricPerformanceCounters" },
                    new SettingsOverridesTypeSectionParameter { Name = "IsEnabled", Value = "true" }
                }
            };

            int indexForSetup = 13;
            if (isUpgrade)
            {
                this.sections[indexForSetup] = new SettingsOverridesTypeSection() { Name = "Federation" };
                this.sections[indexForSetup].Parameter = new SettingsOverridesTypeSectionParameter[1];
                this.sections[indexForSetup].Parameter[0] = new SettingsOverridesTypeSectionParameter() { Name = "UseV2NodeIdGenerator", Value = "true" };
                indexForSetup++;
            }
            if (isConfigure)
            {
                int parametersCount = 4;

                if (disAllowDnsSetup)
                {
                    parametersCount = 5;
                }

                this.sections[indexForSetup] = new SettingsOverridesTypeSection() { Name = "Setup" };
                this.sections[indexForSetup].Parameter = new SettingsOverridesTypeSectionParameter[parametersCount];
                this.sections[indexForSetup].Parameter[0] = new SettingsOverridesTypeSectionParameter() { Name = "FabricDataRoot", Value = Path.Combine(FabricDeployerDeploymentTests.RootLocation, testName) };
                this.sections[indexForSetup].Parameter[1] = new SettingsOverridesTypeSectionParameter() { Name = "FabricLogRoot", Value = Path.Combine(FabricDeployerDeploymentTests.RootLocation, testName, "LogRoot") };
                this.sections[indexForSetup].Parameter[2] = new SettingsOverridesTypeSectionParameter() { Name = "ServiceStartupType", Value = "Manual" };
                this.sections[indexForSetup].Parameter[3] = new SettingsOverridesTypeSectionParameter() { Name = "ServiceRunAsAccountName", Value = "NT Authority\\NetworkService" };

                if (disAllowDnsSetup)
                {
                    this.sections[indexForSetup].Parameter[4] = new SettingsOverridesTypeSectionParameter() { Name = "ContainerDnsSetup", Value = "Disallow" };
                }
            }
        }

        private ClusterManifestTypeInfrastructure infrastructure;
        private FabricNodeType[] nodes;
        private InfrastructureNodeType[] infraNodes;
        private ClusterManifestTypeNodeType[] nodeTypes;
        private SettingsOverridesTypeSection[] sections;

        public static void VerifyAreEqual(ClusterManifestType cm1, ClusterManifestType cm2)
        {
            Assert.AreEqual(cm1.Name, cm2.Name, "Name");
            Assert.AreEqual(cm1.Description, cm2.Description, "Description");
            Assert.AreEqual(cm1.Version, cm2.Version, "Version");
            VerifyAreEqual(cm1.NodeTypes, cm2.NodeTypes);
            VerifyAreEqual(cm1.FabricSettings, cm2.FabricSettings);
            VerifyAreEqual(cm1.Infrastructure, cm2.Infrastructure);
        }

        private static void VerifyAreEqual(ClusterManifestTypeInfrastructure infra1, ClusterManifestTypeInfrastructure infra2)
        {
            Assert.AreEqual(infra1.Item.GetType(), infra2.Item.GetType());
            if (infra1.Item is ClusterManifestTypeInfrastructureWindowsServer)
            {
                var infrastructure1 = infra1.Item as ClusterManifestTypeInfrastructureWindowsServer;
                var infrastructure2 = infra2.Item as ClusterManifestTypeInfrastructureWindowsServer;
                VerifyAreEqual(infrastructure1, infrastructure2);
            }
        }

        private static void VerifyAreEqual(ClusterManifestTypeInfrastructureWindowsServer infrastructure1, ClusterManifestTypeInfrastructureWindowsServer infrastructure2)
        {
            Assert.AreEqual(infrastructure1.IsScaleMin, infrastructure2.IsScaleMin, "Infrastructure.IsScaleMin");
            Assert.AreEqual(infrastructure1.NodeList.Length, infrastructure2.NodeList.Length, "Infrastructure.NodeList.Length");
            for (int i = 0; i < infrastructure1.NodeList.Length; i++)
            {
                FabricNodeType node1 = infrastructure1.NodeList[i];
                FabricNodeType node2 = infrastructure2.NodeList[i];
                Assert.AreEqual(node1.FaultDomain, node2.FaultDomain, string.Format(CultureInfo.InvariantCulture, "Infrastrucuture.NodeList[{0}]", i));
                Assert.AreEqual(node1.IPAddressOrFQDN, node2.IPAddressOrFQDN, string.Format(CultureInfo.InvariantCulture, "Infrastructure.NodeList[{0}].IPAddressOrFQDNL", i));
                Assert.AreEqual(node1.IsSeedNode, node2.IsSeedNode, string.Format("Infrastructure.NodeList[{0}].IsSeedNode", i));
                Assert.AreEqual(node1.NodeName, node2.NodeName, string.Format("Infrastructure.NodeList[{0}].NodeName", i));
                Assert.AreEqual(node1.NodeTypeRef, node2.NodeTypeRef, string.Format("Infrastructure.NodeList[{0}].NodeTypeRef", i));
                Assert.AreEqual(node1.UpgradeDomain, node2.UpgradeDomain, string.Format("Infrastructure.NodeList[{0}].UpgradeDomain", i));
            }
        }

        public static void VerifyAreEqual(SettingsOverridesTypeSection[] sections1, SettingsOverridesTypeSection[] sections2)
        {
            if (sections1 == null)
            {
                Assert.IsNull(sections2);
                return;
            }
            Assert.AreEqual(sections1.Length, sections2.Length, "FabricSettings.Sections.Lenght");
            for (int i = 0; i < sections1.Length; i++)
            {
                Assert.AreEqual(sections1[i].Name, sections2[i].Name, string.Format(CultureInfo.InvariantCulture, "FabricSettings.Sections[{0}]", i));
                if (sections1[i] == null)
                {
                    Assert.IsNull(sections2[i]);
                    continue;
                }
                if (sections1[i].Parameter == null)
                {
                    Assert.IsNull(sections2[i].Parameter);
                    continue;
                }
                Assert.AreEqual(sections1[i].Parameter.Length, sections2[i].Parameter.Length, string.Format(CultureInfo.InvariantCulture, "FabricSettings.Sections[{0}].Parameters.Lengt", i));
                for (int j = 0; j < sections1[i].Parameter.Length; j++)
                {
                    Assert.AreEqual(sections1[i].Parameter[j].Name, sections2[i].Parameter[j].Name, string.Format(CultureInfo.InvariantCulture, "FabricSettings.Sections[{0}].Parameters[{1}].Name", i, j));
                    Assert.AreEqual(sections1[i].Parameter[j].Value, sections2[i].Parameter[j].Value, string.Format(CultureInfo.InvariantCulture, "FabricSettings.Sections[{0}].Parameters[{1}].Value", i, j));
                    Assert.AreEqual(sections1[i].Parameter[j].IsEncrypted, sections2[i].Parameter[j].IsEncrypted, string.Format(CultureInfo.InvariantCulture, "FabricSettings.Sections[{0}].Parameters[{1}].IsEncrypted", i, j));
                }
            }
        }

        private static void VerifyAreEqual(ClusterManifestTypeNodeType[] nodeTypes1, ClusterManifestTypeNodeType[] nodeTypes2)
        {
            if (nodeTypes1 == null)
            {
                Assert.IsNull(nodeTypes2);
                return;
            }
            Assert.AreEqual(nodeTypes1.Length, nodeTypes2.Length, "NodeTypes.Length");
            for (int i = 0; i < nodeTypes1.Length; i++)
            {
                Assert.AreEqual(nodeTypes1[i].Name, nodeTypes2[i].Name, string.Format(CultureInfo.InvariantCulture, "NodeTypes[{0}].Name", i));
                VerifyAreEqual(nodeTypes1[i].Endpoints, nodeTypes2[i].Endpoints);
                VerifyAreEqual(nodeTypes1[i].Certificates, nodeTypes2[i].Certificates);
                VerifyAreEqual(nodeTypes1[i].Capacities, nodeTypes2[i].Capacities);
                VerifyAreEqual(nodeTypes1[i].PlacementProperties, nodeTypes2[i].PlacementProperties);
            }
        }

        private static void VerifyAreEqual(KeyValuePairType[] values1, KeyValuePairType[] values2)
        {
            if (values1 == null)
            {
                Assert.IsNull(values2);
                return;
            }
            Assert.AreEqual(values1.Length, values2.Length, "Capacites|PlacementProperties.Length");
            for (int i = 0; i < values1.Length; i++)
            {
                Assert.AreEqual(values1[i].Name, values2[i].Name);
                Assert.AreEqual(values1[i].Value, values2[i].Value);
            }
        }

        public static void VerifyAreEqual(CertificatesType certs1, CertificatesType certs2)
        {
            if (certs1 == null)
            {
                Assert.IsNull(certs2);
                return;
            }
            if (certs1.ClientCertificate == null)
            {
                Assert.IsNull(certs2.ClientCertificate);
                return;
            }
            else
            {
                Assert.AreEqual(certs1.ClientCertificate.X509FindType, certs2.ClientCertificate.X509FindType, "Certificates.Client.X509FindType");
                Assert.AreEqual(certs1.ClientCertificate.X509FindValue, certs2.ClientCertificate.X509FindValue, "Certificates.Client.X509FindValue");
                Assert.AreEqual(certs1.ClientCertificate.X509StoreName, certs2.ClientCertificate.X509StoreName, "Certificates.Client.X509StoreName");
            }

            if (certs1.ClusterCertificate == null)
            {
                Assert.IsNull(certs2.ClusterCertificate);
                return;
            }
            else
            {
                Assert.AreEqual(certs1.ClusterCertificate.X509FindType, certs2.ClusterCertificate.X509FindType, "Certificates.Cluster.X509FindType");
                Assert.AreEqual(certs1.ClusterCertificate.X509FindValue, certs2.ClusterCertificate.X509FindValue, "Certificates.Cluster.X509FindValue");
                Assert.AreEqual(certs1.ClusterCertificate.X509StoreName, certs2.ClusterCertificate.X509StoreName, "Certificates.Cluster.X509StoreName");
            }

            if (certs1.ServerCertificate == null)
            {
                Assert.IsNull(certs2.ServerCertificate);
            }
            else
            {
                Assert.AreEqual(certs1.ServerCertificate.X509FindType, certs2.ServerCertificate.X509FindType, "Certificates.Server.X509FindType");
                Assert.AreEqual(certs1.ServerCertificate.X509FindValue, certs2.ServerCertificate.X509FindValue, "Certificates.Server.X509FindValue");
                Assert.AreEqual(certs1.ServerCertificate.X509StoreName, certs2.ServerCertificate.X509StoreName, "Certificates.Server.X509StoreName");
            }
        }

        private static void VerifyAreEqual(FabricEndpointsType endpoints1, FabricEndpointsType endpoints2)
        {
            Assert.AreEqual(endpoints1.ApplicationEndpoints.EndPort, endpoints2.ApplicationEndpoints.EndPort, "ApplicationEndpoints.EndPort");
            Assert.AreEqual(endpoints1.ApplicationEndpoints.StartPort, endpoints2.ApplicationEndpoints.StartPort, "ApplicationEndpoints.StartPort");
            Assert.AreEqual(endpoints1.ClientConnectionEndpoint.Port, endpoints2.ClientConnectionEndpoint.Port, "ClientConnectionEndpoint.Port");
            Assert.AreEqual(endpoints1.ClientConnectionEndpoint.Protocol, endpoints2.ClientConnectionEndpoint.Protocol, "ClientConnectionEndpoint.Protocol");
            Assert.AreEqual(endpoints1.ClusterConnectionEndpoint.Port, endpoints2.ClusterConnectionEndpoint.Port, "ClusterConnectionEndpoint.Port");
            Assert.AreEqual(endpoints1.ClusterConnectionEndpoint.Protocol, endpoints2.ClusterConnectionEndpoint.Protocol, "ClusterConnectionEndpoint.Protocol");
            Assert.AreEqual(endpoints1.LeaseDriverEndpoint.Port, endpoints2.LeaseDriverEndpoint.Port, "LeaseDriverEndpoint.Port");
            Assert.AreEqual(endpoints1.LeaseDriverEndpoint.Protocol, endpoints2.LeaseDriverEndpoint.Protocol, "LeaseDriverEndpoint.Protocol");
            if (endpoints1.HttpGatewayEndpoint == null)
            {
                Assert.IsNull(endpoints2.HttpGatewayEndpoint);
            }
            else
            {
                Assert.AreEqual(endpoints1.HttpGatewayEndpoint.Port, endpoints2.HttpGatewayEndpoint.Port, "HttpGatewayEndpoint.Port");
                Assert.AreEqual(endpoints1.HttpGatewayEndpoint.Protocol, endpoints2.HttpGatewayEndpoint.Protocol, "HttpGatewayEndpoint.Protocol");
            }
            if (endpoints1.HttpGatewayEndpoint == null)
            {
                Assert.IsNull(endpoints2.HttpGatewayEndpoint);
            }
            else
            {
                Assert.AreEqual(endpoints1.HttpGatewayEndpoint.Port, endpoints2.HttpGatewayEndpoint.Port, "HttpGatewayEndpoint.Port");
                Assert.AreEqual(endpoints1.HttpGatewayEndpoint.Protocol, endpoints2.HttpGatewayEndpoint.Protocol, "HttpGatewayEndpoint.Protocol");
            }
            if (endpoints1.HttpGatewayEndpoint == null)
            {
                Assert.IsNull(endpoints2.HttpGatewayEndpoint);
            }
            else
            {
                Assert.AreEqual(endpoints1.HttpGatewayEndpoint.Port, endpoints2.HttpGatewayEndpoint.Port, "HttpGatewayEndpoint.Port");
                Assert.AreEqual(endpoints1.HttpGatewayEndpoint.Protocol, endpoints2.HttpGatewayEndpoint.Protocol, "HttpGatewayEndpoint.Protocol");
            }
            if (endpoints1.ClusterManagerReplicatorEndpoint == null)
            {
                Assert.IsNull(endpoints2.ClusterManagerReplicatorEndpoint);
            }
            else
            {
                Assert.AreEqual(endpoints1.ClusterManagerReplicatorEndpoint.Port, endpoints2.ClusterManagerReplicatorEndpoint.Port, "ClusterManagerReplicatorEndpoint.Port");
                Assert.AreEqual(endpoints1.ClusterManagerReplicatorEndpoint.Protocol, endpoints2.ClusterManagerReplicatorEndpoint.Protocol, "ClusterManagerReplicatorEndpoint.Protocol");
            }
            if (endpoints1.DefaultReplicatorEndpoint == null)
            {
                Assert.IsNull(endpoints2.DefaultReplicatorEndpoint);
            }
            else
            {
                Assert.AreEqual(endpoints1.DefaultReplicatorEndpoint.Port, endpoints2.DefaultReplicatorEndpoint.Port, "DefaultReplicatorEndpoint.Port");
                Assert.AreEqual(endpoints1.DefaultReplicatorEndpoint.Protocol, endpoints2.DefaultReplicatorEndpoint.Protocol, "DefaultReplicatorEndpoint.Protocol");
            }
            if (endpoints1.NamingReplicatorEndpoint == null)
            {
                Assert.IsNull(endpoints2.NamingReplicatorEndpoint);
            }
            else
            {
                Assert.AreEqual(endpoints1.NamingReplicatorEndpoint.Port, endpoints2.NamingReplicatorEndpoint.Port, "NamingReplicatorEndpoint.Port");
                Assert.AreEqual(endpoints1.NamingReplicatorEndpoint.Protocol, endpoints2.NamingReplicatorEndpoint.Protocol, "NamingReplicatorEndpoint.Protocol");
            }
            if (endpoints1.FailoverManagerReplicatorEndpoint == null)
            {
                Assert.IsNull(endpoints2.FailoverManagerReplicatorEndpoint);
            }
            else
            {
                Assert.AreEqual(endpoints1.FailoverManagerReplicatorEndpoint.Port, endpoints2.FailoverManagerReplicatorEndpoint.Port, "FailoverManagerReplicatorEndpoint.Port");
                Assert.AreEqual(endpoints1.FailoverManagerReplicatorEndpoint.Protocol, endpoints2.FailoverManagerReplicatorEndpoint.Protocol, "FailoverManagerReplicatorEndpoint.Protocol");
            }
            if (endpoints1.ServiceConnectionEndpoint == null)
            {
                Assert.IsNull(endpoints2.ServiceConnectionEndpoint);
            }
            else
            {
                Assert.AreEqual(endpoints1.ServiceConnectionEndpoint.Port, endpoints2.ServiceConnectionEndpoint.Port, "ServiceConnectionEndpoint.Port");
                Assert.AreEqual(endpoints1.ServiceConnectionEndpoint.Protocol, endpoints2.ServiceConnectionEndpoint.Protocol, "ServiceConnectionEndpoint.Protocol");
            }
        }

        public static void VerifyAreEqual(InfrastructureInformationType im1, InfrastructureInformationType im2)
        {
            Assert.AreEqual(im1.NodeList.Length, im2.NodeList.Length, "InfrastructureManifest.NodeList.Length");
            for (int i = 0; i < im1.NodeList.Length; i++)
            {
                var node1 = im1.NodeList[i];
                var node2 = im2.NodeList[i];
                VerifyAreEqual(node1.Certificates, node2.Certificates);
                VerifyAreEqual(node1.Endpoints, node2.Endpoints);
                Assert.AreEqual(node1.FaultDomain, node2.FaultDomain, "FaultDomain");
                Assert.AreEqual(node1.IPAddressOrFQDN, node2.IPAddressOrFQDN, "IPAddressOrFQDN");
                Assert.AreEqual(node1.IsSeedNode, node2.IsSeedNode, "IsSeedNode");
                Assert.AreEqual(node1.NodeName, node2.NodeName, "NodeName");
                Assert.AreEqual(node1.NodeTypeRef, node2.NodeTypeRef, "NodeTypeRef");
                Assert.AreEqual(node1.RoleOrTierName, node2.RoleOrTierName, "RoleOrTierName");
                Assert.AreEqual(node1.UpgradeDomain, node2.UpgradeDomain, "UpgradeDomain");
            }
        }
    }
}
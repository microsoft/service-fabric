// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Management.ServiceModel;
    using System.IO;
    using System.Xml.Serialization;

    /// <summary>
    /// This is the class that tests the cluster manifest builder in Microsoft.SystemFabric.ClusterManagementCommon DLL.
    /// </summary>
    [TestClass]
    internal class ClusterManifestBuilderTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestGenerateClusterManifest()
        {
            this.InternalGenerateClusterManifestTest("ClusterConfig.TestGenerateClusterManifest.ThreeNodes.json", "ClusterManifest.TestGenerateClusterManifest.ThreeNodes.xml");
            
            this.InternalGenerateClusterManifestTest("ClusterConfig.TestGenerateClusterManifest.FiveNodes.json", "ClusterManifest.TestGenerateClusterManifest.FiveNodes.xml");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestUpdateCertificateInClusterManifest()
        {
            this.InternalUpdateCertificateTest("ClusterConfig.TestUpdateCertificateInClusterManifest.V1.json",
                                               "ClusterConfig.TestUpdateCertificateInClusterManifest.V2.json",
                                               new string[3]{"ClusterManifest.TestUpdateCertificateInClusterManifest.V2(0).xml", 
                                                             "ClusterManifest.TestUpdateCertificateInClusterManifest.V2(1).xml",
                                                             "ClusterManifest.TestUpdateCertificateInClusterManifest.V2(2).xml"});

            this.InternalUpdateCertificateTest("ClusterConfig.TestUpdateCertificateInClusterManifest.V3.json",
                                               "ClusterConfig.TestUpdateCertificateInClusterManifest.V4.json",
                                               new string[3]{"ClusterManifest.TestUpdateCertificateInClusterManifest.V4(0).xml", 
                                                             "ClusterManifest.TestUpdateCertificateInClusterManifest.V4(1).xml",
                                                             "ClusterManifest.TestUpdateCertificateInClusterManifest.V4(2).xml"});

        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestUpdateClusterManifest()
        {
            this.InternalUpdateClusterManifestTest("ClusterConfig.TestUpdateClusterManifest.V1.json",
                                                   "ClusterConfig.TestUpdateClusterManifest.V2.json",
                                                   "ClusterManifest.TestUpdateClusterManifest.V2.xml");

        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestUpdateNodeTypeInClusterManifest()
        {
            this.InternalUpdateClusterManifestTest("ClusterConfig.TestUpdateClusterManifest-NodeType.V1.json",
                                                   "ClusterConfig.TestUpdateClusterManifest-NodeType.V2.json",
                                                   "ClusterManifest.TestUpdateClusterManifest-NodeType.V2.xml");

        }

        internal void InternalGenerateClusterManifestTest(string clusterConfigPath, string clusterManifestPath)
        {
            MockupJsonModel jsonConfig = Utility.GetJsonConfigFromFile(Path.Combine(Constant.TestDirectory, clusterConfigPath));

            var userConfig = jsonConfig.GetUserConfig();
            var clusterTopology = jsonConfig.GetClusterTopology();
            var adminConfig = new MockupAdminConfig();
            var logger = new MockupTraceLogger();

            List<NodeStatus> nodesStatus = new List<NodeStatus>();
            foreach (NodeDescription node in clusterTopology.Nodes.Values)
            {
                NodeStatus nodeStatus = new NodeStatus()
                {
                    NodeName = node.NodeName,
                    InstanceId = 0,
                    NodeDeactivationIntent = WrpNodeDeactivationIntent.Invalid,
                    NodeState = NodeState.Enabled,
                    NodeType = node.NodeTypeRef
                };

                nodesStatus.Add(nodeStatus);
            }

            MockupCluster clusterResource = new MockupCluster(adminConfig, userConfig, clusterTopology, logger);
            MockupClusterManifestBuilder mockupClusterManifestBuilder = new MockupClusterManifestBuilder(
                                                                            clusterTopology,
                                                                            clusterResource.SeedNodeSelector,
                                                                            userConfig,
                                                                            adminConfig,
                                                                            new ClusterNodeConfig(nodesStatus, 2),
                                                                            new MockupManifestVersionGenerator(),
                                                                            clusterResource.FabricSettingsActivator,
                                                                            clusterResource.ClusterManifestGeneratorSettings,
                                                                            new MockupTraceLogger());

            var generatedClusterManifest = mockupClusterManifestBuilder.GenerateClusterManifest();
            var expectedClusterManifest = System.Fabric.Interop.Utility.ReadXml<ClusterManifestType>(Path.Combine(Constant.TestDirectory, clusterManifestPath), SchemaLocation.GetWindowsFabricSchemaLocation());
            Assert.AreEqual(Utility.RemoveRandomGuid(Utility.GetClusterManifestXMLString(generatedClusterManifest)), Utility.RemoveRandomGuid(Utility.GetClusterManifestXMLString(expectedClusterManifest)));
        }

        internal void InternalUpdateCertificateTest(string v1clusterConfigPath, string v2clusterConfigPath, string[] targetClusterManifests)
        {
            MockupJsonModel jsonConfig = Utility.GetJsonConfigFromFile(Path.Combine(Constant.TestDirectory, v1clusterConfigPath));

            var userConfig = jsonConfig.GetUserConfig();
            var clusterTopology = jsonConfig.GetClusterTopology();
            var adminConfig = new MockupAdminConfig();
            var logger = new MockupTraceLogger();

            List<NodeStatus> nodesStatus = new List<NodeStatus>();
            foreach (NodeDescription node in clusterTopology.Nodes.Values)
            {
                NodeStatus nodeStatus = new NodeStatus()
                {
                    NodeName = node.NodeName,
                    InstanceId = 0,
                    NodeDeactivationIntent = WrpNodeDeactivationIntent.Invalid,
                    NodeState = NodeState.Enabled,
                    NodeType = node.NodeTypeRef
                };

                nodesStatus.Add(nodeStatus);
            }

            MockupCluster clusterResource = new MockupCluster(adminConfig, userConfig, clusterTopology, logger);
            MockupClusterManifestBuilder v1mockupClusterManifestBuilder = new MockupClusterManifestBuilder(
                                                                            clusterTopology,
                                                                            clusterResource.SeedNodeSelector,
                                                                            userConfig,
                                                                            adminConfig,
                                                                            new ClusterNodeConfig(nodesStatus, 2),
                                                                            new MockupManifestVersionGenerator(),
                                                                            clusterResource.FabricSettingsActivator,
                                                                            clusterResource.ClusterManifestGeneratorSettings,
                                                                            new MockupTraceLogger());

            MockupJsonModel v2JsonConfig = Utility.GetJsonConfigFromFile(Path.Combine(Constant.TestDirectory, v2clusterConfigPath));
            var v2generatedClusterManifests = v1mockupClusterManifestBuilder.UpdateCertificateInClusterManifest(v1mockupClusterManifestBuilder.GenerateClusterManifest(), v2JsonConfig.Properties.Security, new MockupAdminConfig().GetFabricSettingsMetadata());

            XmlSerializer xmlSerializer = new XmlSerializer(typeof(ClusterManifestType));

            int i = 0;
            foreach (string path in targetClusterManifests)
            {
                StringWriter generatedClusterManifestWriter = new StringWriter();
                xmlSerializer.Serialize(generatedClusterManifestWriter, v2generatedClusterManifests[i++]);
                string generatedClusterManifestString = generatedClusterManifestWriter.ToString();

                var targetClusterManifest = System.Fabric.Interop.Utility.ReadXml<ClusterManifestType>(Path.Combine(Constant.TestDirectory, path), SchemaLocation.GetWindowsFabricSchemaLocation());
                StringWriter targetClusterManifestWriter = new StringWriter();
                xmlSerializer.Serialize(targetClusterManifestWriter, targetClusterManifest);
                string targetClusterManifestString = targetClusterManifestWriter.ToString();

                Assert.AreEqual(Utility.RemoveRandomGuid(generatedClusterManifestString), Utility.RemoveRandomGuid(targetClusterManifestString));
            }    
        }

        internal void InternalUpdateClusterManifestTest(string v1clusterConfigPath, string v2clusterConfigPath, string targetClusterManifestPath)
        {
            MockupJsonModel jsonConfig = Utility.GetJsonConfigFromFile(Path.Combine(Constant.TestDirectory, v1clusterConfigPath));

            var userConfig = jsonConfig.GetUserConfig();
            var clusterTopology = jsonConfig.GetClusterTopology();
            var adminConfig = new MockupAdminConfig();
            var logger = new MockupTraceLogger();

            List<NodeStatus> nodesStatus = new List<NodeStatus>();
            foreach (NodeDescription node in clusterTopology.Nodes.Values)
            {
                NodeStatus nodeStatus = new NodeStatus()
                {
                    NodeName = node.NodeName,
                    InstanceId = 0,
                    NodeDeactivationIntent = WrpNodeDeactivationIntent.Invalid,
                    NodeState = NodeState.Enabled,
                    NodeType = node.NodeTypeRef
                };

                nodesStatus.Add(nodeStatus);
            }

            MockupCluster clusterResource = new MockupCluster(adminConfig, userConfig, clusterTopology, logger);
            MockupClusterManifestBuilder v1mockupClusterManifestBuilder = new MockupClusterManifestBuilder(
                                                                            clusterTopology,
                                                                            clusterResource.SeedNodeSelector,
                                                                            userConfig,
                                                                            adminConfig,
                                                                            new ClusterNodeConfig(nodesStatus, 2),
                                                                            new MockupManifestVersionGenerator(),
                                                                            clusterResource.FabricSettingsActivator,
                                                                            clusterResource.ClusterManifestGeneratorSettings,
                                                                            new MockupTraceLogger());

            MockupJsonModel v2JsonConfig = Utility.GetJsonConfigFromFile(Path.Combine(Constant.TestDirectory, v2clusterConfigPath));
            var v2generatedClusterManifest = v1mockupClusterManifestBuilder.UpdateClusterManifest(v1mockupClusterManifestBuilder.GenerateClusterManifest(), new MockupAdminConfig().GetFabricSettingsMetadata());

            XmlSerializer xmlSerializer = new XmlSerializer(typeof(ClusterManifestType));

            StringWriter generatedClusterManifestWriter = new StringWriter();
            xmlSerializer.Serialize(generatedClusterManifestWriter, v2generatedClusterManifest);
            string generatedClusterManifestString = generatedClusterManifestWriter.ToString();

            var targetClusterManifest = System.Fabric.Interop.Utility.ReadXml<ClusterManifestType>(Path.Combine(Constant.TestDirectory, targetClusterManifestPath), SchemaLocation.GetWindowsFabricSchemaLocation());
            StringWriter targetClusterManifestWriter = new StringWriter();
            xmlSerializer.Serialize(targetClusterManifestWriter, targetClusterManifest);
            string targetClusterManifestString = targetClusterManifestWriter.ToString();

            Assert.AreEqual(Utility.RemoveRandomGuid(generatedClusterManifestString), Utility.RemoveRandomGuid(targetClusterManifestString));
        }
    }
}
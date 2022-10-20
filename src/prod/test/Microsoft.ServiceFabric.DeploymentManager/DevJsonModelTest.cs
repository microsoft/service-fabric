// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Test
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Management.ServiceModel;
    using System.IO;
    using System.Linq;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Microsoft.ServiceFabric.DeploymentManager;
    using Microsoft.ServiceFabric.DeploymentManager.Common;
    using Microsoft.ServiceFabric.DeploymentManager.Model;
    using Newtonsoft.Json;
    using System.Reflection;

    [TestClass]
    public class DevJsonModelTest
    {
        static readonly string JsonConfigUnsecureDev5NodeFilename = "ClusterConfig.Unsecure.DevCluster.Dev5Nodetemplate.json";        
        static readonly string JsonConfigUnsecureDev1NodeFilename = "ClusterConfig.Unsecure.DevCluster.Dev1Nodetemplate.json";
        static readonly string JsonConfigX509SecureDev5NodeFilename = "ClusterConfig.X509.DevCluster.Dev5Nodetemplate.json";
        static readonly string JsonConfigX509SecureDev1NodeFilename = "ClusterConfig.X509.DevCluster.Dev1Nodetemplate.json";
        static readonly string JsonConfigUnsecureDev1NodeOverrideFilename = "ClusterConfig.Unsecure.DevCluster.Dev1NodetemplateOverride.json";

        private static List<string> sectionsWhiteList = new List<string>
        {
            StringConstants.SectionName.Security,
            StringConstants.SectionName.FailoverManager,
            StringConstants.SectionName.ReconfigurationAgent,
            StringConstants.SectionName.ClusterManager,
            StringConstants.SectionName.NamingService,
            StringConstants.SectionName.Management,
            StringConstants.SectionName.Hosting,
            StringConstants.SectionName.HttpGateway,
            StringConstants.SectionName.PlacementAndLoadBalancing,
            StringConstants.SectionName.Federation,
            StringConstants.SectionName.HttpApplicationGateway,
            StringConstants.SectionName.FaultAnalysisService,
            StringConstants.SectionName.TraceEtw,
            StringConstants.SectionName.Diagnostics,
            StringConstants.SectionName.TransactionalReplicator,
            StringConstants.SectionName.FabricClient,
            StringConstants.SectionName.Failover,
            StringConstants.SectionName.ImageStoreService,
            StringConstants.SectionName.DnsService,
            StringConstants.SectionName.Setup
        };

        private static string BaseDir = new FileInfo(Assembly.GetExecutingAssembly().Location).DirectoryName;

        internal static bool IsSectionAllowed(int nodeCount, string settingName)
        {
            switch (nodeCount)
            {
                case 5:
                    {
                        return sectionsWhiteList.Contains(settingName) || settingName.StartsWith("ServiceFabric");
                    }

                case 1:
                    {
                        return (sectionsWhiteList.Contains(settingName) && settingName != StringConstants.SectionName.FaultAnalysisService) || settingName.StartsWith("ServiceFabric");
                    }

                default:
                    {
                        throw new ValidationException(ClusterManagementErrorCode.DevClusterSizeNotSupported, nodeCount);
                    }
            }
        }

        internal void ConvertDevJsonToXml_Utility(int expectedReplicaSize, string inputFile, string outputFile, string sectionToChangeWin7, bool isSecure = false, bool testOverride = false)
        {
            Console.WriteLine("Current directory: {0}", Environment.CurrentDirectory);
            string clusterConfigPath = Path.Combine(DevJsonModelTest.BaseDir, inputFile);
            Directory.SetCurrentDirectory(DevJsonModelTest.BaseDir);
            Console.WriteLine("New current directory changed to: {0}", Environment.CurrentDirectory);
            string clusterManifestPath = Path.Combine(Path.GetTempPath(), outputFile);
            var clusterResource = DeploymentManagerInternal.GetClusterResource(clusterConfigPath, System.Guid.NewGuid().ToString());
            var cm = clusterResource.Current.ExternalState.ClusterManifest;
            Assert.IsNotNull(cm, "Cluster manifest was null.");

            File.Delete(clusterManifestPath);
            XMLHelper.WriteXmlExclusive<ClusterManifestType>(clusterManifestPath, cm);
            List<string> machines = StandaloneUtility.GetMachineNamesFromClusterManifest(clusterManifestPath);
            Assert.IsNotNull(machines);

            Assert.AreEqual(expectedReplicaSize, ((WindowsInfrastructureType)cm.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());

            if (isSecure)
            {
                Assert.AreEqual(InputEndpointTypeProtocol.https, cm.NodeTypes[0].Endpoints.HttpGatewayEndpoint.Protocol);
                Assert.IsNotNull(cm.NodeTypes[0].Certificates.ClientCertificate, "Client certificate was null.");
            }

            foreach (SettingsOverridesTypeSection setting in cm.FabricSettings)
            {
                Assert.IsTrue(IsSectionAllowed(cm.NodeTypes.Count(), setting.Name));
                if (setting.Name == StringConstants.SectionName.Setup)
                {
                    foreach (SettingsOverridesTypeSectionParameter param in setting.Parameter)
                    {
                        Assert.AreNotEqual(StringConstants.ParameterName.IsDevCluster, param.Name);
                        if (param.Name == StringConstants.ParameterName.FabricDataRoot)
                        {
                            Assert.AreEqual(Environment.ExpandEnvironmentVariables("%systemdrive%\\SfDevCluster\\Data"), param.Value);
                        }
                    }
                }
                if (setting.Name == StringConstants.SectionName.ClusterManager)
                {
                    SettingsOverridesTypeSectionParameter TargetReplicaSetSizeParam = setting.Parameter.FirstOrDefault(x => x.Name == StringConstants.ParameterName.TargetReplicaSetSize);
                    Assert.IsNotNull(TargetReplicaSetSizeParam, "Cannot find TargetReplicaSetSize for Cluster Manager");
                    Assert.AreEqual(expectedReplicaSize.ToString(), TargetReplicaSetSizeParam.Value);
                }
                if (DevJsonModel.IsWin7() && setting.Name == sectionToChangeWin7)
                {
                    SettingsOverridesTypeSectionParameter isEnabledParam = setting.Parameter.FirstOrDefault(x => x.Name == StringConstants.ParameterName.IsEnabled);
                    Assert.IsNotNull(isEnabledParam, "Cannot find isEnabled for " + sectionToChangeWin7);
                    Assert.AreEqual("false", isEnabledParam.Value);
                }
                if (setting.Name == StringConstants.SectionName.Diagnostics && testOverride)
                {
                    SettingsOverridesTypeSectionParameter isEnabledTraceSessionParam = setting.Parameter.FirstOrDefault(x => x.Name == StringConstants.ParameterName.EnableCircularTraceSession);
                    Assert.IsNotNull(isEnabledTraceSessionParam, "Cannot find isEnabled for EnableCircularTraceSession");
                    Assert.AreEqual("false", isEnabledTraceSessionParam.Value);
                }
                if (setting.Name == StringConstants.SectionName.FailoverManager && expectedReplicaSize == 1)
                {
                    SettingsOverridesTypeSectionParameter IsSingletonReplicaMoveAllowedDuringUpgradeParam = setting.Parameter.FirstOrDefault(x => x.Name == StringConstants.ParameterName.IsSingletonReplicaMoveAllowedDuringUpgrade);
                    Assert.IsNotNull(IsSingletonReplicaMoveAllowedDuringUpgradeParam, "Cannot find isEnabled for EnableCircularTraceSession");
                    Assert.AreEqual("false", IsSingletonReplicaMoveAllowedDuringUpgradeParam.Value);
                }
            }           

            //DevCluster should have one distinct IPaddress, i.e. localhost
            Assert.AreEqual(1, machines.Count);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("taxi")]
        public void DeploymentManagerTest_ConvertUnSecure5NodeDevJsonToXml()
        {
            ConvertDevJsonToXml_Utility(3, JsonConfigUnsecureDev5NodeFilename, "cmtest_unsecure5node.xml", StringConstants.SectionName.HttpApplicationGateway);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("taxi")]
        public void DeploymentManagerTest_ConvertUnSecure1NodeDevJsonToXml()
        {
            ConvertDevJsonToXml_Utility(1, JsonConfigUnsecureDev1NodeFilename, "cmtest_unsecure1node.xml", StringConstants.SectionName.HttpApplicationGateway);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("taxi")]
        public void DeploymentManagerTest_ConvertSecure5NodeDevJsonToXml()
        {
            ConvertDevJsonToXml_Utility(3, JsonConfigX509SecureDev5NodeFilename, "cmtest_secure5node.xml", StringConstants.SectionName.HttpApplicationGateway, true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("taxi")]
        public void DeploymentManagerTest_ConvertSecure1NodeDevJsonToXml()
        {
            ConvertDevJsonToXml_Utility(1, JsonConfigX509SecureDev1NodeFilename, "cmtest_secure1node.xml", StringConstants.SectionName.HttpGateway, true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("taxi")]
        public void DeploymentManagerTest_TestOverrideFromUser()
        {
            ConvertDevJsonToXml_Utility(1, JsonConfigUnsecureDev1NodeOverrideFilename, "cmtest_unsecure1nodeoverride.xml", StringConstants.SectionName.HttpApplicationGateway, false, true);
        }
    }
}
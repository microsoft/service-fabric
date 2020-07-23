// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using Management.Test;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.Win32;
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Diagnostics.Tracing;
    using System.Fabric.Common;
    using System.Fabric.Common.ImageModel;
    using System.Fabric.Management.FabricDeployer;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Threading;
    using System.Xml;
    using System.Xml.Serialization;
    using WEX.TestExecution;

    /// <summary>
    /// This is the class that tests the validation of the files that we deploy.
    /// </summary>
    [TestClass]
    public class FabricDeployerDeploymentTests
    {
        [TestCleanup]
        public void Cleanup()
        {
            var exitCode = ExecuteProcess("logman.exe", "stop FabricCounters");
            Assert.IsTrue(exitCode == 0 || exitCode == -2144337918);
            exitCode = ExecuteProcess("logman.exe", "delete FabricCounters");
            Assert.IsTrue(exitCode == 0 || exitCode == -2144337918);
            TestFirewallManiuplator.RemoveWinFabRules();
            //exitCode = ExecuteProcess("sc.exe", "delete FabricHostSvc");
            //Assert.IsTrue(exitCode == 0 || exitCode == 1060);
            try
            {
                using (RegistryKey key = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, RegistryView.Default).OpenSubKey(FabricConstants.FabricRegistryKeyPath, true))
                {
                    key.DeleteValue("FabricDataRoot");
                    key.DeleteValue("FabricLogRoot");
                }
            }
            catch (Exception) { }
            exitCode = ExecuteProcess("net.exe", "user testConfigureNode /DELETE");
            Assert.IsTrue(exitCode == 0 || exitCode == 2);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void CreateLogicalDirectoriesTest()
        {
            string testName = System.Reflection.MethodBase.GetCurrentMethod().Name;
            Initialize(testName);
            TestData data = data = new TestData(false, 2, true, InfrastructureType.WindowsServer, new int[2] { 3, 3 },
                new int[2] { 3, 0 }, new int[2] { 3, 3 }, new int[2] { 3, 3 }, testName, "2.0", "200", "200", "200", DropLocation, "NodeType0.Node.1", true, false, DeploymentOperations.Configure, false, true);
            Deploy(data);
            Assert.IsTrue(Directory.Exists(TestUtility.LogicalDirectoriesLogDir));
            Assert.IsTrue(Directory.Exists(TestUtility.LogicalDirectoriesBackupDir));
            Assert.IsTrue(Directory.Exists(TestUtility.LogicalDirectoriesApplicationCheckPointFilesDir));
            Assert.IsTrue(Directory.Exists(TestUtility.LogicalDirectoriesUserDefined1Dir));
            Assert.IsTrue(Directory.Exists(TestUtility.LogicalDirectoriesUserDefined2Dir));
            Assert.IsTrue(Directory.Exists(TestUtility.LogicalDirectoriesLogDir2));
            Assert.IsTrue(Directory.Exists(TestUtility.LogicalDirectoriesBackupDir2));
            Assert.IsTrue(Directory.Exists(TestUtility.LogicalDirectoriesApplicationCheckPointFilesDir2));
            Assert.IsTrue(Directory.Exists(TestUtility.LogicalDirectoriesUserDefined1Dir2));
            Assert.IsTrue(Directory.Exists(TestUtility.LogicalDirectoriesUserDefined2Dir2));

            data = data = new TestData(false, 2, true, InfrastructureType.WindowsServer, new int[2] { 3, 3 },
                new int[2] { 3, 0 }, new int[2] { 3, 3 }, new int[2] { 3, 3 }, testName, "2.0", "200", "200", "200", DropLocation, "NodeType0.Node.1", true, false, DeploymentOperations.RemoveNodeConfig, false, true);
            Deploy(data);
            Assert.IsFalse(Directory.Exists(TestUtility.LogicalDirectoriesLogDir));
            Assert.IsFalse(Directory.Exists(TestUtility.LogicalDirectoriesBackupDir));
            Assert.IsFalse(Directory.Exists(TestUtility.LogicalDirectoriesApplicationCheckPointFilesDir));
            Assert.IsFalse(Directory.Exists(TestUtility.LogicalDirectoriesUserDefined1Dir));
            Assert.IsFalse(Directory.Exists(TestUtility.LogicalDirectoriesUserDefined2Dir));
            Assert.IsFalse(Directory.Exists(TestUtility.LogicalDirectoriesLogDir2));
            Assert.IsFalse(Directory.Exists(TestUtility.LogicalDirectoriesBackupDir2));
            Assert.IsFalse(Directory.Exists(TestUtility.LogicalDirectoriesApplicationCheckPointFilesDir2));
            Assert.IsFalse(Directory.Exists(TestUtility.LogicalDirectoriesUserDefined1Dir2));
            Assert.IsFalse(Directory.Exists(TestUtility.LogicalDirectoriesUserDefined2Dir2));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void CreateScaleMinOneNode()
        {
            string testName = System.Reflection.MethodBase.GetCurrentMethod().Name;
            Initialize(testName);
            TestData data = new TestData(false, 1, true, InfrastructureType.WindowsServer,
                new int[1] { 1 }, new int[1] { 1 }, new int[] { 1 }, new int[1] { 1 },
                testName, "1.0", "0", "100", "100", DropLocation, null, true, true, DeploymentOperations.Create);
            Deploy(data);
            DeploymentValidator.ValidateDeployment(data, true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void CreateNonScaleMinOneNode()
        {
            string testName = System.Reflection.MethodBase.GetCurrentMethod().Name;
            Initialize(testName);
            TestData data = new TestData(false, 1, false, InfrastructureType.WindowsServer, new int[1] { 1 },
                new int[1] { 1 }, new int[1] { 1 }, new int[1] { 1 }, testName, "1.0", "0", "100", "100", DropLocation, null,
                true, false, DeploymentOperations.Create);
            Deploy(data);
            DeploymentValidator.ValidateDeployment(data, false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void UpdateNodeStateScaleMinOneNode()
        {
            string testName = System.Reflection.MethodBase.GetCurrentMethod().Name;
            Initialize(testName);
            TestData data = new TestData(false, 1, true, InfrastructureType.WindowsServer,
                new int[1] { 1 }, new int[1] { 1 }, new int[] { 1 }, new int[1] { 1 },
                testName, "1.0", "0", "100", "100", DropLocation, null, true, true, DeploymentOperations.Create);
            Deploy(data);
            DeploymentValidator.ValidateDeployment(data, true);
            data = new TestData(false, 1, true, InfrastructureType.WindowsServer,
                new int[1] { 1 }, new int[1] { 1 }, new int[] { 1 }, new int[1] { 1 },
                testName, "1.0", "0", "100", "100", DropLocation, null, true, true, DeploymentOperations.UpdateNodeState);
            Deploy(data);
            DeploymentValidator.ValidateDeployment(data, true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void UpdateNodeStateNonScaleMinOneNode()
        {
            string testName = System.Reflection.MethodBase.GetCurrentMethod().Name;
            Initialize(testName);
            TestData data = new TestData(false, 1, false, InfrastructureType.WindowsServer, new int[1] { 1 },
                new int[1] { 1 }, new int[1] { 1 }, new int[1] { 1 }, testName, "1.0", "0", "100", "100", DropLocation, null,
                true, false, DeploymentOperations.Create);
            Deploy(data);
            DeploymentValidator.ValidateDeployment(data, false);
            data = new TestData(false, 1, false, InfrastructureType.WindowsServer, new int[1] { 1 },
                new int[1] { 1 }, new int[1] { 1 }, new int[1] { 1 }, testName, "1.0", "0", "100", "100", DropLocation, null,
                true, false, DeploymentOperations.UpdateNodeState);
            Deploy(data);
            DeploymentValidator.ValidateDeployment(data, false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [TestProperty("ContainerFeatureRequired", "true")]
        public void CreateNonScaleMinOneNodeAndVerifyDNSFirewallRule()
        {
            string testName = System.Reflection.MethodBase.GetCurrentMethod().Name;
            Initialize(testName);

            List<TestFirewallRule> startRules = TestFirewallManiuplator.GetRules();

            // Create node
            TestData data = new TestData(false, 1, false, InfrastructureType.WindowsServer, new int[1] { 1 },
                new int[1] { 1 }, new int[1] { 1 }, new int[1] { 1 }, testName, "1.0", "0", "100", "100", DropLocation, null,
                true, false, DeploymentOperations.Create);
            Deploy(data);
            DeploymentValidator.ValidateDeployment(data, false);

            List<TestFirewallRule> newRules = TestFirewallManiuplator.GetRules();

            VerifyDNSRule(newRules, startRules, 1);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [TestProperty("ContainerFeatureRequired", "true")]
        public void CreateNonScaleMinOneNodeFollowedByRestartAndVerifyDNSFirewallRule()
        {
            string testName = System.Reflection.MethodBase.GetCurrentMethod().Name;
            Initialize(testName);

            List<TestFirewallRule> startRules = TestFirewallManiuplator.GetRules();

            // Create node
            TestData data = new TestData(false, 1, false, InfrastructureType.WindowsServer, new int[1] { 1 },
                new int[1] { 1 }, new int[1] { 1 }, new int[1] { 1 }, testName, "1.0", "0", "100", "100", DropLocation, null,
                true, false, DeploymentOperations.Create);
            Deploy(data);
            DeploymentValidator.ValidateDeployment(data, false);

            // Restart node
            data.Operation = DeploymentOperations.None;
            Deploy(data);
            DeploymentValidator.ValidateDeployment(data, false);

            List<TestFirewallRule> newRules = TestFirewallManiuplator.GetRules();

            VerifyDNSRule(newRules, startRules, 1);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [TestProperty("ContainerFeatureRequired", "true")]
        public void CreateNonScaleMinOneNodeFollowedByRemoveAndVerifyDNSFirewallRule()
        {
            string testName = System.Reflection.MethodBase.GetCurrentMethod().Name;
            Initialize(testName);

            List<TestFirewallRule> startRules = TestFirewallManiuplator.GetRules();

            // Create node
            TestData data = new TestData(false, 1, false, InfrastructureType.WindowsServer, new int[1] { 1 },
                new int[1] { 1 }, new int[1] { 1 }, new int[1] { 1 }, testName, "1.0", "0", "100", "100", DropLocation, null,
                true, false, DeploymentOperations.Create);
            Deploy(data);
            DeploymentValidator.ValidateDeployment(data, false);

            // Remove node
            data.Operation = DeploymentOperations.Remove;
            Deploy(data);
            DeploymentValidator.ValidateDeployment(data, false);

            List<TestFirewallRule> newRules = TestFirewallManiuplator.GetRules();

            VerifyDNSRule(newRules, startRules, 0);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [TestProperty("ContainerFeatureRequired", "true")]
        public void CreateNonScaleMinOneNodeWithDnsSetupDisallowedAndVerifyDNSFirewallRule()
        {
            string testName = System.Reflection.MethodBase.GetCurrentMethod().Name;
            Initialize(testName);

            List<TestFirewallRule> startRules = TestFirewallManiuplator.GetRules();

            // Create node
            TestData data = new TestData(false, 1, false, InfrastructureType.WindowsServer, new int[1] { 1 },
                new int[1] { 1 }, new int[1] { 1 }, new int[1] { 1 }, testName, "1.0", "0", "100", "100", DropLocation, null,
                true, false, DeploymentOperations.Create, true, false, true);
            Deploy(data);
            DeploymentValidator.ValidateDeployment(data, false);

            List<TestFirewallRule> newRules = TestFirewallManiuplator.GetRules();

            VerifyDNSRule(newRules, startRules, 0);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void AzureCreate()
        {
            string testName = System.Reflection.MethodBase.GetCurrentMethod().Name;
            Initialize(testName);
            TestData data = new TestData(false, 1, false, InfrastructureType.WindowsAzure,
                new int[1] { 5 }, new int[1] { 3 }, new int[] { 2 }, new int[1] { 3 },
                testName, "1.0", "0", "100", "100", DropLocation, null, true, true, DeploymentOperations.Create);
            Deploy(data);
            DeploymentValidator.ValidateDeployment(data, false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void AzureUpgrade()
        {
            string testName = System.Reflection.MethodBase.GetCurrentMethod().Name;
            Initialize(testName);
            TestData data = new TestData(false, 1, false, InfrastructureType.WindowsAzure,
                new int[1] { 5 }, new int[1] { 3 }, new int[] { 2 }, new int[1] { 3 },
                testName, "1.0", "0", "100", "100", DropLocation, null, true, true, DeploymentOperations.Create);
            Deploy(data);
            DeploymentValidator.ValidateDeployment(data, false);
            data = new TestData(false, 1, false, InfrastructureType.WindowsAzure,
                new int[1] { 5 }, new int[1] { 3 }, new int[] { 2 }, new int[1] { 3 },
                testName, "2.0", "200", "200", "200", DropLocation, null, true, true, DeploymentOperations.Update);
            Deploy(data);
            DeploymentValidator.ValidateDeployment(data, false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void AzureUpgradeSeedNodeCountIncrementByOne()
        {
            string testName = System.Reflection.MethodBase.GetCurrentMethod().Name;
            Initialize(testName);
            TestData data = new TestData(false, 1, false, InfrastructureType.WindowsAzure,
                new int[1] { 5 }, new int[1] { 3 }, new int[] { 2 }, new int[1] { 3 },
                testName, "1.0", "0", "100", "100", DropLocation, null, true, true, DeploymentOperations.Create);
            Deploy(data);
            DeploymentValidator.ValidateDeployment(data, false);
            data = new TestData(false, 1, false, InfrastructureType.WindowsAzure,
                new int[1] { 5 }, new int[1] { 4 }, new int[] { 2 }, new int[1] { 3 },
                testName, "2.0", "200", "200", "200", DropLocation, null, true, true, DeploymentOperations.Update);
            Deploy(data);
            DeploymentValidator.ValidateDeployment(data, false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void UpgradeOfOneNodeWorksForScaleMin()
        {
            string testName = System.Reflection.MethodBase.GetCurrentMethod().Name;
            Initialize(testName);
            TestData data = new TestData(false, 2, true, InfrastructureType.WindowsServer, new int[2] { 3, 3 },
                new int[2] { 3, 0 }, new int[2] { 3, 3 }, new int[2] { 3, 3 }, testName, "1.0", "0", "100", "100", DropLocation, null, true, false, DeploymentOperations.Create);
            Deploy(data);
            DeploymentValidator.ValidateDeployment(data, true);
            data = new TestData(false, 2, true, InfrastructureType.WindowsServer, new int[2] { 3, 3 },
                new int[2] { 3, 0 }, new int[2] { 3, 3 }, new int[2] { 3, 3 }, testName, "2.0", "200", "200", "200", DropLocation, "NodeType0.Node.1", true, false, DeploymentOperations.Update);
            Deploy(data);
            DeploymentValidator.ValidateDeployment(data, true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void PaaSCreate()
        {
            string testName = System.Reflection.MethodBase.GetCurrentMethod().Name;
            Initialize(testName);
            TestData data = new TestData(false, 1, false, InfrastructureType.PaaS,
                new int[1] { 5 }, new int[1] { 3 }, new int[] { 2 }, new int[1] { 3 },
                testName, "1.0", "0", "100", "100", DropLocation, null, true, true, DeploymentOperations.Create);
            Deploy(data);
            DeploymentValidator.ValidateDeployment(data, false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ServerInfrastructureMergeNodesFromCMandIM()
        {
            string fabunittest = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests");
            var clusterManifestLocation = Path.Combine(fabunittest, "ClusterManifest.MultiMachine.xml");
            var infrastructureManifestLocation = Path.Combine(fabunittest, "InfrastructureManifest.MultiMachine.xml");

            var infrastructureManifest = XmlHelper.ReadXml<InfrastructureInformationType>(infrastructureManifestLocation, SchemaLocation.GetWindowsFabricSchemaLocation());
            var clusterManifest = XmlHelper.ReadXml<ClusterManifestType>(clusterManifestLocation, SchemaLocation.GetWindowsFabricSchemaLocation());

            Infrastructure serverInfrastructure = Infrastructure.Create(clusterManifest.Infrastructure, infrastructureManifest.NodeList, clusterManifest.NodeTypes);
            Assert.AreEqual(serverInfrastructure.InfrastructureNodes.Count, 4);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ServerInfrastructureMergeNodesFromCMandIMLocalhost()
        {
            string fabunittest = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests");
            var clusterManifestLocation = Path.Combine(fabunittest, "ClusterManifest.MultiMachine-LocalHost.xml");
            var infrastructureManifestLocation = Path.Combine(fabunittest, "InfrastructureManifest.MultiMachine-LocalHost.xml");

            var infrastructureManifest = XmlHelper.ReadXml<InfrastructureInformationType>(infrastructureManifestLocation, SchemaLocation.GetWindowsFabricSchemaLocation());
            var clusterManifest = XmlHelper.ReadXml<ClusterManifestType>(clusterManifestLocation, SchemaLocation.GetWindowsFabricSchemaLocation());

            Infrastructure serverInfrastructure = Infrastructure.Create(clusterManifest.Infrastructure, infrastructureManifest.NodeList, clusterManifest.NodeTypes);
            Assert.AreEqual(serverInfrastructure.InfrastructureNodes.Count, 5);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void UpgradeOfOneNodeWorksForNonScaleMin()
        {
            string testName = System.Reflection.MethodBase.GetCurrentMethod().Name;
            Initialize(testName);
            TestData data = new TestData(false, 1, false, InfrastructureType.WindowsServer, new int[1] { 1 },
                new int[1] { 1 }, new int[1] { 1 }, new int[1] { 1 }, testName, "1.0", "0", "100", "100", DropLocation, null, true, false, DeploymentOperations.Create);
            Deploy(data);
            DeploymentValidator.ValidateDeployment(data, false);
            data = new TestData(false, 1, false, InfrastructureType.WindowsServer, new int[1] { 1 },
                new int[1] { 1 }, new int[1] { 1 }, new int[1] { 1 }, testName, "2.0", "200", "200", "200", DropLocation, "NodeType0.Node.0", true, false, DeploymentOperations.Update);
            Deploy(data);
            DeploymentValidator.ValidateDeployment(data, false);
        }

        //[TestMethod]
        //[TestProperty("ThreadingModel", "MTA")]
        //public void ConfigureNode()
        //{
        //    string testName = System.Reflection.MethodBase.GetCurrentMethod().Name;
        //    Initialize(testName);
        //    TestData data = new TestData(false, 1, false, InfrastructureType.WindowsServer, new int[1] { 1 }, new int[1] { 1 }, new int[1] { 1 }, new int[1] { 1 }, testName, "1.0",
        //        "", "", "", DropLocation, "1", false, false, DeploymentOperations.Configure, true);
        //    int exitCode = ExecuteProcess("sc.exe", string.Format("create FabricHostSvc binPath= {0}", Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), @"FabricUnitTests\FabricHost.exe")));
        //    Assert.IsTrue(exitCode == 0 || exitCode == 1073);
        //    Deploy(data);
        //    ValidateConfiguration(data, true);
        //}

        //[TestMethod]
        //[TestProperty("ThreadingModel", "MTA")]
        //public void ConfigureNodeFromCommandLineWithDataRootOnly()
        //{
        //    string testName = System.Reflection.MethodBase.GetCurrentMethod().Name;
        //    Initialize(testName);
        //    TestData data = new TestData(false, 1, false, InfrastructureType.WindowsServer, new int[1] { 1 }, new int[1] { 1 }, new int[1] { 1 }, new int[1] { 1 }, testName, "1.0",
        //        "", "", "", DropLocation, "1", false, false, DeploymentOperations.Configure, false);
        //    int exitCode = ExecuteProcess("sc.exe", string.Format("create FabricHostSvc binPath= {0}", Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), @"FabricUnitTests\FabricHost.exe")));
        //    Assert.IsTrue(exitCode == 0 || exitCode == 1073);
        //    Deploy(data);
        //    ValidateConfiguration(data, false);
        //}

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void Remove()
        {
            string testName = System.Reflection.MethodBase.GetCurrentMethod().Name;
            Initialize(testName);
            TestData data = new TestData(false, 1, false, InfrastructureType.WindowsServer, new int[1] { 1 },
                new int[1] { 1 }, new int[1] { 1 }, new int[1] { 1 }, testName, "1.0", "0", "100", "100", DropLocation, null,
                true, false, DeploymentOperations.Create);
            data.DataRoot = Path.Combine(data.DataRoot, "TestDataRoot");
            Deploy(data);
            DeploymentValidator.ValidateDeployment(data, false);
            data.Operation = DeploymentOperations.Remove;
            Deploy(data);
            //DeploymentValidator.ValidateDeployment(data, false);
        }

        ////[TestMethod]
        ////[TestProperty("ThreadingModel", "MTA")]
        //public void StartFabricHostServiceWithoutConfigure()
        //{
        //    string testName = System.Reflection.MethodBase.GetCurrentMethod().Name;
        //    Initialize(testName);
        //    int exitCode = ExecuteProcess("sc.exe", string.Format("create FabricHostSvc binPath= {0}", Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), @"FabricUnitTests\FabricHost.exe")));
        //    Assert.IsTrue(exitCode == 0 || exitCode == 1073);
        //    exitCode = ExecuteProcess("sc.exe", "start FabricHostSvc");
        //    Assert.IsTrue(exitCode == 0);
        //    TestData data = new TestData(false, 1, false, InfrastructureType.WindowsServer, new int[1] { 1 },
        //        new int[1] { 1 }, new int[1] { 1 }, new int[1] { 1 }, testName, "1.0", "0", "100", "100", DropLocation, null,
        //        true, false, DeploymentOperations.Remove);
        //    Deploy(data);
        //}

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollbackNoTargetInformationFile()
        {
            string testName = System.Reflection.MethodBase.GetCurrentMethod().Name;
            Initialize(testName);
            TestData data = new TestData(false, 1, false, InfrastructureType.WindowsServer, new int[1] { 1 },
                new int[1] { 1 }, new int[1] { 1 }, new int[1] { 1 }, testName, "1.0", "0", "100", "100", DropLocation, null, true, false, DeploymentOperations.Create);
            Deploy(data);
            DeploymentValidator.ValidateDeployment(data, false);
            data = new TestData(false, 1, false, InfrastructureType.WindowsServer, new int[1] { 1 },
                new int[1] { 1 }, new int[1] { 1 }, new int[1] { 1 }, testName, "2.0", "200", "200", "200", DropLocation, "NodeType0.Node.0", true, false, DeploymentOperations.Update);
            Deploy(data);
            DeploymentValidator.ValidateDeployment(data, false);
            data.Operation = DeploymentOperations.Rollback;
            Deploy(data);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [ExpectedException(typeof(ClusterManifestValidationException))]
        public void UpdateFabricLogAndDataRoot()
        {
            string testName = System.Reflection.MethodBase.GetCurrentMethod().Name;
            Initialize(testName);
            TestData data = new TestData(false, 1, false, InfrastructureType.WindowsServer, new int[1] { 1 }, new int[1] { 1 }, new int[1] { 1 }, new int[1] { 1 }, testName, "1.0", "0", "100", "100", DropLocation, null, true, false, DeploymentOperations.Create, true);
            Deploy(data);
            DeploymentValidator.ValidateDeployment(data, false);
            data = new TestData(false, 1, false, InfrastructureType.WindowsServer, new int[1] { 1 }, new int[1] { 1 }, new int[1] { 1 }, new int[1] { 1 },
                testName, "2.0", "200", "200", "200", DropLocation, "NodeType0.Node.0", true, false, DeploymentOperations.Update, true);
            //Update FabricLogRoot not allowed
            ChangeFabricDataLogRoots(data.ClusterManifest, Constants.ParameterNames.FabricLogRoot);
            Verify.Throws<ClusterManifestValidationException>(
                    () => Deploy(data));
            //Update FabricDataRoot not allowed
            ChangeFabricDataLogRoots(data.ClusterManifest, Constants.ParameterNames.FabricDataRoot);
            Verify.Throws<ClusterManifestValidationException>(
                    () => Deploy(data));
        }

        public void ChangeFabricDataLogRoots(ClusterManifestType clusterManifest, string parameterName)
        {
            var setupSection = clusterManifest.FabricSettings.FirstOrDefault(section => section.Name.Equals(Constants.SectionNames.Setup, StringComparison.OrdinalIgnoreCase));
            if (setupSection != null)
            {
                var parameter = setupSection.Parameter.FirstOrDefault(par => par.Name.Equals(parameterName, StringComparison.OrdinalIgnoreCase));
                if (parameter != null)
                {
                    parameter.Value = Path.Combine(FabricDeployerDeploymentTests.RootLocation, clusterManifest.Description, parameterName, "_2");
                }
            }
        }

        private static void VerifyDNSRule(List<TestFirewallRule> newRules, List<TestFirewallRule> startRules, int expectedCount)
        {
            List<TestFirewallRule> addedRules = new List<TestFirewallRule>(newRules.Except(startRules));
            TestFirewallRule dnsRule = TestFirewallManiuplator.GetServiceFabricDNSRule();

            int count = 0;
            for (int i = 0; i < addedRules.Count; i++)
            {
                if (addedRules[i].CompareTo(dnsRule) == 0)
                {
                    count++;
                }
            }

            Assert.AreEqual(expectedCount, count);
        }

        private void ValidateConfiguration(TestData data, bool explicitSetupConfiguration)
        {
            var expectedDataRoot = data.DataRoot;
            var expectedLogRoot = explicitSetupConfiguration ? Path.Combine(FabricDeployerDeploymentTests.RootLocation, data.TestName, "LogRoot") : Path.Combine(data.DataRoot, "log");
            var actualDataRoot = FabricEnvironment.GetDataRoot();
            var actualLogRoot = FabricEnvironment.GetLogRoot();
            Assert.AreEqual<string>(expectedDataRoot, actualDataRoot);
            Assert.AreEqual<string>(expectedLogRoot, actualLogRoot);
            var actualServiceStartupType = FabricDeployerServiceController.GetServiceStartupType();
            Assert.AreEqual<FabricDeployerServiceController.ServiceStartupType>(
                explicitSetupConfiguration ? FabricDeployerServiceController.ServiceStartupType.Manual : FabricDeployerServiceController.ServiceStartupType.Automatic,
                actualServiceStartupType);
            string targetFilePath = Path.Combine(expectedDataRoot, "TargetInformation.xml");
            Assert.IsTrue(File.Exists(targetFilePath));
            using (var fileStream = File.Open(targetFilePath, FileMode.Open, FileAccess.Read, FileShare.Read))
            {
                using (var xmlReader = XmlReader.Create(fileStream, new XmlReaderSettings() { XmlResolver = null }))
                {
                    var serializer = new XmlSerializer(typeof(TargetInformationType));
                    var targetInformation = (TargetInformationType)serializer.Deserialize(xmlReader);
                    Assert.AreEqual<string>(data.ClusterManifestFileName, targetInformation.TargetInstallation.ClusterManifestLocation);
                }
            }
            string actualRunAsUserName = FabricDeployerServiceController.GetRunAsAccountName();
            Assert.AreEqual<string>(explicitSetupConfiguration ? "NT Authority\\NetworkService" : "LocalSystem", actualRunAsUserName);
        }

        internal static string RootLocation = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests", "log", "FD.Tests");

        private static readonly object SyncRoot = new object();
        private static readonly TimeSpan DeploymentCleanupWaitSeconds = TimeSpan.FromSeconds(60);

        private static void Deploy(TestData data)
        {
            lock (SyncRoot)
            {
                string cmFileName = Path.Combine(RootLocation, data.ClusterManifestFileName);
                XmlHelper.WriteXml<ClusterManifestType>(cmFileName, data.ClusterManifest);
                string imFileName = null;
                if (data.InfrastructureType != InfrastructureType.WindowsServer)
                {
                    imFileName = Path.Combine(RootLocation, data.InfrastructureManifestFileName);
                    XmlHelper.WriteXml<InfrastructureInformationType>(imFileName, data.InfrastructureManifest);
                }
                DeploymentParameters parameters = new DeploymentParameters();

                if (data.Operation == DeploymentOperations.Configure)
                {
                    Environment.SetEnvironmentVariable("FabricDataRoot", "");
                    Environment.SetEnvironmentVariable("FabricLogRoot", "");
                    string fabricCodePackage = null;
                    string fabricAppDirectory = Path.Combine(Directory.GetCurrentDirectory(), DropLocation, @"bin\Fabric");
                    foreach (string dir in Directory.EnumerateDirectories(fabricAppDirectory, "Fabric.Code"))
                    {
                        fabricCodePackage = dir;
                    }

                    string path = Environment.GetEnvironmentVariable("path");
                    path = string.Format(CultureInfo.InvariantCulture, "{0};{1}", path, fabricCodePackage);
                    Environment.SetEnvironmentVariable("path", path);
                    Environment.SetEnvironmentVariable("FabricCodePath", fabricCodePackage);
                    parameters.SetParameters(new Dictionary<string, dynamic>() {
                    {DeploymentParameters.ClusterManifestString, cmFileName},
                    {DeploymentParameters.InfrastructureManifestString, imFileName},
                    {DeploymentParameters.FabricBinRootString, data.BinaryRoot},
                    {DeploymentParameters.FabricDataRootString, data.DataRoot},
                    {DeploymentParameters.InstanceIdString, data.InstanceId},
                    {DeploymentParameters.TargetVersionString, data.TargetVersion},
                    {DeploymentParameters.CurrentVersionString, data.CurrentVersion},
                },
                    data.Operation);
                }
                else
                {
                    Environment.SetEnvironmentVariable("FabricDataRoot", data.DataRoot);
                    Environment.SetEnvironmentVariable("FabricLogRoot", Path.Combine(data.DataRoot, "log"));
                    string fabricCodePackage = null;
                    string fabricAppDirectory = Path.Combine(Directory.GetCurrentDirectory(), DropLocation, @"bin\Fabric");
                    foreach (string dir in Directory.EnumerateDirectories(fabricAppDirectory, "Fabric.Code"))
                    {
                        fabricCodePackage = dir;
                    }

                    string binPath = Path.Combine(Directory.GetCurrentDirectory(), "Drop", "bin");
                    string dataPath = Path.Combine(Directory.GetCurrentDirectory(), "Drop", "Data");
                    string path = Environment.GetEnvironmentVariable("path");
                    Environment.SetEnvironmentVariable("FabricCodePath", fabricCodePackage);
                    path = string.Format(CultureInfo.InvariantCulture, "{0};{1}", path, fabricCodePackage);
                    Environment.SetEnvironmentVariable("path", path);
                    parameters.SetParameters(new Dictionary<string, dynamic>() {
                        {DeploymentParameters.ClusterManifestString, cmFileName},
                        {DeploymentParameters.InfrastructureManifestString, imFileName},
                        {DeploymentParameters.FabricBinRootString, data.BinaryRoot},
                        {DeploymentParameters.InstanceIdString, data.InstanceId},
                        {DeploymentParameters.TargetVersionString, data.TargetVersion},
                        {DeploymentParameters.CurrentVersionString, data.CurrentVersion}
                        },
                    data.Operation);
                }

                DeploymentOperation.ExecuteOperation(parameters);

                Thread.Sleep(DeploymentCleanupWaitSeconds);
            }
        }

        private void Initialize(string testName)
        {
            if (!Directory.Exists(RootLocation))
            {
                Directory.CreateDirectory(RootLocation);
            }

            if (!Directory.Exists(DropLocation))
            {
                string makeDropScriptPath = Environment.ExpandEnvironmentVariables(@"%SRCROOT%\prod\Setup\scripts\MakeDrop.cmd");
                if (File.Exists(makeDropScriptPath))
                {
                    int exitCode = ExecuteProcess(
                        makeDropScriptPath,
                        string.Format(CultureInfo.InvariantCulture, @"/dest:{0} /symbolsDestination:nul", DropLocation));
                    Assert.AreEqual<int>(0, exitCode, "Unable to create Drop");
                }
                else
                {
                    string fabricDropPath = Environment.ExpandEnvironmentVariables(@"%_NTTREE%\FabricDrop");
                    DirectoryCopy(fabricDropPath, DropLocation, true);
                }
            }

            DeployerTrace.UpdateConsoleLevel(EventLevel.Informational);
            string logFileName = Path.Combine(GetTestExecutionDirectory(testName), "FabricDeployer.trace");
            DeployerTrace.UpdateFileLocation(logFileName);
            DeployerTrace.WriteInfo("Test", "Starting test {0}", testName);
        }

        private string GetTestExecutionDirectory(string testName)
        {
            string testDirectory = Path.Combine(RootLocation, testName);
            if (!Directory.Exists(testDirectory))
            {
                Directory.CreateDirectory(testDirectory);
            }
            return testDirectory;
        }

        public static int ExecuteProcess(string fileName, string arguments = null)
        {
            System.Diagnostics.ProcessStartInfo processStartInfo = new System.Diagnostics.ProcessStartInfo()
            {
                FileName = fileName,
                WindowStyle = System.Diagnostics.ProcessWindowStyle.Hidden,
                Arguments = arguments
            };

            System.Diagnostics.Process p = System.Diagnostics.Process.Start(processStartInfo);
            p.WaitForExit();
            return p.ExitCode;
        }

        public static void DirectoryCopy(string sourceDirName, string destDirName, bool copySubDirs)
        {
            DirectoryInfo dir = new DirectoryInfo(sourceDirName);
            DirectoryInfo[] dirs = dir.GetDirectories();

            if (!dir.Exists)
            {
                throw new DirectoryNotFoundException(string.Format(
                    CultureInfo.InvariantCulture,
                    "Source directory {0} does not exist or could not be found",
                    sourceDirName));
            }

            if (!Directory.Exists(destDirName))
            {
                Directory.CreateDirectory(destDirName);
            }

            FileInfo[] files = dir.GetFiles();
            foreach (FileInfo file in files)
            {
                string temppath = Path.Combine(destDirName, file.Name);
                file.CopyTo(temppath, false);
            }

            if (copySubDirs)
            {
                foreach (DirectoryInfo subdir in dirs)
                {
                    string temppath = Path.Combine(destDirName, subdir.Name);
                    DirectoryCopy(subdir.FullName, temppath, copySubDirs);
                }
            }
        }

        private static string DropLocation = "Drop";
        private static string FabricUnitTestLocation = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests");
    }

    internal class TestData
    {
        public bool IsHttpGatewayEnabled { get; private set; }
        public string TestName { get; private set; }
        public string InstanceId { get; private set; }
        public string TargetVersion { get; private set; }
        public string CurrentVersion { get; private set; }
        public ClusterManifestType ClusterManifest { get; private set; }
        public InfrastructureInformationType InfrastructureManifest { get; private set; }
        public string DataRoot { get; set; }
        public string BinaryRoot { get; private set; }
        public string NodeName { get; private set; }
        public bool IsSingleMachine { get; private set; }
        public bool AreCountersEnabeld { get; private set; }
        public string InfrastructureManifestFileName { get; private set; }
        public string ClusterManifestFileName { get; private set; }
        public FabricDeploymentSpecification DeploymentSpecification { get; private set; }
        public InfrastructureType InfrastructureType { get; private set; }
        public SettingsTypeSection[] ClusterSettings { get; private set; }
        public DeploymentOperations Operation { get; set; }

        public TestData(
            bool isHttpGatewayEnabled,
            int nNodeTypes,
            bool isScaleMin,
            InfrastructureType infrastructureType,
            int[] nNodes,
            int[] nSeedNodes,
            int[] nFaultDomains,
            int[] nUpgradeDomains,
            string testName,
            string version,
            string instanceId,
            string targetVersion,
            string currentVersion,
            string dropRoot,
            string nodeName,
            bool isSingleMachine,
            bool areCountersEnabled,
            DeploymentOperations operation,
            bool addSetupSectionToCM = false,
            bool isLogicalDirectories = false,
            bool disAllowDnsSetup = false)
        {
            this.IsHttpGatewayEnabled = isHttpGatewayEnabled;
            this.Operation = operation;
            this.TestName = testName;
            string rootLocation = Path.GetDirectoryName(FabricDeployerDeploymentTests.RootLocation);
            this.InfrastructureManifestFileName = Path.Combine(rootLocation, string.Format("IM-{0}-{1}.xml", testName, operation));
            this.ClusterManifestFileName = Path.Combine(rootLocation, string.Format("CM-{0}-{1}.xml", testName, operation));
            this.InfrastructureType = infrastructureType;
            TestManifests manifests = new TestManifests(
                isHttpGatewayEnabled,
                operation == DeploymentOperations.Update,
                isLogicalDirectories,
                nNodeTypes,
                isScaleMin,
                infrastructureType,
                nNodes,
                nSeedNodes,
                nFaultDomains,
                nUpgradeDomains,
                testName,
                version,
                addSetupSectionToCM,
                disAllowDnsSetup);
            this.ClusterSettings = manifests.ClusterSettings;
            this.ClusterManifest = manifests.ClusterManifest;
            this.InfrastructureManifest = manifests.InfrastructureManifest;
            this.InstanceId = instanceId;
            this.TargetVersion = targetVersion;
            this.CurrentVersion = currentVersion;
            this.DataRoot = Path.Combine(FabricDeployerDeploymentTests.RootLocation, testName);
            this.BinaryRoot = Path.Combine(dropRoot, "bin");
            this.NodeName = nodeName;
            this.IsSingleMachine = isSingleMachine;
            this.AreCountersEnabeld = areCountersEnabled;
            this.DeploymentSpecification = FabricDeploymentSpecification.Create();
            this.DeploymentSpecification.SetDataRoot(DataRoot);
        }
    }

    internal class DeploymentValidator
    {
        public static bool ValidateDeployment(TestData data, bool isScaleMin)
        {
            if (data.Operation == DeploymentOperations.Remove)
            {
                ValidateRemoved(data);
                return true;
            }

            if (data.Operation == DeploymentOperations.Configure)
            {
                ValidateConfiguration(data);
                return true;
            }
            ClusterManifestType manifest = data.ClusterManifest;
            List<FabricNodeType> nodes = new List<FabricNodeType>();
            if (manifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsServer)
            {
                var infra = (ClusterManifestTypeInfrastructureWindowsServer)manifest.Infrastructure.Item;
                nodes.AddRange(infra.NodeList);
            }

            ValidateCodeDeployment(data.DataRoot, nodes, data.IsHttpGatewayEnabled, data.IsSingleMachine, isScaleMin);
            ValidateCounters(data.AreCountersEnabeld, data.DataRoot);
            ValidateConfigDeployment(
                data.ClusterManifest,
                data.DataRoot,
                nodes,
                data.IsHttpGatewayEnabled,
                data.IsSingleMachine,
                data.CurrentVersion,
                data.Operation == DeploymentOperations.Update ? data.InstanceId : "0",
                data.TargetVersion,
                manifest.Version,
                data.Operation == DeploymentOperations.Update ? "1.1" : "1.0",
                data.ClusterSettings);
            VerifyHostSettings(data.DataRoot, data.ClusterSettings);

            if (data.InfrastructureType != InfrastructureType.WindowsServer && data.InfrastructureType != InfrastructureType.PaaS)
            {
                if (data.InfrastructureManifest != null)
                {
                    VerifySeedInfo(data.DataRoot, data.InfrastructureManifest.NodeList);
                }
            }

            // validate container network set up on Azure with Windows 2016 with containers
            if (data.InfrastructureType == InfrastructureType.WindowsAzure ||
                data.InfrastructureType == InfrastructureType.PaaS)
            {
                if (Utility.IsContainersFeaturePresent())
                {
                    ValidateContainerNetworkSetup(data);
                }
            }

            return true;
        }

        private static void ValidateRemoved(TestData data)
        {
            Assert.IsTrue(Directory.Exists(data.DataRoot));

            var directories = Directory.GetDirectories(data.DataRoot);
            if (directories.Length == 1)
            {
                Assert.AreEqual(1, directories.Length, string.Format("The only directory in data root is: {0}", directories[0]));
                Assert.AreEqual(Path.Combine(data.DataRoot, "log"), directories[0]);
            }

            var files = Directory.GetFiles(data.DataRoot);

            if (files.Length == 1)
            {
                Assert.AreEqual(1, files.Length, string.Format("The only file in data root is: {0}", files[0]));
                Assert.AreEqual(Path.Combine(data.DataRoot, "FabricHostSettings.xml"), files[0]);
                //var hostSettings = XmlHelper.ReadXml<SettingsType>(files[0]);
                //Assert.IsNull(hostSettings.Section);
            }
        }

        private static void ValidateConfiguration(TestData data)
        {
            string dataRoot = Path.Combine(FabricDeployerDeploymentTests.RootLocation, data.TestName, "DataRoot");
            string logRoot = Path.Combine(FabricDeployerDeploymentTests.RootLocation, data.TestName, "LogRoot");
            Assert.AreEqual(dataRoot, FabricEnvironment.GetDataRoot());
            Assert.AreEqual(logRoot, FabricEnvironment.GetLogRoot());
            Assert.AreEqual(FabricDeployerServiceController.ServiceStartupType.Manual, FabricDeployerServiceController.GetServiceStartupType());
        }

        private static int SectionComparer(SettingsTypeSection section1, SettingsTypeSection section2)
        {
            return section1.Name.CompareTo(section2.Name);
        }

        private static void VerifyHostSettings(string dataRoot, SettingsTypeSection[] inputSections)
        {
            var fileName = Path.Combine(dataRoot, "FabricHostSettings.xml");
            Assert.IsTrue(File.Exists(fileName));
            var hostSettings = XmlHelper.ReadXml<SettingsType>(fileName);
            CompareSections(inputSections, hostSettings.Section);
        }

        private static void VerifySeedInfo(string dataRoot, InfrastructureNodeType[] expectNodeTypes)
        {
            var fileName = Path.Combine(dataRoot, "SeedInfo.ini");
            Assert.IsTrue(File.Exists(fileName));
            var lines = File.ReadAllLines(fileName);
            var actualSeedNodesCount = lines.Length - 1;
            var expectSeedNodesCount = expectNodeTypes.Count(nodeType => nodeType.IsSeedNode);
            Assert.AreEqual(expectSeedNodesCount, actualSeedNodesCount, "Seed nodes count are supposed to be the same");
        }

        private static void CompareSections(SettingsTypeSection[] expectedSections, SettingsTypeSection[] actualSections)
        {
            Array.Sort(actualSections, SectionComparer);
            Array.Sort(expectedSections, SectionComparer);

            int j = 0;
            for (int i = 0; i < actualSections.Length; i++)
            {
                while (actualSections[i].Name.StartsWith("HostedService/") || actualSections[i].Name.Equals("FabricNode") || actualSections[i].Name.Equals("NodeDomainIds") || actualSections[i].Name.Equals("SeedNodeClientConnectionAddresses"))
                {
                    i++;
                }
                Assert.AreEqual(expectedSections[j].Name, actualSections[i].Name);
                if (actualSections[i].Parameter == null)
                {
                    Assert.IsNull(expectedSections[j].Parameter);
                }
                else
                {
                    Assert.AreEqual(actualSections[i].Parameter.Length, expectedSections[j].Parameter.Length);
                    for (int k = 0; k < actualSections[i].Parameter.Length; k++)
                    {
                        Assert.AreEqual(actualSections[i].Parameter[k].Name, expectedSections[j].Parameter[k].Name);
                        Assert.AreEqual(actualSections[i].Parameter[k].Value, expectedSections[j].Parameter[k].Value);
                    }
                }


                j++;
            }
        }

        private static void ValidateConfigDeployment(
            ClusterManifestType inputManifest,
            string dataRoot,
            List<FabricNodeType> nodes,
            bool isHttpServerEnabled,
            bool isSingleMachine,
            string currentVersion,
            string instanceId,
            string targetVersion,
            string cmVersion,
            string rolloutVersion,
            SettingsTypeSection[] expectedSections)
        {
            foreach (var node in nodes)
            {
                string fabricDir = Path.Combine(dataRoot, node.NodeName, "Fabric");
                string configDir = Path.Combine(fabricDir, string.Format("Fabric.Config.{0}.{1}", cmVersion, instanceId));
                Assert.IsTrue(Directory.Exists(configDir));
                string configFile = Path.Combine(configDir, "Settings.xml");
                Assert.IsTrue(File.Exists(configFile));
                var settings = XmlHelper.ReadXml<SettingsType>(configFile);
                CompareSections(expectedSections, settings.Section);
                string clusterManifestCurrentFile = Path.Combine(fabricDir, "ClusterManifest.current.xml");
                Assert.IsTrue(File.Exists(clusterManifestCurrentFile));
                ClusterManifestType manifest = XmlHelper.ReadXml<ClusterManifestType>(clusterManifestCurrentFile);
                TestManifests.VerifyAreEqual(manifest, inputManifest);
                string clusterManifestVersionedFile = Path.Combine(fabricDir, string.Format("ClusterManifest.{0}.xml", cmVersion));
                Assert.IsTrue(File.Exists(clusterManifestVersionedFile));
                manifest = XmlHelper.ReadXml<ClusterManifestType>(clusterManifestCurrentFile);
                TestManifests.VerifyAreEqual(manifest, inputManifest);
                string cmCurrentDataFile = Path.Combine(fabricDir, "ClusterManifest.current.xml");
                Assert.IsTrue(File.Exists(cmCurrentDataFile));
                manifest = XmlHelper.ReadXml<ClusterManifestType>(clusterManifestCurrentFile);
                TestManifests.VerifyAreEqual(manifest, inputManifest);
                string fabricPackageCurrentFile = Path.Combine(fabricDir, "Fabric.Package.current.xml");
                Assert.IsTrue(File.Exists(fabricPackageCurrentFile));
                string fabricPackageVersionedFile = Path.Combine(fabricDir, string.Format("Fabric.Package.{0}.xml", rolloutVersion));
                Assert.IsTrue(File.Exists(fabricPackageVersionedFile));
            }
        }

        private static void ValidateDataDeployment(string dataRoot, InfrastructureInformationType inputManifest)
        {
            string imRootData = Path.Combine(dataRoot, "InfrastructureManifest.xml");
            Assert.IsTrue(File.Exists(imRootData));
            InfrastructureInformationType manifest = XmlHelper.ReadXml<InfrastructureInformationType>(imRootData);
            TestManifests.VerifyAreEqual(manifest, inputManifest);
            string imDataFile = Path.Combine(dataRoot, "Fabric", "Fabric.Data", "InfrastructureManifest.xml");
            Assert.IsTrue(File.Exists(imDataFile));
            manifest = XmlHelper.ReadXml<InfrastructureInformationType>(imDataFile);
            TestManifests.VerifyAreEqual(manifest, inputManifest);
        }

        private static void ValidateCodeDeployment(string dataRoot, List<FabricNodeType> nodes, bool isHttpServerEnabled, bool isSingleMachine, bool isScaleMin)
        {
            if (!isScaleMin)
            {
                return;
            }
            bool isFirst = true;
            foreach (var node in nodes)
            {
                string nodeDir = Path.Combine(dataRoot, node.NodeName);
                Assert.IsTrue(Directory.Exists(nodeDir));
                string fabricDir = Path.Combine(nodeDir, "Fabric");
                Assert.IsTrue(Directory.Exists(fabricDir));
                string fabricCodeDir = Path.Combine(fabricDir, "Fabric.Code");
                Assert.IsTrue(Directory.Exists(fabricCodeDir));
                string dcaCodeDir = Path.Combine(fabricDir, "DCA.Code");
                Assert.IsTrue(!isFirst || Directory.Exists(dcaCodeDir));
                if (isHttpServerEnabled)
                {
                    string httpCodeDir = Path.Combine(fabricDir, "HTTPServer.Code");
                    Assert.IsTrue(Directory.Exists(httpCodeDir));
                }
                isFirst = false;
            }
        }

        private static void ValidateCounters(bool areCountersEnabled, string dataDirectory)
        {
            ProcessStartInfo psi = new ProcessStartInfo()
            {
                FileName = "logman.exe",
                Arguments = "query FabricCounters",
                RedirectStandardOutput = true,
                UseShellExecute = false
            };
            Process p = new Process() { StartInfo = psi };
            p.Start();
            string[] output = p.StandardOutput.ReadToEnd().Split(Environment.NewLine.ToCharArray());
            p.WaitForExit();
            foreach (var line in output)
            {
                if (line.Contains(RootPathPrefix))
                {
                    var info = line.Replace(RootPathPrefix, "").Trim().TrimEnd('\\');
                    Assert.AreEqual(info, Path.Combine(dataDirectory, "log", "PerformanceCountersBinary"));
                }
            }
        }

        private static void ValidateContainerNetworkSetup(TestData data)
        {
            bool containerServiceRunning = false;
            var containerSetupOperation = new ContainerNetworkSetupOperation();

            // check if container NT service is stopped
            bool containerNTServiceStopped;
            var stopcontainerNTService = containerSetupOperation.StopContainerNTService(containerServiceRunning, out containerNTServiceStopped);
            Assert.IsTrue(stopcontainerNTService);

            // start container service
            var startServiceSuccess = containerSetupOperation.StartContainerProviderService(
                FlatNetworkConstants.ContainerServiceArguments, 
                data.DataRoot,
                containerServiceRunning);
            Assert.IsTrue(startServiceSuccess);

            // check if container network is set up
            string dockerNetworkName = string.Empty;
            string dockerNetworkId = string.Empty;
            containerSetupOperation.GetNetwork(FlatNetworkConstants.NetworkName, out dockerNetworkName, out dockerNetworkId);
            Assert.IsNotNull(dockerNetworkId);

            //stop container service on localhost:2375 
            var stopServiceSuccess = containerSetupOperation.StopContainerProviderService(data.DataRoot, containerServiceRunning);
            Assert.IsTrue(stopServiceSuccess);

            // start container NT service if stopped by test
            var startContainerNTService = containerSetupOperation.StartContainerNTService(containerServiceRunning, containerNTServiceStopped);
            Assert.IsTrue(startContainerNTService);

            //check if the firewall rule is set up
            var firewallPolicy = containerSetupOperation.GetFirewallPolicy();
            Assert.IsNotNull(firewallPolicy);
            Assert.IsTrue(containerSetupOperation.DoesFirewallRuleExist(firewallPolicy));
        }

        private const string RootPathPrefix = "Root Path:";
    }
}
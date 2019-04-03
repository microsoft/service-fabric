// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.Win32;

    using System.Diagnostics;
    using System.Fabric.Test;
    using System.Fabric.Test.Common.FabricUtility;
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;
    using System.IO;
    using System.Management.Automation;
    using System.Management.Automation.Runspaces;
    using System.Xml;
    using System.Xml.Serialization;
    using System.Management;
    using System.Fabric;
    using System;

    [TestClass]
    public class ClusterManagementTests
    {
        private static Runspace runspace;
        private static string baseDropPath;
        private static bool createdWinFabRegistryKey;
        private static string originalFabricCodePath;
        private static string originalFabricBinRoot;

        [ClassInitialize]
        public static void ClassInitialize(TestContext context)
        {
            LogHelper.Log("Make drop");            
            TestCommon.MakeDrop(Constants.BaseDrop);
            baseDropPath = Path.Combine(Directory.GetCurrentDirectory(), Constants.BaseDrop);

            LogHelper.Log("Create and open runspace");
            runspace = TestCommon.CreateRunSpace(baseDropPath);
            runspace.Open();
        }

        [ClassCleanup]
        public static void ClassCleanup()
        {
            LogHelper.Log("Close and dispose runspace");
            runspace.Close();
            runspace.Dispose();
            LogHelper.Log("Delete drop folder");
            FileUtility.RecursiveDelete(baseDropPath);
            try
            {
                Directory.Delete(baseDropPath);                
            }
            catch(System.IO.IOException exception)
            {
                LogHelper.Log("Exception {0} while deleting {1}", exception, baseDropPath);
            }
        }

        [TestInitialize]
        public static void TestInitialize()
        {
            LogHelper.Log("Set FabricCodePath and FabricBinRoot registry");
            
            //// Unlike RegCreateKeyEx in native code, there are no managed APIs to tell us if a registry key existed before.
            //// Hence first open the key and create in case it does not exist
            createdWinFabRegistryKey = false;
            RegistryKey winfabRegKey = Registry.LocalMachine.OpenSubKey(FabricConstants.FabricRegistryKeyPath, true);
            if (winfabRegKey == null)
            {
                winfabRegKey = Registry.LocalMachine.CreateSubKey(FabricConstants.FabricRegistryKeyPath);
                createdWinFabRegistryKey = true;
            }
            using (winfabRegKey)
            {
                originalFabricCodePath = (string)winfabRegKey.GetValue(Constants.FabricCodePath);
                winfabRegKey.SetValue(Constants.FabricCodePath, Path.Combine(baseDropPath, @"bin\Fabric\Fabric.Code"));

                originalFabricBinRoot = (string)winfabRegKey.GetValue(Constants.FabricBinRoot);
                winfabRegKey.SetValue(Constants.FabricBinRoot, Path.Combine(baseDropPath, @"bin"));
            }

            LogHelper.Log("Delete fabric host service");
            DeleteFabricHostSvc();
        }

        [TestCleanup]
        public static void TestCleanup()
        {
            if (createdWinFabRegistryKey)
            {
                LogHelper.Log("Deleting Windows Fabric registry hive");
                Registry.LocalMachine.DeleteSubKeyTree(FabricConstants.FabricRegistryKeyPath, false);
            }
            else
            {
                LogHelper.Log("Restoring values of FabricCodePath and FabricBinRoot in registry");
                using (var registry = Registry.LocalMachine.CreateSubKey(FabricConstants.FabricRegistryKeyPath))
                {
                    if (!string.IsNullOrEmpty(originalFabricCodePath))
                    {
                        registry.SetValue(Constants.FabricCodePath, originalFabricCodePath);
                    }
                    if (!string.IsNullOrEmpty(originalFabricCodePath))
                    {
                        registry.SetValue(Constants.FabricBinRoot, originalFabricBinRoot);
                    }
                }
            }

            LogHelper.Log("Delete fabric host service");
            DeleteFabricHostSvc();
        }

        [TestMethod]
        [Owner("heeldin")]
        public void NewAndRemoveNodeConfigurationTestClusterManifestOnly()
        {
            LogHelper.Log("NewNodeConfigurationTestClusterManifestOnly");
            CreateFabricHostSvc();
            var fabricDataRoot = Path.Combine(baseDropPath, @"Test Data");
            var fabricLogRoot = Path.Combine(baseDropPath, @"Test Log");
            var clusterManifestPath = Path.Combine(Environment.ExpandEnvironmentVariables(Constants.TestRootPath), "DevEnv-FiveNodes.xml");
            Directory.CreateDirectory(fabricDataRoot);
            var command = new Command(Constants.NewNodeConfigurationCommand);
            command.Parameters.Add("ClusterManifestPath", clusterManifestPath);
            command.Parameters.Add("FabricDataRoot", fabricDataRoot);
            command.Parameters.Add("FabricLogRoot", fabricLogRoot);
            command.Parameters.Add("RunFabricHostServiceAsManual");
            TestCommon.Log(command);
            var pipeline = runspace.CreatePipeline();
            pipeline.Commands.Add(command);
            var result = pipeline.Invoke();
            TestCommon.Log(result);
            TestUtility.AssertAreEqual(1, result.Count);

            using (var registry = Registry.LocalMachine.CreateSubKey(FabricConstants.FabricRegistryKeyPath))
            {
                Assert.AreEqual(0, string.Compare((string)registry.GetValue(Constants.FabricDataRoot), fabricDataRoot, true));
                Assert.AreEqual(0, string.Compare((string)registry.GetValue(Constants.FabricLogRoot), fabricLogRoot, true));
            }

            var targetFilePath = Path.Combine(fabricDataRoot, Constants.TargetInformationFileName);
            Assert.IsTrue(File.Exists(targetFilePath));
            using (var fileStream = File.Open(targetFilePath, FileMode.Open, FileAccess.Read, FileShare.Read))
            {
                using (var xmlReader = XmlReader.Create(fileStream, new XmlReaderSettings() { XmlResolver = null }))
                {
                    var serializer = new XmlSerializer(typeof(TargetInformationType));
                    var targetInformation = (TargetInformationType)serializer.Deserialize(xmlReader);
                    Assert.AreEqual(0, string.Compare(clusterManifestPath, targetInformation.TargetInstallation.ClusterManifestLocation, true));
                }
            }

            Assert.AreEqual("Manual", GetServiceStartupType(Constants.FabricHostSerivceName));

            command = new Command(Constants.RemoveNodeConfigurationCommand);
            command.Parameters.Add("DeleteLog");
            command.Parameters.Add("Force");

            TestCommon.Log(command);
            pipeline = runspace.CreatePipeline();
            pipeline.Commands.Add(command);
            result = pipeline.Invoke();
            TestCommon.Log(result);
            TestUtility.AssertAreEqual(1, result.Count);
            var folders = Directory.GetDirectories(fabricLogRoot);
            TestUtility.AssertAreEqual(1, folders.Length);
        }

        [TestMethod]
        [Owner("heeldin")]
        public void NewNodeConfigurationTestInfrastructureManifest()
        {
            LogHelper.Log("NewNodeConfigurationTestInfrastructureManifest");
            CreateFabricHostSvc();
            var fabricDataRoot = Path.Combine(baseDropPath, "Test Data");
            var fabricLogRoot = Path.Combine(fabricDataRoot, "Log");
            var clusterManifestPath = Path.Combine(Environment.ExpandEnvironmentVariables(Constants.TestRootPath), "ClusterManifest-Server-Default.xml");
            var infrastructureManifestPath = Path.Combine(Environment.ExpandEnvironmentVariables(Constants.TestRootPath), "InfrastructureManifest.xml");
            Directory.CreateDirectory(fabricDataRoot);
            var command = new Command(Constants.NewNodeConfigurationCommand);
            command.Parameters.Add("ClusterManifestPath", clusterManifestPath);
            command.Parameters.Add("InfrastructureManifestPath", infrastructureManifestPath);
            command.Parameters.Add("FabricDataRoot", fabricDataRoot);
            TestCommon.Log(command);
            var pipeline = runspace.CreatePipeline();
            pipeline.Commands.Add(command);
            var result = pipeline.Invoke();
            TestCommon.Log(result);
            TestUtility.AssertAreEqual(1, result.Count);

            using (var registry = Registry.LocalMachine.CreateSubKey(FabricConstants.FabricRegistryKeyPath))
            {
                Assert.AreEqual(0, string.Compare((string)registry.GetValue(Constants.FabricDataRoot), fabricDataRoot, true));
                Assert.AreEqual(0, string.Compare((string)registry.GetValue(Constants.FabricLogRoot), fabricLogRoot, true));
            }

            var targetFilePath = Path.Combine(fabricDataRoot, Constants.TargetInformationFileName);
            Assert.IsTrue(File.Exists(targetFilePath));
            using (var fileStream = File.Open(targetFilePath, FileMode.Open, FileAccess.Read, FileShare.Read))
            {
                using (var xmlReader = XmlReader.Create(fileStream, new XmlReaderSettings() { XmlResolver = null }))
                {
                    var serializer = new XmlSerializer(typeof(TargetInformationType));
                    var targetInformation = (TargetInformationType)serializer.Deserialize(xmlReader);
                    Assert.AreEqual(0, string.Compare(clusterManifestPath, targetInformation.TargetInstallation.ClusterManifestLocation, true));
                    Assert.AreEqual(0, string.Compare(infrastructureManifestPath, targetInformation.TargetInstallation.InfrastructureManifestLocation, true));
                }
            }
        }


        //[TestMethod]
        [Owner("heeldin")]
        public void NewNodeConfigurationTestFabricDataRootHasTrailingBackSlash()
        {
            LogHelper.Log("NewNodeConfigurationTestFabricDataRootHasTrailingBackSlash");
            CreateFabricHostSvc();
            var fabricDataRoot = Path.Combine(baseDropPath, @"Test Data\");
            Directory.CreateDirectory(fabricDataRoot);
            var command = new Command(Constants.NewNodeConfigurationCommand);
            var clusterManifestPath = Path.Combine(Environment.ExpandEnvironmentVariables(Constants.TestRootPath), "ClusterManifest-Server-Default.xml");
            command.Parameters.Add("ClusterManifestPath", clusterManifestPath);
            command.Parameters.Add("FabricDataRoot", fabricDataRoot);
            TestCommon.Log(command);
            var pipeline = runspace.CreatePipeline();
            pipeline.Commands.Add(command);

            try
            {
                pipeline.Invoke();
                Assert.Fail();
            }
            catch(CmdletInvocationException exception)
            {
                Assert.IsTrue(exception.ErrorRecord.Exception is InvalidOperationException);
            }
        }

        //[TestMethod]
        [Owner("subiswal")]
        public void NewAndRemoveNodeConfigurationUsingFabricPackage()
        {
            LogHelper.Log("NewAndRemoveNodeConfigurationUsingFabricPackage");
            var fabricDataRoot = Path.Combine(baseDropPath, @"TestData");
            var fabricLogRoot = Path.Combine(baseDropPath, @"TestLog");
            Directory.CreateDirectory(fabricDataRoot);
            string defaultDropPath = Environment.ExpandEnvironmentVariables(Constants.FabricDropPath);
            var clusterManifestPath = Path.Combine(Environment.ExpandEnvironmentVariables(Constants.TestRootPath), "DevEnv-FiveNodes.xml");
            var command = new Command(Constants.NewNodeConfigurationCommand);
            command.Parameters.Add("ClusterManifestPath", clusterManifestPath);
            command.Parameters.Add("UsingFabricPackage");
            command.Parameters.Add("FabricPackageRoot", defaultDropPath);
            command.Parameters.Add("FabricDataRoot", fabricDataRoot);
            command.Parameters.Add("FabricLogRoot", fabricLogRoot);
            command.Parameters.Add("RunFabricHostServiceAsManual");
            TestCommon.Log(command);
            var pipeline = runspace.CreatePipeline();
            pipeline.Commands.Add(command);
            var result = pipeline.Invoke();
            TestCommon.Log(result);
            TestUtility.AssertAreEqual(1, result.Count);
            Assert.AreEqual("Manual", GetServiceStartupType(Constants.FabricHostSerivceName));
            using (var registry = Registry.LocalMachine.OpenSubKey(FabricConstants.FabricRegistryKeyPath))
            {
                Assert.IsNotNull(registry);
                Assert.AreEqual(0, string.Compare((string)registry.GetValue(Constants.FabricDataRoot), fabricDataRoot, true));
                Assert.AreEqual(0, string.Compare((string)registry.GetValue(Constants.FabricLogRoot), fabricLogRoot, true));
                Assert.AreEqual(0, string.Compare((string)registry.GetValue(Constants.FabricRoot), defaultDropPath, true));
                Assert.AreEqual(0, string.Compare((string)registry.GetValue(Constants.FabricBinRoot), Path.Combine(defaultDropPath, "bin"), true));
                Assert.AreEqual(0, string.Compare((string)registry.GetValue(Constants.FabricCodePath), Path.Combine(defaultDropPath, @"bin\fabric\fabric.code"), true));
            }

            command = new Command(Constants.RemoveNodeConfigurationCommand);
            command.Parameters.Add("DeleteLog");
            command.Parameters.Add("Force");
            command.Parameters.Add("UsingFabricPackage");
            TestCommon.Log(command);
            pipeline = runspace.CreatePipeline();
            pipeline.Commands.Add(command);
            result = pipeline.Invoke();
            TestCommon.Log(result);
            TestUtility.AssertAreEqual(1, result.Count);
            Assert.AreEqual("<null>", GetServiceStartupType(Constants.FabricHostSerivceName));
            using (var registry = Registry.LocalMachine.OpenSubKey(FabricConstants.FabricRegistryKeyPath))
            {
                Assert.IsNull(registry);
            }

            var folders = Directory.GetDirectories(fabricLogRoot);
            TestUtility.AssertAreEqual(1, folders.Length);
        }

        [TestMethod]
        [Owner("heeldin")]
        public void InvokeEncryptSecretTest()
        {
            LogHelper.Log("InvokeEncryptSecretTest");
            var command = new Command(Constants.InvokeEncryptSecretCommand);
            command.Parameters.Add("CertThumbPrint", "7812205a39d22376daa037f05aede3601a7e64bf");
            command.Parameters.Add("CertStoreLocation", X509Credentials.StoreNameDefault);
            command.Parameters.Add("Text", "!Password");
            TestCommon.Log(command);
            var pipeline = runspace.CreatePipeline();
            pipeline.Commands.Add(command);
            var result = pipeline.Invoke();
            TestCommon.Log(result);
            TestUtility.AssertAreEqual(1, result.Count);
            pipeline.Dispose();
        }

        [TestMethod]
        [Owner("heeldin")]
        public void TestClusterManifestTest()
        {
            LogHelper.Log("TestClusterManifestTest");
            var pipeline = runspace.CreatePipeline();
            var command = new Command(Constants.TestClusterManifestCommand);
            command.Parameters.Add("ClusterManifestPath", @"..\..\..\DevEnv-FiveNodes.xml");
            pipeline.Commands.Add(command);
            TestCommon.Log(command);
            var result = pipeline.Invoke();
            TestCommon.Log(result);
            TestUtility.AssertAreEqual(1, result.Count);
            Assert.IsTrue((bool)(result[0].ImmediateBaseObject));
        }

        private static void CreateFabricHostSvc()
        {
            LogHelper.Log("CreateFabricHostSvc");
            var processStartInfo = new ProcessStartInfo
            {
                FileName = Constants.ServiceControllerFileName,
                Arguments = string.Format(CultureInfo.InvariantCulture, "create {0} binPath= {1}", Constants.FabricHostSerivceName, Constants.FabricHostFileName),
                WindowStyle = ProcessWindowStyle.Hidden
            };
            var process = Process.Start(processStartInfo);
            process.WaitForExit();
        }

        private static void DeleteFabricHostSvc()
        {
            LogHelper.Log("DeleteFabricHostSvc");
            var processStartInfo = new ProcessStartInfo
            {
                FileName = Constants.ServiceControllerFileName,
                Arguments = string.Format(CultureInfo.InvariantCulture, "Delete {0}", Constants.FabricHostSerivceName),
                WindowStyle = ProcessWindowStyle.Hidden
            };
            var process = Process.Start(processStartInfo);
            process.WaitForExit();
        }

        private static string GetServiceStartupType(string serviceName)
        {
            string filter = String.Format("SELECT * FROM Win32_Service WHERE Name = '{0}'", serviceName);

            ManagementObjectSearcher query = new ManagementObjectSearcher(filter);

            if (query == null) return "<null>";

            try
            {
                ManagementObjectCollection services = query.Get();

                foreach (ManagementObject service in services)
                {
                    return service.GetPropertyValue("StartMode").ToString() == "Auto" ? "Automatic" : "Manual";
                }
            }
            catch (Exception)
            {
                return "<null>";
            }

            return "<null>";
        }
    }
}
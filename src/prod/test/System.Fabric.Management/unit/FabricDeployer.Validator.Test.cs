// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.Fabric.FabricDeployer
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using NetFwTypeLib;
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;
    using System.IO;
    using WEX.TestExecution;

    /// <summary>
    /// This is the class that tests the validation of the files that we deploy.
    /// </summary>
    [TestClass]
    internal class FabricDeployerValidatorTests
    {
        ConsoleEventListener validatorConsoleTraces = new ConsoleEventListener();

        ~FabricDeployerValidatorTests()
        {
            validatorConsoleTraces.Dispose();
        }
        
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateProductionClusterManifests()
        {
            InitializeTracing("ValidateProductionClusterManifests");
            string[] productionConfigurationFilesToBeValidated = { "ProdEnv-FiveNodes-CertSecurity.xml", "ProdEnv-NineNodes-CertSecurity.xml", "ProdEnv-SevenNodes-CertSecurity.xml", "ProdEnv-ThreeNodes-CertSecurity.xml" };
            string fabunittest = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests");
            string schemaFile = Path.Combine(fabunittest, "ServiceFabricServiceModel.xsd");
            foreach (string fileName in productionConfigurationFilesToBeValidated)
            {
                string data = File.ReadAllText(Path.Combine(fabunittest, fileName));
                data = data.Replace("IsScaleMin=\"false\"", "IsScaleMin=\"true\"");
                string clusterManifestFilePath = "Test-" + fileName;
                File.WriteAllText(clusterManifestFilePath, data);
                Verify.IsTrue(File.Exists(clusterManifestFilePath));
                Deployer deployer;
                int error = Deployer.TryCreateDeployer(clusterManifestFilePath, string.Empty, string.Empty, string.Empty, string.Empty, string.Empty, Utility.GetCurrentCodeVersion(), out deployer, schemaFile);
                Verify.AreEqual(error, Constants.ErrorCode_Success);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateManifestWithAllValidConfigurationsPasses()
        {
            InitializeTracing("ValidateManifestWithAllValidConfigurationsPasses");
            string fabunittest = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests");
            string schemaFile = Path.Combine(fabunittest, "ServiceFabricServiceModel.xsd");
            Deployer deployer;
            int error = Deployer.TryCreateDeployer(Path.Combine(fabunittest, "ClusterManifest-AllSettings.xml"), string.Empty, string.Empty, string.Empty, string.Empty, string.Empty, Utility.GetCurrentCodeVersion(), out deployer, schemaFile);
            Verify.AreEqual(error, Constants.ErrorCode_Success);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateInvalidSectionsNotAllowed()
        {
            InitializeTracing("ValidateInvalidSectionsNotAllowed");
            string fabunittest = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests");
            string schemaFile = Path.Combine(fabunittest, "ServiceFabricServiceModel.xsd");
            Deployer deployer;
            int error = Deployer.TryCreateDeployer(Path.Combine(fabunittest, "ClusterManifest-FabricNodeSection.xml"), string.Empty, string.Empty, string.Empty, string.Empty, string.Empty, Utility.GetCurrentCodeVersion(), out deployer, schemaFile);
            Verify.AreEqual(error, Constants.ErrorCode_ClusterManifestValidationError);
            error = Deployer.TryCreateDeployer(Path.Combine(fabunittest, "ClusterManifest-NodeCapacitiesSection.xml"), string.Empty, string.Empty, string.Empty, string.Empty, string.Empty, Utility.GetCurrentCodeVersion(), out deployer, schemaFile);
            Verify.AreEqual(error, Constants.ErrorCode_ClusterManifestValidationError);
            error = Deployer.TryCreateDeployer(Path.Combine(fabunittest, "ClusterManifest-NodeDomainIdsSection.xml"), string.Empty, string.Empty, string.Empty, string.Empty, string.Empty, Utility.GetCurrentCodeVersion(), out deployer, schemaFile);
            Verify.AreEqual(error, Constants.ErrorCode_ClusterManifestValidationError);
            error = Deployer.TryCreateDeployer(Path.Combine(fabunittest, "ClusterManifest-NodePropertiesSection.xml"), string.Empty, string.Empty, string.Empty, string.Empty, string.Empty, Utility.GetCurrentCodeVersion(), out deployer, schemaFile);
            Verify.AreEqual(error, Constants.ErrorCode_ClusterManifestValidationError);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateDependenciesAreChecked()
        {
            InitializeTracing("ValidateDependenciesAreChecked");
            string fabunittest = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests");
            string schemaFile = Path.Combine(fabunittest, "ServiceFabricServiceModel.xsd");
            Deployer deployer;
            int error = Deployer.TryCreateDeployer(Path.Combine(fabunittest, "ClusterManifest-ClusterCredentialTypeDependencyCheck.xml"), string.Empty, string.Empty, string.Empty, string.Empty, string.Empty, Utility.GetCurrentCodeVersion(), out deployer, schemaFile);
            Verify.AreEqual(error, Constants.ErrorCode_ClusterManifestValidationError);
            error = Deployer.TryCreateDeployer(Path.Combine(fabunittest, "ClusterManifest-NTLMEnabledDependencyCheck.xml"), string.Empty, string.Empty, string.Empty, string.Empty, string.Empty, Utility.GetCurrentCodeVersion(), out deployer, schemaFile);
            Verify.AreEqual(error, Constants.ErrorCode_ClusterManifestValidationError);
            error = Deployer.TryCreateDeployer(Path.Combine(fabunittest, "ClusterManifest-ServerAuthCredentialTypeDependencyCheck.xml"), string.Empty, string.Empty, string.Empty, string.Empty, string.Empty, Utility.GetCurrentCodeVersion(), out deployer, schemaFile);
            Verify.AreEqual(error, Constants.ErrorCode_ClusterManifestValidationError);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateFabricHostSettingsInvalid()
        {
            InitializeTracing("ValidateFabricHostSettingsInvalid");
            string fabunittest = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests");
            string fabricHostSettingsFile = Path.Combine(fabunittest, "FabricHostSettings-InvalidFormat.xml");
            int error = FabricHostSettings.AddFabricUpgradeSection("arguments1", fabricHostSettingsFile, true);
            Verify.AreEqual(error, Constants.ErrorCode_UnableToUpdateFabricHostSettings);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateFabricHostSettingsMultipleFabricUpgradeSections()
        {
            InitializeTracing("ValidateFabricHostSettingsMultipleFabricUpgradeSections");
            string fabunittest = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests");
            string fabricHostSettingsFile = Path.Combine(fabunittest, "FabricHostSettings-MultipleFabricUpgradeSections.xml");
            int error = FabricHostSettings.AddFabricUpgradeSection("arguments1", fabricHostSettingsFile, true);
            Verify.AreEqual(error, Constants.ErrorCode_UnableToUpdateFabricHostSettings);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateFabricHostSettingsValid()
        {
            InitializeTracing("ValidateFabricHostSettingsValid");
            string fabunittest = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests");
            string fabricHostSettingsFile = Path.Combine(fabunittest, "FabricHostSettings-Valid.xml");
            int error = FabricHostSettings.AddFabricUpgradeSection("arguments1", fabricHostSettingsFile, true);
            Verify.AreEqual(error, Constants.ErrorCode_Success);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateFabricHostSettingsValidReplace()
        {
            InitializeTracing("ValidateFabricHostSettingsValidReplace");
            string fabunittest = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests");
            string fabricHostSettingsFile = Path.Combine(fabunittest, "FabricHostSettings-ValidReplace.xml");
            int error = FabricHostSettings.AddFabricUpgradeSection("arguments1", fabricHostSettingsFile, true);
            Verify.AreEqual(error, Constants.ErrorCode_Success);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ValidateClusterHealthPolicy()
        {
            InitializeTracing("ValidateClusterHealthPolicy");
            string fabunittest = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests");
            string schemaFile = Path.Combine(fabunittest, "ServiceFabricServiceModel.xsd");
            string fileName = "ClusterManifest-AllSettings.xml";

            string data = File.ReadAllText(Path.Combine(fabunittest, fileName));

            {
                string copy = data.Replace("MaxPercentUnhealthyNodes\" Value=\"0", "MaxPercentUnhealthyNodes\" Value=\"101");
                UpdateTestClusterManifestAndVerify(copy, fileName, Constants.ErrorCode_ClusterManifestValidationError);
            }

            {
                string copy = data.Replace("MaxPercentUnhealthyApplications\" Value=\"0", "MaxPercentUnhealthyApplications\" Value=\"101");
                UpdateTestClusterManifestAndVerify(copy, fileName, Constants.ErrorCode_ClusterManifestValidationError);
            }
        }

        private static void InitializeTracing(string testName)
        {
            string log = Path.Combine(Environment.GetEnvironmentVariable("_NTTREE"), "FabricUnitTests", "log");
            Utility.TraceSource = Trace.GetEventSource(FabricEvents.Tasks.FabricDeployer);
            validatorConsoleTraces.EnableEvents(FabricEvents.Events, EventLevel.Informational);
            string fileName = Path.Combine(log, testName);
            Utility.InitializeTraceSource(fileName);
            Utility.TraceSource.WriteInfo("Test", "Starting test {0}", testName);
        }

        private static void UpdateTestClusterManifestAndVerify(string fileName, string data, int expectedErrorCode)
        {
            string clusterManifestFilePath = "Test-" + fileName;
            File.WriteAllText(clusterManifestFilePath, data);
            Verify.IsTrue(File.Exists(clusterManifestFilePath));
            Deployer deployer;
            int error = Deployer.TryCreateDeployer(clusterManifestFilePath, string.Empty, string.Empty, string.Empty, string.Empty, string.Empty, Utility.GetCurrentCodeVersion(), out deployer, schemaFile);
            Verify.AreEqual(error, expectedErrorCode);
        }
    }
}
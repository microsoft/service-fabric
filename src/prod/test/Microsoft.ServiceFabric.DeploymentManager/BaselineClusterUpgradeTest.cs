// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Test
{
    using System;
    using System.Fabric.Management.ServiceModel;
    using System.Linq;

    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Microsoft.ServiceFabric.DeploymentManager.Common;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    /// <summary>
    /// test 
    /// </summary>
    [TestClass]
    public class BaselineClusterUpgradeTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneRollForwardTest()
        {
            string jsonFilePath = "myClusterConfig.UnSecure.DevCluster.json";
            string expectedConfigVersion = "1.0.0";
            StandAloneCluster cluster = Utility.PopulateStandAloneClusterWithBaselineJson(jsonFilePath);

            Assert.IsTrue(Utility.RunStateMachine(cluster));

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneBaselineUpgradeState));
            Assert.AreEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            var clusterManagerSection = cluster.Pending.ExternalState.ClusterManifest.FabricSettings.Single(section => section.Name.Equals(StringConstants.SectionName.ClusterManager));
            Assert.IsTrue(clusterManagerSection.Parameter.Any(param => param.Name == StringConstants.ParameterName.EnableAutomaticBaseline && param.Value == "True"));

            Utility.RollForwardOneIteration(cluster);

            Assert.IsFalse(Utility.RunStateMachine(cluster));

            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);

            Assert.AreEqual(expectedConfigVersion, cluster.Current.CSMConfig.Version.Version);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneRollForwardTestWithCustomdeserealization()
        {
            string jsonFilePath = "myClusterConfig.UnSecure.DevCluster.json";
            string expectedConfigVersion = "1.0.0";
            StandAloneCluster cluster = Utility.PopulateStandAloneClusterWithBaselineJson(jsonFilePath);

            var defaultSerializedCluster = Utility.SerealizeUsingDefaults(cluster);
            cluster = Utility.DeserealizeUsingCustomDeserealizer(defaultSerializedCluster);

            Assert.IsTrue(Utility.RunStateMachine(cluster));

            defaultSerializedCluster = Utility.SerealizeUsingDefaults(cluster);
            cluster = Utility.DeserealizeUsingCustomDeserealizer(defaultSerializedCluster);

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneBaselineUpgradeState));
            Assert.AreEqual(upgradeState.TargetCsmConfig.Version, cluster.Current.CSMConfig.Version);

            Utility.RollForwardOneIteration(cluster);

            defaultSerializedCluster = Utility.SerealizeUsingDefaults(cluster);
            cluster = Utility.DeserealizeUsingCustomDeserealizer(defaultSerializedCluster);

            Assert.IsFalse(Utility.RunStateMachine(cluster));

            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);

            Assert.AreEqual(expectedConfigVersion, cluster.Current.CSMConfig.Version.Version);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneRollBackTest()
        {
            string jsonFilePath = "myClusterConfig.UnSecure.DevCluster.json";
            StandAloneCluster cluster = Utility.PopulateStandAloneClusterWithBaselineJson(jsonFilePath);

            Assert.IsTrue(Utility.RunStateMachine(cluster));

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneBaselineUpgradeState));
            Assert.AreEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);

            Utility.RollBackOneIteration(cluster);

            // baseline upgrade failure results in retry
            Assert.IsFalse(Utility.RunStateMachine(cluster));

            Assert.IsNotNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneSetClusterIdTest()
        {
            string[] jsonFilePaths = new string[]
            {
                "myClusterId.Unsecure.DevCluster.GA.json",
                "myClusterId.Unsecure.DevCluster.Jan2017.json"
            };

            foreach (string jsonFilePath in jsonFilePaths)
            {
                StandAloneCluster cluster = Utility.PopulateStandAloneClusterWithBaselineJson(jsonFilePath);
                Assert.IsTrue(Utility.RunStateMachine(cluster));
                ClusterUpgradeStateBase upgradeState = cluster.Pending;
                Assert.IsInstanceOfType(upgradeState, typeof(StandAloneBaselineUpgradeState));
                Assert.AreEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
                Utility.RollForwardOneIteration(cluster);
                Assert.IsFalse(Utility.RunStateMachine(cluster));
                Assert.IsNull(cluster.Pending);
                Assert.IsNotNull(cluster.Current);

                string clusterIdInManifest = cluster.Current.ExternalState.ClusterManifest.FabricSettings.First(
                    section => section.Name == StringConstants.SectionName.UpgradeOrchestrationService).Parameter.First(
                    param => param.Name == StringConstants.ParameterName.ClusterId).Value;
                Assert.AreEqual("EAB8C37A-CC8A-4EE8-8250-3B0A6B7D0B59", clusterIdInManifest);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneNonsecureClusterManifestTest()
        {
            this.TestStandAloneClusterManifestFabricSettingsParameter(
                "myClusterConfig.UnSecure.DevCluster.json",
                "Security",
                "ClientRoleEnabled",
                "false");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneSecuredClusterManifestTest()
        {
            this.TestStandAloneClusterManifestFabricSettingsParameter(
                "myClusterConfig.X509.DevCluster.json",
                "Security",
                "ClientRoleEnabled",
                "true");
        }

        private void TestStandAloneClusterManifestFabricSettingsParameter(
            string baselineJsonPath,
            string sectionName,
            string parameterName,
            string expectedValue)
        {
            var cluster = Utility.PopulateStandAloneClusterWithBaselineJson(baselineJsonPath);

            Assert.IsNull(cluster.Pending);
            Assert.IsNull(cluster.Current);

            Assert.IsTrue(Utility.RunStateMachine(cluster));

            Assert.IsNotNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);

            this.TestParameterInFabricSettings(
                cluster.Pending.ExternalState.ClusterManifest.FabricSettings,
                sectionName,
                parameterName,
                expectedValue);

            this.TestParameterInFabricSettings(
                cluster.Current.ExternalState.ClusterManifest.FabricSettings,
                sectionName,
                parameterName,
                expectedValue);

            Utility.RollForwardOneIteration(cluster);

            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);

            this.TestParameterInFabricSettings(
                cluster.Current.ExternalState.ClusterManifest.FabricSettings,
                sectionName,
                parameterName,
                expectedValue);
        }

        private void TestParameterInFabricSettings(
            SettingsOverridesTypeSection[] fabricSettings,
            string sectionName,
            string parameterName,
            string expectedValue)
        {
            Assert.IsNotNull(fabricSettings);
            Assert.IsNotNull(sectionName);
            Assert.IsNotNull(parameterName);

            var targetSection =
                fabricSettings
                    .FirstOrDefault((section) => string.Equals(sectionName, section.Name, StringComparison.Ordinal));

            Assert.IsNotNull(targetSection);

            var targetParameter =
                targetSection.Parameter
                    .FirstOrDefault((parameter) => string.Equals(parameterName, parameter.Name, StringComparison.Ordinal));

            Assert.IsNotNull(targetParameter);
            Assert.AreEqual(expectedValue, targetParameter.Value);
        }
    }
}
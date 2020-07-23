// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.ClusterManagementCommon;

    /// <summary>
    /// test 
    /// </summary>
    [TestClass]
    public class BaselineClusterUpgradeTest
    {
        private const int BaselineDataDeletionAgeInDays = 7;

        private const int TargetDataDeletionAgeInDays = 8;

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollForwardTest()
        {
            string jsonFilePath = "ClusterConfig.Unsecured.DevCluster.json";
            string expectedConfigVersion = "1.0.0";
            MockupCluster cluster = Utility.PopulateClusterWithBaselineJson(jsonFilePath);

            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(MockupBaselineUpgradeState));
            Assert.AreEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);

            Utility.RollForwardOneIteration(cluster);

            Assert.IsFalse(cluster.RunStateMachine());

            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);

            Assert.AreEqual(expectedConfigVersion, cluster.Current.CSMConfig.Version.Version);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollBackTest()
        {
            string jsonFilePath = "ClusterConfig.Unsecured.DevCluster.json";
            MockupCluster cluster = Utility.PopulateClusterWithBaselineJson(jsonFilePath);

            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(MockupBaselineUpgradeState));
            Assert.AreEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);

            Utility.RollBackOneIteration(cluster);

            // baseline upgrade failure results in retry
            Assert.IsFalse(cluster.RunStateMachine());

            Assert.IsNotNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void InterruptWithSimpleClusterUpgradeTest()
        {
            string jsonFilePath = "ClusterConfig.Unsecured.DevCluster.json";
            MockupCluster cluster = Utility.PopulateClusterWithBaselineJson(jsonFilePath);

            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(MockupBaselineUpgradeState));
            Assert.AreEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);

            //User starts a simple cluster upgrade.
            Utility.UpdateCluster("ClusterConfig.Unsecured.DevClusterV2.json", cluster);

            //Baseline upgrade can't be interrupted.
            Assert.IsFalse(cluster.RunStateMachine());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void InterruptWithScaleUpClusterUpgradeTest()
        {
            string jsonFilePath = "ClusterConfig.Unsecured.DevCluster.json";
            MockupCluster cluster = Utility.PopulateClusterWithBaselineJson(jsonFilePath);

            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(MockupBaselineUpgradeState));
            Assert.AreEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);

            //User starts a scale up cluster upgrade.
            Utility.UpdateCluster("ClusterConfig.Unsecured.DevCluster.ScaleUp.json", cluster);

            //Baseline upgrade can't be interrupted.
            Assert.IsFalse(cluster.RunStateMachine());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void InterruptWithAutoScaleUpgradeTest()
        {
            string jsonFilePath = "ClusterConfig.Unsecured.DevCluster.json";
            MockupCluster cluster = Utility.PopulateClusterWithBaselineJson(jsonFilePath);

            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(MockupBaselineUpgradeState));
            Assert.AreEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);

            //User starts a auto scale cluster upgrade.
            Utility.UpdateCluster("ClusterConfig.Unsecured.DevCluster.AutoScale.V2.json", cluster);

            //Baseline upgrade can't be interrupted.
            Assert.IsFalse(cluster.RunStateMachine());
        }
    }
}
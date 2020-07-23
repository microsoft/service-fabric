// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System.Collections.Generic;
    using System.IO;

    [TestClass]
    internal class SimpleClusterUpgradeTest
    {
        private const string BaselineConfigVersion = "1.0.0";

        private const string TargetConfigVersion = "2.0.0";

        private const string InterruptedTargetConfigVersion = "3.0.0";

        private const int BaselineDataDeletionAgeInDays = 7;

        private const int TargetDataDeletionAgeInDays = 8;

        private const int InterruptedTargetDataDeletionAgeInDays = 12;

        private const string TargetMsiVersion = "9.0.0.0";

        private const string TargetClusterSettingsVersion = "9.0.0.0";

        private const string InterruptedTargetMsiVersion = "11.0.0.0";

        private const string InterruptedTargetClusterSettingsVersion = "11.0.0.0";

        private const string BaselineMsiVersion = "Baseline";

        private const string BaselineClusterSettingsVersion = "Baseline";

        private const int InterruptedTargetVMInstanceCount = 4;

        private const string TargetClusterCertificateThumbprint = "0059B4D1357C30CD8FC65BE06484E4B2477764Q6";

        private const string TargetServerCertificateThumbprint = "0059B4D1357C30CD8FC65BE06484E4B2477764Q6";

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ConfigOnlyRollForwardTest()
        {
            MockupCluster cluster = Utility.DoBaselineUpgrade("ClusterConfig.Unsecured.DevCluster.json");
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.UpdateCluster("ClusterConfig.Unsecured.DevClusterV2.json", cluster);

            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(MockupSimpleClusterUpgradeState));
            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);
            Assert.AreNotEqual(SimpleClusterUpgradeTest.TargetDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);

            Utility.RollForwardOneIteration(cluster);

            Assert.IsFalse(cluster.RunStateMachine());

            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreEqual(SimpleClusterUpgradeTest.TargetDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);
            Assert.AreEqual(SimpleClusterUpgradeTest.TargetConfigVersion, cluster.Current.CSMConfig.Version.Version);
            Assert.AreEqual(cluster.TargetNodeConfig, null);
            Assert.AreEqual(cluster.TargetWrpConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfigUpgradeFailed, false);
            Assert.AreEqual(cluster.TargetWrpConfigUpgradeFailed, false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ConfigOnlyRollBackTest()
        {
            MockupCluster cluster = Utility.DoBaselineUpgrade("ClusterConfig.Unsecured.DevCluster.json");
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.UpdateCluster("ClusterConfig.Unsecured.DevClusterV2.json", cluster);

            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(MockupSimpleClusterUpgradeState));
            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);
            Assert.AreNotEqual(SimpleClusterUpgradeTest.TargetDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);

            Utility.RollBackOneIteration(cluster);

            Assert.IsFalse(cluster.RunStateMachine());

            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreNotEqual(SimpleClusterUpgradeTest.TargetDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);
            Assert.AreEqual(cluster.TargetNodeConfig, null);
            Assert.AreEqual(cluster.TargetWrpConfig, null);
            Assert.AreNotEqual(cluster.TargetCsmConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfigUpgradeFailed, true);
            Assert.AreEqual(cluster.TargetWrpConfigUpgradeFailed, false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void MsiOnlyRollForwardTest()
        {
            MockupCluster cluster = Utility.DoBaselineUpgrade("ClusterConfig.Unsecured.DevCluster.json");
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.UpdateCluster(cluster, SimpleClusterUpgradeTest.TargetMsiVersion, SimpleClusterUpgradeTest.TargetClusterSettingsVersion);

            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(MockupSimpleClusterUpgradeState));
            Assert.AreNotEqual(upgradeState.TargetWrpConfig.Version, cluster.Current.WRPConfig.Version);

            Utility.RollForwardOneIteration(cluster);

            Assert.IsFalse(cluster.RunStateMachine());

            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreEqual(SimpleClusterUpgradeTest.TargetMsiVersion, cluster.Current.WRPConfig.Version.MsiVersion);
            Assert.AreEqual(SimpleClusterUpgradeTest.TargetClusterSettingsVersion, cluster.Current.WRPConfig.Version.ClusterSettingsVersion);
            Assert.AreEqual(cluster.TargetNodeConfig, null);
            Assert.AreEqual(cluster.TargetWrpConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfigUpgradeFailed, false);
            Assert.AreEqual(cluster.TargetWrpConfigUpgradeFailed, false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void MsiOnlyRollForwardTestUserSet()
        {
            MockupCluster cluster = Utility.DoBaselineUpgrade("ClusterConfig.Unsecured.DevCluster.json");
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.UpdateCluster(cluster, SimpleClusterUpgradeTest.TargetMsiVersion, SimpleClusterUpgradeTest.TargetClusterSettingsVersion, true);

            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(MockupSimpleClusterUpgradeState));
            Assert.AreNotEqual(upgradeState.TargetWrpConfig.Version, cluster.Current.WRPConfig.Version);

            Utility.RollForwardOneIteration(cluster);

            Assert.IsFalse(cluster.RunStateMachine());

            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreEqual(SimpleClusterUpgradeTest.TargetMsiVersion, cluster.Current.WRPConfig.Version.MsiVersion);
            Assert.AreEqual(SimpleClusterUpgradeTest.TargetClusterSettingsVersion, cluster.Current.WRPConfig.Version.ClusterSettingsVersion);
            Assert.AreEqual(cluster.TargetNodeConfig, null);
            Assert.AreEqual(cluster.TargetWrpConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfigUpgradeFailed, false);
            Assert.AreEqual(cluster.TargetWrpConfigUpgradeFailed, false);
        }


        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void MsiOnlyRollBackTest()
        {
            MockupCluster cluster = Utility.DoBaselineUpgrade("ClusterConfig.Unsecured.DevCluster.json");
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.UpdateCluster(cluster, SimpleClusterUpgradeTest.TargetMsiVersion, SimpleClusterUpgradeTest.TargetClusterSettingsVersion);

            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(MockupSimpleClusterUpgradeState));
            Assert.AreNotEqual(upgradeState.TargetWrpConfig.Version, cluster.Current.WRPConfig.Version);

            Utility.RollBackOneIteration(cluster);

            Assert.IsFalse(cluster.RunStateMachine());

            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineMsiVersion, cluster.Current.WRPConfig.Version.MsiVersion);
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineClusterSettingsVersion, cluster.Current.WRPConfig.Version.ClusterSettingsVersion);
            Assert.AreEqual(cluster.TargetNodeConfig, null);
            Assert.AreNotEqual(cluster.TargetWrpConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfigUpgradeFailed, false);
            Assert.AreEqual(cluster.TargetWrpConfigUpgradeFailed, true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void MsiOnlyRollBackTestUserSet()
        {
            MockupCluster cluster = Utility.DoBaselineUpgrade("ClusterConfig.Unsecured.DevCluster.json");
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.UpdateCluster(cluster, SimpleClusterUpgradeTest.TargetMsiVersion, SimpleClusterUpgradeTest.TargetClusterSettingsVersion, true);

            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(MockupSimpleClusterUpgradeState));
            Assert.AreNotEqual(upgradeState.TargetWrpConfig.Version, cluster.Current.WRPConfig.Version);

            Utility.RollBackOneIteration(cluster);

            Assert.IsFalse(cluster.RunStateMachine());

            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineMsiVersion, cluster.Current.WRPConfig.Version.MsiVersion);
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineClusterSettingsVersion, cluster.Current.WRPConfig.Version.ClusterSettingsVersion);
            Assert.AreEqual(cluster.TargetNodeConfig, null);
            Assert.AreNotEqual(cluster.TargetWrpConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfigUpgradeFailed, true);
            Assert.AreEqual(cluster.TargetWrpConfigUpgradeFailed, true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void InterruptWithSimpleClusterUpgradeTest()
        {
            MockupCluster cluster = Utility.DoBaselineUpgrade("ClusterConfig.Unsecured.DevCluster.json");
            Utility.UpdateCluster("ClusterConfig.Unsecured.DevClusterV2.json", cluster);

            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;

            //Interrupt with another simple cluster upgrade
            Utility.UpdateCluster("ClusterConfig.Unsecured.DevClusterV3.json", cluster);

            //Simple cluster upgrade can be interrupted
            Assert.IsTrue(cluster.RunStateMachine());

            Utility.RollForwardOneIteration(cluster);

            Assert.IsFalse(cluster.RunStateMachine());
            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);

            Assert.AreEqual(SimpleClusterUpgradeTest.InterruptedTargetDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);
            Assert.AreEqual(SimpleClusterUpgradeTest.InterruptedTargetConfigVersion, cluster.Current.CSMConfig.Version.Version);
            Assert.AreEqual(cluster.TargetNodeConfig, null);
            Assert.AreEqual(cluster.TargetWrpConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfigUpgradeFailed, false);
            Assert.AreEqual(cluster.TargetWrpConfigUpgradeFailed, false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void InterruptWithSimpleCodeClusterUpgradeTest()
        {
            MockupCluster cluster = Utility.DoBaselineUpgrade("ClusterConfig.Unsecured.DevCluster.json");
            Utility.UpdateCluster(cluster, SimpleClusterUpgradeTest.TargetMsiVersion, SimpleClusterUpgradeTest.TargetClusterSettingsVersion);
            ClusterUpgradeStateBase upgradeState = cluster.Pending;

            Utility.UpdateCluster(cluster, InterruptedTargetMsiVersion, InterruptedTargetClusterSettingsVersion);
            Assert.IsTrue(cluster.RunStateMachine());

            Utility.RollForwardOneIteration(cluster);

            Assert.IsFalse(cluster.RunStateMachine());

            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreEqual(SimpleClusterUpgradeTest.InterruptedTargetMsiVersion, cluster.Current.WRPConfig.Version.MsiVersion);
            Assert.AreEqual(SimpleClusterUpgradeTest.InterruptedTargetClusterSettingsVersion, cluster.Current.WRPConfig.Version.ClusterSettingsVersion);
            Assert.AreEqual(cluster.TargetNodeConfig, null);
            Assert.AreEqual(cluster.TargetWrpConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfigUpgradeFailed, false);
            Assert.AreEqual(cluster.TargetWrpConfigUpgradeFailed, false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void SimpleConfigUpgradeInterruptWithSimpleCodeUpgrade()
        {
            MockupCluster cluster = Utility.DoBaselineUpgrade("ClusterConfig.Unsecured.DevCluster.json");
            Utility.UpdateCluster("ClusterConfig.Unsecured.DevClusterV2.json", cluster);

            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(MockupSimpleClusterUpgradeState));

            //Interrupt
            Utility.UpdateCluster(cluster, SimpleClusterUpgradeTest.TargetMsiVersion, SimpleClusterUpgradeTest.TargetClusterSettingsVersion);
            Assert.IsFalse(cluster.RunStateMachine());
            upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(MockupSimpleClusterUpgradeState));

            Utility.RollForwardOneIteration(cluster);

            Assert.AreEqual(SimpleClusterUpgradeTest.TargetDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);
            Assert.AreEqual(SimpleClusterUpgradeTest.TargetConfigVersion, cluster.Current.CSMConfig.Version.Version);

            Assert.IsFalse(cluster.RunStateMachine());

            Utility.RollForwardOneIteration(cluster);

            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreEqual(SimpleClusterUpgradeTest.TargetMsiVersion, cluster.Current.WRPConfig.Version.MsiVersion);
            Assert.AreEqual(SimpleClusterUpgradeTest.TargetClusterSettingsVersion, cluster.Current.WRPConfig.Version.ClusterSettingsVersion);
            Assert.AreEqual(cluster.TargetNodeConfig, null);
            Assert.AreEqual(cluster.TargetWrpConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfigUpgradeFailed, false);
            Assert.AreEqual(cluster.TargetWrpConfigUpgradeFailed, false);
        }
    }
}
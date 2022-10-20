// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Test
{
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Microsoft.ServiceFabric.DeploymentManager.Common;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System.Fabric.Management.ServiceModel;
    using System.Linq;

    /// <summary>
    /// test 
    /// </summary>
    [TestClass]
    public class SimpleClusterUpgradeTest
    {
        private const string BaselineConfigVersion = "1.0.0";

        private const string TargetConfigVersion = "2.0.0";

        private const string BaselineAdminConfigClusterSettingsVersion = "Baseline";

        private const string TargetAdminConfigClusterSettingsVersion = "1.0";

        private const int BaselineDataDeletionAgeInDays = 7;

        private const int TargetDataDeletionAgeInDays = 8;

        private const int BaselineSeedNodeCount = 9;

        private const int TargetSeedNodeCount = 9;

        private const string TargetMsiVersion = "5.6.0.0";

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneConfigOnlyRollForwardTest()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.UnSecure.DevCluster.json");
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.UpdateStandAloneCluster("myClusterConfig.UnSecure.DevClusterV2.json", cluster);

            Assert.IsTrue(Utility.RunStateMachine(cluster));

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);
            Assert.AreNotEqual(SimpleClusterUpgradeTest.TargetDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);

            Utility.RollForwardOneIteration(cluster);

            Assert.IsFalse(Utility.RunStateMachine(cluster));

            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreEqual(SimpleClusterUpgradeTest.TargetDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);

            Assert.AreEqual(SimpleClusterUpgradeTest.TargetConfigVersion, cluster.Current.CSMConfig.Version.Version);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneRollForwardTestWithCustomDeserealization()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.UnSecure.DevCluster.json");
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.UpdateStandAloneCluster("myClusterConfig.UnSecure.DevClusterV2.json", cluster);

            Assert.IsTrue(Utility.RunStateMachine(cluster));

            string defaultSerializedCluster = Utility.SerealizeUsingDefaults(cluster);           
            cluster = Utility.DeserealizeUsingCustomDeserealizer(defaultSerializedCluster);

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);
            Assert.AreNotEqual(SimpleClusterUpgradeTest.TargetDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);

            Utility.RollForwardOneIteration(cluster);

            defaultSerializedCluster = Utility.SerealizeUsingDefaults(cluster);
            cluster = Utility.DeserealizeUsingCustomDeserealizer(defaultSerializedCluster);

            Assert.IsFalse(Utility.RunStateMachine(cluster));
            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreEqual(SimpleClusterUpgradeTest.TargetDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);

            Assert.AreEqual(SimpleClusterUpgradeTest.TargetConfigVersion, cluster.Current.CSMConfig.Version.Version);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneConfigOnlyRollBackTest()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.UnSecure.DevCluster.json");
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.UpdateStandAloneCluster("myClusterConfig.UnSecure.DevClusterV2.json", cluster);

            Assert.IsTrue(Utility.RunStateMachine(cluster));

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);
            Assert.AreNotEqual(SimpleClusterUpgradeTest.TargetDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);

            Utility.RollBackOneIteration(cluster);

            Assert.IsFalse(Utility.RunStateMachine(cluster));

            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreNotEqual(SimpleClusterUpgradeTest.TargetDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);

            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneCodeUpgradeOnlyRollForwardTestAuto()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("ClusterConfig.Windows.MultiMachine.June2018.V1.json");
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.UpdateStandAloneClusterAutoWrpUpgrade(cluster, "5.6.0.0");

            Assert.IsTrue(Utility.RunStateMachine(cluster));

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Assert.AreNotEqual(upgradeState.TargetWrpConfig.Version, cluster.Current.WRPConfig.Version);
            
            Utility.RollForwardOneIteration(cluster);

            Assert.IsFalse(Utility.RunStateMachine(cluster));

            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreEqual(SimpleClusterUpgradeTest.TargetMsiVersion, cluster.Current.WRPConfig.Version.MsiVersion);
            Assert.AreEqual(cluster.TargetNodeConfig, null);
            Assert.AreEqual(cluster.TargetWrpConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfigUpgradeFailed, false);
            Assert.AreEqual(cluster.TargetWrpConfigUpgradeFailed, false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneCodeUpgradeOnlyRollBackTestAuto()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("ClusterConfig.Windows.MultiMachine.June2018.V1.json");
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.UpdateStandAloneClusterAutoWrpUpgrade(cluster, "5.6.0.0");

            Assert.IsTrue(Utility.RunStateMachine(cluster));

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Assert.AreNotEqual(upgradeState.TargetWrpConfig.Version, cluster.Current.WRPConfig.Version);

            Utility.RollBackOneIteration(cluster);

            Assert.IsFalse(Utility.RunStateMachine(cluster));

            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreNotEqual(SimpleClusterUpgradeTest.TargetMsiVersion, cluster.Current.WRPConfig.Version.MsiVersion);
            Assert.AreEqual(cluster.TargetNodeConfig, null);
            Assert.AreNotEqual(cluster.TargetWrpConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfigUpgradeFailed, false);
            Assert.AreEqual(cluster.TargetWrpConfigUpgradeFailed, true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneCodeUpgradeOnlyRollForwardTestUserSet()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("ClusterConfig.Windows.MultiMachine.June2018.V1.json");
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.UpdateStandAloneCluster("ClusterConfig.Windows.MultiMachine.June2018.V2.json", cluster, true);

            Assert.IsTrue(Utility.RunStateMachine(cluster));

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Assert.AreNotEqual(upgradeState.TargetWrpConfig.Version, cluster.Current.WRPConfig.Version);

            Utility.RollForwardOneIteration(cluster);

            Assert.IsFalse(Utility.RunStateMachine(cluster));

            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreEqual(SimpleClusterUpgradeTest.TargetMsiVersion, cluster.Current.WRPConfig.Version.MsiVersion);
            Assert.AreEqual(cluster.TargetNodeConfig, null);
            Assert.AreEqual(cluster.TargetWrpConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfigUpgradeFailed, false);
            Assert.AreEqual(cluster.TargetWrpConfigUpgradeFailed, false);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneCodeUpgradeOnlyRollBackTestUserSet()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("ClusterConfig.Windows.MultiMachine.June2018.V1.json");
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.UpdateStandAloneCluster("ClusterConfig.Windows.MultiMachine.June2018.V2.json", cluster, true);

            Assert.IsTrue(Utility.RunStateMachine(cluster));

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Assert.AreNotEqual(upgradeState.TargetWrpConfig.Version, cluster.Current.WRPConfig.Version);

            Utility.RollBackOneIteration(cluster);

            Assert.IsFalse(Utility.RunStateMachine(cluster));

            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreNotEqual(SimpleClusterUpgradeTest.TargetMsiVersion, cluster.Current.WRPConfig.Version.MsiVersion);
            Assert.AreEqual(cluster.TargetNodeConfig, null);
            Assert.AreNotEqual(cluster.TargetWrpConfig, null);
            Assert.AreNotEqual(cluster.TargetCsmConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfigUpgradeFailed, true);
            Assert.AreEqual(cluster.TargetWrpConfigUpgradeFailed, true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneCodeUpgradeIsNotInterruptiple()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("ClusterConfig.Windows.MultiMachine.June2018.V1.json");
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.UpdateStandAloneCluster("ClusterConfig.Windows.MultiMachine.June2018.V2.json", cluster, true);

            Assert.IsTrue(Utility.RunStateMachine(cluster));

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.IsFalse(upgradeState.CanInterruptUpgrade());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneRollBackTestWithCustomDeserealization()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.UnSecure.DevCluster.json");
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            string defaultSerializedCluster = Utility.SerealizeUsingDefaults(cluster);
            cluster = Utility.DeserealizeUsingCustomDeserealizer(defaultSerializedCluster);

            Utility.UpdateStandAloneCluster("myClusterConfig.UnSecure.DevClusterV2.json", cluster);

            defaultSerializedCluster = Utility.SerealizeUsingDefaults(cluster);
            cluster = Utility.DeserealizeUsingCustomDeserealizer(defaultSerializedCluster);

            Assert.IsTrue(Utility.RunStateMachine(cluster));

            defaultSerializedCluster = Utility.SerealizeUsingDefaults(cluster);
            cluster = Utility.DeserealizeUsingCustomDeserealizer(defaultSerializedCluster);

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);
            Assert.AreNotEqual(SimpleClusterUpgradeTest.TargetDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);

            Utility.RollBackOneIteration(cluster);

            defaultSerializedCluster = Utility.SerealizeUsingDefaults(cluster);
            cluster = Utility.DeserealizeUsingCustomDeserealizer(defaultSerializedCluster);

            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreNotEqual(SimpleClusterUpgradeTest.TargetDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);

            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneRollForwardWRPTest()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.UnSecure.DevCluster.json");
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.UpdateStandAloneClusterWRPSettings("myClusterSettings.1.0.json", "1.0", cluster);

            Assert.IsTrue(Utility.RunStateMachine(cluster));

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreNotEqual(upgradeState.TargetWrpConfig, cluster.Current.WRPConfig);
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineAdminConfigClusterSettingsVersion, cluster.Current.WRPConfig.Version.ClusterSettingsVersion);

            Utility.RollForwardOneIteration(cluster);

            Assert.IsFalse(Utility.RunStateMachine(cluster));

            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreEqual(SimpleClusterUpgradeTest.TargetAdminConfigClusterSettingsVersion, cluster.Current.WRPConfig.Version.ClusterSettingsVersion);
            Assert.AreEqual(cluster.TargetNodeConfig, null);
            Assert.AreEqual(cluster.TargetWrpConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfigUpgradeFailed, false);
            Assert.AreEqual(cluster.TargetWrpConfigUpgradeFailed, false);

            // Perform simple upgrade after WRP upgrade
            Utility.UpdateStandAloneCluster("myClusterConfig.UnSecure.DevClusterV2.json", cluster);

            Assert.IsTrue(Utility.RunStateMachine(cluster));

            upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);
            Assert.AreNotEqual(SimpleClusterUpgradeTest.TargetDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);

            Utility.RollForwardOneIteration(cluster);

            Assert.IsFalse(Utility.RunStateMachine(cluster));

            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreEqual(SimpleClusterUpgradeTest.TargetDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);

            Assert.AreEqual(SimpleClusterUpgradeTest.TargetConfigVersion, cluster.Current.CSMConfig.Version.Version);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneRollForwardWRPTestWithCustomDeserealizer()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.UnSecure.DevCluster.json");
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            string defaultSerializedCluster = Utility.SerealizeUsingDefaults(cluster);
            cluster = Utility.DeserealizeUsingCustomDeserealizer(defaultSerializedCluster);

            Utility.UpdateStandAloneClusterWRPSettings("myClusterSettings.1.0.json", "1.0", cluster);

            defaultSerializedCluster = Utility.SerealizeUsingDefaults(cluster);
            cluster = Utility.DeserealizeUsingCustomDeserealizer(defaultSerializedCluster);

            Assert.IsTrue(Utility.RunStateMachine(cluster));

            defaultSerializedCluster = Utility.SerealizeUsingDefaults(cluster);
            cluster = Utility.DeserealizeUsingCustomDeserealizer(defaultSerializedCluster);

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreNotEqual(upgradeState.TargetWrpConfig, cluster.Current.WRPConfig);
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineAdminConfigClusterSettingsVersion, cluster.Current.WRPConfig.Version.ClusterSettingsVersion);

            Utility.RollForwardOneIteration(cluster);

            defaultSerializedCluster = Utility.SerealizeUsingDefaults(cluster);
            cluster = Utility.DeserealizeUsingCustomDeserealizer(defaultSerializedCluster);

            Assert.IsFalse(Utility.RunStateMachine(cluster));

            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreEqual(SimpleClusterUpgradeTest.TargetAdminConfigClusterSettingsVersion, cluster.Current.WRPConfig.Version.ClusterSettingsVersion);
            Assert.AreEqual(cluster.TargetNodeConfig, null);
            Assert.AreEqual(cluster.TargetWrpConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfigUpgradeFailed, false);
            Assert.AreEqual(cluster.TargetWrpConfigUpgradeFailed, false);

            // Perform simple upgrade after WRP upgrade
            Utility.UpdateStandAloneCluster("myClusterConfig.UnSecure.DevClusterV2.json", cluster);

            defaultSerializedCluster = Utility.SerealizeUsingDefaults(cluster);
            cluster = Utility.DeserealizeUsingCustomDeserealizer(defaultSerializedCluster);

            Assert.IsTrue(Utility.RunStateMachine(cluster));

            defaultSerializedCluster = Utility.SerealizeUsingDefaults(cluster);
            cluster = Utility.DeserealizeUsingCustomDeserealizer(defaultSerializedCluster);

            upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);
            Assert.AreNotEqual(SimpleClusterUpgradeTest.TargetDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);

            Utility.RollForwardOneIteration(cluster);

            defaultSerializedCluster = Utility.SerealizeUsingDefaults(cluster);
            cluster = Utility.DeserealizeUsingCustomDeserealizer(defaultSerializedCluster);

            Assert.IsFalse(Utility.RunStateMachine(cluster));

            defaultSerializedCluster = Utility.SerealizeUsingDefaults(cluster);
            cluster = Utility.DeserealizeUsingCustomDeserealizer(defaultSerializedCluster);

            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreEqual(SimpleClusterUpgradeTest.TargetDataDeletionAgeInDays, cluster.Current.CSMConfig.DiagnosticsStoreInformation.DataDeletionAgeInDays);

            Assert.AreEqual(SimpleClusterUpgradeTest.TargetConfigVersion, cluster.Current.CSMConfig.Version.Version);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneRollBackWRPTest()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.UnSecure.DevCluster.json");
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.UpdateStandAloneClusterWRPSettings("myClusterSettings.1.0.json", "1.0", cluster);

            Assert.IsTrue(Utility.RunStateMachine(cluster));

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreNotEqual(upgradeState.TargetWrpConfig, cluster.Current.WRPConfig);
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineAdminConfigClusterSettingsVersion, cluster.Current.WRPConfig.Version.ClusterSettingsVersion);

            Utility.RollBackOneIteration(cluster);

            Assert.IsFalse(Utility.RunStateMachine(cluster));

            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineAdminConfigClusterSettingsVersion, cluster.Current.WRPConfig.Version.ClusterSettingsVersion);
            Assert.AreEqual(cluster.TargetNodeConfig, null);
            Assert.AreNotEqual(cluster.TargetWrpConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfig, null);
            Assert.AreEqual(cluster.TargetCsmConfigUpgradeFailed, false);
            Assert.AreEqual(cluster.TargetWrpConfigUpgradeFailed, true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void StandAloneRollForwardTestEnsureNoSeedNodeShift()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("ClusterConfig.Unsecure.MultiMachine.SeedCheckV1.json");
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);
            var seedNodes = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Select(node => node.NodeName).ToList();
            Assert.AreEqual(SimpleClusterUpgradeTest.BaselineSeedNodeCount, seedNodes.Count());

            Utility.UpdateStandAloneCluster("ClusterConfig.Unsecure.MultiMachine.SeedCheckV2.json", cluster);

            Assert.IsTrue(Utility.RunStateMachine(cluster));

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);

            var targetSeedNodes = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Select(node => node.NodeName).ToList();
            Assert.AreEqual(SimpleClusterUpgradeTest.TargetSeedNodeCount, targetSeedNodes.Count());

            var firstNotSecond = targetSeedNodes.Except(seedNodes).ToList();
            var secondNotFirst = seedNodes.Except(targetSeedNodes).ToList();
            Assert.IsTrue(!firstNotSecond.Any() && !secondNotFirst.Any());

            Utility.RollForwardOneIteration(cluster);

            Assert.IsFalse(Utility.RunStateMachine(cluster));

            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreEqual(SimpleClusterUpgradeTest.TargetConfigVersion, cluster.Current.CSMConfig.Version.Version);
        }
    }
}

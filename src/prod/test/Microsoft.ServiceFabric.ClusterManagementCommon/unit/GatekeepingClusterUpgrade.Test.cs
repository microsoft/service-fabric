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
    public class GatekeepingClusterUpgradeTest
    {
        private const string ExpectedMsiVersion = "0.0.0.1";

        private const string ExpectedClusterSettingsVersion = "0.0.1";

        private const string WrongMsiVersion = "1.0.0.0";

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollForwardTest()
        {
            MockupCluster cluster = Utility.DoBaselineUpgrade("ClusterConfig.Unsecured.DevCluster.json");

            //Start a simple cluster upgrade to update the MSI version to the ExpectedMsiVersion.
            Utility.UpdateCluster(cluster, GatekeepingClusterUpgradeTest.ExpectedMsiVersion, GatekeepingClusterUpgradeTest.ExpectedClusterSettingsVersion);

            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Utility.RollForwardOneIteration(cluster);

            Assert.IsFalse(cluster.RunStateMachine());

            //Set the MSI version in current to the wrong MSI verion.
            cluster.Current.WRPConfig.Version.MsiVersion = GatekeepingClusterUpgradeTest.WrongMsiVersion;

            Assert.AreNotEqual(cluster.Current.WRPConfig.Version.MsiVersion, cluster.Current.ExternalState.MsiVersion);

            //Sice the MSI version number is not expected. Trigger a GatekeepingClusterUpgrade to rollback the version number to the ExpectedMsiVersion.
            MockupUpgradeStateActivator mockupUpgradeStateActivator = new MockupUpgradeStateActivator();

            cluster.Pending = mockupUpgradeStateActivator.CreateGatekeepingClusterUpgradeState(cluster.Current.CSMConfig, cluster.Current.WRPConfig, cluster.Current.NodeConfig, cluster, new MockupTraceLogger());

            bool processingSuccess = cluster.Pending.StartProcessing();

            Utility.RollForwardOneIteration(cluster);

            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreEqual(GatekeepingClusterUpgradeTest.ExpectedMsiVersion, cluster.Current.ExternalState.MsiVersion);
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Test
{
    using System;
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

    /// <summary>
    /// multi-phase cluster upgrade state test 
    /// </summary>
    [TestClass]
    public class MultiphaseClusterUpgradeStateTest
    {
        private const string BaselineConfigVersion = "1.0.0";

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollForwardTest()
        {
            int totalIterations = 5;

            ICluster cluster = this.InitializeCluster();

            MockUpMultiphaseClusterUpgradeState upgradeState = new MockUpMultiphaseClusterUpgradeState(totalIterations, cluster);
            Assert.IsTrue(upgradeState.StartProcessing());
            Assert.IsNotNull(upgradeState.ExternalState);
            Assert.IsNotNull(upgradeState.PreviousExternalState);
            Assert.AreNotEqual(upgradeState.ExternalState, upgradeState.PreviousExternalState);
            Assert.AreEqual(totalIterations, upgradeState.ClusterManifestList.Count());
            Assert.AreEqual(0, upgradeState.CurrentListIndex);
            Assert.IsNull(upgradeState.ClusterUpgradeStarted());

            for (int i = 0; i < totalIterations; i++)
            {
                ClusterExternalState currentState = upgradeState.ExternalState;
                ClusterState finalState = upgradeState.RollForwardOneIteration();

                Assert.AreEqual(currentState, upgradeState.PreviousExternalState);
                Assert.AreEqual(i == totalIterations - 1, finalState != null);

                Assert.IsNotNull(upgradeState.ExternalState);
                Assert.IsNotNull(upgradeState.PreviousExternalState);
                Assert.AreNotSame(upgradeState.ExternalState, upgradeState.PreviousExternalState);
                Assert.AreEqual(totalIterations, upgradeState.ClusterManifestList.Count());

                if (i < totalIterations - 1)
                {
                    Assert.AreEqual(i + 1, upgradeState.CurrentListIndex);
                }
                else
                {
                    Assert.AreEqual(i, upgradeState.CurrentListIndex);
                }
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollBackTest()
        {
            this.InternalRollBackTest(10, successfulRollForwardIteration: -1, unsuccessfulRollBackIteration: null);

            this.InternalRollBackTest(10, successfulRollForwardIteration: 0, unsuccessfulRollBackIteration: 1);

            this.InternalRollBackTest(10, successfulRollForwardIteration: 8, unsuccessfulRollBackIteration: null);

            this.InternalRollBackTest(10, successfulRollForwardIteration: 8, unsuccessfulRollBackIteration: 5);

            this.InternalRollBackTest(10, successfulRollForwardIteration: 8, unsuccessfulRollBackIteration: 1);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void InterruptTest()
        {
            int totalIterations = 5;

            ICluster cluster = this.InitializeCluster();
            MockUpMultiphaseClusterUpgradeState upgradeState = new MockUpMultiphaseClusterUpgradeState(totalIterations, cluster);
            Assert.IsTrue(upgradeState.CanInterruptUpgrade());

            upgradeState.StartProcessing();
            Assert.IsTrue(upgradeState.CanInterruptUpgrade());

            upgradeState.ClusterUpgradeStarted();
            Assert.IsTrue(upgradeState.CanInterruptUpgrade());

            for (int i = 0; i < totalIterations; i++)
            {
                upgradeState.RollForwardOneIteration();
                Assert.IsFalse(upgradeState.CanInterruptUpgrade());
            }

            Assert.IsFalse(upgradeState.CanInterruptUpgrade());
        }

        internal ICluster InitializeCluster()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.X509.DevCluster.json");
            Assert.AreEqual(MultiphaseClusterUpgradeStateTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);
            Utility.UpdateStandAloneCluster("myClusterConfig.X509.DevClusterV2.json", cluster);
            cluster.TargetWrpConfig = new StandaloneAdminConfig();
            cluster.TargetNodeConfig = new ClusterNodeConfig();
            return cluster;
        }

        internal void InternalRollBackTest(int totalIterations, int successfulRollForwardIteration, int? unsuccessfulRollBackIteration)
        {
            Assert.IsTrue(totalIterations > successfulRollForwardIteration + 1);

            ICluster cluster = this.InitializeCluster();

            MockUpMultiphaseClusterUpgradeState upgradeState = new MockUpMultiphaseClusterUpgradeState(totalIterations, cluster);
            Assert.IsTrue(upgradeState.StartProcessing());
            Assert.IsNull(upgradeState.ClusterUpgradeStarted());

            // roll forward
            for (int i = 0; i <= successfulRollForwardIteration; i++)
            {
                upgradeState.RollForwardOneIteration();
            }

            // prepare roll back
            ClusterExternalState currentState = upgradeState.ExternalState;
            int currentIndex = upgradeState.CurrentListIndex;
            ClusterState targetState = upgradeState.RollBackOneIteration();
            Assert.AreEqual(successfulRollForwardIteration < 0, targetState != null);
            if (targetState == null)
            {
                Assert.AreEqual(upgradeState.PreviousExternalState, currentState);
                Assert.AreEqual(currentIndex, upgradeState.CurrentListIndex);
            }

            // successful roll back
            int totalSuccessfulRollbackIterations = unsuccessfulRollBackIteration.HasValue ?
                successfulRollForwardIteration - unsuccessfulRollBackIteration.Value + 1: successfulRollForwardIteration + 1;
            for (int i = 0; i < totalSuccessfulRollbackIterations; i++)
            {
                currentState = upgradeState.ExternalState;
                targetState = upgradeState.RollForwardOneIteration();

                Assert.AreEqual(currentState, upgradeState.PreviousExternalState);
                Assert.AreEqual(i == successfulRollForwardIteration, targetState != null);

                Assert.IsNotNull(upgradeState.ExternalState);
                Assert.IsNotNull(upgradeState.PreviousExternalState);
                Assert.AreNotSame(upgradeState.ExternalState, upgradeState.PreviousExternalState);
                Assert.AreEqual(totalIterations, upgradeState.ClusterManifestList.Count());

                if (i < totalSuccessfulRollbackIterations - 1 || unsuccessfulRollBackIteration.HasValue)
                {
                    Assert.AreEqual(successfulRollForwardIteration - i, upgradeState.CurrentListIndex);
                }
                else
                {
                    Assert.AreEqual(successfulRollForwardIteration - i + 1, upgradeState.CurrentListIndex);
                }
            }

            // unsuccessful rollback
            if (unsuccessfulRollBackIteration.HasValue)
            {
                currentIndex = upgradeState.CurrentListIndex;
                targetState = upgradeState.RollBackOneIteration();
                Assert.AreEqual(currentIndex, upgradeState.CurrentListIndex);
                Assert.IsNull(targetState);
            }
            else
            {
                Assert.AreEqual(successfulRollForwardIteration > 0 ? 1 : 0, upgradeState.CurrentListIndex);
            }
        }
    }
}
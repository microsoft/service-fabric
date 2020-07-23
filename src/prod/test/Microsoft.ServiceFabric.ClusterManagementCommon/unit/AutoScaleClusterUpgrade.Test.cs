// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using System.Collections.Generic;

    /// <summary>
    /// test 
    /// </summary>
    [TestClass]
    public class AutoScaleClusterUpgradeTest
    {
        private const int BaselineVMInstanceCount = 3;

        private const int TargetVMInstanceCount = 6;

        private const string BaselineConfigVersion = "2.0.0";

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollForwardTest()
        {
            MockupCluster cluster = Utility.DoBaselineUpgrade("ClusterConfig.Unsecured.DevCluster.json");
            Assert.AreEqual(AutoScaleClusterUpgradeTest.BaselineVMInstanceCount, cluster.Current.CSMConfig.NodeTypes[0].VMInstanceCount);

            Utility.UpdateCluster("ClusterConfig.Unsecured.DevCluster.AutoScale.V2.json", cluster);

            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;

            Assert.IsInstanceOfType(upgradeState, typeof(MockupAutoScaleClusterUpgradeState));
            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);

            //Need to set targetReplicaSetSize to an empty dictionary, in order to get the system think the replica set size is updated. 
            //Otherwise VerifyTargetSystemServicesReplicaSetSize() will fail.

            var targetReplicaSetSize = new Dictionary<string, ReplicaSetSize>() { {"", new ReplicaSetSize()} };

            MockupAutoScaleClusterUpgradeState scaleUpUpgradeState = (MockupAutoScaleClusterUpgradeState)upgradeState;
            scaleUpUpgradeState.TargetSystemServicesSize = new Dictionary<string, ReplicaSetSize>();

            scaleUpUpgradeState.GetSystemServiceReplicaSetSize(targetReplicaSetSize);

            Utility.RollForwardOneIteration(cluster);

            upgradeState = (MockupAutoScaleClusterUpgradeState)cluster.Pending;

            Utility.RollForwardOneIteration(cluster);

            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.TargetVMInstanceCount, cluster.Current.CSMConfig.NodeTypes[0].VMInstanceCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollbackTest()
        {
            MockupCluster cluster = Utility.DoBaselineUpgrade("ClusterConfig.Unsecured.DevCluster.json");
            Utility.UpdateCluster("ClusterConfig.Unsecured.DevCluster.AutoScale.V2.json", cluster);

            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;

            Utility.RollBackOneIteration(cluster);
            Assert.IsFalse(cluster.RunStateMachine());
            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);

            cluster.TargetCsmConfigUpgradeFailed = false;
            cluster.TargetCsmConfig = null;
            cluster.TargetNodeConfig = null;
            Utility.UpdateCluster("ClusterConfig.Unsecured.DevCluster.AutoScale.V2.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());
            upgradeState = cluster.Pending;
            Utility.RollForwardOneIteration(cluster);

            upgradeState = cluster.Pending;
            var targetReplicaSetSize = new Dictionary<string, ReplicaSetSize>() { { "", new ReplicaSetSize() } };
            MockupAutoScaleClusterUpgradeState scaleUpUpgradeState = (MockupAutoScaleClusterUpgradeState)upgradeState;
            scaleUpUpgradeState.TargetSystemServicesSize = new Dictionary<string, ReplicaSetSize>();
            scaleUpUpgradeState.GetSystemServiceReplicaSetSize(targetReplicaSetSize);

            Utility.RollForwardOneIteration(cluster);

            Assert.IsFalse(cluster.RunStateMachine());
            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.TargetVMInstanceCount, cluster.Current.CSMConfig.NodeTypes[0].VMInstanceCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.BaselineConfigVersion, cluster.Current.CSMConfig.Version.Version);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void InterruptWithSimpleClusterUpgrade()
        {
            MockupCluster cluster = Utility.DoBaselineUpgrade("ClusterConfig.Unsecured.DevCluster.json");
            Assert.AreEqual(AutoScaleClusterUpgradeTest.BaselineVMInstanceCount, cluster.Current.CSMConfig.NodeTypes[0].VMInstanceCount);

            Utility.UpdateCluster("ClusterConfig.Unsecured.DevCluster.AutoScale.V2.json", cluster);

            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;

            Assert.IsInstanceOfType(upgradeState, typeof(MockupAutoScaleClusterUpgradeState));
            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);

            //Need to set targetReplicaSetSize to an empty dictionary, in order to get the system think the replica set size is updated. 
            //Otherwise VerifyTargetSystemServicesReplicaSetSize() will fail.

            var targetReplicaSetSize = new Dictionary<string, ReplicaSetSize>() { { "", new ReplicaSetSize() } };

            MockupAutoScaleClusterUpgradeState scaleUpUpgradeState = (MockupAutoScaleClusterUpgradeState)upgradeState;
            scaleUpUpgradeState.TargetSystemServicesSize = new Dictionary<string, ReplicaSetSize>();

            scaleUpUpgradeState.GetSystemServiceReplicaSetSize(targetReplicaSetSize);

            //Interrupt with a simple cluster upgrade
            Utility.UpdateCluster("ClusterConfig.Unsecured.DevCluster.AutoScale.V3.json", cluster);

            //AutoScaleClusterUpgrade can't be interrupted.
            Assert.IsFalse(cluster.RunStateMachine());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void InterruptWithCertificateClusterUpgrade()
        {
            MockupCluster cluster = Utility.DoBaselineUpgrade("ClusterConfig.Unsecured.DevCluster.json");
            Assert.AreEqual(AutoScaleClusterUpgradeTest.BaselineVMInstanceCount, cluster.Current.CSMConfig.NodeTypes[0].VMInstanceCount);

            Utility.UpdateCluster("ClusterConfig.Unsecured.DevCluster.AutoScale.V2.json", cluster);

            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;

            Assert.IsInstanceOfType(upgradeState, typeof(MockupAutoScaleClusterUpgradeState));
            Assert.AreNotEqual(upgradeState.TargetCsmConfig, cluster.Current.CSMConfig);

            //Need to set targetReplicaSetSize to an empty dictionary, in order to get the system think the replica set size is updated. 
            //Otherwise VerifyTargetSystemServicesReplicaSetSize() will fail.

            var targetReplicaSetSize = new Dictionary<string, ReplicaSetSize>() { { "", new ReplicaSetSize() } };

            MockupAutoScaleClusterUpgradeState scaleUpUpgradeState = (MockupAutoScaleClusterUpgradeState)upgradeState;
            scaleUpUpgradeState.TargetSystemServicesSize = new Dictionary<string, ReplicaSetSize>();

            scaleUpUpgradeState.GetSystemServiceReplicaSetSize(targetReplicaSetSize);

            //Interrupt with a certificate cluster upgrade
            Utility.UpdateCluster("ClusterConfig.X509.DevCluster.AddPrimary.V1.json", cluster);

            //AutoScaleClusterUpgrade can't be interrupted.
            Assert.IsFalse(cluster.RunStateMachine());
        }
    }
}
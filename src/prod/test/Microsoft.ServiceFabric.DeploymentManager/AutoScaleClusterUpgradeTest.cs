// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Test
{
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Microsoft.ServiceFabric.DeploymentManager.Common;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System;
    using System.Fabric.Management.ServiceModel;
    using System.Linq;

    /// <summary>
    /// Auto-scale cluster upgrade state test 
    /// </summary>
    [TestClass]
    public class AutoScaleClusterUpgradeTest
    {
        private const ReliabilityLevel AddPrimary_BaselineReliabilityLevel = ReliabilityLevel.Bronze;

        private const int AddPrimary_BaselineTotalNodeCount = 3;

        private const string AddPrimary_TargetClusterManifestVersion = "2";

        private const ReliabilityLevel AddPrimary_TargetdReliabilityLevel = ReliabilityLevel.Silver;

        private const int AddPrimary_BaselineTotalPrimaryNodeTypeNodeCount = 3;

        private const ReliabilityLevel AddPrimary_FirstIterationReliabilityLevel = ReliabilityLevel.Bronze;

        private const int AddPrimary_TargetTotalNodeCount = 5;

        private const int AddPrimary_TargetSeedNodeCount = 5;

        private const int AddPrimary_FirstIterationNodeCount = 4;

        private const int AddPrimary_SecondIterationNodeCount = 5;

        private const int AddPrimary_FirstIterationSeedNodeCount = 4;

        private const int AddPrimary_SecondIterationSeedNodeCount = 5;

        [TestInitialize]
        public void Setup()
        {
            Environment.SetEnvironmentVariable("StandaloneAutoScaleClusterUpgradeStateSkipQuorumChecksTestHook", "true");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollForwardTest_AddPrimary()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.X509.DevCluster.Bronze.json");
            Utility.UpdateStandAloneClusterForAddNode("myClusterConfig.X509.DevCluster.Silver.AddPrimary.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));

            //Need to set targetReplicaSetSize to an empty dictionary, in order to get the system think the replica set size is updated. 
            //Otherwise VerifyTargetSystemServicesReplicaSetSize() will fail.
            Utility.SetTargetReplicaSetSize(upgradeState);

            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_BaselineTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_FirstIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_FirstIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Select(node => node.IsSeedNode).Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_TargetdReliabilityLevel, cluster.TargetCsmConfig.ReliabilityLevel);

            Utility.RollForwardOneIteration(cluster);

            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_FirstIterationReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_BaselineTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_BaselineTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_SecondIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_SecondIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Select(node => node.IsSeedNode).Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_TargetTotalNodeCount, cluster.TargetNodeConfig.NodesStatus.Count);


            Utility.RollForwardOneIteration(cluster);
            Assert.IsNull(cluster.Pending);
            Assert.IsFalse(cluster.RunStateMachine());

            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_TargetdReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_TargetTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_TargetSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_TargetClusterManifestVersion, cluster.Current.ExternalState.ClusterManifest.Version);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollForwardTest_AddPrimaryWithCustomDeserealisation()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.X509.DevCluster.Bronze.json");
            Utility.UpdateStandAloneClusterForAddNode("myClusterConfig.X509.DevCluster.Silver.AddPrimary.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());

            var defaultSerializedCluster = Utility.SerealizeUsingDefaults(cluster);
            cluster = Utility.DeserealizeUsingCustomDeserealizer(defaultSerializedCluster);

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));

            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_BaselineTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_FirstIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_FirstIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Select(node => node.IsSeedNode).Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_TargetdReliabilityLevel, cluster.TargetCsmConfig.ReliabilityLevel);

            Utility.RollForwardOneIteration(cluster);

            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_FirstIterationReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_BaselineTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_BaselineTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_SecondIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_SecondIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Select(node => node.IsSeedNode).Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_TargetTotalNodeCount, cluster.TargetNodeConfig.NodesStatus.Count);

            defaultSerializedCluster = Utility.SerealizeUsingDefaults(cluster);
            cluster = Utility.DeserealizeUsingCustomDeserealizer(defaultSerializedCluster);

            Utility.RollForwardOneIteration(cluster);

            upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));

            //Need to set targetReplicaSetSize to an empty dictionary, in order to get the system think the replica set size is updated. 
            //Otherwise VerifyTargetSystemServicesReplicaSetSize() will fail.
            Utility.SetTargetReplicaSetSize(upgradeState);
            Utility.RollForwardOneIteration(cluster);

            defaultSerializedCluster = Utility.SerealizeUsingDefaults(cluster);
            cluster = Utility.DeserealizeUsingCustomDeserealizer(defaultSerializedCluster);

            Assert.IsNull(cluster.Pending);
            Assert.IsFalse(cluster.RunStateMachine());

            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_TargetdReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_TargetTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_TargetSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimary_TargetClusterManifestVersion, cluster.Current.ExternalState.ClusterManifest.Version);
        }

        private const ReliabilityLevel Add2Primary_BaselineReliabilityLevel = ReliabilityLevel.Bronze;

        private const int Add2Primary_BaselineTotalNodeCount = 3;

        private const string Add2Primary_BaselineClusterManifestVersion = "1.0.0";

        private const int Add2Primary_BaselineTotalPrimaryNodeTypeNodeCount = 3;

        private const int Add2Primary_TargetTotalNodeCount = 4;

        private const int Add2Primary_FirstIterationNodeCount = 4;

        private const string Add2Primary_TargetClusterManifestVersion = "2.0.0";

        private const int Add2Primary_TargetNodeCount = 4;

        private const int Add2Primary_TargetSeedNodeCount = 3;

        private const int Add2Primary_FirstIterationSeedNodeCount = 3;

        private const int Add2Primary_TargetIterationSeedNodeCount = 3;


        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollForwardTest_AddPrimaryWithoutReliabilityLevelChange()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.X509.DevCluster.Bronze.V2.json");
            Utility.UpdateStandAloneClusterForAddNode("myClusterConfig.X509.DevCluster.Bronze.AddPrimaryWithoutReliabilityLevelChange.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));

            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_BaselineTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_BaselineTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_BaselineTotalNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_FirstIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_TargetTotalNodeCount, cluster.TargetNodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_BaselineClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.RollForwardOneIteration(cluster);

            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_TargetTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_TargetClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_TargetNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_TargetIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_TargetSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster
                .Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());
            Assert.IsNull(cluster.Pending);
            Assert.IsFalse(cluster.RunStateMachine());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollBackTest_AddPrimaryWithoutReliabilityLevelChange()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.X509.DevCluster.Bronze.V2.json");
            Utility.UpdateStandAloneClusterForAddNode("myClusterConfig.X509.DevCluster.Bronze.AddPrimaryWithoutReliabilityLevelChange.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));

            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_BaselineTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_BaselineTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_BaselineClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.RollBackOneIteration(cluster);
            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_BaselineTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_BaselineTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_BaselineTotalNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_FirstIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_BaselineClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);

            // Manually try to do upgrade again
            cluster.Reset();
            Utility.UpdateStandAloneClusterForAddNode("myClusterConfig.X509.DevCluster.Bronze.AddPrimaryWithoutReliabilityLevelChange.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());

            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_BaselineTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_BaselineTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_FirstIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_FirstIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_TargetTotalNodeCount, cluster.TargetNodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_BaselineClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.RollForwardOneIteration(cluster);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_TargetTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_TargetClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_TargetNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_TargetIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add2Primary_TargetSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster
                .Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());
            Assert.IsNull(cluster.Pending);
            Assert.IsFalse(cluster.RunStateMachine());
        }

        private const ReliabilityLevel AddNonPrimary_BaselineReliabilityLevel = ReliabilityLevel.Bronze;

        private const int AddNonPrimary_BaselineTotalNodeCount = 3;

        private const string AddNonPrimary_BaselineClusterManifestVersion = "1.0.0";

        private const int AddNonPrimary_BaselineTotalPrimaryNodeTypeNodeCount = 3;

        private const int AddNonPrimary_TargetTotalNodeCount = 5;

        private const int AddNonPrimary_FirstIterationNodeCount = 5;

        private const string AddNonPrimary_TargetClusterManifestVersion = "2.0.0";

        private const int AddNonPrimary_TargetNodeCount = 5;


        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollForwardTest_AddNonPrimary()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.X509.DevCluster.Bronze.V2.json");
            Utility.UpdateStandAloneClusterForAddNode("myClusterConfig.X509.DevCluster.Bronze.AddNonPrimary.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));

            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddNonPrimary_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddNonPrimary_BaselineTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddNonPrimary_BaselineTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddNonPrimary_FirstIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddNonPrimary_TargetTotalNodeCount, cluster.TargetNodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddNonPrimary_BaselineClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.RollForwardOneIteration(cluster);

            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddNonPrimary_TargetTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddNonPrimary_TargetClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddNonPrimary_TargetNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.IsNull(cluster.Pending);
            Assert.IsFalse(cluster.RunStateMachine());
        }

        private const ReliabilityLevel Add4NonPrimary_BaselineReliabilityLevel = ReliabilityLevel.Bronze;

        private const int Add4NonPrimary_BaselineTotalNodeCount = 3;

        private const string Add4NonPrimary_BaselineClusterManifestVersion = "1.0.0";

        private const int Add4NonPrimary_BaselineTotalPrimaryNodeTypeNodeCount = 3;

        private const int Add4NonPrimary_TargetTotalNodeCount = 7;

        private const int Add4NonPrimary_FirstIterationNodeCount = 7;

        private const string Add4NonPrimary_TargetClusterManifestVersion = "2.0.0";

        private const int Add4NonPrimary_TargetNodeCount = 7;


        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollForwardTest_Add4NonPrimary()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.X509.DevCluster.Bronze.V2.json");
            Utility.UpdateStandAloneClusterForAddNode("myClusterConfig.X509.DevCluster.Bronze.Add4NonPrimary.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));

            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add4NonPrimary_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add4NonPrimary_BaselineTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add4NonPrimary_BaselineTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add4NonPrimary_FirstIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add4NonPrimary_TargetTotalNodeCount, cluster.TargetNodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add4NonPrimary_BaselineClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.RollForwardOneIteration(cluster);

            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add4NonPrimary_TargetTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add4NonPrimary_TargetClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.Add4NonPrimary_TargetNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.IsNull(cluster.Pending);
            Assert.IsFalse(cluster.RunStateMachine());
        }

        private const ReliabilityLevel AddPrimaryAndNonPrimary_BaselineReliabilityLevel = ReliabilityLevel.Bronze;

        private const int AddPrimaryAndNonPrimary_BaselineTotalNodeCount = 3;

        private const string AddPrimaryAndNonPrimary_BaselineClusterManifestVersion = "1.0.0";

        private const string AddPrimaryAndNonPrimary_FirstIterationClusterManifestVersion = "2.0.0";

        private const ReliabilityLevel AddPrimaryAndNonPrimary_TargetdReliabilityLevel = ReliabilityLevel.Silver;

        private const int AddPrimaryAndNonPrimary_BaselineTotalPrimaryNodeTypeNodeCount = 3;

        private const ReliabilityLevel AddPrimaryAndNonPrimary_FirstIterationReliabilityLevel = ReliabilityLevel.Bronze;

        private const int AddPrimaryAndNonPrimary_TargetTotalNodeCount = 8;

        private const int AddPrimaryAndNonPrimary_TargetSeedNodeCount = 5;

        private const int AddPrimaryAndNonPrimary_FirstIterationNodeCount = 7;

        private const int AddPrimaryAndNonPrimary_SecondIterationNodeCount = 8;

        private const long AddPrimaryAndNonPrimary_TargetNodeConfigVersion = 2;

        private const int AddPrimaryAndNonPrimary_TargetNodeCount = 8;

        private const int AddPrimaryAndNonPrimary_FirstIterationSeedNodeCount = 4;

        private const ReliabilityLevel AddPrimaryAndNonPrimary_SecondIterationReliabilityLevel = ReliabilityLevel.Bronze;

        private const int AddPrimaryAndNonPrimary_SecondIterationSeedNodeCount = 5;


        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollForwardTest_AddPrimaryAndNonPrimaryWithReliabilityLevelChange()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.X509.DevCluster.Bronze.V2.json");
            Utility.UpdateStandAloneClusterForAddNode("myClusterConfig.X509.DevCluster.Silver.AddPrimaryAndNonPrimary.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));

            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary_BaselineTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary_BaselineTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary_FirstIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary_FirstIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary_TargetdReliabilityLevel, cluster.TargetCsmConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary_TargetTotalNodeCount, cluster.TargetNodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary_BaselineClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);

            //Need to set targetReplicaSetSize to an empty dictionary, in order to get the system think the replica set size is updated. 
            //Otherwise VerifyTargetSystemServicesReplicaSetSize() will fail.
            Utility.SetTargetReplicaSetSize(upgradeState);

            Utility.RollForwardOneIteration(cluster);
            Assert.IsNotNull(cluster.Pending);
            upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));

            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary_SecondIterationReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary_SecondIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary_SecondIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary_TargetdReliabilityLevel, cluster.TargetCsmConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary_TargetTotalNodeCount, cluster.TargetNodeConfig.NodesStatus.Count);

            Utility.RollForwardOneIteration(cluster);
            Assert.IsNull(cluster.Pending);
            Assert.IsFalse(cluster.RunStateMachine());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary_FirstIterationClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary_TargetTotalNodeCount, cluster.Current.CSMConfig.TotalNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary_TargetSeedNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary_TargetNodeConfigVersion, cluster.Current.NodeConfig.Version);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary_TargetNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary_TargetSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());
        }

        private const ReliabilityLevel SeedCheck_BaselineReliabilityLevel = ReliabilityLevel.Silver;
        private const int SeedCheck_BaselineTotalNodeCount = 9;
        private const int SeedCheck_BaselineTotalPrimaryNodeTypeNodeCount = 6;
        private const int SeedCheck_TargetTotalPrimaryNodeTypeNodeCount = 10;
        private const int SeedCheck_FirstIterationNodeCount = 12;
        private const int SeedCheck_FirstIterationSeedNodeCount = 6;
        private const int SeedCheck_SecondIterationNodeCount = 13;
        private const int SeedCheck_SecondIterationSeedNodeCount = 7;
        private const int SeedCheck_ThirdIterationNodeCount = 14;
        private const int SeedCheck_ThirdIterationSeedNodeCount = 8;
        private const int SeedCheck_FourthIterationNodeCount = 15;
        private const int SeedCheck_FourthIterationSeedNodeCount = 9;
        private const ReliabilityLevel SeedCheck_TargetdReliabilityLevel = ReliabilityLevel.Platinum;
        private const int SeedCheck_TargetTotalNodeCount = 15;
        private const string SeedCheck_BaselineClusterManifestVersion = "1.0.0";
        private const string SeedCheck_TargetClusterManifestVersion = "2.0.0";

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollForwardTest_SeedCheckWithReliabilityLevelChangeScaleUp()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("ClusterConfig.Unsecure.MultiMachine.AutoScaleSeedCheckV1.json");
            var seedNodes = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Select(node => node.NodeName).ToList();

            Utility.UpdateStandAloneClusterForAddNode("ClusterConfig.Unsecure.MultiMachine.AutoScaleSeedCheckV2.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));
            var targetSeedNodes1 = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Select(node => node.NodeName).ToList();
            Assert.IsTrue(targetSeedNodes1.Count() - seedNodes.Count() == 1);
            Assert.IsTrue(seedNodes.All(n => targetSeedNodes1.Contains(n)));

            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_BaselineTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_BaselineTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_FirstIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_FirstIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_TargetdReliabilityLevel, cluster.TargetCsmConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_TargetTotalNodeCount, cluster.TargetNodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_BaselineClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);

            //Need to set targetReplicaSetSize to an empty dictionary, in order to get the system think the replica set size is updated. 
            //Otherwise VerifyTargetSystemServicesReplicaSetSize() will fail.
            Utility.SetTargetReplicaSetSize(upgradeState);

            Utility.RollForwardOneIteration(cluster);
            var targetSeedNodes2 = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Select(node => node.NodeName).ToList();
            Assert.IsTrue(targetSeedNodes2.Count() - targetSeedNodes1.Count() == 1);
            Assert.IsTrue(targetSeedNodes1.All(n => targetSeedNodes2.Contains(n)));
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_SecondIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_SecondIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());

            Utility.RollForwardOneIteration(cluster);
            var targetSeedNodes3 = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Select(node => node.NodeName).ToList();
            Assert.IsTrue(targetSeedNodes3.Count() - targetSeedNodes2.Count() == 1);
            Assert.IsTrue(targetSeedNodes2.All(n => targetSeedNodes3.Contains(n)));
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_ThirdIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_ThirdIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());

            Utility.RollForwardOneIteration(cluster);
            var targetSeedNodes4 = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Select(node => node.NodeName).ToList();
            Assert.IsTrue(targetSeedNodes4.Count() - targetSeedNodes3.Count() == 1);
            Assert.IsTrue(targetSeedNodes3.All(n => targetSeedNodes4.Contains(n)));
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_FourthIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_FourthIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());

            Utility.RollForwardOneIteration(cluster);
            Assert.IsNull(cluster.Pending);
            Assert.IsFalse(cluster.RunStateMachine());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_TargetClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_TargetTotalNodeCount, cluster.Current.CSMConfig.TotalNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_TargetTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_TargetTotalNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_FourthIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollBackTest_ReliabilityLevelChangeScaleUp()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("ClusterConfig.Unsecure.MultiMachine.AutoScaleSeedCheckV1.json");
            var seedNodes = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Select(node => node.NodeName).ToList();

            Utility.UpdateStandAloneClusterForAddNode("ClusterConfig.Unsecure.MultiMachine.AutoScaleSeedCheckV2.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));
            var targetSeedNodes1 = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Select(node => node.NodeName).ToList();
            Assert.IsTrue(targetSeedNodes1.Count() - seedNodes.Count() == 1);
            Assert.IsTrue(seedNodes.All(n => targetSeedNodes1.Contains(n)));

            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_BaselineTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_BaselineTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_FirstIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_FirstIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_TargetdReliabilityLevel, cluster.TargetCsmConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_TargetTotalNodeCount, cluster.TargetNodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_BaselineClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);

            //Fail upgrade in 1st step
            Utility.RollBackOneIteration(cluster);
            Assert.IsFalse(Utility.RunStateMachine(cluster));
            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            
            // Manually try to do upgrade again
            cluster.Reset();
            Utility.UpdateStandAloneClusterForAddNode("ClusterConfig.Unsecure.MultiMachine.AutoScaleSeedCheckV2.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());
            upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));
            targetSeedNodes1 = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Select(node => node.NodeName).ToList();
            Assert.IsTrue(targetSeedNodes1.Count() - seedNodes.Count() == 1);
            Assert.IsTrue(seedNodes.All(n => targetSeedNodes1.Contains(n)));

            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_BaselineTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_BaselineTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_FirstIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_FirstIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_TargetdReliabilityLevel, cluster.TargetCsmConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_TargetTotalNodeCount, cluster.TargetNodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_BaselineClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.RollForwardOneIteration(cluster);
            var targetSeedNodes2 = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Select(node => node.NodeName).ToList();
            Assert.IsTrue(targetSeedNodes2.Count() - targetSeedNodes1.Count() == 1);
            Assert.IsTrue(targetSeedNodes1.All(n => targetSeedNodes2.Contains(n)));
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_SecondIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_SecondIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());

            //Fail upgrade in 2nd step
            Utility.RollBackOneIteration(cluster);
            Assert.IsFalse(Utility.RunStateMachine(cluster));
            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_FirstIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());

            // Manually try to do upgrade again
            cluster.Reset();
            Utility.UpdateStandAloneClusterForAddNode("ClusterConfig.Unsecure.MultiMachine.AutoScaleSeedCheckV2.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());
            upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));

            targetSeedNodes2 = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Select(node => node.NodeName).ToList();
            Assert.IsTrue(targetSeedNodes2.Count() - targetSeedNodes1.Count() == 1);
            Assert.IsTrue(targetSeedNodes1.All(n => targetSeedNodes2.Contains(n)));
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_SecondIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());

            Utility.RollForwardOneIteration(cluster);
            var targetSeedNodes3 = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Select(node => node.NodeName).ToList();
            Assert.IsTrue(targetSeedNodes3.Count() - targetSeedNodes2.Count() == 1);
            Assert.IsTrue(targetSeedNodes2.All(n => targetSeedNodes3.Contains(n)));
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_ThirdIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());

            //Fail upgrade in 3rd step
            Utility.RollBackOneIteration(cluster);
            Assert.IsFalse(Utility.RunStateMachine(cluster));
            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_SecondIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());

            // Manually try to do upgrade again
            cluster.Reset();
            Utility.UpdateStandAloneClusterForAddNode("ClusterConfig.Unsecure.MultiMachine.AutoScaleSeedCheckV2.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());
            upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));

            targetSeedNodes3 = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Select(node => node.NodeName).ToList();
            Assert.IsTrue(targetSeedNodes3.Count() - targetSeedNodes2.Count() == 1);
            Assert.IsTrue(targetSeedNodes2.All(n => targetSeedNodes3.Contains(n)));
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_ThirdIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());

            Utility.RollForwardOneIteration(cluster);
            var targetSeedNodes4 = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Select(node => node.NodeName).ToList();
            Assert.IsTrue(targetSeedNodes4.Count() - targetSeedNodes3.Count() == 1);
            Assert.IsTrue(targetSeedNodes3.All(n => targetSeedNodes4.Contains(n)));
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_FourthIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());

            //Need to set targetReplicaSetSize to an empty dictionary, in order to get the system think the replica set size is updated. 
            //Otherwise VerifyTargetSystemServicesReplicaSetSize() will fail.
            Utility.SetTargetReplicaSetSize(upgradeState);
            Utility.RollForwardOneIteration(cluster);
            Assert.IsNull(cluster.Pending);
            Assert.IsFalse(cluster.RunStateMachine());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_TargetClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_TargetTotalNodeCount, cluster.Current.CSMConfig.TotalNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_TargetTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_TargetTotalNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck_FourthIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());
        }

        private const ReliabilityLevel SeedCheck2_BaselineReliabilityLevel = ReliabilityLevel.Platinum;
        private const int SeedCheck2_BaselineTotalNodeCount = 15;
        private const int SeedCheck2_BaselineTotalPrimaryNodeTypeNodeCount = 10;
        private const int SeedCheck2_TargetTotalPrimaryNodeTypeNodeCount = 6;
        private const int SeedCheck2_FirstIterationNodeCount = 15;
        private const int SeedCheck2_FirstIterationSeedNodeCount = 8;
        private const int SeedCheck2_SecondIterationNodeCount = 15;
        private const int SeedCheck2_SecondIterationSeedNodeCount = 7;
        private const int SeedCheck2_ThirdIterationNodeCount = 15;
        private const int SeedCheck2_ThirdIterationSeedNodeCount = 6;
        private const int SeedCheck2_FourthIterationNodeCount = 15;
        private const int SeedCheck2_FourthIterationSeedNodeCount = 5;
        private const ReliabilityLevel SeedCheck2_TargetdReliabilityLevel = ReliabilityLevel.Silver;
        private const int SeedCheck2_TargetTotalNodeCount = 9;
        private const string SeedCheck2_BaselineClusterManifestVersion = "2.0.0";
        private const string SeedCheck2_TargetClusterManifestVersion = "1.0.0";
        private const long SeedCheck2_TargetNodeConfigVersion = 3;

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollForwardTest_SeedCheckWithReliabilityLevelChangeScaleDown()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("ClusterConfig.Unsecure.MultiMachine.AutoScaleSeedCheckV2.json");
            var seedNodes = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Select(node => node.NodeName).ToList();

            Utility.UpdateStandAloneClusterForDisableNode("ClusterConfig.Unsecure.MultiMachine.AutoScaleSeedCheckV1.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));
            var targetSeedNodes1 = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Select(node => node.NodeName).ToList();
            Assert.IsTrue(seedNodes.Count() - targetSeedNodes1.Count() == 1);
            Assert.IsTrue(targetSeedNodes1.All(n => seedNodes.Contains(n)));

            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_BaselineTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_BaselineTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_FirstIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_FirstIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_TargetdReliabilityLevel, cluster.TargetCsmConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_BaselineTotalNodeCount, cluster.TargetNodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_BaselineClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);

            //Need to set targetReplicaSetSize to an empty dictionary, in order to get the system think the replica set size is updated. 
            //Otherwise VerifyTargetSystemServicesReplicaSetSize() will fail.
            Utility.SetTargetReplicaSetSize(upgradeState);

            Utility.RollForwardOneIteration(cluster);
            var targetSeedNodes2 = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Select(node => node.NodeName).ToList();
            Assert.IsTrue(targetSeedNodes1.Count() - targetSeedNodes2.Count() == 1);
            Assert.IsTrue(targetSeedNodes2.All(n => targetSeedNodes1.Contains(n)));
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_SecondIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_SecondIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());

            Utility.RollForwardOneIteration(cluster);
            var targetSeedNodes3 = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Select(node => node.NodeName).ToList();
            Assert.IsTrue(targetSeedNodes2.Count() - targetSeedNodes3.Count() == 1);
            Assert.IsTrue(targetSeedNodes3.All(n => targetSeedNodes2.Contains(n)));
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_ThirdIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_ThirdIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());

            Utility.RollForwardOneIteration(cluster);
            var targetSeedNodes4 = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Select(node => node.NodeName).ToList();
            Assert.IsTrue(targetSeedNodes3.Count() - targetSeedNodes4.Count() == 1);
            Assert.IsTrue(targetSeedNodes4.All(n => targetSeedNodes3.Contains(n)));
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_FourthIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_FourthIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());

            Utility.RollForwardOneIteration(cluster);
            Assert.IsNull(cluster.Pending);
            Assert.IsFalse(cluster.RunStateMachine());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_TargetClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_TargetTotalNodeCount, cluster.Current.CSMConfig.TotalNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_TargetTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_FourthIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_FourthIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());

            Utility.UpdateStandAloneClusterForRemoveNode("ClusterConfig.Unsecure.MultiMachine.AutoScaleSeedCheckV1.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());
            upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_TargetTotalNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());

            Utility.RollForwardOneIteration(cluster);

            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_TargetNodeConfigVersion, cluster.Current.NodeConfig.Version);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_TargetTotalNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());

            Assert.IsNull(cluster.Pending);
            Assert.IsFalse(cluster.RunStateMachine());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollBackTest_ReliabilityLevelChangeScaleDown()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("ClusterConfig.Unsecure.MultiMachine.AutoScaleSeedCheckV2.json");
            var seedNodes = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Select(node => node.NodeName).ToList();

            Utility.UpdateStandAloneClusterForDisableNode("ClusterConfig.Unsecure.MultiMachine.AutoScaleSeedCheckV1.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));
            var targetSeedNodes1 = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Select(node => node.NodeName).ToList();
            Assert.IsTrue(seedNodes.Count() - targetSeedNodes1.Count() == 1);
            Assert.IsTrue(targetSeedNodes1.All(n => seedNodes.Contains(n)));
 
            //Fail upgrade in 1st step
            Utility.RollBackOneIteration(cluster);
            Assert.IsFalse(Utility.RunStateMachine(cluster));
            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
 
            // Manually try to do upgrade again
            cluster.Reset();
            Utility.UpdateStandAloneClusterForDisableNode("ClusterConfig.Unsecure.MultiMachine.AutoScaleSeedCheckV1.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());
            upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));

            upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));
            targetSeedNodes1 = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Select(node => node.NodeName).ToList();
            Assert.IsTrue(seedNodes.Count() - targetSeedNodes1.Count() == 1);
            Assert.IsTrue(targetSeedNodes1.All(n => seedNodes.Contains(n)));
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_BaselineTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_BaselineTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_FirstIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_FirstIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_TargetdReliabilityLevel, cluster.TargetCsmConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_BaselineTotalNodeCount, cluster.TargetNodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_BaselineClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.RollForwardOneIteration(cluster);
            var targetSeedNodes2 = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Select(node => node.NodeName).ToList();
            Assert.IsTrue(targetSeedNodes1.Count() - targetSeedNodes2.Count() == 1);
            Assert.IsTrue(targetSeedNodes2.All(n => targetSeedNodes1.Contains(n)));
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_SecondIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_SecondIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());

            //Fail upgrade in 2nd step
            Utility.RollBackOneIteration(cluster);
            Assert.IsFalse(Utility.RunStateMachine(cluster));
            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_FirstIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());

            // Manually try to do upgrade again
            cluster.Reset();
            Utility.UpdateStandAloneClusterForDisableNode("ClusterConfig.Unsecure.MultiMachine.AutoScaleSeedCheckV1.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());
            upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));

            targetSeedNodes2 = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Select(node => node.NodeName).ToList();
            Assert.IsTrue(targetSeedNodes1.Count() - targetSeedNodes2.Count() == 1);
            Assert.IsTrue(targetSeedNodes2.All(n => targetSeedNodes1.Contains(n)));
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_SecondIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_SecondIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());

            Utility.RollForwardOneIteration(cluster);
            var targetSeedNodes3 = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Select(node => node.NodeName).ToList();
            Assert.IsTrue(targetSeedNodes2.Count() - targetSeedNodes3.Count() == 1);
            Assert.IsTrue(targetSeedNodes3.All(n => targetSeedNodes2.Contains(n)));
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_ThirdIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_ThirdIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());

            //Fail upgrade in 3rd step
            Utility.RollBackOneIteration(cluster);
            Assert.IsFalse(Utility.RunStateMachine(cluster));
            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_SecondIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());

            // Manually try to do upgrade again
            cluster.Reset();
            Utility.UpdateStandAloneClusterForDisableNode("ClusterConfig.Unsecure.MultiMachine.AutoScaleSeedCheckV1.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());
            upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));

            targetSeedNodes3 = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Select(node => node.NodeName).ToList();
            Assert.IsTrue(targetSeedNodes2.Count() - targetSeedNodes3.Count() == 1);
            Assert.IsTrue(targetSeedNodes3.All(n => targetSeedNodes2.Contains(n)));
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_ThirdIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_ThirdIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());

            Utility.RollForwardOneIteration(cluster);
            var targetSeedNodes4 = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Select(node => node.NodeName).ToList();
            Assert.IsTrue(targetSeedNodes3.Count() - targetSeedNodes4.Count() == 1);
            Assert.IsTrue(targetSeedNodes4.All(n => targetSeedNodes3.Contains(n)));
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_FourthIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_FourthIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());

            Utility.RollForwardOneIteration(cluster);

            //Need to set targetReplicaSetSize to an empty dictionary, in order to get the system think the replica set size is updated. 
            //Otherwise VerifyTargetSystemServicesReplicaSetSize() will fail.
            Utility.SetTargetReplicaSetSize(upgradeState);
            Utility.RollForwardOneIteration(cluster);

            Assert.IsNull(cluster.Pending);
            Assert.IsFalse(cluster.RunStateMachine());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_TargetClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_TargetTotalNodeCount, cluster.Current.CSMConfig.TotalNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_TargetTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_FourthIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_FourthIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());
           
            Utility.UpdateStandAloneClusterForRemoveNode("ClusterConfig.Unsecure.MultiMachine.AutoScaleSeedCheckV1.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());
            upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_TargetTotalNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());

            //Fail upgrade
            Utility.RollBackOneIteration(cluster);
            Assert.IsFalse(Utility.RunStateMachine(cluster));
            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_FourthIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());

            // Manually try to do upgrade again
            cluster.Reset();
            Utility.UpdateStandAloneClusterForRemoveNode("ClusterConfig.Unsecure.MultiMachine.AutoScaleSeedCheckV1.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());
            upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_TargetTotalNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());

            Utility.RollForwardOneIteration(cluster);      
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_TargetNodeConfigVersion, cluster.Current.NodeConfig.Version);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.SeedCheck2_TargetTotalNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());

            Assert.IsNull(cluster.Pending);
            Assert.IsFalse(cluster.RunStateMachine());
        }

        private const ReliabilityLevel AddPrimaryAndNonPrimary3_BaselineReliabilityLevel = ReliabilityLevel.Silver;

        private const int AddPrimaryAndNonPrimary3_BaselineTotalNodeCount = 5;

        private const string AddPrimaryAndNonPrimary3_BaselineClusterManifestVersion = "2.0.0";

        private const string AddPrimaryAndNonPrimary3_FirstIterationClusterManifestVersion = "3.0.0";

        private const ReliabilityLevel AddPrimaryAndNonPrimary3_TargetdReliabilityLevel = ReliabilityLevel.Gold;

        private const int AddPrimaryAndNonPrimary3_BaselineTotalPrimaryNodeTypeNodeCount = 5;

        private const ReliabilityLevel AddPrimaryAndNonPrimary3_FirstIterationReliabilityLevel = ReliabilityLevel.Silver;

        private const int AddPrimaryAndNonPrimary3_TargetTotalNodeCount = 11;

        private const int AddPrimaryAndNonPrimary3_TargetPrimaryNodeTypeNodeCount = 8;

        private const int AddPrimaryAndNonPrimary3_FirstIterationNodeCount = 10;

        private const int AddPrimaryAndNonPrimary3_SecondIterationNodeCount = 11;

        private const long AddPrimaryAndNonPrimary3_TargetNodeConfigVersion = 2;

        private const long AddPrimaryAndNonPrimary3_TargetSeedNodeCount = 7;

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollForwardTest_AddPrimaryAndNonPrimaryWithReliabilityLevelChange3()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.X509.DevCluster.Silver.json");
            Utility.UpdateStandAloneClusterForAddNode("myClusterConfig.X509.DevCluster.Gold.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));

            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary3_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary3_BaselineTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary3_BaselineTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary3_FirstIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary3_TargetdReliabilityLevel, cluster.TargetCsmConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary3_TargetTotalNodeCount, cluster.TargetNodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary3_BaselineClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);

            //Need to set targetReplicaSetSize to an empty dictionary, in order to get the system think the replica set size is updated. 
            //Otherwise VerifyTargetSystemServicesReplicaSetSize() will fail.
            Utility.SetTargetReplicaSetSize(upgradeState);

            Utility.RollForwardOneIteration(cluster);
            Assert.IsNotNull(cluster.Pending);
            upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));

            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary3_FirstIterationReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary3_SecondIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary3_TargetdReliabilityLevel, cluster.TargetCsmConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary3_TargetTotalNodeCount, cluster.TargetNodeConfig.NodesStatus.Count);

            Utility.RollForwardOneIteration(cluster);
            Assert.IsNull(cluster.Pending);
            Assert.IsFalse(cluster.RunStateMachine());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary3_FirstIterationClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary3_TargetTotalNodeCount, cluster.Current.CSMConfig.TotalNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary3_TargetPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary3_TargetNodeConfigVersion, cluster.Current.NodeConfig.Version);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary3_TargetTotalNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary3_TargetSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster
                .Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());
        }

        private const ReliabilityLevel AddPrimaryAndNonPrimary2_BaselineReliabilityLevel = ReliabilityLevel.Bronze;

        private const int AddPrimaryAndNonPrimary2_BaselineTotalNodeCount = 3;

        private const string AddPrimaryAndNonPrimary2_BaselineClusterManifestVersion = "1.0.0";

        private const string AddPrimaryAndNonPrimary2_FirstIterationClusterManifestVersion = "2.0.0";

        private const ReliabilityLevel AddPrimaryAndNonPrimary2_TargetdReliabilityLevel = ReliabilityLevel.Bronze;

        private const int AddPrimaryAndNonPrimary2_BaselineTotalPrimaryNodeTypeNodeCount = 3;

        private const int AddPrimaryAndNonPrimary2_TargetTotalNodeCount = 7;

        private const int AddPrimaryAndNonPrimary2_TargetPrimaryNodeTypeNodeCount = 4;

        private const int AddPrimaryAndNonPrimary2_FirstIterationNodeCount = 7;

        private const long AddPrimaryAndNonPrimary2_TargetNodeConfigVersion = 2;

        private const int AddPrimaryAndNonPrimary2_TargetNodeCount = 7;

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollForwardTest_AddPrimaryAndNonPrimaryWithoutReliabilityLevelChange()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.X509.DevCluster.Bronze.V2.json");
            Utility.UpdateStandAloneClusterForAddNode("myClusterConfig.X509.DevCluster.Bronze.AddPrimaryAndNonPrimary.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));

            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary2_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary2_BaselineTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary2_BaselineTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary2_FirstIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary2_TargetdReliabilityLevel, cluster.TargetCsmConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary2_TargetTotalNodeCount, cluster.TargetNodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary2_BaselineClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);

            Utility.RollForwardOneIteration(cluster);
            Assert.IsNull(cluster.Pending);
            Assert.IsFalse(cluster.RunStateMachine());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary2_FirstIterationClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary2_TargetTotalNodeCount, cluster.Current.CSMConfig.TotalNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary2_TargetPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary2_TargetNodeConfigVersion, cluster.Current.NodeConfig.Version);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.AddPrimaryAndNonPrimary2_TargetNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
        }

        private const ReliabilityLevel RemovePrimary_BaselineReliabilityLevel = ReliabilityLevel.Silver;

        private const string RemovePrimary_BaselineClusterManifestVersion = "1";

        private const ReliabilityLevel RemovePrimary_TargetdReliabilityLevel = ReliabilityLevel.Bronze;

        private const int RemovePrimary_FirstIterationSeedNode = 4;

        private const int RemovePrimary_SecondIterationSeedNode = 3;

        private const ReliabilityLevel RemovePrimary_FirstIterationReliabilityLevel = ReliabilityLevel.Bronze;

        private const int RemovePrimary_TargetTotalNodeCount = 3;

        private const int RemovePrimary_TargetSeedNodeCount = 3;

        private const int RemovePrimary_FirstIterationNodeCount = 5;

        private const int RemovePrimary_FinalIterationNodeCount = 3;

        private const int RemovePrimary_SecondIterationNodeCount = 5;


        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollForwardTest_RemovePrimaryWithReliabilityLevelChange()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.X509.DevCluster.Silver.AddPrimary.json");
            Utility.UpdateStandAloneClusterForDisableNode("myClusterConfig.X509.DevCluster.Bronze.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());

            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimary_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimary_FirstIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimary_FirstIterationSeedNode, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Where(node => node.IsSeedNode).Count());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;

            //Need to set targetReplicaSetSize to an empty dictionary, in order to get the system think the replica set size is updated. 
            //Otherwise VerifyTargetSystemServicesReplicaSetSize() will fail.
            Utility.SetTargetReplicaSetSize(upgradeState);

            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));
            Utility.RollForwardOneIteration(cluster);

            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimary_SecondIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimary_SecondIterationSeedNode, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Where(node => node.IsSeedNode).Count());

            while (cluster.Pending != null)
            {
                upgradeState = cluster.Pending;

                //Need to set targetReplicaSetSize to an empty dictionary, in order to get the system think the replica set size is updated. 
                //Otherwise VerifyTargetSystemServicesReplicaSetSize() will fail.
                Utility.SetTargetReplicaSetSize(upgradeState);

                Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));
                Utility.RollForwardOneIteration(cluster);
            }

            var seednodes = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Where(n => n.IsSeedNode).Select(q => q.NodeName);
            Assert.IsTrue(!seednodes.Contains("vm3") && !seednodes.Contains("vm4") && seednodes.Contains("vm1") && seednodes.Contains("vm2") && seednodes.Contains("vm0"));

            Utility.UpdateStandAloneClusterForRemoveNode("myClusterConfig.X509.DevCluster.Bronze.json", cluster);
            cluster.RunStateMachine();

            Assert.IsNotNull(cluster.Pending);
            upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimary_TargetSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster
                .Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());

            Utility.RollForwardOneIteration(cluster);

            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimary_FinalIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimary_TargetdReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimary_TargetSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster
                .Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());
            Assert.IsNull(cluster.Pending);
            Assert.IsFalse(cluster.RunStateMachine());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollForwardTest_RemovePrimaryWithReliabilityLevelChangeWithCustomDeserealisation()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.X509.DevCluster.Silver.AddPrimary.json");
            Utility.UpdateStandAloneClusterForDisableNode("myClusterConfig.X509.DevCluster.Bronze.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());

            var defaultSerializedCluster = Utility.SerealizeUsingDefaults(cluster);
            cluster = Utility.DeserealizeUsingCustomDeserealizer(defaultSerializedCluster);

            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimary_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimary_FirstIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimary_FirstIterationSeedNode, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Where(node => node.IsSeedNode).Count());

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));
            Utility.RollForwardOneIteration(cluster);

            defaultSerializedCluster = Utility.SerealizeUsingDefaults(cluster);
            cluster = Utility.DeserealizeUsingCustomDeserealizer(defaultSerializedCluster);

            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimary_SecondIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimary_SecondIterationSeedNode, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Where(node => node.IsSeedNode).Count());

            upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));    
            Utility.RollForwardOneIteration(cluster);

            //Need to set targetReplicaSetSize to an empty dictionary, in order to get the system think the replica set size is updated. 
            //Otherwise VerifyTargetSystemServicesReplicaSetSize() will fail.
            Utility.SetTargetReplicaSetSize(upgradeState);
            Utility.RollForwardOneIteration(cluster);

            defaultSerializedCluster = Utility.SerealizeUsingDefaults(cluster);
            cluster = Utility.DeserealizeUsingCustomDeserealizer(defaultSerializedCluster);
            
            var seednodes = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Where(n => n.IsSeedNode).Select(q => q.NodeName);
            Assert.IsTrue(!seednodes.Contains("vm3") && !seednodes.Contains("vm4") && seednodes.Contains("vm1") && seednodes.Contains("vm2") && seednodes.Contains("vm0"));

            Utility.UpdateStandAloneClusterForRemoveNode("myClusterConfig.X509.DevCluster.Bronze.json", cluster);
            cluster.RunStateMachine();

            defaultSerializedCluster = Utility.SerealizeUsingDefaults(cluster);
            cluster = Utility.DeserealizeUsingCustomDeserealizer(defaultSerializedCluster);

            Assert.IsNotNull(cluster.Pending);
            upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimary_TargetSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster
                .Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());

            Utility.RollForwardOneIteration(cluster);

            defaultSerializedCluster = Utility.SerealizeUsingDefaults(cluster);
            cluster = Utility.DeserealizeUsingCustomDeserealizer(defaultSerializedCluster);

            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimary_FinalIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimary_TargetdReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimary_TargetSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster
                .Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());
            Assert.IsNull(cluster.Pending);
            Assert.IsFalse(cluster.RunStateMachine());
        }

        private const ReliabilityLevel RemoveSeedNode_BaselineReliabilityLevel = ReliabilityLevel.Bronze;

        private const int RemoveSeedNode_BaselineTotalNodeCount = 4;

        private const int RemoveSeedNode_BaselineTotalPrimaryNodeTypeNodeCount = 4;

        private const int RemoveSeedNode_TargetTotalNodeCount = 3;

        private const int RemoveSeedNode_FirstIterationNodeCount = 4;

        private const int RemoveSeedNode_FirstIterationSeedNodeCount = 4;

        private const long RemoveSeedNode_TargetNodeConfigVersion = 3;

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollForwardTest_RemoveOneSeedNodeWithoutReliabilityLevelChange()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.X509.DevCluster.Bronze.4Nodes.json");
            Utility.UpdateStandAloneClusterForDisableNode("myClusterConfig.X509.DevCluster.Bronze.3Nodes.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());

            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveSeedNode_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveSeedNode_BaselineTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveSeedNode_FirstIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveSeedNode_FirstIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Where(node => node.IsSeedNode).Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveSeedNode_BaselineTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(cluster.Pending, typeof(StandAloneAutoScaleClusterUpgradeState));
            Utility.RollForwardOneIteration(cluster);

            while (cluster.Pending != null)
            {
                upgradeState = cluster.Pending;

                //Need to set targetReplicaSetSize to an empty dictionary, in order to get the system think the replica set size is updated. 
                //Otherwise VerifyTargetSystemServicesReplicaSetSize() will fail.
                Utility.SetTargetReplicaSetSize(upgradeState);

                Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));
                Utility.RollForwardOneIteration(cluster);
            }

            var seednodes = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Where(n => n.IsSeedNode).Select(q => q.NodeName);
            Assert.IsTrue(!seednodes.Contains("vm0") && seednodes.Contains("vm1") && seednodes.Contains("vm2") && seednodes.Contains("vm3"));

            Utility.UpdateStandAloneClusterForRemoveNode("myClusterConfig.X509.DevCluster.Bronze.3Nodes.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());
            upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveSeedNode_TargetTotalNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());

            Utility.RollForwardOneIteration(cluster);

            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveSeedNode_TargetNodeConfigVersion, cluster.Current.NodeConfig.Version);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveSeedNode_TargetTotalNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());

            Assert.IsNull(cluster.Pending);
            Assert.IsFalse(cluster.RunStateMachine());
        }

        private const ReliabilityLevel RemoveNonPrimary_BaselineReliabilityLevel = ReliabilityLevel.Bronze;

        private const int RemoveNonPrimary_BaselineTotalNodeCount = 7;

        private const int RemoveNonPrimary_BaselineTotalPrimaryNodeTypeNodeCount = 3;

        private const int RemoveNonPrimary_TargetTotalNodeCount = 3;

        private const long RemoveNonPrimary_TargetNodeConfigVersion = 3;

        private const long RemoveNonPrimary_SecondIterationNodeCount = 3;

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollForwardTest_RemoveNonPrimary()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.X509.DevCluster.Bronze.Add4NonPrimary.json");
            Utility.UpdateStandAloneClusterForDisableNode("myClusterConfig.X509.DevCluster.Bronze.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());

            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveNonPrimary_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveNonPrimary_BaselineTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveNonPrimary_BaselineTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.IsNull(cluster.Pending);

            Utility.UpdateStandAloneClusterForRemoveNode("myClusterConfig.X509.DevCluster.Bronze.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());
            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveNonPrimary_SecondIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());

            Utility.RollForwardOneIteration(cluster);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveNonPrimary_TargetNodeConfigVersion, cluster.Current.NodeConfig.Version);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveNonPrimary_TargetTotalNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.IsTrue(((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Select(n => n.NodeName).Contains("vm0"));
            Assert.IsTrue(((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Select(n => n.NodeName).Contains("vm1"));
            Assert.IsTrue(((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Select(n => n.NodeName).Contains("vm2"));
            Assert.IsNull(cluster.Pending);
            Assert.IsFalse(cluster.RunStateMachine());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollBackTest_RemoveNonPrimary()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.X509.DevCluster.Bronze.Add4NonPrimary.json");
            Utility.UpdateStandAloneClusterForDisableNode("myClusterConfig.X509.DevCluster.Bronze.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());

            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveNonPrimary_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveNonPrimary_BaselineTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveNonPrimary_BaselineTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.IsNull(cluster.Pending);

            Utility.UpdateStandAloneClusterForRemoveNode("myClusterConfig.X509.DevCluster.Bronze.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());
            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveNonPrimary_SecondIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());

            // Fail upgrade
            Utility.RollBackOneIteration(cluster);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveNonPrimary_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveNonPrimary_BaselineTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveNonPrimary_BaselineTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);

            // Manually try to do upgrade again
            cluster.Reset();
            Utility.UpdateStandAloneClusterForRemoveNode("myClusterConfig.X509.DevCluster.Bronze.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());
            upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveNonPrimary_SecondIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());

            Utility.RollForwardOneIteration(cluster);

            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveNonPrimary_TargetNodeConfigVersion, cluster.Current.NodeConfig.Version);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveNonPrimary_TargetTotalNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.IsTrue(((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Select(n => n.NodeName).Contains("vm0"));
            Assert.IsTrue(((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Select(n => n.NodeName).Contains("vm1"));
            Assert.IsTrue(((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Select(n => n.NodeName).Contains("vm2"));
            Assert.IsNull(cluster.Pending);
            Assert.IsFalse(cluster.RunStateMachine());
        }

        private const ReliabilityLevel RemoveNonPrimary2_BaselineReliabilityLevel = ReliabilityLevel.Bronze;

        private const int RemoveNonPrimary2_BaselineTotalNodeCount = 5;

        private const int RemoveNonPrimary2_BaselineTotalPrimaryNodeTypeNodeCount = 4;

        private const int RemoveNonPrimary2_TargetTotalNodeCount = 4;

        private const long RemoveNonPrimary2_TargetNodeConfigVersion = 3;

        private const long RemoveNonPrimary2_SecondIterationNodeCount = 4;

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollForwardTest_RemoveNonPrimary2()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.X509.DevCluster.Bronze.5Nodes2.json");
            Utility.UpdateStandAloneClusterForDisableNode("myClusterConfig.X509.DevCluster.Bronze.4Nodes2.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());

            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveNonPrimary2_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveNonPrimary2_BaselineTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveNonPrimary2_BaselineTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            var seednodes = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Select(n => n.IsSeedNode);

            Assert.IsNull(cluster.Pending);
            
            Utility.UpdateStandAloneClusterForRemoveNode("myClusterConfig.X509.DevCluster.Bronze.4Nodes2.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());
            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveNonPrimary2_SecondIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());

            Utility.RollForwardOneIteration(cluster);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveNonPrimary2_TargetNodeConfigVersion, cluster.Current.NodeConfig.Version);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemoveNonPrimary2_TargetTotalNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.IsNull(cluster.Pending);
            Assert.IsFalse(cluster.RunStateMachine());
            var finalSeedNodes = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Select(n => n.IsSeedNode);
            Assert.IsTrue(finalSeedNodes.Except(seednodes).Count() == 0);

        }

        private const ReliabilityLevel RemovePrimaryAndNonPrimary_BaselineReliabilityLevel = ReliabilityLevel.Silver;

        private const int RemovePrimaryAndNonPrimary_BaselineTotalNodeCount = 8;

        private const string RemovePrimaryAndNonPrimary_FirstIterationClusterManifestVersion = "1.0.0";

        private const ReliabilityLevel RemovePrimaryAndNonPrimary_TargetdReliabilityLevel = ReliabilityLevel.Bronze;

        private const int RemovePrimaryAndNonPrimary_BaselineTotalPrimaryNodeTypeNodeCount = 5;

        private const ReliabilityLevel RemovePrimaryAndNonPrimary_FirstIterationReliabilityLevel = ReliabilityLevel.Silver;

        private const int RemovePrimaryAndNonPrimary_TargetTotalNodeCount = 3;

        private const int RemovePrimaryAndNonPrimary_TargetSeedNodeCount = 3;

        private const long RemovePrimaryAndNonPrimary_TargetNodeConfigVersion = 3;

        private const int RemovePrimaryAndNonPrimary_FinalIterationNodeCount = 3;

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollForwardTest_RemovePrimaryAndNonPrimaryWithReliabilityLevelChange()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.X509.DevCluster.Silver.AddPrimaryAndNonPrimary.json");
            Utility.UpdateStandAloneClusterForDisableNode("myClusterConfig.X509.DevCluster.Bronze.V2.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());

            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimaryAndNonPrimary_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimaryAndNonPrimary_BaselineTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimaryAndNonPrimary_BaselineTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);

            ClusterUpgradeStateBase upgradeState;

            while (cluster.Pending != null)
            {
                upgradeState = cluster.Pending;
                Utility.SetTargetReplicaSetSize(upgradeState);
                Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));
                Utility.RollForwardOneIteration(cluster);
            }

            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimaryAndNonPrimary_TargetdReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimaryAndNonPrimary_BaselineTotalNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.IsTrue(((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Where(node => node.IsSeedNode).Select(n => n.NodeName).Contains("vm0"));
            Assert.IsTrue(((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Where(node => node.IsSeedNode).Select(n => n.NodeName).Contains("vm1"));
            Assert.IsTrue(((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Where(node => node.IsSeedNode).Select(n => n.NodeName).Contains("vm2"));
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimaryAndNonPrimary_TargetSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Where(node => node.IsSeedNode).Count());

            Utility.UpdateStandAloneClusterForRemoveNode("myClusterConfig.X509.DevCluster.Bronze.V2.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());
            Assert.IsNotNull(cluster.Pending);
            upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));

            Utility.RollForwardOneIteration(cluster);
            Assert.IsNull(cluster.Pending);
            Assert.IsFalse(cluster.RunStateMachine());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimaryAndNonPrimary_FirstIterationClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimaryAndNonPrimary_TargetTotalNodeCount, cluster.Current.CSMConfig.TotalNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimaryAndNonPrimary_TargetSeedNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimaryAndNonPrimary_TargetNodeConfigVersion, cluster.Current.NodeConfig.Version);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimaryAndNonPrimary_FinalIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
        }

        private const ReliabilityLevel RemovePrimaryAndNonPrimary3_BaselineReliabilityLevel = ReliabilityLevel.Gold;

        private const string RemovePrimaryAndNonPrimary3_BaselineClusterManifestVersion = "3.0.0";

        private const string RemovePrimaryAndNonPrimary3_FirstIterationClusterManifestVersion = "2.0.0";

        private const ReliabilityLevel RemovePrimaryAndNonPrimary3_TargetdReliabilityLevel = ReliabilityLevel.Silver;

        private const int RemovePrimaryAndNonPrimary3_BaselineTotalPrimaryNodeTypeNodeCount = 8;

        private const int RemovePrimaryAndNonPrimary3_TargetTotalNodeCount = 5;

        private const int RemovePrimaryAndNonPrimary3_TargetTotalPrimaryNodeTypeNodeCount = 5;

        private const int RemovePrimaryAndNonPrimary3_FirstIterationNodeCount = 11;

        private const long RemovePrimaryAndNonPrimary3_TargetNodeConfigVersion = 3;

        private const int RemovePrimaryAndNonPrimary3_FinalIterationNodeCount = 5;

        private const int RemovePrimaryAndNonPrimary3_TargetSeedNodeCount = 5;

        private const int RemovePrimaryAndNonPrimary3_FirstIterationSeedNodeCount = 6;


        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollForwardTest_RemovePrimaryAndNonPrimaryWithReliabilityLevelChange3()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.X509.DevCluster.Gold.json");
            Utility.UpdateStandAloneClusterForDisableNode("myClusterConfig.X509.DevCluster.Silver.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());

            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimaryAndNonPrimary3_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimaryAndNonPrimary3_BaselineTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimaryAndNonPrimary3_FirstIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimaryAndNonPrimary3_FirstIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster
                .Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimaryAndNonPrimary3_TargetdReliabilityLevel, cluster.TargetCsmConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimaryAndNonPrimary3_BaselineClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);

            ClusterUpgradeStateBase upgradeState;

            while (cluster.Pending != null)
            {
                upgradeState = cluster.Pending;
                Utility.SetTargetReplicaSetSize(upgradeState);
                Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));
                Assert.IsTrue(cluster.Pending.ExternalState.ClusterManifest.FabricSettings.FirstOrDefault(setting =>
                setting.Name == "FailoverManager").Parameter.FirstOrDefault(param => param.Name.Equals("ExpectedNodeDeactivationDuration")).Value.Equals("172800"));
                Utility.RollForwardOneIteration(cluster);
            }

            Utility.UpdateStandAloneClusterForRemoveNode("myClusterConfig.X509.DevCluster.Silver.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());
            Assert.IsNotNull(cluster.Pending);
            upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.IsTrue(((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Where(node => node.IsSeedNode).Select(n => n.NodeName).Contains("vm0"));
            Assert.IsTrue(((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Where(node => node.IsSeedNode).Select(n => n.NodeName).Contains("vm1"));
            Assert.IsTrue(((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Where(node => node.IsSeedNode).Select(n => n.NodeName).Contains("vm2"));
            Assert.IsTrue(((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Where(node => node.IsSeedNode).Select(n => n.NodeName).Contains("vm3"));
            Assert.IsTrue(((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Where(node => node.IsSeedNode).Select(n => n.NodeName).Contains("vm4"));

            Utility.RollForwardOneIteration(cluster);
            Assert.IsNull(cluster.Pending);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimaryAndNonPrimary3_FirstIterationClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimaryAndNonPrimary3_TargetTotalNodeCount, cluster.Current.CSMConfig.TotalNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimaryAndNonPrimary3_TargetTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimaryAndNonPrimary3_TargetNodeConfigVersion, cluster.Current.NodeConfig.Version);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimaryAndNonPrimary3_TargetSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster
                .Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.ToList().Where(node => node.IsSeedNode).Count());
        }

        private const ReliabilityLevel RemovePrimaryAndNonPrimary2_BaselineReliabilityLevel = ReliabilityLevel.Gold;

        private const string RemovePrimaryAndNonPrimary2_FinalIterationClusterManifestVersion = "2.0.0";

        private const int RemovePrimaryAndNonPrimary2_TargetTotalNodeCount = 7;

        private const int RemovePrimaryAndNonPrimary2_TargetSeedNodeCount = 7;

        private const int RemovePrimaryAndNonPrimary2_FinalIterationNodeCount = 7;

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollForwardTest_RemovePrimaryAndNonPrimaryWithoutReliabilityLevelChange()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.X509.DevCluster.Gold.RemovePrimaryAndNonPrimary.json");
            var seednodes = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Where(n => n.IsSeedNode).Select(q => q.NodeName);
            Assert.IsTrue(seednodes.Contains("vm1") && seednodes.Contains("vm2") && seednodes.Contains("vm3") 
                          && seednodes.Contains("vm4") && seednodes.Contains("vm5") && seednodes.Contains("vm6") && seednodes.Contains("vm7"),
                          "Seed node assert failed after Disabling nodes.");

            Utility.UpdateStandAloneClusterForDisableNode("myClusterConfig.X509.DevCluster.Gold.RemovePrimaryAndNonPrimaryV2.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine(), "Running state machine for disable failed.");

            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimaryAndNonPrimary2_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            seednodes = ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Where(n => n.IsSeedNode).Select(q => q.NodeName);
            Assert.IsTrue(seednodes.Contains("vm1") && seednodes.Contains("vm2") && seednodes.Contains("vm3")
                          && seednodes.Contains("vm4") && seednodes.Contains("vm5") && seednodes.Contains("vm6") && seednodes.Contains("vm7"),
                          "Seed node assert failed after Remove nodes.");

            Assert.IsNull(cluster.Pending);

            Utility.UpdateStandAloneClusterForRemoveNode("myClusterConfig.X509.DevCluster.Gold.RemovePrimaryAndNonPrimaryV2.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine(), "Running state machine for remove failed.");
            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneSimpleClusterUpgradeState));
            Assert.IsFalse(cluster.RunStateMachine());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimaryAndNonPrimary2_FinalIterationClusterManifestVersion, cluster.Current.CSMConfig.Version.Version);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimaryAndNonPrimary2_TargetTotalNodeCount, cluster.Current.CSMConfig.TotalNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimaryAndNonPrimary2_TargetSeedNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);

            Utility.RollForwardOneIteration(cluster);
            Assert.IsNull(cluster.Pending);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimaryAndNonPrimary2_FinalIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollbackTest_RemovePrimary()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.X509.DevCluster.Silver.AddPrimary.json");
            Utility.UpdateStandAloneClusterForDisableNode("myClusterConfig.X509.DevCluster.Bronze.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());

            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimary_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimary_FirstIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Pending.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimary_TargetdReliabilityLevel, cluster.TargetCsmConfig.ReliabilityLevel);

            ClusterUpgradeStateBase upgradeState = cluster.Pending;
            Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));

            //Need to set targetReplicaSetSize to an empty dictionary, in order to get the system think the replica set size is updated. 
            //Otherwise VerifyTargetSystemServicesReplicaSetSize() will fail.
            Utility.SetTargetReplicaSetSize(upgradeState);

            Utility.RollForwardOneIteration(cluster);

            Utility.RollBackOneIteration(cluster);
            Assert.IsFalse(Utility.RunStateMachine(cluster));
            Assert.IsNull(cluster.Pending);
            Assert.IsNotNull(cluster.Current);

            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimary_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.RemovePrimary_FirstIterationNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
        }

        private const ReliabilityLevel ReliabilityLevelScaleDown_BaselineReliabilityLevel = ReliabilityLevel.Silver;

        private const int ReliabilityLevelScaleDown_BaselineTotalNodeCount = 7;

        private const string ReliabilityLevelScaleDown_FirstIterationClusterManifestVersion = "1.0.0";

        private const ReliabilityLevel ReliabilityLevelScaleDown_TargetdReliabilityLevel = ReliabilityLevel.Bronze;

        private const int ReliabilityLevelScaleDown_BaselineTotalPrimaryNodeTypeNodeCount = 7;

        private const ReliabilityLevel ReliabilityLevelScaleDown_FirstIterationReliabilityLevel = ReliabilityLevel.Silver;

        private const int ReliabilityLevelScaleDown_TargetTotalNodeCount = 7;

        private const int ReliabilityLevelScaleDown_TargePrimaryNodeCount = 7;

        private const long ReliabilityLevelScaleDown_TargetNodeConfigVersion = 2;

        private const int ReliabilityLevelScaleDown_FinalIterationSeedNodeCount = 3;

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollForwardTest_ReliabilityLevelScaleDown012017()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.X509.DevCluster.Silver.7Nodes.json");
            Utility.UpdateStandAloneClusterForDisableNode("myClusterConfig.X509.DevCluster.Bronze.7Nodes.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());

            Assert.AreEqual(AutoScaleClusterUpgradeTest.ReliabilityLevelScaleDown_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.ReliabilityLevelScaleDown_BaselineTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.ReliabilityLevelScaleDown_BaselineTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);

            ClusterUpgradeStateBase upgradeState;

            while (cluster.Pending != null)
            {
                upgradeState = cluster.Pending;
                Utility.SetTargetReplicaSetSize(upgradeState);
                Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));
                Utility.RollForwardOneIteration(cluster);
            }

            Assert.IsNull(cluster.Pending);
            Assert.IsFalse(cluster.RunStateMachine());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.ReliabilityLevelScaleDown_TargetdReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.ReliabilityLevelScaleDown_BaselineTotalNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.ReliabilityLevelScaleDown_FinalIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Where(node => node.IsSeedNode).Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.ReliabilityLevelScaleDown_TargetTotalNodeCount, cluster.Current.CSMConfig.TotalNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.ReliabilityLevelScaleDown_TargePrimaryNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.ReliabilityLevelScaleDown_TargetNodeConfigVersion, cluster.Current.NodeConfig.Version);
        }

        private const ReliabilityLevel ReliabilityLevelScaleUp_BaselineReliabilityLevel = ReliabilityLevel.Bronze;

        private const int ReliabilityLevelScaleUp_BaselineTotalNodeCount = 7;

        private const string ReliabilityLevelScaleUp_FirstIterationClusterManifestVersion = "1.0.0";

        private const ReliabilityLevel ReliabilityLevelScaleUp_TargetdReliabilityLevel = ReliabilityLevel.Silver;

        private const int ReliabilityLevelScaleUp_BaselineTotalPrimaryNodeTypeNodeCount = 7;

        private const ReliabilityLevel ReliabilityLevelScaleUp_FirstIterationReliabilityLevel = ReliabilityLevel.Bronze;

        private const int ReliabilityLevelScaleUp_TargetTotalNodeCount = 7;

        private const int ReliabilityLevelScaleUp_TargePrimaryNodeCount = 7;

        private const long ReliabilityLevelScaleUp_TargetNodeConfigVersion = 2;

        private const int ReliabilityLevelScaleUp_FinalIterationSeedNodeCount = 5;

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void RollForwardTest_ReliabilityLevelScaleUp012017()
        {
            StandAloneCluster cluster = Utility.DoBaselineUpgrade("myClusterConfig.X509.DevCluster.Bronze.7Nodes.json");
            Utility.UpdateStandAloneClusterForDisableNode("myClusterConfig.X509.DevCluster.Silver.7Nodes.json", cluster);
            Assert.IsTrue(cluster.RunStateMachine());

            Assert.AreEqual(AutoScaleClusterUpgradeTest.ReliabilityLevelScaleUp_BaselineReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.ReliabilityLevelScaleUp_BaselineTotalNodeCount, cluster.Current.NodeConfig.NodesStatus.Count);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.ReliabilityLevelScaleUp_BaselineTotalPrimaryNodeTypeNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);

            ClusterUpgradeStateBase upgradeState;

            while (cluster.Pending != null)
            {
                upgradeState = cluster.Pending;
                Utility.SetTargetReplicaSetSize(upgradeState);
                Assert.IsInstanceOfType(upgradeState, typeof(StandAloneAutoScaleClusterUpgradeState));
                Utility.RollForwardOneIteration(cluster);
            }

            Assert.IsNull(cluster.Pending);
            Assert.IsFalse(cluster.RunStateMachine());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.ReliabilityLevelScaleUp_TargetdReliabilityLevel, cluster.Current.CSMConfig.ReliabilityLevel);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.ReliabilityLevelScaleUp_BaselineTotalNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.ReliabilityLevelScaleUp_FinalIterationSeedNodeCount, ((ClusterManifestTypeInfrastructureWindowsServer)cluster.Current.ExternalState.ClusterManifest.Infrastructure.Item).NodeList.Where(node => node.IsSeedNode).Count());
            Assert.AreEqual(AutoScaleClusterUpgradeTest.ReliabilityLevelScaleUp_TargetTotalNodeCount, cluster.Current.CSMConfig.TotalNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.ReliabilityLevelScaleUp_TargePrimaryNodeCount, cluster.Current.CSMConfig.TotalPrimaryNodeTypeNodeCount);
            Assert.AreEqual(AutoScaleClusterUpgradeTest.ReliabilityLevelScaleUp_TargetNodeConfigVersion, cluster.Current.NodeConfig.Version);
        }
    }
}
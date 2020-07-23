// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System.Fabric.Management.ServiceModel;
using System.Fabric.FabricDeployer;
using System.Fabric.Management.FabricDeployer;
using WEX.TestExecution;

namespace System.Fabric.Management.Test
{
    [TestClass]
    class FabricValidatorWrapperTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestSeedNodesChangeValidation()
        {
            var clusterManifest = new ClusterManifestHelper(true, false).ClusterManifest;
            ClusterManifestTypeInfrastructureWindowsServer infra = new ClusterManifestTypeInfrastructureWindowsServer();
            infra.IsScaleMin = true;
            infra.NodeList = new FabricNodeType[3];
            infra.NodeList[0] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node1",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = true
            };
            infra.NodeList[1] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node2",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = true
            };
            infra.NodeList[2] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node3",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = false
            };
            clusterManifest.Infrastructure.Item = infra;

            var targetClusterManifest = new ClusterManifestHelper(true, false).ClusterManifest;
            ClusterManifestTypeInfrastructureWindowsServer infraT = new ClusterManifestTypeInfrastructureWindowsServer();
            infraT.IsScaleMin = true;
            infraT.NodeList = new FabricNodeType[3];
            infraT.NodeList[0] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node1",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = true
            };
            infraT.NodeList[1] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node2",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = false
            };
            infraT.NodeList[2] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node3",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = true
            };

            targetClusterManifest.Infrastructure.Item = infraT;

            var dParameters = new DeploymentParameters();
            Infrastructure infrastructure = Infrastructure.Create(targetClusterManifest.Infrastructure, null, targetClusterManifest.NodeTypes);

            Verify.Throws<ClusterManifestValidationException>(
                () => FabricValidatorWrapper.CompareAndAnalyze(
                    clusterManifest,
                    targetClusterManifest,
                    infrastructure,
                    dParameters));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestSeedNodesDoNotChangeValidation()
        {
            var clusterManifest = new ClusterManifestHelper(true, false).ClusterManifest;
            ClusterManifestTypeInfrastructureWindowsServer infra = new ClusterManifestTypeInfrastructureWindowsServer();
            infra.IsScaleMin = true;
            infra.NodeList = new FabricNodeType[3];
            infra.NodeList[0] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node1",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = true
            };
            infra.NodeList[1] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node2",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = true
            };
            infra.NodeList[2] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node3",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = false
            };
            clusterManifest.Infrastructure.Item = infra;

            var targetClusterManifest = new ClusterManifestHelper(true, false).ClusterManifest;
            ClusterManifestTypeInfrastructureWindowsServer infraT = new ClusterManifestTypeInfrastructureWindowsServer();
            infraT.IsScaleMin = true;
            infraT.NodeList = new FabricNodeType[3];
            infraT.NodeList[0] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node1",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = true
            };
            infraT.NodeList[1] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node2",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = true
            };
            infraT.NodeList[2] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node3",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = false
            };

            targetClusterManifest.Infrastructure.Item = infraT;

            var dParameters = new DeploymentParameters();
            Infrastructure infrastructure = Infrastructure.Create(targetClusterManifest.Infrastructure, null, targetClusterManifest.NodeTypes);

            Verify.Throws<FabricHostRestartNotRequiredException>(
                () => FabricValidatorWrapper.CompareAndAnalyze(
                    clusterManifest,
                    targetClusterManifest,
                    infrastructure,
                    dParameters));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestSeedNodesChangeWhenAddValidation()
        {
            var clusterManifest = new ClusterManifestHelper(true, false).ClusterManifest;
            ClusterManifestTypeInfrastructureWindowsServer infra = new ClusterManifestTypeInfrastructureWindowsServer();
            infra.IsScaleMin = true;
            infra.NodeList = new FabricNodeType[2];
            infra.NodeList[0] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node1",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = true
            };
            infra.NodeList[1] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node2",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = true
            };
            clusterManifest.Infrastructure.Item = infra;

            var targetClusterManifest = new ClusterManifestHelper(true, false).ClusterManifest;
            ClusterManifestTypeInfrastructureWindowsServer infraT = new ClusterManifestTypeInfrastructureWindowsServer();
            infraT.IsScaleMin = true;
            infraT.NodeList = new FabricNodeType[3];
            infraT.NodeList[0] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node1",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = true
            };
            infraT.NodeList[1] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node2",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = false
            };
            infraT.NodeList[2] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node3",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = true
            };

            targetClusterManifest.Infrastructure.Item = infraT;

            var dParameters = new DeploymentParameters();
            Infrastructure infrastructure = Infrastructure.Create(targetClusterManifest.Infrastructure, null, targetClusterManifest.NodeTypes);

            Verify.Throws<ClusterManifestValidationException>(
                () => FabricValidatorWrapper.CompareAndAnalyze(
                    clusterManifest,
                    targetClusterManifest,
                    infrastructure,
                    dParameters));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestSeedNodesChangeWhenRemoveValidation()
        {
            var clusterManifest = new ClusterManifestHelper(true, false).ClusterManifest;
            ClusterManifestTypeInfrastructureWindowsServer infra = new ClusterManifestTypeInfrastructureWindowsServer();
            infra.IsScaleMin = true;
            infra.NodeList = new FabricNodeType[3];
            infra.NodeList[0] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node1",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = true
            };
            infra.NodeList[1] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node2",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = true
            };
            infra.NodeList[2] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node3",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = false
            };
            clusterManifest.Infrastructure.Item = infra;

            var targetClusterManifest = new ClusterManifestHelper(true, false).ClusterManifest;
            ClusterManifestTypeInfrastructureWindowsServer infraT = new ClusterManifestTypeInfrastructureWindowsServer();
            infraT.IsScaleMin = true;
            infraT.NodeList = new FabricNodeType[2];
            infraT.NodeList[0] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node1",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = true
            };
            infraT.NodeList[1] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node3",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = true
            };

            targetClusterManifest.Infrastructure.Item = infraT;

            var dParameters = new DeploymentParameters();
            Infrastructure infrastructure = Infrastructure.Create(targetClusterManifest.Infrastructure, null, targetClusterManifest.NodeTypes);

            Verify.Throws<ClusterManifestValidationException>(
                () => FabricValidatorWrapper.CompareAndAnalyze(
                    clusterManifest,
                    targetClusterManifest,
                    infrastructure,
                    dParameters));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestAddSeedNodeValidation()
        {
            var clusterManifest = new ClusterManifestHelper(true, false).ClusterManifest;
            ClusterManifestTypeInfrastructureWindowsServer infra = new ClusterManifestTypeInfrastructureWindowsServer();
            infra.IsScaleMin = true;
            infra.NodeList = new FabricNodeType[1];
            infra.NodeList[0] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node1",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = true
            };
            clusterManifest.Infrastructure.Item = infra;

            var targetClusterManifest = new ClusterManifestHelper(true, false).ClusterManifest;
            ClusterManifestTypeInfrastructureWindowsServer infraT = new ClusterManifestTypeInfrastructureWindowsServer();
            infraT.IsScaleMin = true;
            infraT.NodeList = new FabricNodeType[2];
            infraT.NodeList[0] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node1",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = true
            };
            infraT.NodeList[1] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node2",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = true
            };

            targetClusterManifest.Infrastructure.Item = infraT;

            var dParameters = new DeploymentParameters();
            Infrastructure infrastructure = Infrastructure.Create(targetClusterManifest.Infrastructure, null, targetClusterManifest.NodeTypes);

            Verify.Throws<FabricHostRestartNotRequiredException>(
                () => FabricValidatorWrapper.CompareAndAnalyze(
                    clusterManifest,
                    targetClusterManifest,
                    infrastructure,
                    dParameters));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestRemoveSeedNodesValidation()
        {
            var clusterManifest = new ClusterManifestHelper(true, false).ClusterManifest;
            ClusterManifestTypeInfrastructureWindowsServer infra = new ClusterManifestTypeInfrastructureWindowsServer();
            infra.IsScaleMin = true;
            infra.NodeList = new FabricNodeType[2];
            infra.NodeList[0] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node1",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = true
            };
            infra.NodeList[1] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node2",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = true
            };
            clusterManifest.Infrastructure.Item = infra;

            var targetClusterManifest = new ClusterManifestHelper(true, false).ClusterManifest;
            ClusterManifestTypeInfrastructureWindowsServer infraT = new ClusterManifestTypeInfrastructureWindowsServer();
            infraT.IsScaleMin = true;
            infraT.NodeList = new FabricNodeType[1];
            infraT.NodeList[0] = new FabricNodeType()
            {
                FaultDomain = "fd:/RACK1",
                UpgradeDomain = "MYUD1",
                NodeName = "Node1",
                NodeTypeRef = "NodeType1",
                IPAddressOrFQDN = "localhost",
                IsSeedNode = true
            };

            targetClusterManifest.Infrastructure.Item = infraT;

            var dParameters = new DeploymentParameters();
            Infrastructure infrastructure = Infrastructure.Create(targetClusterManifest.Infrastructure, null, targetClusterManifest.NodeTypes);

            Verify.Throws<FabricHostRestartNotRequiredException>(
                () => FabricValidatorWrapper.CompareAndAnalyze(
                    clusterManifest,
                    targetClusterManifest,
                    infrastructure,
                    dParameters));
        }
    }
}
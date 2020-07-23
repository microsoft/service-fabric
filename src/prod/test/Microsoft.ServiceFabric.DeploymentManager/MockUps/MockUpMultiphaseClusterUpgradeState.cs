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

    internal class MockUpMultiphaseClusterUpgradeState : MultiphaseClusterUpgradeState
    {
        private int totalIterations;

        public MockUpMultiphaseClusterUpgradeState(int iterations, ICluster cluster)
            : base(cluster.TargetCsmConfig, cluster.TargetWrpConfig, cluster.TargetNodeConfig, cluster, new StandAloneTraceLogger("StandAloneDeploymentManager"))
        {
            Assert.IsTrue(iterations > 0);

            this.totalIterations = iterations;
        }

        public ClusterState RollForwardOneIteration()
        {
            return this.ClusterUpgradeCompleted();
        }

        public ClusterState RollBackOneIteration()
        {
            return this.ClusterUpgradeRolledbackOrFailed();
        }

        protected override ClusterManifestType[] OnStartProcessingMultiphaseUpgrade()
        {
            ClusterManifestType[] result = new ClusterManifestType[this.totalIterations];

            for (int i = 0; i < totalIterations; i++)
            {
                result[i] = new ClusterManifestType();
                result[i].Name = totalIterations.ToString();
            }
            
            return result;
        }

        protected override bool OnMultiphaseUpgradeStarted()
        {
            return true;
        }

        protected override void OnMultiphaseUpgradeCompleted()
        {
        }

        protected override void OnMultiphaseUpgradeRolledbackOrFailed()
        {
        }

        protected override IClusterManifestBuilderActivator ClusterManifestBuilderActivator
        {
            get { throw new NotImplementedException(); }
        }

        public override ClusterUpgradePolicy GetUpgradePolicy()
        {
            return MockUpMultiphaseClusterUpgradeState.ForceUpgradePolicy;
        }

        public override string GetNextClusterManifestVersion()
        {
            throw new NotImplementedException();
        }
    }
}
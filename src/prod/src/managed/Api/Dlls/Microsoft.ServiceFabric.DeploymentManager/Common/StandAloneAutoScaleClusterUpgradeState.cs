// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Common
{
    using System;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Query;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using DMConstants = Microsoft.ServiceFabric.DeploymentManager.Constants;

    internal sealed class StandAloneAutoScaleClusterUpgradeState : AutoScaleClusterUpgradeStateBase
    {
        public StandAloneAutoScaleClusterUpgradeState()
        {
        }

        public StandAloneAutoScaleClusterUpgradeState(
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            ClusterNodeConfig targetNodeConfig,
            ICluster clusterResource,
            ITraceLogger traceLogger)
        : base(targetCsmConfig, targetWrpConfig, targetNodeConfig, clusterResource, traceLogger)
        {
        }

        protected override IClusterManifestBuilderActivator ClusterManifestBuilderActivator
        {
            get
            {
                return StandAloneClusterManifestBuilderActivator.Instance;
            }
        }

        public override string GetNextClusterManifestVersion()
        {
            return StandaloneUtility.GetNextClusterManifestVersionHelper(this.ClusterResource, this.TraceLogger, "StandAloneAutoScaleClusterUpgradeState");
        }

        protected override void Validate()
        {
            base.Validate();

            // StandaloneAutoScaleClusterUpgradeStateSkipQuorumChecksTestHook is set only for test environment.
            if (Environment.GetEnvironmentVariable("StandaloneAutoScaleClusterUpgradeStateSkipQuorumChecksTestHook") == null)
            {
                this.EnsureSeedNodeQuorumForTargetSetAsync().Wait();
            }
        }

        protected override ClusterState OnAutoScaleUpgradeCompleted()
        {
            return new ClusterState(this.TargetCsmConfig, this.TargetWrpConfig, this.TargetNodeConfig, this.ExternalState);
        }

        protected override ClusterState OnAutoScaleUpgradeStarted()
        {
            return new ClusterState(this.TargetCsmConfig, this.TargetWrpConfig, this.TargetNodeConfig, this.ExternalState);
        }

        private async Task EnsureSeedNodeQuorumForTargetSetAsync()
        {
            var finalClusterManifest = this.SeedNodeUpgradeState.ClusterManifestList[this.SeedNodeUpgradeState.ClusterManifestList.Length - 1];
            var targetSeedNodes = ((ClusterManifestTypeInfrastructureWindowsServer)finalClusterManifest.Infrastructure.Item).NodeList.Where(node => node.IsSeedNode).ToArray();
            NodeList nodes = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                                       () =>
                                       new FabricClient().QueryManager.GetNodeListAsync(
                                           null,
                                           DMConstants.FabricQueryNodeListTimeout,
                                           CancellationToken.None),
                                       DMConstants.FabricQueryNodeListRetryTimeout).ConfigureAwait(false);

            int targetSeedUpCount = nodes.Where(n => targetSeedNodes.Any(targetSeedNode => targetSeedNode.NodeName == n.NodeName)
                                                             && n.NodeStatus == System.Fabric.Query.NodeStatus.Up).Count();

            int minRequired = (targetSeedNodes.Count() / 2) + 1;

            if (targetSeedUpCount < minRequired)
            {
                throw new FabricException(string.Format("Sufficient targetSeedNodes are not up to maintain quorum. Upgrade will not be performed.TargetSeedUpCount {0}, MinRequired {1}", targetSeedUpCount, minRequired));
            }
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Linq;
    using Newtonsoft.Json;

    // This class performs all the steps required for Scaling down a cluster in the following order
    // 1. Perform ClusterManifest upgrade to remove seed nodes if required
    // 2. Update the system services replica set size
    // 3. Enable/Disable nodes as required
    [JsonObject]
    internal abstract class ScaleDownClusterUpgradeStateBase : ScaleClusterUpgradeBase
    {
        private static readonly TraceType TraceType = new TraceType("ScaleDownClusterUpgradeState");

        [JsonConstructor]
        protected ScaleDownClusterUpgradeStateBase(
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            ClusterNodeConfig targetNodeConfig,
            ICluster clusterResource,
            Dictionary<string, int> nodeDiffList,
            ITraceLogger traceLogger)
        : base(targetCsmConfig, targetWrpConfig, targetNodeConfig, clusterResource, nodeDiffList, traceLogger)
        {
        }

        [JsonProperty]
        private MultiphaseClusterUpgradeState SeedNodeUpgradeState
        {
            get;
            set;
        }

        public override bool StartProcessing()
        {
            // Get list of already disabled nodes
            List<string> disabledNodes = new List<string>();
            foreach (var nodesPerNodeType in this.ClusterResource.NodeTypeNodeStatusList.Values)
            {
                var nodesDisabledInNodeType = nodesPerNodeType.Values.Where(nodeStatus => nodeStatus.NodeState == NodeState.Disabled)
                                              .Select(nodeStatus => nodeStatus.NodeName);
                disabledNodes.AddRange(nodesDisabledInNodeType);
            }

            MultiphaseClusterUpgradeState seedNodeUpgradeState;
            if (!this.ClusterResource.UpgradeFactory.TryCreateSeedNodeUpgradeState(
                    this.TargetCsmConfig,
                    this.TargetWrpConfig,
                    this.TargetNodeConfig,
                    disabledNodes,
                    null /*removedNodes is specified only for AutoScale*/,
                    out seedNodeUpgradeState))
            {
                ReleaseAssert.Failfast("We should not have a case where enough nodes are not available during scale down.");
            }
            else
            {
                this.SeedNodeUpgradeState = seedNodeUpgradeState;
                this.SeedNodeUpgradeState.ClusterUpgradeStarted();

                this.ExternalState = this.SeedNodeUpgradeState.ExternalState;
                this.PreviousExternalState = this.SeedNodeUpgradeState.PreviousExternalState;
            }

            return true;
        }

        public override ClusterState ClusterUpgradeStarted()
        {
            this.TraceLogger.WriteInfo(
                TraceType,
                "Scaledown cluster upgrade initiated. TargetCsmVersion: {0}, TargetWrpVersion: {1}",
                this.TargetCsmConfig.Version,
                this.TargetWrpConfig.Version);

            this.ClusterResource.ClusterState = ClusterProvisioningState.ScaleDown;

            this.OnScaleDownClusterUpgradeStarted();

            return null;
        }

        public override ClusterState ClusterUpgradeCompleted()
        {
            if (this.SeedNodeUpgradeState != null)
            {
                var clusterState = this.SeedNodeUpgradeState.ClusterUpgradeCompleted();
                if (clusterState == null)
                {
                    // SeedNode upgrade has not completed yet
                    this.ExternalState = this.SeedNodeUpgradeState.ExternalState;
                    this.PreviousExternalState = this.SeedNodeUpgradeState.PreviousExternalState;

                    return null;
                }
                else
                {
                    // SeedNode upgrade has completed
                    this.SeedNodeUpgradeState = null;
                }
            }

            // Initiate system services replica set size adjustment
            // once SeedNode upgrade is done
            this.SetTargetSystemServicesReplicaSetSize();

            if (this.VerifyTargetSystemServicesReplicaSetSize())
            {
                // Disable/Enable nodes once system services replica set size
                // is adjusted
                if (!this.UpdateClusterResourceNodesStatus())
                {
                    this.TraceLogger.WriteInfo(
                        TraceType,
                        "Scaledown cluster upgrade completed. TargetCsmVersion: {0}, TargetWrpVersion: {1}",
                        this.TargetCsmConfig.Version,
                        this.TargetWrpConfig.Version);

                    this.ClusterResource.ClusterState = ClusterProvisioningState.Ready;

                    this.ResetTargetConfigs();

                    this.OnScaleDownClusterUpgradeCompleted();

                    return new ClusterState(this.TargetCsmConfig, this.TargetWrpConfig, this.TargetNodeConfig, this.ExternalState);
                }
            }

            return null;
        }

        public override ClusterState ClusterUpgradeRolledbackOrFailed()
        {
            this.TraceLogger.WriteWarning(
                TraceType,
                "ScaleDownClusterUpgrade rolledback or failed. Retrying the upgrade.");

            // Rollback/Failure during ScaleDown is not supported
            // Keep retrying
            return null;
        }

        protected virtual void OnScaleDownClusterUpgradeStarted()
        {
        }

        protected virtual void OnScaleDownClusterUpgradeCompleted()
        {
        }
    }
}
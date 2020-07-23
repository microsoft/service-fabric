// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System.Collections.Generic;
    using System.Linq;
    using Newtonsoft.Json;

    // This class performs all the steps required for Scaling up a cluster in the following order
    // 1. Enable/Disable nodes as required
    // 2. Perform ClusterManifest upgrade to add seed nodes if required
    // 3. Update the system services replica set size
    [JsonObject]
    internal abstract class ScaleUpClusterUpgradeStateBase : ScaleClusterUpgradeBase
    {
        private static readonly TraceType TraceType = new TraceType("ScaleUpClusterUpgradeState");

        [JsonConstructor]
        protected ScaleUpClusterUpgradeStateBase(
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            ClusterNodeConfig targetNodeConfig,
            ICluster clusterResource,
            Dictionary<string, int> nodeDiffCountList,
            ITraceLogger traceLogger)
        : base(targetCsmConfig, targetWrpConfig, targetNodeConfig, clusterResource, nodeDiffCountList, traceLogger)
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
            // No new ClusterManifest upgrade initiated
            this.ExternalState = this.ClusterResource.Current.ExternalState;
            this.PreviousExternalState = this.ExternalState;
            return true;
        }

        public override ClusterState ClusterUpgradeStarted()
        {
            this.TraceLogger.WriteInfo(
                TraceType,
                "Scaleup cluster upgrade initiated. TargetCsmVersion: {0}, TargetWrpVersion: {1}",
                this.TargetCsmConfig.Version,
                this.TargetWrpConfig.Version);

            this.ClusterResource.ClusterState = ClusterProvisioningState.ScaleUp;

            this.UpdateClusterResourceNodesStatus();

            this.OnScaleUpClusterUpgradeStarted();

            return null;
        }

        public override ClusterState ClusterUpgradeCompleted()
        {
            // Ensure that the Nodes are Disabled/Enabled
            if (this.SeedNodeUpgradeState == null && this.VerifyExpectedTopologyStateReached())
            {
                this.TraceLogger.WriteInfo(
                    TraceType,
                    "Expected node status reached. Initiating Seed Node upgrade.");

                List<string> disabledNodes = new List<string>();
                foreach (var nodesPerNodeType in this.ClusterResource.NodeTypeNodeStatusList.Values)
                {
                    var nodesDisabledInNodeType = nodesPerNodeType.Values.Where(nodeStatus => nodeStatus.NodeState == NodeState.Disabled)
                                                  .Select(nodeStatus => nodeStatus.NodeName);
                    disabledNodes.AddRange(nodesDisabledInNodeType);
                }

                // Disable/Enabled nodes completed. Initiate SeedNode upgrade if required
                MultiphaseClusterUpgradeState seedNodeUpgradeState;
                if (!this.ClusterResource.UpgradeFactory.TryCreateSeedNodeUpgradeState(
                        this.TargetCsmConfig,
                        this.TargetWrpConfig,
                        this.TargetNodeConfig,
                        disabledNodes,
                        null /*removedNodes is specified only for AutoScale*/,
                        out seedNodeUpgradeState))
                {
                    this.TraceLogger.WriteInfo(
                        TraceType,
                        "Enough nodes are not available for scaling up seed nodes.");

                    // enough nodes are not available for scaling up seed nodes
                    return null;
                }
                else
                {
                    this.SeedNodeUpgradeState = seedNodeUpgradeState;
                    this.SeedNodeUpgradeState.ClusterUpgradeStarted();
                }
            }
            else if (this.SeedNodeUpgradeState != null)
            {
                var clusterState = this.SeedNodeUpgradeState.ClusterUpgradeCompleted();
                if (clusterState != null)
                {
                    // SeedNode upgrade is completed. Initiate adjustment of system services replica set size
                    this.SetTargetSystemServicesReplicaSetSize();
                }
            }

            if (this.VerifyTargetSystemServicesReplicaSetSize())
            {
                this.TraceLogger.WriteInfo(
                    TraceType,
                    "Scaleup cluster upgrade completed. TargetCsmVersion: {0}, TargetWrpVersion: {1}",
                    this.TargetCsmConfig.Version,
                    this.TargetWrpConfig.Version);

                this.ClusterResource.ClusterState = ClusterProvisioningState.Ready;

                this.ResetTargetConfigs();

                this.OnScaleUpClusterUpgradeCompleted();

                // return ClusterState to indicate that ScaleUpClusterUpgrade has completed
                return new ClusterState(this.TargetCsmConfig, this.TargetWrpConfig, this.TargetNodeConfig, this.ExternalState);
            }

            if (this.SeedNodeUpgradeState != null)
            {
                // SeedNode upgrade is still going on
                this.ExternalState = this.SeedNodeUpgradeState.ExternalState;
                this.PreviousExternalState = this.SeedNodeUpgradeState.PreviousExternalState;
            }

            return null;
        }

        public override ClusterState ClusterUpgradeRolledbackOrFailed()
        {
            this.TraceLogger.WriteWarning(
                TraceType,
                "ScaleUpClusterUpgrade rolledback or failed. Retrying the upgrade.");

            // Scaleup Upgrade cannot fail. Keep retrying on failure
            return null;
        }

        protected virtual void OnScaleUpClusterUpgradeStarted()
        {
        }

        protected virtual void OnScaleUpClusterUpgradeCompleted()
        {
        }
    }
}
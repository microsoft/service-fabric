// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using Newtonsoft.Json;

    [JsonObject]
    internal abstract class SeedNodeUpgradeStateBase : MultiphaseClusterUpgradeState
    {
        private static readonly TraceType TraceType = new TraceType("ScaleUpClusterUpgradeState");
        private List<string> disabledNodes;
        private List<string> removedNodes;

        [JsonConstructor]
        protected SeedNodeUpgradeStateBase()
        {
        }

        protected SeedNodeUpgradeStateBase(
            List<string> disabledNodes,
            List<string> removedNodes,
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            ClusterNodeConfig targetNodeConfig,
            ICluster clusterResource,
            ITraceLogger traceLogger)
        : base(targetCsmConfig, targetWrpConfig, targetNodeConfig, clusterResource, traceLogger)
        {
            this.disabledNodes = disabledNodes ?? new List<string>();
            this.removedNodes = removedNodes ?? new List<string>();
        }

        protected override ClusterManifestType[] OnStartProcessingMultiphaseUpgrade()
        {
            var manifestBuilder = this.ClusterManifestBuilderActivator.CreateClusterManifestBuilder(
                                this.ClusterResource.Topology,
                                this.ClusterResource.SeedNodeSelector,
                                this.TargetCsmConfig,
                                this.TargetWrpConfig,
                                this.TargetNodeConfig,
                                this /*versionGenerator*/,
                                this.ClusterResource.FabricSettingsActivator,
                                this.ClusterResource.ClusterManifestGeneratorSettings,
                                this.TraceLogger);

            var clusterManifestList = manifestBuilder.UpdateSeedNodesInClusterManifest(
                                          this.ClusterResource.Current.ExternalState.ClusterManifest,
                                          this.ClusterResource.Current.WRPConfig.GetFabricSettingsMetadata(),
                                          this.disabledNodes, 
                                          this.removedNodes);

            if (clusterManifestList == null)
            {
                this.TraceLogger.WriteInfo(TraceType, "Enough nodes are not available to update the seed nodes. TopologyNodeCount:{0}, DisabledNodes:{1}", this.ClusterResource.Topology.Nodes.Count, this.disabledNodes.Count);

                // Enough nodes are not available
                return null;
            }
            else if (clusterManifestList.Length == 0)
            {
                this.TraceLogger.WriteInfo(
                    TraceType,
                    "Seed nodes have not changed. No upgrade required.");

                // No seed node upgrade required
                // Return the current cluster manifest which will cause no real upgrade to happen
                return new[] { this.ClusterResource.Current.ExternalState.ClusterManifest };
            }
            else
            {
                this.TraceLogger.WriteInfo(
                    TraceType,
                    "{0} cluster manifest files generated to reach the target seed node state.",
                    clusterManifestList.Length);

                // Upgrade is required
                return clusterManifestList;
            }
        }

        protected override bool OnMultiphaseUpgradeStarted()
        {
            //// No-op. No state change required
            //// State changes are done by ScaleUpClusterUpgradeState/ScaleDownClusterUpgradeState

            if (this.ExternalState.Equals(this.ClusterResource.Current.ExternalState))
            {
                return false;
            }

            this.TraceLogger.WriteInfo(
                TraceType,
                "SeedNodeUpgrade initiated");

            return true;
        }

        protected override void OnMultiphaseUpgradeCompleted()
        {
            // No-op. No state change required
            // State changes are done by ScaleUpClusterUpgradeState/ScaleDownClusterUpgradeState
            this.TraceLogger.WriteInfo(
                TraceType,
                "SeedNodeUpgrade completed");
        }

        protected override void OnMultiphaseUpgradeRolledbackOrFailed()
        {
            //// No-op. No state change required
            //// State changes are done by ScaleUpClusterUpgradeState/ScaleDownClusterUpgradeState

            this.TraceLogger.WriteWarning(
                TraceType,
                "SeedNodeUpgrade rolledback or failed");
        }
    }
}
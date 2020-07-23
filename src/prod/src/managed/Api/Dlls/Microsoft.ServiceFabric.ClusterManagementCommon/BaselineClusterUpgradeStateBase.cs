// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using Newtonsoft.Json;

    [JsonObject]
    internal abstract class BaselineClusterUpgradeStateBase : ClusterUpgradeStateBase
    {
        private static readonly TraceType TraceType = new TraceType("BaselineClusterUpgradeState");

        [JsonConstructor]
        protected BaselineClusterUpgradeStateBase()
        {
        }

        protected BaselineClusterUpgradeStateBase(
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            ClusterNodeConfig targetNodeConfig,
            ICluster clusterResource,
            ITraceLogger traceLogger)
        : base(targetCsmConfig, targetWrpConfig, targetNodeConfig, clusterResource, traceLogger)
        {
        }

        public override bool CanInterruptUpgrade()
        {
            return false;
        }

        public override bool StartProcessing()
        {
            var manifestBuilder = this.ClusterManifestBuilderActivator.CreateClusterManifestBuilder(
                                      this.ClusterResource.Topology,
                                      this.ClusterResource.SeedNodeSelector,
                                      this.TargetCsmConfig,
                                      this.TargetWrpConfig,
                                      this.TargetNodeConfig,
                                      this /*VersionGenerator*/,
                                      this.ClusterResource.FabricSettingsActivator,
                                      this.ClusterResource.ClusterManifestGeneratorSettings,
                                      this.TraceLogger);
            var clusterManifest = manifestBuilder.GenerateClusterManifest();
            if (clusterManifest == null)
            {
                // ClusterManifest not ready to be generated
                return false;
            }

            this.ExternalState = new ClusterExternalState(clusterManifest, this.TargetWrpConfig.Version.MsiVersion);
            this.PreviousExternalState = this.ExternalState;

            return true;
        }

        public override ClusterState ClusterUpgradeStarted()
        {
            this.TraceLogger.WriteInfo(
                TraceType,
                "Baseline upgrade started.");

            this.OnBaselineUpgradeStarted();

            return null;
        }

        public override ClusterState ClusterUpgradeCompleted()
        {
            this.TraceLogger.WriteInfo(
                TraceType,
                "Baseline upgrade completed.");

            this.OnBaselineUpgradeCompleted();

            this.ResetTargetConfigs();

            return this.ClusterResource.Current;
        }

        public override ClusterState ClusterUpgradeRolledbackOrFailed()
        {
            this.TraceLogger.WriteWarning(
                TraceType,
                "Baseline upgrade failed. Retrying baseline upgrade.");

            // Baseline upgrade should not fail. Incase it fails, then keep retrying
            return null;
        }

        protected abstract void OnBaselineUpgradeStarted();

        protected abstract void OnBaselineUpgradeCompleted();
    }
}
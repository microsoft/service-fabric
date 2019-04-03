// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using Newtonsoft.Json;

    [JsonObject]
    internal abstract class GatekeepingClusterUpgradeState : ClusterUpgradeStateBase
    {
        private static readonly TraceType TraceType = new TraceType("GatekeepingClusterUpgradeState");

        protected GatekeepingClusterUpgradeState(
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
            this.ExternalState = this.ClusterResource.Current.ExternalState;
            this.PreviousExternalState = this.ClusterResource.Current.ExternalState;
            return true;
        }

        public override ClusterState ClusterUpgradeStarted()
        {
            this.TraceLogger.WriteInfo(
                TraceType,
                "Gatekeeping upgrade started to MSI Version:{0} and ClusterManifest Version {1}.",
                this.ExternalState.MsiVersion,
                this.ExternalState.ClusterManifest.Version);            

            this.ClusterResource.ClusterState = ClusterProvisioningState.EnforcingClusterVersion;

            this.OnGatekeepingClusterUpgradeStarted();

            return null;
        }

        public override ClusterState ClusterUpgradeCompleted()
        {
            this.TraceLogger.WriteInfo(
                TraceType,
                "Gatekeeping upgrade to MSI Version:{0} and ClusterManifest Version {1} completed.",
                this.ExternalState.MsiVersion,
                this.ExternalState.ClusterManifest.Version);

            this.ClusterResource.ClusterState = ClusterProvisioningState.Ready;

            this.OnGatekeepingClusterUpgradeCompleted();

            return this.ClusterResource.Current;
        }        

        public override ClusterState ClusterUpgradeRolledbackOrFailed()
        {
            this.TraceLogger.WriteWarning(
                TraceType,
                "Gatekeeping upgrade to MSI Version:{0} and ClusterManifest Version {1} rolledback/failed. Retrying baseline upgrade.",
                this.ExternalState.MsiVersion,
                this.ExternalState.ClusterManifest.Version);

            // Gatekeeping upgrade should not fail. Incase it fails, then keep retrying
            return null;
        }

        protected virtual void OnGatekeepingClusterUpgradeStarted()
        {   
        }

        protected virtual void OnGatekeepingClusterUpgradeCompleted()
        { 
        }        
    }
}
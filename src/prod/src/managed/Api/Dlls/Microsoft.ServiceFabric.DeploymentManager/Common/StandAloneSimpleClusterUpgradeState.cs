// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Common
{
    using System;
    using System.Globalization;
    using Microsoft.ServiceFabric.ClusterManagementCommon;

    internal class StandAloneSimpleClusterUpgradeState : SimpleClusterUpgradeState
    {
        public StandAloneSimpleClusterUpgradeState()
        {
        }

        public StandAloneSimpleClusterUpgradeState(
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

        public override bool CanInterruptUpgrade()
        {
            if (this.ClusterResource.Pending == null || this.ClusterResource.Pending.ExternalState == null)
            {
                return base.CanInterruptUpgrade();
            }

            // Code upgrade is not interruptible.
            return this.ClusterResource.Current.ExternalState.MsiVersion.Equals(this.ClusterResource.Pending.ExternalState.MsiVersion);
        }

        public override string GetNextClusterManifestVersion()
        {
            return StandaloneUtility.GetNextClusterManifestVersionHelper(this.ClusterResource, this.TraceLogger, "StandAloneSimpleClusterUpgradeState");
        }

        public override ClusterState ClusterUpgradeRolledbackOrFailed()
        {
            this.TraceLogger.WriteWarning(
                new TraceType("StandAloneSimpleClusterUpgradeState"),
                "ClusterUpgradeRolledbackOrFailed called for {0} initiated upgrade. TargetCsmVersion: {1}, TargetWrpVersion: {2}",
                this.IsAdminInitiatedUpgrade() ? "Admin" : "User",
                this.TargetCsmConfig.Version.Version,
                this.TargetWrpConfig.Version);

                return base.ClusterUpgradeRolledbackOrFailed();
        }
    }
}
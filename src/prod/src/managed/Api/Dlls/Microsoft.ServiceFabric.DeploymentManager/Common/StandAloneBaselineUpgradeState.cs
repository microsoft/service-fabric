// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Common
{
    using System.Globalization;
    using Microsoft.ServiceFabric.ClusterManagementCommon;

    internal class StandAloneBaselineUpgradeState : BaselineClusterUpgradeStateBase
    {
        public StandAloneBaselineUpgradeState()
        {
        }

        public StandAloneBaselineUpgradeState(
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
            int version = this.ClusterResource.ClusterManifestVersion;
            return version.ToString(CultureInfo.InvariantCulture);
        }

        protected override void OnBaselineUpgradeStarted()
        {
            // nothing needed for StandAlone baseline upgrade
        }

        protected override void OnBaselineUpgradeCompleted()
        {
            // nothing needed for StandAlone baseline upgrade
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Common
{
    using System.Collections.Generic;
    using System.Globalization;
    using Microsoft.ServiceFabric.ClusterManagementCommon;

    internal sealed class StandAloneSeedNodeUpgradeState : SeedNodeUpgradeStateBase
    {
        public StandAloneSeedNodeUpgradeState()
        {
        }

        public StandAloneSeedNodeUpgradeState(
            List<string> disabledNodes,
            List<string> removedNodes,
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            ClusterNodeConfig targetNodeConfig,
            ICluster clusterResource,
            ITraceLogger traceLogger)
        : base(disabledNodes, removedNodes, targetCsmConfig, targetWrpConfig, targetNodeConfig, clusterResource, traceLogger)
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
            return StandaloneUtility.GetNextClusterManifestVersionHelper(this.ClusterResource, this.TraceLogger, "StandAloneSeedNodeUpgradeState");
        }
    }
}
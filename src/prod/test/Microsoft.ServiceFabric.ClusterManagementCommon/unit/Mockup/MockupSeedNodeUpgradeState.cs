// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon.Test
{
    using System.Collections.Generic;
    using System.Globalization;

    internal class MockupSeedNodeUpgradeState: SeedNodeUpgradeStateBase
    {
        public MockupSeedNodeUpgradeState (
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
                return MockupClusterManifestBuilderActivator.Instance;
            }
        }

        public override ClusterUpgradePolicy GetUpgradePolicy()
        {
            return ClusterUpgradeStateBase.ForceUpgradePolicy;
        }

        public override string GetNextClusterManifestVersion()
        {
            int version = ++this.ClusterResource.ClusterManifestVersion;
            return version.ToString(CultureInfo.InvariantCulture);
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon.Test
{
    using System.Globalization;

    internal class MockupBaselineUpgradeState: BaselineClusterUpgradeStateBase
    {
        public MockupBaselineUpgradeState(
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
                return MockupClusterManifestBuilderActivator.Instance;
            }
        }

        public override string GetNextClusterManifestVersion()
        {
            int version = this.ClusterResource.ClusterManifestVersion;
            return version.ToString(CultureInfo.InvariantCulture);
        }

        protected override void OnBaselineUpgradeStarted()
        {

        }

        protected override void OnBaselineUpgradeCompleted()
        {

        }
    }
}
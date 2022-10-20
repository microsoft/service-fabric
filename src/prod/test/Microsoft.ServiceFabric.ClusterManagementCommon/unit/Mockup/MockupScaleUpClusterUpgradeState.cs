// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon.Test
{
    using System.Collections.Generic;
    using System.Globalization;

    internal class MockupScaleUpClusterUpgradeState : ScaleUpClusterUpgradeStateBase
    {
        public MockupScaleUpClusterUpgradeState(
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            ClusterNodeConfig targetNodeConfig,
            ICluster clusterResource,
            Dictionary<string, int> nodeDiffCountList,
            ITraceLogger traceLogger)
            : base(targetCsmConfig, targetWrpConfig, targetNodeConfig, clusterResource, nodeDiffCountList, traceLogger)
        {
        }

        protected override void OnScaleUpClusterUpgradeStarted()
        {
        }

        protected override void OnScaleUpClusterUpgradeCompleted()
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
            return "2.0.0";
        }
    }
}
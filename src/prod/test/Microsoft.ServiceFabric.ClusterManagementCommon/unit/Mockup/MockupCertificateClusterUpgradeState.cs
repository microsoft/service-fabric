// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon.Test
{
    using System.Collections.Generic;
    using System.Globalization;

    internal class MockupCertificateClusterUpgradeState : CertificateClusterUpgradeStateBase
    {
        public MockupCertificateClusterUpgradeState(
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            ClusterNodeConfig targetNodeConfig,
            ICluster clusterResource,
            HashSet<string> certsAdded,
            HashSet<string> certsRemoved,
            ITraceLogger traceLogger)
        : base(targetCsmConfig, targetWrpConfig, targetNodeConfig, clusterResource, traceLogger, certsAdded, certsRemoved)
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

        protected override void OnCertificateUpgradeStarted()
        {
        }

        protected override void OnCertificateUpgradeCompleted()
        {
        }

        protected override void OnCertificateUpgradeRolledbackOrFailed()
        {
        }
    }
}
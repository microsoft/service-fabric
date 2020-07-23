// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Common
{
    using System.Collections.Generic;
    using System.Globalization;
    using Microsoft.ServiceFabric.ClusterManagementCommon;

    internal class StandAloneCertificateClusterUpgradeState : CertificateClusterUpgradeStateBase
    {
        public StandAloneCertificateClusterUpgradeState()
        {
        }

        public StandAloneCertificateClusterUpgradeState(
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
                return StandAloneClusterManifestBuilderActivator.Instance;
            }
        }

        public override string GetNextClusterManifestVersion()
        {
            return StandaloneUtility.GetNextClusterManifestVersionHelper(this.ClusterResource, this.TraceLogger, "StandAloneCertificateClusterUpgradeState");
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
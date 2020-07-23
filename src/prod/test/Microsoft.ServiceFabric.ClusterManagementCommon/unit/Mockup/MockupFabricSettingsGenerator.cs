// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon.Test
{
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;

    internal class MockupFabricSettingsGenerator : FabricSettingsGeneratorBase
    {
        public MockupFabricSettingsGenerator(
            string clusterIdOrName,
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            FabricSettingsMetadata settingsMetadata,
            ClusterManifestType existingClusterManifest,
            ClusterManifestGeneratorSettings clusterManifestGeneratorSettings,
            ITraceLogger traceLogger)
            : base(clusterIdOrName, targetCsmConfig, targetWrpConfig, settingsMetadata, existingClusterManifest, clusterManifestGeneratorSettings, traceLogger)
        {

        }

        protected override List<SettingsTypeSection> OnGenerateFabricSettings(Security security, CertificateDescription thumbprintFileStoreCert, List<string> thumbprintClusterCertList, ServerCertificateCommonNames commonNameFileStoreCert, Dictionary<string, string> commonNameClusterCertList, int currentClusterSize)
        {
            return new List<SettingsTypeSection>();
        }
    }
}
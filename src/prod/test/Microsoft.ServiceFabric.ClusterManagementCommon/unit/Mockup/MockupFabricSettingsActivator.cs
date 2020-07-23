// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon.Test
{
    using System.Fabric.Management.ServiceModel;

    internal class MockupFabricSettingsActivator : IFabricSettingsActivator
    {
        public FabricSettingsGeneratorBase CreateWindowsSettingsGenerator(
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            FabricSettingsMetadata settingsMetadata,
            ClusterManifestType existingClusterManifest,
            ClusterManifestGeneratorSettings clusterManifestGeneratorSettings,
            ITraceLogger traceLogger)
        {
            return new MockupFabricSettingsGenerator(
               "acf4dc93-9b64-4504-8b6f-0b7fd052e096",
               targetCsmConfig,
               targetWrpConfig,
               settingsMetadata,
               existingClusterManifest,
               clusterManifestGeneratorSettings,
               traceLogger);       
        }

        public FabricSettingsGeneratorBase CreateLinuxSettingsGenerator(
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            FabricSettingsMetadata settingsMetadata,
            ClusterManifestType existingClusterManifest,
            ClusterManifestGeneratorSettings clusterManifestGeneratorSettings,
            ITraceLogger traceLogger)
        {
            throw new System.NotImplementedException();    
        }
    }
}
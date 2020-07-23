// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System.Fabric.Management.ServiceModel;

    internal interface IFabricSettingsActivator
    {
        FabricSettingsGeneratorBase CreateWindowsSettingsGenerator(
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            FabricSettingsMetadata settingsMetadata,
            ClusterManifestType existingClusterManifest,
            ClusterManifestGeneratorSettings clusterManifestGeneratorSettings,
            ITraceLogger traceLogger);

        FabricSettingsGeneratorBase CreateLinuxSettingsGenerator(
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            FabricSettingsMetadata settingsMetadata,
            ClusterManifestType existingClusterManifest,
            ClusterManifestGeneratorSettings clusterManifestGeneratorSettings,
            ITraceLogger traceLogger);
    }
}
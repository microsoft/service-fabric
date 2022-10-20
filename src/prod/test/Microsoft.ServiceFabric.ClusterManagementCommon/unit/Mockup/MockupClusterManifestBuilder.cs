// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon.Test
{
    using System.Fabric.Management.ServiceModel;

    internal class MockupClusterManifestBuilder: ClusterManifestBuilder
    {
        public MockupClusterManifestBuilder(
            IClusterTopology topology,
            SeedNodeSelector seedNodeSelector,
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            ClusterNodeConfig targetNodeConfig,
            IManifestVersionGenerator versionGenerator,
            IFabricSettingsActivator fabricSettingsActivator,
            ClusterManifestGeneratorSettings clusterManifestGeneratorSettings,
            ITraceLogger traceLogger)
        : base(topology, seedNodeSelector, targetCsmConfig, targetWrpConfig, targetNodeConfig, versionGenerator, fabricSettingsActivator, clusterManifestGeneratorSettings, traceLogger)
        {
        }

        protected override ClusterManifestTypeInfrastructure OnSortInfrastructure(ClusterManifestTypeInfrastructure infrastructure)
        {
            return infrastructure;
        }
    }
}
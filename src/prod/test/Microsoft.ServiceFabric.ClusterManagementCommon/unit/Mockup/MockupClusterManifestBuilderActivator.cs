// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon.Test
{
    internal class MockupClusterManifestBuilderActivator: IClusterManifestBuilderActivator
    {
        private static MockupClusterManifestBuilderActivator instance = new MockupClusterManifestBuilderActivator();

        private MockupClusterManifestBuilderActivator()
        {
        }

        public static MockupClusterManifestBuilderActivator Instance
        {
            get
            {
                return instance;
            }
        }

        public ClusterManifestBuilder CreateClusterManifestBuilder(
            IClusterTopology topology,
            SeedNodeSelector seedNodeSelector,
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            ClusterNodeConfig targetNodeConfig,
            IManifestVersionGenerator versionGenerator,
            IFabricSettingsActivator fabricSettingsActivator,
            ClusterManifestGeneratorSettings clusterManifestGeneratorSettings,
            ITraceLogger traceLogger)
        {
            return new MockupClusterManifestBuilder(
                       topology,
                       seedNodeSelector,
                       targetCsmConfig,
                       targetWrpConfig,
                       targetNodeConfig,
                       versionGenerator,
                       fabricSettingsActivator,
                       clusterManifestGeneratorSettings,
                       traceLogger);
        }
    }
}
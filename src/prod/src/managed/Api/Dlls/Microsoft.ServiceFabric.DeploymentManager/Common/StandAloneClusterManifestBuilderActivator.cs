// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Common
{
    using Microsoft.ServiceFabric.ClusterManagementCommon;

    internal sealed class StandAloneClusterManifestBuilderActivator : IClusterManifestBuilderActivator
    {
        private static StandAloneClusterManifestBuilderActivator instance = new StandAloneClusterManifestBuilderActivator();

        private StandAloneClusterManifestBuilderActivator()
        {
        }

        public static StandAloneClusterManifestBuilderActivator Instance
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
            return new StandAloneClusterManifestBuilder(
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
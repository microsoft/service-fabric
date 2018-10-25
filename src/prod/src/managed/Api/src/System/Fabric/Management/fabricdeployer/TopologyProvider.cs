// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.FabricDeployer
{
    using System.Fabric.Management.ServiceModel;
    using System.Globalization;

    internal static class TopologyProvider
    {
        public enum TopologyProviderType
        {
            NotApplicable,

            ServiceRuntimeTopologyProvider,

            StaticTopologyProvider
        }

        public static TopologyProviderType GetTopologyProviderTypeFromClusterManifest(ClusterManifestType clusterManifest)
        {
            if (clusterManifest == null)
            {
                return TopologyProviderType.NotApplicable;
            }

            return GetTopologyProviderTypeFromClusterManifestTypeInfrastructure(clusterManifest.Infrastructure);        
        }

        public static TopologyProviderType GetTopologyProviderTypeFromClusterManifestTypeInfrastructure(ClusterManifestTypeInfrastructure clusterManifestTypeInfrastructure)
        {
            if (clusterManifestTypeInfrastructure == null || clusterManifestTypeInfrastructure.Item == null)
            {
                return TopologyProviderType.NotApplicable;
            }

            if (clusterManifestTypeInfrastructure.Item is ClusterManifestTypeInfrastructureWindowsAzure)
            {
                return TopologyProviderType.ServiceRuntimeTopologyProvider;
            }

            if (clusterManifestTypeInfrastructure.Item is ClusterManifestTypeInfrastructureWindowsAzureStaticTopology)
            {
                return TopologyProviderType.StaticTopologyProvider;
            }

            return TopologyProviderType.NotApplicable;
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Autopilot
{
    using System;
    using System.Fabric.Management.ServiceModel;

    public enum TopologyProviderType
    {
        NotApplicable,

        ServiceRuntimeTopologyProvider,

        StaticTopologyProvider
    }

    internal static class TopologyProvider
    {
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

        public static ClusterManifestTypeInfrastructureWindowsAzureStaticTopology GetStaticTopologyProviderElementFromClusterManifest(ClusterManifestType clusterManifest)
        {
            TopologyProviderType topologyProviderType = TopologyProvider.GetTopologyProviderTypeFromClusterManifest(clusterManifest);

            if (topologyProviderType != TopologyProviderType.StaticTopologyProvider)
            {
                return null;
            }

            return (clusterManifest.Infrastructure.Item as ClusterManifestTypeInfrastructureWindowsAzureStaticTopology);
        }
    }
}
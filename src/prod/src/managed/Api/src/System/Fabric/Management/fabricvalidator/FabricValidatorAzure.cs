// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.WindowsFabricValidator
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Management.FabricDeployer;
    using System.Fabric.Management.ServiceModel;
    using System.Linq;

    /// <summary>
    /// This is the class that handles the creation of server deployment on a single node based on the
    /// input manifest.
    /// </summary>
    internal class FabricValidatorAzure : FabricValidator
    {
        private ClusterManifestTypeInfrastructure infrastructure;

        public FabricValidatorAzure(ClusterManifestType clusterManifest, List<InfrastructureNodeType> infrastructureInformation, WindowsFabricSettings windowsFabricSettings)
            : base(clusterManifest, infrastructureInformation, windowsFabricSettings)
        {
            this.infrastructure = clusterManifest.Infrastructure;
            this.IsScaleMin = infrastructureInformation != null && FabricValidatorUtility.IsNodeListScaleMin(this.infrastructureInformation);
        }

        protected override void ValidateInfrastructure()
        {
            TopologyProvider.TopologyProviderType topologyProviderType = TopologyProvider.GetTopologyProviderTypeFromClusterManifestTypeInfrastructure(this.infrastructure);

            if (topologyProviderType == TopologyProvider.TopologyProviderType.ServiceRuntimeTopologyProvider)
            {
                ValidateInfrastructureWithServiceRuntimeTopologyProvider(this.infrastructure.Item as ClusterManifestTypeInfrastructureWindowsAzure);
            }
            else if (topologyProviderType == TopologyProvider.TopologyProviderType.StaticTopologyProvider)
            {
                ValidateInfrastructureWithStaticTopologyProvider(this.infrastructure.Item as ClusterManifestTypeInfrastructureWindowsAzureStaticTopology);
            }

#if !DotNetCoreClr
            if (this.IsV2NodeIdGeneratorEnabled
                && (this.NodeIdGeneratorVersion.Equals("V3", StringComparison.InvariantCultureIgnoreCase)
                || this.NodeIdGeneratorVersion.Equals("V4", StringComparison.InvariantCultureIgnoreCase)))
#else
            if (this.IsV2NodeIdGeneratorEnabled && (this.NodeIdGeneratorVersion.Equals("V3") || this.NodeIdGeneratorVersion.Equals("V4")))
#endif
            {
                throw new ArgumentException(FabricValidatorUtility.TraceTag, "V2 and V3/V4 NodeIdGenerator cannot be enabled simultaneously.");
            }

#if !DotNetCoreClr
            if(    !string.IsNullOrEmpty(this.NodeIdGeneratorVersion)
                && !this.NodeIdGeneratorVersion.Equals("V3", StringComparison.InvariantCultureIgnoreCase)
                && !this.NodeIdGeneratorVersion.Equals("V4", StringComparison.InvariantCultureIgnoreCase) )
#else
            if(    !string.IsNullOrEmpty(this.NodeIdGeneratorVersion)
                && !this.NodeIdGeneratorVersion.Equals("V3")
                && !this.NodeIdGeneratorVersion.Equals("V4") )
#endif
            {
                throw new ArgumentException(FabricValidatorUtility.TraceTag, "v3/V3 and v4/V4 are the only allowed values for the NodeIdGeneratorVersion configuration.");
            }

            if (this.DiagnosticsFileStoreConnectionString != null)
            {
                char[] secureFileStoreConnection = FabricValidatorUtility.SecureStringToCharArray(this.DiagnosticsFileStoreConnectionString);
                try
                {
                    if (!FabricValidatorUtility.StartsWithIgnoreCase(secureFileStoreConnection, FabricValidatorConstants.XStoreImageStoreConnectionStringPrefix))
                    {
                        throw new ArgumentException(FabricValidatorUtility.TraceTag, "Only xstore store diagnostics file connection string is officially supported for file stores in azure deployment.");
                    }
                }
                finally
                {
                    Array.Clear(secureFileStoreConnection, 0, secureFileStoreConnection.Length);
                }
            }

            if (this.DiagnosticsTableStoreConnectionString != null)
            {
                char[] secureTableStoreConnection = FabricValidatorUtility.SecureStringToCharArray(this.DiagnosticsTableStoreConnectionString);
                try
                {
                    if (!FabricValidatorUtility.StartsWithIgnoreCase(secureTableStoreConnection, FabricValidatorConstants.XStoreImageStoreConnectionStringPrefix))
                    {
                        throw new ArgumentException("Only xstore store diagnostics table connection string is officially supported for table stores in azure deployment.");
                    }
                }
                finally
                {
                    Array.Clear(secureTableStoreConnection, 0, secureTableStoreConnection.Length);
                }
            }
        }

        private void ValidateInfrastructureWithServiceRuntimeTopologyProvider(ClusterManifestTypeInfrastructureWindowsAzure azureInfrastructure)
        {
            IEnumerable<AzureRoleType> seedNodes = azureInfrastructure.Roles.Where(roleType => roleType.SeedNodeCount > 0);

            if (seedNodes.Count() != 1)
            {
                throw new ArgumentException("Exactly one role has to have a non zero seed node count.");
            }

            if (this.clusterManifest.NodeTypes.Length != azureInfrastructure.Roles.Length)
            {
                // Meaning there are more than 1 more infrastructure roles than are types.
                throw new ArgumentException("Roles and node types should have a one to one mapping.");
            }

            Array.Sort(this.clusterManifest.NodeTypes, NodeTypeNameComparator);
            Array.Sort(azureInfrastructure.Roles, AzureRoleTypeNodeTypeRefComparator);

            for (int i = 0; i < this.clusterManifest.NodeTypes.Length; i++)
            {
                if (this.clusterManifest.NodeTypes[i].Name != azureInfrastructure.Roles[i].NodeTypeRef)
                {
                    throw new ArgumentException(
                        string.Format(
                        "Role {0} found but corresponding node type not found. or Node type {1} found but corresponding role not found.",
                        azureInfrastructure.Roles[i].RoleName,
                        this.clusterManifest.NodeTypes[i].Name));
                }

                if (this.clusterManifest.NodeTypes[i].Endpoints != null)
                {
                    throw new ArgumentException(
                        string.Format("Node type {0} should not define endpoint section on Windows Azure infrastructure.", 
                        this.clusterManifest.NodeTypes[i].Name));
                }
            }
        }

        private void ValidateInfrastructureWithStaticTopologyProvider(ClusterManifestTypeInfrastructureWindowsAzureStaticTopology azureStaticTopologyInfrastructure)
        {
            foreach (ClusterManifestTypeNodeType nodeType in this.clusterManifest.NodeTypes)
            {
                if (nodeType.Endpoints == null)
                {
                    throw new ArgumentException(
                        String.Format(
                        "Endpoint section was not defined by node type {0}. The section has to be defined for all node types when staticTopologyProvider is used on Windows Azure infrastructure.",
                        nodeType.Name));
                }
            }
        }

        private int AzureRoleTypeNodeTypeRefComparator(AzureRoleType role1, AzureRoleType role2)
        {
            return string.Compare(role1.NodeTypeRef, role2.NodeTypeRef, StringComparison.OrdinalIgnoreCase);
        }

        private int NodeTypeNameComparator(ClusterManifestTypeNodeType nodeType1, ClusterManifestTypeNodeType nodeType2)
        {
            return string.Compare(nodeType1.Name, nodeType2.Name, StringComparison.OrdinalIgnoreCase);
        }
    }
}
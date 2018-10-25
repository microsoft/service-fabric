// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.WindowsFabricValidator
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Management.ServiceModel;

    internal class FabricValidatorBlackbird : FabricValidator
    {
        public FabricValidatorBlackbird(ClusterManifestType clusterManifest, List<InfrastructureNodeType> infrastructureInformation, WindowsFabricSettings windowsFabricSettings)
            : base(clusterManifest, infrastructureInformation, windowsFabricSettings)
        {
            ReleaseAssert.AssertIf(clusterManifest == null, "clusterManifest should be non-null");

            ReleaseAssert.AssertIfNot(
                clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureBlackbird, 
                "Only ClusterManifestTypeInfrastructureBlackbird is supported by ServerDeployer");

            this.IsScaleMin = infrastructureInformation != null && FabricValidatorUtility.IsNodeListScaleMin(this.infrastructureInformation);
        }

        protected override void ValidateInfrastructure()
        {
            foreach (ClusterManifestTypeNodeType nodeType in this.clusterManifest.NodeTypes)
            {
                if (nodeType.Endpoints == null)
                {
                    throw new ArgumentException(
                        string.Format(
                        "Node type {0} has no endpoints. In Blackbird deployment all node types need to have endpoints",
                        nodeType.Name));
                }
            }

            // add more validation here specific to blackbird infrastructure
        }
    }
}
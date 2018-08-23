// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.WindowsFabricValidator
{
    using Fabric.FabricDeployer;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Management.ServiceModel;

    internal class FabricValidatorPaaS : FabricValidator
    {
        public FabricValidatorPaaS(ClusterManifestType clusterManifest, List<InfrastructureNodeType> infrastructureInformation, WindowsFabricSettings windowsFabricSettings)
            : base(clusterManifest, infrastructureInformation, windowsFabricSettings)
        {
            ReleaseAssert.AssertIf(clusterManifest == null, "clusterManifest should be non-null");

            ReleaseAssert.AssertIfNot(
                clusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructurePaaS,
                "Only ClusterManifestTypeInfrastructurePaaS is supported by FabricValidatorPaaS");

            this.IsScaleMin = infrastructureInformation != null && FabricValidatorUtility.IsNodeListScaleMin(this.infrastructureInformation);
        }

        protected override void ValidateInfrastructure()
        {
            if (this.IsDCAFileStoreEnabled)
            {
                throw new ArgumentException("Diagnostics file store is not supported on server.");
            }

            if (this.IsDCATableStoreEnabled)
            {
                throw new ArgumentException("Diagnostics table store is not supported on server.");
            }

            foreach (ClusterManifestTypeNodeType nodeType in this.clusterManifest.NodeTypes)
            {
                if (nodeType.Endpoints == null)
                {
                    throw new ArgumentException(
                        string.Format(
                        "Node type {0} has no endpoints. In PaaS deployment all node types need to have endpoints",
                        nodeType.Name));
                }                
            }            

            // add more validation here specific to PaaS infrastructure
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.WindowsFabricValidator
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Management.ServiceModel;

    internal class FabricValidatorServer : FabricValidator
    {
        public FabricValidatorServer(ClusterManifestType clusterManifest, List<InfrastructureNodeType> infrastructureInformation, WindowsFabricSettings windowsFabricSettings)
            : base(clusterManifest, infrastructureInformation, windowsFabricSettings)
        {
            ReleaseAssert.AssertIf(clusterManifest == null, "clusterManifest should be non-null");
#if DotNetCoreClrLinux
            var infrastructure = clusterManifest.Infrastructure.Item as ClusterManifestTypeInfrastructureLinux;
#else
            var infrastructure = clusterManifest.Infrastructure.Item as ClusterManifestTypeInfrastructureWindowsServer;
#endif
            ReleaseAssert.AssertIf(infrastructure == null, "Invalid infrastructure item type {0}", clusterManifest.Infrastructure.Item.GetType());
            this.IsScaleMin = infrastructure.IsScaleMin;
        }

        protected override void ValidateInfrastructure()
        {

            if (this.IsDCAFileStoreEnabled)
            {
                throw new ArgumentException(
                        string.Format("Diagnostics file store is not supported on server."));
            }

            if (this.IsDCATableStoreEnabled)
            {
                throw new ArgumentException(
                        string.Format("Diagnostics table store is not supported on server."));
            }

            foreach (ClusterManifestTypeNodeType nodeType in this.clusterManifest.NodeTypes)
            {
                if (nodeType.Endpoints == null)
                {
                   throw new ArgumentException(
                        string.Format(
                        "Node type {0} has no endpoints. In Server deployment all node types need to have endpoints",
                        nodeType.Name));
                }
            }
        }
    }
}
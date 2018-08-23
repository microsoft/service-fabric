// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System.Fabric.Management.FabricDeployer;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;

    internal class UpdateInstanceIdOperation : CreateorUpdateOperation
    {
        protected override void OnExecuteOperation(DeploymentParameters parameters, ClusterManifestType clusterManifest, Infrastructure infrastructure)
        {
            base.OnExecuteOperation(parameters, clusterManifest, infrastructure);
        }
    }
}
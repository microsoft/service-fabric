// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System.Fabric.Management.FabricDeployer;
    using System.Fabric.Management.ServiceModel;

    internal class ValidateClusterManifestOperation : DeploymentOperation
    {
        protected override void OnExecuteOperation(DeploymentParameters parameters, ClusterManifestType clusterManifest, Infrastructure infrastructure)
        {
            var fabricValidator = new FabricValidatorWrapper(parameters, clusterManifest, infrastructure);
            fabricValidator.ValidateAndEnsureDefaultImageStore();
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric;
    using System.Fabric.Result;
    using System.Management.Automation;  
    using System.Threading;
    using System.Threading.Tasks;

    [Cmdlet(VerbsLifecycle.Restart, "ServiceFabricDeployedCodePackage")]
    public sealed class RestartDeployedCodePackage : CodePackageControlOperationBase
    {
        internal override Task<RestartDeployedCodePackageResult> InvokeCommandAsync(
            IClusterConnection clusterConnection,
            string nodeName,
            Uri applicationName,
            string serviceManifestName,
            string servicePackageActivationId,
            string codePackageName,
            long codePackageInstanceId,
            CancellationToken cancellationToken)
        {
            return clusterConnection.RestartDeployedCodePackageAsync(
                       nodeName,
                       applicationName,
                       serviceManifestName,
                       servicePackageActivationId,
                       codePackageName,
                       codePackageInstanceId,
                       this.TimeoutSec,
                       this.CommandCompletionMode ?? CompletionMode.Verify,
                       cancellationToken);
        }

        internal override Task<RestartDeployedCodePackageResult> InvokeCommandAsync(
            IClusterConnection clusterConnection,
            Uri applicationName,
            ReplicaSelector replicaSelector)
        {
            return clusterConnection.RestartDeployedCodePackageAsync(
                       replicaSelector,
                       applicationName,
                       this.TimeoutSec,
                       this.CommandCompletionMode ?? CompletionMode.Verify,
                       this.GetCancellationToken());
        }

        internal override string GetOperationNameForSuccessTrace()
        {
            return "RestartCodePackage";
        }
    }
}
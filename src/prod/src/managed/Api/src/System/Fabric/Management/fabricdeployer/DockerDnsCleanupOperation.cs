// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using System.Fabric.Common;
    using System.Fabric.Management.FabricDeployer;
    using System.Fabric.Management.ServiceModel;
    using System.Diagnostics.Tracing;

    /// <summary>
    /// Used for testing <see cref="DockerDnsHelper"/> setup operations independently.
    /// <seealso cref="DockerDnsSetupOperation"/>
    /// </summary>
    internal class DockerDnsCleanupOperation : DeploymentOperation
    {
        protected override void OnExecuteOperation(DeploymentParameters parameters, ClusterManifestType targetClusterManifest, Infrastructure infrastructure)
        {
            DeployerTrace.UpdateConsoleLevel(EventLevel.Verbose);
#if !DotNetCoreClrIOT
            new DockerDnsHelper(parameters, string.Empty).CleanupAsync().GetAwaiter().GetResult();
#else
            DeployerTrace.WriteInfo("CoreClrIOT: Dns cleanup skipped on CoreClrIOT.");
#endif
        }
    }
}
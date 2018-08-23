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
    using System.Fabric.Strings;

    /// <summary>
    /// Used for testing <see cref="DockerDnsHelper"/> setup operations independently.
    /// </summary>
    /// <example>
    /// To test as a standalone on a VM that is part of the SF cluster, 
    /// a. Build FabricDeployer, xcopy the FabricDeployer folder under %outputroot%\debug-amd64 to the VM.
    /// b. Run
    ///    FabricDeployer.exe /operation:DockerDnsSetup
    /// c. For cleanup, run
    ///    FabricDeployer.exe /operation:DockerDnsCleanup
    ///    <seealso cref="DockerDnsCleanupOperation"/>
    /// This should execute the code in <see cref="DockerDnsHelper"/> without depenency on other deployer related code.
    /// </example>
    internal class DockerDnsSetupOperation : DeploymentOperation
    {
        protected override void OnExecuteOperation(DeploymentParameters parameters, ClusterManifestType targetClusterManifest, Infrastructure infrastructure)
        {
            DeployerTrace.UpdateConsoleLevel(EventLevel.Verbose);

            try
            {
                string currentNodeIPAddressOrFQDN = string.Empty;
                if ((infrastructure != null) && (infrastructure.InfrastructureNodes != null))
                {
                    foreach (var infraNode in infrastructure.InfrastructureNodes)
                    {
                        if (NetworkApiHelper.IsAddressForThisMachine(infraNode.IPAddressOrFQDN))
                        {
                            currentNodeIPAddressOrFQDN = infraNode.IPAddressOrFQDN;
                            break;
                        }
                    }
                }

                new DockerDnsHelper(parameters, currentNodeIPAddressOrFQDN).SetupAsync().GetAwaiter().GetResult();
            }
            catch (Exception ex)
            {
                DeployerTrace.WriteWarning(
                    StringResources.Warning_FabricDeployer_DockerDnsSetup_ErrorContinuing2,
                    ex);                
            }            
        }
    }
}
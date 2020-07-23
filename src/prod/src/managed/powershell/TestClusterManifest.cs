// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.Tracing;
    using System.Fabric.Common.Tracing;
    using System.Fabric.FabricDeployer;
    using System.Fabric.Management.ServiceModel;
    using System.Fabric.Strings;
    using System.Management.Automation;

    [Cmdlet(VerbsDiagnostic.Test, "ServiceFabricClusterManifest")]
    public sealed class TestClusterManifest : CommonCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0)]
        public string ClusterManifestPath
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string OldClusterManifestPath
        {
            get;
            set;
        }

        private new int? TimeoutSec
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            try
            {
                TraceConfig.SetDefaultLevel(TraceSinkType.Console, EventLevel.Error);
                var newClusterManifest = this.ReadXml<ClusterManifestType>(this.GetAbsolutePath(this.ClusterManifestPath), this.GetFabricFilePath(Constants.ServiceModelSchemaFileName));
                if (newClusterManifest.Infrastructure.Item is ClusterManifestTypeInfrastructureWindowsServer)
                {
                    if (string.IsNullOrEmpty(this.OldClusterManifestPath))
                    {
                        var parameters = new Dictionary<string, dynamic>
                        {
                            { DeploymentParameters.ClusterManifestString, this.GetAbsolutePath(this.ClusterManifestPath) }
                        };

                        var deploymentParameters = new DeploymentParameters();
                        deploymentParameters.SetParameters(parameters, DeploymentOperations.ValidateClusterManifest);
                        DeploymentOperation.ExecuteOperation(deploymentParameters);
                        this.WriteObject(true);
                    }
                    else
                    {
                        var upgradeValidationParameters = new Dictionary<string, dynamic>
                        {
                            { DeploymentParameters.OldClusterManifestString, this.GetAbsolutePath(this.OldClusterManifestPath) },
                            { DeploymentParameters.ClusterManifestString, this.GetAbsolutePath(this.ClusterManifestPath) }
                        };

                        var upgradeDeploymentParameters = new DeploymentParameters();
                        upgradeDeploymentParameters.SetParameters(upgradeValidationParameters, DeploymentOperations.Validate);
                        try
                        {
                            DeploymentOperation.ExecuteOperation(upgradeDeploymentParameters);
                        }
                        catch (Exception exception)
                        {
                            if (exception is FabricHostRestartRequiredException || exception is FabricHostRestartNotRequiredException)
                            {
                                this.WriteObject(true);
                                return;
                            }

                            throw;
                        }
                    }
                }
                else
                {
                    throw new InvalidOperationException(StringResources.Error_NonServerClusterManifestValidationNotSupport);
                }
            }
            catch (Exception exception)
            {
                this.WriteObject(false);
                this.ThrowTerminatingError(
                    exception,
                    Constants.TestClusterManifestErrorId,
                    null);
            }
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric;
    using System.Fabric.Result;
    using System.IO;
    using System.Management.Automation;
    using System.Numerics;

    [Cmdlet(VerbsCommon.Set, "ServiceFabricUpgradeOrchestrationServiceState")]

    public sealed class SetUpgradeOrchestrationServiceState : CommonCmdletBase
    {
        [Parameter(Mandatory = true)]
        public string StateFilePath
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            IClusterConnection clusterConnection = this.GetClusterConnection();

            try
            {
                string fileAbsolutePath = this.GetAbsolutePath(this.StateFilePath);
                string state = File.ReadAllText(fileAbsolutePath);
                
                FabricUpgradeOrchestrationServiceState result = clusterConnection.SetUpgradeOrchestrationServiceStateAsync(
                    state,                           
                    this.GetTimeout(),
                    this.GetCancellationToken()).Result;
                this.WriteObject(result);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        "SetUpgradeOrchestrationServiceStateErrorId",
                        clusterConnection);
                    return true;
                });
            }
        }
    }
}
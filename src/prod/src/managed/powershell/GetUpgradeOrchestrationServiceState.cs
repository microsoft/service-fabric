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
    using System.Numerics;

    [Cmdlet(VerbsCommon.Get, Constants.GetUpgradeOrchestrationServiceState)]

    public sealed class GetUpgradeOrchestrationServiceState : CommonCmdletBase
    {
        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                string uosServiceState = clusterConnection.GetUpgradeOrchestrationServiceStateAsync(
                                               this.GetTimeout(),
                                               this.GetCancellationToken()).Result;
                this.WriteObject(uosServiceState);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetUpgradeOrchestrationServiceStateErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }
    }
}
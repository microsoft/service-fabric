// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric;
    using System.Fabric.Description;
    using System.Fabric.Result;
    using System.Management.Automation;
    using System.Numerics;

    [Cmdlet(VerbsLifecycle.Start, Constants.StartOrchestrationApprovedUpgrades)]

    internal sealed class StartApprovedUpgradesOrchestration : CommonCmdletBase
    {
        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                clusterConnection.StartApprovedUpgradesAsync(
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.StartOrchestrationApprovedUpgradesErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }
    }
}
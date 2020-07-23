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

    [Cmdlet(VerbsCommon.Get, Constants.GetOrchestrationUpgradesPendingApproval)]

    internal sealed class GetUpgradesPendingApprovalOrchestration : CommonCmdletBase
    {
        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                clusterConnection.GetUpgradesPendingApprovalAsync(
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetOrchestrationUpgradesPendingApprovalErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }
    }
}
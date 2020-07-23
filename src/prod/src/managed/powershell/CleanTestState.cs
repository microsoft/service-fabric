// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Strings;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Remove, "ServiceFabricTestState")]
    public sealed class CleanTestState : CommonCmdletBase
    {
        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                clusterConnection.CleanTestStateAsync(
                    this.TimeoutSec,
                    this.GetCancellationToken()).Wait();

                this.WriteObject(StringResources.Info_CleanTestStateSucceeded);
            }
            catch (AggregateException aggregateException)
            {
                this.ThrowTerminatingError(
                    aggregateException.InnerException,
                    Constants.CleanTestStateCommandErrorId,
                    clusterConnection);
            }
        }
    }
}
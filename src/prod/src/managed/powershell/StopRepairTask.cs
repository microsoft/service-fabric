// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Management.Automation;

    [Cmdlet(VerbsLifecycle.Stop, Constants.ServiceFabricRepairTask)]
    public sealed class StopRepairTask : CommonCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0)]
        public string TaskId
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, Position = 1)]
        public long Version
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                clusterConnection.CancelRepairTaskAsync(
                    this.TaskId,
                    this.Version,
                    false,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.StopRepairTaskErrorId,
                        clusterConnection);
                    return true;
                });
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.StopRepairTaskErrorId,
                    clusterConnection);
            }
        }
    }
}
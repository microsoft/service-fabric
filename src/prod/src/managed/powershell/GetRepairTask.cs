// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Repair;
    using System.Linq;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, Constants.ServiceFabricRepairTask)]
    public sealed class GetRepairTask : CommonCmdletBase
    {
        public GetRepairTask()
        {
            this.State = RepairTaskStateFilter.Default;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, Position = 0)]
        public string TaskId
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public RepairTaskStateFilter State
        {
            get;
            set;
        }       

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                var queryResult = clusterConnection.GetRepairTaskListAsync(
                                      this.TaskId,
                                      this.State,
                                      null, // any executor
                                      this.GetTimeout(),
                                      this.GetCancellationToken()).Result;

                foreach (var item in queryResult
                         .OrderBy(t => t.State != RepairTaskState.Completed)
                         .ThenBy(t => t.State == RepairTaskState.Completed ? t.CompletedTimestamp : t.CreatedTimestamp))
                {
                    this.WriteObject(this.FormatOutput(item));
                }
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetRepairTaskErrorId,
                        clusterConnection);
                    return true;
                });
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.GetRepairTaskErrorId,
                    clusterConnection);
            }
        }
    }
}
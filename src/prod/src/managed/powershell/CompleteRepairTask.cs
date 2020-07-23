// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric;
    using System.Fabric.Repair;
    using System.Fabric.Strings;
    using System.Linq;
    using System.Management.Automation;

    [Cmdlet(
         VerbsLifecycle.Complete,
         Constants.ServiceFabricRepairTask,
         SupportsShouldProcess = true,
         ConfirmImpact = ConfirmImpact.High)]
    public sealed class CompleteRepairTask : CommonCmdletBase
    {
        public CompleteRepairTask()
        {
            this.ResultStatus = RepairTaskResult.Succeeded;
        }

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

        [Parameter(Mandatory = false)]
        public RepairTaskResult ResultStatus
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public int ResultCode
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string ResultDetails
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public SwitchParameter Force
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
                                      RepairTaskStateFilter.All,
                                      null, // any executor
                                      this.GetTimeout(),
                                      this.GetCancellationToken()).Result;

                var task = queryResult.Where(t => t.TaskId == this.TaskId).SingleOrDefault();

                if (task == null)
                {
                    this.ThrowTerminatingError(
                        new FabricException(StringResources.Error_RepairTaskNotFound, FabricErrorCode.RepairTaskNotFound),
                        Constants.CompleteRepairTaskErrorId,
                        clusterConnection);
                }

                if (this.ShouldProcess(this.TaskId))
                {
                    bool isManualRepair = task.Action == SystemRepairActionHelper.ManualRepairAction;

                    if (isManualRepair || this.Force || this.ShouldContinue(
                            StringResources.Warning_CompleteRepairExecutionNotManual,
                            string.Empty))
                    {
                        task.State = RepairTaskState.Restoring;
                        task.ResultStatus = this.ResultStatus;
                        task.ResultCode = this.ResultCode;
                        task.ResultDetails = this.ResultDetails;

                        clusterConnection.UpdateRepairExecutionStateAsync(
                            task,
                            this.GetTimeout(),
                            this.GetCancellationToken()).Wait();
                    }
                }
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.CompleteRepairTaskErrorId,
                        clusterConnection);
                    return true;
                });
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.CompleteRepairTaskErrorId,
                    clusterConnection);
            }
        }
    }
}
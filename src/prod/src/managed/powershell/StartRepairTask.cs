// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Repair;
    using System.Globalization;
    using System.Management.Automation;

    [Cmdlet(
         VerbsLifecycle.Start,
         Constants.ServiceFabricRepairTask,
         DefaultParameterSetName = ParamSetNodeBuiltInAuto)]
    public sealed class StartRepairTask : CommonCmdletBase
    {
        private const string DefaultTaskIdFormat = "FabricClient/{0}";
        private const string ParamSetNodeBuiltInAuto = "NodeBuiltInAuto";
        private const string ParamSetNodeCustomAuto = "NodeCustomAuto";
        private const string ParamSetNodeManual = "NodeManual";

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0, ParameterSetName = ParamSetNodeBuiltInAuto)]
        public string NodeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 1, ParameterSetName = ParamSetNodeBuiltInAuto)]
        public SystemNodeRepairAction NodeAction
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 0, ParameterSetName = ParamSetNodeCustomAuto)]
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = ParamSetNodeManual)]
        public string[] NodeNames
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 1, ParameterSetName = ParamSetNodeCustomAuto)]
        [ValidateNotNullOrEmpty]
        public string CustomAction
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 1, ParameterSetName = ParamSetNodeManual)]
        public NodeImpactLevel NodeImpact
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        [ValidateNotNullOrEmpty]
        public string TaskId
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        [ValidateNotNullOrEmpty]
        public string Description
        {
            get;
            set;
        }      

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                var repairTask = this.CreateRepairTask();

                long commitVersion = clusterConnection.CreateRepairTaskAsync(
                                         repairTask,
                                         this.GetTimeout(),
                                         this.GetCancellationToken()).Result;

                var output = new PSObject();

                output.Properties.Add(new PSNoteProperty(
                                          Constants.PropertyNameTaskId,
                                          repairTask.TaskId));

                output.Properties.Add(new PSNoteProperty(
                                          Constants.PropertyNameVersion,
                                          commitVersion));

                this.WriteObject(output);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.StartRepairTaskErrorId,
                        clusterConnection);
                    return true;
                });
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.StartRepairTaskErrorId,
                    clusterConnection);
            }
        }

        private RepairTask CreateRepairTask()
        {
            string taskId = string.Format(
                                CultureInfo.InvariantCulture,
                                DefaultTaskIdFormat,
                                Guid.NewGuid());

            if (!string.IsNullOrEmpty(this.TaskId))
            {
                taskId = this.TaskId;
            }

            switch (this.ParameterSetName)
            {
                case ParamSetNodeBuiltInAuto:
                    {
                        var repairTask = new ClusterRepairTask(
                            taskId,
                            SystemRepairActionHelper.GetActionString(this.NodeAction));

                        repairTask.Description = this.Description;
                        repairTask.Target = new NodeRepairTargetDescription(this.NodeName);

                        return repairTask;
                    }

                case ParamSetNodeCustomAuto:
                    {
                        var repairTask = new ClusterRepairTask(taskId, this.CustomAction);

                        repairTask.Description = this.Description;
                        repairTask.Target = new NodeRepairTargetDescription(this.NodeNames);

                        return repairTask;
                    }

                case ParamSetNodeManual:
                    {
                        var repairTask = new ClusterRepairTask(
                            taskId,
                            SystemRepairActionHelper.ManualRepairAction);

                        repairTask.Description = this.Description;
                        repairTask.State = RepairTaskState.Preparing;
                        repairTask.Executor = "Manual";

                        // Informational only
                        repairTask.Target = new NodeRepairTargetDescription(this.NodeNames);

                        var impact = new NodeRepairImpactDescription();
                        foreach (var nodeName in this.NodeNames)
                        {
                            impact.ImpactedNodes.Add(new NodeImpact(nodeName, this.NodeImpact));
                        }

                        repairTask.Impact = impact;

                        return repairTask;
                    }

                default:
                    // Unsupported parameter set
                    throw new NotSupportedException();
            }
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Autopilot
{
    using Collections.Generic;
    using Linq;
    using Repair;
    using Threading.Tasks;

    internal interface IAction
    {
        Task ExecuteAsync(IRepairManager repairManager);
    }

    internal sealed class AskModeContext
    {
        private static readonly TraceType traceType = new TraceType("APAskModeContext");

        private readonly Dictionary<MaintenanceRecordId, MachineMaintenanceRecord> maintenanceMap =
            new Dictionary<MaintenanceRecordId, MachineMaintenanceRecord>();

        private readonly HashSet<string> doNotCancelMachineNames = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

        private readonly List<IAction> outputActions = new List<IAction>();

        private readonly ConfigReader configReader;
        private readonly string serviceName;

        private IList<IRepairTask> repairTasks;

        public AskModeContext(ConfigReader configReader, string serviceName)
        {
            this.configReader = configReader.Validate("configReader");
            this.serviceName = serviceName.Validate("serviceName");
        }

        public IEnumerable<MachineMaintenanceRecord> RepairRecords
        {
            get
            {
                return this.maintenanceMap.Values;
            }
        }

        public int OverdueRepairTaskCount { get; private set; }

        /// <returns>true if the record was added, false if it was ignored
        public bool SetRepairRecord(MachineMaintenanceRecord record)
        {
            record.Validate("record");

            if (record.RepairType == RepairType.NoOp)
            {
                traceType.WriteInfo("Ignoring repair {0}", record.RecordId);
                return false;
            }

            this.maintenanceMap[record.RecordId] = record;
            return true;
        }

        public void DoNotCancelRepairsForMachine(string machineName)
        {
            this.doNotCancelMachineNames.Add(machineName);
        }

        public void UpdateRepairDelays()
        {
            foreach (var record in this.RepairRecords)
            {
                if (record.IsPendingApproval)
                {
                    if (this.configReader.AutoApprove && !record.IsApproved)
                    {
                        record.IsApproved = true;
                        traceType.WriteInfo("Auto-approved {0}", record.RecordId);
                    }

                    if (record.IsApproved)
                    {
                        // TODO different values for destructive and non-destructive
                        TimeSpan approvalDelay = this.configReader.ApprovedNonDestructiveRepairDelayValue;

                        // Only reduce the delay for approved records, never increase it
                        if (approvalDelay < record.NewDelay)
                        {
                            record.NewDelay = approvalDelay;
                        }
                    }
                    else if (record.OriginalDelay < this.configReader.ExtendRepairDelayThreshold)
                    {
                        // Only extend the delay once it has dropped below a certain threshold,
                        // so that in most cases when approval is fast, the delay is never extended.
                        record.NewDelay = this.configReader.ExtendRepairDelayValue;
                    }
                }

                traceType.WriteInfo(
                    "ProcessRepair: {0}: IsPendingApproval={1}, IsApproved={2}, OriginalDelay={3}, NewDelay={4}",
                    record.RecordId,
                    record.IsPendingApproval,
                    record.IsApproved,
                    record.OriginalDelay,
                    record.NewDelay);
            }
        }

        public void SetRepairTasks(IList<IRepairTask> repairTasks)
        {
            if (this.repairTasks != null)
            {
                throw new InvalidOperationException("Repair task list is already set");
            }

            this.repairTasks = repairTasks.Validate("repairTasks");
        }

        public IEnumerable<IAction> Reconcile()
        {
            this.outputActions.Clear();
            this.OverdueRepairTaskCount = 0;

            IEnumerable<IRepairTask> unmatchedRepairTasks = new List<IRepairTask>(this.repairTasks ?? Enumerable.Empty<IRepairTask>());

            foreach (MachineMaintenanceRecord record in this.maintenanceMap.Values)
            {
                unmatchedRepairTasks = ReconcileRepairRecord(record, unmatchedRepairTasks);
            }

            foreach (IRepairTask repairTask in unmatchedRepairTasks)
            {
                ReconcileUnmatchedRepairTask(repairTask);
            }

            return this.outputActions;
        }

        private IEnumerable<IRepairTask> ReconcileRepairRecord(MachineMaintenanceRecord record, IEnumerable<IRepairTask> candidateRepairTasks)
        {
            bool foundMatch = false;
            List<IRepairTask> unmatchedRepairTasks = new List<IRepairTask>();

            string taskIdPrefix = GenerateTaskId(record.RecordId, null);

            foreach (IRepairTask repairTask in candidateRepairTasks)
            {
                if (TryReconcileRepair(record, taskIdPrefix, repairTask))
                {
                    foundMatch = true;
                }
                else
                {
                    // Not matched, put it back in the queue
                    unmatchedRepairTasks.Add(repairTask);
                }
            }

            if (!foundMatch)
            {
                if (!record.IsPendingApproval)
                {
                    traceType.WriteWarning(
                        "Repair {0} is executing without approval from a repair task",
                        record.RecordId);
                }

                EmitActionCreateRepairTaskInPreparing(record);
            }

            return unmatchedRepairTasks;
        }

        private bool TryReconcileRepair(MachineMaintenanceRecord record, string taskIdPrefix, IRepairTask repairTask)
        {
            bool isMatch = repairTask.TaskId.StartsWith(taskIdPrefix);

            if (isMatch)
            {
                if (repairTask.State != RepairTaskState.Completed)
                {
                    traceType.WriteInfo(
                        "{3} repair {0} matched repair task {1} in state {2}",
                        record.RecordId,
                        repairTask.TaskId,
                        repairTask.State,
                        record.IsPendingApproval ? "Pending" : "Active");
                }

                switch (repairTask.State)
                {
                    case RepairTaskState.Preparing:

                        if (IsTimestampOld(repairTask.PreparingTimestamp, this.configReader.OverdueRepairTaskPreparingThreshold))
                        {
                            traceType.WriteWarning(
                                "Repair task {0} has been in Preparing since {1:O} ({2} ago)",
                                repairTask.TaskId,
                                repairTask.PreparingTimestamp,
                                DateTime.UtcNow - repairTask.PreparingTimestamp);

                            this.OverdueRepairTaskCount++;
                        }

                        if (!record.IsPendingApproval)
                        {
                            traceType.WriteWarning(
                                "Repair {0} is executing without approval from repair task {1}",
                                record.RecordId,
                                repairTask.TaskId);
                        }

                        // Nothing else to do
                        break;

                    case RepairTaskState.Approved:

                        if (!IsTimestampOld(repairTask.ApprovedTimestamp, this.configReader.PostApprovalExecutionDelay))
                        {
                            traceType.WriteInfo(
                                "Delaying execution of {0} because repair task {1} was approved recently (at {2:O})",
                                record.RecordId,
                                repairTask.TaskId,
                                repairTask.ApprovedTimestamp);
                        }
                        else if (record.IsPendingApproval)
                        {
                            EmitActionExecuteRepair(record, repairTask);
                        }
                        else
                        {
                            traceType.WriteWarning(
                                "Repair {0} is executing without approval from repair task {1}",
                                record.RecordId,
                                repairTask.TaskId);
                        }
                        break;

                    case RepairTaskState.Executing:

                        if (record.IsPendingApproval)
                        {
                            EmitActionExecuteRepair(record, repairTask);
                        }
                        break;

                    case RepairTaskState.Restoring:

                        if (IsTimestampOld(repairTask.RestoringTimestamp, this.configReader.OverdueRepairTaskRestoringThreshold))
                        {
                            traceType.WriteWarning(
                                "Repair task {0} has been in Restoring since {1:O} ({2} ago)",
                                repairTask.TaskId,
                                repairTask.RestoringTimestamp,
                                DateTime.UtcNow - repairTask.RestoringTimestamp);

                            this.OverdueRepairTaskCount++;
                        }
                        break;

                    default:
                        isMatch = false;
                        break;
                }
            }

            return isMatch;
        }

        private static bool IsTimestampOld(DateTime? timestamp, TimeSpan ageThreshold, bool unknownIsOld = false)
        {
            if (!timestamp.HasValue)
            {
                return unknownIsOld;
            }

            return (DateTime.UtcNow - timestamp.Value) > ageThreshold;
        }

        private void ReconcileUnmatchedRepairTask(IRepairTask repairTask)
        {
            if (repairTask.State != RepairTaskState.Completed)
            {
                traceType.WriteInfo(
                    "Repair task {0} in state {1} does not match any pending or active repairs",
                    repairTask.TaskId,
                    repairTask.State);
            }

            if (this.doNotCancelMachineNames.Contains(GetMachineNameFromTaskId(repairTask.TaskId)))
            {
                // e.g. once in SoftWipe Proceed, the machine is in C, and *ImageBurnin tasks proceed without waiting for ask mode
                traceType.WriteInfo(
                    "Repair task {0} will not be cancelled/completed at this time because the current machine state allows additional repairs to proceed without approval",
                    repairTask.TaskId);

                return;
            }

            switch (repairTask.State)
            {
                case RepairTaskState.Preparing:
                    EmitActionCancelRepairTask(repairTask);
                    break;

                case RepairTaskState.Approved:
                    EmitActionMoveRepairTaskToRestoring(repairTask);
                    break;

                case RepairTaskState.Executing:

                    // Dampen cancellation for repair tasks that recently changed to Executing,
                    // to give the repair record time to show up in the active repairs list.
                    if (IsTimestampOld(repairTask.ExecutingTimestamp, this.configReader.MinimumRepairExecutionTime, unknownIsOld: true))
                    {
                        EmitActionMoveRepairTaskToRestoring(repairTask);
                    }
                    else
                    {
                        traceType.WriteInfo(
                            "Delaying completion of repair task {0} because it was moved to Executing recently (at {1:O})",
                            repairTask.TaskId,
                            repairTask.ExecutingTimestamp);
                    }
                    break;
            }

            // Nothing to do in other cases
        }

        private NodeImpactLevel TranslateRepairToNodeImpact(RepairType repairType)
        {
            switch (repairType)
            {
                case RepairType.SwitchService:
                case RepairType.SoftReboot:
                case RepairType.HardReboot:
                case RepairType.OSUpgrade:
                case RepairType.ConfigurationProvision:
                case RepairType.DIPDeploy:
                case RepairType.ServiceRestart:
                case RepairType.BIOSUpgrade:        // AP considers this non-destructive
                case RepairType.FirmwareUpgrade:    // AP considers this non-destructive
                case RepairType.SoftRebootRollout:
                case RepairType.UpstreamRepair:
                case RepairType.ImpNonDestructive:
                case RepairType.AppRollout:
                case RepairType.AzureConfigurationUpdate:
                case RepairType.AzureApplicationUpdate:
                case RepairType.NonDestructiveUpdateMachine:
                    return NodeImpactLevel.Restart;

                default:
                    return (this.configReader.LimitRepairTaskImpactLevel ? NodeImpactLevel.Restart : NodeImpactLevel.RemoveData);
            }
        }

        private void EmitActionCreateRepairTaskInPreparing(MachineMaintenanceRecord record)
        {
            // TODO allow config overrides
            NodeImpactLevel impactLevel = TranslateRepairToNodeImpact(record.RepairType);
            bool performHealthCheck = false; // TODO

            traceType.WriteInfo(
                "Creating repair task for {0} (impact = {1}, health check = {2})",
                record.RecordId,
                impactLevel,
                performHealthCheck);

            EmitAction(new CreateRepairTaskInPreparingAction(
                record,
                this.serviceName,
                impactLevel,
                performHealthCheck));
        }

        private void EmitActionCancelRepairTask(IRepairTask repairTask)
        {
            traceType.WriteInfo("Cancelling repair task {0}", repairTask.TaskId);
            EmitAction(new CancelRepairTaskAction(repairTask));
        }

        private void EmitActionMoveRepairTaskToRestoring(IRepairTask repairTask)
        {
            traceType.WriteInfo("Restoring repair task {0}", repairTask.TaskId);
            EmitAction(new MoveRepairTaskToRestoringAction(repairTask));
        }

        private void EmitActionExecuteRepair(MachineMaintenanceRecord record, IRepairTask repairTask)
        {
            traceType.WriteInfo("Executing repair {0} with approval from repair task {1}", record.RecordId, repairTask.TaskId);
            EmitAction(new ExecuteRepairAction(record, repairTask));
        }

        private void EmitAction(IAction action)
        {
            this.outputActions.Add(action);
        }

        private static string GenerateTaskId(MaintenanceRecordId recordId, DateTime? timestamp)
        {
            recordId.Validate("recordId");

            // A custom date format rather than "O" (ISO 8601) is used here because
            // "O" includes characters that are not allowed in repair task ID strings.
            return $"AP/{recordId}/{timestamp:yyyyMMdd-HHmmss}";
        }

        private static string GetMachineNameFromTaskId(string repairTaskId)
        {
            string[] segments = repairTaskId.Split('/');

            if (segments.Length < 2)
            {
                traceType.WriteWarning("Unable to parse task ID: '{0}'", repairTaskId);
                return string.Empty;
            }

            return segments[1];
        }

        private sealed class CancelRepairTaskAction : IAction
        {
            private readonly IRepairTask repairTask;

            public CancelRepairTaskAction(IRepairTask repairTask)
            {
                this.repairTask = repairTask;
            }

            public Task ExecuteAsync(IRepairManager repairManager)
            {
                return repairManager.CancelRepairTaskAsync(Guid.Empty, this.repairTask);
            }

            public override string ToString()
            {
                return $"CancelRepairTask:{this.repairTask.TaskId}";
            }
        }

        private sealed class MoveRepairTaskToRestoringAction : IAction
        {
            private readonly IRepairTask repairTask;

            public MoveRepairTaskToRestoringAction(IRepairTask repairTask)
            {
                this.repairTask = repairTask;
            }

            public Task ExecuteAsync(IRepairManager repairManager)
            {
                this.repairTask.State = RepairTaskState.Restoring;
                this.repairTask.ResultStatus = RepairTaskResult.Succeeded;

                return repairManager.UpdateRepairExecutionStateAsync(Guid.Empty, this.repairTask);
            }

            public override string ToString()
            {
                return $"MoveRepairTaskToRestoring:{this.repairTask.TaskId}";
            }
        }

        private sealed class ExecuteRepairAction : IAction
        {
            private readonly MachineMaintenanceRecord record;
            private readonly IRepairTask repairTask;

            public ExecuteRepairAction(MachineMaintenanceRecord record, IRepairTask repairTask)
            {
                this.record = record;
                this.repairTask = repairTask;
            }

            public async Task ExecuteAsync(IRepairManager repairManager)
            {
                this.repairTask.State = RepairTaskState.Executing;

                await repairManager.UpdateRepairExecutionStateAsync(Guid.Empty, this.repairTask);

                this.record.IsApproved = true;
            }

            public override string ToString()
            {
                return $"ExecuteRepair:{this.record.RecordId}:{this.repairTask.TaskId}";
            }
        }

        private sealed class CreateRepairTaskInPreparingAction : IAction
        {
            private readonly MachineMaintenanceRecord record;
            private readonly string serviceName;
            private readonly NodeImpactLevel impactLevel;
            private readonly bool performHealthCheck;

            public CreateRepairTaskInPreparingAction(
                MachineMaintenanceRecord record,
                string serviceName,
                NodeImpactLevel impactLevel,
                bool performHealthCheck)
            {
                this.record = record.Validate("record");
                this.serviceName = serviceName.Validate("serviceName");
                this.impactLevel = impactLevel;
                this.performHealthCheck = performHealthCheck;
            }

            public async Task ExecuteAsync(IRepairManager repairManager)
            {
                string taskId = GenerateTaskId(this.record.RecordId, DateTime.UtcNow);

                var repairTask = repairManager.NewRepairTask(taskId, this.record.RepairType.ToString());
                repairTask.State = RepairTaskState.Preparing;
                repairTask.Executor = this.serviceName;

                var target = new NodeRepairTargetDescription(this.record.MachineName);
                repairTask.Target = target;

                var impact = new NodeRepairImpactDescription();
                impact.ImpactedNodes.Add(new NodeImpact(this.record.MachineName, this.impactLevel));
                repairTask.Impact = impact;
                repairTask.PerformPreparingHealthCheck = this.performHealthCheck;

                await repairManager.CreateRepairTaskAsync(Guid.Empty, repairTask);
            }

            public override string ToString()
            {
                return $"CreateRepairTaskInPreparing:{this.record.RecordId}:{this.impactLevel}:{this.performHealthCheck}";
            }
        }
    }
}
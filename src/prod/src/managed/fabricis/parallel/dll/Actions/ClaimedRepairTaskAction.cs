// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Collections.Generic;
    using Linq;
    using RD.Fabric.PolicyAgent;
    using Repair;
    using Threading.Tasks;

    internal sealed class ClaimedRepairTaskAction : Action
    {
        private readonly IRepairManager repairManager;
        private readonly IRepairTask repairTask;
        private readonly IList<string> roleInstanceNames;
        private readonly RepairActionProvider repairActionProvider;

        public ClaimedRepairTaskAction(
            CoordinatorEnvironment environment,
            IRepairManager repairManager, 
            IRepairTask repairTask,
            IList<string> roleInstanceNames,
            RepairActionProvider repairActionProvider)
            : base(environment, ActionType.None)
        {
            this.repairManager = repairManager.Validate("repairManager");
            this.repairTask = repairTask.Validate("repairTask");
            this.roleInstanceNames = roleInstanceNames.Validate("roleInstanceName");
            this.repairActionProvider = repairActionProvider.Validate("repairActionProvider");
        }

        public override async Task ExecuteAsync(Guid activityId)
        {
            if (repairTask.State != RepairTaskState.Created)
            {
                this.Environment.DefaultTraceType.WriteInfo("Repair task {0}: not claiming it since its state isn't {1}", repairTask.TaskId, RepairTaskState.Created);
                return;
            }

            // Is this an action that this executor can process?
            RepairActionTypeEnum? action = repairActionProvider.GetRepairAction(repairTask.Action);
            if (action == null)
            {
                this.Environment.DefaultTraceType.WriteInfo("Repair task {0}: ignoring unknown action {1}", repairTask.TaskId, repairTask.Action);
                return;
            }

            if (repairTask.Scope.Kind != RepairScopeIdentifierKind.Cluster)
            {
                this.Environment.DefaultTraceType.WriteInfo("Repair task {0}: ignoring unknown scope {1}", repairTask.TaskId, repairTask.Scope);
                return;
            }

            // Ignore requests for anything other than a single node
            IList<string> targetNodeNames = repairTask.GetTargetNodeNames();
            if (targetNodeNames == null || targetNodeNames.Count != 1)
            {
                this.Environment.DefaultTraceType.WriteInfo("Repair task {0}: ignoring bad target description {1}", repairTask.TaskId, repairTask.Target);
                return;
            }

            // Map Service Fabric node name to Azure role instance Id
            string targetRoleInstance = targetNodeNames[0].TranslateNodeNameToRoleInstance();

            // Is the target a valid role instance in this tenant?
            if (!roleInstanceNames.Contains(targetRoleInstance, StringComparer.OrdinalIgnoreCase))
            {
                this.Environment.DefaultTraceType.WriteInfo("Repair task {0}: ignoring unknown role instance {1}", repairTask.TaskId, targetRoleInstance);
                return;
            }

            // All checks passed; this executor can claim and execute this repair task
            repairTask.Executor = this.Environment.ServiceName;

            // Zero impact, because approval for actual execution will go through the
            // existing infrastructure task mechanism (MR job queue)
            repairTask.State = RepairTaskState.Claimed;

            await repairManager.UpdateRepairExecutionStateAsync(activityId, repairTask).ConfigureAwait(false);
        }

        public override string ToString()
        {
            var targetNodeName = repairTask.GetTargetNodeNames() != null
                ? repairTask.GetTargetNodeNames().FirstOrDefault()
                : null;

            return "{0}, RepairTask: {1}, Node: {2}".ToString(base.ToString(), repairTask, targetNodeName);
        }
    }
}
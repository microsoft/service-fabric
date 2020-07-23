// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Repair;
    using Threading.Tasks;

    internal sealed class MoveToExecutingAction : Action
    {
        private readonly IRepairManager repairManager;
        private readonly IRepairTask repairTask;
        private readonly string warning;

        public MoveToExecutingAction(CoordinatorEnvironment environment, IRepairManager repairManager, IRepairTask repairTask, string warning = null)
            : base(environment, ActionType.Execute)
        {
            this.repairManager = repairManager.Validate("repairManager");
            this.repairTask = repairTask.Validate("repairTask");
            this.warning = warning;
        }

        public override Task ExecuteAsync(Guid activityId)
        {
            repairTask.State = RepairTaskState.Executing;

            if (warning != null)
            {
                this.Environment.DefaultTraceType.WriteWarning("{0}", warning);
            }

            return repairManager.UpdateRepairExecutionStateAsync(activityId, repairTask);
        }

        public override string ToString()
        {
            return "{0}, RepairTask: {1}".ToString(base.ToString(), repairTask);
        }
    }
}
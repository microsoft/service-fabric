// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Threading.Tasks;

    internal sealed class CancelRepairTaskAction : Action
    {
        private readonly IRepairManager repairManager;
        private readonly IRepairTask repairTask;

        public CancelRepairTaskAction(CoordinatorEnvironment environment, IRepairManager repairManager, IRepairTask repairTask)
            : base(environment, ActionType.None)
        {
            this.repairManager = repairManager.Validate("repairManager");
            this.repairTask = repairTask.Validate("repairTask");
        }

        public override Task ExecuteAsync(Guid activityId)
        {
            return repairManager.CancelRepairTaskAsync(activityId, repairTask);
        }

        public override string ToString()
        {
            return "{0}, RepairTask: {1}".ToString(base.ToString(), repairTask);
        }
    }
}
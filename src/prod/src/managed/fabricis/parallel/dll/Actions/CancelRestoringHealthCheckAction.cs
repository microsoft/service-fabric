// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Threading.Tasks;

    internal sealed class CancelRestoringHealthCheckAction : Action
    {
        private readonly IRepairManager repairManager;
        private readonly IRepairTask repairTask;

        public CancelRestoringHealthCheckAction(
            CoordinatorEnvironment environment,
            IRepairManager repairManager,
            IRepairTask repairTask)
            : base(environment, ActionType.Restore)
        {
            this.repairManager = repairManager.Validate("repairManager");
            this.repairTask = repairTask.Validate("repairTask");
        }

        public override async Task ExecuteAsync(Guid activityId)
        {
            await repairManager.UpdateRepairTaskHealthPolicyAsync(activityId, repairTask, null, false).ConfigureAwait(false);
        }

        public override string ToString()
        {
            return "{0}, RepairTask: {1}".ToString(base.ToString(), repairTask);
        }
    }
}
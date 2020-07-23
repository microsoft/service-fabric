// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Repair;
    using Threading.Tasks;

    internal sealed class ExecuteJobAction : Action
    {
        private readonly Action<Guid> approveJobAction;
        private readonly IRepairManager repairManager;
        private readonly ITenantJob tenantJob;
        private readonly IRepairTask repairTask;

        public ExecuteJobAction(
            CoordinatorEnvironment environment,
            Action<Guid> approveJobAction,
            IRepairManager repairManager,
            ITenantJob tenantJob,
            IRepairTask repairTask)
            : base(environment, ActionType.None)
        {
            this.approveJobAction = approveJobAction.Validate("approveJobAction");
            this.repairManager = repairManager.Validate("repairManager");
            this.tenantJob = tenantJob.Validate("tenantJob");
            this.repairTask = repairTask.Validate("repairTask");
        }

        public override async Task ExecuteAsync(Guid activityId)
        {
            // TODO: Validate that state is already Executing; this action can have ActionType.None because
            // of this assumption that this action isn't the one that moves from Approved to Executing.
            repairTask.State = RepairTaskState.Executing;
            await repairManager.UpdateRepairExecutionStateAsync(activityId, repairTask).ConfigureAwait(false);

            approveJobAction(tenantJob.Id);
        }

        public override string ToString()
        {
            return "{0}, TenantJob: [{1}], RepairTask: {2}".ToString(base.ToString(), tenantJob, repairTask);
        }
    }
}
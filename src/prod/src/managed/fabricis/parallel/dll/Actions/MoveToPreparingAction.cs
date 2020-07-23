// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using System.Linq;
    using Repair;
    using Threading.Tasks;

    internal sealed class MoveToPreparingAction : Action
    {
        private readonly IRepairManager repairManager;
        private readonly IRepairTask repairTask;
        private readonly ITenantJob tenantJob;
        private readonly bool surpriseJob;
        private readonly RepairTaskPrepareArgs args;

        public MoveToPreparingAction(
            CoordinatorEnvironment environment,
            IRepairManager repairManager,
            IRepairTask repairTask,
            ITenantJob tenantJob,
            bool surpriseJob,
            RepairTaskPrepareArgs args)
            : base(environment, ActionType.Prepare)
        {
            this.repairManager = repairManager.Validate("repairManager");
            this.repairTask = repairTask.Validate("repairTask");
            this.tenantJob = tenantJob.Validate("tenantJob");
            this.surpriseJob = surpriseJob;
            this.args = args.Validate("args");
        }

        public override async Task ExecuteAsync(Guid activityId)
        {
            if (surpriseJob)
            {
                // TODO, add surprise in executordata
                this.Environment.DefaultTraceType.WriteWarning("MoveToPreparing: job bypassed normal automated workflow: [{0}]", tenantJob);
            }

            repairTask.State = RepairTaskState.Preparing;
            repairTask.ExecutorData = args.ExecutorData.ToJson();
            repairTask.Impact = args.Impact;

            bool? prepareHc = args.PerformPreparingHealthCheck;
            bool? restoreHc = args.PerformRestoringHealthCheck;

            if (prepareHc == repairTask.PerformPreparingHealthCheck)
            {
                prepareHc = null;
            }

            if (restoreHc == repairTask.PerformRestoringHealthCheck)
            {
                restoreHc = null;
            }

            // since IS has claimed the task, it is free to update the health policy as it seems fit
            if (prepareHc != null || restoreHc != null)
            {
                // TODO: See if this can be removed by allowing health policy updates via UpdateRepairExecutionStateAsync
                // (currently those ignored for backward compatibility reasons)
                repairTask.Version = await repairManager.UpdateRepairTaskHealthPolicyAsync(activityId, repairTask, prepareHc, restoreHc).ConfigureAwait(false);
            }

            await repairManager.UpdateRepairExecutionStateAsync(activityId, repairTask).ConfigureAwait(false);
        }

        public override string ToString()
        {
            return "{0}, RepairTask: {1}, TenantJob: {2}, IsSurpriseJob: {3}, Args: {4}".ToString(base.ToString(), repairTask, tenantJob, surpriseJob, args);
        }
    }
}
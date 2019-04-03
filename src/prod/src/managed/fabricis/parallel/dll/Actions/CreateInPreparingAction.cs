// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Repair;
    using Threading.Tasks;

    internal sealed class CreateInPreparingAction : Action
    {
        private readonly IRepairManager repairManager;
        private readonly ITenantJob tenantJob;
        private readonly bool surpriseJob;
        private readonly RepairTaskPrepareArgs args;

        public CreateInPreparingAction(
            CoordinatorEnvironment environment,
            IRepairManager repairManager, 
            ITenantJob tenantJob,
            bool surpriseJob,
            RepairTaskPrepareArgs args)
            : base(environment, ActionType.Prepare)
        {
            this.repairManager = repairManager.Validate("repairManager");
            this.tenantJob = tenantJob.Validate("tenantJob");
            this.surpriseJob = surpriseJob;
            this.args = args.Validate("args");
        }

        public override async Task ExecuteAsync(Guid activityId)
        {
            if (surpriseJob)
            {
                // TODO additional info in executordata
                this.Environment.DefaultTraceType.WriteWarning("CreateInPreparing: job bypassed normal automated workflow: [{0}]", tenantJob);
            }

            // version parameter isn't used while creating a new task. it is only used during updates (for optimistic concurrency)
            var repairTask = repairManager.NewRepairTask(args.TaskId, args.Action);

            repairTask.State = RepairTaskState.Preparing;
            repairTask.Description = args.Description;
            repairTask.Executor = this.Environment.ServiceName;

            repairTask.ExecutorData = args.ExecutorData.ToJson();
            repairTask.Target = args.Target;
            repairTask.Impact = args.Impact;
            repairTask.PerformPreparingHealthCheck = args.PerformPreparingHealthCheck;
            repairTask.PerformRestoringHealthCheck = args.PerformRestoringHealthCheck;

            await repairManager.CreateRepairTaskAsync(activityId, repairTask).ConfigureAwait(false);
        }

        public override string ToString()
        {
            return "{0}, TenantJob: [{1}], Args: {2}, IsSurpriseJob: {3}".ToString(base.ToString(), tenantJob, args, surpriseJob);
        }
    }
}
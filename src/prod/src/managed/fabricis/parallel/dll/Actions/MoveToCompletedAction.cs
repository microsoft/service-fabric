// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Repair;
    using Threading.Tasks;

    internal sealed class MoveToCompletedAction : Action
    {
        private readonly IRepairManager repairManager;
        private readonly IRepairTask repairTask;
        private readonly RepairTaskResult repairTaskResult;
        private readonly string repairTaskResultDetails;
        private readonly bool surpriseJob;

        public MoveToCompletedAction(
            CoordinatorEnvironment environment,
            IRepairManager repairManager,
            IRepairTask repairTask,
            RepairTaskResult repairTaskResult,
            string repairTaskResultDetails,
            bool surpriseJob)
            : base(environment, ActionType.None)
        {
            this.repairManager = repairManager.Validate("repairManager");
            this.repairTask = repairTask.Validate("repairTask");
            this.repairTaskResult = repairTaskResult;
            this.repairTaskResultDetails = repairTaskResultDetails;
            this.surpriseJob = surpriseJob;
        }

        public override async Task ExecuteAsync(Guid activityId)
        {
            repairTask.State = RepairTaskState.Completed;
            repairTask.ResultStatus = repairTaskResult;
            repairTask.ResultDetails = repairTaskResultDetails;

            if (surpriseJob)
            {
                // TODO, add surprise in executordata
                this.Environment.DefaultTraceType.WriteWarning("MoveToCompleted: job bypassed normal automated workflow for repair task '{0}'", repairTask.TaskId);
            }

            await repairManager.UpdateRepairExecutionStateAsync(activityId, repairTask).ConfigureAwait(false);

            // assuming above call succeeds. Else exceptions are handled by caller and the tracing code below doesn't execute
            Environment.DefaultTraceType.WriteAggregatedEvent(
                "CompletedRepairTask", 
                repairTask.TaskId, 
                "{0}", 
                repairTask.ToJson());
        }

        public override string ToString()
        {
            return "{0}, RepairTask: {1}, IsSurpriseJob: {2}".ToString(
                base.ToString(), repairTask, surpriseJob);
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel.Test
{
    using Collections.Generic;
    using RD.Fabric.PolicyAgent;
    using Repair;
    using Threading;
    using Threading.Tasks;

    /// <summary>
    /// Wraps a simple workflow by mocking Repair Manager (RM) and Policy Agent Service (PA 
    /// or Fabric Controller - FC). 
    /// This tests the state machine changes inside of IS. This is a happy path scenario. i.e.
    /// In this case, 
    /// a. PA creates a new job document
    /// b. IS polls and gets this job
    /// c. IS invokes RM to create a repair task corresponding to the job, RM approves it
    /// d. IS acks the job, so PA can work on it
    /// e. PA then updates job status and tosses job to IS for a final ack
    /// f. IS updates the corresponding repair task status. RM then marks it as completed
    /// g. IS sees RM marking repair task as completed, it acks the job again
    /// h. PA sees this ack and marks the job as completed
    /// i. IS checks if RM has also marked the repair task (corresponding to this job) as completed
    /// j. IS then invokes RM to delete the repair task.
    /// </summary>
    internal class WorkflowRunAsyncTest2 : BaseWorkflowExecutor
    {
        public WorkflowRunAsyncTest2()
            : base(
                new Dictionary<string, string>(),
                new FC(),
                new RM())
        {
        }

        protected override void Run(CancellationToken cancellationToken)
        {
            var tenantJob = TenantJobHelper.CreateNewTenantJob();

            PolicyAgentService.CreateTenantJob(tenantJob);

            RepairManager.CompletedEvent.WaitOne();
            CompletedEvent.Set();
        }

        private class FC : MockPolicyAgentService
        {
            private int policyAgentStage;

            public override async Task PostPolicyAgentRequestAsync(
                Guid activityId,
                PolicyAgentRequest request,
                CancellationToken cancellationToken)
            {
                request.Validate("request");

                if (policyAgentStage == 0)
                {
                    SendImpactEnd(request);
                }
                else if (policyAgentStage == 1)
                {
                    SendCompletion(request, true);
                }

                policyAgentStage++;

                await Task.FromResult(0);
            }
        }

        private class RM : MockRepairManager
        {
            protected override void AfterUpdateRepairExecutionState(Guid activityId, MockRepairTask repairTask)
            {
                base.AfterUpdateRepairExecutionState(activityId, repairTask);

                if (repairTask.State == RepairTaskState.Completed)
                {
                    CompletedEvent.Set();
                }
            }
        }
    }
}
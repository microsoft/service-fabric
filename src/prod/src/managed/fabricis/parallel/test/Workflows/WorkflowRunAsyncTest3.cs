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
    /// Similar to <see cref="WorkflowRunAsyncTest2"/>. In this case, the job moves UD by UD.
    /// For each UD, a new repair task is created and processed till completion.
    /// </summary>
    internal class WorkflowRunAsyncTest3 : BaseWorkflowExecutor
    {
        public WorkflowRunAsyncTest3()
            : base(
                new Dictionary<string, string>(),
                new FC(),
                new RM())
        {
        }

        protected override void Run(CancellationToken cancellationToken)
        {
            var tenantJob = TenantJobHelper.CreateNewTenantJob();

            // this kicks things off
            PolicyAgentService.CreateTenantJob(tenantJob);

            RepairManager.CompletedEvent.WaitOne();
            CompletedEvent.Set();
        }

        private class FC : MockPolicyAgentService
        {
            private int policyAgentStage;

            private void SendNewJobStep(PolicyAgentRequest request)
            {
                foreach (JobStepResponse jobStepResponse in request.JobResponse.JobStepResponses)
                {
                    Guid guid = jobStepResponse.JobId.ToGuid();
                    var id = guid;

                    MockTenantJob tenantJob = (MockTenantJob)GetTenantJob(id);

                    tenantJob.JobStep = TenantJobHelper.CreateNewJobStepInfo(1, new List<string> { "Role_IN_2", "Role_IN_3" });

                    // update job after it does its job, so that IS gets it again when it polls for it.
                    UpdateTenantJob(tenantJob);
                }
            }

            private void SendImpactEndOnSecondUD(PolicyAgentRequest request)
            {
                SendImpactEnd(request);
            }

            public override async Task PostPolicyAgentRequestAsync(
                Guid activityId,
                PolicyAgentRequest request,
                CancellationToken cancellationToken)
            {
                request.Validate("request");

                switch (policyAgentStage)
                {
                    case 0:
                        SendImpactEnd(request);
                        break;
                    case 1:
                        SendNewJobStep(request);
                        break;
                    case 2:
                        SendImpactEndOnSecondUD(request);
                        break;
                    case 3:
                        SendCompletion(request, true);
                        break;
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
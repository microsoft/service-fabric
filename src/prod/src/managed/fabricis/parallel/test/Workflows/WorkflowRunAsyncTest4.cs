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
    /// Root HE scenario where the job moves UD by UD. Then comes back and re-iterates over the UDs.
    /// </summary>
    internal class WorkflowRunAsyncTest4 : BaseWorkflowExecutor
    {
        public WorkflowRunAsyncTest4()
            : base(
                new Dictionary<string, string>(),
                new FC(),
                new RM())
        {
        }

        protected override void Run(CancellationToken cancellationToken)
        {
            var tenantJob = TenantJobHelper.CreateNewTenantJob(ImpactActionEnum.PlatformUpdate); // Root HE

            // this kicks things off
            PolicyAgentService.CreateTenantJob(tenantJob);

            RepairManager.CompletedEvent.WaitOne();
            CompletedEvent.Set();
        }

        private class FC : MockPolicyAgentService
        {
            private int policyAgentStage;

            public async override Task PostPolicyAgentRequestAsync(
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
                        SendNewJobStep(request, 1, new List<string> { "Role_IN_2", "Role_IN_3" });
                        break;
                    case 2:
                        SendImpactEndOnSecondUD(request);
                        break;
                    case 3:
                        SendNewJobStep(request, 0, new List<string> { "Role_IN_0", "Role_IN_1" });
                        break;
                    case 4:
                        SendImpactEnd(request);
                        break;
                    case 5:
                        SendNewJobStep(request, 1, new List<string> { "Role_IN_2", "Role_IN_3" });
                        break;
                    case 6:
                        SendImpactEndOnSecondUD(request);
                        break;
                    case 7:
                        SendCompletion(request, true);
                        break;
                }

                policyAgentStage++;

                await Task.FromResult(0);
            }

            private void SendNewJobStep(PolicyAgentRequest request, uint ud, List<string> roleInstances)
            {
                foreach (JobStepResponse jobStepResponse in request.JobResponse.JobStepResponses)
                {
                    Guid guid = jobStepResponse.JobId.ToGuid();
                    var id = guid;

                    var tenantJob = (MockTenantJob)GetTenantJob(id);

                    tenantJob.JobStep = TenantJobHelper.CreateNewJobStepInfo(ud, roleInstances);

                    // update job after it does its job, so that IS gets it again when it polls for it.
                    UpdateTenantJob(tenantJob);
                }
            }

            private void SendImpactEndOnSecondUD(PolicyAgentRequest request)
            {
                SendImpactEnd(request);
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
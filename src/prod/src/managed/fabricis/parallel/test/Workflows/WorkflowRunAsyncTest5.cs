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
    /// Job is suspended midway and while the repair task winds down, the job is resumed. 
    /// A new repair task has to be created to handle this.
    /// This scenario is pretty common in root HE where another tenant owner or FC owner
    /// suspends the root HE, then resumes it, say after a few days.
    /// So, 2 repair tasks exist for a given tenant job. And both should eventually be completed.
    /// </summary>
    /// TODO fix this test!!!!
    internal class WorkflowRunAsyncTest5 : BaseWorkflowExecutor
    {
        private readonly ISClient infrastructureServiceClient = new ISClient();

        public WorkflowRunAsyncTest5()
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

        private class ISClient
        {
            public void RequestSuspend(IInfrastructureCoordinator coordinator, Guid jobId)
            {
                coordinator
                    .RunCommandAsync(true, "RequestSuspend:{0}".ToString(jobId), TimeSpan.MaxValue, CancellationToken.None)
                    .GetAwaiter()
                    .GetResult();
            }    
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
                        SuspendAndResumeJob(request, TimeSpan.FromSeconds(15));
                        break;

                    case 1:
                        SendImpactEnd(request);
                        break;

                    case 2:
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
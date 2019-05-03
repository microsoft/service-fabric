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
    /// Workflow for a repair task created externally via RM (Start-ServiceFabricRepairTask).
    /// In this case, the repair task is in Created State. IS acts like the repair executor (RE)
    /// and claims the task and requests maintenance to FC. Then, once the tenant job arrives, it
    /// matches the repair task with the tenant job (from the ContextId field of the job), then
    /// marks it as Preparing and lets it go through the normal workflow.
    /// </summary>
    internal class WorkflowExternalRepairTaskTest : BaseWorkflowExecutor
    {
        public WorkflowExternalRepairTaskTest()
            : base(
                new Dictionary<string, string>(),
                new FC(),
                new RM())
        {
        }

        protected override void Run(CancellationToken cancellationToken)
        {
            IRepairTask repairTask = CreateRepairTask();
            RepairManager.CreateRepairTaskAsync(Guid.NewGuid(), repairTask).GetAwaiter().GetResult();
            
            RepairManager.CompletedEvent.WaitOne();
            PolicyAgentService.CompletedEvent.WaitOne();
            CompletedEvent.Set();
        }

        private IRepairTask CreateRepairTask()
        {
            var repairTask = RepairManager.NewRepairTask(
                Guid.NewGuid().ToString(),
                InfrastructureService.Parallel.Constants.RepairActionFormat.ToString(RepairActionTypeEnum.Reboot));

            repairTask.Target = new NodeRepairTargetDescription("Role.0");
            repairTask.State = RepairTaskState.Created;
            repairTask.Description = "Creating a repair task externally and not via IS";

            return repairTask;
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

        private class FC : MockPolicyAgentService
        {
            private readonly HashSet<string> repairRequests = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

            private int policyAgentStage;

            /// <summary>
            /// When IS makes a RepairRequest post to PolicyAgent, then we create a new tenant job in response
            /// </summary>
            public override async Task PostPolicyAgentRequestAsync(
                Guid activityId,
                PolicyAgentRequest request,
                CancellationToken cancellationToken)
            {
                if (request.RepairRequest != null && request.RepairRequest.RoleInstanceId != null)
                {
                    string id = request.RepairRequest.RoleInstanceId + "/" + request.RepairRequest.Action;

                    if (!repairRequests.Contains(id))
                    {
                        var tenantJob = TenantJobHelper.CreateNewTenantJob(
                            ImpactActionEnum.TenantMaintenance, 
                            0, 
                            new List<string> { "Role_IN_1" }, 
                            request.RepairRequest.ContextId);
                        CreateTenantJob(tenantJob);

                        repairRequests.Add(id);
                        return;
                    }
                }

                if (policyAgentStage == 0)
                {
                    SendImpactEnd(request);
                }
                else if (policyAgentStage == 1)
                {
                    SendCompletion(request, true);
                    CompletedEvent.Set();
                }

                policyAgentStage++;
               
                await Task.FromResult(0);
            }
        }
    }
}
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

    /// <summary>
    /// This tests if a repair task is canceled if it doesn't receive a matching tenant job within a certain threshold
    /// after going into the Claimed state.    
    /// TODO fix this test and add the TestMethod attribute in the caller.
    /// </summary>
    internal class WorkflowExternalRepairTaskWithFCNotRespondingTest : BaseWorkflowExecutor
    {
        public WorkflowExternalRepairTaskWithFCNotRespondingTest()
            : base(
                new Dictionary<string, string> { { Parallel.Constants.ConfigKeys.CancelThresholdForUnmatchedRepairTask, TimeSpan.FromSeconds(15).ToString() }},
                new FC(),
                new RM())
        {
        }

        protected override void Run(CancellationToken cancellationToken)
        {
            IRepairTask repairTask = CreateRepairTask();
            RepairManager.CreateRepairTaskAsync(Guid.NewGuid(), repairTask).GetAwaiter().GetResult();
            
            ((MockRepairTask)repairTask).CreatedTimestamp = DateTime.UtcNow;            

            RepairManager.CompletedEvent.WaitOne();
            CompletedEvent.Set();
        }

        private static IRepairTask CreateRepairTask()
        {
            var repairTask = new MockRepairTask(
                "rt1",
                Parallel.Constants.RepairActionFormat.ToString(RepairActionTypeEnum.Reboot))
            {
                Target = new NodeRepairTargetDescription("Role.0"),
                State = RepairTaskState.Created,
                Description = "Creating a repair task externally and not via IS",
            };

            return repairTask;
        }

        private class RM : MockRepairManager
        {
            protected override void AfterUpdateRepairExecutionState(Guid activityId, MockRepairTask repairTask)
            {
                base.AfterUpdateRepairExecutionState(activityId, repairTask);

                if (repairTask.State == RepairTaskState.Claimed && !repairTask.ClaimedTimestamp.HasValue)
                {
                    repairTask.ClaimedTimestamp = DateTime.UtcNow;
                }

                if (repairTask.State == RepairTaskState.Completed && repairTask.ResultStatus == RepairTaskResult.Cancelled)
                {
                    CompletedEvent.Set();
                }
            }
        }

        private class FC : MockPolicyAgentService
        {            
        }                   
    }
}
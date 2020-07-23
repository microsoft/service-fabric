// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel.Test
{
    using Collections.Generic;
    using Linq;
    using Repair;
    using Threading;
    using Threading.Tasks;

    internal class MockRepairManager : IRepairManager
    {
        private readonly IDictionary<string, IRepairTask> repairTasks = new Dictionary<string, IRepairTask>(StringComparer.OrdinalIgnoreCase);

        /// <summary>
        /// Used to indicate that a work flow is complete. The mock repair manager instance shouldn't be used for 
        /// new repair processing after this.
        /// </summary>
        public ManualResetEvent CompletedEvent { get; private set; }

        public MockRepairManager()
        {
            CompletedEvent = new ManualResetEvent(false);
        }

        public IRepairTask NewRepairTask(string taskId, string action)
        {
            return new MockRepairTask(taskId, action);
        }

        public Task RemoveNodeStateAsync(Guid activityId, string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            Constants.TraceType.WriteInfo("RemoveNodeState: {0}", nodeName);
            return Task.FromResult(0);
        }

        private static MockRepairTask VerifyMockRepairTask(IRepairTask repairTask)
        {
            return (MockRepairTask)repairTask;
        }

        public Task<long> CreateRepairTaskAsync(Guid activityId, IRepairTask repairTask)
        {
            repairTask.Validate("repairTask");
            var mockTask = VerifyMockRepairTask(repairTask);

            if (repairTasks.ContainsKey(repairTask.TaskId))
            {
                Constants.TraceType.WriteInfo("Repair task {0} already exists. No action taken. Details: {1}", repairTask.TaskId, repairTask);
            }
            else
            {                
                repairTasks[repairTask.TaskId] = repairTask;
            }

            AfterCreateRepairTask(activityId, mockTask);

            return Task.FromResult(0L);            
        }

        public Task<long> UpdateRepairExecutionStateAsync(Guid activityId, IRepairTask repairTask)
        {
            var mockTask = VerifyMockRepairTask(repairTask);

            repairTasks[repairTask.TaskId] = repairTask;

            AfterUpdateRepairExecutionState(activityId, mockTask);

            return Task.FromResult(0L);
        }

        public Task<IList<IRepairTask>> GetRepairTaskListAsync(
            Guid activityId, 
            string taskIdFilter = null, 
            RepairTaskStateFilter stateFilter = RepairTaskStateFilter.Default,
            string executorFilter = null)
        {
            IList<IRepairTask> matchingRepairTasks = new List<IRepairTask>();

            foreach (var rt in repairTasks.Values)
            {
                if (taskIdFilter != null && !rt.TaskId.StartsWith(taskIdFilter, StringComparison.OrdinalIgnoreCase))
                {
                    continue;
                }

                if (executorFilter != null)
                {
                    if (rt.Executor == null)
                    {
                        continue;
                    }

                    if (!rt.Executor.StartsWith(executorFilter, StringComparison.OrdinalIgnoreCase))
                    {
                        continue;
                    }
                }

                if (stateFilter != RepairTaskStateFilter.Default)
                {
                    var stateMask = (RepairTaskStateFilter) rt.State;
                    if (stateMask != stateFilter)
                    {
                        continue;
                    }
                }

                matchingRepairTasks.Add(rt);
            }

            return Task.FromResult(matchingRepairTasks);
        }

        public Task<long> CancelRepairTaskAsync(Guid activityId, IRepairTask repairTask)
        {
            VerifyMockRepairTask(repairTask);

            // TODO - mock a safe cancellation (not a request abort)
            repairTask.ResultStatus = RepairTaskResult.Cancelled;
            repairTask.State = RepairTaskState.Completed;
            return Task.FromResult(0L);
        }

        public Task<long> UpdateRepairTaskHealthPolicyAsync(
            Guid activityId, 
            IRepairTask repairTask, 
            bool? performPreparingHealthCheck,
            bool? performRestoringHealthCheck)
        {
            VerifyMockRepairTask(repairTask);
            // Note: no implementation. For now, set the corresponding properties accordingly
            return Task.FromResult(0L);
        }

        /// <summary>
        /// TODO, consider providing overloads in case we need timeout and cancellationToken parameters.
        /// </summary>
        protected virtual void AfterUpdateRepairExecutionState(
            Guid activityId, 
            MockRepairTask repairTask)
        {
            bool stateChanged = false;

            switch (repairTask.State)
            {
                case RepairTaskState.Claimed:
                    repairTask.ClaimedTimestamp = DateTime.UtcNow;
                    break;
                case RepairTaskState.Preparing:
                    repairTask.PreparingTimestamp = DateTime.UtcNow;
                    repairTask.ApprovedTimestamp = DateTime.UtcNow;
                    repairTask.State = RepairTaskState.Approved;
                    stateChanged = true;
                    break;
                case RepairTaskState.Executing:
                    repairTask.ExecutingTimestamp = DateTime.UtcNow;
                    break;
                case RepairTaskState.Restoring:
                    repairTask.RestoringTimestamp = DateTime.UtcNow;
                    repairTask.CompletedTimestamp = DateTime.UtcNow;
                    repairTask.State = RepairTaskState.Completed;
                    stateChanged = true;
                    break;
                default:
                    return;
            }

            if (stateChanged)
            {
                UpdateRepairExecutionStateAsync(activityId, repairTask).GetAwaiter().GetResult();
            }
        }

        protected virtual void AfterCreateRepairTask(
            Guid activityId,
            MockRepairTask repairTask)
        {
            // the "real" repair manager requires repair tasks to be created with the initial state as Preparing
            if (repairTask.State != RepairTaskState.Preparing)
            {
                return;
            }

            repairTask.State = RepairTaskState.Approved;
            UpdateRepairExecutionStateAsync(activityId, repairTask).GetAwaiter().GetResult();
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Autopilot.Test
{
    using Autopilot;
    using Collections.Generic;
    using Repair;
    using Threading;
    using Threading.Tasks;

    internal class NullRepairManager : IRepairManager
    {
        public Task<long> CancelRepairTaskAsync(Guid activityId, IRepairTask repairTask)
        {
            return Task.FromResult(0L);
        }

        public Task<long> CreateRepairTaskAsync(Guid activityId, IRepairTask repairTask)
        {
            return Task.FromResult(0L);
        }

        public Task<IList<IRepairTask>> GetRepairTaskListAsync(Guid activityId, string taskIdFilter = null, RepairTaskStateFilter stateFilter = RepairTaskStateFilter.Default, string executorFilter = null)
        {
            return Task.FromResult((IList<IRepairTask>)new List<IRepairTask>());
        }

        public IRepairTask NewRepairTask(string taskId, string action)
        {
            return new MockRepairTask(taskId, action);
        }

        public Task RemoveNodeStateAsync(Guid activityId, string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.FromResult(0L);
        }

        public Task<long> UpdateRepairExecutionStateAsync(Guid activityId, IRepairTask repairTask)
        {
            return Task.FromResult(0L);
        }

        public Task<long> UpdateRepairTaskHealthPolicyAsync(Guid activityId, IRepairTask repairTask, bool? performPreparingHealthCheck, bool? performRestoringHealthCheck)
        {
            return Task.FromResult(0L);
        }
    }
}
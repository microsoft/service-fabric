// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Autopilot
{
    using Collections.Generic;
    using Threading.Tasks;
    using Repair;
    using Threading;
    /// <summary>
    /// Wrapper for <see cref="FabricClient.RepairManagementClient"/>.
    /// </summary>
    internal interface IRepairManager
    {
        IRepairTask NewRepairTask(string taskId, string action);

        Task RemoveNodeStateAsync(Guid activityId, string nodeName, TimeSpan timeout, CancellationToken cancellationToken);

        Task<Int64> CreateRepairTaskAsync(Guid activityId, IRepairTask repairTask);

        Task<Int64> UpdateRepairExecutionStateAsync(Guid activityId, IRepairTask repairTask);

        Task<IList<IRepairTask>> GetRepairTaskListAsync(
            Guid activityId,
            string taskIdFilter = null,
            RepairTaskStateFilter stateFilter = RepairTaskStateFilter.Default,
            string executorFilter = null);

        /// <summary>
        /// Only safe-cancel is supported. Request abort is not.
        /// </summary>
        Task<long> CancelRepairTaskAsync(Guid activityId, IRepairTask repairTask);

        Task<Int64> UpdateRepairTaskHealthPolicyAsync(
            Guid activityId, IRepairTask repairTask, bool? performPreparingHealthCheck, bool? performRestoringHealthCheck);
    }
}
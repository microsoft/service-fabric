// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Repair;

    /// <summary>
    /// A wrapper over <see cref="RepairTask"/> mainly for unit-testability.
    /// </summary>
    internal class ServiceFabricRepairTask : IRepairTask
    {
        private readonly RepairTask repairTask;

        public ServiceFabricRepairTask(RepairTask repairTask)
        {
            this.repairTask = repairTask.Validate("repairTask");
        }

        public RepairScopeIdentifier Scope { get { return repairTask.Scope; } }

        public string TaskId { get { return repairTask.TaskId; } }

        public long Version { get { return repairTask.Version; } set { repairTask.Version = value; } }

        public string Description { get { return repairTask.Description; } set { repairTask.Description = value; } }

        public RepairTaskState State { get { return repairTask.State; } set { repairTask.State = value; } }

        public RepairTaskFlags Flags { get { return repairTask.Flags; } }

        public string Action { get { return repairTask.Action; } }

        public RepairTargetDescription Target { get { return repairTask.Target; } set { repairTask.Target = value; } }

        public string Executor { get { return repairTask.Executor; } set { repairTask.Executor = value; } }

        public string ExecutorData { get { return repairTask.ExecutorData; } set { repairTask.ExecutorData = value; } }

        public RepairImpactDescription Impact { get { return repairTask.Impact; } set { repairTask.Impact = value; } }

        public RepairTaskResult ResultStatus { get { return repairTask.ResultStatus; } set { repairTask.ResultStatus = value; } }

        public int ResultCode { get { return repairTask.ResultCode; } set { repairTask.ResultCode = value; } }

        public string ResultDetails { get { return repairTask.ResultDetails; } set { repairTask.ResultDetails = value; } }

        public DateTime? CreatedTimestamp { get { return repairTask.CreatedTimestamp; } }

        public DateTime? ClaimedTimestamp { get { return repairTask.ClaimedTimestamp; } }

        public DateTime? PreparingTimestamp { get { return repairTask.PreparingTimestamp; } }

        public DateTime? ApprovedTimestamp { get { return repairTask.ApprovedTimestamp; } }

        public DateTime? ExecutingTimestamp { get { return repairTask.ExecutingTimestamp; } }

        public DateTime? RestoringTimestamp { get { return repairTask.RestoringTimestamp; } }

        public DateTime? CompletedTimestamp { get { return repairTask.CompletedTimestamp; } }

        public DateTime? PreparingHealthCheckStartTimestamp { get { return repairTask.PreparingHealthCheckStartTimestamp; } }

        public DateTime? PreparingHealthCheckEndTimestamp { get { return repairTask.PreparingHealthCheckEndTimestamp; } }

        public DateTime? RestoringHealthCheckStartTimestamp { get { return repairTask.RestoringHealthCheckStartTimestamp; } }

        public DateTime? RestoringHealthCheckEndTimestamp { get { return repairTask.RestoringHealthCheckEndTimestamp; } }

        public RepairTaskHealthCheckState PreparingHealthCheckState { get { return repairTask.PreparingHealthCheckState; } }

        public RepairTaskHealthCheckState RestoringHealthCheckState { get { return repairTask.RestoringHealthCheckState; } }

        public bool PerformPreparingHealthCheck { get { return repairTask.PerformPreparingHealthCheck; } set { repairTask.PerformPreparingHealthCheck = value; } }

        public bool PerformRestoringHealthCheck { get { return repairTask.PerformRestoringHealthCheck; } set { repairTask.PerformRestoringHealthCheck = value; } }

        /// <summary>
        /// Get's the underlying 'real' repair task that this class wraps.
        /// </summary>
        /// <remarks>
        /// Making it a method instead of a property to avoid <see cref="CoordinatorHelper.ToJson"/> method showing an inner repair task property also
        /// while deserializing an <see cref="IRepairTask"/> object.
        /// </remarks>
        public RepairTask Unwrap()
        {
            return repairTask;
        }

        public override string ToString()
        {
            return "{0}, State: {1}".ToString(TaskId, State);
        }
    }
}
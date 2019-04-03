// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel.Test
{
    using Repair;

    internal sealed class MockRepairTask : IRepairTask
    {
        /// <summary>
        /// Required for automatic Json serialization/deserialization
        /// </summary>
        public MockRepairTask()
        {
        }

        public MockRepairTask(string taskId, string action)
        {
            TaskId = taskId.Validate("taskId");
            Action = action.Validate("action");

            Scope = new ClusterRepairScopeIdentifier();
        }

        public RepairScopeIdentifier Scope { get; set; }
        public string TaskId { get; set; }
        public long Version { get; set; }
        public string Description { get; set; }
        public RepairTaskState State { get; set; }
        public RepairTaskFlags Flags { get; set; }
        public string Action { get; set; }
        public RepairTargetDescription Target { get; set; }
        public string Executor { get; set; }
        public string ExecutorData { get; set; }
        public RepairImpactDescription Impact { get; set; }
        public RepairTaskResult ResultStatus { get; set; }
        public int ResultCode { get; set; }
        public string ResultDetails { get; set; }
        public DateTime? CreatedTimestamp { get; set; }
        public DateTime? ClaimedTimestamp { get; set; }
        public DateTime? PreparingTimestamp { get; set; }
        public DateTime? ApprovedTimestamp { get; set; }
        public DateTime? ExecutingTimestamp { get; set; }
        public DateTime? RestoringTimestamp { get; set; }
        public DateTime? CompletedTimestamp { get; set; }
        public DateTime? PreparingHealthCheckStartTimestamp { get; set; }
        public DateTime? PreparingHealthCheckEndTimestamp { get; set; }
        public DateTime? RestoringHealthCheckStartTimestamp { get; set; }
        public DateTime? RestoringHealthCheckEndTimestamp { get; set; }
        public RepairTaskHealthCheckState PreparingHealthCheckState { get; set; }
        public RepairTaskHealthCheckState RestoringHealthCheckState { get; set; }
        public bool PerformPreparingHealthCheck { get; set; }
        public bool PerformRestoringHealthCheck { get; set; }

        public override string ToString()
        {
            string text = "{0}, State: {1}".ToString(TaskId, State);
            return text;
        }
    }
}
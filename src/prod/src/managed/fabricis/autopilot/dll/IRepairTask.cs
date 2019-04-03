// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Autopilot
{
    using Repair;

    /// <summary>
    /// A wrapper for <see cref="RepairTask"/>.
    /// </summary>
    internal interface IRepairTask
    {
        RepairScopeIdentifier Scope { get; }

        string TaskId { get; }

        Int64 Version { get; set; }

        string Description { get; set; }

        RepairTaskState State { get; set; }

        RepairTaskFlags Flags { get; }

        string Action { get; }

        RepairTargetDescription Target { get; set; }

        string Executor { get; set; }

        string ExecutorData { get; set; }

        RepairImpactDescription Impact { get; set; }

        RepairTaskResult ResultStatus { get; set; }

        int ResultCode { get; set; }

        string ResultDetails { get; set; }

        DateTime? CreatedTimestamp { get; }

        DateTime? ClaimedTimestamp { get; }

        DateTime? PreparingTimestamp { get; }

        DateTime? ApprovedTimestamp { get; }

        DateTime? ExecutingTimestamp { get; }

        DateTime? RestoringTimestamp { get; }

        DateTime? CompletedTimestamp { get; }

        DateTime? PreparingHealthCheckStartTimestamp { get; }

        DateTime? PreparingHealthCheckEndTimestamp { get; }

        DateTime? RestoringHealthCheckStartTimestamp { get; }

        DateTime? RestoringHealthCheckEndTimestamp { get; }

        RepairTaskHealthCheckState PreparingHealthCheckState { get; }

        RepairTaskHealthCheckState RestoringHealthCheckState { get; }

        bool PerformPreparingHealthCheck { get; set; }

        bool PerformRestoringHealthCheck { get; set; }
    }
}
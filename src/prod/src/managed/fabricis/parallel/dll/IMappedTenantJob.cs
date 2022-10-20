// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using System.Fabric.Repair;
    using Collections.Generic;
    internal sealed class RepairTaskSummary
    {
        public string TaskId { get; set; }
        public RepairTaskState State { get; set; }
    }

    internal interface IMappedTenantJob : IActionableWorkItem
    {
        Guid Id { get; }

        /// <summary>
        /// The tenant job that this object wraps.
        /// </summary>
        ITenantJob TenantJob { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the tenant job is still active as
        /// far is IS is concerned.
        /// </summary>
        bool IsActive { get; set; }

        /// <summary>
        /// Gets or sets the number of nodes that will be impacted by the job.
        /// </summary>
        int ImpactedNodeCount { get; set; }

        IList<RepairTaskSummary> MatchedTasks { get; }
    }
}
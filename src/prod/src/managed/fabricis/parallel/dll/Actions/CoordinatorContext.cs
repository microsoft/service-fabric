// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using System.Collections.Generic;

    /// <summary>
    /// Provides a structure for passing commonly used objects in the coordinator.
    /// </summary>
    internal sealed class CoordinatorContext
    {
        public CoordinatorContext()
        {
            MappedTenantJobs = new Dictionary<Guid, IMappedTenantJob>();
            MappedRepairTasks = new Dictionary<string, IMappedRepairTask>(StringComparer.OrdinalIgnoreCase);
            RunStartTime = DateTimeOffset.UtcNow;
        }

        public void MarkFinished()
        {
            RunFinishTime = DateTimeOffset.UtcNow;
        }

        public IPolicyAgentDocumentForTenant Doc { get; set; }

        public IDictionary<Guid, IMappedTenantJob> MappedTenantJobs { get; set; }

        public IDictionary<string, IMappedRepairTask> MappedRepairTasks { get; set; }

        public DateTimeOffset RunStartTime { get; private set; }

        public DateTimeOffset RunFinishTime { get; private set; }
    }
}
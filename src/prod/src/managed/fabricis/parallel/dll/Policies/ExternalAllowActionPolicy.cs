// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Threading.Tasks;
    using System.Collections.Generic;

    /// <summary>
    /// This policy looks all the actions that the user (operator) has specified for manual approval and approves them.
    /// Please see command 'AllowAction' for context. 
    /// </summary>
    internal sealed class ExternalAllowActionPolicy : ActionPolicy
    {
        private readonly TraceType traceType;
        private readonly IAllowActionMap allowActionMap;

        public ExternalAllowActionPolicy(CoordinatorEnvironment environment, IAllowActionMap allowActionMap)
        {
            this.traceType = environment.Validate("environment").CreateTraceType("PolicyExternalAllow");
            this.allowActionMap = allowActionMap.Validate("allowActionMap");
        }

        public override Task ApplyAsync(Guid activityId, CoordinatorContext coordinatorContext)
        {
            coordinatorContext.Validate("coordinatorContext");

            foreach (var job in coordinatorContext.MappedTenantJobs.Values)
            {
                // Avoid logging or doing any processing for jobs that have no actions pending (e.g. completed jobs)
                if (job.PendingActions == ActionType.None)
                {
                    continue;
                }

                var jobId = job.Id;
                var ud = job.TenantJob.GetJobUD();

                IAllowActionRecord allowAction = allowActionMap.Get(jobId, ud);
                if (allowAction != null)
                {
                    // Allow the user-specified actions
                    job.AllowActions(this.traceType, allowAction.Action);
                }
            }

            return Task.FromResult(0);
        }
    }
}
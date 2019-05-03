// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{        
    using Threading.Tasks;

    internal sealed class JobBlockingActionPolicy : ActionPolicy
    {
        private readonly IJobBlockingPolicyManager jobBlockingPolicyManager;
        private readonly TraceType traceType;

        public JobBlockingActionPolicy(CoordinatorEnvironment environment, IJobBlockingPolicyManager jobBlockingPolicyManager)
        {
            environment.Validate("environment");

            this.jobBlockingPolicyManager = jobBlockingPolicyManager.Validate("jobBlockingPolicyManager");
            this.traceType = environment.CreateTraceType("PolicyJobBlocking");
        }

        private bool IsBlocked(JobBlockingPolicy policy, IMappedTenantJob mappedTenantJob)
        {
            var job = mappedTenantJob.TenantJob;

            switch (policy)
            {
                case JobBlockingPolicy.BlockNone:
                    return false;

                case JobBlockingPolicy.BlockAllJobs:
                case JobBlockingPolicy.BlockAllNewJobs:
                    return true;

                case JobBlockingPolicy.BlockNewMaintenanceJob:
                    return job.IsRepairJobType();

                case JobBlockingPolicy.BlockNewUpdateJob:
                    return job.IsUpdateJobType();

                case JobBlockingPolicy.BlockNewImpactfulTenantUpdateJobs:
                    return job.IsTenantUpdateJobType() && (mappedTenantJob.ImpactedNodeCount > 0);

                case JobBlockingPolicy.BlockNewImpactfulPlatformUpdateJobs:
                    return job.IsPlatformUpdateJobType() && (mappedTenantJob.ImpactedNodeCount > 0);

                case JobBlockingPolicy.BlockNewImpactfulUpdateJobs:
                    return job.IsUpdateJobType() && (mappedTenantJob.ImpactedNodeCount > 0);
            }

            traceType.WriteError("Invalid value for job blocking policy. Not approving any job till this is resolved. Policy: {0}", policy);
            return true;
        }

        public override async Task ApplyAsync(Guid activityId, CoordinatorContext coordinatorContext)
        {
            coordinatorContext.Validate("coordinatorContext");

            if (coordinatorContext.MappedTenantJobs.Count == 0)
            {
                return;
            }

            var policy = await jobBlockingPolicyManager.GetPolicyAsync();
            traceType.WriteInfo("Current job blocking policy is {0}", policy);

            foreach (var mappedTenantJob in coordinatorContext.MappedTenantJobs.Values)
            {
                if (IsBlocked(policy, mappedTenantJob))
                {
                    mappedTenantJob.DenyActions(this.traceType, ActionType.Prepare);
                }
            }
        }

        public override void Reset()
        {
            // clear the cache so that it doesn't hold on to the old cached value
            // especially when the IS replica changes from primary->secondary->primary
            // with another replica updating the job blocking policy while this one was 
            // demoted to a secondary for a brief period.
            jobBlockingPolicyManager.ClearCache();
        }
    }
}
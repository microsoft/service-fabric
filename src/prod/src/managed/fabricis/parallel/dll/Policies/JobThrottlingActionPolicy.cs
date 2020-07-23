// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Collections.Generic;
    using Linq;
    using RD.Fabric.PolicyAgent;
    using Threading.Tasks;

    /// <summary>
    /// The config keys that could be set to control this are
    ///  - MaxParallelJobCount.Total
    ///  - MaxParallelJobCount.[ImpactActionEnum] e.g. MaxParallelJobCount.PlatformMaintenance
    /// MaxParallelJobCount.Unknown type is ignored even if set in config
    /// The final job count is limited by MaxParallelJobCount.Total even if the individual counts for each job type
    /// adds up to more than MaxParallelJobCount.Total.
    /// </summary>
    internal sealed class JobThrottlingActionPolicy : ActionPolicy
    {
        /// <summary>
        /// The default value for the max. number of parallel jobs that are allowed.
        /// </summary>
        internal const int DefaultMaxParallelJobCountTotal = 2;

        /// <summary>
        /// The default value for the max. number of parallel update jobs that are allowed.
        /// </summary>
        internal const int DefaultMaxParallelJobCountUpdate = 1;

        /// <summary>
        /// The default value for the max. number of parallel jobs per <see cref="ImpactActionEnum"/> that are allowed.
        /// </summary>
        /// <remarks>
        /// If there are 4 <see cref="ImpactActionEnum"/> values and each allows 1 job, 
        /// but if MaxParallelJobCount.Total is 2, then only 2 jobs out of the 4 will be selected.
        /// </remarks>
        internal const int DefaultMaxParallelJobCountPerImpactAction = 1;

        private readonly IConfigSection configSection;
        private readonly TraceType traceType;

        public JobThrottlingActionPolicy(CoordinatorEnvironment environment)
        {
            environment.Validate("environment");
            this.configSection = environment.Config;
            this.traceType = environment.CreateTraceType("PolicyJobThrottling");
        }

        private struct JobCount
        {
            public int ActiveCount;
            public int MaxCount;

            public override string ToString()
            {
                return "{0}/{1}".ToString(ActiveCount, MaxCount);
            }
        }

        private class JobTypeCounter
        {
            private readonly IConfigSection configSection;
            private readonly Dictionary<ImpactActionEnum, JobCount> map = new Dictionary<ImpactActionEnum, JobCount>();

            public JobTypeCounter(IConfigSection configSection)
            {
                this.configSection = configSection.Validate("configSection");

                InitializeCount(ImpactActionEnum.Unknown, 0);
                InitializeCount(ImpactActionEnum.PlatformMaintenance);
                InitializeCount(ImpactActionEnum.PlatformUpdate);
                InitializeCount(ImpactActionEnum.TenantMaintenance);
                InitializeCount(ImpactActionEnum.TenantUpdate);
            }

            private void InitializeCount(ImpactActionEnum jobType, int? maxCount = null)
            {
                JobCount count = new JobCount()
                {
                    ActiveCount = 0,
                    MaxCount = maxCount.HasValue ? maxCount.Value : GetMaxCountFromConfig(jobType),
                };

                map.Add(jobType, count);
            }

            private int GetMaxCountFromConfig(ImpactActionEnum jobType)
            {
                return configSection.ReadConfigValue(
                    Constants.ConfigKeys.MaxParallelJobCountKeyPrefix + jobType,
                    DefaultMaxParallelJobCountPerImpactAction);
            }

            public void AddActiveJob(ITenantJob job)
            {
                var jobType = job.GetImpactAction();

                JobCount count;
                if (map.TryGetValue(jobType, out count))
                {
                    count.ActiveCount++;
                    map[jobType] = count;
                }
            }

            public bool CanAddActiveJob(ITenantJob job, out JobCount count)
            {
                var jobType = job.GetImpactAction();

                if (map.TryGetValue(jobType, out count) && (count.ActiveCount < count.MaxCount))
                {
                    return true;
                }

                return false;
            }

            public override string ToString()
            {
                return string.Join(", ", map.Select(e => "{0}: {1}".ToString(e.Key, e.Value)));
            }
        }

        // TODO, ensure stability
        // i.e. on re-execution, the same things should re-execute (if the state hasn't changed)
        public override Task ApplyAsync(Guid activityId, CoordinatorContext coordinatorContext)
        {
            coordinatorContext.Validate("coordinatorContext");

            if (coordinatorContext.MappedTenantJobs.Count == 0)
            {
                return Task.FromResult(0);
            }

            var allJobs = coordinatorContext.MappedTenantJobs.Values;
            var activeJobs = allJobs.Where(j => j.IsActive).ToList();

            // Count all active jobs
            int totalActiveJobCount = activeJobs.Count;

            // Count active jobs by type
            int activeUpdateJobCount = 0;
            JobTypeCounter jobTypeCounter = new JobTypeCounter(this.configSection);
            foreach (var job in activeJobs)
            {
                traceType.WriteInfo("Active job {0} ({1})", job.Id, job.TenantJob.GetImpactAction());

                jobTypeCounter.AddActiveJob(job.TenantJob);

                if (job.TenantJob.IsUpdateJobType())
                {
                    ++activeUpdateJobCount;
                }
            }

            int maxParallelJobCount = configSection.ReadConfigValue(
                Constants.ConfigKeys.MaxParallelJobCountTotal,
                DefaultMaxParallelJobCountTotal);

            int maxParallelUpdateJobCount = configSection.ReadConfigValue(
                Constants.ConfigKeys.MaxParallelJobCountUpdate,
                DefaultMaxParallelJobCountUpdate);

            traceType.WriteInfo(
                "Active/max job counts: Total: {0}/{1}, Updates: {2}/{3}, {4}",
                totalActiveJobCount,
                maxParallelJobCount,
                activeUpdateJobCount,
                maxParallelUpdateJobCount,
                jobTypeCounter);

            // Find all jobs that are waiting to prepare
            var pendingJobs = allJobs
                .Where(j => !j.IsActive && ((j.AllowedActions & ActionType.Prepare) == ActionType.Prepare))
                .OrderBy(j => j.TenantJob.GetImpactAction()) // Apply default static priority based on job type
                .ToList();

            // TODO, ensure that we don't ack too many in the 2nd pass just after acking once
            // choose the simplest logic for now. In future, we will pick based on oldest document incarnation number etc.

            foreach (var pendingJob in pendingJobs)
            {
                // Fall through the checks, so that all blocking reasons are logged
                bool allowJob = true;

                if (totalActiveJobCount >= maxParallelJobCount)
                {
                    traceType.WriteInfo(
                        "Not starting job {0} because it would exceed max total parallel job count ({1}/{2})",
                        pendingJob.Id,
                        totalActiveJobCount,
                        maxParallelJobCount);

                    allowJob = false;
                }

                JobCount count;
                if (!jobTypeCounter.CanAddActiveJob(pendingJob.TenantJob, out count))
                {
                    traceType.WriteInfo(
                        "Not starting job {0} because it would exceed max parallel job count for type {1} ({2})",
                        pendingJob.Id,
                        pendingJob.TenantJob.GetImpactAction(),
                        count);

                    allowJob = false;
                }

                if (pendingJob.TenantJob.IsUpdateJobType() && (activeUpdateJobCount >= maxParallelUpdateJobCount))
                {
                    traceType.WriteInfo(
                        "Not starting job {0} because it would exceed max parallel update job count ({1}/{2})",
                        pendingJob.Id,
                        activeUpdateJobCount,
                        maxParallelUpdateJobCount);

                    allowJob = false;
                }

                if (allowJob)
                {
                    ++totalActiveJobCount;
                    jobTypeCounter.AddActiveJob(pendingJob.TenantJob);

                    if (pendingJob.TenantJob.IsUpdateJobType())
                    {
                        ++activeUpdateJobCount;
                    }

                    traceType.WriteInfo("Allowing job {0} to start", pendingJob.Id);
                }
                else
                {
                    pendingJob.DenyActions(traceType, ActionType.Prepare);
                }
            }

            return Task.FromResult(0);
        }
    }
}
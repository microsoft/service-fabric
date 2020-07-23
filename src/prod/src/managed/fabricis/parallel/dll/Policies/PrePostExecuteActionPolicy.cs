// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Threading.Tasks;

    /// <summary>
    /// </summary>
    internal sealed class PrePostExecuteActionPolicy : ActionPolicy
    {
        private readonly TraceType traceType;
        private readonly IConfigSection config;

        public PrePostExecuteActionPolicy(CoordinatorEnvironment environment)
        {
            this.traceType = environment.Validate("environment").CreateTraceType("PolicyPrePostExecute");
            this.config = environment.Config;
        }

        public override Task ApplyAsync(Guid activityId, CoordinatorContext coordinatorContext)
        {
            coordinatorContext.Validate("coordinatorContext");

            foreach (var job in coordinatorContext.MappedTenantJobs.Values)
            {
                ApplyActionPolicy(job, ActionType.Execute);
                ApplyActionPolicy(job, ActionType.Restore);
            }

            return Task.FromResult(0);
        }

        private void ApplyActionPolicy(IMappedTenantJob job, ActionType actionType)
        {
            if (job.AllowedActions.HasFlag(actionType))
            {
                var jobType = job.TenantJob.GetImpactAction();

                // e.g. Azure.AutoExecute.TenantUpdate=false
                string configKey = Constants.ConfigKeys.AutoActionTypeHandlingFormat.ToString(actionType, jobType);

                if (!this.config.ReadConfigValue(configKey, true))
                {
                    traceType.WriteInfo(
                        "Job {0}: automated processing of {1} action for {2} jobs is disallowed by configuration ({3})",
                        job.Id,
                        actionType,
                        jobType,
                        configKey);

                    job.DenyActions(this.traceType, actionType);
                }
            }
        }
    }
}
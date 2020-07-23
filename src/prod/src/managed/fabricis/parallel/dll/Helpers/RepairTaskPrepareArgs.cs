// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using System.Collections.Generic;
    using System.Fabric.Repair;
    using System.Linq;
    using RD.Fabric.PolicyAgent;

    internal sealed class RepairTaskPrepareArgs
    {
        public string TaskId { get; private set; }
        public string Description { get; private set; }
        public string Action { get; private set; }
        public NodeRepairTargetDescription Target { get; private set; }
        public RepairTaskExecutorData ExecutorData { get; private set; }
        public NodeRepairImpactDescription Impact { get; private set; }
        public bool PerformPreparingHealthCheck { get; private set; }
        public bool PerformRestoringHealthCheck { get; private set; }

        private RepairTaskPrepareArgs()
        {
        }

        public override string ToString()
        {
            return this.ToJson();
        }

        public static RepairTaskPrepareArgs FromTenantJob(
            ITenantJob tenantJob,
            int jobDocIncarnation,
            CoordinatorEnvironment environment,
            bool isVendorRepair,
            bool restoringHealthCheckOnly,
            string description = null)
        {
            tenantJob.Validate("tenantJob");
            environment.Validate("environment");

            var jobId = tenantJob.Id;
            var ud = tenantJob.GetJobUD();

            string jobStepId = tenantJob.GetJobStepId();
            if (jobStepId == null)
            {
                environment.DefaultTraceType.WriteWarning(
                    "RepairTaskPrepareArgs.FromTenantJob: not continuing since job step ID is null in job: {0}",
                    tenantJob.ToJson());

                return null;
            }

            // use the role instance names from the JobStep. Don't use tenantJob.RoleInstancesToBeImpacted
            // since that lists all the role instances that will be impacted.
            // E.g. in a tenant update job, where multiple UDs are walked, if there are 8 role instances, 
            // tenantJob.RoleInstancesToBeImpacted will list all 8, whereas
            // tenantJob.JobStep.CurrentlyImpactedRoleInstances will list only those in the current UD of the jobstep
            var nodeNames = new List<string>();
            if (tenantJob.JobStep.CurrentlyImpactedRoleInstances != null)
            {
                nodeNames.AddRange(tenantJob.JobStep.CurrentlyImpactedRoleInstances.Select(
                    e => e.RoleInstanceName.TranslateRoleInstanceToNodeName()));
            }

            var executorData = new RepairTaskExecutorData
            {
                JobId = jobId.ToString(),
                UD = ud,
                StepId = jobStepId,
            };

            if (isVendorRepair)
            {
                executorData.Flags = RepairTaskExecutorData.VendorRepairFlag;
            }

            string repairTaskId = GenerateRepairTaskId(
                tenantJob.GetImpactAction(),
                jobId,
                ud,
                jobDocIncarnation);

            string repairAction = GenerateRepairAction(tenantJob.GetImpactAction());

            var args = new RepairTaskPrepareArgs()
            {
                TaskId = repairTaskId,
                Description = description,
                Action = repairAction,
                ExecutorData = executorData,
                Target = new NodeRepairTargetDescription(nodeNames),
            };

            if (restoringHealthCheckOnly)
            {
                args.Impact = new NodeRepairImpactDescription();
                args.PerformPreparingHealthCheck = false;
                args.PerformRestoringHealthCheck = true;
            }
            else
            {
                args.Impact = GetImpactFromDetails(tenantJob, environment);
                args.PerformPreparingHealthCheck = tenantJob.DoesJobRequirePreparingHealthCheck(environment.Config);
                args.PerformRestoringHealthCheck = tenantJob.DoesJobRequireRestoringHealthCheck(environment.Config);
            }

            if (tenantJob.IsTenantUpdateJobType() && nodeNames.Count == 0)
            {
                // Never perform health checks on TenantUpdate job steps that have zero role
                // instances listed. These occur at the end of each UD walk when the tenant
                // setting Tenant.PolicyAgent.TenantUpdateUdCleanupApprovalRequired == true.
                args.PerformPreparingHealthCheck = false;
                args.PerformRestoringHealthCheck = false;
            }

            return args;
        }

        private static string GenerateRepairTaskId(ImpactActionEnum impactAction, Guid jobId, uint? jobUD, int jobDocIncarnation)
        {
            return Constants.DefaultRepairTaskIdFormat.ToString(
                impactAction,
                jobId,
                jobUD.HasValue ? jobUD.ToString() : "-",
                jobDocIncarnation);
        }

        private static string GenerateRepairAction(ImpactActionEnum impactAction)
        {
            // e.g. System.Azure.Job.PlatformUpdate
            return Constants.RepairActionFormat.ToString("Job." + impactAction);
        }

        private static NodeRepairImpactDescription GetImpactFromDetails(ITenantJob tenantJob, CoordinatorEnvironment environment)
        {
            environment.Validate("environment");

            var translator = new ImpactTranslator(environment);
            var impact = new NodeRepairImpactDescription();

            if (tenantJob.JobStep == null || tenantJob.JobStep.CurrentlyImpactedRoleInstances == null)
            {
                return impact;
            }

            foreach (var roleInstance in tenantJob.JobStep.CurrentlyImpactedRoleInstances)
            {
                string nodeName = roleInstance.RoleInstanceName.TranslateRoleInstanceToNodeName();

                NodeImpactLevel impactLevel = translator.TranslateImpactDetailToNodeImpactLevel(
                    tenantJob.ImpactDetail.ImpactAction,
                    roleInstance.ExpectedImpact);

                if (impactLevel != NodeImpactLevel.None)
                {
                    var nodeImpact = new NodeImpact(nodeName, impactLevel);
                    impact.ImpactedNodes.Add(nodeImpact);
                }
            }

            return impact;
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel.Test
{
    using Collections.Generic;
    using Linq;
    using RD.Fabric.PolicyAgent;

    internal static class TenantJobHelper
    {
        public static IPolicyAgentDocumentForTenant CreateNewPolicyAgentDocumentForTenant(List<ITenantJob> tenantJobs, int jobDocIncarnation)
        {
            // TODO currently unrelated to role instance data in tenant job
            var rih0 = new RoleInstanceHealthInfo
            {
                RoleInstanceName = "Role_IN_0",
                Health = RoleStateEnum.ReadyRole,
            };
            var rih1 = new RoleInstanceHealthInfo
            {
                RoleInstanceName = "Role_IN_1",
                Health = RoleStateEnum.StoppedVM,
            };

            var roleInstanceHealths = new List<RoleInstanceHealthInfo> { rih0, rih1 };

            var doc = new MockPolicyAgentDocumentForTenant
            {
                Incarnation = "1",
                Jobs = tenantJobs,
                JobDocumentIncarnation = jobDocIncarnation,
                RoleInstanceHealthInfos = roleInstanceHealths,
                RoleInstanceHealthInfoIncarnation = 2,
                RoleInstanceHealthInfoTimestamp = DateTimeOffset.UtcNow.ToString(),
            };

            return doc;
        }

        public static ITenantJob CreateNewTenantJob(
            ImpactActionEnum impactAction = ImpactActionEnum.TenantUpdate,
            uint ud = 0,
            List<string> roleInstances = null,
            string context = null)
        {
            var impactedResources = new AffectedResourceImpact
            {
                ListOfImpactTypes = new List<ImpactTypeEnum> {ImpactTypeEnum.Reboot}
            };

            if (roleInstances == null)
            {
                roleInstances = new List<string> { "Role_IN_0", "Role_IN_1" };    
            }

            var tenantJob = new MockTenantJob(Guid.NewGuid())
            {
                JobStep = CreateNewJobStepInfo(ud, roleInstances),
                ImpactDetail = new MockImpactInfo { ImpactAction = impactAction, ImpactedResources = impactedResources },
                JobStatus = JobStatusEnum.Executing,
            };

            if (context != null)
            {
                // the explicit null check is to signify that the context should not be added for jobs like platformupdate etc.
                tenantJob.ContextStringGivenByTenant = context;
            }

            tenantJob.RoleInstancesToBeImpacted =
                tenantJob.JobStep.CurrentlyImpactedRoleInstances.Select(e => e.RoleInstanceName).ToList();

            return tenantJob;
        }

        public static MockJobStepInfo CreateNewJobStepInfo(uint ud, List<string> roleInstances)
        {
            var impactedRoleInstances = new List<IRoleInstanceImpactedByJob>();
            foreach (var ri in roleInstances)
            {
                var riwo = new MockRoleInstanceImpactedByJob { RoleInstanceName = ri, UpdateDomain = ud };
                impactedRoleInstances.Add(riwo);
            }

            var jsi = new MockJobStepInfo
            {
                ImpactStep = ImpactStepEnum.ImpactStart,
                AcknowledgementStatus = AcknowledgementStatusEnum.WaitingForAcknowledgement,
                ActionStatus = ImpactActionStatus.NotExecuted,
                CurrentlyImpactedRoleInstances = impactedRoleInstances,
            };

            return jsi;
        }
    }
}
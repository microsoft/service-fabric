// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Collections.Generic;
    using RD.Fabric.PolicyAgent;

    /// <remarks>
    /// <see cref="PolicyAgentDocumentForTenantWrapper"/> on why we are copying instead of wrapping.
    /// </remarks>
    internal sealed class JobStepInfoWrapper : IJobStepInfo
    {
        public ImpactStepEnum ImpactStep { get; private set; }

        public AcknowledgementStatusEnum AcknowledgementStatus { get; private set; }

        public string DeadlineForResponse { get; private set; }

        public List<IRoleInstanceImpactedByJob> CurrentlyImpactedRoleInstances { get; private set; }

        public ImpactActionStatus ActionStatus { get; private set; }

        public JobStepInfoWrapper(JobStepInfo jobStepInfo)
        {
            jobStepInfo.Validate("jobStepInfo");
            this.ImpactStep = jobStepInfo.ImpactStep;
            this.AcknowledgementStatus = jobStepInfo.AcknowledgementStatus;
            this.DeadlineForResponse = jobStepInfo.DeadlineForResponse;
            this.CurrentlyImpactedRoleInstances = new List<IRoleInstanceImpactedByJob>();
            this.ActionStatus = jobStepInfo.ActionStatus;

            if (jobStepInfo.CurrentlyImpactedRoleInstances != null)
            {
                foreach (var ri in jobStepInfo.CurrentlyImpactedRoleInstances)
                {
                    this.CurrentlyImpactedRoleInstances.Add(new RoleInstanceImpactedByJobWrapper(ri));
                }
            }
        }

        public override string ToString()
        {
            string text = "{0}, {1}, Deadline: {2}, Impacted RIs: {3}, ActionStatus: {4}".ToString(
                ImpactStep, AcknowledgementStatus, DeadlineForResponse, CurrentlyImpactedRoleInstances.Count, ActionStatus);
            return text;
        }
    }
}
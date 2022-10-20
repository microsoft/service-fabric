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
    internal sealed class TenantJobWrapper : ITenantJob
    {
        public TenantJobWrapper(TenantJob tenantJob)
        {
            tenantJob.Validate("tenantJob");

            this.Id = tenantJob.Id.ToGuid();
            this.JobStatus = tenantJob.JobStatus;
            this.RoleInstancesToBeImpacted = tenantJob.RoleInstancesToBeImpacted ?? new List<string>();
            this.ContextStringGivenByTenant = tenantJob.ContextStringGivenByTenant;

            this.JobStep = tenantJob.JobStep != null ? new JobStepInfoWrapper(tenantJob.JobStep) : null;
            this.ImpactDetail = tenantJob.ImpactDetail != null ? new ImpactInfoWrapper(tenantJob.ImpactDetail) : null;

            this.CorrelationId = tenantJob.CorrelationId;
        }

        public Guid Id { get; private set; }

        public JobStatusEnum JobStatus { get; private set; }

        public IList<string> RoleInstancesToBeImpacted { get; private set; }

        public string ContextStringGivenByTenant { get; private set; }

        public IJobStepInfo JobStep { get; private set; }

        public IImpactInfo ImpactDetail { get; private set; }

        public string CorrelationId { get; private set; }

        public override string ToString()
        {
            string text = "Id: {0}, JobStatus: {1}, Context: {2}, RIs: {3}, JobStep: [{4}], ImpactDetail: [{5}], CorrelationId: {6}".ToString(
                Id, JobStatus, ContextStringGivenByTenant, RoleInstancesToBeImpacted.Count, JobStep, ImpactDetail, CorrelationId);
            return text;
        }
    }
}
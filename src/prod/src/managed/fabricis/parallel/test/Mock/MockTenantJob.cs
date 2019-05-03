// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel.Test
{
    using Collections.Generic;
    using RD.Fabric.PolicyAgent;

    internal sealed class MockTenantJob : ITenantJob
    {
        /// <summary>
        /// Required for automatic Json serialization/deserialization
        /// </summary>
        public MockTenantJob()
        {
        }

        public MockTenantJob(Guid id)
        {
            Id = id;
            RoleInstancesToBeImpacted = new List<string>();

        }

        public Guid Id { get; set; }
        public JobStatusEnum JobStatus { get; set; }
        public IList<string> RoleInstancesToBeImpacted { get; set; }
        public string ContextStringGivenByTenant { get; set; }
        public IJobStepInfo JobStep { get; set; }
        public IImpactInfo ImpactDetail { get; set; }
        public string CorrelationId { get; set; }

        public override string ToString()
        {
            string text = "Id: {0}, JobStatus: {1}, Context: {2}, RIs: {3}, JobStep: [{4}], ImpactDetail: [{5}] CorrelationId: {6}".ToString(
                Id, JobStatus, ContextStringGivenByTenant, RoleInstancesToBeImpacted.Count, JobStep, ImpactDetail, CorrelationId);
            return text;
        }
    }
}
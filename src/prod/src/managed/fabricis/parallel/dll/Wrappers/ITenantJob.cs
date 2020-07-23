// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Collections.Generic;
    using RD.Fabric.PolicyAgent;

    /// <summary>
    /// Wrapper for <see cref="TenantJob"/> mainly for unit-testability.
    /// </summary>
    internal interface ITenantJob
    {
        /// <remarks>
        /// This should not be null.
        /// </remarks>
        Guid Id { get; }

        JobStatusEnum JobStatus { get; }

        IList<string> RoleInstancesToBeImpacted { get; }

        string ContextStringGivenByTenant { get; }

        IJobStepInfo JobStep { get; }

        IImpactInfo ImpactDetail { get; }

        string CorrelationId { get; }
    }
}
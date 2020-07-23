// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Collections.Generic;
    using RD.Fabric.PolicyAgent;

    /// <summary>
    /// Wrapper for <see cref="JobStepInfo"/> mainly for unit-testability.
    /// </summary>
    internal interface IJobStepInfo
    {
        ImpactStepEnum ImpactStep { get; }

        AcknowledgementStatusEnum AcknowledgementStatus { get; }

        string DeadlineForResponse { get; }

        List<IRoleInstanceImpactedByJob> CurrentlyImpactedRoleInstances { get; }

        ImpactActionStatus ActionStatus { get; }
    }
}
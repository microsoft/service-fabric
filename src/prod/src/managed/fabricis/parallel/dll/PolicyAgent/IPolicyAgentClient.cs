// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Collections.Generic;
    using RD.Fabric.PolicyAgent;
    using Threading;
    using Threading.Tasks;

    internal interface IPolicyAgentClient
    {
        Task<IPolicyAgentDocumentForTenant> GetDocumentAsync(Guid activityId, CancellationToken cancellationToken);

        Task<byte[]> GetDocumentRawAsync(Guid activityId, CancellationToken cancellationToken);

        Task RequestRepairAsync(Guid activityId, RepairRequest repairRequest, CancellationToken cancellationToken);

        Task RequestCancelAsync(Guid activityId, JobCancelRequest jobCancelRequest, CancellationToken cancellationToken);

        Task RequestRollbackAsync(Guid activityId, UpdateRollbackRequest updateRollbackRequest, CancellationToken cancellationToken);

        Task RequestSuspendAsync(Guid activityId, JobSuspendRequest jobSuspendRequest, CancellationToken cancellationToken);

        Task RequestResumeAsync(Guid activityId, JobResumeRequest jobResumeRequest, CancellationToken cancellationToken);

        Task SendJobResponseAsync(
            Guid activityId,
            int documentIncarnation,
            IReadOnlyDictionary<Guid, JobStepResponseEnum> jobResponseMap,
            string comment,
            CancellationToken cancellationToken);
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using RD.Fabric.PolicyAgent;
    using Threading;
    using Threading.Tasks;

    /// <summary>
    /// Represents the policy agent, Fabric Controller (FC) related service that the policy agent client communicates with.
    /// The goal is to abstract the communication logic with policyagent client.
    /// </summary>
    internal interface IPolicyAgentServiceWrapper
    {
        Task<IPolicyAgentDocumentForTenant> GetDocumentAsync(Guid activityId, CancellationToken cancellationToken);

        Task<byte[]> GetDocumentRawAsync(Guid activityId, CancellationToken cancellationToken);

        Task PostPolicyAgentRequestAsync(Guid activityId, PolicyAgentRequest request, CancellationToken cancellationToken);
    }
}
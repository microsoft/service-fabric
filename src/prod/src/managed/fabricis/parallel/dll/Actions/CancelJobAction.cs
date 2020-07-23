// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using RD.Fabric.PolicyAgent;
    using Threading;
    using Threading.Tasks;

    internal sealed class CancelJobAction : Action
    {
        private readonly IPolicyAgentClient policyAgentClient;
        private readonly ITenantJob tenantJob;

        public CancelJobAction(CoordinatorEnvironment environment, IPolicyAgentClient policyAgentClient, ITenantJob tenantJob)
            : base(environment, ActionType.None)
        {
            this.policyAgentClient = policyAgentClient.Validate("policyAgentClient");
            this.tenantJob = tenantJob.Validate("tenantJob");
        }

        public override Task ExecuteAsync(Guid activityId)
        {
            var request = new JobCancelRequest
            {
                JobId = this.tenantJob.Id.ToGUID(),
            };

            return this.policyAgentClient.RequestCancelAsync(activityId, request, CancellationToken.None);
        }

        public override string ToString()
        {
            return "{0}, TenantJob: [{1}]".ToString(base.ToString(), tenantJob);
        }
    }
}
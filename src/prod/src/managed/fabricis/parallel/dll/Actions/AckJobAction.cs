// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using RD.Fabric.PolicyAgent;
    using Threading.Tasks;

    // TODO consider deriving this class into AckImpactStartJob and AckImpactEndJob to override the ActionType.
    internal sealed class AckJobAction : Action
    {
        private readonly Action<Guid> approveJobAction;
        private readonly ITenantJob tenantJob;

        public AckJobAction(CoordinatorEnvironment environment, Action<Guid> approveJobAction, ITenantJob tenantJob)
            : base(environment, ActionType.None)
        {
            this.approveJobAction = approveJobAction.Validate("approveJobAction");
            this.tenantJob = tenantJob.Validate("tenantJob");
        }

        public override Task ExecuteAsync(Guid activityId)
        {
            approveJobAction(tenantJob.Id);

            return Task.FromResult(0);
        }

        public override string ToString()
        {
            return "{0}, TenantJob: [{1}]".ToString(base.ToString(), tenantJob);
        }
    }
}
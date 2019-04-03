// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel.Test
{
    using System.Collections.Generic;
    using System.Threading;
    using RD.Fabric.PolicyAgent;
    using System.Threading.Tasks;

    /// <summary>
    /// Workflow when incarnation isn't updated for some time.
    /// The <see cref="ProcessCloser"/> event should be set after some time by the coordinator.
    /// </summary>
    internal class WorkflowHIDetection1Test : BaseWorkflowExecutor
    {
        public WorkflowHIDetection1Test()
            : base(
                new Dictionary<string, string>
                {
                    { InfrastructureService.Parallel.Constants.ConfigKeys.MaxIncarnationUpdateWaitTime, 15.ToString() }
                })
        {
        }

        protected override void Run(CancellationToken cancellationToken)
        {
            var tenantJob = TenantJobHelper.CreateNewTenantJob(ImpactActionEnum.PlatformMaintenance);
            
            // this kicks things off
            PolicyAgentService.CreateTenantJob(tenantJob);

            ProcessCloser.ExitEvent.WaitOne();

            CompletedEvent.Set();
        }
    }

    /// <summary>
    /// Workflow when document is null preventing reading of incarnation number.
    /// The <see cref="ProcessCloser"/> event should be set after some time by the coordinator.
    /// </summary>
    internal class WorkflowHIDetection2Test : BaseWorkflowExecutor
    {
        public WorkflowHIDetection2Test()
            : base(
                new Dictionary<string, string>
                {
                    { InfrastructureService.Parallel.Constants.ConfigKeys.MaxIncarnationUpdateWaitTime, 15.ToString() }
                },
                new FC())
        {
        }

        private class FC : MockPolicyAgentService
        {
            public async override Task<IPolicyAgentDocumentForTenant> GetDocumentAsync(Guid activityId, CancellationToken cancellationToken)
            {
                return await Task.FromResult<IPolicyAgentDocumentForTenant>(null);
            }
        }

        protected override void Run(CancellationToken cancellationToken)
        {
            ProcessCloser.ExitEvent.WaitOne();

            CompletedEvent.Set();
        }
    }

    /// <summary>
    /// Workflow when we need to wait for incarnation number to change (not just be set one time).
    /// In this case, it doesn't change (and we keep receiving the same one all the time). After a
    /// while the <see cref="ProcessCloser"/> event should be set by the coordinator.
    /// </summary>
    internal class WorkflowHIDetection3Test : BaseWorkflowExecutor
    {
        public WorkflowHIDetection3Test()
            : base(
                new Dictionary<string, string>
                {
                    { InfrastructureService.Parallel.Constants.ConfigKeys.MaxIncarnationUpdateWaitTime, 15.ToString() },
                    { InfrastructureService.Parallel.Constants.ConfigKeys.WaitForIncarnationChangeOnStartup, true.ToString() }
                })
        {
        }

        protected override void Run(CancellationToken cancellationToken)
        {
            ProcessCloser.ExitEvent.WaitOne();

            CompletedEvent.Set();
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel.Test
{
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;
    using RD.Fabric.PolicyAgent;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    /// <summary>
    /// Workflow that tests retry errors in the coordinator. In this case, FC
    /// throws a transient exception that should be retried till CoordinatorFailureRetryDuration is exceeded.
    /// After that, the coordinator gives up and sets the event to exit the process.
    /// </summary>
    internal class WorkflowRunAsyncRetryLogicTest : BaseWorkflowExecutor
    {
        private const int MaxRetryDurationInSeconds = 10;

        private readonly DateTimeOffset startTime = DateTimeOffset.UtcNow;
                
        public WorkflowRunAsyncRetryLogicTest()
            : base(
                new Dictionary<string, string>
                {
                    //{ InfrastructureService.Parallel.Constants.ConfigKeys.JobPollingIntervalInSeconds, 5.ToString() },
                    { InfrastructureService.Parallel.Constants.ConfigKeys.CoordinatorFailureRetryDuration, MaxRetryDurationInSeconds.ToString() },
                },
                new FC())
        {
        }

        protected override void Run(CancellationToken cancellationToken)
        {
            var tenantJob = TenantJobHelper.CreateNewTenantJob(ImpactActionEnum.PlatformMaintenance);

            // this kicks things off
            PolicyAgentService.CreateTenantJob(tenantJob);

            ProcessCloser.ExitEvent.WaitOne();

            var elapsed = DateTimeOffset.UtcNow - startTime;

            Assert.IsTrue(elapsed >= TimeSpan.FromSeconds(MaxRetryDurationInSeconds), "Verifying if max retry duration is exceeded");
            CompletedEvent.Set();
        }

        private class FC : MockPolicyAgentService
        {
            public override Task PostPolicyAgentRequestAsync(
                Guid activityId,
                PolicyAgentRequest request,
                CancellationToken cancellationToken)
            {
                throw new TimeoutException();
            }
        }
    }
}
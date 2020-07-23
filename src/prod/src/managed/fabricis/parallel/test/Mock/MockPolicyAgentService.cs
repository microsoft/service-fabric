// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel.Test
{
    using Collections.Generic;
    using Linq;
    using Bond;
    using RD.Fabric.PolicyAgent;
    using Threading;
    using Threading.Tasks;

    internal class MockPolicyAgentService : IPolicyAgentServiceWrapper
    {
        private int jobDocIncarnation;

        private readonly object locker = new object();

        private readonly Dictionary<Guid, ITenantJob> tenantJobs = new Dictionary<Guid, ITenantJob>();

        public MockPolicyAgentService()
        {
            CompletedEvent = new ManualResetEvent(false);
        }

        public ManualResetEvent CompletedEvent { get; private set; }

        public void CreateTenantJob(ITenantJob tenantJob)
        {
            tenantJob.Validate("tenantJob");

            lock (locker)
            {
                tenantJobs.Add(tenantJob.Id, tenantJob);
                jobDocIncarnation++;
            }
        }

        public ITenantJob GetTenantJob(Guid tenantJobId)
        {
            return tenantJobs[tenantJobId];
        }

        public void UpdateTenantJob(ITenantJob tenantJob)
        {
            tenantJob.Validate("tenantJob");

            lock (locker)
            {
                tenantJobs[tenantJob.Id] = tenantJob;
                jobDocIncarnation++;
            }
        }

        public void RemoveTenantJob(Guid tenantJobId)
        {
            lock (locker)
            {
                tenantJobs.Remove(tenantJobId);
                jobDocIncarnation++;
            }
        }

        #region IPolicyAgentServiceWrapper methods

        public virtual Task<IPolicyAgentDocumentForTenant> GetDocumentAsync(Guid activityId, CancellationToken cancellationToken)
        {            
            var doc = TenantJobHelper.CreateNewPolicyAgentDocumentForTenant(tenantJobs.Values.ToList(), jobDocIncarnation);

            return Task.FromResult(doc);
        }

        public virtual Task<byte[]> GetDocumentRawAsync(Guid activityId, CancellationToken cancellationToken)
        {
            return Task.FromResult(new byte[] { 1, 2, 3, 4, 5 });
        }

        public virtual Task PostPolicyAgentRequestAsync(Guid activityId, PolicyAgentRequest request, CancellationToken cancellationToken)
        {
            return Task.FromResult(0);
        }

        #endregion IPolicyAgentServiceWrapper methods

        protected void SendImpactEnd(PolicyAgentRequest request)
        {
            foreach (JobStepResponse jobStepResponse in request.JobResponse.JobStepResponses)
            {
                Guid guid = jobStepResponse.JobId.ToGuid();
                var id = guid;

                MockTenantJob tenantJob = (MockTenantJob)GetTenantJob(id);

                TakeSomeActionOnJob(tenantJob);

                ((MockJobStepInfo)tenantJob.JobStep).ImpactStep = ImpactStepEnum.ImpactEnd;
                ((MockJobStepInfo)tenantJob.JobStep).AcknowledgementStatus = AcknowledgementStatusEnum.WaitingForAcknowledgement;

                // update job after it does its job, so that IS gets it again when it polls for it.
                UpdateTenantJob(tenantJob);
            }
        }

        protected void SuspendAndResumeJob(PolicyAgentRequest request, TimeSpan suspendInterval)
        {
            SuspendJob(request);

            Task.Run(() =>
            {
                Task.Delay(suspendInterval).GetAwaiter().GetResult();
                ResumeJob(request);
            });
        }

        protected void SuspendJob(PolicyAgentRequest request)
        {
            ChangeJobStatus(request, JobStatusEnum.Suspended);
        }

        protected void ResumeJob(PolicyAgentRequest request)
        {
            ChangeJobStatus(request, JobStatusEnum.Executing);
        }

        protected void ChangeJobStatus(PolicyAgentRequest request, JobStatusEnum status)
        {
            foreach (JobStepResponse jobStepResponse in request.JobResponse.JobStepResponses)
            {
                Guid guid = jobStepResponse.JobId.ToGuid();
                var id = guid;

                var tenantJob = GetTenantJob(id);

                ((MockTenantJob)tenantJob).JobStatus = status;

                // update job after it does its job, so that IS gets it again when it polls for it.
                UpdateTenantJob(tenantJob);
            }
        }

        protected void SendCompletion(PolicyAgentRequest request, bool deleteJobStep = false)
        {
            request.Validate("request");

            foreach (JobStepResponse jobStepResponse in request.JobResponse.JobStepResponses)
            {
                Guid guid = jobStepResponse.JobId.ToGuid();
                var id = guid;

                var tenantJob = GetTenantJob(id);

                ((MockTenantJob)tenantJob).JobStatus = JobStatusEnum.Completed;

                if (deleteJobStep && tenantJob.JobStep != null)
                {
                    ((MockTenantJob)tenantJob).JobStep = null;
                }

                // update job after it does its job, so that IS gets it again when it polls for it.
                UpdateTenantJob(tenantJob);
            }
        }

        private static void TakeSomeActionOnJob(ITenantJob tenantJob)
        {
            // pretend that FC actually reboots the VM or takes some action
            // since the job is approved                    
        }        
    }
}
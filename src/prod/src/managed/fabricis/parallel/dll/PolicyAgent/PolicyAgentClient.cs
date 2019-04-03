// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Collections.Generic;
    using Linq;
    using RD.Fabric.PolicyAgent;
    using Threading;
    using Threading.Tasks;

    /// <summary>
    /// Communicates with the policy agent service and IS.
    /// Wraps some functionality to make it easier for IS to send and receive documents.
    /// </summary>
    internal sealed class PolicyAgentClient : IPolicyAgentClient
    {
        private readonly TraceType traceType;
        private readonly IPolicyAgentServiceWrapper policyAgentService;
        private readonly IActivityLogger activityLogger;

        public PolicyAgentClient(CoordinatorEnvironment environment, IPolicyAgentServiceWrapper policyAgentService, IActivityLogger activityLogger)
        {
            this.traceType = environment.Validate("environment").CreateTraceType("PolicyAgentClient");
            this.policyAgentService = policyAgentService.Validate("policyAgentService");
            this.activityLogger = activityLogger.Validate("activityLogger");
        }

        public Task<IPolicyAgentDocumentForTenant> GetDocumentAsync(Guid activityId, CancellationToken cancellationToken)
        {
            return policyAgentService.GetDocumentAsync(activityId, cancellationToken);
        }

        public Task<byte[]> GetDocumentRawAsync(Guid activityId, CancellationToken cancellationToken)
        {
            return policyAgentService.GetDocumentRawAsync(activityId, cancellationToken);
        }

        /// <summary>
        /// Invoked by repair executor
        /// </summary>
        public async Task RequestRepairAsync(Guid activityId, RepairRequest repairRequest, CancellationToken cancellationToken)
        {
            repairRequest.Validate("repairRequest");

            var policyAgentRequest = new PolicyAgentRequest
            {
                RepairRequest = repairRequest
            };

            await PostPolicyAgentRequestAsync(activityId, policyAgentRequest, cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Invoked by repair executor
        /// </summary>
        public async Task RequestCancelAsync(Guid activityId, JobCancelRequest jobCancelRequest, CancellationToken cancellationToken)
        {
            jobCancelRequest.Validate("jobCancelRequest");

            var policyAgentRequest = new PolicyAgentRequest
            {
                JobCancelRequest = jobCancelRequest
            };

            await PostPolicyAgentRequestAsync(activityId, policyAgentRequest, cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Invoked by repair executor
        /// </summary>
        public async Task RequestRollbackAsync(Guid activityId, UpdateRollbackRequest updateRollbackRequest, CancellationToken cancellationToken)
        {
            updateRollbackRequest.Validate("updateRollbackRequest");
            
            var policyAgentRequest = new PolicyAgentRequest
            {
                UpdateRollbackRequest = updateRollbackRequest
            };

            await PostPolicyAgentRequestAsync(activityId, policyAgentRequest, cancellationToken).ConfigureAwait(false);
        }

        public async Task RequestSuspendAsync(Guid activityId, JobSuspendRequest jobSuspendRequest, CancellationToken cancellationToken)
        {
            jobSuspendRequest.Validate("jobSuspendRequest");

            var policyAgentRequest = new PolicyAgentRequest
            {
                JobSuspendRequest = jobSuspendRequest
            };

            await PostPolicyAgentRequestAsync(activityId, policyAgentRequest, cancellationToken).ConfigureAwait(false);
        }

        public async Task RequestResumeAsync(Guid activityId, JobResumeRequest jobResumeRequest, CancellationToken cancellationToken)
        {
            jobResumeRequest.Validate("jobResumeRequest");

            var policyAgentRequest = new PolicyAgentRequest
            {
                JobResumeRequest = jobResumeRequest
            };

            await PostPolicyAgentRequestAsync(activityId, policyAgentRequest, cancellationToken).ConfigureAwait(false);
        }

        public async Task SendJobResponseAsync(
            Guid activityId,
            int documentIncarnation,
            IReadOnlyDictionary<Guid, JobStepResponseEnum> jobResponseMap,
            string comment,
            CancellationToken cancellationToken)
        {
            jobResponseMap.Validate("jobResponseMap");

            if (jobResponseMap.Count == 0)
            {
                traceType.WriteInfo("No job responses to send");
                return;
            }

            if (documentIncarnation < 0)
            {
                throw new ArgumentException(
                    "documentIncarnation is {0}. It cannot be less than 0".ToString(documentIncarnation),
                    "documentIncarnation");
            }

            ParallelJobResponse parallelJobResponse = BuildParallelJobResponse(documentIncarnation, jobResponseMap, comment);

            var policyAgentRequest = new PolicyAgentRequest
            {
                JobResponse = parallelJobResponse,                
            };

            await PostPolicyAgentRequestAsync(activityId, policyAgentRequest, cancellationToken).ConfigureAwait(false);
        }

        private static ParallelJobResponse BuildParallelJobResponse(
            int documentIncarnation,
            IReadOnlyDictionary<Guid, JobStepResponseEnum> jobResponseMap,
            string comment)
        {
            var response = new ParallelJobResponse
            {
                DocumentIncarnation = documentIncarnation
            };

            foreach (var responseEntry in jobResponseMap)
            {
                var jsr = GetJobStepResponse(responseEntry.Key, responseEntry.Value, comment);
                response.JobStepResponses.Add(jsr);
            }

            return response;
        }

        private static JobStepResponse GetJobStepResponse(Guid jobId, JobStepResponseEnum jobStepResponseEnum, string comment)
        {
            var jobStepResponse = new JobStepResponse
            {
                JobId = jobId.ToGUID(),
                Response = jobStepResponseEnum,
                Description = comment,
            };

            return jobStepResponse;
        }

        private async Task PostPolicyAgentRequestAsync(Guid activityId, PolicyAgentRequest request, CancellationToken cancellationToken)
        {
            await policyAgentService.PostPolicyAgentRequestAsync(activityId, request, cancellationToken).ConfigureAwait(false);
        }
    }
}
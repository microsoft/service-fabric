// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using System.Fabric.Health;
    using Collections.Generic;
    using Diagnostics;
    using RD.Fabric.PolicyAgent;
    using Reflection;
    using Repair;
    using Threading;
    using Threading.Tasks;

    /// <summary>
    /// Implements a tenant policy agent for Service Fabric on Azure Compute using the MR ZeroSDK (Parallel Jobs) API.
    /// This coordinator attempts to cancel or complete all repair tasks claimed by this instance of the InfrastructureService,
    /// and auto-approves all MR jobs.
    /// </summary>
    internal sealed class AzureParallelDisabledCoordinator : IInfrastructureCoordinator
    {
        private readonly TraceType traceType;
        private readonly CoordinatorEnvironment environment;
        private readonly string assemblyFileVersion;
        private readonly string tenantId;
        private readonly Guid partitionId;

        private readonly Func<Task<IPolicyAgentClient>> policyAgentClientAsyncFactory;
        private readonly IConfigSection configSection;
        private readonly IRepairManager repairManager;
        private readonly IHealthClient healthClient;
        private readonly IActivityLogger activityLogger;

        private DateTimeOffset coordinatorRunAsyncStartTime;
        private int isCoordinatorRunning;

        public AzureParallelDisabledCoordinator(
            CoordinatorEnvironment environment,
            string tenantId,
            Func<Task<IPolicyAgentClient>> policyAgentClientAsyncFactory,
            IRepairManager repairManager,
            IHealthClient healthClient,
            IActivityLogger activityLogger,
            Guid partitionId,
            long replicaId)
        {
            this.environment = environment.Validate("environment");
            this.tenantId = tenantId.Validate("tenantId");
            this.partitionId = partitionId;
            this.policyAgentClientAsyncFactory = policyAgentClientAsyncFactory.Validate("policyAgentClientAsyncFactory");

            this.traceType = environment.CreateTraceType("DisabledCoordinator");

            this.repairManager = repairManager.Validate("repairManager");
            this.healthClient = healthClient.Validate("healthClient");

            this.configSection = environment.Config;
            this.activityLogger = activityLogger.Validate("activityLogger");

            var assembly = this.GetType().GetTypeInfo().Assembly;
            assemblyFileVersion = FileVersionInfo.GetVersionInfo(assembly.Location).FileVersion;
        }

        public Task<string> RunCommandAsync(bool isAdminCommand, string command, TimeSpan timeout, CancellationToken cancellationToken)
        {
            traceType.WriteInfo("Ignoring RunCommand(isAdminCommand = {0}, command = '{1}')", isAdminCommand, command);
            return Task.FromResult(string.Empty);
        }

        public Task ReportStartTaskSuccessAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            traceType.WriteWarning("Processing unexpected CM callback ReportStartTaskSuccessAsync('{0}',0x{1:X})", taskId, instanceId);
            return Task.FromResult(0);
        }

        public Task ReportFinishTaskSuccessAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            traceType.WriteWarning("Processing unexpected CM callback ReportFinishTaskSuccessAsync('{0}',0x{1:X})", taskId, instanceId);
            return Task.FromResult(0);
        }

        public Task ReportTaskFailureAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            traceType.WriteWarning("Processing unexpected CM callback ReportTaskFailureAsync('{0}',0x{1:X})", taskId, instanceId);
            return Task.FromResult(0);
        }

        public async Task RunAsync(int primaryEpoch, CancellationToken token)
        {
            if (primaryEpoch < 0)
            {
                throw new ArgumentOutOfRangeException("primaryEpoch");
            }

            if (!this.TryStartRunAsync())
            {
                throw new InvalidOperationException("Coordinator has already been started");
            }

            try
            {
                coordinatorRunAsyncStartTime = DateTimeOffset.UtcNow;

                traceType.WriteInfo("RunAsync: primaryEpoch = 0x{0:X}", primaryEpoch);

                await RunProcessingLoopAsync(token).ConfigureAwait(false);
            }
            finally
            {
                this.CompleteRunAsync();
            }
        }

        private bool TryStartRunAsync()
        {
            return Interlocked.CompareExchange(ref this.isCoordinatorRunning, 1, 0) == 0;
        }

        private async Task RunProcessingLoopAsync(CancellationToken cancellationToken)
        {
            traceType.WriteInfo("Starting processing");

            try
            {
                while (!cancellationToken.IsCancellationRequested)
                {
                    traceType.WriteInfo(
                        "AzureParallelDisabled coordinator: service name = {0}, partition ID = {1}, tenant ID = {2}, code version = {3}, start time = {4:O}",
                        this.environment.ServiceName,
                        this.partitionId,
                        this.tenantId,
                        this.assemblyFileVersion,
                        this.coordinatorRunAsyncStartTime);

                    TimeSpan timerPeriod = this.GetPollingInterval(
                        "AzureParallelDisabled.JobPollingIntervalInSeconds",
                        15);

                    await RunProcessingOnceAsync(timerPeriod, cancellationToken);

                    TimeSpan maxRunTime = this.GetPollingInterval(
                        "AzureParallelDisabled.MaximumRunDurationInSeconds",
                        120);

                    TimeSpan elapsedRunTime = DateTimeOffset.UtcNow - this.coordinatorRunAsyncStartTime;

                    if (maxRunTime < elapsedRunTime)
                    {
                        traceType.WriteInfo("Maximum run duration reached ({0} < {1}); exiting coordinator", maxRunTime, elapsedRunTime);
                        ProcessCloser.ExitEvent.Set();
                    }

                    traceType.WriteInfo("Next run will occur in {0} at {1:O}", timerPeriod, DateTimeOffset.UtcNow + timerPeriod);
                    await Task.Delay(timerPeriod, cancellationToken);
                }

                traceType.WriteInfo("Cancellation requested");
            }
            catch (OperationCanceledException)
            {
                traceType.WriteInfo("Processing cancelled");
            }
            catch (Exception ex)
            {
                traceType.WriteWarning("Error processing policy agent document in StartJobTask. Exiting process. Exception: {0}", ex);

                // the Program's Main method is waiting on this global event. Once set, it exits the process.
                // Workaround over using Environment.Exit which isn't supported on CoreCLR. Please see this
                // method's comments for more.
                ProcessCloser.ExitEvent.Set();
            }
        }

        private async Task RunProcessingOnceAsync(TimeSpan runInterval, CancellationToken cancellationToken)
        {
            var activityId = Guid.NewGuid();

            bool repairProcessingSucceeded = false;
            try
            {
                await CompleteAllRepairTasksAsync(activityId, cancellationToken);
                repairProcessingSucceeded = true;
            }
            catch (Exception e)
            {
                traceType.WriteWarning("Repair task processing failed; continuing: {0}", e);
            }

            bool jobProcessingSucceeded = false;
            try
            {
                await ApproveAllJobsAsync(activityId, cancellationToken);
                jobProcessingSucceeded = true;
            }
            catch (Exception e)
            {
                traceType.WriteInfo("Azure job auto-approval was unsuccessful, this is expected when Azure coordination is disabled: {0}", e.Message);
            }

            string healthMessage = "Azure coordination is disabled.";
            HealthState healthState = HealthState.Ok;

            if (jobProcessingSucceeded)
            {
                // Success is *abnormal*, because if the tenant has MR enabled then this is the wrong coordinator to use.
                // This condition will occur as MR is being enabled or disabled, but should not persist for a long time.
                // Warn to make the condition visible.
                healthMessage = "Azure coordination is in auto-approve mode. If coordination is currently being enabled or disabled, this is normal. Otherwise, check VM and cluster configuration.";
                healthState = HealthState.Warning;
            }

            if (!repairProcessingSucceeded)
            {
                healthMessage += " Repair task processing failed.";
                healthState = HealthState.Warning;
            }

            UpdateCoordinatorHealthStatus(healthState, healthMessage, runInterval);
        }

        private async Task ApproveAllJobsAsync(Guid activityId, CancellationToken cancellationToken)
        {
            // Create the PA client; this will fail when MR is off, which it is expected to be
            IPolicyAgentClient policyAgentClient = await this.policyAgentClientAsyncFactory();

            IPolicyAgentDocumentForTenant doc = await policyAgentClient.GetDocumentAsync(activityId, cancellationToken);
            if (doc == null)
            {
                // Parallel job mode is not enabled
                throw new InvalidOperationException("Failed to get job info. Verify that job parallelism is enabled for this tenant.");
            }

            string docString = CoordinatorHelper.FormatDocumentForTracing(doc);
            traceType.WriteInfo("Received document: {0}", docString);

            Dictionary<Guid, JobStepResponseEnum> jobResponses = new Dictionary<Guid, JobStepResponseEnum>();
            foreach (var job in doc.Jobs)
            {
                string message;

                if (IsJobWaitingForApproval(job))
                {
                    message = $"Auto-approving job {job.Id}";
                    jobResponses.Add(job.Id, JobStepResponseEnum.Acknowledged);
                }
                else
                {
                    message = $"Ignoring job {job.Id} in state that does not require tenant approval";
                }

                traceType.WriteInfo(
                    "{0}: {1} {2} ({3}/{4})",
                    message,
                    job.GetImpactAction(),
                    job.JobStatus,
                    job.JobStep == null ? "-" : job.JobStep.ImpactStep.ToString(),
                    job.JobStep == null ? "-" : job.JobStep.AcknowledgementStatus.ToString());
            }

            if (jobResponses.Count > 0)
            {
                // Send all job responses (if any) as a single batch response
                await policyAgentClient.SendJobResponseAsync(
                    activityId,
                    doc.JobDocumentIncarnation,
                    jobResponses,
                    "Auto-approved by Service Fabric because coordination is disabled",
                    cancellationToken);
            }
            else
            {
                traceType.WriteInfo("No jobs to approve");
            }
        }

        private static bool IsJobWaitingForApproval(ITenantJob job)
        {
            return
                (job.JobStatus == JobStatusEnum.Executing) &&
                (job.JobStep != null) &&
                (job.JobStep.AcknowledgementStatus == AcknowledgementStatusEnum.WaitingForAcknowledgement);
        }

        /// <summary>
        /// Reports health at the IS service level.
        /// </summary>
        private void UpdateCoordinatorHealthStatus(HealthState serviceHealthState, string description, TimeSpan runInterval)
        {
            // Until v5.6, RDBug#8034630 results in descriptions over 4KB causing an exception.
            // For now, trim the string well below that limit, to avoid the exception.
            const int DescriptionMaxLength = 3 * 1024;
            if (description.Length > DescriptionMaxLength)
            {
                description = description.Substring(0, DescriptionMaxLength) + "[Truncated]";
            }

            var hi = new HealthInformation(
                InfrastructureService.Constants.HealthReportSourceId,
                InfrastructureService.Constants.CoordinatorStatus,
                serviceHealthState)
            {
                Description = description,
                RemoveWhenExpired = true,
                TimeToLive = TimeSpan.FromSeconds(runInterval.TotalSeconds * 10),
            };

            var serviceUri = new Uri(this.environment.ServiceName);
            var healthReport = new ServiceHealthReport(serviceUri, hi);
            this.healthClient.ReportHealth(healthReport);
        }

        private async Task CompleteAllRepairTasksAsync(Guid activityId, CancellationToken cancellationToken)
        {
            // Get the repair tasks that are claimed by this service
            var repairTasks = await repairManager.GetRepairTaskListAsync(
                activityId,
                null,
                RepairTaskStateFilter.Active,
                this.environment.ServiceName);

            traceType.WriteInfo(
                "Cancelling active repair tasks owned by {0} (count: {1})",
                this.environment.ServiceName,
                repairTasks.Count);

            int successfulCount = 0;

            foreach (IRepairTask task in repairTasks)
            {
                cancellationToken.ThrowIfCancellationRequested();

                if (await CompleteRepairTaskAsync(activityId, task))
                {
                    ++successfulCount;
                }
            }

            traceType.WriteInfo(
                "Cancelled {0} of {1} active repair tasks owned by {2}",
                successfulCount,
                repairTasks.Count,
                this.environment.ServiceName);
        }

        private async Task<bool> CompleteRepairTaskAsync(Guid activityId, IRepairTask task)
        {
            traceType.WriteInfo(
                "Cancelling repair task {0} in state {1}:{2}{3}",
                task.TaskId,
                task.State,
                Environment.NewLine,
                task.ToJson());

            bool succeeded = true;

            try
            {
                switch (task.State)
                {
                    case RepairTaskState.Claimed:
                    case RepairTaskState.Preparing:
                        await repairManager.CancelRepairTaskAsync(activityId, task);
                        break;

                    case RepairTaskState.Approved:
                    case RepairTaskState.Executing:
                        task.State = RepairTaskState.Restoring;
                        task.ResultStatus = RepairTaskResult.Interrupted;
                        task.ResultDetails = "Repair task was cancelled because Azure coordination is disabled";
                        await repairManager.UpdateRepairExecutionStateAsync(activityId, task);
                        break;

                    case RepairTaskState.Restoring:
                        await repairManager.UpdateRepairTaskHealthPolicyAsync(activityId, task, null, performRestoringHealthCheck: false);
                        break;
                }
            }
            catch (Exception e)
            {
                traceType.WriteWarning(
                    "Failed to cancel repair task {0} in state {1}: {2}",
                    task.TaskId,
                    task.State,
                    e);

                succeeded = false;
            }

            return succeeded;
        }

        /// <summary>
        /// Gets the polling interval.
        /// </summary>
        /// <param name="keyName">Name of the key.</param>
        /// <param name="defaultValue">The default value in seconds.</param>
        /// <returns>The polling interval.</returns>
        private TimeSpan GetPollingInterval(string keyName, int defaultValue)
        {
            return TimeSpan.FromSeconds(configSection.ReadConfigValue(keyName, defaultValue));
        }

        /// <summary>
        /// Cleans up state during coordinator shutdown and releases the lock.
        /// </summary>
        private void CompleteRunAsync()
        {
            traceType.WriteInfo("CompleteRunAsync: coordinator is stopped");
            Interlocked.Exchange(ref this.isCoordinatorRunning, 0);
        }
    }
}
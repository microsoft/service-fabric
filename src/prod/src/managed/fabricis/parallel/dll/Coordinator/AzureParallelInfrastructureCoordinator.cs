// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Collections.Generic;
    using Diagnostics;
    using Linq;
    using Reflection;
    using Threading;
    using Threading.Tasks;
    using Repair;
    using System.Fabric.Health;

    /// <summary>
    /// Implements a tenant policy agent for Service Fabric on Azure Compute using the MR ZeroSDK (Parallel Jobs) API
    /// </summary>
    internal sealed class AzureParallelInfrastructureCoordinator : IInfrastructureCoordinator
    {        
        private const int DefaultJobPollingIntervalInSeconds = 15;
        
        /// <summary>
        /// A factor used for calculating time to live (or expiry time of a health report).
        /// expiry time = factor * polling period
        /// E.g. 12 * 15 seconds = 3 minutes
        /// </summary>
        private const int DefaultHealthReportExpiryFactor = 12;

        private readonly TraceType traceType;
        private readonly TraceType actionTraceType;
        private readonly CoordinatorEnvironment environment;

        /// <summary>
        /// For display in traces (since we have typically asked customers for version of Service Fabric
        /// for debugging issues).
        /// </summary>
        private readonly string assemblyFileVersion;

        /// <summary>
        /// useful mainly for debugging purpose. E.g. FC folks frequently ask for tenant Id for investigations. 
        /// Easier to look up in powershell than in TraceViewer or remote into VM and look up reg key.
        /// </summary>
        private readonly string tenantId;
        private readonly Guid partitionId;

        private static readonly TimeSpan MaxJobPollingInterval = TimeSpan.FromDays(10);        

        private readonly IPolicyAgentClient policyAgentClient;
        private readonly IConfigSection configSection;
        private readonly IRepairManager repairManager;
        private readonly IHealthClient healthClient;
        
        private readonly IRoleInstanceHealthWatchdog roleInstanceHealthWatchdog;

        private readonly ICoordinatorCommandProcessor coordinatorCommandProcessor;
        private readonly IJobBlockingPolicyManager jobBlockingPolicyManager;

        private readonly IList<IActionPolicy> actionPolicies;

        private readonly IActivityLogger activityLogger;

        private RepairActionProvider repairActionProvider;
        private DateTimeOffset coordinatorRunAsyncStartTime;
        private int isCoordinatorRunning;
        private int primaryEpoch;
        private uint nextSequenceNumber;

        /// <summary>
        /// Indicates if notification from the MR channel is available.
        /// </summary>
        private bool isNotificationAvailable;

        /// <summary>
        /// The last health incarnation. If this doesn't change periodically (say 1 minute interval), then
        /// the host node (containing this VM which contains this replica) may have lost connection with FC
        /// and needs to failover (hopefully to a healthier VM).
        /// </summary>
        private ulong lastHealthIncarnation;

        /// <summary>
        /// The time when <see cref="lastHealthIncarnation"/> was updated. This time is used to determine when to failover.
        /// </summary>
        private DateTimeOffset? lastHealthIncarnationUpdatedAt;

        public AzureParallelInfrastructureCoordinator(
            CoordinatorEnvironment environment,
            string tenantId,            
            IPolicyAgentClient policyAgentClient,            
            IRepairManager repairManager,
            IHealthClient healthClient,
            ICoordinatorCommandProcessor coordinatorCommandProcessor,
            IJobBlockingPolicyManager jobBlockingPolicyManager,
            IActionPolicyFactory actionPolicyFactory,
            IActivityLogger activityLogger,
            Guid partitionId,
            long replicaId)
        {
            this.environment = environment.Validate("environment");
            this.tenantId = tenantId.Validate("tenantId");
            this.partitionId = partitionId;
            this.policyAgentClient = policyAgentClient.Validate("policyAgentClient");

            this.traceType = environment.CreateTraceType("Coordinator");
            this.actionTraceType = environment.CreateTraceType("Action");

            this.repairManager = repairManager.Validate("repairManager");
            this.healthClient = healthClient.Validate("healthClient");

            this.configSection = environment.Config;
            this.coordinatorCommandProcessor = coordinatorCommandProcessor.Validate("coordinatorCommandProcessor");
            this.jobBlockingPolicyManager = jobBlockingPolicyManager.Validate("jobBlockingPolicyManager");
            this.actionPolicies = actionPolicyFactory.Validate("actionPolicyFactory").Create();
            this.activityLogger = activityLogger.Validate("activityLogger");

            var assembly = this.GetType().GetTypeInfo().Assembly;
            assemblyFileVersion = FileVersionInfo.GetVersionInfo(assembly.Location).FileVersion;

            var healthWatchdogFactory = new RoleInstanceHealthWatchdogFactory(configSection, healthClient);
            roleInstanceHealthWatchdog = healthWatchdogFactory.Create(Constants.ConfigKeys.ConfigKeyPrefix);
        }

        public Task<string> RunCommandAsync(bool isAdminCommand, string command, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return coordinatorCommandProcessor.RunCommandAsync(isAdminCommand, command, timeout, cancellationToken);
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
                if (primaryEpoch < this.primaryEpoch)
                {
                    throw new ArgumentException("Primary epoch must be non-decreasing", "primaryEpoch");
                }

                if (this.primaryEpoch < primaryEpoch)
                {
                    this.nextSequenceNumber = 0;
                }

                coordinatorCommandProcessor.Reset();
                coordinatorRunAsyncStartTime = DateTimeOffset.UtcNow;

                coordinatorCommandProcessor.CoordinatorDiagnosticInfo = new CoordinatorDiagnosticInfo
                {
                    CoordinatorStartTime = "{0:o}".ToString(coordinatorRunAsyncStartTime),
                    AzureTenantId = this.tenantId,
                    AssemblyFileVersion = this.assemblyFileVersion,
                };

                this.primaryEpoch = primaryEpoch;

                traceType.WriteInfo("RunAsync: primaryEpoch = 0x{0:X}, nextSequenceNumber = 0x{1:X}",
                    this.primaryEpoch,
                    this.nextSequenceNumber);

                traceType.WriteAggregatedEvent(
                    InfrastructureService.Constants.CoordinatorModeKey, 
                    this.tenantId, 
                    "{0}", 
                    new CoordinatorStateData().Mode);

                this.isNotificationAvailable = false;
                this.lastHealthIncarnation = 0;
                this.lastHealthIncarnationUpdatedAt = null;

                this.repairActionProvider = new RepairActionProvider(configSection);

                ActionHelper.ResetActionPolicies(actionPolicies);

                // Clear old AzureSerial health property
                ClearLegacyNotificationApprovalHealthStatus();

                await StartJobTaskAsync(token).ConfigureAwait(false);
            }
            finally
            {
                this.CompleteRunAsync();
            }
        }

        private void ClearLegacyNotificationApprovalHealthStatus()
        {
            var hi = new HealthInformation(
                InfrastructureService.Constants.HealthReportSourceId,
                RoleInstanceHealthConstants.HealthReportManagementNotificationApprovalStatusPropertyName,
                HealthState.Ok)
            {
                Description = "(Deprecated)",
                TimeToLive = TimeSpan.FromSeconds(1),
                RemoveWhenExpired = true,
            };

            var serviceUri = new Uri(this.environment.ServiceName);
            var healthReport = new ServiceHealthReport(serviceUri, hi);

            this.healthClient.ReportHealth(healthReport);
        }

        private bool TryStartRunAsync()
        {
            return Interlocked.CompareExchange(ref this.isCoordinatorRunning, 1, 0) == 0;
        }

        private async Task StartJobTaskAsync(CancellationToken cancellationToken)
        {
            traceType.WriteInfo("Starting job task.");
            var activityId = Guid.NewGuid();

            try
            {
                await ProcessPolicyAgentDocumentAsync(cancellationToken).ConfigureAwait(false);
                activityLogger.LogOperation(activityId, this.coordinatorRunAsyncStartTime);
            }
            catch (OperationCanceledException)
            {
                traceType.WriteInfo("Start job task canceled.");
                activityLogger.LogOperation(activityId, this.coordinatorRunAsyncStartTime);
            }
            catch (Exception ex)
            {
                traceType.WriteWarning("Error processing policy agent document in StartJobTask. Exiting process. Exception: {0}", ex);
                activityLogger.LogOperation(activityId, this.coordinatorRunAsyncStartTime, OperationResult.Failure, ex);

                // the Program's Main method is waiting on this global event. Once set, it exits the process.
                // Workaround over using Environment.Exit which isn't supported on CoreCLR. Please see this
                // method's comments for more.
                ProcessCloser.ExitEvent.Set();                
            }            
        }

        private async Task ProcessPolicyAgentDocumentAsync(CancellationToken cancellationToken)
        {
            traceType.WriteInfo("Starting to process policy agent documents.");

            var lastSuccessfulRunAt = DateTimeOffset.UtcNow;

            while (!cancellationToken.IsCancellationRequested)
            {
                traceType.WriteInfo(
                    "Azure coordinator: service name = {0}, partition ID = {1}, tenant ID = {2}, code version = {3}, start time = {4:O}",
                    this.environment.ServiceName,
                    this.partitionId,
                    this.tenantId,
                    this.assemblyFileVersion,
                    this.coordinatorRunAsyncStartTime);

                TimeSpan timerPeriod = this.GetPollingInterval(Constants.ConfigKeys.JobPollingIntervalInSeconds,
                    DefaultJobPollingIntervalInSeconds,
                    MaxJobPollingInterval);

                try
                {
                    var activityId = Guid.NewGuid();

                    IPolicyAgentDocumentForTenant doc = await policyAgentClient.GetDocumentAsync(activityId, cancellationToken).ConfigureAwait(false);

                    if (doc == null)
                    {
                        // Parallel job mode is not enabled
                        throw new InvalidOperationException("Failed to get job info. Verify that job parallelism is enabled for this tenant.");
                    }
                    else 
                    {
                        // If the incarnation stops changing for a long time, the MR channel may be unhealthy,
                        // due to the physical node being in HI/OFR state. Exit the process to cause a failover,
                        // hopefully to a healthier node.
                        this.CheckChannelHealth(activityId, doc);

                        if (!this.isNotificationAvailable)
                        {
                            traceType.WriteInfo("MR channel notification is not available; waiting for channel health check to pass.");
                        }
                        else
                        {
                            string docString = CoordinatorHelper.FormatDocumentForTracing(doc);
                            traceType.WriteInfo("Received document: {0}", docString);

                            Task healthProcessingTask = ProcessHealthAsync(activityId, doc, timerPeriod, cancellationToken);
                            Task jobProcessingTask = ProcessJobInfoAsync(activityId, doc, cancellationToken);

                            await healthProcessingTask.ConfigureAwait(false);
                            await jobProcessingTask.ConfigureAwait(false);
                        }
                    }

                    lastSuccessfulRunAt = DateTimeOffset.UtcNow;
                    UpdateCoordinatorHealthStatus(HealthState.Ok, "Azure coordinator is operating normally.");
                }
                catch (Exception ex)
                {
                    if (!HandleCoordinatorException(ex, lastSuccessfulRunAt))
                    {
                        throw;
                    }
                }

                traceType.WriteInfo("Next run will occur in {0} at {1:O}", timerPeriod, DateTimeOffset.UtcNow + timerPeriod);
                await Task.Delay(timerPeriod, cancellationToken);
            }

            traceType.WriteInfo("Cancellation requested. Returning from processing policy agent documents.");
        }

        /// <remarks>
        /// Returning bool instead of void since we want to not 'throw' from this method to preserve exception stack info.
        /// </remarks>
        private bool HandleCoordinatorException(Exception ex, DateTimeOffset lastSuccessfulRunAt)
        {
            if (ex is ManagementChannelTerminallyUnhealthyException)
            {
                return false;
            }

            var warningThreshold =
                TimeSpan.FromSeconds(configSection.ReadConfigValue(Constants.ConfigKeys.CoordinatorFailureWarningThreshold, 120));
            var maxRetryDuration =
                TimeSpan.FromSeconds(configSection.ReadConfigValue(Constants.ConfigKeys.CoordinatorFailureRetryDuration, 900));

            var now = DateTimeOffset.UtcNow;
            var elapsed = now - lastSuccessfulRunAt;

            if (elapsed > maxRetryDuration)
            {
                var message =
                    "Not retrying further since 'CoordinatorFailureRetryDuration' of {0} has been exceeded".ToString(maxRetryDuration);
                traceType.WriteError("Error while processing policy agent document. {0}. Exception: {1}", message, ex);
                return false;
            }

            string warningText = null;
            if (elapsed > warningThreshold)
            {
                UpdateCoordinatorHealthStatus(
                    HealthState.Warning,
                    "Azure coordinator encountered errors. Last successful run was at: {0:O}. Last error: {1}"
                        .ToString(lastSuccessfulRunAt, ex));

                warningText =
                    "Health warning reported since 'CoordinatorFailureWarningThreshold' of {0} has exceeded. ".ToString(
                        warningThreshold);
            }

            traceType.WriteWarning(
                "Error while processing policy agent document. Last successful run was at: {0:O}. {1}Retrying until 'CoordinatorFailureRetryDuration' of {2} is exceeded. Exception: {3}",
                lastSuccessfulRunAt, warningText, maxRetryDuration, ex);

            return true;
        }

        /// <summary>
        /// Reports health at the IS service level.
        /// </summary>
        private void UpdateCoordinatorHealthStatus(HealthState serviceHealthState, string description)
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
            };

            var serviceUri = new Uri(this.environment.ServiceName);
            var healthReport = new ServiceHealthReport(serviceUri, hi);
            this.healthClient.ReportHealth(healthReport);
        }

        private static bool IsOldCompletedRepairTask(IRepairTask task, DateTime now, TimeSpan ageThreshold)
        {
            return
                (task.State == RepairTaskState.Completed) &&
                task.CompletedTimestamp.HasValue &&
                ((now - task.CompletedTimestamp) > ageThreshold);
        }

        private async Task ProcessJobInfoAsync(Guid activityId, IPolicyAgentDocumentForTenant doc, CancellationToken cancellationToken)
        {
            DateTime now = DateTime.UtcNow;
            TimeSpan completedTaskAgeThreshold = this.GetCompletedTaskAgeThreshold();

            // get the ones that are claimed by this service (which is the Repair Executor (RE))
            var repairTasks = await repairManager
                .GetRepairTaskListAsync(activityId, null, RepairTaskStateFilter.Default, this.environment.ServiceName)
                .ConfigureAwait(false);

            // Count completed tasks that are going to be ignored because they are too old
            int oldCompletedTaskCount = repairTasks.Count(t => IsOldCompletedRepairTask(t, now, completedTaskAgeThreshold));

            // Keep all of the rest (active or recent)
            repairTasks = repairTasks.Where(t => !IsOldCompletedRepairTask(t, now, completedTaskAgeThreshold)).ToList();

            // get all the unclaimed repair tasks too... some may not match, we'll evaluate and ignore them later
            var unclaimedRepairTasks = await repairManager
                .GetRepairTaskListAsync(activityId, null, RepairTaskStateFilter.Created, null)
                .ConfigureAwait(false);

            foreach (var unclaimedRepairTask in unclaimedRepairTasks)
            {
                repairTasks.Add(unclaimedRepairTask);
            }

            traceType.WriteInfo(
                "Processing repair tasks (count: {0}; ignored completed tasks older than {1}: {2})",
                repairTasks.Count,
                completedTaskAgeThreshold,
                oldCompletedTaskCount);

            // Dump each task separately to avoid exceeding the ETW event size limit
            for (int i = 0; i < repairTasks.Count; ++i)
            {
                traceType.WriteInfo("Repair task {0} of {1}:{2}{3}", i + 1, repairTasks.Count, Environment.NewLine, repairTasks[i].ToJson());
            }

            var coordinatorContext = new CoordinatorContext {Doc = doc};
            ActionHelper.CreateMappedWorkItems(doc.Jobs, repairTasks, coordinatorContext);

            var reconciler = new Reconciler(
                this.environment,
                this.policyAgentClient,
                this.repairManager,
                this.repairActionProvider);

            await reconciler.ReconcileAsync(activityId, coordinatorContext).ConfigureAwait(false);

            await ActionHelper.ApplyPoliciesAsync(activityId, actionPolicies, coordinatorContext).ConfigureAwait(false);

            // Publish state after reconcile + policy phases have completed
            coordinatorContext.MarkFinished();
            coordinatorCommandProcessor.CoordinatorContext = coordinatorContext;

            var blockingPolicy = await this.jobBlockingPolicyManager.GetPolicyAsync();
            if (blockingPolicy == JobBlockingPolicy.BlockAllJobs)
            {
                traceType.WriteInfo("All actions are blocked by job blocking policy");
            }
            else
            {
                var actionsToExecute = ActionHelper.GetActions(activityId, coordinatorContext);
                await ActionHelper.ExecuteActionsAsync(activityId, this.actionTraceType, actionsToExecute).ConfigureAwait(false);

                // Send all job responses (if any) as a single batch response
                await policyAgentClient.SendJobResponseAsync(
                    activityId,
                    doc.JobDocumentIncarnation,
                    reconciler.GetJobStepResponses(),
                    "Automated response by Service Fabric Infrastructure Service",
                    cancellationToken).ConfigureAwait(false);
            }
        }

        private async Task ProcessHealthAsync(
            Guid activityId,
            IPolicyAgentDocumentForTenant doc, 
            TimeSpan pollingInterval, 
            CancellationToken cancellationToken)
        {
            // Read the old AzureSerial setting.  If it is explicitly disabled there, then skip the
            // health reporting here too.  This allows health reporting for SFRP clusters (where it
            // was already disabled in serial jobs mode) to be disabled in parallel mode too, without
            // requiring a new config setting that would also have to be added to SFRP manifest generation.
            if (this.configSection.ReadConfigValue("WindowsAzure.HealthPollingIntervalInSeconds", 1) == 0)
            {
                // Health reporting was explicitly disabled in AzureSerial, so treat it as disabled here too.
                return;
            }

            // dump health info here using HealthClient APIs

            try
            {
                IList<RoleInstance> roleInstances = doc.GetRoleInstances();
                TimeSpan validityPeriod = GetHealthValidityPeriod(pollingInterval);

                roleInstanceHealthWatchdog.ReportHealth(roleInstances, validityPeriod);
            }
            catch (Exception ex)
            {
                // no choice but to catch everything since we don't know what to expect from each policy implementation
                traceType.WriteWarning("Error while processing role instance health. Exception: {0}", ex);

                throw;
            }

            await Task.FromResult(0);
        }

        /// <summary>
        /// Gets the polling interval. Caps the polling interval to a suitable max value.
        /// </summary>
        /// <param name="keyName">Name of the key.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <param name="maxValue">The maximum value.</param>
        /// <returns>The polling interval.</returns>
        private TimeSpan GetPollingInterval(string keyName, int defaultValue, TimeSpan maxValue)
        {
            int pollingInterval = configSection.ReadConfigValue(keyName, defaultValue);

            var interval = TimeSpan.FromSeconds(Math.Min(pollingInterval, maxValue.TotalSeconds));
            return interval;
        }       

        private TimeSpan GetHealthValidityPeriod(TimeSpan pollingInterval)
        {
            int expiryFactor = configSection.ReadConfigValue(Constants.ConfigKeys.HealthReportExpiryFactor,
                DefaultHealthReportExpiryFactor);

            TimeSpan validityPeriod = TimeSpan.FromSeconds(pollingInterval.TotalSeconds * expiryFactor);

            return validityPeriod;
        }

        private TimeSpan GetCompletedTaskAgeThreshold()
        {
            return GetPollingInterval(
                Constants.ConfigKeys.CompletedTaskAgeThreshold,
                4 * 60 * 60, // 4 hours
                TimeSpan.MaxValue);
        }

        /// <summary>
        /// Cleans up state during coordinator shutdown and releases the lock.
        /// </summary>
        private void CompleteRunAsync()
        {
            traceType.WriteInfo("CompleteRunAsync: coordinator is stopped");

            this.isNotificationAvailable = false;
            this.lastHealthIncarnation = 0;
            this.lastHealthIncarnationUpdatedAt = null;
            this.repairActionProvider = null;

            Interlocked.Exchange(ref this.isCoordinatorRunning, 0);
        }

        /// <summary>
        /// Observes the current incarnation number from the MR channel, and notes
        /// the timestamp of changes. The first such change marks the MR notification
        /// as available, if that has not already been done.
        /// </summary>
        private void ProcessCurrentIncarnation(IPolicyAgentDocumentForTenant doc)
        {
            // If wait for incarnation change is disabled, then start processing notifications immediately.
            if (!configSection.ReadConfigValue(Constants.ConfigKeys.WaitForIncarnationChangeOnStartup, false))
            {
                this.isNotificationAvailable = true;
            }

            ulong healthIncarnation = doc.RoleInstanceHealthInfoIncarnation;
            
            if (healthIncarnation == 0)
            {
                if (this.lastHealthIncarnationUpdatedAt == null)
                {
                    traceType.WriteInfo("No health incarnation found");

                    // CheckChannelHealth clock starts now, to catch the case
                    // where the incarnation never changes from null.
                    this.lastHealthIncarnationUpdatedAt = DateTimeOffset.UtcNow;
                }
            }
            else
            {
                if (this.lastHealthIncarnation == 0)
                {
                    traceType.WriteInfo("First observed health incarnation: {0}", healthIncarnation);

                    this.lastHealthIncarnation = healthIncarnation;
                    this.lastHealthIncarnationUpdatedAt = DateTimeOffset.UtcNow;
                }
                else if (this.lastHealthIncarnation != healthIncarnation)
                {
                    traceType.WriteInfo("Health incarnation changed: {0} -> {1}", this.lastHealthIncarnation, healthIncarnation);

                    this.lastHealthIncarnation = healthIncarnation;
                    this.lastHealthIncarnationUpdatedAt = DateTimeOffset.UtcNow;

                    // Since a change was observed, assume the channel is good, and
                    // mark the notification as available in case it is not already.
                    this.isNotificationAvailable = true;
                }
            }
        }

        /// <summary>
        /// Checks if the MR channel is unhealthy, due to the MR incarnation number failing to change
        /// within the configured timeout period. If the channel is unhealthy, this will cause the
        /// IS process to exit.
        /// </summary>        
        private void CheckChannelHealth(Guid activityId, IPolicyAgentDocumentForTenant doc)
        {
            ProcessCurrentIncarnation(doc);

            TimeSpan maxIncarnationUpdateWaitTime = GetPollingInterval(
                Constants.ConfigKeys.MaxIncarnationUpdateWaitTime,
                defaultValue: 1800,
                maxValue: TimeSpan.MaxValue);

            if (maxIncarnationUpdateWaitTime > TimeSpan.Zero && (this.lastHealthIncarnationUpdatedAt != null))
            {
                var now = DateTimeOffset.UtcNow;
                TimeSpan elapsedSinceLastUpdate = now - this.lastHealthIncarnationUpdatedAt.Value;

                if (elapsedSinceLastUpdate > maxIncarnationUpdateWaitTime)
                {
                    var message = "Health incarnation ({0}) last changed at {1:O} ({2} ago). Management channel may be down on this node; exiting process to cause failover."
                        .ToString(
                            this.lastHealthIncarnation,
                            this.lastHealthIncarnationUpdatedAt,
                            elapsedSinceLastUpdate);

                    traceType.WriteWarning("{0}", message);
                    throw new ManagementChannelTerminallyUnhealthyException(message);
                }
            }
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Diagnostics;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Interop;
    using System.Fabric.Query;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Json;
    using System.Threading;
    using System.Threading.Tasks;
    using Collections;
    using Microsoft.WindowsAzure.ServiceRuntime.Management;

    /// <summary>
    /// Coordinates interactions between Windows Azure and the Microsoft Azure Service Fabric Cluster Manager.
    /// </summary>
    /// <remarks>TODO - move common constants to a common class.</remarks>
    internal sealed class WindowsAzureInfrastructureCoordinator : IInfrastructureCoordinator
    {
        #region Static state

        /// <summary>
        /// Represents the state of the currently-active job, if any.
        /// </summary>
        private enum JobState
        {
            /// <summary>
            /// Initial query to CM has not completed
            /// </summary>
            Unknown,

            /// <summary>
            /// No job is currently active
            /// </summary>
            Idle,

            /// <summary>
            /// Job is waiting for CM approval before execution (MR pre-UD approval is pending)
            /// </summary>
            WaitingForApproval,

            /// <summary>
            /// Job is executing in Windows Azure (MR pre-UD approval has been sent)
            /// </summary>
            Executing,

            /// <summary>
            /// Job is waiting for CM health evaluation after execution (MR post-UD approval is pending)
            /// </summary>
            WaitingForHealthCheck,
        }

        private sealed class CommandHandlerEntry
        {
            public CommandHandlerEntry(bool isAdminCommand, AsyncCommandHandler commandHandler)
            {
                this.IsAdminCommand = isAdminCommand;
                this.CommandHandler = commandHandler;
            }

            public bool IsAdminCommand { get; private set; }
            public AsyncCommandHandler CommandHandler { get; private set; }
        }

        private const string ServiceNamePrefix = "fabric:/System/";
        private const string TaskIdFormat = "WindowsAzure/{0}/{1}"; // "WindowsAzure/<TenantID>/<PartitionID>"

        /// <remarks>
        /// Marked internal for unit testing.
        /// </remarks>
        internal const uint SequenceNumberStride = 4;

        private const string ConfigJobPollingIntervalInSecondsKeyName = "WindowsAzure.JobPollingIntervalInSeconds";
        private const string ConfigAllowJobAlertOnTopologyChangeKeyName = "WindowsAzure.AllowJobAlertOnTopologyChange";

        private const int DefaultJobPollingIntervalInSeconds = 15;

        /// <summary>
        /// Config key that enables the infrastructure service's repair executor.
        /// </summary>
        private const string ConfigEnableRepairExecutionKeyName = "WindowsAzure.EnableRepairExecution";

        /// <summary>
        /// The config key for adjusting the health polling interval.
        /// </summary>
        private const string ConfigHealthPollingIntervalInSecondsKeyName = "WindowsAzure.HealthPollingIntervalInSeconds";

        /// <summary>
        /// The config key for adjusting the expiration time of a health report.
        /// expiry time = factor * polling period
        /// </summary>
        private const string ConfigHealthReportExpiryFactorKeyName = "WindowsAzure.HealthReportExpiryFactor";

        /// <summary>
        /// Config key for allowing the behavior of re-acking with a (SignalReady or SignalError) on a 
        /// CompleteJobStep notification in cases where the previous CompleteJobStep ack sent by Infrastructure Service
        /// is missed out by Fabric Controller/Management Role (FC/MR)
        /// With re-acking, the channel between IS and MR/FC doesn't need manual operator intervention.
        /// </summary>
        private const string ConfigAllowCompletedJobStepReAckKeyName = "WindowsAzure.AllowCompletedJobStepReAck";

        /// <summary>
        /// Config key that governs the timeout duration for responding to a job that needs to be processed.
        /// </summary>
        /// <remarks>
        /// This is more an internal setting and is not needed externally (no customer asks). 
        /// Making it configurable just for unit testability.
        /// </remarks>
        private const string ConfigRecentJobStateUpdateThresholdInSecondsKeyName = "WindowsAzure.RecentJobStateUpdateThresholdInSeconds";

        /// <summary>
        /// Config key format that maps the <see cref="ImpactReason"/> to a corresponding <see cref="NodeTask"/>.
        /// This mapping is provided by the user (in the service configuration)
        /// </summary>
        private const string ConfigNodeTaskForImpactReasonFormatKeyName =
            "WindowsAzure.NodeTaskForImpactReason.{0}";

        /// <summary>
        /// Config key format that enables learning mode (<see cref="IJobImpactManager"/>) for each job type.
        /// This mapping is provided by the user (in the service configuration).
        /// </summary>
        internal const string ConfigEnableLearningModeForJobTypeFormatKeyName =
            "WindowsAzure.LearningMode.Enable.{0}";

        /// <summary>
        /// Config key that defines minimum impact for a platform update job.
        /// </summary>
        private const string ConfigMinimumImpactPlatformUpdateJobKeyName = "WindowsAzure.MinimumImpact.PlatformUpdateJob";

        /// <summary>
        /// Default value for learning mode. <see cref="ConfigEnableLearningModeForJobTypeFormatKeyName"/>.
        /// </summary>
        internal const bool DefaultEnableLearningMode = false;

        /// <summary>
        /// The default health polling interval in seconds if none is provided in the configuration (clustermanifest.xml)
        /// </summary>
        private const int DefaultHealthPollingIntervalInSeconds = 30;

        /// <summary>
        /// A factor used for calculating time to live (or expiry time of a health report).
        /// expiry time = factor * polling period
        /// E.g. 6 * 30 seconds = 3 minutes
        /// </summary>
        private const int DefaultHealthReportExpiryFactor = 6;

        /// <summary>
        /// <see cref="ConfigAllowCompletedJobStepReAckKeyName"/>
        /// </summary>
        private const bool DefaultAllowCompletedJobStepReAck = true;

        /// <summary>
        /// Config key that determines whether the coordinator needs to wait for a change in the incarnation
        /// number after it becomes active, so that it can be more confident that the MR channel is functioning properly.
        /// </summary>
        private const string ConfigWaitForIncarnationChangeOnStartupKeyName = "WindowsAzure.WaitForIncarnationChangeOnStartup";

        /// <summary>
        /// Config key that determines how long the coordinator should wait for the incarnation
        /// number to change before concluding that the MR channel is down. Exceeding this time
        /// will cause the IS to exit in order to trigger a failover.
        /// </summary>
        private const string ConfigMaxIncarnationUpdateWaitTimeKeyName = "WindowsAzure.MaxIncarnationUpdateWaitTimeInSeconds";

        /// <summary>
        /// Config key that determines whether the ReadyToExecute breakpoint is enabled for a particular job type.
        /// This breakpoint occurs after node deactivation completion and before SignalReady.
        /// </summary>
        /// <example>
        /// WindowsAzure.EnableBreakpoint.ReadyToExecute.DeploymentUpdateJob=true
        /// </example>
        private const string ConfigEnableBreakpointReadyToExecuteFormatKeyName = "WindowsAzure.EnableBreakpoint.ReadyToExecute.{0}";

        /// <summary>
        /// Config key that determines whether the JobExecutionCompleted breakpoint is enabled.
        /// This breakpoint occurs after job execution completion and before node reactivation.
        /// </summary>
        private const string ConfigEnableBreakpointJobExecutionCompletedKeyName = "WindowsAzure.EnableBreakpoint.JobExecutionCompleted";

        /// <summary>
        /// Config key that determines whether post (CompleteJobStep) notifications are always
        /// acknowledged, regardless of health check result.
        /// </summary>
        private const string ConfigAlwaysAckPostNotifications = "WindowsAzure.AlwaysAckPostNotifications";

        /// <summary>
        /// The maximum health polling interval to apply a suitable cap
        /// (in case the user supplies an unreasonable value in clustermanifest.xml)
        /// </summary>
        private static readonly TimeSpan MaxHealthPollingInterval = TimeSpan.FromDays(10);

        private static readonly TraceType TraceType = new TraceType("WindowsAzureFabricCoordinator");

        private static readonly TimeSpan TimeSpanInfinite = TimeSpan.FromMilliseconds(-1);
        private static readonly TimeSpan ClusterManagerRequestTimeout = TimeSpan.FromSeconds(30);
        private static readonly TimeSpan DefaultRecentJobStateUpdateThreshold = TimeSpan.FromMinutes(5);
        private static readonly TimeSpan MaxJobPollingInterval = TimeSpan.FromDays(10);
        private static readonly TimeSpan QueryRetryDelay = TimeSpan.FromSeconds(5);
        private static readonly char[] CommandTokenDelimiter = new[] { ':' };

        #endregion

        #region Instance state

        delegate Task<string> AsyncCommandHandler(string args);

        private readonly Dictionary<string, CommandHandlerEntry> commandHandlerMap =
            new Dictionary<string, CommandHandlerEntry>(StringComparer.OrdinalIgnoreCase);

        private readonly object stateLock = new object();

        private readonly string tenantId;
        private readonly string taskId;

        private readonly IInfrastructureAgentWrapper infrastructureAgent;

        private TaskCompletionSource<object> runTaskCompletionSource;
        private CancellationToken runCancellationToken = CancellationToken.None;

        private int primaryEpoch;
        private uint nextSequenceNumber;

        private Task activeClusterManagerTask;

        private DateTime lastKnownJobStateUpdateTime = DateTime.UtcNow;
        private JobState lastKnownJobState = JobState.Unknown;
        private InfrastructureTaskDescription lastKnownTask;

        private bool jobRefreshTimerIsScheduled;
        private Timer jobRefreshTimer;
        private bool isNotificationAvailable;
        private ulong? lastHealthIncarnation;
        private DateTimeOffset? lastHealthIncarnationUpdatedAt;
        private IManagementNotificationContext lastNotification;
        private readonly IJobBlockingPolicyManager jobBlockingPolicyManager;
        private readonly IAzureModeDetector modeDetector;

        private BreakpointManager breakpointManager;

        private readonly IJobImpactManager jobImpactManager;
        private readonly ImpactTranslator impactTranslator;

        private WindowsAzureRepairExecutor repairExecutor;

        private bool healthRefreshTimerIsScheduled;
        private Timer healthRefreshTimer;
        private readonly IRoleInstanceHealthWatchdog roleInstanceHealthWatchdog;

        private readonly IHealthClient healthClient;

        /// <summary>
        /// <see cref="VolatileLastCompletedJobStepInfo"/>.
        /// </summary>
        private readonly VolatileLastCompletedJobStepInfo lastCompletedJobStepInfo;

        /// <summary>
        /// <see cref="ConfigRecentJobStateUpdateThresholdInSecondsKeyName"/>
        /// </summary>
        private readonly TimeSpan recentJobStateUpdateThreshold;

        /// <summary>
        /// Used for learning mode. The result of <see cref="IJobImpactManager.EvaluateJobImpactAsync"/>
        /// in StartInfrastructureTask while sending the infrastructure task to CM is saved in this variable.
        /// It is then used to compare the task received from CM with the notification before finally post-acking
        /// the completion of the job (i.e. in CompleteJobStep).
        /// </summary>
        private JobImpactTranslationMode jobImpactTranslationMode;

        /// <summary>
        /// Number of nodes that may be impacted after the job is approved.
        /// </summary>
        private int nodesImpactedCount = -1;

        #endregion

        #region IInfrastructureCoordinator public methods

        public WindowsAzureInfrastructureCoordinator(
            string tenantId,
            IManagementClient managementClient,
            IInfrastructureAgentWrapper infrastructureAgent,
            IConfigStore configStore,
            string configSectionName,
            Guid partitionId,
            long replicaId,
            IHealthClient healthClient,
            IJobBlockingPolicyManager jobBlockingPolicyManager,
            IJobImpactManager jobImpactManager,
            IAzureModeDetector modeDetector)
        {
            this.tenantId = tenantId.Validate("tenantId");

            managementClient.ThrowIfNull("managementClient");
            infrastructureAgent.ThrowIfNull("infrastructureAgent");
            configStore.ThrowIfNull("configStore");
            configSectionName.ThrowIfNullOrWhiteSpace("configSectionName");
            healthClient.ThrowIfNull("healthClient");
            jobBlockingPolicyManager.ThrowIfNull("jobBlockingPolicyManager");
            jobImpactManager.Validate("jobImpactManager");

            this.PartitionId = partitionId;
            this.ReplicaId = replicaId;
            this.taskId = String.Format(CultureInfo.InvariantCulture, TaskIdFormat, tenantId, partitionId);
            this.ManagementClient = managementClient;
            this.infrastructureAgent = infrastructureAgent;
            this.ConfigStore = configStore;
            this.ConfigSectionName = configSectionName;
            this.healthClient = healthClient;
            this.jobBlockingPolicyManager = jobBlockingPolicyManager;
            this.jobImpactManager = jobImpactManager;

            this.modeDetector = modeDetector;
            if (modeDetector != null)
            {
                TraceType.WriteInfo("Azure mode auto-detection is enabled");
            }

            var configSection = new ConfigSection(TraceType, configStore, configSectionName);

            var healthWatchdogFactory = new RoleInstanceHealthWatchdogFactory(configSection, healthClient);
            roleInstanceHealthWatchdog = healthWatchdogFactory.Create("WindowsAzure.");

            this.impactTranslator = new ImpactTranslator(configSection);

            this.recentJobStateUpdateThreshold = TimeSpan.FromSeconds(
                ReadConfigValue(ConfigRecentJobStateUpdateThresholdInSecondsKeyName, (int)DefaultRecentJobStateUpdateThreshold.TotalSeconds));

            lastCompletedJobStepInfo = new VolatileLastCompletedJobStepInfo(this.recentJobStateUpdateThreshold);

            this.RegisterCommandHandlers();

            Trace.WriteInfo(TraceType, "Created coordinator: taskId = '{0}', configSectionName = '{1}'", this.taskId, this.ConfigSectionName);
        }
        
        /// <summary>
        /// Gets the task Id which is of the format WindowsAzure/[TenantID]/[PartitionID]
        /// </summary>
        public string TaskId
        {
            get { return taskId; }
        }

        public Guid PartitionId { get; private set; }

        public long ReplicaId { get; private set; }

        public IManagementClient ManagementClient { get; private set; }

        /// <summary>
        /// Gets the configuration store.
        /// </summary>
        public IConfigStore ConfigStore { get; private set; }

        public string ConfigSectionName { get; private set; }

        /// <summary>
        /// Starts the coordinator.
        /// </summary>
        /// <param name="primaryEpoch">
        /// Primary epoch number, used as the leading bits of the instance ID
        /// in interactions with the Cluster Manager.
        /// </param>
        /// <param name="token">
        /// The token to monitor for cancellation requests.
        /// </param>
        /// <returns>
        /// A long-running task that represents the asynchronous execution of the coordinator.
        /// The task runs until the coordinator has finished shutting down in response to a cancellation request.
        /// </returns>
        /// <remarks>Async entry point</remarks>
        public Task RunAsync(int primaryEpoch, CancellationToken token)
        {
            if (primaryEpoch < 0)
                throw new ArgumentOutOfRangeException("primaryEpoch");

            TaskCompletionSource<object> tcs = new TaskCompletionSource<object>();

            lock (this.stateLock)
            {
                if (this.runTaskCompletionSource != null)
                {
                    throw new InvalidOperationException("Coordinator has already been started");
                }
                if (primaryEpoch < this.primaryEpoch)
                {
                    throw new ArgumentException("Primary epoch must be non-decreasing", "primaryEpoch");
                }

                if (this.primaryEpoch < primaryEpoch)
                {
                    this.nextSequenceNumber = 0;
                }
                this.primaryEpoch = primaryEpoch;

                Trace.WriteInfo(TraceType, "RunAsync: primaryEpoch = 0x{0:X}, nextSequenceNumber = 0x{1:X}",
                    this.primaryEpoch,
                    this.nextSequenceNumber);

                TraceType.WriteAggregatedEvent(
                    Constants.CoordinatorModeKey, 
                    this.tenantId, 
                    "{0}", 
                    new CoordinatorStateData().Mode);

                this.runTaskCompletionSource = tcs;
                this.runCancellationToken = token;

                this.jobRefreshTimer = new Timer(this.OnJobRefreshTimerExpired, null, Timeout.Infinite, Timeout.Infinite);
                this.jobRefreshTimerIsScheduled = false;

                this.healthRefreshTimer = new Timer(this.OnHealthRefreshTimerExpired, null, Timeout.Infinite, Timeout.Infinite);
                this.healthRefreshTimerIsScheduled = false;

                this.lastNotification = null;
                this.isNotificationAvailable = false;
                this.lastHealthIncarnation = null;
                this.lastHealthIncarnationUpdatedAt = null;

                this.breakpointManager = new BreakpointManager();

                jobImpactTranslationMode = JobImpactTranslationMode.Default;
                jobImpactManager.Reset();

                // clear the cache so that it doesn't hold on to the old cached value
                // especially when the IS replica changes from primary->secondary->primary
                // with another replica updating the job blocking policy while this one was 
                // demoted to a secondary for a brief period.
                jobBlockingPolicyManager.ClearCache();

                // Begin the initial CM query to get the last known state
                this.RecoverState();
            }
            
            this.ScheduleJobPollingTimer(false);
            this.ScheduleHealthPollingTimer(false);

            // This could result in OnCancelRequested being called synchronously, if the token is already cancelled.
            // Register outside of the lock to avoid an unnecessarily-confusing recursive acquire.
            token.Register(this.OnCancelRequested);
            // TODO: track and clear cancellation token registration?

            return tcs.Task;
        }

        /// <remarks>Async entry point</remarks>
        public Task ReportStartTaskSuccessAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() => this.ProcessStartTaskSuccess(taskId, instanceId));
        }

        /// <remarks>Async entry point</remarks>
        public Task ReportFinishTaskSuccessAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() => this.ProcessFinishTaskSuccess(taskId, instanceId));
        }

        /// <remarks>Async entry point</remarks>
        public Task ReportTaskFailureAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.Factory.StartNew(() => this.ProcessTaskFailure(taskId, instanceId));
        }

        /// <remarks>Async entry point</remarks>
        public Task<string> RunCommandAsync(bool isAdminCommand, string input, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.DispatchCommand(input, isAdminCommand);
        }

        #endregion

        #region IInfrastructureService handlers for CM messages

        private static void SignalReady(IManagementNotificationContext notification)
        {
            Stopwatch watch = Stopwatch.StartNew();
            try
            {
                notification.SignalReady();
                Trace.WriteInfo(TraceType, "SignalReady completed in {0}", watch.Elapsed);
            }
            catch (Exception e)
            {
                Trace.WriteWarning(TraceType, "SignalReady failed in {0}: {1}", watch.Elapsed, e);
                throw;
            }
        }

        private static void SignalError(IManagementNotificationContext notification, string errorDescription)
        {
            Stopwatch watch = Stopwatch.StartNew();
            try
            {
                notification.SignalError(errorDescription);
                Trace.WriteInfo(TraceType, "SignalError completed in {0}", watch.Elapsed);
            }
            catch (Exception e)
            {
                Trace.WriteWarning(TraceType, "SignalError failed in {0}: {1}", watch.Elapsed, e);
                throw;
            }
        }

        /// <remarks>Caller must hold stateLock</remarks>
        private IManagementNotificationContext GetCurrentNotification()
        {
            if (!this.isNotificationAvailable)
            {
                throw Trace.CreateException(
                    TraceType,
                    NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NOT_READY,
                    "Management notification is not available yet");
            }

            return this.lastNotification;
        }

        private bool StopAtReadyToExecuteBreakpoint(IManagementNotificationContext notification)
        {
            string configKey = string.Format(CultureInfo.InvariantCulture, ConfigEnableBreakpointReadyToExecuteFormatKeyName, notification.ActiveJobType);
            string breakpointName = string.Format(CultureInfo.InvariantCulture, "ReadyToExecute:{0}:{1}", notification.ActiveJobId, notification.ActiveJobStepTargetUD);

            return this.ReadConfigValue(configKey, false) && this.breakpointManager.StopAtBreakpoint(breakpointName);
        }

        // The current notification may not match the job ID that was executing, or the
        // current notification may have no active job at all.  So InstanceId is the only
        // unique value available at this point.
        private bool StopAtJobExecutionCompletedBreakpoint(long taskInstanceId)
        {
            string configKey = ConfigEnableBreakpointJobExecutionCompletedKeyName;
            string breakpointName = string.Format(CultureInfo.InvariantCulture, "JobExecutionCompleted:0x{0:X}", taskInstanceId);

            return this.ReadConfigValue(configKey, false) && this.breakpointManager.StopAtBreakpoint(breakpointName);
        }

        private bool StopAtJobBlockingBreakpoint(JobBlockingPolicy policy, IManagementNotificationContext notification)
        {
            string breakpointName = string.Format(CultureInfo.InvariantCulture,
                "{0}:{1}:{2}",
                policy,
                notification.ActiveJobId,
                notification.ActiveJobStepTargetUD);

            return this.breakpointManager.StopAtBreakpoint(breakpointName);
        }

        private void ProcessStartTaskSuccess(string taskId, long instanceId)
        {
            Trace.WriteInfo(TraceType, "ProcessStartTaskSuccess('{0}', 0x{1:X})", taskId, instanceId);
            this.VerifyTaskId(taskId);

            if (!ShouldAllowStateTransition(null))
            {
                return;
            }

            lock (this.stateLock)
            {
                this.VerifyIsRunning();
                this.VerifyKnownInstanceId(instanceId);

                if (this.IsStaleInstanceId(instanceId))
                {
                    Trace.WriteInfo(TraceType, "Acknowledging ProcessStartTaskSuccess with stale instance ID 0x{0:X}", instanceId);
                    return;
                }

                IManagementNotificationContext notification = this.GetCurrentNotification();

                if (this.NotificationMatchesExpectedState(notification, NotificationType.StartJobStep, jobImpactTranslationMode))
                {
                    if ((this.lastKnownJobState == JobState.WaitingForApproval) && this.StopAtReadyToExecuteBreakpoint(notification))
                    {
                        // Fail here instead of returning successfully, so that the CM
                        // task state stays in PreAckPending (interpreted as WaitingForApproval)
                        // instead of PreAcked (interpreted as Executing, which is misleading).
                        // If the IS fails over after hitting a ReadyToExecute breakpoint, the
                        // last known job state should remain WaitingForApproval.
                        throw Trace.CreateException(
                            TraceType,
                            NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NOT_READY,
                            "Cannot process start task success until ReadyToExecute breakpoint is disabled or continued");
                    }

                    Trace.WriteInfo(TraceType,
                        "ProcessStartTaskSuccess: calling SignalReady for notification {0} (task 0x{1:X}, {2})",
                        NotificationToShortDisplayString(notification),
                        this.lastKnownTask.InstanceId,
                        this.lastKnownJobState);

                    SignalReady(notification);
                }
                else
                {
                    Trace.WriteWarning(TraceType,
                        "ProcessStartTaskSuccess: skipping SignalReady because notification ({0}) does not match current task (0x{1:X}, {2})",
                        NotificationToShortDisplayString(notification),
                        this.lastKnownTask.InstanceId,
                        this.lastKnownJobState);
                }

                if (this.lastKnownJobState < JobState.Executing)
                {
                    this.SetLastKnownJobState(JobState.Executing);
                }
            }
        }

        private void ProcessFinishTaskSuccess(string taskId, long instanceId)
        {
            Trace.WriteInfo(TraceType, "ProcessFinishTaskSuccess('{0}', 0x{1:X})", taskId, instanceId);
            this.VerifyTaskId(taskId);

            if (!ShouldAllowStateTransition(null))
            {
                return;
            }

            lock (this.stateLock)
            {
                this.VerifyIsRunning();
                this.VerifyKnownInstanceId(instanceId);

                if (this.IsStaleInstanceId(instanceId))
                {
                    Trace.WriteInfo(TraceType, "Acknowledging ProcessFinishTaskSuccess with stale instance ID 0x{0:X}", instanceId);
                    return;
                }

                IManagementNotificationContext notification = this.GetCurrentNotification();

                bool matches = this.NotificationMatchesExpectedStateInCompleteJobStep(notification);

                if (matches)
                {
                    Trace.WriteInfo(TraceType,
                        "ProcessFinishTaskSuccess: calling SignalReady for notification {0} (task 0x{1:X}, {2})",
                        NotificationToShortDisplayString(notification),
                        this.lastKnownTask.InstanceId,
                        this.lastKnownJobState);

                    SignalReady(notification);
                    lastCompletedJobStepInfo.Update(
                        notification.ActiveJobId,
                        notification.ActiveJobStepTargetUD,
                        notification.NotificationType,
                        NotificationAction.SignalReady);
                }
                else if (notification == null)
                {
                    Trace.WriteInfo(TraceType, "ProcessFinishTaskSuccess: skipping SignalReady because there is no active notification");
                }
                else
                {
                    Trace.WriteWarning(TraceType,
                        "ProcessFinishTaskSuccess: skipping SignalReady because notification ({0}) does not match current task (0x{1:X}, {2})",
                        NotificationToShortDisplayString(notification),
                        this.lastKnownTask.InstanceId,
                        this.lastKnownJobState);
                }

                this.SetLastKnownJobState(JobState.Idle);
            }
        }

        private void ProcessTaskFailure(string taskId, long instanceId)
        {
            Trace.WriteWarning(TraceType, "ProcessTaskFailure('{0}', 0x{1:X})", taskId, instanceId);
            this.VerifyTaskId(taskId);

            if (this.AlwaysAckPostNotifications())
            {
                TraceType.WriteInfo("AlwaysAckPostNotifications is enabled; ignoring CM task failure");
                ProcessFinishTaskSuccess(taskId, instanceId);
                return;
            }

            if (!ShouldAllowStateTransition(null))
            {
                return;
            }

            lock (this.stateLock)
            {
                this.VerifyIsRunning();
                this.VerifyKnownInstanceId(instanceId);

                if (this.IsStaleInstanceId(instanceId))
                {
                    Trace.WriteInfo(TraceType, "Acknowledging ProcessTaskFailure with stale instance ID 0x{0:X}", instanceId);
                    return;
                }

                if (this.lastKnownJobState == JobState.WaitingForApproval ||
                    this.lastKnownJobState == JobState.Executing)
                {
                    // A failure response is currently not expected in the Pre phase, though at the moment
                    // it occurs when the deactivations would cause loss of seed quorum.
                    throw Trace.CreateException(
                        TraceType,
                        NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NOT_READY,
                        "Unexpected failure report for task instance ID 0x{0:X}",
                        instanceId);
                }

                IManagementNotificationContext notification = this.GetCurrentNotification();

                bool matches = this.NotificationMatchesExpectedStateInCompleteJobStep(notification);

                // if learning mode was initially on during a pre, but later turned off during a post, then the notification won't match and 
                // manual intervention is necessary. but that is a rare scenario which we will not handle now.
                if (matches)
                {
                    if (!this.JobCanBeAlerted(notification))
                    {
                        throw Trace.CreateException(
                            TraceType,
                            NativeTypes.FABRIC_ERROR_CODE.E_ABORT,
                            "ProcessTaskFailure: Current job cannot be alerted; operator intervention may be required: {0}",
                            NotificationToShortDisplayString(notification));
                    }

                    Trace.WriteInfo(TraceType,
                        "ProcessTaskFailure: calling SignalError for notification {0} (task 0x{1:X}, {2})",
                        NotificationToShortDisplayString(notification),
                        this.lastKnownTask.InstanceId,
                        this.lastKnownJobState);

                    string errorDescription = String.Format(
                        CultureInfo.InvariantCulture,
                        "Microsoft Azure Service Fabric infrastructure task instance ID 0x{0:X} failed",
                        instanceId);

                    SignalError(notification, errorDescription);
                    lastCompletedJobStepInfo.Update(
                        notification.ActiveJobId, 
                        notification.ActiveJobStepTargetUD, 
                        notification.NotificationType, 
                        NotificationAction.SignalError,
                        errorDescription);
                }
                else if (notification == null)
                {
                    Trace.WriteInfo(TraceType, "ProcessTaskFailure: skipping SignalError because there is no active notification");
                }
                else
                {
                    Trace.WriteWarning(TraceType,
                        "ProcessTaskFailure: skipping SignalError because notification ({0}) does not match current task (0x{1:X}, {2})",
                        NotificationToShortDisplayString(notification),
                        this.lastKnownTask.InstanceId,
                        this.lastKnownJobState);
                }

                this.SetLastKnownJobState(JobState.Idle);
            }
        }

        private bool JobCanBeAlerted(IManagementNotificationContext notification)
        {
            bool allowJobAlertOnTopologyChange = this.ReadConfigValue<bool>(ConfigAllowJobAlertOnTopologyChangeKeyName);

            // The job can be alerted if configuration allows alerting on topology changes,
            // or if the job does not have a topology change.
            return allowJobAlertOnTopologyChange || !notification.ActiveJobIncludesTopologyChange;
        }

        private bool AlwaysAckPostNotifications()
        {
            return this.ReadConfigValue(ConfigAlwaysAckPostNotifications, defaultValue: false);
        }

        #endregion

        #region Cancellation and timer callbacks

        /// <summary>Handles the request to cancel execution of the coordinator.</summary>
        /// <remarks>Async entry point</remarks>
        private void OnCancelRequested()
        {
            Trace.WriteInfo(TraceType, "Cancel requested");

            var jobTimerDisposedEvent = new ManualResetEventSlim(false);
            var healthTimerDisposedEvent = new ManualResetEventSlim(false);

            Task jobTimerDisposeTask = TaskUtility.WaitAsync(jobTimerDisposedEvent.WaitHandle);
            Task healthTimerDisposeTask = TaskUtility.WaitAsync(healthTimerDisposedEvent.WaitHandle);

            Task clusterManagerTask = Utility.CreateCompletedTask<object>(null);

            lock (this.stateLock)
            {
                // If there is an active CM call, capture that task for the continuation.
                // If there is no active CM call, use an already-completed task instead.
                if (this.activeClusterManagerTask != null)
                {
                    Trace.WriteInfo(TraceType, "Will wait on active cluster manager task");
                    clusterManagerTask = this.activeClusterManagerTask;
                }
            }

            Trace.WriteInfo(TraceType, "Disposing timers");
            this.jobRefreshTimer.Dispose(jobTimerDisposedEvent.WaitHandle);
            this.healthRefreshTimer.Dispose(healthTimerDisposedEvent.WaitHandle);

            Task[] cleanupTasks = { jobTimerDisposeTask, healthTimerDisposeTask, clusterManagerTask };

            Task.Factory.ContinueWhenAll(cleanupTasks, t => CompleteRunTask());
        }

        /// <summary>
        /// Cleans up state after coordinator shutdown tasks have completed, and then marks the run task as cancelled.
        /// </summary>
        private void CompleteRunTask()
        {
            Trace.WriteInfo(TraceType, "CompleteRunTask called");

            lock (this.stateLock)
            {
                Debug.Assert(!this.IsClusterManagerAsyncTaskRunning());

                this.SetLastKnownJobState(JobState.Unknown, null, null);

                this.jobRefreshTimer = null;
                this.jobRefreshTimerIsScheduled = false;

                this.healthRefreshTimer = null;
                this.healthRefreshTimerIsScheduled = false;

                this.lastNotification = null;
                this.isNotificationAvailable = false;
                this.lastHealthIncarnation = null;
                this.lastHealthIncarnationUpdatedAt = null;

                this.runCancellationToken = CancellationToken.None;
                this.runTaskCompletionSource.SetCanceled();
                this.runTaskCompletionSource = null;

                Trace.WriteInfo(TraceType, "CompleteRunTask finished; coordinator is stopped");
            }
        }

        /// <summary>
        /// Job notification refresh timer callback.
        /// </summary>
        /// <param name="state">The state.</param>
        /// <remarks>
        /// Async entry point
        /// </remarks>
        private void OnJobRefreshTimerExpired(object state)
        {
            Trace.WriteNoise(TraceType, "OnJobRefreshTimerExpired called");
            try
            {
                this.OnRunProcessCurrentNotification(true);
            }
            catch (Exception ex)
            {
                Trace.WriteWarning(TraceType, "Error processing current notification in OnJobRefreshTimerExpired. Exiting process. Exception: {0}", ex);

                // the Program's Main method is waiting on this global event. Once set, it exits the process.
                // Workaround over using Environment.Exit which isn't supported on CoreCLR. Please see this
                // method's comments for more.
                ProcessCloser.ExitEvent.Set();
            }
        }

        /// <summary>
        /// Health notification refresh timer callback.
        /// </summary>
        /// <param name="state">The state.</param>
        private void OnHealthRefreshTimerExpired(object state)
        {
            Trace.WriteNoise(TraceType, "OnHealthRefreshTimerExpired called");
            this.ProcessHealthCheck(true);
        }

        #endregion

        private TimeSpan GetHealthValidityPeriod()
        {
            TimeSpan pollingInterval = GetPollingInterval(
                ConfigHealthPollingIntervalInSecondsKeyName,
                DefaultHealthPollingIntervalInSeconds,
                MaxHealthPollingInterval);

            int expiryFactor = ReadConfigValue(
                ConfigHealthReportExpiryFactorKeyName,
                DefaultHealthReportExpiryFactor);

            TimeSpan validityPeriod = TimeSpan.FromSeconds(pollingInterval.TotalSeconds * expiryFactor);

            return validityPeriod;
        }

        private IList<RoleInstance> GetRoleInstances()
        {
            IList<RoleInstance> roleInstances = null;

            try
            {
                roleInstances = ManagementClient.GetRoleInstances();
            }
            catch (Exception ex)
            {
                // no choice but to catch everything since we don't know what to expect from management client
                TraceType.WriteWarning(                    
                    "Unable to get role instance info from management client. Error(s): {0}",
                    ex.GetMessage());
            }

            return roleInstances;
        }

        private void ProcessHealthCheck(bool isTimerCallback)
        {
            lock (this.stateLock)
            {
                ulong? serviceHealthIncarnation = this.ManagementClient.GetServiceHealthIncarnation();

                Trace.WriteInfo(
                    TraceType,
                    "ProcessHealthCheck (task ID = '{0}', ServiceHealthIncarnation: {1})",
                    this.taskId,
                    serviceHealthIncarnation.HasValue ? serviceHealthIncarnation.ToString() : "<null>");

                var sw = Stopwatch.StartNew();

                try
                {
                    IList<RoleInstance> roleInstances = GetRoleInstances();
                    TimeSpan validityPeriod = GetHealthValidityPeriod();   
                                     
                    roleInstanceHealthWatchdog.ReportHealth(roleInstances, validityPeriod);

                    Trace.WriteInfo(TraceType, "ProcessHealthCheck (task ID = '{0}') completed. Time taken: {1} ms", taskId, sw.Elapsed.TotalMilliseconds);
                }
                catch (Exception ex)
                {
                    // no choice but to catch everything since we don't know what to expect from each policy implementation
                    string text = ex.GetMessage();

                    Trace.WriteWarning(
                        TraceType,
                        "ProcessHealthCheck (task ID = '{0}') failed. Time taken: {1} ms. Error(s): {2}",
                        taskId,
                        sw.Elapsed.TotalSeconds,
                        text);
                }
            }

            // If invoked by the timer callback, this will reschedule the timer.
            // If invoked by the manual command, this will schedule the timer only if it is disabled.
            this.ScheduleHealthPollingTimer(isTimerCallback);
        }

        #region Cluster manager queries

        /// <summary>
        /// Starts recovery of task state by querying the CM.
        /// </summary>
        private void RecoverState()
        {
            Trace.WriteInfo(TraceType, "RecoverState()");

            Task queryTask = this.infrastructureAgent.QueryInfrastructureTaskAsync(
                ClusterManagerRequestTimeout,
                this.runCancellationToken).ContinueWith(this.RecoverStateCompleted);

            this.BeginClusterManagerAsyncTask(queryTask);

            this.SetLastKnownJobState(JobState.Unknown, null, null);
        }

        /// <summary>
        /// Completion callback for RecoveryState async task.  Processes the query result from CM.
        /// </summary>
        /// <remarks>Async entry point</remarks>
        private void RecoverStateCompleted(Task<InfrastructureTaskQueryResult> task)
        {
            Trace.WriteInfo(TraceType, "RecoverStateCompleted: task {0}", task.Status);
            lock (this.stateLock)
            {
                this.EndClusterManagerAsyncTask();

                if (this.runCancellationToken.IsCancellationRequested)
                {
                    Trace.WriteInfo(TraceType, "RecoverStateCompleted: discarding results because coordinator is stopping");
                    return;
                }

                if (task.IsFaulted)
                {
                    Exception taskException = task.Exception.InnerException;
                    Trace.WriteWarning(TraceType, "RecoverStateCompleted: query faulted: {0}", taskException);

                    if (taskException is TimeoutException)
                    {
                        // Timed out, so retry immediately
                        this.RecoverState();
                    }
                    else
                    {
                        // Otherwise, delay before retrying
                        this.DelayInvoke(QueryRetryDelay, "RecoverState", this.RecoverState);
                    }
                    return;
                }

                Trace.WriteInfo(TraceType, "RecoverStateCompleted: query succeeded");

                InfrastructureTaskDescription lastTask = null;
                JobState jobState = JobState.Idle;

                InfrastructureTaskResultItem taskResult = this.FindTask(task.Result);
                if (taskResult != null)
                {
                    lastTask = taskResult.Description;
                    jobState = TranslateTaskStateToJobState(taskResult.State);
                }

                this.SetLastKnownJobState(jobState, lastTask, null);
                this.DumpLastKnownTaskState();
            }
        }

        /// <summary>
        /// Invokes a continuation action after a delay.
        /// </summary>
        /// <param name="delay">Amount of time to delay.</param>
        /// <param name="continuationName">Description of the continuation action.</param>
        /// <param name="continuation">The continuation action to be invoked.</param>
        private void DelayInvoke(TimeSpan delay, string continuationName, Action continuation)
        {
            Trace.WriteInfo(TraceType, "Sleeping for {0} before invoking {1}", delay, continuationName);

            Task sleepTask = TaskUtility.SleepAsync(delay).ContinueWith(
                t => this.DelayInvokeCompleted(t, continuationName, continuation));

            this.BeginClusterManagerAsyncTask(sleepTask);
        }

        /// <summary>
        /// Completion callback for DelayInvoke async task.  Invokes the given continuation.
        /// </summary>
        /// <remarks>Async entry point</remarks>
        private void DelayInvokeCompleted(Task task, string continuationName, Action continuation)
        {
            Trace.WriteInfo(TraceType, "DelayInvokeCompleted({1}): task {0}", task.Status, continuationName);
            lock (this.stateLock)
            {
                this.EndClusterManagerAsyncTask();

                if (this.runCancellationToken.IsCancellationRequested)
                {
                    Trace.WriteInfo(TraceType, "DelayInvokeCompleted: skipping continuation because coordinator is stopping");
                    return;
                }

                continuation();
            }
        }

        /// <summary>
        /// Tries to evaluate the impact of the job by 'learning' from previous data.
        /// <see cref="isLearningModeEligible"/> is set to true if learning mode is enabled.
        /// <c>true</c> is returned if evaluation completed without throwing an exception.
        /// </summary>
        private bool TryEvaluateJobImpact(IManagementNotificationContext notification, out bool isLearningModeEligible)
        {
            jobImpactTranslationMode = JobImpactTranslationMode.Default;

            isLearningModeEligible = IsJobEligibleForLearningModeOptimization(notification);
            if (!isLearningModeEligible)
            {
                return true;
            }
            
            Exception exception = null;
            try
            {
                jobImpactTranslationMode =
                    jobImpactManager.EvaluateJobImpactAsync(notification).GetAwaiter().GetResult();
            }
            catch (TimeoutException ex)
            {
                exception = ex;
            }
            catch (FabricException ex)
            {
                exception = ex;
            }

            if (exception != null)
            {
                TraceType.WriteWarning(
                    "Error in EvaluateJobImpactAsync. Returning from StartInfrastructureTask. " +
                    "Manual/operator intervention is required if you see this repeat more than a few times. " +
                    "Notification: {0}, Error: {1}",
                    notification.ToShortDisplayString(),
                    exception);
                return false;
            }

            return true;            
        }

        /// <summary>
        /// Starts a StartInfrastructureTask call to the CM.
        /// </summary>
        /// <param name="notification">The MR notification representing the job that is starting.</param>
        /// <param name="instanceId">The infrastructure task instance ID.</param>
        private void StartInfrastructureTask(IManagementNotificationContext notification, long instanceId)
        {
            Trace.WriteInfo(TraceType, "StartInfrastructureTask(0x{0:X})", instanceId);
            Debug.Assert(!this.IsStaleInstanceId(instanceId));

            bool isLearningModeEligible;
            bool success = TryEvaluateJobImpact(notification, out isLearningModeEligible);
            if (!success)
            {
                return;
            }

            InfrastructureTaskDescription taskDescription = TranslateNotificationToTask(notification, instanceId, jobImpactTranslationMode);

            TraceType.WriteInfo(
                "Notification: {0}, Learning mode: {1}, Job impact: {2}, Task Id: {3}, Instance Id: 0x{4:X}, Node task count: {5}, Node task details: {6}",
                notification.ToShortDisplayString(),
                isLearningModeEligible,
                jobImpactTranslationMode, 
                TaskId,
                instanceId,
                taskDescription.NodeTasks.Count,
                ConvertNodeTasksToString(taskDescription));

            // Optionally block starting impactful update jobs
            this.nodesImpactedCount = taskDescription.NodeTasks.Count;
            if ((this.lastKnownJobState == JobState.Idle) && (this.nodesImpactedCount > 0))
            {
                bool isCurrentJobUpdateJob =
                    notification.ActiveJobType == JobType.DeploymentUpdateJob ||
                    notification.ActiveJobType == JobType.PlatformUpdateJob;

                bool isCurrentJobPlatformUpdateJob = (notification.ActiveJobType == JobType.PlatformUpdateJob);
                bool isCurrentJobTenantUpdateJob = (notification.ActiveJobType == JobType.DeploymentUpdateJob);


                JobBlockingPolicy currentPolicy = jobBlockingPolicyManager.Policy;

                if ((isCurrentJobUpdateJob && currentPolicy == JobBlockingPolicy.BlockNewImpactfulUpdateJobs)
                    || (isCurrentJobPlatformUpdateJob && currentPolicy == JobBlockingPolicy.BlockNewImpactfulPlatformUpdateJobs)
                    || (isCurrentJobTenantUpdateJob && currentPolicy == JobBlockingPolicy.BlockNewImpactfulTenantUpdateJobs))
                {
                    TraceType.WriteInfo(
                        "JobBlockingPolicy {0}, blocking job with impact (node count = {1}, job = {2})",
                        currentPolicy,
                        this.nodesImpactedCount,
                        notification.ToShortDisplayString());

                    if (this.StopAtJobBlockingBreakpoint(currentPolicy, notification))
                    {
                        return;
                    }
                }
            }

            Task asyncTask = infrastructureAgent.StartInfrastructureTaskAsync(
                taskDescription,
                ClusterManagerRequestTimeout,
                this.runCancellationToken).ContinueWith(t => this.StartInfrastructureTaskCompleted(t, instanceId));

            this.BeginClusterManagerAsyncTask(asyncTask);

            if (this.lastKnownJobState == JobState.Idle)
            {
                this.SetLastKnownJobState(JobState.WaitingForApproval, taskDescription, notification);
            }
        }

        /// <summary>
        /// Completion callback for StartInfrastructureTask async task.  Triggers state recovery if the call failed.
        /// </summary>
        /// <remarks>Async entry point</remarks>
        private void StartInfrastructureTaskCompleted(Task task, long instanceId)
        {
            Trace.WriteInfo(TraceType, "StartInfrastructureTaskCompleted(0x{0:X}): task {1}", instanceId, task.Status);
            lock (this.stateLock)
            {
                this.EndClusterManagerAsyncTask();

                if (this.runCancellationToken.IsCancellationRequested)
                {
                    Trace.WriteInfo(TraceType, "StartInfrastructureTaskCompleted: discarding results because coordinator is stopping");
                    return;
                }

                if (task.IsFaulted)
                {
                    Trace.WriteWarning(TraceType, "StartInfrastructureTaskCompleted: task faulted: {0}", task.Exception.InnerException);

                    // Failure may be due to having a stale instance ID, so refresh it via RecoverState
                    this.RecoverState();
                    return;
                }

                Trace.WriteInfo(TraceType, "StartInfrastructureTaskCompleted: task completed successfully");
            }
        }

        /// <summary>
        /// Starts a FinishInfrastructureTask call to the CM for the last known task.
        /// </summary>
        private void FinishInfrastructureTask()
        {
            FinishInfrastructureTask(this.lastKnownTask.InstanceId);
        }

        /// <summary>
        /// Starts a FinishInfrastructureTask call to the CM for the given task.
        /// </summary>
        /// <param name="instanceId">The infrastructure task instance ID.</param>
        private void FinishInfrastructureTask(long instanceId)
        {
            Trace.WriteInfo(TraceType, "FinishInfrastructureTask(0x{0:X})", instanceId);

            Task asyncTask = this.infrastructureAgent.FinishInfrastructureTaskAsync(
                this.taskId,
                instanceId,
                ClusterManagerRequestTimeout,
                this.runCancellationToken).ContinueWith(t => this.FinishInfrastructureTaskCompleted(t, instanceId));

            this.BeginClusterManagerAsyncTask(asyncTask);

            if (this.lastKnownJobState != JobState.Idle)
            {
                this.SetLastKnownJobState(JobState.WaitingForHealthCheck);
            }
        }

        /// <summary>
        /// Completion callback for FinishInfrastructureTask async task.  Triggers state recovery if the call failed.
        /// </summary>
        /// <remarks>Async entry point</remarks>
        private void FinishInfrastructureTaskCompleted(Task task, long instanceId)
        {
            Trace.WriteInfo(TraceType, "FinishInfrastructureTaskCompleted(0x{0:X}): task {1}", instanceId, task.Status);
            lock (this.stateLock)
            {
                this.EndClusterManagerAsyncTask();

                if (this.runCancellationToken.IsCancellationRequested)
                {
                    Trace.WriteInfo(TraceType, "FinishInfrastructureTaskCompleted: discarding results because coordinator is stopping");
                    return;
                }

                if (task.IsFaulted)
                {
                    Trace.WriteWarning(TraceType, "FinishInfrastructureTaskCompleted: task faulted: {0}", task.Exception.InnerException);

                    // Failure may be due to having a stale instance ID, so refresh it via RecoverState
                    this.RecoverState();
                    return;
                }

                Trace.WriteInfo(TraceType, "FinishInfrastructureTaskCompleted: task completed successfully");
            }
        }

        #endregion

        #region MR notification handlers

        /// <summary>
        /// Fetches the current MR notification and processes it.
        /// </summary>
        private void OnRunProcessCurrentNotification(bool isTimerCallback)
        {
            lock (this.stateLock)
            {
                IManagementNotificationContext notification = this.ManagementClient.GetCurrentNotification();
                this.ProcessManagementNotification(notification);
            }

            // If invoked by the timer callback, this will reschedule the timer.
            // If invoked by the manual command, this will schedule the timer only if it is disabled.
            this.ScheduleJobPollingTimer(isTimerCallback);
        }

        /// <summary>
        /// Evaluates the current job execution blocking policy and the current job state and returns if the current notification
        /// can be processed or not.
        /// </summary>
        private bool ShouldAllowStateTransition(IManagementNotificationContext notification)
        {
            JobBlockingPolicy currentPolicy = jobBlockingPolicyManager.Policy;

            if (currentPolicy == JobBlockingPolicy.Invalid)
            {
                Trace.WriteInfo(TraceType, "JobBlockingPolicy {0}, current policy unknown blocking all jobs and state transitions", currentPolicy.ToString());
                return false;
            }

            if (currentPolicy == JobBlockingPolicy.BlockAllJobs)
            {
                Trace.WriteInfo(TraceType, "JobBlockingPolicy {0}, blocking all jobs and state transitions", currentPolicy.ToString());
                return false;
            }

            if (notification == null || currentPolicy == JobBlockingPolicy.BlockNone)
            {
                TraceType.WriteInfo(
                    "JobBlockingPolicy {0}, {1}allowing all jobs and state transitions",
                    currentPolicy,
                    notification == null ? "notification is null, " : string.Empty);
                return true;
            }

            if ((notification.NotificationType == NotificationType.StartJobStep) && (lastKnownJobState == JobState.Idle))
            {
                bool isCurrentJobUpdateJob = (notification.ActiveJobType == JobType.DeploymentUpdateJob || notification.ActiveJobType == JobType.PlatformUpdateJob);
                bool isCurrentJobMaintenanceJob = (notification.ActiveJobType == JobType.DeploymentMaintenanceJob ||
                                                   notification.ActiveJobType == JobType.PlatformMaintenanceJob ||
                                                   notification.ActiveJobType == JobType.PlatformRepairJob);

                if ((isCurrentJobMaintenanceJob && (currentPolicy == JobBlockingPolicy.BlockNewMaintenanceJob)) ||
                    (isCurrentJobUpdateJob && (currentPolicy == JobBlockingPolicy.BlockNewUpdateJob)) ||
                    (currentPolicy == JobBlockingPolicy.BlockAllNewJobs))
                {
                    Trace.WriteInfo(TraceType, "JobBlockingPolicy {0}, blocking new jobs", currentPolicy);
                    return !this.StopAtJobBlockingBreakpoint(currentPolicy, notification);
                }
            }

            TraceType.WriteInfo("JobBlockingPolicy {0}, allowing state transition", currentPolicy);
            return true;
        }

        private void UpdateNotificationApprovalHealthStatus(HealthState serviceHealthState, string description)
        {
            if (serviceHealthState == HealthState.Ok && this.ReadConfigValue("WindowsAzure.DeprecationWarningEnabled", true))
            {
                serviceHealthState = HealthState.Warning;
                description = "The current Azure coordination mode is no longer supported. This cluster requires configuration changes.";
            }

            // Deprecated
            UpdateServiceHealthProperty(
                RoleInstanceHealthConstants.HealthReportManagementNotificationApprovalStatusPropertyName,
                serviceHealthState,
                description);

            // New common CoordinatorStatus property (used by Serial and Parallel)
            UpdateServiceHealthProperty(
                Constants.CoordinatorStatus,
                serviceHealthState,
                description);
        }

        private void UpdateServiceHealthProperty(string propertyName, HealthState serviceHealthState, string description)
        {
            var hi = new HealthInformation(
                Constants.HealthReportSourceId,
                propertyName,
                serviceHealthState)
            {
                Description = description,
            };

            // Construct this name ourselves since there is currently a bug where the service Uri which
            // could have been passed in all the way from ServiceFactory's CreateReplica is an empty string.
            var serviceUri = new Uri(ServiceNamePrefix + ConfigSectionName);

            var healthReport = new ServiceHealthReport(serviceUri, hi);

            healthClient.ReportHealth(healthReport);
        }

        /// <summary>
        /// Runs Azure mode autodetection, if enabled.
        /// </summary>
        /// <returns>true if normal coordinator execution should continue; otherwise false</returns>
        private bool CheckAzureModeAutodetect()
        {
            if (this.modeDetector == null)
            {
                // Autodetect disabled
                return true;
            }

            AzureMode mode = this.modeDetector.DetectModeAsync().GetAwaiter().GetResult();

            switch (mode)
            {
                case AzureMode.Unknown:
                    TraceType.WriteWarning("Failed to auto-detect Azure mode; continuing in serial mode");
                    return true;

                case AzureMode.Serial:
                    return true;
            }

            TraceType.WriteWarning("Auto-detected Azure mode: {0}; exiting serial coordinator", mode);
            ProcessCloser.ExitEvent.Set();
            return false;
        }

        /// <summary>
        /// Processes the given MR notification, taking actions to synchronize the CM infrastructure task state with it.
        /// </summary>
        private void ProcessManagementNotification(IManagementNotificationContext notification)
        {
            Trace.WriteInfo(TraceType, "ProcessManagementNotification (task ID = '{0}', next sequence number = 0x{1:X})",
                this.taskId,
                this.PeekNextSequenceNumber());

            Trace.WriteInfo(TraceType, "Current management notification: {0}", NotificationToShortDisplayString(notification));

            if (notification != null)
            {
                Trace.WriteInfo(TraceType, "Current management notification details: {0}", NotificationDetailsToString(notification));
            }

            if (!this.CheckAzureModeAutodetect())
            {
                return;
            }

            var jobList = this.ManagementClient.GetAllJobs().ToList();

            string jobListText = string.Format(
                CultureInfo.InvariantCulture,
                "Total number of jobs: {0}{1}",
                jobList.Count,
                jobList.Any()
                    ? string.Format(CultureInfo.InvariantCulture, ". Jobs: {0}{1}", Environment.NewLine, string.Join(Environment.NewLine, jobList))
                    : string.Empty);

            ulong? jobListIncarnation = this.ManagementClient.GetJobListIncarnation();

            Trace.WriteInfo(
                TraceType,
                "JobListIncarnation: {0}, {1}",
                jobListIncarnation.HasValue ? jobListIncarnation.ToString() : "<null>",
                jobListText);

            if (this.IsClusterManagerAsyncTaskRunning())
            {
                Trace.WriteInfo(TraceType, "Processing of management notifications is suspended while cluster manager communication is in progress");
                return;
            }

            this.DumpLastKnownTaskState();

            // Store updated state from Windows Azure runtime
            this.lastNotification = notification;

            // Always clear this; it gets set again as needed in StartInfrastructureTask,
            // and in cases where that does not get called it is not a relevant number.
            this.nodesImpactedCount = -1;

            // If wait for incarnation change is disabled, then start processing notifications immediately.
            if (!ReadConfigValue<bool>(ConfigWaitForIncarnationChangeOnStartupKeyName, false))
            {
                this.isNotificationAvailable = true;
            }

            // Observe incarnation number changes to determine if the MR channel is healthy.
            // If a change is observed, then this node has received some update over the channel,
            // and can start safely processing notifications.
            this.ProcessCurrentIncarnation();

            // If the incarnation stops changing for a long time, the MR channel may be unhealthy,
            // due to the physical node being in HI/OFR state. Exit the process to cause a failover,
            // hopefully to a healthier node.
            if (!this.CheckChannelHealth())
            {
                // Warning was logged already, and process should be exiting.
                return;
            }

            if (!this.isNotificationAvailable)
            {
                Trace.WriteInfo(TraceType, "Management notification is not available; waiting for channel health check to pass");
                return;
            }

            if (!ShouldAllowStateTransition(notification))
            {
                Trace.WriteInfo(TraceType, "Not processing management notification further since state transition is not allowed. Notification details: {0}", NotificationDetailsToString(notification));
                return;
            }

            Trace.WriteInfo(TraceType, "Processing management notification further since state transition is allowed. Notification details: {0}", NotificationDetailsToString(notification));

            if (notification.IsManualApprovalRequired())
            {
                // whatever jobstate we may be in, we don't progress further unless operator/admin manually approves this notification
                Trace.WriteWarning(TraceType, "Notification requires manual approval. Notification details: {0}",
                    NotificationDetailsToString(notification));

                UpdateNotificationApprovalHealthStatus(
                    HealthState.Warning,
                    string.Format(
                        CultureInfo.InvariantCulture,
                        "Please check current management notification '{0}' as it requires manual approval by admin/operator",
                        notification.NotificationId)); // notification cannot be null inside this if block

                return;
            }

            UpdateNotificationApprovalHealthStatus(
                HealthState.Ok,
                notification == null
                    ? "No management notification currently"
                    : string.Format(CultureInfo.InvariantCulture, "Current management notification '{0}' approved for further processing", notification.NotificationId));

            this.ProcessRepairs(notification, jobList);

            // Based on the last known CM task state + the current MR notification, decide what to tell the CM next
            switch (this.lastKnownJobState)
            {
                case JobState.Idle:
                    IdleJobStateAction(notification);
                    break;

                case JobState.WaitingForApproval:
                case JobState.Executing:

                    // If IS has failed over thus resetting jobImpactTranslationMode from Optimized to Default, then this if block won't
                    // match, but the IsJobExecutionComplete will and the task will be run to completion by CM with no actual SignalReady's 
                    // being sent.
                    if (this.NotificationMatchesExpectedState(notification, NotificationType.StartJobStep, jobImpactTranslationMode))
                    {
                        // Repeat the start request for the last known task
                        // This also has the effect of handling the "lost pre ack" issue in the WA FC
                        this.StartInfrastructureTask(notification, this.lastKnownTask.InstanceId);
                    }
                    else if (IsJobExecutionComplete(notification))
                    {
                        // Execution completed normally or cancelled
                        // TODO: Dampen cancellation by looking at this.IsLastKnownJobStateRecentlyUpdated()
                        if (!this.StopAtJobExecutionCompletedBreakpoint(this.lastKnownTask.InstanceId))
                        {
                            this.FinishInfrastructureTask();
                        }
                    }
                    else
                    {
                        // Something else unexpected. E.g. the FC appears to change the impact list after pre ack
                        // when the job contains RemoveInstance impacts. Those role instances disappear from
                        // the impact list as soon as the job moves to Executing. This should not be treated
                        // as job cancellation/preemption; the notification should instead be ignored.
                        Trace.WriteWarning(TraceType,
                            "Unexpected notification; job impact changed or status is not waiting for tenant acknowledgment: {0}",
                            NotificationDetailsToString(notification));
                    }
                    break;

                case JobState.WaitingForHealthCheck:

                    // Repeat the finish request for the last known task
                    // ReportFinishTaskSuccess or ReportTaskFailure will cause an eventual transition back to Idle
                    this.FinishInfrastructureTask();
                    break;
            }

            Trace.WriteInfo(TraceType, "ProcessManagementNotification completed");
        }

        /// <summary>
        /// Observes the current incarnation number from the MR channel, and notes
        /// the timestamp of changes. The first such change marks the MR notification
        /// as available, if that has not already been done.
        /// </summary>
        private void ProcessCurrentIncarnation()
        {
            ulong? healthIncarnation = this.ManagementClient.GetServiceHealthIncarnation();

            if (healthIncarnation == null)
            {
                if (this.lastHealthIncarnationUpdatedAt == null)
                {
                    TraceType.WriteInfo("No health incarnation found");

                    // CheckChannelHealth clock starts now, to catch the case
                    // where the incarnation never changes from null.
                    this.lastHealthIncarnationUpdatedAt = DateTimeOffset.UtcNow;
                }
            }
            else
            {
                if (this.lastHealthIncarnation == null)
                {
                    TraceType.WriteInfo("First observed health incarnation: {0}", healthIncarnation);

                    this.lastHealthIncarnation = healthIncarnation.Value;
                    this.lastHealthIncarnationUpdatedAt = DateTimeOffset.UtcNow;
                }
                else if (this.lastHealthIncarnation.Value != healthIncarnation.Value)
                {
                    TraceType.WriteInfo("Health incarnation changed: {0} -> {1}", this.lastHealthIncarnation, healthIncarnation);

                    this.lastHealthIncarnation = healthIncarnation.Value;
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
        /// <returns>false if the channel is unhealthy, true otherwise.</returns>
        private bool CheckChannelHealth()
        {
            TimeSpan maxIncarnationUpdateWaitTime = GetPollingInterval(
                ConfigMaxIncarnationUpdateWaitTimeKeyName,
                defaultValue: 1800,
                maxValue: TimeSpan.MaxValue);

            if ((maxIncarnationUpdateWaitTime > TimeSpan.Zero) && (this.lastHealthIncarnationUpdatedAt != null))
            {
                TimeSpan elapsedSinceLastUpdate = DateTimeOffset.UtcNow - this.lastHealthIncarnationUpdatedAt.Value;

                if (elapsedSinceLastUpdate > maxIncarnationUpdateWaitTime)
                {
                    TraceType.WriteWarning(
                        "Health incarnation ({0}) last changed at {1:O} ({2} ago). Management channel may be down on this node; exiting process to cause failover.",
                        this.lastHealthIncarnation,
                        this.lastHealthIncarnationUpdatedAt,
                        elapsedSinceLastUpdate);

                    ProcessCloser.ExitEvent.Set();
                    return false;
                }
            }

            return true;
        }

        private void IdleJobStateAction(IManagementNotificationContext notification)
        {
            if (notification == null)
            {
                // No active MR job
                if (this.lastKnownTask == null)
                {
                    // Task does not exist, just query again
                    this.RecoverState();
                }
                else
                {
                    // Repeat the finish request for the last known task
                    this.FinishInfrastructureTask();
                }
            }
            else if (notification.NotificationType == NotificationType.StartJobStep)
            {
                // Start a new job
                long instanceId = this.IssueSequenceNumber();
                this.StartInfrastructureTask(notification, instanceId);
            }
            else
            {
                ReAckNotification(notification);
            }
        }

        /// <summary>
        /// Returns true if the current notification indicates that the job is no longer executing,
        /// either because no job is active, or because the notification is for a CompleteJobStep,
        /// or because the FC is waiting for acknowledgement (presumably of the next job).
        /// </summary>
        private static bool IsJobExecutionComplete(IManagementNotificationContext notification)
        {
            return
                notification == null ||
                notification.NotificationType == NotificationType.CompleteJobStep ||
                notification.ActiveJobDetailedStatus == ManagementJobDetailedStatus.WaitingForStartStepAcknowledgement;
        }

        /// <summary>
        /// Many times, Fabric Controller (FC) loses (or doesn't receive) the CompleteJobStep notification that IS sends it. 
        /// In that case, it resends the notification. Now, we check if the notification qualifies and re-ack it in a similar fashion.
        /// </summary>
        /// <param name="notification">The notification to be re-acked (i.e. re-sent to FC)</param>
        private void ReAckNotification(IManagementNotificationContext notification)
        {
            if (notification.NotificationType == NotificationType.CompleteJobStep && this.AlwaysAckPostNotifications())
            {
                TraceType.WriteInfo("AlwaysAckPostNotifications is enabled; acknowledging CompleteJobStep notification");
                SignalReady(notification);
                return;
            }

            bool allowReAck = ReadConfigValue(ConfigAllowCompletedJobStepReAckKeyName, DefaultAllowCompletedJobStepReAck);

            if (allowReAck)
            {
                if (!lastCompletedJobStepInfo.QualifiesForReAck(notification))
                {
                    Trace.WriteWarning(TraceType,
                        "Unexpected post-UD notification. This may require manual approval if notification persists beyond {0}. Notification: {1}",
                        lastCompletedJobStepInfo.UpdateThreshold,
                        NotificationDetailsToString(notification));
                    return;
                }

                switch (lastCompletedJobStepInfo.Action)
                {
                    case NotificationAction.SignalReady:
                        SignalReady(notification);
                        break;

                    case NotificationAction.SignalError:
                        SignalError(notification, lastCompletedJobStepInfo.Detail);
                        break;
                }

                lastCompletedJobStepInfo.Update(
                    notification.ActiveJobId,
                    notification.ActiveJobStepTargetUD,
                    notification.NotificationType,
                    lastCompletedJobStepInfo.Action,
                    lastCompletedJobStepInfo.Detail);

                Trace.WriteWarning(
                    TraceType,
                    "Post-UD notification while no task is active. Re-acked this notification with similar action as before. Notification: {0}, previous action taken: {1}, previous description: {2}",
                    NotificationDetailsToString(notification),
                    lastCompletedJobStepInfo.Action,
                    lastCompletedJobStepInfo.Detail ?? "<null>");

            }
            else if (this.IsLastKnownJobStateRecentlyUpdated())
            {
                // Post-UD notifications take a while to disappear; don't complain if the last job finished recently
                Trace.WriteWarning(
                    TraceType,
                    "Post-UD notification while no task is active. Manual approval may be required if notification persists beyond {0}. Notification: {1}", 
                    lastCompletedJobStepInfo.UpdateThreshold,
                    NotificationDetailsToString(notification));
            }
            else
            {
                // Unexpected Post notification, possibly from a previously-alerted job
                Trace.WriteWarning(
                    TraceType,
                    "Unexpected post-UD notification while no task is active. Manual approval may be required. Notification: {0}", 
                    NotificationDetailsToString(notification));
            }
        }

        private bool IsRepairExecutionEnabled()
        {
            // Execution disabled by default in v2.2 because Repair Manager is disabled by default
            bool isEnabled = this.ReadConfigValue<bool>(ConfigEnableRepairExecutionKeyName);
            return isEnabled;
        }

        private static bool IsJobPending(IManagementJobWrapper job)
        {
            switch (job.DetailedStatus)
            {
                case ManagementJobDetailedStatus.Completed:
                case ManagementJobDetailedStatus.Failed:
                case ManagementJobDetailedStatus.Suspended:
                case ManagementJobDetailedStatus.Alerted:
                case ManagementJobDetailedStatus.Cancelled:
                    return false;
            }

            return true;
        }

        private void ProcessRepairs(IManagementNotificationContext notification, IList<IManagementJobWrapper> jobList)
        {
            try
            {
                if (!this.IsRepairExecutionEnabled())
                {
                    Trace.WriteInfo(TraceType, "Repair execution is disabled");
                    return;
                }

                if (this.repairExecutor == null)
                {
                    this.repairExecutor = new WindowsAzureRepairExecutor(this, this.ConfigSectionName, this.ManagementClient);
                }

                // Completed jobs stick around in the job list now; exclude those from the count.
                int pendingJobCount = jobList.Count(j => IsJobPending(j));

                // To avoid creating a deep queue, new repairs should only be started if nothing else is happening.
                // Determine that by looking directly at the job list, but eventually time out and queue a repair.
                bool canExecuteNewRepairs =
                    (this.lastKnownJobState == JobState.Idle) &&
                    (pendingJobCount == 0 || !this.IsLastKnownJobStateRecentlyUpdated());

                this.repairExecutor.ProcessRepairs(canExecuteNewRepairs, notification);
            }
            catch (Exception e)
            {
                Trace.WriteWarning(TraceType, "ProcessRepairs failed: {0}", e);
            }
        }

        #endregion

        #region RunCommandAsync handling

        private void RegisterCommandHandlers()
        {
            // Admin commands
            RegisterCommand("SignalReady", true, WrapSyncVoidCommand(this.HandleCommandSignalReady));
            RegisterCommand("SignalError", true, WrapSyncVoidCommand(this.HandleCommandSignalError));
            RegisterCommand("ProcessCurrentNotification", true, WrapSyncVoidCommand(this.HandleCommandProcessCurrentNotification));
            RegisterCommand("FailFast", true, WrapSyncVoidCommand(this.HandleCommandFailFast));
            RegisterCommand("UpdateJobBlockingPolicy", true, this.HandleUpdateJobBlockingPolicy);
            RegisterCommand("RequestMaintenance", true, WrapSyncVoidCommand(this.HandleCommandRequestMaintenance));
            RegisterCommand("RequestResume", true, WrapSyncVoidCommand(this.HandleCommandRequestResume));
            RegisterCommand("RequestSuspend", true, WrapSyncVoidCommand(this.HandleCommandRequestSuspend));
            RegisterCommand("BreakpointContinue", true, WrapSyncVoidCommand(this.HandleCommandBreakpointContinue));

            // User commands
            RegisterCommand("GetCurrentState", false, WrapSyncCommand(this.HandleCommandGetCurrentState));
            RegisterCommand("GetJobs", false, WrapSyncCommand(this.HandleCommandGetJobs));
            RegisterCommand("GetRoleInstances", false, WrapSyncCommand(this.HandleCommandGetRoleInstances));            
        }

        private void RegisterCommand(string commandName, bool isAdminCommand, AsyncCommandHandler commandHandler)
        {
            var entry = new CommandHandlerEntry(isAdminCommand, commandHandler);
            this.commandHandlerMap.Add(commandName, entry);
        }

        private Task<string> DispatchCommand(string input, bool allowAdminCommands)
        {
            if (input == null)
                return TaskUtility.ThrowAsync(new ArgumentNullException("input"));

            Trace.WriteInfo(TraceType, "DispatchCommand: {0}", input);

            // Command format is "CommandName:Arguments"
            string[] tokens = input.Split(CommandTokenDelimiter, 2);
            string commandName = tokens[0];

            string args = String.Empty;
            if (tokens.Length > 1)
            {
                args = tokens[1];
            }

            try
            {
                CommandHandlerEntry handlerEntry;
                if (commandHandlerMap.TryGetValue(commandName, out handlerEntry))
                {
                    if (allowAdminCommands || !handlerEntry.IsAdminCommand)
                    {
                        return handlerEntry.CommandHandler(args);
                    }
                    else
                    {
                        Trace.WriteWarning(TraceType, "Permission denied for command: {0}", commandName);
                        return TaskUtility.ThrowAsync(new ArgumentException("Unrecognized command name"));
                    }
                }
                else
                {
                    Trace.WriteWarning(TraceType, "Unrecognized command name: {0}", commandName);
                    return TaskUtility.ThrowAsync(new ArgumentException("Unrecognized command name"));
                }
            }
            catch (Exception e)
            {
                Trace.WriteWarning(TraceType, "Command handler for {0} threw exception: {1}", commandName, e);
                throw;
            }
        }

        private Task<string> HandleUpdateJobBlockingPolicy(string args)
        {
            if (args == null)
                return TaskUtility.ThrowAsync(new ArgumentException("Invalid command arguments"));

            JobBlockingPolicy newBlockingPolicy;
            try
            {
                 newBlockingPolicy = (JobBlockingPolicy)Enum.Parse(typeof(JobBlockingPolicy), args);
            }
            catch (Exception e)
            {
                Trace.WriteWarning(TraceType, "Invalid blocking policy specified: Policy - {0}, exception - {1}", args, e.Message);
                return TaskUtility.ThrowAsync(new ArgumentException("Invalid blocking policy specified"));
            }

            Trace.WriteInfo(TraceType, "UpdateJobBlockingPolicy: new policy {0} requested", newBlockingPolicy);

            return jobBlockingPolicyManager.UpdatePolicyAsync(newBlockingPolicy).Then<string>(_ => null);
        }

        private void HandleCommandSignalReady(string args)
        {
            string[] tokens = args.Split(CommandTokenDelimiter);
            if (tokens.Length != 2)
                throw new ArgumentException("Invalid command arguments");

            string jobId = tokens[0];

            int upgradeDomain;
            if (!Int32.TryParse(tokens[1], out upgradeDomain))
                throw new ArgumentException("Invalid command arguments");

            this.OnRunSignalReady(jobId, upgradeDomain);
        }

        private void HandleCommandSignalError(string args)
        {
            string[] tokens = args.Split(CommandTokenDelimiter, 3);
            if (tokens.Length != 3)
                throw new ArgumentException("Invalid command arguments");

            string jobId = tokens[0];

            int upgradeDomain;
            if (!Int32.TryParse(tokens[1], out upgradeDomain))
                throw new ArgumentException("Invalid command arguments");

            string errorDescription = tokens[2];

            this.OnRunSignalError(jobId, upgradeDomain, errorDescription);
        }

        private void HandleCommandProcessCurrentNotification(string args)
        {
            this.OnRunProcessCurrentNotification(false);
        }

        private string HandleCommandGetCurrentState(string args)
        {
            CoordinatorStateData currentState;
            lock (this.stateLock)
            {
                currentState = GetCurrentStateData();
            }

            MemoryStream stream = new MemoryStream();
            DataContractJsonSerializer jsonSerializer = new DataContractJsonSerializer(typeof(CoordinatorStateData));
            jsonSerializer.WriteObject(stream, currentState);

            stream.Position = 0;

            StreamReader reader = new StreamReader(stream);
            return reader.ReadToEnd();
        }

        private void HandleCommandFailFast(string args)
        {
            // Delay slightly to give the process some time to acknowledge the client request
            TimeSpan delay = TimeSpan.FromSeconds(1);
            Trace.WriteWarning(TraceType, "Received FailFast command; terminating process after {0} delay", delay);
            TaskUtility.SleepAsync(delay).ContinueWith(_ => FailFast());
        }

        // RequestMaintenance:<RoleInstanceName>:<InfrastructureServiceMaintenanceAction>[:<Reason>]
        private void HandleCommandRequestMaintenance(string args)
        {
            Trace.WriteInfo(TraceType, "Requesting maintenance. Input: {0}", args);

            string[] tokens = args.Split(CommandTokenDelimiter, 3);
            if (tokens.Length < 2)
            {
                throw new ArgumentException("Invalid command arguments");
            }

            string roleInstanceName = tokens[0];

            InfrastructureServiceMaintenanceAction action;
            if (!Enum.TryParse(tokens[1], out action))
            {
                throw new ArgumentException("Invalid command arguments");
            }

            string reason = null;
            if (tokens.Length >= 3)
            {
                reason = tokens[2];
            }

            string contextId = null;
            if (tokens.Length >= 4)
            {
                contextId = tokens[3];
            }

            try
            {
                this.ManagementClient.RequestMaintenance(roleInstanceName, action, reason, contextId);

                Trace.WriteInfo(
                    TraceType,
                    "Successfully requested maintenance of role instance: {0}, action: {1}, reason: {2}, contextId: {3}",
                    roleInstanceName,
                    action,
                    reason,
                    contextId);
            }
            catch (KeyNotFoundException)
            {
                throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "Role instance '{0}' not found",
                    roleInstanceName));
            }
            catch (ManagementException)
            {
                Trace.WriteWarning(
                    TraceType,
                    "Error in management protocol while requesting maintenance of role instance: {0}, action: {1}, reason: {2}, contextId: {3}",
                    roleInstanceName,
                    action,
                    reason,
                    contextId);

                throw;
            }
        }

        /// <summary>
        /// Handles the command to request suspension of an Azure job.
        /// </summary>
        /// <param name="args">
        /// The arguments passed in from the client (e.g. a user via the PowerShell cmdlet).
        /// </param>
        /// <example>
        /// RequestSuspend:4aa9103c-3946-41c5-a4cd-2f0d03882a1a
        /// where the Guid represents the job id that is created by Azure.
        /// </example>
        private void HandleCommandRequestSuspend(string args)
        {
            Trace.WriteInfo(TraceType, "Requesting suspension of job. Input: {0}", args);

            var job = GetJobFromId(args);
            try
            {
                job.RequestSuspend();
            }
            catch (ManagementException)
            {
                Trace.WriteWarning(TraceType, "Error in management protocol while requesting suspension of job: {0}", job);
                throw;
            }

            Trace.WriteInfo(
                TraceType,
                "Successfully requested suspension of job: {0} with action: RequestSuspend",
                job.Id);
        }

        /// <summary>
        /// Handles the command to continue past a breakpoint
        /// </summary>
        /// <example>
        /// BreakpointContinue:BlockNewImpactfulUpdateJobs:4aa9103c-3946-41c5-a4cd-2f0d03882a1a:5
        /// where the argument to BreakpointContinue is the LastHitBreakpoint value returned from GetCurrentState
        /// </example>
        private void HandleCommandBreakpointContinue(string args)
        {
            lock (this.stateLock)
            {
                this.breakpointManager.SetBreakpointContinue(args);
            }
        }

        /// <summary>
        /// Handles the command to request resumption of a suspended Azure job
        /// </summary>
        /// <param name="args">
        /// The arguments passed in from the client (e.g. a user via the PowerShell cmdlet).
        /// </param>
        /// <example>
        /// RequestResume:4aa9103c-3946-41c5-a4cd-2f0d03882a1a
        /// where the Guid represents the job id that is created by Azure.
        /// </example>
        private void HandleCommandRequestResume(string args)
        {
            Trace.WriteInfo(TraceType, "Requesting resumption of job. Input: {0}", args);

            var job = GetJobFromId(args);
            try
            {
                job.RequestResume();
            }
            catch (ManagementException)
            {
                Trace.WriteWarning(TraceType, "Error in management protocol while requesting resumption of job: {0}", job);
                throw;
            }

            Trace.WriteInfo(
                TraceType,
                "Successfully requested resumption of job: {0} with action: RequestResume",
                job.Id);
        }

        /// <summary>
        /// Handles the command to get all jobs from the MR SDK.
        /// </summary>
        /// <param name="args">
        /// The arguments passed in from the client (e.g. a user via the PowerShell cmdlet).
        /// This parameter is currently not used.
        /// </param>
        /// <returns>A JSON serialized object that is then sent to the client querying for jobs.</returns>
        private string HandleCommandGetJobs(string args)
        {
            Trace.WriteInfo(TraceType, "GetJobs requested");

            IList<IManagementJobWrapper> wrappedJobs = ManagementClient.GetAllJobs();

            Trace.WriteInfo(TraceType, "Jobs returned from MR SDK: {0}", wrappedJobs.Count);

            var jobCollectionData = new JobCollectionData
            {
                Jobs = wrappedJobs
                    .Select(
                        e =>
                            new JobData
                            {
                                Id = e.Id,
                                Status = e.Status.ToString(),
                                Type = e.Type.ToString(),
                                DetailedStatus = e.DetailedStatus.ToString(),
                                ContextId = e.ContextId
                            })
                    .ToArray()
            };

            var stream = new MemoryStream();
            var jsonSerializer = new DataContractJsonSerializer(typeof(JobCollectionData));
            jsonSerializer.WriteObject(stream, jobCollectionData);

            stream.Position = 0;

            var reader = new StreamReader(stream);
            return reader.ReadToEnd();
        }

        /// <summary>
        /// Handles the command to get all role instances from the MR SDK.
        /// </summary>
        /// <param name="args">
        /// The arguments passed in from the client (e.g. a user via the PowerShell cmdlet).
        /// This parameter is currently not used.
        /// </param>
        /// <returns>A JSON serialized object that is then sent to the client querying for role instances.</returns>
        private string HandleCommandGetRoleInstances(string args)
        {
            Trace.WriteInfo(TraceType, "GetRoleInstances requested");

            IList<RoleInstance> roleInstances = ManagementClient.GetRoleInstances();

            Trace.WriteInfo(TraceType, "Role instances returned from MR SDK: {0}", roleInstances.Count);

            var roleInstanceCollectionData = new RoleInstanceCollectionData
            {
                RoleInstances = roleInstances
                    .Select(
                        e =>
                            new RoleInstanceData
                            {
                                Id = e.Id,
                                Status = e.Status.ToString(),
                                LastUpdateTime = e.LastUpdateTime
                            })
                    .ToArray()
            };

            var stream = new MemoryStream();
            var jsonSerializer = new DataContractJsonSerializer(typeof(RoleInstanceCollectionData));
            jsonSerializer.WriteObject(stream, roleInstanceCollectionData);

            stream.Position = 0;

            var reader = new StreamReader(stream);
            return reader.ReadToEnd();
        }

        private static void FailFast()
        {
            Trace.WriteWarning(TraceType, "Terminating process due to FailFast client command");
            Environment.FailFast("FailFast requested by client command");
        }

        private void OnRunSignalReady(string jobId, int upgradeDomain)
        {
            IManagementNotificationContext notification = this.ManagementClient.GetCurrentNotification();

            Trace.WriteInfo(TraceType, "SignalReady:{0}:{1} requested; current notification: {2}",
                jobId,
                upgradeDomain,
                NotificationToShortDisplayString(notification));

            if (notification.Matches(jobId, upgradeDomain))
            {
                SignalReady(notification);

                if (notification.NotificationType == NotificationType.CompleteJobStep)
                {
                    lastCompletedJobStepInfo.Update(
                        notification.ActiveJobId,
                        notification.ActiveJobStepTargetUD,
                        notification.NotificationType,
                        NotificationAction.SignalReady);
                }

                Trace.WriteInfo(TraceType, "SignalReady:{0}:{1} completed", jobId, upgradeDomain);
            }
            else
            {
                Trace.WriteWarning(TraceType, "SignalReady request does not match current notification; nothing to do");
            }
        }

        private void OnRunSignalError(string jobId, int upgradeDomain, string errorDescription)
        {
            IManagementNotificationContext notification = this.ManagementClient.GetCurrentNotification();

            Trace.WriteInfo(TraceType, "SignalError:{0}:{1} requested; current notification: {2}",
                jobId,
                upgradeDomain,
                NotificationToShortDisplayString(notification));

            if (notification.Matches(jobId, upgradeDomain))
            {
                SignalError(notification, errorDescription);

                if (notification.NotificationType == NotificationType.CompleteJobStep)
                {
                    lastCompletedJobStepInfo.Update(
                        notification.ActiveJobId, 
                        notification.ActiveJobStepTargetUD, 
                        notification.NotificationType, 
                        NotificationAction.SignalError,
                        errorDescription);
                }

                Trace.WriteInfo(TraceType, "SignalError:{0}:{1} completed", jobId, upgradeDomain);
            }
            else
            {
                Trace.WriteWarning(TraceType, "SignalError request does not match current notification; nothing to do");
            }
        }

        private IManagementJobWrapper GetJobFromId(string args)
        {
            string[] tokens = args.Split(CommandTokenDelimiter);
            if (tokens.Length != 1)
                throw new ArgumentException("Invalid command arguments");

            string jobId = tokens[0];

            var jobWrapper = ManagementClient
                .GetAllJobs()
                .FirstOrDefault(e => e.Id.Equals(jobId, StringComparison.OrdinalIgnoreCase));

            if (jobWrapper != null)
            {
                Trace.WriteNoise(TraceType, "Job Id: {0} found by MR SDK", jobId);
                return jobWrapper;
            }

            string message = string.Format(CultureInfo.InvariantCulture, "Job Id: {0} not found by MR SDK", jobId);
            Trace.WriteWarning(TraceType, message);
            throw new ArgumentException(message);
        }

        #endregion

        #region Internal state management

        /// <summary>
        /// Reads the configuration value.
        /// </summary>
        /// <typeparam name="T">The type of the converted result.</typeparam>
        /// <param name="keyName">Name of the key.</param>
        /// <param name="defaultValue">The default value to be used if read was not successful.</param>
        /// <returns>The configuration value or the provided default value.</returns>
        public T ReadConfigValue<T>(string keyName, T defaultValue = default(T))
        {
            keyName.ThrowIfNullOrWhiteSpace("keyName");

            T result = defaultValue;

            string configValue = this.ConfigStore.ReadUnencryptedString(this.ConfigSectionName, keyName);

            if (String.IsNullOrEmpty(configValue))
            {
                Trace.WriteNoise(
                    TraceType,
                    "Config section/key name '{0}/{1}' was either not found or its value was empty. Returning default value: {2}",
                    this.ConfigSectionName,
                    keyName,
                    result);
            }
            else
            {
                Exception ex = null;

                try
                {
                    if (typeof(T).GetTypeInfo().IsEnum)
                    {
                        result = (T)Enum.Parse(typeof(T), configValue, true);
                    }
                    else
                    {
                        result = (T)Convert.ChangeType(configValue, typeof(T), CultureInfo.InvariantCulture);
                    }
                }
                catch (ArgumentException e)
                {
                    ex = e;
                }
                catch (InvalidCastException e)
                {
                    ex = e;
                }
                catch (FormatException e)
                {
                    ex = e;
                }
                catch (OverflowException e)
                {
                    ex = e;
                }

                if (ex != null)
                {
                    Trace.WriteWarning(
                        TraceType,
                        "Error converting value '{0}' from config section/key name '{1}/{2}'. Returning default value: {3}. Error details: {4}",
                        configValue,
                        this.ConfigSectionName,
                        keyName,
                        result,
                        ex);
                }
                else
                {
                    Trace.WriteNoise(
                        TraceType,
                        "Converted value '{0}' from config section/key name '{1}/{2}'. Returning converted value: {3}",
                        configValue,
                        this.ConfigSectionName,
                        keyName,
                        result);
                }
            }

            return result;
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
            int pollingInterval = ReadConfigValue(keyName, defaultValue);

            var interval = TimeSpan.FromSeconds(Math.Min(pollingInterval, maxValue.TotalSeconds));
            return interval;
        }

        private void ScheduleTimer(Timer timer, string timerName, TimeSpan timerPeriod, ref bool isScheduled, bool isRescheduling)
        {
            lock (this.stateLock)
            {
                if (!isRescheduling && isScheduled)
                {
                    // Timer is already scheduled, and caller is not the timer callback.
                    // So do not reschedule the timer, because the callback will take care of that.
                    return;
                }

                if (this.runCancellationToken.IsCancellationRequested)
                {
                    Trace.WriteInfo(
                        TraceType,
                        "Not scheduling timer '{0}' callback because coordinator is stopping",
                        timerName);

                    isScheduled = false;
                }
                else if (timerPeriod > TimeSpan.Zero)
                {
                    Trace.WriteInfo(
                        TraceType,
                        "Scheduling next notification check for timer '{0}' {1:O} (period = {2})",
                        timerName,
                        DateTime.UtcNow + timerPeriod,
                        timerPeriod);

                    timer.Change(timerPeriod, TimeSpanInfinite);
                    isScheduled = true;
                }
                else
                {
                    Trace.WriteInfo(
                        TraceType,
                        "Not scheduling timer '{0}' callback because timer is disabled",
                        timerName);

                    isScheduled = false;
                }
            }
        }

        private void ScheduleJobPollingTimer(bool isRescheduling)
        {
            TimeSpan timerPeriod = this.GetPollingInterval(
                ConfigJobPollingIntervalInSecondsKeyName,
                DefaultJobPollingIntervalInSeconds,
                MaxJobPollingInterval);

            ScheduleTimer(
                jobRefreshTimer,
                "JobRefresh",
                timerPeriod,
                ref this.jobRefreshTimerIsScheduled,
                isRescheduling);
        }

        private void ScheduleHealthPollingTimer(bool isRescheduling)
        {
            TimeSpan timerPeriod = this.GetPollingInterval(
                ConfigHealthPollingIntervalInSecondsKeyName,
                DefaultHealthPollingIntervalInSeconds,
                MaxHealthPollingInterval);

            ScheduleTimer(
                healthRefreshTimer,
                "HealthRefresh",
                timerPeriod,
                ref this.healthRefreshTimerIsScheduled,
                isRescheduling);
        }

        /// <summary>
        /// Returns the next sequence number that will be issued by <see cref="IssueSequenceNumber"/>.
        /// </summary>
        private long PeekNextSequenceNumber()
        {
            // 64-bit sequence number is formed from:
            // - High 32 bits = primary epoch
            // - Low 32 bits = local sequence number
            long sequenceNumber = this.primaryEpoch;
            sequenceNumber <<= 32;
            sequenceNumber |= (long)this.nextSequenceNumber;
            return sequenceNumber;
        }

        /// <summary>
        /// Issues the next sequence number, guaranteed to be monotonically increasing.
        /// </summary>
        private long IssueSequenceNumber()
        {
            long sequenceNumber = this.PeekNextSequenceNumber();

            // Increment the local sequence number
            this.nextSequenceNumber += SequenceNumberStride;

            Trace.WriteInfo(TraceType, "Issuing sequence number 0x{0:X}", sequenceNumber);
            return sequenceNumber;
        }

        private void VerifyTaskId(string taskId)
        {
            if (!String.Equals(this.taskId, taskId, StringComparison.Ordinal))
            {
                throw Trace.CreateException(
                    TraceType,
                    NativeTypes.FABRIC_ERROR_CODE.E_INVALIDARG,
                    "Invalid task ID '{0}', expected '{1}'",
                    taskId,
                    this.taskId);
            }
        }

        private void VerifyIsRunning()
        {
            if (this.runTaskCompletionSource == null)
            {
                throw Trace.CreateException(
                    TraceType,
                    NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NOT_PRIMARY,
                    "Coordinator is not running");
            }
        }

        private void VerifyKnownInstanceId(long instanceId)
        {
            if (this.lastKnownTask == null || this.lastKnownTask.InstanceId < instanceId)
            {
                // Unexpected larger instance ID.  At worst, this should be resolved by the callback generated by the
                // next run of ProcessManagementNotification ending up in RecoverState.
                // Cannot call RecoverState directly here because there may already be an outstanding call to the CM.
                // Just nack this message.
                throw Trace.CreateException(
                    TraceType,
                    NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NOT_READY,
                    "Not ready to process instance ID 0x{0:X} yet",
                    instanceId);
            }
        }

        private void BeginClusterManagerAsyncTask(Task task)
        {
            Debug.Assert(this.activeClusterManagerTask == null);
            this.activeClusterManagerTask = task;
        }

        private void EndClusterManagerAsyncTask()
        {
            Debug.Assert(this.activeClusterManagerTask != null);
            this.activeClusterManagerTask = null;
        }

        private bool IsClusterManagerAsyncTaskRunning()
        {
            return (this.activeClusterManagerTask != null);
        }

        private bool IsStaleInstanceId(long instanceId)
        {
            return (this.lastKnownTask != null && instanceId < this.lastKnownTask.InstanceId);
        }

        private void SetLastKnownJobState(JobState newState, InfrastructureTaskDescription newTaskDescription, IManagementNotificationContext notification)
        {
            this.SetLastKnownJobStateCore(newState, notification);
            this.lastKnownTask = newTaskDescription;
        }

        private void SetLastKnownJobState(JobState newState)
        {
            this.SetLastKnownJobStateCore(newState, null);
        }

        private void SetLastKnownJobStateCore(JobState newState, IManagementNotificationContext notification)
        {
            if (this.lastKnownJobState != newState)
            {
                Trace.WriteInfo(TraceType, "Updating last known job state: {0} -> {1}{2}",
                    this.lastKnownJobState,
                    newState,
                    notification == null ? string.Empty : Environment.NewLine + NotificationDetailsToString(notification));
                this.lastKnownJobState = newState;
                this.lastKnownJobStateUpdateTime = DateTime.UtcNow;
            }
        }

        private bool IsLastKnownJobStateRecentlyUpdated()
        {
            return (DateTime.UtcNow - this.lastKnownJobStateUpdateTime < this.recentJobStateUpdateThreshold);
        }

        private void DumpLastKnownTaskState()
        {
            Trace.WriteInfo(TraceType, "Last known job state: {0},{1}",
                this.lastKnownTask == null ? "<null>" : String.Format(CultureInfo.InvariantCulture, "0x{0:X}", this.lastKnownTask.InstanceId),
                this.lastKnownJobState);

            if (this.lastKnownJobState != JobState.Idle)
            {
                DumpTaskDescription(this.lastKnownTask);
            }
        }

        private CoordinatorStateData GetCurrentStateData()
        {
            IManagementNotificationContext notification = this.lastNotification;
            ManagementNotificationData notificationData = null;
            if (notification != null)
            {
                notificationData = new ManagementNotificationData
                {
                    NotificationId = notification.NotificationId,
                    NotificationStatus = notification.NotificationStatus.ToString(),
                    NotificationType = notification.NotificationType.ToString(),
                    NotificationDeadline = notification.NotificationDeadline,
                    ActiveJobStepId = notification.ActiveJobStepId,
                    ActiveJobId = notification.ActiveJobId,
                    ActiveJobType = notification.ActiveJobType.ToString(),
                    ActiveJobIncludesTopologyChange = notification.ActiveJobIncludesTopologyChange,
                    ActiveJobStepTargetUD = notification.ActiveJobStepTargetUD,
                    JobStepImpact = ImpactedInstancesToString(notification.ImpactedInstances),
                    Incarnation = notification.Incarnation,
                    ActiveJobDetailedStatus = notification.ActiveJobDetailedStatus.ToString(),
                    ActiveJobContextId = notification.ActiveJobContextId,
                    ActiveJobStepStatus = notification.ActiveJobStepStatus.ToString(),
                };
            }

            InfrastructureTaskDescription task = this.lastKnownTask;
            InfrastructureTaskStateData taskData = null;
            if (task != null)
            {
                var taskDescription = task.NodeTasks.Select(
                    t => new NodeTaskDescriptionData() { NodeName = t.NodeName, TaskType = t.TaskType.ToString() }).ToArray();

                taskData = new InfrastructureTaskStateData()
                {
                    TaskInstanceId = task.InstanceId,
                    TaskDescription = taskDescription,
                };
            }

            var coordinatorData = new CoordinatorStateData()
            {
                InfrastructureTaskId = this.taskId,
                JobBlockingPolicy = this.jobBlockingPolicyManager.Policy.ToString(),
                LastKnownJobState = this.lastKnownJobState.ToString(),
                LastKnownTask = taskData,
                ManagementNotification = notificationData,
                IsManagementNotificationAvailable = this.isNotificationAvailable,
                NodesImpactedCount = this.nodesImpactedCount,
            };

            if (this.breakpointManager != null)
            {
                coordinatorData.LastHitBreakpoint = this.breakpointManager.LastHitBreakpoint;
                coordinatorData.ContinuePastBreakpoint = this.breakpointManager.ContinuePastBreakpoint;
            }

            return coordinatorData;
        }

        private InfrastructureTaskResultItem FindTask(InfrastructureTaskQueryResult queryResult)
        {
            if (queryResult == null)
            {
                return null;
            }

            InfrastructureTaskResultItem findResult = null;

            // Log each query result and locate the one (if any) with the task ID matching this coordinator
            foreach (var taskResultItem in queryResult.Items)
            {
                bool isFound = String.Equals(taskResultItem.Description.TaskId, this.taskId, StringComparison.Ordinal);

                Trace.WriteInfo(TraceType, "Infrastructure task: TaskId = '{0}', InstanceId = 0x{1:X}, State = {2}, {3}",
                    taskResultItem.Description.TaskId,
                    taskResultItem.Description.InstanceId,
                    taskResultItem.State,
                    isFound ? "Managed by this service" : "Not managed by this service");

                if (isFound)
                {
                    Debug.Assert(findResult == null); // only one should match
                    findResult = taskResultItem;
                }
            }

            return findResult;
        }

        private bool NotificationMatchesExpectedStateInCompleteJobStep(IManagementNotificationContext notification)
        {
            bool matches =
                this.NotificationMatchesExpectedState(notification, NotificationType.CompleteJobStep, JobImpactTranslationMode.Default);

            if (!matches && IsJobEligibleForLearningModeOptimization(notification))
            {
                matches =
                    this.NotificationMatchesExpectedState(notification, NotificationType.CompleteJobStep, JobImpactTranslationMode.Optimized);
            }

            return matches;
        }

        private bool NotificationMatchesExpectedState(IManagementNotificationContext notification, NotificationType expectedType, JobImpactTranslationMode jobImpactModeToMatchAgainst)
        {
            if (notification == null)
                return false;

            if (notification.NotificationType != expectedType)
                return false;

            return NotificationMatchesCurrentTask(notification, jobImpactModeToMatchAgainst);
        }

        private bool NotificationMatchesCurrentTask(IManagementNotificationContext notification, JobImpactTranslationMode jobImpactModeToMatchAgainst)
        {
            if (notification == null)
            {
                return false;
            }

            InfrastructureTaskDescription oldTask = this.lastKnownTask;
            Debug.Assert(oldTask != null);

            InfrastructureTaskDescription newTask = TranslateNotificationToTask(notification, 0, jobImpactModeToMatchAgainst);

            bool tasksAreEqual = NodeTaskDescription.AreEqual(oldTask.NodeTasks, newTask.NodeTasks);

            // See RDBug 1225448: IS: Scale-down fails, because CompleteJobStep notification impact list does not match StartJobStep impact list
            if (!tasksAreEqual && notification.NotificationType == NotificationType.CompleteJobStep)
            {
                tasksAreEqual = AreTaskDescriptionsEqualIgnoringMissingRemovals(oldTask.NodeTasks, newTask.NodeTasks);
                if (tasksAreEqual)
                {
                    Trace.WriteWarning(TraceType,
                        "Notification matches current task only if missing Remove impacts are ignored; old = {0}; new = {1}",
                        ConvertNodeTasksToString(oldTask),
                        ConvertNodeTasksToString(newTask));
                }
            }

            TraceType.WriteInfo(
                "NotificationMatchesCurrentTask returning {0}; jobImpactModeToMatchAgainst = {1}; old = {2}; new = {3}",
                tasksAreEqual,
                jobImpactModeToMatchAgainst,
                ConvertNodeTasksToString(oldTask),
                ConvertNodeTasksToString(newTask));

            return tasksAreEqual;
        }

        private static bool AreTaskDescriptionsEqualIgnoringMissingRemovals(ReadOnlyCollection<NodeTaskDescription> oldTasks, ReadOnlyCollection<NodeTaskDescription> newTasks)
        {
            HashSet<string> removedNodes = new HashSet<string>(
                oldTasks.Where(d => d.TaskType == NodeTask.Remove).Select(d => d.NodeName));

            var trimmedOldTasks = oldTasks.Where(d => d.TaskType != NodeTask.Remove).ToList().AsReadOnly();

            var trimmedNewTasks = newTasks.Where(d => d.TaskType != NodeTask.Remove || !removedNodes.Contains(d.NodeName)).ToList().AsReadOnly();

            return NodeTaskDescription.AreEqual(trimmedOldTasks, trimmedNewTasks);
        }

        private bool IsJobEligibleForLearningModeOptimization(IManagementNotificationContext notification)
        {
            if (notification == null)
            {
                return false;
            }

            bool jobTypeMatches = notification.ActiveJobType == JobType.DeploymentUpdateJob ||
                                  notification.ActiveJobType == JobType.PlatformUpdateJob;
            if (!jobTypeMatches)
            {
                return false;
            }

            string key = ConfigEnableLearningModeForJobTypeFormatKeyName.ToString(notification.ActiveJobType);

            bool enabled = ReadConfigValue(key, DefaultEnableLearningMode);
            return enabled;
        }

        private InfrastructureTaskDescription TranslateNotificationToTask(IManagementNotificationContext notification, long instanceId, JobImpactTranslationMode jobImpactTranslationModeToCreateTaskFor)
        {
            var nodeTasks = new List<NodeTaskDescription>();

            foreach (ImpactedInstance instance in notification.ImpactedInstances)
            {
                // TODO: Handle cases when instance name cannot be mapped
                string nodeName = instance.Id.TranslateRoleInstanceToNodeName();
                NodeTask taskType = ConvertImpactReasonsToNodeTask(notification.ActiveJobType, instance.ImpactReasons);

                if (jobImpactTranslationModeToCreateTaskFor == JobImpactTranslationMode.Optimized && 
                    taskType == NodeTask.Restart) // we don't optimize for higher impacts like Relocate and Remove
                {
                    taskType = NodeTask.Invalid;
                }

                // There are three criteria for using the detailed impact statement instead of the old impact reasons:
                //
                // 1. This new behavior is enabled via config for this job type
                // 2. There is a detailed impact statement
                // 3. The task is not already set to Remove, from an impact reason of RemoveInstance. Since the detailed
                //    impact statement currently only exists for PlatformUpdateJobs this should never come up. Regardless,
                //    there is no known reason to downgrade from Remove, so that is enforced here.
                //
                if (this.impactTranslator.UseImpactDetail(notification.ActiveJobType) &&
                    (instance.ImpactDetail != null) &&
                    (taskType != NodeTask.Remove))
                {
                    taskType = this.impactTranslator.TranslateImpactDetailToNodeTask(instance.ImpactDetail);
                    Trace.WriteInfo(TraceType, "TranslateImpactDetailToNodeTask: {0} -> {1}", instance, taskType);
                }

                if (taskType != NodeTask.Invalid)
                {
                    var nodeTaskDescription = new NodeTaskDescription()
                    {
                        NodeName = nodeName,
                        TaskType = taskType,
                    };

                    nodeTasks.Add(nodeTaskDescription);
                }
            }

            InfrastructureTaskDescription taskDescription = new InfrastructureTaskDescription(
                this.PartitionId,
                this.taskId,
                instanceId,
                new ReadOnlyCollection<NodeTaskDescription>(nodeTasks));

            return taskDescription;
        }

        #endregion

        #region Static helpers

        private static void DumpTaskDescription(InfrastructureTaskDescription description)
        {
            if (description == null)
            {
                return;
            }

            TraceType.WriteInfo("Node tasks count: {0}, Node task details: {1}", description.NodeTasks.Count, ConvertNodeTasksToString(description));
        }

        private static string ConvertNodeTasksToString(InfrastructureTaskDescription description)
        {
            return string.Join(
                "; ",
                description.NodeTasks.Select(d => string.Format(CultureInfo.InvariantCulture, "Node {0} -> Task {1}", d.NodeName, d.TaskType)));
        }

        private NodeTask ConvertImpactReasonsToNodeTask(JobType jobType, IEnumerable<ImpactReason> reasons)
        {
            bool impactReasonsEmpty = true;
            NodeTask mostSevereTaskType = NodeTask.Invalid;

            foreach (ImpactReason reason in reasons)
            {
                impactReasonsEmpty = false;
                NodeTask task = ConvertImpactReasonToNodeTask(reason);

                if (task > mostSevereTaskType)
                {
                    mostSevereTaskType = task;
                }
            }

            if (impactReasonsEmpty)
            {
                // We do not always get an impact reason for RootHE updates; treat these as restarts
                if (jobType == JobType.PlatformUpdateJob)
                {
                    mostSevereTaskType = ReadConfigValue(ConfigMinimumImpactPlatformUpdateJobKeyName, NodeTask.Restart);
                }
                else
                {
                    // Other job types are expected to have some impact listed
                    string message = String.Format(
                        CultureInfo.InvariantCulture,
                        "Failed to convert impact reasons ({0}) to node task",
                        ConvertImpactReasonsToString(reasons));

                    Trace.WriteWarning(TraceType, message);
                    throw new InvalidOperationException(message);
                }
            }

            return mostSevereTaskType;
        }

        private static string ConvertImpactReasonsToString(IEnumerable<ImpactReason> reasons)
        {
            return String.Join("|", reasons);
        }

        private NodeTask ConvertImpactReasonToNodeTask(ImpactReason reason)
        {
            NodeTask defaultNodeTask;

            // Note: reasons prefixed with 'Host' cannot be tested on the portal. They need a test cluster with
            // the following settings in the .csdef
            //   vmsize = "ExtraLarge"
            //   alertOnTimeout = "true"
            //   requestExtendedTimeout = "true"
            switch (reason)
            {
                case ImpactReason.AddInstance:
                    defaultNodeTask = NodeTask.Invalid; // There should be no impact if the node is being added
                    break;

                case ImpactReason.ConfigurationUpdate:
                case ImpactReason.ApplicationUpdate:                
                case ImpactReason.OSUpdate:
                case ImpactReason.Reboot:
                case ImpactReason.ReimageOS:
                case ImpactReason.HostReboot:
                    defaultNodeTask = NodeTask.Restart;
                    break;

                case ImpactReason.Relocation:
                case ImpactReason.RepaveData:
                case ImpactReason.HostRepaveData:
                    defaultNodeTask = NodeTask.Relocate;
                    break;

                case ImpactReason.RemoveInstance:
                    defaultNodeTask = NodeTask.Remove;
                    break;

                case ImpactReason.Unknown:
                default:
                    throw new InvalidOperationException(String.Format(CultureInfo.InvariantCulture, "Unknown impact reason: {0}", reason));
            }

            // E.g. key = WindowsAzure.NodeTaskForImpactReason.ConfigurationUpdate
            string key = string.Format(CultureInfo.InvariantCulture, ConfigNodeTaskForImpactReasonFormatKeyName, reason);

            var nodeTask = ReadConfigValue(key, defaultNodeTask);

            if (defaultNodeTask != nodeTask)
            {
                Trace.WriteInfo(
                    TraceType,
                    "NodeTask for ImpactReason {0} is overridden in config. Default NodeTask: {1}, config provided NodeTask: {2}",
                    reason,
                    defaultNodeTask,
                    nodeTask);    
            }

            return nodeTask;
        }

        private static string NotificationToShortDisplayString(IManagementNotificationContext notification)
        {
            return notification.ToShortDisplayString();
        }

        private static string ImpactedInstancesToString(IEnumerable<ImpactedInstance> impactedInstances)
        {
            return String.Join(",", impactedInstances);
        }

        private static string NotificationDetailsToString(IManagementNotificationContext notification)
        {
            if (notification == null)
            {
                return "<null>";
            }

            string impactSummary = ImpactedInstancesToString(notification.ImpactedInstances);

            Dictionary<string, object> jobProperties = new Dictionary<string, object>(StringComparer.Ordinal)
            {
                { "NotificationID", notification.NotificationId },
                { "NotificationStatus", notification.NotificationStatus },
                { "NotificationType", notification.NotificationType },
                { "NotificationDeadline", notification.NotificationDeadline },
                { "JobStepID", notification.ActiveJobStepId },
                { "JobID", notification.ActiveJobId },
                { "JobType", notification.ActiveJobType },
                { "JobIncludesTopologyChange", notification.ActiveJobIncludesTopologyChange },
                { "TargetUD", notification.ActiveJobStepTargetUD },
                { "Incarnation", notification.Incarnation },
                { "JobDetailedStatus", notification.ActiveJobDetailedStatus },
                { "JobContextId", notification.ActiveJobContextId },
                { "JobStepStatus", notification.ActiveJobStepStatus },
                { "JobStepImpact", impactSummary }, // ISSUE: May exceed ETW buffer size?
            };

            string details = String.Join(Environment.NewLine, jobProperties.Select(
                e => String.Format(CultureInfo.InvariantCulture, "{0}={1}", e.Key, e.Value)));

            return details;
        }

        private static JobState TranslateTaskStateToJobState(InfrastructureTaskState taskState)
        {
            switch (taskState)
            {
                case InfrastructureTaskState.PreProcessing:
                case InfrastructureTaskState.PreAckPending:
                    return JobState.WaitingForApproval;

                case InfrastructureTaskState.PreAcked:
                    return JobState.Executing;

                case InfrastructureTaskState.PostProcessing:
                case InfrastructureTaskState.PostAckPending:
                    return JobState.WaitingForHealthCheck;

                case InfrastructureTaskState.PostAcked:
                    return JobState.Idle;
            }

            Debug.Fail("Unexpected job state");
            return JobState.Unknown;
        }

        /// <summary>
        /// Creates a delegate which can produce a Task&lt;string&gt; that calls the given synchronous void method and returns a null string.
        /// </summary>
        private static AsyncCommandHandler WrapSyncVoidCommand(Action<string> commandHandler)
        {
            return delegate(string args) { return Task.Factory.StartNew<string>(() => { commandHandler(args); return null; }); };
        }

        /// <summary>
        /// Creates a delegate which can produce a Task&lt;string&gt; that calls the given synchronous method which returns a string.
        /// </summary>
        private static AsyncCommandHandler WrapSyncCommand(Func<string, string> commandHandler)
        {
            return delegate(string args) { return Task.Factory.StartNew<string>(() => commandHandler(args)); };
        }

        #endregion

        #region Data classes for GetCurrentState serialization
        
        [DataContract]
        private class CoordinatorStateData
        {
            private const string CoordinatorMode = "Serial";

            public CoordinatorStateData()
            {
                Mode = CoordinatorMode;
            }

            [DataMember]
            public string InfrastructureTaskId { get; set; }
            [DataMember]
            public string JobBlockingPolicy { get; set; }
            [DataMember]
            public string LastKnownJobState { get; set; }
            [DataMember]
            public InfrastructureTaskStateData LastKnownTask { get; set; }
            [DataMember]
            public ManagementNotificationData ManagementNotification { get; set; }
            [DataMember]
            public bool IsManagementNotificationAvailable { get; set; }
            [DataMember]
            public int NodesImpactedCount { get; set; }
            [DataMember]
            public string Mode { get; set; }
            [DataMember]
            public string LastHitBreakpoint { get; set; }
            [DataMember]
            public string ContinuePastBreakpoint { get; set; }
        }

        [DataContract]
        private class InfrastructureTaskStateData
        {
            [DataMember]
            public long TaskInstanceId { get; set; }
            [DataMember]
            public NodeTaskDescriptionData[] TaskDescription { get; set; }
        }

        [DataContract]
        private class NodeTaskDescriptionData
        {
            [DataMember]
            public string NodeName { get; set; }
            [DataMember]
            public string TaskType { get; set; }
        }

        [DataContract]
        private class ManagementNotificationData
        {
            [DataMember]
            public string NotificationId { get; set; }
            [DataMember]
            public string NotificationStatus { get; set; }
            [DataMember]
            public string NotificationType { get; set; }
            [DataMember]
            public DateTime NotificationDeadline { get; set; }
            [DataMember]
            public string ActiveJobStepId { get; set; }
            [DataMember]
            public string ActiveJobId { get; set; }
            [DataMember]
            public string ActiveJobType { get; set; }
            [DataMember]
            public bool ActiveJobIncludesTopologyChange { get; set; }
            [DataMember]
            public int ActiveJobStepTargetUD { get; set; }
            [DataMember]
            public string JobStepImpact { get; set; }
            [DataMember]
            public ulong Incarnation { get; set; }
            [DataMember]
            public string ActiveJobDetailedStatus { get; set; }
            [DataMember]
            public string ActiveJobContextId { get; set; }
            [DataMember]
            public string ActiveJobStepStatus { get; set; }
        }

        #endregion

        #region Data classes for GetJobs serialization

        [DataContract]
        private class JobData
        {
            [DataMember]
            public string Id { get; set; }

            [DataMember]
            public string Status { get; set; }

            [DataMember]
            public string Type { get; set; }
            [DataMember]
            public string ContextId { get; set; }

            [DataMember]
            public string DetailedStatus { get; set; }
        }

        [DataContract]
        private class JobCollectionData
        {
            [DataMember]
            public JobData[] Jobs { get; set; }
        }

        #endregion

        #region Data classes for GetRoleInstances serialization

        [DataContract]
        private class RoleInstanceData
        {
            [DataMember]
            public string Id { get; set; }

            [DataMember]
            public string Status { get; set; }

            [DataMember]
            public DateTime LastUpdateTime { get; set; }
        }

        [DataContract]
        private class RoleInstanceCollectionData
        {
            [DataMember]
            public RoleInstanceData[] RoleInstances { get; set; }
        }

        #endregion
    }
}
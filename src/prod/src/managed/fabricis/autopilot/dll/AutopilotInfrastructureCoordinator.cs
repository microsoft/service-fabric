// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Autopilot
{
    using System.Fabric.Health;
    using Collections.Generic;
    using Diagnostics;
    using Globalization;
    using Linq;
    using Repair;
    using Threading;
    using Threading.Tasks;

    /// <summary>
    /// Implements a Service Fabric infrastructure coordinator for Autopilot
    /// </summary>
    internal sealed class AutopilotInfrastructureCoordinator : IInfrastructureCoordinator
    {
        private readonly TraceType traceType;
        private readonly CoordinatorEnvironment environment;

        private readonly Guid partitionId;

        private readonly ConfigReader configReader;
        private readonly IRepairManager repairManager;
        private readonly IHealthClient healthClient;
        private readonly AutopilotDMClientWrapper dmClient;

        private DateTimeOffset coordinatorRunAsyncStartTime;
        private int isCoordinatorRunning;
        private int primaryEpoch;
        private uint nextSequenceNumber;

        private readonly List<Tuple<bool, string>> failureMessages = new List<Tuple<bool, string>>();
        private DateTimeOffset lastSuccessfulRunAt;

        public AutopilotInfrastructureCoordinator(
            CoordinatorEnvironment environment,
            IAutopilotDMClient dmClient,
            IRepairManager repairManager,
            IHealthClient healthClient,
            Guid partitionId,
            long replicaId)
        {
            this.environment = environment.Validate("environment");
            this.partitionId = partitionId;

            this.traceType = environment.CreateTraceType("Coordinator");

            this.repairManager = repairManager.Validate("repairManager");
            this.healthClient = healthClient.Validate("healthClient");
            this.dmClient = new AutopilotDMClientWrapper(dmClient.Validate("dmClient"));

            this.configReader = new ConfigReader(environment.Config);
        }

        public Task<string> RunCommandAsync(bool isAdminCommand, string command, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
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

                coordinatorRunAsyncStartTime = DateTimeOffset.UtcNow;

                this.primaryEpoch = primaryEpoch;

                traceType.WriteInfo("RunAsync: primaryEpoch = 0x{0:X}, nextSequenceNumber = 0x{1:X}",
                    this.primaryEpoch,
                    this.nextSequenceNumber);

                await ProcessAskModeAsync(token).ConfigureAwait(false);
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

        private async Task ProcessAskModeAsync(CancellationToken cancellationToken)
        {
            try
            {
                await ProcessAskModeCoreAsync(cancellationToken).ConfigureAwait(false);
            }
            catch (Exception ex)
            {
                if (cancellationToken.IsCancellationRequested && (ex is OperationCanceledException))
                {
                    traceType.WriteInfo("ProcessAskModeAsync cancelled");
                }
                else
                {
                    traceType.WriteError("Error in ProcessAskModeAsync. Exiting process. Exception: {0}", ex);

                    // the Program's Main method is waiting on this global event. Once set, it exits the process.
                    // Workaround over using Environment.Exit which isn't supported on CoreCLR. Please see this
                    // method's comments for more.
                    ProcessCloser.ExitEvent.Set();
                }
            }
        }

        private async Task ProcessAskModeCoreAsync(CancellationToken cancellationToken)
        {
            traceType.WriteInfo("Starting Autopilot coordination");

            this.lastSuccessfulRunAt = DateTimeOffset.UtcNow;

            while (!cancellationToken.IsCancellationRequested)
            {
                traceType.WriteInfo(
                    "Autopilot coordinator: service name = {0}, partition ID = {1}, start time = {2:O}",
                    this.environment.ServiceName,
                    this.partitionId,
                    this.coordinatorRunAsyncStartTime);

                ClearFailureMessages();
                Stopwatch watch = Stopwatch.StartNew();

                var context = new AskModeContext(this.configReader, this.environment.ServiceName);

                try
                {
                    await ProcessAskModeOnce(context, cancellationToken);
                }
                catch (Exception ex)
                {
                    if (!cancellationToken.IsCancellationRequested)
                    {
                        traceType.WriteWarning("Ask mode processing failed: {0}", ex);
                        AddFailureMessage(false, $"Ask mode processing failed: {ex.Message}");
                    }
                }

                TimeSpan timerPeriod = this.configReader.JobPollingInterval;

                ProcessRunResults(context, watch.Elapsed, timerPeriod);

                if (!cancellationToken.IsCancellationRequested)
                {
                    traceType.WriteInfo("Next run will occur in {0} at {1:O}", timerPeriod, DateTimeOffset.UtcNow + timerPeriod);
                    await Task.Delay(timerPeriod, cancellationToken);
                }
            }

            traceType.WriteInfo("Cancellation requested. Stopping Autopilot coordination.");
        }

        private void ClearFailureMessages()
        {
            this.failureMessages.Clear();
        }

        private void AddFailureMessage(bool isFatal, string message)
        {
            this.failureMessages.Add(Tuple.Create(isFatal, message));
        }

        private void ProcessRunResults(AskModeContext context, TimeSpan runDuration, TimeSpan runInterval)
        {
            TimeSpan warningThreshold = this.configReader.CoordinatorFailureWarningThreshold;
            TimeSpan maxRetryDuration = this.configReader.CoordinatorFailureRetryDuration;
            DateTimeOffset now = DateTimeOffset.UtcNow;
            TimeSpan elapsed = now - lastSuccessfulRunAt;

            int errorCount = this.failureMessages.Count;
            int fatalCount = this.failureMessages.Count(m => m.Item1); // isFatal == true

            traceType.WriteInfo(
                "ProcessRunResults: error count = {0}, fatal error count = {1}, last successful run = {2:O} ({3} ago), run duration = {4} ms, run interval = {5} ms",
                errorCount,
                fatalCount,
                lastSuccessfulRunAt,
                elapsed,
                runDuration.TotalMilliseconds,
                runInterval.TotalMilliseconds);

            if (errorCount == 0)
            {
                this.lastSuccessfulRunAt = now;

                int pendingCount = context.RepairRecords.Count(r => r.IsPendingApproval);
                int activeCount = context.RepairRecords.Count() - pendingCount;
                int overdueCount = context.OverdueRepairTaskCount;

                if (overdueCount > 0)
                {
                    UpdateCoordinatorHealthStatus(
                        HealthState.Warning,
                        $"Autopilot coordinator is operating normally, but processing of some repair tasks is taking longer than expected. " +
                        $"(overdue: {overdueCount}, pending: {pendingCount}, active: {activeCount})");
                }
                else if (runDuration.TotalMilliseconds > (runInterval.TotalMilliseconds * this.configReader.CoordinatorDutyCycleWarningThreshold))
                {
                    UpdateCoordinatorHealthStatus(
                        HealthState.Warning,
                        $"Autopilot coordinator processing is taking longer than expected. " +
                        $"(last run duration: {runDuration}, pending: {pendingCount}, active: {activeCount})");
                }
                else
                {
                    UpdateCoordinatorHealthStatus(
                        HealthState.Ok,
                        $"Autopilot coordinator is operating normally. (pending: {pendingCount}, active: {activeCount})");
                }
            }
            else if (fatalCount > 0)
            {
                string firstFatalError = this.failureMessages.First(m => m.Item1).Item2;
                string message = $"Autopilot coordinator is restarting due to error: {firstFatalError}";
                UpdateCoordinatorHealthStatus(HealthState.Warning, message);
                throw new ApplicationException(message);
            }
            else if (elapsed > maxRetryDuration)
            {
                string firstError = this.failureMessages.First().Item2;
                string message = $"Autopilot coordinator is restarting due to lack of recent success: {firstError}";
                UpdateCoordinatorHealthStatus(HealthState.Warning, message);
                throw new ApplicationException(message);
            }
            else if (elapsed > warningThreshold)
            {
                string firstError = this.failureMessages.First().Item2;
                string message = $"Autopilot coordinator encountered errors: {firstError}";
                UpdateCoordinatorHealthStatus(HealthState.Warning, message);
            }
        }

        private async Task ProcessAskModeOnce(AskModeContext context, CancellationToken cancellationToken)
        {
            cancellationToken.ThrowIfCancellationRequested();
            var machineInfo = await this.dmClient.GetMachineInfoAsync();

            cancellationToken.ThrowIfCancellationRequested();
            var pendingRecords = await this.dmClient.GetDevicesPendingMaintenanceAsync();

            foreach (var record in machineInfo)
            {
                if (record.Id.RepairType != RepairType.None)
                {
                    traceType.WriteInfo("GetMachineInfo: {0},{1},{2},{3}", record.Id.MachineName, record.Status, record.Id.RepairType, record.RepairActionState);
                    context.SetRepairRecord(MachineMaintenanceRecord.FromActiveRepair(record.Id));
                }

                if (!this.configReader.AllowRepairTaskCompletionInMachineStates.Contains(record.Status))
                {
                    traceType.WriteInfo(
                        "Disallowing completion of repair tasks for {0} because its state is {1}",
                        record.Id.MachineName,
                        record.Status);

                    context.DoNotCancelRepairsForMachine(record.Id.MachineName);
                }
            }

            foreach (var record in pendingRecords)
            {
                context.SetRepairRecord(record);
            }

            // TODO consider retrying the fetch/reconcile/execute block, to avoid
            // extending repairs on transient RM errors
            try
            {
                cancellationToken.ThrowIfCancellationRequested();

                // Fetch active repair tasks owned by this service
                IList<IRepairTask> repairTasks = await this.repairManager.GetRepairTaskListAsync(
                    Guid.Empty,
                    null,
                    RepairTaskStateFilter.Active,
                    this.environment.ServiceName);

                context.SetRepairTasks(repairTasks);

                // Reconcile DM and RM
                IEnumerable<IAction> actions = context.Reconcile();

                // Execute actions
                foreach (IAction action in actions)
                {
                    cancellationToken.ThrowIfCancellationRequested();
                    await ExecuteActionAsync(action);
                }
            }
            catch (Exception e)
            {
                traceType.WriteWarning("Reconcile/execute failed: {0}", e);
                AddFailureMessage(false, $"Reconcile/execute failed: {e.Message}");
            }

            // Modify delay times
            context.UpdateRepairDelays();

            // Push updates back to the DM
            foreach (var record in context.RepairRecords.Where(r => r.IsDelayModified))
            {
                cancellationToken.ThrowIfCancellationRequested();
                await UpdateMaintenanceDelayAsync(record);
            }

            // Report health against the nodes
            if (this.configReader.EnableNodeHealthReports)
            {
                foreach (var record in machineInfo)
                {
                    ReportNodeHealth(
                        "Autopilot:MachineStatus",
                        record.Id.MachineName,
                        (record.Status == "H" || record.Status == "P") ? HealthState.Ok : HealthState.Warning,
                        record.Status);

                    ReportNodeHealth(
                        "Autopilot:Repair",
                        record.Id.MachineName,
                        (record.Id.RepairType == RepairType.None) ? HealthState.Ok : HealthState.Warning,
                        $"{record.Id.RepairType},{record.RepairActionState}");
                }
            }
        }

        private void ReportNodeHealth(string property, string nodeName, HealthState healthState, string description)
        {
            var healthInfo = new HealthInformation(
                InfrastructureService.Constants.HealthReportSourceId,
                property,
                healthState)
            {
                Description = description,
                TimeToLive = this.configReader.NodeHealthEventTTL,
                RemoveWhenExpired = true,
            };

            var report = new NodeHealthReport(nodeName, healthInfo);
            this.healthClient.ReportHealth(report);
        }

        private async Task ExecuteActionAsync(IAction action)
        {
            try
            {
                traceType.WriteInfo("Executing action {0}", action);
                await action.ExecuteAsync(this.repairManager);
            }
            catch (Exception e)
            {
                traceType.WriteWarning("Failed to execute action {0}: {1}", action, e);
                AddFailureMessage(false, $"Failed to execute action: {action}");
            }
        }

        private async Task UpdateMaintenanceDelayAsync(MachineMaintenanceRecord record)
        {
            try
            {
                traceType.WriteInfo(
                    "Updating maintenance delay for {0}: {1} -> {2}",
                    record.RecordId,
                    record.OriginalDelay,
                    record.NewDelay);

                await this.dmClient.UpdateMaintenanceDelayAsync(record.MachineName, record.NewDelay);
            }
            catch (Exception e)
            {
                traceType.WriteWarning("Failed to process {0}: {1}", record.RecordId, e);
                AddFailureMessage(false, $"Failed to update maintenance delay: {record.RecordId}");
            }
        }

        /// <summary>
        /// Reports health at the IS service level.
        /// </summary>
        private void UpdateCoordinatorHealthStatus(HealthState serviceHealthState, string description)
        {
            traceType.WriteInfo("Reporting coordinator status: {0}: {1}", serviceHealthState, description);

            var hi = new HealthInformation(
                InfrastructureService.Constants.HealthReportSourceId,
                InfrastructureService.Constants.CoordinatorStatus,
                serviceHealthState)
            {
                Description = description,
            };

            TimeSpan ttl = this.configReader.CoordinatorStatusHealthEventTTL;
            if (ttl > TimeSpan.Zero)
            {
                hi.TimeToLive = ttl;
                hi.RemoveWhenExpired = false;
            }

            var serviceUri = new Uri(this.environment.ServiceName);
            var healthReport = new ServiceHealthReport(serviceUri, hi);
            this.healthClient.ReportHealth(healthReport);
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
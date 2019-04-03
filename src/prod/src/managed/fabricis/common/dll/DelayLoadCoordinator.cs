// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using Diagnostics;
    using Health;
    using Interop;

    /// <summary>
    /// Coordinator that creates and wraps another coordinator when RunAsync is called.
    /// </summary>
    internal sealed class DelayLoadCoordinator : IInfrastructureCoordinator
    {
        private const string ConfigKeyInitTimeout = "DelayLoad.InitTimeoutInSeconds";
        private const string ConfigKeyRetryDelay = "DelayLoad.RetryDelayInSeconds";
        private const string ConfigKeyWarnOnInitFailure = "DelayLoad.WarnOnInitFailure";

        private static readonly TraceType traceType = new TraceType("DelayLoadCoordinator");

        private readonly Func<CoordinatorFactoryArgs, IInfrastructureCoordinator> factory;
        private readonly CoordinatorFactoryArgs factoryArgs;
        private readonly IHealthClient healthClient;
        private readonly IConfigSection configSection;
        private readonly object stateLock = new object();

        private bool isRunning;
        private IInfrastructureCoordinator coordinator;

        public DelayLoadCoordinator(
            Func<CoordinatorFactoryArgs, IInfrastructureCoordinator> factory,
            CoordinatorFactoryArgs factoryArgs,
            IHealthClient healthClient,
            IConfigSection configSection)
        {
            this.factory = factory.Validate("factory");
            this.factoryArgs = factoryArgs.Validate("factoryArgs");
            this.healthClient = healthClient.Validate("healthClient");
            this.configSection = configSection.Validate("configSection");
        }

        public async Task RunAsync(int primaryEpoch, CancellationToken token)
        {
            lock (stateLock)
            {
                if (this.isRunning)
                {
                    throw new InvalidOperationException("Coordinator is already running");
                }

                this.isRunning = true;
            }

            try
            {
                TimeSpan initTimeout = GetInitTimeout();
                traceType.WriteInfo("Initializing coordinator (timeout = {0})", initTimeout);
                IInfrastructureCoordinator currentCoordinator = await InitializeCoordinator(initTimeout, token).ConfigureAwait(false);
                traceType.WriteInfo("Coordinator created successfully");

                lock (stateLock)
                {
                    this.coordinator = currentCoordinator;
                }

                await currentCoordinator.RunAsync(primaryEpoch, token).ConfigureAwait(false);
                traceType.WriteInfo("Coordinator RunAsync completed normally");
            }
            finally
            {
                lock (stateLock)
                {
                    this.coordinator = null;
                    this.isRunning = false;
                }

                traceType.WriteInfo("Exiting RunAsync");
            }
        }

        private TimeSpan GetInitTimeout()
        {
            return TimeSpan.FromSeconds(
                this.configSection.ReadConfigValue(
                    ConfigKeyInitTimeout,
                    defaultValue: 2 * 60));
        }

        private TimeSpan GetRetryDelay()
        {
            return TimeSpan.FromSeconds(
                this.configSection.ReadConfigValue(
                    ConfigKeyRetryDelay,
                    defaultValue: 15));
        }

        private IInfrastructureCoordinator GetCoordinator()
        {
            IInfrastructureCoordinator currentCoordinator;

            lock (stateLock)
            {
                currentCoordinator = this.coordinator;
            }

            if (currentCoordinator == null)
            {
                throw traceType.CreateException(
                    NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NOT_READY,
                    "Coordinator is not ready");
            }

            return currentCoordinator;
        }

        private async Task<IInfrastructureCoordinator> InitializeCoordinator(TimeSpan timeout, CancellationToken token)
        {
            Stopwatch watch = Stopwatch.StartNew();

            while (true)
            {
                try
                {
                    IInfrastructureCoordinator innerCoordinator = factory(factoryArgs);
                    return innerCoordinator;
                }
                catch (Exception e)
                {
                    string message = "Failed to create infrastructure coordinator: {0}".ToString(e);
                    traceType.WriteWarning("{0}", message);

                    if (this.configSection.ReadConfigValue(ConfigKeyWarnOnInitFailure, true))
                    {
                        this.UpdateCoordinatorStatusHealthProperty(
                            HealthState.Warning,
                            message);
                    }
                    else
                    {
                        this.UpdateCoordinatorStatusHealthProperty(
                            HealthState.Ok,
                            "Coordinator is not expected to run.");
                    }
                }

                if (watch.Elapsed > timeout)
                {
                    traceType.WriteWarning("Timed out trying to create coordinator; exiting process");
                    ProcessCloser.ExitEvent.Set();
                }

                TimeSpan retryDelay = GetRetryDelay();
                traceType.WriteInfo("Retrying in {0}; remaining until timeout = {1}", retryDelay, timeout - watch.Elapsed);
                await Task.Delay(retryDelay, token).ConfigureAwait(false);
            }
        }

        private void UpdateCoordinatorStatusHealthProperty(HealthState serviceHealthState, string description)
        {
            var hi = new HealthInformation(
                Constants.HealthReportSourceId,
                Constants.CoordinatorStatus,
                serviceHealthState)
            {
                Description = description,
            };

            var serviceUri = this.factoryArgs.ServiceName;
            var healthReport = new ServiceHealthReport(serviceUri, hi);

            healthClient.ReportHealth(healthReport);
        }

        public Task ReportFinishTaskSuccessAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            var currentCoordinator = GetCoordinator();
            return currentCoordinator.ReportFinishTaskSuccessAsync(taskId, instanceId, timeout, cancellationToken);
        }

        public Task ReportStartTaskSuccessAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            var currentCoordinator = GetCoordinator();
            return currentCoordinator.ReportStartTaskSuccessAsync(taskId, instanceId, timeout, cancellationToken);
        }

        public Task ReportTaskFailureAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            var currentCoordinator = GetCoordinator();
            return currentCoordinator.ReportTaskFailureAsync(taskId, instanceId, timeout, cancellationToken);
        }

        public Task<string> RunCommandAsync(bool isAdminCommand, string command, TimeSpan timeout, CancellationToken cancellationToken)
        {
            var currentCoordinator = GetCoordinator();
            return currentCoordinator.RunCommandAsync(isAdminCommand, command, timeout, cancellationToken);
        }
    }
}
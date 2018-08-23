// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Chaos.Common
{
    using System.Collections.Generic;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Common;
    using System.Fabric.Health;
    using System.Fabric.Query;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Linq;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;

    internal sealed class ValidationHelper
    {
        private const string TraceType = "ValidationHelper";

        private static readonly Uri SystemApplicationNameUri =
            new Uri(Constants.SystemApplicationName);

        private static readonly TimeSpan RetryWaitTimeoutDefault = TimeSpan.FromSeconds(5d);

        private static readonly ValidationCheckFlag CheckFlags = ValidationCheckFlag.All;

        private readonly ChaosParameters chaosParameters;

        private readonly FabricClient fabricClient;

        public ValidationHelper(ChaosParameters chaosParameters, FabricClient fabricClient)
        {
            this.chaosParameters = chaosParameters;
            this.fabricClient = fabricClient;
        }

        public async Task<ValidationReport> ValidateClusterHealthAsync(TimeSpan timeout, CancellationToken token)
        {
            TimeoutHelper timer = new TimeoutHelper(timeout);

            bool success = false;
            StringBuilder healthinfo = new StringBuilder();
            while (!success && timer.GetRemainingTime() > TimeSpan.Zero)
            {
                healthinfo.Clear();
                TestabilityTrace.TraceSource.WriteInfo(
                    TraceType,
                    "ClusterHealthPolicy={0}",
                    this.chaosParameters.ClusterHealthPolicy);

                var clusterHealthResult =
                    await
                        FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                            () =>
                                this.fabricClient.HealthManager.GetClusterHealthAsync(
                                    this.chaosParameters.ClusterHealthPolicy,
                                    this.chaosParameters.RequestTimeout,
                                    token),
                            FabricClientRetryErrors.GetEntityHealthFabricErrors.Value,
                            timer.GetRemainingTime(),
                            token).ConfigureAwait(false);

                TestabilityTrace.TraceSource.WriteInfo(
                    TraceType,
                    "ClusterHealthResult.AggregateHealthState={0}",
                    clusterHealthResult.AggregatedHealthState);

                if (clusterHealthResult.AggregatedHealthState == HealthState.Error)
                {
                    foreach (HealthEvent healthEvent in clusterHealthResult.HealthEvents.Where(h => h.HealthInformation.HealthState != HealthState.Ok))
                    {
                        healthinfo.AppendLine(
                            StringHelper.Format(
                                "Cluster health state is '{0}' with property '{1}', sourceId '{2}' and description '{3}'",
                                healthEvent.HealthInformation.HealthState,
                                healthEvent.HealthInformation.Property,
                                healthEvent.HealthInformation.SourceId,
                                healthEvent.HealthInformation.Description));
                    }

                    // Collect health events on the unhealthy nodes
                    foreach (var nodeHealthState in clusterHealthResult.NodeHealthStates.Where(n => n.AggregatedHealthState != HealthState.Ok))
                    {
                        var healthState = nodeHealthState;

                        var nodeHealthResult =
                            await
                                FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                                    () =>
                                        this.fabricClient.HealthManager.GetNodeHealthAsync(
                                            healthState.NodeName,
                                            this.chaosParameters.RequestTimeout,
                                            token),
                                    FabricClientRetryErrors.GetEntityHealthFabricErrors.Value,
                                    timer.GetRemainingTime(),
                                    token).ConfigureAwait(false);

                        foreach (var healthEvent in nodeHealthResult.HealthEvents.Where(h => h.HealthInformation.HealthState != HealthState.Ok))
                        {
                            healthinfo.AppendLine(
                                StringHelper.Format(
                                    "Node '{0}' health state is '{1}' with property '{2}', sourceId '{3}' and description '{4}'",
                                    nodeHealthResult.NodeName,
                                    healthEvent.HealthInformation.HealthState,
                                    healthEvent.HealthInformation.Property,
                                    healthEvent.HealthInformation.SourceId,
                                    healthEvent.HealthInformation.Description));
                        }
                    }

                    // Collect health events on the unhealthy apps
                    foreach (var appHealthState in clusterHealthResult.ApplicationHealthStates.Where(app => app.AggregatedHealthState != HealthState.Ok))
                    {
                        var healthState = appHealthState;

                        var appHealthResult =
                            await
                                FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                                    () =>
                                        this.fabricClient.HealthManager.GetApplicationHealthAsync(
                                            healthState.ApplicationName,
                                            this.chaosParameters.RequestTimeout,
                                            token),
                                    FabricClientRetryErrors.GetEntityHealthFabricErrors.Value,
                                    timer.GetRemainingTime(),
                                    token).ConfigureAwait(false);

                        foreach (var healthEvent in appHealthResult.HealthEvents.Where(h => h.HealthInformation.HealthState != HealthState.Ok))
                        {
                            healthinfo.AppendLine(
                                StringHelper.Format(
                                    "Application '{0}' health state is '{1}' with property '{2}', sourceId '{3}' and description '{4}'",
                                    appHealthResult.ApplicationName,
                                    healthEvent.HealthInformation.HealthState,
                                    healthEvent.HealthInformation.Property,
                                    healthEvent.HealthInformation.SourceId,
                                    healthEvent.HealthInformation.Description));
                        }
                    }

                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0}", healthinfo.ToString());
                }
                else
                {
                    success = true;
                }

                if (!success)
                {
                    // Delay before querying again so we allow some time for state to change - don't spam the node
                    await AsyncWaiter.WaitAsync(RetryWaitTimeoutDefault, token).ConfigureAwait(false);
                }
            }

            if (success)
            {
                return ValidationReport.Default;
            }
            else
            {
                return new ValidationReport(
                           true,
                           StringHelper.Format(StringResources.Error_ClusterNotHealthy, timeout, healthinfo.ToString()));
            }
        }

        public async Task<ValidationReport> ValidateAllServicesAsync(TimeSpan timeout, CancellationToken token)
        {
            var timer = new TimeoutHelper(timeout);

            var taskList = new List<Task<ValidationReport>>
                               {
                                   this.ValidateSystemServicesAsync(timer.GetRemainingTime(), token),
                                   this.ValidateApplicationServicesAsync(timer.GetRemainingTime(), token)
                               };

            var report = await this.MergeReportsAsync(taskList, timer.GetRemainingTime()).ConfigureAwait(false);

            return report;
        }

        public async Task<ValidationReport> ValidateSystemServicesAsync(TimeSpan timeout, CancellationToken token)
        {
            var timer = new TimeoutHelper(timeout);

            var svcListResult =
                await
                    FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () =>
                            this.fabricClient.QueryManager.GetServiceListAsync(
                                SystemApplicationNameUri,
                                null,
                                this.chaosParameters.RequestTimeout,
                                token),
                        timer.GetRemainingTime(),
                        token).ConfigureAwait(false);

            var tasks = new List<Task<ValidationReport>>();
            foreach (var service in svcListResult)
            {
                var task = this.ValidateServiceAsync(service.ServiceName, timer.GetRemainingTime(), token);
                tasks.Add(task);
            }

            var report = await this.MergeReportsAsync(tasks, timer.GetRemainingTime()).ConfigureAwait(false);

            return report;
        }

        public async Task<ValidationReport> ValidateApplicationServicesAsync(TimeSpan timeout, CancellationToken token)
        {
            var timer = new TimeoutHelper(timeout);

            var appListResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                () => this.fabricClient.QueryManager.GetApplicationListAsync(
                    null,
                    string.Empty,
                    this.chaosParameters.RequestTimeout,
                    token),
                timer.GetRemainingTime(),
                token).ConfigureAwait(false);

            var taskList = new List<Task<ValidationReport>>();
            foreach (var app in appListResult)
            {
                var app1 = app;
                var serviceListResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => this.fabricClient.QueryManager.GetServiceListAsync(
                        app1.ApplicationName,
                        null,
                        this.chaosParameters.RequestTimeout,
                        token),
                    timer.GetRemainingTime(),
                    token).ConfigureAwait(false);

                foreach (var svc in serviceListResult)
                {
                    taskList.Add(this.ValidateServiceAsync(svc.ServiceName, timer.GetRemainingTime(), token));
                }
            }

            var report = await this.MergeReportsAsync(taskList, timer.GetRemainingTime()).ConfigureAwait(false);

            return report;
        }

        public async Task<ValidationReport> ValidateServiceAsync(Uri serviceName, TimeSpan timeout, CancellationToken token)
        {
            var timer = new TimeoutHelper(timeout);

            var taskList = new List<Task<ValidationReport>>();

            var serviceQueryClient = new ServiceQueryClient(
                serviceName,
                this.fabricClient.FaultManager.TestContext,
                CheckFlags,
                this.chaosParameters.OperationTimeout,
                this.chaosParameters.RequestTimeout);

            var ensureStabilityTask = serviceQueryClient.EnsureStabilityWithReportAsync(
                    timer.GetRemainingTime(),
                    this.chaosParameters.RequestTimeout,
                    token);

            var ensureHealthTask = serviceQueryClient.ValidateHealthWithReportAsync(
                timer.GetRemainingTime(),
                this.chaosParameters.RequestTimeout,
                token);

            taskList.Add(ensureStabilityTask);
            taskList.Add(ensureHealthTask);

            var report = await this.MergeReportsAsync(taskList, timer.GetRemainingTime()).ConfigureAwait(false);

            return report;
        }

        private async Task<ValidationReport> MergeReportsAsync(
            IList<Task<ValidationReport>> taskList,
            TimeSpan timeout)
        {
            var reports = await Task.WhenAll(taskList).ConfigureAwait(false);

            StringBuilder combinedReport = new StringBuilder();
            foreach (var res in reports.Where(res => res.ValidationFailed))
            {
                combinedReport.AppendLine(res.FailureReason);
            }

            string reason = combinedReport.ToString();

            return string.IsNullOrEmpty(reason)
                ? ValidationReport.Default
                : new ValidationReport(true, reason);
        }
    }
}
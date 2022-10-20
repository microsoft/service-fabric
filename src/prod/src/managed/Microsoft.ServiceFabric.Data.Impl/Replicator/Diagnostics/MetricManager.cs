// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator.Diagnostics
{
    using Microsoft.ServiceFabric.ReplicatedStore.Diagnostics;
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common.Tracing;
    using System.Globalization;
    using System.Threading.Tasks;
    using System.Threading;

    /// <summary>
    /// MetricManager maintains a dictionary of &lt;CounterType, IMetricCounter&gt; pairs.
    /// It aggregates counting information from all state providers and reports them to 
    /// telemetry based on an aggregation window.
    /// </summary>
    internal class MetricManager : IMetricManager
    {
        private const string ClassName = "MetricManager";

        private readonly IReadOnlyDictionary<MetricProviderType, IMetricProvider> metricProviders;
        private readonly TimeSpan aggregationWindow = TimeSpan.FromMinutes(15);
        private readonly TimeSpan samplingInterval = TimeSpan.FromMinutes(1);

        private TimeSpan timeSinceLastReport = TimeSpan.Zero;
        private StatefulServiceContext initializationParameters;
        private Task metricProcessingTask;
        private CancellationTokenSource metricProcessingTaskCts;
        private string traceType = string.Empty;

        /// <summary>
        /// Initializes a new instance of the MetricManager class.
        /// </summary>
        public MetricManager(ITransactionalReplicator replicator)
        {
            this.metricProviders = new Dictionary<MetricProviderType, IMetricProvider>()
            {
                { MetricProviderType.TStore, new TStoreMetricProvider(replicator) }
            };
        }

        /// <summary>
        /// Gets the metric provider that is associated with the specified type.
        /// </summary>
        /// <param name="type">The metric provider type to locate.</param>
        /// <returns>The metric provider associated with the specified type.</returns>
        public IMetricProvider GetProvider(MetricProviderType type)
        {
            bool success = metricProviders.TryGetValue(type, out IMetricProvider result);

            Utility.Assert(
                    success,
                    "{0}: Metric provider could not be found.\n {1}",
                    this.traceType,
                    type);

            return metricProviders[type];
        }

        /// <summary>
        /// Initialize metric manager with stateful service context information.
        /// </summary>
        /// <param name="parameters">Stateful service context information</param>
        public void Initialize(StatefulServiceContext parameters)
        {
            this.initializationParameters = parameters;
            this.traceType = string.Format(
                CultureInfo.InvariantCulture,
                "{0}@{1}@{2}",
                parameters.PartitionId,
                parameters.ReplicaId,
                ClassName);
        }

        /// <summary>
        /// Start metric polling, aggregation, and reporting task.
        /// </summary>
        public void StartTask()
        {
            Utility.Assert(
                this.metricProcessingTaskCts == null && this.metricProcessingTask == null,
                "{0}:StartTask(): Task has been started more than once.\n",
                this.traceType);

            this.metricProcessingTaskCts = new CancellationTokenSource();
            this.metricProcessingTask = MetricProcessingTaskAsync(metricProcessingTaskCts.Token);
        }

        /// <summary>
        /// Stop metric polling, aggregation, and reporting task.
        /// </summary>
        public async Task StopTaskAsync()
        {
            Utility.Assert(
                this.metricProcessingTaskCts != null && this.metricProcessingTask != null,
                "{0}:StopTaskAsync(): There is no available task.\n",
                this.traceType);

            try
            {
                this.metricProcessingTaskCts.Cancel();
                await this.metricProcessingTask.ConfigureAwait(false);
            }
            catch (Exception e)
            {
                FabricEvents.Events.Warning(
                    this.traceType,
                    "StopTaskAsync() threw unexpected exception.\n" + e);
            }

            this.metricProcessingTask = null;
            this.metricProcessingTaskCts = null;
        }

        private async Task MetricProcessingTaskAsync(CancellationToken cancellationToken)
        {
            try
            {
                while (cancellationToken.IsCancellationRequested == false)
                {
                    await Task.Delay(this.samplingInterval, cancellationToken);

                    this.timeSinceLastReport += this.samplingInterval;

                    if (this.timeSinceLastReport >= this.aggregationWindow)
                    {
                        this.timeSinceLastReport = TimeSpan.Zero;
                        ReportToTelemetry();
                    }

                    ResetSampling();
                }
            }
            catch (ArgumentOutOfRangeException e)
            {
                Utility.Assert(
                    false,
                    "{0}: MetricProcessingTask threw argument exception.\n {1}",
                    this.traceType, e);
            }
            catch (TaskCanceledException e)
            {
                FabricEvents.Events.Warning(
                    this.traceType,
                    "MetricProcessingTask stopped due to cancellation.\n " + e);
            }
            catch (Exception e)
            {
                FabricEvents.Events.Warning(
                    this.traceType,
                    "MetricProcessingTask stopped due to unexpected exception.\n " + e);
                throw;
            }
        }

        /// <summary>
        /// Report all valid metric counter information to telemetry.
        /// </summary>
        private void ReportToTelemetry()
        {
            if (this.initializationParameters == null)
            {
                return;
            }

            foreach (var metricProvider in this.metricProviders.Values)
            {
                metricProvider.Report(this.initializationParameters);
            }
        }

        /// <summary>
        /// Re-enable measurements for any metric that requires sampling.
        /// </summary>
        private void ResetSampling()
        {
            if (this.initializationParameters == null)
            {
                return;
            }

            foreach (var metricProvider in this.metricProviders.Values)
            {
                metricProvider.ResetSampling();
            }
        }
    }
}

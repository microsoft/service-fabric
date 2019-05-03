// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights.PartitionInsight
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using Cluster.ClusterQuery;
    using ClusterAnalysis.AnalysisCore.Callback;
    using ClusterAnalysis.AnalysisCore.ClusterEntityDescription;
    using ClusterAnalysis.AnalysisCore.Insights.Agents;
    using ClusterAnalysis.Common;
    using ClusterAnalysis.Common.Config;
    using ClusterAnalysis.Common.Log;
    using ClusterAnalysis.Common.Store;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.RA;
    using TraceAccessLayer;
    using Assert = ClusterAnalysis.Common.Util.Assert;

    /// <summary>
    /// Agent for analyzing details of a Reconfiguration that has happened.
    /// </summary>
    internal class ReconfigAnalysisAgent : Agent
    {
        private ReconfigAnalysisAgentConfig currentConfig;

        /// <inheritdoc />
        internal ReconfigAnalysisAgent(
            Config config,
            ITaskRunner taskRunner,
            ILogger logger,
            IStoreProvider storeProvider,
            ITraceStoreReader eventStoreReader,
            ITraceStoreReader queryStoreReader,
            IClusterQuery clusterQuery,
            CancellationToken token) : base(taskRunner, logger, storeProvider, eventStoreReader, queryStoreReader, clusterQuery, token)
        {
            Assert.IsNotNull(config, "Config can't be null");
            this.currentConfig = config.NamedConfigManager.GetConfiguration<ReconfigAnalysisAgentConfig>();
            this.Logger.LogMessage("Init with Config: {0}", this.currentConfig);
        }

        /// <inheritdoc />
        public override AgentIdentifier AgentIdentifier
        {
            get { return AgentIdentifier.ReconfigurationAnalysisAgent; }
        }

        /// <inheritdoc />
        public override IEnumerable<NotifyFilter> NotifyFilters
        {
            get { return new List<NotifyFilter> { new NotifyFilter(Scenario.Reconfiguration, TimeSpan.Zero) }; }
        }

        /// <inheritdoc />
        public override Task StopAsync(StopOptions options)
        {
            return Task.FromResult(true);
        }

        /// <inheritdoc />
        public override Task<bool> IsEligibleForAnalysisAsync(BaseEntity targetEntity, ScenarioData data)
        {
            Assert.IsNotNull(targetEntity, "targetEntity");
            Assert.IsNotNull(data, "data");

            var reconfig = data.TraceRecord as ReconfigurationCompletedTraceRecord;
            Assert.IsNotNull(
                reconfig,
                string.Format(CultureInfo.InvariantCulture, "Actual Type: {0}, Expected: {1}", data.TraceRecord.GetType(), typeof(ReconfigurationCompletedTraceRecord)));

            if (reconfig.TotalDurationMs > this.currentConfig.ReconfigTimeThresholdForAnalysis.TotalMilliseconds)
            {
                return Task.FromResult(true);
            }

            if (reconfig.Result != ReconfigurationResult.Completed)
            {
                return Task.FromResult(true);
            }

            return Task.FromResult(true);
        }

        /// <inheritdoc />
        public override Task<Continuation> DoAnalysisAsync(AnalysisContainer reconfigAnalysis)
        {
            Assert.IsNotNull(reconfigAnalysis, "We expect Analysis of Type ReconfigInstanceAnalysisDetails");

            this.Logger.LogMessage("Current State {0}", reconfigAnalysis.GetProgressedTill());

            if (reconfigAnalysis.GetProgressedTill() == ProgressTracker.NotStarted)
            {
                this.Logger.LogMessage("DoAnalysisAsync:: Populating Duration");
                this.PopulateStartEndTimes((ReconfigurationAnalysisEvent)reconfigAnalysis.AnalysisEvent);
                reconfigAnalysis.SetProgressedTill(ProgressTracker.Finished);
            }

            if (reconfigAnalysis.GetProgressedTill() == ProgressTracker.Finished)
            {
                return Task.FromResult(Continuation.Done);
            }

            throw new Exception(string.Format(CultureInfo.InvariantCulture, "Progress Stage {0} not Valid", reconfigAnalysis.GetProgressedTill()));
        }

        /// <inheritdoc />
        public override AnalysisContainer CreateNewAnalysisContainer(TraceRecord traceRecord)
        {
            return new AnalysisContainer(new ReconfigurationAnalysisEvent((ReconfigurationCompletedTraceRecord)traceRecord), AgentIdentifier.ReconfigurationAnalysisAgent);
        }

        private void PopulateStartEndTimes(ReconfigurationAnalysisEvent reconfigAnalysis)
        {
            // I'm seeing some discrepancy (3-5 ms magnitude) between time stamp of the same event in blob and table. While DCA investigate why
            // adding some buffer. Once we have data, this will either be removed or changed.
            reconfigAnalysis.ReconfigFinishTime = reconfigAnalysis.TriggerReconfigurationCompletedTraceRecord.TimeStamp + TimeSpan.FromMilliseconds(15);
            reconfigAnalysis.ReconfigStartTime = reconfigAnalysis.ReconfigFinishTime -
                                                 TimeSpan.FromMilliseconds(reconfigAnalysis.TriggerReconfigurationCompletedTraceRecord.TotalDurationMs);
        }
    }
}
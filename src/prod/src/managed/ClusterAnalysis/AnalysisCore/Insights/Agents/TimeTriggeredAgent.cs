// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights.Agents
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;
    using Cluster.ClusterQuery;
    using ClusterAnalysis.Common;
    using ClusterAnalysis.Common.Log;
    using ClusterAnalysis.Common.Store;
    using TraceAccessLayer;

    /// <summary>
    /// Base class for all Time Triggered agents (agents that need to run periodically)
    /// </summary>
    internal abstract class TimeTriggeredAgent : Agent
    {
        /// <inheritdoc />
        protected TimeTriggeredAgent(
            ITaskRunner taskRunner,
            ILogger logger,
            IStoreProvider storeProvider,
            ITraceStoreReader eventStoreReader,
            ITraceStoreReader queryStoreReader,
            IClusterQuery clusterQuery,
            CancellationToken token) : base(taskRunner, logger, storeProvider, eventStoreReader, queryStoreReader, clusterQuery, token)
        {
        }

        /// <summary>
        /// Gets the scheduling frequency for this particular agent. This 
        /// dictate how frequently the scheduler will trigger this agent.
        /// </summary>
        public abstract TimeSpan SchedulingFrequencyDuration { get; }

        public async Task StartAsync()
        {
            this.Logger.LogMessage("StartAsync:: Entering");

            var analysis = await this.AnalyzeAsync().ConfigureAwait(false);

            // TODO: Persist
        }

        /// <summary>
        /// Entry point for the analysis. When its time to run this agent
        /// this entry point will be called.
        /// </summary>
        /// <returns>This returns a collection of analysis object that captures
        /// analysis done during the current run of the agent</returns>
        /// <remarks>
        /// Today there is no provision to support resuming of unfinished analysis
        /// that are returned by this routine.
        /// </remarks>
        protected abstract Task<IEnumerable<AnalysisContainer>> AnalyzeAsync();
    }
}
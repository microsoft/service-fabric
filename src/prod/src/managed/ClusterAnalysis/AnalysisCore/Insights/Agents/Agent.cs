// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights.Agents
{
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;
    using Cluster.ClusterQuery;
    using ClusterAnalysis.AnalysisCore;
    using ClusterAnalysis.AnalysisCore.Callback;
    using ClusterAnalysis.AnalysisCore.ClusterEntityDescription;
    using ClusterAnalysis.Common;
    using ClusterAnalysis.Common.Log;
    using ClusterAnalysis.Common.Store;
    using ClusterAnalysis.TraceAccessLayer;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    /// <summary>
    /// Base class for all Agents.
    /// </summary>
    internal abstract class Agent
    {
        protected Agent(
            ITaskRunner taskRunner,
            ILogger logger,
            IStoreProvider storeProvider,
            ITraceStoreReader eventStoreReader,
            ITraceStoreReader queryStoreReader,
            IClusterQuery clusterQuery,
            CancellationToken token)
        {
            Common.Util.Assert.IsNotNull(storeProvider, "Store Provider can't be null");
            Common.Util.Assert.IsNotNull(logger, "Loger can't be null");
            Common.Util.Assert.IsNotNull(eventStoreReader, "Query Store reader can't be null");
            Common.Util.Assert.IsNotNull(taskRunner, "Task Runner can't be null");
            Common.Util.Assert.IsNotNull(queryStoreReader, "Query Store reader can't be null");
            Common.Util.Assert.IsNotNull(clusterQuery, "Cluster Query can't be null");

            this.TaskRunner = taskRunner;
            this.Logger = logger;
            this.StoreProvider = storeProvider;
            this.EventStoreReader = eventStoreReader;
            this.QueryStoreReader = queryStoreReader;
            this.ClusterQueryInstance = clusterQuery;
            this.CancelToken = token;
        }

        /// <summary>
        /// List of Filters for which an Agent should be notified.
        /// </summary>
        public abstract IEnumerable<NotifyFilter> NotifyFilters { get; }

        /// <summary>
        /// Unique Identifier for the Agent. This is unique across all agents and also immutable for the life time of an Agent.
        /// </summary>
        /// <returns></returns>
        public abstract AgentIdentifier AgentIdentifier { get; }

        /// <summary>
        /// Task runner
        /// </summary>
        /// <remarks>
        /// If you need to create a new Task, create through this Task Runner.
        /// </remarks>
        protected ITaskRunner TaskRunner { get; }

        /// <summary>
        /// Logger
        /// </summary>
        protected ILogger Logger { get; }

        /// <summary>
        /// Abstraction to Read events from Events Store
        /// </summary>
        protected ITraceStoreReader EventStoreReader { get; }

        /// <summary>
        /// Abstraction to Read events from Query Store
        /// </summary>
        protected ITraceStoreReader QueryStoreReader { get; }

        /// <summary>
        /// Abstraction to read events through Cluster Query
        /// </summary>
        protected IClusterQuery ClusterQueryInstance { get; }

        /// <summary>
        /// Provider for Reliable State persistence
        /// </summary>
        protected IStoreProvider StoreProvider { get; }

        /// <summary>
        /// Token to be respected.
        /// </summary>
        protected CancellationToken CancelToken { get; }

        /// <summary>
        /// Using the received information, returns if this data should be analyzed any further.
        /// </summary>
        /// <remarks>
        /// If an existing analysis container has registered interest in this data, then this routine is not invoked.
        /// If no existing analysis container is waiting for it, this routine is first invoked to determine if
        /// new analysis container should be created for this data to analyze it.
        /// </remarks>
        /// <param name="targetEntity"></param>
        /// <param name="data"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        public virtual Task<bool> IsEligibleForAnalysisAsync(BaseEntity targetEntity, ScenarioData data)
        {
            return Task.FromResult(true);
        }

        /// <summary>
        /// Create an Empty Container to do analysis in response to a specific Trace Record
        /// </summary>
        /// <param name="signalInfo"></param>
        /// <returns></returns>
        public abstract AnalysisContainer CreateNewAnalysisContainer(TraceRecord signalInfo);

        /// <summary>
        /// Do Analysis
        /// </summary>
        /// <param name="analysis"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        public abstract Task<Continuation> DoAnalysisAsync(AnalysisContainer analysis);

        /// <summary>
        /// Update an Analysis. This routine will be called whenever we see new data
        /// that an analysis has expressed interest in. The routine is only called once
        /// the main analysis is finished.
        /// </summary>
        /// <param name="analysis"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        public virtual Task<Continuation> UpdateAnalysisAsync(AnalysisContainer analysis)
        {
            return Task.FromResult(Continuation.Done);
        }

        /// <summary>
        /// Stop Async.
        /// </summary>
        /// <remarks>
        /// A hook for children to stop themselves and clear state (if requested).
        /// This is marked abstract so that child classes are forced to implement and most importantly think about this.
        /// So, for example, if some kid is creating their own storage and stop intent is to remove data, they should clear it.
        /// </remarks>
        /// <param name="options"></param>
        /// <returns></returns>
        public abstract Task StopAsync(StopOptions options);

        /// <summary>
        /// Reload Configuration.
        /// </summary>
        public virtual void ReloadConfiguration()
        {
        }
    }
}
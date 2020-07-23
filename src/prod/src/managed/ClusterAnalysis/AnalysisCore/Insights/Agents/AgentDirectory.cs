// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights.Agents
{
    using System;
    using System.Collections.Concurrent;
    using System.Threading;
    using Cluster.ClusterQuery;
    using ClusterAnalysis.AnalysisCore.Insights.PartitionInsight;
    using ClusterAnalysis.Common;
    using ClusterAnalysis.Common.Config;
    using ClusterAnalysis.Common.Log;
    using ClusterAnalysis.Common.Store;
    using ClusterAnalysis.Common.Util;
    using ClusterAnalysis.TraceAccessLayer.StoreConnection;

    /// <summary>
    /// A simple Agent Directory. This is the only "Supported" way of Creating Agents.
    /// </summary>
    internal sealed class AgentDirectory
    {
        private static object singleAccessLock = new object();
        private ITaskRunner taskRunner;
        private ILogProvider logProvider;
        private IStoreProvider storeProvider;
        private IClusterQuery clusterQuery;
        private TraceStoreConnection storeConnection;
        private ConcurrentDictionary<AgentIdentifier, Agent> agentMap;
        private CancellationToken token;
        private Config configuration;

        private AgentDirectory(
            Config configuration,
            ILogProvider logProvider,
            IStoreProvider storeProvider,
            ITaskRunner taskRunner,
            TraceStoreConnection traceStoreConnection,
            IClusterQuery clusterQuery,
            CancellationToken token)
        {
            Assert.IsNotNull(configuration, "Config can't be null");
            Assert.IsNotNull(logProvider, "Log Provider can't be null");
            Assert.IsNotNull(storeProvider, "Store Provider can't be null");
            Assert.IsNotNull(taskRunner, "Task Runner can't be null");
            Assert.IsNotNull(traceStoreConnection, "Trace store connection can't be null");
            Assert.IsNotNull(clusterQuery, "Cluster Query can't be null");
            this.taskRunner = taskRunner;
            this.logProvider = logProvider;
            this.clusterQuery = clusterQuery;
            this.storeConnection = traceStoreConnection;
            this.storeProvider = storeProvider;
            this.token = token;
            this.configuration = configuration;
            this.agentMap = new ConcurrentDictionary<AgentIdentifier, Agent>();
        }

        public static AgentDirectory SingleInstance { get; private set; }

        public static void InitializeSingleInstance(
            Config config,
            ILogProvider logProvider,
            IStoreProvider storeProvider,
            ITaskRunner taskRunner,
            TraceStoreConnection traceStoreConnection,
            IClusterQuery clusterQuery,
            CancellationToken token)
        {
            if (SingleInstance != null)
            {
                return;
            }

            lock (singleAccessLock)
            {
                if (SingleInstance == null)
                {
                    SingleInstance = new AgentDirectory(config, logProvider, storeProvider, taskRunner, traceStoreConnection, clusterQuery, token);
                }
            }
        }

        public static void ReleaseInstance()
        {
            Assert.IsNotNull(SingleInstance, "Instance was never created, or already released");
            SingleInstance.agentMap = null;
            SingleInstance = null;
        }

        public Agent GetOrCreateAgentInstance(AgentIdentifier identifier)
        {
            return this.agentMap.GetOrAdd(identifier, this.Create);
        }

        private Agent Create(AgentIdentifier identifier)
        {
            if (identifier == AgentIdentifier.PrimaryMoveAnalysisAgent)
            {
                return new PrimaryMoveAnalysisAgent(
                    this.configuration,
                    this.taskRunner,
                    this.logProvider.CreateLoggerInstance(AgentIdentifier.PrimaryMoveAnalysisAgent.ToString()),
                    this.storeProvider,
                    this.storeConnection.EventStoreReader,
                    this.storeConnection.QueryStoreReader,
                    this.clusterQuery,
                    this.token);
            }

            throw new NotSupportedException(string.Format("Idenfier: {0}", identifier));
        }
    }
}
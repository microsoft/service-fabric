// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using Cluster.ClusterQuery;
    using ClusterAnalysis.AnalysisCore.Callback;
    using ClusterAnalysis.AnalysisCore.Exceptions;
    using ClusterAnalysis.AnalysisCore.Insights;
    using ClusterAnalysis.AnalysisCore.Insights.Agents;
    using ClusterAnalysis.AnalysisCore.Insights.ClusterLevelInsight;
    using ClusterAnalysis.AnalysisCore.Insights.NodeInsight;
    using ClusterAnalysis.AnalysisCore.Insights.PartitionInsight;
    using ClusterAnalysis.Common.Runtime;
    using ClusterAnalysis.Common.Util;
    using ClusterAnalysis.TraceAccessLayer.StoreConnection;

    public class ClusterAnalysisEngine : IDisposable
    {
        private SemaphoreSlim singleAccess = new SemaphoreSlim(1);

        private bool isRunning = false;

        private PartitionInsightGenerator partitionInsightGenerator;

        private NodeInsightGenerator nodeInsightGenerator;

        private ClusterInsightGenerator clusterInsightGenerator;

        private ICallbackStore callBackStore;

        public async Task StartAnalysisEngineAsync(IList<InsightType> insightTypes, IInsightRuntime runtime, CancellationToken token)
        {
            await this.singleAccess.WaitAsync(token).ConfigureAwait(false);

            try
            {
                if (this.isRunning)
                {
                    throw new ClusterAnalysisAlreadyStartedException();
                }

                var connectionInfo = (TraceStoreConnectionInformation)runtime.GetService(typeof(TraceStoreConnectionInformation));
                var traceStoreConnection = new TraceStoreConnection(connectionInfo, runtime.GetLogProvider());

                // Add the connection to Runtime. TODO Get rid of this and pass this explicitly.
                runtime.AddService(typeof(TraceStoreConnection), traceStoreConnection);

                var analysisScheduler = new AnalysisScheduler(
                    runtime.GetLogProvider(),
                    runtime.GetStoreProvider(),
                    runtime.TaskRunner,
                    traceStoreConnection,
                    token);

                runtime.AddService(typeof(AnalysisScheduler), analysisScheduler);

                Assert.IsNotNull(connectionInfo, "connectionInfo != null");

                this.callBackStore = new SimpleCallbackStore(
                    runtime.GetStoreProvider(),
                    runtime.TaskRunner,
                    runtime.GetLogProvider(),
                    traceStoreConnection.EventStoreReader,
                    token);

                AgentDirectory.InitializeSingleInstance(
                    runtime.GetCurrentConfig(),
                    runtime.GetLogProvider(),
                    runtime.GetStoreProvider(),
                    runtime.TaskRunner,
                    traceStoreConnection,
                    (IClusterQuery)runtime.GetService(typeof(IClusterQuery)),
                    token);

                foreach (var oneInsightType in insightTypes)
                {
                    switch (oneInsightType)
                    {
                        case InsightType.PartitionInsight:
                            this.partitionInsightGenerator = new PartitionInsightGenerator(runtime, this.callBackStore, token);
                            await this.partitionInsightGenerator.StartGeneratingAsync().ConfigureAwait(false);
                            break;

                        case InsightType.ReplicaInsight:
                            break;

                        case InsightType.CodePackageInsight:
                            break;

                        case InsightType.NodeInsight:
                            break;

                        case InsightType.ClusterInsight:
                            break;

                        default:
                            throw new NotSupportedException(
                                string.Format(CultureInfo.InvariantCulture, "Insight Type '{0}' is currently not supported", oneInsightType));
                    }
                }

                this.isRunning = true;
            }
            catch
            {
                AgentDirectory.ReleaseInstance();
                throw;
            }
            finally
            {
                this.singleAccess.Release();
            }
        }

        public void AddConsumer(InsightType insightType, IAnalysisConsumer analysisConsumer, ConsumeOptions options)
        {
            switch (insightType)
            {
                case InsightType.PartitionInsight:
                    if (this.partitionInsightGenerator != null)
                    {
                        this.partitionInsightGenerator.AddConsumer(analysisConsumer, options);
                    }

                    break;

                case InsightType.ReplicaInsight:
                    break;

                case InsightType.CodePackageInsight:
                    break;

                case InsightType.NodeInsight:
                    break;

                case InsightType.ClusterInsight:
                    break;

                default:
                    throw new NotSupportedException(string.Format(CultureInfo.InvariantCulture, "Insight Type '{0}' is currently not supported", insightType));
            }
        }

        public bool IsRunning()
        {
            return this.isRunning;
        }

        public async Task StopAnalysisEngineAsync(StopOptions stopOptions, CancellationToken token)
        {
            await this.singleAccess.WaitAsync(token).ConfigureAwait(false);

            try
            {
                if (this.partitionInsightGenerator != null)
                {
                    await this.partitionInsightGenerator.StopGeneratingAsync(stopOptions);
                    this.partitionInsightGenerator = null;
                }

                if (this.nodeInsightGenerator != null)
                {
                    await this.nodeInsightGenerator.StopGeneratingAsync(stopOptions);
                    this.nodeInsightGenerator = null;
                }

                if (this.clusterInsightGenerator != null)
                {
                    await this.clusterInsightGenerator.StopGeneratingAsync(stopOptions);
                    this.clusterInsightGenerator = null;
                }

                if (stopOptions.HasFlag(StopOptions.RemoveAllState) || stopOptions.HasFlag(StopOptions.RemoveTemporaryState))
                {
                    await this.callBackStore.ResetCheckpointAsync(token).ConfigureAwait(false);
                    this.callBackStore = null;
                }

                // Release the Agent Directory Instance.
                AgentDirectory.ReleaseInstance();

                this.isRunning = false;
            }
            finally
            {
                this.singleAccess.Release();
            }
        }

        public void Dispose()
        {
            this.singleAccess.Dispose();
            GC.SuppressFinalize(this);
        }
    }
}
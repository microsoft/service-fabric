// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.ClusterAnalysis
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Threading;
    using System.Threading.Tasks;
    using global::ClusterAnalysis.AnalysisCore;
    using global::ClusterAnalysis.AnalysisCore.Insights;
    using global::ClusterAnalysis.AnalysisCore.Insights.Agents;
    using global::ClusterAnalysis.Common.Log;
    using global::ClusterAnalysis.Common.Runtime;

    internal class ClusterAnalysisRunner
    {
        private const string ClusterAnalyzerRunnerTraceIdentifier = "ClusterAnalysisRunner";

        private CancellationTokenSource stopCancellationTokenSource;

        private ILogger logger;

        private ClusterAnalysisEngine analysisEngine;

        internal ClusterAnalysisRunner(ILogProvider logProvider)
        {
            ReleaseAssert.AssertIfNull(logProvider, "logProvider != null");
            this.logger = logProvider.CreateLoggerInstance(ClusterAnalyzerRunnerTraceIdentifier);
            this.analysisEngine = new ClusterAnalysisEngine();
        }

        public async Task StartClusterAnalysisAsync(IInsightRuntime runtime, IList<IAnalysisConsumer> consumers, CancellationToken token)
        {
            // Create a linked token source.
            this.stopCancellationTokenSource = CancellationTokenSource.CreateLinkedTokenSource(token);

            this.logger.LogMessage("Creating Generators");

            try
            {
                var insightTypesToLoad = new List<InsightType>
                {
                    InsightType.ClusterInsight,
                    InsightType.PartitionInsight,
                    InsightType.CodePackageInsight,
                    InsightType.ReplicaInsight
                };

                await this.analysisEngine.StartAnalysisEngineAsync(insightTypesToLoad, runtime, token).ConfigureAwait(false);

                this.logger.LogMessage("Created Generators");

                foreach (var oneConsumer in consumers)
                {
                    foreach (var oneInsightType in insightTypesToLoad)
                    {
                        this.analysisEngine.AddConsumer(oneInsightType, oneConsumer, ConsumeOptions.Finished);
                    }
                }
            }
            catch (Exception e)
            {
                this.logger.LogError("StartClusterAnalysisAsync:: Encountered Exception {0} while launching Cluster Analysis.", e);

                this.analysisEngine.StopAnalysisEngineAsync(StopOptions.StopAnalysis, token).GetAwaiter().GetResult();

                throw;
            }
        }

        public async Task StopClusterAnalysisAsync(StopOptions stopOptions, CancellationToken token)
        {
            try
            {
                await this.analysisEngine.StopAnalysisEngineAsync(stopOptions, token).ConfigureAwait(false);

                // Signal the cancellation token
                this.stopCancellationTokenSource.Cancel();
            }
            catch (OperationCanceledException exp)
            {
                this.logger.LogWarning("Op Cancelled Exception: {0}", exp);
            }
            catch (Exception exp)
            {
                this.logger.LogError("Task Died with Exception: {0}", exp);
                throw;
            }
        }
    }
}
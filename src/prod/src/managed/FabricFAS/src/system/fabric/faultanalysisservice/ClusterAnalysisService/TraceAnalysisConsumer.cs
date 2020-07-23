// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.ClusterAnalysis
{
    using System;
    using System.Fabric.Common.Tracing;
    using System.Fabric.FaultAnalysis.Service.ClusterAnalysis.Services;
    using System.Threading;
    using System.Threading.Tasks;
    using global::ClusterAnalysis.AnalysisCore.AnalysisEvents;
    using global::ClusterAnalysis.AnalysisCore.Insights;
    using global::ClusterAnalysis.Common.Log;

    internal class TraceAnalysisConsumer : IAnalysisConsumer
    {
        private const string AnalysisAvailableIdentifier = "AnalysisAvailable";

        private ILogger logger;

        public TraceAnalysisConsumer()
        {
            this.logger = ClusterAnalysisLogProvider.LogProvider.CreateLoggerInstance(AnalysisAvailableIdentifier);
        }

        public Task ConsumeAsync(FabricAnalysisEvent analysisDetails, CancellationToken token)
        {
            if (analysisDetails == null)
            {
                throw new ArgumentNullException("analysisDetails");
            }

            AnalysisEventTraceWriter.Instance.WriteTraceEvent(analysisDetails);

            return Task.FromResult(true);
        }
    }
}
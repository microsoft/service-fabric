// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights
{
    using System.Threading;
    using System.Threading.Tasks;
    using ClusterAnalysis.AnalysisCore.AnalysisEvents;

    /// <summary>
    /// Define a contract for consumers of analysis.
    /// </summary>
    public interface IAnalysisConsumer
    {
        /// <summary>
        /// Consume an Analysis event
        /// </summary>
        /// <param name="analysisEvent"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        Task ConsumeAsync(FabricAnalysisEvent analysisEvent, CancellationToken token);
    }
}
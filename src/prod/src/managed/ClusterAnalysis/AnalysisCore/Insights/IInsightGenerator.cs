// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore
{
    using System;
    using System.Threading.Tasks;
    using ClusterAnalysis.AnalysisCore.Insights;
    using ClusterAnalysis.AnalysisCore.Insights.Agents;

    /// <summary>
    /// Defines contract for Insight generators.
    /// </summary>
    public interface IInsightGenerator
    {
        /// <summary>
        /// Find out till what time Insight is available
        /// </summary>
        /// <remarks>
        /// Insight can be returned while while some analysis is still going on. The time returned by this routine
        /// is an indication of till what time insight is available but the insight may not be complete yet.
        /// </remarks>
        /// <returns>time in UTC till which Insight is available</returns>
        DateTime InsightAvailableTillTime();

        /// <summary>
        /// Get Type of Insights this object generates
        /// </summary>
        /// <returns>An object of <see cref="InsightType"/></returns>
        InsightType GetInsightType();

        /// <summary>
        /// Start Generating Insights
        /// </summary>
        /// <returns></returns>
        Task StartGeneratingAsync();

        /// <summary>
        /// Stop Generating Insight and StopGeneratingAsync instance..
        /// </summary>
        /// <param name="stopOption">Options available for stop operation</param>
        /// <returns></returns>
        Task StopGeneratingAsync(StopOptions stopOption);
    }
}
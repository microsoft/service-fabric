// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights.Agents
{
    /// <summary>
    /// Status of an Analysis
    /// </summary>
    public enum AnalysisStatus
    {
        /// <summary>
        /// Analysis hasn't been started yet.
        /// </summary>
        NotStarted,

        /// <summary>
        /// Analysis is queued in Compute Queue
        /// </summary>
        Queued,

        /// <summary>
        /// Analysis is currently in progress.
        /// </summary>
        InProgress,

        /// <summary>
        /// Analysis is complete
        /// </summary>
        Completed,

        /// <summary>
        /// Analysis is currently suspended and will be resumed.
        /// </summary>
        Suspended,

        /// <summary>
        /// Analysis Failed
        /// </summary>
        Failed
    }
}
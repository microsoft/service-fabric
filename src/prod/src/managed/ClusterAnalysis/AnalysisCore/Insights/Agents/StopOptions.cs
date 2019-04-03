// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights.Agents
{
    using System;

    [Flags]
    public enum StopOptions
    {
        /// <summary>
        ///  Implies that just stop the analysis and no data cleaning.
        /// </summary>
        StopAnalysis = 1,

        /// <summary>
        /// Force stop
        /// </summary>
        ForceStop = 2,

        /// <summary>
        /// Remove All temporary State
        /// </summary>
        RemoveTemporaryState = 4,

        /// <summary>
        /// Remove all Information which includes any temporary state (e.g. checkpoints, unfinished analysis, unprocessed signals), as well as any finished analysis.
        /// </summary>
        RemoveAllState = 8
    }
}
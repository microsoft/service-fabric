// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights
{
    /// <summary>
    /// Define different consumption options.
    /// </summary>
    public enum ConsumeOptions
    {
        /// <summary>
        /// Include all finished analysis, irrespective of they failed/passed
        /// </summary>
        Finished,

        /// <summary>
        /// Only include the ones which are successfully done
        /// </summary>
        OnlySuccessfullyFinished,

        /// <summary>
        /// Only include failed Analysis.
        /// </summary>
        OnlyFailed
    }
}
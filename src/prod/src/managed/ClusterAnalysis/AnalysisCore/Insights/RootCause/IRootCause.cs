// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.RootCause
{
    /// <summary>
    /// Coming Soon.
    /// </summary>
    public interface IRootCause
    {
        /// <summary>
        /// Impact of this root cause
        /// </summary>
        string Impact { get; }

        /// <summary>
        /// Get user friendly summary of root cause
        /// </summary>
        /// <returns></returns>
        string GetUserFriendlySummary();
    }
}
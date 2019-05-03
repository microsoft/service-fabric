// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights.Agents
{
    /// <summary>
    /// Type of Agent
    /// </summary>
    public enum AgentTypes
    {
        /// <summary>
        /// Agent will be triggered periodically.
        /// </summary>
        TimeTriggered = 0,

        /// <summary>
        /// Agent will be triggered when the signal agent is interested in, is seen.
        /// </summary>
        SignalTriggered
    }
}
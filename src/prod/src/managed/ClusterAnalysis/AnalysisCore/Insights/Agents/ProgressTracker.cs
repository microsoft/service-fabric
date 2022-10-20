// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights.Agents
{
    /// <summary>
    /// Track progress by creating checkpoints.
    /// These are some pre defined checkpoints which analysis agent can use in 
    /// whatever way they want.
    /// </summary>
    public enum ProgressTracker
    {
        NotStarted = 0,

        Checkpoint1,

        Checkpoint2,

        Checkpoint3,

        Checkpoint4,

        Checkpoint5,

        Checkpoint6,

        Finished
    }
}
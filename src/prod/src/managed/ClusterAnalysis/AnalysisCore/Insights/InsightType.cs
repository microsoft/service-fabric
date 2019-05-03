// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights
{
    /// <summary>
    /// Defines the different types of Insights
    /// </summary>
    public enum InsightType
    {
        /// <summary>
        /// Insight on a partition
        /// </summary>
        PartitionInsight,

        /// <summary>
        /// Insight for a Replica
        /// </summary>
        ReplicaInsight,

        /// <summary>
        /// Code Package Insight
        /// </summary>
        CodePackageInsight,

        /// <summary>
        /// Node Level Insight
        /// </summary>
        NodeInsight,

        /// <summary>
        /// Cluster level Insight
        /// </summary>
        ClusterInsight
    }
}
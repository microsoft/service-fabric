// ----------------------------------------------------------------------
//  <copyright file="PrimaryReplicaContextConstants.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights.PartitionInsight
{
    using System;
    using ClusterAnalysis.Common.Store;

    internal static class PrimaryReplicaContextConstants
    {
        public const string PrimaryReplicaContextStoreId = "PrimaryReplicaContextStore-27C9A967-0CB2-4761-BC0C-32EDB0645A10";
        public static readonly DataRetentionPolicy PrimaryReplicaContextPersistentStoreRetentionPolicy = AgeBasedRetentionPolicy.TwoWeek;
        public static readonly long TraceReadTimeSpanMaxTicks = TimeSpan.FromMinutes(10.0).Ticks;
    }
}
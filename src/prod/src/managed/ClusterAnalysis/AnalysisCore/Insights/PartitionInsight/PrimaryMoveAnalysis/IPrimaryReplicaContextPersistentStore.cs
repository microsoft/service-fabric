// ----------------------------------------------------------------------
//  <copyright file="IPrimaryReplicaContextPersistentStore.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights.PartitionInsight
{
    using System;
    using System.Threading.Tasks;

    internal interface IPrimaryReplicaContextPersistentStore
    {
        Task<bool> IsPartitionIdKnownAsync(Guid partitionId);

        Task SavePrimaryReplicaContextAsync(PrimaryReplicaContext context);

        Task<PrimaryReplicaContext> GetPrimaryReplicaContextAsync(Guid partitionId);
    }
}
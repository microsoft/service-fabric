// ----------------------------------------------------------------------
//  <copyright file="IPrimaryMoveAnalysisEventStoreReader.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights.PartitionInsight
{
    using System;
    using System.Collections.Generic;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.FM;

    internal interface IPrimaryMoveAnalysisEventStoreReader
    {
        Task<IEnumerable<NodeDownTraceRecord>> GetNodeDownTraceRecordsAsync(PrimaryMoveAnalysisEvent analysisEvent);

        Task<IEnumerable<NodeUpTraceRecord>> GetNodeUpTraceRecordsAsync(PrimaryMoveAnalysisEvent analysisEvent);
    }
}
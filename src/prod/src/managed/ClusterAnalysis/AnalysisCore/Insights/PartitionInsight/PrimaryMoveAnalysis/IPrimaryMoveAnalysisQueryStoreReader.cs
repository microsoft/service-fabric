// ----------------------------------------------------------------------
//  <copyright file="IPrimaryMoveAnalysisQueryStoreReader.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights.PartitionInsight
{
    using System;
    using System.Collections.Generic;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Client;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.CRM;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Hosting;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.RA;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.RAP;

    internal interface IPrimaryMoveAnalysisQueryStoreReader
    {
        Task<IEnumerable<ReplicaStateChangeTraceRecord>> GetReplicaStateChangeTraceRecordsAsync(PrimaryMoveAnalysisEvent analysisEvent);

        Task<IEnumerable<ApplicationHostTerminatedTraceRecord>> GetApplicationHostTerminatedTraceRecordsAsync(PrimaryMoveAnalysisEvent analysisEvent);

        Task<IEnumerable<ApiReportFaultTraceRecord>> GetApiReportFaultTraceRecordsAsync(PrimaryMoveAnalysisEvent analysisEvent);

        Task<IEnumerable<BeginReportFaultTraceRecord>> GetBeginReportFaultTraceRecordsAsync(PrimaryMoveAnalysisEvent analysisEvent);

        Task<IEnumerable<ReportFaultTraceRecord>> GetReportFaultTraceRecordsAsync(PrimaryMoveAnalysisEvent analysisEvent);

        Task<IEnumerable<OperationTraceRecord>> GetEffectiveOperationTraceRecordsAsync(PrimaryMoveAnalysisEvent analysisEvent);
    }
}
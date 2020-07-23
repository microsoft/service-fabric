// ----------------------------------------------------------------------
//  <copyright file="PrimaryMoveAnalysisEventStoreReaderWrapper.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights.PartitionInsight
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using ClusterAnalysis.Common.Log;
    using ClusterAnalysis.TraceAccessLayer;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.FM;
    [SuppressMessage("StyleCop.CSharp.ReadabilityRules", "SA1118:ParameterMustNotSpanMultipleLines", Justification = "Reviewed.")]
    [SuppressMessage("StyleCop.CSharp.SpacingRules", "SA1009:ClosingParenthesisMustBeSpacedCorrectly", Justification = "Reviewed.")]
    [SuppressMessage("StyleCop.CSharp.ReadabilityRules", "SA1111:ClosingParenthesisMustBeOnLineOfLastParameter", Justification = "Reviewed.")]
    [SuppressMessage("Microsoft.StyleCop.CSharp.DocumentationRules", "SA1600:ElementsMustBeDocumented", Justification = "Reviewed.")]
    [SuppressMessage("StyleCop.CSharp.LayoutRules", "SA1513:ClosingCurlyBracketMustBeFollowedByBlankLine", Justification = "Reviewed.")]
    internal sealed class PrimaryMoveAnalysisEventStoreReaderWrapper : IPrimaryMoveAnalysisEventStoreReader
    {
        private readonly ITraceStoreReader eventStoreReader;
        private readonly CancellationToken cancellationToken;
        private readonly ILogger logger;

        public PrimaryMoveAnalysisEventStoreReaderWrapper(
            ILogger logger,
            ITraceStoreReader eventStoreReader,
            CancellationToken cancellationToken)
        {
            this.eventStoreReader = eventStoreReader;
            this.cancellationToken = cancellationToken;
            this.logger = logger;
        }

        public async Task<IEnumerable<NodeDownTraceRecord>> GetNodeDownTraceRecordsAsync(PrimaryMoveAnalysisEvent analysisEvent)
        {
            var records = await this.eventStoreReader.ReadTraceRecordsAsync(
                analysisEvent.GetDuration(),
                ReadFilter.CreateReadFilter(
                    new[]
                    {
                        typeof(NodeDownTraceRecord)
                    }
                ),
                this.cancellationToken).ConfigureAwait(false);

            var interstingRecords = records.Select(item => item as NodeDownTraceRecord).Where(item => item.NodeName.Equals(analysisEvent.PreviousPrimaryContext.NodeName, StringComparison.OrdinalIgnoreCase));

            return interstingRecords;
        }

        public async Task<IEnumerable<NodeUpTraceRecord>> GetNodeUpTraceRecordsAsync(PrimaryMoveAnalysisEvent analysisEvent)
        {
            var records = await this.eventStoreReader.ReadTraceRecordsAsync(
                analysisEvent.GetDuration(),
                ReadFilter.CreateReadFilter(
                    new[]
                    {
                        typeof(NodeUpTraceRecord)
                    }
                ),
                this.cancellationToken).ConfigureAwait(false);

            var interstingRecords = records.Select(item => item as NodeUpTraceRecord).Where(item => item.NodeName.Equals(analysisEvent.PreviousPrimaryContext.NodeName, StringComparison.OrdinalIgnoreCase));

            return interstingRecords;
        }
    }
}
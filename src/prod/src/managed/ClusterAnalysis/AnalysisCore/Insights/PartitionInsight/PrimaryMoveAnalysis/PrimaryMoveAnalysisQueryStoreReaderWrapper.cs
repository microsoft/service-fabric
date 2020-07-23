// ----------------------------------------------------------------------
//  <copyright file="PrimaryMoveAnalysisQueryStoreReaderWrapper.cs" company="Microsoft">
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
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Client;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.CRM;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.FM;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Hosting;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.RA;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.RAP;
    [SuppressMessage("StyleCop.CSharp.ReadabilityRules", "SA1118:ParameterMustNotSpanMultipleLines", Justification = "Reviewed.")]
    [SuppressMessage("StyleCop.CSharp.SpacingRules", "SA1009:ClosingParenthesisMustBeSpacedCorrectly", Justification = "Reviewed.")]
    [SuppressMessage("StyleCop.CSharp.ReadabilityRules", "SA1111:ClosingParenthesisMustBeOnLineOfLastParameter", Justification = "Reviewed.")]
    [SuppressMessage("Microsoft.StyleCop.CSharp.DocumentationRules", "SA1600:ElementsMustBeDocumented", Justification = "Reviewed.")]
    [SuppressMessage("StyleCop.CSharp.LayoutRules", "SA1513:ClosingCurlyBracketMustBeFollowedByBlankLine", Justification = "Reviewed.")]
    internal sealed class PrimaryMoveAnalysisQueryStoreReaderWrapper : IPrimaryMoveAnalysisQueryStoreReader
    {
        private readonly ITraceStoreReader queryStoreReader;
        private readonly CancellationToken cancellationToken;
        private readonly ILogger logger;

        public PrimaryMoveAnalysisQueryStoreReaderWrapper(
            ILogger logger,
            ITraceStoreReader queryStoreReader,
            CancellationToken cancellationToken)
        {
            this.queryStoreReader = queryStoreReader;
            this.cancellationToken = cancellationToken;
            this.logger = logger;
        }

        public async Task<IEnumerable<ApiReportFaultTraceRecord>> GetApiReportFaultTraceRecordsAsync(PrimaryMoveAnalysisEvent analysisEvent)
        {
            var records = await this.queryStoreReader.ReadTraceRecordsAsync(
                analysisEvent.GetDuration(),
                ReadFilter.CreateReadFilter(
                    new[]
                    {
                        typeof(ApiReportFaultTraceRecord)
                    }
                ),
                this.cancellationToken).ConfigureAwait(false);

            var interstingRecords = records.Select(item => item as ApiReportFaultTraceRecord).Where(item => item.ReasonActivityId == analysisEvent.ReasonActivityId);

            return interstingRecords;
        }

        public async Task<IEnumerable<ApplicationHostTerminatedTraceRecord>> GetApplicationHostTerminatedTraceRecordsAsync(PrimaryMoveAnalysisEvent analysisEvent)
        {
            var records = await this.queryStoreReader.ReadTraceRecordsAsync(
                analysisEvent.GetDuration(),
                ReadFilter.CreateReadFilter(
                    new[] 
                    {
                        typeof(ApplicationHostTerminatedTraceRecord)
                    }
                ),
                this.cancellationToken).ConfigureAwait(false);

            var interstingRecords = records.Select(item => item as ApplicationHostTerminatedTraceRecord).Where(item => item.ReasonActivityId == analysisEvent.ReasonActivityId);

            return interstingRecords;
        }

        public async Task<IEnumerable<BeginReportFaultTraceRecord>> GetBeginReportFaultTraceRecordsAsync(PrimaryMoveAnalysisEvent analysisEvent)
        {
            var records = await this.queryStoreReader.ReadTraceRecordsAsync(
                analysisEvent.GetDuration(),
                ReadFilter.CreateReadFilter(
                    new[]
                    {
                        typeof(BeginReportFaultTraceRecord)
                    }
                ),
                this.cancellationToken).ConfigureAwait(false);

            var interstingRecords = records.Select(item => item as BeginReportFaultTraceRecord).Where(item => item.BodyActivityId == analysisEvent.ReasonActivityId);

            return interstingRecords;
        }

        public async Task<IEnumerable<ReportFaultTraceRecord>> GetReportFaultTraceRecordsAsync(PrimaryMoveAnalysisEvent analysisEvent)
        {
            var records = await this.queryStoreReader.ReadTraceRecordsAsync(
                analysisEvent.GetDuration(),
                ReadFilter.CreateReadFilter(
                    new[]
                    {
                        typeof(ReportFaultTraceRecord)
                    }
                ),
                this.cancellationToken).ConfigureAwait(false);

            var interstingRecords = records.Select(item => item as ReportFaultTraceRecord).Where(item => item.ReasonActivityId == analysisEvent.ReasonActivityId);

            return interstingRecords;
        }

        public async Task<IEnumerable<NodeDownTraceRecord>> GetNodeDownTraceRecordsAsync(PrimaryMoveAnalysisEvent analysisEvent)
        {
            var records = await this.queryStoreReader.ReadTraceRecordsAsync(
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
            var records = await this.queryStoreReader.ReadTraceRecordsAsync(
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

        public async Task<IEnumerable<OperationTraceRecord>> GetEffectiveOperationTraceRecordsAsync(PrimaryMoveAnalysisEvent analysisEvent)
        {
            var records = await this.queryStoreReader.ReadTraceRecordsAsync(
                analysisEvent.GetDuration(),
                ReadFilter.CreateReadFilter(
                    new[] 
                    {
                        typeof(OperationTraceRecord),
                        typeof(OperationIgnoredTraceRecord)
                    }
                ),
                this.cancellationToken).ConfigureAwait(false);

            HashSet<Guid> ignoredDecisions = new HashSet<Guid>();
            Dictionary<Guid, OperationTraceRecord> decisionOperationMap = new Dictionary<Guid, OperationTraceRecord>();
            List<OperationTraceRecord> effectiveOperationRecords = new List<OperationTraceRecord>();

            foreach (var r in records)
            {
                OperationTraceRecord operation = r as OperationTraceRecord;
                if (operation == null)
                {
                    OperationIgnoredTraceRecord ignored = r as OperationIgnoredTraceRecord;

                    if (ignored != null && ignored.FailoverUnitId == analysisEvent.TriggerReconfigurationCompletedTraceRecord.PartitionId)
                    {
                        ignoredDecisions.Add(ignored.DecisionId);
                        if (decisionOperationMap.ContainsKey(ignored.DecisionId))
                        {
                            effectiveOperationRecords.Remove(decisionOperationMap[ignored.DecisionId]);
                        }
                    }
                }
                else if (this.MatchPartitionSourceAndDestination(operation, analysisEvent) && !ignoredDecisions.Contains(operation.DecisionId))
                {
                    effectiveOperationRecords.Add(operation);
                    decisionOperationMap[operation.DecisionId] = operation;
                }
            }

            var effectiveOperationRecordsMatchingSourceTarget = effectiveOperationRecords.Where(r => r.SourceNode.StartsWith(analysisEvent.PreviousNode) && r.TargetNode.StartsWith(analysisEvent.CurrentNode));

            return effectiveOperationRecords;
        }

        public async Task<IEnumerable<ReplicaStateChangeTraceRecord>> GetReplicaStateChangeTraceRecordsAsync(PrimaryMoveAnalysisEvent analysisEvent)
        {
            var records = await this.queryStoreReader.ReadTraceRecordsAsync(
                analysisEvent.GetDuration(),
                ReadFilter.CreateReadFilter(
                    new[] 
                    {
                        typeof(ReplicaStateChangeTraceRecord)
                    }
                ),
                this.cancellationToken).ConfigureAwait(false);

            var interstingRecords = records.Select(item => item as ReplicaStateChangeTraceRecord).Where(item => item.PartitionId == analysisEvent.TriggerReconfigurationCompletedTraceRecord.PartitionId && item.Role == global::ReplicaRole.P && item.Status == ReplicaLifeCycleState.Closing && item.NodeInstanceId.StartsWith(analysisEvent.PreviousPrimaryContext.NodeId) && item.ReasonActivityId != Guid.Empty);

            return interstingRecords;
        }

        private bool MatchPartitionSourceAndDestination(OperationTraceRecord operation, PrimaryMoveAnalysisEvent analysisEvent)
        {
            return operation.PartitionId == analysisEvent.TriggerReconfigurationCompletedTraceRecord.PartitionId && operation.SourceNode.Equals(analysisEvent.PreviousPrimaryContext.NodeId, StringComparison.OrdinalIgnoreCase) && operation.TargetNode.Equals(analysisEvent.CurrentPrimaryContext.NodeId, StringComparison.OrdinalIgnoreCase);
        }
    }
}
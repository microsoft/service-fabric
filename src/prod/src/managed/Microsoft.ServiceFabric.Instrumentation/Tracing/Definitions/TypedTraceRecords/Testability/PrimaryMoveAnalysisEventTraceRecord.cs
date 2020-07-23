// ----------------------------------------------------------------------
//  <copyright file="PrimaryMoveAnalysisEventTraceRecord.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability
{
    using System;
    using System.Globalization;
    using System.Runtime.Serialization;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    [DataContract]
    public sealed class PrimaryMoveAnalysisEventTraceRecord : StronglyTypedTraceRecord, IEquatable<PrimaryMoveAnalysisEventTraceRecord>
    {
        private Func<PrimaryMoveAnalysisEventTraceRecord, CancellationToken, Task> onOccurrence;

        public PrimaryMoveAnalysisEventTraceRecord(Func<PrimaryMoveAnalysisEventTraceRecord, CancellationToken, Task> onOccurenceAction) : base(
            65003,
            TaskName.Testability)
        {
           this.onOccurrence = onOccurenceAction;
        }

        [TraceField(0, OriginalName = "whenMoveCompleted")]
        public DateTime WhenMoveCompleted
        {
            get { return this.PropertyValueReader.ReadDateTimeAt(0); }
        }

        [TraceField(1, OriginalName = "analysisDelayInSeconds")]
        public double AnalysisDelayInSeconds
        {
            get { return this.PropertyValueReader.ReadDoubleAt(1); }
        }

        [TraceField(2, OriginalName = "analysisDurationInSeconds")]
        public double AnalysisDurationInSeconds
        {
            get { return this.PropertyValueReader.ReadDoubleAt(2); }
        }

        [TraceField(3, OriginalName = "partitionId")]
        public Guid PartitionId
        {
            get { return this.PropertyValueReader.ReadGuidAt(3); }
        }

        [TraceField(4, OriginalName = "previousNode")]
        public string PreviousNode
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(4); }
        }

        [TraceField(5, OriginalName = "currentNode")]
        public string CurrentNode
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(5); }
        }

        [TraceField(6, OriginalName = "reason")]
        public string Reason
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(6); }
        }

        [TraceField(7, OriginalName = "correlatedTraceRecords")]
        public string CorrelatedTraceRecords
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(7); }
        }

        public override Task DispatchAsync(CancellationToken token)
        {
            return this.onOccurrence?.Invoke(this, token);
        }

        public override Delegate Target
        {
            get { return this.onOccurrence; }
            protected set { this.onOccurrence = (Func<PrimaryMoveAnalysisEventTraceRecord, CancellationToken, Task>)value; }
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
               "TimeStamp : {0}, ProcessId : {1}, ThreadId : {2}, Level : {3}, WhenMoveCompleted : {4}, AnalysisDelayInSeconds : {5}, AnalysisDurationInSeconds : {6}, PartitionId : {7}, PreviousNode : {8}, CurrentNode : {9}, Reason : {10}, CorrelatedTraceRecords : {11}",
                this.TimeStamp,
                this.ProcessId,
                this.ThreadId,
                this.Level,
                this.WhenMoveCompleted,
                this.AnalysisDelayInSeconds,
                this.AnalysisDurationInSeconds,
                this.PartitionId,
                this.PreviousNode,
                this.CurrentNode,
                this.Reason,
                this.CorrelatedTraceRecords);
        }

        public override bool Equals(object right)
        {
            if (ReferenceEquals(right, null))
            {
                return false;
            }

            if (ReferenceEquals(this, right))
            {
                return true;
            }

            if (this.GetType() != right.GetType())
            {
                return false;
            }

            return this.Equals(right as PrimaryMoveAnalysisEventTraceRecord);
        }

        public bool Equals(PrimaryMoveAnalysisEventTraceRecord other)
        {
            return this.TimeStamp == other.TimeStamp
                   && this.ProcessId == other.ProcessId
                   && this.ThreadId == other.ThreadId
                   && this.Level == other.Level
                   && this.PartitionId == other.PartitionId
                   && this.PreviousNode.Equals(other.PreviousNode, StringComparison.Ordinal);
        }

        public override int GetHashCode()
        {
            return this.TimeStamp.GetHashCode()
                ^ this.ProcessId.GetHashCode()
                ^ this.ThreadId.GetHashCode()
                ^ this.Level.GetHashCode()
                ^ this.PartitionId.GetHashCode()
                ^ this.PreviousNode.GetHashCode()
                ^ this.CurrentNode.GetHashCode();
        }
    }
}


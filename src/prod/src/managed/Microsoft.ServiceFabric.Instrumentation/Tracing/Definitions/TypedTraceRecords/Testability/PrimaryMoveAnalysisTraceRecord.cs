// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability
{
    using System;
    using System.Globalization;
    using System.Runtime.Serialization;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    [DataContract]
    public sealed class PrimaryMoveAnalysisTraceRecord : StronglyTypedTraceRecord
    {
        private Func<PrimaryMoveAnalysisTraceRecord, CancellationToken, Task> onOccurrence;

        public PrimaryMoveAnalysisTraceRecord(Func<PrimaryMoveAnalysisTraceRecord, CancellationToken, Task> onOccurenceAction) : base(
            65003,
            TaskName.Testability)
        {
           this.onOccurrence = onOccurenceAction;
        }

        public override InstanceIdentity ObjectInstanceId
        {
           get { return new InstanceIdentity(this.EventInstanceId); }
        }

        [TraceField(index : 0, version : 2, OriginalName = "eventName", DefaultValue = "NotAvailable")]
        public string EventName
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(0); }
        }

        [TraceField(index: 1, version: 2, OriginalName = "category", DefaultValue = "Default")]
        public string Category
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(1); }
        }

        [TraceField(2, OriginalName = "eventInstanceId")]
        public Guid EventInstanceId
        {
            get { return this.PropertyValueReader.ReadGuidAt(2); }
        }

        [TraceField(3, OriginalName = "partitionId")]
        public Guid PartitionId
        {
            get { return this.PropertyValueReader.ReadGuidAt(3); }
        }

        [TraceField(4, OriginalName = "whenMoveCompleted")]
        public DateTime WhenMoveCompleted
        {
            get { return this.PropertyValueReader.ReadDateTimeAt(4); }
        }

        [TraceField(5, OriginalName = "analysisDelayInSeconds")]
        public double AnalysisDelayInSeconds
        {
            get { return this.PropertyValueReader.ReadDoubleAt(5); }
        }

        [TraceField(6, OriginalName = "analysisDurationInSeconds")]
        public double AnalysisDurationInSeconds
        {
            get { return this.PropertyValueReader.ReadDoubleAt(6); }
        }

        [TraceField(7, OriginalName = "previousNode")]
        public string PreviousNode
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(7); }
        }

        [TraceField(8, OriginalName = "currentNode")]
        public string CurrentNode
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(8); }
        }

        [TraceField(9, OriginalName = "reason")]
        public string Reason
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(9); }
        }

        [TraceField(10, OriginalName = "correlatedTraceRecords")]
        public string CorrelatedTraceRecords
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(10); }
        }

        public override Task DispatchAsync(CancellationToken token)
        {
            return this.onOccurrence?.Invoke(this, token);
        }

        public override Delegate Target
        {
            get { return this.onOccurrence; }
            protected set { this.onOccurrence = (Func<PrimaryMoveAnalysisTraceRecord, CancellationToken, Task>)value; }
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
               "TimeStamp : {0}, TracingProcessId : {1}, ThreadId : {2}, Level : {3}, EventName : {4}, Category : {5}, EventInstanceId : {6}, PartitionId : {7}, WhenMoveCompleted : {8}, AnalysisDelayInSeconds : {9}, AnalysisDurationInSeconds : {10}, PreviousNode : {11}, CurrentNode : {12}, Reason : {13}, CorrelatedTraceRecords : {14}",
                this.TimeStamp,
                this.TracingProcessId,
                this.ThreadId,
                this.Level,
                this.EventName,
                this.Category,
                this.EventInstanceId,
                this.PartitionId,
                this.WhenMoveCompleted,
                this.AnalysisDelayInSeconds,
                this.AnalysisDurationInSeconds,
                this.PreviousNode,
                this.CurrentNode,
                this.Reason,
                this.CorrelatedTraceRecords);
        }
    }
}

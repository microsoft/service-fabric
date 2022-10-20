// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.HM
{
    using System;
    using System.Globalization;
    using System.Runtime.Serialization;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    [DataContract]
    public sealed class PartitionHealthReportCreatedTraceRecord : StronglyTypedTraceRecord
    {
        private Func<PartitionHealthReportCreatedTraceRecord, CancellationToken, Task> onOccurrence;

        public PartitionHealthReportCreatedTraceRecord(Func<PartitionHealthReportCreatedTraceRecord, CancellationToken, Task> onOccurenceAction) : base(
            54422,
            TaskName.HM)
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

        [TraceField(4, OriginalName = "sourceId")]
        public string SourceId
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(4); }
        }

        [TraceField(5, OriginalName = "property")]
        public string Property
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(5); }
        }

        [TraceField(6, OriginalName = "healthState")]
        public HealthState HealthState
        {
            get { return (HealthState)this.PropertyValueReader.ReadInt32At(6); }
        }

        [TraceField(7, OriginalName = "TTL.timespan")]
        public long TTLTimespan
        {
            get { return this.PropertyValueReader.ReadInt64At(7); }
        }

        [TraceField(8, OriginalName = "sequenceNumber")]
        public long SequenceNumber
        {
            get { return this.PropertyValueReader.ReadInt64At(8); }
        }

        [TraceField(9, OriginalName = "description")]
        public string Description
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(9); }
        }

        [TraceField(10, OriginalName = "removeWhenExpired")]
        public bool RemoveWhenExpired
        {
            get { return this.PropertyValueReader.ReadBoolAt(10); }
        }

        [TraceField(11, OriginalName = "sourceUtcTimestamp")]
        public DateTime SourceUtcTimestamp
        {
            get { return this.PropertyValueReader.ReadDateTimeAt(11); }
        }

        public override Task DispatchAsync(CancellationToken token)
        {
            return this.onOccurrence?.Invoke(this, token);
        }

        public override Delegate Target
        {
            get { return this.onOccurrence; }
            protected set { this.onOccurrence = (Func<PartitionHealthReportCreatedTraceRecord, CancellationToken, Task>)value; }
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
               "TimeStamp : {0}, TracingProcessId : {1}, ThreadId : {2}, Level : {3}, EventName : {4}, Category : {5}, EventInstanceId : {6}, PartitionId : {7}, SourceId : {8}, Property : {9}, HealthState : {10}, TTLTimespan : {11}, SequenceNumber : {12}, Description : {13}, RemoveWhenExpired : {14}, SourceUtcTimestamp : {15}",
                this.TimeStamp,
                this.TracingProcessId,
                this.ThreadId,
                this.Level,
                this.EventName,
                this.Category,
                this.EventInstanceId,
                this.PartitionId,
                this.SourceId,
                this.Property,
                this.HealthState,
                this.TTLTimespan,
                this.SequenceNumber,
                this.Description,
                this.RemoveWhenExpired,
                this.SourceUtcTimestamp);
        }
    }
}

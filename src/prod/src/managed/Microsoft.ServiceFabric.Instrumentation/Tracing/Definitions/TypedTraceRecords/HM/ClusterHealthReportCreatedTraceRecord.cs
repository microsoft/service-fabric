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
    public sealed class ClusterHealthReportCreatedTraceRecord : StronglyTypedTraceRecord
    {
        private Func<ClusterHealthReportCreatedTraceRecord, CancellationToken, Task> onOccurrence;

        public ClusterHealthReportCreatedTraceRecord(Func<ClusterHealthReportCreatedTraceRecord, CancellationToken, Task> onOccurenceAction) : base(
            54428,
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

        [TraceField(3, OriginalName = "sourceId")]
        public string SourceId
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(3); }
        }

        [TraceField(4, OriginalName = "property")]
        public string Property
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(4); }
        }

        [TraceField(5, OriginalName = "healthState")]
        public HealthState HealthState
        {
            get { return (HealthState)this.PropertyValueReader.ReadInt32At(5); }
        }

        [TraceField(6, OriginalName = "TTL.timespan")]
        public long TTLTimespan
        {
            get { return this.PropertyValueReader.ReadInt64At(6); }
        }

        [TraceField(7, OriginalName = "sequenceNumber")]
        public long SequenceNumber
        {
            get { return this.PropertyValueReader.ReadInt64At(7); }
        }

        [TraceField(8, OriginalName = "description")]
        public string Description
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(8); }
        }

        [TraceField(9, OriginalName = "removeWhenExpired")]
        public bool RemoveWhenExpired
        {
            get { return this.PropertyValueReader.ReadBoolAt(9); }
        }

        [TraceField(10, OriginalName = "sourceUtcTimestamp")]
        public DateTime SourceUtcTimestamp
        {
            get { return this.PropertyValueReader.ReadDateTimeAt(10); }
        }

        public override Task DispatchAsync(CancellationToken token)
        {
            return this.onOccurrence?.Invoke(this, token);
        }

        public override Delegate Target
        {
            get { return this.onOccurrence; }
            protected set { this.onOccurrence = (Func<ClusterHealthReportCreatedTraceRecord, CancellationToken, Task>)value; }
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
               "TimeStamp : {0}, TracingProcessId : {1}, ThreadId : {2}, Level : {3}, EventName : {4}, Category : {5}, EventInstanceId : {6}, SourceId : {7}, Property : {8}, HealthState : {9}, TTLTimespan : {10}, SequenceNumber : {11}, Description : {12}, RemoveWhenExpired : {13}, SourceUtcTimestamp : {14}",
                this.TimeStamp,
                this.TracingProcessId,
                this.ThreadId,
                this.Level,
                this.EventName,
                this.Category,
                this.EventInstanceId,
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

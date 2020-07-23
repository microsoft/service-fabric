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
    public sealed class CorrelationTraceRecord : StronglyTypedTraceRecord
    {
        public const string EventInstanceIdPropertyName = "EventInstanceId";
        public const string RelatedFromIdPropertyName = "RelatedFromId";
        public const string RelatedFromTypePropertyName = "RelatedFromType";
        public const string RelatedToIdPropertyName = "RelatedToId";
        public const string RelatedToTypePropertyName = "RelatedToType";

        private Func<CorrelationTraceRecord, CancellationToken, Task> onOccurrence;

        public CorrelationTraceRecord(Func<CorrelationTraceRecord, CancellationToken, Task> onOccurenceAction) : base(
            65011,
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

        [TraceField(3, OriginalName = "relatedFromId")]
        public Guid RelatedFromId
        {
            get { return this.PropertyValueReader.ReadGuidAt(3); }
        }

        [TraceField(4, OriginalName = "relatedFromType")]
        public string RelatedFromType
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(4); }
        }

        [TraceField(5, OriginalName = "relatedToId")]
        public Guid RelatedToId
        {
            get { return this.PropertyValueReader.ReadGuidAt(5); }
        }

        [TraceField(6, OriginalName = "relatedToType")]
        public string RelatedToType
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(6); }
        }

        public override Task DispatchAsync(CancellationToken token)
        {
            return this.onOccurrence?.Invoke(this, token);
        }

        public override Delegate Target
        {
            get { return this.onOccurrence; }
            protected set { this.onOccurrence = (Func<CorrelationTraceRecord, CancellationToken, Task>)value; }
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
               "TimeStamp : {0}, TracingProcessId : {1}, ThreadId : {2}, Level : {3}, EventName : {4}, Category : {5}, EventInstanceId : {6}, RelatedFromId : {7}, RelatedFromType : {8}, RelatedToId : {9}, RelatedToType : {10}",
                this.TimeStamp,
                this.TracingProcessId,
                this.ThreadId,
                this.Level,
                this.EventName,
                this.Category,
                this.EventInstanceId,
                this.RelatedFromId,
                this.RelatedFromType,
                this.RelatedToId,
                this.RelatedToType);
        }
    }
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.FM
{
    using System;
    using System.Globalization;
    using System.Runtime.Serialization;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    [DataContract]
    public sealed class NodeDeactivateCompleteTraceRecord : StronglyTypedTraceRecord
    {
        private Func<NodeDeactivateCompleteTraceRecord, CancellationToken, Task> onOccurrence;

        public NodeDeactivateCompleteTraceRecord(Func<NodeDeactivateCompleteTraceRecord, CancellationToken, Task> onOccurenceAction) : base(
            18602,
            TaskName.FM)
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

        [TraceField(3, OriginalName = "nodeName")]
        public string NodeName
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(3); }
        }

        [TraceField(4, OriginalName = "nodeInstance")]
        public long NodeInstance
        {
            get { return this.PropertyValueReader.ReadInt64At(4); }
        }

        [TraceField(5, OriginalName = "effectiveDeactivateIntent")]
        public NodeDeactivationIntent EffectiveDeactivateIntent
        {
            get { return (NodeDeactivationIntent)this.PropertyValueReader.ReadInt32At(5); }
        }

        [TraceField(6, OriginalName = "batchIdsWithDeactivateIntent")]
        public string BatchIdsWithDeactivateIntent
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(6); }
        }

        [TraceField(7, OriginalName = "startTime")]
        public DateTime StartTime
        {
            get { return this.PropertyValueReader.ReadDateTimeAt(7); }
        }

        public override Task DispatchAsync(CancellationToken token)
        {
            return this.onOccurrence?.Invoke(this, token);
        }

        public override Delegate Target
        {
            get { return this.onOccurrence; }
            protected set { this.onOccurrence = (Func<NodeDeactivateCompleteTraceRecord, CancellationToken, Task>)value; }
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
               "TimeStamp : {0}, TracingProcessId : {1}, ThreadId : {2}, Level : {3}, EventName : {4}, Category : {5}, EventInstanceId : {6}, NodeName : {7}, NodeInstance : {8}, EffectiveDeactivateIntent : {9}, BatchIdsWithDeactivateIntent : {10}, StartTime : {11}",
                this.TimeStamp,
                this.TracingProcessId,
                this.ThreadId,
                this.Level,
                this.EventName,
                this.Category,
                this.EventInstanceId,
                this.NodeName,
                this.NodeInstance,
                this.EffectiveDeactivateIntent,
                this.BatchIdsWithDeactivateIntent,
                this.StartTime);
        }
    }
}

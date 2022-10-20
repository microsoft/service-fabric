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
    public sealed class ChaosMoveSecondaryFaultScheduledTraceRecord : StronglyTypedTraceRecord
    {
        private Func<ChaosMoveSecondaryFaultScheduledTraceRecord, CancellationToken, Task> onOccurrence;

        public ChaosMoveSecondaryFaultScheduledTraceRecord(Func<ChaosMoveSecondaryFaultScheduledTraceRecord, CancellationToken, Task> onOccurenceAction) : base(
            50077,
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

        [TraceField(4, OriginalName = "faultGroupId")]
        public Guid FaultGroupId
        {
            get { return this.PropertyValueReader.ReadGuidAt(4); }
        }

        [TraceField(5, OriginalName = "faultId")]
        public Guid FaultId
        {
            get { return this.PropertyValueReader.ReadGuidAt(5); }
        }

        [TraceField(6, OriginalName = "serviceName")]
        public string ServiceName
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(6); }
        }

        [TraceField(7, OriginalName = "sourceNode")]
        public string SourceNode
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(7); }
        }

        [TraceField(8, OriginalName = "destinationNode")]
        public string DestinationNode
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(8); }
        }

        [TraceField(9, OriginalName = "forcedMove")]
        public bool ForcedMove
        {
            get { return this.PropertyValueReader.ReadBoolAt(9); }
        }

        public override Task DispatchAsync(CancellationToken token)
        {
            return this.onOccurrence?.Invoke(this, token);
        }

        public override Delegate Target
        {
            get { return this.onOccurrence; }
            protected set { this.onOccurrence = (Func<ChaosMoveSecondaryFaultScheduledTraceRecord, CancellationToken, Task>)value; }
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
               "TimeStamp : {0}, TracingProcessId : {1}, ThreadId : {2}, Level : {3}, EventName : {4}, Category : {5}, EventInstanceId : {6}, PartitionId : {7}, FaultGroupId : {8}, FaultId : {9}, ServiceName : {10}, SourceNode : {11}, DestinationNode : {12}, ForcedMove : {13}",
                this.TimeStamp,
                this.TracingProcessId,
                this.ThreadId,
                this.Level,
                this.EventName,
                this.Category,
                this.EventInstanceId,
                this.PartitionId,
                this.FaultGroupId,
                this.FaultId,
                this.ServiceName,
                this.SourceNode,
                this.DestinationNode,
                this.ForcedMove);
        }
    }
}

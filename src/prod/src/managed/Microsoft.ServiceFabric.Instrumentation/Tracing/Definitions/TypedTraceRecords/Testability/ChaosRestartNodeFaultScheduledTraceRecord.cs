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
    public sealed class ChaosRestartNodeFaultScheduledTraceRecord : StronglyTypedTraceRecord
    {
        private Func<ChaosRestartNodeFaultScheduledTraceRecord, CancellationToken, Task> onOccurrence;

        public ChaosRestartNodeFaultScheduledTraceRecord(Func<ChaosRestartNodeFaultScheduledTraceRecord, CancellationToken, Task> onOccurenceAction) : base(
            50033,
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

        [TraceField(3, OriginalName = "nodeName")]
        public string NodeName
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(3); }
        }

        [TraceField(4, OriginalName = "nodeInstanceId")]
        public long NodeInstanceId
        {
            get { return this.PropertyValueReader.ReadInt64At(4); }
        }

        [TraceField(5, OriginalName = "faultGroupId")]
        public Guid FaultGroupId
        {
            get { return this.PropertyValueReader.ReadGuidAt(5); }
        }

        [TraceField(6, OriginalName = "faultId")]
        public Guid FaultId
        {
            get { return this.PropertyValueReader.ReadGuidAt(6); }
        }

        public override Task DispatchAsync(CancellationToken token)
        {
            return this.onOccurrence?.Invoke(this, token);
        }

        public override Delegate Target
        {
            get { return this.onOccurrence; }
            protected set { this.onOccurrence = (Func<ChaosRestartNodeFaultScheduledTraceRecord, CancellationToken, Task>)value; }
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
               "TimeStamp : {0}, TracingProcessId : {1}, ThreadId : {2}, Level : {3}, EventName : {4}, Category : {5}, EventInstanceId : {6}, NodeName : {7}, NodeInstanceId : {8}, FaultGroupId : {9}, FaultId : {10}",
                this.TimeStamp,
                this.TracingProcessId,
                this.ThreadId,
                this.Level,
                this.EventName,
                this.Category,
                this.EventInstanceId,
                this.NodeName,
                this.NodeInstanceId,
                this.FaultGroupId,
                this.FaultId);
        }
    }
}

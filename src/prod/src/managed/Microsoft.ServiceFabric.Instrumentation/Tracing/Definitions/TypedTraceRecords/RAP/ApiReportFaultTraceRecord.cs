// ----------------------------------------------------------------------
//  <copyright file="ApiReportFaultTraceRecord.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.RAP
{
    using System;
    using System.Globalization;
    using System.Runtime.Serialization;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    [DataContract]
    public sealed class ApiReportFaultTraceRecord : StronglyTypedTraceRecord
    {
        private Func<ApiReportFaultTraceRecord, CancellationToken, Task> onOccurrence;

        public ApiReportFaultTraceRecord(Func<ApiReportFaultTraceRecord, CancellationToken, Task> onOccurenceAction) : base(
            19191,
            TaskName.RAP)
        {
           this.onOccurrence = onOccurenceAction;
        }

        [TraceField(0, OriginalName = "id")]
        public Guid PartitionId
        {
            get { return this.PropertyValueReader.ReadGuidAt(0); }
        }

        [TraceField(1, OriginalName = "nodeId")]
        public string NodeId
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(1); }
        }

        [TraceField(2, OriginalName = "replicaId")]
        public long ReplicaId
        {
            get { return this.PropertyValueReader.ReadInt64At(2); }
        }

        [TraceField(3, OriginalName = "replicaInstanceId")]
        public long ReplicaInstanceId
        {
            get { return this.PropertyValueReader.ReadInt64At(3); }
        }

        [TraceField(4, OriginalName = "faultType")]
        public FaultType FaultType
        {
            get { return (FaultType)this.PropertyValueReader.ReadInt32At(4); }
        }

        [TraceField(5, OriginalName = "reasonActivityDescription.activityId.id")]
        public Guid ReasonActivityId
        {
            get { return this.PropertyValueReader.ReadGuidAt(5); }
        }

        [TraceField(6, OriginalName = "reasonActivityDescription.activityId.index")]
        public long ReasonActivityIdIndex
        {
            get { return this.PropertyValueReader.ReadInt64At(6); }
        }

        [TraceField(7, OriginalName = "reasonActivityDescription.activityType")]
        public ActivityType ReasonActivityType
        {
            get { return (ActivityType)this.PropertyValueReader.ReadInt32At(7); }
        }

        public override Task DispatchAsync(CancellationToken token)
        {
            return this.onOccurrence?.Invoke(this, token);
        }

        public override Delegate Target
        {
            get { return this.onOccurrence; }
            protected set { this.onOccurrence = (Func<ApiReportFaultTraceRecord, CancellationToken, Task>)value; }
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
               "{0}, NodeId : {1}, PartitionId : {2}, ReplicaId : {3}, FaultType : {4}",
                base.ToString(),
                this.NodeId,
                this.PartitionId,
                this.ReplicaId,
                this.FaultType);
        }

        public override string ToPublicString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
               "{0}, NodeId : {1}, PartitionId : {2}, ReplicaId : {3}, FaultType : {4}",
                this.ToStringCommonPrefix(),
                this.NodeId,
                this.PartitionId,
                this.ReplicaId,
                this.FaultType);
        }
    }
}


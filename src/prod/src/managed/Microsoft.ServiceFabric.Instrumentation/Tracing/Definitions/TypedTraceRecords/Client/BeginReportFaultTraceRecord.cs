// ----------------------------------------------------------------------
//  <copyright file="BeginReportFaultTraceRecord.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Client
{
    using System;
    using System.Globalization;
    using System.Runtime.Serialization;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    [DataContract]
    public sealed class BeginReportFaultTraceRecord : StronglyTypedTraceRecord
    {
        private Func<BeginReportFaultTraceRecord, CancellationToken, Task> onOccurrence;

        public BeginReportFaultTraceRecord(Func<BeginReportFaultTraceRecord, CancellationToken, Task> onOccurenceAction) : base(
            55657,
            TaskName.Client)
        {
           this.onOccurrence = onOccurenceAction;
        }

        [TraceField(0, OriginalName = "id")]
        public string Id
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(0); }
        }

        [TraceField(1, OriginalName = "activityId.id")]
        public Guid ReasonActivityId
        {
            get { return this.PropertyValueReader.ReadGuidAt(1); }
        }

        [TraceField(2, OriginalName = "activityId.index")]
        public long ReasonActivityIdIndex
        {
            get { return this.PropertyValueReader.ReadInt64At(2); }
        }

        [TraceField(3, OriginalName = "body.node")]
        public string NodeId
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(3); }
        }

        [TraceField(4, OriginalName = "body.guid")]
        public Guid PartitionId
        {
            get { return this.PropertyValueReader.ReadGuidAt(4); }
        }

        [TraceField(5, OriginalName = "body.replica")]
        public long ReplicaId
        {
            get { return this.PropertyValueReader.ReadInt64At(5); }
        }

        [TraceField(6, OriginalName = "body.faultType")]
        public FaultType FaultType
        {
            get { return (FaultType)this.PropertyValueReader.ReadInt32At(6); }
        }

        [TraceField(7, OriginalName = "body.isForce")]
        public bool IsForce
        {
            get { return this.PropertyValueReader.ReadInt32At(7) > 0; }
        }

        [TraceField(8, OriginalName = "body.activityDescription.activityId.id")]
        public Guid BodyActivityId
        {
            get { return this.PropertyValueReader.ReadGuidAt(8); }
        }

        [TraceField(9, OriginalName = "body.activityDescription.activityId.index")]
        public long BodyActivityIdIndex
        {
            get { return this.PropertyValueReader.ReadInt64At(9); }
        }

        [TraceField(10, OriginalName = "body.activityDescription.activityType")]
        public ActivityType BodyActivityType
        {
            get { return (ActivityType)this.PropertyValueReader.ReadInt32At(10); }
        }

        public override Task DispatchAsync(CancellationToken token)
        {
            return this.onOccurrence?.Invoke(this, token);
        }

        public override Delegate Target
        {
            get { return this.onOccurrence; }
            protected set { this.onOccurrence = (Func<BeginReportFaultTraceRecord, CancellationToken, Task>)value; }
        }

        internal string CorrelationId
        {
            get
            {
                return this.ReasonActivityIdIndex > 0 ? string.Format(CultureInfo.InvariantCulture, "{0}:{1}", this.ReasonActivityId, this.ReasonActivityIdIndex) : string.Format(CultureInfo.InvariantCulture, "{0}", this.ReasonActivityId);
            }
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
               "{0}, NodeId : {1}, PartitionId : {2}, ReplicaId : {3}, FaultType : {4}, IsForce : {5}",
                base.ToString(),
                this.NodeId,
                this.PartitionId,
                this.ReplicaId,
                this.FaultType,
                this.IsForce);
        }

        public override string ToPublicString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
               "{0}, NodeId : {1}, PartitionId : {2}, ReplicaId : {3}, FaultType : {4}, IsForce : {5}",
                this.ToStringCommonPrefix(),
                this.NodeId,
                this.PartitionId,
                this.ReplicaId,
                this.FaultType,
                this.IsForce);
        }
    }
}


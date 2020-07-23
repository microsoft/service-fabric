// ----------------------------------------------------------------------
//  <copyright file="ReportFaultTraceRecord.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.RA
{
    using System;
    using System.Globalization;
    using System.Runtime.Serialization;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    [DataContract]
    public sealed class ReportFaultTraceRecord : StronglyTypedTraceRecord
    {
        private Func<ReportFaultTraceRecord, CancellationToken, Task> onOccurrence;

        public ReportFaultTraceRecord(Func<ReportFaultTraceRecord, CancellationToken, Task> onOccurenceAction) : base(
            18847,
            TaskName.RA)
        {
           this.onOccurrence = onOccurenceAction;
        }

        [TraceField(0, OriginalName = "id")]
        public Guid Id
        {
            get { return this.PropertyValueReader.ReadGuidAt(0); }
        }

        [TraceField(1, OriginalName = "nodeInstance")]
        public string NodeInstance
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(1); }
        }

        [TraceField(2, OriginalName = "partitionId")]
        public Guid PartitionId
        {
            get { return this.PropertyValueReader.ReadGuidAt(2); }
        }

        [TraceField(3, OriginalName = "replicaId")]
        public long ReplicaId
        {
            get { return this.PropertyValueReader.ReadInt64At(3); }
        }

        [TraceField(4, OriginalName = "faultType")]
        public FaultType FaultType
        {
            get { return (FaultType)this.PropertyValueReader.ReadInt32At(4); }
        }

        [TraceField(5, OriginalName = "isForce")]
        public bool IsForce
        {
            get { return this.PropertyValueReader.ReadInt32At(5) > 0; }
        }

        [TraceField(6, OriginalName = "reasonActivityDescription.activityId.id")]
        public Guid ReasonActivityId
        {
            get { return this.PropertyValueReader.ReadGuidAt(6); }
        }

        [TraceField(7, OriginalName = "reasonActivityDescription.activityId.index")]
        public long ReasonActivityIdIndex
        {
            get { return this.PropertyValueReader.ReadInt64At(7); }
        }

        [TraceField(8, OriginalName = "reasonActivityDescription.activityType")]
        public ActivityType ReasonActivityType
        {
            get { return (ActivityType)this.PropertyValueReader.ReadInt32At(8); }
        }

        public override Task DispatchAsync(CancellationToken token)
        {
            return this.onOccurrence?.Invoke(this, token);
        }

        public override Delegate Target
        {
            get { return this.onOccurrence; }
            protected set { this.onOccurrence = (Func<ReportFaultTraceRecord, CancellationToken, Task>)value; }
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
               "{0}, Id : {1}, NodeInstance : {2}, PartitionId : {3}, ReplicaId : {4}, FaultType : {5}, IsForce : {6}",
                base.ToString(),
                this.Id,
                this.NodeInstance,
                this.PartitionId,
                this.ReplicaId,
                this.FaultType,
                this.IsForce);
        }

        public override string ToPublicString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
               "{0}, NodeInstance : {1}, PartitionId : {2}, ReplicaId : {3}, FaultType : {4}, IsForce : {5}",
                this.ToStringCommonPrefix(),
                this.NodeInstance,
                this.PartitionId,
                this.ReplicaId,
                this.FaultType,
                this.IsForce);
        }
    }
}

public enum FaultType
{
    Invalid = 0x0,
    Transient = 0x1,
    Permanent = 0x2,
}


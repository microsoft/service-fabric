// ----------------------------------------------------------------------
//  <copyright file="ReplicaStateChangeTraceRecord.cs" company="Microsoft">
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
    public sealed class ReplicaStateChangeTraceRecord : StronglyTypedTraceRecord
    {
        private Func<ReplicaStateChangeTraceRecord, CancellationToken, Task> onOccurrence;

        public ReplicaStateChangeTraceRecord(Func<ReplicaStateChangeTraceRecord, CancellationToken, Task> onOccurenceAction) : base(
            18941,
            TaskName.RA)
        {
           this.onOccurrence = onOccurenceAction;
        }

        [TraceField(0, OriginalName = "nodeInstance_id")]
        public string NodeInstanceId
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(0); }
        }

        [TraceField(1, OriginalName = "partitionId")]
        public Guid PartitionId
        {
            get { return this.PropertyValueReader.ReadGuidAt(1); }
        }

        [TraceField(2, OriginalName = "replicaId")]
        public long ReplicaId
        {
            get { return this.PropertyValueReader.ReadInt64At(2); }
        }

        [TraceField(3, OriginalName = "epoch.dataLossVersion")]
        public long EpochDataLossVersion
        {
            get { return this.PropertyValueReader.ReadInt64At(3); }
        }

        [TraceField(4, OriginalName = "epoch.configVersion")]
        public long EpochConfigVersion
        {
            get { return this.PropertyValueReader.ReadInt64At(4); }
        }

        [TraceField(5, OriginalName = "status")]
        public ReplicaLifeCycleState Status
        {
            get { return (ReplicaLifeCycleState)this.PropertyValueReader.ReadInt32At(5); }
        }

        [TraceField(6, OriginalName = "role")]
        public ReplicaRole Role
        {
            get { return (ReplicaRole)this.PropertyValueReader.ReadInt32At(6); }
        }

        [TraceField(7, OriginalName = "reasonActivityDescription.activityId.id")]
        public Guid ReasonActivityId
        {
            get { return this.PropertyValueReader.ReadGuidAt(7); }
        }

        [TraceField(8, OriginalName = "reasonActivityDescription.activityId.index")]
        public long ReasonActivityIdIndex
        {
            get { return this.PropertyValueReader.ReadInt64At(8); }
        }

        [TraceField(9, OriginalName = "reasonActivityDescription.activityType")]
        public ActivityType ReasonActivityType
        {
            get { return (ActivityType)this.PropertyValueReader.ReadInt32At(9); }
        }

        public override Task DispatchAsync(CancellationToken token)
        {
            return this.onOccurrence?.Invoke(this, token);
        }

        public override Delegate Target
        {
            get { return this.onOccurrence; }
            protected set { this.onOccurrence = (Func<ReplicaStateChangeTraceRecord, CancellationToken, Task>)value; }
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
               "{0}, NodeInstance : {1}, PartitionId : {2}, ReplicaId : {3}, EpochDataLossVersion : {4}, EpochConfigVersion : {5}, Status : {6}, Role : {7}",
                base.ToString(),
                this.NodeInstanceId,
                this.PartitionId,
                this.ReplicaId,
                this.EpochDataLossVersion,
                this.EpochConfigVersion,
                this.Status,
                this.Role);
        }

        public override string ToPublicString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
               "{0}, NodeInstance : {1}, PartitionId : {2}, ReplicaId : {3}, EpochDataLossVersion : {4}, EpochConfigVersion : {5}, Status : {6}, Role : {7}",
                this.ToStringCommonPrefix(),
                this.NodeInstanceId,
                this.PartitionId,
                this.ReplicaId,
                this.EpochDataLossVersion,
                this.EpochConfigVersion,
                this.Status,
                this.Role);
        }
    }
}

public enum ActivityType
{
    Unknown = 0x0,
    ClientReportFaultEvent = 0x1,
    ServiceReportFaultEvent = 0x2,
    ReplicaEvent = 0x3,
    ServicePackageEvent = 0x4,
}

public enum ReplicaLifeCycleState
{
    Invalid = 0x0,
    Closing = 0x1,
}

public enum ReplicaRole
{
    U = 0x0,
    N = 0x1,
    I = 0x2,
    S = 0x3,
    P = 0x4,
}


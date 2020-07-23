// ----------------------------------------------------------------------
//  <copyright file="OperationIgnoredTraceRecord.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.CRM
{
    using System;
    using System.Globalization;
    using System.Runtime.Serialization;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    [DataContract]
    public sealed class OperationIgnoredTraceRecord : StronglyTypedTraceRecord
    {
        public const string FailoverUnitIdPropertyName = "FailoverUnitId";
        public const string ReasonPropertyName = "Reason";
        public const string DecisionIdPropertyName = "DecisionId";

        private Func<OperationIgnoredTraceRecord, CancellationToken, Task> onOccurrence;

        public OperationIgnoredTraceRecord(Func<OperationIgnoredTraceRecord, CancellationToken, Task> onOccurenceAction) : base(
            17552,
            TaskName.CRM)
        {
           this.onOccurrence = onOccurenceAction;
        }

        [TraceField(0, OriginalName = "FailoverUnitId")]
        public Guid FailoverUnitId
        {
            get { return this.PropertyValueReader.ReadGuidAt(0); }
        }

        [TraceField(1, OriginalName = "Reason")]
        public PlbMovementIgnoredReasons Reason
        {
            get { return (PlbMovementIgnoredReasons)this.PropertyValueReader.ReadInt32At(1); }
        }

        [TraceField(2, OriginalName = "DecisionId")]
        public Guid DecisionId
        {
            get { return this.PropertyValueReader.ReadGuidAt(2); }
        }

        public override Task DispatchAsync(CancellationToken token)
        {
            return this.onOccurrence?.Invoke(this, token);
        }

        public override Delegate Target
        {
            get { return this.onOccurrence; }
            protected set { this.onOccurrence = (Func<OperationIgnoredTraceRecord, CancellationToken, Task>)value; }
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
               "{0}, FailoverUnitId : {1}, Reason : {2}, DecisionId : {3}",
                base.ToString(),
                this.TracingProcessId,
                this.ThreadId,
                this.Level,
                this.FailoverUnitId,
                this.Reason,
                this.DecisionId);
        }

        public override string ToPublicString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
               "{0}, FailoverUnitId : {1}, Reason : {2}, DecisionId : {3}",
                this.ToStringCommonPrefix(),
                this.FailoverUnitId,
                this.Reason,
                this.DecisionId);
        }
    }
}

public enum PlbMovementIgnoredReasons
{
    None = 0x0,
    ClusterPaused = 0x1,
    DropAllPLBMovementsConfigurationSetToTrue = 0x2,
    FailoverUnitNotFound = 0x3,
    FailoverUnitToBeDeleted = 0x4,
    VersionMismatch = 0x5,
    FailoverUnitIsChanging = 0x6,
    NodePendingClose = 0x7,
    NodeIsUploadingReplicas = 0x8,
    ReplicaNotDeleted = 0x9,
    NodePendingDeactivate = 0xa,
    InvalidReplicaState = 0xb,
    CurrentConfigurationIsEmpty = 0xc,
    ReplicaConfigurationIsChanging = 0xd,
    ToBePromotedReplicaAlreadyExists = 0xe,
    ReplicaToBeDropped = 0xf,
    ReplicaNotFound = 0x10,
}


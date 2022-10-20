// ----------------------------------------------------------------------
//  <copyright file="OperationTraceRecord.cs" company="Microsoft">
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
    public sealed class OperationTraceRecord : StronglyTypedTraceRecord
    {
        public const string IdPropertyName = "Id";
        public const string DecisionIdPropertyName = "DecisionId";
        public const string SchedulerPhasePropertyName = "SchedulerPhase";
        public const string ActionPropertyName = "Action";
        public const string ServiceNamePropertyName = "ServiceName";
        public const string SourceNodePropertyName = "SourceNode";
        public const string TargetNodePropertyName = "TargetNode";

        private Func<OperationTraceRecord, CancellationToken, Task> onOccurrence;

        public OperationTraceRecord(Func<OperationTraceRecord, CancellationToken, Task> onOccurenceAction) : base(
            17551,
            TaskName.CRM)
        {
           this.onOccurrence = onOccurenceAction;
        }

        [TraceField(0, OriginalName = "id")]
        public Guid PartitionId
        {
            get { return this.PropertyValueReader.ReadGuidAt(0); }
        }

        [TraceField(1, OriginalName = "decisionId")]
        public Guid DecisionId
        {
            get { return this.PropertyValueReader.ReadGuidAt(1); }
        }

        [TraceField(2, OriginalName = "SchedulerPhase")]
        public PLBSchedulerActionType SchedulerPhase
        {
            get { return (PLBSchedulerActionType)this.PropertyValueReader.ReadInt32At(2); }
        }

        [TraceField(3, OriginalName = "Action")]
        public FailoverUnitMovementType Action
        {
            get { return (FailoverUnitMovementType)this.PropertyValueReader.ReadInt32At(3); }
        }

        [TraceField(4, OriginalName = "ServiceName")]
        public string ServiceName
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(4); }
        }

        [TraceField(5, OriginalName = "SourceNode")]
        public string SourceNode
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(5); }
        }

        [TraceField(6, OriginalName = "TargetNode")]
        public string TargetNode
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
            protected set { this.onOccurrence = (Func<OperationTraceRecord, CancellationToken, Task>)value; }
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
               "{0}, PartitionId : {1}, DecisionId : {2}, SchedulerPhase : {3}, Action : {4}, ServiceName : {5}, SourceNodeId : {6}, TargetNodeId : {7}",
                base.ToString(),
                this.PartitionId,
                this.DecisionId,
                this.SchedulerPhase,
                this.Action,
                this.ServiceName,
                this.SourceNode,
                this.TargetNode);
        }

        public override string ToPublicString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
               "{0}, PartitionId : {1}, DecisionId : {2}, SchedulerPhase : {3}, Action : {4}, ServiceName : {5}, SourceNodeId : {6}, TargetNodeId : {7}",
                this.ToStringCommonPrefix(),
                this.PartitionId,
                this.DecisionId,
                this.SchedulerPhase,
                this.Action,
                this.ServiceName,
                this.SourceNode,
                this.TargetNode);
        }
    }
}

public enum FailoverUnitMovementType
{
    SwapPrimarySecondary = 0x0,
    MoveSecondary = 0x1,
    MovePrimary = 0x2,
    MoveInstance = 0x3,
    AddPrimary = 0x4,
    AddSecondary = 0x5,
    AddInstance = 0x6,
    PromoteSecondary = 0x7,
    RequestedPlacementNotPossible = 0x8,
    DropPrimary = 0x9,
    DropSecondary = 0xa,
    DropInstance = 0xb,
}

public enum PLBSchedulerActionType
{
    None = 0x0,
    NewReplicaPlacement = 0x1,
    NewReplicaPlacementWithMove = 0x2,
    ConstraintCheck = 0x3,
    QuickLoadBalancing = 0x4,
    LoadBalancing = 0x5,
    NoActionNeeded = 0x6,
    Upgrade = 0x7,
    ClientApiPromoteToPrimary = 0x8,
    ClientApiMovePrimary = 0x9,
    ClientApiMoveSecondary = 0xa,
}


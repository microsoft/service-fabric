// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.RA
{
    using System;
    using System.Globalization;
    using System.Runtime.Serialization;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    [DataContract]
    public sealed class ReconfigurationCompletedTraceRecord : StronglyTypedTraceRecord
    {
        private Func<ReconfigurationCompletedTraceRecord, CancellationToken, Task> onOccurrence;

        public ReconfigurationCompletedTraceRecord(Func<ReconfigurationCompletedTraceRecord, CancellationToken, Task> onOccurenceAction) : base(
            18940,
            TaskName.RA)
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

        [TraceField(4, OriginalName = "nodeName")]
        public string NodeName
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(4); }
        }

        [TraceField(5, OriginalName = "nodeInstance_id")]
        public string NodeInstanceId
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(5); }
        }

        [TraceField(6, OriginalName = "serviceType")]
        public string ServiceType
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(6); }
        }

        [TraceField(7, OriginalName = "ccEpoch.dataLossVersion")]
        public long CcEpochDataLossVersion
        {
            get { return this.PropertyValueReader.ReadInt64At(7); }
        }

        [TraceField(8, OriginalName = "ccEpoch.configVersion")]
        public long CcEpochConfigVersion
        {
            get { return this.PropertyValueReader.ReadInt64At(8); }
        }

        [TraceField(9, OriginalName = "reconfigType")]
        public ReconfigurationType ReconfigType
        {
            get { return (ReconfigurationType)this.PropertyValueReader.ReadInt32At(9); }
        }

        [TraceField(10, OriginalName = "result")]
        public ReconfigurationResult Result
        {
            get { return (ReconfigurationResult)this.PropertyValueReader.ReadInt32At(10); }
        }

        [TraceField(11, OriginalName = "phase0DurationMs")]
        public double Phase0DurationMs
        {
            get { return this.PropertyValueReader.ReadDoubleAt(11); }
        }

        [TraceField(12, OriginalName = "phase1DurationMs")]
        public double Phase1DurationMs
        {
            get { return this.PropertyValueReader.ReadDoubleAt(12); }
        }

        [TraceField(13, OriginalName = "phase2DurationMs")]
        public double Phase2DurationMs
        {
            get { return this.PropertyValueReader.ReadDoubleAt(13); }
        }

        [TraceField(14, OriginalName = "phase3DurationMs")]
        public double Phase3DurationMs
        {
            get { return this.PropertyValueReader.ReadDoubleAt(14); }
        }

        [TraceField(15, OriginalName = "phase4DurationMs")]
        public double Phase4DurationMs
        {
            get { return this.PropertyValueReader.ReadDoubleAt(15); }
        }

        [TraceField(16, OriginalName = "totalDurationMs")]
        public double TotalDurationMs
        {
            get { return this.PropertyValueReader.ReadDoubleAt(16); }
        }

        public override Task DispatchAsync(CancellationToken token)
        {
            return this.onOccurrence?.Invoke(this, token);
        }

        public override Delegate Target
        {
            get { return this.onOccurrence; }
            protected set { this.onOccurrence = (Func<ReconfigurationCompletedTraceRecord, CancellationToken, Task>)value; }
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
               "TimeStamp : {0}, TracingProcessId : {1}, ThreadId : {2}, Level : {3}, EventName : {4}, Category : {5}, EventInstanceId : {6}, PartitionId : {7}, NodeName : {8}, NodeInstanceId : {9}, ServiceType : {10}, CcEpochDataLossVersion : {11}, CcEpochConfigVersion : {12}, ReconfigType : {13}, Result : {14}, Phase0DurationMs : {15}, Phase1DurationMs : {16}, Phase2DurationMs : {17}, Phase3DurationMs : {18}, Phase4DurationMs : {19}, TotalDurationMs : {20}",
                this.TimeStamp,
                this.TracingProcessId,
                this.ThreadId,
                this.Level,
                this.EventName,
                this.Category,
                this.EventInstanceId,
                this.PartitionId,
                this.NodeName,
                this.NodeInstanceId,
                this.ServiceType,
                this.CcEpochDataLossVersion,
                this.CcEpochConfigVersion,
                this.ReconfigType,
                this.Result,
                this.Phase0DurationMs,
                this.Phase1DurationMs,
                this.Phase2DurationMs,
                this.Phase3DurationMs,
                this.Phase4DurationMs,
                this.TotalDurationMs);
        }
    }
}

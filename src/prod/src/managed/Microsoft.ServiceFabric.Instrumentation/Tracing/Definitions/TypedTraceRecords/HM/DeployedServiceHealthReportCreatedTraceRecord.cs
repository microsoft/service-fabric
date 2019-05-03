// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.HM
{
    using System;
    using System.Globalization;
    using System.Runtime.Serialization;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    [DataContract]
    public sealed class DeployedServiceHealthReportCreatedTraceRecord : StronglyTypedTraceRecord
    {
        private Func<DeployedServiceHealthReportCreatedTraceRecord, CancellationToken, Task> onOccurrence;

        public DeployedServiceHealthReportCreatedTraceRecord(Func<DeployedServiceHealthReportCreatedTraceRecord, CancellationToken, Task> onOccurenceAction) : base(
            54427,
            TaskName.HM)
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

        [TraceField(3, OriginalName = "applicationName")]
        public string ApplicationName
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(3); }
        }

        [TraceField(4, OriginalName = "serviceManifestName")]
        public string ServiceManifestName
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(4); }
        }

        [TraceField(5, OriginalName = "servicePackageInstanceId")]
        public long ServicePackageInstanceId
        {
            get { return this.PropertyValueReader.ReadInt64At(5); }
        }

        [TraceField(6, OriginalName = "servicePackageActivationId")]
        public string ServicePackageActivationId
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(6); }
        }

        [TraceField(7, OriginalName = "nodeName")]
        public string NodeName
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(7); }
        }

        [TraceField(8, OriginalName = "sourceId")]
        public string SourceId
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(8); }
        }

        [TraceField(9, OriginalName = "property")]
        public string Property
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(9); }
        }

        [TraceField(10, OriginalName = "healthState")]
        public HealthState HealthState
        {
            get { return (HealthState)this.PropertyValueReader.ReadInt32At(10); }
        }

        [TraceField(11, OriginalName = "TTL.timespan")]
        public long TTLTimespan
        {
            get { return this.PropertyValueReader.ReadInt64At(11); }
        }

        [TraceField(12, OriginalName = "sequenceNumber")]
        public long SequenceNumber
        {
            get { return this.PropertyValueReader.ReadInt64At(12); }
        }

        [TraceField(13, OriginalName = "description")]
        public string Description
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(13); }
        }

        [TraceField(14, OriginalName = "removeWhenExpired")]
        public bool RemoveWhenExpired
        {
            get { return this.PropertyValueReader.ReadBoolAt(14); }
        }

        [TraceField(15, OriginalName = "sourceUtcTimestamp")]
        public DateTime SourceUtcTimestamp
        {
            get { return this.PropertyValueReader.ReadDateTimeAt(15); }
        }

        public override Task DispatchAsync(CancellationToken token)
        {
            return this.onOccurrence?.Invoke(this, token);
        }

        public override Delegate Target
        {
            get { return this.onOccurrence; }
            protected set { this.onOccurrence = (Func<DeployedServiceHealthReportCreatedTraceRecord, CancellationToken, Task>)value; }
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
               "TimeStamp : {0}, TracingProcessId : {1}, ThreadId : {2}, Level : {3}, EventName : {4}, Category : {5}, EventInstanceId : {6}, ApplicationName : {7}, ServiceManifestName : {8}, ServicePackageInstanceId : {9}, ServicePackageActivationId : {10}, NodeName : {11}, SourceId : {12}, Property : {13}, HealthState : {14}, TTLTimespan : {15}, SequenceNumber : {16}, Description : {17}, RemoveWhenExpired : {18}, SourceUtcTimestamp : {19}",
                this.TimeStamp,
                this.TracingProcessId,
                this.ThreadId,
                this.Level,
                this.EventName,
                this.Category,
                this.EventInstanceId,
                this.ApplicationName,
                this.ServiceManifestName,
                this.ServicePackageInstanceId,
                this.ServicePackageActivationId,
                this.NodeName,
                this.SourceId,
                this.Property,
                this.HealthState,
                this.TTLTimespan,
                this.SequenceNumber,
                this.Description,
                this.RemoveWhenExpired,
                this.SourceUtcTimestamp);
        }
    }
}

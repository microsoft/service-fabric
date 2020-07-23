// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.FM
{
    using System;
    using System.Globalization;
    using System.Runtime.Serialization;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    [DataContract]
    public sealed class ServiceCreatedTraceRecord : StronglyTypedTraceRecord
    {
        private Func<ServiceCreatedTraceRecord, CancellationToken, Task> onOccurrence;

        public ServiceCreatedTraceRecord(Func<ServiceCreatedTraceRecord, CancellationToken, Task> onOccurenceAction) : base(
            18657,
            TaskName.FM)
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

        [TraceField(3, OriginalName = "serviceName")]
        public string ServiceName
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(3); }
        }

        [TraceField(4, OriginalName = "serviceTypeName")]
        public string ServiceTypeName
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(4); }
        }

        [TraceField(5, OriginalName = "applicationName")]
        public string ApplicationName
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(5); }
        }

        [TraceField(6, OriginalName = "applicationTypeName")]
        public string ApplicationTypeName
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(6); }
        }

        [TraceField(7, OriginalName = "serviceInstance")]
        public long ServiceInstance
        {
            get { return this.PropertyValueReader.ReadInt64At(7); }
        }

        [TraceField(8, OriginalName = "isStateful")]
        public bool IsStateful
        {
            get { return this.PropertyValueReader.ReadBoolAt(8); }
        }

        [TraceField(9, OriginalName = "partitionCount")]
        public int PartitionCount
        {
            get { return this.PropertyValueReader.ReadInt32At(9); }
        }

        [TraceField(10, OriginalName = "targetReplicaSetSize")]
        public int TargetReplicaSetSize
        {
            get { return this.PropertyValueReader.ReadInt32At(10); }
        }

        [TraceField(11, OriginalName = "minReplicaSetSize")]
        public int MinReplicaSetSize
        {
            get { return this.PropertyValueReader.ReadInt32At(11); }
        }

        [TraceField(12, OriginalName = "servicePackageVersion")]
        public string ServicePackageVersion
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(12); }
        }

        [TraceField(13, OriginalName = "partitionId")]
        public Guid PartitionId
        {
            get { return this.PropertyValueReader.ReadGuidAt(13); }
        }

        public override Task DispatchAsync(CancellationToken token)
        {
            return this.onOccurrence?.Invoke(this, token);
        }

        public override Delegate Target
        {
            get { return this.onOccurrence; }
            protected set { this.onOccurrence = (Func<ServiceCreatedTraceRecord, CancellationToken, Task>)value; }
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
               "TimeStamp : {0}, TracingProcessId : {1}, ThreadId : {2}, Level : {3}, EventName : {4}, Category : {5}, EventInstanceId : {6}, ServiceName : {7}, ServiceTypeName : {8}, ApplicationName : {9}, ApplicationTypeName : {10}, ServiceInstance : {11}, IsStateful : {12}, PartitionCount : {13}, TargetReplicaSetSize : {14}, MinReplicaSetSize : {15}, ServicePackageVersion : {16}, PartitionId : {17}",
                this.TimeStamp,
                this.TracingProcessId,
                this.ThreadId,
                this.Level,
                this.EventName,
                this.Category,
                this.EventInstanceId,
                this.ServiceName,
                this.ServiceTypeName,
                this.ApplicationName,
                this.ApplicationTypeName,
                this.ServiceInstance,
                this.IsStateful,
                this.PartitionCount,
                this.TargetReplicaSetSize,
                this.MinReplicaSetSize,
                this.ServicePackageVersion,
                this.PartitionId);
        }
    }
}

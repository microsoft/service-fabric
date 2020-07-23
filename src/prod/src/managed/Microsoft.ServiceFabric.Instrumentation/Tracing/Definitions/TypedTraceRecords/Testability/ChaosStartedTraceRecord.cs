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
    public sealed class ChaosStartedTraceRecord : StronglyTypedTraceRecord
    {
        private Func<ChaosStartedTraceRecord, CancellationToken, Task> onOccurrence;

        public ChaosStartedTraceRecord(Func<ChaosStartedTraceRecord, CancellationToken, Task> onOccurenceAction) : base(
            50021,
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

        [TraceField(3, OriginalName = "maxConcurrentFaults")]
        public long MaxConcurrentFaults
        {
            get { return this.PropertyValueReader.ReadInt64At(3); }
        }

        [TraceField(4, OriginalName = "timeToRunInSeconds")]
        public double TimeToRunInSeconds
        {
            get { return this.PropertyValueReader.ReadDoubleAt(4); }
        }

        [TraceField(5, OriginalName = "maxClusterStabilizationTimeoutInSeconds")]
        public double MaxClusterStabilizationTimeoutInSeconds
        {
            get { return this.PropertyValueReader.ReadDoubleAt(5); }
        }

        [TraceField(6, OriginalName = "waitTimeBetweenIterationsInSeconds")]
        public double WaitTimeBetweenIterationsInSeconds
        {
            get { return this.PropertyValueReader.ReadDoubleAt(6); }
        }

        [TraceField(7, OriginalName = "waitTimeBetweenFautlsInSeconds")]
        public double WaitTimeBetweenFautlsInSeconds
        {
            get { return this.PropertyValueReader.ReadDoubleAt(7); }
        }

        [TraceField(8, OriginalName = "moveReplicaFaultEnabled")]
        public bool MoveReplicaFaultEnabled
        {
            get { return this.PropertyValueReader.ReadBoolAt(8); }
        }

        [TraceField(9, OriginalName = "includedNodeTypeList")]
        public string IncludedNodeTypeList
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(9); }
        }

        [TraceField(10, OriginalName = "includedApplicationList")]
        public string IncludedApplicationList
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(10); }
        }

        [TraceField(11, OriginalName = "clusterHealthPolicy")]
        public string ClusterHealthPolicy
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(11); }
        }

        [TraceField(12, OriginalName = "chaosContext")]
        public string ChaosContext
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(12); }
        }

        public override Task DispatchAsync(CancellationToken token)
        {
            return this.onOccurrence?.Invoke(this, token);
        }

        public override Delegate Target
        {
            get { return this.onOccurrence; }
            protected set { this.onOccurrence = (Func<ChaosStartedTraceRecord, CancellationToken, Task>)value; }
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
               "TimeStamp : {0}, TracingProcessId : {1}, ThreadId : {2}, Level : {3}, EventName : {4}, Category : {5}, EventInstanceId : {6}, MaxConcurrentFaults : {7}, TimeToRunInSeconds : {8}, MaxClusterStabilizationTimeoutInSeconds : {9}, WaitTimeBetweenIterationsInSeconds : {10}, WaitTimeBetweenFautlsInSeconds : {11}, MoveReplicaFaultEnabled : {12}, IncludedNodeTypeList : {13}, IncludedApplicationList : {14}, ClusterHealthPolicy : {15}, ChaosContext : {16}",
                this.TimeStamp,
                this.TracingProcessId,
                this.ThreadId,
                this.Level,
                this.EventName,
                this.Category,
                this.EventInstanceId,
                this.MaxConcurrentFaults,
                this.TimeToRunInSeconds,
                this.MaxClusterStabilizationTimeoutInSeconds,
                this.WaitTimeBetweenIterationsInSeconds,
                this.WaitTimeBetweenFautlsInSeconds,
                this.MoveReplicaFaultEnabled,
                this.IncludedNodeTypeList,
                this.IncludedApplicationList,
                this.ClusterHealthPolicy,
                this.ChaosContext);
        }
    }
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.FabricNode
{
    using System;
    using System.Globalization;
    using System.Runtime.Serialization;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    [DataContract]
    public sealed class NodeAbortedTraceRecord : StronglyTypedTraceRecord
    {
        private Func<NodeAbortedTraceRecord, CancellationToken, Task> onOccurrence;

        public NodeAbortedTraceRecord(Func<NodeAbortedTraceRecord, CancellationToken, Task> onOccurenceAction) : base(
            25626,
            TaskName.FabricNode)
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

        [TraceField(3, OriginalName = "nodeName")]
        public string NodeName
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(3); }
        }

        [TraceField(4, OriginalName = "nodeInstance")]
        public long NodeInstance
        {
            get { return this.PropertyValueReader.ReadInt64At(4); }
        }

        [TraceField(5, OriginalName = "nodeId")]
        public string NodeId
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(5); }
        }

        [TraceField(6, OriginalName = "upgradeDomain")]
        public string UpgradeDomain
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(6); }
        }

        [TraceField(7, OriginalName = "faultDomain")]
        public string FaultDomain
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(7); }
        }

        [TraceField(8, OriginalName = "ipAddressOrFQDN")]
        public string IpAddressOrFQDN
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(8); }
        }

        [TraceField(9, OriginalName = "hostname")]
        public string Hostname
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(9); }
        }

        [TraceField(10, OriginalName = "isSeedNode")]
        public bool IsSeedNode
        {
            get { return this.PropertyValueReader.ReadBoolAt(10); }
        }

        [TraceField(11, OriginalName = "nodeVersion")]
        public string NodeVersion
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(11); }
        }

        public override Task DispatchAsync(CancellationToken token)
        {
            return this.onOccurrence?.Invoke(this, token);
        }

        public override Delegate Target
        {
            get { return this.onOccurrence; }
            protected set { this.onOccurrence = (Func<NodeAbortedTraceRecord, CancellationToken, Task>)value; }
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
               "TimeStamp : {0}, TracingProcessId : {1}, ThreadId : {2}, Level : {3}, EventName : {4}, Category : {5}, EventInstanceId : {6}, NodeName : {7}, NodeInstance : {8}, NodeId : {9}, UpgradeDomain : {10}, FaultDomain : {11}, IpAddressOrFQDN : {12}, Hostname : {13}, IsSeedNode : {14}, NodeVersion : {15}",
                this.TimeStamp,
                this.TracingProcessId,
                this.ThreadId,
                this.Level,
                this.EventName,
                this.Category,
                this.EventInstanceId,
                this.NodeName,
                this.NodeInstance,
                this.NodeId,
                this.UpgradeDomain,
                this.FaultDomain,
                this.IpAddressOrFQDN,
                this.Hostname,
                this.IsSeedNode,
                this.NodeVersion);
        }
    }
}

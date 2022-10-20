// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.CM
{
    using System;
    using System.Globalization;
    using System.Runtime.Serialization;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    [DataContract]
    public sealed class ClusterUpgradeStartTraceRecord : StronglyTypedTraceRecord
    {
        private Func<ClusterUpgradeStartTraceRecord, CancellationToken, Task> onOccurrence;

        public ClusterUpgradeStartTraceRecord(Func<ClusterUpgradeStartTraceRecord, CancellationToken, Task> onOccurenceAction) : base(
            29627,
            TaskName.CM)
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

        [TraceField(3, OriginalName = "currentClusterVersion")]
        public string CurrentClusterVersion
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(3); }
        }

        [TraceField(4, OriginalName = "targetClusterVersion")]
        public string TargetClusterVersion
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(4); }
        }

        [TraceField(5, OriginalName = "upgradeType")]
        public UpgradeType UpgradeType
        {
            get { return (UpgradeType)this.PropertyValueReader.ReadInt32At(5); }
        }

        [TraceField(6, OriginalName = "rollingUpgradeMode")]
        public RollingUpgradeMode RollingUpgradeMode
        {
            get { return (RollingUpgradeMode)this.PropertyValueReader.ReadInt32At(6); }
        }

        [TraceField(7, OriginalName = "failureAction")]
        public MonitoredUpgradeFailureAction FailureAction
        {
            get { return (MonitoredUpgradeFailureAction)this.PropertyValueReader.ReadInt32At(7); }
        }

        public override Task DispatchAsync(CancellationToken token)
        {
            return this.onOccurrence?.Invoke(this, token);
        }

        public override Delegate Target
        {
            get { return this.onOccurrence; }
            protected set { this.onOccurrence = (Func<ClusterUpgradeStartTraceRecord, CancellationToken, Task>)value; }
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
               "TimeStamp : {0}, TracingProcessId : {1}, ThreadId : {2}, Level : {3}, EventName : {4}, Category : {5}, EventInstanceId : {6}, CurrentClusterVersion : {7}, TargetClusterVersion : {8}, UpgradeType : {9}, RollingUpgradeMode : {10}, FailureAction : {11}",
                this.TimeStamp,
                this.TracingProcessId,
                this.ThreadId,
                this.Level,
                this.EventName,
                this.Category,
                this.EventInstanceId,
                this.CurrentClusterVersion,
                this.TargetClusterVersion,
                this.UpgradeType,
                this.RollingUpgradeMode,
                this.FailureAction);
        }
    }
}

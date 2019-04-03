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
    public sealed class ApplicationUpgradeDomainCompleteTraceRecord : StronglyTypedTraceRecord
    {
        private Func<ApplicationUpgradeDomainCompleteTraceRecord, CancellationToken, Task> onOccurrence;

        public ApplicationUpgradeDomainCompleteTraceRecord(Func<ApplicationUpgradeDomainCompleteTraceRecord, CancellationToken, Task> onOccurenceAction) : base(
            29626,
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

        [TraceField(3, OriginalName = "applicationName")]
        public string ApplicationName
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(3); }
        }

        [TraceField(4, OriginalName = "applicationTypeName")]
        public string ApplicationTypeName
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(4); }
        }

        [TraceField(5, OriginalName = "currentApplicationTypeVersion")]
        public string CurrentApplicationTypeVersion
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(5); }
        }

        [TraceField(6, OriginalName = "applicationTypeVersion")]
        public string ApplicationTypeVersion
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(6); }
        }

        [TraceField(7, OriginalName = "upgradeState")]
        public ApplicationUpgradeState UpgradeState
        {
            get { return (ApplicationUpgradeState)this.PropertyValueReader.ReadInt32At(7); }
        }

        [TraceField(8, OriginalName = "upgradeDomains")]
        public string UpgradeDomains
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(8); }
        }

        [TraceField(9, OriginalName = "upgradeDomainElapsedTimeInMs")]
        public double UpgradeDomainElapsedTimeInMs
        {
            get { return this.PropertyValueReader.ReadDoubleAt(9); }
        }

        public override Task DispatchAsync(CancellationToken token)
        {
            return this.onOccurrence?.Invoke(this, token);
        }

        public override Delegate Target
        {
            get { return this.onOccurrence; }
            protected set { this.onOccurrence = (Func<ApplicationUpgradeDomainCompleteTraceRecord, CancellationToken, Task>)value; }
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
               "TimeStamp : {0}, TracingProcessId : {1}, ThreadId : {2}, Level : {3}, EventName : {4}, Category : {5}, EventInstanceId : {6}, ApplicationName : {7}, ApplicationTypeName : {8}, CurrentApplicationTypeVersion : {9}, ApplicationTypeVersion : {10}, UpgradeState : {11}, UpgradeDomains : {12}, UpgradeDomainElapsedTimeInMs : {13}",
                this.TimeStamp,
                this.TracingProcessId,
                this.ThreadId,
                this.Level,
                this.EventName,
                this.Category,
                this.EventInstanceId,
                this.ApplicationName,
                this.ApplicationTypeName,
                this.CurrentApplicationTypeVersion,
                this.ApplicationTypeVersion,
                this.UpgradeState,
                this.UpgradeDomains,
                this.UpgradeDomainElapsedTimeInMs);
        }
    }
}

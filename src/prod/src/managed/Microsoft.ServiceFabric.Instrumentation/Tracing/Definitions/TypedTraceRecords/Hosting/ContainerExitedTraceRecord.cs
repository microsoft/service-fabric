// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Hosting
{
    using System;
    using System.Globalization;
    using System.Runtime.Serialization;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    [DataContract]
    public sealed class ContainerExitedTraceRecord : StronglyTypedTraceRecord
    {
        private Func<ContainerExitedTraceRecord, CancellationToken, Task> onOccurrence;

        public ContainerExitedTraceRecord(Func<ContainerExitedTraceRecord, CancellationToken, Task> onOccurenceAction) : base(
            23082,
            TaskName.Hosting)
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

        [TraceField(4, OriginalName = "ServiceName")]
        public string ServiceName
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(4); }
        }

        [TraceField(5, OriginalName = "ServicePackageName")]
        public string ServicePackageName
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(5); }
        }

        [TraceField(6, OriginalName = "ServicePackageActivationId")]
        public string ServicePackageActivationId
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(6); }
        }

        [TraceField(7, OriginalName = "IsExclusive")]
        public bool IsExclusive
        {
            get { return this.PropertyValueReader.ReadBoolAt(7); }
        }

        [TraceField(8, OriginalName = "CodePackageName")]
        public string CodePackageName
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(8); }
        }

        [TraceField(9, OriginalName = "EntryPointType")]
        public EntryPointType EntryPointType
        {
            get { return (EntryPointType)this.PropertyValueReader.ReadInt32At(9); }
        }

        [TraceField(10, OriginalName = "ImageName")]
        public string ImageName
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(10); }
        }

        [TraceField(11, OriginalName = "ContainerName")]
        public string ContainerName
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(11); }
        }

        [TraceField(12, OriginalName = "HostId")]
        public string HostId
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(12); }
        }

        [TraceField(13, OriginalName = "ExitCode")]
        public long ExitCode
        {
            get { return this.PropertyValueReader.ReadInt64At(13); }
        }

        [TraceField(14, OriginalName = "UnexpectedTermination")]
        public bool UnexpectedTermination
        {
            get { return this.PropertyValueReader.ReadBoolAt(14); }
        }

        [TraceField(15, OriginalName = "StartTime")]
        public DateTime StartTime
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
            protected set { this.onOccurrence = (Func<ContainerExitedTraceRecord, CancellationToken, Task>)value; }
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
               "TimeStamp : {0}, TracingProcessId : {1}, ThreadId : {2}, Level : {3}, EventName : {4}, Category : {5}, EventInstanceId : {6}, ApplicationName : {7}, ServiceName : {8}, ServicePackageName : {9}, ServicePackageActivationId : {10}, IsExclusive : {11}, CodePackageName : {12}, EntryPointType : {13}, ImageName : {14}, ContainerName : {15}, HostId : {16}, ExitCode : {17}, UnexpectedTermination : {18}, StartTime : {19}",
                this.TimeStamp,
                this.TracingProcessId,
                this.ThreadId,
                this.Level,
                this.EventName,
                this.Category,
                this.EventInstanceId,
                this.ApplicationName,
                this.ServiceName,
                this.ServicePackageName,
                this.ServicePackageActivationId,
                this.IsExclusive,
                this.CodePackageName,
                this.EntryPointType,
                this.ImageName,
                this.ContainerName,
                this.HostId,
                this.ExitCode,
                this.UnexpectedTermination,
                this.StartTime);
        }
    }
}

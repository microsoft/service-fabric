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
    public sealed class ProcessUnexpectedTerminationTraceRecord : StronglyTypedTraceRecord
    {
        private Func<ProcessUnexpectedTerminationTraceRecord, CancellationToken, Task> onOccurrence;

        public ProcessUnexpectedTerminationTraceRecord(Func<ProcessUnexpectedTerminationTraceRecord, CancellationToken, Task> onOccurenceAction) : base(
            23073,
            TaskName.Hosting)
        {
            this.onOccurrence = onOccurenceAction;
        }

        [TraceField(0, OriginalName = "id")]
        public string NodeId
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(0); }
        }

        [TraceField(1, OriginalName = "AppId")]
        public string AppId
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(1); }
        }

        [TraceField(2, OriginalName = "ReturnCode")]
        public long ReturnCode
        {
            get { return this.PropertyValueReader.ReadInt64At(2); }
        }

        [TraceField(3, OriginalName = "ProcessName")]
        public string ProcessName
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(3); }
        }

        [TraceField(4, OriginalName = "ProcessId")]
        public long CrashingProcessId
        {
            get { return this.PropertyValueReader.ReadInt64At(4); }
        }

        [TraceField(5, OriginalName = "StartTime")]
        public DateTime StartTime
        {
            get { return this.PropertyValueReader.ReadDateTimeAt(5); }
        }

        public override Task DispatchAsync(CancellationToken token)
        {
            return this.onOccurrence?.Invoke(this, token);
        }

        public override Delegate Target
        {
            get { return this.onOccurrence; }
            protected set { this.onOccurrence = (Func<ProcessUnexpectedTerminationTraceRecord, CancellationToken, Task>)value; }
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "TimeStamp : {0}, ProcessId : {1}, ThreadId : {2}, Level : {3}, NodeId : {4}, AppId : {5}, ReturnCode : {6}, ProcessName : {7}, CrashingProcessId : {8}, StartTime : {9}",
                this.TimeStamp,
                this.TracingProcessId,
                this.ThreadId,
                this.Level,
                this.NodeId,
                this.AppId,
                this.ReturnCode,
                this.ProcessName,
                this.CrashingProcessId,
                this.StartTime);
        }
    }
}
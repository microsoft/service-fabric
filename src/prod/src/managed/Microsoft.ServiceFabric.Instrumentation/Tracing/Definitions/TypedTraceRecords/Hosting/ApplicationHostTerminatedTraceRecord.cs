// ----------------------------------------------------------------------
//  <copyright file="ApplicationHostTerminatedTraceRecord.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Hosting
{
    using System;
    using System.Globalization;
    using System.Runtime.Serialization;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    [DataContract]
    public sealed class ApplicationHostTerminatedTraceRecord : StronglyTypedTraceRecord
    {
        private Func<ApplicationHostTerminatedTraceRecord, CancellationToken, Task> onOccurrence;

        public ApplicationHostTerminatedTraceRecord(Func<ApplicationHostTerminatedTraceRecord, CancellationToken, Task> onOccurenceAction) : base(
            23084,
            TaskName.Hosting)
        {
           this.onOccurrence = onOccurenceAction;
        }

        [TraceField(0, OriginalName = "HostId")]
        public string HostId
        {
            get { return this.PropertyValueReader.ReadUnicodeStringAt(0); }
        }

        [TraceField(1, OriginalName = "ExitCode")]
        public long ExitCode
        {
            get { return this.PropertyValueReader.ReadInt64At(1); }
        }

        [TraceField(2, OriginalName = "ActivityDescription.activityId.id")]
        public Guid ReasonActivityId
        {
            get { return this.PropertyValueReader.ReadGuidAt(2); }
        }

        [TraceField(3, OriginalName = "ActivityDescription.activityId.index")]
        public long ReasonActivityIdIndex
        {
            get { return this.PropertyValueReader.ReadInt64At(3); }
        }

        [TraceField(4, OriginalName = "ActivityDescription.activityType")]
        public ActivityType ReasonActivityType
        {
            get { return (ActivityType)this.PropertyValueReader.ReadInt32At(4); }
        }

        public override Task DispatchAsync(CancellationToken token)
        {
            return this.onOccurrence?.Invoke(this, token);
        }

        public override Delegate Target
        {
            get { return this.onOccurrence; }
            protected set { this.onOccurrence = (Func<ApplicationHostTerminatedTraceRecord, CancellationToken, Task>)value; }
        }

        internal string CorrelationId
        {
            get
            {
                return this.ReasonActivityIdIndex > 0 ? string.Format(CultureInfo.InvariantCulture, "{0}:{1}", this.ReasonActivityId, this.ReasonActivityIdIndex) : string.Format(CultureInfo.InvariantCulture, "{0}", this.ReasonActivityId);
            }
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
               "{0}, HostId : {1}, ExitCode : {2}",
                base.ToString(),
                this.HostId,
                this.ExitCode);
        }

        public override string ToPublicString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
               "{0}, HostId : {1}, ExitCode : {2}",
                this.ToStringCommonPrefix(),
                this.HostId,
                this.ExitCode);
        }
    }
}


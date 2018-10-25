// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Tracing
{
    using System.Diagnostics.CodeAnalysis;
    using System.Diagnostics.Tracing;

    internal class TraceFilterDescription
    {
        private readonly EventTask traceTaskId;
        private readonly string traceEventName;
        private readonly EventLevel level;
        private readonly int samplingRatio;

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        public TraceFilterDescription(
            EventTask taskId,
            string eventName,
            EventLevel level,
            int samplingRatio)
        {
            this.traceTaskId = taskId;
            this.traceEventName = eventName;
            this.level = level;
            this.samplingRatio = samplingRatio;
        }

        public EventLevel Level
        {
            get
            {
                return this.level;
            }
        }

        public int SamplingRatio
        {
            get
            {
                return this.samplingRatio;
            }
        }

        [SuppressMessage("Microsoft.Performance", "CA1811", Justification = "Used in some dlls while not in others")]
        public bool Matches(EventTask taskId, string eventName)
        {
            return (this.traceTaskId == taskId && this.traceEventName == eventName);
        }

        public int StaticCheck(EventTask taskId, string eventName)
        {
            if (this.traceTaskId != taskId)
            {
                return -1;
            }

            if (string.IsNullOrEmpty(this.traceEventName))
            {
                return 1;
            }

            if (eventName != this.traceEventName)
            {
                return -1;
            }

            return 2;
        }
    }
}
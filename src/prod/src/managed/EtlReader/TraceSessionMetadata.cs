// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;

    public class TraceSessionMetadata
    {
        public TraceSessionMetadata(string traceSessionName, uint eventsLostCount, DateTime startTime, DateTime endTime)
        {
            TraceSessionName = traceSessionName;
            EventsLostCount = eventsLostCount;
            StartTime = startTime;
            EndTime = endTime;
        }

        public string TraceSessionName { get; private set; }

        public uint EventsLostCount { get; private set; }

        public DateTime StartTime { get; private set; }

        public DateTime EndTime { get; private set; }
    }
}
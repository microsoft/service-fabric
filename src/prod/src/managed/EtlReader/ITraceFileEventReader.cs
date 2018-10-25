// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;

namespace Tools.EtlReader
{
    public interface ITraceFileEventReader : IDisposable
    {
        event EventHandler<EventRecordEventArgs> EventRead;

        TraceSessionMetadata ReadTraceSessionMetadata();
        
        uint EventsLost { get; }

        void ReadEvents(DateTime startTime, DateTime endTime);
    }
}
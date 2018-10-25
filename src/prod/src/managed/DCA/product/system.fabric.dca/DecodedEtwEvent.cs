// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    using System;
    using Tools.EtlReader;

    public struct DecodedEtwEvent
    {
        // The raw ETW event
        public EventRecord EventRecord;

        // Name of the task corresponding to the ETW event
        public string TaskName;

        // Name given to the ETW event
        public string EventType;

        // Level of the ETW event
        public byte Level;

        // Timestamp of the ETW event
        public DateTime Timestamp;

        // Thread ID of the thread that logged the ETW event
        public uint ThreadId;

        // Process ID of the process that logged the ETW event
        public uint ProcessId;

        // The ETW event decoded as a human-readable string
        public string EventText;

        // A string that contains the above fields in a human-readable, comma-separated form
        public string StringRepresentation;
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    using System;

    internal class CsvEvent
    {
        // Timestamp of the ETW event
        internal DateTime Timestamp;

        // Name given to the ETW event with Task Name
        internal string EventType;

        // Name given to the ETW event with Task Name
        internal string EventTypeWithoutId;

        // Task Name which sent the Event
        internal string TaskName;

        // Event Type
        internal string EventName;

        // Partition Id
        internal string EventNameId;

        // Level of the ETW event
        internal byte Level;

        // Thread ID of the thread that logged the ETW event
        internal uint ThreadId;

        // Process ID of the process that logged the ETW event
        internal uint ProcessId;

        // The ETW event decoded as a human-readable string
        internal string EventText;

        // A string that contains the above fields in a human-readable, comma-separated form
        internal string StringRepresentation;

        internal bool IsMalformed()
        {
            if ((this.Timestamp == null)
                || (this.EventType == null)
                || (this.EventTypeWithoutId == null)
                || (this.TaskName == null)
                || (this.EventName == null)
                || (this.EventNameId == null)
                || (this.EventText == null)
                || (this.StringRepresentation == null))
            {
                return true;
            }
            return false;
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace AzureTableUploaderTest
{
    using System;
    using System.Fabric.Common.Tracing;
    using System.Runtime.InteropServices;
    using Tools.EtlReader;

    abstract class EventSource
    {
        private Guid providerId;
        private DateTime startTime;

        protected EventSource(Guid providerId, DateTime startTime) 
        {
            this.providerId = providerId;
            this.startTime = startTime;
        }

        protected void InitializeEventRecord(
                        ushort taskId,
                        ushort eventId,
                        byte processorNumber,
                        int timestampOffsetMinutes,
                        ref EventRecord eventRecord,
                        out DateTime eventTimestamp)
        {
            eventTimestamp = this.startTime.AddMinutes(timestampOffsetMinutes);

            eventRecord.BufferContext.ProcessorNumber = processorNumber;
            eventRecord.EventHeader.ProcessId = 1234;
            eventRecord.EventHeader.ThreadId = 5678;
            eventRecord.EventHeader.ProviderId = this.providerId;
            eventRecord.EventHeader.TimeStamp = eventTimestamp.ToFileTimeUtc();
            eventRecord.EventHeader.EventDescriptor.Id = eventId;
            eventRecord.EventHeader.EventDescriptor.Task = taskId;
            eventRecord.EventHeader.EventDescriptor.Keyword = (ulong) FabricEvents.Keywords.ForQuery;
        }

        protected void UpdateEventRecordBuffer(ref EventRecord eventRecord, IntPtr userData, ushort userDataLength)
        {
            // UserData field is declared as readonly, so work around it and forcibly assign
            // our own buffer to it.
            unsafe
            {
                int userDataOffset = (int)Marshal.OffsetOf(typeof(EventRecord), "UserData");
                fixed (EventRecord* eventRecordPtr = &eventRecord)
                {
                    IntPtr* userDataPtr = (IntPtr*)(((IntPtr)eventRecordPtr) + userDataOffset);
                    *userDataPtr = userData;
                }
            }
            eventRecord.UserDataLength = userDataLength;
        }

        internal void FreeEventBuffers(EventRecord eventRecord)
        {
            Marshal.FreeHGlobal(eventRecord.UserData);
        }

        internal abstract bool GetNextEvent(out EventRecord eventRecord);
    }
}
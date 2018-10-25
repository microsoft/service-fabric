// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;
    using System.ComponentModel;
    using System.Fabric.Strings;
    using System.IO;
    using System.Runtime.InteropServices;

    public class TraceFileEventReader : ITraceFileEventReader
    {
        private const uint TraceModeEventRecord = 0x10000000;
        private const ulong InvalidHandle = 0x00000000FFFFFFFF;
        private static readonly Guid EventTraceSystem = new Guid("68fdd900-4a3e-11d1-84f4-0000f80464e3");

        private readonly string fileName;
        private ulong traceHandle;
        private EventTraceLogfile eventTraceLogFile;
        private GCHandle callbackHandle;

        // Whether or not the object has been disposed
        private bool disposed;

        public TraceFileEventReader(string fileName)
        {
            if (!File.Exists(fileName))
            {
                throw new FileNotFoundException(StringResources.ETLReaderError_TracefileNotFound, fileName);
            }

            this.fileName = fileName;
        }

        public event EventHandler<EventRecordEventArgs> EventRead;

        public uint EventsLost
        {
            get;
            private set;
        }

        internal uint TestEventsLost
        {
            get;
            set;
        }

        private void InitializeTraceHandle()
        {
            if (traceHandle == 0)
            {
                this.eventTraceLogFile = new EventTraceLogfile
                {
                    ProcessTraceMode = TraceModeEventRecord,
                    LogFileName = this.fileName
                };

                // wrap callback in GCHandle to prevent object from being garbage collected
                EventRecordCallback callback = this.EventRecordCallback;
                this.callbackHandle = GCHandle.Alloc(callback);
                this.eventTraceLogFile.EventRecordCallback = this.EventRecordCallback;

                this.traceHandle = NativeMethods.OpenTrace(ref eventTraceLogFile);
                if (this.traceHandle == InvalidHandle)
                {
                    throw new Win32Exception();
                }

                this.EventsLost = this.eventTraceLogFile.LogfileHeader.EventsLost;
            }
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }

            this.disposed = true;

            if (this.traceHandle != 0 && this.traceHandle != InvalidHandle)
            {
                NativeMethods.CloseTrace(this.traceHandle);
            }

            if (this.callbackHandle.IsAllocated)
            {
                this.callbackHandle.Free();
            }

            GC.SuppressFinalize(this);
        }

        public TraceSessionMetadata ReadTraceSessionMetadata()
        {
            // initialize trace handle
            InitializeTraceHandle();

            // This is an approximation based on the naming of the file.
            var traceSessionName = Path.GetFileName(this.eventTraceLogFile.LogFileName).Split('_' )[0];
            var eventsLost = (this.TestEventsLost > 0) ? (this.TestEventsLost) : (this.EventsLost);
            var sessionStartTime = DateTime.FromFileTimeUtc(this.eventTraceLogFile.LogfileHeader.StartTime);
            var sessionEndTime = DateTime.FromFileTimeUtc(this.eventTraceLogFile.LogfileHeader.EndTime);
            var traceSessionMetadata = new TraceSessionMetadata(traceSessionName, eventsLost, sessionStartTime, sessionEndTime);
            return traceSessionMetadata;
        }

        public void ReadEvents(DateTime startTime, DateTime endTime)
        {
            if (startTime > endTime)
            {
                throw new ArgumentException(StringResources.ETLReaderError_StartTimeGreaterThanEndTime, "startTime");
            }

            if (this.EventRead == null)
            {
                throw new InvalidOperationException(StringResources.ETLReaderError_NoSubscribersForEventRead);
            }

            // initialize trace handle
            InitializeTraceHandle();

            int error = ProcessTrace(startTime, endTime, this.traceHandle);
            if (error != 0)
            {
                throw new Win32Exception(error);
            }
        }

        private static int ProcessTrace(DateTime startTime, DateTime endTime, ulong traceHandle)
        {
            ulong[] array = { traceHandle };
            int error;
            unsafe
            {
                long* p1;
                long* p2;

                if (startTime != DateTime.MinValue)
                {
                    var t1 = startTime.ToFileTime();
                    p1 = &t1;
                }
                else
                {
                    p1 = null;
                }

                if (endTime != DateTime.MaxValue)
                {
                    var t2 = endTime.ToFileTime();
                    p2 = &t2;
                }
                else
                {
                    p2 = null;
                }

                error = NativeMethods.ProcessTrace(array, 1, p1, p2);
            }

            return error;
        }

        private uint EventRecordCallback([In] ref EventRecord eventRecord)
        {
            uint returnValue = uint.MaxValue;
            if (eventRecord.EventHeader.ProviderId != EventTraceSystem)
            {
                var e = new EventRecordEventArgs(eventRecord);

                // Guaranteed non-null due to check in ReadEvents()
                this.EventRead(this, e);

                if (e.Cancel)
                {
                    // Return 'FALSE' to cancel
                    returnValue = 0;
                }
            }

            return returnValue;
        }
    }
}
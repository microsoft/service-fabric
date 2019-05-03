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
    using System.Text;
    using System.Runtime.InteropServices;

    /**
    * In LTTng, each processor writes traces to a different file inside a folder hierarchy.
    * Thus the chronological sequence of trace events is spread across these files.
    * Therefore, this class reads traces from this folder instead of specific files.
    **/
    public class LttngTraceFolderEventReader : ITraceFileEventReader
    {
        private readonly string path;
        private LTTngTraceHandle traceHandle;
        private ulong lastEventReadTimestamp;

        // Whether or not the object has been disposed
        private bool disposed;

        public LttngTraceFolderEventReader(string path)
        {
            if (!Directory.Exists(path))
            {
                throw new DirectoryNotFoundException($"LTTng trace folder not found at: {path}");
            }

            this.path = path;
            LttngTraceFolderEventReader.clearLTTngTraceHandle(ref this.traceHandle);
        }

        public event EventHandler<EventRecordEventArgs> EventRead;

        // TODO - The value returned is not accurate
        // BugId: 13429860
        public uint EventsLost
        {
            get;
            private set;
        }

        // For unit testing
        internal uint TestEventsLost
        {
            get;
            set;
        }

        private void InitializeTraceHandle()
        {
            if (this.traceHandle.id < 0)
            {
                // initializing for reading traces with iterator at oldest trace event so we can query the total number of events lost in the session.
                StringBuilder sbPath = new StringBuilder();
                sbPath.Append(this.path);

                // trace is always initialized with first and last events since
                // the timerange is always reset for each ReadEvents call
                LttngReaderStatusCode res = LttngReaderBindings.initialize_trace_processing(
                    sbPath,
                    (ulong)LTTngTraceTimestampFlag.FIRST_TRACE_TIMESTAMP,
                    (ulong)LTTngTraceTimestampFlag.LAST_TRACE_TIMESTAMP,
                    out this.traceHandle);

                if (res != LttngReaderStatusCode.SUCCESS)
                {
                    string errorMessage = LttngReaderStatusMessage.GetMessage(res);
                    throw new InvalidDataException($"{errorMessage}. Occured while trying to initialize processing of LTTng traces for folder {this.path}");
                }

                this.EventsLost = (uint)this.traceHandle.trace_info.events_lost;
            }
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }

            this.disposed = true;

            if (this.traceHandle.id >= 0)
            {
                LttngReaderBindings.finalize_trace_processing(ref this.traceHandle);
            }

            GC.SuppressFinalize(this);
        }

        public TraceSessionMetadata ReadTraceSessionMetadata()
        {
            // initialize trace handle
            InitializeTraceHandle();

            // This is an approximation based on the naming of the file.
            var traceSessionName = Path.GetFileName(this.path).Split('_' )[0];
            var eventsLost = (this.TestEventsLost > 0) ? (this.TestEventsLost) : (this.EventsLost);
            var sessionStartTime = LttngTraceFolderEventReader.ConvertFromUnixEpoch(this.traceHandle.trace_info.start_time);
            var sessionEndTime = LttngTraceFolderEventReader.ConvertFromUnixEpoch(this.traceHandle.trace_info.end_time);
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

            ulong eventReadFailureCount;

            LttngReaderStatusCode res = ProcessTrace(startTime, endTime, ref this.traceHandle, out eventReadFailureCount);

            if (res != LttngReaderStatusCode.SUCCESS && res != LttngReaderStatusCode.END_OF_TRACE)
            {
                string errorMessage = LttngReaderStatusMessage.GetMessage(res);
                throw new InvalidDataException(
                    $"{errorMessage}. When processing traces at folder: {this.path}.\n" + 
                    $"Last event successfully read has timestamp: {LttngTraceFolderEventReader.ConvertFromUnixEpoch(this.lastEventReadTimestamp).ToString("o")}\n" +
                    $"Total events events skipped due to failure before exception : {eventReadFailureCount}");
            }

            if (eventReadFailureCount != 0)
            {
                throw new InvalidDataException(
                    $"Failed to read {eventReadFailureCount} events from trace at {this.path}" +
                    $"StartTime: {startTime}, EndTime: {endTime}");
            }
        }

        public static DateTime ConvertFromUnixEpoch(ulong nanoSeconds)
        {
            switch(nanoSeconds)
            {
                case (ulong)LTTngTraceTimestampFlag.FIRST_TRACE_TIMESTAMP:
                    return DateTime.MinValue;
                case (ulong)LTTngTraceTimestampFlag.LAST_TRACE_TIMESTAMP:
                    return DateTime.MaxValue;
                default:
                    return (new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc)).AddTicks((long)(nanoSeconds/100));
            }
        }

        public static ulong ConvertToUnixEpoch(DateTime time)
        {
            if(time == DateTime.MinValue)
            {
                return (ulong)LTTngTraceTimestampFlag.FIRST_TRACE_TIMESTAMP;
            }
            else if (time == DateTime.MaxValue)
            {
                return (ulong)LTTngTraceTimestampFlag.LAST_TRACE_TIMESTAMP;
            }
  
            return (ulong)(time.ToUniversalTime() - (new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc))).Ticks * 100;
        }

        private static void clearLTTngTraceHandle(ref LTTngTraceHandle handle)
        {
            handle.id = -1;
            handle.ctx = IntPtr.Zero;
            handle.ctf_iter = IntPtr.Zero;
            handle.last_iter_pos = IntPtr.Zero;
        }

        private LttngReaderStatusCode ProcessTrace(DateTime startTime, DateTime endTime, ref LTTngTraceHandle traceHandle, out ulong eventReadFailureCount)
        {
            EventRecord eventRecord = new EventRecord();
            ulong lastReadTimestamp = 0;

            // needed for processing unstructured events in Linux
            bool isUnstructured = false;
            StringBuilder unstructuredRecord = new StringBuilder(LttngReaderBindings.MAX_LTTNG_UNSTRUCTURED_EVENT_LEN + 1);
            StringBuilder taskNameEventName = new StringBuilder(LttngReaderBindings.MAX_LTTNG_TASK_EVENT_NAME_LEN + 1);

            ulong startTimeEpochNanoS = LttngTraceFolderEventReader.ConvertToUnixEpoch(startTime);
            ulong endTimeEpochNanoS = LttngTraceFolderEventReader.ConvertToUnixEpoch(endTime);

            // set start and end time for reading traces
            LttngReaderStatusCode res = LttngReaderBindings.set_start_end_time(ref traceHandle, startTimeEpochNanoS, endTimeEpochNanoS);

            eventReadFailureCount = 0;

            if (res != LttngReaderStatusCode.SUCCESS)
            {
                return res;
            }

            res = LttngReaderBindings.read_next_event(ref traceHandle, ref eventRecord, unstructuredRecord, taskNameEventName, ref lastReadTimestamp, ref isUnstructured);

            while (res == LttngReaderStatusCode.SUCCESS || res == LttngReaderStatusCode.FAILED_TO_READ_EVENT)
            {
                if (res == LttngReaderStatusCode.FAILED_TO_READ_EVENT)
                {
                    eventReadFailureCount++;
                    continue;
                }

                EventRecordEventArgs eventRecordEventArgs;
                this.lastEventReadTimestamp = lastReadTimestamp;

                if (isUnstructured)
                {
                    try
                    {
                        eventRecord.EventHeader.TimeStamp = LttngTraceFolderEventReader.ConvertFromUnixEpoch(lastReadTimestamp).ToFileTimeUtc();
                    }
                    catch (Exception)
                    {
                        // Flagging error with minimum timestamp so we can continue processing other events.
                        eventRecord.EventHeader.TimeStamp =  0;
                    }

                    eventRecordEventArgs = new EventRecordEventArgs(eventRecord);

                    eventRecordEventArgs.IsUnstructured = isUnstructured;
                    eventRecordEventArgs.TaskNameEventName = taskNameEventName.ToString();
                    eventRecordEventArgs.UnstructuredRecord = unstructuredRecord.ToString();
                }
                else
                {
                    eventRecordEventArgs = new EventRecordEventArgs(eventRecord);
                }

                this.EventRead(this, eventRecordEventArgs);

                if (eventRecordEventArgs.Cancel)
                {
                    return LttngReaderStatusCode.SUCCESS;
                }

                res = LttngReaderBindings.read_next_event(ref traceHandle, ref eventRecord, unstructuredRecord, taskNameEventName, ref lastReadTimestamp, ref isUnstructured);
            }

            return res;
        }
    }
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;
    using System.Runtime.InteropServices;
    using System.Text;

    [StructLayout(LayoutKind.Sequential)]
    public struct EVENT_DESCRIPTOR
    {
        public ushort Id;
        public byte   Version;
        public byte   Channel;
        public byte   Level;
        public byte   Opcode;
        public ushort Task;
        public ulong  Keyword;
    }

    // Flags for specifying the first and last trace events
    public enum LTTngTraceTimestampFlag : ulong
    {
        FIRST_TRACE_TIMESTAMP = ulong.MinValue,
        LAST_TRACE_TIMESTAMP = ulong.MaxValue
    }

    public class LttngReaderBindings
    {
        public const int MAX_LTTNG_EVENT_DATA_SIZE = 65536;

        public const int MAX_LTTNG_UNSTRUCTURED_EVENT_LEN = MAX_LTTNG_EVENT_DATA_SIZE / sizeof(char) -1;

        public const int MAX_LTTNG_TASK_EVENT_NAME_LEN = 1000;

        // Initializes the processing of traces by creating a context and event iterator given the folder containing the traces
        [DllImport("LttngReader.so", CharSet = CharSet.Ansi)]
        public static extern LttngReaderStatusCode initialize_trace_processing(StringBuilder trace_file_folder_path, ulong start_time_epoch_ns, ulong end_time_epoch_ns, out LTTngTraceHandle trace_handle);

        // Cleans up the context created and iterator
        [DllImport("LttngReader.so", CharSet = CharSet.Ansi)]
        public static extern LttngReaderStatusCode finalize_trace_processing(ref LTTngTraceHandle trace_handle);

        // Reads the next trace event and moves iterator to next one returns 0 on success.
        [DllImport("LttngReader.so", CharSet = CharSet.Unicode)]
        public static extern LttngReaderStatusCode read_next_event(ref LTTngTraceHandle trace_handle, ref EventRecord evDesc, StringBuilder unstructured_event_text, [MarshalAs(UnmanagedType.LPStr)] StringBuilder taskname_eventname, ref ulong timestamp_unix_epoch_nano_s, ref bool is_unstructured);

        [DllImport("LttngReader.so")]
        public static extern LttngReaderStatusCode set_start_end_time(ref LTTngTraceHandle handle, ulong start_time_epoch, ulong end_time_epoch);

        // Gets the name of an active Lttng session that starts with the provided value. The output_name buffer needs to be large enough to handle the name
        [DllImport("LttngReader.so", CharSet = CharSet.Ansi)]
        public static extern LttngReaderStatusCode get_active_lttng_session_path(StringBuilder session_name_startswith, StringBuilder output_active_session_path, ulong active_session_buffer_size);
    }
}


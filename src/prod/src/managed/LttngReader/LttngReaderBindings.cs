// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace LttngReader
{
    using System;
    using System.Runtime.InteropServices;
    using System.Text;
    using Tools.EtlReader;

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

    public class LttngReaderBindings
    {
        // Initializes the processing of traces by creating a context and event iterator given the folder containing the traces
        [DllImport("LttngReader.so", CharSet = CharSet.Ansi)]
        public static extern int initialize_trace_processing(StringBuilder trace_file_folder_path, out IntPtr ctx, out IntPtr ctf_iter, bool seek_timestamp, ulong timestamp_unix_epoch_nano_s);

        // Cleans up the context created and iterator
        [DllImport("LttngReader.so", CharSet = CharSet.Ansi)]
        public static extern int finalize_trace_processing(IntPtr ctx, IntPtr ctf_iter, int t_handle_id);

        // Reads the next trace event and moves iterator to next one returns 0 on success.
        [DllImport("LttngReader.so", CharSet = CharSet.Unicode)]
        public static extern int read_next_event(ref EventRecord evDesc, StringBuilder unstructured_event_text, [MarshalAs(UnmanagedType.LPStr)] StringBuilder taskname_eventname, IntPtr ctf_iter, ref ulong timestamp_unix_epoch_nano_s, ref bool is_unstructured);

        // Gets the name of an active Lttng session that starts with the provided value. The output_name buffer needs to be large enough to handle the name
        [DllImport("LttngReader.so", CharSet = CharSet.Ansi)]
        public static extern bool get_active_lttng_session_path(StringBuilder session_name_startswith, StringBuilder output_active_session_path, ulong active_session_buffer_size);
    }
}




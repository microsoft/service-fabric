// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Runtime.InteropServices;
    using System.Security;

    [SuppressUnmanagedCodeSecurityAttribute]
    internal static class NativeMethods 
    {
        [SuppressMessage("Microsoft.Security", "CA2118", Justification = "None of the arguments to these methods can be directly set publically")]
        [DllImport("advapi32.dll", ExactSpelling = true, EntryPoint = "OpenTraceW", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern ulong OpenTrace(ref EventTraceLogfile logfile);

        [SuppressMessage("Microsoft.Security", "CA2118", Justification = "None of the arguments to these methods can be directly set publically")]
        [DllImport("advapi32.dll", ExactSpelling = true, EntryPoint = "StartTraceW", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern int StartTrace(out ulong SessionHandle, [MarshalAs(UnmanagedType.LPWStr)] string SessionName, ref EventTraceProperties Properties);

        [SuppressMessage("Microsoft.Security", "CA2118", Justification = "None of the arguments to these methods can be directly set publically")]
        [DllImport("advapi32.dll", ExactSpelling = true, EntryPoint = "ControlTraceW", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern int ControlTrace(ulong SessionHandle, string SessionName, ref EventTraceProperties Properties, uint ControlCode);

        [SuppressMessage("Microsoft.Security", "CA2118", Justification = "None of the arguments to these methods can be directly set publically")]
        [DllImport("advapi32.dll", ExactSpelling = true, EntryPoint = "EnableTraceEx", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern int EnableTraceEx(
            ref Guid ProviderId,
            IntPtr SourceId,
            ulong TraceHandle,
            uint IsEnabled,
            byte Level,
            ulong MatchAnyKeyword,
            ulong MatchAllKeyword,
            uint EnableProperty,
            IntPtr EnableFilterDesc);

        [SuppressMessage("Microsoft.Security", "CA2118", Justification = "None of the arguments to these methods can be directly set publically")]
        [DllImport("advapi32.dll", ExactSpelling = true, EntryPoint = "FlushTraceW", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern int FlushTrace(ulong SessionHandle, string SessionName, ref EventTraceProperties Properties);

        [SuppressMessage("Microsoft.Security", "CA2118", Justification = "None of the arguments to these methods can be directly set publically")]
        [DllImport("advapi32.dll", ExactSpelling = true, EntryPoint = "ProcessTrace", SetLastError = true)]
        public static unsafe extern int ProcessTrace(
            ulong[] HandleArray,
            uint HandleCount,
            long* StartTime,
            long* EndTime);

        [SuppressMessage("Microsoft.Security", "CA2118", Justification = "None of the arguments to these methods can be directly set publically")]
        [DllImport("advapi32.dll", ExactSpelling = true, EntryPoint = "CloseTrace")]
        public static extern int CloseTrace(ulong traceHandle);

        [SuppressMessage("Microsoft.Security", "CA2118", Justification = "None of the arguments to these methods can be directly set publically")]
        [DllImport("tdh.dll", ExactSpelling = true, EntryPoint = "TdhGetEventInformation")]
        public static extern int TdhGetEventInformation(
            ref EventRecord Event,
            uint TdhContextCount,
            IntPtr TdhContext,
            [Out] IntPtr eventInfoPtr,
            ref int BufferSize);
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;

    [DataContract]
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    internal struct EventTraceLogfile
    {
        [DataMember]
        [MarshalAs(UnmanagedType.LPWStr)]
        public string LogFileName;

        [DataMember]
        [MarshalAs(UnmanagedType.LPWStr)]
        public string LoggerName;

        [DataMember]
        public long CurrentTime;

        [DataMember]
        public uint BuffersRead;

        [DataMember]
        public uint ProcessTraceMode;

        [DataMember]
        public EventTrace CurrentEvent;

        [DataMember]
        public TraceLogfileHeader LogfileHeader;

        [DataMember]
        public BufferCallback BufferCallback;

        [DataMember]
        public uint BufferSize;

        [DataMember]
        public uint Filled;

        [DataMember]
        public uint EventsLost;

        [DataMember]
        public EventRecordCallback EventRecordCallback;

        [DataMember]
        public uint IsKernelTrace;

        [DataMember]
        public IntPtr Context;
    }
}
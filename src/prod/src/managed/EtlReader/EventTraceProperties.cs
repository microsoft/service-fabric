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
    internal struct EventTraceProperties
    {
        [DataMember]
        public const int MaxLoggerNameLength = 1024;

        [DataMember]
        public const int MaxLogFileNameLength = 1024;

        [DataMember]
        public WnodeHeader Wnode;

        [DataMember]
        public uint BufferSize;

        [DataMember]
        public uint MinimumBuffers;

        [DataMember]
        public uint MaximumBuffers;

        [DataMember]
        public uint MaximumFileSize;

        [DataMember]
        public uint LogFileMode;

        [DataMember]
        public uint FlushTimer;

        [DataMember]
        public uint EnableFlags;

        [DataMember]
        public int AgeLimit;

        [DataMember]
        public uint NumberOfBuffers;

        [DataMember]
        public uint FreeBuffers;

        [DataMember]
        public uint EventsLost;

        [DataMember]
        public uint BuffersWritten;

        [DataMember]
        public uint LogBuffersLost;

        [DataMember]
        public uint RealTimeBuffersLost;

        [DataMember]
        public IntPtr LoggerThreadId;

        [DataMember]
        public uint LogFileNameOffset;

        [DataMember]
        public uint LoggerNameOffset;

        // Reserve buffer space so the ETW system can fill this with the logger name
        [DataMember]
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = EventTraceProperties.MaxLoggerNameLength)]
        public string LoggerName;

        // Reserve buffer space so the ETW system can fill this with the log file name
        [DataMember]
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = EventTraceProperties.MaxLogFileNameLength)]
        public string LogFileName;

        [DataContract]
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        internal struct WnodeHeader
        {
            [DataMember]
            public uint BufferSize;

            [DataMember]
            public uint ProviderId;

            [DataMember]
            public ulong HistoricalContext;

            [DataMember]
            public long TimeStamp;

            [DataMember]
            public Guid Guid;

            [DataMember]
            public uint ClientContext;

            [DataMember]
            public uint Flags;
        }
    }
}
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
    [StructLayout(LayoutKind.Sequential)]
    internal struct TraceLogfileHeader 
    {
        [DataMember]
        public uint BufferSize;

        [DataMember]
        public uint Version;

        [DataMember]
        public uint ProviderVersion;

        [DataMember]
        public uint NumberOfProcessors;

        [DataMember]
        public long EndTime;

        [DataMember]
        public uint TimerResolution;

        [DataMember]
        public uint MaximumFileSize;

        [DataMember]
        public uint LogFileMode;

        [DataMember]
        public uint BuffersWritten;

        [DataMember]
        public uint StartBuffers;

        [DataMember]
        public uint PointerSize;

        [DataMember]
        public uint EventsLost;

        [DataMember]
        public uint CpuSpeedInMHz;

        [DataMember]
        public IntPtr LoggerName;

        [DataMember]
        public IntPtr LogFileName;

        [DataMember]
        public Win32TimeZoneInfo TimeZone;

        [DataMember]
        public long BootTime;

        [DataMember]
        public long PerfFreq;

        [DataMember]
        public long StartTime;

        [DataMember]
        public uint ReservedFlags;

        [DataMember]
        public uint BuffersLost;
    }
}
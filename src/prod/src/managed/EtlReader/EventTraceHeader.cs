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
    internal struct EventTraceHeader
    {
        [DataMember]
        public ushort Size;

        [DataMember]
        public ushort FieldTypeFlags;

        [DataMember]
        public uint Version;

        [DataMember]
        public uint ThreadId;

        [DataMember]
        public uint ProcessId;

        [DataMember]
        public long TimeStamp;

        [DataMember]
        public Guid Guid;

        [DataMember]
        public uint KernelTime;

        [DataMember]
        public uint UserTime;
    }
}
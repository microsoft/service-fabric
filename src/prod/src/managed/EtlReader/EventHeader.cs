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
    public struct EventHeader
    {
        [DataMember]
        public ushort Size;

        [DataMember]
        public ushort HeaderType;

        [DataMember]
        public ushort Flags;

        [DataMember]
        public ushort EventProperty;

        [DataMember]
        public uint ThreadId;

        [DataMember]
        public uint ProcessId;

        [DataMember]
        public long TimeStamp;

        [DataMember]
        public Guid ProviderId;

        [DataMember]
        public EventDescriptor EventDescriptor;

        [DataMember]
        public ulong ProcessorTime;

        [DataMember]
        public Guid ActivityId;
    }
}
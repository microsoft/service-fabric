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
    public struct EventDescriptor
    {
        [DataMember]
        public ushort Id;

        [DataMember]
        public byte Version;

        [DataMember]
        public byte Channel;

        [DataMember]
        public byte Level;

        [DataMember]
        public byte Opcode;

        [DataMember]
        public ushort Task;

        [DataMember]
        public ulong Keyword;
    }
}
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
    public struct EventRecord
    {
        [DataMember]
        public EventHeader EventHeader;

        [DataMember]
        public EtwBufferContext BufferContext;

        [DataMember]
        public ushort ExtendedDataCount;

        [DataMember]
        public ushort UserDataLength;

        [DataMember]
        public readonly IntPtr ExtendedData;

        [DataMember]
        public readonly IntPtr UserData;

        [DataMember]
        public readonly IntPtr UserContext;
    }
}
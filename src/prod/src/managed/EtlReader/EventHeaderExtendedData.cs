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
    public struct EventHeaderExtendedData
    {
        [DataMember]
        public ushort Reserved1;

        [DataMember]
        public ushort ExtType;

        [DataMember]
        public ushort Reserved2;

        [DataMember]
        public ushort DataSize;

        [DataMember]
        public ulong DataPtr;
    }
}
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
    public struct EventDataDescriptor
    {
        [DataMember]
        public ulong Ptr;

        [DataMember]
        public uint Size;

        [DataMember]
        public uint Reserved;
    }
}
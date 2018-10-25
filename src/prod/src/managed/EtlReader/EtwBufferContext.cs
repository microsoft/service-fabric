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
    public struct EtwBufferContext
    {
        [DataMember]
        public byte ProcessorNumber;

        [DataMember]
        public byte Alignment;

        [DataMember]
        public ushort LoggerId;
    }
}
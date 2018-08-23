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
    [StructLayout(LayoutKind.Explicit)]
    internal struct EventPropertyInfo
    {
        [DataMember]
        [FieldOffset(0)]
        public PropertyFlags Flags;

        [DataMember]
        [FieldOffset(4)]
        public uint NameOffset;

        [DataMember]
        [FieldOffset(8)]
        public NonStructType NonStructTypeValue;

        [DataMember]
        [FieldOffset(8)]
        public StructType StructTypeValue;

        [DataMember]
        [FieldOffset(16)]
        public ushort CountPropertyIndex;

        [DataMember]
        [FieldOffset(18)]
        public ushort LengthPropertyIndex;

        [DataMember]
        [FieldOffset(20)]
        private uint reserved;

        [DataContract]
        [StructLayout(LayoutKind.Sequential)]
        public struct NonStructType
        {
            [DataMember]
            public TdhInType InType;

            [DataMember]
            public TdhOutType OutType;

            [DataMember]
            public uint MapNameOffset;
        }

        [DataContract]
        [StructLayout(LayoutKind.Sequential)]
        public struct StructType
        {
            [DataMember]
            public ushort StructStartIndex;

            [DataMember]
            public ushort NumOfStructMembers;

            [DataMember]
            private uint padding;
        }
    }
}
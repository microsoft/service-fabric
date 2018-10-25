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
    internal struct EventTrace
    {
        [DataMember]
        public EventTraceHeader Header;

        [DataMember]
        public uint InstanceId;

        [DataMember]
        public uint ParentInstanceId;

        [DataMember]
        public Guid ParentGuid;

        [DataMember]
        public IntPtr MofData;

        [DataMember]
        public uint MofLength;

        [DataMember]
        public uint ClientContext;
    }
}
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
    internal struct SystemTime
    {
        [DataMember]
        public short Year;

        [DataMember]
        public short Month;

        [DataMember]
        public short DayOfWeek;

        [DataMember]
        public short Day;

        [DataMember]
        public short Hour;

        [DataMember]
        public short Minute;

        [DataMember]
        public short Second;

        [DataMember]
        public short Milliseconds;
    }
}
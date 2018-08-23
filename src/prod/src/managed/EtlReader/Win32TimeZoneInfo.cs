// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    internal struct Win32TimeZoneInfo
    {
        public int Bias;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 32)]
        public char[] StandardName;
        public SystemTime StandardDate;
        public int StandardBias;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 32)]
        public char[] DaylightName;
        public SystemTime DaylightDate;
        public int DaylightBias;
    }
}
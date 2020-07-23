// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Common
{
    using System;
    using System.Runtime.InteropServices;

    [Serializable]
    [StructLayout(LayoutKind.Explicit, Size = 16)]
    public struct WinFabricNodeId
    {
        private static readonly WinFabricNodeId one = new WinFabricNodeId(0, 1);
        private static readonly WinFabricNodeId zero = new WinFabricNodeId(0, 0);
        private static readonly WinFabricNodeId zeroMinusOne = new WinFabricNodeId(ulong.MaxValue, ulong.MaxValue);

        private const int BitsPerPart = 64;

        [FieldOffset(0)]
        private ulong high;

        [FieldOffset(8)]
        private ulong low;

        public WinFabricNodeId(ulong high, ulong low)
        {
            this.high = high;
            this.low = low;
        }

        public ulong High { get { return this.high; } }

        public ulong Low { get { return this.low; } }
    }
}
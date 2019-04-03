// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;
    using System.Runtime.InteropServices;

    // Main struct for manipulating traces
    [StructLayout(LayoutKind.Sequential)]
    public struct LTTngTraceHandle
    {
        public int id;
        public IntPtr ctx;
        public IntPtr ctf_iter;
        public LTTngTraceInfo trace_info;

        // for allowing freeing used memory
        public IntPtr last_iter_pos;

        public IntPtr data_buffer;
    };
}
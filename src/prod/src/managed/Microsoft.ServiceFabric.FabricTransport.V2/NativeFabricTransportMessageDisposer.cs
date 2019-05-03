// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport.V2
{
    using System;
    using System.Runtime.InteropServices;

    class NativeFabricTransportMessageDisposer : NativeFabricTransport.IFabricTransportMessageDisposer
    {
        public unsafe void Dispose(uint count, IntPtr messages)
        {
            var sizeOfPtr = Marshal.SizeOf(typeof(IntPtr));
            for (var i = 0; i < count; i++)
                unsafe
                {
                    var msgBuffer =
                        (NativeFabricTransport.IFabricTransportMessage) Marshal.GetObjectForIUnknown(Marshal.ReadIntPtr(
                            IntPtr.Add(messages, i * sizeOfPtr)));
                    msgBuffer.Dispose();
                }

            GC.KeepAlive(messages);
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Replication
{
    using System;
    using System.Fabric.Interop;
    using System.Runtime.InteropServices;

    internal class OperationData : NativeRuntime.IFabricOperationData
    {
        private readonly uint size;
        private byte[] buffer = null;

        public OperationData(uint size)
        {
            this.size = size;
        }

        public IntPtr GetData(out UInt32 length)
        {
            length = 1;

            if (buffer == null)
            {
                this.buffer = new byte[this.size];                
            }

            GCHandle handle = GCHandle.Alloc(this.buffer, GCHandleType.Pinned);

            var operationDataBuffer = new NativeTypes.FABRIC_OPERATION_DATA_BUFFER();

            operationDataBuffer.BufferSize = this.size;
            operationDataBuffer.Buffer = handle.AddrOfPinnedObject();

            GCHandle operationHandle = GCHandle.Alloc(operationDataBuffer, GCHandleType.Pinned);

            return operationHandle.AddrOfPinnedObject();
        }
    }
}
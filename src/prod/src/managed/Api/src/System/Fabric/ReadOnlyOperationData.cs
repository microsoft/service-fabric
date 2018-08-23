// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;

    internal sealed class ReadOnlyOperationData : IOperationData
    {
        private readonly NativeRuntime.IFabricOperationData nativeOperationData;

        internal ReadOnlyOperationData(NativeRuntime.IFabricOperationData operationData)
        {
            Requires.Argument("operationData", operationData).NotNull();

            this.nativeOperationData = operationData;
        }

        internal NativeRuntime.IFabricOperationData NativeOperationData
        {
            get { return this.nativeOperationData; }
        } 

        public IEnumerator<ArraySegment<byte>> GetEnumerator()
        {
            uint count = 0;
            IntPtr buffer = this.nativeOperationData.GetData(out count);

            if (count == 0 || buffer == IntPtr.Zero)
            {
                yield break;
            }
            
            for (int i = 0; i < count; i++)
            {
                ArraySegment<byte> segment;

                segment = GetSegment(buffer, i);

                yield return segment;
            }
        }

        Collections.IEnumerator Collections.IEnumerable.GetEnumerator()
        {
            return this.GetEnumerator();
        }

        private static ArraySegment<byte> GetSegment(IntPtr buffer, int index)
        {
            unsafe
            {
                NativeTypes.FABRIC_OPERATION_DATA_BUFFER* nativeBuffer = (NativeTypes.FABRIC_OPERATION_DATA_BUFFER*)(buffer + (Marshal.SizeOf(typeof(NativeTypes.FABRIC_OPERATION_DATA_BUFFER)) * index));

                return new ArraySegment<byte>(NativeTypes.FromNativeBytes(nativeBuffer->Buffer, nativeBuffer->BufferSize));
            }
        }
    }
}
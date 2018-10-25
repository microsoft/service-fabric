// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Interop
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Linq;
    using System.Runtime.InteropServices;

    internal sealed class OperationDataFactoryWrapper
    {
        private readonly NativeRuntime.IOperationDataFactory operationDataFactory;

        public OperationDataFactoryWrapper(NativeRuntime.IOperationDataFactory operationDataFactory)
        {
            Requires.Argument("operationDataFactory", operationDataFactory).NotNull();

            this.operationDataFactory = operationDataFactory;
        }

        public unsafe NativeRuntime.IFabricOperationData CreateOperationData(IOperationData data)
        {
            if (data == null)
            {
                return null;
            }

            uint[] sizes = data.Select(element => (uint)element.Count).ToArray();

            if (sizes.Length == 0)
            {
                return null;
            }

            return CreateNativeOperationData(data, sizes);
        }

        public unsafe NativeRuntime.IFabricOperationData CreateOperationData(OperationData data)
        {
            if (data == null || data.Count == 0)
            {
                return null;
            }

            int index = 0;
            int totalBytesBeingAllocatedInNativeCode = 0;

            // create an array of uints that represents the size of each array segment 
            // for which we want the native operation data to be created
            uint[] sizes = new uint[data.Count];
            foreach (var item in data)
            {
                sizes[index++] = (uint)item.Count;

                // for each item native code will allocate a byte array of the size of that item
                // it will also allocate a NativeTypes.FABRIC_OPERATION_DATA_BUFFER struct
                totalBytesBeingAllocatedInNativeCode += item.Count + Marshal.SizeOf(typeof(NativeTypes.FABRIC_OPERATION_DATA_BUFFER));
            }

            return CreateNativeOperationData(data, sizes);
        }

        unsafe private NativeRuntime.IFabricOperationData CreateNativeOperationData(IOperationData data, uint[] sizes)
        {
            NativeRuntime.IFabricOperationData operationData = null;
            using (var pin = new PinBlittable(sizes))
            {
                operationData = this.operationDataFactory.CreateOperationData(pin.AddrOfPinnedObject(), (uint)sizes.Length);
            }

            // get a pointer to the memory that native code allocated in the operation data it created for us
            uint countAllocatedByNative;
            IntPtr nativeBuffersRaw = operationData.GetData(out countAllocatedByNative);

            ReleaseAssert.AssertIfNot(countAllocatedByNative == (uint)sizes.Length, StringResources.Error_BufferNumberMismatach);
            NativeTypes.FABRIC_OPERATION_DATA_BUFFER* nativeBuffers = (NativeTypes.FABRIC_OPERATION_DATA_BUFFER*)nativeBuffersRaw;

            // copy the data into the buffers allocated by native
            int index = 0;
            foreach (var item in data)
            {
                NativeTypes.FABRIC_OPERATION_DATA_BUFFER* nativeBuffer = nativeBuffers + index;
                ReleaseAssert.AssertIfNot(nativeBuffer->BufferSize == sizes[index], string.Format(CultureInfo.CurrentCulture, StringResources.Error_BufferAllocationSizeMismatch_Formatted, index, nativeBuffer->BufferSize, sizes[index]));

                Marshal.Copy(item.Array, item.Offset, nativeBuffer->Buffer, item.Count);
                index++;
            }

            return operationData;
        }
    }
}
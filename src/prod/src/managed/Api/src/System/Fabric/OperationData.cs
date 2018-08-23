// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Linq;
    using System.Runtime.InteropServices;

    /// <summary>
    /// <para>An <see cref="System.Fabric.OperationData" /> is used to transfer copy state changes and copy context between replicas.</para>
    /// </summary>
    public class OperationData : Collection<ArraySegment<byte>>, IOperationData
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.OperationData" /> class.</para>
        /// </summary>
        public OperationData()
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.OperationData" /> class from the specified collection of 
        /// <see cref="System.ArraySegment{T}" /> of bytes.</para>
        /// </summary>
        /// <param name="operationData">
        /// <para>The bytes from which to create 
        /// the <see cref="System.Fabric.OperationData" /> object.</para>
        /// </param>
        public OperationData(IEnumerable<ArraySegment<byte>> operationData)
            : base(operationData == null ? null : operationData.ToList())
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.OperationData" /> class from the specified <see cref="System.ArraySegment{T}" /> of bytes.</para>
        /// </summary>
        /// <param name="operationData">
        /// <para>The <see cref="System.ArraySegment{T}" /> of bytes from which to create the <see cref="System.Fabric.OperationData" /> object.</para>
        /// </param>
        public OperationData(ArraySegment<byte> operationData)
        {
            this.Add(operationData);
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.OperationData" /> class from the specified collection of byte arrays.</para>
        /// </summary>
        /// <param name="operationData">
        /// <para>The <see cref="System.Collections.Generic.IEnumerable{T}" /> of byte arrays from which to create the <see cref="System.Fabric.OperationData" /> object.</para>
        /// </param>
        public OperationData(IEnumerable<byte[]> operationData)
            : this(operationData == null ? null : operationData.Select(data => new ArraySegment<byte>(data)))
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.OperationData" /> class from the specified byte array.</para>
        /// </summary>
        /// <param name="operationData">
        /// <para>The byte array from which to create the <see cref="System.Fabric.OperationData" /> object.</para>
        /// </param>
        public OperationData(byte[] operationData)
            : this(new ArraySegment<byte>(operationData))
        {
        }

        // Private constructor. Enables adding readonly list from CreateFromNative.
        private OperationData(IList<ArraySegment<byte>> operationData)
            : base(operationData)
        {
        }

        internal static OperationData CreateFromNative(NativeRuntime.IFabricOperationData operationData)
        {
            // null operationData is already handled by the caller.
            Requires.Argument("operationData", operationData).NotNull();

            uint count;
            IntPtr buffer = operationData.GetData(out count);

            return OperationData.CreateFromNative(count, buffer);
        }

        internal static OperationData CreateFromNative(uint count, IntPtr buffer)
        {
            if (count == 0)
            {
                return new OperationData();
            }

            // We have the count > 0 but buffers are empty
            if (buffer == IntPtr.Zero)
            {
                throw new InvalidOperationException(StringResources.Error_InvalidDataReceived);
            }

            return new OperationData(OperationData.CreateFromNativeInternal(count, buffer));
        }

        private static unsafe IList<ArraySegment<byte>> CreateFromNativeInternal(uint count, IntPtr buffer)
        {
            List<ArraySegment<byte>> returnValue = new List<ArraySegment<byte>>();

            for (int i = 0; i < count; i++)
            {
                NativeTypes.FABRIC_OPERATION_DATA_BUFFER* nativeBuffer = (NativeTypes.FABRIC_OPERATION_DATA_BUFFER*)(buffer + (Marshal.SizeOf(typeof(NativeTypes.FABRIC_OPERATION_DATA_BUFFER)) * i));
                returnValue.Add(new ArraySegment<byte>(NativeTypes.FromNativeBytes(nativeBuffer->Buffer, nativeBuffer->BufferSize)));
            }

            return returnValue.AsReadOnly();
        }
    }
}
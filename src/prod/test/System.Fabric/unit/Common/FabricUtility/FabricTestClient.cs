// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Common.FabricUtility
{
    using System;
    using System.Fabric.Interop;
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Threading.Tasks;

    class FabricTestClient
    {
        private readonly IFabricTestClient nativeTestClient;

        public FabricTestClient(FabricClient client)
        {
            if (client != null)
            {
                this.Client = client;
                this.nativeTestClient = (IFabricTestClient)client.NativePropertyClient;
            }
        }

        public FabricClient Client { get; private set; }

        public Guid GetNthNamingPartitionId(int n)
        {
            try
            {
                Guid partitionId;
                this.nativeTestClient.GetNthNamingPartitionId(
                    (UInt32)n,
                    out partitionId);
                return partitionId;
            }
            catch (COMException e)
            {
                throw Utility.TranslateCOMExceptionToManaged(e, "FabricTestClient.GetPartition");
            }
        }

        public Task<NativeClient.IFabricResolvedServicePartitionResult> ResolvePartitionAsync(Guid partitionId, TimeSpan timeout)
        {
            return Utility.WrapNativeAsyncInvoke(
                (callback) => this.nativeTestClient.BeginResolvePartition(ref partitionId, Utility.ToMilliseconds(timeout, "timeout"), callback),
                (context) => this.nativeTestClient.EndResolvePartition(context),
                CancellationToken.None,
                "ResolvePartitionAsync");

        }

        public string GetCurrentGatewayAddress()
        {
            try
            {
                var stringResult = this.nativeTestClient.GetCurrentGatewayAddress();
                string address = NativeTypes.FromNativeString(stringResult.get_String());
                GC.KeepAlive(stringResult);
                return address;
            }
            catch (COMException e)
            {
                throw Utility.TranslateCOMExceptionToManaged(e, "FabricTestClient.GetCurrentGatewayAddress");
            }
        }

        [ComImport]
        [Guid("498f4c96-c685-49a9-b80b-b5ac482be165")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        private interface IFabricTestClient
        {
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            void GetNthNamingPartitionId(
                [In] UInt32 n,
                [Out] out Guid partitionId);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginResolvePartition(
                [In] ref Guid partitionId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeClient.IFabricResolvedServicePartitionResult EndResolvePartition(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginResolveNameOwner(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeClient.IFabricResolvedServicePartitionResult EndResolveNameOwner(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            void NodeIdFromNameOwnerAddress(
                [In] IntPtr address,
                [Out] out WinFabricNodeId nodeId);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            void NodeIdFromFMAddress(
                [In] IntPtr address,
                [Out] out WinFabricNodeId nodeId);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricStringResult GetCurrentGatewayAddress();
        }
    }
}
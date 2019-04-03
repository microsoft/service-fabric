// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Interop
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;
    using BOOLEAN = System.SByte;
    using FABRIC_PARTITION_ID = System.Guid;
    using FABRIC_REPLICA_ID = System.Int64;

    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1201:ElementsMustAppearInTheCorrectOrder", Justification = "Matches order in IDL.")]
    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121:UseBuiltInTypeAlias", Justification = "It is important here to explicitly state the size of integer parameters.")]
    internal static class NativeGatewayResourceManager
    {
        //// ----------------------------------------------------------------------------
        //// Interfaces

        [ComImport]
        [Guid("bf9b93a9-5b74-4a28-b205-edf387adf3db")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricGatewayResourceManager
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginUpdateOrCreateGatewayResource(
                [In] IntPtr resourceDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
                
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult EndUpdateOrCreateGatewayResource(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginQueryGatewayResources(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
                
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringListResult EndQueryGatewayResources(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginDeleteGatewayResource(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
                
            void EndDeleteGatewayResource(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("029614d9-928a-489d-8eaf-c09f44028f46")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricGatewayResourceManagerAgent
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult RegisterGatewayResourceManager(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] FABRIC_REPLICA_ID replicaId,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricGatewayResourceManager service);

            void UnregisterGatewayResourceManager(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] FABRIC_REPLICA_ID replicaId);
        }

        //// ----------------------------------------------------------------------------
        //// Entry Points

#if DotNetCoreClrLinux
        [DllImport("libFabricGatewayResourceManager.so", PreserveSig = false)]
#else
        [DllImport("FabricGatewayResourceManager.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricGatewayResourceManagerAgent CreateFabricGatewayResourceManagerAgent(
            ref Guid iid);
    }
}
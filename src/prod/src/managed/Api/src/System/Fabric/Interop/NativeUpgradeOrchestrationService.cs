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
    internal static class NativeUpgradeOrchestrationService
    {
        //// ----------------------------------------------------------------------------
        //// Interfaces
        [ComImport]
        [Guid("41695A06-EC43-4F96-A785-A38872FC39BC")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricUpgradeOrchestrationService
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginUpgradeConfiguration(
                [In] IntPtr startUpgradeDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            void EndUpgradeConfiguration(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginGetClusterConfigurationUpgradeStatus(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeClient.IFabricOrchestrationUpgradeStatusResult EndGetClusterConfigurationUpgradeStatus(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginGetClusterConfiguration(
                [In] IntPtr apiVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricStringResult EndGetClusterConfiguration(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginGetUpgradesPendingApproval(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            void EndGetUpgradesPendingApproval(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginStartApprovedUpgrades(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            void EndStartApprovedUpgrades(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginCallSystemService(
                [In] IntPtr action,
                [In] IntPtr inputBlob,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricStringResult EndCallSystemService(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("F035ECFB-00F7-4316-9645-ED46D7E1D725")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricUpgradeOrchestrationServiceAgent
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricStringResult RegisterUpgradeOrchestrationService(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] FABRIC_REPLICA_ID replicaId,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricUpgradeOrchestrationService service);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            void UnregisterUpgradeOrchestrationService(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] FABRIC_REPLICA_ID replicaId);
        }

        //// ----------------------------------------------------------------------------
        //// Entry Points
#if DotNetCoreClrLinux
        [DllImport("libFabricUpgradeOrchestrationService.so", PreserveSig = false)]
#else
        [DllImport("FabricUpgradeOrchestrationService.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricUpgradeOrchestrationServiceAgent CreateFabricUpgradeOrchestrationServiceAgent(
            ref Guid iid);
    }
}
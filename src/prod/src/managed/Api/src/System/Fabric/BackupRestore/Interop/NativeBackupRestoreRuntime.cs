// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Interop
{
    using System;
    using System.Fabric.Interop;
    using System.Diagnostics.CodeAnalysis;
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;
    using FABRIC_PARTITION_ID = System.Guid;
    using FABRIC_REPLICA_ID = System.Int64;
    using FABRIC_BACKUP_OPERATION_ID = System.Guid;

    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1201:ElementsMustAppearInTheCorrectOrder", Justification = "Matches order in IDL.")]
    [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1310:FieldNamesMustNotContainUnderscore", Justification = "Matches native API.")]
    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121:UseBuiltInTypeAlias", Justification = "It is important here to explicitly state the size of integer parameters")]
    internal static class NativeBackupRestoreRuntime
    {
        //// ----------------------------------------------------------------------------
        //// Interfaces

        [ComImport]
        [Guid("5b3e9db0-c280-4a16-ab73-2c0005c1543f")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricGetBackupSchedulePolicyResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_BackupSchedulePolicy();
        }

        [ComImport]
        [Guid("64965ed4-b3d0-4574-adb8-35f084f0aa06")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricGetRestorePointDetailsResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_RestorePointDetails();
        }

        [ComImport]
        [Guid("1380040a-4cee-42af-92d5-25669ff45544")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricBackupRestoreAgent
        {
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            void RegisterBackupRestoreReplica(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] FABRIC_REPLICA_ID replicaId,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricBackupRestoreHandler backupRestoreHandler);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            void UnregisterBackupRestoreReplica(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] FABRIC_REPLICA_ID replicaId);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetBackupSchedulingPolicy(
                [In] IntPtr partitionInfo,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetBackupSchedulePolicyResult EndGetBackupSchedulingPolicy(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetRestorePointDetails(
                [In] IntPtr partitionInfo,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetRestorePointDetailsResult EndGetRestorePointDetails(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginReportBackupOperationResult(
                [In] IntPtr operationResult,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            void EndReportBackupOperationResult(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginReportRestoreOperationResult(
                [In] IntPtr operationResult,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            void EndReportRestoreOperationResult(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginUploadBackup(
                [In] IntPtr uploadInfo,
                [In] IntPtr backupStorageInfo,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            void EndUploadBackup(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginDownloadBackup(
                [In] IntPtr downloadInfo,
                [In] IntPtr backupStorageInfo,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            void EndDownlaodBackup(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("39aa5c2c-7c6c-455a-b9a1-0c08964507c0")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricBackupRestoreHandler
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginUpdateBackupSchedulePolicy(
                [In] IntPtr backupPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndUpdateBackupSchedulePolicy(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginPartitionBackupOperation(
                [In] FABRIC_BACKUP_OPERATION_ID operationId,
                [In] IntPtr backupConfiguration,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndPartitionBackupOperation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        //// ----------------------------------------------------------------------------
        //// Entry Points

#if DotNetCoreClrLinux
        [DllImport("libFabricRuntime.so", PreserveSig = false)]
#else
        [DllImport("FabricRuntime.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricBackupRestoreAgent FabricCreateBackupRestoreAgent(
            ref Guid riid);
    }
}
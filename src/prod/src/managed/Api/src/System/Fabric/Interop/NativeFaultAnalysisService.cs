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
    using TESTABILITY_OPERATION_ID = System.Guid;

    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1201:ElementsMustAppearInTheCorrectOrder", Justification = "Matches order in IDL.")]
    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121:UseBuiltInTypeAlias", Justification = "It is important here to explicitly state the size of integer parameters.")]
    internal static class NativeFaultAnalysisService
    {
        //// ----------------------------------------------------------------------------
        //// Interfaces

        [ComImport]
        [Guid("bf9b93a9-5b74-4a28-b205-edf387adf3db")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricFaultAnalysisService
        {
            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginStartPartitionDataLoss(
                [In] IntPtr invokeDataLossDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            void EndPartitionDataLoss(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginGetPartitionDataLossProgress(
                [In] TESTABILITY_OPERATION_ID operationId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeClient.IFabricPartitionDataLossProgressResult EndGetPartitionDataLossProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
            
             // InvokeQuorumLoss
            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginStartPartitionQuorumLoss(
                [In] IntPtr invokeQuorumLossDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            void EndPartitionQuorumLoss(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginGetPartitionQuorumLossProgress(
                [In] TESTABILITY_OPERATION_ID operationId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeClient.IFabricPartitionQuorumLossProgressResult EndGetPartitionQuorumLossProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // RestartPartition
            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginStartPartitionRestart(
                [In] IntPtr restartPartitionDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            void EndStartPartitionRestart(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginGetPartitionRestartProgress(
                [In] TESTABILITY_OPERATION_ID operationId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeClient.IFabricPartitionRestartProgressResult EndGetPartitionRestartProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginGetTestCommandStatusList(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeClient.IFabricTestCommandStatusResult EndGetTestCommandStatusList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginCancelTestCommand(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            void EndCancelTestCommand(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // Chaos
            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginStartChaos(
                [In] IntPtr startChaosDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            void EndStartChaos(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginStopChaos(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            void EndStopChaos(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginGetChaosReport(
                [In] IntPtr getChaosReportDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeClient.IFabricChaosReportResult EndGetChaosReport(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // GetStoppedNodeList
            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginGetStoppedNodeList(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricStringResult EndGetStoppedNodeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // StartNodeTransition
            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginStartNodeTransition(         
              [In] IntPtr description,
              [In] UInt32 timeoutMilliseconds,
              [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            void EndStartNodeTransition(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginGetNodeTransitionProgress(
                [In] TESTABILITY_OPERATION_ID operationId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeClient.IFabricNodeTransitionProgressResult EndGetNodeTransitionProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginCallSystemService(
                [In] IntPtr action,
                [In] IntPtr inputBlob,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricStringResult EndCallSystemService(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("a202ba9d-1478-42a8-ad03-4a1f15c7d255")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricFaultAnalysisServiceAgent
        {
            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricStringResult RegisterFaultAnalysisService(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] FABRIC_REPLICA_ID replicaId,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricFaultAnalysisService service);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            void UnregisterFaultAnalysisService(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] FABRIC_REPLICA_ID replicaId);
        }

        //// ----------------------------------------------------------------------------
        //// Entry Points

#if DotNetCoreClrLinux
        [DllImport("libFabricFaultAnalysisService.so", PreserveSig = false)]
#else
        [DllImport("FabricFaultAnalysisService.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricFaultAnalysisServiceAgent CreateFabricFaultAnalysisServiceAgent(
            ref Guid iid);
    }
}
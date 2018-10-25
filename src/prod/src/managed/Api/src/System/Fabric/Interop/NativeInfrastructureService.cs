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
    internal static class NativeInfrastructureService
    {
        //// ----------------------------------------------------------------------------
        //// Enumerations
        
        [Guid("dcb33c4a-134b-44f6-aaa2-f6358b955670")]
        internal enum FABRIC_NODE_TASK_TYPE : int
        {
            FABRIC_NODE_TASK_TYPE_INVALID     = 0x0000,
            FABRIC_NODE_TASK_TYPE_RESTART     = 0x0001,
            FABRIC_NODE_TASK_TYPE_RELOCATE    = 0x0002,
            FABRIC_NODE_TASK_TYPE_REMOVE      = 0x0003,
        }
                
        [Guid("23a82620-e134-40da-9872-1238762b72f1")]
        internal enum FABRIC_INFRASTRUCTURE_TASK_STATE : int
        {
            FABRIC_INFRASTRUCTURE_TASK_STATE_INVALID             = 0x0000,
            FABRIC_INFRASTRUCTURE_TASK_STATE_PRE_PROCESSING      = 0x0001,
            FABRIC_INFRASTRUCTURE_TASK_STATE_PRE_ACK_PENDING     = 0x0002,
            FABRIC_INFRASTRUCTURE_TASK_STATE_PRE_ACKED           = 0x0003,
            FABRIC_INFRASTRUCTURE_TASK_STATE_POST_PROCESSING     = 0x0004,
            FABRIC_INFRASTRUCTURE_TASK_STATE_POST_ACK_PENDING    = 0x0005,
            FABRIC_INFRASTRUCTURE_TASK_STATE_POST_ACKED          = 0x0006
        }

        //// ----------------------------------------------------------------------------
        //// Structures

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct FABRIC_NODE_TASK_DESCRIPTION
        {
            public IntPtr NodeName;
            public FABRIC_NODE_TASK_TYPE TaskType;
            public IntPtr Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct FABRIC_NODE_TASK_DESCRIPTION_LIST
        {
            public UInt32 Count;
            public IntPtr Items;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct FABRIC_INFRASTRUCTURE_TASK_DESCRIPTION
        {
            public FABRIC_PARTITION_ID SourcePartitionId;
            public IntPtr TaskId;
            public Int64 InstanceId;
            public IntPtr NodeTasks;
            public IntPtr Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct FABRIC_INFRASTRUCTURE_TASK_QUERY_RESULT_ITEM
        {
            public IntPtr Description;
            public FABRIC_INFRASTRUCTURE_TASK_STATE State;
            public IntPtr Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct FABRIC_INFRASTRUCTURE_TASK_QUERY_RESULT_LIST
        {
            public UInt32 Count;
            public IntPtr Items;
        }

        //// ----------------------------------------------------------------------------
        //// Interfaces

        [ComImport]
        [Guid("d948b384-fd16-48f3-a4ad-d6e68c6bf2bb")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricInfrastructureService
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginRunCommand(
                [In] BOOLEAN isAdminCommand,
                [In] IntPtr command,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            NativeCommon.IFabricStringResult EndRunCommand(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginReportStartTaskSuccess(
                [In] IntPtr taskId,
                [In] Int64 instanceId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            void EndReportStartTaskSuccess(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginReportFinishTaskSuccess(
                [In] IntPtr taskId,
                [In] Int64 instanceId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            void EndReportFinishTaskSuccess(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginReportTaskFailure(
                [In] IntPtr taskId,
                [In] Int64 instanceId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            void EndReportTaskFailure(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("2416a4e2-9313-42ce-93c9-b499764840ce")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricInfrastructureServiceAgent
        {
            void RegisterInfrastructureServiceFactory(
                [In] NativeRuntime.IFabricStatefulServiceFactory factory);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult RegisterInfrastructureService(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] FABRIC_REPLICA_ID replicaId,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricInfrastructureService service);

            void UnregisterInfrastructureService(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] FABRIC_REPLICA_ID replicaId);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginStartInfrastructureTask(
                [In] IntPtr taskDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            void EndStartInfrastructureTask(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginFinishInfrastructureTask(
                [In] IntPtr taskId,
                [In] Int64 instanceId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            void EndFinishInfrastructureTask(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginQueryInfrastructureTask(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricInfrastructureTaskQueryResult EndQueryInfrastructureTask(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("209e28bb-4f8b-4f03-88c9-7b54f2cd29f9")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricInfrastructureTaskQueryResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Result();
        }

        //// ----------------------------------------------------------------------------
        //// Entry Points

#if DotNetCoreClrLinux
        [DllImport("libFabricInfrastructureService.so", PreserveSig = false)]
#else        
        [DllImport("FabricInfrastructureService.dll", PreserveSig = false)]
#endif 
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricInfrastructureServiceAgent CreateFabricInfrastructureServiceAgent(
            ref Guid iid);
    }
}
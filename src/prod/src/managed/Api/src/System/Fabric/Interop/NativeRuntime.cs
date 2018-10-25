// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Interop
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;
    using BOOLEAN = System.SByte;
    using FABRIC_ATOMIC_GROUP_ID = System.Int64;
    using FABRIC_INSTANCE_ID = System.Int64;
    using FABRIC_PARTITION_ID = System.Guid;
    using FABRIC_REPLICA_ID = System.Int64;
    using FABRIC_SEQUENCE_NUMBER = System.Int64;
    using FABRIC_TRANSACTION_ID = System.Guid;
    using GUID = System.Guid;

    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1201:ElementsMustAppearInTheCorrectOrder",
        Justification = "Matches order in IDL.")]
    [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1310:FieldNamesMustNotContainUnderscore",
        Justification = "Matches native API.")]
    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121:UseBuiltInTypeAlias",
        Justification = "It is important here to explicitly state the size of integer parameters")]
    internal static class NativeRuntime
    {
        //// ----------------------------------------------------------------------------
        //// Interfaces

        [ComImport]
        [Guid("cc53af8e-74cd-11df-ac3e-0024811e3892")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricRuntime
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginRegisterStatelessServiceFactory(
                [In] IntPtr serviceType,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricStatelessServiceFactory factory,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndRegisterStatelessServiceFactory(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            void RegisterStatelessServiceFactory(
                [In] IntPtr serviceType,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricStatelessServiceFactory factory);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginRegisterStatefulServiceFactory(
                [In] IntPtr serviceType,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricStatefulServiceFactory factory,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndRegisterStatefulServiceFactory(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            void RegisterStatefulServiceFactory(
                [In] IntPtr serviceType,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricStatefulServiceFactory factory);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeRuntime.IFabricServiceGroupFactoryBuilder CreateServiceGroupFactoryBuilder();

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginRegisterServiceGroupFactory(
                [In] IntPtr groupServiceType,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricServiceGroupFactory factory,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndRegisterServiceGroupFactory(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            void RegisterServiceGroupFactory(
                [In] IntPtr groupServiceType,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricServiceGroupFactory factory);
        }

        [ComImport]
        [Guid("cc53af8f-74cd-11df-ac3e-0024811e3892")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricStatelessServiceFactory
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricStatelessServiceInstance CreateInstance(
                [In] IntPtr serviceType,
                [In] IntPtr serviceName,
                [In] UInt32 initializationDataLength,
                [In] IntPtr initializationData,
                [In] FABRIC_PARTITION_ID partitionId,
                [In] FABRIC_INSTANCE_ID instanceId);
        }

        [ComImport]
        [Guid("cc53af90-74cd-11df-ac3e-0024811e3892")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricStatelessServiceInstance
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginOpen(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricStatelessServicePartition partition,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult EndOpen(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [PreserveSig]
            void Abort();
        }

        [ComImport]
        [Guid("cc53af91-74cd-11df-ac3e-0024811e3892")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricStatelessServicePartition
        {
            IntPtr GetPartitionInfo();

            void ReportLoad(
                [In] UInt32 metricCount,
                [In] IntPtr metrics);

            void ReportFault([In] NativeTypes.FABRIC_FAULT_TYPE faultType);
        }

        [ComImport]
        [Guid("bf6bb505-7bd0-4371-b6c0-cba319a5e50b")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricStatelessServicePartition1 : IFabricStatelessServicePartition
        {
            //// ----------------------------------------------------------------
            //// IFabricStatelessServicePartition methods
            //// Base interface methods must come first to reserve vtable slots
            new IntPtr GetPartitionInfo();

            new void ReportLoad(
                [In] UInt32 metricCount,
                [In] IntPtr metrics);

            new void ReportFault([In] NativeTypes.FABRIC_FAULT_TYPE faultType);


            //// ----------------------------------------------------------------
            //// IFabricStatelessServicePartition1 methods
            void ReportMoveCost([In] NativeTypes.FABRIC_MOVE_COST moveCost);
        }

        [ComImport]
        [Guid("9ff35b6c-9d97-4312-93ad-7f34cbdb4ca4")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricStatelessServicePartition2 : IFabricStatelessServicePartition1
        {
            //// ----------------------------------------------------------------
            //// IFabricStatelessServicePartition methods
            //// Base interface methods must come first to reserve vtable slots
            new IntPtr GetPartitionInfo();

            new void ReportLoad(
                [In] UInt32 metricCount,
                [In] IntPtr metrics);

            new void ReportFault([In] NativeTypes.FABRIC_FAULT_TYPE faultType);


            //// ----------------------------------------------------------------
            //// IFabricStatelessServicePartition1 methods
            new void ReportMoveCost([In] NativeTypes.FABRIC_MOVE_COST moveCost);

            //// ----------------------------------------------------------------
            //// IFabricStatelessServicePartition2 methods
            void ReportInstanceHealth([In] IntPtr healthInfo);

            void ReportPartitionHealth([In] IntPtr healthInfo);
        }

        [ComImport]
        [Guid("f2fa2000-70a7-4ed5-9d3e-0b7deca2433f")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricStatelessServicePartition3 : IFabricStatelessServicePartition2
        {
            //// ----------------------------------------------------------------
            //// IFabricStatelessServicePartition methods
            //// Base interface methods must come first to reserve vtable slots
            new IntPtr GetPartitionInfo();

            new void ReportLoad(
                [In] UInt32 metricCount,
                [In] IntPtr metrics);

            new void ReportFault([In] NativeTypes.FABRIC_FAULT_TYPE faultType);


            //// ----------------------------------------------------------------
            //// IFabricStatelessServicePartition1 methods
            new void ReportMoveCost([In] NativeTypes.FABRIC_MOVE_COST moveCost);

            //// ----------------------------------------------------------------
            //// IFabricStatelessServicePartition2 methods
            new void ReportInstanceHealth([In] IntPtr healthInfo);

            new void ReportPartitionHealth([In] IntPtr healthInfo);

            //// ----------------------------------------------------------------
            //// IFabricStatelessServicePartition3 methods
            void ReportInstanceHealth2([In] IntPtr healthInfo, [In] IntPtr sendOptions);

            void ReportPartitionHealth2([In] IntPtr healthInfo, [In] IntPtr sendOptions);
        }

        [ComImport]
        [Guid("77ff0c6b-6780-48ec-b4b0-61989327b0f2")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricStatefulServiceFactory
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricStatefulServiceReplica CreateReplica(
                [In] IntPtr serviceType,
                [In] IntPtr serviceName,
                [In] UInt32 initializationDataLength,
                [In] IntPtr initializationData,
                [In] FABRIC_PARTITION_ID partitionId,
                [In] FABRIC_REPLICA_ID replicaId);
        }

        [ComImport]
        [Guid("8ae3be0e-505d-4dc1-ad8f-0cb0f9576b8a")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricStatefulServiceReplica
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginOpen(
                [In] NativeTypes.FABRIC_REPLICA_OPEN_MODE openMode,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricStatefulServicePartition partition,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricReplicator EndOpen(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginChangeRole(
                [In] NativeTypes.FABRIC_REPLICA_ROLE newRole,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult EndChangeRole(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [PreserveSig]
            void Abort();
        }

        [ComImport]
        [Guid("5beccc37-8655-4f20-bd43-f50691d7cd16")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricStatefulServicePartition
        {
            IntPtr GetPartitionInfo();

            NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetReadStatus();

            NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetWriteStatus();

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricStateReplicator CreateReplicator(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricStateProvider stateProvider,
                [In] IntPtr fabricReplicatorSettings,
                [Out, MarshalAs(UnmanagedType.Interface)] out IFabricReplicator replicator);

            void ReportLoad(
                [In] UInt32 metricCount,
                [In] IntPtr metrics);

            void ReportFault([In] NativeTypes.FABRIC_FAULT_TYPE faultType);
        }

        [ComImport]
        [Guid("c9c66f2f-9dff-4c87-bbe4-a08b4c4074cf")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricStatefulServicePartition1 : IFabricStatefulServicePartition
        {
            //// ----------------------------------------------------------------
            //// IFabricStatelessServicePartition methods
            //// Base interface methods must come first to reserve vtable slots
            new IntPtr GetPartitionInfo();

            new NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetReadStatus();

            new NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetWriteStatus();

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricStateReplicator CreateReplicator(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricStateProvider stateProvider,
                [In] IntPtr fabricReplicatorSettings,
                [Out, MarshalAs(UnmanagedType.Interface)] out IFabricReplicator replicator);

            new void ReportLoad(
                [In] UInt32 metricCount,
                [In] IntPtr metrics);

            new void ReportFault([In] NativeTypes.FABRIC_FAULT_TYPE faultType);

            //// ----------------------------------------------------------------
            //// IFabricStatelessServicePartition1 methods
            void ReportMoveCost([In] NativeTypes.FABRIC_MOVE_COST moveCost);
        }

        [ComImport]
        [Guid("df27b476-fa25-459f-a7d3-87d3eec9c73c")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricStatefulServicePartition2 : IFabricStatefulServicePartition1
        {
            //// ----------------------------------------------------------------
            //// IFabricStatelessServicePartition methods
            //// Base interface methods must come first to reserve vtable slots
            new IntPtr GetPartitionInfo();

            new NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetReadStatus();

            new NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetWriteStatus();

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricStateReplicator CreateReplicator(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricStateProvider stateProvider,
                [In] IntPtr fabricReplicatorSettings,
                [Out, MarshalAs(UnmanagedType.Interface)] out IFabricReplicator replicator);

            new void ReportLoad(
                [In] UInt32 metricCount,
                [In] IntPtr metrics);

            new void ReportFault([In] NativeTypes.FABRIC_FAULT_TYPE faultType);

            //// ----------------------------------------------------------------
            //// IFabricStatefulServicePartition1 methods
            new void ReportMoveCost([In] NativeTypes.FABRIC_MOVE_COST moveCost);

            //// ----------------------------------------------------------------
            //// IFabricStatefulServicePartition2 methods
            void ReportReplicaHealth([In] IntPtr healthInfo);

            void ReportPartitionHealth([In] IntPtr healthInfo);
        }

        [ComImport]
        [Guid("51f1269d-b061-4c1c-96cf-6508cece813b")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricStatefulServicePartition3 : IFabricStatefulServicePartition2
        {
            //// ----------------------------------------------------------------
            //// IFabricStatelessServicePartition methods
            //// Base interface methods must come first to reserve vtable slots
            new IntPtr GetPartitionInfo();

            new NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetReadStatus();

            new NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetWriteStatus();

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricStateReplicator CreateReplicator(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricStateProvider stateProvider,
                [In] IntPtr fabricReplicatorSettings,
                [Out, MarshalAs(UnmanagedType.Interface)] out IFabricReplicator replicator);

            new void ReportLoad(
                [In] UInt32 metricCount,
                [In] IntPtr metrics);

            new void ReportFault([In] NativeTypes.FABRIC_FAULT_TYPE faultType);

            //// ----------------------------------------------------------------
            //// IFabricStatefulServicePartition1 methods
            new void ReportMoveCost([In] NativeTypes.FABRIC_MOVE_COST moveCost);

            //// ----------------------------------------------------------------
            //// IFabricStatefulServicePartition2 methods
            new void ReportReplicaHealth([In] IntPtr healthInfo);

            new void ReportPartitionHealth([In] IntPtr healthInfo);

            //// ----------------------------------------------------------------
            //// IFabricStatefulServicePartition3 methods
            void ReportReplicaHealth2([In] IntPtr healthInfo, [In] IntPtr sendOptions);

            void ReportPartitionHealth2([In] IntPtr healthInfo, [In] IntPtr sendOptions);
        }

        [ComImport]
        [Guid("89e9a978-c771-44f2-92e8-3bf271cabe9c")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricStateReplicator
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginReplicate(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricOperationData operationData,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback,
                [Out] out FABRIC_SEQUENCE_NUMBER sequenceNumber);

            FABRIC_SEQUENCE_NUMBER EndReplicate(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricOperationStream GetReplicationStream();

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricOperationStream GetCopyStream();

            void UpdateReplicatorSettings([In] IntPtr fabricReplicatorSettings);
        }

        [ComImport]
        [Guid("4A28D542-658F-46F9-9BF4-79B7CAE25C5D")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricStateReplicator2 : IFabricStateReplicator
        {
            //// ----------------------------------------------------------------
            //// IFabricStateReplicator methods
            //// Base interface methods must come first to reserve vtable slots
            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginReplicate(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricOperationData operationData,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback,
                [Out] out FABRIC_SEQUENCE_NUMBER sequenceNumber);

            new FABRIC_SEQUENCE_NUMBER EndReplicate(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricOperationStream GetReplicationStream();

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricOperationStream GetCopyStream();

            new void UpdateReplicatorSettings([In] IntPtr fabricReplicatorSettings);

            //// ----------------------------------------------------------------
            //// IFabricStateReplicator2 methods
            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricReplicatorSettingsResult GetReplicatorSettings();
        }

        [ComImport]
        [Guid("3ebfec79-bd27-43f3-8be8-da38ee723951")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricStateProvider
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginUpdateEpoch(
                [In] IntPtr epoch,
                [In] FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndUpdateEpoch(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            FABRIC_SEQUENCE_NUMBER GetLastCommittedSequenceNumber();

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginOnDataLoss(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            BOOLEAN EndOnDataLoss(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricOperationDataStream GetCopyContext();

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricOperationDataStream GetCopyState(
                [In] FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricOperationDataStream copyContextStream);
        }

        [ComImport]
        [Guid("f4ad6bfa-e23c-4a48-9617-c099cd59a23a")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricOperation
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
                Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Metadata();

            IntPtr GetData(
                [Out] out UInt32 length);

            void Acknowledge();
        }

        [ComImport]
        [Guid("bab8ad87-37b7-482a-985d-baf38a785dcd")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricOperationData
        {
            IntPtr GetData(
                [Out] out UInt32 length);
        }

        [ComImport]
        [Guid("A98FB97A-D6B0-408A-A878-A9EDB09C2587")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricOperationStream
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetOperation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricOperation EndGetOperation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("0930199B-590A-4065-BEC9-5F93B6AAE086")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricOperationStream2 : IFabricOperationStream
        {
            //// ----------------------------------------------------------------
            //// IFabricOperationStream methods
            //// Base interface methods must come first to reserve vtable slots
            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetOperation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricOperation EndGetOperation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            //// ----------------------------------------------------------------
            //// IFabricOperationStream2 methods
            void ReportFault([In] NativeTypes.FABRIC_FAULT_TYPE faultType);
        }

        [ComImport]
        [Guid("c4e9084c-be92-49c9-8c18-d44d088c2e32")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricOperationDataStream
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetNext(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricOperationData EndGetNext(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("067f144a-e5be-4f5e-a181-8b5593e20242")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricReplicator
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginOpen(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult EndOpen(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginChangeRole(
                [In] IntPtr epoch,
                [In] NativeTypes.FABRIC_REPLICA_ROLE role,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndChangeRole(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            NativeCommon.IFabricAsyncOperationContext BeginUpdateEpoch(
                [In] IntPtr epoch,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndUpdateEpoch(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [PreserveSig]
            void Abort();

            void GetCurrentProgress(
                [Out] out FABRIC_SEQUENCE_NUMBER lastSequenceNumber);

            void GetCatchUpCapability(
                [Out] out FABRIC_SEQUENCE_NUMBER fromSequenceNumber);
        }

        [ComImport]
        [Guid("564e50dd-c3a4-4600-a60e-6658874307ae")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricPrimaryReplicator : IFabricReplicator
        {
            //// ----------------------------------------------------------------
            //// IFabricReplicator methods
            //// Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginOpen(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult EndOpen(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginChangeRole(
                [In] IntPtr epoch,
                [In] NativeTypes.FABRIC_REPLICA_ROLE role,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndChangeRole(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpdateEpoch(
                [In] IntPtr epoch,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpdateEpoch(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            new void Abort();

            new void GetCurrentProgress(
                [Out] out FABRIC_SEQUENCE_NUMBER lastSequenceNumber);

            new void GetCatchUpCapability(
                [Out] out FABRIC_SEQUENCE_NUMBER fromSequenceNumber);

            //// ----------------------------------------------------------------
            //// IFabricPrimaryReplicator methods

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginOnDataLoss(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            BOOLEAN EndOnDataLoss(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            void UpdateCatchUpReplicaSetConfiguration(
                [In] IntPtr currentConfiguration,
                [In] IntPtr previousConfiguration);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginWaitForCatchUpQuorum(
                [In] NativeTypes.FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndWaitForCatchUpQuorum(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            void UpdateCurrentReplicaSetConfiguration(
                [In] IntPtr currentConfiguration);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginBuildReplica(
                [In] IntPtr replica,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndBuildReplica(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            void RemoveReplica(
                [In] FABRIC_REPLICA_ID replicaId);
        }


        [ComImport]
        [Guid("aa3116fe-277d-482d-bd16-5366fa405757")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricReplicatorCatchupSpecificQuorum
        {
        }

        // This interface is a private interface to enable managed code to create a native
        // IFabricOperationData.  This ensures that the data guarded remains valid if the AppDomain
        // unloads.
        [ComImport]
        [Guid("01af2d2f-e878-4db2-89ec-5f6afec2c56f")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IOperationDataFactory
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricOperationData CreateOperationData(
                [In] IntPtr segmentSizes,
                [In] UInt32 segmentSizesCount);
        }

        // This interface is a private interface to enable managed code to invoke BeginReplicate without passing in com object
        // and to bypass the above operation data factory
        [ComImport]
        [Guid("ee723951-c3a1-44f2-92e8-f50691d7cd16")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricInternalManagedReplicator
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginReplicate2(
                [In] Int32 bufferCount,
                [In] IntPtr buffers,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback,
                [Out] out FABRIC_SEQUENCE_NUMBER sequenceNumber);
        }

        [ComImport]
        [Guid("908dcbb4-bb4e-4b2e-ae0a-46dac42184bc")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricStatefulServiceReplicaStatusResult
        {
            IntPtr get_Result();
        }

        [ComImport]
        [Guid("80d2155c-4fc2-4fde-9696-c2f39b471c3d")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricAtomicGroupStateReplicator
        {
            FABRIC_ATOMIC_GROUP_ID CreateAtomicGroup();

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginReplicateAtomicGroupOperation(
                [In] FABRIC_ATOMIC_GROUP_ID atomicGroupId,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricOperationData operationData,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback,
                [Out] out FABRIC_SEQUENCE_NUMBER operationSequenceNumber);

            FABRIC_SEQUENCE_NUMBER EndReplicateAtomicGroupOperation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginReplicateAtomicGroupCommit(
                [In] FABRIC_ATOMIC_GROUP_ID atomicGroupId,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback,
                [Out] out FABRIC_SEQUENCE_NUMBER commitSequenceNumber);

            FABRIC_SEQUENCE_NUMBER EndReplicateAtomicGroupCommit(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginReplicateAtomicGroupRollback(
                [In] FABRIC_ATOMIC_GROUP_ID atomicGroupId,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback,
                [Out] out FABRIC_SEQUENCE_NUMBER rollbackSequenceNumber);

            FABRIC_SEQUENCE_NUMBER EndReplicateAtomicGroupRollback(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("2b670953-6148-4f7d-a920-b390de43d913")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricAtomicGroupStateProvider
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginAtomicGroupCommit(
                [In] FABRIC_ATOMIC_GROUP_ID atomicGroupId,
                [In] FABRIC_SEQUENCE_NUMBER commitSequenceNumber,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndAtomicGroupCommit(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginAtomicGroupRollback(
                [In] FABRIC_ATOMIC_GROUP_ID atomicGroupId,
                [In] FABRIC_SEQUENCE_NUMBER rollbackSequenceNumber,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndAtomicGroupRollback(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginUndoProgress(
                [In] FABRIC_SEQUENCE_NUMBER fromCommitSequenceNumber,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndUndoProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("3860d61d-1e51-4a65-b109-d93c11311657")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricServiceGroupFactory
        {
        }

        [ComImport]
        [Guid("a9fe8b06-19b1-49e6-8911-41d9d9219e1c")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricServiceGroupFactoryBuilder
        {
            void AddStatelessServiceFactory(
                [In] IntPtr memberServiceType,
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricStatelessServiceFactory factory);

            void AddStatefulServiceFactory(
                [In] IntPtr memberServiceType,
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricStatefulServiceFactory factory);

            void RemoveServiceFactory(
                [In] IntPtr memberServiceType);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeRuntime.IFabricServiceGroupFactory ToServiceGroupFactory();
        }

        [ComImport]
        [Guid("2b24299a-7489-467f-8e7f-4507bff73b86")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricServiceGroupPartition
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            object ResolveMember(
                [In] IntPtr name,
                [In] ref Guid riid);
        }

        [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
            Justification = "Matches native API.")]
        [ComImport]
        [Guid("68a971e2-f15f-4d95-a79c-8a257909659e")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricCodePackageActivationContext
        {
            [PreserveSig]
            IntPtr get_ContextId();

            [PreserveSig]
            IntPtr get_CodePackageName();

            [PreserveSig]
            IntPtr get_CodePackageVersion();

            [PreserveSig]
            IntPtr get_WorkDirectory();

            [PreserveSig]
            IntPtr get_LogDirectory();

            [PreserveSig]
            IntPtr get_TempDirectory();

            [PreserveSig]
            IntPtr get_ServiceTypes();

            [PreserveSig]
            IntPtr get_ServiceGroupTypes();

            [PreserveSig]
            IntPtr get_ApplicationPrincipals();

            [PreserveSig]
            IntPtr get_ServiceEndpointResources();

            IntPtr GetServiceEndpointResource(IntPtr serviceEndpointResourceName);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringListResult GetCodePackageNames();

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringListResult GetConfigurationPackageNames();

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringListResult GetDataPackageNames();

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricCodePackage GetCodePackage(
                [In] IntPtr codePackageName);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricConfigurationPackage GetConfigurationPackage(
                [In] IntPtr configurationPackageName);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricDataPackage GetDataPackage(
                [In] IntPtr dataPackageName);

            Int64 RegisterCodePackageChangeHandler(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricCodePackageChangeHandler callback);

            void UnregisterCodePackageChangeHandler(
                [In] Int64 callbackHandle);

            Int64 RegisterConfigurationPackageChangeHandler(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricConfigurationPackageChangeHandler callback);

            void UnregisterConfigurationPackageChangeHandler(
                [In] Int64 callbackHandle);

            Int64 RegisterDataPackageChangeHandler(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricDataPackageChangeHandler callback);

            void UnregisterDataPackageChangeHandler(
                [In] Int64 callbackHandle);
        }


        [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
            Justification = "Matches native API.")]
        [ComImport]
        [Guid("6c83d5c1-1954-4b80-9175-0d0e7c8715c9")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricCodePackageActivationContext2 : IFabricCodePackageActivationContext
        {
            // IFabricCodePackageActivationContext2 methods 
            // base interface methods comes first to reserve vtable slots
            [PreserveSig]
            new IntPtr get_ContextId();

            [PreserveSig]
            new IntPtr get_CodePackageName();

            [PreserveSig]
            new IntPtr get_CodePackageVersion();

            [PreserveSig]
            new IntPtr get_WorkDirectory();

            [PreserveSig]
            new IntPtr get_LogDirectory();

            [PreserveSig]
            new IntPtr get_TempDirectory();

            [PreserveSig]
            new IntPtr get_ServiceTypes();

            [PreserveSig]
            new IntPtr get_ServiceGroupTypes();

            [PreserveSig]
            new IntPtr get_ApplicationPrincipals();

            [PreserveSig]
            new IntPtr get_ServiceEndpointResources();

            new IntPtr GetServiceEndpointResource(IntPtr serviceEndpointResourceName);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringListResult GetCodePackageNames();

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringListResult GetConfigurationPackageNames();

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringListResult GetDataPackageNames();

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricCodePackage GetCodePackage(
                [In] IntPtr codePackageName);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricConfigurationPackage GetConfigurationPackage(
                [In] IntPtr configurationPackageName);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricDataPackage GetDataPackage(
                [In] IntPtr dataPackageName);

            new Int64 RegisterCodePackageChangeHandler(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricCodePackageChangeHandler callback);

            new void UnregisterCodePackageChangeHandler(
                [In] Int64 callbackHandle);

            new Int64 RegisterConfigurationPackageChangeHandler(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricConfigurationPackageChangeHandler callback);

            new void UnregisterConfigurationPackageChangeHandler(
                [In] Int64 callbackHandle);

            new Int64 RegisterDataPackageChangeHandler(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricDataPackageChangeHandler callback);

            new void UnregisterDataPackageChangeHandler(
                [In] Int64 callbackHandle);

            // IFabricCodePackageActivationContext2 methods
            [PreserveSig]
            IntPtr get_ApplicationName();

            [PreserveSig]
            IntPtr get_ApplicationTypeName();

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetServiceManifestName();

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetServiceManifestVersion();
        }

        [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
            Justification = "Matches native API.")]
        [ComImport]
        [Guid("6efee900-f491-4b03-bc5b-3a70de103593")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricCodePackageActivationContext3 : IFabricCodePackageActivationContext2
        {
            // IFabricCodePackageActivationContext methods 
            // base interface methods comes first to reserve vtable slots
            [PreserveSig]
            new IntPtr get_ContextId();

            [PreserveSig]
            new IntPtr get_CodePackageName();

            [PreserveSig]
            new IntPtr get_CodePackageVersion();

            [PreserveSig]
            new IntPtr get_WorkDirectory();

            [PreserveSig]
            new IntPtr get_LogDirectory();

            [PreserveSig]
            new IntPtr get_TempDirectory();

            [PreserveSig]
            new IntPtr get_ServiceTypes();

            [PreserveSig]
            new IntPtr get_ServiceGroupTypes();

            [PreserveSig]
            new IntPtr get_ApplicationPrincipals();

            [PreserveSig]
            new IntPtr get_ServiceEndpointResources();

            new IntPtr GetServiceEndpointResource(IntPtr serviceEndpointResourceName);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringListResult GetCodePackageNames();

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringListResult GetConfigurationPackageNames();

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringListResult GetDataPackageNames();

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricCodePackage GetCodePackage(
                [In] IntPtr codePackageName);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricConfigurationPackage GetConfigurationPackage(
                [In] IntPtr configurationPackageName);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricDataPackage GetDataPackage(
                [In] IntPtr dataPackageName);

            new Int64 RegisterCodePackageChangeHandler(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricCodePackageChangeHandler callback);

            new void UnregisterCodePackageChangeHandler(
                [In] Int64 callbackHandle);

            new Int64 RegisterConfigurationPackageChangeHandler(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricConfigurationPackageChangeHandler callback);

            new void UnregisterConfigurationPackageChangeHandler(
                [In] Int64 callbackHandle);

            new Int64 RegisterDataPackageChangeHandler(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricDataPackageChangeHandler callback);

            new void UnregisterDataPackageChangeHandler(
                [In] Int64 callbackHandle);

            // IFabricCodePackageActivationContext2 methods
            [PreserveSig]
            new IntPtr get_ApplicationName();

            [PreserveSig]
            new IntPtr get_ApplicationTypeName();

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetServiceManifestName();

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetServiceManifestVersion();

            // IFabricCodePackageActivationContext3 methods
            void ReportApplicationHealth([In] IntPtr healthInfo);

            void ReportDeployedApplicationHealth([In] IntPtr healthInfo);

            void ReportDeployedServicePackageHealth([In] IntPtr healthInfo);
        }

        [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
            Justification = "Matches native API.")]
        [ComImport]
        [Guid("99efebb6-a7b4-4d45-b45e-f191a66eef03")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricCodePackageActivationContext4 : IFabricCodePackageActivationContext3
        {
            // IFabricCodePackageActivationContext methods 
            // base interface methods comes first to reserve vtable slots
            [PreserveSig]
            new IntPtr get_ContextId();

            [PreserveSig]
            new IntPtr get_CodePackageName();

            [PreserveSig]
            new IntPtr get_CodePackageVersion();

            [PreserveSig]
            new IntPtr get_WorkDirectory();

            [PreserveSig]
            new IntPtr get_LogDirectory();

            [PreserveSig]
            new IntPtr get_TempDirectory();

            [PreserveSig]
            new IntPtr get_ServiceTypes();

            [PreserveSig]
            new IntPtr get_ServiceGroupTypes();

            [PreserveSig]
            new IntPtr get_ApplicationPrincipals();

            [PreserveSig]
            new IntPtr get_ServiceEndpointResources();

            new IntPtr GetServiceEndpointResource(IntPtr serviceEndpointResourceName);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringListResult GetCodePackageNames();

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringListResult GetConfigurationPackageNames();

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringListResult GetDataPackageNames();

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricCodePackage GetCodePackage(
                [In] IntPtr codePackageName);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricConfigurationPackage GetConfigurationPackage(
                [In] IntPtr configurationPackageName);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricDataPackage GetDataPackage(
                [In] IntPtr dataPackageName);

            new Int64 RegisterCodePackageChangeHandler(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricCodePackageChangeHandler callback);

            new void UnregisterCodePackageChangeHandler(
                [In] Int64 callbackHandle);

            new Int64 RegisterConfigurationPackageChangeHandler(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricConfigurationPackageChangeHandler callback);

            new void UnregisterConfigurationPackageChangeHandler(
                [In] Int64 callbackHandle);

            new Int64 RegisterDataPackageChangeHandler(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricDataPackageChangeHandler callback);

            new void UnregisterDataPackageChangeHandler(
                [In] Int64 callbackHandle);

            // IFabricCodePackageActivationContext2 methods
            [PreserveSig]
            new IntPtr get_ApplicationName();

            [PreserveSig]
            new IntPtr get_ApplicationTypeName();

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetServiceManifestName();

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetServiceManifestVersion();

            // IFabricCodePackageActivationContext3 methods
            new void ReportApplicationHealth([In] IntPtr healthInfo);

            new void ReportDeployedApplicationHealth([In] IntPtr healthInfo);

            new void ReportDeployedServicePackageHealth([In] IntPtr healthInfo);

            // IFabricCodePackageActivationContext3 methods
            void ReportApplicationHealth2([In] IntPtr healthInfo, [In] IntPtr sendOptions);

            void ReportDeployedApplicationHealth2([In] IntPtr healthInfo, [In] IntPtr sendOptions);

            void ReportDeployedServicePackageHealth2([In] IntPtr healthInfo, [In] IntPtr sendOptions);
        }

        [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
            Justification = "Matches native API.")]
        [ComImport]
        [Guid("fe45387e-8711-4949-ac36-31dc95035513")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricCodePackageActivationContext5 : IFabricCodePackageActivationContext4
        {
            // IFabricCodePackageActivationContext methods 
            // base interface methods comes first to reserve vtable slots
            [PreserveSig]
            new IntPtr get_ContextId();

            [PreserveSig]
            new IntPtr get_CodePackageName();

            [PreserveSig]
            new IntPtr get_CodePackageVersion();

            [PreserveSig]
            new IntPtr get_WorkDirectory();

            [PreserveSig]
            new IntPtr get_LogDirectory();

            [PreserveSig]
            new IntPtr get_TempDirectory();

            [PreserveSig]
            new IntPtr get_ServiceTypes();

            [PreserveSig]
            new IntPtr get_ServiceGroupTypes();

            [PreserveSig]
            new IntPtr get_ApplicationPrincipals();

            [PreserveSig]
            new IntPtr get_ServiceEndpointResources();

            new IntPtr GetServiceEndpointResource(IntPtr serviceEndpointResourceName);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringListResult GetCodePackageNames();

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringListResult GetConfigurationPackageNames();

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringListResult GetDataPackageNames();

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricCodePackage GetCodePackage(
                [In] IntPtr codePackageName);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricConfigurationPackage GetConfigurationPackage(
                [In] IntPtr configurationPackageName);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricDataPackage GetDataPackage(
                [In] IntPtr dataPackageName);

            new Int64 RegisterCodePackageChangeHandler(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricCodePackageChangeHandler callback);

            new void UnregisterCodePackageChangeHandler(
                [In] Int64 callbackHandle);

            new Int64 RegisterConfigurationPackageChangeHandler(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricConfigurationPackageChangeHandler callback);

            new void UnregisterConfigurationPackageChangeHandler(
                [In] Int64 callbackHandle);

            new Int64 RegisterDataPackageChangeHandler(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricDataPackageChangeHandler callback);

            new void UnregisterDataPackageChangeHandler(
                [In] Int64 callbackHandle);

            // IFabricCodePackageActivationContext2 methods
            [PreserveSig]
            new IntPtr get_ApplicationName();

            [PreserveSig]
            new IntPtr get_ApplicationTypeName();

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetServiceManifestName();

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetServiceManifestVersion();

            // IFabricCodePackageActivationContext3 methods
            new void ReportApplicationHealth([In] IntPtr healthInfo);

            new void ReportDeployedApplicationHealth([In] IntPtr healthInfo);

            new void ReportDeployedServicePackageHealth([In] IntPtr healthInfo);

            // IFabricCodePackageActivationContext4 methods
            new void ReportApplicationHealth2([In] IntPtr healthInfo, [In] IntPtr sendOptions);

            new void ReportDeployedApplicationHealth2([In] IntPtr healthInfo, [In] IntPtr sendOptions);

            new void ReportDeployedServicePackageHealth2([In] IntPtr healthInfo, [In] IntPtr sendOptions);

            // IFabricCodePackageActivationContext5 methods

            [PreserveSig]
            IntPtr get_ServiceListenAddress();

            [PreserveSig]
            IntPtr get_ServicePublishAddress();
        }

        [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
        [ComImport]
        [Guid("fa5fda9b-472c-45a0-9b60-a374691227a4")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricCodePackageActivationContext6 : IFabricCodePackageActivationContext5
        {
            // IFabricCodePackageActivationContext methods
            // base interface methods comes first to reserve vtable slots
            [PreserveSig]
            new IntPtr get_ContextId();

            [PreserveSig]
            new IntPtr get_CodePackageName();

            [PreserveSig]
            new IntPtr get_CodePackageVersion();

            [PreserveSig]
            new IntPtr get_WorkDirectory();

            [PreserveSig]
            new IntPtr get_LogDirectory();

            [PreserveSig]
            new IntPtr get_TempDirectory();

            [PreserveSig]
            new IntPtr get_ServiceTypes();

            [PreserveSig]
            new IntPtr get_ServiceGroupTypes();

            [PreserveSig]
            new IntPtr get_ApplicationPrincipals();

            [PreserveSig]
            new IntPtr get_ServiceEndpointResources();

            new IntPtr GetServiceEndpointResource(IntPtr serviceEndpointResourceName);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringListResult GetCodePackageNames();

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringListResult GetConfigurationPackageNames();

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringListResult GetDataPackageNames();

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricCodePackage GetCodePackage(
                [In] IntPtr codePackageName);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricConfigurationPackage GetConfigurationPackage(
                [In] IntPtr configurationPackageName);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricDataPackage GetDataPackage(
                [In] IntPtr dataPackageName);

            new Int64 RegisterCodePackageChangeHandler(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricCodePackageChangeHandler callback);

            new void UnregisterCodePackageChangeHandler(
                [In] Int64 callbackHandle);

            new Int64 RegisterConfigurationPackageChangeHandler(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricConfigurationPackageChangeHandler callback);

            new void UnregisterConfigurationPackageChangeHandler(
                [In] Int64 callbackHandle);

            new Int64 RegisterDataPackageChangeHandler(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricDataPackageChangeHandler callback);

            new void UnregisterDataPackageChangeHandler(
                [In] Int64 callbackHandle);

            // IFabricCodePackageActivationContext2 methods
            [PreserveSig]
            new IntPtr get_ApplicationName();

            [PreserveSig]
            new IntPtr get_ApplicationTypeName();

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetServiceManifestName();

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult GetServiceManifestVersion();

            // IFabricCodePackageActivationContext3 methods
            new void ReportApplicationHealth([In] IntPtr healthInfo);

            new void ReportDeployedApplicationHealth([In] IntPtr healthInfo);

            new void ReportDeployedServicePackageHealth([In] IntPtr healthInfo);

            // IFabricCodePackageActivationContext4 methods
            new void ReportApplicationHealth2([In] IntPtr healthInfo, [In] IntPtr sendOptions);

            new void ReportDeployedApplicationHealth2([In] IntPtr healthInfo, [In] IntPtr sendOptions);

            new void ReportDeployedServicePackageHealth2([In] IntPtr healthInfo, [In] IntPtr sendOptions);

            // IFabricCodePackageActivationContext5 methods

            [PreserveSig]
            new IntPtr get_ServiceListenAddress();

            [PreserveSig]
            new IntPtr get_ServicePublishAddress();

            // IFabricCodePackageActivationContext6 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetDirectory([In] IntPtr logicalDirectoryName);
        }

        [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
            Justification = "Matches native API.")]
        [ComImport]
        [Guid("20792b45-4d13-41a4-af13-346e529f00c5")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricCodePackage
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
                Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Description();

            [PreserveSig]
            IntPtr get_Path();
        }

        [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
            Justification = "Matches native API.")]
        [ComImport]
        [Guid("cdf0a4e6-ad80-4cd6-b67e-e4c002428600")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricCodePackage2 : IFabricCodePackage
        {
            // IFabricCodePackage methods 
            // base interface methods comes first to reserve vtable slots
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
                Justification = "Matches native API.")]
            [PreserveSig]
            new IntPtr get_Description();

            [PreserveSig]
            new IntPtr get_Path();

            // IFabricCodePackage2 methods 
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
                Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_SetupEntryPointRunAsPolicy();

            [PreserveSig]
            IntPtr get_EntryPointRunAsPolicy();
        }

        [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
            Justification = "Matches native API.")]
        [ComImport]
        [Guid("ac4c3bfa-2563-46b7-a71d-2dca7b0a8f4d")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricConfigurationPackage
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
                Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Description();

            [PreserveSig]
            IntPtr get_Path();

            [PreserveSig]
            IntPtr get_Settings();

            IntPtr GetSection(IntPtr sectionName);

            IntPtr GetValue(IntPtr sectionName, IntPtr parameterName);
        }

        [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
            Justification = "Matches native API.")]
        [ComImport]
        [Guid("d3161f31-708a-4f83-91ff-f2af15f74a2f")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricConfigurationPackage2 : IFabricConfigurationPackage
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
                Justification = "Matches native API.")]
            [PreserveSig]
            new IntPtr get_Description();

            [PreserveSig]
            new IntPtr get_Path();

            [PreserveSig]
            new IntPtr get_Settings();

            new IntPtr GetSection(IntPtr sectionName);

            new IntPtr GetValue(IntPtr sectionName, IntPtr parameterName);

            IntPtr GetValues(IntPtr sectionName, IntPtr parameterPrefix);
        }

        [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
            Justification = "Matches native API.")]
        [ComImport]
        [Guid("aa67de09-3657-435f-a2f6-b3a17a0a4371")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricDataPackage
        {
            [PreserveSig]
            IntPtr get_Description();

            [PreserveSig]
            IntPtr get_Path();
        }

        [ComImport]
        [Guid("725AD67D-CCB3-4178-8ECE-C8F6856F3BA0")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricServiceManifestDescriptionResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
                Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Description();
        }

        [ComImport]
        [Guid("b90d36cd-acb5-427a-b318-3b045981d0cc")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricCodePackageChangeHandler
        {
            [PreserveSig]
            void OnPackageAdded(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricCodePackageActivationContext source,
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricCodePackage newPackage);

            [PreserveSig]
            void OnPackageRemoved(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricCodePackageActivationContext source,
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricCodePackage oldPackage);

            [PreserveSig]
            void OnPackageModified(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricCodePackageActivationContext source,
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricCodePackage oldPackage,
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricCodePackage newPackage);
        }

        [ComImport]
        [Guid("c3954d48-b5ee-4ff4-9bc0-c30f6d0d3a85")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricConfigurationPackageChangeHandler
        {
            [PreserveSig]
            void OnPackageAdded(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricCodePackageActivationContext source,
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricConfigurationPackage newPackage);

            [PreserveSig]
            void OnPackageRemoved(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricCodePackageActivationContext source,
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricConfigurationPackage oldPackage);

            [PreserveSig]
            void OnPackageModified(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricCodePackageActivationContext source,
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricConfigurationPackage oldPackage,
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricConfigurationPackage newPackage);
        }

        [ComImport]
        [Guid("8d0a726f-bd17-4b32-807b-be2a8024b2e0")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricDataPackageChangeHandler
        {
            [PreserveSig]
            void OnPackageAdded(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricCodePackageActivationContext source,
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricDataPackage newPackage);

            [PreserveSig]
            void OnPackageRemoved(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricCodePackageActivationContext source,
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricDataPackage oldPackage);

            [PreserveSig]
            void OnPackageModified(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricCodePackageActivationContext source,
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricDataPackage oldPackage,
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricDataPackage newPackage);
        }

        [ComImport]
        [Guid("c58d50a2-01f0-4267-bbe7-223b565c1346")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricProcessExitHandler
        {
            [PreserveSig]
            void FabricProcessExited();
        }

        [ComImport]
        [Guid("32d656a1-7ad5-47b8-bd66-a2e302626b7e")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricTransactionBase
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
                Justification = "Matches native API.")]
            [PreserveSig]
#if !DotNetCoreClrLinux
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            [return: MarshalAs(UnmanagedType.LPStruct)]
#endif
            FABRIC_TRANSACTION_ID get_Id();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
                Justification = "Matches native API.")]
            [PreserveSig]
            NativeTypes.FABRIC_TRANSACTION_ISOLATION_LEVEL get_IsolationLevel();
        }

        [ComImport]
        [Guid("19ee48b4-6d4d-470b-ac1e-2d3996a173c8")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricTransaction : IFabricTransactionBase
        {
            //// IFabrictransactionBase methods

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
                Justification = "Matches native API.")]
            [PreserveSig]
#if !DotNetCoreClrLinux
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            [return: MarshalAs(UnmanagedType.LPStruct)]
#endif
            new FABRIC_TRANSACTION_ID get_Id();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
                Justification = "Matches native API.")]
            [PreserveSig]
            new NativeTypes.FABRIC_TRANSACTION_ISOLATION_LEVEL get_IsolationLevel();

            //// IFabrictransaction methods

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginCommit(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndCommit(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context,
                [Out] out FABRIC_SEQUENCE_NUMBER commitSequenceNumber);

            [PreserveSig]
            void Rollback();
        }

        [ComImport]
        [Guid("97da35c4-38ed-4a2a-8f37-fbeb56382235")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricKeyValueStoreReplica : IFabricStatefulServiceReplica
        {
            //// ----------------------------------------------------------------
            //// IFabricStatefulServiceReplica methods
            //// Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginOpen(
                [In] NativeTypes.FABRIC_REPLICA_OPEN_MODE openMode,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricStatefulServicePartition partition,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricReplicator EndOpen(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginChangeRole(
                [In] NativeTypes.FABRIC_REPLICA_ROLE newRole,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult EndChangeRole(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            new void Abort();

            //// ----------------------------------------------------------------
            //// IFabricKeyValueStoreReplica methods

            void GetCurrentEpoch(
                [Out] out NativeTypes.FABRIC_EPOCH currentEpoch);

            void UpdateReplicatorSettings(
                [In] IntPtr fabricReplicatorSettings);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricTransaction CreateTransaction();

            void Add(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key,
                [In] Int32 valueSizeInBytes,
                [In] IntPtr value);

            void Remove(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key,
                [In] FABRIC_SEQUENCE_NUMBER checkSequenceNumber);

            void Update(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key,
                [In] Int32 valueSizeInBytes,
                [In] IntPtr value,
                [In] FABRIC_SEQUENCE_NUMBER checkSequenceNumber);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricKeyValueStoreItemResult Get(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricKeyValueStoreItemMetadataResult GetMetadata(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key);

            BOOLEAN Contains(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricKeyValueStoreItemEnumerator Enumerate(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricKeyValueStoreItemEnumerator EnumerateByKey(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr keyPrefix);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricKeyValueStoreItemMetadataEnumerator EnumerateMetadata(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricKeyValueStoreItemMetadataEnumerator EnumerateMetadataByKey(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr keyPrefix);
        }

        [ComImport]
        [Guid("fef805b2-5aca-4caa-9c51-fb3bd577a792")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricKeyValueStoreReplica2 : IFabricKeyValueStoreReplica
        {
            //// ----------------------------------------------------------------
            //// IFabricStatefulServiceReplica methods
            //// Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginOpen(
                [In] NativeTypes.FABRIC_REPLICA_OPEN_MODE openMode,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricStatefulServicePartition partition,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricReplicator EndOpen(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginChangeRole(
                [In] NativeTypes.FABRIC_REPLICA_ROLE newRole,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult EndChangeRole(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            new void Abort();

            //// ----------------------------------------------------------------
            //// IFabricKeyValueStoreReplica methods

            new void GetCurrentEpoch(
                [Out] out NativeTypes.FABRIC_EPOCH currentEpoch);

            new void UpdateReplicatorSettings(
                [In] IntPtr fabricReplicatorSettings);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricTransaction CreateTransaction();

            new void Add(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key,
                [In] Int32 valueSizeInBytes,
                [In] IntPtr value);

            new void Remove(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key,
                [In] FABRIC_SEQUENCE_NUMBER checkSequenceNumber);

            new void Update(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key,
                [In] Int32 valueSizeInBytes,
                [In] IntPtr value,
                [In] FABRIC_SEQUENCE_NUMBER checkSequenceNumber);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemResult Get(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemMetadataResult GetMetadata(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key);

            new BOOLEAN Contains(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemEnumerator Enumerate(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemEnumerator EnumerateByKey(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr keyPrefix);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemMetadataEnumerator EnumerateMetadata(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemMetadataEnumerator EnumerateMetadataByKey(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr keyPrefix);

            //// ----------------------------------------------------------------
            //// IFabricStatefulServiceReplica2 methods

            void Backup(
                [In] IntPtr backupDirectory);

            void Restore(
                [In] IntPtr backupDirectory);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricTransaction CreateTransaction2(IntPtr txSettings);
        }

        [ComImport]
        [Guid("c1297172-a8aa-4096-bdcc-1ece0c5d8c8f")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricKeyValueStoreReplica3 : IFabricKeyValueStoreReplica2
        {
            //// ----------------------------------------------------------------
            //// IFabricStatefulServiceReplica methods
            //// Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginOpen(
                [In] NativeTypes.FABRIC_REPLICA_OPEN_MODE openMode,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricStatefulServicePartition partition,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricReplicator EndOpen(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginChangeRole(
                [In] NativeTypes.FABRIC_REPLICA_ROLE newRole,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult EndChangeRole(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            new void Abort();

            new void GetCurrentEpoch(
                [Out] out NativeTypes.FABRIC_EPOCH currentEpoch);

            new void UpdateReplicatorSettings(
                [In] IntPtr fabricReplicatorSettings);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricTransaction CreateTransaction();

            new void Add(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key,
                [In] Int32 valueSizeInBytes,
                [In] IntPtr value);

            new void Remove(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key,
                [In] FABRIC_SEQUENCE_NUMBER checkSequenceNumber);

            new void Update(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key,
                [In] Int32 valueSizeInBytes,
                [In] IntPtr value,
                [In] FABRIC_SEQUENCE_NUMBER checkSequenceNumber);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemResult Get(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemMetadataResult GetMetadata(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key);

            new BOOLEAN Contains(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemEnumerator Enumerate(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemEnumerator EnumerateByKey(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr keyPrefix);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemMetadataEnumerator EnumerateMetadata(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemMetadataEnumerator EnumerateMetadataByKey(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr keyPrefix);

            new void Backup(
                [In] IntPtr backupDirectory);

            new void Restore(
                [In] IntPtr backupDirectory);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricTransaction CreateTransaction2(IntPtr txSettings);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginBackup(
                [In] IntPtr backupDirectory,
                [In] NativeTypes.FABRIC_STORE_BACKUP_OPTION backupOption,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricStorePostBackupHandler postBackupHandler,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndBackup(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        /// <summary>
        /// IFabricKeyValueStoreReplica4 interface.
        /// </summary>
        /// <remarks>Ensure that the GUID matches the one in FabricRuntime.idl</remarks>
        [ComImport]
        [Guid("ff16d2f1-41a9-4c64-804a-a20bf28c04f3")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricKeyValueStoreReplica4 : IFabricKeyValueStoreReplica3
        {
            //// ----------------------------------------------------------------
            //// IFabricStatefulServiceReplica methods
            //// Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginOpen(
                [In] NativeTypes.FABRIC_REPLICA_OPEN_MODE openMode,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricStatefulServicePartition partition,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricReplicator EndOpen(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginChangeRole(
                [In] NativeTypes.FABRIC_REPLICA_ROLE newRole,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult EndChangeRole(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            new void Abort();

            new void GetCurrentEpoch(
                [Out] out NativeTypes.FABRIC_EPOCH currentEpoch);

            new void UpdateReplicatorSettings(
                [In] IntPtr fabricReplicatorSettings);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricTransaction CreateTransaction();

            new void Add(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key,
                [In] Int32 valueSizeInBytes,
                [In] IntPtr value);

            new void Remove(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key,
                [In] FABRIC_SEQUENCE_NUMBER checkSequenceNumber);

            new void Update(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key,
                [In] Int32 valueSizeInBytes,
                [In] IntPtr value,
                [In] FABRIC_SEQUENCE_NUMBER checkSequenceNumber);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemResult Get(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemMetadataResult GetMetadata(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key);

            new BOOLEAN Contains(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemEnumerator Enumerate(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemEnumerator EnumerateByKey(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr keyPrefix);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemMetadataEnumerator EnumerateMetadata(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemMetadataEnumerator EnumerateMetadataByKey(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr keyPrefix);

            new void Backup(
                [In] IntPtr backupDirectory);

            new void Restore(
                [In] IntPtr backupDirectory);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricTransaction CreateTransaction2(IntPtr txSettings);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginBackup(
                [In] IntPtr backupDirectory,
                [In] NativeTypes.FABRIC_STORE_BACKUP_OPTION backupOption,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricStorePostBackupHandler postBackupHandler,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndBackup(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginRestore(
                [In] IntPtr backupDirectory,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndRestore(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        /// <summary>
        /// IFabricKeyValueStoreReplica5 interface.
        /// </summary>
        /// <remarks>Ensure that the GUID matches the one in FabricRuntime.idl</remarks>
        [ComImport]
        [Guid("34f2da40-6227-448a-be72-c517b0d69432")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricKeyValueStoreReplica5 : IFabricKeyValueStoreReplica4
        {
            //// ----------------------------------------------------------------
            //// IFabricStatefulServiceReplica methods
            //// Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginOpen(
                [In] NativeTypes.FABRIC_REPLICA_OPEN_MODE openMode,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricStatefulServicePartition partition,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricReplicator EndOpen(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginChangeRole(
                [In] NativeTypes.FABRIC_REPLICA_ROLE newRole,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult EndChangeRole(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            new void Abort();

            new void GetCurrentEpoch(
                [Out] out NativeTypes.FABRIC_EPOCH currentEpoch);

            new void UpdateReplicatorSettings(
                [In] IntPtr fabricReplicatorSettings);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricTransaction CreateTransaction();

            new void Add(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key,
                [In] Int32 valueSizeInBytes,
                [In] IntPtr value);

            new void Remove(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key,
                [In] FABRIC_SEQUENCE_NUMBER checkSequenceNumber);

            new void Update(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key,
                [In] Int32 valueSizeInBytes,
                [In] IntPtr value,
                [In] FABRIC_SEQUENCE_NUMBER checkSequenceNumber);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemResult Get(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemMetadataResult GetMetadata(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key);

            new BOOLEAN Contains(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemEnumerator Enumerate(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemEnumerator EnumerateByKey(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr keyPrefix);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemMetadataEnumerator EnumerateMetadata(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemMetadataEnumerator EnumerateMetadataByKey(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr keyPrefix);

            new void Backup(
                [In] IntPtr backupDirectory);

            new void Restore(
                [In] IntPtr backupDirectory);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricTransaction CreateTransaction2(IntPtr txSettings);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginBackup(
                [In] IntPtr backupDirectory,
                [In] NativeTypes.FABRIC_STORE_BACKUP_OPTION backupOption,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricStorePostBackupHandler postBackupHandler,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndBackup(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRestore(
                [In] IntPtr backupDirectory,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRestore(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricKeyValueStoreReplica5

            BOOLEAN TryAdd(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key,
                [In] Int32 valueSizeInBytes,
                [In] IntPtr value);

            BOOLEAN TryRemove(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key,
                [In] FABRIC_SEQUENCE_NUMBER checkSequenceNumber);

            BOOLEAN TryUpdate(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key,
                [In] Int32 valueSizeInBytes,
                [In] IntPtr value,
                [In] FABRIC_SEQUENCE_NUMBER checkSequenceNumber);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricKeyValueStoreItemResult TryGet(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricKeyValueStoreItemMetadataResult TryGetMetadata(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricKeyValueStoreItemEnumerator EnumerateByKey2(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr keyPrefix,
                [In] BOOLEAN strictPrefix);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricKeyValueStoreItemMetadataEnumerator EnumerateMetadataByKey2(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr keyPrefix,
                [In] BOOLEAN strictPrefix);
        }

        /// <summary>
        /// IFabricKeyValueStoreReplica6 interface.
        /// </summary>
        /// <remarks>Ensure that the GUID matches the one in FabricRuntime.idl</remarks>
        [ComImport]
        [Guid("56e77be1-e81f-4e42-8522-162c2d608184")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricKeyValueStoreReplica6 : IFabricKeyValueStoreReplica5
        {
            //// ----------------------------------------------------------------
            //// IFabricStatefulServiceReplica methods
            //// Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginOpen(
                [In] NativeTypes.FABRIC_REPLICA_OPEN_MODE openMode,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricStatefulServicePartition partition,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricReplicator EndOpen(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginChangeRole(
                [In] NativeTypes.FABRIC_REPLICA_ROLE newRole,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricStringResult EndChangeRole(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            new void Abort();

            new void GetCurrentEpoch(
                [Out] out NativeTypes.FABRIC_EPOCH currentEpoch);

            new void UpdateReplicatorSettings(
                [In] IntPtr fabricReplicatorSettings);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricTransaction CreateTransaction();

            new void Add(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key,
                [In] Int32 valueSizeInBytes,
                [In] IntPtr value);

            new void Remove(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key,
                [In] FABRIC_SEQUENCE_NUMBER checkSequenceNumber);

            new void Update(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key,
                [In] Int32 valueSizeInBytes,
                [In] IntPtr value,
                [In] FABRIC_SEQUENCE_NUMBER checkSequenceNumber);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemResult Get(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemMetadataResult GetMetadata(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key);

            new BOOLEAN Contains(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemEnumerator Enumerate(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemEnumerator EnumerateByKey(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr keyPrefix);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemMetadataEnumerator EnumerateMetadata(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemMetadataEnumerator EnumerateMetadataByKey(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr keyPrefix);

            new void Backup(
                [In] IntPtr backupDirectory);

            new void Restore(
                [In] IntPtr backupDirectory);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricTransaction CreateTransaction2(IntPtr txSettings);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginBackup(
                [In] IntPtr backupDirectory,
                [In] NativeTypes.FABRIC_STORE_BACKUP_OPTION backupOption,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricStorePostBackupHandler postBackupHandler,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndBackup(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRestore(
                [In] IntPtr backupDirectory,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRestore(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            new BOOLEAN TryAdd(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key,
                [In] Int32 valueSizeInBytes,
                [In] IntPtr value);

            new BOOLEAN TryRemove(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key,
                [In] FABRIC_SEQUENCE_NUMBER checkSequenceNumber);

            new BOOLEAN TryUpdate(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key,
                [In] Int32 valueSizeInBytes,
                [In] IntPtr value,
                [In] FABRIC_SEQUENCE_NUMBER checkSequenceNumber);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemResult TryGet(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemMetadataResult TryGetMetadata(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr key);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemEnumerator EnumerateByKey2(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr keyPrefix,
                [In] BOOLEAN strictPrefix);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemMetadataEnumerator EnumerateMetadataByKey2(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransactionBase transaction,
                [In] IntPtr keyPrefix,
                [In] BOOLEAN strictPrefix);

            // IFabricKeyValueStoreReplica6

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginRestore2(
                [In] IntPtr backupDirectory,
                [In] IntPtr restoreSettings,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
        }

        [ComImport]
        [Guid("2af2e8a6-41df-4e32-9d2a-d73a711e652a")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricStorePostBackupHandler
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginPostBackup(
                [In] NativeTypes.FABRIC_STORE_BACKUP_INFO backupInfo,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            BOOLEAN EndPostBackup(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("6722b848-15bb-4528-bf54-c7bbe27b6f9a")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricKeyValueStoreEnumerator
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricKeyValueStoreItemEnumerator EnumerateByKey(
                [In] IntPtr keyPrefix);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricKeyValueStoreItemMetadataEnumerator EnumerateMetadataByKey(
                [In] IntPtr keyPrefix);
        }

        [ComImport]
        [Guid("63dfd264-4f2b-4be6-8234-1fa200165fe9")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricKeyValueStoreEnumerator2 : IFabricKeyValueStoreEnumerator
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemEnumerator EnumerateByKey(
                [In] IntPtr keyPrefix);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemMetadataEnumerator EnumerateMetadataByKey(
                [In] IntPtr keyPrefix);

            // IFabricKeyValueStoreEnumerator2

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricKeyValueStoreItemEnumerator EnumerateByKey2(
                [In] IntPtr keyPrefix,
                [In] BOOLEAN strictPrefix);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricKeyValueStoreItemMetadataEnumerator EnumerateMetadataByKey2(
                [In] IntPtr keyPrefix,
                [In] BOOLEAN strictPrefix);
        }

        [ComImport]
        [Guid("ef25bc08-be76-43c7-adad-20f01fba3399")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricKeyValueStoreNotificationEnumerator
        {
            void MoveNext();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
                Justification = "Matches native API.")]
            [PreserveSig]
            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricKeyValueStoreNotification get_Current();

            [PreserveSig]
            void Reset();

        }

        [ComImport]
        [Guid("55eec7c6-ae81-407a-b84c-22771d314ac7")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricKeyValueStoreNotificationEnumerator2 : IFabricKeyValueStoreNotificationEnumerator
        {
            new void MoveNext();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
                Justification = "Matches native API.")]
            [PreserveSig]
            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreNotification get_Current();

            [PreserveSig]
            new void Reset();

            // IFabricKeyValueStoreNotificationEnumerator2

            BOOLEAN TryMoveNext();
        }

        [ComImport]
        [Guid("c1f1c89d-b0b8-44dc-bc97-6c074c1a805e")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricKeyValueStoreItemResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
                Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Item();
        }

        [ComImport]
        [Guid("17c483a1-69e6-4bdc-a058-54fd4a1839fd")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricKeyValueStoreItemMetadataResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
                Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Metadata();
        }

        [ComImport]
        [Guid("cb660aa6-c51e-4f05-9526-93982b550e8f")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricKeyValueStoreNotification : IFabricKeyValueStoreItemResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
                Justification = "Matches native API.")]
            [PreserveSig]
            new IntPtr get_Item();

            [PreserveSig]
            BOOLEAN IsDelete();
        }

        [ComImport]
        [Guid("c202788f-54d3-44a6-8f3c-b4bbfcdb95d2")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricKeyValueStoreItemEnumerator
        {
            void MoveNext();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
                Justification = "Matches native API.")]
            [PreserveSig]
            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricKeyValueStoreItemResult get_Current();
        }

        [ComImport]
        [Guid("da143bbc-81e1-48cd-afd7-b642bc5b9bfd")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricKeyValueStoreItemEnumerator2 : IFabricKeyValueStoreItemEnumerator
        {
            new void MoveNext();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
                Justification = "Matches native API.")]
            [PreserveSig]
            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemResult get_Current();

            // IFabricKeyValueStoreItemEnumerator2

            BOOLEAN TryMoveNext();
        }

        [ComImport]
        [Guid("0bc06aee-fffa-4450-9099-116a5f0e0b53")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricKeyValueStoreItemMetadataEnumerator
        {
            void MoveNext();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
                Justification = "Matches native API.")]
            [PreserveSig]
            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricKeyValueStoreItemMetadataResult get_Current();
        }

        [ComImport]
        [Guid("8803d53e-dd73-40fc-a662-1bfe999419ea")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricKeyValueStoreItemMetadataEnumerator2 : IFabricKeyValueStoreItemMetadataEnumerator
        {
            new void MoveNext();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
                Justification = "Matches native API.")]
            [PreserveSig]
            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricKeyValueStoreItemMetadataResult get_Current();

            // IFabricKeyValueStoreItemMetadataEnumerator2

            BOOLEAN TryMoveNext();
        }

        [ComImport]
        [Guid("220e6da4-985b-4dee-8fe9-77521b838795")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricStoreEventHandler
        {
            [PreserveSig]
            void OnDataLoss();
        }

        [ComImport]
        [Guid("CCE4523F-614B-4D6A-98A3-1E197C0213EA")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricStoreEventHandler2 : IFabricStoreEventHandler
        {
            // IFabricStoreEventHandler
            [PreserveSig]
            new void OnDataLoss();

            // IFabricStoreEventHandler2
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginOnDataLoss(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            BOOLEAN EndOnDataLoss(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("7d124a7d-258e-49f2-a9b0-e800406103fb")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricSecondaryEventHandler
        {
            void OnCopyComplete(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricKeyValueStoreEnumerator enumerator);

            void OnReplicationOperation(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricKeyValueStoreNotificationEnumerator enumerator);
        }

        [ComImport]
        [Guid("31027dc1-b9eb-4992-8793-67367bfc2d1a")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricCodePackageHost
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginActivate(
                [In] IntPtr codePackageId,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricCodePackageActivationContext activationContext,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricRuntime runtime,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndActivate(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginDeactivate(
                [In] IntPtr codePackageId,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndDeactivate(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("0952f885-6f5a-4ed3-abe4-90c403d1e3ce")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricNodeContextResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
                Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_NodeContext();
        }

        [ComImport]
        [Guid("472bf2e1-d617-4b5c-a91d-fabed9ff3550")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricNodeContextResult2 : IFabricNodeContextResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
                Justification = "Matches native API.")]
            [PreserveSig]
            new IntPtr get_NodeContext();

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringResult GetDirectory([In] IntPtr logicalDirectoryName);
        }

        [ComImport]
        [Guid("718954F3-DC1E-4060-9806-0CBF36F71051")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricReplicatorSettingsResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
                Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ReplicatorSettings();
        }

        [ComImport]
        [Guid("049A111D-6A30-48E9-8F69-470760D3EFB9")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricSecurityCredentialsResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
                Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_SecurityCredentials();
        }

        [ComImport]
        [Guid("AACE77AE-D8E1-4144-B1EE-5AC74FD54F65")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricEseLocalStoreSettingsResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
                Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Settings();
        }

        //// ----------------------------------------------------------------------------
        //// Entry Points

#if DotNetCoreClrLinux
        [DllImport("libFabricRuntime.so", PreserveSig = false)]
#else
        [DllImport("FabricRuntime.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern NativeCommon.IFabricAsyncOperationContext FabricBeginCreateRuntime(
            ref Guid riid,
            [In, MarshalAs(UnmanagedType.Interface)] IFabricProcessExitHandler exitHandler,
            [In] UInt32 timeoutMilliseconds,
            [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

#if DotNetCoreClrLinux
        [DllImport("libFabricRuntime.so", PreserveSig = false)]
#else
        [DllImport("FabricRuntime.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricRuntime FabricEndCreateRuntime(
            [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

#if DotNetCoreClrLinux
        [DllImport("libFabricRuntime.so", PreserveSig = false)]
#else
        [DllImport("FabricRuntime.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricRuntime FabricCreateRuntime(
            ref Guid riid);

#if DotNetCoreClrLinux
        [DllImport("libFabricRuntime.so", PreserveSig = false)]
#else
        [DllImport("FabricRuntime.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern NativeCommon.IFabricAsyncOperationContext FabricBeginGetActivationContext(
            ref Guid riid,
            [In] UInt32 timeoutMilliseconds,
            [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

#if DotNetCoreClrLinux
        [DllImport("libFabricRuntime.so", PreserveSig = false)]
#else
        [DllImport("FabricRuntime.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricCodePackageActivationContext6 FabricEndGetActivationContext(
            [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

#if DotNetCoreClrLinux
        [DllImport("libFabricRuntime.so", PreserveSig = false)]
#else
        [DllImport("FabricRuntime.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricCodePackageActivationContext6 FabricGetActivationContext(
            ref Guid riid);

#if DotNetCoreClrLinux
        [DllImport("libFabricRuntime.so", PreserveSig = false)]
#else
        [DllImport("FabricRuntime.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern NativeCommon.IFabricAsyncOperationContext FabricBeginGetNodeContext(
            [In] UInt32 timeoutMilliseconds,
            [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

#if DotNetCoreClrLinux
        [DllImport("libFabricRuntime.so", PreserveSig = false)]
#else
        [DllImport("FabricRuntime.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricNodeContextResult2 FabricEndGetNodeContext(
            [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

#if DotNetCoreClrLinux
        [DllImport("libFabricRuntime.so", PreserveSig = false)]
#else
        [DllImport("FabricRuntime.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricNodeContextResult2 FabricGetNodeContext();

#if DotNetCoreClrLinux
        [DllImport("libFabricRuntime.so", PreserveSig = false)]
#else
        [DllImport("FabricRuntime.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricReplicatorSettingsResult FabricLoadReplicatorSettings(
            [In, MarshalAs(UnmanagedType.Interface)] IFabricCodePackageActivationContext codePackageActivationContext,
            [In] IntPtr configurationPackageName,
            [In] IntPtr sectionName);

#if DotNetCoreClrLinux
        [DllImport("libFabricRuntime.so", PreserveSig = false)]
#else
        [DllImport("FabricRuntime.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricEseLocalStoreSettingsResult FabricLoadEseLocalStoreSettings(
            [In, MarshalAs(UnmanagedType.Interface)] IFabricCodePackageActivationContext codePackageActivationContext,
            [In] IntPtr configurationPackageName,
            [In] IntPtr sectionName);

#if DotNetCoreClrLinux
        [DllImport("libFabricRuntime.so", PreserveSig = false)]
#else
        [DllImport("FabricRuntime.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricSecurityCredentialsResult FabricLoadSecurityCredentials(
            [In, MarshalAs(UnmanagedType.Interface)] IFabricCodePackageActivationContext codePackageActivationContext,
            [In] IntPtr configurationPackageName,
            [In] IntPtr sectionName);

#if DotNetCoreClrLinux
        [DllImport("libFabricRuntime.so", PreserveSig = false)]
#else
        [DllImport("FabricRuntime.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricKeyValueStoreReplica4 FabricCreateKeyValueStoreReplica4(
            ref Guid riid,
            [In] IntPtr storeName,
            [In] FABRIC_PARTITION_ID partitionId,
            [In] FABRIC_REPLICA_ID replicaId,
            [In] IntPtr serviceName,
            [In] IntPtr fabricReplicatorSettings,
            [In] NativeTypes.FABRIC_LOCAL_STORE_KIND localStoreKind,
            [In] IntPtr localStorageSettings,
            [In, MarshalAs(UnmanagedType.Interface)] IFabricStoreEventHandler storeEventHandler,
            [In, MarshalAs(UnmanagedType.Interface)] IFabricSecondaryEventHandler secondaryEventHandler,
            [In] NativeTypes.FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE notificationMode);

#if DotNetCoreClrLinux
        [DllImport("libFabricRuntime.so", PreserveSig = false)]
#else
        [DllImport("FabricRuntime.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricKeyValueStoreReplica6 FabricCreateKeyValueStoreReplica5(
            ref Guid riid,
            [In] IntPtr storeName,
            [In] FABRIC_PARTITION_ID partitionId,
            [In] FABRIC_REPLICA_ID replicaId,
            [In] IntPtr serviceName,
            [In] IntPtr fabricReplicatorSettings,
            [In] IntPtr kvsSettings,
            [In] NativeTypes.FABRIC_LOCAL_STORE_KIND localStoreKind,
            [In] IntPtr localStorageSettings,
            [In, MarshalAs(UnmanagedType.Interface)] IFabricStoreEventHandler storeEventHandler,
            [In, MarshalAs(UnmanagedType.Interface)] IFabricSecondaryEventHandler secondaryEventHandler);

#if DotNetCoreClrLinux
        [DllImport("libFabricRuntime.so", PreserveSig = false)]
#else
        [DllImport("FabricRuntime.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricKeyValueStoreReplica6 FabricCreateKeyValueStoreReplica_V2(
            ref Guid riid,
            [In] FABRIC_PARTITION_ID partitionId,
            [In] FABRIC_REPLICA_ID replicaId,
            [In] IntPtr storeSettings,
            [In] IntPtr replicatorSettings,
            [In, MarshalAs(UnmanagedType.Interface)] IFabricStoreEventHandler storeEventHandler,
            [In, MarshalAs(UnmanagedType.Interface)] IFabricSecondaryEventHandler secondaryEventHandler);
    }

    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "TODO: move to seperate file")]
    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121:UseBuiltInTypeAlias", Justification = "It is important here to explicitly state the size of integer parameters")]
    internal static class NativeRuntimeInternal
    {
        [ComImport]
        [Guid("ed70f43b-8574-487c-b5ff-fcfcf6a6c6ea")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricInternalBrokeredService
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            object GetBrokeredService();
        }

        [ComImport]
        [Guid("0FE01003-90F4-456D-BD36-6B479B473978")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricDisposable
        {
            [PreserveSig]
            void Dispose();
        }
        
        [ComImport]
        [Guid("AE4F9D4A-406E-4FFE-A35D-0EF82A9D53CE")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricKeyValueStoreReplicaSettingsResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Settings();
        }

        [ComImport]
        [Guid("63BE1B43-1519-43F3-A1F5-5C2F3010009A")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricKeyValueStoreReplicaSettings_V2Result
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Settings();
        }

        [ComImport]
        [Guid("FDCEAB77-F1A2-4E87-8B41-4849C5EBB348")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricSharedLogSettingsResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Settings();
        }
                
        [ComImport]
        [Guid("e5395a45-c9fc-43f7-a5c7-4c34d6b56791")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricInternalStatefulServiceReplica
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeRuntime.IFabricStatefulServiceReplicaStatusResult GetStatus();
        }

        [ComImport]
        [Guid("d37ce92b-db5a-430d-8d4d-b40a9331f263")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricStateProviderSupportsCopyUntilLatestLsn
        {
        }

        //// ----------------------------------------------------------------------------
        //// Entry Points

#if DotNetCoreClrLinux
        [DllImport("libFabricRuntime.so", PreserveSig = false)]
#else
        [DllImport("FabricRuntime.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricKeyValueStoreReplicaSettingsResult GetFabricKeyValueStoreReplicaDefaultSettings();

#if DotNetCoreClrLinux
        [DllImport("libFabricRuntime.so", PreserveSig = false)]
#else
        [DllImport("FabricRuntime.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricKeyValueStoreReplicaSettings_V2Result GetFabricKeyValueStoreReplicaDefaultSettings_V2(
            [In] IntPtr workingDirectory,
            [In] IntPtr sharedLogDirectory,
            [In] IntPtr sharedLogFileName,
            [In] GUID sharedLogGuid);

#if DotNetCoreClrLinux
        [DllImport("libFabricRuntime.so", PreserveSig = false)]
#else
        [DllImport("FabricRuntime.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricSharedLogSettingsResult GetFabricSharedLogDefaultSettings(
            [In] IntPtr workingDirectory,
            [In] IntPtr sharedLogDirectory,
            [In] IntPtr sharedLogFileName,
            [In] GUID sharedLogGuid);

#if DotNetCoreClrLinux
        [DllImport("libFabricRuntime.so", PreserveSig = false)]
#else
        [DllImport("FabricRuntime.dll", PreserveSig = false)]
#endif        
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern NativeRuntime.IFabricSecurityCredentialsResult FabricLoadClusterSecurityCredentials();

#if DotNetCoreClrLinux
        [DllImport("libFabricRuntime.so", PreserveSig = false)]
#else
        [DllImport("FabricRuntime.dll", PreserveSig = false)]
#endif        
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern NativeCommon.IFabricStringResult FabricGetRuntimeDllVersion();
    }
}
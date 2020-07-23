// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ReliableMessaging.Interop
{
    using System.Fabric.Interop;
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;
    using BOOLEAN = System.SByte;
    using FABRIC_RELIABLE_SESSION_ID = System.Guid;
    using FABRIC_SEQUENCE_NUMBER = System.Int64;
    using FABRIC_PARTITION_ID = System.Guid;
    using FABRIC_REPLICA_ID = System.Int64;

    internal static class NativeReliableMessaging
    {
        [Guid("b4b9295d-b65c-4777-ab54-6199877cb2fc")]
        internal enum FABRIC_INBOUND_RELIABLE_SESSION_RESPONSE : int
        {
            FABRIC_INBOUND_RELIABLE_SESSION_ACCEPTED,
            FABRIC_INBOUND_RELIABLE_SESSION_REJECTED,
            FABRIC_INBOUND_RELIABLE_SESSION_RESUMED
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct SESSION_DATA_SNAPSHOT
        {
            public int InboundMessageQuota;
            public int InboundMessageCount;
            public int SendOperationQuota;
            public int SendOperationCount;
            public int ReceiveOperationQuota;
            public int ReceiveOperationCount;
            public BOOLEAN IsOutbound;
            public FABRIC_RELIABLE_SESSION_ID SessionId;
            public BOOLEAN NormalCloseOccurred;
            public BOOLEAN IsOpenForSend;
            public BOOLEAN IsOpenForReceive;
            public FABRIC_SEQUENCE_NUMBER LastOutboundSequenceNumber;
            public FABRIC_SEQUENCE_NUMBER FinalInboundSequenceNumberInSession;
            private IntPtr Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct HOST_SERVICE_PARTITION_INFO
        {
            public FABRIC_PARTITION_ID PartitionId;
            public FABRIC_REPLICA_ID ReplicaId;
            private IntPtr Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct INTEGER_PARTITION_KEY_RANGE
        {
            public long IntegerKeyLow;
            public long IntegerKeyHigh;
        }

        [ComImport]
        [Guid("b2cc5f0e-2bdb-433d-9f73-c7e096037770")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricMessageDataFactory
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeRuntime.IFabricOperationData CreateMessageData(
                [In] UInt32 buffercount,
                [In] IntPtr bufferSizes,
                [In] IntPtr bufferPointers);
        }

        [ComImport]
        [Guid("df1e6d90-3fab-4c36-ae22-7213223f46ae")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricReliableSession
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginOpen(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            void EndOpen(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            void EndClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            void Abort();

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginSend(
                [In, MarshalAs(UnmanagedType.Interface)] NativeRuntime.IFabricOperationData message,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            void EndSend(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginReceive(
                [In] BOOLEAN waitForMessages,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeRuntime.IFabricOperationData EndReceive(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            IntPtr GetSessionDataSnapshot();
        }


        [ComImport]
        [Guid("6f6110e0-3eec-4d65-a761-218d7f479515")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricReliableSessionPartitionIdentity
        {
            // response is true if accepted or false if rejected
            [PreserveSig]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            IntPtr getServiceName();

            [PreserveSig]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeTypes.FABRIC_PARTITION_KEY_TYPE getPartitionKeyType();

            [PreserveSig]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            IntPtr getPartitionKey();
        }

        [ComImport]
        [Guid("496b7bbc-b6d3-4990-adf4-d6e06eedfd7d")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricReliableInboundSessionCallback
        {
            // response is true if accepted or false if rejected
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            int AcceptInboundSession(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricReliableSessionPartitionIdentity partitionId,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricReliableSession session);
        }

        [ComImport]
        [Guid("978932f4-c93f-45ac-a804-d0fb4b4f7193")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricReliableSessionAbortedCallback
        {
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            void SessionAbortedByPartner(
                [In] BOOLEAN isInbound,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricReliableSessionPartitionIdentity partitionId,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricReliableSession session);
        }


        [ComImport]
        [Guid("90f13016-7c1e-48f4-a7db-479c22f87252")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricReliableSessionManager
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginOpen(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricReliableSessionAbortedCallback sessionCallback,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            void EndOpen(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            void EndClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginCreateOutboundSession(
                [In] IntPtr targetName,
                [In] NativeTypes.FABRIC_PARTITION_KEY_TYPE partitionKeyType,
                [In] IntPtr partitionKey,
                [In] IntPtr targetCommunicationEndpoint,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            IFabricReliableSession EndCreateOutboundSession(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginRegisterInboundSessionCallback(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricReliableInboundSessionCallback sessionCallback,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricStringResult EndRegisterInboundSessionCallback(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginUnregisterInboundSessionCallback(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            void EndUnregisterInboundSessionCallback(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }


        //----------------------------------------------------------------------
        // Entry Point
        //----------------------------------------------------------------------
        //
#if DotNetCoreClrLinux
        [DllImport("libFabricReliableMessaging.so", PreserveSig = false)]
#else
        [DllImport("FabricReliableMessaging.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        public static extern IFabricReliableSessionManager CreateReliableSessionManager(
            [In] IntPtr hostServicePartitionInfo,
            [In] IntPtr name,
            [In] NativeTypes.FABRIC_PARTITION_KEY_TYPE partitionKeyType,
            [In] IntPtr partitionKey);
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport.V2
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Interop;
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;

    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1201:ElementsMustAppearInTheCorrectOrder",
        Justification = "Matches order in IDL.")]
    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121:UseBuiltInTypeAlias",
        Justification = "It is important here to explicitly state the size of integer parameters.")]
    internal static class NativeFabricTransport
    {
        // ------------------------------------------------------------------------
        // Fabric Transport Structures
        // ------------------------------------------------------------------------
        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct FABRIC_TRANSPORT_SETTINGS
        {
            public UInt32 OperationTimeoutInSeconds;
            public UInt32 KeepAliveTimeoutInSeconds;
            public UInt32 MaxMessageSize;
            public UInt32 MaxConcurrentCalls;
            public UInt32 MaxQueueSize;
            public IntPtr SecurityCredentials;
            public IntPtr Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct FABRIC_TRANSPORT_LISTEN_ADDRESS
        {
            public IntPtr IPAddressOrFQDN;
            public UInt32 Port;
            public IntPtr Path;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        internal struct FABRIC_TRANSPORT_MESSAGE_BUFFER
        {
            public UInt32 BufferSize;
            public IntPtr Buffer;
        }

        // Entry Points 
#if DotNetCoreClrLinux
        [DllImport("libFabricTransport.so", PreserveSig = false)]
#else
        [DllImport("FabricTransport.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricTransportListener CreateFabricTransportListener(
            ref Guid iid,
            [In] IntPtr transportSettingsPtr,
            [In] IntPtr listenerAddressPtr,
            [In, MarshalAs(UnmanagedType.Interface)] IFabricTransportMessageHandler messageHandler,
            [In, MarshalAs(UnmanagedType.Interface)] IFabricTransportConnectionHandler clientConnectionHandler,
            [In, MarshalAs(UnmanagedType.Interface)] IFabricTransportMessageDisposer messageMessageDisposer
            );

#if DotNetCoreClrLinux
        [DllImport("libFabricTransport.so", PreserveSig = false)]
#else
        [DllImport("FabricTransport.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricTransportClient CreateFabricTransportClient(
            ref Guid iid,
            [In] IntPtr transportSettingsPtr,
            [In] IntPtr connectionAddress,
            [In, MarshalAs(UnmanagedType.Interface)] IFabricTransportCallbackMessageHandler callbackMessageHandler,
            [In, MarshalAs(UnmanagedType.Interface)] IFabricTransportClientEventHandler connectionEventHandler,
            [In, MarshalAs(UnmanagedType.Interface)] IFabricTransportMessageDisposer messageMessageDisposer
            );


        [ComImport]
        [Guid("b4357dab-ef06-465f-b453-938f3b0ad4b5")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricTransportMessage
        {
            [PreserveSig]
            void GetHeaderAndBodyBuffer(
           [Out] out IntPtr HeaderPtr,
           [Out] out UInt32 bufferlength,
           [Out] out IntPtr bufferPtr);

            [PreserveSig]
            void Dispose();
        }

        [ComImport]
        [Guid("914097f3-a821-46ea-b3d9-feafe5f7c4a9")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricTransportMessageDisposer
        {
            [PreserveSig]
            void Dispose(
                [In] UInt32 count,
                [In] IntPtr messages);
            
        }

        //// ----------------------------------------------------------------------------
        //// Interfaces
        ///     
        /// 
        [ComImport]
        [Guid("6815bdb4-1479-4c44-8b9d-57d6d0cc9d64")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricTransportMessageHandler
        {
            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginProcessRequest(
                [In] IntPtr clientId,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransportMessage message,
                [In] uint timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IFabricTransportMessage EndProcessRequest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            void HandleOneWay(
                [In] IntPtr clientId,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransportMessage message);
        }


        [ComImport]
        [Guid("1b63a266-1eeb-4f3e-8886-521458980d10")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricTransportListener
        {
            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginOpen(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricStringResult EndOpen(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            void EndClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [PreserveSig]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            void Abort();
        }


        [ComImport]
        [Guid("5b0634fe-6a52-4bd9-8059-892c72c1d73a")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricTransportClient
        {
            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginRequest(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransportMessage message,
                [In] uint timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IFabricTransportMessage EndRequest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            void Send(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransportMessage message);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginOpen(
                [In] uint timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);


#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            void EndOpen(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginClose(
                [In] uint timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);


#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            void EndClose(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [PreserveSig]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            void Abort();
        }

        [ComImport]
        [Guid("9ba8ac7a-3464-4774-b9b9-1d7f0f1920ba")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricTransportCallbackMessageHandler
        {
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            void HandleOneWay(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransportMessage message);
        }

        [ComImport]
        [Guid("a54c17f7-fe94-4838-b14d-e9b5c258e2d0")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricTransportClientConnection
        {
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            void Send(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransportMessage message);

            [PreserveSig]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IntPtr get_ClientId();
        }

        [ComImport]
        [Guid("b069692d-e8f0-4f25-a3b6-b2992598a64c")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricTransportConnectionHandler
        {
            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginProcessConnect(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricTransportClientConnection clientConnection,
                [In] uint timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);


#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            void EndProcessConnect(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginProcessDisconnect(
                [In] IntPtr clientId,
                [In] uint timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            void EndProcessDisconnect(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("4935ab6f-a8bc-4b10-a69e-7a3ba3324892")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricTransportClientEventHandler
        {
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            void OnConnected(
                [In] IntPtr connectionAddress);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            void OnDisconnected(
                [In] IntPtr connectionAddress,
                [In] int errorCode);
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Interop;
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;
    using FABRIC_PARTITION_ID = System.Guid;

    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1201:ElementsMustAppearInTheCorrectOrder",
        Justification = "Matches order in IDL.")]
    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121:UseBuiltInTypeAlias",
        Justification = "It is important here to explicitly state the size of integer parameters.")]
    internal static class NativeServiceCommunication
    {
        // Entry Points 
#if DotNetCoreClrLinux
        [DllImport("libFabricServiceCommunication.so", PreserveSig = false)]
#else
        [DllImport("FabricServiceCommunication.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricServiceCommunicationListener CreateServiceCommunicationListener(
            ref FABRIC_PARTITION_ID iid,
            [In] IntPtr transportSettingsPtr,
            [In] IntPtr listenerAddressPtr,
            [In, MarshalAs(UnmanagedType.Interface)] IFabricCommunicationMessageHandler messageHandler,
            [In, MarshalAs(UnmanagedType.Interface)] IFabricServiceConnectionHandler clientConnectionHandler);

#if DotNetCoreClrLinux
        [DllImport("libFabricServiceCommunication.so", PreserveSig = false)]
#else
        [DllImport("FabricServiceCommunication.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricServiceCommunicationClient CreateServiceCommunicationClient(
            ref Guid iid,
            [In] IntPtr transportSettingsPtr,
            [In] IntPtr connectionAddress,
            [In] IFabricCommunicationMessageHandler notificationHandler,
            [In] IFabricServiceConnectionEventHandler connectionEventHandler);

        //// ----------------------------------------------------------------------------
        //// Interfaces
        ///     
        [ComImport]
        [Guid("7e010010-80b2-453c-aab3-a73f0790dfac")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricCommunicationMessageHandler
        {
            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginProcessRequest(
                [In] IntPtr clientId,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricServiceCommunicationMessage message,
                [In] uint timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IFabricServiceCommunicationMessage EndProcessRequest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            void HandleOneWay(
                [In] IntPtr clientId,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricServiceCommunicationMessage message);
        }

        [ComImport]
        [Guid("fdf2bcd7-14f9-463f-9b70-ae3b5ff9d83f")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricCommunicationMessageSender
        {
            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginRequest(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricServiceCommunicationMessage message,
                [In] uint timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IFabricServiceCommunicationMessage EndRequest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            void SendMessage(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricServiceCommunicationMessage message);
        }

        [ComImport]
        [Guid("60ae1ab3-5f00-404d-8f89-96485c8b013e")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricClientConnection : IFabricCommunicationMessageSender
        {
            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginRequest(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricServiceCommunicationMessage message,
                [In] uint timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricServiceCommunicationMessage EndRequest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new void SendMessage(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricServiceCommunicationMessage message);

            [PreserveSig]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IntPtr get_ClientId();
        }

        [ComImport]
        [Guid("b069692d-e8f0-4f25-a3b6-b2992598a64c")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricServiceConnectionHandler
        {
            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginProcessConnect(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricClientConnection clientConnection,
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
        [Guid("dc6e168a-dbd4-4ce1-a3dc-5f33494f4972")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricServiceCommunicationMessage
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
                Justification = "Matches native API.")]
            [PreserveSig]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IntPtr Get_Body();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter",
                Justification = "Matches native API.")]
            [PreserveSig]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IntPtr Get_Headers();
        }

        [ComImport]
        [Guid("ad5d9f82-d62c-4819-9938-668540248e97")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricServiceCommunicationListener
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
        [Guid("255ecbe8-96b8-4f47-9e2c-1235dba3220a")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricServiceCommunicationClient : IFabricCommunicationMessageSender
        {
            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginRequest(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricServiceCommunicationMessage message,
                [In] uint timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricServiceCommunicationMessage EndRequest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new void SendMessage(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricServiceCommunicationMessage message);
        }

        [ComImport]
        [Guid("73b2cac5-4278-475b-82e6-1e33ebe20767")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricServiceCommunicationClient2 : IFabricServiceCommunicationClient
        {
            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginRequest(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricServiceCommunicationMessage message,
                [In] uint timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricServiceCommunicationMessage EndRequest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new void SendMessage(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricServiceCommunicationMessage message);

            [PreserveSig]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            void Abort();
        }

        [ComImport]
        [Guid("77f434b1-f9e9-4cb1-b0c4-c7ea2984aa8d")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricServiceConnectionEventHandler
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